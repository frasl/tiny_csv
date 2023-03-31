#include <tiny_csv/tokenizer.hpp>

#include <stack>

using namespace tiny_csv;

Tokenizer::Tokenizer(const char_t escape, const char_t quote,
                     const MultiMatch &token_separators, const MultiMatch &line_separators) :
        buf_(nullptr),
        size_(0),
        token_separators(token_separators),
        line_separators(line_separators),
        quote_(quote),
        escape_(escape) {}

bool Tokenizer::NextToken(TextBuffer<char_t> &token) {
    enum State {
        InToken,
        InQuotes,
        InEscape,
        TokenEnd,
        Invalid
    };

    token.Clear();
    if (offset_ >= size_)
        throw std::runtime_error("Expected token not found");

    const char *ptr = buf_ + offset_;

    size_t size = 0;
    State state = InToken, prev_state = Invalid;
    for (char_t c = ptr[size]; ; c=ptr[++size]) {
        if (offset_ + size >= size_) {
            offset_ += (size + 1);
            return false; // Token ends together with the input
        }

        switch (state) {
            case InToken:
                if (c == quote_) {
                    if (size > 0)
                        throw std::logic_error("Quotes not allowed in the middle of the column");

                    state = InQuotes;
                } else if (token_separators.Check(c)) {
                    offset_ += (size + 1);
                    return true; // Reached end of a valid token
                } else {
                    token.PushBack(c);
                }

                break;

            case InQuotes:
                if (c == escape_) {
                    prev_state = state;
                    state = InEscape;
                } else if (c == quote_) {
                    state = TokenEnd;
                } else {
                    token.PushBack(c);
                }

                break;

            case InEscape:
                token.PushBack(c);
                state = prev_state;
                break;

            case TokenEnd:
                if (token_separators.Check(c)) {
                    offset_ += (size + 1);
                    return true; // Reached end of a valid token
                } else {
                    throw std::logic_error("Quotation must end together with the token");
                }
                break;

            default:
                throw std::logic_error("Token parser FSM error");

        }
    }
}

bool Tokenizer::NextLine(TextBuffer<char_t> &line) {
    line.Clear();
    for (const char *ptr = buf_ + offset_; !line_separators.Check(*ptr); ptr = buf_ + (++offset_)) {
        if (offset_ >= size_)
            return false; // No more data will follow

        line.PushBack(*ptr);
    }

    // Chew trailing separators
    for (const char *ptr = buf_ + offset_; line_separators.Check(*ptr) && offset_ < size_; ptr = buf_ + (++offset_));

    return true;
}

void Tokenizer::Reset(const char_t *buf, size_t size) {
    buf_ = buf;
    size_ = size;
    offset_ = 0;
}