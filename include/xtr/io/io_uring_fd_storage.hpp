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

#ifndef XTR_IO_IO_URING_FD_STORAGE_HPP
#define XTR_IO_IO_URING_FD_STORAGE_HPP

#include "xtr/config.hpp"

#if XTR_USE_IO_URING
#include "detail/fd_storage_base.hpp"
#include "xtr/detail/align.hpp"

#include <liburing.h>

#include <cstddef>
#include <memory>
#include <span>
#include <string>

namespace xtr
{
    class io_uring_fd_storage;
}

/**
 * An implementation of @ref storage_interface that uses <a
 * href="https://www.man7.org/linux/man-pages/man7/io_uring.7.html">io_uring(7)</a>
 * to perform file I/O (Linux only).
 */
class xtr::io_uring_fd_storage : public detail::fd_storage_base
{
public:
    /**
     * Default value for the buffer_capacity constructor argument.
     */
    static constexpr std::size_t default_buffer_capacity = 64UL * 1024UL;

    /**
     * Default value for the queue_size constructor argument.
     */
    static constexpr std::size_t default_queue_size = 1024UL;

    /**
     * Default value for the batch_size constructor argument.
     */
    static constexpr std::size_t default_batch_size = 32UL;

private:
    struct buffer
    {
        int index_;     // io_uring_prep_write_fixed accepts indexes as int
        unsigned size_; // io_uring_cqe::res is an unsigned int
        std::size_t offset_;
        std::size_t file_offset_;
        buffer* next_;
        __extension__ char data_[];

        static std::size_t size(std::size_t n)
        {
            return detail::align(offsetof(buffer, data_) + n, alignof(buffer));
        }

        static buffer* data_to_buffer(char* data)
        {
            return reinterpret_cast<buffer*>(data - offsetof(buffer, data_));
        }
    };

protected:
    using io_uring_submit_func_t = decltype(&io_uring_submit);
    using io_uring_get_sqe_func_t = decltype(&io_uring_get_sqe);
    using io_uring_wait_cqe_func_t = decltype(&io_uring_wait_cqe);
    using io_uring_sqring_wait_func_t = decltype(&io_uring_sqring_wait);
    using io_uring_peek_cqe_func_t = decltype(&io_uring_peek_cqe);

    io_uring_fd_storage(
        int fd,
        std::string reopen_path,
        std::size_t buffer_capacity,
        std::size_t queue_size,
        std::size_t batch_size,
        io_uring_submit_func_t io_uring_submit_func,
        io_uring_get_sqe_func_t io_uring_get_sqe_func,
        io_uring_wait_cqe_func_t io_uring_wait_cqe_func,
        io_uring_sqring_wait_func_t io_uring_sqring_wait_func,
        io_uring_peek_cqe_func_t io_uring_peek_cqe_func);

public:
    /**
     * File descriptor constructor.
     *
     * @param fd: File descriptor to write to. This will be duplicated via a
     * call to <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     * so callers may close the file descriptor immediately after this
     * constructor returns if desired.
     *
     * @param reopen_path: The path of the file associated with the fd argument.
     * This path will be used to reopen the file if requested via the xtrctl
     * <a href="xtrctl.html#reopening-log-files">reopen command</a>. Pass @ref
     * null_reopen_path if no filename is associated with the file descriptor.
     *
     * @param buffer_capacity: The size in bytes of a single io_uring buffer.
     *
     * @param queue_size: The size of the io_uring submission queue.
     *
     * @param batch_size: The number of buffers to collect before submitting the
     * buffers to io_uring. If @ref XTR_IO_URING_POLL is set to 1 in
     * xtr/config.hpp then this parameter has no effect.
     */
    explicit io_uring_fd_storage(
        int fd,
        std::string reopen_path = null_reopen_path,
        std::size_t buffer_capacity = default_buffer_capacity,
        std::size_t queue_size = default_queue_size,
        std::size_t batch_size = default_batch_size);

    ~io_uring_fd_storage() override;

    void sync() noexcept final;

    void flush() final;

    std::span<char> allocate_buffer() final;

    void submit_buffer(char* data, std::size_t size) final;

protected:
    void replace_fd(int newfd) noexcept final;

private:
    void allocate_buffers(std::size_t queue_size);

    io_uring_sqe* get_sqe();

    void wait_for_one_cqe();

    void resubmit_buffer(buffer* buf, unsigned nwritten);

    void free_buffer(buffer* buf);

    io_uring ring_;
    std::size_t buffer_capacity_;
    std::size_t batch_size_;
    std::size_t batch_index_ = 0;
    std::size_t pending_cqe_count_ = 0;
    std::size_t offset_ = 0;
    buffer* free_list_;
    std::unique_ptr<std::byte[]> buffer_storage_;
    io_uring_submit_func_t io_uring_submit_func_;
    io_uring_get_sqe_func_t io_uring_get_sqe_func_;
    io_uring_wait_cqe_func_t io_uring_wait_cqe_func_;
    io_uring_sqring_wait_func_t io_uring_sqring_wait_func_;
    io_uring_peek_cqe_func_t io_uring_peek_cqe_func_;
};

#endif
#endif
