// Copyright 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_PRINT_HPP
#define XTR_DETAIL_PRINT_HPP

#include "xtr/log_level.hpp"

#include <fmt/format.h>

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename ErrorFunction,typename Timestamp>
    [[gnu::cold, gnu::noinline]] void report_error(
        fmt::memory_buffer& mbuf,
        const ErrorFunction& err,
        log_level_style_t lstyle,
        Timestamp ts,
        const std::string& name,
        const char* reason)
    {
        using namespace std::literals::string_view_literals;
        mbuf.clear();
#if FMT_VERSION >= 80000
        fmt::format_to(
            std::back_inserter(mbuf),
            "{}{} {}: Error: {}\n"sv,
            lstyle(log_level_t::error),
            ts,
            name,
            reason);
#else
        fmt::format_to(
            mbuf,
            "{}{} {}: Error: {}\n"sv,
            lstyle(log_level_t::error),
            ts,
            name,
            reason);
#endif
        err(mbuf.data(), mbuf.size());
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Timestamp,
        typename... Args>
    void print(
        fmt::memory_buffer& mbuf,
        const OutputFunction& out,
        [[maybe_unused]] const ErrorFunction& err,
        log_level_style_t lstyle,
        std::string_view fmt,
        log_level_t level,
        Timestamp ts,
        const std::string& name,
        const Args&... args)
    {
#if __cpp_exceptions
        try
        {
#endif
            mbuf.clear();
#if FMT_VERSION >= 80000
            fmt::format_to(
                std::back_inserter(mbuf),
                fmt::runtime(fmt),
                lstyle(level),
                ts,
                name,
                args...);
#else
            fmt::format_to(mbuf, fmt, lstyle(level), ts, name, args...);
#endif
            const auto result = out(level, mbuf.data(), mbuf.size());
            if (result == -1)
                return report_error(mbuf, err, lstyle, ts, name, "Write error");
            if (std::size_t(result) != mbuf.size())
                return report_error(mbuf, err, lstyle, ts, name, "Short write");
#if __cpp_exceptions
        }
        catch (const std::exception& e)
        {
            report_error(mbuf, err, lstyle, ts, name, e.what());
        }
#endif
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Timestamp,
        typename... Args>
    void print_ts(
        fmt::memory_buffer& mbuf,
        const OutputFunction& out,
        const ErrorFunction& err,
        log_level_style_t lstyle,
        std::string_view fmt,
        log_level_t level,
        const std::string& name,
        Timestamp ts,
        const Args&... args)
    {
        print(mbuf, out, err, lstyle, fmt, level, ts, name, args...);
    }
}

#endif
