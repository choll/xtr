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

#ifndef XTR_DETAIL_BUFFER_HPP
#define XTR_DETAIL_BUFFER_HPP

#include "xtr/io/storage_interface.hpp"
#include "xtr/log_level.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>

namespace xtr::detail
{
    class buffer;
}

class xtr::detail::buffer
{
public:
    using value_type = char;

    explicit buffer(storage_interface_ptr storage, log_level_style_t ls);

    buffer(buffer&&) = default;

    ~buffer();

    template<typename InputIterator>
    void append(InputIterator first, InputIterator last);

    void flush() noexcept;

    storage_interface& storage() noexcept
    {
        return *storage_;
    }

    void append_line();

    std::string line;
    log_level_style_t lstyle;

private:
    void next_buffer();

    storage_interface_ptr storage_;
    char* pos_ = nullptr;
    char* begin_ = nullptr;
    char* end_ = nullptr;
};

#endif
