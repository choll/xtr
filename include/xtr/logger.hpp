// Copyright 2019, 2020, 2021 Chris E. Holloway
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

#include "command_path.hpp"
#include "detail/align.hpp"
#include "detail/clock_ids.hpp"
#include "detail/commands/command_dispatcher_fwd.hpp"
#include "detail/commands/requests_fwd.hpp"
#include "detail/get_time.hpp"
#include "detail/is_c_string.hpp"
#include "detail/pause.hpp"
#include "detail/print.hpp"
#include "detail/string.hpp"
#include "detail/string_ref.hpp"
#include "detail/string_table.hpp"
#include "detail/synchronized_ring_buffer.hpp"
#include "detail/tags.hpp"
#include "detail/throw.hpp"
#include "detail/trampolines.hpp"
#include "detail/tsc.hpp"
#include "log_level.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <concepts>
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

// __extension__ is to silence the gnu-zero-variadic-macro-arguments warning in clang

#define XTR_LOG(...)                                \
    (__extension__                                  \
        ({                                          \
            XTR_LOG_TAGS(void(), "I", __VA_ARGS__); \
        }))

#define XTR_TRY_LOG(...)                                            \
    (__extension__                                                  \
        ({                                                          \
            XTR_LOG_TAGS(xtr::non_blocking_tag, "I", __VA_ARGS__); \
        }))

#define XTR_LOG_TS(...) XTR_LOG_TAGS(xtr::timestamp_tag, "I", __VA_ARGS__)

#define XTR_TRY_LOG_TS(...)         \
    XTR_LOG_TAGS((xtr::non_blocking_tag, xtr::timestamp_tag), "I", __VA_ARGS__)

#define XTR_LOG_RTC(SINK, FMT, ...)                         \
    XTR_LOG_TS(                                             \
        SINK,                                               \
        FMT,                                                \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()    \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_RTC(SINK, FMT, ...)                     \
    XTR_TRY_LOG_TS(                                         \
        SINK,                                               \
        FMT,                                                \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()    \
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

#define XTR_LOG_LEVEL(LEVELSTR, LEVEL, SINK, ...)                   \
    (__extension__                                                  \
        ({                                                          \
            if ((SINK).level() >= xtr::log_level_t::LEVEL)          \
                XTR_LOG_TAGS(void(), LEVELSTR, SINK, __VA_ARGS__);  \
        }))                                                         \

#define XTR_LOG_FATAL(SINK, ...)                            \
    (__extension__                                          \
        ({                                                  \
            XTR_LOG_LEVEL("F", fatal, SINK, __VA_ARGS__);   \
            (SINK).sync();                                  \
            std::abort();                                   \
        }))

#define XTR_LOG_ERROR(...) XTR_LOG_LEVEL("E", error, __VA_ARGS__)
#define XTR_LOG_WARN(...) XTR_LOG_LEVEL("W", warning, __VA_ARGS__)
#define XTR_LOG_INFO(...) XTR_LOG_LEVEL("I", info, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_LOG_DEBUG(...)
#else
#define XTR_LOG_DEBUG(...) XTR_LOG_LEVEL("D", debug, __VA_ARGS__)
#endif

#define XTR_LOGF(...) XTR_LOG_FATAL(__VA_ARGS__)
#define XTR_LOGE(...) XTR_LOG_ERROR(__VA_ARGS__)
#define XTR_LOGW(...) XTR_LOG_WARN(__VA_ARGS__)
#define XTR_LOGI(...) XTR_LOG_INFO(__VA_ARGS__)
#define XTR_LOGD(...) XTR_LOG_DEBUG(__VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

// '{}{}:' in the format string is for the timestamp and producer name
#define XTR_LOG_TAGS(TAGS, LEVELSTR, SINK, FORMAT, ...)                     \
    (__extension__                                                          \
        ({                                                                  \
            static constexpr auto xtr_fmt =                                 \
                xtr::detail::string{LEVELSTR " {} {} "} +                   \
                xtr::detail::rcut<                                          \
                    xtr::detail::rindex(__FILE__, '/') + 1>(__FILE__) +     \
                xtr::detail::string{":"} +                                  \
                xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};   \
            using xtr::nocopy;                                              \
            (SINK).log<&xtr_fmt, void(TAGS)>(__VA_ARGS__);                  \
        }))

// Returns true if the given value is nothrow `ingestible', i.e. the value can
// be nothrow copied/moved into the logger because either:
// * The value is nothrow-copyable or nothrow-movable (dependent upon the
//   value being passed by value/reference/r-value reference), or
// * The value is a std::string, which is nothrow ingestible because the string
//   contents are directly copied into the log buffer.
#define XTR_NOTHROW_INGESTIBLE(TYPE, VALUE)                     \
    (noexcept(std::decay_t<TYPE>{std::forward<TYPE>(VALUE)}) || \
    std::is_same_v<std::remove_cvref_t<TYPE>, std::string>)

namespace xtr
{
    class logger;

    // This can be replaced with a template alias once clang supports it:
    // template<typename T> using nocopy = detail::string_ref<T>;
    template<typename T>
    inline auto nocopy(const T& arg)
    {
        return detail::string_ref(arg);
    }
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

    inline auto make_reopen_func(std::string path, FILE* stream)
    {
        return
            [path = std::move(path), stream]()
            {
                return std::freopen(path.c_str(), "a", stream) != nullptr;
            };
    }

    inline FILE* open_path(const char* path)
    {
        FILE* const fp = std::fopen(path, "a");
        if (fp == nullptr)
            detail::throw_system_error_fmt("Failed to open `%s'", path);
        return fp;
    }
}

// TODO: Specify buffer size via template parameter, or as dynamic,
// dynamic_capacity will need to become part of the interface.
class xtr::logger
{
private:
    using ring_buffer = detail::synchronized_ring_buffer<64 * 1024>;

    class consumer;

    // XXX TODO: fptr_t is not a very helpful name
    using fptr_t =
        std::byte* (*)(
            fmt::memory_buffer& mbuf,
            std::byte* buf, // pointer to log record
            consumer&,
            const char* timestamp,
            std::string& name) noexcept;

public:
    // XXX is sink a better name? log_sink? logger::sink?
    class producer
    {
    public:
        producer() = default;

        producer(const producer& other);

        producer& operator=(const producer& other);

        ~producer();

        void close();

        void sync()
        {
            sync(/*destroy=*/false);
        }

        void set_name(std::string name);

        template<auto Format, typename Tags = void()>
        void log() noexcept;

        template<auto Format, typename Tags = void(), typename... Args>
        void log(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

        void set_level(log_level_t l)
        {
            level_.store(l, std::memory_order_relaxed);
        }

        log_level_t level() const
        {
            return level_.load(std::memory_order_relaxed);
        }

    private:
        producer(logger& owner, std::string name);

        template<typename T>
        void copy(std::byte* pos, T&& value)
            noexcept(XTR_NOTHROW_INGESTIBLE(T, value));

        template<
            auto Format = nullptr,
            typename Tags = void(),
            typename Func>
        void post(Func&& func) noexcept(XTR_NOTHROW_INGESTIBLE(Func, func));

        template<auto Format, typename Tags, typename... Args>
        void post_with_str_table(Args&&... args)
            noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

        template<typename Tags, typename... Args>
        auto make_lambda(Args&&... args)
            noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

        void sync(bool destruct);

        ring_buffer buf_;
        std::atomic<log_level_t> level_{log_level_t::info};
        bool open_ = false;
        friend logger;
    };

private:
    class consumer
    {
    private:
        struct producer_handle
        {
            producer* operator->()
            {
                return p;
            }

            producer* p;
            std::string name;
            std::size_t dropped_count = 0;
        };

    public:
        void run(std::function<::timespec()> clock) noexcept;
        void set_command_path(std::string path) noexcept;

        template<
            typename OutputFunction,
            typename ErrorFunction,
            typename FlushFunction,
            typename SyncFunction,
            typename ReopenFunction,
            typename CloseFunction>
        consumer(
            OutputFunction&& of,
            ErrorFunction&& ef,
            FlushFunction&& ff,
            SyncFunction&& sf,
            ReopenFunction&& rf,
            CloseFunction&& cf,
            producer* control)
        :
            out(std::forward<OutputFunction>(of)),
            err(std::forward<ErrorFunction>(ef)),
            flush(std::forward<FlushFunction>(ff)),
            sync(std::forward<SyncFunction>(sf)),
            reopen(std::forward<ReopenFunction>(rf)),
            close(std::forward<CloseFunction>(cf)),
            producers_({{control, "control", 0}})
        {
        }

        void add_producer(producer& p, const std::string& name);

        std::function<::ssize_t(const char* buf, std::size_t size)> out;
        std::function<void(const char* buf, std::size_t size)> err;
        std::function<void()> flush;
        std::function<void()> sync;
        std::function<bool()> reopen;
        std::function<void()> close;
        bool destroy = false;

    private:
        void status_handler(int fd, detail::status&);
        void set_level_handler(int fd, detail::set_level&);
        void reopen_handler(int fd, detail::reopen&);

        std::vector<producer_handle> producers_;
        std::unique_ptr<
            detail::command_dispatcher,
            detail::command_dispatcher_deleter> cmds_;
    };

public:
    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            path,
            detail::open_path(path),
            stderr,
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    template<typename Clock = std::chrono::system_clock>
    logger(
        const char* path,
        FILE* stream,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            detail::make_reopen_func(path, stream),
            [stream](){ std::fclose(stream); }, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    template<typename Clock = std::chrono::system_clock>
    logger(
        FILE* stream = stderr,
        FILE* err_stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Clock = std::chrono::system_clock>
    requires
        std::invocable<OutputFunction, const char*, std::size_t> &&
        std::invocable<ErrorFunction, const char*, std::size_t>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            std::forward<OutputFunction>(out),
            std::forward<ErrorFunction>(err),
            [](){}, // flush
            [](){}, // sync
            [](){ return true; }, // reopen
            [](){}, // close
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename FlushFunction,
        typename SyncFunction,
        typename ReopenFunction,
        typename CloseFunction,
        typename Clock = std::chrono::system_clock>
    requires
        std::invocable<OutputFunction, const char*, std::size_t> &&
        std::invocable<ErrorFunction, const char*, std::size_t> &&
        std::invocable<FlushFunction> &&
        std::invocable<SyncFunction> &&
        std::invocable<ReopenFunction> &&
        std::invocable<CloseFunction>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        FlushFunction&& flush,
        SyncFunction&& sync,
        ReopenFunction&& reopen,
        CloseFunction&& close,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    {
        // The consumer thread must be started after control_ has been
        // constructed
        consumer_ =
            std::jthread(
                &consumer::run,
                consumer(
                    std::forward<OutputFunction>(out),
                    std::forward<ErrorFunction>(err),
                    std::forward<FlushFunction>(flush),
                    std::forward<SyncFunction>(sync),
                    std::forward<ReopenFunction>(reopen),
                    std::forward<CloseFunction>(close),
                    &control_),
                make_clock(std::forward<Clock>(clock)));
        // Passing control_ to the consumer is equivalent to calling
        // register_producer, so mark it as open.
        control_.open_ = true;
        set_command_path(std::move(command_path));
    }

    ~logger();

    std::thread::native_handle_type consumer_thread_native_handle()
    {
        return consumer_.native_handle();
    }

    // XXX Rename as create? people might assume that multiple get
    // calls return the same producer.
    [[nodiscard]] producer get_producer(std::string name);

    void register_producer(producer& p, const std::string& name) noexcept;

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
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.out = std::move(f); });
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
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.err = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_flush_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Flush function must be of type void()");
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.flush = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_sync_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Sync function must be of type void()");
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.sync = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_reopen_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, bool>,
            "Reopen function must be of type bool()");
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.reopen = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_close_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Close function must be of type void()");
        post([f = std::forward<Func>(f)](consumer& c, auto&) { c.close = std::move(f); });
        control_.close();
    }

    void set_command_path(std::string path) noexcept;

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
                return std::timespec{
                    .tv_sec=sec.time_since_epoch().count(),
                    .tv_nsec=duration_cast<nanoseconds>(now - sec).count()};
            };
    }

    producer control_; // aligned to cache line so first to avoid extra padding
    std::jthread consumer_;
    std::mutex control_mutex_;
};

template<auto Format, typename Tags>
void xtr::logger::producer::log() noexcept
{
    // This function is just an optimisation; if the log line has no arguments
    // then creating a lambda for it would waste space in the queue (as even
    // if the lambda captures nothing it still has a non-zero size).
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, consumer>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::log(Args&&... args)
    noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    static_assert(sizeof...(Args) > 0);
    constexpr bool is_str =
        std::disjunction_v<
            detail::is_c_string<decltype(std::forward<Args>(args))>...,
            std::is_same<std::remove_cvref_t<Args>, std::string_view>...,
            std::is_same<std::remove_cvref_t<Args>, std::string>...>;
    if constexpr (is_str)
        post_with_str_table<Format, Tags>(std::forward<Args>(args)...);
    else
        post<Format, Tags>(make_lambda<Tags>(std::forward<Args>(args)...));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::post_with_str_table(Args&&... args)
    noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
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

    while (s.size() < size) [[unlikely]]
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

    copy(s.begin(), &detail::trampolineS<Format, consumer, lambda_t>);
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
void xtr::logger::producer::copy(std::byte* pos, T&& value)
    noexcept(XTR_NOTHROW_INGESTIBLE(T, value))
{
    assert(std::uintptr_t(pos) % alignof(T) == 0);
    pos = static_cast<std::byte*>(std::assume_aligned<alignof(T)>(pos));
    new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, typename Tags, typename Func>
void xtr::logger::producer::post(Func&& func)
    noexcept(XTR_NOTHROW_INGESTIBLE(Func, func))
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

    while ((s.size() < size)) [[unlikely]]
    {
        if constexpr (!detail::is_non_blocking_v<Tags>)
            detail::pause();
        s = buf_.write_span<Tags>();
        if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
            return;
    }

    copy(s.begin(), &detail::trampolineN<Format, consumer, Func>);
    copy(func_pos, std::forward<Func>(func));

    buf_.reduce_writable(size);
}

template<typename Tags, typename... Args>
auto xtr::logger::producer::make_lambda(Args&&... args)
    noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
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
