#include "xtr/detail/throw.hpp"

#include <catch2/catch.hpp>

#if __cpp_exceptions
TEST_CASE("system_error", "[throw]")
{
    REQUIRE_THROWS_AS(xtr::detail::throw_system_error(""), std::system_error);
}

TEST_CASE("invalid_argument", "[throw]")
{
    REQUIRE_THROWS_AS(xtr::detail::throw_invalid_argument(""), std::invalid_argument);
}
#endif

