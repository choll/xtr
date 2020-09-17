## Dependencies

* [libfmt](https://github.com/fmtlib/fmt)
* [Catch2](https://github.com/catchorg/Catch2) (optional, for unit tests)
* [Google Benchmark](https://github.com/google/benchmark) (optional, for benchmarks)
* [Gcovr](https://github.com/gcovr/gcovr) (optional, for local code coverage reports)

---

## Installing using Conan

The easiest way to install is using Conan, run:

```
TODO
```

to search for packages, then install via:

```
TODO
```

---

## Header only includes

TODO

---

## Building

### Satisfying build dependencies with Conan

A `conanfile.txt` is provided for use with Conan. Run `conan install .` to
install all dependencies, then use make to build and install:

```
make
make install
```

The installation directory can be overridden via the `PREFIX` option.

### Satisfying build dependencies with Git submodules

If you do not want to use Conan but want to get started quickly then
the repository submodules can be used to satisfy dependencies:

```
git submodule init
git submodule update
```

Then use make to build and install:

```
make
make install
```

The installation directory can be overridden via the `PREFIX` option.

Note that if submodules are used only libfmt and Catch2 are provided, and libfmt is used in header-only mode.

### Satisfying build dependencies manually

See the list of makefile path options [below](#Makefile paths).

### Makefile options

Name                       | Description                    | Default value
---------------------------|--------------------------------|--------------
`PREFIX`                   | Installation prefix            | `/usr/local`
`EXCEPTIONS`               | Set to 0 to disable exceptions | 1
`SANITIZER`                | Set to a sanitizer such as 'address', 'undefined', 'thread', 'leak' | None
`COVERAGE`                 | Set to 1 to build with code coverage instrumentation | 0
`PIC`                      | Set to 1 to build with -fPIC (position independent code, may be required if you want to link the produced static library in your own shared object) | 0
`RTTI`                     | Set to 1 to build with RTTI (run-time type information) | 0

### Makefile targets

Name                     | Description         | Required dependencies
-------------------------|---------------------|------------------
`all` / default          | Build library       | libfmt
`check`                  | Build and run tests | libfmt, Catch2
`benchmark`              | Build and run benchmarks | libfmt, Google Benchmark
`coverage_report`        | Create a local code coverage report | libfmt, Catch2, gcovr
`install`                | Install the library and headers under `PREFIX` |

### Makefile paths

The following makefile options can be used to set the include and library
search paths for build dependencies if they have been installed to non-standard
locations (if you are using Conan or submodules then you don't need to use
these).

Name                       | Description | Required by targets
---------------------------|-------------|--------------------
`FMT_INCLUDE_DIR`          | Path to the libfmt include directory | All targets
`FMT_LIB_DIR`              | Path to the libfmt lib directory | All targets
`CATCH2_INCLUDE_DIR`       | Path to the Catch2 include directory | `test`, `run_test`, `coverage_html`
`GOOGLE_BENCH_INCLUDE_DIR` | Path to the Google Benchmark include directory | `benchmark`, `run_benchmark`
`GOOGLE_BENCH_LIB_DIR`     | Path to the Google Benchmark lib directory | `benchmark`, `run_benchmark`

