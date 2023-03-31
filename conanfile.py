from conans import ConanFile, CMake, tools
import os, sys


class TinyCSVConan(ConanFile):
    def set_version(self):
        self.version = "0.1.0"

    name = "tiny_csv"

    generators = "cmake_find_package", "cmake"
    settings = "os", "compiler", "build_type", "arch"

    exports_sources = [
        "CMakeLists.txt",
        "tiny_csv/*",
    ]
    no_copy_source = True
    build_policy = "missing"
    _cmake = None
         
    def requirements(self):
        # open-source
        self.requires("fmt/8.1.1")

    def build_requirements(self):
        self.build_requires("gtest/cci.20210126")

    def configure(self):
        minimal_cpp_standard = "20"
        if self.settings.compiler.cppstd:
            tools.check_min_cppstd(self, minimal_cpp_standard)
        minimal_version = {
            "gcc": "10",
            "clang": "10",
        }
        compiler = str(self.settings.compiler)
        if compiler not in minimal_version:
            self.output.warn(
                (
                    "%s recipe lacks information about the %s compiler "
                    "standard version support"
                )
                % (self.name, compiler)
            )
            self.output.warn(
                "%s requires a compiler that supports at least C++%s"
                % (self.name, minimal_cpp_standard)
            )
            return

        version = tools.Version(self.settings.compiler.version)
        if version < minimal_version[compiler]:
            raise ConanInvalidConfiguration(
                "%s requires a compiler that supports at least C++%s"
                % (self.name, minimal_cpp_standard)
            )

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake

        self._cmake = CMake(self)
        self._cmake.configure()
        self._cmake.definitions["CONAN_BUILD"] = True
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "tiny_csv"
        self.cpp_info.names["cmake_find_package_multi"] = "tiny_csv"

        self.cpp_info.components["tiny_csv"].libs = ["tiny_csv"]
        self.cpp_info.components["tiny_csv"].requires = [
            "fmt::fmt"
        ]


