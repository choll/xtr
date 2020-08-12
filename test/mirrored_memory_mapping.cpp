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

#include "xtr/detail/mirrored_memory_mapping.hpp"
#include "xtr/detail/pagesize.hpp"

#include <catch2/catch.hpp>

#include <stdexcept>
#include <utility>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

namespace xtrd = xtr::detail;

namespace
{
    void test_mirroring(xtrd::mirrored_memory_mapping& m)
    {
        volatile unsigned int* first = reinterpret_cast<unsigned int*>(m.get());
        volatile unsigned int* second =
            reinterpret_cast<unsigned int*>(m.get() + m.length());

        *first = 0xDEADBEEF;
        REQUIRE(*second == 0xDEADBEEF);

        *first = 0xC0DECAFE;
        REQUIRE(*second == 0xC0DECAFE);

        *second = 0xDEADBEEF;
        REQUIRE(*first == 0xDEADBEEF);

        *second = 0xC0DECAFE;
        REQUIRE(*first == 0xC0DECAFE);

        REQUIRE(m.length() % sizeof(int) == 0);

        for (
            volatile unsigned int* it1 = first, *it2 = second, *end1 = second;
            it1 != end1;
            ++it1, ++it2)
        {
            *it1 = unsigned(it1 - first);
            REQUIRE(*it1 == *it2);
        }

        for (
            volatile unsigned int* it1 = first, *it2 = second, *end1 = second;
            it1 != end1;
            ++it1, ++it2)
        {
            *it2 = unsigned(it2 - second);
            REQUIRE(*it1 == *it2);
        }
    }
}

TEST_CASE("mirrored_memory_mapping anonymous mapping", "[mirrored_memory_mapping]")
{
    const std::size_t len = xtrd::align_to_page_size(1);

    xtrd::mirrored_memory_mapping m(len);

    test_mirroring(m);
}

TEST_CASE("mirrored_memory_mapping file mapping", "[mirrored_memory_mapping]")
{
    char path[] = "xtrd.mirrored_memory_mapping_test.XXXXXX";
    const int fd = ::mkstemp(path);
    REQUIRE(fd != -1);
    ::unlink(path);

    const std::size_t len = xtrd::align_to_page_size(1);

    REQUIRE(::ftruncate(fd, ::off_t(len)) != -1);

    xtrd::mirrored_memory_mapping m(len, fd);

    test_mirroring(m);

    ::close(fd);
}

#if __cpp_exceptions
TEST_CASE("mirrored_memory_mapping size not page aligned", "[mirrored_memory_mapping]")
{
    REQUIRE_THROWS_AS(xtrd::mirrored_memory_mapping(1), std::invalid_argument);
    REQUIRE_THROWS_WITH(
        xtrd::mirrored_memory_mapping(1),
        Catch::Matchers::Contains("not page-aligned"));
}
#endif

TEST_CASE("mirrored_memory_mapping move constructor", "[mirrored_memory_mapping]")
{
    xtrd::mirrored_memory_mapping m1(xtrd::align_to_page_size(1));

    void* const saved_buf = m1.get();
    const std::size_t saved_length = m1.length();

    xtrd::mirrored_memory_mapping m2(std::move(m1));
    REQUIRE(!m1);
    REQUIRE(m2);

    REQUIRE(m2.get() == saved_buf);
    REQUIRE(m2.length() == saved_length);

    xtrd::mirrored_memory_mapping m3(std::move(m2));
    REQUIRE(!m2);
    REQUIRE(m3);

    REQUIRE(m3.get() == saved_buf);
    REQUIRE(m3.length() == saved_length);
}

TEST_CASE("mirrored_memory_mapping move assignment", "[mirrored_memory_mapping]")
{
    xtrd::mirrored_memory_mapping m1(xtrd::align_to_page_size(1));

    void* const saved_buf = m1.get();
    const std::size_t saved_length = m1.length();

    xtrd::mirrored_memory_mapping m2;
    m2 = std::move(m1);
    REQUIRE(!m1);
    REQUIRE(m2);

    REQUIRE(m2.get() == saved_buf);
    REQUIRE(m2.length() == saved_length);

    xtrd::mirrored_memory_mapping m3;
    m3 = std::move(m2);
    REQUIRE(!m2);
    REQUIRE(m3);

    REQUIRE(m3.get() == saved_buf);
    REQUIRE(m3.length() == saved_length);
}

TEST_CASE("mirrored_memory_mapping swap", "[mirrored_memory_mapping]")
{
    xtrd::mirrored_memory_mapping m1(xtrd::align_to_page_size(1));

    void* const saved_buf = m1.get();
    const std::size_t saved_length = m1.length();

    xtrd::mirrored_memory_mapping m2;

    using std::swap;
    swap(m1, m2);

    REQUIRE(!m1);
    REQUIRE(m2);

    REQUIRE(m2.get() == saved_buf);
    REQUIRE(m2.length() == saved_length);

    swap(m1, m2);

    REQUIRE(!m2);
    REQUIRE(m1);

    REQUIRE(m1.get() == saved_buf);
    REQUIRE(m1.length() == saved_length);
}

