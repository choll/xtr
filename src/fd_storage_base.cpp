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

#include "xtr/io/detail/fd_storage_base.hpp"
#include "xtr/detail/retry.hpp"
#include "xtr/detail/throw.hpp"

#include <utility>

#include <fcntl.h>
#include <unistd.h>

XTR_FUNC
xtr::detail::fd_storage_base::fd_storage_base(int fd, std::string reopen_path)
:
    reopen_path_(std::move(reopen_path)),
    // The input file descriptor is duplicated so that there is no ambiguity
    // regarding ownership of the fd---we effectively increment a reference
    // count that we are responsible for decrementing later, and the user
    // remains responsible for decrementing their own reference count.
    fd_(::dup(fd))
{
    if (!fd_)
    {
        detail::throw_system_error_fmt(
            errno,
            "xtr::detail::fd_storage_base::fd_storage_base: dup(2) failed");
    }
}

XTR_FUNC
void xtr::detail::fd_storage_base::sync() noexcept
{
    ::fsync(fd_.get());
}

XTR_FUNC
int xtr::detail::fd_storage_base::reopen() noexcept
{
    if (reopen_path_ == null_reopen_path)
        return ENOENT;

    const int newfd =
        XTR_TEMP_FAILURE_RETRY(
            ::open(
                reopen_path_.c_str(),
                O_CREAT|O_APPEND|O_WRONLY,
                S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH));

    if (newfd == -1)
        return errno;

    replace_fd(newfd);

    return 0;
}

XTR_FUNC
void xtr::detail::fd_storage_base::replace_fd(int newfd) noexcept
{
    fd_.reset(newfd);
}
