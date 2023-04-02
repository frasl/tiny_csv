#include <tiny_csv/tiny_csv.hpp>

#include <gtest/gtest.h>
#include <chrono>

namespace /* anonymous */
{

#ifdef TEST_PERFORMANCE

int64_t getTimestampMicroseconds() {
    auto now = std::chrono::system_clock::now();
    auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_us.time_since_epoch();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();
    return timestamp;
}

uint64_t getFileSize(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    return file.tellg();
}

// This test uses https://www.kaggle.com/datasets/city-of-seattle/seattle-checkouts-by-title cut down to 1M lines
// Can be found in perf-test.zip
TEST(Performance, Optionals) {
    const std::string fname = "perf-test.csv";
    std::vector<std::string> column_headers = {"UsageClass", "CheckoutType", "MaterialType", "CheckoutYear",
                                               "CheckoutMonth", "Checkouts", "Title", "Creator", "Subjects",
                                               "Publisher",
                                               "PublicationYear"};

    using Columns = tiny_csv::ColTuple<std::optional<std::string>,
            std::optional<std::string>,
            std::optional<std::string>,
            std::optional<int>,
            std::optional<int>,
            std::optional<int>,
            std::optional<std::string>,
            std::optional<std::string>,
            std::optional<std::string>,
            std::optional<std::string>,
            std::optional<std::string>>;

    tiny_csv::ParserConfig cfg;
    cfg.escape_char = '\"';

    int64_t size = getFileSize(fname);
    int64_t t_begin = getTimestampMicroseconds();

    auto csv =
            tiny_csv::CreateFromFile<Columns, Columns::DefaultLoaders>(fname, column_headers, cfg);

    int64_t duration = getTimestampMicroseconds() - t_begin;

    std::cout << fmt::format("Simple read of {} MB file took {} usec, giving {} MB/sec performance (all cols optional, ints parsed)",
                             size / 1048576, duration, (double)size / duration) << std::endl;
}

TEST(Performance, AllStrings) {
    const std::string fname = "perf-test.csv";
    std::vector<std::string> column_headers = {"UsageClass", "CheckoutType", "MaterialType", "CheckoutYear",
                                               "CheckoutMonth", "Checkouts", "Title", "Creator", "Subjects",
                                               "Publisher",
                                               "PublicationYear"};

    using Columns = tiny_csv::ColTuple<std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string>;

    tiny_csv::ParserConfig cfg;
    cfg.escape_char = '\"';

    int64_t size = getFileSize(fname);
    int64_t t_begin = getTimestampMicroseconds();

    auto csv =
            tiny_csv::CreateFromFile<Columns, Columns::DefaultLoaders>(fname, column_headers, cfg);

    int64_t duration = getTimestampMicroseconds() - t_begin;

    std::cout << fmt::format("Simple read of {} MB file took {} usec, giving {} MB/sec performance (all strings)",
                             size / 1048576, duration, (double)size / duration) << std::endl;
}


#endif

}

