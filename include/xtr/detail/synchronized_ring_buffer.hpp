// Copyright 2019, 2020 Chris E. Holloway
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

#ifndef XTR_DETAIL_SYNCHRONIZED_RING_BUFFER_HPP
#define XTR_DETAIL_SYNCHRONIZED_RING_BUFFER_HPP

#include "config.hpp"
#include "tags.hpp"
#include "mirrored_memory_mapping.hpp"
#include "pagesize.hpp"
#include "pause.hpp"

#include <atomic>
#include <cassert>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <version>

namespace xtr::detail
{
    inline constexpr std::size_t dynamic_capacity = std::size_t(-1);

#if defined(MAP_POPULATE)
    inline constexpr int srb_flags = MAP_POPULATE;
#else
    inline constexpr int srb_flags = 0;
#endif

    template<std::size_t Capacity>
    class synchronized_ring_buffer;

    template<std::size_t N>
    struct least_uint
    {
        typedef
            std::conditional_t<
                N <= std::size_t{std::numeric_limits<std::uint8_t>::max()},
                std::uint8_t,
                std::conditional_t<
                    N <= std::size_t{std::numeric_limits<std::uint16_t>::max()},
                    std::uint16_t,
                    std::conditional_t<
                        N <= std::size_t{std::numeric_limits<std::uint32_t>::max()},
                        std::uint32_t,
                        std::uint64_t>>> type;
    };

    template<std::size_t N>
    using least_uint_t = typename least_uint<N>::type;

    inline constexpr std::size_t cacheline_size = 64;

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert(std::is_same<std::uint8_t, least_uint_t<0UL>>::value);
    static_assert(std::is_same<std::uint8_t, least_uint_t<255UL>>::value);

    static_assert(std::is_same<std::uint16_t,least_uint_t<256UL>>::value);
    static_assert(std::is_same<std::uint16_t, least_uint_t<65535UL>>::value);

    static_assert(std::is_same<std::uint32_t, least_uint_t<65536UL>>::value);
    static_assert(std::is_same<std::uint32_t, least_uint_t<4294967295UL>>::value);

    static_assert(std::is_same<std::uint64_t, least_uint_t<4294967296UL>>::value);
    static_assert(std::is_same<std::uint64_t, least_uint_t<18446744073709551615UL>>::value);
#endif

    template<typename T, typename SizeType>
    class span
    {
    public:
        using size_type = SizeType;
        using value_type = T;
        using iterator = value_type*;

        span() = default;

        span(const span<std::remove_const_t<T>, SizeType>& other) noexcept
        :
            begin_(other.begin()),
            size_(other.size())
        {
        }

        span& operator=(const span<std::remove_const_t<T>, SizeType>& other) noexcept
        {
            begin_ = other.begin();
            size_ = other.size();
            return *this;
        }

        span(iterator begin, iterator end)
        :
            begin_(begin),
            size_(size_type(end - begin))
        {
            assert(begin <= end);
            assert(
                size_type(end - begin) <=
                    std::numeric_limits<size_type>::max());
        }

        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return size_;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size_ == 0;
        }

        [[nodiscard]] constexpr iterator begin() const noexcept
        {
            return begin_;
        }

        [[nodiscard]] constexpr iterator end() const noexcept
        {
            return begin_ + size_;
        }

    private:
        iterator begin_ = nullptr;
        size_type size_ = 0;
    };
}

template<std::size_t Capacity = xtr::detail::dynamic_capacity>
class xtr::detail::synchronized_ring_buffer
{
public:
    using iterator = std::byte*;
    using const_iterator = const std::byte*;
    using size_type = std::size_t;
    using span = detail::span<std::byte, size_type>;
    using const_span = detail::span<const std::byte, size_type>;

    static constexpr bool is_dynamic = Capacity == dynamic_capacity;

public:
    synchronized_ring_buffer(
        int fd = -1,
        std::size_t offset = 0,
        int flags = srb_flags)
    requires (!is_dynamic)
    {
        m_ = mirrored_memory_mapping{capacity(), fd, offset, flags};
        nread_plus_capacity_ = wrnread_plus_capacity_ = capacity();
        wrbase_ = begin();
    }

    explicit synchronized_ring_buffer(
        size_type min_capacity,
        int fd = -1,
        std::size_t offset = 0,
        int flags = srb_flags)
    requires is_dynamic
    :
        m_(
            align_to_page_size(
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
                std::bit_ceil(min_capacity)),
#else
                std::ceil2(min_capacity)),
#endif
            fd,
            offset,
            flags)
    {
        assert(capacity() <= std::numeric_limits<size_type>::max());
        wrbase_ = begin();
        wrcapacity_ = capacity();
        nread_plus_capacity_ = wrnread_plus_capacity_ = capacity();
    }

    void clear() noexcept
    {
        nwritten_ = 0;
        wrnread_plus_capacity_ = capacity();
        wrnwritten_ = 0;
        nread_plus_capacity_ = capacity();
        dropped_count_ = 0;
    }

    constexpr size_type capacity() const noexcept
    {
        if constexpr (is_dynamic)
            return m_.length();
        else
            return Capacity;
    }

    // write_span() returns a contiguous span of bytes currently available
    // for writing into the buffer.
    template<typename Tags = void()>
    span write_span(size_type minsize = 0) noexcept
    {
        assert(minsize <= capacity());

        if constexpr (!is_speculative_v<Tags>)
        {
            wrnread_plus_capacity_ =
                nread_plus_capacity_.load(std::memory_order_acquire);
        }

        size_type sz = wrnread_plus_capacity_ - wrnwritten_;
        const auto b = wrbase_ + clamp(wrnwritten_, wrcapacity());

        while (sz < minsize) [[unlikely]]
        {
            if constexpr (!is_non_blocking_v<Tags>)
                pause();

            wrnread_plus_capacity_ =
                nread_plus_capacity_.load(std::memory_order_acquire);
            sz = wrnread_plus_capacity_ - wrnwritten_;

            if constexpr (is_non_blocking_v<Tags>)
                break;
        }

        if (is_non_blocking_v<Tags> && sz < minsize) [[unlikely]]
        {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            return span{};
        }

        // span must always begin in the first mapping
        assert(b >= begin());
        assert(b < end());
        return {b, b + sz};
    }

    template<typename Tags = void()>
    span write_span_spec(size_type minsize = 0) noexcept
    {
        return write_span<add_tag_t<speculative_tag, Tags>>(minsize);
    }

    void reduce_writable(size_type nbytes) noexcept
    {
        assert(nbytes <= nread_plus_capacity_.load() - nwritten_.load());
        // This release pairs with the acquire in read_span(). No reads or writes
        // in the current thread can be reordered after this store.
        wrnwritten_ += nbytes;
        nwritten_.store(wrnwritten_, std::memory_order_release);
    }

    const_span read_span() const noexcept
    {
        return const_cast<synchronized_ring_buffer<Capacity>&>(*this).read_span();
    }

    span read_span() noexcept
    {
        // read_span() returns a contiguous span of bytes currently available to
        // be read from the buffer.
        const size_type nr =
            nread_plus_capacity_.load(std::memory_order_relaxed) - capacity();
        const auto b = begin() + clamp(nr, capacity());
        // This acquire pairs with the release in reduce_writable(). No reads or
        // writes in the current thread can be reordered before this load.
        const size_type sz = nwritten_.load(std::memory_order_acquire) - nr;
        // span must always begin in the first mapping
        assert(b >= begin());
        assert(b < end());
        return {b, b + sz};
    }

    void reduce_readable(size_type nbytes) noexcept
    {
        // This release pairs with the acquire in write_span(). No reads or writes
        // in the current thread can be reordered after this store.
        nread_plus_capacity_.fetch_add(nbytes, std::memory_order_release);
#if !defined(XTR_THREAD_SANITIZER_ENABLED)
        assert(nread_plus_capacity_.load() - nwritten_.load() <= capacity());
#endif
    }

    iterator begin() noexcept
    {
        return static_cast<iterator>(m_.get());
    }

    iterator end() noexcept
    {
        return begin() + capacity();
    }

    const_iterator begin() const noexcept
    {
        return static_cast<const_iterator>(m_.get());
    }

    const_iterator end() const noexcept
    {
        return begin() + capacity();
    }

    std::size_t dropped_count() noexcept
    {
        return dropped_count_.exchange(0, std::memory_order_relaxed);
    }

private:
    static_assert(
        is_dynamic ||
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
        std::has_single_bit(Capacity)
#else
        std::ispow2(Capacity)
#endif
        );
    static_assert(is_dynamic || Capacity > 0);
    static_assert(is_dynamic || Capacity <= std::numeric_limits<size_type>::max());

    size_type wrcapacity() const noexcept
    {
        if constexpr (is_dynamic)
            return wrcapacity_;
        else
            return Capacity;
    }

    size_type clamp(size_type n, size_type capacity)
    {
        assert(capacity > 0);
        // Clamping/wrapping can be optimised away if one minus the buffer
        // capacity is the same as the maximum value representible by rdoff
        // and wroff
        using clamp_type =
            std::conditional_t<
                is_dynamic,
                size_type,
                least_uint_t<Capacity - 1>>;
        return clamp_type(n) & clamp_type(capacity - 1);
    }

    struct empty{};
    using capacity_type = std::conditional_t<is_dynamic, size_type, empty>;

    // Shared, but written by the writer only:
    alignas(cacheline_size) std::atomic<size_type> nwritten_{};
    // Writer data, wrbase shadows m_.get() and wrcapacity shadows m_.length()
    // to reduce the number of cache lines accessed while also avoiding false
    // sharing (otherwise m_ would need to be cache-line aligned to avoid false
    // sharing):
    alignas(cacheline_size) std::byte* wrbase_{};
    [[no_unique_address]] capacity_type wrcapacity_;
    size_type wrnread_plus_capacity_;
    size_type wrnwritten_{};

    // Shared, but written by the reader only:
    alignas(cacheline_size) std::atomic<size_type> nread_plus_capacity_{};
    // Reader data, note that the first member of mirrored_memory_mapping
    // is a pointer to the mapping:
    mirrored_memory_mapping m_;

    // Written by both the reader and writer
    alignas(cacheline_size) std::atomic<size_type> dropped_count_{};
};

#endif
