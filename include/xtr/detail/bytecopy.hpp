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

#ifndef XTR_DETAIL_BYTECOPY_HPP
#define XTR_DETAIL_BYTECOPY_HPP

#include <cassert>
#include <cstddef>
#include <cstring>

namespace xtr::detail
{
    // Performs a branch-free copy of n bytes using two overlapping copies of N
    // bytes. For example copying 24 bytes using two 16 byte blocks:
    //
    // byte     0       8       16      24
    //          |       |       |       |
    // block 1  [--------------]
    // block 2          [--------------]
    // n        [----------------------]
    //                  ^^^^^^^^
    //                  copied twice
    //
    // __builtin_memcpy is used as a convenient way of performing a fixed size
    // unaligned load/store (i.e. as N is known at compile time we assume the
    // compiler replaces it with a single load/store).
    template<std::size_t N>
    __attribute__((always_inline)) inline void copy_overlapping(
        std::byte* dst, const std::byte* src, std::size_t n)
    {
        assert(n >= N);
        assert(n <= 2 * N);
        __builtin_memcpy(dst, src, N);
        __builtin_memcpy(dst + n - N, src + n - N, N);
    }

    struct copy_inline_t
    {
    };

    struct copy_memcpy_t
    {
    };

    inline constexpr copy_inline_t copy_inline{};
    inline constexpr copy_memcpy_t copy_memcpy{};

    __attribute__((always_inline)) inline void bytecopy(
        void* dst, const void* src, std::size_t n, copy_memcpy_t)
    {
        std::memcpy(dst, src, n);
    }

    __attribute__((always_inline)) inline void bytecopy(
        void* dst_v, const void* src_v, std::size_t n, copy_inline_t)
    {
        auto* dst = static_cast<std::byte*>(dst_v);
        const auto* src = static_cast<const std::byte*>(src_v);

        if (n > 16)
        {
            if (n <= 32)
                copy_overlapping<16>(dst, src, n);
            else if (n <= 64)
                copy_overlapping<32>(dst, src, n);
            else
                std::memcpy(dst, src, n);
        }
        else if (n >= 4)
        {
            if (n <= 8)
                copy_overlapping<4>(dst, src, n);
            else
                copy_overlapping<8>(dst, src, n);
        }
        else if (n > 0)
        {
            // Copy lengths of 1, 2 or 3
            dst[0] = src[0];
            dst[n / 2] = src[n / 2];
            dst[n - 1] = src[n - 1];
        }
    }
}

#endif
