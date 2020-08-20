// Copyright 2019, 2020 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef XTR_LOGGER_HPP
#define XTR_LOGGER_HPP

#include "detail/align.hpp"
#include "detail/assume.hpp"
#include "detail/clock_ids.hpp"
#include "detail/is_c_string.hpp"
#include "detail/pause.hpp"
#include "detail/print.hpp"
#include "detail/string.hpp"
#include "detail/string_ref.hpp"
#include "detail/string_table.hpp"
#include "detail/synchronized_ring_buffer.hpp"
#include "detail/get_time.hpp"
#include "detail/tags.hpp"
#include "detail/trampolines.hpp"
#include "detail/tsc.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

// FIXME C++20: [[likely]]
#define XTR_LIKELY(x)      __builtin_expect(!!(x), 1)
#define XTR_UNLIKELY(x)    __builtin_expect(!!(x), 0)

// __extension__ is to silence the gnu-zero-variadic-macro-arguments warning in clang

#define XTR_LOG(...)                            \
    (__extension__                              \
        ({                                      \
            XTR_LOG_TAGS(void(), __VA_ARGS__);  \
        }))

#define XTR_TRY_LOG(...)                                        \
    (__extension__                                              \
        ({                                                      \
            XTR_LOG_TAGS(xtr::non_blocking_tag, __VA_ARGS__);   \
        }))

#define XTR_LOG_TS(...)            \
    XTR_LOG_TAGS(                  \
        xtr::timestamp_tag         \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TS(...)         \
    XTR_LOG_TAGS(                   \
        (xtr::non_blocking_tag,     \
            xtr::timestamp_tag)     \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_LOG_RTC(SINK, FMT, ...)                     \
    XTR_LOG_TS(                                         \
        SINK,                                           \
        FMT,                                            \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()  \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_RTC(SINK, FMT, ...)                 \
    XTR_TRY_LOG_TS(                                     \
        SINK,                                           \
        FMT,                                            \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()  \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_LOG_TSC(SINK, FMT, ...) \
    XTR_LOG_TS(                     \
        SINK,                       \
        FMT,                        \
        xtr::detail::tsc::now()     \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TSC(SINK, FMT, ...) \
    XTR_TRY_LOG_TS(                     \
        SINK,                           \
        FMT,                            \
        xtr::detail::tsc::now()         \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

// '{}{}:' in the format string is for the timestamp and producer name
#define XTR_LOG_TAGS(TAGS, SINK, FORMAT, ...)                               \
    (__extension__                                                          \
        ({                                                                  \
            static constexpr auto xtr_fmt =                                 \
                xtr::detail::string{"{}: {}: "} +                           \
                xtr::detail::rcut<                                          \
                    xtr::detail::rindex(__FILE__, '/') + 1>(__FILE__) +     \
                xtr::detail::string{":"} +                                  \
                xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};   \
            using xtr::nocopy;                                              \
            (SINK).log<&xtr_fmt, void(TAGS)>(__VA_ARGS__);                  \
        }))

namespace xtr
{
    class logger;

    template<typename T>
    using nocopy = detail::string_ref<T>;
}

namespace xtr::detail
{
    inline auto make_output_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                return std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_error_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                (void)std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_flush_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
                std::fflush(stream);
                std::fflush(err_stream);
            };
    }

    inline auto make_sync_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
                ::fsync(::fileno(stream));
                ::fsync(::fileno(err_stream));
#endif
            };
    }
}

class xtr::logger
{
private:
    // XXX use of _t is not consistent, eg not here but there is fptr_t.
    // same for state_t, also you need to allow specifying the buffer
    // size
    using ring_buffer = detail::synchronized_ring_buffer<64 * 1024>;

    struct state;

    typedef std::byte* (*fptr_t)(
        fmt::memory_buffer&,
        std::byte*,
        state& st,
        const char* ts,
        const std::string& name) noexcept;

public:
    // XXX is sink a better name? log_sink? logger::sink?
    class producer
    {
    public:
        ~producer();

        void sync()
        {
            sync(false);
        }

        void set_name(std::string name);

        template<auto Format, typename Tags = void()>
        void log() noexcept;

        // FIXME: noexcept check is stricter than necessary
        template<auto Format, typename Tags = void(), typename... Args>
        void log(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

    private:
        producer() = default;

        producer(logger& owner, std::string name);

        template<auto Format, typename Tags, typename... Args>
        void log_str(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

        template<typename T>
        void copy(std::byte* pos, T&& value) noexcept; // XXX noexcept

        template<
            auto Format = nullptr,
            typename Tags = void(),
            typename Func>
        void post(Func&& func)
            noexcept(std::is_nothrow_move_constructible_v<Func>);

        template<typename Tags, typename... Args>
        auto make_lambda(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

        void sync(bool destructing);

        ring_buffer buf_;
        std::string name_;
        friend logger;
    };

private:
    struct state
    {
        std::function<::ssize_t(const char* buf, std::size_t size)> out;
        std::function<void(const char* buf, std::size_t size)> err;
        std::function<void()> flush;
        std::function<void()> sync;
        std::vector<producer*> producers;
        bool destroy;
    };

public:

    // XXX
    // const char* path option?
    // rotating files?
    //
    // Also perhaps have a single constructor that accepts
    // variadic args, then just check the type of them? eg
    // is_same<FILE*>, is_clock_v?

    template<typename Clock = std::chrono::system_clock>
    logger(
        FILE* stream = stderr,
        FILE* err_stream = stderr,
        Clock&& clock = Clock())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            std::forward<Clock>(clock))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Clock = std::chrono::system_clock,
        typename =
            std::enable_if_t<
                std::is_invocable_v<OutputFunction, const char*, std::size_t> &&
                std::is_invocable_v<ErrorFunction, const char*, std::size_t>>>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        Clock&& clock = Clock())
    :
        logger(
            std::forward<OutputFunction>(out),
            std::forward<ErrorFunction>(err),
            [](){}, // flush
            [](){}, // sync
            std::forward<Clock>(clock))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename FlushFunction,
        typename SyncFunction,
        typename Clock = std::chrono::system_clock,
        typename =
            std::enable_if_t<
                std::is_invocable_v<OutputFunction, const char*, std::size_t> &&
                std::is_invocable_v<ErrorFunction, const char*, std::size_t> &&
                std::is_invocable_v<FlushFunction> &&
                std::is_invocable_v<SyncFunction>>>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        FlushFunction&& flush,
        SyncFunction&& sync,
        Clock&& clock = Clock())
    {
        // The consumer thread must be started after control_ has been
        // constructed, but control_ must be placed after consumer_ so that it
        // is destructed before consumer_.
        consumer_ =
            std::jthread(
                &logger::consumer,
                this,
                state{
                    std::forward<OutputFunction>(out),
                    std::forward<ErrorFunction>(err),
                    std::forward<FlushFunction>(flush),
                    std::forward<SyncFunction>(sync),
                    {&control_},
                    false},
                make_clock(std::forward<Clock>(clock)));
    }

    auto consumer_thread_native_handle()
    {
        return consumer_.native_handle();
    }

    // XXX Rename as create? people might assume that multiple get
    // calls return the same producer.
    [[nodiscard]] producer get_producer(std::string name);

    void register_producer(producer& p) noexcept;

    void set_output_stream(FILE* stream) noexcept;
    void set_error_stream(FILE* stream) noexcept;

    template<typename Func>
    void set_output_function(Func&& f) noexcept
    {
        static_assert(
            std::is_convertible_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                ::ssize_t>,
            "Output function type must be of type ssize_t(const char*, size_t) "
            "(returning the number of bytes written or -1 on error)");
        post([f_ = std::forward<Func>(f)](state& st) { st.out = std::move(f_); });
        control_.sync();
    }

    template<typename Func>
    void set_error_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                void>,
            "Error function must be of type void(const char*, size_t)");
        post([f_ = std::forward<Func>(f)](state& st) { st.err = std::move(f_); });
        control_.sync();
    }

    template<typename Func>
    void set_flush_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Flush function must be of type void()");
        post([f_ = std::forward<Func>(f)](state& st) { st.flush = std::move(f_); });
        control_.sync();
    }

    template<typename Func>
    void set_sync_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Sync function must be of type void()");
        post([f_ = std::forward<Func>(f)](state& st) { st.sync = std::move(f_); });
        control_.sync();
    }

private:
    template<typename Func>
    void post(Func&& f)
    {
        std::scoped_lock lock{control_mutex_};
        control_.post(std::forward<Func>(f));
    }

    template<typename Clock>
    std::function<std::timespec()> make_clock(Clock&& clock)
    {
        return
            [clock_{std::forward<Clock>(clock)}]() -> std::timespec
            {
                // Note: to_time_t would be useful here except it is unspecified
                // whether time_t rounds up or truncates if time_t has a lower
                // precision than the input time_point.
                using namespace std::chrono;
                const auto now = clock_.now();
                auto sec = time_point_cast<seconds>(now);
                if (sec > now)
                    sec - seconds{1};
                const auto nanos = duration_cast<nanoseconds>(now - sec);
                std::timespec ts;
                ts.tv_sec = sec.time_since_epoch().count();
                ts.tv_nsec = nanos.count();
                return ts; // C++20: Designated initializer (not in Clang yet)
            };
    }

    void consumer(state st, std::function<std::timespec()> clock) noexcept;

    std::jthread consumer_;
    producer control_;
    std::mutex control_mutex_;
};

// should std::string be copied? if you have Args as below then it will
// always be copied, unless a ref is passed?
//
// std::string s;
//
// log(s) copy
// log(std::move(s)) move
// log(std::ref(s)) reference
// log(s.c_str()) ingested
//
//
// Args&& and forward might be better as it will default to reference, but
// also allow moving? how can you detect if it is a ref or move though?
//
// easily: just apply a function that has && and const& overloads.
//
// if you call make_tuple then it is going to also perfectly
// forward? sort of, the types will be decayed for the tuple type, so it
// will copy/move as desired, no references unless std::reference_wrapper
// is present.

template<auto Format, typename Tags>
void xtr::logger::producer::log() noexcept
{
    // This function is just an optimisation; if the log line has no arguments
    // then creating a lambda for it would waste space in the queue (as even
    // if the lambda captures nothing it still has a non-zero size).
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, state>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::log(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    static_assert(sizeof...(Args) > 0);
    constexpr bool is_str =
        std::disjunction_v<
            detail::is_c_string<decltype(std::forward<Args>(args))>...,
            std::is_same<std::remove_cvref_t<Args>, std::string_view>...,
            std::is_same<std::remove_cvref_t<Args>, std::string>...>;
    if constexpr (is_str)
        log_str<Format, Tags>(std::forward<Args>(args)...);
    else
        post<Format, Tags>(make_lambda<Tags>(std::forward<Args>(args)...));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::log_str(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    using lambda_t =
        decltype(
            make_lambda<Tags>(
                detail::build_string_table<Tags>(
                    std::declval<std::byte*&>(),
                    std::declval<std::byte*&>(),
                    buf_,
                    std::forward<Args>(args))...));

    ring_buffer::span s = buf_.write_span_spec();

    static_assert(alignof(std::size_t) <= alignof(fptr_t));
    const auto size_pos = s.begin() + sizeof(fptr_t);

    auto func_pos = size_pos + sizeof(size_t);
    if constexpr (alignof(lambda_t) > alignof(size_t))
        func_pos = align<alignof(lambda_t)>(func_pos);

    static_assert(alignof(char) == 1);
    const auto str_pos = func_pos + sizeof(lambda_t);
    const auto size = ring_buffer::size_type(str_pos - s.begin());

    while (XTR_UNLIKELY(s.size() < size)) [[unlikely]]
    {
        if constexpr (!detail::is_non_blocking_v<Tags>)
            detail::pause();
        s = buf_.write_span<Tags>();
        if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
            return;
    }

    // str_cur and str_end are mutated by build_string_table as the
    // table is built
    auto str_cur = str_pos;
    auto str_end = s.end();

    copy(s.begin(), &detail::trampolineS<Format, state, lambda_t>);
    copy(
        func_pos,
        make_lambda<Tags>(
            detail::build_string_table<Tags>(
                str_cur,
                str_end,
                buf_,
                std::forward<Args>(args))...));

    const auto next = detail::align<alignof(fptr_t)>(str_cur);
    const auto total_size = ring_buffer::size_type(next - s.begin());

    copy(size_pos, total_size);
    buf_.reduce_writable(total_size);
}

template<typename T>
void xtr::logger::producer::copy(std::byte* pos, T&& value) noexcept
{
    assert(std::uintptr_t(pos) % alignof(T) == 0);
    // C++20: std::assume_aligned
    pos =
        static_cast<std::byte*>(
            __builtin_assume_aligned(pos, alignof(T)));
    // In gcc 7.4 and below placement new contains a null pointer check
    XTR_ASSUME(pos != nullptr);
    new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, typename Tags, typename Func>
void xtr::logger::producer::post(Func&& func)
    noexcept(std::is_nothrow_move_constructible_v<Func>)
{
    ring_buffer::span s = buf_.write_span_spec();

    // GCC as of 9.2.1 does not optimise away this call to align if pos
    // is marked as aligned, hence these constexpr conditionals. Clang
    // does optimise as of 8.0.1-3+b1.
    auto func_pos = s.begin() + sizeof(fptr_t);
    if constexpr (alignof(Func) > alignof(fptr_t))
        func_pos = detail::align<alignof(Func)>(func_pos);

    // We can calculate 'next' aligned to fptr_t in this way because we know
    // that func_pos has alignment that is at least alignof(fptr_t), so the
    // size of Func can simply be rounded up.
    const auto next = func_pos + detail::align(sizeof(Func), alignof(fptr_t));
    const auto size = ring_buffer::size_type(next - s.begin());

    while (XTR_UNLIKELY(s.size() < size)) [[unlikely]] // XXX UNLIKELY
    {
        if constexpr (!detail::is_non_blocking_v<Tags>)
            detail::pause();
        s = buf_.write_span<Tags>();
        if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
            return;
    }

    copy(s.begin(), &detail::trampolineN<Format, state, Func>);
    copy(func_pos, std::forward<Func>(func));

    buf_.reduce_writable(size);
}

template<typename Tags, typename... Args>
auto xtr::logger::producer::make_lambda(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    // This lambda is mutable so that std::forward works correctly, without it
    // there is a mismatch between Args and args, due to args becoming const
    // if the lambda is not mutable.
    return
        [... args = std::forward<Args>(args)](
            fmt::memory_buffer& mbuf,
            const auto& out,
            const auto& err,
            std::string_view fmt,
            [[maybe_unused]] const char* ts,
            const std::string& name) mutable noexcept
        {
            // args are passed by reference because although they were
            // forwarded into the lambda, they were still captured by copy,
            // so there is no point in moving them out of the lambda.
            if constexpr (detail::is_timestamp_v<Tags>)
            {
                xtr::detail::print_ts(
                    mbuf,
                    out,
                    err,
                    fmt,
                    name,
                    args...);
            }
            else
            {
                xtr::detail::print(
                    mbuf,
                    out,
                    err,
                    fmt,
                    ts,
                    name,
                    args...);
            }
        };
}

#endif

