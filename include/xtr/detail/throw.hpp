// Copyright 2014, 2015, 2019, 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_THROW_HPP
#define XTR_DETAIL_THROW_HPP

namespace xtr::detail
{
    [[noreturn, gnu::cold]] void throw_runtime_error(const char* what);
    [[noreturn, gnu::cold, gnu::format(printf, 1, 2)]]
    void throw_runtime_error_fmt(const char* format, ...);

    [[noreturn, gnu::cold]] void throw_system_error(int errnum, const char* what);

    [[noreturn, gnu::cold, gnu::format(printf, 2, 3)]]
    void throw_system_error_fmt(int errnum, const char* format, ...);

    [[noreturn, gnu::cold]] void throw_invalid_argument(const char* what);

    [[noreturn, gnu::cold]] void throw_bad_alloc();
}

#endif
