#include <tiny_csv/multimatch.hpp>
#include <tiny_csv/text_buffer.hpp>
#include <tiny_csv/config.hpp>

#include <cctype>
#include <tuple>
#include <stdexcept>

namespace tiny_csv {

class Tokenizer {
public:
    struct ParsingException : public std::runtime_error {
        ParsingException(const std::string &what) : std::runtime_error(what) {}
    };

    Tokenizer(const ParserConfig &cfg);

    void Reset(const char_t *buf, size_t size);

    // Returns next token, sets complete flag for the last one
    bool NextToken(TextBuffer<char_t> &token);
    bool NextLine(TextBuffer<char_t> &line);

    // Returns true if there will be more tokens to enjoy
    bool HasMore() const {
        return offset_ < size_;
    }

    size_t Offset() const {
        return offset_;
    }

    // For debug and exception texts only
    std::string DebugPrint() const;

private:
    const char *buf_;
    size_t      size_;
    size_t      offset_ { 0 };

    // Configuration
    const ParserConfig cfg_;
    const MultiMatch   token_separators_;
    const MultiMatch   line_separators_;

};

}