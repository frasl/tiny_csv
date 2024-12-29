from conan import ConanFile
from conan.tools.cmake import cmake_layout
import os

class TickTickConan(ConanFile):
    name = "tiny_csv"
    version = "0.1.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = "spdlog/1.11.0", "fmt/10.0.0", "gtest/cci.20210126"
    #tool_requires = "gtest/cci.20210126"
    options = {        
    }

    default_options = {
        "boost/*:shared": False,
        "boost/*:without_python": True,
        "boost/*:without_graph": True
    }

    def layout(self):
        cmake_layout(self)
        self.folders.build = "build"  # Set the build folder to './build'

    def generate(self):
        pass

    def build(self):
        pass

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={"CONAN_BUILD": "True"})
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

