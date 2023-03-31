#include <tiny_csv/tiny_csv.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <vector>

namespace /* anonymous */
{

TEST(TinyCSV, ColTranslation) { // Test translation of index id into column id
    auto col_id = tiny_csv::TinyCSV<std::tuple<char, int, char>, 0, 2>::Idx2ColId<0>();
    EXPECT_EQ(col_id, 0);

    col_id = tiny_csv::TinyCSV<std::tuple<char, int, char>, 0, 2>::Idx2ColId<1>();
    EXPECT_EQ(col_id, 2);
}

TEST(TinyCSV, LoadCSV) {
        auto csv =
                tiny_csv::CreateFromFile<std::tuple<int, uint16_t, int64_t, std::string, std::string, std::string>,
                        0, 2, 4>("basic_test.csv",
                                 {"id", "short_num", "long_num", "simple_string", "date", "escaped_string"});
    auto it0 = csv.Find<0>(3); // Search by 0-th index
    EXPECT_EQ(it0.HasData(), true);
    EXPECT_EQ(std::get<2>(*it0), 123454567799);
    ++it0;
    EXPECT_EQ(it0.HasData(), false);

    auto it2 = csv.Find<1>(123454567788); // Search by 1-st index
    EXPECT_EQ(it2.Size(), 10);
}



} // namespace