#ifndef XTR_DETAIL_CONCEPTS_HPP
#define XTR_DETAIL_CONCEPTS_HPP

#include "is_c_string.hpp"
#include "vcopy_wrapper.hpp"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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

    // XXX THIS SHOULD BE OUTSIDE OF THE DETAIL NAMESPACE BECAUSE USERS ARE PERMITTED
    // TO SPECIALIZE IT.
    template<typename T>
    struct enable_container_serialization;

    template<typename T>
    constexpr bool enable_container_serialization_v =
        enable_container_serialization<T>::value;

    // True if the serialized form of T is trivially destructible, false
    // otherwise. We only serialize containers that have a trivially
    // destructible serialized form in order to avoid having to implement
    // complicated unwind logic in case an exception is thrown when copying
    // items in.
    //
    // XXX is_c_string is wrong! just need to check std::string
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

    ////

    template<typename T>
    struct enable_container_serialization : std::false_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::deque<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::list<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::map<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_map<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::unordered_set<Args...>> :
        std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::set<Args...>> : std::true_type
    {
    };

    template<typename... Args>
    struct enable_container_serialization<std::vector<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept container_serializable =
        enable_container_serialization_v<T> &&
        is_serialized_trivially_destructible_v<T> && iterable<T>;

    // True if T is serialized into the variable-length area of a log record,
    // either directly (strings, vcopy) or by containing such a type.
    // Specializations are on is_variable_length_impl, which only ever sees
    // types with cv-qualifiers and references removed.
    template<typename T>
    struct is_variable_length;

    template<typename T>
    struct is_variable_length_impl :
        std::disjunction<
            is_c_string<T>,
            std::is_same<T, std::string>,
            std::is_same<T, std::string_view>,
            is_vcopy_wrapper<T>>
    {
    };

    template<typename... Ts>
    struct is_variable_length_impl<std::tuple<Ts...>> :
        std::disjunction<is_variable_length<Ts>...>
    {
    };

    template<typename... Ts>
    struct is_variable_length_impl<std::pair<Ts...>> :
        std::disjunction<is_variable_length<Ts>...>
    {
    };

    template<typename T, std::size_t N>
    struct is_variable_length_impl<std::array<T, N>> : is_variable_length<T>
    {
    };

    template<typename T>
    struct is_variable_length : is_variable_length_impl<std::remove_cvref_t<T>>
    {
    };

    template<typename T>
    constexpr bool is_variable_length_v = is_variable_length<T>::value;

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
