from conans import ConanFile, AutoToolsBuildEnvironment, tools

import os

class XtrConan(ConanFile):
    requires = "fmt/7.0.1@"
    build_requires = "benchmark/1.5.0@", "catch2/2.13.0@"
    name = "xtr"
    version = "0.1"
    license = "MIT"
    author = "Chris Holloway (c.holloway@gmail.com)"
    url = "https://github.com/choll/xtr"
    description = "A C++ logging library"
    settings = {
        "os": ["Linux", "FreeBSD"],
        "compiler": ["gcc", "clang"],
        "build_type": ["Release", "Debug"],
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
    exports_sources = ["src/*", "include/*", "Makefile", "LICENSE"]

    def build(self):
        autotools = AutoToolsBuildEnvironment(self)
        env_build_vars = autotools.vars
        # Conan uses LIBS, presumably following autotools conventions, while
        # the XTR makefile follows GNU make conventions and uses LDLIBS, so
        # copy it.
        env_build_vars["LDLIBS"] = env_build_vars["LIBS"]
        # fPIC is handled by AutoToolsBuildEnvironment modifying CXXFLAGS, set
        # PIC anyway.
        env_build_vars["PIC"] = str(int(bool(self.options.fPIC)))
        env_build_vars["EXCEPTIONS"] = str(int(bool(self.options.enable_exceptions)))
        env_build_vars["LTO"] = str(int(bool(self.options.enable_lto)))
        env_build_vars["DEBUG"] = "1" if self.settings.build_type == "Debug" else "0"
        autotools.make(vars=env_build_vars)
        autotools.make(vars=env_build_vars, target="xtrctl")

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*/libxtr.a", dst="lib", src="build", keep_path=False)
        self.copy("*/xtrctl", dst="bin", src="build", keep_path=False)
        self.copy("LICENSE", "licenses")

    def package_info(self):
        self.cpp_info.libs = ["xtr"]
        self.cpp_info.system_libs = ["pthread"]
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
