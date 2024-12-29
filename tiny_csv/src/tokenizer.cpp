#include <tiny_csv/tokenizer.hpp>

#include <stack>

using namespace tiny_csv;

Tokenizer::Tokenizer(const ParserConfig &cfg) :
        buf_(nullptr),
        size_(0),
        cfg_(cfg),        
        token_separators_( { cfg.token_separator, '\0' } ),
        line_separators_( { '\n', '\r', '\0' } ) {}

bool Tokenizer::NextToken(TextBuffer<char_t> &token) {
    enum State {
        InToken,
        InQuotes,
        InDblQuotes,
        InEscape,
        TokenEnd,
        Invalid
    };

    token.Clear();
    if (offset_ >= size_)
        throw ParsingException("Expected token not found");

    const char *ptr = buf_ + offset_;
    if (token_separators_.Check(*ptr))
        ptr = buf_ + (++offset_);

    size_t size = 0;
    State state = InToken;
    State prev_state = Invalid; // CSV format gives us a luxury of not having a full-scale stack here
    for (char_t c = ptr[size]; ; c=ptr[++size]) {
        if (offset_ + size >= size_) {
            offset_ += size;
            return false; // Token ends together with the input
        }

        switch (state) {
            case InToken:
                if (c == cfg_.quote_char) {
                    if (size > 0)
                        throw ParsingException("Quotes not allowed in the middle of the column");

                    state = InQuotes;
                } else if (token_separators_.Check(c)) {
                    offset_ += size;
                    return true; // Reached end of a valid token
                } else {
                    token.PushBack(c);
                }

                break;

            case InQuotes:
                if (cfg_.quote_char == cfg_.escape_char) {
                    // Some CSVs use "" to escape quotes
                    if (c == cfg_.quote_char) {
                        prev_state = state;
                        state = InDblQuotes;
                    } else {
                        token.PushBack(c);
                    }
                } else if (c == cfg_.escape_char) {
                    prev_state = state;
                    state = InEscape;
                } else if (c == cfg_.quote_char) {
                    state = TokenEnd;
                } else {
                    token.PushBack(c);
                }

                break;

            case InDblQuotes:
                if (c == cfg_.quote_char) {
                    // Double quotes, indeed
                    token.PushBack(c);
                    prev_state = state;
                    state = InQuotes;
                } else if (token_separators_.Check(c) || offset_ + size == size_) {
                    // Not the case, it was quotes termination, token ends
                    offset_ += size;
                    return true; // Reached end of a valid token
                } else {
                    throw ParsingException("Unexpected character after a quote");
                }
                break;

            case InEscape:
                token.PushBack(c);
                state = prev_state;
                break;

            case TokenEnd:
                if (token_separators_.Check(c)) {
                    offset_ += size;
                    return true; // Reached end of a valid token
                } else {
                    throw ParsingException("Quotation must end together with the token");
                }
                break;

            default:
                throw ParsingException("Token parser FSM error");

        }
    }
}

bool Tokenizer::NextLine(TextBuffer<char_t> &line) {
    line.Clear();
    for (const char *ptr = buf_ + offset_; !line_separators_.Check(*ptr); ptr = buf_ + (++offset_)) {
        if (offset_ >= size_)
            return false; // No more data will follow

        line.PushBack(*ptr);
    }

    // Chew trailing separators
    for (const char *ptr = buf_ + offset_; line_separators_.Check(*ptr) && offset_ < size_; ptr = buf_ + (++offset_));

    return true;
}

void Tokenizer::Reset(const char_t *buf, size_t size) {
    buf_ = buf;
    size_ = size;
    offset_ = 0;
}

std::string Tokenizer::DebugPrint() const {
    return std::string(buf_, size_);
}