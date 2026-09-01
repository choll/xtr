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
relative to the overall cost of writing to the sink.

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
2026-08-01T17:24:09+01:00
Running build/g++-lto-release/benchmark/benchmark
Run on (16 X 5086.18 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 512 KiB (x16)
  L3 Unified 32768 KiB (x2)
Load Average: 1.49, 1.30, 1.12
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations
---------------------------------------------------------------------------------
logger_benchmark                            0.892 ns        0.892 ns    729236263
logger_benchmark_int                         1.25 ns         1.25 ns    546887959
logger_benchmark_long                        1.29 ns         1.29 ns    538424449
logger_benchmark_double                      1.30 ns         1.30 ns    544161657
logger_benchmark_c_str_8                     5.91 ns         5.91 ns    118986321
logger_benchmark_c_str_16                    5.95 ns         5.94 ns    118211133
logger_benchmark_c_str_32                    6.20 ns         6.19 ns    113695149
logger_benchmark_c_str_64                    6.55 ns         6.53 ns    107151690
logger_benchmark_str_view_8                  3.53 ns         3.53 ns    198737333
logger_benchmark_str_view_16                 3.66 ns         3.65 ns    192806602
logger_benchmark_str_view_32                 3.83 ns         3.82 ns    185727265
logger_benchmark_str_view_64                 4.26 ns         4.24 ns    165482328
logger_benchmark_str_8                       3.79 ns         3.79 ns    184713006
logger_benchmark_str_16                      3.89 ns         3.88 ns    180855799
logger_benchmark_str_32                      3.77 ns         3.76 ns    185398315
logger_benchmark_str_64                      4.19 ns         4.17 ns    163856703
logger_benchmark_vcopy_64                    5.51 ns         5.49 ns    129211425
logger_benchmark_vcopy_128                   7.51 ns         7.49 ns     95641328
logger_benchmark_vcopy_256                   11.5 ns         11.5 ns     60935115
logger_benchmark_tsc                         9.52 ns         9.52 ns     73570265
logger_benchmark_tsc_int                     9.88 ns         9.88 ns     70904838
logger_benchmark_tsc_long                    9.77 ns         9.77 ns     71655166
logger_benchmark_tsc_double                  9.78 ns         9.77 ns     71643407
logger_benchmark_tsc_c_str_8                 10.3 ns         10.3 ns     67998753
logger_benchmark_tsc_c_str_16                10.2 ns         10.2 ns     68780399
logger_benchmark_tsc_c_str_32                10.6 ns         10.6 ns     66282842
logger_benchmark_tsc_c_str_64                11.1 ns         11.1 ns     63067116
logger_benchmark_tsc_str_view_8              10.2 ns         10.2 ns     68745944
logger_benchmark_tsc_str_view_16             10.3 ns         10.3 ns     68059516
logger_benchmark_tsc_str_view_32             10.4 ns         10.4 ns     67622660
logger_benchmark_tsc_str_view_64             10.3 ns         10.3 ns     68253218
logger_benchmark_tsc_str_8                   10.1 ns         10.0 ns     69713560
logger_benchmark_tsc_str_16                  10.0 ns         10.0 ns     70089365
logger_benchmark_tsc_str_32                  10.3 ns         10.3 ns     68213933
logger_benchmark_tsc_str_64                  10.3 ns         10.3 ns     68165396
logger_benchmark_tsc_vcopy_64                10.1 ns         10.0 ns     69241914
logger_benchmark_tsc_vcopy_128               11.3 ns         11.3 ns     62303537
logger_benchmark_tsc_vcopy_256               15.0 ns         15.0 ns     46441248
logger_benchmark_clock_realtime_coarse       6.77 ns         6.77 ns    103671157
logger_benchmark_non_blocking               0.905 ns        0.904 ns    778123386
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
