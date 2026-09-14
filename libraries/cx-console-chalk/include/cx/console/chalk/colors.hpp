#pragma once

// Color model for cx-console-chalk: the 16 standard ANSI colors plus
// 256-color and 24-bit truecolor variants for foreground/background.

#include <cstdint>
#include <string>
#include <variant>

namespace cx::console::chalk {

/// The 16 standard ANSI terminal colors.
enum class color {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    gray, // bright black
    bright_red,
    bright_green,
    bright_yellow,
    bright_blue,
    bright_magenta,
    bright_cyan,
    bright_white,
};

/// An indexed 256-color (ANSI SGR 38;5;n / 48;5;n).
struct color256 {
    std::uint8_t index;
    explicit constexpr color256(std::uint8_t idx) : index(idx) {}
};

/// A 24-bit truecolor RGB triple (ANSI SGR 38;2;r;g;b / 48;2;r;g;b).
struct rgb {
    std::uint8_t r, g, b;
    constexpr rgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_) : r(r_), g(g_), b(b_) {}
};

/// Any of the supported color representations. `std::monostate` means
/// "no color set" (leave the terminal's default).
using color_value = std::variant<std::monostate, color, color256, rgb>;

namespace detail {

inline int standard_color_code(color c) {
    switch (c) {
        case color::black: return 30;
        case color::red: return 31;
        case color::green: return 32;
        case color::yellow: return 33;
        case color::blue: return 34;
        case color::magenta: return 35;
        case color::cyan: return 36;
        case color::white: return 37;
        case color::gray: return 90;
        case color::bright_red: return 91;
        case color::bright_green: return 92;
        case color::bright_yellow: return 93;
        case color::bright_blue: return 94;
        case color::bright_magenta: return 95;
        case color::bright_cyan: return 96;
        case color::bright_white: return 97;
    }
    return 39; // default
}

// Foreground base code -> background base code is +10 for the
// standard palette (30->40, 90->100).
inline int to_background_code(int fg_code) {
    return fg_code + 10;
}

/// Build the SGR parameter list (without the leading "\x1b[" / trailing "m")
/// for a color value used as foreground (`is_bg == false`) or background.
inline std::string sgr_params_for(const color_value& v, bool is_bg) {
    return std::visit(
        [&](auto&& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            } else if constexpr (std::is_same_v<T, color>) {
                int code = standard_color_code(value);
                if (is_bg) code = to_background_code(code);
                return std::to_string(code);
            } else if constexpr (std::is_same_v<T, color256>) {
                return (is_bg ? std::string("48;5;") : std::string("38;5;")) +
                       std::to_string(static_cast<int>(value.index));
            } else if constexpr (std::is_same_v<T, rgb>) {
                std::string prefix = is_bg ? "48;2;" : "38;2;";
                return prefix + std::to_string(value.r) + ";" + std::to_string(value.g) + ";" +
                       std::to_string(value.b);
            }
        },
        v);
}

} // namespace detail

} // namespace cx::console::chalk
