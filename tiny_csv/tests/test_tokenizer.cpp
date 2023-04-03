#include <tiny_csv/tokenizer.hpp>

#include <gtest/gtest.h>

namespace  {

std::string FetchNextToken(tiny_csv::Tokenizer &tokenizer) {
    tiny_csv::TextBuffer<char> buf;
    tokenizer.NextToken(buf);

    return std::string(buf.Data(), buf.Size());
}

std::string FetchNextLine(tiny_csv::Tokenizer &tokenizer) {
    tiny_csv::TextBuffer<char> buf;
    tokenizer.NextLine(buf);

    return std::string(buf.Data(), buf.Size());
}

TEST(Tokenizer, TokenizerLineTest) {
    tiny_csv::ParserConfig cfg; // Default
    std::string test_str = "1\n2\r\n3\n\n4\n\n\n\n\n";
    tiny_csv::Tokenizer tokenizer(cfg);

    tokenizer.Reset(test_str.c_str(), test_str.size());
    EXPECT_EQ(FetchNextLine(tokenizer), "1");
    EXPECT_EQ(FetchNextLine(tokenizer), "2");
    EXPECT_EQ(FetchNextLine(tokenizer), "3");
    EXPECT_EQ(FetchNextLine(tokenizer), "4");

    EXPECT_EQ(tokenizer.HasMore(), false);

    tiny_csv::TextBuffer<char> buf;
    EXPECT_ANY_THROW(tokenizer.NextToken(buf));

}

TEST(Tokenizer, TokenizerTokenTest) {
    std::string test_str = "17,2.5,ABCDE,-1,\"Quoted String\",\"Quoted string,\",\"Quoted, \\\"string\\\"\"";
    tiny_csv::ParserConfig cfg; // Default
    tiny_csv::Tokenizer tokenizer(cfg);

    tokenizer.Reset(test_str.c_str(), test_str.size());
    EXPECT_EQ(FetchNextToken(tokenizer), "17");
    EXPECT_EQ(FetchNextToken(tokenizer), "2.5");
    EXPECT_EQ(FetchNextToken(tokenizer), "ABCDE");
    EXPECT_EQ(FetchNextToken(tokenizer), "-1");
    EXPECT_EQ(FetchNextToken(tokenizer), "Quoted String");
    EXPECT_EQ(FetchNextToken(tokenizer), "Quoted string,");
    EXPECT_EQ(FetchNextToken(tokenizer), "Quoted, \"string\"");

    EXPECT_EQ(tokenizer.HasMore(), false);

    tiny_csv::TextBuffer<char> buf;
    EXPECT_ANY_THROW(tokenizer.NextToken(buf));
}

TEST(Tokenizer, TokenizerEmptyTokenTest) {
    std::string test_str = "12,,11,10,,,,";
    tiny_csv::ParserConfig cfg; // Default
    tiny_csv::Tokenizer tokenizer(cfg);

    tokenizer.Reset(test_str.c_str(), test_str.size());
    EXPECT_EQ(FetchNextToken(tokenizer), "12");
    EXPECT_EQ(FetchNextToken(tokenizer), "");
    EXPECT_EQ(FetchNextToken(tokenizer), "11");
    EXPECT_EQ(FetchNextToken(tokenizer), "10");
    EXPECT_EQ(FetchNextToken(tokenizer), "");
    EXPECT_EQ(FetchNextToken(tokenizer), "");
    EXPECT_EQ(FetchNextToken(tokenizer), "");
    EXPECT_EQ(FetchNextToken(tokenizer), "");

    EXPECT_EQ(tokenizer.HasMore(), false);
}


}