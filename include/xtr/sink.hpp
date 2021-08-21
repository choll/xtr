// Copyright 2021 Chris E. Holloway
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

#ifndef XTR_SINK_HPP
#define XTR_SINK_HPP

#include "detail/pause.hpp"
#include "detail/print.hpp"
#include "detail/string_table.hpp"
#include "detail/synchronized_ring_buffer.hpp"
#include "detail/tags.hpp"
#include "detail/trampolines.hpp"
#include "log_level.hpp"

#include <atomic>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace xtr
{
    class sink;
    class logger;

    namespace detail
    {
        class consumer;
    }
}

// Returns true if the given value is nothrow `ingestible', i.e. the value can
// be nothrow copied/moved into the logger because either:
// * The value is nothrow-copyable or nothrow-movable (dependent upon the
//   value being passed by value/reference/r-value reference), or
// * The value is a std::string, which is nothrow ingestible because the string
//   contents are directly copied into the log buffer.
#define XTR_NOTHROW_INGESTIBLE(TYPE, VALUE)                     \
    (noexcept(std::decay_t<TYPE>{std::forward<TYPE>(VALUE)}) || \
    std::is_same_v<std::remove_cvref_t<TYPE>, std::string>)

/**
 * Log sink class. A sink is how log messages are written to a log.
 * Each sink has its own queue which is used to send log messages
 * to the logger. Sink operations are not thread safe, with the
 * exception of @ref set_level and @ref level.
 *
 * It is expected that an application will have many sinks, such
 * as a sink per thread or sink per component. A sink that is connected
 * to a logger may be created by calling @ref get_sink. A sink
 * that is not connected to a logger may be created simply by default
 * construction, then the sink may be connected to a logger by calling
 * @ref register_sink.
 */
class xtr::sink
{
private:
    using fptr_t =
        std::byte* (*)(
            fmt::memory_buffer& mbuf,
            std::byte* buf, // pointer to log record
            detail::consumer&,
            const char* timestamp,
            std::string& name) noexcept;

public:
    sink() = default;

    /**
     * Sink copy constructor. When a sink is copied it is automatically
     * registered with the same logger object as the source sink, using
     * the same sink name. The sink name may be modified by calling @ref
     * set_name.
     */
    sink(const sink& other);

    /**
     * Sink copy assignment operator. When a sink is copy assigned it
     * closed in order to disconnect it from any existing logger object,
     * and is then automatically registered with the same logger object as
     * the source sink, using the same sink name. The sink name may be
     * modified by calling @ref set_name.
     */
    sink& operator=(const sink& other);

    /**
     * Sink destructor. When a sink is destructed it is automatically
     * closed.
     */
    ~sink();

    /**
     *  Closes the sink. After this function returns the sink is closed and
     *  log() functions may not be called on the sink. The sink may be
     *  re-opened by calling logger::register_sink.
     */
    void close();

    /**
     *  Synchronizes all log calls previously made by this sink to back-end
     *  storage.
     *
     *  @post All entries in the sink's queue have been delivered to the
     *        back-end, and the flush() and sync() functions associated
     *        with the back-end have been called. For the default (disk)
     *        back-end this means fflush(3) and fsync(2) (if available)
     *        have been called.
     */
    void sync()
    {
        sync(/*destroy=*/false);
    }

    /**
     *  Sets the sink's name to the specified value.
     */
    void set_name(std::string name);

    /**
     *  Logs the given format string and arguments. This function is not
     *  intended to be used directly, instead one of the XTR_LOG macros
     *  should be used. It is provided for use in situations where use of
     *  a macro may be undesirable.
     */
    template<auto Format, typename Tags = void(), typename... Args>
    void log(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    /**
     *  Sets the log level of the sink to the specified level.
     */
    void set_level(log_level_t l)
    {
        level_.store(l, std::memory_order_relaxed);
    }

    /**
     *  Returns the current log level.
     */
    log_level_t level() const
    {
        return level_.load(std::memory_order_relaxed);
    }

private:
    sink(logger& owner, std::string name);

    template<auto Format, typename Tags = void()>
    void log_impl() noexcept;

    template<auto Format, typename Tags, typename... Args>
    void log_impl(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

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

    using ring_buffer = detail::synchronized_ring_buffer<64 * 1024>;

    ring_buffer buf_;
    std::atomic<log_level_t> level_{log_level_t::info};
    bool open_ = false;

    friend detail::consumer;
    friend logger;
};

template<auto Format, typename Tags, typename... Args>
void xtr::sink::log(Args&&... args)
    noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    log_impl<Format, Tags>(std::forward<Args>(args)...);
}

template<auto Format, typename Tags>
void xtr::sink::log_impl() noexcept
{
    // This function is just an optimisation; if the log line has no arguments
    // then creating a lambda for it would waste space in the queue (as even
    // if the lambda captures nothing it still has a non-zero size).
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, detail::consumer>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, typename Tags, typename... Args>
void xtr::sink::log_impl(Args&&... args)
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
void xtr::sink::post_with_str_table(Args&&... args)
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

    copy(s.begin(), &detail::trampolineS<Format, detail::consumer, lambda_t>);
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
void xtr::sink::copy(std::byte* pos, T&& value)
    noexcept(XTR_NOTHROW_INGESTIBLE(T, value))
{
    assert(std::uintptr_t(pos) % alignof(T) == 0);
#if defined(__cpp_lib_assume_aligned)
    pos = static_cast<std::byte*>(std::assume_aligned<alignof(T)>(pos));
#else
    // This can be removed when libc++ supports assume_aligned
    pos = static_cast<std::byte*>(__builtin_assume_aligned(pos, alignof(T)));
#endif
    new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, typename Tags, typename Func>
void xtr::sink::post(Func&& func)
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

    copy(s.begin(), &detail::trampolineN<Format, detail::consumer, Func>);
    copy(func_pos, std::forward<Func>(func));

    buf_.reduce_writable(size);
}

template<typename Tags, typename... Args>
auto xtr::sink::make_lambda(Args&&... args)
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
