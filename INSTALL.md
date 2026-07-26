## Dependencies

* [libfmt](https://github.com/fmtlib/fmt)
* [liburing](https://github.com/axboe/liburing) (optional, for the io\_uring back-end)
* [Catch2](https://github.com/catchorg/Catch2) (optional, for unit tests)
* [Google Benchmark](https://github.com/google/benchmark) (optional, for benchmarks)
* [Gcovr](https://github.com/gcovr/gcovr) (optional, for local code coverage reports)

Python and pkg-config are required to build, as dependencies are installed
via [Conan](https://conan.io) and located via pkg-config.

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

Dependencies are installed via Conan, which the makefile installs into a
virtualenv under `build/`, so only Python and pkg-config are required. A Conan
profile is needed before the first build:

```
make conan-profile
make
make install
```

The installation directory can be overridden via the `PREFIX` option.

### Satisfying build dependencies manually

Dependencies are located via pkg-config, so to build against libraries already
installed on the system, use the system pkg-config and disable the Conan
generated files:

```
make PKG_CONFIG=pkg-config PKG_CONFIG_FILES=
```

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
`RELDEBUG`   | Set to 1 to produce a release-debug build (optimized with debug symbols) | 0
`URING`      | Set to 1 or 0 to force the io\_uring back-end on or off | `auto`
`PKG_CONFIG` | pkg-config command used to locate dependencies | `PKG_CONFIG_PATH=build/pkgconfig pkg-config`

### Makefile targets

Name                     | Description         | Required dependencies
-------------------------|---------------------|------------------
default                  | Build library       | libfmt
`all`                    | Build the library, tests, benchmarks, xtrctl and the single include header | libfmt, Catch2, Google Benchmark
`check`                  | Build and run tests | libfmt, Catch2
`benchmark`              | Build and run benchmarks | libfmt, Google Benchmark
`coverage_report`        | Create a local code coverage report | libfmt, Catch2, gcovr
`conan-profile`          | Create a Conan profile, if none exists |
`install`                | Install the library under `PREFIX` |
`clean`                  | Remove build output for the current configuration |
`distclean`              | Remove `build`, including the Conan virtualenv |

### Building with CMake

You may also use CMake for building. All dependencies can be solved by Conan:

```
mkdir -p build/cmake && cd build/cmake
conan install ../.. -g CMakeDeps -g CMakeToolchain --output-folder=${PWD} --build missing
cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DBUILD_TESTING=ON
cmake --build .
cmake --build . --target test
cmake --build . --target install
```

The Conan client will install all dependencies listed in `conanfile.py` and generate the config files that CMake's `find_package` will load.
By default, CMake will build all project on `Debug` mode, but you can set `Release` instead.
Also, `BUILD_TESTING` by default is disabled, without this option, the project will not build testing.
The target install folder can be customized by `CMAKE_INSTALL_PREFIX` definition
