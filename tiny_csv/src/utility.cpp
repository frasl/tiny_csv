#include "tiny_csv/utility.hpp"

#include <fmt/format.h>

#include <fstream>


size_t tiny_csv::Load(const std::string &filename, std::vector<char> &buffer) {
    static const size_t READ_SIZE = 1024 * 1024;

    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile.is_open())
        throw std::runtime_error(fmt::format("Cannot open {}", filename));

    // Determine the size of the file
    inputFile.seekg(0, std::ios::end);
    std::streamsize size = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    buffer.clear();
    buffer.resize(size);

    // Allocate a buffer to hold the file contents
    size_t total_read = 0;
    while (total_read < size) {
        const size_t to_read = std::min(size - total_read, READ_SIZE);
        inputFile.read(buffer.data() + total_read, to_read);
        if (inputFile.fail() && !inputFile.eof()) {
            throw std::runtime_error(fmt::format("Cannot read from {}", filename));
        }
        total_read += to_read;
    }

    return size;
}
