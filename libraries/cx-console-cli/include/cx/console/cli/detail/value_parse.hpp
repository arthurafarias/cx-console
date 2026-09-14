#pragma once

// Internal: string -> T parsing for the types cx-console-cli supports as
// argument/option values, plus a small human-readable type-name table
// used for [TYPE] hints in generated help and "not a valid X" errors.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cx::console::cli::detail {

/// Thrown by parse_value<T> on a malformed input. Carries only the
/// Click-style detail fragment (e.g. "'abc' is not a valid integer.");
/// callers wrap it with the "Invalid value for '...'" prefix.
struct value_parse_error {
    std::string detail;
};

template <class T>
struct type_traits; // specialized below for each supported T

template <class T>
std::string type_name() {
    return type_traits<T>::name;
}

template <class T>
T parse_value(std::string_view raw) {
    return type_traits<T>::parse(raw);
}

inline std::string lowercase(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

template <>
struct type_traits<std::string> {
    static constexpr const char* name = "TEXT";
    static std::string parse(std::string_view raw) { return std::string(raw); }
};

template <>
struct type_traits<int> {
    static constexpr const char* name = "INTEGER";
    static int parse(std::string_view raw) {
        try {
            std::size_t pos = 0;
            std::string s(raw);
            int v = std::stoi(s, &pos);
            if (pos != s.size()) throw std::invalid_argument("trailing");
            return v;
        } catch (...) {
            throw value_parse_error{"'" + std::string(raw) + "' is not a valid integer."};
        }
    }
};

template <>
struct type_traits<long> {
    static constexpr const char* name = "INTEGER";
    static long parse(std::string_view raw) {
        try {
            std::size_t pos = 0;
            std::string s(raw);
            long v = std::stol(s, &pos);
            if (pos != s.size()) throw std::invalid_argument("trailing");
            return v;
        } catch (...) {
            throw value_parse_error{"'" + std::string(raw) + "' is not a valid integer."};
        }
    }
};

template <>
struct type_traits<double> {
    static constexpr const char* name = "FLOAT";
    static double parse(std::string_view raw) {
        try {
            std::size_t pos = 0;
            std::string s(raw);
            double v = std::stod(s, &pos);
            if (pos != s.size()) throw std::invalid_argument("trailing");
            return v;
        } catch (...) {
            throw value_parse_error{"'" + std::string(raw) + "' is not a valid float."};
        }
    }
};

template <>
struct type_traits<bool> {
    static constexpr const char* name = "BOOLEAN";
    static bool parse(std::string_view raw) {
        std::string s = lowercase(std::string(raw));
        if (s == "1" || s == "true" || s == "yes" || s == "y" || s == "on") return true;
        if (s == "0" || s == "false" || s == "no" || s == "n" || s == "off") return false;
        throw value_parse_error{"'" + std::string(raw) + "' is not a valid boolean."};
    }
};

template <>
struct type_traits<std::filesystem::path> {
    static constexpr const char* name = "PATH";
    static std::filesystem::path parse(std::string_view raw) { return std::filesystem::path(raw); }
};

} // namespace cx::console::cli::detail
