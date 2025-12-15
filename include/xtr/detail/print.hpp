// Copyright 2020 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef XTR_DETAIL_PRINT_HPP
#define XTR_DETAIL_PRINT_HPP

#include "buffer.hpp"
#include "xtr/log_level.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename Format, typename Timestamp, typename... Args>
    void print(
        buffer& buf,
        const Format& fmt,
        log_level_t level,
        Timestamp ts,
        const std::string& name,
        const Args&... args) noexcept
    {
#if __cpp_exceptions
        try
        {
#endif
            fmt::format_to(
                std::back_inserter(buf.line),
                fmt,
                buf.lstyle(level),
                ts,
                name,
                args...);

            buf.append_line();
#if __cpp_exceptions
        }
        catch (const std::exception& e)
        {
            using namespace std::literals::string_view_literals;
            fmt::print(
                stderr,
                FMT_COMPILE("{}{}: Error writing log: {}\n"),
                buf.lstyle(log_level_t::error),
                ts,
                e.what());
            buf.line.clear();
        }
#endif
    }

    template<typename Format, typename Timestamp, typename... Args>
    void print_ts(
        buffer& buf,
        const Format& fmt,
        log_level_t level,
        const std::string& name,
        Timestamp ts,
        const Args&... args) noexcept
    {
        print(buf, fmt, level, ts, name, args...);
    }
}

#endif
