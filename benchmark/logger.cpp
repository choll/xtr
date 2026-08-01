#include "xtr/logger.hpp"
#include "xtr/vcopy.hpp"

#include <benchmark/benchmark.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>

#include <pthread.h>
#if __has_include(<pthread_np.h>)
#include <pthread_np.h>
#endif
#include <sched.h>

namespace
{
    void set_thread_attrs(pthread_t thread, int cpu)
    {
#if defined(__FreeBSD__)
        cpuset_t cpus;
#else
        cpu_set_t cpus;
#endif
        CPU_ZERO(&cpus);
        CPU_SET(cpu, &cpus);
        if (::pthread_setaffinity_np(thread, sizeof(cpus), &cpus) != 0)
            abort();
    }

    int getenv_int(const char* name)
    {
        const char* env = ::getenv(name);
        if (env == nullptr)
            return -1;
        char* end;
        errno = 0;
        const long result = std::strtol(env, &end, 10);
        if (errno != 0 || name == end || *end != '\0')
        {
            std::cerr << name << "=" << env << " is invalid\n";
            abort();
        }
        return int(result);
    }

    // These are all mutable so that they can be used with DoNotOptimize()
    const char* c_str_arg8{"12345678"};
    const char* c_str_arg16{"1234567890123456"};
    const char* c_str_arg32{"12345678901234567890123456789012"};
    const char* c_str_arg64{
        "1234567890123456789012345678901234567890123456789012345678901234"};
    std::string_view sv_arg8{"12345678"};
    std::string_view sv_arg16{"1234567890123456"};
    std::string_view sv_arg32{"12345678901234567890123456789012"};
    std::string_view sv_arg64{
        "1234567890123456789012345678901234567890123456789012345678901234"};
    std::string str_arg8{"12345678"};
    std::string str_arg16{"1234567890123456"};
    std::string str_arg32{"12345678901234567890123456789012"};
    std::string str_arg64{
        "1234567890123456789012345678901234567890123456789012345678901234"};
    int int_arg = 42;
    long long_arg = 42L;
    double double_arg = 42.0;

    struct variable_length_struct
    {
        std::size_t size;
        __extension__ std::byte data[];
    };

    template<std::size_t N>
    variable_length_struct& make_vls()
    {
        alignas(variable_length_struct) static std::byte storage[N];
        auto& vls = *reinterpret_cast<variable_length_struct*>(storage);
        vls.size = N - offsetof(variable_length_struct, data);
        return vls;
    }

    variable_length_struct& vcopy_arg64 = make_vls<64>();
    variable_length_struct& vcopy_arg128 = make_vls<128>();
    variable_length_struct& vcopy_arg256 = make_vls<256>();
    std::size_t vcopy_size64 = 64;
    std::size_t vcopy_size128 = 128;
    std::size_t vcopy_size256 = 256;
}

template<>
struct fmt::formatter<variable_length_struct>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    // Only the element count is formatted, so that the consumer thread does
    // not become the bottleneck---the benchmark measures the call site.
    template<typename FormatContext>
    auto format(const variable_length_struct& v, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", v.size);
    }
};

// The logger has a fixed size ring buffer, msgsize is to ensure that
// the test isn't bottlenecked on I/O to the log file.
#define LOG_BENCH(NAME, X, MSGSIZE)                                       \
    void NAME(benchmark::State& state)                                    \
    {                                                                     \
        FILE* fp = ::fopen("/dev/null", "w");                             \
        xtr::logger log{fp};                                              \
                                                                          \
        if (const int cpu = getenv_int("PRODUCER_CPU"); cpu != -1)        \
            set_thread_attrs(::pthread_self(), cpu);                      \
                                                                          \
        if (const int cpu = getenv_int("CONSUMER_CPU"); cpu != -1)        \
            set_thread_attrs(log.consumer_thread_native_handle(), cpu);   \
                                                                          \
        xtr::sink p = log.get_sink("Name");                               \
        std::size_t n = 0;                                                \
        constexpr std::size_t sync_every = XTR_SINK_CAPACITY / (MSGSIZE); \
        for (auto _ : state)                                              \
        {                                                                 \
            X;                                                            \
            if (++n % sync_every == 0)                                    \
            {                                                             \
                state.PauseTiming();                                      \
                p.sync();                                                 \
                state.ResumeTiming();                                     \
            }                                                             \
        }                                                                 \
                                                                          \
        ::fclose(fp);                                                     \
    }                                                                     \
    BENCHMARK(NAME);

LOG_BENCH(logger_benchmark, XTR_LOG(p, "Test"), 8)
LOG_BENCH(
    logger_benchmark_int,
    (benchmark::DoNotOptimize(int_arg), XTR_LOG(p, "Test {}", int_arg)),
    16)
LOG_BENCH(
    logger_benchmark_long,
    (benchmark::DoNotOptimize(long_arg), XTR_LOG(p, "Test {}", long_arg)),
    16)
LOG_BENCH(
    logger_benchmark_double,
    (benchmark::DoNotOptimize(double_arg), XTR_LOG(p, "Test {}", double_arg)),
    16)
LOG_BENCH(
    logger_benchmark_c_str_8,
    (benchmark::DoNotOptimize(c_str_arg8), XTR_LOG(p, "Test {}", c_str_arg8)),
    32)
LOG_BENCH(
    logger_benchmark_c_str_16,
    (benchmark::DoNotOptimize(c_str_arg16), XTR_LOG(p, "Test {}", c_str_arg16)),
    40)
LOG_BENCH(
    logger_benchmark_c_str_32,
    (benchmark::DoNotOptimize(c_str_arg32), XTR_LOG(p, "Test {}", c_str_arg32)),
    56)
LOG_BENCH(
    logger_benchmark_c_str_64,
    (benchmark::DoNotOptimize(c_str_arg64), XTR_LOG(p, "Test {}", c_str_arg64)),
    88)
LOG_BENCH(
    logger_benchmark_str_view_8,
    (benchmark::DoNotOptimize(sv_arg8), XTR_LOG(p, "Test {}", sv_arg8)),
    32)
LOG_BENCH(
    logger_benchmark_str_view_16,
    (benchmark::DoNotOptimize(sv_arg16), XTR_LOG(p, "Test {}", sv_arg16)),
    40)
LOG_BENCH(
    logger_benchmark_str_view_32,
    (benchmark::DoNotOptimize(sv_arg32), XTR_LOG(p, "Test {}", sv_arg32)),
    56)
LOG_BENCH(
    logger_benchmark_str_view_64,
    (benchmark::DoNotOptimize(sv_arg64), XTR_LOG(p, "Test {}", sv_arg64)),
    88)
LOG_BENCH(
    logger_benchmark_str_8,
    (benchmark::DoNotOptimize(str_arg8), XTR_LOG(p, "Test {}", str_arg8)),
    32)
LOG_BENCH(
    logger_benchmark_str_16,
    (benchmark::DoNotOptimize(str_arg16), XTR_LOG(p, "Test {}", str_arg16)),
    40)
LOG_BENCH(
    logger_benchmark_str_32,
    (benchmark::DoNotOptimize(str_arg32), XTR_LOG(p, "Test {}", str_arg32)),
    56)
LOG_BENCH(
    logger_benchmark_str_64,
    (benchmark::DoNotOptimize(str_arg64), XTR_LOG(p, "Test {}", str_arg64)),
    88)
LOG_BENCH(
    logger_benchmark_vcopy_64,
    (benchmark::DoNotOptimize(vcopy_arg64),
     benchmark::DoNotOptimize(vcopy_size64),
     XTR_LOG(p, "Test {}", xtr::vcopy(vcopy_arg64, vcopy_size64))),
    96)
LOG_BENCH(
    logger_benchmark_vcopy_128,
    (benchmark::DoNotOptimize(vcopy_arg128),
     benchmark::DoNotOptimize(vcopy_size128),
     XTR_LOG(p, "Test {}", xtr::vcopy(vcopy_arg128, vcopy_size128))),
    160)
LOG_BENCH(
    logger_benchmark_vcopy_256,
    (benchmark::DoNotOptimize(vcopy_arg256),
     benchmark::DoNotOptimize(vcopy_size256),
     XTR_LOG(p, "Test {}", xtr::vcopy(vcopy_arg256, vcopy_size256))),
    288)
LOG_BENCH(logger_benchmark_tsc, XTR_LOG_TSC(p, "Test"), 16)
LOG_BENCH(
    logger_benchmark_tsc_int,
    (benchmark::DoNotOptimize(int_arg), XTR_LOG_TSC(p, "Test {}", int_arg)),
    24)
LOG_BENCH(
    logger_benchmark_tsc_long,
    (benchmark::DoNotOptimize(long_arg), XTR_LOG_TSC(p, "Test {}", long_arg)),
    24)
LOG_BENCH(
    logger_benchmark_tsc_double,
    (benchmark::DoNotOptimize(double_arg), XTR_LOG_TSC(p, "Test {}", double_arg)),
    24)
LOG_BENCH(
    logger_benchmark_tsc_c_str_8,
    (benchmark::DoNotOptimize(c_str_arg8), XTR_LOG_TSC(p, "Test {}", c_str_arg8)),
    40)
LOG_BENCH(
    logger_benchmark_tsc_c_str_16,
    (benchmark::DoNotOptimize(c_str_arg16), XTR_LOG_TSC(p, "Test {}", c_str_arg16)),
    48)
LOG_BENCH(
    logger_benchmark_tsc_c_str_32,
    (benchmark::DoNotOptimize(c_str_arg32), XTR_LOG_TSC(p, "Test {}", c_str_arg32)),
    64)
LOG_BENCH(
    logger_benchmark_tsc_c_str_64,
    (benchmark::DoNotOptimize(c_str_arg64), XTR_LOG_TSC(p, "Test {}", c_str_arg64)),
    96)
LOG_BENCH(
    logger_benchmark_tsc_str_view_8,
    (benchmark::DoNotOptimize(sv_arg8), XTR_LOG_TSC(p, "Test {}", sv_arg8)),
    40)
LOG_BENCH(
    logger_benchmark_tsc_str_view_16,
    (benchmark::DoNotOptimize(sv_arg16), XTR_LOG_TSC(p, "Test {}", sv_arg16)),
    48)
LOG_BENCH(
    logger_benchmark_tsc_str_view_32,
    (benchmark::DoNotOptimize(sv_arg32), XTR_LOG_TSC(p, "Test {}", sv_arg32)),
    64)
LOG_BENCH(
    logger_benchmark_tsc_str_view_64,
    (benchmark::DoNotOptimize(sv_arg64), XTR_LOG_TSC(p, "Test {}", sv_arg64)),
    96)
LOG_BENCH(
    logger_benchmark_tsc_str_8,
    (benchmark::DoNotOptimize(str_arg8), XTR_LOG_TSC(p, "Test {}", str_arg8)),
    40)
LOG_BENCH(
    logger_benchmark_tsc_str_16,
    (benchmark::DoNotOptimize(str_arg16), XTR_LOG_TSC(p, "Test {}", str_arg16)),
    48)
LOG_BENCH(
    logger_benchmark_tsc_str_32,
    (benchmark::DoNotOptimize(str_arg32), XTR_LOG_TSC(p, "Test {}", str_arg32)),
    64)
LOG_BENCH(
    logger_benchmark_tsc_str_64,
    (benchmark::DoNotOptimize(str_arg64), XTR_LOG_TSC(p, "Test {}", str_arg64)),
    96)
LOG_BENCH(
    logger_benchmark_tsc_vcopy_64,
    (benchmark::DoNotOptimize(vcopy_arg64),
     benchmark::DoNotOptimize(vcopy_size64),
     XTR_LOG_TSC(p, "Test {}", xtr::vcopy(vcopy_arg64, vcopy_size64))),
    104)
LOG_BENCH(
    logger_benchmark_tsc_vcopy_128,
    (benchmark::DoNotOptimize(vcopy_arg128),
     benchmark::DoNotOptimize(vcopy_size128),
     XTR_LOG_TSC(p, "Test {}", xtr::vcopy(vcopy_arg128, vcopy_size128))),
    168)
LOG_BENCH(
    logger_benchmark_tsc_vcopy_256,
    (benchmark::DoNotOptimize(vcopy_arg256),
     benchmark::DoNotOptimize(vcopy_size256),
     XTR_LOG_TSC(p, "Test {}", xtr::vcopy(vcopy_arg256, vcopy_size256))),
    296)
LOG_BENCH(logger_benchmark_clock_realtime_coarse, XTR_LOG_RTC(p, "Test"), 24)
LOG_BENCH(logger_benchmark_non_blocking, XTR_TRY_LOG(p, "Test"), 8)
