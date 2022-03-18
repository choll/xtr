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

#include "xtr/detail/buffer.hpp"
#include "xtr/detail/clock_ids.hpp"
#include "xtr/detail/get_time.hpp"

#include <fmt/format.h>

#include <span>
#include <stdexcept>
#include <utility>

xtr::detail::buffer::buffer(storage_interface_ptr storage, log_level_style_t ls)
:
    lstyle(ls),
    storage_(std::move(storage))
{
}

xtr::detail::buffer::~buffer()
{
    flush();
}

void xtr::detail::buffer::flush() noexcept
{
#if __cpp_exceptions
    try
    {
#endif
        if (pos_ != begin_)
        {
            storage_->submit_buffer(begin_, std::size_t(pos_ - begin_));
            pos_ = begin_ = end_ = nullptr;
        }
        if (storage_)
            storage_->flush();
#if __cpp_exceptions
    }
    catch (const std::exception& e)
    {
        using namespace std::literals::string_view_literals;
        fmt::print(
            stderr,
            "{}{}: Error flushing log: {}\n"sv,
            lstyle(log_level_t::error),
            detail::get_time<XTR_CLOCK_WALL>(),
            e.what());
    }
#endif
}

template<typename InputIterator>
void xtr::detail::buffer::append(InputIterator first, InputIterator last)
{
    while (first != last)
    {
        if (pos_ == end_) [[unlikely]]
            next_buffer();

        const auto n = std::min(last - first, end_ - pos_);

        // Safe as the input iterator is contiguous
        std::memcpy(pos_, &*first, std::size_t(n));

        pos_ += n;
        first += n;
    }
}

// This class could support zero-copy I/O, with fmt writing directly into it
// via a push_back method. Instead fmt writes into a std::string (the line
// member of this class), then the function below copies the string into this
// buffer. This slightly odd arrangement is done because it is around 1.5x
// faster than push_back. This is because fmt inspects the iterator passed to
// it, then if it is a std::back_insert_iterator containing a certain type like
// std::string, it extracts the string object and writes to it in a way that is
// more efficient than using push_back. There is also type trait that can be
// defined declaring the input type as contiguous, however the interface to it
// is incompatible with io_uring_fd_storage because it expects to be able to
// resize the buffer, while io_uring_fd_storage operates on a sequence of
// fixed-sized buffers.
//
// TL;DR: Some weirdness is traded for a gain in performance.
void xtr::detail::buffer::append_line()
{
    append(line.begin(), line.end());
    line.clear();
}

void xtr::detail::buffer::next_buffer()
{
    if (pos_ != begin_) [[likely]] // if not the first call to push_back
        storage_->submit_buffer(begin_, std::size_t(pos_ - begin_));
    const std::span<char> s = storage_->allocate_buffer();
    begin_ = s.data();
    end_ = begin_ + s.size();
    pos_ = begin_;
}
