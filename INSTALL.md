## Dependencies

* [libfmt](https://github.com/fmtlib/fmt)
* [Catch2](https://github.com/catchorg/Catch2) (optional, for unit tests)
* [Google Benchmark](https://github.com/google/benchmark) (optional, for benchmarks)
* [Gcovr](https://github.com/gcovr/gcovr) (optional, for local code coverage reports)

---

## Installing using Conan

1. If using the Conan Center [package](https://conan.io/center/xtr) then skip to the
next step on editing your conanfile, otherwise create a package in your local cache via `conan create .`
2. Add a requirement for 'xtr' in your conanfile and run `conan install .` (refer to the Conan
   [cheat sheet](https://docs.conan.io/en/latest/cheatsheet.html#using-packages-in-an-application)
   for a quick guide to setting up a conanfile and installing dependencies).

The Conan package also includes man pages which can be accessed by using the virtualenv generator.

---

## Header only includes

See `single_include/xtr/logger.hpp`

---

## Building

### Satisfying build dependencies with Conan

A `conanfile.py` is provided for use with Conan. Run `conan install .` to
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

See the list of makefile path options [below](#makefile-paths).

### Makefile options

Name         | Description                    | Default value
-------------|--------------------------------|--------------
`PREFIX`     | Installation prefix            | `/usr/local`
`EXCEPTIONS` | Set to 0 to disable exceptions | 1
`COVERAGE`   | Set to 1 to build with code coverage instrumentation | 0
`SANITIZER`  | Set to a sanitizer such as 'address', 'undefined', 'thread', 'leak' | None
`PIC`        | Set to 1 to build with -fPIC (position independent code, may be required if you want to link the produced static library in your own shared object) | 0
`LTO`        | Set to 1 to build with LTO (link-time optimization) | 1
`DEBUG`      | Set to 1 to produce a debug build | 0
`RELDEBUG`   | Set to 1 to produce a release-debug build (optimized with debug symbols) | 1

### Makefile targets

Name                     | Description         | Required dependencies
-------------------------|---------------------|------------------
`all` / default          | Build library       | libfmt
`check`                  | Build and run tests | libfmt, Catch2
`benchmark`              | Build and run benchmarks | libfmt, Google Benchmark
`coverage_report`        | Create a local code coverage report | libfmt, Catch2, gcovr
`install`                | Install the library under `PREFIX` |

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
`LIBURING_INCLUDE_DIR` | Path to the liburing include directory | All targets, optional
`LIBURING_LIB_DIR`     | Path to the liburing lib directory | All targets, optional

### Building with CMake

Besides Makefile, you can also use CMake for building. All dependencies can be solved by Conan:

```
mkdir build && cd build/
conan install .. -g cmake_find_package
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MODULE_PATH=${PWD} -DBUILD_TESTING=ON
cmake --build .
cmake --build . --target test
cmake --build . --target install
```

The Conan client will install all dependencies listed in `conanfile.py` and generate Findxxx.cmake files, which will be loaded by CMake.
By default, CMake will build all project on `Debug` mode, but you can set `Release` instead.
The `CMAKE_MODULE_PATH` is required to locale the find cmake file generated by Conan.
Also, `BUILD_TESTING` by default is disabled, without this option, the project will not build testing.
The target install folder can be customized by `CMAKE_INSTALL_PREFIX` definition
