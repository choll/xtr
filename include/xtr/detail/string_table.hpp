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

#ifndef XTR_DETAIL_STRING_TABLE_HPP
#define XTR_DETAIL_STRING_TABLE_HPP

#include "pause.hpp"
#include "is_c_string.hpp"
#include "string_ref.hpp"
#include "tags.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace xtr::detail
{
    // Forward if T is either:
    // * std::string&&
    // * Not a C string, string, or string_view
    // C string and string_view r-value references are not forwarded because
    // they are non-owning so being moved is meaningless.
    template<typename Tags, typename T, typename Buffer>
    requires
        (std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))> &&
         std::is_same_v<std::remove_cvref_t<T>, std::string>) ||
        (!is_c_string<T>::value &&
         !std::is_same_v<std::remove_cvref_t<T>, std::string> &&
         !std::is_same_v<std::remove_cvref_t<T>, std::string_view>)
    T&& build_string_table(std::byte*&, std::byte*&, Buffer&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename Buffer, typename String>
    requires
        std::is_same_v<String, std::string> ||
        std::is_same_v<String, std::string_view>
    string_ref<const char*> build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const String& sv)
    {
        std::byte* str_end = pos + sv.length();
        while (end < str_end + 1) [[unlikely]]
        {
            detail::pause();
            const auto s = buf.write_span();
            if (s.end() < str_end + 1) [[unlikely]]
            {
                if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    return string_ref("<truncated>");
            }
            end = s.end();
        }
        const char* result = reinterpret_cast<char*>(pos);
        const char* str = sv.data();
        while (pos != str_end)
            new (pos++) char(*str++);
        new (pos++) char('\0');
        return string_ref(result);
    }

    template<typename Tags, typename Buffer>
    string_ref<const char*> build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const char* str)
    {
        const char* result = reinterpret_cast<char*>(pos);
        do
        {
            while (pos == end) [[unlikely]]
            {
                detail::pause();
                const auto s = buf.write_span();
                if (s.end() == end) [[unlikely]]
                {
                    if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                        return string_ref("<truncated>");
                }
                end = s.end();
            }
            new (pos++) char(*str);
        } while (*str++ != '\0');
        return string_ref(result);
    }
}

#endif

