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

#include "xtr/io/posix_fd_storage.hpp"
#include "xtr/detail/retry.hpp"
#include "xtr/detail/throw.hpp"

#include <cerrno>
#include <memory>
#include <utility>

#include <unistd.h>

XTR_FUNC
xtr::posix_fd_storage::posix_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity)
:
    fd_storage_base(fd, std::move(reopen_path)),
    buf_(new char[buffer_capacity]),
    buffer_capacity_(buffer_capacity)
{
}

XTR_FUNC
std::span<char> xtr::posix_fd_storage::allocate_buffer()
{
    return {buf_.get(), buffer_capacity_};
}

XTR_FUNC
void xtr::posix_fd_storage::submit_buffer(char* buf, std::size_t size)
{
    while (size > 0)
    {
        const ::ssize_t nwritten =
            XTR_TEMP_FAILURE_RETRY(::write(fd_.get(), buf, size));
        if (nwritten == -1)
        {
            detail::throw_system_error_fmt(
                errno,
                "xtr::posix_fd_storage::submit_buffer: write failed");
            return;
        }
        size -= std::size_t(nwritten);
        buf += std::size_t(nwritten);
    }
}
