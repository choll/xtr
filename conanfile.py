from conans import ConanFile, AutoToolsBuildEnvironment, tools
from conans.errors import ConanInvalidConfiguration

import os


class XtrConan(ConanFile):
    requires = "fmt/[>=6.0.0 < 8.0.0 || > 8.0.0]"
    build_requires = "benchmark/1.6.0", "catch2/3.2.0"
    name = "xtr"
    license = "MIT"
    author = "Chris Holloway (c.holloway@gmail.com)"
    url = "https://github.com/choll/xtr"
    description = "A C++ logging library"
    settings = {
        "os": ["Linux", "FreeBSD"],
        "compiler": ["gcc", "clang"],
        "build_type": None,
        "arch": ["x86_64"]}
    options = {
        "fPIC": [True, False],
        "enable_exceptions": [True, False],
        "enable_lto": [True, False]}
    default_options = {
        "fPIC": True,
        "enable_exceptions": True,
        "enable_lto": False}
    generators = "make"
    exports_sources = ["src/*", "include/*", "Makefile"]
    exports = ["docs/libxtr.3", "docs/libxtr-quickstart.3",
               "docs/libxtr-userguide.3", "docs/xtrctl.1", "LICENSE"]

    def configure(self):
        minimal_cpp_standard = "20"
        if (self.settings.compiler.cppstd):
            tools.check_min_cppstd(self, minimal_cpp_standard)

        minimum_version = {"gcc": 10, "clang": 12}
        compiler = str(self.settings.compiler)
        version = tools.Version(self.settings.compiler.version)

        if version < minimum_version[compiler]:
            raise ConanInvalidConfiguration(
                "%s requires %s version %d or later"
                % (self.name, compiler, minimum_version[compiler]))

    def requirements(self):
        # Require liburing on any Linux system as a run-time check will be
        # done to detect if the host kernel supports io_uring.
        if self.settings.os == "Linux":
            self.requires("liburing/2.1")

    def build(self):
        autotools = AutoToolsBuildEnvironment(self)
        env_build_vars = autotools.vars
        # Conan uses LIBS, presumably following autotools conventions, while
        # the XTR makefile follows GNU make conventions and uses LDLIBS
        env_build_vars["LDLIBS"] = env_build_vars["LIBS"]
        # fPIC and Release/Debug/RelWithDebInfo etc are set via CXXFLAGS,
        # CPPFLAGS etc.
        env_build_vars["EXCEPTIONS"] = \
            str(int(bool(self.options.enable_exceptions)))
        env_build_vars["LTO"] = str(int(bool(self.options.enable_lto)))
        autotools.make(vars=env_build_vars)
        autotools.make(vars=env_build_vars, target="xtrctl")

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*/libxtr.a", dst="lib", src="build", keep_path=False)
        self.copy("*/xtrctl", dst="bin", src="build", keep_path=False)
        self.copy("libxtr.3", dst="man/man3", src="docs", keep_path=False)
        self.copy(
            "libxtr-quickstart.3",
            dst="man/man3",
            src="docs",
            keep_path=False)
        self.copy(
            "libxtr-userguide.3",
            dst="man/man3",
            src="docs",
            keep_path=False)
        self.copy("xtrctl.1", dst="man/man1", src="docs", keep_path=False)
        self.copy("LICENSE", "licenses")

    def package_info(self):
        self.cpp_info.libs = ["xtr"]
        self.cpp_info.system_libs = ["pthread"]
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.env_info.MANPATH.append(os.path.join(self.package_folder, "man"))
