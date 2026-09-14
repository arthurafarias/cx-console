#pragma once

// The styled_text builder: chalk::style().fg(color::red).bold()("text")
// plus the ergonomic free functions (chalk::red(s), chalk::bold(s), ...).
//
// Note: nested calls like chalk::bold(chalk::red(s)) each independently
// wrap-then-reset (\x1b[0m at the end of every call), unlike JS chalk's
// smarter "reopen parent codes after an inner reset" behavior. This is a
// documented simplification -- nesting still looks correct visually
// (the inner reset just also clears the outer style for the remainder of
// the string, but since the outer wrap re-applies its own codes around
// the whole inner result, the visible output is correct); it just means
// the emitted escape sequences aren't maximally minimal.

#include <string>
#include <string_view>
#include <vector>

#include "cx/console/chalk/colors.hpp"
#include "cx/console/chalk/detection.hpp"
#include "cx/console/chalk/styles.hpp"

namespace cx::console::chalk {

class style_builder {
  public:
    style_builder() = default;

    style_builder& fg(color_value c) {
        fg_ = c;
        return *this;
    }
    style_builder& bg(color_value c) {
        bg_ = c;
        return *this;
    }

    style_builder& add_style(text_style s) {
        style_ |= s;
        return *this;
    }

    style_builder& bold() { return add_style(text_style::bold); }
    style_builder& dim() { return add_style(text_style::dim); }
    style_builder& italic() { return add_style(text_style::italic); }
    style_builder& underline() { return add_style(text_style::underline); }
    style_builder& inverse() { return add_style(text_style::inverse); }
    style_builder& strikethrough() { return add_style(text_style::strikethrough); }
    style_builder& hidden() { return add_style(text_style::hidden); }

    /// The raw ANSI "set" escape sequence for the accumulated fg/bg/style
    /// (no reset). Empty if nothing was set or styling is disabled.
    [[nodiscard]] std::string escape_prefix() const {
        if (!is_enabled()) return {};

        std::vector<std::string> params;
        for (int code : detail::style_sgr_codes(style_)) {
            params.push_back(std::to_string(code));
        }
        if (std::string fg_params = detail::sgr_params_for(fg_, false); !fg_params.empty()) {
            params.push_back(fg_params);
        }
        if (std::string bg_params = detail::sgr_params_for(bg_, true); !bg_params.empty()) {
            params.push_back(bg_params);
        }
        if (params.empty()) return {};

        std::string out = "\x1b[";
        for (std::size_t i = 0; i < params.size(); ++i) {
            if (i != 0) out += ';';
            out += params[i];
        }
        out += 'm';
        return out;
    }

    /// Apply the accumulated style to `text`, returning the ANSI-wrapped
    /// result (or the plain text unchanged if styling is disabled).
    [[nodiscard]] std::string operator()(std::string_view text) const {
        std::string prefix = escape_prefix();
        if (prefix.empty()) {
            return std::string(text);
        }
        std::string out;
        out.reserve(prefix.size() + text.size() + 4);
        out += prefix;
        out += text;
        out += "\x1b[0m";
        return out;
    }

  private:
    color_value fg_{};
    color_value bg_{};
    text_style style_{text_style::none};
};

/// Start building a styled_text combination: chalk::style().fg(...).bold()(text)
inline style_builder style() {
    return style_builder{};
}

namespace detail {
inline std::string apply_color(color c, std::string_view text) {
    return style_builder{}.fg(c)(text);
}
inline std::string apply_style(text_style s, std::string_view text) {
    return style_builder{}.add_style(s)(text);
}
} // namespace detail

inline std::string black(std::string_view s) { return detail::apply_color(color::black, s); }
inline std::string red(std::string_view s) { return detail::apply_color(color::red, s); }
inline std::string green(std::string_view s) { return detail::apply_color(color::green, s); }
inline std::string yellow(std::string_view s) { return detail::apply_color(color::yellow, s); }
inline std::string blue(std::string_view s) { return detail::apply_color(color::blue, s); }
inline std::string magenta(std::string_view s) { return detail::apply_color(color::magenta, s); }
inline std::string cyan(std::string_view s) { return detail::apply_color(color::cyan, s); }
inline std::string white(std::string_view s) { return detail::apply_color(color::white, s); }
inline std::string gray(std::string_view s) { return detail::apply_color(color::gray, s); }

inline std::string bold(std::string_view s) { return detail::apply_style(text_style::bold, s); }
inline std::string dim(std::string_view s) { return detail::apply_style(text_style::dim, s); }
inline std::string italic(std::string_view s) { return detail::apply_style(text_style::italic, s); }
inline std::string underline(std::string_view s) {
    return detail::apply_style(text_style::underline, s);
}
inline std::string inverse(std::string_view s) {
    return detail::apply_style(text_style::inverse, s);
}
inline std::string strikethrough(std::string_view s) {
    return detail::apply_style(text_style::strikethrough, s);
}

} // namespace cx::console::chalk
