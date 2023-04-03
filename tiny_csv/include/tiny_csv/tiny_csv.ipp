#pragma once

#include <tiny_csv/utility.hpp>

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
header_checked_( from.header_checked_ ),
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
void TINY_CSV::Append(const std::vector<TinyCSV<ColumnTypeTuple, Loaders>> &objs, size_t num_threads) {
    size_t total_size = 0;
    for (const auto &obj: objs)
        total_size += obj.Size();

    size_t base_index = Size(), current_index = Size();
    data_->resize(current_index + total_size);


    {
        // Offset to copy to, source object
        using ArgsT = std::pair<size_t, const TinyCSV<ColumnTypeTuple, Loaders> &>;
        TaskQueue<ArgsT> copy_tasks(num_threads);

        // Schedule the copy
        for (const auto &obj: objs) {
            copy_tasks.Enqueue([this](const ArgsT &args) {
                std::copy(args.second.begin(), args.second.end(), data_->begin() + args.first);
            }, { current_index, obj });
            current_index += obj.Size();
        }
    }

    // Build indices
    {
        using ArgsT = std::pair<size_t, size_t>;
        TaskQueue<ArgsT> index_tasks(num_threads);

        AppendIndicesMT<std::tuple_size_v<IndexTupleType>>(base_index, total_size, index_tasks);
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


template<typename ColumnTypeTuple, typename Loaders, size_t... IndexCols>
template<size_t idx_cnt>
void TINY_CSV::AppendIndices(const ColumnTypeTuple &row, size_t offset) {
    if constexpr (idx_cnt > 1)
        AppendIndices<idx_cnt - 1>(row, offset);

    if constexpr (idx_cnt > 0)
        std::get<idx_cnt - 1>(indices_).Add(row, offset);
}

template<typename ColumnTypeTuple, typename Loaders, size_t... IndexCols>
template <size_t idx_cnt>
void TINY_CSV::AppendIndicesMT(size_t start_idx, size_t n_elems,
                               TaskQueue<std::pair<size_t, size_t>> &queue) {
    if constexpr (idx_cnt > 1)
        AppendIndicesMT<idx_cnt - 1>(start_idx, n_elems, queue);

    if constexpr (idx_cnt > 0) {
        queue.Enqueue([this](const std::pair<size_t, size_t> &args) {
            for (size_t i = args.first; i < args.first + args.second; ++i) {
                const ColumnTypeTuple &row = (*data_)[i];
                std::get<idx_cnt - 1>(indices_).Add(row, i);
            }
        }, { start_idx, n_elems });
    }
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
    std::vector<char> buffer;
    Load(filename, buffer);
    TinyCSV<ColumnTypeTuple, Loaders, IndexCols...> csv(cfg, col_headers);
    csv.Append(buffer.data(), buffer.size());
    return csv;
}

template <typename ColumnTypeTuple, typename Loaders, size_t ...IndexCols>
TINY_CSV CreateFromFileMT(const std::string &filename,
                          const std::vector<std::string> &col_headers,
                          const ParserConfig &cfg,
                          size_t num_threads) {
    static const size_t READ_SIZE = 1024 * 1024;

    if (num_threads == 0)
        throw std::logic_error("num_threads must be > 0");

    std::vector<TinyCSV<ColumnTypeTuple, Loaders>> parts;
    // First part checks the headers
    parts.emplace_back(TinyCSV<ColumnTypeTuple, Loaders>(cfg, col_headers));

    for (size_t i = 1; i < num_threads; ++i)
        parts.emplace_back(TinyCSV<ColumnTypeTuple, Loaders>(cfg, {}));

    std::vector<char_t> buffer;
    size_t size = Load(filename, buffer);

    // Now we need to approximately divide the file into portions
    std::vector<size_t> offsets;
    static const MultiMatch line_separators({ '\n', '\r', '\0' });
    offsets.push_back(0);
    for (size_t i = 1; i < num_threads; ++i)
    {
        size_t starting_offset = offsets[i - 1] + size / num_threads;
        while (!line_separators.Check(buffer[starting_offset]) && starting_offset < size) {
            ++starting_offset;
        }

        if (starting_offset < size)
            offsets.push_back(starting_offset);
    }
    offsets.push_back(size);

    // Fire the engines!
    {
        struct Args {
            size_t fragment_begin, fragment_end;
            size_t part_idx;
        };
        TaskQueue <Args> task_queue(num_threads);

        for (size_t i = 1; i < num_threads; ++i) {
            Args args{offsets[i - 1], offsets[i], i - 1};

            task_queue.Enqueue([&parts, &buffer](const Args &args) {
                parts[args.part_idx].Append(buffer.data() + args.fragment_begin,
                                            args.fragment_end - args.fragment_begin);
            }, args);
        }
    }

    // And merge/index
    TinyCSV<ColumnTypeTuple, Loaders, IndexCols...> csv(cfg, col_headers);
    csv.Append(parts, num_threads);

    return csv;
}

}