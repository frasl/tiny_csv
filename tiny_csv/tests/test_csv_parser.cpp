#include <tiny_csv/tiny_csv.hpp>

#include <gtest/gtest.h>

namespace /* anonymous */
{

std::string Token2Str(tiny_csv::Tokenizer::Token &&token) {
    return std::string(std::get<0>(token), std::get<1>(token));
}

TEST(TinyCSV, TokenizerTest) {
    std::string test_str = "17,2.5,ABCDE,-1";
    tiny_csv::Tokenizer tokenizer(test_str.c_str(), test_str.size(), ',');

    EXPECT_EQ(Token2Str(tokenizer.Next()), "17");
    EXPECT_EQ(Token2Str(tokenizer.Next()), "2.5");
    EXPECT_EQ(Token2Str(tokenizer.Next()), "ABCDE");
    EXPECT_EQ(Token2Str(tokenizer.Next()), "-1");
    EXPECT_EQ(tokenizer.HasMore(), false);
    EXPECT_ANY_THROW(tokenizer.Next());
}

TEST(TinyCSV, BasicTest) {
    tiny_csv::TinyCSV<std::tuple<char, int, int, int, double>, 0, 1> csv;

    /*auto it = csv.Find<int, 1>(3);
    if (it != csv.End<1>()) {

    }*/
    auto res = csv.Test<0>();
    //auto val = tiny_csv::TinyCSV<std::tuple<char, int, int, int, double>, 0, 1>::IndexTupleType ();
}

} // namespace