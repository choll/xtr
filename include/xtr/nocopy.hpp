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

#include "detail/concepts.hpp"
#include "detail/nocopy_wrapper.hpp"
#include "detail/string_ref.hpp"

namespace xtr
{
    // XXX review comment
    /**
     * nocopy is used to specify that an argument should be passed by
     * reference instead of by value, so that `arg` becomes `nocopy(arg)`.
     * Note that by default, strings (including C strings and
     * std::string_view), containers and tuples are copied. When wrapped in a
     * call to nocopy, only a pointer to a string's data is copied, and any
     * other argument (e.g. a container or tuple) is passed as a reference to
     * the object. The user is then responsible for ensuring that the data
     * remains valid long enough for the logger to process the log statement.
     * Please see the <a
     * href="guide.html#passing-arguments-by-value-or-reference"> passing
     * arguments by value or reference</a> and <a href="guide.html#string-arguments">string
     * arguments</a> sections of the user guide for further details.
     */
    template<typename T>
        requires detail::string_like<T>
    inline auto nocopy(const T& arg)
    {
        return detail::string_ref(arg);
    }

    template<typename T>
        requires(!detail::string_like<T>)
    inline auto nocopy(const T& arg)
    {
        return detail::nocopy_wrapper<T>{arg};
    }

    template<typename T>
        requires(!detail::string_like<T>)
    auto nocopy(const T&&) = delete;

    auto nocopy(const std::string&&) = delete;
}

#endif
