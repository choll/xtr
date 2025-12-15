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

#if XTR_USE_IO_URING
#include "xtr/io/io_uring_fd_storage.hpp"
#include "xtr/detail/throw.hpp"

#include <liburing.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>

XTR_FUNC
xtr::io_uring_fd_storage::io_uring_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity,
    std::size_t queue_size,
    std::size_t batch_size,
    io_uring_submit_func_t io_uring_submit_func,
    io_uring_get_sqe_func_t io_uring_get_sqe_func,
    io_uring_wait_cqe_func_t io_uring_wait_cqe_func,
    io_uring_sqring_wait_func_t io_uring_sqring_wait_func,
    io_uring_peek_cqe_func_t io_uring_peek_cqe_func)
:
    fd_storage_base(fd, std::move(reopen_path)),
    buffer_capacity_(buffer_capacity),
    batch_size_(batch_size),
    io_uring_submit_func_(io_uring_submit_func),
    io_uring_get_sqe_func_(io_uring_get_sqe_func),
    io_uring_wait_cqe_func_(io_uring_wait_cqe_func),
    io_uring_sqring_wait_func_(io_uring_sqring_wait_func),
    io_uring_peek_cqe_func_(io_uring_peek_cqe_func)
{
    if (buffer_capacity > std::numeric_limits<decltype(io_uring_cqe::res)>::max())
        detail::throw_invalid_argument("buffer_capacity too large");

    if (queue_size > std::numeric_limits<decltype(buffer::index_)>::max())
        detail::throw_invalid_argument("queue_size too large");

    if (batch_size == 0)
        detail::throw_invalid_argument("batch_size cannot be zero");

    const int flags =
#if XTR_IO_URING_POLL
        IORING_SETUP_SQPOLL;
#else
        0;
#endif

    if (const int errnum =
        ::io_uring_queue_init(unsigned(queue_size), &ring_, flags))
    {
        detail::throw_system_error_fmt(
            -errnum,
            "xtr::io_uring_fd_storage::io_uring_fd_storage: "
            "io_uring_queue_init failed");
    }

    allocate_buffers(queue_size);
}

XTR_FUNC
xtr::io_uring_fd_storage::io_uring_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity,
    std::size_t queue_size,
    std::size_t batch_size)
:
    io_uring_fd_storage(
        fd,
        std::move(reopen_path),
        buffer_capacity,
        queue_size,
        batch_size,
        ::io_uring_submit,
        ::io_uring_get_sqe,
        ::io_uring_wait_cqe,
        ::io_uring_sqring_wait,
        ::io_uring_peek_cqe)
{
}

XTR_FUNC
xtr::io_uring_fd_storage::~io_uring_fd_storage()
{
    flush();
    sync();
    ::io_uring_queue_exit(&ring_);
}

XTR_FUNC
void xtr::io_uring_fd_storage::flush()
{
    // SQEs may have been prepared but not submitted, due to batching
    io_uring_submit_func_(&ring_);
}

XTR_FUNC
void xtr::io_uring_fd_storage::sync() noexcept
{
    while (pending_cqe_count_ > 0)
        wait_for_one_cqe();
    fd_storage_base::sync();
}

XTR_FUNC
std::span<char> xtr::io_uring_fd_storage::allocate_buffer()
{
    while (free_list_ == nullptr)
        wait_for_one_cqe();

    buffer* buf = free_list_;
    free_list_ = free_list_->next_;

    buf->size_ = 0;
    buf->offset_ = 0;
    buf->file_offset_ = offset_;

    return {buf->data_, buffer_capacity_};
}

XTR_FUNC
void xtr::io_uring_fd_storage::submit_buffer(char* data, std::size_t size)
{
    buffer* buf = buffer::data_to_buffer(data);
    buf->size_ = unsigned(size);

    io_uring_sqe* sqe = get_sqe();

    // Per https://github.com/axboe/liburing/issues/507#issuecomment-1005869351
    // using seek offsets is preferred over appending (via passing an offset of
    // -1).
    ::io_uring_prep_write_fixed(
        sqe, fd_.get(), buf->data_, buf->size_, buf->file_offset_, buf->index_);

    ::io_uring_sqe_set_data(sqe, buf);

    offset_ += size;
    ++pending_cqe_count_;

    if (++batch_index_ % batch_size_ == 0)
        io_uring_submit_func_(&ring_);
}

XTR_FUNC
void xtr::io_uring_fd_storage::replace_fd(int newfd) noexcept
{
    // As write events might be in-flight, closing the file descriptor must
    // be queued up behind any pending writes. IOSQE_IO_DRAIN is used to
    // ensure that all previous writes complete before closing.
    io_uring_sqe* sqe = get_sqe();
    ::io_uring_prep_close(sqe, fd_.release());
    sqe->flags |= IOSQE_IO_DRAIN;
    ++pending_cqe_count_;
    io_uring_submit_func_(&ring_);

    fd_storage_base::replace_fd(newfd);
}

XTR_FUNC
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

    if (const int errnum =
        ::io_uring_register_buffers(&ring_, &iov[0], unsigned(iov.size())))
    {
        detail::throw_system_error_fmt(
            -errnum,
            "xtr::io_uring_fd_storage::allocate_buffers: "
            "io_uring_register_buffers failed");
    }
}

XTR_FUNC
io_uring_sqe* xtr::io_uring_fd_storage::get_sqe()
{
    io_uring_sqe* sqe;

    while ((sqe = io_uring_get_sqe_func_(&ring_)) == nullptr)
    {
#if XTR_IO_URING_POLL
        io_uring_sqring_wait_func_(&ring_);
#else
        // Waiting for a CQE seems to be the only option here
        wait_for_one_cqe();
#endif
    }

    return sqe;
}

XTR_FUNC
void xtr::io_uring_fd_storage::wait_for_one_cqe()
{
    assert(pending_cqe_count_ > 0);

    struct io_uring_cqe* cqe = nullptr;
    int errnum;

retry:
#if XTR_IO_URING_POLL
    while ((errnum = io_uring_peek_cqe_func_(&ring_, &cqe)) == -EAGAIN)
        ;
#else
    errnum = io_uring_wait_cqe_func_(&ring_, &cqe);
#endif

    if (errnum != 0) [[unlikely]]
    {
        std::fprintf(
            stderr,
            "xtr::io_uring_fd_storage::wait_for_one_cqe: "
            "io_uring_peek_cqe/io_uring_wait_cqe failed: %s\n",
            std::strerror(-errnum));
        return;
    }

    --pending_cqe_count_;

    const int res = cqe->res;

    auto deleter = [this](buffer* ptr) { free_buffer(ptr); };

    std::unique_ptr<buffer, decltype(deleter)> buf(
        static_cast<buffer*>(::io_uring_cqe_get_data(cqe)),
        std::move(deleter));

    ::io_uring_cqe_seen(&ring_, cqe);

    if (!buf) // close operation queued by replace_fd()
    {
        // If close(2) failed, there is nothing to be done except report an
        // error, as the state of the file descriptor is ambiguous (according
        // to POSIX).
        if (res < 0)
        {
            std::fprintf(
                stderr,
                "xtr::io_uring_fd_storage::wait_for_one_cqe: "
                "Error: close(2) failed during reopen: %s\n",
                std::strerror(-res));
        }
        return;
    }

    if (res == -EAGAIN) [[unlikely]]
    {
        resubmit_buffer(buf.release(), 0);
        goto retry;
    }

    if (res < 0) [[unlikely]]
    {
        std::fprintf(
            stderr,
            "xtr::io_uring_fd_storage::wait_for_one_cqe: "
            "Error: Write of %u bytes at offset %zu to \"%s\" (fd %d) failed: %s\n",
            buf->size_,
            buf->file_offset_,
            reopen_path_.c_str(),
            fd_.get(),
            std::strerror(-res));
        return;
    }

    const auto nwritten = unsigned(res);

    assert(nwritten <= buf->size_);

    if (nwritten != buf->size_) [[unlikely]] // Short write
    {
        resubmit_buffer(buf.release(), nwritten);
        goto retry;
    }
}

XTR_FUNC
void xtr::io_uring_fd_storage::resubmit_buffer(buffer* buf, unsigned nwritten)
{
    buf->size_ -= nwritten;
    buf->offset_ += nwritten;
    buf->file_offset_ += nwritten;

    assert(io_uring_sq_space_left(&ring_) >= 1);

    // Don't call get_sqe() as this function is only called from wait_for_one_cqe,
    // so space in the submission queue must be available (and if this is incorrect,
    // calling get_sqe() might have problems due to it calling wait_for_one_cqe).
    io_uring_sqe* sqe = io_uring_get_sqe_func_(&ring_);

    assert(sqe != nullptr);

    ::io_uring_prep_write_fixed(
        sqe,
        fd_.get(),
        buf->data_ + buf->offset_,
        buf->size_,
        buf->file_offset_,
        buf->index_);

    ::io_uring_sqe_set_data(sqe, buf);

    ++pending_cqe_count_;

    io_uring_submit_func_(&ring_);
}

XTR_FUNC
void xtr::io_uring_fd_storage::free_buffer(buffer* buf)
{
    // Push the buffer to the front of free_list_
    assert(buf != nullptr);
    buf->next_ = free_list_;
    free_list_ = buf;
}

#endif
