cmake_minimum_required(VERSION 3.14)

set(TARGET_PROJECT ${LIBRARY_NAME})


# Library sources
set(TINY_CSV_SOURCES
    src/tiny_csv.cpp
    src/type_loader.cpp
    src/tokenizer.cpp
    src/multimatch.cpp
)

# Library headers
set(TINY_CSV_HEADERS
    include/tiny_csv/tiny_csv.hpp
    include/tiny_csv/type_loader.hpp
    include/tiny_csv/tokenizer.hpp
    include/tiny_csv/multimatch.hpp
    include/tiny_csv/text_buffer.hpp
    include/tiny_csv/config.hpp
)

# Create the library target
add_library(tiny_csv ${TINY_CSV_SOURCES} ${TINY_CSV_HEADERS})
target_include_directories(tiny_csv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tiny_csv PUBLIC
        fmt::fmt)

# Add test executable
add_executable(tiny_csv_tests
        tests/test_csv_parser.cpp
        tests/test_tokenizer.cpp
        tests/test_perf.cpp)
target_link_libraries(tiny_csv_tests PRIVATE
        tiny_csv
        GTest::gtest_main)
target_compile_definitions(tiny_csv_tests PRIVATE
        TEST_PERFORMANCE)

include(GoogleTest)
gtest_discover_tests(tiny_csv_tests)

# Enable testing
enable_testing()

add_custom_command(TARGET tiny_csv_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tiny_csv/tests/basic_test.csv
        $<TARGET_FILE_DIR:tiny_csv_tests>/basic_test.csv
        )
add_custom_command(TARGET tiny_csv_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tiny_csv/tests/optional.csv
        $<TARGET_FILE_DIR:tiny_csv_tests>/optional.csv
        )
