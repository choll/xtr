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

#include "xtr/io/fd_storage.hpp"
#include "xtr/io/io_uring_fd_storage.hpp"

#include "temp_file.hpp"

#include <catch2/catch.hpp>
#include <liburing.h>

#include <functional>

#include <dlfcn.h>
//#include <sys/socket.h>
#include <unistd.h>

#include <iostream> // XXX
#include <fcntl.h> // XXX

namespace
{
    const auto submit_next =
        reinterpret_cast<decltype(&io_uring_submit)>(
            ::dlsym(RTLD_NEXT, "io_uring_submit"));

    const auto get_sqe_next =
        reinterpret_cast<decltype(&io_uring_get_sqe)>(
            ::dlsym(RTLD_NEXT, "io_uring_get_sqe"));

    const auto get_cqe_next =
        reinterpret_cast<decltype(&__io_uring_get_cqe)>(
            ::dlsym(RTLD_NEXT, "__io_uring_get_cqe"));

    std::function<decltype(io_uring_submit)> submit_hook = submit_next;
    std::function<decltype(io_uring_get_sqe)> get_sqe_hook = get_sqe_next;
    std::function<decltype(__io_uring_get_cqe)> get_cqe_hook = get_cqe_next;

    // XXX make a separate socket fixture?

    struct fixture
    {
        fixture()
        {
            //REQUIRE(::socketpair(AF_LOCAL, SOCK_STREAM, 0, fds_) == 0);
            storage_ = xtr::make_fd_storage(tmp_.fd_.release(), tmp_.path_);
        }

        ~fixture()
        {
            std::cout << "dtor\n";
            //::close(fds_[0]);
            //::close(fds_[1]);
            reset_hooks();
        }

        void reset_hooks()
        {
            submit_hook = submit_next;
            get_sqe_hook = get_sqe_next;
        }

        temp_file tmp_;
        //int fd_;
        //int fds_[2];
        xtr::storage_interface_ptr storage_;
    };
}

// XXX
// FLUSHING AS YOU DO WHEN THERE IS NOTHING TO READ IS INCORRECT, IF A SLOW
// STREAM OF DATA IS WRITTEN IT WILL BE VERY SLOW FOR IT TO APPEAR, POSSIBLY
// WANT TO FLUSH EVERY X SECONDS? Why would it though, if data is written slowly
// then it will surely loop and find empty queues?

extern "C"
{
    int io_uring_submit(io_uring* ring)
    {
        return submit_hook(ring);
    }

    io_uring_sqe* io_uring_get_sqe(io_uring* ring)
    {
        return get_sqe_hook(ring);
    }

    int __io_uring_get_cqe(
        io_uring* ring,
        io_uring_cqe** cqe,
        unsigned submit,
        unsigned wait_nr,
        ::sigset_t* sigmask)
    {
        return get_cqe_hook(ring, cqe, submit, wait_nr, sigmask);
    }
}

TEST_CASE_METHOD(fixture, "small write test", "[fd_storage]")
{
    for (std::size_t i = 0; i < 16; ++i)
    {
        std::span<char> span;
        REQUIRE_NOTHROW(span = storage_->allocate_buffer());
        REQUIRE_NOTHROW(storage_->submit_buffer(span.data(), span.size()));
    }
}

TEST_CASE_METHOD(fixture, "big write test", "[fd_storage]")
{
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size * 2; ++i)
    {
        std::span<char> span;
        REQUIRE_NOTHROW(span = storage_->allocate_buffer());
        REQUIRE_NOTHROW(storage_->submit_buffer(span.data(), span.size()));
    }
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

    submit_next(saved_ring); // release all buffers at once
    REQUIRE(storage_->reopen() == 0);
    reset_hooks();

    // Queue up more data so that:
    // (1) CQEs from the first loop are retrieved and checked for errors
    // (2) Writing to the new file is tested
    for (std::size_t i = 0; i < xtr::io_uring_fd_storage::default_queue_size; ++i)
    {
        std::span<char> span;
        REQUIRE_NOTHROW(span = storage_->allocate_buffer());
        storage_->submit_buffer(span.data(), span.size());
    }
}

TEST_CASE_METHOD(fixture, "null return from get_sqe", "[fd_storage]")
{
    // Simulate the submission queue becoming full by returning nullptr
    // from the get_sqe hook.

    io_uring* saved_ring = nullptr;

    submit_hook =
        [&](io_uring* ring)
        {
            saved_ring = ring;
            return 0;
        };

    // First send a close operation so that there is a completion event to wait on
    REQUIRE(storage_->reopen() == 0);

    get_sqe_hook =
        [&](io_uring*)
        {
            return nullptr;
        };

    // XXX need to also test reopen():
    // REQUIRE(storage_->reopen() == 0);

//int __io_uring_get_cqe(struct io_uring *ring,
//			struct io_uring_cqe **cqe_ptr, unsigned submit,
//			unsigned wait_nr, sigset_t *sigmask);

    // Try to submit a buffer, get_sqe returns null so get_cqe should be called
    const auto span = storage_->allocate_buffer();
    storage_->submit_buffer(span.data(), span.size());




    std::cout << "Not reached\n";
}

#if 0

TEST_CASE_METHOD(fixture, "short write test", "[fd_storage]")
{
    const auto span = storage_->allocate_buffer();

    const std::size_t size = 1024;

    REQUIRE(span.size() >= size);

    io_uring_sqe* sqe = nullptr;

    submit_hook =
        [&](io_uring* ring)
        {
            std::cout << "submit, sqe length=" << sqe->len << "\n";
            if (sqe->len == 64 * 1024)
            {
                std::cout << "adjust\n";
                sqe->len -= 1024;
            }
            return submit_next(ring);
        };
    get_sqe_hook =
        [&](io_uring* ring)
        {
            return get_sqe_next(ring);
        };

    storage_->submit_buffer(span.data(), span.size());
    storage_->flush();

/*
    char buf[16];
    std::size_t total_nread = 0;

    while (total_nread < span.size())
    {
        const ::ssize_t nread = ::read(fds_[1], buf, sizeof(buf));
        REQUIRE(nread != -1);
        total_nread += std::size_t(nread);
    }*/
}
#endif
// WRITE TESTS FOR SHORT WRITES
//
// Could just interpose? Then when entering the test you modify a function pointer.
//
// Either that or open a socketpair, but don't read from the socket? Interposing seems better really.
//
//
