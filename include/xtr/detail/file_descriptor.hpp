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

#ifndef XTR_DETAIL_FILE_DESCRIPTOR_HPP
#define XTR_DETAIL_FILE_DESCRIPTOR_HPP

#include <utility>

namespace xtr::detail
{
    class file_descriptor;
}

class xtr::detail::file_descriptor
{
public:
    file_descriptor() = default;

    explicit file_descriptor(int fd)
    :
        fd_(fd)
    {
    }

    file_descriptor(const char* path, int flags, int mode = 0);

    file_descriptor(const file_descriptor&) = delete;

    file_descriptor(file_descriptor&& other) noexcept
    :
        fd_(other.fd_)
    {
        other.release();
    }

    file_descriptor& operator=(const file_descriptor&) = delete;

    file_descriptor& operator=(file_descriptor&& other) noexcept;

    ~file_descriptor();

    bool is_open() const noexcept
    {
        return fd_ != -1;
    }

    explicit operator bool() const noexcept
    {
        return is_open();
    }

    void reset(int fd = -1) noexcept;

    int get() const noexcept
    {
        return fd_;
    }

    int release() noexcept
    {
        return std::exchange(fd_, -1);
    }

private:
    int fd_ = -1;

    friend void swap(file_descriptor& a, file_descriptor& b) noexcept
    {
        using std::swap;
        swap(a.fd_, b.fd_);
    }
};

#endif

