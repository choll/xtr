from conan import ConanFile
from conan.tools.gnu import MakeDeps

class Conan(ConanFile):
    name = "xtr"
    settings = "os", "arch", "compiler", "build_type"

    def requirements(self):
        self.requires("fmt/10.1.1", transitive_headers=True, transitive_libs=True)
        if self.settings.os == "Linux":
            self.requires("liburing/2.4")
        self.requires("benchmark/1.6.0")
        self.requires("catch2/2.13.9")

    def generate(self):
        make_toolchain = MakeDeps(self)
        make_toolchain.generate()
