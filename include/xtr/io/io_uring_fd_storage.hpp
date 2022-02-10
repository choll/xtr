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

#ifndef XTR_IO_IO_URING_FD_STORAGE_HPP
#define XTR_IO_IO_URING_FD_STORAGE_HPP

#include "detail/fd_storage_base.hpp"
#include "xtr/detail/align.hpp"
#include "xtr/detail/throw.hpp"

#include <liburing.h>

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace xtr
{
    class io_uring_fd_storage;
}

class xtr::io_uring_fd_storage : public detail::fd_storage_base
{
public:
    static constexpr std::size_t default_buffer_capacity = 64 * 1024;
    static constexpr std::size_t default_queue_size = 1024;
    static constexpr std::size_t default_batch_size = 32;

private:
    struct buffer
    {
        int index_; // io_uring_prep_write_fixed accepts indexes as int
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

public:
    explicit io_uring_fd_storage(
        int fd,
        std::string reopen_path = null_reopen_path,
        std::size_t buffer_capacity = default_buffer_capacity,
        std::size_t queue_size = default_queue_size,
        std::size_t batch_size = default_batch_size)
    :
        fd_storage_base(fd, std::move(reopen_path)),
        buffer_capacity_(buffer_capacity),
        batch_size_(batch_size)
    {
        if (buffer_capacity > std::numeric_limits<decltype(io_uring_cqe::res)>::max())
            detail::throw_invalid_argument("buffer_capacity too large");

        if (queue_size > std::numeric_limits<decltype(buffer::index_)>::max())
            detail::throw_invalid_argument("queue_size too large");

        if (const int errnum =
            ::io_uring_queue_init(unsigned(queue_size), &ring_, /* flags = */ 0))
        {
            detail::throw_system_error_fmt(
                errnum,
                "xtr::io_uring_fd_storage::io_uring_fd_storage: "
                "io_uring_queue_init failed");
        }

        allocate_buffers(queue_size);
    }

    ~io_uring_fd_storage()
    {
        sync();
        ::io_uring_queue_exit(&ring_);
    }

    void sync() override final
    {
        while (pending_cqe_count_ > 0)
            wait_for_one_cqe();
        fd_storage_base::sync();
    }

protected:
    std::span<char> allocate_buffer() override final
    {
        if (free_list_ == nullptr)
            wait_for_one_cqe();

        assert(free_list_ != nullptr);

        buffer* buf = free_list_;
        free_list_ = free_list_->next_;

        buf->size_ = 0;
        buf->offset_ = 0;
        buf->file_offset_ = offset_;

        return {buf->data_, buffer_capacity_};
    }

    void submit_buffer(char* data, std::size_t size, bool flushed) override final
    {
        buffer* buf = buffer::data_to_buffer(data);
        buf->size_ = unsigned(size);

        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);

        io_uring_prep_write_fixed(
            sqe, fd_.get(), buf->data_, buf->size_, buf->file_offset_, buf->index_);
        io_uring_sqe_set_data(sqe, buf);

        offset_ += size;
        ++pending_cqe_count_;

        if (flushed || ++batch_index_ % batch_size_ == 0)
            io_uring_submit(&ring_);
    }

private:
    void allocate_buffers(std::size_t queue_size)
    {
        assert(queue_size > 0);

        std::vector<::iovec> iov;
        iov.reserve(queue_size);

        buffer** next = &free_list_;

        buffer_storage_ =
            std::make_unique_for_overwrite<std::byte[]>(
                buffer::size(buffer_capacity_) * queue_size);

        for (std::size_t i = 0; i < queue_size; ++i)
        {
            std::byte* storage =
                buffer_storage_.get() + buffer::size(buffer_capacity_) * i;
            buffer* buf = ::new (storage) buffer;
            buf->index_ = int(i);
            iov.push_back({buf->data_, buffer_capacity_});
            // Push the buffer to the end of free_list_:
            *next = buf;
            next = &buf->next_;
        }

        *next = nullptr;
        assert(free_list_ != nullptr);

        io_uring_register_buffers(&ring_, &iov[0], unsigned(iov.size()));
    }

    void wait_for_one_cqe()
    {
        assert(pending_cqe_count_ > 0);

        struct io_uring_cqe* cqe = nullptr;
        int errnum;

    retry:
        while ((errnum = io_uring_peek_cqe(&ring_, &cqe)) == -EAGAIN)
            ;

        if (errnum != 0) [[unlikely]]
        {
            detail::throw_system_error(
                -errnum,
                "xtr::io_uring_fd_storage::wait_for_one_cqe: io_uring_peek_cqe failed");
        }

        --pending_cqe_count_;

        buffer* buf = static_cast<buffer*>(io_uring_cqe_get_data(cqe));

        if (cqe->res == -EAGAIN)
        {
            resubmit_buffer(buf, 0);
            goto retry;
        }

        if (cqe->res < 0) [[unlikely]]
        {
            detail::throw_system_error(
                -cqe->res,
                "xtr::io_uring_fd_storage::wait_for_one_cqe: write error");
        }

        const auto nwritten = unsigned(cqe->res);

        assert(nwritten <= buf->size_);

        if (nwritten != buf->size_) [[unlikely]] // Short write
        {
            resubmit_buffer(buf, nwritten);
            goto retry;
        }

        // Push the reclaimed buffer to the front of free_list_
        assert(buf != nullptr);
        buf->next_ = free_list_;
        free_list_ = buf;

        io_uring_cqe_seen(&ring_, cqe);
    }

    void resubmit_buffer(buffer* buf, unsigned nwritten)
    {
        buf->size_ -= nwritten;
        buf->offset_ += nwritten;
        buf->file_offset_ += nwritten;

        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);

        io_uring_prep_write_fixed(
            sqe,
            fd_.get(),
            buf->data_ + buf->offset_,
            buf->size_,
            buf->file_offset_,
            buf->index_);

        io_uring_sqe_set_data(sqe, buf);

        ++pending_cqe_count_;
    }

    io_uring ring_;
    std::size_t buffer_capacity_;
    std::size_t batch_size_;
    std::size_t batch_index_ = 0;
    std::size_t pending_cqe_count_ = 0;
    std::size_t offset_ = 0;
    buffer* free_list_;
    std::unique_ptr<std::byte[]> buffer_storage_;
};

#endif
