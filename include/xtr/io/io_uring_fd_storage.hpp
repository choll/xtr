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

#include <liburing.h>

#include <cstddef>
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
        std::size_t batch_size = default_batch_size);

    ~io_uring_fd_storage();

    void sync() noexcept override final;

protected:
    std::span<char> allocate_buffer() override final;

    void submit_buffer(char* data, std::size_t size, bool flushed) override final;

private:
    void allocate_buffers(std::size_t queue_size);

    void wait_for_one_cqe();

    void resubmit_buffer(buffer* buf, unsigned nwritten);

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
