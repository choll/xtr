// Copyright 2014, 2019 Chris E. Holloway
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

#ifndef XTR_DETAIL_ALIGN_HPP
#define XTR_DETAIL_ALIGN_HPP

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail
{
    // value is unchanged if it is already aligned
    template<typename T>
    constexpr T align(T value, T alignment) noexcept
    {
        static_assert(std::is_unsigned_v<T>, "value must be unsigned");
        assert(std::has_single_bit(alignment));
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    // Align is only a template argument to allow it to be used with
    // assume_aligned. FIXME: Make Align a plain argument if it is possible to
    // do so while still marking the return value as aligned.
    template<std::size_t Align, typename T>
    __attribute__((assume_aligned(Align))) T* align(T* ptr) noexcept
    {
        static_assert(
            std::is_same_v<std::remove_cv_t<T>, std::byte> ||
            std::is_same_v<std::remove_cv_t<T>, char> ||
            std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
            std::is_same_v<std::remove_cv_t<T>, signed char>,
            "value must be a char or byte pointer");
        return reinterpret_cast<T*>(align(std::uintptr_t(ptr), Align));
    }
}

#endif

