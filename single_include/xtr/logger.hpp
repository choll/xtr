/*
MIT License

Copyright (c) 2020 Chris E. Holloway

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
#ifndef XTR_TIMESPEC_HPP
#define XTR_TIMESPEC_HPP

#include <ctime>

#include <fmt/chrono.h>

namespace xtr
{
    // This class exists to avoid clashing with user code---if a formatter
    // was created for std::timespec then it may conflict with a user
    // defined formatter.
    struct timespec : std::timespec
    {
        timespec() = default;

        timespec(std::timespec ts)
        :
            std::timespec(ts)
        {
        }
    };
}

namespace fmt
{
    template<>
    struct formatter<xtr::timespec>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::timespec ts, FormatContext &ctx)
        {
            std::tm temp;
            return
                fmt::format_to(
                    ctx.out(),
                    "{:%Y-%m-%d %T}.{:06}",
                    *::gmtime_r(&ts.tv_sec, &temp),
                    ts.tv_nsec / 1000);
        }
    };
}

#endif


#ifndef XTR_TAGS_HPP
#define XTR_TAGS_HPP

namespace xtr
{
    struct non_blocking_tag;
    struct timestamp_tag;
}

#endif


#ifndef XTR_DETAIL_THROW_HPP
#define XTR_DETAIL_THROW_HPP

namespace xtr::detail
{
    // errno isn't passed as an argument just to make the calling code
    // marginally smaller (i.e. avoid a call to __errno_location)
    [[noreturn, gnu::cold]] void throw_system_error(const char* what);

    [[noreturn, gnu::cold, gnu::format(printf, 1, 2)]]
    void throw_system_error_fmt(const char* format, ...);

    [[noreturn, gnu::cold]] void throw_invalid_argument(const char* what);
}

#endif


#ifndef XTR_DETAIL_RETRY_HPP
#define XTR_DETAIL_RETRY_HPP

#define XTR_TEMP_FAILURE_RETRY(expr)                    \
    (__extension__                                      \
        ({                                              \
            decltype(expr) xtr_result;                  \
            do xtr_result = (expr);                     \
            while (xtr_result == -1 && errno == EINTR); \
            xtr_result;                                 \
        }))

#endif


#ifndef XTR_DETAIL_ALIGN_HPP
#define XTR_DETAIL_ALIGN_HPP

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail
{
    // value is unchanged if it is already aligned
    template<typename T>
    constexpr T align(T value, T alignment) noexcept
    {
        static_assert(std::is_unsigned_v<T>, "value must be unsigned");
        assert(std::has_single_bit(alignment));
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    // Align is only a template argument to allow it to be used with
    // assume_aligned. Improvement: Make Align a plain argument if it is
    // possible to do so while still marking the return value as aligned.
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

#endif


#ifndef XTR_DETAIL_PAGESIZE_HPP
#define XTR_DETAIL_PAGESIZE_HPP

#include <cstddef>

namespace xtr::detail
{
    std::size_t align_to_page_size(std::size_t length);
}

#endif

#ifndef XTR_DETAIL_CPUID_HPP
#define XTR_DETAIL_CPUID_HPP

#include <array>
#include <cstdint>


namespace xtr::detail
{
    constexpr inline std::uint32_t intel_fam6_skylake_l          = 0x4E;
    constexpr inline std::uint32_t intel_fam6_skylake            = 0x5E;
    constexpr inline std::uint32_t intel_fam6_kabylake_l         = 0x8E;
    constexpr inline std::uint32_t intel_fam6_kabylake           = 0x9E;
    constexpr inline std::uint32_t intel_fam6_cometlake          = 0xA5;
    constexpr inline std::uint32_t intel_fam6_cometlake_l        = 0xA6;
    constexpr inline std::uint32_t intel_fam6_atom_tremont_d     = 0x86;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_d    = 0x5F;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont      = 0x5C;
    constexpr inline std::uint32_t intel_fam6_atom_goldmont_plus = 0x7A;

    inline std::array<std::uint32_t, 4> cpuid(int leaf, int subleaf = 0) noexcept
    {
        std::array<std::uint32_t, 4> out;
        asm (
            "cpuid" :
            // output
            "=a" (out[0]),
            "=b" (out[1]),
            "=c" (out[2]),
            "=d" (out[3]) :
            // input
            "a" (leaf),
            "c" (subleaf));
        return out;
    }

    inline std::array<std::uint16_t, 2> get_family_model() noexcept
    {
        // See https://www.felixcloutier.com/x86/cpuid#fig-3-6
        const std::uint32_t fms = cpuid(0x1)[0];
        std::uint16_t model = (fms & 0xF0) >> 4;
        std::uint16_t family = (fms & 0xF00) >> 8;
        if (family == 0xF)
            family |= std::uint16_t((fms & 0xFF00000) >> 16);
        if (family == 0x6 || family == 0xF)
            model |= std::uint16_t((fms & 0xF0000) >> 12);
        return {family,  model};
    }
}

#endif


#ifndef XTR_DETAIL_IS_C_STRING_HPP
#define XTR_DETAIL_IS_C_STRING_HPP

#include <type_traits>

namespace xtr::detail
{
    // This is long-winded because remove_cv on a const pointer removes const
    // from the pointer, not from the the object being pointed to, so we have
    // to use remove_pointer before applying remove_cv. decay turns arrays
    // into pointers and removes references.
    template<typename T>
    using is_c_string =
        std::conjunction<
            std::is_pointer<std::decay_t<T>>,
            std::is_same<
                char,
                std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>>>;

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

#endif


#ifndef XTR_DETAIL_FILE_DESCRIPTOR_HPP
#define XTR_DETAIL_FILE_DESCRIPTOR_HPP

#include <utility>

namespace xtr::detail
{
    class file_descriptor;
}

class xtr::detail::file_descriptor
{
public:
    file_descriptor() = default;

    explicit file_descriptor(int fd)
    :
        fd_(fd)
    {
    }

    file_descriptor(const char* path, int flags, int mode = 0);

    file_descriptor(const file_descriptor&) = delete;

    file_descriptor(file_descriptor&& other) noexcept
    :
        fd_(other.fd_)
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

#endif


#ifndef XTR_DETAIL_MEMORY_MEMORY_MAPPING_HPP
#define XTR_DETAIL_MEMORY_MEMORY_MAPPING_HPP

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
        int fd = - 1,
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

#endif


#ifndef XTR_DETAIL_MEMORY_MIRRORED_MEMORY_MAPPING_HPP
#define XTR_DETAIL_MEMORY_MIRRORED_MEMORY_MAPPING_HPP


#include <cstddef>
#include <utility>


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

#endif


#ifndef XTR_DETAIL_PAUSE_HPP
#define XTR_DETAIL_PAUSE_HPP

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

#endif


#ifndef XTR_DETAIL_SANITIZE_HPP
#define XTR_DETAIL_SANITIZE_HPP

namespace xtr::detail
{
    // Transforms non-printable characters and backslash to hex (\xFF).
    // Backslash is treated like this to prevent terminal escape
    // sequence injection attacks.
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

#endif


#ifndef XTR_DETAIL_STRING_REF_HPP
#define XTR_DETAIL_STRING_REF_HPP


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
        explicit string_ref(const char* s)
        :
            str(s)
        {
        }

        explicit string_ref(const std::string& s)
        :
            str(s.c_str())
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
        explicit string_ref(std::string_view s)
        :
            str(s)
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
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(xtr::detail::string_ref<const char*> ref, FormatContext &ctx)
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
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::detail::string_ref<std::string_view> ref, FormatContext &ctx)
        {
            auto pos = ctx.out();
            for (const char c : ref.str)
                xtr::detail::sanitize_char_to(pos, c);
            return pos;
        }
    };
}

#endif


#ifndef XTR_DETAIL_TAGS_HPP
#define XTR_DETAIL_TAGS_HPP


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

#endif


#ifndef XTR_DETAIL_SYNCHRONIZED_RING_BUFFER_HPP
#define XTR_DETAIL_SYNCHRONIZED_RING_BUFFER_HPP


#include <atomic>
#include <bit>
#include <cstddef>
#include <limits>
#include <new>
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

#if defined(__has_cpp_attribute) && \
    __has_cpp_attribute(__cpp_lib_hardware_interference_size)
    inline constexpr std::size_t cacheline_size =
        std::hardware_destructive_interference_size;
#else
    inline constexpr std::size_t cacheline_size = 64;
#endif

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

    // XXX Put in detail/ directory? explain non-use of std::span?
    // Probably ring buffer should be in detail too,
    // everything except the logger really
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

    //using span = yeti::span<std::byte>;
    //using const_span = yeti::span<const std::byte>;

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
        nwritten_.fetch_add(nbytes, std::memory_order_release);
        wrnwritten_ += nbytes;
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
        assert(nread_plus_capacity_.load() - nwritten_.load() <= capacity());
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


#ifndef XTR_DETAIL_INTERPROCESS_RING_BUFFER_HPP
#define XTR_DETAIL_INTERPROCESS_RING_BUFFER_HPP


#include <cassert>

namespace xtr::detail
{
    class interprocess_ring_buffer;
}

    // So, the issue is that this will not store control variables in the
    // buffer... what you actually need is to create a normal memory mapping,
    //
    // synchronized_ring_buffer could accept a mirrored_memory_mapping?
    //
    // So:
    //
    // open fd, ftruncate to getpagesize()+64*1024
    //
    // memory_mapping control_{nullptr, getpagesize(), prot, flags, fd};
    // mirrored_memory_mapping buf_{64*1024, fd, getpagesize(), ;
    //
    // synchronized_ring_buffer* ring_buffer =
    //     new(control_.get()) synchronized_ring_buffer
    //
    //
    //    const int fd = /*YETI_TEMP_FAILURE_RETRY*/(open(path, flags, mode));
    //    if (fd == -1)
    //    {
            //throw_system_error_fmt(
            //    "xtr::synchronized_ring_buffer::synchronized_ring_buffer: "
            //    "Failed to open `%s'", path);
    //    }
    //
    //

class xtr::detail::interprocess_ring_buffer
{
private:
    static constexpr std::size_t capacity = 64 * 1024;

    using ring_buffer = synchronized_ring_buffer<capacity>;

public:
    using span = ring_buffer::span;
    using const_span = ring_buffer::const_span;
    using size_type = ring_buffer::size_type;

    explicit interprocess_ring_buffer(const char* path, int flags, int mode = 0);

    interprocess_ring_buffer() = default;
    interprocess_ring_buffer(interprocess_ring_buffer&&) = default;
    interprocess_ring_buffer& operator=(interprocess_ring_buffer&&) = default;

    ~interprocess_ring_buffer();

    const_span read_span() noexcept
    {
        if (!buf_) [[unlikely]]
            return span{};
        return ring_buf()->read_span();
    }

    span write_span() noexcept
    {
        if (!buf_) [[unlikely]]
            return span{};
        return ring_buf()->write_span();
    }

    void reduce_readable(size_type nbytes) noexcept
    {
        ring_buf()->reduce_readable(nbytes);
    }

private:
    ring_buffer* ring_buf() noexcept
    {
        return static_cast<ring_buffer*>(buf_.get());
    }

    memory_mapping buf_;
};

#endif

#ifndef XTR_DETAIL_TSC_HPP
#define XTR_DETAIL_TSC_HPP


#include <cstdint>
#include <ctime>

#include <fmt/chrono.h>

namespace xtr::detail
{
    struct tsc;

    std::uint64_t read_tsc_hz() noexcept;
    std::uint64_t estimate_tsc_hz() noexcept;

    inline std::uint64_t get_tsc_hz() noexcept
    {
        __extension__ static std::uint64_t tsc_hz =
            read_tsc_hz() ?: estimate_tsc_hz();
        return tsc_hz;
    }
}

struct xtr::detail::tsc
{
    inline static tsc now() noexcept
    {
        std::uint32_t a, d;
        asm volatile(
            "rdtsc;"
            : "=a" (a), "=d" (d)); // output, a=eax, d=edx
        return
            {static_cast<std::uint64_t>(a) | (static_cast<uint64_t>(d) << 32)};
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
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const xtr::detail::tsc ticks, FormatContext &ctx)
        {
            const auto ts = xtr::detail::tsc::to_timespec(ticks);
            return formatter<xtr::timespec>().format(ts, ctx);
        }
    };
}

#endif

#ifndef XTR_DETAIL_CLOCK_IDS_HPP
#define XTR_DETAIL_CLOCK_IDS_HPP

#include <time.h>

#if defined(CLOCK_REALTIME_COARSE)
#define XTR_CLOCK_REALTIME_FAST CLOCK_REALTIME_COARSE
#elif defined (CLOCK_REALTIME_FAST)
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

#endif

#ifndef XTR_DETAIL_GET_TIME_HPP
#define XTR_DETAIL_GET_TIME_HPP


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

#endif


#ifndef XTR_DETAIL_PRINT_HPP
#define XTR_DETAIL_PRINT_HPP


#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace xtr::detail
{
    template<typename ErrorFunction,typename Timestamp>
    [[gnu::cold, gnu::noinline]] void report_error(
        fmt::memory_buffer& mbuf,
        const ErrorFunction& err,
        Timestamp ts,
        const std::string& name,
        const char* reason)
    {
        using namespace std::literals::string_view_literals;
        mbuf.clear();
        fmt::format_to(mbuf, "{}: {}: Error: {}\n"sv, ts, name, reason);
        err(mbuf.data(), mbuf.size());
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Timestamp,
        typename... Args>
    void print(
        fmt::memory_buffer& mbuf,
        const OutputFunction& out,
        [[maybe_unused]] const ErrorFunction& err,
        std::string_view fmt,
        Timestamp ts,
        const std::string& name,
        const Args&... args)
    {
#if __cpp_exceptions
        try
        {
#endif
            mbuf.clear();
            fmt::format_to(mbuf, fmt, ts, name, args...);
            const auto result = out(mbuf.data(), mbuf.size());
            if (result == -1)
                return report_error(mbuf, err, ts, name, "Write error");
            if (std::size_t(result) != mbuf.size())
                return report_error(mbuf, err, ts, name, "Short write");
#if __cpp_exceptions
        }
        catch (const std::exception& e)
        {
            report_error(mbuf, err, ts, name, e.what());
        }
#endif
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Timestamp,
        typename... Args>
    void print_ts(
        fmt::memory_buffer& mbuf,
        const OutputFunction& out,
        const ErrorFunction& err,
        std::string_view fmt,
        const std::string& name,
        Timestamp ts,
        const Args&... args)
    {
        print(mbuf, out, err, fmt, ts, name, args...);
    }
}

#endif


#ifndef XTR_DETAIL_STRING_HPP
#define XTR_DETAIL_STRING_HPP

#include <cstddef>
#include <string_view>

namespace xtr::detail
{
    template<std::size_t N>
    struct string
    {
        constexpr operator std::string_view() const noexcept
        {
            return std::string_view{str, N};
        }

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

#endif


#ifndef XTR_DETAIL_STRING_TABLE_HPP
#define XTR_DETAIL_STRING_TABLE_HPP


#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace xtr::detail
{
    // Forward if T is either:
    // * std::string&&
    // * Not a C string, string, or string_view
    // C string and string_view r-value references are not forwarded because
    // they are non-owning so being moved is meaningless.
    template<typename Tags, typename T, typename Buffer>
    requires
        (std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))> &&
         std::same_as<std::remove_cvref_t<T>, std::string>) ||
        (!is_c_string<T>::value &&
         !std::same_as<std::remove_cvref_t<T>, std::string> &&
         !std::same_as<std::remove_cvref_t<T>, std::string_view>)
    T&& build_string_table(std::byte*&, std::byte*&, Buffer&, T&& value)
    {
        return std::forward<T>(value);
    }

    template<typename Tags, typename Buffer, typename String>
    requires
        std::same_as<String, std::string> ||
        std::same_as<String, std::string_view>
    string_ref<const char*> build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const String& sv)
    {
        std::byte* str_end = pos + sv.length();
        while (end < str_end + 1) [[unlikely]]
        {
            detail::pause();
            const auto s = buf.write_span();
            if (s.end() < str_end + 1) [[unlikely]]
            {
                if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                    return string_ref("<truncated>");
            }
            end = s.end();
        }
        const char* result = reinterpret_cast<char*>(pos);
        const char* str = sv.data();
        while (pos != str_end)
            new (pos++) char(*str++);
        new (pos++) char('\0');
        return string_ref(result);
    }

    template<typename Tags, typename Buffer>
    string_ref<const char*> build_string_table(
        std::byte*& pos,
        std::byte*& end,
        Buffer& buf,
        const char* str)
    {
        const char* result = reinterpret_cast<char*>(pos);
        do
        {
            while (pos == end) [[unlikely]]
            {
                detail::pause();
                const auto s = buf.write_span();
                if (s.end() == end) [[unlikely]]
                {
                    if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
                        return string_ref("<truncated>");
                }
                end = s.end();
            }
            new (pos++) char(*str);
        } while (*str++ != '\0');
        return string_ref(result);
    }
}

#endif


#ifndef XTR_DETAIL_TRAMPOLINES_HPP
#define XTR_DETAIL_TRAMPOLINES_HPP


#include <fmt/format.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace xtr::detail
{
    // trampoline_no_capture
    // trampoline_fixed_size_capture
    // trampoline_variable_size_capture

    template<auto Format, typename State>
    std::byte* trampoline0(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        const char* ts,
        const std::string& name) noexcept
    {
        print(mbuf, state.out, state.err, *Format, ts, name);
        return buf + sizeof(void(*)());
    }

    template<auto Format, typename State, typename Func>
    std::byte* trampolineN(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        [[maybe_unused]] const char* ts,
        const std::string& name) noexcept
    {
        typedef void(*fptr_t)();

        auto func_pos = buf + sizeof(fptr_t);
        if constexpr (alignof(Func) > alignof(fptr_t))
            func_pos = align<alignof(Func)>(func_pos);

        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        // Invoke lambda, the first call is for commands sent to the consumer
        // thread such as adding a new producer or modifying the output stream.
        auto& func = *reinterpret_cast<Func*>(func_pos);
        if constexpr (std::is_same_v<decltype(Format), std::nullptr_t>)
            func(state);
        else
            func(mbuf, state.out, state.err, *Format, ts, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return func_pos + align(sizeof(Func), alignof(fptr_t));
    }

    template<auto Format, typename State, typename Func>
    std::byte* trampolineS(
        fmt::memory_buffer& mbuf,
        std::byte* buf,
        State& state,
        const char* ts,
        const std::string& name) noexcept
    {
        typedef void(*fptr_t)();

        auto size_pos = buf + sizeof(fptr_t);
        assert(std::uintptr_t(size_pos) % alignof(std::size_t) == 0);

        auto func_pos = size_pos + sizeof(std::size_t);
        if constexpr (alignof(Func) > alignof(std::size_t))
            func_pos = align<alignof(Func)>(func_pos);
        assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

        auto& func = *reinterpret_cast<Func*>(func_pos);
        func(mbuf, state.out, state.err, *Format, ts, name);

        static_assert(noexcept(func.~Func()));
        std::destroy_at(std::addressof(func));

        return buf + *reinterpret_cast<const std::size_t*>(size_pos);
    }
}

#endif


#ifndef XTR_LOGGER_HPP
#define XTR_LOGGER_HPP


#include <fmt/format.h>

#include <algorithm>
#include <concepts>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

#define XTR_LIKELY(x)      __builtin_expect(!!(x), 1)
#define XTR_UNLIKELY(x)    __builtin_expect(!!(x), 0)


#define XTR_LOG(...)                            \
    (__extension__                              \
        ({                                      \
            XTR_LOG_TAGS(void(), __VA_ARGS__);  \
        }))

#define XTR_TRY_LOG(...)                                        \
    (__extension__                                              \
        ({                                                      \
            XTR_LOG_TAGS(xtr::non_blocking_tag, __VA_ARGS__);   \
        }))

#define XTR_LOG_TS(...)            \
    XTR_LOG_TAGS(                  \
        xtr::timestamp_tag         \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TS(...)         \
    XTR_LOG_TAGS(                   \
        (xtr::non_blocking_tag,     \
            xtr::timestamp_tag)     \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_LOG_RTC(SINK, FMT, ...)                         \
    XTR_LOG_TS(                                             \
        SINK,                                               \
        FMT,                                                \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()    \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_RTC(SINK, FMT, ...)                     \
    XTR_TRY_LOG_TS(                                         \
        SINK,                                               \
        FMT,                                                \
        xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()    \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_LOG_TSC(SINK, FMT, ...) \
    XTR_LOG_TS(                     \
        SINK,                       \
        FMT,                        \
        xtr::detail::tsc::now()     \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_TRY_LOG_TSC(SINK, FMT, ...) \
    XTR_TRY_LOG_TS(                     \
        SINK,                           \
        FMT,                            \
        xtr::detail::tsc::now()         \
        __VA_OPT__(,) __VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

#define XTR_LOG_TAGS(TAGS, SINK, FORMAT, ...)                               \
    (__extension__                                                          \
        ({                                                                  \
            static constexpr auto xtr_fmt =                                 \
                xtr::detail::string{"{}: {}: "} +                           \
                xtr::detail::rcut<                                          \
                    xtr::detail::rindex(__FILE__, '/') + 1>(__FILE__) +     \
                xtr::detail::string{":"} +                                  \
                xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};   \
            using xtr::nocopy;                                              \
            (SINK).log<&xtr_fmt, void(TAGS)>(__VA_ARGS__);                  \
        }))

namespace xtr
{
    class logger;

    template<typename T>
    using nocopy = detail::string_ref<T>;
}

namespace xtr::detail
{
    inline auto make_output_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                return std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_error_func(FILE* stream)
    {
        return
            [stream](const char* buf, std::size_t size)
            {
                (void)std::fwrite(buf, 1, size, stream);
            };
    }

    inline auto make_flush_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
                std::fflush(stream);
                std::fflush(err_stream);
            };
    }

    inline auto make_sync_func(FILE* stream, FILE* err_stream)
    {
        return
            [stream, err_stream]()
            {
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
                ::fsync(::fileno(stream));
                ::fsync(::fileno(err_stream));
#endif
            };
    }
}

class xtr::logger
{
private:
    using ring_buffer = detail::synchronized_ring_buffer<64 * 1024>;

    class consumer;

    using fptr_t =
        std::byte* (*)(
            fmt::memory_buffer& mbuf,
            std::byte* buf, // pointer to log record
            consumer&,
            const char* timestamp,
            const std::string& name) noexcept;

public:
    // XXX is sink a better name? log_sink? logger::sink?
    class producer
    {
    public:
        ~producer();

        void close();

        void sync()
        {
            sync(/*destruct=*/false);
        }

        void set_name(std::string name);

        template<auto Format, typename Tags = void()>
        void log() noexcept;

        // FIXME: noexcept check is stricter than necessary
        template<auto Format, typename Tags = void(), typename... Args>
        void log(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

    private:
        producer() = default;

        producer(logger& owner, std::string name);

        template<typename T>
        void copy(std::byte* pos, T&& value) noexcept; // XXX noexcept

        template<
            auto Format = nullptr,
            typename Tags = void(),
            typename Func>
        void post(Func&& func)
            noexcept(std::is_nothrow_move_constructible_v<Func>);

        template<auto Format, typename Tags, typename... Args>
        void post_with_str_table(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

        template<typename Tags, typename... Args>
        auto make_lambda(Args&&... args)
            noexcept(std::conjunction_v<
                std::is_nothrow_copy_constructible<Args>...,
                std::is_nothrow_move_constructible<Args>...>);

        void sync(bool destruct);

        ring_buffer buf_;
        std::string name_;
        bool closed_ = false;
        friend logger;
    };

private:
    class consumer
    {
    public:
        void run(std::function<::timespec()> clock) noexcept;
        void process_commands() noexcept;
        void set_command_path(std::string path) noexcept;

        template<typename O, typename E, typename F, typename S>
        consumer(O&& o, E&& e, F&& f, S&& s, producer* control)
        :
            out(std::move(o)),
            err(std::move(e)),
            flush(std::move(f)),
            sync(std::move(s)),
            producers_({control})
        {
        }

        consumer(consumer&&) = default;
        ~consumer();

        void add_producer(producer& p)
        {
            producers_.push_back(&p);
        }

        std::function<::ssize_t(const char* buf, std::size_t size)> out;
        std::function<void(const char* buf, std::size_t size)> err;
        std::function<void()> flush;
        std::function<void()> sync;
        bool destroy = false;

    private:
        std::vector<producer*> producers_;
        int cmd_fd_; // XXX file_descriptor
        std::string cmd_path_;
    };

public:

    // XXX
    // const char* path option?
    //
    // should logs auto-rotate? don't think so, just write to one file,
    // other issue is if you should overwrite, append or create a new
    // file?
    //
    // rotating files?
    //
    // Also perhaps have a single constructor that accepts
    // variadic args, then just check the type of them? eg
    // is_same<FILE*>, is_clock_v?
    //
    //
    //

    template<typename Clock = std::chrono::system_clock>
    logger(
        FILE* stream = stderr,
        FILE* err_stream = stderr,
        Clock&& clock = Clock())
    :
        logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            std::forward<Clock>(clock))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename Clock = std::chrono::system_clock>
    requires
        std::invocable<OutputFunction, const char*, std::size_t> &&
        std::invocable<ErrorFunction, const char*, std::size_t>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    :
        logger(
            std::forward<OutputFunction>(out),
            std::forward<ErrorFunction>(err),
            [](){}, // flush
            [](){}, // sync
            std::forward<Clock>(clock),
            std::move(command_path))
    {
    }

    template<
        typename OutputFunction,
        typename ErrorFunction,
        typename FlushFunction,
        typename SyncFunction,
        typename Clock = std::chrono::system_clock>
    requires
        std::invocable<OutputFunction, const char*, std::size_t> &&
        std::invocable<ErrorFunction, const char*, std::size_t> &&
        std::invocable<FlushFunction> &&
        std::invocable<SyncFunction>
    logger(
        OutputFunction&& out,
        ErrorFunction&& err,
        FlushFunction&& flush,
        SyncFunction&& sync,
        Clock&& clock = Clock(),
        std::string command_path = default_command_path())
    {
        // The consumer thread must be started after control_ has been
        // constructed
        consumer_ =
            std::jthread(
                &consumer::run,
                consumer(
                    std::forward<OutputFunction>(out),
                    std::forward<ErrorFunction>(err),
                    std::forward<FlushFunction>(flush),
                    std::forward<SyncFunction>(sync),
                    &control_),
                make_clock(std::forward<Clock>(clock)));
        set_command_path(std::move(command_path));
    }

    ~logger()
    {
        control_.close();
    }

    auto consumer_thread_native_handle()
    {
        return consumer_.native_handle();
    }

    // XXX Rename as create? people might assume that multiple get
    // calls return the same producer.
    [[nodiscard]] producer get_producer(std::string name);

    void register_producer(producer& p) noexcept;

    void set_output_stream(FILE* stream) noexcept;
    void set_error_stream(FILE* stream) noexcept;

    template<typename Func>
    void set_output_function(Func&& f) noexcept
    {
        static_assert(
            std::is_convertible_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                ::ssize_t>,
            "Output function type must be of type ssize_t(const char*, size_t) "
            "(returning the number of bytes written or -1 on error)");
        post([f = std::forward<Func>(f)](consumer& c) { c.out = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_error_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<
                std::invoke_result_t<Func, const char*, std::size_t>,
                void>,
            "Error function must be of type void(const char*, size_t)");
        post([f = std::forward<Func>(f)](consumer& c) { c.err = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_flush_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Flush function must be of type void()");
        post([f = std::forward<Func>(f)](consumer& c) { c.flush = std::move(f); });
        control_.sync();
    }

    template<typename Func>
    void set_sync_function(Func&& f) noexcept
    {
        static_assert(
            std::is_same_v<std::invoke_result_t<Func>, void>,
            "Sync function must be of type void()");
        post([f = std::forward<Func>(f)](consumer& c) { c.sync = std::move(f); });
        control_.sync();
    }

    void set_command_path(std::string path) noexcept
    {
        post([p = std::move(path)](consumer& c) { c.set_command_path(std::move(p)); });
        control_.sync();
    }

private:
    static std::string default_command_path();

    template<typename Func>
    void post(Func&& f)
    {
        std::scoped_lock lock{control_mutex_};
        control_.post(std::forward<Func>(f));
    }

    template<typename Clock>
    std::function<std::timespec()> make_clock(Clock&& clock)
    {
        return
            [clock_{std::forward<Clock>(clock)}]() -> std::timespec
            {
                // Note: to_time_t would be useful here except it is unspecified
                // whether time_t rounds up or truncates if time_t has a lower
                // precision than the input time_point.
                using namespace std::chrono;
                const auto now = clock_.now();
                auto sec = time_point_cast<seconds>(now);
                if (sec > now)
                    sec - seconds{1};
                return std::timespec{
                    .tv_sec=sec.time_since_epoch().count(),
                    .tv_nsec=duration_cast<nanoseconds>(now - sec).count()};
            };
    }

    producer control_; // aligned to cache line so first to avoid extra padding
    std::jthread consumer_;
    std::mutex control_mutex_;
};

template<auto Format, typename Tags>
void xtr::logger::producer::log() noexcept
{
    // This function is just an optimisation; if the log line has no arguments
    // then creating a lambda for it would waste space in the queue (as even
    // if the lambda captures nothing it still has a non-zero size).
    const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
    if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    copy(s.begin(), &detail::trampoline0<Format, consumer>);
    buf_.reduce_writable(sizeof(fptr_t));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::log(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    static_assert(sizeof...(Args) > 0);
    constexpr bool is_str =
        std::disjunction_v<
            detail::is_c_string<decltype(std::forward<Args>(args))>...,
            std::is_same<std::remove_cvref_t<Args>, std::string_view>...,
            std::is_same<std::remove_cvref_t<Args>, std::string>...>;
    if constexpr (is_str)
        post_with_str_table<Format, Tags>(std::forward<Args>(args)...);
    else
        post<Format, Tags>(make_lambda<Tags>(std::forward<Args>(args)...));
}

template<auto Format, typename Tags, typename... Args>
void xtr::logger::producer::post_with_str_table(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    using lambda_t =
        decltype(
            make_lambda<Tags>(
                detail::build_string_table<Tags>(
                    std::declval<std::byte*&>(),
                    std::declval<std::byte*&>(),
                    buf_,
                    std::forward<Args>(args))...));

    ring_buffer::span s = buf_.write_span_spec();

    static_assert(alignof(std::size_t) <= alignof(fptr_t));
    const auto size_pos = s.begin() + sizeof(fptr_t);

    auto func_pos = size_pos + sizeof(size_t);
    if constexpr (alignof(lambda_t) > alignof(size_t))
        func_pos = align<alignof(lambda_t)>(func_pos);

    static_assert(alignof(char) == 1);
    const auto str_pos = func_pos + sizeof(lambda_t);
    const auto size = ring_buffer::size_type(str_pos - s.begin());

    while (XTR_UNLIKELY(s.size() < size)) [[unlikely]]
    {
        if constexpr (!detail::is_non_blocking_v<Tags>)
            detail::pause();
        s = buf_.write_span<Tags>();
        if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
            return;
    }

    // str_cur and str_end are mutated by build_string_table as the
    // table is built
    auto str_cur = str_pos;
    auto str_end = s.end();

    copy(s.begin(), &detail::trampolineS<Format, consumer, lambda_t>);
    copy(
        func_pos,
        make_lambda<Tags>(
            detail::build_string_table<Tags>(
                str_cur,
                str_end,
                buf_,
                std::forward<Args>(args))...));

    const auto next = detail::align<alignof(fptr_t)>(str_cur);
    const auto total_size = ring_buffer::size_type(next - s.begin());

    copy(size_pos, total_size);
    buf_.reduce_writable(total_size);
}

template<typename T>
void xtr::logger::producer::copy(std::byte* pos, T&& value) noexcept
{
    assert(std::uintptr_t(pos) % alignof(T) == 0);
    // C++20: std::assume_aligned
    pos =
        static_cast<std::byte*>(
            __builtin_assume_aligned(pos, alignof(T)));
    new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template<auto Format, typename Tags, typename Func>
void xtr::logger::producer::post(Func&& func)
    noexcept(std::is_nothrow_move_constructible_v<Func>)
{
    ring_buffer::span s = buf_.write_span_spec();

    // GCC as of 9.2.1 does not optimise away this call to align if pos
    // is marked as aligned, hence these constexpr conditionals. Clang
    // does optimise as of 8.0.1-3+b1.
    auto func_pos = s.begin() + sizeof(fptr_t);
    if constexpr (alignof(Func) > alignof(fptr_t))
        func_pos = detail::align<alignof(Func)>(func_pos);

    // We can calculate 'next' aligned to fptr_t in this way because we know
    // that func_pos has alignment that is at least alignof(fptr_t), so the
    // size of Func can simply be rounded up.
    const auto next = func_pos + detail::align(sizeof(Func), alignof(fptr_t));
    const auto size = ring_buffer::size_type(next - s.begin());

    while (XTR_UNLIKELY(s.size() < size)) [[unlikely]] // XXX UNLIKELY
    {
        if constexpr (!detail::is_non_blocking_v<Tags>)
            detail::pause();
        s = buf_.write_span<Tags>();
        if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
            return;
    }

    copy(s.begin(), &detail::trampolineN<Format, consumer, Func>);
    copy(func_pos, std::forward<Func>(func));

    buf_.reduce_writable(size);
}

template<typename Tags, typename... Args>
auto xtr::logger::producer::make_lambda(Args&&... args)
    noexcept(std::conjunction_v<
        std::is_nothrow_copy_constructible<Args>...,
        std::is_nothrow_move_constructible<Args>...>)
{
    // This lambda is mutable so that std::forward works correctly, without it
    // there is a mismatch between Args and args, due to args becoming const
    // if the lambda is not mutable.
    return
        [... args = std::forward<Args>(args)](
            fmt::memory_buffer& mbuf,
            const auto& out,
            const auto& err,
            std::string_view fmt,
            [[maybe_unused]] const char* ts,
            const std::string& name) mutable noexcept
        {
            // args are passed by reference because although they were
            // forwarded into the lambda, they were still captured by copy,
            // so there is no point in moving them out of the lambda.
            if constexpr (detail::is_timestamp_v<Tags>)
            {
                xtr::detail::print_ts(
                    mbuf,
                    out,
                    err,
                    fmt,
                    name,
                    args...);
            }
            else
            {
                xtr::detail::print(
                    mbuf,
                    out,
                    err,
                    fmt,
                    ts,
                    name,
                    args...);
            }
        };
}

#endif

#define XTR_FUNC inline


#include <cassert>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

XTR_FUNC
xtr::detail::file_descriptor::file_descriptor(
    const char* path,
    int flags,
    int mode)
:
    fd_(XTR_TEMP_FAILURE_RETRY(::open(path, flags, mode)))
{
    if (fd_ == -1)
    {
        throw_system_error_fmt(
            "xtr::detail::file_descriptor::file_descriptor: "
            "Failed to open `%s'", path);
    }
}

XTR_FUNC
xtr::detail::file_descriptor& xtr::detail::file_descriptor::operator=(
    xtr::detail::file_descriptor&& other) noexcept
{
    swap(*this, other);
    return *this;
}

XTR_FUNC
xtr::detail::file_descriptor::~file_descriptor()
{
    reset();
}

XTR_FUNC
void xtr::detail::file_descriptor::reset(int fd) noexcept
{
    if (is_open())
    {
#ifndef NDEBUG
        const int result =
#endif
            XTR_TEMP_FAILURE_RETRY(::close(fd_));
        assert(result == 0);
    }
    fd_ = fd;
}



#include <memory>

#include <fcntl.h>
#include <unistd.h>

/*
 XXX Needs to be shared with mirrored mapping
namespace xtr::detail
{
    struct fd_closer
    {
        ~fd_closer()
        {
            if (fd != -1)
                ::close(fd);
        }

        int fd = -1;
    };
}*/

XTR_FUNC
xtr::detail::interprocess_ring_buffer::interprocess_ring_buffer(
    const char* path,
    int flags,
    int mode)
{
    // XXX NEED RAII
    const int fd = XTR_TEMP_FAILURE_RETRY(::open(path, flags, mode));

    if (fd == -1)
    {
        throw_system_error_fmt(
            "xtr::detail::interprocess_ring_buffer: "
            "Failed to open `%s'", path);
    }

    const std::size_t sz = align_to_page_size(sizeof(ring_buffer));

    if (XTR_TEMP_FAILURE_RETRY(::ftruncate(fd, ::off_t(sz + capacity))) == -1)
    {
        throw_system_error(
            "xtr::detail::interprocess_ring_buffer: "
            "Failed to ftruncate backing file");
    }

    buf_ = memory_mapping(nullptr, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd);

    new (buf_.get()) ring_buffer(fd, sz);
}

XTR_FUNC
xtr::detail::interprocess_ring_buffer::~interprocess_ring_buffer()
{
    if (buf_)
        std::destroy_at(ring_buf());
}



#include <fmt/chrono.h>

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <climits>

#include <iostream> // XXX TESTING

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

XTR_FUNC
xtr::logger::producer xtr::logger::get_producer(std::string name)
{
    return producer{*this, std::move(name)};
}

XTR_FUNC
void xtr::logger::register_producer(producer& p) noexcept
{
    post(
        [&p](consumer& c)
        {
            c.add_producer(p);
        });
}

XTR_FUNC
void xtr::logger::set_output_stream(FILE* stream) noexcept
{
    set_output_function(detail::make_output_func(stream));
}

XTR_FUNC
void xtr::logger::set_error_stream(FILE* stream) noexcept
{
    set_error_function(detail::make_error_func(stream));
}

XTR_FUNC
void xtr::logger::consumer::run(std::function<::timespec()> clock) noexcept
{
    char ts[32] = {};
    bool ts_stale = true;
    std::size_t flush_count = 0;
    fmt::memory_buffer mbuf;

    for (std::size_t i = 0; !producers_.empty(); ++i)
    {
        ring_buffer::span span;
        // The inner loop below can modify producers so a reference cannot be taken
        const std::size_t n = i % producers_.size();

        if (n == 0)
        {
            // Read the clock and commands once per loop over producers
            ts_stale |= true;
            process_commands();
        }

        if ((span = producers_[n]->buf_.read_span()).empty())
        {
            // flush if no further data available (all producers empty)
            if (flush_count != 0 && flush_count-- == 1)
                flush();
            continue;
        }

        destroy = false;

        if (ts_stale)
        {
            fmt::format_to(ts, "{}", xtr::timespec{clock()});
            ts_stale = false;
        }

        // span.end is capped to the end of the first mapping to guarantee that
        // data is only read from the same address that it was written to (the
        // producer always begins log records in the first mapping, so we do not
        // read a record beginning in the second mapping). This is done to
        // avoid undefined behaviour---reading an object from a different
        // address than it was written to will work on Intel and probably many
        // other CPUs but is outside of what is permitted by the C++ memory
        // model.
        std::byte* pos = span.begin();
        std::byte* end = std::min(span.end(), producers_[n]->buf_.end());
        do
        {
            assert(std::uintptr_t(pos) % alignof(fptr_t) == 0);
            fptr_t fptr = *reinterpret_cast<const fptr_t*>(pos);
            pos = fptr(mbuf, pos, *this, ts, producers_[n]->name_);
            if (destroy)
            {
                using std::swap;
                swap(producers_[n], producers_.back()); // possible self-swap, ok
                producers_.pop_back();
                goto next;
            }
        } while (pos < end);

        producers_[n]->buf_.reduce_readable(
            ring_buffer::size_type(pos - span.begin()));

        std::size_t n_dropped;
        if (producers_[n]->buf_.read_span().empty() &&
            (n_dropped = producers_[n]->buf_.dropped_count()) > 0)
        {
            detail::print(
                mbuf,
                out,
                err,
                "{}: {}: {} messages dropped\n",
                ts,
                producers_[n]->name_,
                n_dropped);
        }

        // flushing may be late/early if producers is modified, doesn't matter
        flush_count = producers_.size();

        next:;
    }
}

namespace xtr::detail
{
/*
    std::generator<interprocess_ring_buffer::span> get_write_span(
        interprocess_ring_buffer& buf)
    {
        co_await std::suspend_always();
    }
*/


}

XTR_FUNC
void xtr::logger::consumer::process_commands() noexcept
{
/*
    const auto now = detail::tsc::now().ticks;

    if (now < process_cmd_next_)
        return;

    process_cmd_next_ = now + 123;

    // This is admittedly hacky, however

    fd_set fds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(cmd_fd_, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // first arg is the max fd

    ::select(cmd_fd_, &fds,  )
*/
/*
    // co_await, co_return, co_yield

    detail::interprocess_ring_buffer::span rs;
    detail::interprocess_ring_buffer::span ws;

    //while ((ws = cmd_buf_.write_span()).size() < 42)
    //    co_await std::suspend_always();


    while ((ws = cmd_buf_.write_span()).size() < 42)
        co_await std::suspend_always();


    // Need to define a structure,
    //
    // also need to write tests---you can just make a free function
    // or class that communicates with the logger.
    //
    // generate a random name then set the output function to test?
    //
    // or query the name?
    //
    //
    //
    // include/xtr/detail/command.hpp
    // include/xtr/detail/send_command.hpp
    //
    // void send_command
    //
    // So, command types, you want to be able to:
    //
    // * Modify producers based on a regular expression
    //   - ie set the log level on them
    // * Dry run of modify
    // * Wildcards?
    //
    // XXX HOW TO HANDLE THE COMMAND LINE TOOL BLOCKING WHEN SENDING REPLY?
    //
    // XXX fnmatch

    // int socket(int domain, int type, int protocol)
    // ::socket(AF_LOCAL, SOCK_SEQPACKET, 0);
    //
    // ::socket(AF_LOCAL, SOCK_STREAM, IPPROTO_SCTP)
    //
    // set non blocking
    // listen
    // add to epoll?
    // accept

    const auto span = cmd_buf_.read_span();
    if (span.size() > 0)
    {
        cmd_buf_.reduce_readable(span.size());
    }
*/
}

XTR_FUNC
std::string xtr::logger::default_command_path()
{
    static std::atomic<unsigned> ctl_count{0};

    const unsigned long pid = ::getpid();
    const unsigned long uid = ::geteuid();
    const unsigned n = ctl_count++;
    char path[PATH_MAX];
    std::snprintf(path, sizeof(path), "/run/user/%lu/xtrctl.%lu.%u", uid, pid, n);
    return path;
}

XTR_FUNC
void xtr::logger::consumer::set_command_path(std::string path) noexcept
{
    // XXX Should you just read from a fifo?
    // no, because you need connect/disconnect behaviour,
    // same issue as with a shm buffer, if a request is made
    // and the program disconnects, you need to know.

#if 0
#if __cpp_exceptions
    try
    {
#endif
        if (path.size() > sizeof(std::declval<sockaddr_un>().sun_path))
        {
        }

        cmd_fd_ = ::socket(AF_LOCAL, SOCK_SEQPACKET|SOCK_NONBLOCK, 0);

        sockaddr_un addr;
        //struct sockaddr* = &addrp;




        addr.sun_family = AF_LOCAL;
        // XXX strncpy, maybe use std::copy?
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path));

        //oldmask = umask(S_IXUSR | S_IRWXG | S_IRWXO);

        if (::bind(cmd_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            std::cout << "bind error\n";
        }

        if (::listen(cmd_fd_, 5) == -1)
        {
            std::cout << "listen error\n";
        }

        //int connfd;

        //::fcntl(cmd_fd_, F_SETFD, 

        // SET O_NONBLOCK, then select

        //if ((connfd = ::accept(cmd_fd_, NULL, NULL)) == -1)
        //{
        //    std::cout << "accept error\n";
        //}

        // XXX Need to do something about signals, e.g. SIGPIPE
        //
        // - can use send with MSG_NOSIGNAL

        // so, if you want to maintain portability you can't use epoll, but,
        // you will probably only have 2 fds at a time anyway

        //cmd_buf_ =
        //    detail::interprocess_ring_buffer(
        //        path.c_str(),
        //        O_CREAT|O_EXCL|O_RDWR,
        //        S_IRUSR|S_IWUSR);

        cmd_path_ = std::move(path);
#if __cpp_exceptions
    }
    catch (const std::exception& e)
    {
    }
#endif
#endif
}

XTR_FUNC
xtr::logger::consumer::~consumer()
{
    if (!cmd_path_.empty())
        ::unlink(cmd_path_.c_str());
}

XTR_FUNC
xtr::logger::producer::producer(logger& owner, std::string name)
:
    name_(std::move(name))
{
    owner.register_producer(*this);
}

XTR_FUNC
void xtr::logger::producer::close()
{
    if (!closed_)
    {
        sync(/*destruct=*/true);
        closed_ = true;
    }
}

XTR_FUNC
void xtr::logger::producer::sync(bool destruct)
{
    std::condition_variable cv;
    std::mutex m;
    bool notified = false;

    post(
        [&cv, &m, &notified, destruct](consumer& c)
        {
            c.destroy = destruct;

            c.flush();
            c.sync();

            std::scoped_lock lock{m};
            notified = true;
            // Do not move this notify outside of the protection of m. The
            // standard guarantees that a mutex may be destructed while
            // another thread is still inside unlock (but does not hold the
            // lock). From the mutex requirements:
            //
            // ``Note: After a thread A has called unlock(), releasing a
            // mutex, it is possible for another thread B to lock the same
            // mutex, observe that it is no longer in use, unlock it, and
            // destroy it, before thread A appears to have returned from
            // its unlock call. Implementations are required to handle such
            // scenarios correctly, as long as thread A doesn't access the
            // mutex after the unlock call returns.''
            //
            // No such requirement exists for condition_variable and notify,
            // which may access memory (e.g. an internal mutex in pthreads) in
            // the signalling thread after the waiting thread has woken up---so
            // if the lock is not held, the condition_variable could already
            // have been destructed at this time (due to the stack being
            // unwound).
            cv.notify_one();
            // Do not access any captured variables after notifying because if
            // the producer is destructing then the underlying storage may have
            // been freed already.
        });

    std::unique_lock lock{m};
    while (!notified)
        cv.wait(lock);
}

XTR_FUNC
void xtr::logger::producer::set_name(std::string name)
{
    post(
        [this, name = std::move(name)](auto&)
        {
            this->name_ = std::move(name);
        });
    sync();
}

XTR_FUNC
xtr::logger::producer::~producer()
{
    close();
}



#include <cassert>

#include <unistd.h>

XTR_FUNC
xtr::detail::memory_mapping::memory_mapping(
    void* addr,
    std::size_t length,
    int prot,
    int flags,
    int fd,
    std::size_t offset)
:
    mem_(::mmap(addr, length, prot, flags, fd, ::off_t(offset))),
    length_(length)
{
    if (mem_ == MAP_FAILED)
    {
        throw_system_error(
            "xtr::detail::memory_mapping::memory_mapping: mmap failed");
    }
}

XTR_FUNC
xtr::detail::memory_mapping::memory_mapping(memory_mapping&& other) noexcept
:
    mem_(other.mem_),
    length_(other.length_)
{
    other.release();
}

XTR_FUNC
xtr::detail::memory_mapping& xtr::detail::memory_mapping::operator=(
    memory_mapping&& other) noexcept
{
    swap(*this, other);
    return *this;
}

XTR_FUNC xtr::detail::memory_mapping::~memory_mapping()
{
    reset();
}

XTR_FUNC
void xtr::detail::memory_mapping::reset(void* addr, std::size_t length) noexcept
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
#include <cstdlib>
#include <random>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if !defined(__linux__)
namespace xtr::detail
{
    XTR_FUNC
    file_descriptor shm_open_anon(int oflag, mode_t mode)
    {
#if defined(SHM_ANON) // FreeBSD extension
        return XTR_TEMP_FAILURE_RETRY(::shm_open(SHM_ANON, oflag, mode));
#else
        int fd;

        // Some platforms don't allow slashes in shm object names except
        // for the first character, hence not using the usual base64 table
        const char ctable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789~-";
        char name[] = "/xtr.XXXXXXXXXXXXXXXX";

        std::random_device rd;
        std::uniform_int_distribution<> udist(0, sizeof(ctable) - 2);

        // As there is no way to create an anonymous shm object we generate
        // random names, retrying up to 64 times if the name already exists.
        std::size_t retries = 64;
        do
        {
            for (char* pos = name + 5; *pos != '\0'; ++pos)
                *pos = ctable[udist(rd)];
            fd = XTR_TEMP_FAILURE_RETRY(::shm_open(name, oflag|O_EXCL|O_CREAT, mode));
        }
        while (--retries > 0 && fd == -1 && errno == EEXIST);

        if (fd != -1)
            ::shm_unlink(name);

        return file_descriptor(fd);
#endif
    }
}
#endif

XTR_FUNC
xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping(
    std::size_t length,
    int fd,
    std::size_t offset,
    int flags)
{
    assert(!(flags & MAP_ANONYMOUS) || fd == -1);
    assert((flags & MAP_FIXED) == 0); // Not implemented (would be easy though)
    assert((flags & MAP_PRIVATE) == 0); // Can't be private, must be shared for mirroring to work

    // length is not automatically rounded up because it would make the class
    // error prone---mirroring would not take place where the user expects.
    if (length != align_to_page_size(length))
    {
        throw_invalid_argument(
            "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
            "Length argument is not page-aligned");
    }

    // To create two adjacent mappings without race conditions:
    // 1.) Create a mapping A of twice the requested size, L*2
    // 2.) Create a mapping of size L at A+L using MAP_FIXED
    // 3.) Create a mapping at of size L at A using MAP_FIXED, this will
    //     also destroy the mapping created in (1).

    const int prot = PROT_READ|PROT_WRITE;

    memory_mapping reserve(
        nullptr,
        length * 2,
        prot,
        MAP_PRIVATE|MAP_ANONYMOUS);

#if !defined(__linux__)
    file_descriptor temp_fd;
#endif

    if (fd == -1)
    {
#if defined(__linux__)
        // Note that it is possible to just do a single mmap of length * 2 for
        // m_ then an mremap of length with target address m_.get() + length
        // for mirror. I have not done this just to make the rest of the class
        // cleaner, as with the below method we don't have to deal with the
        // first mapping's length being length * 2.
        memory_mapping mirror(
            static_cast<std::byte*>(reserve.get()) + length,
            length,
            prot,
            MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS|flags);

        m_.reset(
            ::mremap(
                mirror.get(),
                0,
                length,
                MREMAP_FIXED|MREMAP_MAYMOVE,
                reserve.get()),
            length);

        if (!m_)
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
                "mremap failed");
        }

        reserve.release(); // mapping was destroyed by mremap
        mirror.release(); // mirror will be recreated in ~mirrored_memory_mapping
        return;
#else
        if (!(temp_fd = shm_open_anon(O_RDWR, S_IRUSR|S_IWUSR)))
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
                "Failed to shm_open backing file");
        }

        fd = temp_fd.get();

        if (::ftruncate(fd, ::off_t(length)) == -1)
        {
            throw_system_error(
                "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
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
    mirror.release(); // mirror will be recreated in ~mirrored_memory_mapping
}

XTR_FUNC
xtr::detail::mirrored_memory_mapping::~mirrored_memory_mapping()
{
    if (m_)
    {
        memory_mapping{}.reset(
            static_cast<std::byte*>(m_.get()) + m_.length(),
            m_.length());
    }
}



#include <unistd.h>

XTR_FUNC std::size_t xtr::detail::align_to_page_size(std::size_t length)
{
    static const long pagesize(::sysconf(_SC_PAGESIZE));
    if (pagesize == -1)
        throw_system_error("sysconf(_SC_PAGESIZE) failed");
    return align(length, std::size_t(pagesize));
}



#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <system_error>

XTR_FUNC void xtr::detail::throw_system_error(const char* what)
{
#if __cpp_exceptions
    throw std::system_error(std::error_code(errno, std::generic_category()), what);
#else
    std::fprintf(stderr, "system error: %s: %s\n", what, std::strerror(errno));
    std::abort();
#endif
}

XTR_FUNC
void xtr::detail::throw_system_error_fmt(const char* format, ...)
{
    const int errnum = errno; // in case vsnprintf modifies errno
    va_list args;
    va_start(args, format);;
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
#if __cpp_exceptions
    throw std::system_error(std::error_code(errnum, std::generic_category()), buf);
#else
    std::fprintf(stderr, "system error: %s: %s\n", buf, std::strerror(errnum));
    std::abort();
#endif
}

XTR_FUNC void xtr::detail::throw_invalid_argument(const char* what)
{
#if __cpp_exceptions
    throw std::invalid_argument(what);
#else
    std::fprintf(stderr, "invalid argument: %s\n", what);
    std::abort();
#endif
}



#include <array>
#include <chrono>
#include <thread>

#include <time.h>


XTR_FUNC
std::uint64_t xtr::detail::read_tsc_hz() noexcept
{
    constexpr int tsc_leaf = 0x15;

    // Check if CPU supports TSC info leaf
    if (cpuid(0)[0] < tsc_leaf)
        return 0;

    // ratio_den (EAX) : An unsigned integer which is the denominator of the
    //                   TSC / CCC ratio.
    // ratio_num (EBX) : An unsigned integer which is the numerator of the
    //                   TSC / CCC ratio.
    // ccc_hz (ECX)    : An unsigned integer which is the nominal frequency of
    //                   the CCC in Hz.
    //
    // Where CCC = core crystal clock.
    auto [ratio_den, ratio_num, ccc_hz, unused] = cpuid(0x15);

    if (ccc_hz == 0)
    {
        // Core crystal clock frequency not provided, must infer
        // from CPU model number.
        const std::uint16_t model = get_family_model()[1];
        switch(model) {
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
        // Skylake-X is not included as the crystal clock may be either 24 MHz
        // or 25 MHz, with further issues on the 25 MHz variant, for details
        // see: // https://bugzilla.kernel.org/show_bug.cgi?id=197299
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

    // As we have:
    //
    // TSC Hz = ccc_hz * TSC/CCR
    //
    // and:
    //
    // TSC/CCR = ratio_num/ratio_den
    //
    // then:
    //
    // TSC Hz = ccc_hz * ratio_num/ratio_den
    return std::uint64_t(ccc_hz) * ratio_num / ratio_den;
}

XTR_FUNC
std::uint64_t xtr::detail::estimate_tsc_hz() noexcept
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

    // Read the TSC and system clock every 10ms for up to 2 seconds or until
    // last 5 TSC frequency estimations are within a 1000 tick range, whichever
    // occurs first.

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

XTR_FUNC
std::timespec xtr::detail::tsc::to_timespec(tsc ts)
{
    thread_local tsc last_tsc{};
    thread_local std::int64_t last_epoch_nanos;
    static const std::uint64_t one_minute_ticks = 60 * get_tsc_hz();
    static const double tsc_multiplier = 1e9 / double(get_tsc_hz());

    // Sync up TSC and wall clocks every minute
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
    result.tv_sec = total_nanos / 1000000000UL;
    result.tv_nsec = total_nanos % 1000000000UL;

    return result;
}

