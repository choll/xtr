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

#include "xtr/io/io_uring_fd_storage.hpp"
#include "xtr/detail/throw.hpp"

#include <liburing.h>

#include <cassert>
#include <limits>

xtr::io_uring_fd_storage::io_uring_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity,
    std::size_t queue_size,
    std::size_t batch_size)
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

xtr::io_uring_fd_storage::~io_uring_fd_storage()
{
    sync();
    ::io_uring_queue_exit(&ring_);
}

void xtr::io_uring_fd_storage::sync() noexcept
{
    while (pending_cqe_count_ > 0)
        wait_for_one_cqe();
    fd_storage_base::sync();
}

std::span<char> xtr::io_uring_fd_storage::allocate_buffer()
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

void xtr::io_uring_fd_storage::submit_buffer(
    char* data,
    std::size_t size,
    bool flushed)
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

void xtr::io_uring_fd_storage::allocate_buffers(std::size_t queue_size)
{
    assert(queue_size > 0);

    std::vector<::iovec> iov;
    iov.reserve(queue_size);

    buffer** next = &free_list_;

    buffer_storage_.reset(
        new std::byte[buffer::size(buffer_capacity_) * queue_size]);

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

void xtr::io_uring_fd_storage::wait_for_one_cqe()
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

void xtr::io_uring_fd_storage::resubmit_buffer(buffer* buf, unsigned nwritten)
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
