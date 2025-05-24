/*
MIT License

Copyright (c) 2022 Chris E. Holloway

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef XTR_LOGGER_HPP
#define XTR_LOGGER_HPP

/**
 * Sets the capacity (in bytes) of the queue that sinks use to send log data
 * to the background thread. Each sink will have an individual queue of this
 * size. Users are permitted to define this variable in order to set a custom
 * capacity. User provided capacities may be rounded up\---to obtain the
 * actual capacity invoke @ref xtr::sink::capacity.
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_SINK_CAPACITY)
#define XTR_SINK_CAPACITY (256 * 1024)
#endif

/**
 * Set to 1 to enable io_uring support. If this setting is not manually defined
 * then io_uring support will be automatically detected. If libxtr is built with
 * io_uring support enabled then the library will still function on kernels that
 * do not have io_uring support, as a run-time check will be performed before
 * attempting to use any io_uring system calls.
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_USE_IO_URING) || defined(DOXYGEN)
#define XTR_USE_IO_URING __has_include(<liburing.h>)
#endif

/**
 * Set to 1 to enable submission queue polling when using io_uring. If enabled
 * the IORING_SETUP_SQPOLL flag will be passed to io_uring_setup(2).
 *
 * Note that if the single header include file is not used then this setting
 * may only be defined in either config.hpp or by overriding CXXFLAGS, and
 * requires rebuilding libxtr if set.
 */
#if !defined(XTR_IO_URING_POLL)
#define XTR_IO_URING_POLL 0
#endif

#include <algorithm>
#include <ctime>
#include <iterator>

#include <fmt/chrono.h>
#include <fmt/compile.h>

namespace xtr
{
    struct timespec : std::timespec
    {
        timespec() = default;

        timespec(std::timespec ts) : std::timespec(ts)
        {
        }
    };

    namespace detail
    {
        template<typename OutputIterator, typename T>
        inline void format_micros(OutputIterator out, T value)
        {
#pragma GCC unroll 6
            for (std::size_t i = 0; i != 6; ++i)
            {
                *--out = static_cast<char>('0' + value % 10);
                value /= 10;
            }
        }
    }
}

template<>
struct fmt::formatter<xtr::timespec>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const timespec& ts, FormatContext& ctx) const
    {
        thread_local struct
        {
            std::time_t sec;
            char buf[26] = {"1970-01-01 00:00:00."};
        } last;

        if (ts.tv_sec != last.sec) [[unlikely]]
        {
            fmt::format_to(
                last.buf,
                FMT_COMPILE("{:%Y-%m-%d %T}."),
                fmt::gmtime(ts.tv_sec));
            last.sec = ts.tv_sec;
        }

        xtr::detail::format_micros(std::end(last.buf), ts.tv_nsec / 1000);

        return std::copy(std::begin(last.buf), std::end(last.buf), ctx.out());
    }
};

namespace xtr
{
    struct non_blocking_tag;
    struct timestamp_tag;
}

namespace xtr::detail
{
    [[noreturn, gnu::cold]] void throw_runtime_error(const char* what);
    [[noreturn, gnu::cold, gnu::format(printf, 1, 2)]]
    void throw_runtime_error_fmt(const char* format, ...);

    [[noreturn, gnu::cold]] void throw_system_error(int errnum, const char* what);

    [[noreturn, gnu::cold, gnu::format(printf, 2, 3)]]
    void throw_system_error_fmt(int errnum, const char* format, ...);

    [[noreturn, gnu::cold]] void throw_invalid_argument(const char* what);

    [[noreturn, gnu::cold]] void throw_bad_alloc();
}

#include <cerrno>

#define XTR_TEMP_FAILURE_RETRY(expr)                \
    (__extension__({                                \
        decltype(expr) xtr_result;                  \
        do                                          \
            xtr_result = (expr);                    \
        while (xtr_result == -1 && errno == EINTR); \
        xtr_result;                                 \
    }))

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail
{
    template<typename T>
    constexpr T align(T value, T alignment) noexcept
    {
        static_assert(std::is_unsigned_v<T>, "value must be unsigned");
        assert(std::has_single_bit(alignment));
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    template<std::size_t Align, typename T>
    __attribute__((assume_aligned(Align))) T* align(T* ptr) noexcept
    {
        static_assert(
            std::is_same_v<std::remove_cv_t<T>, std::byte> ||
                std::is_same_v<std::remove_cv_t<T>, char> ||
                std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
                std::is_same_v<std::remove_cv_t<T>, signed char>,
            "value must be a char or byte pointer");
        return reinterpret_cast<T*>(align(std::uintptr_t(ptr), Align));
    }
}

#include <cstddef>

namespace xtr::detail
{
    std::size_t align_to_page_size(std::size_t length);
}

#include <array>
#include <cstdint>

namespace xtr::detail
{
    constexpr inline std::uint32_t intel_fam6_skylake_l = 0x4E;
    constexpr inline std::uint32_t intel_fam6_skylake = 0x5E;
    constexpr inline std::uint32_t intel_fam6_kabylake_l = 0x8E;
    constexpr inline std::uint32_t intel_fam6_kabylake = 0x9E;
    constexpr inline std::uint32_t intel_fam6_cometlake = 0xA5;
    constexpr inline std::uint32_t intel_fam6_cometlake_l = 0xA6;
    constexpr inline std::uint32_t intel_fam6_atom_tremont_d = 0x86;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_d = 0x5F;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont = 0x5C;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_plus = 0x7A;

    inline std::array<std::uint32_t, 4> cpuid(int leaf, int subleaf = 0) noexcept
    {
        std::array<std::uint32_t, 4> out;
        asm("cpuid"
            : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
            : "a"(leaf), "c"(subleaf));
        return out;
    }

    inline std::array<std::uint16_t, 2> get_family_model() noexcept
    {
        const std::uint32_t fms = cpuid(0x1)[0];
        std::uint16_t model = (fms & 0xF0) >> 4;
        std::uint16_t family = (fms & 0xF00) >> 8;
        if (family == 0xF)
            family |= std::uint16_t((fms & 0xFF00000) >> 16);
        if (family == 0x6 || family == 0xF)
            model |= std::uint16_t((fms & 0xF0000) >> 12);
        return {family, model};
    }
}

#include <type_traits>

namespace xtr::detail
{
    template<typename T>
    using is_c_string = std::conjunction<
        std::is_pointer<std::decay_t<T>>,
        std::is_same<char, std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>>>;

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert(is_c_string<char*>::value);
    static_assert(is_c_string<char*&>::value);
    static_assert(is_c_string<char*&&>::value);
    static_assert(is_c_string<char* const>::value);
    static_assert(is_c_string<const char*>::value);
    static_assert(is_c_string<const char* const>::value);
    static_assert(is_c_string<volatile char*>::value);
    static_assert(is_c_string<volatile char* volatile>::value);
    static_assert(is_c_string<const volatile char* volatile>::value);
    static_assert(is_c_string<char[2]>::value);
    static_assert(is_c_string<const char[2]>::value);
    static_assert(is_c_string<volatile char[2]>::value);
    static_assert(is_c_string<const volatile char[2]>::value);
    static_assert(!is_c_string<int>::value);
    static_assert(!is_c_string<int*>::value);
    static_assert(!is_c_string<int[2]>::value);
#endif
}

#include <utility>

namespace xtr::detail
{
    class file_descriptor;
}

class xtr::detail::file_descriptor
{
public:
    file_descriptor() = default;

    explicit file_descriptor(int fd) : fd_(fd)
    {
    }

    file_descriptor(const char* path, int flags, int mode = 0);

    file_descriptor(const file_descriptor&) = delete;

    file_descriptor(file_descriptor&& other) noexcept : fd_(other.fd_)
    {
        other.release();
    }

    file_descriptor& operator=(const file_descriptor&) = delete;

    file_descriptor& operator=(file_descriptor&& other) noexcept;

    ~file_descriptor();

    bool is_open() const noexcept
    {
        return fd_ != -1;
    }

    explicit operator bool() const noexcept
    {
        return is_open();
    }

    void reset(int fd = -1) noexcept;

    int get() const noexcept
    {
        return fd_;
    }

    int release() noexcept
    {
        return std::exchange(fd_, -1);
    }

private:
    int fd_ = -1;

    friend void swap(file_descriptor& a, file_descriptor& b) noexcept
    {
        using std::swap;
        swap(a.fd_, b.fd_);
    }
};

#include <cstddef>
#include <utility>

#include <sys/mman.h>

namespace xtr::detail
{
    class memory_mapping;
}

class xtr::detail::memory_mapping
{
public:
    memory_mapping() = default;

    memory_mapping(
        void* addr,
        std::size_t length,
        int prot,
        int flags,
        int fd = -1,
        std::size_t offset = 0);

    memory_mapping(const memory_mapping&) = delete;

    memory_mapping(memory_mapping&& other) noexcept;

    memory_mapping& operator=(const memory_mapping&) = delete;

    memory_mapping& operator=(memory_mapping&& other) noexcept;

    ~memory_mapping();

    void reset(void* addr = MAP_FAILED, std::size_t length = 0) noexcept;

    void release() noexcept
    {
        mem_ = MAP_FAILED;
    }

    void* get()
    {
        return mem_;
    }

    const void* get() const
    {
        return mem_;
    }

    std::size_t length() const
    {
        return length_;
    }

    explicit operator bool() const
    {
        return mem_ != MAP_FAILED;
    }

private:
    void* mem_ = MAP_FAILED;
    std::size_t length_{};

    friend void swap(memory_mapping& a, memory_mapping& b) noexcept
    {
        using std::swap;
        swap(a.mem_, b.mem_);
        swap(a.length_, b.length_);
    }
};

#include <cstddef>

namespace xtr::detail
{
    class mirrored_memory_mapping;
}

class xtr::detail::mirrored_memory_mapping
{
public:
    mirrored_memory_mapping() = default;
    mirrored_memory_mapping(mirrored_memory_mapping&&) = default;
    mirrored_memory_mapping& operator=(mirrored_memory_mapping&&) = default;

    explicit mirrored_memory_mapping(
        std::size_t length, // must be multiple of page size
        int fd = -1,
        std::size_t offset = 0, // must be multiple of page size
        int flags = 0);

    ~mirrored_memory_mapping();

    void* get()
    {
        return m_.get();
    }

    const void* get() const
    {
        return m_.get();
    }

    std::size_t length() const
    {
        return m_.length();
    }

    explicit operator bool() const
    {
        return !!m_;
    }

private:
    memory_mapping m_;
};

#if defined(__x86_64__)
#include <emmintrin.h>
#endif

namespace xtr::detail
{
    __attribute__((always_inline)) inline void pause() noexcept
    {
#if defined(__x86_64__)
        _mm_pause();
#endif
    }
}

namespace xtr::detail
{
    template<typename OutputIterator>
    void sanitize_char_to(OutputIterator& pos, char c)
    {
        if (c >= ' ' && c <= '~' && c != '\\') [[likely]]
        {
            *pos++ = c;
        }
        else
        {
            constexpr const char hex[] = "0123456789ABCDEF";
            *pos++ = '\\';
            *pos++ = 'x';
            *pos++ = hex[(c >> 4) & 0xF];
            *pos++ = hex[c & 0xF];
        }
    }
}

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename T>
    struct string_ref;

    template<>
    struct string_ref<const char*>
    {
        explicit string_ref(const char* s) : str(s)
        {
        }

        explicit string_ref(const std::string& s) : str(s.c_str())
        {
        }

        const char* str;
    };

    string_ref(const char*) -> string_ref<const char*>;
    string_ref(const std::string&) -> string_ref<const char*>;
    string_ref(const std::string_view&) -> string_ref<std::string_view>;

    template<>
    struct string_ref<std::string_view>
    {
        explicit string_ref(std::string_view s) : str(s)
        {
        }

        std::string_view str;
    };
}

namespace fmt
{
    template<>
    struct formatter<xtr::detail::string_ref<const char*>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(
            xtr::detail::string_ref<const char*> ref, FormatContext& ctx) const
        {
            auto pos = ctx.out();
            while (*ref.str != '\0')
                xtr::detail::sanitize_char_to(pos, *ref.str++);
            return pos;
        }
    };

    template<>
    struct formatter<xtr::detail::string_ref<std::string_view>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(
            const xtr::detail::string_ref<std::string_view> ref,
            FormatContext& ctx) const
        {
            auto pos = ctx.out();
            for (const char c : ref.str)
                xtr::detail::sanitize_char_to(pos, c);
            return pos;
        }
    };
}

#include <type_traits>

namespace xtr::detail
{
    template<typename Tag, typename F>
    struct detect_tag;

    template<typename Tag, typename... Tags>
    struct detect_tag<Tag, void(Tags...)> :
        std::disjunction<std::is_same<Tag, Tags>...>
    {
    };

    template<typename Tag, typename Tags>
    struct add_tag;

    template<typename Tag, typename... Tags>
    struct add_tag<Tag, void(Tags...)>
    {
        using type = void(Tag, Tags...);
    };

    template<typename Tag, typename... Tags>
    using add_tag_t = typename add_tag<Tag, Tags...>::type;

    struct speculative_tag;

    template<typename Tags>
    inline constexpr bool is_non_blocking_v =
        detect_tag<non_blocking_tag, Tags>::value;

    template<typename Tags>
    inline constexpr bool is_speculative_v =
        detect_tag<speculative_tag, Tags>::value;

    template<typename Tags>
    inline constexpr bool is_timestamp_v =
        detect_tag<timestamp_tag, Tags>::value;
}

#include <atomic>
#include <bit>
#include <cassert>
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
        typedef std::conditional_t<
            N <= std::size_t{std::numeric_limits<std::uint8_t>::max()},
            std::uint8_t,
            std::conditional_t<
                N <= std::size_t{std::numeric_limits<std::uint16_t>::max()},
                std::uint16_t,
                std::conditional_t<
                    N <= std::size_t{std::numeric_limits<std::uint32_t>::max()},
                    std::uint32_t,
                    std::uint64_t>>>
            type;
    };

    template<std::size_t N>
    using least_uint_t = typename least_uint<N>::type;

    inline constexpr std::size_t cacheline_size = 64;

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert(std::is_same<std::uint8_t, least_uint_t<0UL>>::value);
    static_assert(std::is_same<std::uint8_t, least_uint_t<255UL>>::value);

    static_assert(std::is_same<std::uint16_t, least_uint_t<256UL>>::value);
    static_assert(std::is_same<std::uint16_t, least_uint_t<65535UL>>::value);

    static_assert(std::is_same<std::uint32_t, least_uint_t<65536UL>>::value);
    static_assert(std::is_same<std::uint32_t, least_uint_t<4294967295UL>>::value);

    static_assert(std::is_same<std::uint64_t, least_uint_t<4294967296UL>>::value);
    static_assert(
        std::is_same<std::uint64_t, least_uint_t<18446744073709551615UL>>::value);
#endif

    template<typename T, typename SizeType>
    class span
    {
    public:
        using size_type = SizeType;
        using value_type = T;
        using iterator = value_type*;

        span() = default;

        span(const span<std::remove_const_t<T>, SizeType>& other) noexcept :
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

        span(iterator begin, iterator end) :
            begin_(begin),
            size_(size_type(end - begin))
        {
            assert(begin <= end);
            assert(
                size_type(end - begin) <= std::numeric_limits<size_type>::max());
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
        int fd = -1, std::size_t offset = 0, int flags = srb_flags)
        requires(!is_dynamic)
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
        m_(align_to_page_size(
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
        wrnwritten_ += nbytes;
        nwritten_.store(wrnwritten_, std::memory_order_release);
    }

    const_span read_span() const noexcept
    {
        return const_cast<synchronized_ring_buffer<Capacity>&>(*this).read_span();
    }

    span read_span() noexcept
    {
        const size_type nr =
            nread_plus_capacity_.load(std::memory_order_relaxed) - capacity();
        const auto b = begin() + clamp(nr, capacity());
        const size_type sz = nwritten_.load(std::memory_order_acquire) - nr;
        assert(b >= begin());
        assert(b < end());
        return {b, b + sz};
    }

    void reduce_readable(size_type nbytes) noexcept
    {
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
        using clamp_type =
            std::conditional_t<is_dynamic, size_type, least_uint_t<Capacity - 1>>;
        return clamp_type(n) & clamp_type(capacity - 1);
    }

    struct empty
    {
    };
    using capacity_type = std::conditional_t<is_dynamic, size_type, empty>;

    alignas(cacheline_size) std::atomic<size_type> nwritten_{};
    alignas(cacheline_size) std::byte* wrbase_{};
    [[no_unique_address]] capacity_type wrcapacity_;
    size_type wrnread_plus_capacity_;
    size_type wrnwritten_{};

    alignas(cacheline_size) std::atomic<size_type> nread_plus_capacity_{};
    mirrored_memory_mapping m_;

    alignas(cacheline_size) std::atomic<size_type> dropped_count_{};
};

#include <cstdint>
#include <ctime>

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace xtr::detail
{
    struct tsc;

    std::uint64_t get_tsc_hz() noexcept;
    std::uint64_t read_tsc_hz() noexcept;
    std::uint64_t estimate_tsc_hz() noexcept;
}

struct xtr::detail::tsc
{
    inline static tsc now() noexcept
    {
        std::uint32_t a, d;
        asm volatile("rdtsc;" : "=a"(a), "=d"(d)); // output, a=eax, d=edx
        return {
            static_cast<std::uint64_t>(a) | (static_cast<uint64_t>(d) << 32)};
    }

    static std::timespec to_timespec(tsc ts);

    std::uint64_t ticks;
};

namespace fmt
{
    template<>
    struct formatter<xtr::detail::tsc>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::detail::tsc ticks, FormatContext& ctx) const
        {
            const auto ts = xtr::detail::tsc::to_timespec(ticks);
            return formatter<xtr::timespec>().format(ts, ctx);
        }
    };
}

#include <time.h>

#if defined(CLOCK_REALTIME_COARSE)
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME_COARSE
#elif defined(CLOCK_REALTIME_FAST)
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME_FAST
#else
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME
#endif

#if defined(CLOCK_MONOTONIC_RAW)
#define XTR_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#else
#define XTR_CLOCK_MONOTONIC CLOCK_MONOTONIC
#endif

#define XTR_CLOCK_WALL CLOCK_REALTIME

#if defined(CLOCK_MONOTONIC_COARSE)
#define XTR_CLOCK_MONOTONIC_FAST CLOCK_MONOTONIC_COARSE
#elif defined(CLOCK_MONOTONIC_FAST)
#define XTR_CLOCK_MONOTONIC_FAST CLOCK_MONOTONIC_FAST
#else
#define XTR_CLOCK_MONOTONIC_FAST CLOCK_MONOTONIC
#endif

#include <ctime>

#include <time.h>

namespace xtr::detail
{
    template<clockid_t ClockId>
    xtr::timespec get_time() noexcept
    {
        std::timespec result;
        ::clock_gettime(ClockId, &result);
        return result;
    }
}

#include <string_view>

namespace xtr
{
    /**
     * Passed to @ref XTR_LOGL, @ref XTR_LOGL_TSC etc to indicate the severity
     * of the log message.
     */
    enum class log_level_t
    {
        none,
        fatal,
        error,
        warning,
        info,
        debug
    };

    /**
     * Converts a string containing a log level name to the corresponding
     * @ref log_level_t enum value. Throws std::invalid_argument if the given
     * string does not correspond to any log level.
     */
    log_level_t log_level_from_string(std::string_view str);

    /**
     * Log level styles are used to customise the formatting used when prefixing
     * log statements with their associated log level (see @ref log_level_t).
     * Styles are simply function pointers\---to provide a custom style, define
     * a function returning a string literal and accepting a single argument of
     * type @ref log_level_t and pass the function to logger::logger or
     * logger::set_log_level_style. The values returned by the function will be
     * prefixed to log statements produced by the logger. Two formatters are
     * provided, the default formatter @ref default_log_level_style and a
     * Systemd compatible style @ref systemd_log_level_style.
     */
    using log_level_style_t = const char* (*)(log_level_t);

    /**
     * The default log level style (see @ref log_level_style_t). Returns a
     * single upper-case character representing the log level followed by a
     * space, e.g. "E ", "W ", "I " for log_level_t::error,
     * log_level_t::warning, log_level_t::info and so on.
     */
    const char* default_log_level_style(log_level_t level);

    /**
     * Systemd log level style (see @ref log_level_style_t). Returns strings as
     * described in
     * <a href="https://man7.org/linux/man-pages/man3/sd-daemon.3.html">sd-daemon(3)</a>,
     * e.g. "<0>", "<1>", "<2>" etc.
     */
    const char* systemd_log_level_style(log_level_t level);
}

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

namespace xtr
{
    struct storage_interface;

    /**
     * Convenience typedef for std::unique_ptr<@ref storage_interface>
     */
    using storage_interface_ptr = std::unique_ptr<storage_interface>;

    /**
     * When passed to the reopen_path argument of @ref make_fd_storage,
     * @ref posix_fd_storage::posix_fd_storage, @ref io_uring_fd_storage or
     * @ref stream-with-reopen-ctor "logger::logger" indicates that
     * the output file handle has no associated filename and so should not be
     * reopened if requested by the
     * xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     */
    inline constexpr auto null_reopen_path = "";
}

/**
 * Interface allowing custom back-ends to be implemented. To create a custom
 * back-end, inherit from @ref storage_interfance, implement all pure-virtual
 * functions then pass a @ref storage_interface_ptr pointing to an instance of
 * the custom back-end to @ref back-end-ctor "logger::logger".
 */
struct xtr::storage_interface
{
    /**
     * Allocates a buffer for formatted log data to be written to. Once a
     * buffer has been allocated, allocate_buffer will not be called again
     * until the buffer has been submitted via @ref submit_buffer.
     */
    virtual std::span<char> allocate_buffer() = 0;

    /**
     * Submits a buffer containing formatted log data to be written.
     */
    virtual void submit_buffer(char* buf, std::size_t size) = 0;

    /**
     * Invoked to indicate that the back-end should write any buffered data to
     * its associated backing store.
     */
    virtual void flush() = 0;

    /**
     * Invoked to indicate that the back-end should ensure that all data
     * written to the associated backing store has reached permanent storage.
     */
    virtual void sync() noexcept = 0;

    /**
     * Invoked to indicate that if the back-end has a regular file opened for
     * writing log data then the file should be reopened.
     */
    virtual int reopen() noexcept = 0;

    virtual ~storage_interface() = default;
};

#include <fmt/core.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>

namespace xtr::detail
{
    class buffer;
}

class xtr::detail::buffer
{
public:
    using value_type = char;

    explicit buffer(storage_interface_ptr storage, log_level_style_t ls);

    buffer(buffer&&) = default;

    ~buffer();

    template<typename InputIterator>
    void append(InputIterator first, InputIterator last);

    void flush() noexcept;

    storage_interface& storage() noexcept
    {
        return *storage_;
    }

    void append_line();

    std::string line;
    log_level_style_t lstyle;

private:
    void next_buffer();

    storage_interface_ptr storage_;
    char* pos_ = nullptr;
    char* begin_ = nullptr;
    char* end_ = nullptr;
};

#include <fmt/compile.h>
#include <fmt/format.h>

#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename Format, typename Timestamp, typename... Args>
    void print(
        buffer& buf,
        const Format& fmt,
        log_level_t level,
        Timestamp ts,
        const std::string& name,
        const Args&... args) noexcept
    {
#if __cpp_exceptions
        try
        {
#endif
            fmt::format_to(
                std::back_inserter(buf.line),
                fmt,
                buf.lstyle(level),
                ts,
                name,
                args...);

            buf.append_line();
#if __cpp_exceptions
        }
        catch (const std::exception& e)
        {
            using namespace std::literals::string_view_literals;
            fmt::print(
                stderr,
                FMT_COMPILE("{}{}: Error writing log: {}\n"),
                buf.lstyle(log_level_t::error),
                ts,
                e.what());
            buf.line.clear();
        }
#endif
    }

    template<typename Format, typename Timestamp, typename... Args>
    void print_ts(
        buffer& buf,
        const Format& fmt,
        log_level_t level,
        const std::string& name,
        Timestamp ts,
        const Args&... args) noexcept
    {
        print(buf, fmt, level, ts, name, args...);
    }
}

#include <cstddef>
#include <string_view>

namespace xtr::detail
{
    template<std::size_t N>
    struct string
    {
        char str[N + 1];
    };

    template<std::size_t N>
    string(const char (&str)[N]) -> string<N - 1>;

    template<std::size_t N1, std::size_t N2>
    constexpr auto operator+(const string<N1>& s1, const string<N2>& s2) noexcept
    {
        string<N1 + N2> result{};

        for (std::size_t i = 0; i < N1; ++i)
            result.str[i] = s1.str[i];

        for (std::size_t i = 0; i < N2; ++i)
            result.str[i + N1] = s2.str[i];

        result.str[N1 + N2] = '\0';

        return result;
    }

    template<std::size_t Pos, std::size_t N>
    constexpr auto rcut(const char (&s)[N]) noexcept
    {
        constexpr std::size_t count = N - Pos - 1;

        string<count> result{};

        for (std::size_t i = 0; i < count; ++i)
            result.str[i] = s[Pos + i];

        result.str[count] = '\0';

        return result;
    }

    template<std::size_t N>
    constexpr auto rindex(const char (&s)[N], char c) noexcept
    {
        std::size_t i = N - 1;
        while (i > 0 && s[--i] != c)
            ;
        return s[i] == c ? i : std::size_t(-1);
    }

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
    static_assert((string<3>{"foo"} + string{"bar"}).str[0] == 'f');
    static_assert((string<3>{"foo"} + string{"bar"}).str[1] == 'o');
    static_assert((string<3>{"foo"} + string{"bar"}).str[2] == 'o');
    static_assert((string<3>{"foo"} + string{"bar"}).str[3] == 'b');
    static_assert((string<3>{"foo"} + string{"bar"}).str[4] == 'a');
    static_assert((string<3>{"foo"} + string{"bar"}).str[5] == 'r');
    static_assert((string<3>{"foo"} + string{"bar"}).str[6] == '\0');

    static_assert(rcut<0>("123").str[0] == '1');
    static_assert(rcut<0>("123").str[1] == '2');
    static_assert(rcut<0>("123").str[2] == '3');
    static_assert(rcut<0>("123").str[3] == '\0');
    static_assert(rcut<1>("123").str[0] == '2');
    static_assert(rcut<1>("123").str[1] == '3');
    static_assert(rcut<1>("123").str[2] == '\0');
    static_assert(rcut<2>("123").str[0] == '3');
    static_assert(rcut<2>("123").str[1] == '\0');
    static_assert(rcut<3>("123").str[0] == '\0');

    static_assert(rindex("foo", '/') == std::size_t(-1));
    static_assert(rindex("/foo", '/') == 0);
    static_assert(rindex("./foo", '/') == 1);
    static_assert(rindex("/foo/bar", '/') == 4);
    static_assert(rindex("/", '/') == 0);
#endif
}

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace xtr::detail
{
    struct string_table_entry
    {
        static constexpr auto truncated =
            std::numeric_limits<std::uint32_t>::max();

        explicit string_table_entry(std::size_t sz) : size(std::uint32_t(sz))
        {
        }

        std::uint32_t size;
    };

    template<typename T>
        requires(!std::same_as<std::remove_cvref_t<T>, string_table_entry>)
    T&& transform_string_table_entry(const std::byte*, T&& value)
    {
        return std::forward<T>(value);
    }

    inline string_ref<std::string_view> transform_string_table_entry(
        std::byte*& pos, string_table_entry entry)
    {
        if (entry.size == string_table_entry::truncated) [[unlikely]]
            return string_ref<std::string_view>("<truncated>");
        const std::string_view str(
            reinterpret_cast<const char*>(pos),
            entry.size);
        pos += entry.size;
        return string_ref<std::string_view>(str);
    }

    template<typename Tags, typename T, typename Buffer>
        requires(std::is_rvalue_reference_v<
                     decltype(std::forward<T>(std::declval<T>()))> &&
                 std::same_as<std::remove_cvref_t<T>, std::string>) ||
                (!is_c_string<T>::value &&
                 !std::same_as<std::remove_cvref_t<T>, std::string> &&
                 !std::same_as<std::remove_cvref_t<T>, std::string_view>)
    T&& build_string_table(std::byte*&, std::byte*&, Buffer&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename Buffer, typename String>
        requires std::same_as<String, std::string> ||
                 std::same_as<String, std::string_view>
    string_table_entry build_string_table(
        std::byte*& pos, std::byte*& end, Buffer& buf, const String& sv)
    {
        std::byte* str_end = pos + sv.length();
        while (end < str_end) [[unlikely]]
        {
            pause();
            const auto s = buf.write_span();
            if (s.end() < str_end) [[unlikely]]
            {
                if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    return string_table_entry{string_table_entry::truncated};
            }
            end = s.end();
        }

        std::memcpy(pos, sv.data(), sv.length());
        pos += sv.length();

        return string_table_entry(sv.length());
    }

    template<typename Tags, typename Buffer>
    string_table_entry build_string_table(
        std::byte*& pos, std::byte*& end, Buffer& buf, const char* str)
    {
        std::byte* begin = pos;
        while (*str != '\0')
        {
            while (pos == end) [[unlikely]]
            {
                pause();
                const auto s = buf.write_span();
                if (s.end() == end) [[unlikely]]
                {
                    if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    {
                        pos = begin;
                        return string_table_entry{string_table_entry::truncated};
                    }
                }
                end = s.end();
            }
            ::new (pos++) char(*str++);
        }
        return string_table_entry(std::size_t(pos - begin));
    }
}

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace xtr::detail
{
    template<auto Format, auto Level, typename State>
    std::byte* trampoline0(
        buffer& buf,
        std::byte* record,
        State&,
        const char* timestamp,
        std::string& name) noexcept
    {
        print(buf, Format, Level, timestamp, name);
        return record + sizeof(void (*)());
    }

    template<auto Format, auto Level, typename State, typename Func>
    std::byte* trampolineN(
        buffer& buf,
        std::byte* record,
        State& st,
        [[maybe_unused]] const char* timestamp,
        std::string& name) noexcept
    {
        typedef void (*fptr_t)();

        auto func_pos = record + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);

        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        auto& func = *reinterpret_cast<Func*>(func_pos);
        if constexpr (std::is_same_v<decltype(Format), std::nullptr_t>)
            func(st, name);
        else
            func(buf, record, Format, Level, timestamp, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return func_pos + align(sizeof(Func), alignof(fptr_t));
    }

    template<auto Format, auto Level, typename State, typename Func>
    std::byte* trampolineS(
        buffer& buf,
        std::byte* record,
        State&,
        const char* timestamp,
        std::string& name) noexcept
    {
        typedef void (*fptr_t)();

        auto func_pos = record + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);
        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        auto& func = *reinterpret_cast<Func*>(func_pos);
        record = func_pos + sizeof(Func);
        func(buf, record, Format, Level, timestamp, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return align<alignof(fptr_t)>(record);
    }
}

#include <algorithm>
#include <cstring>
#include <iterator>

namespace xtr::detail
{
    template<std::size_t DstSz, typename Src>
    void strzcpy(char (&dst)[DstSz], const Src& src)
    {
        const std::size_t n = std::min(DstSz - 1, std::size(src));
        std::memcpy(dst, &src[0], n);
        dst[n] = '\0';
    }
}

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace xtr
{
    class sink;
    class logger;

    namespace detail
    {
        class consumer;
    }
}

#define XTR_NOTHROW_INGESTIBLE(TYPE, VALUE)                     \
    (noexcept(std::decay_t<TYPE>{std::forward<TYPE>(VALUE)}) || \
     std::is_same_v<std::remove_cvref_t<TYPE>, std::string>)

/**
 * Log sink class. A sink is how log messages are written to a log.
 * Each sink has its own queue which is used to send log messages
 * to the logger. Sink operations are not thread safe, with the
 * exception of @ref set_level and @ref level.
 *
 * It is expected that an application will have many sinks, such
 * as a sink per thread or sink per component. A sink that is connected
 * to a logger may be created by calling @ref logger::get_sink. A sink
 * that is not connected to a logger may be created simply by default
 * construction, then the sink may be connected to a logger by calling
 * @ref logger::register_sink.
 */
class xtr::sink
{
private:
    using fptr_t = std::byte* (*)(detail::buffer& buf, // output buffer
                                  std::byte* record,   // pointer to log record
                                  detail::consumer&,
                                  const char* timestamp,
                                  std::string& name) noexcept;

public:
    explicit sink(log_level_t level = log_level_t::info);

    /**
     * Sink copy constructor. When a sink is copied it is automatically
     * registered with the same logger object as the source sink, using
     * the same sink name. The sink name may be modified by calling @ref
     * set_name.
     */
    sink(const sink& other);

    /**
     * Sink copy assignment operator. When a sink is copy assigned it
     * is closed in order to disconnect it from any existing logger object
     * and is then automatically registered with the same logger object as
     * the source sink, using the same sink name. The sink name may be
     * modified by calling @ref set_name.
     */
    sink& operator=(const sink& other);

    /**
     * Sink destructor. When a sink is destructed it is automatically
     * closed.
     */
    ~sink();

    /**
     *  Closes the sink. After this function returns the sink is closed and
     *  log() functions may not be called on the sink. The sink may be
     *  re-opened by calling @ref logger::register_sink.
     */
    void close();

    /**
     * Returns true if the sink is open (connected to a logger), or false if
     * the sink is closed (not connected to a logger). Log messages may only
     * be written to a sink that is open.
     */
    bool is_open() const noexcept;

    /**
     *  Synchronizes all log calls previously made by this sink with the
     *  background thread and syncs all data to back-end storage.
     *
     *  @post All entries in the sink's queue have been processed by the
     *        background thread, buffers have been flushed and the sync()
     *        function on the storage interface has been called. For the
     *        default (disk) storage this means fsync(2) (if available) has
     *        been called.
     */
    void sync();

    /**
     *  Sets the sink's name to the specified value.
     */
    void set_name(std::string name);

    /**
     *  Logs the given format string and arguments. This function is not
     *  intended to be used directly, instead one of the XTR_LOG macros
     *  should be used. It is provided for use in situations where use of
     *  a macro may be undesirable.
     */
    template<auto Format, auto Level, typename Tags = void(), typename... Args>
    void log(Args&&... args) noexcept((XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    /**
     *  Sets the log level of the sink to the specified level (see @ref log_level_t).
     *  Any log statement made with a log level with lower importance than the
     *  current level will be dropped\---please see the <a href="guide.html#log-levels">
     *  log levels</a> section of the user guide for details.
     */
    void set_level(log_level_t level)
    {
        level_.store(level, std::memory_order_relaxed);
    }

    /**
     *  Returns the current log level (see @ref log_level_t).
     */
    log_level_t level() const
    {
        return level_.load(std::memory_order_relaxed);
    }

    /**
     * Returns the capacity (in bytes) of the queue that the sink uses to send
     * log data to the background thread. To override the sink capacity set
     * @ref XTR_SINK_CAPACITY in xtr/config.hpp.
     */
    std::size_t capacity() const
    {
        return buf_.capacity();
    }

private:
    sink(logger& owner, std::string name, log_level_t level);

    template<auto Format, auto Level, typename Tags = void()>
    void log_impl() noexcept;

    template<auto Format, auto Level, typename Tags, typename... Args>
    void log_impl(Args&&... args) noexcept(
        (XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    template<typename T>
    void copy(std::byte* pos, T&& value) noexcept(
        XTR_NOTHROW_INGESTIBLE(T, value));

    template<auto Format = nullptr, auto Level = 0, typename Tags = void(), typename Func>
    void post(Func&& func) noexcept(XTR_NOTHROW_INGESTIBLE(Func, func));

    template<auto Format, auto Level, typename Tags, typename... Args>
    void post_with_str_table(Args&&... args) noexcept(
        (XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    template<typename Func>
    void sync_post(Func&& func);

    template<typename Tags, typename... Args>
    auto make_lambda(Args&&... args) noexcept(
        (XTR_NOTHROW_INGESTIBLE(Args, args) && ...));

    using ring_buffer = detail::synchronized_ring_buffer<XTR_SINK_CAPACITY>;

    static_assert(
        XTR_SINK_CAPACITY <=
            std::numeric_limits<decltype(detail::string_table_entry::size)>::max(),
        "XTR_SINK_CAPACITY is too large");

    ring_buffer buf_;
    std::atomic<log_level_t> level_;
    bool open_ = false;

    friend detail::consumer;
    friend logger;
};

template<auto Format, auto Level, typename Tags, typename... Args>
void xtr::sink::log(Args&&... args) noexcept(
    (XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    log_impl<Format, Level, Tags>(std::forward<Args>(args)...);
}

template<auto Format, auto Level, typename Tags>
void xtr::sink::log_impl() noexcept
{
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, Level, detail::consumer>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, auto Level, typename Tags, typename... Args>
void xtr::sink::log_impl(Args&&... args) noexcept(
    (XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    static_assert(sizeof...(Args) > 0);
    constexpr bool is_str = std::disjunction_v<
        detail::is_c_string<decltype(std::forward<Args>(args))>...,
        std::is_same<std::remove_cvref_t<Args>, std::string_view>...,
        std::is_same<std::remove_cvref_t<Args>, std::string>...>;
    if constexpr (is_str)
        post_with_str_table<Format, Level, Tags>(std::forward<Args>(args)...);
    else
        post<Format, Level, Tags>(
            make_lambda<Tags>(std::forward<Args>(args)...));
}

template<auto Format, auto Level, typename Tags, typename... Args>
void xtr::sink::post_with_str_table(Args&&... args) noexcept(
    (XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    using lambda_t = decltype(make_lambda<Tags>(detail::build_string_table<Tags>(
        std::declval<std::byte*&>(),
        std::declval<std::byte*&>(),
        buf_,
        std::forward<Args>(args))...));

    ring_buffer::span s = buf_.write_span_spec();

    auto func_pos = s.begin() + sizeof(fptr_t);
    if constexpr (alignof(lambda_t) > alignof(fptr_t))
        func_pos = align<alignof(lambda_t)>(func_pos);

    static_assert(alignof(char) == 1);
    const auto str_pos = func_pos + sizeof(lambda_t);
    const auto size = ring_buffer::size_type(str_pos - s.begin());

    if (s.size() < size) [[unlikely]]
        s = buf_.write_span<Tags>(size);

    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;

    auto str_cur = str_pos;
    auto str_end = s.end();

    copy(
        s.begin(),
        &detail::trampolineS<Format, Level, detail::consumer, lambda_t>);
    copy(
        func_pos,
        make_lambda<Tags>(detail::build_string_table<Tags>(
            str_cur,
            str_end,
            buf_,
            std::forward<Args>(args))...));

    const auto next = detail::align<alignof(fptr_t)>(str_cur);
    const auto total_size = ring_buffer::size_type(next - s.begin());

    buf_.reduce_writable(total_size);
}

template<typename T>
void xtr::sink::copy(std::byte* pos, T&& value) noexcept(
    XTR_NOTHROW_INGESTIBLE(T, value))
{
    assert(std::uintptr_t(pos) % alignof(T) == 0);
#if defined(__cpp_lib_assume_aligned)
    pos = static_cast<std::byte*>(std::assume_aligned<alignof(T)>(pos));
#else
    pos = static_cast<std::byte*>(__builtin_assume_aligned(pos, alignof(T)));
#endif
    ::new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, auto Level, typename Tags, typename Func>
void xtr::sink::post(Func&& func) noexcept(XTR_NOTHROW_INGESTIBLE(Func, func))
{
    ring_buffer::span s = buf_.write_span_spec();

    auto func_pos = s.begin() + sizeof(fptr_t);
    if constexpr (alignof(Func) > alignof(fptr_t))
        func_pos = detail::align<alignof(Func)>(func_pos);

    const auto next = func_pos + detail::align(sizeof(Func), alignof(fptr_t));
    const auto size = ring_buffer::size_type(next - s.begin());

    if ((s.size() < size)) [[unlikely]]
        s = buf_.write_span<Tags>(size);

    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;

    copy(s.begin(), &detail::trampolineN<Format, Level, detail::consumer, Func>);
    copy(func_pos, std::forward<Func>(func));

    buf_.reduce_writable(size);
}

template<typename Tags, typename... Args>
auto xtr::sink::make_lambda(Args&&... args) noexcept(
    (XTR_NOTHROW_INGESTIBLE(Args, args) && ...))
{
    return [... args = std::forward<Args>(args)]<typename Format>(
               detail::buffer& buf,
               std::byte*& record,
               const Format& fmt,
               log_level_t level,
               [[maybe_unused]] const char* ts,
               const std::string& name) mutable noexcept
    {
        if constexpr (detail::is_timestamp_v<Tags>)
        {
            detail::print_ts(
                buf,
                fmt,
                level,
                name,
                detail::transform_string_table_entry(record, args)...);
        }
        else
        {
            detail::print(
                buf,
                fmt,
                level,
                ts,
                name,
                detail::transform_string_table_entry(record, args)...);
        }
    };
}

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail
{
    using frame_id_t = std::uint32_t;

    struct frame_header
    {
        frame_id_t frame_id;
    };

    inline constexpr std::size_t max_frame_alignment = alignof(std::max_align_t);
    inline constexpr std::size_t max_frame_size = 512;

    union alignas(max_frame_alignment) frame_buf
    {
        frame_header hdr;
        char buf[max_frame_size];
    };

    template<typename Payload>
    struct frame : frame_header
    {
        static_assert(std::is_standard_layout_v<Payload>);
        static_assert(std::is_trivially_destructible_v<Payload>);
        static_assert(std::is_trivially_copyable_v<Payload>);

        using payload_type = Payload;

        frame()
        {
            static_assert(alignof(frame<Payload>) <= max_frame_alignment);
            static_assert(sizeof(frame<Payload>) <= max_frame_size);

            frame_id = Payload::frame_id;
        }

        Payload* operator->()
        {
            return &payload;
        }

        [[no_unique_address]] Payload payload{};
    };
}

namespace xtr::detail
{
    enum class pattern_type_t
    {
        none,
        extended_regex,
        basic_regex,
        wildcard
    };

    struct pattern
    {
        pattern_type_t type;
        bool ignore_case;
        char text[256];
    };
}

namespace xtr::detail
{
    enum class message_id
    {
        status,
        set_level,
        sink_info,
        success,
        error,
        reopen
    };
}

namespace xtr::detail
{
    struct status
    {
        static constexpr auto frame_id = frame_id_t(message_id::status);

        struct pattern pattern;
    };

    struct set_level
    {
        static constexpr auto frame_id = frame_id_t(message_id::set_level);

        log_level_t level;
        struct pattern pattern;
    };

    struct reopen
    {
        static constexpr auto frame_id = frame_id_t(message_id::reopen);
    };
}

#include <ostream>

namespace xtr::detail
{
    struct sink_info
    {
        static constexpr auto frame_id = frame_id_t(message_id::sink_info);

        log_level_t level;
        std::size_t buf_capacity;
        std::size_t buf_nbytes;
        std::size_t dropped_count;
        char name[128];
    };

    struct success
    {
        static constexpr auto frame_id = frame_id_t(message_id::success);
    };

    struct error
    {
        static constexpr auto frame_id = frame_id_t(message_id::error);

        char reason[256];
    };
}

#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>

namespace xtr::detail
{
    [[nodiscard]] file_descriptor command_connect(std::string_view path);
}

inline xtr::detail::file_descriptor xtr::detail::command_connect(
    std::string_view path)
{
    file_descriptor fd(::socket(AF_LOCAL, SOCK_SEQPACKET, 0));

    if (!fd)
        return {};

    sockaddr_un addr;
    addr.sun_family = AF_LOCAL;

    if (path.size() >= sizeof(addr.sun_path))
    {
        errno = ENAMETOOLONG;
        return {};
    }

    strzcpy(addr.sun_path, path);

#if defined(__linux__)
    if (addr.sun_path[0] == '\0') // abstract socket
    {
        std::memset(
            addr.sun_path + path.size(),
            '\0',
            sizeof(addr.sun_path) - path.size());
    }
#endif

    if (::connect(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) !=
        0)
        return {};

    return fd;
}

#include <sys/socket.h>
#include <sys/types.h>

namespace xtr::detail
{
    [[nodiscard]] ::ssize_t command_send(
        int fd, const void* buf, std::size_t nbytes);
}

inline ::ssize_t xtr::detail::command_send(
    int fd, const void* buf, std::size_t nbytes)
{
    ::msghdr hdr{};
    ::iovec iov;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    iov.iov_base = const_cast<void*>(buf);
    iov.iov_len = nbytes;

    return XTR_TEMP_FAILURE_RETRY(::sendmsg(fd, &hdr, MSG_NOSIGNAL));
}

#include <sys/socket.h>
#include <sys/types.h>

namespace xtr::detail
{
    [[nodiscard]] ::ssize_t command_recv(int fd, frame_buf& buf);
}

inline ::ssize_t xtr::detail::command_recv(int fd, frame_buf& buf)
{
    ::msghdr hdr{};
    ::iovec iov;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    iov.iov_base = &buf;
    iov.iov_len = sizeof(buf);

    return XTR_TEMP_FAILURE_RETRY(::recvmsg(fd, &hdr, 0));
}

namespace xtr::detail
{
    class command_dispatcher;

    struct command_dispatcher_deleter
    {
        void operator()(command_dispatcher*) const;
    };
}

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <poll.h>

namespace xtr::detail
{
    class command_dispatcher;
}

class xtr::detail::command_dispatcher
{
private:
    struct pollfd
    {
        detail::file_descriptor fd;
        short events;
        short revents;
    };

    static_assert(sizeof(pollfd) == sizeof(::pollfd));
    static_assert(alignof(pollfd) == alignof(::pollfd));

    struct buffer
    {
        buffer(const void* srcbuf, std::size_t srcsize);

        std::unique_ptr<char[]> buf;
        std::size_t size;
    };

    struct callback_result
    {
        std::vector<buffer> bufs;
        std::size_t pos = 0;
    };

    struct callback
    {
        std::function<void(int, void*)> func;
        std::size_t size;
    };

public:
    explicit command_dispatcher(std::string path);

    ~command_dispatcher();

    template<typename Payload, typename Callback>
    void register_callback(Callback&& c)
    {
        using frame_type = detail::frame<Payload>;

        static_assert(sizeof(frame_type) <= max_frame_size);
        static_assert(alignof(frame_type) <= max_frame_alignment);

        callbacks_[Payload::frame_id] = callback{
            [c = std::forward<Callback>(c)](int fd, void* buf)
            { c(fd, static_cast<frame_type*>(buf)->payload); },
            sizeof(frame_type)};
    }

    void send(int fd, const void* buf, std::size_t nbytes);

    template<typename FrameType>
    void send(int fd, const FrameType& frame)
    {
        send(fd, &frame, sizeof(frame));
    }

    void send_error(int fd, std::string_view reason);

    void process_commands(int timeout) noexcept;

    bool is_open() const noexcept
    {
        return !pollfds_.empty();
    }

private:
    void process_socket_read(pollfd& pfd) noexcept;
    void process_socket_write(pollfd& pfd) noexcept;
    void disconnect(pollfd& pfd) noexcept;

    std::unordered_map<frame_id_t, callback> callbacks_;
    std::vector<pollfd> pollfds_;
    std::unordered_map<int, callback_result> results_;
    std::string path_;
};

#include <cstddef>
#include <memory>

namespace xtr::detail
{
    class matcher;

    std::unique_ptr<matcher> make_matcher(
        pattern_type_t pattern_type, const char* pattern, bool ignore_case);
}

class xtr::detail::matcher
{
public:
    virtual bool operator()(const char*) const
    {
        return true;
    }

    virtual bool valid() const
    {
        return true;
    }

    virtual void error_reason(char*, std::size_t) const // LCOV_EXCL_LINE
    {
    }

    virtual ~matcher(){};
};

#include <cstddef>

#include <regex.h>

namespace xtr::detail
{
    class regex_matcher;
}

class xtr::detail::regex_matcher final : public matcher
{
public:
    regex_matcher(const char* pattern, bool ignore_case, bool extended);

    ~regex_matcher();

    regex_matcher(const regex_matcher&) = delete;
    regex_matcher& operator=(const regex_matcher&) = delete;

    bool valid() const override;

    void error_reason(char* buf, std::size_t bufsz) const override;

    bool operator()(const char* str) const override;

private:
    ::regex_t regex_;
    int errnum_;
};

namespace xtr::detail
{
    class wildcard_matcher;
}

class xtr::detail::wildcard_matcher final : public matcher
{
public:
    wildcard_matcher(const char* pattern, bool ignore_case);

    bool operator()(const char* str) const override;

private:
    const char* pattern_ = nullptr;
    int flags_;
};

#include <cstddef>
#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

namespace xtr
{
    class sink;

    namespace detail
    {
        class consumer;
    }
}

class xtr::detail::consumer
{
private:
    struct sink_handle
    {
        sink* operator->()
        {
            return p;
        }

        sink* p;
        std::string name;
        std::size_t dropped_count = 0;
    };

public:
    void run(std::function<std::timespec()>&& clock) noexcept;
    void set_command_path(std::string path) noexcept;

    consumer(buffer&& bf, sink* control, std::string command_path) :
        buf(std::move(bf)),
        sinks_({{control, "control", 0}})
    {
        set_command_path(std::move(command_path));
    }

    consumer(const consumer&) = delete;
    consumer(consumer&&) = delete;

    consumer& operator=(const consumer&) = delete;
    consumer& operator=(consumer&&) = delete;

    void add_sink(sink& s, const std::string& name);

    buffer buf;
    bool destroy = false;

private:
    void status_handler(int fd, detail::status&);
    void set_level_handler(int fd, detail::set_level&);
    void reopen_handler(int fd, detail::reopen&);

    std::vector<sink_handle> sinks_;
    std::unique_ptr<detail::command_dispatcher, detail::command_dispatcher_deleter>
        cmds_;
};

#include <string>

namespace xtr
{
    /**
     * When passed to the @ref command_path_arg "command_path" argument of
     * @ref logger::logger (or other logger constructors) indicates that no
     * command socket should be created.
     */
    inline constexpr auto null_command_path = "";

    /**
     * Returns the default command path used for the @ref command_path_arg
     * "command_path" argument of @ref logger::logger (and other logger
     * constructors). A string with the format "$XDG_RUNTIME_DIR/xtrctl.<pid>.<N>"
     * is returned, where N begins at 0 and increases for each call to the
     * function. If the directory specified by $XDG_RUNTIME_DIR does not exist
     * or is inaccessible then $TMPDIR is used instead. If $XDG_RUNTIME_DIR or
     * $TMPDIR are not set then "/run/user/<uid>" and "/tmp" are used instead,
     * respectively.
     */
    std::string default_command_path();
}

#include <fmt/compile.h>

#if defined(XTR_NDEBUG)
#undef XTR_NDEBUG
#define XTR_NDEBUG 1
#else
#define XTR_NDEBUG 0
#endif

/**
 * Basic log macro, logs the specified format string and arguments to the given
 * sink, blocking if the sink is full. Timestamps are read in the background
 * thread\---if this is undesirable use @ref XTR_LOG_RTC or @ref XTR_LOG_TSC
 * which read timestamps at the point of logging. This macro will log
 * regardless of the sink's log level.
 */
#define XTR_LOG(SINK, ...) XTR_LOG_TAGS(void(), info, SINK, __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL(LEVEL, SINK, ...) \
    XTR_LOGL_TAGS(void(), LEVEL, SINK, __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG(SINK, ...) \
    XTR_LOG_TAGS(xtr::non_blocking_tag, info, SINK, __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOGL. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL(LEVEL, SINK, ...) \
    XTR_LOGL_TAGS(xtr::non_blocking_tag, LEVEL, SINK, __VA_ARGS__)

/**
 * User-supplied timestamp log macro, logs the specified format string and
 * arguments to the given sink along with the specified timestamp, blocking if
 * the sink is full. The timestamp may be any type as long as it has a
 * formatter defined\---please see the <a href="guide.html#custom-formatters">
 * custom formatters</a> section of the user guide for details.
 * xtr::timespec is provided as a convenience type which is compatible with std::timespec and has a
 * formatter pre-defined. A formatter for std::timespec isn't defined in
 * order to avoid conflict with user code that also defines such a formatter.
 * This macro will log regardless of the sink's log level.
 *
 * @arg TS: The timestamp to apply to the log statement.
 */
#define XTR_LOG_TS(SINK, TS, ...) \
    (__extension__({ XTR_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_LOG_TS_IMPL(SINK, TS, FMT, ...) \
    XTR_LOG_TAGS(                           \
        xtr::timestamp_tag,                 \
        info,                               \
        SINK,                               \
        FMT,                                \
        TS __VA_OPT__(, ) __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_TS. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 * @arg TS: The timestamp to apply to the log statement.
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_TS(LEVEL, SINK, TS, ...) \
    (__extension__({ XTR_LOGL_TS_IMPL(LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_LOGL_TS_IMPL(LEVEL, SINK, TS, FMT, ...) \
    XTR_LOGL_TAGS(                                  \
        xtr::timestamp_tag,                         \
        LEVEL,                                      \
        SINK,                                       \
        FMT,                                        \
        (TS)__VA_OPT__(, ) __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_TS. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_TS(SINK, TS, ...) \
    (__extension__({ XTR_TRY_LOG_TS_IMPL(SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOG_TS_IMPL(SINK, TS, FMT, ...)      \
    XTR_LOG_TAGS(                                    \
        (xtr::non_blocking_tag, xtr::timestamp_tag), \
        info,                                        \
        SINK,                                        \
        FMT,                                         \
        TS __VA_OPT__(, ) __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_TS. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 * @arg TS: The timestamp to apply to the log statement.
 */
#define XTR_TRY_LOGL_TS(LEVEL, SINK, TS, ...) \
    (__extension__({ XTR_TRY_LOGL_TS_IMPL(LEVEL, SINK, TS, __VA_ARGS__); }))

#define XTR_TRY_LOGL_TS_IMPL(LEVEL, SINK, TS, FMT, ...) \
    XTR_LOGL_TAGS(                                      \
        (xtr::non_blocking_tag, xtr::timestamp_tag),    \
        LEVEL,                                          \
        SINK,                                           \
        FMT,                                            \
        TS __VA_OPT__(, ) __VA_ARGS__)

/**
 * Timestamped log macro, logs the specified format string and arguments to
 * the given sink along with a timestamp obtained by invoking
 * <a href="https://www.man7.org/linux/man-pages/man3/clock_gettime.3.html">clock_gettime(3)</a>
 * with a clock source of CLOCK_REALTIME_COARSE on Linux or CLOCK_REALTIME_FAST
 * on FreeBSD. Depending on the host CPU this may be faster than @ref
 * XTR_LOG_TSC. The non-blocking variant of this macro is @ref XTR_TRY_LOG_RTC
 * which will discard the message if the sink is full. This macro will log
 * regardless of the sink's log level.
 */
#define XTR_LOG_RTC(SINK, ...)                            \
    XTR_LOG_TS(                                           \
        SINK,                                             \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), \
        __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_RTC. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_RTC(LEVEL, SINK, ...)                    \
    XTR_LOGL_TS(                                          \
        LEVEL,                                            \
        SINK,                                             \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), \
        __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_RTC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_RTC(SINK, ...)                        \
    XTR_TRY_LOG_TS(                                       \
        SINK,                                             \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), \
        __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_RTC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL_RTC(LEVEL, SINK, ...)                \
    XTR_TRY_LOGL_TS(                                      \
        LEVEL,                                            \
        SINK,                                             \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>(), \
        __VA_ARGS__)

/**
 * Timestamped log macro, logs the specified format string and arguments to
 * the given sink along with a timestamp obtained by reading the CPU timestamp
 * counter via the RDTSC instruction. The non-blocking variant of this macro is
 * @ref XTR_TRY_LOG_TSC which will discard the message if the sink is full. This
 * macro will log regardless of the sink's log level.
 */
#define XTR_LOG_TSC(SINK, ...) \
    XTR_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Log level variant of @ref XTR_LOG_TSC. If the specified log level has lower
 * importance than the log level of the sink, then the message is dropped
 * (please see the <a href="guide.html#log-levels">log levels</a> section of
 * the user guide for details).
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 *
 * @note If the 'fatal' level is passed then the log message is written,
 *       @ref xtr::sink::sync is invoked, then the program is terminated via
 *       abort(3).
 *
 * @note Log statements with the 'debug' level can be disabled at build time by
 *       defining @ref XTR_NDEBUG.
 */
#define XTR_LOGL_TSC(LEVEL, SINK, ...) \
    XTR_LOGL_TS(LEVEL, SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_LOG_TSC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 */
#define XTR_TRY_LOG_TSC(SINK, ...) \
    XTR_TRY_LOG_TS(SINK, xtr::detail::tsc::now(), __VA_ARGS__)

/**
 * Non-blocking variant of @ref XTR_TRY_LOGL_TSC. The message will be discarded if the
 * sink is full. If a message is dropped a warning will appear in the log.
 *
 * @arg LEVEL: The unqualified log level name, for example simply "info" or "error".
 */
#define XTR_TRY_LOGL_TSC(LEVEL, SINK, ...) \
    XTR_TRY_LOGL_TS(LEVEL, SINK, xtr::detail::tsc::now(), __VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s)  #s

#define XTR_LOGL_TAGS(TAGS, LEVEL, SINK, ...)                                            \
    (__extension__({                                                                     \
        if constexpr (xtr::log_level_t::LEVEL != xtr::log_level_t::debug || !XTR_NDEBUG) \
        {                                                                                \
            if ((SINK).level() >= xtr::log_level_t::LEVEL)                               \
                XTR_LOG_TAGS(TAGS, LEVEL, SINK, __VA_ARGS__);                            \
            if constexpr (xtr::log_level_t::LEVEL == xtr::log_level_t::fatal)            \
            {                                                                            \
                (SINK).sync();                                                           \
                std::abort();                                                            \
            }                                                                            \
        }                                                                                \
    }))

#define XTR_LOG_TAGS(TAGS, LEVEL, SINK, ...) \
    (__extension__({ XTR_LOG_TAGS_IMPL(TAGS, LEVEL, SINK, __VA_ARGS__); }))

#define XTR_LOG_TAGS_IMPL(TAGS, LEVEL, SINK, FORMAT, ...)              \
    (__extension__({                                                   \
        static constexpr auto xtr_fmt =                                \
            xtr::detail::string{"{}{} {} "} +                          \
            xtr::detail::rcut<xtr::detail::rindex(__FILE__, '/') + 1>( \
                __FILE__) +                                            \
            xtr::detail::string{":"} +                                 \
            xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};  \
        using xtr::nocopy;                                             \
        (SINK)                                                         \
            .template log<                                             \
                FMT_COMPILE(xtr_fmt.str),                              \
                xtr::log_level_t::LEVEL,                               \
                void(TAGS)>(__VA_ARGS__);                              \
    }))

#include <string>

namespace xtr::detail
{
    class fd_storage_base;
}

class xtr::detail::fd_storage_base : public storage_interface
{
public:
    fd_storage_base(int fd, std::string reopen_path);

    void sync() noexcept override;

    int reopen() noexcept override;

protected:
    virtual void replace_fd(int newfd) noexcept;

    std::string reopen_path_;
    detail::file_descriptor fd_;
};

#include <cstddef>
#include <string>

namespace xtr
{
    class posix_fd_storage;
}

/**
 * An implementation of @ref storage_interface that uses standard
 * <a href="https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html">POSIX</a>
 * functions to perform file I/O.
 */
class xtr::posix_fd_storage : public detail::fd_storage_base
{
public:
    /**
     * Default value for the buffer_capacity constructor argument.
     */
    static constexpr std::size_t default_buffer_capacity = 64 * 1024;

    /**
     * File descriptor constructor.
     *
     * @arg fd: File descriptor to write to. This will be duplicated via a call to
     *          <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     *          so callers may close the file descriptor immediately after this
     *          constructor returns if desired.
     * @arg reopen_path: The path of the file associated with the fd argument.
     *                   This path will be used to reopen the file if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the file
     *                   descriptor.
     * @arg buffer_capacity: The size in bytes of the internal write buffer.
     */
    explicit posix_fd_storage(
        int fd,
        std::string reopen_path = null_reopen_path,
        std::size_t buffer_capacity = default_buffer_capacity);

    std::span<char> allocate_buffer() override final;

    void submit_buffer(char* buf, std::size_t size) override final;

    void flush() override final
    {
    }

private:
    std::unique_ptr<char[]> buf_;
    std::size_t buffer_capacity_;
};

#if XTR_USE_IO_URING

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

/**
 * An implementation of @ref storage_interface that uses
 * <a href="https://www.man7.org/linux/man-pages/man7/io_uring.7.html">io_uring(7)</a>
 * to perform file I/O (Linux only).
 */
class xtr::io_uring_fd_storage : public detail::fd_storage_base
{
public:
    /**
     * Default value for the buffer_capacity constructor argument.
     */
    static constexpr std::size_t default_buffer_capacity = 64 * 1024;

    /**
     * Default value for the queue_size constructor argument.
     */
    static constexpr std::size_t default_queue_size = 1024;

    /**
     * Default value for the batch_size constructor argument.
     */
    static constexpr std::size_t default_batch_size = 32;

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
     * @arg fd: File descriptor to write to. This will be duplicated via a call to
     *          <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     *          so callers may close the file descriptor immediately after this
     *          constructor returns if desired.
     * @arg reopen_path: The path of the file associated with the fd argument.
     *                   This path will be used to reopen the file if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the file
     *                   descriptor.
     * @arg buffer_capacity: The size in bytes of a single io_uring buffer.
     * @arg queue_size: The size of the io_uring submission queue.
     * @arg batch_size: The number of buffers to collect before submitting the
     *                  buffers to io_uring. If @ref XTR_IO_URING_POLL is set
     *                  to 1 in xtr/config.hpp then this parameter has no effect.
     */
    explicit io_uring_fd_storage(
        int fd,
        std::string reopen_path = null_reopen_path,
        std::size_t buffer_capacity = default_buffer_capacity,
        std::size_t queue_size = default_queue_size,
        std::size_t batch_size = default_batch_size);

    ~io_uring_fd_storage();

    void sync() noexcept override final;

    void flush() override final;

    std::span<char> allocate_buffer() override final;

    void submit_buffer(char* data, std::size_t size) override final;

protected:
    void replace_fd(int newfd) noexcept override final;

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

#include <cstddef>
#include <string>

namespace xtr
{
    /**
     * Creates a storage interface object from a path. If the host kernel supports
     * <a href="https://www.man7.org/linux/man-pages/man7/io_uring.7.html">io_uring(7)</a>
     * and libxtr was built on a machine with liburing header files available then
     * an instance of @ref io_uring_fd_storage will be created, otherwise an instance
     * of @ref posix_fd_storage will be created. To prevent @ref io_uring_fd_storage
     * from being used define set XTR_USE_IO_URING to 0 in xtr/config.hpp.
     */
    storage_interface_ptr make_fd_storage(const char* path);

    /**
     * Creates a storage interface object from a file descriptor and reopen path.
     * Either an instance of @ref io_uring_fd_storage or @ref posix_fd_storage
     * will be created, refer to @ref make_fd_storage(const char*) for details.
     *
     * @arg fd: File handle to write to. The underlying file descriptor will be
     *          duplicated via a call to
     *          <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     *          so callers may close the file handle immediately after this
     *          function returns if desired.
     * @arg reopen_path: The path of the file associated with the fp argument.
     *                   This path will be used to reopen the file if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the file
     *                   handle.
     */
    storage_interface_ptr make_fd_storage(
        FILE* fp, std::string reopen_path = null_reopen_path);

    /**
     * Creates a storage interface object from a file descriptor and reopen path.
     * Either an instance of @ref io_uring_fd_storage or @ref posix_fd_storage
     * will be created, refer to @ref make_fd_storage(const char*) for details.
     *
     * @arg fd: File descriptor to write to. This will be duplicated via a call to
     *          <a href="https://www.man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>,
     *          so callers may close the file descriptor immediately after this
     *          function returns if desired.
     * @arg reopen_path: The path of the file associated with the fd argument.
     *                   This path will be used to reopen the file if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the file
     *                   descriptor.
     */
    storage_interface_ptr make_fd_storage(
        int fd, std::string reopen_path = null_reopen_path);
}

#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <stdio.h>

namespace xtr
{
    class logger;

    /**
     * nocopy is used to specify that a log argument should be passed by
     * reference instead of by value, so that `arg` becomes `nocopy(arg)`.
     * Note that by default, all strings including C strings and
     * std::string_view are copied. In order to pass strings by reference
     * they must be wrapped in a call to nocopy.
     * Please see the <a href="guide.html#passing-arguments-by-value-or-reference">
     * passing arguments by value or reference</a> and
     * <a href="guide.html#string-arguments">string arguments</a> sections of
     * the user guide for further details.
     */
    template<typename T>
    inline auto nocopy(const T& arg)
    {
        return detail::string_ref(arg);
    }
}

/**
 * The main logger class. When constructed a background thread will be created
 * which is used for formatting log messages and performing I/O. To write to the
 * logger call @ref logger::get_sink to create a sink, then pass the sink
 * to a macro such as @ref XTR_LOG
 * (see the <a href="guide.html#creating-and-writing-to-sinks">creating and
 * writing to sinks</a> section of the user guide for details).
 */
class xtr::logger
{
private:
#if defined(__cpp_lib_jthread)
    using jthread = std::jthread;
#else
    struct jthread : std::thread
    {
        using std::thread::thread;

        jthread& operator=(jthread&&) noexcept = default;

        ~jthread()
        {
            if (joinable())
                join();
        }
    };
#endif

public:
    /**
     * Path constructor. The first argument is the path to a file which
     * should be opened and logged to. The file will be opened in append mode,
     * and will be created if it does not exist. Errors will be written to
     * stdout.
     *
     * @arg path: The path of a file to write log statements to.
     * @arg clock: @anchor clock_arg
     *             A function returning the current time of day as a
     *             std::timespec. This function will be invoked when creating
     *             timestamps for log statements produced by the basic log
     *             macros\--- please see the
     *             <a href="guide.html#basic-time-source">basic time source</a>
     *             section of the user guide for details. The default clock is
     *             std::chrono::system_clock.
     * @arg command_path: @anchor command_path_arg
     *                    The path where the local domain socket used to
     *                    communicate with <a href="xtrctl.html">xtrctl</a>
     *                    should be created. The default behaviour is to create
     *                    sockets in $XDG_RUNTIME_DIR (if set, otherwise
     *                    "/run/user/<uid>"). If that directory does not exist
     *                    or is inaccessible then $TMPDIR (if set, otherwise
     *                    "/tmp") will be used instead. See @ref default_command_path for
     *                    further details. To prevent a socket from being created, pass
     *                    @ref null_command_path.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        const char* path,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style) :
        logger(
            make_fd_storage(path),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * Stream constructor.
     *
     * It is expected that this constructor will be used with streams such as
     * stdout or stderr. If a stream that has been opened by the user is to
     * be passed to the logger then the
     * @ref stream-with-reopen-ctor "stream constructor with reopen path"
     * constructor is recommended instead, as this will mean that the log file
     * can be rotated\---please refer to the xtrctl documentation for the
     * <a href="xtrctl.html#reopening-log-files">reopening log files</a> command
     * for details.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is *not*
     *       supported if this constructor is used.
     *
     * @arg stream: The stream to write log statements to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        FILE* stream = stderr,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style) :
        logger(
            make_fd_storage(stream, null_reopen_path),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * @anchor stream-with-reopen-ctor
     *
     * Stream constructor with reopen path.
     *
     * @note Reopening the log file via the
     *       <a href="xtrctl.html#rotating-log-files">xtrctl</a> tool is supported,
     *       with the reopen_path argument specifying the path to reopen.
     *
     * @arg reopen_path: The path of the file associated with the stream argument.
     *                   This path will be used to reopen the stream if requested via
     *                   the xtrctl <a href="xtrctl.html#reopening-log-files">reopen command</a>.
     *                   Pass @ref null_reopen_path if no filename is associated with the stream.
     * @arg stream: The stream to write log statements to.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    logger(
        std::string reopen_path,
        FILE* stream,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style) :
        logger(
            make_fd_storage(stream, std::move(reopen_path)),
            std::forward<Clock>(clock),
            std::move(command_path),
            level_style)
    {
    }

    /**
     * @anchor back-end-ctor
     *
     * Custom back-end constructor (please refer to the
     * <a href="guide.html#custom-back-ends">custom back-ends</a> section of
     * the user guide for further details on implementing a custom back-end).
     *
     * @arg storage: Unique pointer to an object implementing the
     *               @ref storage_interface interface. The logger will invoke
     *               methods on this object from the background thread in
     *               order to write log data to whatever underlying storage
     *               medium is implemented by the object, such as disk,
     *               network, dot-matrix printer etc.
     * @arg clock: Please refer to the @ref clock_arg "description"
     *             above.
     * @arg command_path: Please refer to the @ref command_path_arg
     *                    "description" above.
     * @arg level_style: The log level style that will be used to prefix each log
     *                   statement\---please refer to the @ref log_level_style_t
     *                   documentation for details.
     */
    template<typename Clock = std::chrono::system_clock>
    explicit logger(
        storage_interface_ptr storage,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path(),
        log_level_style_t level_style = default_log_level_style)
    {
        consumer_ = jthread(
            &detail::consumer::run,
            std::make_unique<detail::consumer>(
                detail::buffer(std::move(storage), level_style),
                &control_,
                std::move(command_path)),
            make_clock(std::forward<Clock>(clock)));
        control_.open_ = true;
        (void)detail::get_tsc_hz();
    }

    /**
     * Logger destructor. This function will join the consumer thread. If
     * sinks are still connected to the logger then the consumer thread
     * will not terminate until the sinks disconnect, i.e. the destructor
     * will block until all connected sinks disconnect from the logger.
     */
    ~logger() = default;

    /**
     *  Returns the native handle for the logger's consumer thread. This
     *  may be used for setting thread affinities or other thread attributes.
     */
    std::thread::native_handle_type consumer_thread_native_handle()
    {
        return consumer_.native_handle();
    }

    /**
     *  Creates a sink with the specified name. Note that each call to this
     *  function creates a new sink; if repeated calls are made with the
     *  same name, separate sinks with the name name are created.
     *
     *  @param name: The name for the given sink.
     */
    [[nodiscard]] sink get_sink(std::string name);

    /**
     *  Registers the sink with the logger. Note that the sink name does not
     *  need to be unique; if repeated calls are made with the same name,
     *  separate sinks with the same name are registered.
     *
     *  @param s: The sink to register.
     *  @param name: The name for the given sink.
     *
     *  @pre The sink must be closed.
     */
    void register_sink(sink& s, std::string name) noexcept;

    /**
     * Sets the logger command path\---please refer to the 'command_path' argument
     * @ref command_path_arg "description" above for details.
     */
    void set_command_path(std::string path) noexcept;

    /**
     * Sets the logger log level style\---please refer to the @ref log_level_style_t
     * documentation for details.
     */
    void set_log_level_style(log_level_style_t level_style) noexcept;

    /**
     * Sets the default log level. Sinks created via future calls to @ref get_sink
     * will be created with the given log level.
     */
    void set_default_log_level(log_level_t level);

private:
    template<typename Func>
    void post(Func&& f)
    {
        std::scoped_lock lock{control_mutex_};
        control_.post(std::forward<Func>(f));
    }

    template<typename Clock>
    std::function<std::timespec()> make_clock(Clock&& clock)
    {
        return [clock_{std::forward<Clock>(clock)}]() -> std::timespec
        {
            using namespace std::chrono;
            const auto now = clock_.now();
            auto sec = time_point_cast<seconds>(now);
            if (sec > now)
                sec - seconds{1};
            return std::timespec{
                .tv_sec = sec.time_since_epoch().count(),
                .tv_nsec = duration_cast<nanoseconds>(now - sec).count()};
        };
    }

    jthread consumer_;
    sink control_;
    std::mutex control_mutex_;
    log_level_t default_log_level_ = log_level_t::info;

    friend sink;
};

#include <concepts>
#include <cstddef>
#include <utility>

#include <fmt/format.h>

template<typename T>
concept iterable = requires(T t) {
    std::begin(t);
    std::end(t);
};

template<typename T>
concept associative_container = requires(T t) { typename T::mapped_type; };

template<typename T>
concept tuple_like = requires(T t) { std::tuple_size<T>(); };

namespace fmt
{

    template<typename T>
        requires tuple_like<T> && (!iterable<T>)
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            return format_impl(
                value,
                ctx,
                std::make_index_sequence<std::tuple_size_v<T>>{});
        }

        template<typename FormatContext, std::size_t Index, std::size_t... Indexes>
        auto format_impl(
            const T& value,
            FormatContext& ctx,
            std::index_sequence<Index, Indexes...>) const
        {
            fmt::format_to(ctx.out(), "(");
            if (std::tuple_size_v<T> > 0)
            {
                fmt::format_to(ctx.out(), FMT_COMPILE("{}"), std::get<0>(value));
                ((fmt::format_to(
                     ctx.out(),
                     FMT_COMPILE(", {}"),
                     std::get<Indexes>(value))),
                 ...);
            }
            return fmt::format_to(ctx.out(), ")");
        }
    };

    template<associative_container T>
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            fmt::format_to(ctx.out(), "{{");
            if (!value.empty())
            {
                auto it = std::begin(value);
                ;
                fmt::format_to(
                    ctx.out(),
                    FMT_COMPILE("{}: {}"),
                    it->first,
                    it->second);
                ++it;
                while (it != std::end(value))
                {
                    fmt::format_to(
                        ctx.out(),
                        FMT_COMPILE(", {}: {}"),
                        it->first,
                        it->second);
                    ++it;
                }
            }
            return fmt::format_to(ctx.out(), "}}");
        }
    };

    template<typename T>
        requires iterable<T> && (!std::is_constructible_v<T, const char*>) &&
                 (!associative_container<T>)
    struct formatter<T>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const T& value, FormatContext& ctx) const
        {
            fmt::format_to(ctx.out(), "[");
            if (!value.empty())
            {
                auto it = std::begin(value);
                ;
                fmt::format_to(ctx.out(), FMT_COMPILE("{}"), *it++);
                while (it != std::end(value))
                    fmt::format_to(ctx.out(), FMT_COMPILE(", {}"), *it++);
            }
            return fmt::format_to(ctx.out(), "]");
        }
    };
}

#include <fmt/format.h>

#include <span>
#include <stdexcept>
#include <utility>

inline xtr::detail::buffer::buffer(
    storage_interface_ptr storage, log_level_style_t ls) :
    lstyle(ls),
    storage_(std::move(storage))
{
}

inline xtr::detail::buffer::~buffer()
{
    flush();
}

inline void xtr::detail::buffer::flush() noexcept
{
#if __cpp_exceptions
    try
    {
#endif
        if (pos_ != begin_)
        {
            storage_->submit_buffer(begin_, std::size_t(pos_ - begin_));
            pos_ = begin_ = end_ = nullptr;
        }
        if (storage_)
            storage_->flush();
#if __cpp_exceptions
    }
    catch (const std::exception& e)
    {
        using namespace std::literals::string_view_literals;
        fmt::print(
            stderr,
            "{}{}: Error flushing log: {}\n"sv,
            lstyle(log_level_t::error),
            detail::get_time<XTR_CLOCK_WALL>(),
            e.what());
    }
#endif
}

template<typename InputIterator>
void xtr::detail::buffer::append(InputIterator first, InputIterator last)
{
    while (first != last)
    {
        if (pos_ == end_) [[unlikely]]
            next_buffer();

        const auto n = std::min(last - first, end_ - pos_);

        std::memcpy(pos_, &*first, std::size_t(n));

        pos_ += n;
        first += n;
    }
}

inline void xtr::detail::buffer::append_line()
{
    append(line.begin(), line.end());
    line.clear();
}

inline void xtr::detail::buffer::next_buffer()
{
    if (pos_ != begin_) [[likely]] // if not the first call to push_back
        storage_->submit_buffer(begin_, std::size_t(pos_ - begin_));
    const std::span<char> s = storage_->allocate_buffer();
    begin_ = s.data();
    end_ = begin_ + s.size();
    pos_ = begin_;
}

#include <cassert>
#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace xtr::detail
{
    template<typename... Args>
    void errx(Args&&... args)
    {
        (std::cerr << ... << args) << "\n";
    }

    template<typename... Args>
    void err(Args&&... args)
    {
        const int errnum = errno;
        errx(std::forward<Args>(args)..., ": ", std::strerror(errnum));
    }
}

inline xtr::detail::command_dispatcher::command_dispatcher(std::string path)
{
    sockaddr_un addr;

    if (path.size() > sizeof(addr.sun_path) - 1)
    {
        errx("Error: Command path '", path, "' is too long");
        return;
    }

    detail::file_descriptor fd(
        ::socket(AF_LOCAL, SOCK_SEQPACKET | SOCK_NONBLOCK, 0));

    if (!fd)
    {
        err("Error: Failed to create command socket");
        return;
    }

    addr.sun_family = AF_LOCAL;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);

#if defined(__linux__)
    if (addr.sun_path[0] == '\0') // abstract socket
    {
        std::memset(
            addr.sun_path + path.size(),
            '\0',
            sizeof(addr.sun_path) - path.size());
    }
#endif

    if (::bind(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        err("Error: Failed to bind command socket to path '", path, "'");
        return;
    }

    path_ = std::move(path);

    if (::listen(fd.get(), 64) == -1)
    {
        err("Error: Failed to listen on command socket");
        return;
    }

    pollfds_.emplace_back(std::move(fd), POLLIN, 0);
}

inline xtr::detail::command_dispatcher::~command_dispatcher()
{
    if (!path_.empty())
        ::unlink(path_.c_str());
}

inline xtr::detail::command_dispatcher::buffer::buffer(
    const void* srcbuf, std::size_t srcsize) :
    buf(new char[srcsize]),
    size(srcsize)
{
    std::memcpy(buf.get(), srcbuf, size);
}

inline void xtr::detail::command_dispatcher::send(
    int fd, const void* buf, std::size_t nbytes)
{
    results_[fd].bufs.emplace_back(buf, nbytes);
}

inline void xtr::detail::command_dispatcher::process_commands(int timeout) noexcept
{
    int nfds = ::poll(
        reinterpret_cast<::pollfd*>(&pollfds_[0]),
        ::nfds_t(pollfds_.size()),
        timeout);

    if ((pollfds_[0].revents & POLLIN) != 0)
    {
        detail::file_descriptor fd(
            ::accept4(pollfds_[0].fd.get(), nullptr, nullptr, SOCK_NONBLOCK));
        if (fd)
            pollfds_.push_back(pollfd{std::move(fd), POLLIN, 0});
        else
            err("Error: Failed to accept connection on command socket");
        --nfds;
    }

    for (std::size_t i = 1; i < pollfds_.size() && nfds > 0; ++i)
    {
        const std::size_t n = pollfds_.size();
        if ((pollfds_[i].revents & POLLOUT) != 0)
        {
            process_socket_write(pollfds_[i]);
            --nfds;
        }
        else if ((pollfds_[i].revents & (POLLHUP | POLLIN)) != 0)
        {
            process_socket_read(pollfds_[i]);
            --nfds;
        }
        assert(pollfds_.size() <= n);
        if (pollfds_.size() < n)
            --i; // Adjust for erased item
    }
}

inline void xtr::detail::command_dispatcher::process_socket_read(pollfd& pfd) noexcept
{
    const int fd = pfd.fd.get();
    frame_buf buf;

    const ::ssize_t nbytes = command_recv(fd, buf);

    pfd.events = POLLOUT;

    if (nbytes == -1)
    {
        err("Error: Failed to read command");
        return;
    }

    if (nbytes == 0) // EOF
        return;

    if (nbytes < ::ssize_t(sizeof(frame_header)))
    {
        send_error(fd, "Incomplete frame header");
        return;
    }

    const auto cpos = callbacks_.find(buf.hdr.frame_id);

    if (cpos == callbacks_.end())
    {
        send_error(fd, "Invalid frame id");
        return;
    }

    if (nbytes != ::ssize_t(cpos->second.size))
    {
        send_error(fd, "Invalid frame length");
        return;
    }

#if __cpp_exceptions
    try
    {
#endif
        cpos->second.func(fd, &buf);
#if __cpp_exceptions
    }
    catch (const std::exception& e)
    {
        send_error(fd, e.what());
    }
#endif
}

inline void xtr::detail::command_dispatcher::process_socket_write(
    pollfd& pfd) noexcept
{
    const int fd = pfd.fd.get();

    callback_result& cr = results_[fd];

    ::ssize_t nwritten = 0;

    for (; cr.pos < cr.bufs.size(); ++cr.pos)
    {
        nwritten =
            command_send(fd, cr.bufs[cr.pos].buf.get(), cr.bufs[cr.pos].size);
        if (nwritten != ::ssize_t(cr.bufs[cr.pos].size))
            break;
    }

    if ((nwritten == -1 && errno != EAGAIN) || cr.pos == cr.bufs.size())
    {
        results_.erase(fd);
        disconnect(pfd);
    }
}

inline void xtr::detail::command_dispatcher::disconnect(pollfd& pfd) noexcept
{
    assert(results_.count(pfd.fd.get()) == 0);
    std::swap(pfd, pollfds_.back());
    pollfds_.pop_back();
}

inline void xtr::detail::command_dispatcher::send_error(
    int fd, std::string_view reason)
{
    frame<error> ef;
    strzcpy(ef->reason, reason);
    send(fd, ef);
}

inline void xtr::detail::command_dispatcher_deleter::operator()(
    command_dispatcher* d) const
{
    delete d;
}

#include <atomic>
#include <cstdio>

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

namespace xtr::detail
{
    inline std::string get_rundir()
    {
        if (const char* rundir = ::getenv("XDG_RUNTIME_DIR"))
            return rundir;
        const unsigned long uid = ::geteuid();
        char rundir[32];
        std::snprintf(rundir, sizeof(rundir), "/run/user/%lu", uid);
        return rundir;
    }

    inline std::string get_tmpdir()
    {
        if (const char* tmpdir = ::getenv("TMPDIR"))
            return tmpdir;
        return "/tmp";
    }
}

inline std::string xtr::default_command_path()
{
    static std::atomic<unsigned> ctl_count{0};
    const long pid = ::getpid();
    const unsigned n = ctl_count++;
    char path[PATH_MAX];

    std::string dir = detail::get_rundir();

    if (::access(dir.c_str(), W_OK) != 0)
        dir = detail::get_tmpdir();

    std::snprintf(path, sizeof(path), "%s/xtrctl.%ld.%u", dir.c_str(), pid, n);

    return path;
}

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <version>

inline void xtr::detail::consumer::run(std::function<::timespec()>&& clock) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;
    std::size_t flush_count = 0;

    for (std::size_t i = 0; !sinks_.empty(); ++i)
    {
        sink::ring_buffer::span span;
        const std::size_t n = i % sinks_.size();

        if (n == 0)
        {
            ts_stale |= true;
            if (cmds_)
                cmds_->process_commands(/* timeout= */ 0);
        }

        if ((span = sinks_[n]->buf_.read_span()).empty())
        {
            if (flush_count != 0 && flush_count-- == 1)
                buf.flush();
            continue;
        }

        destroy = false;

        if (ts_stale)
        {
            fmt::format_to(ts, FMT_COMPILE("{}"), xtr::timespec{clock()});
            ts_stale = false;
        }

        std::byte* pos = span.begin();
        std::byte* end = std::min(span.end(), sinks_[n]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(sink::fptr_t) == 0);
            assert(!destroy);
            const sink::fptr_t fptr =
                *reinterpret_cast<const sink::fptr_t*>(pos);
            pos = fptr(buf, pos, *this, ts, sinks_[n].name);
        } while (pos < end);

        if (destroy)
        {
            using std::swap;
            swap(sinks_[n], sinks_.back()); // possible self-swap, ok
            sinks_.pop_back();
            continue;
        }

        sinks_[n]->buf_.reduce_readable(
            sink::ring_buffer::size_type(pos - span.begin()));

        std::size_t n_dropped;
        if (sinks_[n]->buf_.read_span().empty() &&
            (n_dropped = sinks_[n]->buf_.dropped_count()) > 0)
        {
            detail::print(
                buf,
                FMT_COMPILE("{}{} {}: {} messages dropped\n"),
                log_level_t::warning,
                ts,
                sinks_[n].name,
                n_dropped);
            sinks_[n].dropped_count += n_dropped;
        }

        flush_count = sinks_.size();
    }
}

inline void xtr::detail::consumer::add_sink(sink& s, const std::string& name)
{
    sinks_.push_back(sink_handle{&s, name});
}

inline void xtr::detail::consumer::set_command_path(std::string path) noexcept
{
    if (path == null_command_path)
        return;

    cmds_.reset(new detail::command_dispatcher(std::move(path)));

    if (!cmds_->is_open())
    {
        cmds_.reset();
        return;
    }

#if defined(__cpp_lib_bind_front)
    cmds_->register_callback<detail::status>(
        std::bind_front(&consumer::status_handler, this));

    cmds_->register_callback<detail::set_level>(
        std::bind_front(&consumer::set_level_handler, this));

    cmds_->register_callback<detail::reopen>(
        std::bind_front(&consumer::reopen_handler, this));
#else
    cmds_->register_callback<detail::status>(
        [this](auto&&... args)
        { status_handler(std::forward<decltype(args)>(args)...); });

    cmds_->register_callback<detail::set_level>(
        [this](auto&&... args)
        { set_level_handler(std::forward<decltype(args)>(args)...); });

    cmds_->register_callback<detail::reopen>(
        [this](auto&&... args)
        { reopen_handler(std::forward<decltype(args)>(args)...); });
#endif
}

inline void xtr::detail::consumer::status_handler(int fd, detail::status& st)
{
    st.pattern.text[sizeof(st.pattern.text) - 1] = '\0';

    const auto matcher = detail::make_matcher(
        st.pattern.type,
        st.pattern.text,
        st.pattern.ignore_case);

    if (!matcher->valid())
    {
        detail::frame<detail::error> ef;
        matcher->error_reason(ef->reason, sizeof(ef->reason));
        cmds_->send(fd, ef);
        return;
    }

    for (std::size_t i = 1; i < sinks_.size(); ++i)
    {
        auto& s = sinks_[i];

        if (!(*matcher)(s.name.c_str()))
            continue;

        detail::frame<detail::sink_info> sif;

        sif->level = s->level();
        sif->buf_capacity = s->buf_.capacity();
        sif->buf_nbytes = s->buf_.read_span().size();
        sif->dropped_count = s.dropped_count;
        detail::strzcpy(sif->name, s.name);

        cmds_->send(fd, sif);
    }
}

inline void xtr::detail::consumer::set_level_handler(int fd, detail::set_level& sl)
{
    sl.pattern.text[sizeof(sl.pattern.text) - 1] = '\0';

    if (sl.level > xtr::log_level_t::debug)
    {
        cmds_->send_error(fd, "Invalid level");
        return;
    }

    const auto matcher = detail::make_matcher(
        sl.pattern.type,
        sl.pattern.text,
        sl.pattern.ignore_case);

    if (!matcher->valid())
    {
        detail::frame<detail::error> ef;
        matcher->error_reason(ef->reason, sizeof(ef->reason));
        cmds_->send(fd, ef);
        return;
    }

    for (std::size_t i = 1; i < sinks_.size(); ++i)
    {
        auto& s = sinks_[i];

        if (!(*matcher)(s.name.c_str()))
            continue;

        s->set_level(sl.level);
    }

    cmds_->send(fd, detail::frame<detail::success>());
}

inline void xtr::detail::consumer::reopen_handler(
    int fd, detail::reopen& /* unused */)
{
    buf.flush();
    if (const int errnum = buf.storage().reopen())
        cmds_->send_error(fd, std::strerror(errnum));
    else
        cmds_->send(fd, detail::frame<detail::success>());
}

#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

inline xtr::detail::fd_storage_base::fd_storage_base(
    int fd, std::string reopen_path) :
    reopen_path_(std::move(reopen_path)),
    fd_(::dup(fd))
{
    if (!fd_)
    {
        detail::throw_system_error_fmt(
            errno,
            "xtr::detail::fd_storage_base::fd_storage_base: dup(2) failed");
    }
}

inline void xtr::detail::fd_storage_base::sync() noexcept
{
    ::fsync(fd_.get());
}

inline int xtr::detail::fd_storage_base::reopen() noexcept
{
    if (reopen_path_ == null_reopen_path)
        return ENOENT;

    const int newfd = XTR_TEMP_FAILURE_RETRY(::open(
        reopen_path_.c_str(),
        O_CREAT | O_APPEND | O_WRONLY,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));

    if (newfd == -1)
        return errno;

    replace_fd(newfd);

    return 0;
}

inline void xtr::detail::fd_storage_base::replace_fd(int newfd) noexcept
{
    fd_.reset(newfd);
}

#include <cerrno>
#include <iostream>
#include <memory>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

inline xtr::storage_interface_ptr xtr::make_fd_storage(const char* path)
{
    const int fd = XTR_TEMP_FAILURE_RETRY(::open(
        path,
        O_CREAT | O_APPEND | O_WRONLY,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));

    if (fd == -1)
        detail::throw_system_error_fmt(errno, "Failed to open `%s'", path);

    return make_fd_storage(fd, path);
}

inline xtr::storage_interface_ptr xtr::make_fd_storage(
    FILE* fp, std::string reopen_path)
{
    return make_fd_storage(::fileno(fp), std::move(reopen_path));
}

inline xtr::storage_interface_ptr xtr::make_fd_storage(
    int fd, std::string reopen_path)
{
#if XTR_USE_IO_URING
    errno = 0;
    (void)syscall(__NR_io_uring_setup, 0, nullptr);

    if (errno != ENOSYS)
    {
        try
        {
            return std::make_unique<io_uring_fd_storage>(
                fd,
                std::move(reopen_path));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Falling back to posix_fd_storage due to "
                         "io_uring_fd_storage error: "
                      << e.what() << "\n";
        }
    }
#endif
    return std::make_unique<posix_fd_storage>(fd, std::move(reopen_path));
}

#include <fcntl.h>
#include <unistd.h>

inline xtr::detail::file_descriptor::file_descriptor(
    const char* path, int flags, int mode) :
    fd_(XTR_TEMP_FAILURE_RETRY(::open(path, flags, mode)))
{
    if (fd_ == -1)
    {
        throw_system_error_fmt(
            errno,
            "xtr::detail::file_descriptor::file_descriptor: "
            "Failed to open `%s'",
            path);
    }
}

inline xtr::detail::file_descriptor& xtr::detail::file_descriptor::operator=(
    xtr::detail::file_descriptor&& other) noexcept
{
    swap(*this, other);
    return *this;
}

inline xtr::detail::file_descriptor::~file_descriptor()
{
    reset();
}

inline void xtr::detail::file_descriptor::reset(int fd) noexcept
{
    if (is_open())
        (void)::close(fd_);
    fd_ = fd;
}

#if XTR_USE_IO_URING

#include <liburing.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>

inline xtr::io_uring_fd_storage::io_uring_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity,
    std::size_t queue_size,
    std::size_t batch_size,
    io_uring_submit_func_t io_uring_submit_func,
    io_uring_get_sqe_func_t io_uring_get_sqe_func,
    io_uring_wait_cqe_func_t io_uring_wait_cqe_func,
    io_uring_sqring_wait_func_t io_uring_sqring_wait_func,
    io_uring_peek_cqe_func_t io_uring_peek_cqe_func) :
    fd_storage_base(fd, std::move(reopen_path)),
    buffer_capacity_(buffer_capacity),
    batch_size_(batch_size),
    io_uring_submit_func_(io_uring_submit_func),
    io_uring_get_sqe_func_(io_uring_get_sqe_func),
    io_uring_wait_cqe_func_(io_uring_wait_cqe_func),
    io_uring_sqring_wait_func_(io_uring_sqring_wait_func),
    io_uring_peek_cqe_func_(io_uring_peek_cqe_func)
{
    if (buffer_capacity >
        std::numeric_limits<decltype(io_uring_cqe::res)>::max())
        detail::throw_invalid_argument("buffer_capacity too large");

    if (queue_size > std::numeric_limits<decltype(buffer::index_)>::max())
        detail::throw_invalid_argument("queue_size too large");

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

inline xtr::io_uring_fd_storage::io_uring_fd_storage(
    int fd,
    std::string reopen_path,
    std::size_t buffer_capacity,
    std::size_t queue_size,
    std::size_t batch_size) :
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

inline xtr::io_uring_fd_storage::~io_uring_fd_storage()
{
    flush();
    sync();
    ::io_uring_queue_exit(&ring_);
}

inline void xtr::io_uring_fd_storage::flush()
{
    io_uring_submit_func_(&ring_);
}

inline void xtr::io_uring_fd_storage::sync() noexcept
{
    while (pending_cqe_count_ > 0)
        wait_for_one_cqe();
    fd_storage_base::sync();
}

inline std::span<char> xtr::io_uring_fd_storage::allocate_buffer()
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

inline void xtr::io_uring_fd_storage::submit_buffer(char* data, std::size_t size)
{
    buffer* buf = buffer::data_to_buffer(data);
    buf->size_ = unsigned(size);

    io_uring_sqe* sqe = get_sqe();

    ::io_uring_prep_write_fixed(
        sqe,
        fd_.get(),
        buf->data_,
        buf->size_,
        buf->file_offset_,
        buf->index_);

    ::io_uring_sqe_set_data(sqe, buf);

    offset_ += size;
    ++pending_cqe_count_;

    if (++batch_index_ % batch_size_ == 0)
        io_uring_submit_func_(&ring_);
}

inline void xtr::io_uring_fd_storage::replace_fd(int newfd) noexcept
{
    io_uring_sqe* sqe = get_sqe();
    ::io_uring_prep_close(sqe, fd_.release());
    sqe->flags |= IOSQE_IO_DRAIN;
    ++pending_cqe_count_;
    io_uring_submit_func_(&ring_);

    fd_storage_base::replace_fd(newfd);
}

inline void xtr::io_uring_fd_storage::allocate_buffers(std::size_t queue_size)
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

inline io_uring_sqe* xtr::io_uring_fd_storage::get_sqe()
{
    io_uring_sqe* sqe;

    while ((sqe = io_uring_get_sqe_func_(&ring_)) == nullptr)
    {
#if XTR_IO_URING_POLL
        io_uring_sqring_wait_func_(&ring_);
#else
        wait_for_one_cqe();
#endif
    }

    return sqe;
}

inline void xtr::io_uring_fd_storage::wait_for_one_cqe()
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
            "Error: Write of %u bytes at offset %zu to \"%s\" (fd %d) failed: "
            "%s\n",
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

inline void xtr::io_uring_fd_storage::resubmit_buffer(
    buffer* buf, unsigned nwritten)
{
    buf->size_ -= nwritten;
    buf->offset_ += nwritten;
    buf->file_offset_ += nwritten;

    assert(io_uring_sq_space_left(&ring_) >= 1);

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

inline void xtr::io_uring_fd_storage::free_buffer(buffer* buf)
{
    assert(buf != nullptr);
    buf->next_ = free_list_;
    free_list_ = buf;
}

#endif

#include <cassert>

inline xtr::sink xtr::logger::get_sink(std::string name)
{
    return sink(*this, std::move(name), default_log_level_);
}

inline void xtr::logger::register_sink(sink& s, std::string name) noexcept
{
    assert(!s.open_);
    post([&s, name = std::move(name)](detail::consumer& c, auto&)
         { c.add_sink(s, name); });
    s.open_ = true;
}

inline void xtr::logger::set_command_path(std::string path) noexcept
{
    post([s = std::move(path)](detail::consumer& c, auto&) mutable
         { c.set_command_path(std::move(s)); });
    control_.sync();
}

inline void xtr::logger::set_log_level_style(log_level_style_t level_style) noexcept
{
    post([=](detail::consumer& c, auto&) { c.buf.lstyle = level_style; });
    control_.sync();
}

inline void xtr::logger::set_default_log_level(log_level_t level)
{
    default_log_level_ = level;
}

#define XTR_LOG_LEVELS \
    X(none)            \
    X(fatal)           \
    X(error)           \
    X(warning)         \
    X(info)            \
    X(debug)

inline xtr::log_level_t xtr::log_level_from_string(std::string_view str)
{
#define X(LEVEL)       \
    if (str == #LEVEL) \
        return log_level_t::LEVEL;
    XTR_LOG_LEVELS
#undef X
    detail::throw_invalid_argument("Invalid log level");
}

inline const char* xtr::default_log_level_style(log_level_t level)
{
    switch (level)
    {
    case log_level_t::fatal:
        return "F ";
    case log_level_t::error:
        return "E ";
    case log_level_t::warning:
        return "W ";
    case log_level_t::info:
        return "I ";
    case log_level_t::debug:
        return "D ";
    default:
        return "";
    }
}

inline const char* xtr::systemd_log_level_style(log_level_t level)
{
    switch (level)
    {
    case log_level_t::fatal:
        return "<0>"; // SD_EMERG
    case log_level_t::error:
        return "<3>"; // SD_ERR
    case log_level_t::warning:
        return "<4>"; // SD_WARNING
    case log_level_t::info:
        return "<6>"; // SD_INFO
    case log_level_t::debug:
        return "<7>"; // SD_DEBUG
    default:
        return "";
    }
}

inline std::unique_ptr<xtr::detail::matcher> xtr::detail::make_matcher(
    pattern_type_t pattern_type, const char* pattern, bool ignore_case)
{
    switch (pattern_type)
    {
    case pattern_type_t::wildcard:
        return std::make_unique<wildcard_matcher>(pattern, ignore_case);
    case pattern_type_t::extended_regex:
        return std::make_unique<regex_matcher>(pattern, ignore_case, true);
    case pattern_type_t::basic_regex:
        return std::make_unique<regex_matcher>(pattern, ignore_case, false);
    case pattern_type_t::none:
        break;
    }
    return std::make_unique<matcher>();
}

#include <cassert>
#include <cerrno>

inline xtr::detail::memory_mapping::memory_mapping(
    void* addr,
    std::size_t length,
    int prot,
    int flags,
    int fd,
    std::size_t offset) :
    mem_(::mmap(addr, length, prot, flags, fd, ::off_t(offset))),
    length_(length)
{
    if (mem_ == MAP_FAILED)
    {
        throw_system_error(
            errno,
            "xtr::detail::memory_mapping::memory_mapping: mmap failed");
    }
}

inline xtr::detail::memory_mapping::memory_mapping(memory_mapping&& other) noexcept
    :
    mem_(other.mem_),
    length_(other.length_)
{
    other.release();
}

inline xtr::detail::memory_mapping& xtr::detail::memory_mapping::operator=(
    memory_mapping&& other) noexcept
{
    swap(*this, other);
    return *this;
}

inline xtr::detail::memory_mapping::~memory_mapping()
{
    reset();
}

inline void xtr::detail::memory_mapping::reset(
    void* addr, std::size_t length) noexcept
{
    if (mem_ != MAP_FAILED)
    {
#ifndef NDEBUG
        const int result =
#endif
            ::munmap(mem_, length_);
        assert(result == 0);
    }
    mem_ = addr;
    length_ = length;
}

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <random>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#if !defined(__linux__)
namespace xtr::detail
{
    inline file_descriptor shm_open_anon(int oflag, mode_t mode)
    {
        int fd;

#if defined(SHM_ANON) // FreeBSD extension
        fd = XTR_TEMP_FAILURE_RETRY(::shm_open(SHM_ANON, oflag, mode));
#else
        const char ctable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789~-";
        char name[] = "/xtr.XXXXXXXXXXXXXXXX";

        std::random_device rd;
        std::uniform_int_distribution<> udist(0, sizeof(ctable) - 2);

        std::size_t retries = 64;
        do
        {
            for (char* pos = name + 5; *pos != '\0'; ++pos)
                *pos = ctable[udist(rd)];
            fd = XTR_TEMP_FAILURE_RETRY(
                ::shm_open(name, oflag | O_EXCL | O_CREAT, mode));
        } while (--retries > 0 && fd == -1 && errno == EEXIST);

        if (fd != -1)
            ::shm_unlink(name);
#endif

        return file_descriptor(fd);
    }
}
#endif

inline xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping(
    std::size_t length, int fd, std::size_t offset, int flags)
{
    assert(!(flags & MAP_ANONYMOUS) || fd == -1);
    assert((flags & MAP_FIXED) == 0); // Not implemented (would be easy though)
    assert(
        (flags & MAP_PRIVATE) ==
        0); // Can't be private, must be shared for mirroring to work

    if (length != align_to_page_size(length))
    {
        throw_invalid_argument(
            "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
            "Length argument is not page-aligned");
    }

    const int prot = PROT_READ | PROT_WRITE;

    memory_mapping reserve(nullptr, length * 2, prot, MAP_PRIVATE | MAP_ANONYMOUS);

#if !defined(__linux__)
    file_descriptor temp_fd;
#endif

    if (fd == -1)
    {
#if defined(__linux__)
        memory_mapping mirror(
            static_cast<std::byte*>(reserve.get()) + length,
            length,
            prot,
            MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS | flags);

        m_.reset(
            ::mremap(
                mirror.get(),
                0,
                length,
                MREMAP_FIXED | MREMAP_MAYMOVE,
                reserve.get()),
            length);

        if (!m_)
        {
            throw_system_error(
                errno,
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping:"
                " "
                "mremap failed");
        }

        reserve.release(); // mapping was destroyed by mremap
        mirror.release(); // mirror will be recreated in ~mirrored_memory_mapping
        return;
#else
        if (!(temp_fd = shm_open_anon(O_RDWR, S_IRUSR | S_IWUSR)))
        {
            throw_system_error(
                errno,
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping:"
                " "
                "Failed to shm_open backing file");
        }

        fd = temp_fd.get();

        if (::ftruncate(fd, ::off_t(length)) == -1)
        {
            throw_system_error(
                errno,
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping:"
                " "
                "Failed to ftruncate backing file");
        }
#endif
    }

    flags &= ~MAP_ANONYMOUS; // Mappings must be shared
    flags |= MAP_FIXED | MAP_SHARED;

    memory_mapping mirror(
        static_cast<std::byte*>(reserve.get()) + length,
        length,
        prot,
        flags,
        fd,
        offset);

    m_ = memory_mapping(reserve.get(), length, prot, flags, fd, offset);

    reserve.release(); // mapping was destroyed when m_ was created
    mirror.release();  // mirror will be recreated in ~mirrored_memory_mapping
}

inline xtr::detail::mirrored_memory_mapping::~mirrored_memory_mapping()
{
    if (m_)
    {
        memory_mapping{}.reset(
            static_cast<std::byte*>(m_.get()) + m_.length(),
            m_.length());
    }
}

#include <cerrno>

#include <unistd.h>

inline std::size_t xtr::detail::align_to_page_size(std::size_t length)
{
    static const long pagesize(::sysconf(_SC_PAGESIZE));
    if (pagesize == -1)
        throw_system_error(errno, "sysconf(_SC_PAGESIZE) failed");
    return align(length, std::size_t(pagesize));
}

#include <cerrno>
#include <memory>
#include <utility>

#include <unistd.h>

inline xtr::posix_fd_storage::posix_fd_storage(
    int fd, std::string reopen_path, std::size_t buffer_capacity) :
    fd_storage_base(fd, std::move(reopen_path)),
    buf_(new char[buffer_capacity]),
    buffer_capacity_(buffer_capacity)
{
}

inline std::span<char> xtr::posix_fd_storage::allocate_buffer()
{
    return {buf_.get(), buffer_capacity_};
}

inline void xtr::posix_fd_storage::submit_buffer(char* buf, std::size_t size)
{
    while (size > 0)
    {
        const ::ssize_t nwritten =
            XTR_TEMP_FAILURE_RETRY(::write(fd_.get(), buf, size));
        if (nwritten == -1)
        {
            detail::throw_system_error_fmt(
                errno,
                "xtr::posix_fd_storage::submit_buffer: write failed");
            return;
        }
        size -= std::size_t(nwritten);
        buf += std::size_t(nwritten);
    }
}

#include <cassert>

inline xtr::detail::regex_matcher::regex_matcher(
    const char* pattern, bool ignore_case, bool extended)
{
    const int flags = REG_NOSUB | (ignore_case ? REG_ICASE : 0) |
                      (extended ? REG_EXTENDED : 0);
    errnum_ = ::regcomp(&regex_, pattern, flags);
}

inline xtr::detail::regex_matcher::~regex_matcher()
{
    if (valid())
        ::regfree(&regex_);
}

inline bool xtr::detail::regex_matcher::valid() const
{
    return errnum_ == 0;
}

inline void xtr::detail::regex_matcher::error_reason(
    char* buf, std::size_t bufsz) const
{
    assert(errnum_ != 0);
    ::regerror(errnum_, &regex_, buf, bufsz);
}

inline bool xtr::detail::regex_matcher::operator()(const char* str) const
{
    return ::regexec(&regex_, str, 0, nullptr, 0) == 0;
}

#include <condition_variable>
#include <mutex>

inline xtr::sink::sink(log_level_t level) : level_(level)
{
}

inline xtr::sink::sink(const sink& other)
{
    *this = other;
}

inline xtr::sink& xtr::sink::operator=(const sink& other)
{
    if (this == &other) [[unlikely]]
        return *this;

    close();

    level_ = other.level_.load(std::memory_order_relaxed);

    if (other.open_)
    {
        const_cast<sink&>(other).post(
            [this](detail::consumer& c, const auto& name)
            { c.add_sink(*this, name); });
        open_ = true;
    }

    return *this;
}

inline xtr::sink::sink(logger& owner, std::string name, log_level_t level) :
    level_(level)
{
    owner.register_sink(*this, std::move(name));
}

inline void xtr::sink::close()
{
    if (open_)
    {
        sync_post([](detail::consumer& c) { c.destroy = true; });
        open_ = false;
        buf_.clear();
    }
}

inline bool xtr::sink::is_open() const noexcept
{
    return open_;
}

inline void xtr::sink::sync()
{
    sync_post(
        [](detail::consumer& c)
        {
            c.buf.flush();
            c.buf.storage().sync();
        });
}

template<typename Func>
void xtr::sink::sync_post(Func&& func)
{
    std::condition_variable cv;
    std::mutex m;
    bool notified = false; // protected by m

    post(
        [&cv, &m, &notified, &func](detail::consumer& c, auto&)
        {
            func(c);

            std::scoped_lock lock{m};
            notified = true;
            cv.notify_one();
        });

    std::unique_lock lock{m};
    while (!notified)
        cv.wait(lock);
}

inline void xtr::sink::set_name(std::string name)
{
    post([name = std::move(name)](auto&, auto& oldname) mutable
         { oldname = std::move(name); });
}

inline xtr::sink::~sink()
{
    close();
}

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <system_error>

inline void xtr::detail::throw_runtime_error(const char* what)
{
#if __cpp_exceptions
    throw std::runtime_error(what);
#else
    std::fprintf(stderr, "runtime error: %s\n", what);
    std::abort();
#endif
}

inline void xtr::detail::throw_runtime_error_fmt(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    ;
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
#if __cpp_exceptions
    throw std::runtime_error(buf);
#else
    std::fprintf(stderr, "runtime error: %s\n", buf);
    std::abort();
#endif
}

inline void xtr::detail::throw_system_error(int errnum, const char* what)
{
#if __cpp_exceptions
    throw std::system_error(
        std::error_code(errnum, std::system_category()),
        what);
#else
    std::fprintf(stderr, "system error: %s: %s\n", what, std::strerror(errnum));
    std::abort();
#endif
}

inline void xtr::detail::throw_system_error_fmt(
    int errnum, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    ;
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
#if __cpp_exceptions
    throw std::system_error(std::error_code(errnum, std::system_category()), buf);
#else
    std::fprintf(stderr, "system error: %s: %s\n", buf, std::strerror(errnum));
    std::abort();
#endif
}

inline void xtr::detail::throw_invalid_argument(const char* what)
{
#if __cpp_exceptions
    throw std::invalid_argument(what);
#else
    std::fprintf(stderr, "invalid argument: %s\n", what);
    std::abort();
#endif
}

inline void xtr::detail::throw_bad_alloc()
{
#if __cpp_exceptions
    throw std::bad_alloc();
#else
    std::fprintf(stderr, "bad alloc\n");
    std::abort();
#endif
}

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

#include <time.h>

inline std::uint64_t xtr::detail::get_tsc_hz() noexcept
{
    __extension__ static std::uint64_t tsc_hz =
        read_tsc_hz() ?: estimate_tsc_hz();
    return tsc_hz;
}

inline std::uint64_t xtr::detail::read_tsc_hz() noexcept
{
    constexpr int tsc_leaf = 0x15;

    if (cpuid(0)[0] < tsc_leaf)
        return 0;

    auto [ratio_den, ratio_num, ccc_hz, unused] = cpuid(0x15);

    if (ccc_hz == 0)
    {
        const std::uint16_t model = get_family_model()[1];
        switch (model)
        {
        case intel_fam6_skylake_l:
        case intel_fam6_skylake:
        case intel_fam6_kabylake_l:
        case intel_fam6_kabylake:
        case intel_fam6_cometlake_l:
        case intel_fam6_cometlake:
            ccc_hz = 24000000; // 24 MHz
            break;
        case intel_fam6_atom_tremont_d:
        case intel_fam6_atom_goldmont_d:
            ccc_hz = 25000000; // 25 MHz
            break;
        case intel_fam6_atom_goldmont:
        case intel_fam6_atom_goldmont_plus:
            ccc_hz = 19200000; // 19.2 MHz
            break;
        default:
            return 0; // Unknown CPU or crystal frequency
        }
    }

    return std::uint64_t(ccc_hz) * ratio_num / ratio_den;
}

inline std::uint64_t xtr::detail::estimate_tsc_hz() noexcept
{
    const std::uint64_t tsc0 = tsc::now().ticks;
    std::timespec ts0;
    ::clock_gettime(XTR_CLOCK_MONOTONIC, &ts0);

    std::array<std::uint64_t, 5> history;
    std::size_t n = 0;

    const auto sleep_time = std::chrono::milliseconds(10);
    const auto max_sleep_time = std::chrono::seconds(2);
    const std::size_t tick_range = 1000;
    const std::size_t max_iters = max_sleep_time / sleep_time;

    for (;;)
    {
        std::this_thread::sleep_for(sleep_time);

        const std::uint64_t tsc1 = tsc::now().ticks;
        std::timespec ts1;
        ::clock_gettime(XTR_CLOCK_MONOTONIC, &ts1);

        const auto elapsed_nanos =
            std::uint64_t(ts1.tv_sec - ts0.tv_sec) * 1000000000UL +
            std::uint64_t(ts1.tv_nsec) - std::uint64_t(ts0.tv_nsec);

        const std::uint64_t elapsed_ticks = tsc1 - tsc0;

        const std::uint64_t tsc_hz =
            std::uint64_t(double(elapsed_ticks) * 1e9 / double(elapsed_nanos));

        history[n++ % history.size()] = tsc_hz;

        if (n >= history.size())
        {
            const auto min = *std::min_element(history.begin(), history.end());
            const auto max = *std::max_element(history.begin(), history.end());
            if (max - min < tick_range || n >= max_iters)
                return tsc_hz;
        }
    }
}

inline std::timespec xtr::detail::tsc::to_timespec(tsc ts)
{
    thread_local tsc last_tsc{};
    thread_local std::int64_t last_epoch_nanos;
    static const std::uint64_t one_minute_ticks = 60 * get_tsc_hz();
    static const double tsc_multiplier = 1e9 / double(get_tsc_hz());

    if (ts.ticks > last_tsc.ticks + one_minute_ticks)
    {
        last_tsc = tsc::now();
        std::timespec temp;
        ::clock_gettime(XTR_CLOCK_WALL, &temp);
        last_epoch_nanos = temp.tv_sec * 1000000000L + temp.tv_nsec;
    }

    const auto tick_delta = std::int64_t(ts.ticks - last_tsc.ticks);
    const auto nano_delta = std::int64_t(double(tick_delta) * tsc_multiplier);
    const auto total_nanos = std::uint64_t(last_epoch_nanos + nano_delta);

    std::timespec result;
    result.tv_sec = std::time_t(total_nanos / 1000000000UL);
    result.tv_nsec = long(total_nanos % 1000000000UL);

    return result;
}

#include <fnmatch.h>

inline xtr::detail::wildcard_matcher::wildcard_matcher(
    const char* pattern, bool ignore_case) :
    pattern_(pattern),
    flags_(ignore_case ? FNM_CASEFOLD : 0)
{
}

inline bool xtr::detail::wildcard_matcher::operator()(const char* str) const
{
    return ::fnmatch(pattern_, str, flags_) == 0;
}

#endif
