#include <tiny_csv/multimatch.hpp>

using namespace tiny_csv;

MultiMatch::MultiMatch(const std::set<char_t> &chars)
{
    match_table_.resize(SIZE);
    for (IndexType i = 0; i < SIZE; ++i) {
        match_table_[i] = chars.contains(static_cast<char_t>(i - BASE_VALUE));
    }
}

MultiMatch::MultiMatch(MultiMatch &&from) :
        match_table_(std::move(from.match_table_))
{}
