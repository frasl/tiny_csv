cmake_minimum_required(VERSION 3.22)

set(LIBRARY_NAME tiny_csv)
project(${LIBRARY_NAME}_root)

# Set build types
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/generators")

message(STATUS "CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)

# Compiler flags for Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    add_compile_definitions(DEBUG_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g")
endif()

# Compiler flags for Release configuration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring Release build")
    add_compile_definitions(RELEASE_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# Additional flags for RelWithDebInfo (Release with Debug Information)
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Configuring RelWithDebInfo build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -DNDEBUG")
endif()

# Find packages generated by Conan
find_package(fmt REQUIRED)
find_package(GTest REQUIRED)

# Add library
add_subdirectory(${LIBRARY_NAME})
