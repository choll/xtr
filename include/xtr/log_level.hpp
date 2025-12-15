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

#ifndef XTR_LOG_LEVEL_HPP
#define XTR_LOG_LEVEL_HPP

#include <string_view>

namespace xtr
{
    /**
     * Passed to @ref XTR_LOGL, @ref XTR_LOGL_TSC etc to indicate the severity
     * of the log message.
     */
    enum class log_level_t
    {
        none,
        fatal,
        error,
        warning,
        info,
        debug
    };

    /**
     * Converts a string containing a log level name to the corresponding @ref
     * log_level_t enum value. Throws std::invalid_argument if the given string
     * does not correspond to any log level.
     */
    log_level_t log_level_from_string(std::string_view str);

    /**
     * Log level styles are used to customise the formatting used when prefixing
     * log statements with their associated log level (see @ref log_level_t).
     * Styles are simply function pointers\---to provide a custom style, define
     * a function returning a string literal and accepting a single argument of
     * type @ref log_level_t and pass the function to logger::logger or
     * logger::set_log_level_style. The values returned by the function will be
     * prefixed to log statements produced by the logger. Two formatters are
     * provided, the default formatter @ref default_log_level_style and a
     * Systemd compatible style @ref systemd_log_level_style.
     */
    using log_level_style_t = const char* (*)(log_level_t);

    /**
     * The default log level style (see @ref log_level_style_t). Returns a
     * single upper-case character representing the log level followed by a
     * space, e.g. "E ", "W ", "I " for log_level_t::error,
     * log_level_t::warning, log_level_t::info and so on.
     */
    const char* default_log_level_style(log_level_t level);

    /**
     * Systemd log level style (see @ref log_level_style_t). Returns strings as
     * described in
     * <a href="https://man7.org/linux/man-pages/man3/sd-daemon.3.html">sd-daemon(3)</a>,
     * e.g. "<0>", "<1>", "<2>" etc.
     */
    const char* systemd_log_level_style(log_level_t level);
}

#endif
