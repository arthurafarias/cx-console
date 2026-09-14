#pragma once

// Bitmask text-style flags (bold, italic, underline, ...) composable
// with bitwise operators, plus their SGR codes.

#include <string>
#include <vector>

namespace cx::console::chalk {

enum class text_style : unsigned {
    none = 0,
    bold = 1u << 0,
    dim = 1u << 1,
    italic = 1u << 2,
    underline = 1u << 3,
    inverse = 1u << 4,
    strikethrough = 1u << 5,
    hidden = 1u << 6,
};

constexpr text_style operator|(text_style a, text_style b) {
    return static_cast<text_style>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr text_style operator&(text_style a, text_style b) {
    return static_cast<text_style>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}

constexpr text_style& operator|=(text_style& a, text_style b) {
    a = a | b;
    return a;
}

constexpr bool has_style(text_style flags, text_style bit) {
    return static_cast<unsigned>(flags & bit) != 0;
}

namespace detail {

/// SGR "set" codes for each style bit, in a stable emission order.
inline std::vector<int> style_sgr_codes(text_style flags) {
    std::vector<int> codes;
    if (has_style(flags, text_style::bold)) codes.push_back(1);
    if (has_style(flags, text_style::dim)) codes.push_back(2);
    if (has_style(flags, text_style::italic)) codes.push_back(3);
    if (has_style(flags, text_style::underline)) codes.push_back(4);
    if (has_style(flags, text_style::inverse)) codes.push_back(7);
    if (has_style(flags, text_style::hidden)) codes.push_back(8);
    if (has_style(flags, text_style::strikethrough)) codes.push_back(9);
    return codes;
}

} // namespace detail

} // namespace cx::console::chalk
