#pragma once

#include <cctype>
#include <string>
#include <vector>

namespace tiny_csv {

/***
 * Loads file contents into buffer
 * @param filename - file name
 * @param buffer - vector to hold the buffer
 * @return - size of the file loaded
 */
size_t Load(const std::string &filename, std::vector<char> &buffer);

}