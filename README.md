# tiny_csv

## Disclaimer

This is currently no more than a code example, published to demonstrate some C++ capabilities of the author, hopefully turning into a small hobby project later

## Purpose

A template-based C++ library, that would load a CSV file, and provide indexed access to its data. 
Out there exists half a dozen fine libraries to read CSVs, two differentiators of this one are:

- Any row is processed as std::tuple (well, a class, derived from std::tuple)
- User can create indices for any columns desired. For now, std::unordered_multimap is used as an engine
- No type translation (like string -> int) happens during data access, everything happens on load
- User can provide custom loader classes for whatever tricky formats are used. Otherwise, standard, type-based loaders will be used


## Performance

It wasn't "polished" for the most optimal performance, still, you can expect an adequate one. 
A test was made, using 202 MB csv file, 10 columns, all optional, 3 of them integer - a fragment of a library check-out list.
Additionally, indexes over 4 columns were created (year-month-day-title).
- 1 thread, no index: 153 MB/sec
- 8 threads, no index: 324 MB/sec
- 1 thread, index enabled: 87 MB/sec
- 8 threads, index enabled: 150 MB/sec
Tested with Lenovo Legion laptop, Ryzen 7/SSD/Win 11


## How to

-- less bullshit, more examples:

### Load a file the most simple way and scroll around:
```c++
using Columns = 
    tiny_csv::ColTuple<Type0,...,TypeN>;
auto csv =
tiny_csv::CreateFromFile<Columns>("your_file.csv",
{"Column_Name0", ... , "Column_NameN"});

for (const auto &row: csv) {
    std::cout << std::get<0>(row) << " " << ... << "std::get<N>(row)" << std::endl;
}
```

Note, that if csv has a header row, you'll need to provide a string vector for the checkup. 

Easy, huh? 
There's also operator [] and NMatches() functions defined. 

### Make Indices and Search

```c++
using Columns = 
    tiny_csv::ColTuple<Type0,...,TypeN>;
auto csv =
tiny_csv::CreateFromFile<Columns, Columns::DefaultLoaders,
    // Here we enumerate all column ids we want indices on
    ColId0,..., ColIdM 
    >("your_file.csv",
{"Column_Name0", ... , "Column_NameN"});

auto it = csv.Find<Index #>(Some your Value); 
while (it.HasData()) {
    std::cout << std::get<0>(*it) << " " << ... << "std::get<N>(*it)" << std::endl;
    ++it;
}

```
Still not too violent, huh? 

### Make My Own Value Loaders
```c++
// First, define a loader like this one:
struct CustomIntLoader {
    static int Load(const char *buf, size_t len, const tiny_csv::ParserConfig &cfg) {
        int val;
        std::from_chars(buf, buf + len, val);
        return -val; // To distinguish value from normal
    }
};

// Then, feed it either to CreateFromFile or to template params:
tiny_csv::TinyCSV<Columns,
    // Loaders come as a tuple
    std::tuple<tiny_csv::Loader<int> /* Use standard loaded for column 1 */, 
               CustomIntLoader /* Our magnificent loader */>> csv;
    // Then proceed as usual
```

### MultiThreading

If you need to squeeze all the juice available, the library offers a multi-threaded version of CreateFromFile:
```c++
/***
 * Loads CSV from file using multiple threads.
 * Note: the function has some noticeable overheads, so don't use with small files
 * @tparam RowType - a tuple of columns, must be a ColTuple template instance
 * @tparam Loaders - an std::tuple of Loader classes (see readme for details)
 * @tparam IndexCols - list of columns to build indices on
 * @param filename - source file name
 * @param col_headers - a vector of strings, containing expected column names
 * @param cfg - parser configuration, see structure for details
 * num_threads - amount of threads to use during loading. 
 * @return
 */
template <typename RowType, typename Loaders = typename RowType::DefaultLoaders, size_t ...IndexCols>
TinyCSV<RowType, Loaders, IndexCols...> CreateFromFileMT(const std::string &filename,
                                                         const std::vector<std::string> &col_headers = {} /* No check if empty */,
                                                         const ParserConfig &cfg = {},
                                                         size_t num_threads = 4);
```


