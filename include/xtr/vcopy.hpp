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

#ifndef XTR_VCOPY_HPP
#define XTR_VCOPY_HPP

#include "detail/vcopy_wrapper.hpp"

#include <cstddef>
#include <type_traits>

namespace xtr
{
    /**
     * vcopy copies a variable-length trivially-copyable object (e.g. a struct
     * with a flexible array member) into the sink. `size` is the total number
     * of bytes to copy starting at `arg`, which must be at least `sizeof(T)`
     * and must include any trailing data beyond the fixed portion of `T`.
     * Because the entire object is memcpy'd as-is, `T` must be trivially
     * copyable. Unlike string arguments, vcopy cannot truncate: if the sink's
     * ring buffer cannot hold the object, the entire log record is dropped
     * and the dropped-message counter is incremented. Please see the
     * <a href="guide.html#variable-length-arguments">variable-length
     * arguments</a> section of the user guide for further details.
     */
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    inline auto vcopy(const T& arg, std::size_t size)
    {
        return detail::vcopy_wrapper{arg, size};
    }
}

#endif
