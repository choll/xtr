# xtr

[![Ubuntu](https://github.com/choll/xtr/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu.yml)
[![FreeBSD](https://github.com/choll/xtr/actions/workflows/freebsd.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/freebsd.yml)

[![codecov](https://codecov.io/gh/choll/xtr/branch/master/graph/badge.svg?token=FDdI0ZM5tv)](https://codecov.io/gh/choll/xtr)
[![Documentation](https://github.com/choll/xtr/actions/workflows/docs.yml/badge.svg)](https://choll.github.io/xtr)

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
* Support for logging variable-length objects, such as structs with flexible array members.
* Formatting, I/O etc are all delegated to a background thread. Work done at the log statement call-site is minimised---for example a no-argument log statement only involves writing a single pointer to a ring buffer.
* Optional background thread. Users may run the log consumer from a thread of their choosing.
* Safe: No references taken to arguments unless explicitly requested.
* Comprehensive suite of unit tests which run cleanly under AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer and LeakSanitizer.
* Log sinks with independent log levels (so that levels for different subsystems may be modified independently).
* Ability to modify log levels via an external command.
* Non-printable characters are sanitised for safety (to prevent terminal escape sequence injection attacks).
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

Below is the output of `PRODUCER_CPU=2 CONSUMER_CPU=1 make benchmark_cpu` on a stock Ryzen 5950X with SMT disabled, isolated cores and g++ version 15.3.0.

```
Setting cpu: 1
Setting cpu: 2
2026-09-15T20:24:55+01:00
Running build/g++-lto-release/benchmark/benchmark
Run on (16 X 5086.18 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 32768 KiB (x2)
Load Average: 1.48, 1.66, 1.65
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations
---------------------------------------------------------------------------------
logger_benchmark                            0.904 ns        0.903 ns    773992268
logger_benchmark_int                         1.27 ns         1.27 ns    548924685
logger_benchmark_long                        1.29 ns         1.29 ns    538890624
logger_benchmark_double                      1.31 ns         1.30 ns    547294711
logger_benchmark_c_str_8                     5.43 ns         5.42 ns    129716915
logger_benchmark_c_str_16                    5.60 ns         5.59 ns    125096979
logger_benchmark_c_str_32                    6.21 ns         6.20 ns    113414754
logger_benchmark_c_str_64                    6.52 ns         6.51 ns    105804370
logger_benchmark_c_str_128                   8.31 ns         8.29 ns     84816936
logger_benchmark_str_view_8                  3.53 ns         3.52 ns    199019338
logger_benchmark_str_view_16                 3.88 ns         3.87 ns    180409818
logger_benchmark_str_view_32                 3.68 ns         3.67 ns    191723379
logger_benchmark_str_view_64                 4.29 ns         4.28 ns    168197415
logger_benchmark_str_view_128                6.87 ns         6.85 ns    102144441
logger_benchmark_str_view_rand               10.8 ns         10.8 ns     64888650
logger_benchmark_str_view_const_8            2.07 ns         2.06 ns    337372835
logger_benchmark_str_view_const_16           2.30 ns         2.29 ns    311140169
logger_benchmark_str_view_const_32           2.72 ns         2.71 ns    258408553
logger_benchmark_str_view_const_64           3.57 ns         3.56 ns    194648218
logger_benchmark_str_view_const_128          6.03 ns         6.01 ns    117439753
logger_benchmark_str_8                       3.78 ns         3.77 ns    185626506
logger_benchmark_str_16                      3.89 ns         3.88 ns    180322628
logger_benchmark_str_32                      3.76 ns         3.76 ns    187745449
logger_benchmark_str_64                      4.19 ns         4.18 ns    167743865
logger_benchmark_str_128                     6.98 ns         6.96 ns    104261936
logger_benchmark_vcopy_64                    5.11 ns         5.10 ns    142833259
logger_benchmark_vcopy_128                   6.70 ns         6.68 ns    104984332
logger_benchmark_vcopy_256                   10.2 ns         10.2 ns     68636200
logger_benchmark_tsc                         8.65 ns         8.64 ns     81015530
logger_benchmark_tsc_int                     9.79 ns         9.78 ns     71521405
logger_benchmark_tsc_long                    9.70 ns         9.69 ns     72265285
logger_benchmark_tsc_double                  9.82 ns         9.81 ns     71358789
logger_benchmark_tsc_c_str_8                 9.86 ns         9.86 ns     70889821
logger_benchmark_tsc_c_str_16                9.77 ns         9.76 ns     71902812
logger_benchmark_tsc_c_str_32                10.6 ns         10.6 ns     65905563
logger_benchmark_tsc_c_str_64                11.1 ns         11.0 ns     63407079
logger_benchmark_tsc_c_str_128               12.9 ns         12.9 ns     54219854
logger_benchmark_tsc_str_view_8              9.72 ns         9.71 ns     72264341
logger_benchmark_tsc_str_view_16             9.74 ns         9.73 ns     72019586
logger_benchmark_tsc_str_view_32             10.4 ns         10.4 ns     67444695
logger_benchmark_tsc_str_view_64             10.3 ns         10.3 ns     68326353
logger_benchmark_tsc_str_view_128            11.0 ns         11.0 ns     63804455
logger_benchmark_tsc_str_view_rand           14.7 ns         14.7 ns     47704403
logger_benchmark_tsc_str_8                   9.77 ns         9.77 ns     71718915
logger_benchmark_tsc_str_16                  9.50 ns         9.50 ns     73404482
logger_benchmark_tsc_str_32                  10.3 ns         10.3 ns     68101763
logger_benchmark_tsc_str_64                  10.5 ns         10.4 ns     67131809
logger_benchmark_tsc_str_128                 11.0 ns         11.0 ns     63960251
logger_benchmark_tsc_vcopy_64                10.1 ns         10.1 ns     69389378
logger_benchmark_tsc_vcopy_128               11.3 ns         11.3 ns     61829343
logger_benchmark_tsc_vcopy_256               13.8 ns         13.8 ns     50756600
logger_benchmark_clock_realtime_coarse       6.72 ns         6.71 ns    104480882
logger_benchmark_non_blocking               0.897 ns        0.896 ns    783524653
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
| XTR\_LOG       | 14856334   | 1460.89 | 6.73114s |
| XTR\_LOGL\_TSC | 7713411    | 758.494 | 12.9644s |
| fmt::print     | 14545790   | 1430.35 | 6.87484s |

## Installation notes

See [INSTALL.md](INSTALL.md)
