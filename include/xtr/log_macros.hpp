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

#ifndef XTR_LOG_MACROS_HPP
#define XTR_LOG_MACROS_HPP

#include "detail/get_time.hpp"
#include "detail/string.hpp"
#include "detail/tags.hpp"
#include "detail/tsc.hpp"

#if defined(XTR_NDEBUG)
#undef XTR_NDEBUG
#define XTR_NDEBUG 1
#else
#define XTR_NDEBUG 0
#endif

// __extension__ is to silence the gnu-zero-variadic-macro-arguments warning in
// Clang, e.g. if user code does XTR_LOG(s, "fmt") then if XTR_LOG is declared
// as XTR_LOG(SINK, FMT, ...) then Clang will warn that nothing is passed to the
// variadic parameter. As the warning is generated in user code, enclosing the
// macro definitions in #pragma directives to ignore the warning achieves
// nothing.

/**
 * Basic log macro, logs the specified format string and arguments to the given
 * sink, blocking if the sink is full. Timestamps are read in the background
 * thread\---if this is undesirable use @ref XTR_LOG_RTC or @ref XTR_LOG_TSC
 * which read timestamps at the point of logging. This macro will log
 * regardless of the sink's log level.
 */
#define XTR_LOG(SINK, ...) XTR_LOG_TAGS(void(), info, SINK, __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL(LEVEL, SINK, ...) \
    XTR_LOGL_TAGS(void(), LEVEL, SINK, __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG(SINK, ...) \
    XTR_LOG_TAGS(xtr::non_blocking_tag, info, SINK, __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOGL. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL(LEVEL, SINK, ...) \
    XTR_LOGL_TAGS(xtr::non_blocking_tag, LEVEL, SINK, __VA_ARGS__)

/**
 * User-supplied timestamp log macro, logs the specified format string and
 * arguments to the given sink along with the specified timestamp, blocking if
 * the sink is full. The timestamp may be any type as long as it has a
 * formatter defined\---please see the <a href="guide.html#custom-formatters">
 * custom formatters</a> section of the user guide for details.
 * xtr::timespec is provided as a convenience type which is compatible with std::timespec and has a
 * formatter pre-defined. A formatter for std::timespec isn't defined in
 * order to avoid conflict with user code that also defines such a formatter.
 * This macro will log regardless of the sink's log level.
 *
 * @arg TS: The timestamp to apply to the log statement.
 */
#define XTR_LOG_TS(SINK, TS, ...) \
    (__extension__({ XTR_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_LOG_TS_IMPL(SINK, TS, FMT, ...) \
    XTR_LOG_TAGS(xtr::timestamp_tag, info, SINK, FMT, TS __VA_OPT__(,) __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_TS. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 * @arg TS: The timestamp to apply to the log statement.
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_TS(LEVEL, SINK, TS, ...) \
    (__extension__({ XTR_LOGL_TS_IMPL(LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_LOGL_TS_IMPL(LEVEL, SINK, TS, FMT, ...) \
    XTR_LOGL_TAGS(                                  \
        xtr::timestamp_tag,                         \
        LEVEL,                                      \
        SINK,                                       \
        FMT,                                        \
        (TS)                                        \
        __VA_OPT__(,) __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_TS. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_TS(SINK, TS, ...) \
    (__extension__({  XTR_TRY_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOG_TS_IMPL(SINK, TS, FMT, ...)         \
    XTR_LOG_TAGS(                                       \
        (xtr::non_blocking_tag, xtr::timestamp_tag),    \
        info,                                           \
        SINK,                                           \
        FMT,                                            \
        TS                                              \
        __VA_OPT__(,) __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_TS. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 * @arg TS: The timestamp to apply to the log statement.
 */
#define XTR_TRY_LOGL_TS(LEVEL, SINK, TS, ...) \
    (__extension__({  XTR_TRY_LOGL_TS_IMPL(LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOGL_TS_IMPL(LEVEL, SINK, TS, FMT, ...) \
    XTR_LOGL_TAGS(                                      \
        (xtr::non_blocking_tag, xtr::timestamp_tag),    \
        LEVEL,                                          \
        SINK,                                           \
        FMT,                                            \
        TS                                              \
        __VA_OPT__(,) __VA_ARGS__)

/**
 * Timestamped log macro, logs the specified format string and arguments to
 * the given sink along with a timestamp obtained by invoking
 * <a href="https://www.man7.org/linux/man-pages/man3/clock_gettime.3.html">clock_gettime(3)</a>
 * with a clock source of CLOCK_REALTIME_COARSE on Linux or CLOCK_REALTIME_FAST
 * on FreeBSD. Depending on the host CPU this may be faster than @ref
 * XTR_LOG_TSC. The non-blocking variant of this macro is @ref XTR_TRY_LOG_RTC
 * which will discard the message if the sink is full. This macro will log
 * regardless of the sink's log level.
 */
#define XTR_LOG_RTC(SINK, ...) \
    XTR_LOG_TS(SINK, xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_RTC. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_RTC(LEVEL, SINK, ...)                      \
    XTR_LOGL_TS(                                            \
        LEVEL,                                              \
        SINK,                                               \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(),   \
        __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_RTC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_RTC(SINK, ...) \
    XTR_TRY_LOG_TS(SINK, xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_RTC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL_RTC(LEVEL, SINK, ...)                  \
    XTR_TRY_LOGL_TS(                                        \
        LEVEL,                                              \
        SINK,                                               \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(),   \
        __VA_ARGS__)

/**
 * Timestamped log macro, logs the specified format string and arguments to
 * the given sink along with a timestamp obtained by reading the CPU timestamp
 * counter via the RDTSC instruction. The non-blocking variant of this macro is
 * @ref XTR_TRY_LOG_TSC which will discard the message if the sink is full. This
 * macro will log regardless of the sink's log level.
 */
#define XTR_LOG_TSC(SINK, ...) \
    XTR_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_TSC. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_TSC(LEVEL, SINK, ...) \
    XTR_LOGL_TS(LEVEL, SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_TSC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_TSC(SINK, ...) \
    XTR_TRY_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_TSC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL_TSC(LEVEL, SINK, ...)  \
    XTR_TRY_LOGL_TS(                        \
        LEVEL,                              \
        SINK,                               \
        xtr::detail::tsc::now(),            \
        __VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

#define XTR_LOGL_TAGS(TAGS, LEVEL, SINK, ...)                                       \
    (__extension__                                                                  \
        ({                                                                          \
            if constexpr (                                                          \
                xtr::log_level_t::LEVEL != xtr::log_level_t::debug ||               \
                !XTR_NDEBUG)                                                        \
            {                                                                       \
                if ((SINK).level() >= xtr::log_level_t::LEVEL)                      \
                    XTR_LOG_TAGS(TAGS, LEVEL, SINK, __VA_ARGS__);                   \
                if constexpr (xtr::log_level_t::LEVEL == xtr::log_level_t::fatal)   \
                {                                                                   \
                    (SINK).sync();                                                  \
                    std::abort();                                                   \
                }                                                                   \
            }                                                                       \
        }))

#define XTR_LOG_TAGS(TAGS, LEVEL, SINK, ...) \
    (__extension__({ XTR_LOG_TAGS_IMPL(TAGS, LEVEL, SINK, __VA_ARGS__); }))

// '{}{} {} ' in the format string is for the level, timestamp and sink name
#define XTR_LOG_TAGS_IMPL(TAGS, LEVEL, SINK, FORMAT, ...)                           \
    (__extension__                                                                  \
        ({                                                                          \
            static constexpr auto xtr_fmt =                                         \
                xtr::detail::string{"{}{} {} "} +                                   \
                xtr::detail::rcut<                                                  \
                    xtr::detail::rindex(__FILE__, '/') + 1>(__FILE__) +             \
                xtr::detail::string{":"} +                                          \
                xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};           \
            using xtr::nocopy;                                                      \
            (SINK).log<&xtr_fmt, xtr::log_level_t::LEVEL, void(TAGS)>(__VA_ARGS__); \
        }))

#endif
