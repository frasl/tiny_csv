#pragma once

#include <tiny_csv/tokenizer.hpp>
#include <tiny_csv/type_loader.hpp>

#include <fmt/format.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <tuple>

namespace tiny_csv {

template<typename... ColType>
struct ColTuple : public std::tuple<ColType...>
{
    using ColTypesTuple = typename std::tuple<ColType...>;
    using DefaultLoaders = typename std::tuple<Loader<ColType>...>;

    static constexpr size_t num_columns = std::tuple_size_v<ColTypesTuple>;
};

template<typename ColumnTypeTuple,
        typename RowLoaders = typename ColumnTypeTuple::DefaultLoaders,
        size_t ...IndexCols>
class TinyCSV {
    // ============ Constraints ====================
    // Ensure ColumnTypeTuple is an instance of ColTuple template
    template <typename> struct IsColumnTypeTuple: std::false_type {};
    template <typename ...T> struct IsColumnTypeTuple<ColTuple<T...>>: std::true_type {};

    static_assert(IsColumnTypeTuple<ColumnTypeTuple>::value,
                  "ColumnTypeTuple parameter should be an instance of ColTuple template");

    // Check Index values
    template <size_t idx_id, size_t ...Cols>
    struct CheckIndexCols {
        static constexpr bool is_ok = (idx_id < ColumnTypeTuple::num_columns && idx_id >= 0) &&
                                       CheckIndexCols<Cols...>::is_ok;
    };
    template<size_t idx_id>
    struct CheckIndexCols<idx_id> {
        static constexpr bool is_ok = idx_id < ColumnTypeTuple::num_columns && idx_id >= 0;
    };

    // ToDo: make a fold expression out of this mess
    static_assert(CheckIndexCols<0, IndexCols...>::is_ok, "Index column Id out of range");

    // ============ Types ====================
    using DataVector = typename std::vector<ColumnTypeTuple>;
    using ValueOffset = size_t;

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

public:

    /***
     * An internal class, represents an index for a specific column.
     * Currently based on unordered_multimap, todo: allow used to have a choice between unique/non-unique index
     * @tparam col_idx
     */
    template <size_t col_idx> class Index {
        friend class TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...>;

        Index(std::shared_ptr<std::vector<ColumnTypeTuple>> &data) : data_(data) {}
        Index() = delete;

    public:
        using ColType = typename std::tuple_element<col_idx, typename ColumnTypeTuple::ColTypesTuple>::type;
        using IndexMapType = typename std::unordered_multimap<ColType, ValueOffset>;
        using IndexMapConstIterator = typename IndexMapType::const_iterator;
        using IndexMapIteratorPair = typename std::pair<IndexMapConstIterator, IndexMapConstIterator>;

        /***
         * An iterator class
         *
         */
        class Iterator {
            friend class Index<col_idx>;

            Iterator(IndexMapIteratorPair it_pair,
                     std::shared_ptr<DataVector> data) : cur_it_(it_pair.first), boundaries_(it_pair), data_(data) {}
            Iterator() = delete;

        public:
            Iterator& operator++() { ++cur_it_; return *this; }
            Iterator operator++(int) { Iterator tmp(*this); ++(*this); return tmp; }

            ColumnTypeTuple operator *() const {
                ValueOffset offset = cur_it_->second;
                return (*data_)[offset];
            }

            const ColumnTypeTuple *operator->() const {
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

        void Add(const ColumnTypeTuple &row, ValueOffset offset) {
            idx_.insert({ std::get<col_idx>(row), offset });
        }

        Iterator Find(const typename std::tuple_element<col_idx, typename ColumnTypeTuple::ColTypesTuple>::type &key) const;
        constexpr auto ColIdx();

    private:
        IndexMapType                    idx_;
        std::shared_ptr<DataVector>     data_;
    };

private:
    // ============ Private methods ====================
    // Iterate columns and parse according to types

    /***
     * Parses a column, using a corresponding Loader as specified in TinyCSV template params,
     * or a default loader otherwise
     * @tparam C - column index
     * @param row - output row
     */
    template <size_t C>
    void ParseCol(ColumnTypeTuple &row);

    /***
     * Walks throught all columns, calling ParseCol for each of them
     * @tparam num_cols - total number of columns
     * @param row - output row
     */
    template <size_t num_cols>
    void IterateAndParse(ColumnTypeTuple &row);
    // ===========================================================================

    /***
     * Translates a line of text into a parsed row
     * @param data - pointer to data
     * @param size - size, bytes
     * @param row - output row
     */
    void LineToRow(const char *data, size_t size, ColumnTypeTuple &row);

    /***
     * Recursive template function, appends index data for a given row
     * @tparam idx_cnt - amount of indices left
     * @param row - raw to index
     * @param offset - row offset
     */
    template <size_t idx_cnt>
    void AppendIndices(const ColumnTypeTuple &row, size_t offset);

    /***
     * Check if header names match
     * @param ptr - pointer to the first row of text in csv
     * @param size - size of the row, bytes
     * @return true if everything is ok
     */
    bool HeaderMatches(const char_t *ptr, size_t size);

    /***
     * Translates index id into column id
     * @tparam idx_id - index id
     * @return column id
     */
    template<size_t idx_id>
    static constexpr size_t Idx2ColId() { return Index2ColIdTranslator<idx_id, IndexCols...>::Index2ColId(); }


public:
    using IndexTupleType = std::tuple<Index<IndexCols>...>;

    /***
     * Constructs a TinyCSV object
     * @param cfg - see ParserConfig for a complete description
     * @param col_headers - a vector of column header names. If it is empty, header will not be expected. Otherwise,
     * column names will be checked to match
     */
    TinyCSV(const ParserConfig &cfg = {}, const std::vector<std::string> &col_headers = {});

    // Move constructor
    TinyCSV(TinyCSV<ColumnTypeTuple, RowLoaders, IndexCols...> &&from);

    /***
     * Appends a row, including index update
     * @param row
     */
    void Append(const ColumnTypeTuple &row);

    /***
     * Appends a block of data to be parsed and added. Note: it is expected that the block is a valid CSV (no partial
     * lines allowed for now)
     * @param data - pointer to data
     * @param size - size, bytes
     */
    void Append(const char *data, size_t size);

    // Basic vector-like access
    /***
     * @return const_iterator, pointing to the start of the data vector
     */
    typename std::vector<ColumnTypeTuple>::const_iterator begin() const;

    /***
     * @return const_iterator, pointing to the end of the data vector
     */
    typename std::vector<ColumnTypeTuple>::const_iterator end() const;

    /***
     * Index operator, providing access to csv data
     * @param idx index to access
     * @return a const reference to a row
     */
    const ColumnTypeTuple &operator[](size_t idx) const;

    // For C-style access
    /***
     * Direct access to the data vector
     * @return pointer to the first element
     */
    const ColumnTypeTuple *Data() const;

    /***
     * Size of loaded CSV
     * @return amount of rows
     */
    size_t Size() const;

    /***
     * Finds all rows, matching passed key
     * @tparam idx_id
     * @param key
     * @return an iterator of type Index<typename ColumnTypeTuple::ColTypesTuple, Idx2ColId<idx_id>()>::Iterator
     * Note: each column has its own iterator type
     * Note2: use HasData() method of the iterator, to check if there is data left
     */
    template <size_t idx_id>
    auto Find(const
         typename std::tuple_element<Idx2ColId<idx_id>(), const typename ColumnTypeTuple::ColTypesTuple>::type &key) const;

private:
    // Configuration
    const ParserConfig cfg_;

    // Maps index id to column id
    static constexpr std::tuple<size_t> index_col_ids_ { IndexCols... };

    // Text buffers and tokenizers
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

/***
 * Loads CSV from file
 * @tparam RowType - a tuple of columns, must be a ColTuple template instance
 * @tparam Loaders - an std::tuple of Loader classes (see readme for details)
 * @tparam IndexCols - list of columns to build indices on
 * @param filename - source file name
 * @param col_headers - a vector of strings, containing expected column names
 * @param cfg - parser configuration, see structure for details
 * @return
 */
template<typename RowType, typename Loaders = typename RowType::DefaultLoaders, size_t ...IndexCols>
TinyCSV<RowType, Loaders, IndexCols...> CreateFromFile(const std::string &filename,
                                                       const std::vector<std::string> &col_headers = {} /* No check if empty */,
                                                       const ParserConfig &cfg = {});

} // namespace

#include <tiny_csv/tiny_csv.ipp>