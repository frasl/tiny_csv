#pragma once

#include <tiny_csv/types.hpp>

#include <string>

namespace tiny_csv {

/***
 * Parser configuration.
 * Notes:
 * - some files use "" instead of \" - just set escape_char to '"'
 * - datetime_format is used by default loader for the type std::tm
 */
struct ParserConfig {
    char_t          escape_char { '\\' };
    char_t          quote_char { '\"' };
    char_t          token_separator { ',' };
    std::string     datetime_format { "%Y-%m-%d %H:%M:%S" };
};

}