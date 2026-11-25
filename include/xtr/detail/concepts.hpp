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

#ifndef XTR_DETAIL_CONCEPTS_HPP
#define XTR_DETAIL_CONCEPTS_HPP

#include "is_c_string.hpp"
#include "vcopy_wrapper.hpp"
#include "xtr/enable_container_serialization.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
    template<typename T>
    concept iterable = std::ranges::range<T>;

    template<typename T>
    concept associative_container = requires(T t) { typename T::mapped_type; };

    template<typename T>
    concept tuple_like = requires(T t) { std::tuple_size<T>(); };

    template<typename T>
    concept string_like = is_c_string<T>::value ||
                          std::same_as<std::remove_cvref_t<T>, std::string> ||
                          std::same_as<std::remove_cvref_t<T>, std::string_view>;

    // True if the serialized form of T is trivially destructible, false
    // otherwise. Serialized container elements must be trivially destructible
    // to avoid having complex unwind logic for values that either throw while
    // being copied, or fail to copy due to lack of space in the sink buffer.
    template<typename T>
    struct is_serialized_trivially_destructible :
        std::is_trivially_destructible<T>
    {
    };

    template<>
    struct is_serialized_trivially_destructible<std::string> : std::true_type
    {
    };

    // A container can be serialized if its value_type can be serialized
    template<typename T>
        requires enable_container_serialization_v<T>
    struct is_serialized_trivially_destructible<T> :
        is_serialized_trivially_destructible<std::remove_const_t<typename T::value_type>>
    {
        static_assert(
            requires { typename T::value_type; },
            "Types enabled via enable_container_serialization must have a "
            "value_type");
        static_assert(
            std::ranges::range<T>,
            "Types enabled via enable_container_serialization must model "
            "std::ranges::range");
    };

    template<typename... Ts>
    struct is_serialized_trivially_destructible<std::tuple<Ts...>> :
        std::conjunction<is_serialized_trivially_destructible<std::remove_const_t<Ts>>...>
    {
    };

    template<typename... Ts>
    struct is_serialized_trivially_destructible<std::pair<Ts...>> :
        std::conjunction<is_serialized_trivially_destructible<std::remove_const_t<Ts>>...>
    {
    };

    template<typename T, std::size_t N>
    struct is_serialized_trivially_destructible<std::array<T, N>> :
        is_serialized_trivially_destructible<std::remove_const_t<T>>
    {
    };

    template<typename T>
    constexpr bool is_serialized_trivially_destructible_v =
        is_serialized_trivially_destructible<T>::value;

    template<typename T>
    concept container_serializable = enable_container_serialization_v<T> &&
                                     is_serialized_trivially_destructible_v<T>;

    // XXX review comment
    // True if T is serialized into the variable-length area of a log record,
    // either directly (strings, vcopy) or by containing such a type.
    // Specializations are on is_variable_length_impl, which sees the logged
    // type with cv-qualifiers and references removed. Members of a tuple or
    // pair only have cv-qualifiers removed, as a reference member is kept as
    // a reference rather than serialized.
    template<typename T>
    struct is_variable_length;

    template<typename T>
    struct is_variable_length_impl :
        std::disjunction<
            std::conjunction<std::negation<std::is_reference<T>>, is_c_string<T>>,
            std::is_same<T, std::string>,
            std::is_same<T, std::string_view>,
            is_vcopy_wrapper<T>>
    {
    };

    template<typename... Ts>
    struct is_variable_length_impl<std::tuple<Ts...>> :
        std::disjunction<is_variable_length_impl<std::remove_cv_t<Ts>>...>
    {
    };

    template<typename... Ts>
    struct is_variable_length_impl<std::pair<Ts...>> :
        std::disjunction<is_variable_length_impl<std::remove_cv_t<Ts>>...>
    {
    };

    template<typename T, std::size_t N>
    struct is_variable_length_impl<std::array<T, N>> : is_variable_length<T>
    {
    };

    template<typename T>
        requires container_serializable<T>
    struct is_variable_length_impl<T> : std::true_type
    {
    };

    template<typename T>
    struct is_variable_length : is_variable_length_impl<std::remove_cvref_t<T>>
    {
    };

    template<typename T>
    constexpr bool is_variable_length_v = is_variable_length<T>::value;
}

#endif
