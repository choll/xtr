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

#include "xtr/detail/throw.hpp"

#include <catch2/catch.hpp>

#include <stdexcept>
#include <system_error>

#if __cpp_exceptions
TEST_CASE("runtime_error", "[throw]")
{
    REQUIRE_THROWS_AS(xtr::detail::throw_runtime_error(""), std::runtime_error);
}

TEST_CASE("runtime_error_fmt", "[throw]")
{
    REQUIRE_THROWS_WITH(
        xtr::detail::throw_runtime_error_fmt("error text"),
        "error text");
}

TEST_CASE("system_error", "[throw]")
{
    using namespace Catch::Matchers;
    REQUIRE_THROWS_WITH(
        xtr::detail::throw_system_error(EBUSY, "error text"),
        Contains("error text: ") && Contains("busy"));
}

TEST_CASE("system_error_fmt", "[throw]")
{
    using namespace Catch::Matchers;
    REQUIRE_THROWS_WITH(
        xtr::detail::throw_system_error_fmt(EBUSY, "error text %d", 42),
        Contains("error text 42: ") && Contains("busy"));
}

TEST_CASE("invalid_argument", "[throw]")
{
    REQUIRE_THROWS_AS(
        xtr::detail::throw_invalid_argument(""),
        std::invalid_argument);
}

TEST_CASE("bad_alloc", "[throw]")
{
    REQUIRE_THROWS_AS(xtr::detail::throw_bad_alloc(), std::bad_alloc);
}
#endif
