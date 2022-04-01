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

#include "xtr/config.hpp"
#include "xtr/io/fd_storage.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"
#include "xtr/io/posix_fd_storage.hpp"

#include "xtr/detail/retry.hpp"
#include "xtr/detail/throw.hpp"

#include <cerrno>
#include <memory>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(const char* path)
{
    const int fd =
        XTR_TEMP_FAILURE_RETRY(
            ::open(
                path,
                O_CREAT|O_APPEND|O_WRONLY,
                S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH));

    if (fd == -1)
        detail::throw_system_error_fmt(errno, "Failed to open `%s'", path);

    return make_fd_storage(fd, path);
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(FILE* fp, std::string reopen_path)
{
    return make_fd_storage(::fileno(fp), std::move(reopen_path));
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(int fd, std::string reopen_path)
{
#if XTR_USE_IO_URING
    errno = 0;
    (void)syscall(__NR_io_uring_setup, 0, nullptr);

    if (errno != ENOSYS)
        return std::make_unique<io_uring_fd_storage>(fd, std::move(reopen_path));
    else
#endif
        return std::make_unique<posix_fd_storage>(fd, std::move(reopen_path));
}
