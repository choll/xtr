#include "xtr/logger.hpp"

#include <benchmark/benchmark.h>

#include <cerrno>
#include <cstdio>
#include <cstdint>
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
}

// The logger has a 64kb ring buffer, msgsize is to ensure that
// the test isn't bottlenecked on I/O to the log file.
#define LOG_BENCH(NAME, X, MSGSIZE)                                     \
    void NAME(benchmark::State& state)                                  \
    {                                                                   \
        FILE* fp = ::fopen("/dev/null", "w");                           \
        xtr::logger log{fp};                                            \
                                                                        \
        if (const int cpu = getenv_int("PRODUCER_CPU"); cpu != -1)      \
            set_thread_attrs(::pthread_self(), cpu);                    \
                                                                        \
        if (const int cpu = getenv_int("CONSUMER_CPU"); cpu != -1)      \
            set_thread_attrs(log.consumer_thread_native_handle(), cpu); \
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
    BENCHMARK(NAME);

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
