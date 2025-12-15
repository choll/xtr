#ifndef XTR_DETAIL_CONCEPTS_HPP
#define XTR_DETAIL_CONCEPTS_HPP

#include <utility>

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

    template<typename T>
    struct is_allocated;

    template<typename T>
        requires(!tuple_like<T>)
    struct is_allocated<T>
    {
        static constexpr bool value = requires { typename T::allocator_type; };
    };

    template<typename T>
        requires(tuple_like<T>)
    struct is_allocated<T>
    {
        static constexpr bool value =
            []<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return (is_allocated<std::tuple_element_t<Is, T>>::value || ...);
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
    };

    template<typename T>
    concept allocated = is_allocated<T>::value;
}

#endif
