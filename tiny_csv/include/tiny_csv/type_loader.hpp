#pragma once

#include <tiny_csv/config.hpp>

#include <string>
#include <stdexcept>
#include <type_traits>
#include <charconv>
#include <iomanip>

namespace tiny_csv {

template<typename T, typename Enable = void>
struct Loader {
    static T Load(T& t, const char *buf, size_t len, const ParserConfig &cfg) {
        static_assert(std::is_void_v<T>, "Unsupported type");
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_integral_v<T>>> {
    static T Load(const char *buf, size_t len, const ParserConfig &cfg) {
        T value;
        std::from_chars(buf, buf + len, value);
        return value;
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static T Load(const char *buf, size_t len, const ParserConfig &cfg) {
        T value;
        std::from_chars(buf, buf + len, value);
        return value;
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_same_v<T, std::string>>> {
    static T Load(const char *buf, size_t len, const ParserConfig &cfg) {
        return std::string(buf, len);
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_same_v<T, std::tm>>> {
    static T Load(const char *buf, size_t len, const ParserConfig &cfg) {
        std::tm tm_time;
        std::string s(buf, len);
        std::istringstream date_ss(s);
        date_ss >> std::get_time(&tm_time, cfg.datetime_format.c_str());
        return tm_time;
    }
};

} // namespace