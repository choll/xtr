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
#include "xtr/detail/file_descriptor.hpp"
#include "xtr/detail/throw.hpp"
#include "xtr/io/detail/open.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"
#include "xtr/io/posix_fd_storage.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <cerrno>
#include <memory>
#include <string_view>
#include <utility>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace xtr::detail
{
#if XTR_USE_IO_URING
    // This writes a single buffer using the io_uring storage backend and
    // checks it is written correctly. This is done to catch issues with
    // missing features on older kernels (like IOSQE_IO_HARDLINK).
    XTR_FUNC
    bool is_io_uring_working()
    {
        constexpr std::string_view magic = "io_uring is working";
        constexpr std::size_t buf_capacity = 32;

        file_descriptor memfd(::memfd_create("xtr_health_check", MFD_CLOEXEC));

        if (!memfd)
            return false;

#if __cpp_exceptions
        try
#endif
        {
            io_uring_fd_storage storage(
                memfd.get(),
                null_reopen_path,
                buf_capacity,
                /* queue_size= */ 2,
                /* batch_size= */ 1);
            const std::span<char> span = storage.allocate_buffer();
            magic.copy(span.data(), magic.size());
            storage.submit_buffer(span.data(), magic.size());
            storage.sync();
        }
#if __cpp_exceptions
        catch (const std::exception&)
        {
            return false;
        }
#endif

        char buf[buf_capacity];
        const ::ssize_t nread = ::pread(memfd.get(), buf, sizeof(buf), 0);
        return nread >= 0 && std::string_view(buf, std::size_t(nread)) == magic;
    }
#endif

    storage_interface_ptr make_fd_storage(
        int fd, std::string reopen_path, bool fd_created);
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(const char* path)
{
    const auto fd = detail::open_at_end(path);

    if (!fd)
        detail::throw_system_error_fmt(errno, "Failed to open `%s'", path);

    return detail::make_fd_storage(fd.get(), path, /* fd_created= */ true);
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(FILE* fp, std::string reopen_path)
{
    return make_fd_storage(::fileno(fp), std::move(reopen_path));
}

XTR_FUNC
xtr::storage_interface_ptr xtr::make_fd_storage(int fd, std::string reopen_path)
{
    return detail::make_fd_storage(fd, std::move(reopen_path), /* fd_created= */ false);
}

XTR_FUNC
xtr::storage_interface_ptr xtr::detail::make_fd_storage(
    int fd, std::string reopen_path, bool fd_created)
{
#if XTR_USE_IO_URING
    errno = 0;
    (void)syscall(__NR_io_uring_setup, 0, nullptr);
    const bool has_io_uring = errno != ENOSYS;

    // io_uring is disabled on non-seekable files or file descriptors with
    // O_APPEND set because the io_uring backend relies on writing to specific
    // file offsets. Note that on Linux if a file is opened with O_APPEND then
    // file offsets passed to pwrite and io_uring are ignored.
    if (has_io_uring && detail::is_seekable(fd) && !detail::is_append(fd) &&
        detail::is_io_uring_working())
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

    // If we opened the file descriptor, then set O_APPEND. The O_APPEND flag isn't
    // set at creation time because the io_uring backend needs it to be off. Setting
    // it on the POSIX backend is done to preserve existing behaviour---if it is
    // not set then truncating the file (e.g. via logrotate in copytruncate mode)
    // would result in a hole being created instead of writing from offset zero.
    // For user supplied file descriptors we do not modify file flags.
    if (fd_created && !detail::set_append(fd))
    {
        detail::throw_system_error_fmt(
            errno,
            "Failed to set O_APPEND on `%s'",
            reopen_path.c_str());
    }

    return std::make_unique<posix_fd_storage>(fd, std::move(reopen_path));
}
