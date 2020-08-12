#ifndef XTR_DETAIL_IS_C_STRING_HPP
#define XTR_DETAIL_IS_C_STRING_HPP

#include <type_traits>

namespace xtr::detail
{
    // This is long-winded because remove_cv on a const pointer removes const
    // from the pointer, not from the the object being pointed to, so we have
    // to use remove_pointer before applying remove_cv. decay turns arrays
    // into pointers and removes references.
    template<typename T>
    struct is_c_string :
        std::conjunction<
            std::is_pointer<std::decay_t<T>>,
            std::is_same<char, std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>>>
    {
    };

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert(is_c_string<char*>::value);
    static_assert(is_c_string<char*&>::value);
    static_assert(is_c_string<char*&&>::value);
    static_assert(is_c_string<char* const>::value);
    static_assert(is_c_string<const char*>::value);
    static_assert(is_c_string<const char* const>::value);
    static_assert(is_c_string<volatile char*>::value);
    static_assert(is_c_string<volatile char* volatile>::value);
    static_assert(is_c_string<const volatile char* volatile>::value);
    static_assert(is_c_string<char[2]>::value);
    static_assert(is_c_string<const char[2]>::value);
    static_assert(is_c_string<volatile char[2]>::value);
    static_assert(is_c_string<const volatile char[2]>::value);
    static_assert(!is_c_string<int>::value);
    static_assert(!is_c_string<int*>::value);
    static_assert(!is_c_string<int[2]>::value);
#endif
}

#endif

