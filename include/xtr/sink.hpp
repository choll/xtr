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

#include "config.hpp"
#include "detail/align.hpp"
#include "detail/is_c_string.hpp"
#include "detail/buffer.hpp"
#include "detail/print.hpp"
#include "detail/string_table.hpp"
#include "detail/synchronized_ring_buffer.hpp"
#include "detail/trampolines.hpp"
#include "log_level.hpp"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
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
 * to a logger may be created by calling @ref logger::get_sink. A sink
 * that is not connected to a logger may be created simply by default
 * construction, then the sink may be connected to a logger by calling
 * @ref logger::register_sink.
 */
class xtr::sink
{
private:
    // Function pointer type for performing type erasure. Function pointers
    // of this type are written to the sinks ring buffer and point to either
    // trampoline0, trampolineN or trampolineS (see detail/trampolines.hpp)
    using fptr_t =
        std::byte* (*)(
            detail::buffer& buf, // output buffer
            std::byte* record, // pointer to log record
            detail::consumer&,
            const char* timestamp,
            std::string& name) noexcept;

public:
    explicit sink(log_level_t level = log_level_t::info);

    /**
     * Sink copy constructor. When a sink is copied it is automatically
     * registered with the same logger object as the source sink, using
     * the same sink name. The sink name may be modified by calling @ref
     * set_name.
     */
    sink(const sink& other);

    /**
     * Sink copy assignment operator. When a sink is copy assigned it
     * is closed in order to disconnect it from any existing logger object
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
     *  re-opened by calling @ref logger::register_sink.
     */
    void close();

    /**
     * Returns true if the sink is open (connected to a logger), or false if
     * the sink is closed (not connected to a logger). Log messages may only
     * be written to a sink that is open.
     */
    bool is_open() const noexcept;

    /**
     *  Synchronizes all log calls previously made by this sink with the
     *  background thread and syncs all data to back-end storage.
     *
     *  @post All entries in the sink's queue have been processed by the
     *        background thread, buffers have been flushed and the sync()
     *        function on the storage interface has been called. For the
     *        default (disk) storage this means fsync(2) (if available) has
     *        been called.
     */
    void sync();

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
    template<auto Format, auto Level, typename Tags = void(), typename... Args>
    void log(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    /**
     *  Sets the log level of the sink to the specified level (see @ref log_level_t).
     *  Any log statement made with a log level with lower importance than the
     *  current level will be dropped\---please see the <a href="guide.html#log-levels">
     *  log levels</a> section of the user guide for details.
     */
    void set_level(log_level_t level)
    {
        level_.store(level, std::memory_order_relaxed);
    }

    /**
     *  Returns the current log level (see @ref log_level_t).
     */
    log_level_t level() const
    {
        return level_.load(std::memory_order_relaxed);
    }

    /**
     * Returns the capacity (in bytes) of the queue that the sink uses to send
     * log data to the background thread. To override the sink capacity set
     * @ref XTR_SINK_CAPACITY in xtr/config.hpp.
     */
    std::size_t capacity() const
    {
        return buf_.capacity();
    }

private:
    sink(logger& owner, std::string name, log_level_t level);

    template<auto Format, auto Level, typename Tags = void()>
    void log_impl() noexcept;

    template<auto Format, auto Level, typename Tags, typename... Args>
    void log_impl(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    template<typename T>
    void copy(std::byte* pos, T&& value)
        noexcept(XTR_NOTHROW_INGESTIBLE(T, value));

    template<
        auto Format = nullptr,
        auto Level = 0,
        typename Tags = void(),
        typename Func>
    void post(Func&& func) noexcept(XTR_NOTHROW_INGESTIBLE(Func, func));

    template<auto Format, auto Level, typename Tags, typename... Args>
    void post_variable_len(Args&&... args)
        noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    template<typename Func>
    void sync_post(Func&& func);

    template<typename Tags, typename... Args>
    auto make_lambda(Args&&... args)
        noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    using ring_buffer = detail::synchronized_ring_buffer<XTR_SINK_CAPACITY>;

    static_assert(
        XTR_SINK_CAPACITY <=
        std::numeric_limits<decltype(detail::string_table_entry::size)>::max(),
        "XTR_SINK_CAPACITY is too large");

    ring_buffer buf_;
    std::atomic<log_level_t> level_;
    bool open_ = false;

    friend detail::consumer;
    friend logger;
};

template<auto Format, auto Level, typename Tags, typename... Args>
void xtr::sink::log(Args&&... args)
    noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    log_impl<Format, Level, Tags>(std::forward<Args>(args)...);
}

template<auto Format, auto Level, typename Tags>
void xtr::sink::log_impl() noexcept
{
    // This function is just an optimisation; if the log line has no arguments
    // then creating a lambda for it would waste space in the queue (as even
    // if the lambda captures nothing it still has a non-zero size).
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, Level, detail::consumer>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, auto Level, typename Tags, typename... Args>
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
        post_variable_len<Format, Level, Tags>(std::forward<Args>(args)...);
    else
        post<Format, Level, Tags>(make_lambda<Tags>(std::forward<Args>(args)...));
}

template<auto Format, auto Level, typename Tags, typename... Args>
void xtr::sink::post_variable_len(Args&&... args)
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

    auto func_pos = s.begin() + sizeof(fptr_t);
    if constexpr (alignof(lambda_t) > alignof(fptr_t))
        func_pos = align<alignof(lambda_t)>(func_pos);

    static_assert(alignof(char) == 1);
    const auto str_pos = func_pos + sizeof(lambda_t);
    const auto size = ring_buffer::size_type(str_pos - s.begin());

    if (s.size() < size) [[unlikely]]
        s = buf_.write_span<Tags>(size);

    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;

    // str_cur and str_end are mutated by build_string_table as the
    // table is built
    auto str_cur = str_pos;
    auto str_end = s.end();

    copy(s.begin(), &detail::trampolineV<Format, Level, detail::consumer, lambda_t>);
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
    ::new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, auto Level, typename Tags, typename Func>
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

    if ((s.size() < size)) [[unlikely]]
        s = buf_.write_span<Tags>(size);

    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;

    copy(s.begin(), &detail::trampolineN<Format, Level, detail::consumer, Func>);
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
        [... args = std::forward<Args>(args)]<typename Format>(
            detail::buffer& buf,
            std::byte*& record,
            const Format& fmt,
            log_level_t level,
            [[maybe_unused]] const char* ts,
            const std::string& name) mutable noexcept
        {
            // args are passed by reference because although they were
            // forwarded into the lambda, they were still captured by copy,
            // so there is no point in moving them out of the lambda.
            if constexpr (detail::is_timestamp_v<Tags>)
            {
                detail::print_ts(
                    buf,
                    fmt,
                    level,
                    name,
                    detail::transform_string_table_entry(record, args)...);
            }
            else
            {
                detail::print(
                    buf,
                    fmt,
                    level,
                    ts,
                    name,
                    detail::transform_string_table_entry(record, args)...);
            }
        };
}

#endif
