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

#ifndef XTR_DETAIL_VCOPY_WRAPPER_HPP
#define XTR_DETAIL_VCOPY_WRAPPER_HPP

#include <cstddef>
#include <type_traits>

namespace xtr::detail
{
    template<typename T>
    struct vcopy_wrapper
    {
        // Note that vcopy() requires T to be trivially copyable, which also
        // implies trivially destructible.
        const T& value;
        std::size_t size;
    };

    template<typename T>
    struct is_vcopy_wrapper : std::false_type
    {
    };

    template<typename T>
    struct is_vcopy_wrapper<vcopy_wrapper<T>> : std::true_type
    {
    };

    static_assert(is_vcopy_wrapper<vcopy_wrapper<int>>::value);
    static_assert(!is_vcopy_wrapper<int>::value);
}

#endif
