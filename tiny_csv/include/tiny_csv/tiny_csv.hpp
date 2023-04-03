#pragma once

/***
 * Main file to be included, has all necessary interface definitions.
 * For usage examples see README.md
 */

#include <tiny_csv/tokenizer.hpp>
#include <tiny_csv/type_loader.hpp>
#include <tiny_csv/task_queue.hpp>

#include <fmt/format.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <utility>
#include <tuple>

namespace tiny_csv {

/***
 * A type to use as a wrapper for column types. Did you go see examples in README.md?
 * @tparam ColType - A set of column types
 */
template<typename... ColType>
struct ColTuple : public std::tuple<ColType...>
{
    using ColTypesTuple = typename std::tuple<ColType...>;
    using DefaultLoaders = typename std::tuple<Loader<ColType>...>;

    static constexpr size_t num_columns = std::tuple_size_v<ColTypesTuple>;
};

/***
 * Class, representing a CSV file
 * @tparam ColumnTypeTuple - an instance of ColTuple, holding column types
 * @tparam ValueLoaders - an std::tuple of Loader types. If left default, will be deduced by type as seen in type_loader.hpp
 * If you neeed custom loaders, please see same file for inspiration (and README.md)
 * @tparam IndexCols - A set of column ids to make indices on.
 * Example: "0, 2, 3" will mean that index 0 will be for the column 0, index 1 will be for the column 2, index 2 - for column 3
 * You would know this by now, if you read the README.md
 */
template<typename ColumnTypeTuple,
        typename ValueLoaders = typename ColumnTypeTuple::DefaultLoaders,
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
    // Note: 0 is added to deal with the case when no IndexCols specified
    static_assert(CheckIndexCols<0, IndexCols...>::is_ok, "Index column Id out of range");

    // Check the Loaders
    template <size_t col_id>
    struct CheckLoader {
        using Loader = typename std::tuple_element<col_id, ValueLoaders>::type;
        using LoadedType = typename std::tuple_element<col_id, typename ColumnTypeTuple::ColTypesTuple>::type;

        static constexpr bool is_ok =
                std::is_same_v<decltype(Loader::Load), LoadedType((const char *, size_t, const ParserConfig &))>;
    };

    template <size_t n_loaders>
    static constexpr bool CheckLoaders() {
        if constexpr(n_loaders == 0) {
            return true;
        } else {
            return CheckLoader<n_loaders - 1>::is_ok && CheckLoaders<n_loaders - 1>();
        }
    }

    static_assert(CheckLoaders<std::tuple_size_v<ValueLoaders>>(), "Value loader signature(s) do not match, see type_loader.hpp for inspiration");


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
        friend class TinyCSV<ColumnTypeTuple, ValueLoaders, IndexCols...>;

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

            /***
             * @return true if the iterator holds a value. HasData() == false is basically equivalent to it == end() in STL
             */
            bool HasData() const {
                return cur_it_ != boundaries_.second;
            }

            /***
             * @return Amount of elements, matching the search criteria
             */
            size_t NMatches() const {
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
    void LineToRow(const char_t *data, size_t size, ColumnTypeTuple &row);

    /***
     * Recursive template function, appends index data for a given row
     * @tparam idx_cnt - amount of indices left
     * @param row - raw to index
     * @param offset - row offset
     */
    template <size_t idx_cnt>
    void AppendIndices(const ColumnTypeTuple &row, size_t offset);

    /***
     * Recursive template function, appends index data for a given row - in parallel threads
     * @tparam idx_cnt - amount of indices left
     * @param start_idx - starting index in data_ to add to indices
     * @param n_elems - amount of elements to be processed
     * @param queue - a task queue, that will handle the job
     */
    template <size_t idx_cnt>
    void AppendIndicesMT(size_t start_idx, size_t n_elems,
                         TaskQueue<std::pair<size_t, size_t>> &queue);

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
    TinyCSV(TinyCSV<ColumnTypeTuple, ValueLoaders, IndexCols...> &&from);

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
    void Append(const char_t *data, size_t size);

    /***
     * Appends a vector of un-indexed CSVs into one indexed CSV.
     * Used for multi-threaded loads.
     * @param objs - a vector of objects to merge
     */
    void Append(const std::vector<TinyCSV<ColumnTypeTuple, ValueLoaders>> &objs, size_t num_threads);

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
     * NMatches of loaded CSV
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

/***
 * Loads CSV from file using multiple threads
 * @tparam RowType - a tuple of columns, must be a ColTuple template instance
 * @tparam Loaders - an std::tuple of Loader classes (see readme for details)
 * @tparam IndexCols - list of columns to build indices on
 * @param filename - source file name
 * @param col_headers - a vector of strings, containing expected column names
 * @param cfg - parser configuration, see structure for details
 * @return
 */
template <typename RowType, typename Loaders = typename RowType::DefaultLoaders, size_t ...IndexCols>
TinyCSV<RowType, Loaders, IndexCols...> CreateFromFileMT(const std::string &filename,
                                                         const std::vector<std::string> &col_headers = {} /* No check if empty */,
                                                         const ParserConfig &cfg = {},
                                                         size_t num_threads = 4);


} // namespace

#include <tiny_csv/tiny_csv.ipp>