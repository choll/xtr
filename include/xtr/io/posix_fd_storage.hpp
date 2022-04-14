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

#ifndef XTR_IO_POSIX_FD_STORAGE_HPP
#define XTR_IO_POSIX_FD_STORAGE_HPP

#include "detail/fd_storage_base.hpp"

#include <cstddef>
#include <string>

namespace xtr
{
    class posix_fd_storage;
}

/**
 * An implementation of @ref storage_interface that uses standard
 * <a href="https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html">POSIX</a>
 * functions to perform file I/O.
 */
class xtr::posix_fd_storage : public detail::fd_storage_base
{
public:
    /**
     * Default value for the buffer_capacity constructor argument.
     */
    static constexpr std::size_t default_buffer_capacity = 64 * 1024;

    /**
     * File descriptor constructor.
     *
     * @arg fd: File descriptor to write to. This will be duplicated via a call to
     *          <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     *          so callers may close the file descriptor immediately after this
     *          constructor returns if desired.
     * @arg reopen_path: The path of the file associated with the fd argument.
     *                   This path will be used to reopen the file if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the file
     *                   descriptor.
     * @arg buffer_capacity: The size in bytes of the internal write buffer.
     */
    explicit posix_fd_storage(
        int fd,
        std::string reopen_path = null_reopen_path,
        std::size_t buffer_capacity = default_buffer_capacity);

    std::span<char> allocate_buffer() override final;

    void submit_buffer(char* buf, std::size_t size) override final;

    void flush() override final
    {
    }

private:
    std::unique_ptr<char[]> buf_;
    std::size_t buffer_capacity_;
};

#endif
