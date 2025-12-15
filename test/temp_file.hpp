// Copyright 2022 Chris E. Holloway
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

#ifndef XTR_TEST_TEMP_FILE_HPP
#define XTR_TEST_TEMP_FILE_HPP

#include "xtr/detail/file_descriptor.hpp"

#include <catch2/catch.hpp>

#include <string>

#include <stdlib.h>
#include <unistd.h>

struct temp_file
{
    temp_file()
    {
        REQUIRE(fd_);
    }

    ~temp_file()
    {
        ::unlink(path_.c_str());
    }

    static std::string get_tmpdir()
    {
        if (const char* tmpdir = ::getenv("TMPDIR"))
            return tmpdir;
        return "/tmp";
    }

    std::string path_ = get_tmpdir() + "/xtr.test.XXXXXX";
    xtr::detail::file_descriptor fd_{::mkstemp(path_.data())};
};

#endif
