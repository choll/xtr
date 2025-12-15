// Copyright 2020 Chris E. Holloway
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

#include "xtr/detail/align.hpp"

#include <catch2/catch.hpp>

namespace xtrd = xtr::detail;

TEST_CASE("align", "[align]")
{
    REQUIRE(xtrd::align(0U, 8U) == 0);
    REQUIRE(xtrd::align(1U, 8U) == 8);
    REQUIRE(xtrd::align(2U, 8U) == 8);
    REQUIRE(xtrd::align(3U, 8U) == 8);
    REQUIRE(xtrd::align(4U, 8U) == 8);
    REQUIRE(xtrd::align(5U, 8U) == 8);
    REQUIRE(xtrd::align(6U, 8U) == 8);
    REQUIRE(xtrd::align(7U, 8U) == 8);
    REQUIRE(xtrd::align(8U, 8U) == 8);
    REQUIRE(xtrd::align(9U, 8U) == 16);
    REQUIRE(xtrd::align(10U, 8U) == 16);
    REQUIRE(xtrd::align(11U, 8U) == 16);
    REQUIRE(xtrd::align(12U, 8U) == 16);
    REQUIRE(xtrd::align(13U, 8U) == 16);
    REQUIRE(xtrd::align(14U, 8U) == 16);
    REQUIRE(xtrd::align(15U, 8U) == 16);
    REQUIRE(xtrd::align(16U, 8U) == 16);

    for (std::size_t i = 1, j = 8; i < 10000; ++i)
    {
        REQUIRE(xtrd::align(i, 8UL) == j);
        if (i % 8 == 0)
            j += 8;
    }
}

TEST_CASE("ptr align", "[align]")
{
    REQUIRE(
        xtrd::align<2>(reinterpret_cast<char*>(0)) == reinterpret_cast<char*>(0));
    REQUIRE(
        xtrd::align<4>(reinterpret_cast<char*>(0)) == reinterpret_cast<char*>(0));
    REQUIRE(
        xtrd::align<8>(reinterpret_cast<char*>(0)) == reinterpret_cast<char*>(0));
    REQUIRE(
        xtrd::align<16>(reinterpret_cast<char*>(0)) == reinterpret_cast<char*>(0));

    REQUIRE(
        xtrd::align<2>(reinterpret_cast<char*>(3)) == reinterpret_cast<char*>(4));
    REQUIRE(
        xtrd::align<4>(reinterpret_cast<char*>(5)) == reinterpret_cast<char*>(8));
    REQUIRE(
        xtrd::align<8>(reinterpret_cast<char*>(9)) == reinterpret_cast<char*>(16));
    REQUIRE(
        xtrd::align<16>(reinterpret_cast<char*>(17)) ==
        reinterpret_cast<char*>(32));
}
