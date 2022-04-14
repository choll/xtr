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

#ifndef XTR_IO_STORAGE_INTERFACE_HPP
#define XTR_IO_STORAGE_INTERFACE_HPP

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

namespace xtr
{
    struct storage_interface;

    /**
     * Convenience typedef for std::unique_ptr<@ref storage_interface>
     */
    using storage_interface_ptr = std::unique_ptr<storage_interface>;

    /**
     * When passed to the reopen_path argument of @ref make_fd_storage,
     * @ref posix_fd_storage::posix_fd_storage, @ref io_uring_fd_storage or
     * @ref stream-with-reopen-ctor "logger::logger" indicates that
     * the output file handle has no associated filename and so should not be
     * reopened if requested by the
     * xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     */
    inline constexpr auto null_reopen_path = "";
}

/**
 * Interface allowing custom back-ends to be implemented. To create a custom
 * back-end, inherit from @ref storage_interfance, implement all pure-virtual
 * functions then pass a @ref storage_interface_ptr pointing to an instance of
 * the custom back-end to @ref back-end-ctor "logger::logger".
 */
struct xtr::storage_interface
{
    /**
     * Allocates a buffer for formatted log data to be written to. Once a
     * buffer has been allocated, allocate_buffer will not be called again
     * until the buffer has been submitted via @ref submit_buffer.
     */
    virtual std::span<char> allocate_buffer() = 0;

    /**
     * Submits a buffer containing formatted log data to be written.
     */
    virtual void submit_buffer(char* buf, std::size_t size) = 0;

    /**
     * Invoked to indicate that the back-end should write any buffered data to
     * its associated backing store.
     */
    virtual void flush() = 0;

    /**
     * Invoked to indicate that the back-end should ensure that all data
     * written to the associated backing store has reached permanent storage.
     */
    virtual void sync() noexcept = 0;

    /**
     * Invoked to indicate that if the back-end has a regular file opened for
     * writing log data then the file should be reopened.
     */
    virtual int reopen() noexcept = 0;

    virtual ~storage_interface() = default;
};

#endif
