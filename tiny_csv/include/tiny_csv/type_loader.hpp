#pragma once

#include <string>
#include <stdexcept>
#include <type_traits>
#include <charconv>

namespace tiny_csv {

template<typename T, typename Enable = void>
struct Loader {
    static T Load(T& t, const char *buf, size_t len) {
        static_assert(std::is_void_v<T>, "Unsupported type");
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_integral_v<T>>> {
    static T Load(const char *buf, size_t len) {
        T value;
        std::from_chars(buf, buf + len, value);
        return value;
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static T Load(const char *buf, size_t len) {
        T value;
        std::from_chars(buf, buf + len, value);
        return value;
    }
};

template<typename T>
struct Loader<T, std::enable_if_t<std::is_same_v<T, std::string>>> {
    static T Load(const char *buf, size_t len) {
        return std::string(buf, len);
    }
};


} // namespace