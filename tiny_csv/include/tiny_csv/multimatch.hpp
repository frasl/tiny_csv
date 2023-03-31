#pragma once

#include <tiny_csv/types.hpp>

#include <limits>
#include <type_traits>
#include <set>
#include <cctype>
#include <vector>

namespace tiny_csv {

/*
 * Build a table for finding character matches fast - will return true if any of the matching characters get hit
 */
class MultiMatch {
    using IndexType = std::make_unsigned<char_t>::type;

public:
    MultiMatch(const std::set<char_t> &chars);
    MultiMatch(MultiMatch &&from);

    bool Check(char_t c) const {
        return match_table_[c + BASE_VALUE];
    }

private:
    static constexpr size_t BASE_VALUE = -std::numeric_limits<char_t>::min();
    static constexpr size_t SIZE = BASE_VALUE + std::numeric_limits<char_t>::max();

    std::vector<bool>   match_table_;
};

} // namespace