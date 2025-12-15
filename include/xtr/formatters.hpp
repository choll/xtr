// Copyright 2021 Chris E. Holloway
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

#ifndef XTR_FORMATTERS_HPP
#define XTR_FORMATTERS_HPP

#include "detail/concepts.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <fmt/compile.h>
#include <fmt/format.h>

namespace fmt
{
    // Note that https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92944 prevents
    // writing fmt::formatter

    template<typename T>
    requires xtr::detail::tuple_like<T> && (!xtr::detail::iterable<T>)
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            constexpr auto tail_size =
                std::max<std::size_t>(std::tuple_size_v<T>, 1) - 1;
            return format_impl(value, ctx, std::make_index_sequence<tail_size>{});
        }

        template<typename FormatContext, std::size_t... Indexes>
        auto format_impl(
            const T& value, FormatContext& ctx, std::index_sequence<Indexes...>) const
        {
            fmt::format_to(ctx.out(), "(");
            if constexpr (std::tuple_size_v<T> > 0)
            {
                fmt::format_to(ctx.out(), FMT_COMPILE("{}"), std::get<0>(value));
                ((fmt::format_to(
                     ctx.out(),
                     FMT_COMPILE(", {}"),
                     std::get<Indexes + 1>(value))),
                 ...);
            }
            return fmt::format_to(ctx.out(), ")");
        }
    };

    template<xtr::detail::associative_container T>
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            fmt::format_to(ctx.out(), "{{");
            if (!value.empty())
            {
                auto it = std::begin(value);
                ;
                fmt::format_to(ctx.out(), FMT_COMPILE("{}: {}"), it->first, it->second);
                ++it;
                while (it != std::end(value))
                {
                    fmt::format_to(
                        ctx.out(),
                        FMT_COMPILE(", {}: {}"),
                        it->first,
                        it->second);
                    ++it;
                }
            }
            return fmt::format_to(ctx.out(), "}}");
        }
    };

    template<typename T>
    requires
        xtr::detail::iterable<T> &&
        (!std::is_constructible_v<T, const char*>) &&
        (!xtr::detail::associative_container<T>)
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            fmt::format_to(ctx.out(), "[");
            if (!value.empty())
            {
                auto it = std::begin(value);
                ;
                fmt::format_to(ctx.out(), FMT_COMPILE("{}"), *it++);
                while (it != std::end(value))
                    fmt::format_to(ctx.out(), FMT_COMPILE(", {}"), *it++);
            }
            return fmt::format_to(ctx.out(), "]");
        }
    };
}

#endif
