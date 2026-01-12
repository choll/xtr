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
}

#endif
