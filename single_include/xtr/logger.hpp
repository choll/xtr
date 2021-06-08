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

#ifndef XTR_LOGGER_HPP
#define XTR_LOGGER_HPP

#include <ctime>

#include <fmt/chrono.h>

namespace xtr {
struct timespec : std::timespec {
  timespec() = default;

  timespec(std::timespec ts) : std::timespec(ts) {}
};
} // namespace xtr

namespace fmt {
template <> struct formatter<xtr::timespec> {
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const xtr::timespec ts, FormatContext &ctx) {
    std::tm temp;
    return fmt::format_to(ctx.out(), "{:%Y-%m-%d %T}.{:06}",
                          *::gmtime_r(&ts.tv_sec, &temp), ts.tv_nsec / 1000);
  }
};
} // namespace fmt

namespace xtr {
struct non_blocking_tag;
struct timestamp_tag;
} // namespace xtr

namespace xtr::detail {
[[noreturn, gnu::cold]] void throw_runtime_error(const char *what);
[[noreturn, gnu::cold, gnu::format(printf, 1, 2)]] void
throw_runtime_error_fmt(const char *format, ...);

[[noreturn, gnu::cold]] void throw_system_error(const char *what);

[[noreturn, gnu::cold, gnu::format(printf, 1, 2)]] void
throw_system_error_fmt(const char *format, ...);

[[noreturn, gnu::cold]] void throw_invalid_argument(const char *what);
} // namespace xtr::detail

#include <cerrno>

#define XTR_TEMP_FAILURE_RETRY(expr)                                           \
  (__extension__({                                                             \
    decltype(expr) xtr_result;                                                 \
    do                                                                         \
      xtr_result = (expr);                                                     \
    while (xtr_result == -1 && errno == EINTR);                                \
    xtr_result;                                                                \
  }))

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail {
template <typename T> constexpr T align(T value, T alignment) noexcept {
  static_assert(std::is_unsigned_v<T>, "value must be unsigned");
  assert(std::has_single_bit(alignment));
  return (value + (alignment - 1)) & ~(alignment - 1);
}

template <std::size_t Align, typename T>
__attribute__((assume_aligned(Align))) T *align(T *ptr) noexcept {
  static_assert(std::is_same_v<std::remove_cv_t<T>, std::byte> ||
                    std::is_same_v<std::remove_cv_t<T>, char> ||
                    std::is_same_v<std::remove_cv_t<T>, unsigned char> ||
                    std::is_same_v<std::remove_cv_t<T>, signed char>,
                "value must be a char or byte pointer");
  return reinterpret_cast<T *>(align(std::uintptr_t(ptr), Align));
}
} // namespace xtr::detail

#include <cstddef>

namespace xtr::detail {
std::size_t align_to_page_size(std::size_t length);
}

#include <array>
#include <cstdint>

namespace xtr::detail {
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

inline std::array<std::uint32_t, 4> cpuid(int leaf, int subleaf = 0) noexcept {
  std::array<std::uint32_t, 4> out;
  asm("cpuid"
      : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
      : "a"(leaf), "c"(subleaf));
  return out;
}

inline std::array<std::uint16_t, 2> get_family_model() noexcept {
  const std::uint32_t fms = cpuid(0x1)[0];
  std::uint16_t model = (fms & 0xF0) >> 4;
  std::uint16_t family = (fms & 0xF00) >> 8;
  if (family == 0xF)
    family |= std::uint16_t((fms & 0xFF00000) >> 16);
  if (family == 0x6 || family == 0xF)
    model |= std::uint16_t((fms & 0xF0000) >> 12);
  return {family, model};
}
} // namespace xtr::detail

#include <type_traits>

namespace xtr::detail {
template <typename T>
using is_c_string = std::conjunction<
    std::is_pointer<std::decay_t<T>>,
    std::is_same<char,
                 std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>>>;

#if defined(XTR_ENABLE_TEST_STATIC_ASSERTIONS)
static_assert(is_c_string<char *>::value);
static_assert(is_c_string<char *&>::value);
static_assert(is_c_string<char *&&>::value);
static_assert(is_c_string<char *const>::value);
static_assert(is_c_string<const char *>::value);
static_assert(is_c_string<const char *const>::value);
static_assert(is_c_string<volatile char *>::value);
static_assert(is_c_string<volatile char *volatile>::value);
static_assert(is_c_string<const volatile char *volatile>::value);
static_assert(is_c_string<char[2]>::value);
static_assert(is_c_string<const char[2]>::value);
static_assert(is_c_string<volatile char[2]>::value);
static_assert(is_c_string<const volatile char[2]>::value);
static_assert(!is_c_string<int>::value);
static_assert(!is_c_string<int *>::value);
static_assert(!is_c_string<int[2]>::value);
#endif
} // namespace xtr::detail

#include <utility>

namespace xtr::detail {
class file_descriptor;
}

class xtr::detail::file_descriptor {
public:
  file_descriptor() = default;

  explicit file_descriptor(int fd) : fd_(fd) {}

  file_descriptor(const char *path, int flags, int mode = 0);

  file_descriptor(const file_descriptor &) = delete;

  file_descriptor(file_descriptor &&other) noexcept : fd_(other.fd_) {
    other.release();
  }

  file_descriptor &operator=(const file_descriptor &) = delete;

  file_descriptor &operator=(file_descriptor &&other) noexcept;

  ~file_descriptor();

  bool is_open() const noexcept { return fd_ != -1; }

  explicit operator bool() const noexcept { return is_open(); }

  void reset(int fd = -1) noexcept;

  int get() const noexcept { return fd_; }

  int release() noexcept { return std::exchange(fd_, -1); }

private:
  int fd_ = -1;

  friend void swap(file_descriptor &a, file_descriptor &b) noexcept {
    using std::swap;
    swap(a.fd_, b.fd_);
  }
};

#include <cstddef>
#include <utility>

#include <sys/mman.h>

namespace xtr::detail {
class memory_mapping;
}

class xtr::detail::memory_mapping {
public:
  memory_mapping() = default;

  memory_mapping(void *addr, std::size_t length, int prot, int flags,
                 int fd = -1, std::size_t offset = 0);

  memory_mapping(const memory_mapping &) = delete;

  memory_mapping(memory_mapping &&other) noexcept;

  memory_mapping &operator=(const memory_mapping &) = delete;

  memory_mapping &operator=(memory_mapping &&other) noexcept;

  ~memory_mapping();

  void reset(void *addr = MAP_FAILED, std::size_t length = 0) noexcept;

  void release() noexcept { mem_ = MAP_FAILED; }

  void *get() { return mem_; }

  const void *get() const { return mem_; }

  std::size_t length() const { return length_; }

  explicit operator bool() const { return mem_ != MAP_FAILED; }

private:
  void *mem_ = MAP_FAILED;
  std::size_t length_{};

  friend void swap(memory_mapping &a, memory_mapping &b) noexcept {
    using std::swap;
    swap(a.mem_, b.mem_);
    swap(a.length_, b.length_);
  }
};

#include <cstddef>
#include <utility>

namespace xtr::detail {
class mirrored_memory_mapping;
}

class xtr::detail::mirrored_memory_mapping {
public:
  mirrored_memory_mapping() = default;
  mirrored_memory_mapping(mirrored_memory_mapping &&) = default;
  mirrored_memory_mapping &operator=(mirrored_memory_mapping &&) = default;

  explicit mirrored_memory_mapping(
      std::size_t length, // must be multiple of page size
      int fd = -1,
      std::size_t offset = 0, // must be multiple of page size
      int flags = 0);

  ~mirrored_memory_mapping();

  void *get() { return m_.get(); }

  const void *get() const { return m_.get(); }

  std::size_t length() const { return m_.length(); }

  explicit operator bool() const { return !!m_; }

private:
  memory_mapping m_;
};

#if defined(__x86_64__)
#include <emmintrin.h>
#endif

namespace xtr::detail {
__attribute__((always_inline)) inline void pause() noexcept {
#if defined(__x86_64__)
  _mm_pause();
#endif
}
} // namespace xtr::detail

namespace xtr::detail {
template <typename OutputIterator>
void sanitize_char_to(OutputIterator &pos, char c) {
  if (c >= ' ' && c <= '~' && c != '\\') [[likely]] {
    *pos++ = c;
  } else {
    constexpr const char hex[] = "0123456789ABCDEF";
    *pos++ = '\\';
    *pos++ = 'x';
    *pos++ = hex[(c >> 4) & 0xF];
    *pos++ = hex[c & 0xF];
  }
}
} // namespace xtr::detail

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace xtr::detail {
template <typename T> struct string_ref;

template <> struct string_ref<const char *> {
  explicit string_ref(const char *s) : str(s) {}

  explicit string_ref(const std::string &s) : str(s.c_str()) {}

  const char *str;
};

string_ref(const char *)->string_ref<const char *>;
string_ref(const std::string &)->string_ref<const char *>;
string_ref(const std::string_view &)->string_ref<std::string_view>;

template <> struct string_ref<std::string_view> {
  explicit string_ref(std::string_view s) : str(s) {}

  std::string_view str;
};
} // namespace xtr::detail

namespace fmt {
template <> struct formatter<xtr::detail::string_ref<const char *>> {
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(xtr::detail::string_ref<const char *> ref, FormatContext &ctx) {
    auto pos = ctx.out();
    while (*ref.str != '\0')
      xtr::detail::sanitize_char_to(pos, *ref.str++);
    return pos;
  }
};

template <> struct formatter<xtr::detail::string_ref<std::string_view>> {
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const xtr::detail::string_ref<std::string_view> ref,
              FormatContext &ctx) {
    auto pos = ctx.out();
    for (const char c : ref.str)
      xtr::detail::sanitize_char_to(pos, c);
    return pos;
  }
};
} // namespace fmt

#include <type_traits>

namespace xtr::detail {
template <typename Tag, typename F> struct detect_tag;

template <typename Tag, typename... Tags>
struct detect_tag<Tag, void(Tags...)>
    : std::disjunction<std::is_same<Tag, Tags>...> {};

template <typename Tag, typename Tags> struct add_tag;

template <typename Tag, typename... Tags> struct add_tag<Tag, void(Tags...)> {
  using type = void(Tag, Tags...);
};

template <typename Tag, typename... Tags>
using add_tag_t = typename add_tag<Tag, Tags...>::type;

struct speculative_tag;

template <typename Tags>
inline constexpr bool is_non_blocking_v =
    detect_tag<non_blocking_tag, Tags>::value;

template <typename Tags>
inline constexpr bool is_speculative_v =
    detect_tag<speculative_tag, Tags>::value;

template <typename Tags>
inline constexpr bool is_timestamp_v = detect_tag<timestamp_tag, Tags>::value;
} // namespace xtr::detail

#include <atomic>
#include <bit>
#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <version>

namespace xtr::detail {
inline constexpr std::size_t dynamic_capacity = std::size_t(-1);

#if defined(MAP_POPULATE)
inline constexpr int srb_flags = MAP_POPULATE;
#else
inline constexpr int srb_flags = 0;
#endif

template <std::size_t Capacity> class synchronized_ring_buffer;

template <std::size_t N> struct least_uint {
  typedef std::conditional_t<
      N <= std::size_t{std::numeric_limits<std::uint8_t>::max()}, std::uint8_t,
      std::conditional_t<
          N <= std::size_t{std::numeric_limits<std::uint16_t>::max()},
          std::uint16_t,
          std::conditional_t<
              N <= std::size_t{std::numeric_limits<std::uint32_t>::max()},
              std::uint32_t, std::uint64_t>>>
      type;
};

template <std::size_t N> using least_uint_t = typename least_uint<N>::type;

#if defined(__has_cpp_attribute) &&                                            \
    __has_cpp_attribute(__cpp_lib_hardware_interference_size)
inline constexpr std::size_t cacheline_size =
    std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t cacheline_size = 64;
#endif

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

template <typename T, typename SizeType> class span {
public:
  using size_type = SizeType;
  using value_type = T;
  using iterator = value_type *;

  span() = default;

  span(const span<std::remove_const_t<T>, SizeType> &other) noexcept
      : begin_(other.begin()), size_(other.size()) {}

  span &
  operator=(const span<std::remove_const_t<T>, SizeType> &other) noexcept {
    begin_ = other.begin();
    size_ = other.size();
    return *this;
  }

  span(iterator begin, iterator end)
      : begin_(begin), size_(size_type(end - begin)) {
    assert(begin <= end);
    assert(size_type(end - begin) <= std::numeric_limits<size_type>::max());
  }

  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] constexpr iterator begin() const noexcept { return begin_; }

  [[nodiscard]] constexpr iterator end() const noexcept {
    return begin_ + size_;
  }

private:
  iterator begin_ = nullptr;
  size_type size_ = 0;
};
} // namespace xtr::detail

template <std::size_t Capacity = xtr::detail::dynamic_capacity>
class xtr::detail::synchronized_ring_buffer {
public:
  using iterator = std::byte *;
  using const_iterator = const std::byte *;
  using size_type = std::size_t;
  using span = detail::span<std::byte, size_type>;
  using const_span = detail::span<const std::byte, size_type>;

  static constexpr bool is_dynamic = Capacity == dynamic_capacity;

public:
  synchronized_ring_buffer(int fd = -1, std::size_t offset = 0,
                           int flags = srb_flags) requires(!is_dynamic) {
    m_ = mirrored_memory_mapping{capacity(), fd, offset, flags};
    nread_plus_capacity_ = wrnread_plus_capacity_ = capacity();
    wrbase_ = begin();
  }

  explicit synchronized_ring_buffer(size_type min_capacity, int fd = -1,
                                    std::size_t offset = 0,
                                    int flags = srb_flags) requires is_dynamic
      : m_(align_to_page_size(
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
               std::bit_ceil(min_capacity)),
#else
               std::ceil2(min_capacity)),
#endif
           fd, offset, flags) {
    assert(capacity() <= std::numeric_limits<size_type>::max());
    wrbase_ = begin();
    wrcapacity_ = capacity();
    nread_plus_capacity_ = wrnread_plus_capacity_ = capacity();
  }

  void clear() noexcept {
    nwritten_ = 0;
    wrnread_plus_capacity_ = capacity();
    wrnwritten_ = 0;
    nread_plus_capacity_ = capacity();
    dropped_count_ = 0;
  }

  constexpr size_type capacity() const noexcept {
    if constexpr (is_dynamic)
      return m_.length();
    else
      return Capacity;
  }

  template <typename Tags = void()>
  span write_span(size_type minsize = 0) noexcept {
    assert(minsize <= capacity());

    if constexpr (!is_speculative_v<Tags>) {
      wrnread_plus_capacity_ =
          nread_plus_capacity_.load(std::memory_order_acquire);
    }

    size_type sz = wrnread_plus_capacity_ - wrnwritten_;
    const auto b = wrbase_ + clamp(wrnwritten_, wrcapacity());

    while (sz < minsize)
      [[unlikely]] {
        if constexpr (!is_non_blocking_v<Tags>)
          pause();

        wrnread_plus_capacity_ =
            nread_plus_capacity_.load(std::memory_order_acquire);
        sz = wrnread_plus_capacity_ - wrnwritten_;

        if constexpr (is_non_blocking_v<Tags>)
          break;
      }

    if (is_non_blocking_v<Tags> && sz < minsize) [[unlikely]] {
      dropped_count_.fetch_add(1, std::memory_order_relaxed);
      return span{};
    }

    assert(b >= begin());
    assert(b < end());
    return {b, b + sz};
  }

  template <typename Tags = void()>
  span write_span_spec(size_type minsize = 0) noexcept {
    return write_span<add_tag_t<speculative_tag, Tags>>(minsize);
  }

  void reduce_writable(size_type nbytes) noexcept {
    assert(nbytes <= nread_plus_capacity_.load() - nwritten_.load());
    nwritten_.fetch_add(nbytes, std::memory_order_release);
    wrnwritten_ += nbytes;
  }

  const_span read_span() const noexcept {
    return const_cast<synchronized_ring_buffer<Capacity> &>(*this).read_span();
  }

  span read_span() noexcept {
    const size_type nr =
        nread_plus_capacity_.load(std::memory_order_relaxed) - capacity();
    const auto b = begin() + clamp(nr, capacity());
    const size_type sz = nwritten_.load(std::memory_order_acquire) - nr;
    assert(b >= begin());
    assert(b < end());
    return {b, b + sz};
  }

  void reduce_readable(size_type nbytes) noexcept {
    nread_plus_capacity_.fetch_add(nbytes, std::memory_order_release);
#if !defined(XTR_THREAD_SANITIZER_ENABLED)
    assert(nread_plus_capacity_.load() - nwritten_.load() <= capacity());
#endif
  }

  iterator begin() noexcept { return static_cast<iterator>(m_.get()); }

  iterator end() noexcept { return begin() + capacity(); }

  const_iterator begin() const noexcept {
    return static_cast<const_iterator>(m_.get());
  }

  const_iterator end() const noexcept { return begin() + capacity(); }

  std::size_t dropped_count() noexcept {
    return dropped_count_.exchange(0, std::memory_order_relaxed);
  }

private:
  static_assert(is_dynamic ||
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
                std::has_single_bit(Capacity)
#else
                std::ispow2(Capacity)
#endif
  );
  static_assert(is_dynamic || Capacity > 0);
  static_assert(is_dynamic ||
                Capacity <= std::numeric_limits<size_type>::max());

  size_type wrcapacity() const noexcept {
    if constexpr (is_dynamic)
      return wrcapacity_;
    else
      return Capacity;
  }

  size_type clamp(size_type n, size_type capacity) {
    assert(capacity > 0);
    using clamp_type =
        std::conditional_t<is_dynamic, size_type, least_uint_t<Capacity - 1>>;
    return clamp_type(n) & clamp_type(capacity - 1);
  }

  struct empty {};
  using capacity_type = std::conditional_t<is_dynamic, size_type, empty>;

  alignas(cacheline_size) std::atomic<size_type> nwritten_{};
  alignas(cacheline_size) std::byte *wrbase_{};
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

namespace xtr::detail {
struct tsc;

std::uint64_t read_tsc_hz() noexcept;
std::uint64_t estimate_tsc_hz() noexcept;

inline std::uint64_t get_tsc_hz() noexcept {
  __extension__ static std::uint64_t tsc_hz =
      read_tsc_hz() ?: estimate_tsc_hz();
  return tsc_hz;
}
} // namespace xtr::detail

struct xtr::detail::tsc {
  inline static tsc now() noexcept {
    std::uint32_t a, d;
    asm volatile("rdtsc;" : "=a"(a), "=d"(d)); // output, a=eax, d=edx
    return {static_cast<std::uint64_t>(a) | (static_cast<uint64_t>(d) << 32)};
  }

  static std::timespec to_timespec(tsc ts);

  std::uint64_t ticks;
};

namespace fmt {
template <> struct formatter<xtr::detail::tsc> {
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const xtr::detail::tsc ticks, FormatContext &ctx) {
    const auto ts = xtr::detail::tsc::to_timespec(ticks);
    return formatter<xtr::timespec>().format(ts, ctx);
  }
};
} // namespace fmt

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

namespace xtr::detail {
template <clockid_t ClockId> xtr::timespec get_time() noexcept {
  std::timespec result;
  ::clock_gettime(ClockId, &result);
  return result;
}
} // namespace xtr::detail

#include <fmt/format.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace xtr::detail {
template <typename ErrorFunction, typename Timestamp>
[[gnu::cold, gnu::noinline]] void
report_error(fmt::memory_buffer &mbuf, const ErrorFunction &err, Timestamp ts,
             const std::string &name, const char *reason) {
  using namespace std::literals::string_view_literals;
  mbuf.clear();
  fmt::format_to(mbuf, "E {} {}: Error: {}\n"sv, ts, name, reason);
  err(mbuf.data(), mbuf.size());
}

template <typename OutputFunction, typename ErrorFunction, typename Timestamp,
          typename... Args>
void print(fmt::memory_buffer &mbuf, const OutputFunction &out,
           [[maybe_unused]] const ErrorFunction &err, std::string_view fmt,
           Timestamp ts, const std::string &name, const Args &...args) {
#if __cpp_exceptions
  try {
#endif
    mbuf.clear();
    fmt::format_to(mbuf, fmt, ts, name, args...);
    const auto result = out(mbuf.data(), mbuf.size());
    if (result == -1)
      return report_error(mbuf, err, ts, name, "Write error");
    if (std::size_t(result) != mbuf.size())
      return report_error(mbuf, err, ts, name, "Short write");
#if __cpp_exceptions
  } catch (const std::exception &e) {
    report_error(mbuf, err, ts, name, e.what());
  }
#endif
}

template <typename OutputFunction, typename ErrorFunction, typename Timestamp,
          typename... Args>
void print_ts(fmt::memory_buffer &mbuf, const OutputFunction &out,
              const ErrorFunction &err, std::string_view fmt,
              const std::string &name, Timestamp ts, const Args &...args) {
  print(mbuf, out, err, fmt, ts, name, args...);
}
} // namespace xtr::detail

#include <cstddef>
#include <string_view>

namespace xtr::detail {
template <std::size_t N> struct string {
  constexpr operator std::string_view() const noexcept {
    return std::string_view{str, N};
  }

  char str[N + 1];
};

template <std::size_t N> string(const char (&str)[N]) -> string<N - 1>;

template <std::size_t N1, std::size_t N2>
constexpr auto operator+(const string<N1> &s1, const string<N2> &s2) noexcept {
  string<N1 + N2> result{};

  for (std::size_t i = 0; i < N1; ++i)
    result.str[i] = s1.str[i];

  for (std::size_t i = 0; i < N2; ++i)
    result.str[i + N1] = s2.str[i];

  result.str[N1 + N2] = '\0';

  return result;
}

template <std::size_t Pos, std::size_t N>
constexpr auto rcut(const char (&s)[N]) noexcept {
  constexpr std::size_t count = N - Pos - 1;

  string<count> result{};

  for (std::size_t i = 0; i < count; ++i)
    result.str[i] = s[Pos + i];

  result.str[count] = '\0';

  return result;
}

template <std::size_t N>
constexpr auto rindex(const char (&s)[N], char c) noexcept {
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
} // namespace xtr::detail

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace xtr::detail {
template <typename Tags, typename T, typename Buffer>
requires(
    std::is_rvalue_reference_v<decltype(std::forward<T>(std::declval<T>()))>
        &&std::same_as<std::remove_cvref_t<T>, std::string>) ||
    (!is_c_string<T>::value &&
     !std::same_as<std::remove_cvref_t<T>, std::string> &&
     !std::same_as<std::remove_cvref_t<T>, std::string_view>)
        T &&build_string_table(std::byte *&, std::byte *&, Buffer &,
                               T &&value) {
  return std::forward<T>(value);
}

template <typename Tags, typename Buffer, typename String>
requires std::same_as<String, std::string> ||
    std::same_as<String, std::string_view>
        string_ref<const char *>
        build_string_table(std::byte *&pos, std::byte *&end, Buffer &buf,
                           const String &sv) {
  std::byte *str_end = pos + sv.length();
  while (end < str_end + 1)
    [[unlikely]] {
      detail::pause();
      const auto s = buf.write_span();
      if (s.end() < str_end + 1) [[unlikely]] {
        if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
          return string_ref("<truncated>");
      }
      end = s.end();
    }
  const char *result = reinterpret_cast<char *>(pos);
  const char *str = sv.data();
  while (pos != str_end)
    new (pos++) char(*str++);
  new (pos++) char('\0');
  return string_ref(result);
}

template <typename Tags, typename Buffer>
string_ref<const char *> build_string_table(std::byte *&pos, std::byte *&end,
                                            Buffer &buf, const char *str) {
  const char *result = reinterpret_cast<char *>(pos);
  do {
    while (pos == end)
      [[unlikely]] {
        detail::pause();
        const auto s = buf.write_span();
        if (s.end() == end) [[unlikely]] {
          if (s.size() == buf.capacity() || is_non_blocking_v<Tags>)
            return string_ref("<truncated>");
        }
        end = s.end();
      }
    new (pos++) char(*str);
  } while (*str++ != '\0');
  return string_ref(result);
}
} // namespace xtr::detail

#include <fmt/format.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace xtr::detail {

template <auto Format, typename State>
std::byte *trampoline0(fmt::memory_buffer &mbuf, std::byte *buf, State &state,
                       const char *ts, std::string &name) noexcept {
  print(mbuf, state.out, state.err, *Format, ts, name);
  return buf + sizeof(void (*)());
}

template <auto Format, typename State, typename Func>
std::byte *trampolineN(fmt::memory_buffer &mbuf, std::byte *buf, State &state,
                       [[maybe_unused]] const char *ts,
                       std::string &name) noexcept {
  typedef void (*fptr_t)();

  auto func_pos = buf + sizeof(fptr_t);
  if constexpr (alignof(Func) > alignof(fptr_t))
    func_pos = align<alignof(Func)>(func_pos);

  assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

  auto &func = *reinterpret_cast<Func *>(func_pos);
  if constexpr (std::is_same_v<decltype(Format), std::nullptr_t>)
    func(state, name);
  else
    func(mbuf, state.out, state.err, *Format, ts, name);

  static_assert(noexcept(func.~Func()));
  std::destroy_at(std::addressof(func));

  return func_pos + align(sizeof(Func), alignof(fptr_t));
}

template <auto Format, typename State, typename Func>
std::byte *trampolineS(fmt::memory_buffer &mbuf, std::byte *buf, State &state,
                       const char *ts, std::string &name) noexcept {
  typedef void (*fptr_t)();

  auto size_pos = buf + sizeof(fptr_t);
  assert(std::uintptr_t(size_pos) % alignof(std::size_t) == 0);

  auto func_pos = size_pos + sizeof(std::size_t);
  if constexpr (alignof(Func) > alignof(std::size_t))
    func_pos = align<alignof(Func)>(func_pos);
  assert(std::uintptr_t(func_pos) % alignof(Func) == 0);

  auto &func = *reinterpret_cast<Func *>(func_pos);
  func(mbuf, state.out, state.err, *Format, ts, name);

  static_assert(noexcept(func.~Func()));
  std::destroy_at(std::addressof(func));

  return buf + *reinterpret_cast<const std::size_t *>(size_pos);
}
} // namespace xtr::detail

#include <algorithm>
#include <cstring>
#include <iterator>

namespace xtr::detail {
template <std::size_t DstSz, typename Src>
void strzcpy(char (&dst)[DstSz], const Src &src) {
  const std::size_t n = std::min(DstSz - 1, std::size(src));
  std::memcpy(dst, &src[0], n);
  dst[n] = '\0';
}
} // namespace xtr::detail

namespace xtr {
enum class log_level_t { none, fatal, error, warning, info, debug };
}

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xtr::detail {
using frame_id_t = std::uint32_t;

struct frame_header {
  frame_id_t frame_id;
};

inline constexpr std::size_t max_frame_alignment = alignof(std::max_align_t);
inline constexpr std::size_t max_frame_size = 512;

union alignas(max_frame_alignment) frame_buf {
  frame_header hdr;
  char buf[max_frame_size];
};

template <typename Payload> struct frame : frame_header {
  static_assert(std::is_standard_layout_v<Payload>);
  static_assert(std::is_trivially_destructible_v<Payload>);
  static_assert(std::is_trivially_copyable_v<Payload>);

  using payload_type = Payload;

  frame() {
    static_assert(alignof(frame<Payload>) <= max_frame_alignment);
    static_assert(sizeof(frame<Payload>) <= max_frame_size);

    frame_id = Payload::frame_id;
  }

  Payload *operator->() { return &payload; }

  [[no_unique_address]] Payload payload{};
};
} // namespace xtr::detail

namespace xtr::detail {
enum class pattern_type_t { none, extended_regex, basic_regex, wildcard };

struct pattern {
  pattern_type_t type;
  bool ignore_case;
  char text[256];
};
} // namespace xtr::detail

namespace xtr::detail {
enum class message_id { status, set_level, sink_info, success, error, reopen };
}

namespace xtr::detail {
struct status {
  static constexpr auto frame_id = frame_id_t(message_id::status);

  struct pattern pattern;
};

struct set_level {
  static constexpr auto frame_id = frame_id_t(message_id::set_level);

  log_level_t level;
  struct pattern pattern;
};

struct reopen {
  static constexpr auto frame_id = frame_id_t(message_id::reopen);
};
} // namespace xtr::detail

#include <ostream>

namespace xtr::detail {
struct sink_info {
  static constexpr auto frame_id = frame_id_t(message_id::sink_info);

  log_level_t level;
  std::size_t buf_capacity;
  std::size_t buf_nbytes;
  std::size_t dropped_count;
  char name[128];
};

struct success {
  static constexpr auto frame_id = frame_id_t(message_id::success);
};

struct error {
  static constexpr auto frame_id = frame_id_t(message_id::error);

  char reason[256];
};
} // namespace xtr::detail

#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>

namespace xtr::detail {
[[nodiscard]] file_descriptor command_connect(std::string_view path);
}

inline xtr::detail::file_descriptor
xtr::detail::command_connect(std::string_view path) {
  file_descriptor fd(::socket(AF_LOCAL, SOCK_SEQPACKET, 0));

  if (!fd)
    return {};

  sockaddr_un addr;
  addr.sun_family = AF_LOCAL;

  if (path.size() >= sizeof(addr.sun_path)) {
    errno = ENAMETOOLONG;
    return {};
  }

  strzcpy(addr.sun_path, path);

#if defined(__linux__)
  if (addr.sun_path[0] == '\0') // abstract socket
  {
    std::memset(addr.sun_path + path.size(), '\0',
                sizeof(addr.sun_path) - path.size());
  }
#endif

  if (::connect(fd.get(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) !=
      0)
    return {};

  return fd;
}

#include <sys/socket.h>
#include <sys/types.h>

namespace xtr::detail {
[[nodiscard]] ::ssize_t command_send(int fd, const void *buf,
                                     std::size_t nbytes);
}

inline ::ssize_t xtr::detail::command_send(int fd, const void *buf,
                                           std::size_t nbytes) {
  ::msghdr hdr{};
  ::iovec iov;

  hdr.msg_iov = &iov;
  hdr.msg_iovlen = 1;

  iov.iov_base = const_cast<void *>(buf);
  iov.iov_len = nbytes;

  return XTR_TEMP_FAILURE_RETRY(::sendmsg(fd, &hdr, MSG_NOSIGNAL));
}

#include <sys/socket.h>
#include <sys/types.h>

namespace xtr::detail {
[[nodiscard]] ::ssize_t command_recv(int fd, frame_buf &buf);
}

inline ::ssize_t xtr::detail::command_recv(int fd, frame_buf &buf) {
  ::msghdr hdr{};
  ::iovec iov;

  hdr.msg_iov = &iov;
  hdr.msg_iovlen = 1;

  iov.iov_base = &buf;
  iov.iov_len = sizeof(buf);

  return XTR_TEMP_FAILURE_RETRY(::recvmsg(fd, &hdr, 0));
}

namespace xtr::detail {
class command_dispatcher;

struct command_dispatcher_deleter {
  void operator()(command_dispatcher *);
};
} // namespace xtr::detail

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <poll.h>

namespace xtr::detail {
class command_dispatcher;
}

class xtr::detail::command_dispatcher {
private:
  struct pollfd {
    detail::file_descriptor fd;
    short events;
    short revents;
  };

  static_assert(sizeof(pollfd) == sizeof(::pollfd));
  static_assert(alignof(pollfd) == alignof(::pollfd));

  struct buffer {
    buffer(const void *srcbuf, std::size_t srcsize);

    std::unique_ptr<char[]> buf;
    std::size_t size;
  };

  struct callback_result {
    std::vector<buffer> bufs;
    std::size_t pos = 0;
  };

  struct callback {
    std::function<void(int, void *)> func;
    std::size_t size;
  };

public:
  explicit command_dispatcher(std::string path);

  ~command_dispatcher();

  template <typename Payload, typename Callback>
  void register_callback(Callback &&c) {
    using frame_type = detail::frame<Payload>;

    static_assert(sizeof(frame_type) <= max_frame_size);
    static_assert(alignof(frame_type) <= max_frame_alignment);

    callbacks_[Payload::frame_id] =
        callback{[c = std::forward<Callback>(c)](int fd, void *buf) {
                   c(fd, static_cast<frame_type *>(buf)->payload);
                 },
                 sizeof(frame_type)};
  }

  void send(int fd, const void *buf, std::size_t nbytes);

  template <typename FrameType> void send(int fd, const FrameType &frame) {
    send(fd, &frame, sizeof(frame));
  }

  void send_error(int fd, std::string_view reason);

  void process_commands(int timeout) noexcept;

  bool is_open() const noexcept { return !pollfds_.empty(); }

private:
  void process_socket_read(pollfd &pfd) noexcept;
  void process_socket_write(pollfd &pfd) noexcept;
  void disconnect(pollfd &pfd) noexcept;

  std::unordered_map<frame_id_t, callback> callbacks_;
  std::vector<pollfd> pollfds_;
  std::unordered_map<int, callback_result> results_;
  std::string path_;
};

#include <cstddef>
#include <memory>

namespace xtr::detail {
class matcher;

std::unique_ptr<matcher> make_matcher(pattern_type_t pattern_type,
                                      const char *pattern, bool ignore_case);
} // namespace xtr::detail

class xtr::detail::matcher {
public:
  virtual bool operator()(const char *) const { return true; }

  virtual bool valid() const { return true; }

  virtual void error_reason(char *, std::size_t) const // LCOV_EXCL_LINE
  {}

  virtual ~matcher(){};
};

#include <regex.h>

namespace xtr::detail {
class regex_matcher;
}

class xtr::detail::regex_matcher : public matcher {
public:
  regex_matcher(const char *pattern, bool ignore_case, bool extended);

  ~regex_matcher();

  regex_matcher(const regex_matcher &) = delete;
  regex_matcher &operator=(const regex_matcher &) = delete;

  bool valid() const override;

  void error_reason(char *buf, std::size_t bufsz) const override;

  bool operator()(const char *str) const override;

private:
  ::regex_t regex_;
  int errnum_;
};

namespace xtr::detail {
class wildcard_matcher;
}

class xtr::detail::wildcard_matcher : public matcher {
public:
  wildcard_matcher(const char *pattern, bool ignore_case);

  bool operator()(const char *str) const override;

private:
  const char *pattern_ = nullptr;
  int flags_;
};

#include <string>

namespace xtr {
inline constexpr auto null_command_path = "";

std::string default_command_path();
} // namespace xtr

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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

#define XTR_LIKELY(x) __builtin_expect(!!(x), 1)
#define XTR_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define XTR_LOG(...)                                                           \
  (__extension__({ XTR_LOG_TAGS(void(), "I", __VA_ARGS__); }))

#define XTR_TRY_LOG(...)                                                       \
  (__extension__({ XTR_LOG_TAGS(xtr::non_blocking_tag, "I", __VA_ARGS__); }))

#define XTR_LOG_TS(...) XTR_LOG_TAGS(xtr::timestamp_tag, "I", __VA_ARGS__)

#define XTR_TRY_LOG_TS(...)                                                    \
  XTR_LOG_TAGS((xtr::non_blocking_tag, xtr::timestamp_tag), "I", __VA_ARGS__)

#define XTR_LOG_RTC(SINK, FMT, ...)                                            \
  XTR_LOG_TS(SINK, FMT,                                                        \
             xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>() __VA_OPT__(, )   \
                 __VA_ARGS__)

#define XTR_TRY_LOG_RTC(SINK, FMT, ...)                                        \
  XTR_TRY_LOG_TS(SINK, FMT,                                                    \
                 xtr::detail::get_time<XTR_CLOCK_REALTIME_FAST>()              \
                     __VA_OPT__(, ) __VA_ARGS__)

#define XTR_LOG_TSC(SINK, FMT, ...)                                            \
  XTR_LOG_TS(SINK, FMT, xtr::detail::tsc::now() __VA_OPT__(, ) __VA_ARGS__)

#define XTR_TRY_LOG_TSC(SINK, FMT, ...)                                        \
  XTR_TRY_LOG_TS(SINK, FMT, xtr::detail::tsc::now() __VA_OPT__(, ) __VA_ARGS__)

#define XTR_LOG_LEVEL(LEVELSTR, LEVEL, SINK, ...)                              \
  (__extension__({                                                             \
    if ((SINK).level() >= xtr::log_level_t::LEVEL)                             \
      XTR_LOG_TAGS(void(), LEVELSTR, SINK, __VA_ARGS__);                       \
  }))

#define XTR_LOG_FATAL(SINK, ...)                                               \
  (__extension__({                                                             \
    XTR_LOG_LEVEL("F", fatal, SINK, __VA_ARGS__);                              \
    (SINK).sync();                                                             \
    std::abort();                                                              \
  }))

#define XTR_LOG_ERROR(...) XTR_LOG_LEVEL("E", error, __VA_ARGS__)
#define XTR_LOG_WARN(...) XTR_LOG_LEVEL("W", warning, __VA_ARGS__)
#define XTR_LOG_INFO(...) XTR_LOG_LEVEL("I", info, __VA_ARGS__)

#if defined(XTR_NDEBUG)
#define XTR_LOG_DEBUG(...)
#else
#define XTR_LOG_DEBUG(...) XTR_LOG_LEVEL("D", debug, __VA_ARGS__)
#endif

#define XTR_LOGF(...) XTR_LOG_FATAL(__VA_ARGS__)
#define XTR_LOGE(...) XTR_LOG_ERROR(__VA_ARGS__)
#define XTR_LOGW(...) XTR_LOG_WARN(__VA_ARGS__)
#define XTR_LOGI(...) XTR_LOG_INFO(__VA_ARGS__)
#define XTR_LOGD(...) XTR_LOG_DEBUG(__VA_ARGS__)

#define XTR_XSTR(s) XTR_STR(s)
#define XTR_STR(s) #s

#define XTR_LOG_TAGS(TAGS, LEVELSTR, SINK, FORMAT, ...)                        \
  (__extension__({                                                             \
    static constexpr auto xtr_fmt =                                            \
        xtr::detail::string{LEVELSTR " {} {} "} +                              \
        xtr::detail::rcut<xtr::detail::rindex(__FILE__, '/') + 1>(__FILE__) +  \
        xtr::detail::string{":"} +                                             \
        xtr::detail::string{XTR_XSTR(__LINE__) ": " FORMAT "\n"};              \
    using xtr::nocopy;                                                         \
    (SINK).log<&xtr_fmt, void(TAGS)>(__VA_ARGS__);                             \
  }))

namespace xtr {
class logger;

template <typename T> inline auto nocopy(const T &arg) {
  return detail::string_ref(arg);
}
} // namespace xtr

namespace xtr::detail {
inline auto make_output_func(FILE *stream) {
  return [stream](const char *buf, std::size_t size) {
    return std::fwrite(buf, 1, size, stream);
  };
}

inline auto make_error_func(FILE *stream) {
  return [stream](const char *buf, std::size_t size) {
    (void)std::fwrite(buf, 1, size, stream);
  };
}

inline auto make_flush_func(FILE *stream, FILE *err_stream) {
  return [stream, err_stream]() {
    std::fflush(stream);
    std::fflush(err_stream);
  };
}

inline auto make_sync_func(FILE *stream, FILE *err_stream) {
  return [stream, err_stream]() {
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    ::fsync(::fileno(stream));
    ::fsync(::fileno(err_stream));
#endif
  };
}

inline auto make_reopen_func(std::string path, FILE *stream) {
  return [path = std::move(path), stream]() {
    return std::freopen(path.c_str(), "a", stream) != nullptr;
  };
}

inline FILE *open_path(const char *path) {
  FILE *const fp = std::fopen(path, "a");
  if (fp == nullptr)
    detail::throw_system_error_fmt("Failed to open `%s'", path);
  return fp;
}
} // namespace xtr::detail

class xtr::logger {
private:
  using ring_buffer = detail::synchronized_ring_buffer<64 * 1024>;

  class consumer;

  using fptr_t = std::byte *(*)(fmt::memory_buffer &mbuf,
                                std::byte *buf, // pointer to log record
                                consumer &, const char *timestamp,
                                std::string &name) noexcept;

public:
  class producer {
  public:
    producer() = default;

    producer(const producer &other);

    producer &operator=(const producer &other);

    ~producer();

    void close();

    void sync() { sync(/*destroy=*/false); }

    void set_name(std::string name);

    template <auto Format, typename Tags = void()> void log() noexcept;

    template <auto Format, typename Tags = void(), typename... Args>
    void log(Args &&...args) noexcept(
        std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                           std::is_nothrow_move_constructible<Args>...>);

    void set_level(log_level_t l) {
      level_.store(l, std::memory_order_relaxed);
    }

    log_level_t level() const { return level_.load(std::memory_order_relaxed); }

  private:
    producer(logger &owner, std::string name);

    template <typename T>
    void copy(std::byte *pos, T &&value) noexcept; // XXX noexcept

    template <auto Format = nullptr, typename Tags = void(), typename Func>
    void post(Func &&func) noexcept(std::is_nothrow_move_constructible_v<Func>);

    template <auto Format, typename Tags, typename... Args>
    void post_with_str_table(Args &&...args) noexcept(
        std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                           std::is_nothrow_move_constructible<Args>...>);

    template <typename Tags, typename... Args>
    auto make_lambda(Args &&...args) noexcept(
        std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                           std::is_nothrow_move_constructible<Args>...>);

    void sync(bool destruct);

    ring_buffer buf_;
    std::atomic<log_level_t> level_{log_level_t::info};
    bool open_ = false;
    friend logger;
  };

private:
  class consumer {
  private:
    struct producer_handle {
      producer *operator->() { return p; }

      producer *p;
      std::string name;
      std::size_t dropped_count = 0;
    };

  public:
    void run(std::function<::timespec()> clock) noexcept;
    void set_command_path(std::string path) noexcept;

    template <typename OutputFunction, typename ErrorFunction,
              typename FlushFunction, typename SyncFunction,
              typename ReopenFunction, typename CloseFunction>
    consumer(OutputFunction &&of, ErrorFunction &&ef, FlushFunction &&ff,
             SyncFunction &&sf, ReopenFunction &&rf, CloseFunction &&cf,
             producer *control)
        : out(std::forward<OutputFunction>(of)),
          err(std::forward<ErrorFunction>(ef)),
          flush(std::forward<FlushFunction>(ff)),
          sync(std::forward<SyncFunction>(sf)),
          reopen(std::forward<ReopenFunction>(rf)),
          close(std::forward<CloseFunction>(cf)),
          producers_({{control, "control", 0}}) {}

    void add_producer(producer &p, const std::string &name);

    std::function<::ssize_t(const char *buf, std::size_t size)> out;
    std::function<void(const char *buf, std::size_t size)> err;
    std::function<void()> flush;
    std::function<void()> sync;
    std::function<bool()> reopen;
    std::function<void()> close;
    bool destroy = false;

  private:
    void status_handler(int fd, detail::status &);
    void set_level_handler(int fd, detail::set_level &);
    void reopen_handler(int fd, detail::reopen &);

    std::vector<producer_handle> producers_;
    std::unique_ptr<detail::command_dispatcher,
                    detail::command_dispatcher_deleter>
        cmds_;
  };

public:
  template <typename Clock = std::chrono::system_clock>
  logger(const char *path, Clock &&clock = Clock(),
         std::string command_path = default_command_path())
      : logger(path, detail::open_path(path), stderr,
               std::forward<Clock>(clock), std::move(command_path)) {}

  template <typename Clock = std::chrono::system_clock>
  logger(const char *path, FILE *stream, FILE *err_stream = stderr,
         Clock &&clock = Clock(),
         std::string command_path = default_command_path())
      : logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            detail::make_reopen_func(path, stream),
            [stream]() { std::fclose(stream); }, // close
            std::forward<Clock>(clock), std::move(command_path)) {}

  template <typename Clock = std::chrono::system_clock>
  logger(FILE *stream = stderr, FILE *err_stream = stderr,
         Clock &&clock = Clock(),
         std::string command_path = default_command_path())
      : logger(
            detail::make_output_func(stream),
            detail::make_error_func(err_stream),
            detail::make_flush_func(stream, err_stream),
            detail::make_sync_func(stream, err_stream),
            []() { return true; }, // reopen
            []() {},               // close
            std::forward<Clock>(clock), std::move(command_path)) {}

  template <typename OutputFunction, typename ErrorFunction,
            typename Clock = std::chrono::system_clock>
  requires std::invocable<OutputFunction, const char *, std::size_t> &&
      std::invocable<ErrorFunction, const char *, std::size_t>
      logger(OutputFunction &&out, ErrorFunction &&err, Clock &&clock = Clock(),
             std::string command_path = default_command_path())
      : logger(
            std::forward<OutputFunction>(out), std::forward<ErrorFunction>(err),
            []() {},               // flush
            []() {},               // sync
            []() { return true; }, // reopen
            []() {},               // close
            std::forward<Clock>(clock), std::move(command_path)) {}

  template <typename OutputFunction, typename ErrorFunction,
            typename FlushFunction, typename SyncFunction,
            typename ReopenFunction, typename CloseFunction,
            typename Clock = std::chrono::system_clock>
  requires std::invocable<OutputFunction, const char *, std::size_t> &&
      std::invocable<ErrorFunction, const char *, std::size_t> &&
      std::invocable<FlushFunction> && std::invocable<SyncFunction> &&
      std::invocable<ReopenFunction> && std::invocable<CloseFunction>
      logger(OutputFunction &&out, ErrorFunction &&err, FlushFunction &&flush,
             SyncFunction &&sync, ReopenFunction &&reopen,
             CloseFunction &&close, Clock &&clock = Clock(),
             std::string command_path = default_command_path()) {
    consumer_ =
        std::jthread(&consumer::run,
                     consumer(std::forward<OutputFunction>(out),
                              std::forward<ErrorFunction>(err),
                              std::forward<FlushFunction>(flush),
                              std::forward<SyncFunction>(sync),
                              std::forward<ReopenFunction>(reopen),
                              std::forward<CloseFunction>(close), &control_),
                     make_clock(std::forward<Clock>(clock)));
    control_.open_ = true;
    set_command_path(std::move(command_path));
  }

  ~logger();

  std::thread::native_handle_type consumer_thread_native_handle() {
    return consumer_.native_handle();
  }

  [[nodiscard]] producer get_producer(std::string name);

  void register_producer(producer &p, const std::string &name) noexcept;

  void set_output_stream(FILE *stream) noexcept;
  void set_error_stream(FILE *stream) noexcept;

  template <typename Func> void set_output_function(Func &&f) noexcept {
    static_assert(
        std::is_convertible_v<
            std::invoke_result_t<Func, const char *, std::size_t>, ::ssize_t>,
        "Output function type must be of type ssize_t(const char*, size_t) "
        "(returning the number of bytes written or -1 on error)");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.out = std::move(f);
    });
    control_.sync();
  }

  template <typename Func> void set_error_function(Func &&f) noexcept {
    static_assert(
        std::is_same_v<std::invoke_result_t<Func, const char *, std::size_t>,
                       void>,
        "Error function must be of type void(const char*, size_t)");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.err = std::move(f);
    });
    control_.sync();
  }

  template <typename Func> void set_flush_function(Func &&f) noexcept {
    static_assert(std::is_same_v<std::invoke_result_t<Func>, void>,
                  "Flush function must be of type void()");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.flush = std::move(f);
    });
    control_.sync();
  }

  template <typename Func> void set_sync_function(Func &&f) noexcept {
    static_assert(std::is_same_v<std::invoke_result_t<Func>, void>,
                  "Sync function must be of type void()");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.sync = std::move(f);
    });
    control_.sync();
  }

  template <typename Func> void set_reopen_function(Func &&f) noexcept {
    static_assert(std::is_same_v<std::invoke_result_t<Func>, bool>,
                  "Reopen function must be of type bool()");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.reopen = std::move(f);
    });
    control_.sync();
  }

  template <typename Func> void set_close_function(Func &&f) noexcept {
    static_assert(std::is_same_v<std::invoke_result_t<Func>, void>,
                  "Close function must be of type void()");
    post([f = std::forward<Func>(f)](consumer &c, auto &) {
      c.close = std::move(f);
    });
    control_.close();
  }

  void set_command_path(std::string path) noexcept;

private:
  template <typename Func> void post(Func &&f) {
    std::scoped_lock lock{control_mutex_};
    control_.post(std::forward<Func>(f));
  }

  template <typename Clock>
  std::function<std::timespec()> make_clock(Clock &&clock) {
    return [clock_{std::forward<Clock>(clock)}]() -> std::timespec {
      using namespace std::chrono;
      const auto now = clock_.now();
      auto sec = time_point_cast<seconds>(now);
      if (sec > now)
        sec - seconds{1};
      return std::timespec{.tv_sec = sec.time_since_epoch().count(),
                           .tv_nsec =
                               duration_cast<nanoseconds>(now - sec).count()};
    };
  }

  producer control_; // aligned to cache line so first to avoid extra padding
  std::jthread consumer_;
  std::mutex control_mutex_;
};

template <auto Format, typename Tags>
void xtr::logger::producer::log() noexcept {
  const ring_buffer::span s = buf_.write_span_spec<Tags>(sizeof(fptr_t));
  if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
    return;
  copy(s.begin(), &detail::trampoline0<Format, consumer>);
  buf_.reduce_writable(sizeof(fptr_t));
}

template <auto Format, typename Tags, typename... Args>
void xtr::logger::producer::log(Args &&...args) noexcept(
    std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                       std::is_nothrow_move_constructible<Args>...>) {
  static_assert(sizeof...(Args) > 0);
  constexpr bool is_str = std::disjunction_v<
      detail::is_c_string<decltype(std::forward<Args>(args))>...,
      std::is_same<std::remove_cvref_t<Args>, std::string_view>...,
      std::is_same<std::remove_cvref_t<Args>, std::string>...>;
  if constexpr (is_str)
    post_with_str_table<Format, Tags>(std::forward<Args>(args)...);
  else
    post<Format, Tags>(make_lambda<Tags>(std::forward<Args>(args)...));
}

template <auto Format, typename Tags, typename... Args>
void xtr::logger::producer::post_with_str_table(Args &&...args) noexcept(
    std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                       std::is_nothrow_move_constructible<Args>...>) {
  using lambda_t = decltype(make_lambda<Tags>(detail::build_string_table<Tags>(
      std::declval<std::byte *&>(), std::declval<std::byte *&>(), buf_,
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

  while (XTR_UNLIKELY(s.size() < size))
    [[unlikely]] {
      if constexpr (!detail::is_non_blocking_v<Tags>)
        detail::pause();
      s = buf_.write_span<Tags>();
      if (detail::is_non_blocking_v<Tags> && s.empty()) [[unlikely]]
        return;
    }

  auto str_cur = str_pos;
  auto str_end = s.end();

  copy(s.begin(), &detail::trampolineS<Format, consumer, lambda_t>);
  copy(func_pos, make_lambda<Tags>(detail::build_string_table<Tags>(
                     str_cur, str_end, buf_, std::forward<Args>(args))...));

  const auto next = detail::align<alignof(fptr_t)>(str_cur);
  const auto total_size = ring_buffer::size_type(next - s.begin());

  copy(size_pos, total_size);
  buf_.reduce_writable(total_size);
}

template <typename T>
void xtr::logger::producer::copy(std::byte *pos, T &&value) noexcept {
  assert(std::uintptr_t(pos) % alignof(T) == 0);
  pos = static_cast<std::byte *>(__builtin_assume_aligned(pos, alignof(T)));
  new (pos) std::remove_reference_t<T>(std::forward<T>(value));
}

template <auto Format, typename Tags, typename Func>
void xtr::logger::producer::post(Func &&func) noexcept(
    std::is_nothrow_move_constructible_v<Func>) {
  ring_buffer::span s = buf_.write_span_spec();

  auto func_pos = s.begin() + sizeof(fptr_t);
  if constexpr (alignof(Func) > alignof(fptr_t))
    func_pos = detail::align<alignof(Func)>(func_pos);

  const auto next = func_pos + detail::align(sizeof(Func), alignof(fptr_t));
  const auto size = ring_buffer::size_type(next - s.begin());

  while (XTR_UNLIKELY(s.size() < size))
    [[unlikely]] // XXX UNLIKELY
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

template <typename Tags, typename... Args>
auto xtr::logger::producer::make_lambda(Args &&...args) noexcept(
    std::conjunction_v<std::is_nothrow_copy_constructible<Args>...,
                       std::is_nothrow_move_constructible<Args>...>) {
  return [... args = std::forward<Args>(args)](
             fmt::memory_buffer &mbuf, const auto &out, const auto &err,
             std::string_view fmt, [[maybe_unused]] const char *ts,
             const std::string &name) mutable noexcept {
    if constexpr (detail::is_timestamp_v<Tags>) {
      xtr::detail::print_ts(mbuf, out, err, fmt, name, args...);
    } else {
      xtr::detail::print(mbuf, out, err, fmt, ts, name, args...);
    }
  };
}

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

namespace xtr::detail {
template <typename... Args> void errx(Args &&...args) {
  (std::cerr << ... << args) << "\n";
}

template <typename... Args> void err(Args &&...args) {
  const int errnum = errno;
  errx(std::forward<Args>(args)..., ": ", std::strerror(errnum));
}
} // namespace xtr::detail

inline xtr::detail::command_dispatcher::command_dispatcher(std::string path) {
  sockaddr_un addr;

  if (path.size() > sizeof(addr.sun_path) - 1) {
    errx("Error: Command path '", path, "' is too long");
    return;
  }

  detail::file_descriptor fd(
      ::socket(AF_LOCAL, SOCK_SEQPACKET | SOCK_NONBLOCK, 0));

  if (!fd) {
    err("Error: Failed to create command socket");
    return;
  }

  addr.sun_family = AF_LOCAL;
  std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);

#if defined(__linux__)
  if (addr.sun_path[0] == '\0') // abstract socket
  {
    std::memset(addr.sun_path + path.size(), '\0',
                sizeof(addr.sun_path) - path.size());
  }
#endif

  if (::bind(fd.get(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
      -1) {
    err("Error: Failed to bind command socket to path '", path, "'");
    return;
  }

  path_ = std::move(path);

  if (::listen(fd.get(), 64) == -1) {
    err("Error: Failed to listen on command socket");
    return;
  }

  pollfds_.push_back(pollfd{std::move(fd), POLLIN, 0});
}

inline xtr::detail::command_dispatcher::~command_dispatcher() {
  if (!path_.empty())
    ::unlink(path_.c_str());
}

inline xtr::detail::command_dispatcher::buffer::buffer(const void *srcbuf,
                                                       std::size_t srcsize)
    : buf(new char[srcsize]), size(srcsize) {
  std::memcpy(buf.get(), srcbuf, size);
}

inline void xtr::detail::command_dispatcher::send(int fd, const void *buf,
                                                  std::size_t nbytes) {
  results_[fd].bufs.emplace_back(buf, nbytes);
}

inline void
xtr::detail::command_dispatcher::process_commands(int timeout) noexcept {
  int nfds = ::poll(reinterpret_cast<::pollfd *>(&pollfds_[0]),
                    ::nfds_t(pollfds_.size()), timeout);

  if (pollfds_[0].revents & POLLIN) {
    detail::file_descriptor fd(
        ::accept4(pollfds_[0].fd.get(), nullptr, nullptr, SOCK_NONBLOCK));
    if (fd)
      pollfds_.push_back(pollfd{std::move(fd), POLLIN, 0});
    else
      err("Error: Failed to accept connection on command socket");
    --nfds;
  }

  for (std::size_t i = 1; i < pollfds_.size() && nfds > 0; ++i) {
    const int fd = pollfds_[i].fd.get();
    if (pollfds_[i].revents & POLLOUT) {
      process_socket_write(pollfds_[i]);
      --nfds;
    } else if (pollfds_[i].revents & (POLLHUP | POLLIN)) {
      process_socket_read(pollfds_[i]);
      --nfds;
    }
    if (fd != pollfds_[i].fd.get())
      --i; // Adjust for erased item
  }
}

inline void
xtr::detail::command_dispatcher::process_socket_read(pollfd &pfd) noexcept {
  const int fd = pfd.fd.get();
  frame_buf buf;

  const ::ssize_t nbytes = command_recv(fd, buf);

  pfd.events = POLLOUT;

  if (nbytes == -1) {
    err("Error: Failed to read command");
    return;
  }

  if (nbytes == 0) // EOF
    return;

  if (nbytes < ::ssize_t(sizeof(frame_header))) {
    send_error(fd, "Incomplete frame header");
    return;
  }

  const auto cpos = callbacks_.find(buf.hdr.frame_id);

  if (cpos == callbacks_.end()) {
    send_error(fd, "Invalid frame id");
    return;
  }

  if (nbytes != ::ssize_t(cpos->second.size)) {
    send_error(fd, "Invalid frame length");
    return;
  }

#if __cpp_exceptions
  try {
#endif
    cpos->second.func(fd, &buf);
#if __cpp_exceptions
  } catch (const std::exception &e) {
    send_error(fd, e.what());
  }
#endif
}

inline void
xtr::detail::command_dispatcher::process_socket_write(pollfd &pfd) noexcept {
  const int fd = pfd.fd.get();

  callback_result &cr = results_[fd];

  ::ssize_t nwritten = 0;

  for (; cr.pos < cr.bufs.size(); ++cr.pos) {
    nwritten =
        command_send(fd, cr.bufs[cr.pos].buf.get(), cr.bufs[cr.pos].size);
    if (nwritten != ::ssize_t(cr.bufs[cr.pos].size))
      break;
  }

  if ((nwritten == -1 && errno != EAGAIN) || cr.pos == cr.bufs.size()) {
    results_.erase(fd);
    disconnect(pfd);
  }
}

inline void xtr::detail::command_dispatcher::disconnect(pollfd &pfd) noexcept {
  assert(results_.count(pfd.fd.get()) == 0);
  std::swap(pfd, pollfds_.back());
  pollfds_.pop_back();
}

inline void
xtr::detail::command_dispatcher::send_error(int fd, std::string_view reason) {
  frame<error> ef;
  strzcpy(ef->reason, reason);
  send(fd, ef);
}

inline void
xtr::detail::command_dispatcher_deleter::operator()(command_dispatcher *d) {
  delete d;
}

#include <atomic>
#include <cstdio>

#include <limits.h>
#include <sys/types.h>
#include <unistd.h>

inline std::string xtr::default_command_path() {
  static std::atomic<unsigned> ctl_count{0};
  const long pid = ::getpid();
  const unsigned long uid = ::geteuid();
  const unsigned n = ctl_count++;
  char dpath[32];
  char path[PATH_MAX];

  std::snprintf(dpath, sizeof(dpath), "/run/user/%lu", uid);

  if (::access(dpath, W_OK) != 0)
    std::snprintf(dpath, sizeof(dpath), "/tmp");

  std::snprintf(path, sizeof(path), "%s/xtrctl.%ld.%u", dpath, pid, n);

  return path;
}

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

inline xtr::detail::file_descriptor::file_descriptor(const char *path,
                                                     int flags, int mode)
    : fd_(XTR_TEMP_FAILURE_RETRY(::open(path, flags, mode))) {
  if (fd_ == -1) {
    throw_system_error_fmt("xtr::detail::file_descriptor::file_descriptor: "
                           "Failed to open `%s'",
                           path);
  }
}

inline xtr::detail::file_descriptor &xtr::detail::file_descriptor::operator=(
    xtr::detail::file_descriptor &&other) noexcept {
  swap(*this, other);
  return *this;
}

inline xtr::detail::file_descriptor::~file_descriptor() { reset(); }

inline void xtr::detail::file_descriptor::reset(int fd) noexcept {
  if (is_open())
    (void)::close(fd_);
  fd_ = fd;
}

#include <fmt/chrono.h>

#include <algorithm>
#include <climits>
#include <condition_variable>
#include <cstring>

inline xtr::logger::~logger() { control_.close(); }

inline xtr::logger::producer xtr::logger::get_producer(std::string name) {
  return producer(*this, std::move(name));
}

inline void xtr::logger::register_producer(producer &p,
                                           const std::string &name) noexcept {
  post([&p, name](consumer &c, auto &) { c.add_producer(p, name); });
}

inline void xtr::logger::set_output_stream(FILE *stream) noexcept {
  set_output_function(detail::make_output_func(stream));
}

inline void xtr::logger::set_error_stream(FILE *stream) noexcept {
  set_error_function(detail::make_error_func(stream));
}

inline void xtr::logger::set_command_path(std::string path) noexcept {
  post([p = std::move(path)](consumer &c, auto &) {
    c.set_command_path(std::move(p));
  });
  control_.sync();
}

inline void
xtr::logger::consumer::run(std::function<::timespec()> clock) noexcept {
  char ts[32] = {};
  bool ts_stale = true;
  std::size_t flush_count = 0;
  fmt::memory_buffer mbuf;

  for (std::size_t i = 0; !producers_.empty(); ++i) {
    ring_buffer::span span;
    const std::size_t n = i % producers_.size();

    if (n == 0) {
      ts_stale |= true;
      if (cmds_)
        cmds_->process_commands(/* timeout= */ 0);
    }

    if ((span = producers_[n]->buf_.read_span()).empty()) {
      if (flush_count != 0 && flush_count-- == 1)
        flush();
      continue;
    }

    destroy = false;

    if (ts_stale) {
      fmt::format_to(ts, "{}", xtr::timespec{clock()});
      ts_stale = false;
    }

    std::byte *pos = span.begin();
    std::byte *end = std::min(span.end(), producers_[n]->buf_.end());
    do {
      assert(std::uintptr_t(pos) % alignof(fptr_t) == 0);
      assert(!destroy);
      fptr_t fptr = *reinterpret_cast<const fptr_t *>(pos);
      pos = fptr(mbuf, pos, *this, ts, producers_[n].name);
    } while (pos < end);

    if (destroy) {
      using std::swap;
      swap(producers_[n], producers_.back()); // possible self-swap, ok
      producers_.pop_back();
      continue;
    }

    producers_[n]->buf_.reduce_readable(
        ring_buffer::size_type(pos - span.begin()));

    std::size_t n_dropped;
    if (producers_[n]->buf_.read_span().empty() &&
        (n_dropped = producers_[n]->buf_.dropped_count()) > 0) {
      detail::print(mbuf, out, err, "W {} {}: {} messages dropped\n", ts,
                    producers_[n].name, n_dropped);
      producers_[n].dropped_count += n_dropped;
    }

    flush_count = producers_.size();
  }

  close();
}

inline void xtr::logger::consumer::add_producer(producer &p,
                                                const std::string &name) {
  producers_.push_back(producer_handle{&p, name});
}

inline void xtr::logger::consumer::set_command_path(std::string path) noexcept {
  if (path == null_command_path)
    return;

  cmds_.reset(new detail::command_dispatcher(std::move(path)));

  if (!cmds_->is_open()) {
    cmds_.reset();
    return;
  }

  cmds_->register_callback<detail::status>(
      std::bind_front(&consumer::status_handler, this));

  cmds_->register_callback<detail::set_level>(
      std::bind_front(&consumer::set_level_handler, this));

  cmds_->register_callback<detail::reopen>(
      std::bind_front(&consumer::reopen_handler, this));
}

inline void xtr::logger::consumer::status_handler(int fd, detail::status &st) {
  st.pattern.text[sizeof(st.pattern.text) - 1] = '\0';

  const auto matcher = detail::make_matcher(st.pattern.type, st.pattern.text,
                                            st.pattern.ignore_case);

  if (!matcher->valid()) {
    detail::frame<detail::error> ef;
    matcher->error_reason(ef->reason, sizeof(ef->reason));
    cmds_->send(fd, ef);
    return;
  }

  for (std::size_t i = 1; i < producers_.size(); ++i) {
    auto &p = producers_[i];

    if (!(*matcher)(p.name.c_str()))
      continue;

    detail::frame<detail::sink_info> sif;

    sif->level = p->level();
    sif->buf_capacity = p->buf_.capacity();
    sif->buf_nbytes = p->buf_.read_span().size();
    sif->dropped_count = p.dropped_count;
    detail::strzcpy(sif->name, p.name);

    cmds_->send(fd, sif);
  }
}

inline void xtr::logger::consumer::set_level_handler(int fd,
                                                     detail::set_level &sl) {
  sl.pattern.text[sizeof(sl.pattern.text) - 1] = '\0';

  if (sl.level > xtr::log_level_t::debug) {
    cmds_->send_error(fd, "Invalid level");
    return;
  }

  const auto matcher = detail::make_matcher(sl.pattern.type, sl.pattern.text,
                                            sl.pattern.ignore_case);

  if (!matcher->valid()) {
    detail::frame<detail::error> ef;
    matcher->error_reason(ef->reason, sizeof(ef->reason));
    cmds_->send(fd, ef);
    return;
  }

  for (std::size_t i = 1; i < producers_.size(); ++i) {
    auto &p = producers_[i];

    if (!(*matcher)(p.name.c_str()))
      continue;

    p->set_level(sl.level);
  }

  cmds_->send(fd, detail::frame<detail::success>());
}

inline void xtr::logger::consumer::reopen_handler(int fd, detail::reopen &) {
  if (!reopen())
    cmds_->send_error(fd, std::strerror(errno));
  else
    cmds_->send(fd, detail::frame<detail::success>());
}

inline xtr::logger::producer::producer(const producer &other) { *this = other; }

inline xtr::logger::producer &
xtr::logger::producer::operator=(const producer &other) {
  level_ = other.level_.load(std::memory_order_relaxed);
  if (!std::exchange(open_, other.open_)) // if previously closed, register
  {
    const_cast<producer &>(other).post(
        [this](consumer &c, const auto &name) { c.add_producer(*this, name); });
  }
  return *this;
}

inline xtr::logger::producer::producer(logger &owner, std::string name)
    : open_(true) {
  owner.register_producer(*this, name);
}

inline void xtr::logger::producer::close() {
  if (open_) {
    sync(/*destruct=*/true);
    open_ = false;
    buf_.clear();
  }
}

inline void xtr::logger::producer::sync(bool destroy) {
  std::condition_variable cv;
  std::mutex m;
  bool notified = false; // protected by m

  post([&cv, &m, &notified, destroy](consumer &c, auto &) {
    c.destroy = destroy;

    c.flush();
    c.sync();

    std::scoped_lock lock{m};
    notified = true;
    cv.notify_one();
  });

  std::unique_lock lock{m};
  while (!notified)
    cv.wait(lock);
}

inline void xtr::logger::producer::set_name(std::string name) {
  post([name = std::move(name)](auto &, auto &oldname) {
    oldname = std::move(name);
  });
  sync();
}

inline xtr::logger::producer::~producer() { close(); }

inline std::unique_ptr<xtr::detail::matcher>
xtr::detail::make_matcher(pattern_type_t pattern_type, const char *pattern,
                          bool ignore_case) {
  switch (pattern_type) {
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

#include <unistd.h>

inline xtr::detail::memory_mapping::memory_mapping(void *addr,
                                                   std::size_t length, int prot,
                                                   int flags, int fd,
                                                   std::size_t offset)
    : mem_(::mmap(addr, length, prot, flags, fd, ::off_t(offset))),
      length_(length) {
  if (mem_ == MAP_FAILED) {
    throw_system_error(
        "xtr::detail::memory_mapping::memory_mapping: mmap failed");
  }
}

inline xtr::detail::memory_mapping::memory_mapping(
    memory_mapping &&other) noexcept
    : mem_(other.mem_), length_(other.length_) {
  other.release();
}

inline xtr::detail::memory_mapping &
xtr::detail::memory_mapping::operator=(memory_mapping &&other) noexcept {
  swap(*this, other);
  return *this;
}

inline xtr::detail::memory_mapping::~memory_mapping() { reset(); }

inline void xtr::detail::memory_mapping::reset(void *addr,
                                               std::size_t length) noexcept {
  if (mem_ != MAP_FAILED) {
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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#if !defined(__linux__)
namespace xtr::detail {
inline file_descriptor shm_open_anon(int oflag, mode_t mode) {
  int fd;

#if defined(SHM_ANON) // FreeBSD extension
  fd = XTR_TEMP_FAILURE_RETRY(::shm_open(SHM_ANON, oflag, mode));
#else
  const char ctable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789~-";
  char name[] = "/xtr.XXXXXXXXXXXXXXXX";

  std::random_device rd;
  std::uniform_int_distribution<> udist(0, sizeof(ctable) - 2);

  std::size_t retries = 64;
  do {
    for (char *pos = name + 5; *pos != '\0'; ++pos)
      *pos = ctable[udist(rd)];
    fd = XTR_TEMP_FAILURE_RETRY(
        ::shm_open(name, oflag | O_EXCL | O_CREAT, mode));
  } while (--retries > 0 && fd == -1 && errno == EEXIST);

  if (fd != -1)
    ::shm_unlink(name);
#endif

  return file_descriptor(fd);
}
} // namespace xtr::detail
#endif

inline xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping(
    std::size_t length, int fd, std::size_t offset, int flags) {
  assert(!(flags & MAP_ANONYMOUS) || fd == -1);
  assert((flags & MAP_FIXED) == 0); // Not implemented (would be easy though)
  assert((flags & MAP_PRIVATE) ==
         0); // Can't be private, must be shared for mirroring to work

  if (length != align_to_page_size(length)) {
    throw_invalid_argument(
        "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
        "Length argument is not page-aligned");
  }

  const int prot = PROT_READ | PROT_WRITE;

  memory_mapping reserve(nullptr, length * 2, prot,
                         MAP_PRIVATE | MAP_ANONYMOUS);

#if !defined(__linux__)
  file_descriptor temp_fd;
#endif

  if (fd == -1) {
#if defined(__linux__)
    memory_mapping mirror(static_cast<std::byte *>(reserve.get()) + length,
                          length, prot,
                          MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS | flags);

    m_.reset(::mremap(mirror.get(), 0, length, MREMAP_FIXED | MREMAP_MAYMOVE,
                      reserve.get()),
             length);

    if (!m_) {
      throw_system_error(
          "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
          "mremap failed");
    }

    reserve.release(); // mapping was destroyed by mremap
    mirror.release();  // mirror will be recreated in ~mirrored_memory_mapping
    return;
#else
    if (!(temp_fd = shm_open_anon(O_RDWR, S_IRUSR | S_IWUSR))) {
      throw_system_error(
          "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
          "Failed to shm_open backing file");
    }

    fd = temp_fd.get();

    if (::ftruncate(fd, ::off_t(length)) == -1) {
      throw_system_error(
          "xtr::detail::mirrored_memory_mapping::mirrored_memory_mapping: "
          "Failed to ftruncate backing file");
    }
#endif
  }

  flags &= ~MAP_ANONYMOUS; // Mappings must be shared
  flags |= MAP_FIXED | MAP_SHARED;

  memory_mapping mirror(static_cast<std::byte *>(reserve.get()) + length,
                        length, prot, flags, fd, offset);

  m_ = memory_mapping(reserve.get(), length, prot, flags, fd, offset);

  reserve.release(); // mapping was destroyed when m_ was created
  mirror.release();  // mirror will be recreated in ~mirrored_memory_mapping
}

inline xtr::detail::mirrored_memory_mapping::~mirrored_memory_mapping() {
  if (m_) {
    memory_mapping{}.reset(static_cast<std::byte *>(m_.get()) + m_.length(),
                           m_.length());
  }
}

#include <unistd.h>

inline std::size_t xtr::detail::align_to_page_size(std::size_t length) {
  static const long pagesize(::sysconf(_SC_PAGESIZE));
  if (pagesize == -1)
    throw_system_error("sysconf(_SC_PAGESIZE) failed");
  return align(length, std::size_t(pagesize));
}

#include <cassert>

inline xtr::detail::regex_matcher::regex_matcher(const char *pattern,
                                                 bool ignore_case,
                                                 bool extended) {
  const int flags =
      REG_NOSUB | (ignore_case ? REG_ICASE : 0) | (extended ? REG_EXTENDED : 0);
  errnum_ = ::regcomp(&regex_, pattern, flags);
}

inline xtr::detail::regex_matcher::~regex_matcher() {
  if (valid())
    ::regfree(&regex_);
}

inline bool xtr::detail::regex_matcher::valid() const { return errnum_ == 0; }

inline void xtr::detail::regex_matcher::error_reason(char *buf,
                                                     std::size_t bufsz) const {
  assert(errnum_ != 0);
  ::regerror(errnum_, &regex_, buf, bufsz);
}

inline bool xtr::detail::regex_matcher::operator()(const char *str) const {
  return ::regexec(&regex_, str, 0, nullptr, 0) == 0;
}

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <system_error>

inline void xtr::detail::throw_runtime_error(const char *what) {
#if __cpp_exceptions
  throw std::runtime_error(what);
#else
  std::fprintf(stderr, "runtime error: %s\n", what);
  std::abort();
#endif
}

inline void xtr::detail::throw_runtime_error_fmt(const char *format, ...) {
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

inline void xtr::detail::throw_system_error(const char *what) {
#if __cpp_exceptions
  throw std::system_error(std::error_code(errno, std::generic_category()),
                          what);
#else
  std::fprintf(stderr, "system error: %s: %s\n", what, std::strerror(errno));
  std::abort();
#endif
}

inline void xtr::detail::throw_system_error_fmt(const char *format, ...) {
  const int errnum = errno; // in case vsnprintf modifies errno
  va_list args;
  va_start(args, format);
  ;
  char buf[1024];
  std::vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
#if __cpp_exceptions
  throw std::system_error(std::error_code(errnum, std::generic_category()),
                          buf);
#else
  std::fprintf(stderr, "system error: %s: %s\n", buf, std::strerror(errnum));
  std::abort();
#endif
}

inline void xtr::detail::throw_invalid_argument(const char *what) {
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

inline std::uint64_t xtr::detail::read_tsc_hz() noexcept {
  constexpr int tsc_leaf = 0x15;

  if (cpuid(0)[0] < tsc_leaf)
    return 0;

  auto [ratio_den, ratio_num, ccc_hz, unused] = cpuid(0x15);

  if (ccc_hz == 0) {
    const std::uint16_t model = get_family_model()[1];
    switch (model) {
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

inline std::uint64_t xtr::detail::estimate_tsc_hz() noexcept {
  const std::uint64_t tsc0 = tsc::now().ticks;
  std::timespec ts0;
  ::clock_gettime(XTR_CLOCK_MONOTONIC, &ts0);

  std::array<std::uint64_t, 5> history;
  std::size_t n = 0;

  const auto sleep_time = std::chrono::milliseconds(10);
  const auto max_sleep_time = std::chrono::seconds(2);
  const std::size_t tick_range = 1000;
  const std::size_t max_iters = max_sleep_time / sleep_time;

  for (;;) {
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

    if (n >= history.size()) {
      const auto min = *std::min_element(history.begin(), history.end());
      const auto max = *std::max_element(history.begin(), history.end());
      if (max - min < tick_range || n >= max_iters)
        return tsc_hz;
    }
  }
}

inline std::timespec xtr::detail::tsc::to_timespec(tsc ts) {
  thread_local tsc last_tsc{};
  thread_local std::int64_t last_epoch_nanos;
  static const std::uint64_t one_minute_ticks = 60 * get_tsc_hz();
  static const double tsc_multiplier = 1e9 / double(get_tsc_hz());

  if (ts.ticks > last_tsc.ticks + one_minute_ticks) {
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

#include <fnmatch.h>

inline xtr::detail::wildcard_matcher::wildcard_matcher(const char *pattern,
                                                       bool ignore_case)
    : pattern_(pattern), flags_(ignore_case ? FNM_CASEFOLD : 0) {}

inline bool xtr::detail::wildcard_matcher::operator()(const char *str) const {
  return ::fnmatch(pattern_, str, flags_) == 0;
}

#endif
