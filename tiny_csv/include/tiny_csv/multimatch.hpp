#pragma once

#include <tiny_csv/types.hpp>

#include <limits>
#include <type_traits>
#include <ctype.h>

namespace tiny_csv {

/*
 * Build a table for finding character matches fast - will return true if any of the matching characters get hit
 */
class MultiMatch {
public:
     MultiMatch(); {
        for (IndexType i = 0; i < SIZE; ++i) {
            constexpr T val = i - BASE_VALUE;
            match_table[i] = match<val>();
        }
    }

    bool Check(T c) {
        return match_table[c + BASE_VALUE];
    }

private:
    static constexpr size_t BASE_VALUE = -std::numeric_limits<char_t>::min();
    static constexpr size_t SIZE = BASE_VALUE + std::numeric_limits<char_t>::max();

    bool match_table[SIZE];
};

} // namespace