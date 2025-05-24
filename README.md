# xtr

[![Ubuntu-GCC](https://github.com/choll/xtr/workflows/Ubuntu-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC)
[![Ubuntu-GCC-Conan](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Conan)
[![Ubuntu-GCC-CMake](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml)
[![Ubuntu-GCC-Sanitizer](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Sanitizer/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Sanitizer)

[![Ubuntu-Clang](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml)
[![Ubuntu-Clang-Conan](https://github.com/choll/xtr/workflows/Ubuntu-Clang-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-Clang-Conan)
[![Ubuntu-Clang-CMake](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_cmake.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_cmake.yml)
[![Ubuntu-Clang-Sanitizer](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml)

[![FreeBSD-14-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-GCC-Conan](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc-conan)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-GCC-CMake](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc-cmake)](https://cirrus-ci.com/github/choll/xtr)

[![FreeBSD-14-Clang](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-clang)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-Clang-Conan](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-clang-conan)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-Clang-CMake](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-clang-cmake)](https://cirrus-ci.com/github/choll/xtr)

[![FreeBSD-13-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-13-gcc)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-13-Clang](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-13-clang)](https://cirrus-ci.com/github/choll/xtr)

[![codecov](https://codecov.io/gh/choll/xtr/branch/master/graph/badge.svg?token=FDdI0ZM5tv)](https://codecov.io/gh/choll/xtr)
[![Documentation](https://github.com/choll/xtr/actions/workflows/docs.yml/badge.svg)](https://choll.github.io/xtr)
[![CodeQL](https://github.com/choll/xtr/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/codeql-analysis.yml)

## What is it?

XTR is a C++ logging library aimed at applications with low-latency or real-time
requirements. The cost of log statements is minimised by delegating as much work
as possible to a background thread.

It is designed so that the cost of a log statement is consistently fast---i.e.
every call is fast, not just the average case. No allocations or system calls
are made when a log statement is made.

## Features

* Fast (please see benchmark results).
* No allocations when logging, even when logging strings.
* Formatting, I/O etc are all delegated to a background thread. Work done at the log statement call-site is minimized---for example a no argument log statement only involves writing a single pointer to a ring buffer.
* Safe: No references taken to arguments unless explicitly requested.
* Comprehensive suite of unit tests which run cleanly under AddressSanitizer, UndefinedBehaviourSanitizer, ThreadSanitizer and LeakSanitizer.
* Log sinks with independent log levels (so that levels for different subsystems may be modified independently).
* Ability to modify log levels via an external command.
* Non-printable characters are sanitized for safety (to prevent terminal escape sequence injection attacks).
* Type-safe---formatting is done via fmtlib.
* io\_uring support.
* Support for custom I/O back-ends (e.g. to log to the network, write compressed files, etc).
* Support for logrotate integration.
* Support for systemd journal integration.
* CMake and [Conan](https://conan.io/center/xtr) integration supported.
* Fully [documented](https://choll.github.io/xtr).

## Supported platforms

* Linux (x86-64)
* FreeBSD (x86-64)

## Documentation

https://choll.github.io/xtr

## Benchmarks

Below is the output of `PRODUCER_CPU=2 CONSUMER_CPU=1 make benchmark_cpu` on a stock Ryzen 5950X with SMT disabled, isolated cores and g++ version 11.2.0

```
sudo cpupower --cpu 2,1 frequency-set --governor performance
Setting cpu: 1
Setting cpu: 2
build/g++-lto-release/benchmark/benchmark
2022-04-26T22:54:43+01:00
Running build/g++-lto-release/benchmark/benchmark
Run on (16 X 5084 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 32768 KiB (x2)
Load Average: 3.32, 2.74, 1.99
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations
---------------------------------------------------------------------------------
logger_benchmark                             2.19 ns         2.19 ns    318972090
logger_benchmark_tsc                         7.81 ns         7.81 ns     89307646
logger_benchmark_clock_realtime_coarse       8.05 ns         8.05 ns     87732373
logger_benchmark_int                         3.31 ns         3.31 ns    210259973
logger_benchmark_long                        3.33 ns         3.33 ns    209935480
logger_benchmark_double                      3.26 ns         3.26 ns    212240698
logger_benchmark_c_str                       4.86 ns         4.86 ns    146577914
logger_benchmark_str_view                    3.12 ns         3.12 ns    224958501
logger_benchmark_str                         4.36 ns         4.36 ns    158750720
logger_benchmark_non_blocking                2.01 ns         2.01 ns    349312408

```

## Throughput

Below is the result of running the 'logger throughput' unit test on a stock
Ryzen 5950X with no core isolation or other tuning, writing to a 4TB 990 PRO
SSD with io\_uring enabled.

The test involves writing a log message with int and double arguments
100'000'000 times. Afterwards `sync()` is called on the sink (which will drain
the sink's queue, wait for all io\_uring requests to complete then call
`fsync(2)`). The timing ends after `sync()` returns.

Timings for fmt::print are included for comparison. Note that the fmt::print
call just prints static data and doesn't do any timestamp reading or
formatting (i.e. real world use would be slower).

| Function       | Messages/s | MiB/s   | Time     |
|----------------|------------|---------|----------|
| XTR\_LOG       | 13532135   | 1330.67 | 7.38982s |
| XTR\_LOGL\_TSC | 9082393    | 893.112 | 11.0103s |
| fmt::print     | 12514662   | 1230.62 | 7.99063s |

## Benchmarks Comparing Against Other loggers

### Integer Benchmark, One Thread

| Library             |     50 |     75 |     90 |     95 |     99 |   99.9 |     Max | Version                |
|---------------------|--------|--------|--------|--------|--------|--------|---------|------------------------|
| xtr                 |   11   |   12   |   12.5 |   13   |   14   |   41   |    55.5 | 2.0.1                  |
| platformlab_nanolog |   11.5 |   11.5 |   12   |   12   |   12.5 |   14.5 |    33   | 85b71b6                |
| quill               |   12   |   13   |   13.5 |   14   |   14.5 |   16   |   530.5 | v2.0.2                 |
| reckless            |   17   |   17   |   17.5 |   17.5 |   19.5 |   21.5 |    38   | v3.0.3                 |
| ms_binlog           |   29.5 |   31   |   32   |   32.5 |   68   |  105.5 |   384   | 2020-04-26-82-g2de2fa0 |
| iyengar_nanolog     |  142   |  158   |  173.5 |  192.5 |  421   |  484   | 41490   | 3ffc74a                |
| spdlog              |  284.5 |  317   |  346   |  363.5 |  399.5 |  501.5 |   572.5 | v1.10.0                |
| g3log               | 2543   | 2637   | 2724   | 2779   | 2879   | 3015   |  3354   | 1.3.4                  |

### Integer Benchmark, Four Threads

| Library             |     50 |     75 |     90 |     95 |     99 |   99.9 |     Max | Version                |
|---------------------|--------|--------|--------|--------|--------|--------|---------|------------------------|
| xtr                 |   11   |   12   |   12.5 |   13.5 |   14.5 |   41.5 |    73   | 2.0.1                  |
| platformlab_nanolog |   11.5 |   12   |   12.5 |   12.5 |   13   |   17   |    33.5 | 85b71b6                |
| quill               |   13   |   14.5 |   15.5 |   16.5 |   18   |   21.5 |    23   | v2.0.2                 |
| reckless            |   18   |   19.5 |   20.5 |   20.5 |   21.5 |   28   |    65   | v3.0.3                 |
| ms_binlog           |   29.5 |   31   |   32   |   33   |   70.5 |  107.5 |   412   | 2020-04-26-82-g2de2fa0 |
| iyengar_nanolog     |  135   |  152.5 |  172   |  184   |  227   |  507.5 | 63290   | 3ffc74a                |
| spdlog              |  280.5 |  314.5 |  342.5 |  361.5 |  403.5 |  540.5 |   669.5 | v1.10.0                |
| g3log               | 2530   | 2624   | 2710   | 2767   | 2878   | 3017   |  7146   | 1.3.4                  |

### String Benchmark, One Thread

| Library             |     50 |     75 |     90 |     95 |     99 |   99.9 |     Max | Version                |
|---------------------|--------|--------|--------|--------|--------|--------|---------|------------------------|
| xtr                 |   12.5 |   13.5 |   14   |   14.5 |   16   |   41.5 |    51   | 2.0.1                  |
| platformlab_nanolog |   13.5 |   14   |   14.5 |   14.5 |   15.5 |   19   |    35.5 | 85b71b6                |
| quill               |   14.5 |   15.5 |   18   |   18.5 |   20   |   21.5 |    22.5 | v2.0.2                 |
| ms_binlog           |   34   |   35.5 |   36.5 |   38   |   75   |  109.5 |   389.5 | 2020-04-26-82-g2de2fa0 |
| reckless            |   44   |   47.5 |   49.5 |   50.5 |   52.5 |   79.5 |   134.5 | v3.0.3                 |
| iyengar_nanolog     |  120   |  150   |  166.5 |  181   |  379.5 |  492.5 | 41680   | 3ffc74a                |
| spdlog              |  255   |  299   |  344   |  373.5 |  484.5 | 1123   |  1602   | v1.10.0                |
| g3log               | 1975   | 2214   | 2340   | 2416   | 2536   | 2693   |  2888   | 1.3.4                  |

### String Benchmark, Four Threads

| Library             |     50 |     75 |     90 |     95 |     99 |   99.9 |    Max | Version                |
|---------------------|--------|--------|--------|--------|--------|--------|--------|------------------------|
| xtr                 |   13   |   13.5 |   14.5 |   15   |   16.5 |   46   |   66   | 2.0.1                  |
| platformlab_nanolog |   13.5 |   14   |   14.5 |   15   |   16.5 |   22   |   39   | 85b71b6                |
| quill               |   14.5 |   15.5 |   17   |   18   |   20   |   21.5 |   23.5 | v2.0.2                 |
| ms_binlog           |   34.5 |   36   |   37.5 |   38.5 |   75.5 |  115.5 |  408.5 | 2020-04-26-82-g2de2fa0 |
| reckless            |   44   |   48   |   50   |   51   |   52.5 |   68.5 |  216.5 | v3.0.3                 |
| iyengar_nanolog     |   96.5 |  132.5 |  161.5 |  174.5 |  229.5 |  487   | 8354   | 3ffc74a                |
| spdlog              |  256   |  301.5 |  349.5 |  379   |  495.5 | 1177   | 2071   | v1.10.0                |
| g3log               | 1911   | 2245   | 2380   | 2451   | 2582   | 2738   | 6578   | 1.3.4                  |

See [logger_benchmarks](https://github.com/choll/logger_benchmarks) for more details.

## Installation notes

See [INSTALL.md](INSTALL.md)
