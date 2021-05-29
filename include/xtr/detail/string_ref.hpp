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

#ifndef XTR_DETAIL_STRING_REF_HPP
#define XTR_DETAIL_STRING_REF_HPP

#include "sanitize.hpp"

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename T>
    struct string_ref;

    template<>
    struct string_ref<const char*>
    {
        explicit string_ref(const char* s)
        :
            str(s)
        {
        }

        explicit string_ref(const std::string& s)
        :
            str(s.c_str())
        {
        }

        const char* str;
    };

    string_ref(const char*) -> string_ref<const char*>;
    string_ref(const std::string&) -> string_ref<const char*>;
    string_ref(const std::string_view&) -> string_ref<std::string_view>;

    template<>
    struct string_ref<std::string_view>
    {
        explicit string_ref(std::string_view s)
        :
            str(s)
        {
        }

        std::string_view str;
    };
}

namespace fmt
{
    template<>
    struct formatter<xtr::detail::string_ref<const char*>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(xtr::detail::string_ref<const char*> ref, FormatContext &ctx)
        {
            auto pos = ctx.out();
            while (*ref.str != '\0')
                xtr::detail::sanitize_char_to(pos, *ref.str++);
            return pos;
        }
    };

    template<>
    struct formatter<xtr::detail::string_ref<std::string_view>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::detail::string_ref<std::string_view> ref, FormatContext &ctx)
        {
            auto pos = ctx.out();
            for (const char c : ref.str)
                xtr::detail::sanitize_char_to(pos, c);
            return pos;
        }
    };
}

#endif
