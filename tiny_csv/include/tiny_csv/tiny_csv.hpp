#pragma once

#include <tiny_csv/tokenizer.hpp>
#include <tiny_csv/type_loader.hpp>

#include <unordered_map>
#include <string>
#include <vector>

namespace tiny_csv {

template<typename RowType, size_t ...IndexCols>
class TinyCSV {
    // Constraints


    // Types
    using ValueOffset = size_t;

public:
    template <typename RT, size_t col_idx> class Index {
        friend class TinyCSV<RowType, IndexCols...>;

        Index(const std::vector<RowType> &data) : data_(data) {}
        Index() = delete;

    public:
        using ColType = typename std::tuple_element<col_idx, RowType>::type;
        using IndexMapType = typename std::unordered_multimap<ColType, ValueOffset>;

        class Iterator {
            friend class Index<RT, col_idx>;

            Iterator(typename IndexMapType::const_iterator it,
                     const std::vector<RowType> &data) : cur_it_(it), data_(data) {}
            Iterator() = delete;

        public:
            Iterator& operator++() {
                ++cur_it_;
                return *this;
            }

            // Postfix increment operator
            Iterator operator++(int) {
                Iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            // Dereference operator
            RowType operator *() const {
                ValueOffset offset = cur_it_->second;
                return std::get<col_idx>(data_[offset]);
            }

            // Pointer operator
            const RowType *operator->() const {
                ValueOffset offset = cur_it_->second;
                return &std::get<col_idx>(data_[offset]);
            }

            // Equality comparison operator
            bool operator==(const Iterator& other) const {
                return cur_it_ == other.cur_it_;
            }

            // Inequality comparison operator
            bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

        private:
            typename IndexMapType::const_iterator cur_it_;
            const std::vector<RowType> &          data_;
        };

        void Add(const RowType &row, ValueOffset offset) {
            idx_.insert({ std::get<col_idx>(row), offset });
        }

        Iterator Find(const std::tuple_element<col_idx, RowType> &key) const {
            return Iterator(idx_.equal_range(key), data_);
        }

        Iterator End() const {
            return Iterator(idx_.end(), data_);
        }

        constexpr auto ColIdx() {
            return col_idx;
        }

    private:
        IndexMapType                    idx_;
        const std::vector<RowType> &    data_;
    };

    // Iterate columns and parse according to types
    template <size_t C>
    void ParseCol(Tokenizer &tokenizer, RowType &row) {
        using ColumnType = typename std::tuple_element<C, RowType>::type;
        Loader ldr;
        auto token = tokenizer.Next();
        std::get<C>(row) = ldr.Load<ColumnType>(std::get<0>(token), std::get<1>(token));
    }

    template <size_t C>
    void IterateAndParse(Tokenizer &tokenizer, RowType &row) {
        if (C > 0)
            IterateAndParse<C - 1>(tokenizer, row);

        ParseCol<C - 1>(tokenizer, row);
    }
    // ===========================================================================

public:
    using IndexTupleType = std::tuple<Index<RowType, IndexCols>...>;

    // Do not read column headers
    TinyCSV(char separator=',', char escape_char='\\', char quote='\"') :
        separator_(separator),
        escape_char_(escape_char),
        quote_(quote) {
    }

    TinyCSV(const std::vector<std::string> &col_headers,
            char separator=',', char escape_char='\\', char quote='\"') :
        separator_(separator),
        escape_char_(escape_char),
        quote_(quote) {
    }

    void Append(const RowType &row) {
        data_.push_back(row);

        // Add new row to all indices
        for (auto &idx: indices_) {
            idx.Add(row, data_.size() - 1);
        }
    }

    void LineToRow(const char *data, size_t size, RowType &row) {
        Tokenizer tokenizer(data, size, separator);
        IterateAndParse<std::tuple_size_v<RowType>>(tokenizer, row);
    }

    void Append(const char *data, size_t size) {
        MultiMatch<char, '\0', '\n', '\r'> separators;
        Tokenizer tokenizer(data, size, separators);
        while (tokenizer.HasMore()) {
            auto rowToken = tokenizer.Next();
            RowType row;
            LineToRow(std::get<0>(rowToken), std::get<1>(rowToken), separator_);
            Append(row);
        }
    }

    template <size_t idx_id>
    typename std::tuple_element<idx_id, IndexTupleType>::type::Iterator Test() const {
        return std::get<idx_id>(indices_).End();
    }

private:
    // Configuration
    const char separator_;
    const char escape_char_;
    const char quote_;

    std::vector<RowType>    data_;
    IndexTupleType          indices_ { Index<RowType, IndexCols>(data_)... };
};

} // namespace