# xtr

[![Ubuntu-GCC](https://github.com/choll/xtr/workflows/Ubuntu-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC)
[![Ubuntu-GCC-Conan](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Conan)
[![Ubuntu-GCC-Sanitizer](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Sanitizer/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Sanitizer)

[![Ubuntu-Clang](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang.yml)
[![Ubuntu-Clang-Sanitizer](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu_clang_sanitizer.yml)

[![FreeBSD-14-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-14-gcc)
[![FreeBSD-13-GCC](https://api.cirrus-ci.com/github/choll/xtr.svg?task=freebsd-13-gcc)

[![codecov](https://codecov.io/gh/choll/xtr/branch/master/graph/badge.svg?token=FDdI0ZM5tv)](https://codecov.io/gh/choll/xtr)

## What is it?

An asynchronous logger designed to minimise the cost of log statements by delegating as much work as possible to a worker thread.

## Status

:construction: :construction: Under construction! :construction: :construction:

## Features

* Fast (please see benchmark results).
* No allocations when logging, even when logging strings.
* Safe: No references taken to arguments unless explicitly requested.
* Comprehensive suite of unit tests which run cleanly under AddressSanitizer, UndefinedBehaviourSanitizer, ThreadSanitizer and LeakSanitizer.
* Log sinks with independent log levels (so that levels for different subsystems may be modified independently).
* Ability to modify log levels via an external command.
* Non-printable characters are sanitized for safety (to prevent terminal escape sequence injection attacks).
* Formatting done via fmtlib.
* Support for custom I/O back-ends (e.g. to log to the network).
* Support for logrotate integration.

## Supported platforms

* Linux (x86-64)
* FreeBSD (x86-64)

NetBSD and non-x86 coming soon.

## Documentation

Coming soon.

## Benchmarks

Below is the output of `make benchmark` on a stock Ryzen 5950X, with g++ version 10.2.1. No cores are pinned. Benchmarks with pinning, and on other CPUs will be completed soon.

```
Running build/g++-release/benchmark/benchmark
Run on (32 X 6766.8 MHz CPU s)
CPU Caches:
  L1 Data 32K (x16)
  L1 Instruction 32K (x16)
  L2 Unified 512K (x16)
  L3 Unified 32768K (x2)
Load Average: 3.19, 3.19, 2.52
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------------------
Benchmark                                               Time             CPU   Iterations
-----------------------------------------------------------------------------------------
logger_benchmark_discard                             3.61 ns         3.59 ns    192236384
logger_benchmark_devnull                             3.64 ns         3.62 ns    194668568
logger_benchmark_tsc_discard                         11.3 ns         11.3 ns     61844713
logger_benchmark_tsc_devnull                         11.4 ns         11.3 ns     62613091
logger_benchmark_clock_realtime_coarse_discard       10.1 ns         10.0 ns     70203088
logger_benchmark_clock_realtime_coarse_devnull       10.2 ns         10.2 ns     68563014
logger_benchmark_int_discard                         5.14 ns         5.10 ns    137222146
logger_benchmark_int_devnull                         5.26 ns         5.24 ns    133246003
logger_benchmark_long_discard                        5.15 ns         5.11 ns    139830592
logger_benchmark_long_devnull                        5.27 ns         5.24 ns    133512140
logger_benchmark_double_discard                      5.58 ns         5.54 ns    125637225
logger_benchmark_double_devnull                      5.56 ns         5.52 ns    127300064
logger_benchmark_c_str_discard                       7.78 ns         7.72 ns     89508436
logger_benchmark_c_str_devnull                       7.82 ns         7.76 ns     90245907
logger_benchmark_str_view_discard                    6.20 ns         6.14 ns    113756905
logger_benchmark_str_view_devnull                    6.24 ns         6.19 ns    112940946
logger_benchmark_str_discard                         7.55 ns         7.49 ns     94370089
logger_benchmark_str_devnull                         7.73 ns         7.66 ns     92750543
logger_benchmark_non_blocking_discard                3.50 ns         3.47 ns    199295670
logger_benchmark_non_blocking_devnull                3.53 ns         3.51 ns    200382652
```
## Installation notes

See [INSTALL.md](INSTALL.md)

