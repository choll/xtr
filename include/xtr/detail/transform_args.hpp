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

#ifndef XTR_DETAIL_TRANSFORM_ARGS_HPP
#define XTR_DETAIL_TRANSFORM_ARGS_HPP

#include "align.hpp"
#include "is_c_string.hpp"
#include "pause.hpp"
#include "string_ref.hpp"
#include "tags.hpp"
#include "vcopy.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
    struct string_table_entry
    {
        static constexpr auto truncated =
            std::numeric_limits<std::uint32_t>::max();

        // This conversion is safe as sink cannot have a capacity greater
        // than UINT_MAX (via a static assertion in sink.hpp). Additionally
        // truncated having the value UINT_MAX is safe as the size of a log
        // record must be greater than zero.
        explicit string_table_entry(std::size_t sz) :
            size(std::uint32_t(sz))
        {
        }

        std::uint32_t size;
    };

    template<typename T>
    struct variable_length_entry
    {
        std::uint32_t size;
    };

    template<typename T>
    T&& reconstruct_args(const std::byte*, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename T>
    inline const T& reconstruct_args(std::byte*& pos, variable_length_entry<T> entry)
    {
        pos = align<alignof(T)>(pos);
        const T& value = *reinterpret_cast<const T*>(pos);
        pos += entry.size;
        return value;
    }

    inline string_ref<std::string_view> reconstruct_args(
        std::byte*& pos, string_table_entry entry)
    {
        if (entry.size == string_table_entry::truncated) [[unlikely]]
            return string_ref<std::string_view>("<truncated>");
        const std::string_view str(reinterpret_cast<const char*>(pos), entry.size);
        pos += entry.size;
        return string_ref<std::string_view>(str);
    }

    template<typename Tags, typename Buffer>
    __attribute__((always_inline)) inline bool wait_for_capacity(
        std::byte*& end, std::byte* min_end, Buffer& buf)
    {
        while (end < min_end) [[unlikely]]
        {
            pause();
            const auto s = buf.write_span();
            if (s.end() < min_end) [[unlikely]]
            {
                if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    return false;
            }
            end = s.end();
        }
        return true;
    }

    template<typename Tags, typename Buffer>
    __attribute__((always_inline)) inline bool copy(
        std::byte*& pos, std::byte*& end, Buffer& buf, const void* value, std::size_t length)
    {
        std::byte* value_end = pos + length;
        if (!wait_for_capacity<Tags>(end, value_end, buf)) [[unlikely]]
            return false;
        std::memcpy(pos, value, length);
        pos += length;
        return true;
    }

    template<typename Tags, typename T, typename Buffer>
    __attribute__((always_inline)) inline bool copy(
        std::byte*& pos, std::byte*& end, Buffer& buf, T&& value)
    {
        std::byte* value_end = pos + sizeof(T);
        if (!wait_for_capacity<Tags>(end, value_end, buf)) [[unlikely]]
            return false;
        ::new (pos) std::remove_reference_t<T>(std::forward<T>(value));
        pos += sizeof(T);
        return true;
    }

    // Forward if T is either:
    // * std::string&&
    // * Not a C string, string, or string_view
    // C string and string_view r-value references are not forwarded because
    // they are non-owning so being moved is meaningless.
    template<typename Tags, typename T, typename Buffer>
        requires(
            (std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))> &&
             std::same_as<std::remove_cvref_t<T>, std::string>) ||
            (!is_c_string<T>::value &&
             !std::same_as<std::remove_cvref_t<T>, std::string> &&
             !std::same_as<std::remove_cvref_t<T>, std::string_view>))
    T&& transform_args(std::byte*&, std::byte*&, Buffer&, bool&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename T, typename Buffer>
    variable_length_entry<T> transform_args(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        bool& overflow,
        detail::vcopy_wrapper<T> vc)
    {
        pos = align<alignof(T)>(pos);
        if (!copy<Tags>(pos, end, buf, &vc.value, vc.size)) [[unlikely]]
            overflow = true;
        return variable_length_entry<T>(vc.size);
    }

    template<typename Tags, typename Buffer, typename String>
        requires std::same_as<String, std::string> ||
                 std::same_as<String, std::string_view>
    string_table_entry transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool&, const String& str)
    {
        if (!copy<Tags>(pos, end, buf, str.data(), str.length())) [[unlikely]]
            return string_table_entry{string_table_entry::truncated};
        return string_table_entry(str.length());
    }

    template<typename Tags, typename Buffer>
    string_table_entry transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool&, const char* str)
    {
        std::byte* begin = pos;
        while (*str != '\0')
        {
            if (!copy<Tags>(pos, end, buf, *str++)) [[unlikely]]
            {
                pos = begin;
                return string_table_entry{string_table_entry::truncated};
            }
        }
        return string_table_entry(std::size_t(pos - begin));
    }
}

#endif
