#pragma once

#include <vector>

namespace tiny_csv {

/*
 * A multi-use buffer, designed to minimize reallocation during parsing
 */
template <typename T>
class TextBuffer {
public:
    TextBuffer(size_t reserve = 0) {
        data_.resize(reserve);
    }

    void PushBack(T c) {
        if (used_ + 1 >= data_.size())
            data_.resize((data_.size() + 1) * 2); // ToDo: Might need to invent something less aggressive

        data_[used_++] = c;
    }

    void Clear() {
        used_ = 0;
    }

    const T *Data() const {
        return data_.data();
    }

    const size_t Size() const {
        return used_;
    }

private:
    std::vector<T>  data_;
    size_t          used_ { 0 };
};

}