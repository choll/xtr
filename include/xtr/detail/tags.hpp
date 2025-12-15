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

#ifndef XTR_DETAIL_TAGS_HPP
#define XTR_DETAIL_TAGS_HPP

#include "xtr/tags.hpp"

#include <type_traits>

namespace xtr::detail
{
    template<typename Tag, typename F>
    struct detect_tag;

    template<typename Tag, typename... Tags>
    struct detect_tag<Tag, void(Tags...)> :
        std::disjunction<std::is_same<Tag, Tags>...>
    {
    };

    template<typename Tag, typename Tags>
    struct add_tag;

    template<typename Tag, typename... Tags>
    struct add_tag<Tag, void(Tags...)>
    {
        using type = void(Tag, Tags...);
    };

    template<typename Tag, typename... Tags>
    using add_tag_t = typename add_tag<Tag, Tags...>::type;

    struct speculative_tag;

    template<typename Tags>
    inline constexpr bool is_non_blocking_v =
        detect_tag<non_blocking_tag, Tags>::value;

    template<typename Tags>
    inline constexpr bool is_speculative_v =
        detect_tag<speculative_tag, Tags>::value;

    template<typename Tags>
    inline constexpr bool is_timestamp_v = detect_tag<timestamp_tag, Tags>::value;
}

#endif
