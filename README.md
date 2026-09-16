# xtr

[![Ubuntu](https://github.com/choll/xtr/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/ubuntu.yml)
[![FreeBSD](https://github.com/choll/xtr/actions/workflows/freebsd.yml/badge.svg)](https://github.com/choll/xtr/actions/workflows/freebsd.yml)

[![codecov](https://codecov.io/gh/choll/xtr/branch/master/graph/badge.svg?token=FDdI0ZM5tv)](https://codecov.io/gh/choll/xtr)
[![Documentation](https://github.com/choll/xtr/actions/workflows/docs.yml/badge.svg)](https://choll.github.io/xtr)

## What is it?

XTR is a C++ logging library aimed at applications with low-latency or real-time
requirements.

It is designed so that the cost of a log statement is consistently fast---i.e.
every call is fast, not just the average case. No allocations or system calls
are made when a log statement is made.

## Design

The cost of log statements is minimised by delegating as much work as possible to a
background thread. This is done by writing log records to queues that are read by
the background thread. Log records contain a function pointer which is invoked by the
background thread to perform formatting.

With formatting delegated, the remaining costs are writing the record to a queue and
reading the timestamp. XTR makes two departures from traditional logger design (global
logger with either thread-local queues or an MPSC queue) to minimise these costs:

* XTR is designed around the idea of application components writing to their own
sink object that contains an SPSC queue. An application creates many sinks which connect
to a single logger object. As sinks are per-component, no thread-local storage is
required; objects instead have a sink member that is written to without any thread-local
access overhead or contention on a shared queue. This has the added benefit of allowing
log levels to be controlled per component, which can be done from outside the process
while it is running, via the supplied [xtrctl](https://choll.github.io/xtr/xtrctl.html)
tool.
* XTR gives users the choice of where timestamps are taken, in either the producer
or consumer thread. This is done because the cost of reading the timestamp is high
relative to the overall cost of writing to the sink---see the `logger_benchmark` vs
`logger_benchmark_tsc` timings in [benchmarks](#benchmarks).

### Example

```c++
xtr::logger log;
xtr::sink s = log.get_sink("Example");
XTR_LOG(s, "Hello world");
```

The log statement above [compiles](https://godbolt.org/z/bMdof3Ej8) to 11 instructions
(excluding ret) on the fast path with a 64KB queue:

```nasm
f:
    mov rax, [rdi+80]
    mov rcx, [rdi+72]
    movzx edx, ax
    sub rcx, rax
    add rdx, [rdi+64]
    cmp rcx, 7
    jbe .queue_full
.write:
    add rax, 8
    mov qword [rdx], func_ptr
    mov [rdi+80], rax
    mov [rdi], rax
    ret
.queue_full:
    pause
    mov rcx, [rdi+128]
    mov [rdi+72], rcx
    mov rax, [rdi+80]
    sub rcx, rax
    cmp rcx, 7
    ja .write
    jmp .queue_full
```

`func_ptr` is the log record itself---specifically it is a function pointer to an instantiation
of a per-log-record template function that embeds the format string, log level, line number
and source file name. Note that only log statements with no arguments produce a single function
pointer. Refer to the comments in
[trampolines.hpp](https://github.com/choll/xtr/blob/master/include/xtr/detail/trampolines.hpp)
for details.

If the queue is full then the `.queue_full` loop spins until space becomes available. To drop
messages on a full queue use [XTR_TRY_LOG](https://choll.github.io/xtr/api.html#c.XTR_TRY_LOG).

## Features

* Fast (please see [benchmark results](#benchmarks)).
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
2026-09-15T21:10:37+01:00
Running build/g++-lto-release/benchmark/benchmark
Run on (16 X 5086.18 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 32768 KiB (x2)
Load Average: 1.83, 1.74, 1.65
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations
---------------------------------------------------------------------------------
logger_benchmark                            0.894 ns        0.893 ns    781501162
logger_benchmark_int                         1.25 ns         1.25 ns    554495079
logger_benchmark_long                        1.29 ns         1.29 ns    545524626
logger_benchmark_double                      1.28 ns         1.28 ns    549515193
logger_benchmark_c_str_8                     3.45 ns         3.44 ns    204685028
logger_benchmark_c_str_16                    4.00 ns         3.99 ns    177459523
logger_benchmark_c_str_32                    4.34 ns         4.33 ns    157937285
logger_benchmark_c_str_64                    5.15 ns         5.14 ns    136966507
logger_benchmark_c_str_128                   8.83 ns         8.81 ns     78644584
logger_benchmark_str_view_8                  2.33 ns         2.33 ns    296690133
logger_benchmark_str_view_16                 2.69 ns         2.68 ns    259885911
logger_benchmark_str_view_32                 2.98 ns         2.97 ns    239187548
logger_benchmark_str_view_64                 3.70 ns         3.69 ns    190443461
logger_benchmark_str_view_128                7.09 ns         7.07 ns     97809794
logger_benchmark_str_view_rand               7.59 ns         7.58 ns     92095459
logger_benchmark_str_view_const_8            2.09 ns         2.08 ns    338936832
logger_benchmark_str_view_const_16           2.26 ns         2.25 ns    314252338
logger_benchmark_str_view_const_32           2.71 ns         2.70 ns    260566377
logger_benchmark_str_view_const_64           3.56 ns         3.55 ns    194553614
logger_benchmark_str_view_const_128          5.95 ns         5.94 ns    118191888
logger_benchmark_str_8                       2.32 ns         2.31 ns    304413393
logger_benchmark_str_16                      2.59 ns         2.59 ns    269515101
logger_benchmark_str_32                      2.95 ns         2.94 ns    234145125
logger_benchmark_str_64                      3.71 ns         3.70 ns    190204809
logger_benchmark_str_128                     7.14 ns         7.12 ns     99385993
logger_benchmark_vcopy_64                    5.04 ns         5.03 ns    100000000
logger_benchmark_vcopy_128                   6.64 ns         6.63 ns    104604936
logger_benchmark_vcopy_256                   10.2 ns         10.2 ns     67877457
logger_benchmark_tsc                         8.65 ns         8.65 ns     80862546
logger_benchmark_tsc_int                     9.79 ns         9.79 ns     71512031
logger_benchmark_tsc_long                    9.69 ns         9.69 ns     72232093
logger_benchmark_tsc_double                  9.78 ns         9.78 ns     71623464
logger_benchmark_tsc_c_str_8                 9.71 ns         9.70 ns     72258012
logger_benchmark_tsc_c_str_16                9.62 ns         9.61 ns     73139638
logger_benchmark_tsc_c_str_32                10.3 ns         10.3 ns     67870667
logger_benchmark_tsc_c_str_64                11.0 ns         11.0 ns     63613603
logger_benchmark_tsc_c_str_128               13.4 ns         13.4 ns     52503523
logger_benchmark_tsc_str_view_8              9.59 ns         9.58 ns     73071424
logger_benchmark_tsc_str_view_16             9.47 ns         9.47 ns     74063551
logger_benchmark_tsc_str_view_32             9.90 ns         9.89 ns     70940814
logger_benchmark_tsc_str_view_64             10.1 ns         10.1 ns     69144818
logger_benchmark_tsc_str_view_128            10.8 ns         10.8 ns     65078043
logger_benchmark_tsc_str_view_rand           10.4 ns         10.4 ns     67348393
logger_benchmark_tsc_str_8                   9.51 ns         9.51 ns     73742106
logger_benchmark_tsc_str_16                  9.41 ns         9.40 ns     74540626
logger_benchmark_tsc_str_32                  9.83 ns         9.82 ns     71359175
logger_benchmark_tsc_str_64                  10.0 ns         9.99 ns     70081099
logger_benchmark_tsc_str_128                 11.0 ns         11.0 ns     63639608
logger_benchmark_tsc_vcopy_64                10.1 ns         10.1 ns     69282902
logger_benchmark_tsc_vcopy_128               11.4 ns         11.4 ns     62304733
logger_benchmark_tsc_vcopy_256               13.9 ns         13.9 ns     50714167
logger_benchmark_clock_realtime_coarse       6.72 ns         6.72 ns    104293571
logger_benchmark_non_blocking               0.894 ns        0.893 ns    782899524
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
