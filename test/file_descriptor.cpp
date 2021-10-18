// Copyright 2014, 2015, 2019, 2020 Chris E. Holloway
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

#include "xtr/detail/file_descriptor.hpp"

#include <catch2/catch.hpp>

#include <utility>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>

namespace
{
    bool is_fd_open(int fd)
    {
        return ::read(fd, nullptr, 0) == 0;
    }
}

TEST_CASE("file_descriptor construct from fd and destruct", "[file_descriptor]")
{
    int saved_fd;
    {
        xtr::detail::file_descriptor fd(::open("/dev/null", O_RDWR));
        saved_fd = fd.get();
    }
    REQUIRE(!is_fd_open(saved_fd));
}

TEST_CASE("file_descriptor construct from path and destruct", "[file_descriptor]")
{
    int saved_fd;
    {
        xtr::detail::file_descriptor fd("/dev/null", O_RDWR);
        saved_fd = fd.get();
        REQUIRE(fd);
        REQUIRE(fd.is_open());
    }
    REQUIRE(!is_fd_open(saved_fd));
}

TEST_CASE("file_descriptor move constructor", "[file_descriptor]")
{
    xtr::detail::file_descriptor fd("/dev/null", O_RDWR);

    const int saved_fd = fd.get();

    REQUIRE(fd);
    REQUIRE(fd.is_open());

    xtr::detail::file_descriptor fd2(std::move(fd));

    REQUIRE(!fd);
    REQUIRE(!fd.is_open());

    REQUIRE(fd2);
    REQUIRE(fd2.is_open());

    REQUIRE(fd2.get() == saved_fd);

    xtr::detail::file_descriptor fd3(std::move(fd2));

    REQUIRE(!fd2);
    REQUIRE(!fd2.is_open());

    REQUIRE(fd3);
    REQUIRE(fd3.is_open());

    REQUIRE(fd3.get() == saved_fd);

    REQUIRE(is_fd_open(fd3.get()));
}

TEST_CASE("file_descriptor move assignment", "[file_descriptor]")
{
    xtr::detail::file_descriptor fd("/dev/null", O_RDWR);

    const int saved_fd = fd.get();

    REQUIRE(fd);
    REQUIRE(fd.is_open());

    xtr::detail::file_descriptor fd2;
    fd2 = std::move(fd);

    REQUIRE(!fd);
    REQUIRE(!fd.is_open());

    REQUIRE(fd2);
    REQUIRE(fd2.is_open());

    REQUIRE(fd2.get() == saved_fd);

    xtr::detail::file_descriptor fd3;
    fd3 = std::move(fd2);

    REQUIRE(!fd2);
    REQUIRE(!fd2.is_open());

    REQUIRE(fd3);
    REQUIRE(fd3.is_open());

    REQUIRE(fd3.get() == saved_fd);

    REQUIRE(is_fd_open(fd3.get()));
}

TEST_CASE("file_descriptor reset and release", "[file_descriptor]")
{
    xtr::detail::file_descriptor fd("/dev/null", O_RDWR);

    const int saved_fd = fd.get();

    REQUIRE(fd);
    REQUIRE(fd.is_open());

    fd.reset(saved_fd + 1);

    REQUIRE(!is_fd_open(saved_fd));
    REQUIRE(fd);
    REQUIRE(fd.is_open());

    fd.release();
    REQUIRE(!fd);
    REQUIRE(!fd.is_open());
}

TEST_CASE("file_descriptor swap", "[file_descriptor]")
{
    xtr::detail::file_descriptor fd("/dev/null", O_RDWR);

    const int saved_fd = fd.get();

    xtr::detail::file_descriptor fd2;

    using std::swap;
    swap(fd, fd2);

    REQUIRE(!fd);
    REQUIRE(!fd.is_open());

    REQUIRE(fd2);
    REQUIRE(fd2.is_open());

    REQUIRE(fd2.get() == saved_fd);

    swap(fd, fd2);

    REQUIRE(!fd2);
    REQUIRE(!fd2.is_open());

    REQUIRE(fd);
    REQUIRE(fd.is_open());

    REQUIRE(fd.get() == saved_fd);
}

#if __cpp_exceptions
TEST_CASE("file_descriptor open error", "[file_descriptor]")
{
    REQUIRE_THROWS_AS(xtr::detail::file_descriptor("/", O_RDWR), std::system_error);
}
#endif

