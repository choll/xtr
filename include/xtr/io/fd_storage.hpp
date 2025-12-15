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

#ifndef XTR_IO_FD_STORAGE_HPP
#define XTR_IO_FD_STORAGE_HPP

#include "xtr/io/storage_interface.hpp"

#include <cstddef>
#include <string>

namespace xtr
{
    /**
     * Creates a storage interface object from a path. If the host kernel supports
     * <a href="https://www.man7.org/linux/man-pages/man7/io_uring.7.html">io_uring(7)</a>
     * and libxtr was built on a machine with liburing header files available then
     * an instance of @ref io_uring_fd_storage will be created, otherwise an instance
     * of @ref posix_fd_storage will be created. To prevent @ref io_uring_fd_storage
     * from being used define set XTR_USE_IO_URING to 0 in xtr/config.hpp.
     */
    storage_interface_ptr make_fd_storage(const char* path);

    /**
     * Creates a storage interface object from a file descriptor and reopen
     * path. Either an instance of @ref io_uring_fd_storage or @ref
     * posix_fd_storage will be created, refer to @ref make_fd_storage(const
     * char*) for details.
     *
     * @param fd: File handle to write to. The underlying file descriptor will
     * be duplicated via a call to <a
     * href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     * so callers may close the file handle immediately after this function
     * returns if desired.
     *
     * @param reopen_path: The path of the file associated with the fp argument.
     * This path will be used to reopen the file if requested via the xtrctl <a
     * href="xtrctl.html#reopening-log-files">reopen command</a>. Pass @ref
     * null_reopen_path if no filename is associated with the file handle.
     */
    storage_interface_ptr make_fd_storage(
        FILE* fp, std::string reopen_path = null_reopen_path);

    /**
     * Creates a storage interface object from a file descriptor and reopen path.
     * Either an instance of @ref io_uring_fd_storage or @ref posix_fd_storage
     * will be created, refer to @ref make_fd_storage(const char*) for details.
     *
     * @param fd: File descriptor to write to. This will be duplicated via a call
     * to <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     * so callers may close the file descriptor immediately after this function returns if desired.
     *
     * @param reopen_path: The path of the file associated with the fd argument.
     * This path will be used to reopen the file if requested via the xtrctl <a
     * href="xtrctl.html#reopening-log-files">reopen command</a>. Pass @ref null_reopen_path
     * if no filename is associated with the file descriptor.
     */
    storage_interface_ptr make_fd_storage(
        int fd, std::string reopen_path = null_reopen_path);
}

#endif
