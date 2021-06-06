# xtr

[![Ubuntu-GCC](https://github.com/choll/xtr/workflows/Ubuntu-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC)
[![Ubuntu-GCC-Conan](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Conan/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Conan)
[![Ubuntu-GCC-Sanitizer](https://github.com/choll/xtr/workflows/Ubuntu-GCC-Sanitizer/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AUbuntu-GCC-Sanitizer)
[![MacOS-GCC](https://github.com/choll/xtr/workflows/MacOS-GCC/badge.svg)](https://github.com/choll/xtr/actions?query=workflow%3AMacOS-GCC)

[![codecov](https://codecov.io/gh/choll/xtr/branch/master/graph/badge.svg?token=FDdI0ZM5tv)](https://codecov.io/gh/choll/xtr)

## What is it?

An asynchronous logger designed to minimise the cost of log statements by delegating as much work as possible to a worker thread.

## Status

Work in progress

## Features

* No allocations when logging, even when logging strings.
* Safe: No references taken to arguments unless explicitly requested.
* Log sinks with independent log levels (so that levels for different subsystems may be modified independently).
* Ability to modify log levels via an external command.
* Non-printable characters are sanitized for safety.
* Formatting done via fmtlib.
* Support for custom back-ends.

## Supported platforms

* Linux (x86-64)
* FreeBSD (x86-64)
* MacOS (x86-64)

## Documentation

## Benchmarks

## Installation notes

See [INSTALL.md](INSTALL.md)

