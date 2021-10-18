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

#include "xtr/detail/pagesize.hpp"

#include <catch2/catch.hpp>

#include <bit>

TEST_CASE("align_to_page_size", "[pagesize]")
{
    const std::size_t pagesz = xtr::detail::align_to_page_size(1);
    REQUIRE(pagesz >= 4 * 1024);
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
    REQUIRE(std::has_single_bit(pagesz));
#else
    REQUIRE(std::ispow2(pagesz));
#endif
    REQUIRE(pagesz == xtr::detail::align_to_page_size(pagesz));
    REQUIRE(pagesz == xtr::detail::align_to_page_size(pagesz - 1));
    REQUIRE(pagesz * 2 == xtr::detail::align_to_page_size(pagesz + 1));
}

