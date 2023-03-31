#include <tiny_csv/multimatch.hpp>
#include <tiny_csv/text_buffer.hpp>

#include <cctype>
#include <tuple>
#include <stdexcept>

namespace tiny_csv {

class Tokenizer {
public:
    Tokenizer(const char_t escape, const char_t quote,
              const MultiMatch &token_separators, const MultiMatch &line_separators);

    void Reset(const char_t *buf, size_t size);

    // Returns next token, sets complete flag for the last one
    bool NextToken(TextBuffer<char_t> &token);
    bool NextLine(TextBuffer<char_t> &line);

    // Returns true if there will be more tokens to enjoy
    bool HasMore() const {
        return offset_ < size_;
    }

private:
    const char *buf_;
    size_t      size_;
    size_t      offset_ { 0 };

    const MultiMatch   &token_separators;
    const MultiMatch   &line_separators;
    const char_t        quote_;
    const char_t        escape_;

};

}