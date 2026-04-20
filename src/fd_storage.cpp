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

#include "xtr/io/fd_storage.hpp"
#include "xtr/config.hpp"
#include "xtr/detail/throw.hpp"
#include "xtr/io/detail/open.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"
#include "xtr/io/posix_fd_storage.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <cerrno>
#include <memory>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace xtr::detail
{
    XTR_FUNC
    bool is_seekable(int fd)
    {
        struct ::stat st;
        if (::fstat(fd, &st) != 0)
            throw_system_error_fmt(errno, "fstat on fd %d failed", fd);
        return S_ISREG(st.st_mode);
    }

    XTR_FUNC
    bool is_append(int fd)
    {
        const int flags = ::fcntl(fd, F_GETFL);
        if (flags == -1)
            throw_system_error_fmt(errno, "fcntl on fd %d failed", fd);
        return flags & O_APPEND;
    }
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(const char* path)
{
    const int fd = detail::open_at_end(path);

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
    const bool has_io_uring = errno != ENOSYS;

    // io_uring is disabled on non-seekable files or file descriptors with
    // O_APPEND set because the io_uring backend relies on writing to specific
    // file offsets. Note that on Linux if a file is opened with O_APPEND then
    // file offsets passed to pwrite and io_uring are ignored.
    if (has_io_uring && detail::is_seekable(fd) && !detail::is_append(fd))
    {
#if __cpp_exceptions
        try
        {
#endif
            return std::make_unique<io_uring_fd_storage>(fd, std::move(reopen_path));
#if __cpp_exceptions
        }
        catch (const std::exception& e)
        {
            fmt::print(
                stderr,
                FMT_COMPILE(
                    "Falling back to posix_fd_storage due to "
                    "io_uring_fd_storage error: {}\n"),
                e.what());
        }
#endif
    }
#endif
    return std::make_unique<posix_fd_storage>(fd, std::move(reopen_path));
}
