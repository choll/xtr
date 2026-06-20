// Copyright 2026 Chris E. Holloway
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

#ifndef XTR_NOCOPY_HPP
#define XTR_NOCOPY_HPP

#include "detail/string_ref.hpp"

namespace xtr
{
    /**
     * nocopy is used to specify that a string argument should be passed by
     * reference instead of by value, so that `arg` becomes `nocopy(arg)`. Note
     * that by default, all strings including C strings and std::string_view are
     * copied. In order to pass strings by reference they must be wrapped in a
     * call to nocopy. Please see the <a
     * href="guide.html#passing-arguments-by-value-or-reference"> passing
     * arguments by value or reference</a> and <a href="guide.html#string-arguments">string
     * arguments</a> sections of the user guide for further details.
     */
    template<typename T>
    inline auto nocopy(const T& arg)
    {
        return detail::string_ref(arg);
    }
}

#endif
