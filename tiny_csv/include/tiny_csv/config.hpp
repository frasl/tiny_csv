#pragma once

#include <tiny_csv/types.hpp>

#include <string>

namespace tiny_csv {

struct ParserConfig {
    char_t          escape_char { '\\' };
    char_t          quote_char { '\"' };
    char_t          token_separator { ',' };
    std::string     datetime_format { "%Y-%m-%d %H:%M:%S" };
};

}