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
#include "xtr/io/fd_storage.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"

#include "temp_file.hpp"

#include <catch2/catch.hpp>
#include <liburing.h>

#include <cerrno>
#include <functional>
#include <iostream>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#if __cpp_exceptions
#define XTR_REQUIRE_NOERROR(X)  REQUIRE_NOTHROW(X)
#else
#define XTR_REQUIRE_NOERROR(X)  X
#endif

namespace
{
    std::function<decltype(io_uring_submit)> submit_hook = io_uring_submit;
    std::function<decltype(io_uring_get_sqe)> get_sqe_hook = io_uring_get_sqe;
    std::function<decltype(io_uring_wait_cqe)> wait_cqe_hook = io_uring_wait_cqe;
    std::function<decltype(io_uring_sqring_wait)> sqring_wait_hook = io_uring_sqring_wait;
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
        test_fd_storage(int fd, std::string path)
        :
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
            storage_ = std::make_unique<test_fd_storage>(tmp_.fd_.release(), tmp_.path_);
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

        void flush_and_sync()
        {
            storage_->flush();
            storage_->sync();
        }

        temp_file tmp_;
        xtr::storage_interface_ptr storage_;
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
            REQUIRE((*cqe)->res == xtr::io_uring_fd_storage::default_buffer_capacity);
            return ret;
        };

    for (std::size_t i = 0; i < n; ++i)
    {
        std::span<char> span;
        XTR_REQUIRE_NOERROR(span = storage_->allocate_buffer());
        XTR_REQUIRE_NOERROR(storage_->submit_buffer(span.data(), span.size()));
    }

    flush_and_sync();

    REQUIRE(cqe_count == n);
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
            REQUIRE((*cqe)->res == xtr::io_uring_fd_storage::default_buffer_capacity);
            return ret;
        };

    for (std::size_t i = 0; i < n; ++i)
    {
        std::span<char> span;
        XTR_REQUIRE_NOERROR(span = storage_->allocate_buffer());
        XTR_REQUIRE_NOERROR(storage_->submit_buffer(span.data(), span.size()));
    }

    flush_and_sync();

    REQUIRE(cqe_count == n);
}

TEST_CASE_METHOD(fixture, "reopen with writes queued", "[fd_storage]")
{
    io_uring* saved_ring = nullptr;

    submit_hook =
        [&](io_uring* ring)
        {
            saved_ring = ring;
            return 0;
        };

    // Queue up a large number of writes so that some are (almost) guaranteed
    // to still be in flight when reopen() is called.
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size; ++i)
    {
        const auto span = storage_->allocate_buffer();
        storage_->submit_buffer(span.data(), span.size());
    }

    io_uring_submit(saved_ring); // release all buffers at once
    REQUIRE(storage_->reopen() == 0);
    reset_hooks();

    // Queue up more data so that:
    // (1) CQEs from the first loop are retrieved and checked for errors
    // (2) Writing to the new file is tested
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size; ++i)
    {
        std::span<char> span;
        XTR_REQUIRE_NOERROR(span = storage_->allocate_buffer());
        storage_->submit_buffer(span.data(), span.size());
    }
}

TEST_CASE_METHOD(fixture, "submission queue full on submit", "[fd_storage]")
{
    // First send a close operation so that there is a completion event to wait on
    REQUIRE(storage_->reopen() == 0);

    // Return null when an SQE is requested (submission queue full) to cause
    // progress to block/spin waiting for a CQE to complete (which implies a
    // SQE is then available)
    get_sqe_hook =
        [&](io_uring*)
        {
            return nullptr;
        };

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
    const auto span = storage_->allocate_buffer();
    storage_->submit_buffer(span.data(), span.size());

    REQUIRE(wait_cqe_called);
}

TEST_CASE_METHOD(fixture, "submission queue full on reopen", "[fd_storage]")
{
    // First send a close operation so that there is a completion event to wait on
    REQUIRE(storage_->reopen() == 0);

    // Return null when an SQE is requested to cause progress to block/spin
    // waiting for a CQE to complete (which implies a SQE is then available)
    get_sqe_hook =
        [&](io_uring*)
        {
            return nullptr;
        };

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

    // Try to reopen, get_sqe returns null so wait_cqe should be called
    REQUIRE(storage_->reopen() == 0);

    REQUIRE(wait_cqe_called);
}

TEST_CASE_METHOD(fixture, "short write test", "[fd_storage]")
{
    const unsigned missing_size = 1024;

    io_uring_sqe* sqe = nullptr;

    std::size_t sqe_count = 0;
    std::size_t cqe_count = 0;
    std::size_t submit_count = 0;

    get_sqe_hook =
        [&](io_uring* ring)
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

    submit_hook =
        [&](io_uring* ring)
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

    REQUIRE(sqe_count == 2);
    REQUIRE(cqe_count == 2);
    REQUIRE(submit_count == 2);
}

TEST_CASE_METHOD(fixture, "EAGAIN test", "[fd_storage]")
{
    std::cerr << "Note: Log message to stderr is expected\n";

    std::size_t sqe_count = 0;
    std::size_t cqe_count = 0;
    std::size_t submit_count = 0;
    io_uring_sqe* sqe;

    get_sqe_hook =
        [&](io_uring* ring)
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

    submit_hook =
        [&](io_uring* ring)
        {
            ++submit_count;
            // Full buffer should be resubmitted
            REQUIRE(sqe->len == span.size());
            return io_uring_submit(ring);
        };

    storage_->submit_buffer(span.data(), span.size());
    flush_and_sync();

    // Request should be resubmitted
    REQUIRE(sqe_count == 2);
    REQUIRE(cqe_count == 2);
    REQUIRE(submit_count == 2);
}

TEST_CASE_METHOD(fixture, "close fail test", "[fd_storage]")
{
    std::cerr << "Note: Log message to stderr is expected\n";

    std::size_t sqe_count = 0;
    std::size_t cqe_count = 0;
    std::size_t submit_count = 0;

    get_sqe_hook =
        [&](io_uring* ring)
        {
            ++sqe_count;
            return io_uring_get_sqe(ring);
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
            REQUIRE(::io_uring_cqe_get_data(*cqe) == nullptr);
            (*cqe)->res = -EIO;
            return ret;
        };

    submit_hook =
        [&](io_uring* ring)
        {
            ++submit_count;
            return io_uring_submit(ring);
        };

    REQUIRE(storage_->reopen() == 0);
    storage_->sync();

    // Request should not be resubmitted
    REQUIRE(sqe_count == 1);
    REQUIRE(cqe_count == 1);
    REQUIRE(submit_count == 1);
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

    const auto span = storage_->allocate_buffer();
    storage_->submit_buffer(span.data(), span.size());
    flush_and_sync();
}
#endif
