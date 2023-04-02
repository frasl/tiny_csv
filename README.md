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

It wasn't "polished" for the most optimal performance, still, you can expect an adequate one. A row/sec perf test will be added later.

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
There's also operator [] and Size() functions defined. 

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




