// Copyright 2020 Chris E. Holloway
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

#include "xtr/detail/synchronized_ring_buffer.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <span>
#include <thread>

#define XTR_ASSERT_ALWAYS(expr)                                             \
    (__builtin_expect(!(expr), 0)                                           \
         ? (static_cast<void>(std::printf(                                  \
                "Assertion failed: (%s), function %s, file %s, line %u.\n", \
                __STRING(expr),                                             \
                __PRETTY_FUNCTION__,                                        \
                __FILE__,                                                   \
                __LINE__)),                                                 \
            abort())                                                        \
         : static_cast<void>(0))

namespace xtrd = xtr::detail;

namespace
{
    template<typename RingBuffer>
    void alternating_test(RingBuffer& rb, std::size_t small_size, std::size_t rounds)
    {
        const std::size_t nbytes = small_size;

        for (std::size_t n = 0; n < rounds; ++n)
        {
            // Check read span empty
            auto rr = rb.read_span();
            REQUIRE(rr.size() == 0);
            REQUIRE(rr.begin() == rr.end());

            // Check write span full
            auto wr = rb.write_span();
            REQUIRE(wr.size() == rb.capacity());
            REQUIRE(wr.begin() + rb.capacity() == wr.end());

            REQUIRE(wr.begin() == rr.begin());

            // Write some data
            for (std::size_t i = 0; i < nbytes; ++i)
                *(wr.begin() + i) = std::byte(i);
            rb.reduce_writable(nbytes);

            // Check write span is reduced
            wr = rb.write_span();
            REQUIRE(wr.size() == rb.capacity() - nbytes);
            REQUIRE(wr.begin() + rb.capacity() - nbytes == wr.end());

            // Check read span has data available
            rr = rb.read_span();
            REQUIRE(rr.size() == nbytes);
            REQUIRE(rr.begin() + nbytes == rr.end());

            // Read available data
            for (std::size_t i = 0; i < rr.size(); ++i)
                REQUIRE(*(rr.begin() + i) == std::byte(i));
            rb.reduce_readable(rr.size());

            // Check read span empty
            rr = rb.read_span();
            REQUIRE(rr.size() == 0);
            REQUIRE(rr.begin() == rr.end());

            // Check write span full
            wr = rb.write_span();
            REQUIRE(wr.size() == rb.capacity());
            REQUIRE(wr.begin() + rb.capacity() == wr.end());

            // Write as much data as possible
            for (std::size_t i = 0; i < rb.capacity(); ++i)
                *(wr.begin() + i) = std::byte(i);
            rb.reduce_writable(rb.capacity());

            // Check write span is empty
            wr = rb.write_span();
            REQUIRE(wr.size() == 0);
            REQUIRE(wr.begin() == wr.end());

            // Check read span is full
            rr = rb.read_span();
            REQUIRE(rr.size() == rb.capacity());
            REQUIRE(rr.begin() + rb.capacity() == rr.end());

            // Read available data
            for (std::size_t i = 0; i < rr.size(); ++i)
                REQUIRE(*(rr.begin() + i) == std::byte(i));
            rb.reduce_readable(rr.size());
        }
    }
}

TEST_CASE(
    "synchronized_ring_buffer small/full alternating test",
    "[synchronized_ring_buffer]")
{
    // Number of rounds is sized so that the buffer is ran through
    // twice or more.
    xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> rb(4096);

    alternating_test(rb, 1, 8192);
    alternating_test(rb, 7, 2048);
    alternating_test(rb, 8, 1024);
    alternating_test(rb, 64, 128);
    alternating_test(rb, 100, 128);
    alternating_test(rb, 1024, 16);
    alternating_test(rb, 2048, 16);
    alternating_test(rb, 4096, 16);
}

TEST_CASE(
    "synchronized_ring_buffer small/full alternating test static capacity",
    "[synchronized_ring_buffer]")
{
    xtrd::synchronized_ring_buffer<64 * 1024> rb;

    alternating_test(rb, 64, 128);
}

TEST_CASE(
    "synchronized_ring_buffer mirror boundary test",
    "[synchronized_ring_buffer]")
{
    xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> rb(1);

    const char text[] = "Hello world";

    auto wr = rb.write_span();
    rb.reduce_writable(wr.size() - 6);

    auto rr = rb.read_span();
    rb.reduce_readable(rr.size());

    // Write "Hello World\0Hello World\0" such that the first string
    // is written via the first and second mapping and the second
    // string is written via the first mapping.
    for (std::size_t i = 0; i < 2; ++i)
    {
        wr = rb.write_span();
        std::copy(
            reinterpret_cast<const std::byte*>(std::begin(text)),
            reinterpret_cast<const std::byte*>(std::end(text)),
            wr.begin());
        rb.reduce_writable(sizeof(text));
    }

    // Now read the first string in the same way it was written, but
    // read the second string via the second mapping, when it was
    // written using the first mapping.
    rr = rb.read_span();
    REQUIRE(rr.size() == 24);
    REQUIRE(
        std::equal(
            reinterpret_cast<const char*>(rr.begin()),
            reinterpret_cast<const char*>(rr.end()),
            "Hello world\0Hello world"));
}

TEST_CASE("synchronized_ring_buffer capacity test", "[synchronized_ring_buffer]")
{
    xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> rb1(1);
    xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> rb2(1024);

    const std::size_t min_pagesz = 4096;

    REQUIRE(rb1.capacity() == rb2.capacity());
    REQUIRE(rb1.capacity() >= min_pagesz);
    REQUIRE(std::size_t(std::distance(rb1.begin(), rb1.end())) == rb1.capacity());

    const auto wr = rb1.write_span();
    REQUIRE(wr.size() == rb1.capacity());
    REQUIRE(std::size_t(std::distance(wr.begin(), wr.end())) == wr.size());
}

TEST_CASE(
    "synchronized_ring_buffer static capacity test",
    "[synchronized_ring_buffer]")
{
    constexpr std::size_t size = 64 * 1024;

    xtrd::synchronized_ring_buffer<size> rb1;
    xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> rb2(size);

    REQUIRE(rb1.capacity() == rb2.capacity());
    REQUIRE(rb1.capacity() == size);
    REQUIRE(std::distance(rb1.begin(), rb1.end()) == rb1.capacity());

    const auto wr = rb1.write_span();
    REQUIRE(wr.size() == rb1.capacity());
    REQUIRE(std::size_t(std::distance(wr.begin(), wr.end())) == wr.size());
}

TEST_CASE(
    "synchronized_ring_buffer producer consumer test",
    "[synchronized_ring_buffer]")
{
    typedef xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> ring_buf;
    ring_buf rb(16 * 1024);

    std::thread consumer{
        [&rb]()
        {
            std::size_t expected = 1;

            ring_buf::const_span rr;

            for (;;)
            {
                while ((rr = rb.read_span()).empty())
                    ;

                std::span<const std::size_t> ri{
                    reinterpret_cast<const std::size_t*>(rr.begin()),
                    reinterpret_cast<const std::size_t*>(rr.end())};

                for (const auto& x : ri)
                {
                    if (x == 0)
                        return;
                    XTR_ASSERT_ALWAYS(x == expected++);
                }

                rb.reduce_readable(rr.size());
            }
        }};

    std::thread producer{
        [&rb]()
        {
            std::size_t next = 1;
            constexpr std::size_t blocksz = 256;
            constexpr std::size_t count = (1024UL * 1024 * 1024) / blocksz;

            ring_buf::span wr;

            for (std::size_t i = 0; i < count; ++i)
            {
                while ((wr = rb.write_span()).size() < blocksz)
                    ;

                std::span<std::size_t> wi{
                    reinterpret_cast<std::size_t*>(wr.begin()),
                    reinterpret_cast<std::size_t*>(wr.begin() + blocksz)};

                for (auto& x : wi)
                    x = next++;

                rb.reduce_writable(blocksz);
            }

            while ((wr = rb.write_span()).size() < sizeof(std::size_t))
                ;

            *reinterpret_cast<std::size_t*>(wr.begin()) = 0;

            rb.reduce_writable(sizeof(std::size_t));
        }};

    producer.join();
    consumer.join();
}

TEST_CASE(
    "synchronized_ring_buffer const read_span dynamic capacity test",
    "[synchronized_ring_buffer]")
{
    typedef xtrd::synchronized_ring_buffer<xtrd::dynamic_capacity> ring_buf;
    const ring_buf rb(1);
    ring_buf::const_span s = rb.read_span();
    REQUIRE(s.empty());
}

TEST_CASE(
    "synchronized_ring_buffer const read_span test",
    "[synchronized_ring_buffer]")
{
    typedef xtrd::synchronized_ring_buffer<64 * 1024> ring_buf;
    const ring_buf rb;
    ring_buf::const_span s = rb.read_span();
    REQUIRE(s.empty());
}
