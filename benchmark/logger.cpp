#include "xtr/logger.hpp"

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>
#include <string_view>

#include <pthread.h>
#include <sched.h>

// XXX TODO NEED TO ACCEPT AFFINITY AS ARGUMENTS

namespace
{
    void set_thread_attrs(pthread_t thread, int cpu)
    {
        cpu_set_t cpus;
        CPU_ZERO(&cpus);
        CPU_SET(cpu, &cpus);
        if (::pthread_setaffinity_np(thread, sizeof(cpus), &cpus) != 0)
            abort();
    }
}

#define LOG_BENCH_DISCARD(NAME, X, MSGSIZE)                             \
    void NAME ## _discard(benchmark::State& state)                      \
    {                                                                   \
        xtr::logger log{                                                \
            [](const char*, std::size_t size)                           \
            {                                                           \
                return size;                                            \
            },                                                          \
            [](const char*, std::size_t)                                \
            {                                                           \
            }};                                                         \
                                                                        \
        set_thread_attrs(::pthread_self(), 4);                          \
        set_thread_attrs(log.consumer_thread_native_handle(), 5);       \
                                                                        \
        xtr::sink p = log.get_sink("Name");                             \
        std::size_t n = 0;                                              \
        constexpr std::size_t sync_every = (64 * 1024) / (MSGSIZE);     \
        for (auto _ : state)                                            \
        {                                                               \
            asm volatile("# LLVM-MCA-BEGIN " #NAME);                    \
            X;                                                          \
            asm volatile("# LLVM-MCA-END");                             \
            if (++n % sync_every == 0)                                  \
            {                                                           \
                state.PauseTiming();                                    \
                p.sync();                                               \
                state.ResumeTiming();                                   \
            }                                                           \
        }                                                               \
    }                                                                   \
    BENCHMARK(NAME ## _discard);

// The logger has a 64kb ring buffer, msgsize is to ensure that
// the test isn't bottlenecked on I/O to the log file.
#define LOG_BENCH_DEV_NULL(NAME, X, MSGSIZE)                            \
    void NAME ## _devnull(benchmark::State& state)                      \
    {                                                                   \
        FILE* fp = ::fopen("/dev/null", "w");                           \
        xtr::logger log{fp, fp};                                        \
                                                                        \
        set_thread_attrs(::pthread_self(), 4);                          \
        set_thread_attrs(log.consumer_thread_native_handle(), 5);       \
                                                                        \
        xtr::sink p = log.get_sink("Name");                             \
        std::size_t n = 0;                                              \
        constexpr std::size_t sync_every = (64 * 1024) / (MSGSIZE);     \
        for (auto _ : state)                                            \
        {                                                               \
            X;                                                          \
            if (++n % sync_every == 0)                                  \
            {                                                           \
                state.PauseTiming();                                    \
                p.sync();                                               \
                state.ResumeTiming();                                   \
            }                                                           \
        }                                                               \
    }                                                                   \
    BENCHMARK(NAME ## _devnull);

#define LOG_BENCH(NAME, X, MSGSIZE) \
    LOG_BENCH_DISCARD(NAME, X, MSGSIZE) \
    LOG_BENCH_DEV_NULL(NAME, X, MSGSIZE)

const std::string s{"Hello"};

LOG_BENCH(logger_benchmark, XTR_LOG(p, "Test"), 8)
LOG_BENCH(logger_benchmark_tsc, XTR_LOG_TSC(p, "Test"), 16)
LOG_BENCH(logger_benchmark_clock_realtime_coarse, XTR_LOG_RTC(p, "Test"), 24)
LOG_BENCH(logger_benchmark_int, XTR_LOG(p, "Test {}", 42), 16)
LOG_BENCH(logger_benchmark_long, XTR_LOG(p, "Test {}", 42L), 16)
LOG_BENCH(logger_benchmark_double, XTR_LOG(p, "Test {}", 42.0), 16)
LOG_BENCH(logger_benchmark_c_str, XTR_LOG(p, "Test {}", "Hello"), 32)
LOG_BENCH(logger_benchmark_str_view, XTR_LOG(p, "Test {}", std::string_view{"Hello"}), 32)
LOG_BENCH(logger_benchmark_str, XTR_LOG(p, "Test {}", s), 32)
LOG_BENCH(logger_benchmark_non_blocking, XTR_TRY_LOG(p, "Test"), 8)
