// Copyright 2015, 2019 Chris E. Holloway
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

#ifndef XTR_DETAIL_ASSERT_HPP
#define XTR_DETAIL_ASSERT_HPP

#include <cstdio>
#include <cstdlib>

#define XTR_ASSERT_ALWAYS(expr) \
    (__builtin_expect(!(expr), 0) ? \
        (static_cast<void>(std::printf( \
            "Assertion failed: (%s), function %s, file %s, line %u.\n", \
            __STRING(expr), \
            __PRETTY_FUNCTION__, \
            __FILE__, \
            __LINE__)), \
            abort()) : \
        static_cast<void>(0))

#endif

