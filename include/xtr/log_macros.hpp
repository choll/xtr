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

// __extension__ is to silence the gnu-zero-variadic-macro-arguments warning in
// Clang, e.g. if user code does XTR_LOG(s, "fmt") then if XTR_LOG is declared
// as XTR_LOG(SINK, FMT, ...) then Clang will warn that nothing is passed to the
// variadic parameter. As the warning is generated in user code, enclosing the
// macro definitions in #pragma directives to ignore the warning achieves
// nothing.

/**
 * Basic log macro, logs the specified format string and arguments to
 * the given sink, blocking if the sink is full. The non-blocking variant
 * of this macro is @ref XTR_TRY_LOG which will discard the message if
 * the sink is full.
 */
#define XTR_LOG(SINK, ...) XTR_LOG_TAGS(void(), "I", SINK, __VA_ARGS__)

#define XTR_LOG_LEVEL(LEVELSTR, LEVEL, SINK, ...) \
    XTR_LOG_LEVEL_TAGS(void(), LEVELSTR, LEVEL, SINK, __VA_ARGS__)

/**
 *  'Fatal' log level variant of @ref XTR_LOG. An equivalent macro @ref XTR_LOGF
 *  is provided as a short-hand alternative. When this macro is invoked, the
 *  log message is written, @ref xtr::logger::sink::sync is invoked, then the
 *  program is terminated via abort(3).
 */
#define XTR_LOG_FATAL(SINK, ...) XTR_LOG_LEVEL("F", fatal, SINK, __VA_ARGS__)

/**
 *  'Error' log level variant of @ref XTR_LOG An equivalent macro @ref XTR_LOGE
 *  is provided as a short-hand alternative.
 */
#define XTR_LOG_ERROR(SINK, ...) XTR_LOG_LEVEL("E", error, SINK, __VA_ARGS__)

/**
 *  'Warning' log level variant of @ref XTR_LOG An equivalent macro @ref XTR_LOGW
 *  is provided as a short-hand alternative.
 */
#define XTR_LOG_WARN(SINK, ...) XTR_LOG_LEVEL("W", warning, SINK, __VA_ARGS__)

/**
 *  'Info' log level variant of @ref XTR_LOG An equivalent macro @ref XTR_LOGI
 *  is provided as a short-hand alternative.
 */
#define XTR_LOG_INFO(SINK, ...) XTR_LOG_LEVEL("I", info, SINK, __VA_ARGS__)

/**
 *  'Debug' log level variant of @ref XTR_LOG An equivalent macro @ref XTR_LOGD
 *  is provided as a short-hand alternative. This macro can be disabled at build
 *  time by defining @ref XTR_NDEBUG.
 */
#if defined(XTR_NDEBUG)
#define XTR_LOG_DEBUG(...)
#else
#define XTR_LOG_DEBUG(SINK, ...) XTR_LOG_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_LOGF(SINK, ...) XTR_LOG_FATAL(SINK, __VA_ARGS__)
#define XTR_LOGE(SINK, ...) XTR_LOG_ERROR(SINK, __VA_ARGS__)
#define XTR_LOGW(SINK, ...) XTR_LOG_WARN(SINK, __VA_ARGS__)
#define XTR_LOGI(SINK, ...) XTR_LOG_INFO(SINK, __VA_ARGS__)
#define XTR_LOGD(SINK, ...) XTR_LOG_DEBUG(SINK, __VA_ARGS__)

//
// XTR_TRY_LOG
//
#define XTR_TRY_LOG(SINK, ...) \
    XTR_LOG_TAGS(xtr::non_blocking_tag, "I", SINK, __VA_ARGS__)

#define XTR_TRY_LOG_LEVEL(LEVELSTR, LEVEL, SINK, ...) \
    XTR_LOG_LEVEL_TAGS(xtr::non_blocking_tag, LEVELSTR, LEVEL, SINK, __VA_ARGS__)

#define XTR_TRY_LOG_FATAL(SINK, ...) XTR_TRY_LOG_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_ERROR(SINK, ...) XTR_TRY_LOG_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_WARN(SINK, ...) XTR_TRY_LOG_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_INFO(SINK, ...) XTR_TRY_LOG_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_TRY_LOG_DEBUG(...)
#else
#define XTR_TRY_LOG_DEBUG(SINK, ...) XTR_TRY_LOG_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_TRY_LOGF(SINK, ...) XTR_TRY_LOG_FATAL(SINK, __VA_ARGS__)
#define XTR_TRY_LOGE(SINK, ...) XTR_TRY_LOG_ERROR(SINK, __VA_ARGS__)
#define XTR_TRY_LOGW(SINK, ...) XTR_TRY_LOG_WARN(SINK, __VA_ARGS__)
#define XTR_TRY_LOGI(SINK, ...) XTR_TRY_LOG_INFO(SINK, __VA_ARGS__)
#define XTR_TRY_LOGD(SINK, ...) XTR_TRY_LOG_DEBUG(SINK, __VA_ARGS__)

/**
 * Timestamped log macro, logs the specified format string and arguments
 * to the given sink along with the specified timestamp, blocking if the
 * sink is full. The timestamp may be any type so long as it has a formatter
 * defined (see :ref:custom-formatters). xtr::timestamp is provided as a
 * convenience type which is compatible with std::timestamp and has a
 * formatter pre-defined. A formatter for std::timestamp isn't defined in
 * order to avoid conflict with user code that also defines such a formatter.
 */
#define XTR_LOG_TS(SINK, TS, ...) \
    (__extension__({ XTR_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_LOG_TS_IMPL(SINK, TS, FMT, ...) \
    XTR_LOG_TAGS(xtr::timestamp_tag, "I", SINK, FMT, TS __VA_OPT__(,) __VA_ARGS__)

#define XTR_LOG_TS_LEVEL(LEVELSTR, LEVEL, SINK, TS, ...) \
    (__extension__({ XTR_LOG_TS_LEVEL_IMPL(LEVELSTR, LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_LOG_TS_LEVEL_IMPL(LEVELSTR, LEVEL, SINK, TS, FMT, ...)  \
    XTR_LOG_LEVEL_TAGS(                                             \
        xtr::timestamp_tag,                                         \
        LEVELSTR,                                                   \
        LEVEL,                                                      \
        SINK,                                                       \
        FMT,                                                        \
        TS                                                          \
        __VA_OPT__(,) __VA_ARGS__)

/**
 *  'Fatal' log level variant of @ref XTR_LOG_TS. An equivalent macro @ref XTR_LOG_TSF
 *  is provided as a short-hand alternative.
 */
#define XTR_LOG_TS_FATAL(SINK, ...) XTR_LOG_TS_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_LOG_TS_ERROR(SINK, ...) XTR_LOG_TS_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_LOG_TS_WARN(SINK, ...) XTR_LOG_TS_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_LOG_TS_INFO(SINK, ...) XTR_LOG_TS_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_LOG_TS_DEBUG(...)
#else
#define XTR_LOG_TS_DEBUG(SINK, ...) XTR_LOG_TS_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_LOG_TSF(SINK, ...) XTR_LOG_TS_FATAL(SINK, __VA_ARGS__)
#define XTR_LOG_TSE(SINK, ...) XTR_LOG_TS_ERROR(SINK, __VA_ARGS__)
#define XTR_LOG_TSW(SINK, ...) XTR_LOG_TS_WARN(SINK, __VA_ARGS__)
#define XTR_LOG_TSI(SINK, ...) XTR_LOG_TS_INFO(SINK, __VA_ARGS__)
#define XTR_LOG_TSD(SINK, ...) XTR_LOG_TS_DEBUG(SINK, __VA_ARGS__)

/**
 *
 */
#define XTR_TRY_LOG_TS(SINK, TS, ...) \
    (__extension__({  XTR_TRY_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOG_TS_IMPL(SINK, TS, FMT, ...)         \
    XTR_LOG_TAGS(                                       \
        (xtr::non_blocking_tag, xtr::timestamp_tag),    \
        "I",                                            \
        SINK,                                           \
        FMT,                                            \
        TS                                              \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TS_LEVEL(LEVELSTR, LEVEL, SINK, TS, ...) \
    (__extension__({  XTR_TRY_LOG_TS_LEVEL_IMPL(LEVELSTR, LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOG_TS_LEVEL_IMPL(LEVELSTR, LEVEL, SINK, TS, FMT, ...)  \
    XTR_LOG_LEVEL_TAGS(                                                 \
        (xtr::non_blocking_tag, xtr::timestamp_tag),                    \
        LEVELSTR,                                                       \
        LEVEL,                                                          \
        SINK,                                                           \
        FMT,                                                            \
        TS                                                              \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TS_FATAL(SINK, ...) XTR_TRY_LOG_TS_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TS_ERROR(SINK, ...) XTR_TRY_LOG_TS_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TS_WARN(SINK, ...) XTR_TRY_LOG_TS_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TS_INFO(SINK, ...) XTR_TRY_LOG_TS_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_TRY_LOG_TS_DEBUG(...)
#else
#define XTR_TRY_LOG_TS_DEBUG(SINK, ...) XTR_TRY_LOG_TS_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_TRY_LOG_TSF(SINK, ...) XTR_TRY_LOG_TS_FATAL(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSE(SINK, ...) XTR_TRY_LOG_TS_ERROR(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSW(SINK, ...) XTR_TRY_LOG_TS_WARN(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSI(SINK, ...) XTR_TRY_LOG_TS_INFO(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSD(SINK, ...) XTR_TRY_LOG_TS_DEBUG(SINK, __VA_ARGS__)

/**
 *
 */
#define XTR_LOG_RTC(SINK, ...) \
    XTR_LOG_TS(SINK, xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), __VA_ARGS__)

#define XTR_LOG_RTC_LEVEL(LEVELSTR, LEVEL, SINK, ...)       \
    XTR_LOG_TS_LEVEL(                                       \
        LEVELSTR,                                           \
        LEVEL,                                              \
        SINK,                                               \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(),   \
        __VA_ARGS__)

#define XTR_LOG_RTC_FATAL(SINK, ...) XTR_LOG_RTC_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_LOG_RTC_ERROR(SINK, ...) XTR_LOG_RTC_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_LOG_RTC_WARN(SINK, ...) XTR_LOG_RTC_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_LOG_RTC_INFO(SINK, ...) XTR_LOG_RTC_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_LOG_RTC_DEBUG(...)
#else
#define XTR_LOG_RTC_DEBUG(SINK, ...) XTR_LOG_RTC_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_LOG_RTCF(SINK, ...) XTR_LOG_RTC_FATAL(SINK, __VA_ARGS__)
#define XTR_LOG_RTCE(SINK, ...) XTR_LOG_RTC_ERROR(SINK, __VA_ARGS__)
#define XTR_LOG_RTCW(SINK, ...) XTR_LOG_RTC_WARN(SINK, __VA_ARGS__)
#define XTR_LOG_RTCI(SINK, ...) XTR_LOG_RTC_INFO(SINK, __VA_ARGS__)
#define XTR_LOG_RTCD(SINK, ...) XTR_LOG_RTC_DEBUG(SINK, __VA_ARGS__)

/**
 *
 */
#define XTR_TRY_LOG_RTC(SINK, ...) \
    XTR_TRY_LOG_TS(SINK, xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), __VA_ARGS__)

#define XTR_TRY_LOG_RTC_LEVEL(LEVELSTR, LEVEL, SINK, ...)       \
    XTR_TRY_LOG_TS_LEVEL(                                       \
        LEVELSTR,                                           \
        LEVEL,                                              \
        SINK,                                               \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(),   \
        __VA_ARGS__)

#define XTR_TRY_LOG_RTC_FATAL(SINK, ...) XTR_TRY_LOG_RTC_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTC_ERROR(SINK, ...) XTR_TRY_LOG_RTC_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTC_WARN(SINK, ...) XTR_TRY_LOG_RTC_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTC_INFO(SINK, ...) XTR_TRY_LOG_RTC_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_TRY_LOG_RTC_DEBUG(...)
#else
#define XTR_TRY_LOG_RTC_DEBUG(SINK, ...) XTR_TRY_LOG_RTC_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_TRY_LOG_RTCF(SINK, ...) XTR_TRY_LOG_RTC_FATAL(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTCE(SINK, ...) XTR_TRY_LOG_RTC_ERROR(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTCW(SINK, ...) XTR_TRY_LOG_RTC_WARN(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTCI(SINK, ...) XTR_TRY_LOG_RTC_INFO(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_RTCD(SINK, ...) XTR_TRY_LOG_RTC_DEBUG(SINK, __VA_ARGS__)

/**
 *
 */
#define XTR_LOG_TSC(SINK, ...) \
    XTR_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

#define XTR_LOG_TSC_LEVEL(LEVELSTR, LEVEL, SINK, ...)   \
    XTR_LOG_TS_LEVEL(                                   \
        LEVELSTR,                                       \
        LEVEL,                                          \
        SINK,                                           \
        xtr::detail::tsc::now(),                        \
        __VA_ARGS__)

#define XTR_LOG_TSC_FATAL(SINK, ...) XTR_LOG_TSC_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_LOG_TSC_ERROR(SINK, ...) XTR_LOG_TSC_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_LOG_TSC_WARN(SINK, ...) XTR_LOG_TSC_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_LOG_TSC_INFO(SINK, ...) XTR_LOG_TSC_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_LOG_TSC_DEBUG(...)
#else
#define XTR_LOG_TSC_DEBUG(SINK, ...) XTR_LOG_TSC_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_LOG_TSCF(SINK, ...) XTR_LOG_TSC_FATAL(SINK, __VA_ARGS__)
#define XTR_LOG_TSCE(SINK, ...) XTR_LOG_TSC_ERROR(SINK, __VA_ARGS__)
#define XTR_LOG_TSCW(SINK, ...) XTR_LOG_TSC_WARN(SINK, __VA_ARGS__)
#define XTR_LOG_TSCI(SINK, ...) XTR_LOG_TSC_INFO(SINK, __VA_ARGS__)
#define XTR_LOG_TSCD(SINK, ...) XTR_LOG_TSC_DEBUG(SINK, __VA_ARGS__)

/**
 *
 */
#define XTR_TRY_LOG_TSC(SINK, ...) \
    XTR_TRY_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

#define XTR_TRY_LOG_TSC_LEVEL(LEVELSTR, LEVEL, SINK, ...)   \
    XTR_TRY_LOG_TS_LEVEL(                                   \
        LEVELSTR,                                       \
        LEVEL,                                          \
        SINK,                                           \
        xtr::detail::tsc::now(),                        \
        __VA_ARGS__)

#define XTR_TRY_LOG_TSC_FATAL(SINK, ...) XTR_TRY_LOG_TSC_LEVEL("F", fatal, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSC_ERROR(SINK, ...) XTR_TRY_LOG_TSC_LEVEL("E", error, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSC_WARN(SINK, ...) XTR_TRY_LOG_TSC_LEVEL("W", warning, SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSC_INFO(SINK, ...) XTR_TRY_LOG_TSC_LEVEL("I", info, SINK, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_TRY_LOG_TSC_DEBUG(...)
#else
#define XTR_TRY_LOG_TSC_DEBUG(SINK, ...) XTR_TRY_LOG_TSC_LEVEL("D", debug, SINK, __VA_ARGS__)
#endif

#define XTR_TRY_LOG_TSCF(SINK, ...) XTR_TRY_LOG_TSC_FATAL(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSCE(SINK, ...) XTR_TRY_LOG_TSC_ERROR(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSCW(SINK, ...) XTR_TRY_LOG_TSC_WARN(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSCI(SINK, ...) XTR_TRY_LOG_TSC_INFO(SINK, __VA_ARGS__)
#define XTR_TRY_LOG_TSCD(SINK, ...) XTR_TRY_LOG_TSC_DEBUG(SINK, __VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

#define XTR_LOG_LEVEL_TAGS(TAGS, LEVELSTR, LEVEL, SINK, ...)                    \
    (__extension__                                                              \
        ({                                                                      \
            if ((SINK).level() >= xtr::log_level_t::LEVEL)                      \
                XTR_LOG_TAGS(TAGS, LEVELSTR, SINK, __VA_ARGS__);                \
            if constexpr (xtr::log_level_t::LEVEL == xtr::log_level_t::fatal)   \
            {                                                                   \
                (SINK).sync();                                                  \
                std::abort();                                                   \
            }                                                                   \
        }))

#define XTR_LOG_TAGS(TAGS, LEVELSTR, SINK, ...) \
    (__extension__({ XTR_LOG_TAGS_IMPL(TAGS, LEVELSTR, SINK, __VA_ARGS__); }))

// '{}{}:' in the format string is for the timestamp and sink name
#define XTR_LOG_TAGS_IMPL(TAGS, LEVELSTR, SINK, FORMAT, ...)                \
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

#endif
