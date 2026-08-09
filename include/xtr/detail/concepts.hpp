#ifndef XTR_DETAIL_CONCEPTS_HPP
#define XTR_DETAIL_CONCEPTS_HPP

#include "is_c_string.hpp"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xtr::detail
{
    template<typename T>
    concept iterable = requires(T t) {
        std::begin(t);
        std::end(t);
    };

    template<typename T>
    concept associative_container = requires(T t) { typename T::mapped_type; };

    template<typename T>
    concept tuple_like = requires(T t) { std::tuple_size<T>(); };

    // is_serialized_trivially_destructible
    //           is_string_like_v<T> ||
    //           (enable_container_copy_v<T> && storable_v<typename
    //           T::value_type>) || (tuple_like<T> && all_storable_v<T>) ||
    //           std::is_trivially_destructible_v<T>;

    template<typename T>
    struct is_serialized_trivially_destructible : is_c_string<T>
    {
    };

    template<typename... Ts>
    struct is_serialized_trivially_destructible<std::tuple<Ts...>> :
        std::conjunction<is_serialized_trivially_destructible<Ts>...>
    {
    };

    template<typename... Ts>
    struct is_serialized_trivially_destructible<std::pair<Ts...>> :
        std::conjunction<is_serialized_trivially_destructible<Ts>...>
    {
    };

    template<typename T, std::size_t N>
    struct is_serialized_trivially_destructible<std::array<T, N>> :
        is_serialized_trivially_destructible<T>
    {
    };

    template<typename T>
    constexpr bool is_serialized_trivially_destructible_v =
        is_serialized_trivially_destructible<T>::value;

    ////

    template<typename T>
    struct enable_container_copy : std::false_type
    {
    };

    template<typename... Args>
    struct enable_container_copy<std::deque<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_copy<std::list<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_copy<std::map<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_copy<std::unordered_map<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_copy<std::vector<Args...>> : std::true_type
    {
    };

    template<typename T>
    constexpr bool enable_container_copy_v = enable_container_copy<T>::value;

    template<typename T>
    concept container_copyable =
        enable_container_copy_v<T> &&
        is_serialized_trivially_destructible_v<T> && requires(T t) {
            std::begin(t);
            std::end(t);
            std::size(t);
        };

    // flerpfdsfsd

    // template<typename T>
    // struct is_allocated;
    //
    // template<typename T>
    //     requires(!tuple_like<T>)
    // struct is_allocated<T>
    // {
    //     static constexpr bool value = requires { typename T::allocator_type; };
    // };
    //
    // template<typename T>
    //     requires(tuple_like<T>)
    // struct is_allocated<T>
    // {
    //     static constexpr bool value =
    //         []<std::size_t... Is>(std::index_sequence<Is...>)
    //     {
    //         return (is_allocated<std::tuple_element_t<Is, T>>::value || ...);
    //     }(std::make_index_sequence<std::tuple_size_v<T>>{});
    // };
    //
    // template<typename T>
    // concept allocated = is_allocated<T>::value;
}

#endif
