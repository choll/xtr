# xtr

[![Ubuntu-GCC](https://github.com/choll/xtr/workflows/Ubuntu-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC)
[![Ubuntu-GCC-Conan](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Conan)
[![Ubuntu-GCC-Sanitizer](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Sanitizer/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Sanitizer)
[![Ubuntu-GCC-CMake](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_gcc_cmake.yml)

[![Ubuntu-Clang](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml)
[![Ubuntu-Clang-Sanitizer](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml)

[![FreeBSD-13-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-13-gcc)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc)](https://cirrus-ci.com/github/choll/xtr)

[![FreeBSD-13-Clang](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-13-clang)](https://cirrus-ci.com/github/choll/xtr)
[![FreeBSD-14-Clang](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-clang)](https://cirrus-ci.com/github/choll/xtr)

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
* Safe: No references taken to arguments unless explicitly requested.
* Comprehensive suite of unit tests which run cleanly under AddressSanitizer, UndefinedBehaviourSanitizer, ThreadSanitizer and LeakSanitizer.
* Log sinks with independent log levels (so that levels for different subsystems may be modified independently).
* Ability to modify log levels via an external command.
* Non-printable characters are sanitized for safety (to prevent terminal escape sequence injection attacks).
* Formatting done via fmtlib.
* Support for custom I/O back-ends (e.g. to log to the network or syslogd).
* Support for logrotate integration.
* Support for systemd journal integration.

## Supported platforms

* Linux (x86-64)
* FreeBSD (x86-64)

## Documentation

https://choll.github.io/xtr

## Benchmarks

Below is the output of `make benchmark` on a stock Ryzen 5950X, with g++ version 10.2.1. No cores are isolated. Benchmarks with isolation, and on other CPUs will be completed soon.

```
2021-07-31 22:30:11
Running build/g++-release/benchmark/benchmark
Run on (32 X 6614.06 MHz CPU s)
CPU Caches:
  L1 Data 32K (x16)
  L1 Instruction 32K (x16)
  L2 Unified 512K (x16)
  L3 Unified 32768K (x2)
Load Average: 1.26, 1.53, 1.42
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------------------
Benchmark                                               Time             CPU   Iterations
-----------------------------------------------------------------------------------------
logger_benchmark_discard                             3.12 ns         3.11 ns    220642542
logger_benchmark_devnull                             3.17 ns         3.16 ns    213233572
logger_benchmark_tsc_discard                         9.97 ns         9.95 ns     71544694
logger_benchmark_tsc_devnull                         9.81 ns         9.79 ns     72024706
logger_benchmark_clock_realtime_coarse_discard       9.50 ns         9.48 ns     75301078
logger_benchmark_clock_realtime_coarse_devnull       9.02 ns         8.99 ns     77431937
logger_benchmark_int_discard                         4.64 ns         4.63 ns    151861070
logger_benchmark_int_devnull                         4.62 ns         4.61 ns    151949835
logger_benchmark_long_discard                        4.61 ns         4.60 ns    151969132
logger_benchmark_long_devnull                        4.61 ns         4.60 ns    152198709
logger_benchmark_double_discard                      4.84 ns         4.82 ns    145809754
logger_benchmark_double_devnull                      4.81 ns         4.80 ns    145367066
logger_benchmark_c_str_discard                       7.63 ns         7.60 ns     93103583
logger_benchmark_c_str_devnull                       7.33 ns         7.30 ns     95755773
logger_benchmark_str_view_discard                    5.69 ns         5.66 ns    122769764
logger_benchmark_str_view_devnull                    5.67 ns         5.64 ns    124657115
logger_benchmark_str_discard                         7.36 ns         7.33 ns     96002018
logger_benchmark_str_devnull                         7.32 ns         7.29 ns     96072575
logger_benchmark_non_blocking_discard                3.13 ns         3.12 ns    224529741
logger_benchmark_non_blocking_devnull                3.09 ns         3.08 ns    228207878
```
## Installation notes

See [INSTALL.md](INSTALL.md)

