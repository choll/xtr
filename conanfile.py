from conan import ConanFile

class Conan(ConanFile):
    name = "xtr"
    settings = "os", "arch", "compiler", "build_type"
    generators = "PkgConfigDeps"

    def requirements(self):
        self.requires("fmt/12.1.0", transitive_headers=True, transitive_libs=True)
        if self.settings.os == "Linux":
            self.requires("liburing/2.4")
        self.requires("benchmark/1.9.5")
        self.requires("catch2/2.13.9")
