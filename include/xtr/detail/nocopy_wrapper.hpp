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

#ifndef XTR_DETAIL_NOCOPY_WRAPPER_HPP
#define XTR_DETAIL_NOCOPY_WRAPPER_HPP

#include <fmt/core.h>

namespace xtr::detail
{
    // XXX review comment
    // Returned by nocopy for arguments other than strings. Holds a reference
    // to the argument, which is formatted directly by the consumer.
    template<typename T>
    struct nocopy_wrapper
    {
        const T& value;
    };
}

namespace fmt
{
    template<typename T>
    struct formatter<xtr::detail::nocopy_wrapper<T>> : formatter<T>
    {
        template<typename FormatContext>
        auto format(const xtr::detail::nocopy_wrapper<T>& wrapper, FormatContext& ctx) const
        {
            return formatter<T>::format(wrapper.value, ctx);
        }
    };
}

#endif
