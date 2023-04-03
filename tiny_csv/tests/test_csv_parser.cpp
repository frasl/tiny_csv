#include <tiny_csv/tiny_csv.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <vector>

namespace /* anonymous */
{

TEST(TinyCSV, DefaultLoaders) {
    using Columns = tiny_csv::ColTuple<char, int, std::string>;
    Columns::DefaultLoaders ldr;

    static_assert(std::is_same_v<std::tuple_element<0, Columns::DefaultLoaders>::type ,
                                 tiny_csv::Loader<char>>, "Default loaders construct incorrectly");
    static_assert(std::is_same_v<std::tuple_element<1, Columns::DefaultLoaders>::type,
                                 tiny_csv::Loader<int>>, "Default loaders construct incorrectly");
    static_assert(std::is_same_v<std::tuple_element<2, Columns::DefaultLoaders>::type,
                                 tiny_csv::Loader<std::string>>, "Default loaders construct incorrectly");

    static_assert(std::is_same_v<std::tuple<char, int, std::string>, Columns::ColTypesTuple>,
            "Column types constructed incorrectly");
}


TEST(TinyCSV, LoadCSVOptional) {
    const std::string fname = "optional.csv";
    std::vector<std::string> column_names = {"UsageClass", "CheckoutType", "MaterialType", "CheckoutYear",
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
    auto csv =
            tiny_csv::CreateFromFile<Columns>(fname, column_names, cfg);

    EXPECT_EQ(std::get<10>(csv[4]).has_value(), false);
    EXPECT_EQ(std::get<3>(csv[4]).value(), 2006);
}

TEST(TinyCSV, CustomLoaders) {
    using Columns = tiny_csv::ColTuple<int, int>;
    struct CustomIntLoader {
        static int Load(const char *buf, size_t len, const tiny_csv::ParserConfig &cfg) {
            int val;
            std::from_chars(buf, buf + len, val);
            return -val; // To distinguish value in test
        }
    };
    std::string s_csv("1,1\n3,3\n5,5");
    tiny_csv::TinyCSV<Columns, std::tuple<tiny_csv::Loader<int>, CustomIntLoader>> csv;
    csv.Append(s_csv.data(), s_csv.size());
    for (const auto &row: csv) {
        EXPECT_EQ(std::get<0>(row), -std::get<1>(row));
    }
}


TEST(TinyCSV, SizeAndIteration) {
    using Columns = tiny_csv::ColTuple<int, int>;
    tiny_csv::TinyCSV<Columns> csv;
    std::string s_csv("1,2\n3,4\n5,6");
    csv.Append(s_csv.data(), s_csv.size());

    EXPECT_EQ(csv.Size(), 3);
    size_t i = 0;
    for (const auto &row: csv) {
        EXPECT_EQ(row, csv[i++]);
    }
}

TEST(TinyCSV, LoadCSV) {
    using Columns = tiny_csv::ColTuple<int, uint16_t, int64_t, std::string, std::tm, std::string>;
    auto csv =
            tiny_csv::CreateFromFile<Columns, Columns::DefaultLoaders,
                    0, 2, 5>("basic_test.csv",
                             {"id", "short_num", "long_num", "simple_string", "datetime", "escaped_string"});
    auto it0 = csv.Find<0>(3); // Search by 0-th index
    EXPECT_EQ(it0.HasData(), true);
    EXPECT_EQ(std::get<2>(*it0), 123454567799);

    // Check if the date has parsed correctly
    EXPECT_EQ(std::get<4>(*it0).tm_hour,  13);
    EXPECT_EQ(std::get<4>(*it0).tm_year,  123);
    ++it0;
    EXPECT_EQ(it0.HasData(), false);

    auto it2 = csv.Find<1>(123454567788); // Search by 1-st index
    EXPECT_EQ(it2.Size(), 10);
}

} // namespace
