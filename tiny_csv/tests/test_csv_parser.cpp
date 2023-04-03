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
    EXPECT_EQ(it2.NMatches(), 10);
}

TEST(TinyCSV, Merge) {
    using Columns = tiny_csv::ColTuple<int, int>;
    tiny_csv::TinyCSV<Columns, Columns::DefaultLoaders, 0> csv;
    std::vector<tiny_csv::TinyCSV<Columns>> merged_csvs(2);
    std::string s_csv1("1,2\n3,4\n5,6"), s_csv2("10,20\n30,40\n50,60");
    merged_csvs[0].Append(s_csv1.data(), s_csv1.size());
    merged_csvs[1].Append(s_csv2.data(), s_csv2.size());

    csv.Append(merged_csvs, 4);

    EXPECT_EQ(std::get<0>(csv[0]), 1);
    EXPECT_EQ(std::get<1>(csv[0]), 2);
    EXPECT_EQ(std::get<0>(csv[1]), 3);
    EXPECT_EQ(std::get<1>(csv[1]), 4);
    EXPECT_EQ(std::get<0>(csv[2]), 5);
    EXPECT_EQ(std::get<1>(csv[2]), 6);
    EXPECT_EQ(std::get<0>(csv[3]), 10);
    EXPECT_EQ(std::get<1>(csv[3]), 20);
    EXPECT_EQ(std::get<0>(csv[4]), 30);
    EXPECT_EQ(std::get<1>(csv[4]), 40);
    EXPECT_EQ(std::get<0>(csv[5]), 50);
    EXPECT_EQ(std::get<1>(csv[5]), 60);
    EXPECT_EQ(csv.Size(), 6);

    auto it = csv.Find<0>(10);
    EXPECT_EQ(it.HasData(), true);
    EXPECT_EQ(std::get<1>(*it), 20);
}

TEST(TinyCSV, LoadCSVMT) {
    using Columns = tiny_csv::ColTuple<int, uint16_t, int64_t, std::string, std::tm, std::string>;
    auto csv =
            tiny_csv::CreateFromFileMT<Columns, Columns::DefaultLoaders,
                    0, 2, 5>("basic_test.csv",
                             {"id", "short_num", "long_num", "simple_string", "datetime", "escaped_string"}, {}, 8);
    auto it0 = csv.Find<0>(3); // Search by 0-th index
    EXPECT_EQ(it0.HasData(), true);
    EXPECT_EQ(std::get<2>(*it0), 123454567799);

    // Check if the date has parsed correctly
    EXPECT_EQ(std::get<4>(*it0).tm_hour,  13);
    EXPECT_EQ(std::get<4>(*it0).tm_year,  123);
    ++it0;
    EXPECT_EQ(it0.HasData(), false);

    auto it2 = csv.Find<1>(123454567788); // Search by 1-st index
    EXPECT_EQ(it2.NMatches(), 10);

    // The order should remain the same
    for (size_t i = 0; i < csv.Size(); ++i) {
        EXPECT_EQ(std::get<0>(csv[i]), i);
    }
}


} // namespace
