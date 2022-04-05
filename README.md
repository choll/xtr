# xtr

[![Ubuntu-GCC](https://github.com/choll/xtr/workflows/Ubuntu-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC)
[![Ubuntu-GCC-Conan](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Conan)
[![Ubuntu-GCC-CMake](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml)
[![Ubuntu-GCC-Sanitizer](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Sanitizer/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Sanitizer)

[![Ubuntu-Clang](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml)
[![Ubuntu-Clang-Conan](https://github.com/choll/xtr/workflows/Ubuntu-Clang-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-Clang-Conan)
[![Ubuntu-Clang-CMake](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_cmake.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_cmake.yml)
[![Ubuntu-Clang-Sanitizer](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml)

![FreeBSD-14-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc)](https://cirrus-ci.com/github/choll/xtr)
![FreeBSD-14-GCC-Conan](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc-conan)](https://cirrus-ci.com/github/choll/xtr)
![FreeBSD-14-GCC-CMake](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc-cmake)](https://cirrus-ci.com/github/choll/xtr)

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
* Formatting done via fmtlib.
* Support for custom I/O back-ends (e.g. to log to the network).
* Support for logrotate integration.
* Support for systemd journal integration.

## Supported platforms

* Linux (x86-64)
* FreeBSD (x86-64)

## Documentation

https://choll.github.io/xtr

## Benchmarks

Below is the output of `PRODUCER_CPU=2 CONSUMER_CPU=1 make benchmark_cpu` on a stock Ryzen 5950X, with g++ version 11.2.0

```
sudo cpupower --cpu 2,1 frequency-set --governor performance
Setting cpu: 1
Setting cpu: 2
build/g++-lto-release/benchmark/benchmark
2022-03-20T00:49:10+00:00
Running build/g++-lto-release/benchmark/benchmark
Run on (32 X 5083.4 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 32768 KiB (x2)
Load Average: 2.00, 1.83, 1.62
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations
---------------------------------------------------------------------------------
logger_benchmark                             2.36 ns         2.35 ns    311731007
logger_benchmark_tsc                         9.57 ns         9.55 ns     73260008
logger_benchmark_clock_realtime_coarse       8.95 ns         8.92 ns     78251929
logger_benchmark_int                         3.91 ns         3.90 ns    179545113
logger_benchmark_long                        3.94 ns         3.92 ns    176191487
logger_benchmark_double                      3.51 ns         3.49 ns    184301147
logger_benchmark_c_str                       6.27 ns         6.24 ns    108297092
logger_benchmark_str_view                    3.80 ns         3.77 ns    186521782
logger_benchmark_str                         4.81 ns         4.78 ns    139163865
logger_benchmark_non_blocking                2.33 ns         2.32 ns    299429176
```

## Installation notes

See [INSTALL.md](INSTALL.md)
