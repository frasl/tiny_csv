cmake_minimum_required(VERSION 3.14)

set(TARGET_PROJECT ${LIBRARY_NAME})


# Library sources
set(TINY_CSV_SOURCES
    src/tiny_csv.cpp
    src/type_loader.cpp
    src/tokenizer.cpp
)

# Library headers
set(TINY_CSV_HEADERS
    include/tiny_csv/tiny_csv.hpp
    include/tiny_csv/type_loader.hpp
    include/tiny_csv/tokenizer.hpp
)

# Create the library target
add_library(tiny_csv ${TINY_CSV_SOURCES} ${TINY_CSV_HEADERS})
target_include_directories(tiny_csv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add test executable
add_executable(tiny_csv_tests tests/test_csv_parser.cpp)
target_link_libraries(tiny_csv_tests PRIVATE
        tiny_csv
        GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tiny_csv_tests)

# Enable testing
enable_testing()

