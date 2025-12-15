// Copyright 2020 Chris E. Holloway
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
    using is_c_string = std::conjunction<
        std::is_pointer<std::decay_t<T>>,
        std::is_same<char, std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>>>;

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
