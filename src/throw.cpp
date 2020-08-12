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

#include "xtr/detail/throw.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <system_error>

namespace xtrd = xtr::detail;

void xtrd::throw_runtime_error(const char* what)
{
#if __cpp_exceptions
    throw std::runtime_error(what);
#else
    std::fprintf(stderr, "runtime error: %s\n", what);
    std::abort();
#endif
}

void xtrd::throw_system_error(const char* what)
{
#if __cpp_exceptions
    throw std::system_error(std::error_code(errno, std::generic_category()), what);
#else
    std::fprintf(stderr, "system error: %s: %s\n", what, std::strerror(errno));
    std::abort();
#endif
}

void xtrd::throw_invalid_argument(const char* what)
{
#if __cpp_exceptions
    throw std::invalid_argument(what);
#else
    std::fprintf(stderr, "invalid argument: %s\n", what);
    std::abort();
#endif
}

