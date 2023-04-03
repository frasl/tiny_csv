#pragma once

#include <tiny_csv/types.hpp>

#include <limits>
#include <type_traits>
#include <set>
#include <cctype>
#include <vector>

namespace tiny_csv {

/*
 * Builds a lookup table
 */
class MultiMatch {
    using IndexType = std::make_unsigned<char_t>::type;

public:
    /***
     * Constructor
     * @param chars - a set of chars, that would cause Check to return true
     */
    MultiMatch(const std::set<char_t> &chars);
    MultiMatch(MultiMatch &&from);

    /***
     * Checks, if c is in the match set
     * @param c = character to check
     * @return true if c is one of the set
     */
    bool Check(char_t c) const {
        return match_table_[c + BASE_VALUE];
    }

private:
    // To make both signed and unsigned types work
    static constexpr size_t BASE_VALUE = -std::numeric_limits<char_t>::min();
    static constexpr size_t SIZE = BASE_VALUE + std::numeric_limits<char_t>::max();

    std::vector<bool>   match_table_;
};

} // namespace