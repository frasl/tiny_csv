#pragma once

#include <tiny_csv/tokenizer.hpp>
#include <tiny_csv/type_loader.hpp>

#include <fmt/format.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>

namespace tiny_csv {

template<typename RowType, size_t ...IndexCols>
class TinyCSV {
    // Constraints


    // Types
    using DataVector = typename std::vector<RowType>;
    using ValueOffset = size_t;

public:
    template <typename RT, size_t col_idx> class Index {
        friend class TinyCSV<RowType, IndexCols...>;

        Index(std::shared_ptr<std::vector<RowType>> &data) : data_(data) {}
        Index() = delete;

    public:
        using ColType = typename std::tuple_element<col_idx, RowType>::type;
        using IndexMapType = typename std::unordered_multimap<ColType, ValueOffset>;
        using IndexMapConstIterator = typename IndexMapType::const_iterator;
        using IndexMapIteratorPair = typename std::pair<IndexMapConstIterator, IndexMapConstIterator>;

        class Iterator {
            friend class Index<RT, col_idx>;

            Iterator(IndexMapIteratorPair it_pair,
                     std::shared_ptr<DataVector> data) : cur_it_(it_pair.first), boundaries_(it_pair), data_(data) {}
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
                return (*data_)[offset];
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

            bool HasData() const {
                return cur_it_ != boundaries_.second;
            }

            size_t Size() const {
                return std::distance(boundaries_.first, boundaries_.second);
            }

        private:
            IndexMapConstIterator           cur_it_;
            IndexMapIteratorPair            boundaries_;
            std::shared_ptr<DataVector>     data_;
        };

        void Add(const RowType &row, ValueOffset offset) {
            idx_.insert({ std::get<col_idx>(row), offset });
        }

        Iterator Find(typename std::tuple_element<col_idx, RowType>::type key) const {
            return Iterator(idx_.equal_range(key), data_);
        }

        constexpr auto ColIdx() {
            return col_idx;
        }

    private:
        IndexMapType                    idx_;
        std::shared_ptr<DataVector>     data_;
    };

    // Iterate columns and parse according to types
    template <size_t C>
    void ParseCol(RowType &row) {
        using ColumnType = typename std::tuple_element<C, RowType>::type;
        field_tokenizer_.NextToken(token_buffer_);
        std::get<C>(row) = Loader<ColumnType>::Load(token_buffer_.Data(), token_buffer_.Size());
    }

    template <size_t C>
    void IterateAndParse(RowType &row) {
        if constexpr(C > 1)
            IterateAndParse<C - 1>(row);

        ParseCol<C - 1>(row);
    }
    // ===========================================================================

    void LineToRow(const char *data, size_t size, RowType &row) {
        field_tokenizer_.Reset(data, size);
        IterateAndParse<std::tuple_size_v<RowType>>(row);
    }

    template <size_t idx_id>
    void AppendIndices(const RowType &row, size_t offset) {
        if constexpr (idx_id > 1)
            AppendIndices<idx_id - 1>(row, offset);

        std::get<idx_id - 1>(indices_).Add(row, offset);
    }

    bool HeaderMatches(const char_t *ptr, size_t size) {
        field_tokenizer_.Reset(ptr, size);
        for (const auto &hdr_name: headers_) {
            field_tokenizer_.NextToken(token_buffer_);
            std::string hdr_in_file(token_buffer_.Data(), token_buffer_.Size());
            if (hdr_name != hdr_in_file)
                return false;
        }

        return true;
    }

public:
    using IndexTupleType = std::tuple<Index<RowType, IndexCols>...>;

    TinyCSV(const std::vector<std::string> &col_headers = {},
            char separator=',', char escape_char='\\', char quote='\"') :
        escape_char_(escape_char),
        quote_(quote),
        line_separators_( { '\n', '\t', '\0' } ),
        token_separators_( { separator, '\0' } ),
        line_tokenizer_(escape_char_, quote_,
                        token_separators_, line_separators_),
        field_tokenizer_(escape_char_, quote_,
                         token_separators_, line_separators_),
        headers_(col_headers),
        header_checked_( col_headers.empty() ),
        data_(std::make_shared<std::vector<RowType>>()),
        indices_ { Index<RowType, IndexCols>(data_)... } {}

    TinyCSV(TinyCSV<RowType, IndexCols...> &&from) :
        escape_char_(from.escape_char_),
        quote_(from.quote_),
        line_separators_( std::move(from.line_separators_) ),
        token_separators_( std::move(from.token_separators_) ),

        // We don't need to completely copy tokenizers and buffers
        line_tokenizer_(from.escape_char_, from.quote_,
                        token_separators_, line_separators_),
        field_tokenizer_(from.escape_char_, from.quote_,
                         token_separators_, line_separators_),
        headers_( std::move(from.headers_) ),
        data_(std::move(from.data_)),
        indices_(std::move(from.indices_))
    {
    }

    void Append(const RowType &row) {
        data_->push_back(row);
        AppendIndices<std::tuple_size_v<IndexTupleType>>(row, data_->size() - 1);
    }

    void Append(const char *data, size_t size) {
        line_tokenizer_.Reset(data, size);

        while (line_tokenizer_.HasMore()) {
            line_tokenizer_.NextLine(line_buffer_);
            if (header_checked_) [[likely]] {
                if (line_buffer_.Size() > 0) { // The line is not empty
                    RowType row;
                    LineToRow(line_buffer_.Data(), line_buffer_.Size(), row);
                    Append(row);
                }
            } else {
                if (!HeaderMatches(line_buffer_.Data(), line_buffer_.Size()))
                    throw std::runtime_error("Header does not match");

                header_checked_ = true;
            }
        }
    }

    const RowType &operator[](size_t idx) const {
        return data_[idx];
    }

    template<size_t idx_id, size_t IdxCol, size_t ...IdxCols>
    struct Index2ColIdTranslator {
        static constexpr size_t Index2ColId() {
            if constexpr (idx_id == 0) {
                return IdxCol;
            } else {
                return Index2ColIdTranslator<idx_id - 1, IdxCols...>::Index2ColId();
            }
        }
    };

    template<size_t idx_id>
    static constexpr size_t Idx2ColId() {
        return Index2ColIdTranslator<idx_id, IndexCols...>::Index2ColId();
    }


    /*
     * Returns an iterator to rows, that match the key passed
     */
    template <size_t idx_id>
    typename Index<RowType, Idx2ColId<idx_id>()>::Iterator Find(
            typename std::tuple_element<Idx2ColId<idx_id>(), RowType>::type key) {
        return std::get<idx_id>(indices_).Find(key);
    }

private:
    // Configuration
    const char escape_char_;
    const char quote_;

    // Maps index id to column id
    static constexpr std::tuple<size_t> index_col_ids_ { IndexCols... };

    // Separator character lookups
    MultiMatch                      line_separators_, token_separators_;

    TextBuffer<char_t>              line_buffer_;
    TextBuffer<char_t>              token_buffer_;
    Tokenizer                       line_tokenizer_;
    Tokenizer                       field_tokenizer_;
    const std::vector<std::string>  headers_;

    // Variables
    std::shared_ptr<DataVector>     data_;
    IndexTupleType                  indices_;
    bool                            header_checked_;
};

template<typename RowType, size_t ...IndexCols>
TinyCSV<RowType, IndexCols...> CreateFromFile(const std::string &filename,
                                              const std::vector<std::string> &col_headers = {} /* No check if empty */,
                                              char separator = ',', char escape_char = '\\',
                                              char quote_char = '\"') {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open())
        throw std::runtime_error(fmt::format("Cannot open {}", filename));

    // Determine the size of the file
    inputFile.seekg(0, std::ios::end);
    std::streamsize size = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // Allocate a buffer to hold the file contents
    std::vector<char> buffer(size);

    // Read the file into the buffer
    inputFile.read(buffer.data(), size);
    if (!inputFile.fail() || inputFile.eof()) {
        TinyCSV<RowType, IndexCols...> csv(col_headers, separator, escape_char, quote_char);
        csv.Append(buffer.data(), buffer.size());

        return csv;
    } else {
        throw std::runtime_error(fmt::format("Cannot read from {}", filename));
    }
}

} // namespace