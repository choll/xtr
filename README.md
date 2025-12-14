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

## Installation notes

See [INSTALL.md](INSTALL.md)
