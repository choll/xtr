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

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
    struct string_table_entry
    {
        static constexpr auto truncated = std::numeric_limits<std::uint32_t>::max();

        // This conversion is safe as sink cannot have a capacity greater
        // than UINT_MAX (via a static assertion in sink.hpp). Additionally
        // truncated having the value UINT_MAX is safe as the size of a log
        // record must be greater than zero.
        explicit string_table_entry(std::size_t sz)
        :
            size(std::uint32_t(sz))
        {
        }

        std::uint32_t size;
    };

    template<typename T>
    requires (!std::same_as<std::remove_cvref_t<T>, string_table_entry>)
    T&& transform_string_table_entry(const std::byte*, T&& value)
    {
        return std::forward<T>(value);
    }

    inline string_ref<std::string_view> transform_string_table_entry(
        std::byte*& pos,
        string_table_entry entry)
    {
        if (entry.size == string_table_entry::truncated) [[unlikely]]
            return string_ref<std::string_view>("<truncated>");
        const std::string_view str(reinterpret_cast<const char*>(pos), entry.size);
        pos += entry.size;
        return string_ref<std::string_view>(str);
    }

    // Forward if T is either:
    // * std::string&&
    // * Not a C string, string, or string_view
    // C string and string_view r-value references are not forwarded because
    // they are non-owning so being moved is meaningless.
    template<typename Tags, typename T, typename Buffer>
    requires
        (std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))> &&
         std::same_as<std::remove_cvref_t<T>, std::string>) ||
        (!is_c_string<T>::value &&
         !std::same_as<std::remove_cvref_t<T>, std::string> &&
         !std::same_as<std::remove_cvref_t<T>, std::string_view>)
    T&& build_string_table(std::byte*&, std::byte*&, Buffer&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename Buffer, typename String>
    requires
        std::same_as<String, std::string> ||
        std::same_as<String, std::string_view>
    string_table_entry build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const String& sv)
    {
        std::byte* str_end = pos + sv.length();
        while (end < str_end) [[unlikely]]
        {
            pause();
            const auto s = buf.write_span();
            if (s.end() < str_end) [[unlikely]]
            {
                if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    return string_table_entry{string_table_entry::truncated};
            }
            end = s.end();
        }

        std::memcpy(pos, sv.data(), sv.length());
        pos += sv.length();

        return string_table_entry(sv.length());
    }

    template<typename Tags, typename Buffer>
    string_table_entry build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const char* str)
    {
        std::byte* begin = pos;
        while (*str != '\0')
        {
            while (pos == end) [[unlikely]]
            {
                pause();
                const auto s = buf.write_span();
                if (s.end() == end) [[unlikely]]
                {
                    if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    {
                        pos = begin;
                        return string_table_entry{string_table_entry::truncated};
                    }
                }
                end = s.end();
            }
            ::new (pos++) char(*str++);
        }
        return string_table_entry(std::size_t(pos - begin));
    }
}

#endif
