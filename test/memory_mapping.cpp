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

#include "xtr/detail/memory_mapping.hpp"

#include <catch2/catch.hpp>

#include <system_error>
#include <utility>

namespace xtrd = xtr::detail;

TEST_CASE("memory_mapping anonymous mapping", "[memory_mapping]")
{
    xtrd::memory_mapping m(
        nullptr,
        1,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, // fd
        0); // offset

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);
    REQUIRE(m.length() == 1);
}

#if __cpp_exceptions
TEST_CASE("memory_mapping zero length mapping", "[memory_mapping]")
{
    REQUIRE_THROWS_AS(
        xtrd::memory_mapping(
            nullptr,
            0,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANON,
            -1,
            0),
        std::system_error);
}
#endif

TEST_CASE("memory_mapping move constructor", "[memory_mapping]")
{
    xtrd::memory_mapping m(
        nullptr,
        1,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, // fd
        0); // offset

    const void* saved_mem = m.get();

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    xtrd::memory_mapping m2(std::move(m));

    REQUIRE(!m);
    REQUIRE(m.get() == MAP_FAILED);

    REQUIRE(m2);
    REQUIRE(m2.get() != MAP_FAILED);

    REQUIRE(m2.get() == saved_mem);

    xtrd::memory_mapping m3(std::move(m2));

    REQUIRE(!m2);
    REQUIRE(m2.get() == MAP_FAILED);

    REQUIRE(m3);
    REQUIRE(m3.get() != MAP_FAILED);

    REQUIRE(m3.get() == saved_mem);
}

TEST_CASE("memory_mapping move assignment", "[memory_mapping]")
{
    xtrd::memory_mapping m(
        nullptr,
        1,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, // fd
        0); // offset

    const void* saved_mem = m.get();

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    xtrd::memory_mapping m2;
    m2 = std::move(m);

    REQUIRE(!m);
    REQUIRE(m.get() == MAP_FAILED);

    REQUIRE(m2);
    REQUIRE(m2.get() != MAP_FAILED);

    REQUIRE(m2.get() == saved_mem);

    xtrd::memory_mapping m3;
    m3 = std::move(m2);

    REQUIRE(!m2);
    REQUIRE(m2.get() == MAP_FAILED);

    REQUIRE(m3);
    REQUIRE(m3.get() != MAP_FAILED);

    REQUIRE(m3.get() == saved_mem);
}

TEST_CASE("memory_mapping reset and release", "[memory_mapping]")
{
    xtrd::memory_mapping m(
        nullptr,
        1,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, // fd
        0); // offset

    const void* saved_mem = m.get();

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    m.reset(&saved_mem);

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    m.release();
    REQUIRE(!m);
    REQUIRE(m.get() == MAP_FAILED);
}

TEST_CASE("memory_mapping swap", "[memory_mapping]")
{
    xtrd::memory_mapping m(
        nullptr,
        1,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, // fd
        0); // offset

    const void* saved_mem = m.get();

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    xtrd::memory_mapping m2;

    using std::swap;
    swap(m, m2);

    REQUIRE(!m);
    REQUIRE(m.get() == MAP_FAILED);

    REQUIRE(m2);
    REQUIRE(m2.get() != MAP_FAILED);

    REQUIRE(m2.get() == saved_mem);

    swap(m, m2);

    REQUIRE(!m2);
    REQUIRE(m2.get() == MAP_FAILED);

    REQUIRE(m);
    REQUIRE(m.get() != MAP_FAILED);

    REQUIRE(m.get() == saved_mem);
}
