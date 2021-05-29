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

#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/retry.hpp"
#include "xtr/detail/throw.hpp"

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

XTR_FUNC
xtr::detail::file_descriptor::file_descriptor(
    const char* path,
    int flags,
    int mode)
:
    fd_(XTR_TEMP_FAILURE_RETRY(::open(path, flags, mode)))
{
    if (fd_ == -1)
    {
        throw_system_error_fmt(
            "xtr::detail::file_descriptor::file_descriptor: "
            "Failed to open `%s'", path);
    }
}

XTR_FUNC
xtr::detail::file_descriptor& xtr::detail::file_descriptor::operator=(
    xtr::detail::file_descriptor&& other) noexcept
{
    swap(*this, other);
    return *this;
}

XTR_FUNC
xtr::detail::file_descriptor::~file_descriptor()
{
    reset();
}

XTR_FUNC
void xtr::detail::file_descriptor::reset(int fd) noexcept
{
    // Note that although close() can fail due to EINTR we do not retry. This
    // is because the state of the file descriptor is unspecified (according to
    // POSIX) which makes retrying dangerous because if the file descriptor is
    // closed then a race condition exists---the file descriptor could have
    // been reused by another thread calling open(), so would be incorrectly
    // closed if close() is retried. For Linux, the file descriptor is always
    // closed.
    if (is_open())
        (void)::close(fd_);
    fd_ = fd;
}

