// Copyright 2026 Chris E. Holloway
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

#ifndef XTR_DETAIL_RECONSTRUCT_ARGS_HPP
#define XTR_DETAIL_RECONSTRUCT_ARGS_HPP

#include "align.hpp"
#include "serialized_args.hpp"
#include "string_ref.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
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

    // XXX review comment
    // Members are either entries, reconstructed by their own overloads, or
    // values that transform_args forwarded unchanged, which the generic
    // overload returns by reference.
    //
    // The braced initialization below is not cosmetic: each reconstruct_args
    // call advances pos, so members must be read in the order transform_args
    // wrote them. Braced initializers are evaluated left to right, whereas
    // constructor or function call arguments are evaluated in an unspecified
    // order.

    template<typename... Ts>
    auto reconstruct_args(std::byte*& pos, tuple_entry<Ts...>& entry)
    {
        return std::apply(
            [&](auto&... members)
            {
                return std::tuple<decltype(reconstruct_args(pos, members))...>{
                    reconstruct_args(pos, members)...};
            },
            entry.value);
    }

    template<typename T, std::size_t N>
    auto reconstruct_args(std::byte*& pos, array_entry<T, N>& entry)
    {
        using elem_t = decltype(reconstruct_args(pos, entry.value[0]));
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return std::array<elem_t, N>{reconstruct_args(pos, entry.value[Is])...};
        }(std::make_index_sequence<N>{});
    }

    // XXX review comment
    // A view over a container serialized by transform_args: an array of T,
    // followed by the elements' variable-length data in element order.
    // Iterators hold a pointer into the array and a cursor into the data.
    // Dereferencing decodes from a copy of the cursor, and incrementing
    // decodes again to advance it, so iterators share no state.
    template<typename T>
    class container_list_view
    {
    public:
        class iterator
        {
        public:
            using reference = decltype(reconstruct_args(
                std::declval<std::byte*&>(), std::declval<T&>()));
            using value_type = std::remove_cvref_t<reference>;
            using difference_type = std::ptrdiff_t;

            iterator() = default;

            iterator(T* elem, std::byte* data) :
                elem_(elem),
                data_(data)
            {
            }

            reference operator*() const
            {
                std::byte* data = data_;
                return reconstruct_args(data, *elem_);
            }

            iterator& operator++()
            {
                reconstruct_args(data_, *elem_);
                ++elem_;
                return *this;
            }

            iterator operator++(int)
            {
                iterator tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const iterator& other) const
            {
                return elem_ == other.elem_;
            }

        private:
            T* elem_ = nullptr;
            std::byte* data_ = nullptr;
        };

        using value_type = typename iterator::value_type;

        container_list_view(T* elems, std::byte* data, std::size_t size) :
            elems_(elems),
            data_(data),
            size_(size)
        {
        }

        iterator begin() const
        {
            return iterator(elems_, data_);
        }

        iterator end() const
        {
            return iterator(elems_ + size_, nullptr);
        }

        bool empty() const
        {
            return size_ == 0;
        }

    private:
        T* elems_;
        std::byte* data_;
        std::size_t size_;
    };

    // XXX review comment
    // mapped_type makes the associative_container formatter match, printing
    // {k: v, ...}.
    template<typename T>
    class container_map_view : public container_list_view<T>
    {
    public:
        using container_list_view<T>::container_list_view;
        using mapped_type = std::remove_cvref_t<
            std::tuple_element_t<1, typename container_list_view<T>::value_type>>;
    };

    // XXX review comment
    // Advances pos past the element array and then past the elements'
    // variable-length data, by walking the elements. Walking is a trade-off
    // against storing an explicit end offset in the container entry: it keeps
    // the entry small for the common case of fixed-size elements, at the cost
    // of a walk for containers with variable-length elements.
    template<typename T, bool Map>
    auto reconstruct_args(std::byte*& pos, container_entry<T, Map> entry)
    {
        using view_t =
            std::conditional_t<Map, container_map_view<T>, container_list_view<T>>;
        pos = align<alignof(T)>(pos);
        auto* elems = reinterpret_cast<T*>(pos);
        pos += entry.size * sizeof(T);
        view_t view(elems, pos, entry.size);
        for (std::uint32_t i = 0; i != entry.size; ++i)
            reconstruct_args(pos, elems[i]);
        return view;
    }
}

#endif
