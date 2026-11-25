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
#include "bytecopy.hpp"
#include "concepts.hpp"
#include "pause.hpp"
#include "serialized_args.hpp"
#include "tags.hpp"
#include "vcopy_wrapper.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
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

    template<typename Tags, typename Buffer, typename CopyTag>
    __attribute__((always_inline)) inline bool copy(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const void* value,
        std::size_t length,
        CopyTag copy_tag)
    {
        std::byte* value_end = pos + length;
        if (!wait_for_capacity<Tags>(end, value_end, buf)) [[unlikely]]
            return false;
        bytecopy(pos, value, length, copy_tag);
        pos = value_end;
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

    // XXX review comment
    // Forward if T is either:
    // * std::string&& or an r-value serializable container, which are moved
    //   rather than serialized as moving does not copy or allocate
    // * Not serialized into the variable-length area (see is_variable_length)
    // C string and string_view r-value references are not forwarded because
    // they are non-owning so being moved is meaningless.
    template<typename Tags, typename T, typename Buffer>
        requires(
            (std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))> &&
             (std::same_as<std::remove_cvref_t<T>, std::string> ||
              container_serializable<std::remove_cvref_t<T>>)) ||
            !is_variable_length_v<T>)
    T&& transform_args(std::byte*&, std::byte*&, Buffer&, bool&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename T, typename Buffer>
    inline variable_length_entry<T> transform_args(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        bool& overflow,
        detail::vcopy_wrapper<T> vc)
    {
        pos = align<alignof(T)>(pos);
        // Inline memcpy isn't used for vcopy because it is assumed that vcopy
        // targets will be large, e.g. market data or order entry structs.
        if (!copy<Tags>(pos, end, buf, &vc.value, vc.size, copy_memcpy))
            [[unlikely]]
        {
            overflow = true;
        }
        return variable_length_entry<T>(vc.size);
    }

    template<typename Tags, typename Buffer, typename String>
        requires std::same_as<String, std::string> ||
                 std::same_as<String, std::string_view>
    inline string_table_entry transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool&, const String& str)
    {
        if (!copy<Tags>(pos, end, buf, str.data(), str.length(), copy_inline))
            [[unlikely]]
        {
            return string_table_entry{string_table_entry::truncated};
        }
        return string_table_entry(str.length());
    }

    template<typename Tags, typename Buffer>
    inline string_table_entry transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool&, const char* str)
    {
        const std::size_t length = std::strlen(str);
        if (!copy<Tags>(pos, end, buf, str, length, copy_inline)) [[unlikely]]
            return string_table_entry{string_table_entry::truncated};
        return string_table_entry(length);
    }

    // XXX review comment
    // The tuple, array and container overloads below must be declared after
    // the overloads above. Nested tuples and containers are resolved via ADL
    // at instantiation (Buffer is in xtr::detail).
    //
    // The braced initialization below is not cosmetic: each transform_args
    // call advances pos, so members must be written in the order
    // reconstruct_args reads them back. Braced initializers are evaluated
    // left to right, whereas constructor or function call arguments are
    // evaluated in an unspecified order.

    // XXX review comment
    // Transforms a single member of a tuple or pair. Member is the declared
    // type of the member, which decides how it is stored: variable-length
    // members held by value are serialized, all other members are stored as
    // declared, so reference members remain references.
    template<typename Tags, typename Member, typename Buffer, typename T>
    decltype(auto) transform_member(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool& overflow, T& member)
    {
        if constexpr (is_variable_length_impl<std::remove_cv_t<Member>>::value)
            return transform_args<Tags>(pos, end, buf, overflow, member);
        else
            return static_cast<std::remove_cv_t<Member>>(member);
    }

    // XXX review comment
    // Handles std::tuple and std::pair, both of which are reconstructed as a
    // std::tuple.
    template<typename Tags, typename Buffer, typename T>
        requires tuple_like<T> && (!iterable<T>) && is_variable_length_v<T>
    auto transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool& overflow, const T& t)
    {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return tuple_entry<decltype(transform_member<Tags, std::tuple_element_t<Is, T>>(
                pos,
                end,
                buf,
                overflow,
                std::get<Is>(t)))...>{
                {transform_member<Tags, std::tuple_element_t<Is, T>>(
                    pos,
                    end,
                    buf,
                    overflow,
                    std::get<Is>(t))...}};
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
    }

    template<typename Tags, typename Buffer, typename T, std::size_t N>
        requires is_variable_length_v<std::array<T, N>>
    auto transform_args(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        bool& overflow,
        const std::array<T, N>& a)
    {
        using elem_t = std::remove_cvref_t<
            decltype(transform_args<Tags>(pos, end, buf, overflow, a[0]))>;
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return array_entry<elem_t, N>{
                {transform_args<Tags>(pos, end, buf, overflow, a[Is])...}};
        }(std::make_index_sequence<N>{});
    }

    template<typename Tags, typename Buffer, typename C>
        requires container_serializable<C>
    auto transform_args(
        std::byte*& pos, std::byte*& end, Buffer& buf, bool& overflow, const C& c)
    {
        // XXX review comment
        // Needed before any element is transformed, as the element array is
        // reserved first. Derived from a const reference to value_type, as
        // that is how elements are passed to transform_args below, even if
        // the container's iterator yields elements by value.
        using elem_t = std::remove_cvref_t<decltype(transform_args<Tags>(
            pos,
            end,
            buf,
            overflow,
            std::declval<const typename C::value_type&>()))>;
        using entry_t = container_entry<elem_t, associative_container<C>>;

        // XXX review comment
        // Elements are never destroyed, see
        // is_serialized_trivially_destructible.
        static_assert(std::is_trivially_destructible_v<elem_t>);

        const auto n = std::size_t(std::ranges::distance(c));

        // XXX review comment
        // Reserve the element array, then append each element's
        // variable-length data after it, in element order.
        pos = align<alignof(elem_t)>(pos);
        auto* elem = reinterpret_cast<elem_t*>(pos);
        std::byte* data = pos + n * sizeof(elem_t);
        if (!wait_for_capacity<Tags>(end, data, buf)) [[unlikely]]
        {
            overflow = true;
            return entry_t(0);
        }

        for (const auto& e : c)
            ::new (elem++)
                elem_t(transform_args<Tags>(data, end, buf, overflow, e));

        pos = data;
        return entry_t(n);
    }
}

#endif
