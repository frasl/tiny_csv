#pragma once

#include <string>
#include <stdexcept>
#include <type_traits>
#include <charconv>

namespace tiny_csv {

// Integer and floating point loaders
struct Loader {
    // Converter function for integral and float values
    template<typename T>
    typename std::enable_if_t<std::is_integral_v<T> ||
                              std::is_floating_point_v<T>>::type Load(const char *buf, size_t len) const {
        T value;
        std::from_chars(buf, buf + len, value);
        return value;
    }

    // Converter function for strings
    template<typename T>
    typename std::enable_if_t<std::is_same<T, std::string>::value>::type Load(const char *buf, size_t len) const {
        return std::string(buf, len);
    }
};

} // namespace