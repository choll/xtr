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

#include "xtr/config.hpp"

#if XTR_USE_IO_URING
#include "xtr/detail/memory_mapping.hpp"
#include "xtr/io/detail/open.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"

#include "temp_file.hpp"

#include <catch2/catch.hpp>
#include <liburing.h>

#include <algorithm>
#include <cerrno>
#include <functional>
#include <iostream>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#if __cpp_exceptions
#define XTR_REQUIRE_NOERROR(X) REQUIRE_NOTHROW(X)
#else
#define XTR_REQUIRE_NOERROR(X) X
#endif

namespace
{
    std::function<decltype(io_uring_submit)> submit_hook = io_uring_submit;
    std::function<decltype(io_uring_get_sqe)> get_sqe_hook = io_uring_get_sqe;
    std::function<decltype(io_uring_wait_cqe)> wait_cqe_hook = io_uring_wait_cqe;
    std::function<decltype(io_uring_sqring_wait)> sqring_wait_hook =
        io_uring_sqring_wait;
    std::function<decltype(io_uring_peek_cqe)> peek_cqe_hook = io_uring_peek_cqe;

    int io_uring_submit_trampoline(io_uring* ring)
    {
        return submit_hook(ring);
    }

    io_uring_sqe* io_uring_get_sqe_trampoline(io_uring* ring)
    {
        return get_sqe_hook(ring);
    }

    int io_uring_wait_cqe_trampoline(io_uring* ring, io_uring_cqe** cqe)
    {
        return wait_cqe_hook(ring, cqe);
    }

    int io_uring_sqring_wait_trampoline(io_uring* ring)
    {
        return sqring_wait_hook(ring);
    }

    int io_uring_peek_cqe_trampoline(io_uring* ring, io_uring_cqe** cqe)
    {
        return peek_cqe_hook(ring, cqe);
    }

    struct test_fd_storage : xtr::io_uring_fd_storage
    {
        test_fd_storage(int fd, std::string path) :
            io_uring_fd_storage(
                fd,
                std::move(path),
                io_uring_fd_storage::default_buffer_capacity,
                io_uring_fd_storage::default_queue_size,
                io_uring_fd_storage::default_batch_size,
                io_uring_submit_trampoline,
                io_uring_get_sqe_trampoline,
                io_uring_wait_cqe_trampoline,
                io_uring_sqring_wait_trampoline,
                io_uring_peek_cqe_trampoline)
        {
        }
    };

    struct fixture
    {
        fixture()
        {
            storage_ =
                std::make_unique<test_fd_storage>(tmp_.fd_.release(), tmp_.path_);
        }

        ~fixture()
        {
            reset_hooks();
        }

        void reset_hooks()
        {
            submit_hook = io_uring_submit;
            get_sqe_hook = io_uring_get_sqe;
            wait_cqe_hook = io_uring_wait_cqe;
            sqring_wait_hook = io_uring_sqring_wait;
            peek_cqe_hook = io_uring_peek_cqe;
        }

        void send_buffer()
        {
            std::span<char> span;
            XTR_REQUIRE_NOERROR(span = storage_->allocate_buffer());
            std::ranges::fill(span, fill_++);
            XTR_REQUIRE_NOERROR(storage_->submit_buffer(span.data(), span.size()));
        }

        std::size_t buffer_size() const
        {
            return xtr::io_uring_fd_storage::default_buffer_capacity;
        }

        std::size_t file_size(const char* path) const
        {
            struct stat st{};
            REQUIRE(::stat(path, &st) == 0);
            return std::size_t(st.st_size);
        }

        bool verify_file_contents(std::size_t buffer_count) const
        {
            return verify_file_contents(buffer_count, tmp_.path_.c_str());
        }

        bool verify_file_contents(std::size_t buffer_count, const char* path) const
        {
            const std::size_t size = file_size(path);

            CAPTURE(size);
            CAPTURE(buffer_count * buffer_size());

            if (buffer_count * buffer_size() != size)
                return false;

            xtr::detail::file_descriptor fd(path, O_RDONLY);
            xtr::detail::memory_mapping
                m(nullptr, size, PROT_READ, MAP_PRIVATE, fd.get());

            auto begin = static_cast<const char*>(m.get());
            auto end = begin + buffer_size();

            for (std::size_t i = 0; i != buffer_count; ++i)
            {
                const auto fill = static_cast<unsigned char>(i);
                CAPTURE(i);
                CAPTURE(fill);
                if (!std::all_of(
                        begin,
                        end,
                        [=](unsigned char c) { return c == fill; }))
                {
                    return false;
                }
                begin += buffer_size();
                end += buffer_size();
            }

            return true;
        }

        void flush()
        {
            storage_->flush();
        }

        void flush_and_sync()
        {
            storage_->flush();
            storage_->sync();
        }

        temp_file tmp_;
        xtr::storage_interface_ptr storage_;
        unsigned char fill_ = 0;
    };
}

TEST_CASE_METHOD(fixture, "write test", "[fd_storage]")
{
    const std::size_t n = 16;
    std::size_t cqe_count = 0;

#if XTR_IO_URING_POLL
    peek_cqe_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto ring, auto cqe)
    {
        const int ret = io_uring_wait_cqe(ring, cqe);
        ++cqe_count;
        REQUIRE(std::size_t((*cqe)->res) == buffer_size());
        return ret;
    };

    for (std::size_t i = 0; i < n; ++i)
        send_buffer();

    flush_and_sync();

    REQUIRE(cqe_count == n);
    REQUIRE(verify_file_contents(n));
}

TEST_CASE_METHOD(fixture, "write more than queue size test", "[fd_storage]")
{
    const std::size_t n = xtr::io_uring_fd_storage::default_queue_size * 2;
    std::size_t cqe_count = 0;

#if XTR_IO_URING_POLL
    peek_cqe_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto ring, auto cqe)
    {
        const int ret = io_uring_wait_cqe(ring, cqe);
        ++cqe_count;
        REQUIRE(std::size_t((*cqe)->res) == buffer_size());
        return ret;
    };

    for (std::size_t i = 0; i < n; ++i)
        send_buffer();

    flush_and_sync();

    REQUIRE(cqe_count == n);
    REQUIRE(verify_file_contents(n));
}

TEST_CASE_METHOD(fixture, "reopen with writes queued", "[fd_storage]")
{
    io_uring* saved_ring = nullptr;

    submit_hook = [&](io_uring* ring)
    {
        saved_ring = ring;
        return 0;
    };

    // Queue up a large number of writes so that some are (almost) guaranteed
    // to still be in flight when reopen() is called.
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size; ++i)
        send_buffer();

    io_uring_submit(saved_ring); // release all buffers at once
    REQUIRE(storage_->reopen() == 0);
    reset_hooks();

    // Queue up more data so that:
    // (1) CQEs from the first loop are retrieved and checked for errors
    // (2) Writing to the new file is tested
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size; ++i)
        send_buffer();
}

TEST_CASE_METHOD(fixture, "submission queue full on submit", "[fd_storage]")
{
    // First send a buffer so that there is a completion event to wait on
    send_buffer();
    flush();

    // Return null when an SQE is requested (submission queue full) to cause
    // progress to block/spin waiting for a CQE to complete (which implies a
    // SQE is then available)
    get_sqe_hook = [&](io_uring*) { return nullptr; };

    bool wait_cqe_called = false;

#if XTR_IO_URING_POLL
    sqring_wait_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto... args)
    {
        if (!wait_cqe_called)
        {
            wait_cqe_called = true;
            get_sqe_hook = io_uring_get_sqe;
        }
#if XTR_IO_URING_POLL
        return io_uring_sqring_wait(args...);
#else
        return io_uring_wait_cqe(args...);
#endif
    };

    // Try to submit a buffer, get_sqe returns null so wait_cqe should be called
    send_buffer();

    REQUIRE(wait_cqe_called);
}

TEST_CASE_METHOD(fixture, "short write test", "[fd_storage]")
{
    const unsigned missing_size = 1024;

    io_uring_sqe* sqe = nullptr;

    std::size_t sqe_count = 0;
    std::size_t cqe_count = 0;
    std::size_t submit_count = 0;

    get_sqe_hook = [&](io_uring* ring)
    {
        ++sqe_count;
        return sqe = io_uring_get_sqe(ring);
    };

#if XTR_IO_URING_POLL
    peek_cqe_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto ring, auto cqe)
    {
        const int ret = io_uring_wait_cqe(ring, cqe);
        ++cqe_count;
        return ret;
    };

    submit_hook = [&](io_uring* ring)
    {
        ++submit_count;
        if (sqe->len == 64 * 1024)
            sqe->len -= missing_size;
        return io_uring_submit(ring);
    };

    const auto span = storage_->allocate_buffer();
    REQUIRE(span.size() > missing_size);
    storage_->submit_buffer(span.data(), span.size());
    flush_and_sync();

    REQUIRE(verify_file_contents(1));

    REQUIRE(sqe_count == 2);
    REQUIRE(cqe_count == 2);
    REQUIRE(submit_count == 2);
}

TEST_CASE_METHOD(fixture, "EAGAIN test", "[fd_storage]")
{
    std::size_t sqe_count = 0;
    std::size_t cqe_count = 0;
    std::size_t submit_count = 0;
    io_uring_sqe* sqe;

    get_sqe_hook = [&](io_uring* ring)
    {
        ++sqe_count;
        return sqe = io_uring_get_sqe(ring);
    };

#if XTR_IO_URING_POLL
    peek_cqe_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto ring, auto cqe)
    {
        const int ret = io_uring_wait_cqe(ring, cqe);
        if (++cqe_count == 1)
            (*cqe)->res = -EAGAIN;
        return ret;
    };

    const auto span = storage_->allocate_buffer();

    submit_hook = [&](io_uring* ring)
    {
        ++submit_count;
        // Full buffer should be resubmitted
        REQUIRE(sqe->len == span.size());
        return io_uring_submit(ring);
    };

    storage_->submit_buffer(span.data(), span.size());
    flush_and_sync();

    REQUIRE(verify_file_contents(1));

    // Request should be resubmitted
    REQUIRE(sqe_count == 2);
    REQUIRE(cqe_count == 2);
    REQUIRE(submit_count == 2);
}

TEST_CASE_METHOD(fixture, "write error test", "[fd_storage]")
{
#if XTR_IO_URING_POLL
    peek_cqe_hook =
#else
    wait_cqe_hook =
#endif
        [&](auto ring, auto cqe)
    {
        const int ret = io_uring_wait_cqe(ring, cqe);
        (*cqe)->res = -EIO;
        return ret;
    };

    std::cerr << "Note: Log message to stderr is expected\n";

    send_buffer();
    flush_and_sync();
}

TEST_CASE_METHOD(fixture, "reopen with unsent buffers", "[fd_storage]")
{
    // Send without a flush
    send_buffer();

    REQUIRE(storage_->reopen() == 0);

    REQUIRE(verify_file_contents(1));
}

TEST_CASE_METHOD(fixture, "reopen resets file offset", "[fd_storage]")
{
    send_buffer();
    flush_and_sync();

    const std::string rotated = tmp_.path_ + ".1";
    REQUIRE(::rename(tmp_.path_.c_str(), rotated.c_str()) == 0);

    REQUIRE(storage_->reopen() == 0);

    fill_ = 0;
    send_buffer();
    flush_and_sync();

    REQUIRE(verify_file_contents(1));
    REQUIRE(verify_file_contents(1, rotated.c_str()));

    REQUIRE(::unlink(rotated.c_str()) == 0);
}

TEST_CASE_METHOD(fixture, "reopen on same file appends to end", "[fd_storage]")
{
    send_buffer();
    flush_and_sync();

    // Reopen with the file still in place. offset_ must be re-seeded from the
    // new fd's end-of-file so the next write appends rather than overwriting
    // byte 0.
    REQUIRE(storage_->reopen() == 0);

    send_buffer();
    flush_and_sync();

    REQUIRE(verify_file_contents(2));
}

TEST_CASE_METHOD(fixture, "open existing file appends to end", "[fd_storage]")
{
    send_buffer();

    storage_.reset();

    xtr::detail::file_descriptor fd(xtr::detail::open_at_end(tmp_.path_.c_str()));
    REQUIRE(fd);

    storage_ = std::make_unique<test_fd_storage>(fd.release(), tmp_.path_);

    send_buffer();
    flush_and_sync();

    REQUIRE(verify_file_contents(2));
}
#endif
