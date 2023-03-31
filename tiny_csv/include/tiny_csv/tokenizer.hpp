#include <tiny_csv/multimatch.hpp>

#include <cctype>
#include <tuple>
#include <stdexcept>

namespace tiny_csv {

template <typename MatchEngine>
class Tokenizer {
    // Constraints
    static_assert(std::is_same<decltype(&MatchEngine::Check), bool (MatchEngine::*)(char)>::value,
        "MatchEngine must have bool Check(char c)");

public:
    // Buffer, size, parsing complete flag
    using Token = std::tuple<const char *, size_t, bool>;

    Tokenizer(const char *buf, size_t size, const MatchEngine &separator_me) :
        buf_(buf),
        size_(size),
        separator_me_(separator_me) {}

    // Returns next token, sets complete flag for the last one
    Token Next() {
        if (offset_ >= size_)
            throw std::runtime_error("Expected token not found");

        const char *ptr = buf_ + offset_;
        size_t size = 0;
        for (; separator_me_.Check(ptr[size]) != true; ++size) {
            if (offset_ + size > size_) {
                return { ptr, size, true };
            }
        }

        offset_ += (size + 1);

        return { ptr, size, false };
    }

    // Returns true if there will be more tokens to enjoy
    bool HasMore() const {
        return offset_ < size_;
    }

private:
    const char *buf_;
    size_t      size_;
    size_t      offset_ { 0 };

    const MatchEngine &separator_me_;
};

}