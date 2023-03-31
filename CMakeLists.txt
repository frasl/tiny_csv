cmake_minimum_required(VERSION 3.14)

set(LIBRARY_NAME tiny_csv)
project(${LIBRARY_NAME}_root CXX)

message(STATUS "ENABLE C++20")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CONAN_BUILD "A flag that tells if this is a conan controlled build" OFF)
option(CMAKE_RUNS_CONAN "A flag that tells to run conan install within cmake generation step" OFF)

if (CONAN_BUILD AND CMAKE_RUNS_CONAN)
    message(FATAL_ERROR "CONAN_BUILD and CMAKE_RUNS_CONAN cannot be enabled together")
endif ()


#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (CONAN_BUILD)
    # Include Conan
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)
    conan_cmake_install(
            PATH_OR_REFERENCE ${CMAKE_CURRENT_SOURCE_DIR}
            BUILD missing
            SETTINGS compiler.cppstd=${CMAKE_CXX_STANDARD}
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

find_package(GTest REQUIRED)

add_subdirectory(${LIBRARY_NAME})