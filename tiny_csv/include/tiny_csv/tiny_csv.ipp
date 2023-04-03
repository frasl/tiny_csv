#pragma once

#include <cctype>

#define TINY_CSV    TinyCSV<ColumnTypeTuple, Loaders, IndexCols...>

namespace tiny_csv {

// =============================================================================
// TinyCSV class members

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
TINY_CSV::TinyCSV(const ParserConfig &cfg,
        const std::vector<std::string> &col_headers) :
        cfg_(cfg),
        headers_(col_headers),
        line_tokenizer_(cfg),
        field_tokenizer_(cfg),
        header_checked_( col_headers.empty() ),
        data_(std::make_shared<std::vector<ColumnTypeTuple>>()),
        indices_ { Index<IndexCols>(data_)... }
{}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
TINY_CSV::TinyCSV(tiny_csv::TinyCSV<ColumnTypeTuple, Loaders, IndexCols...> &&from) :
cfg_(std::move(from.cfg_)),

// We don't need to completely copy tokenizers and buffers
line_tokenizer_(from.cfg_),
field_tokenizer_(from.cfg_),
headers_( std::move(from.headers_) ),
data_(std::move(from.data_)),
indices_(std::move(from.indices_)) {}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
size_t TINY_CSV::Size() const {
    return data_->size();
}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
const ColumnTypeTuple *TINY_CSV::Data() const {
    
}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
template <size_t idx_id>
auto TINY_CSV::Find(const typename std::tuple_element<Idx2ColId<idx_id>(), const typename ColumnTypeTuple::ColTypesTuple>::type &key) const {
    return std::get<idx_id>(indices_).Find(key);
}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
void TINY_CSV::Append(const ColumnTypeTuple &row) {
    data_->push_back(row);
    AppendIndices<std::tuple_size_v<IndexTupleType>>(row, data_->size() - 1);
}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
void TINY_CSV::Append(const char *data, size_t size) {
    size_t line = 0;
    line_tokenizer_.Reset(data, size);

    try {
        while (line_tokenizer_.HasMore()) {
            line_tokenizer_.NextLine(line_buffer_);
            ++line;

            if (header_checked_) [[likely]] {
                if (line_buffer_.Size() > 0) { // The line is not empty
                    ColumnTypeTuple row;
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
    catch (const Tokenizer::ParsingException &e) {
        throw std::runtime_error(fmt::format("Error at line {}: {}", line, e.what()));
    }
}

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
typename std::vector<ColumnTypeTuple>::const_iterator TINY_CSV::begin() const { return data_->begin(); }

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
typename std::vector<ColumnTypeTuple>::const_iterator TINY_CSV::end() const { return data_->end(); }

template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
const ColumnTypeTuple & TINY_CSV::operator[](size_t idx) const { return (*data_)[idx]; }

// ================================================================================
// TinyCSV::Index class members
template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
template<size_t col_idx>
constexpr auto TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::Index<col_idx>::ColIdx() {
    return col_idx;
}

template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
void TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::LineToRow(
        const char *data, size_t size, ColumnTypeTuple &row) {
    field_tokenizer_.Reset(data, size);
    IterateAndParse<std::tuple_size_v<typename ColumnTypeTuple::ColTypesTuple>>(row);
}

template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
template<size_t NumCols>
void TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::IterateAndParse(ColumnTypeTuple &row) {
    if constexpr (NumCols > 1)
        IterateAndParse<NumCols - 1>(row);

    ParseCol<NumCols - 1>(row);
}

template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
template<size_t C>
void TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::ParseCol(ColumnTypeTuple &row) {
    try {
        field_tokenizer_.NextToken(token_buffer_);
        std::get<C>(row) =
                std::tuple_element<C, RowLoaders>::type::Load(token_buffer_.Data(), token_buffer_.Size(), cfg_);
    } catch (const Tokenizer::ParsingException &e) {
        throw std::runtime_error(fmt::format("Column parsing exception: {} during parsing of '{}'", e.what(),
                                             field_tokenizer_.DebugPrint()));
    }
}


template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
template<size_t idx_cnt>
void TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::AppendIndices(const ColumnTypeTuple &row, size_t offset) {
    if constexpr (idx_cnt > 1)
        AppendIndices<idx_cnt - 1>(row, offset);

    if constexpr (idx_cnt > 0)
        std::get<idx_cnt - 1>(indices_).Add(row, offset);
}

template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
bool TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::HeaderMatches(const char_t *ptr, size_t size) {
    field_tokenizer_.Reset(ptr, size);
    for (const auto &hdr_name : headers_) {
        field_tokenizer_.NextToken(token_buffer_);
        std::string hdr_in_file(token_buffer_.Data(), token_buffer_.Size());
        if (hdr_name != hdr_in_file)
            return false;
    }

    return true;
}

template<typename ColumnTypeTuple, typename RowLoaders, size_t... IndexCols>
template<size_t col_idx>
typename TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::template Index<col_idx>::Iterator
TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>::template Index<col_idx>::Find(
        const typename std::tuple_element<col_idx, typename ColumnTypeTuple::ColTypesTuple>::type &key) const {
    return Iterator(idx_.equal_range(key), data_);
}
// ================================================================================

// File loader
template<typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
TINY_CSV CreateFromFile(const std::string &filename,
                        const std::vector<std::string> &col_headers,
                        const ParserConfig &cfg) {
    static const size_t READ_SIZE = 1024 * 1024;

    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile.is_open())
        throw std::runtime_error(fmt::format("Cannot open {}", filename));

    // Determine the size of the file
    inputFile.seekg(0, std::ios::end);
    std::streamsize size = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // Allocate a buffer to hold the file contents
    std::vector<char> buffer(size);
    TinyCSV<ColumnTypeTuple, Loaders, IndexCols...> csv(cfg, col_headers);
    size_t total_read = 0;
    while (total_read < size) {
        const size_t to_read = std::min(size - total_read, READ_SIZE);
        inputFile.read(buffer.data() + total_read, to_read);
        if (inputFile.fail() && !inputFile.eof()) {
            throw std::runtime_error(fmt::format("Cannot read from {}", filename));
        }
        total_read += to_read;
    }

    csv.Append(buffer.data(), buffer.size());
    return csv;
}


}