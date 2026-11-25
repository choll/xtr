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

#ifndef XTR_DETAIL_SERIALIZED_ARGS_HPP
#define XTR_DETAIL_SERIALIZED_ARGS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>

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
        // See comment on the corresponding string_table_entry constructor
        explicit variable_length_entry(std::size_t sz) :
            size(std::uint32_t(sz))
        {
        }

        std::uint32_t size;
    };

    // XXX review comment
    // T is the type each element is stored as in the variable-length area. Map
    // determines whether reconstruct_args presents the container as key/value
    // pairs printed {k: v, ...}, or as a sequence printed [a, b, ...].
    template<typename T, bool Map>
    struct container_entry
    {
        // See comment on the corresponding string_table_entry constructor
        explicit container_entry(std::size_t sz) :
            size(std::uint32_t(sz))
        {
        }

        std::uint32_t size;
    };

    template<typename... Ts>
    struct tuple_entry
    {
        std::tuple<Ts...> value;
    };

    template<typename T, std::size_t N>
    struct array_entry
    {
        std::array<T, N> value;
    };
}

#endif
