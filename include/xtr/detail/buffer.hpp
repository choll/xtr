// Copyright 2022 Chris E. Holloway
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

#ifndef XTR_DETAIL_BUFFER_HPP
#define XTR_DETAIL_BUFFER_HPP

#include "xtr/io/storage_interface.hpp"

#include <fmt/core.h>
#include <fmt/format.h> // XXX

#include <cstddef>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <cassert> // XXX
#include <cstring> // XXX memcpy
#include <iostream> // XXX
#include <iterator> // XXX contiguous
#include <vector> // XXX

namespace xtr::detail
{
    class buffer;
}

// If is_contiguous yields true, then when fmt::format_to receives a
// std::back_insert_iterator<buffer> it will extract the underlying
// container type from the back-inserter, and will use operator[],
// size() and resize() to copy data into the buffer, instead of
// push_back, which increases throughput by around 1.5x.
//template<>
//struct fmt::is_contiguous<xtr::detail::buffer> : std::true_type {};

class xtr::detail::buffer
{
public:
    using value_type = char;

    explicit buffer(storage_interface_ptr storage)
    :
        storage_(std::move(storage))
    {
    }

    buffer(buffer&&) = default;

    ~buffer()
    {
        flush();
    }

#if 1
    void push_back(char c)
    {
        if (pos_ == end_) [[unlikely]]
            next_buffer(/* flushed= */ false);
        *pos_++ = c;
    }
#endif

    template<std::contiguous_iterator InputIterator>
    void append(InputIterator first, InputIterator last)
    {
        while (first != last)
        {
            if (pos_ == end_) [[unlikely]]
                next_buffer(/* flushed= */ false);

            const auto n = std::min(last - first, end_ - pos_);

            // Safe as the input iterator is contiguous
            std::memcpy(pos_, &*first, std::size_t(n));

            pos_ += n;
            first += n;
        }
    }

    void flush()
    {
        if (pos_ != begin_)
            next_buffer(/* flushed= */ true);
    }

    storage_interface& storage() noexcept
    {
        return *storage_;
    }

    std::string line;

    // XXX explain this, 60%
    void append_line()
    {
        append(line.begin(), line.end());
        line.clear();
    }

private:
    void next_buffer(bool flushed)
    {
        if (pos_ != begin_) [[likely]] // if not the first call to push_back
            storage_->submit_buffer(begin_, std::size_t(pos_ - begin_), flushed);
        const std::span<char> s = storage_->allocate_buffer();
        begin_ = s.data();
        end_ = begin_ + s.size();
        pos_ = begin_;
    }

    storage_interface_ptr storage_;
    char* pos_ = nullptr;
    char* begin_ = nullptr;
    char* end_ = nullptr;
};

#endif
