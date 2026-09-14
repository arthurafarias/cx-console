#pragma once

// TTY / NO_COLOR / FORCE_COLOR detection and an explicit override switch.
//
// Precedence (highest first):
//   1. explicit set_enabled(bool) override, if any
//   2. NO_COLOR env var set (to anything) -> disabled
//   3. FORCE_COLOR env var set to a non-empty, non-"0" value -> enabled
//   4. isatty(stdout) -> enabled, else disabled

#include <cstdlib>
#include <cstring>
#include <optional>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#define CX_CONSOLE_CHALK_HAS_ISATTY 1
#else
#define CX_CONSOLE_CHALK_HAS_ISATTY 0
#endif

namespace cx::console::chalk {

namespace detail {

inline std::optional<bool>& enabled_override() {
    static std::optional<bool> value;
    return value;
}

inline bool env_var_set(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0';
}

inline bool force_color_requested() {
    const char* v = std::getenv("FORCE_COLOR");
    return v != nullptr && v[0] != '\0' && std::strcmp(v, "0") != 0;
}

inline bool stdout_is_tty() {
#if CX_CONSOLE_CHALK_HAS_ISATTY
    return ::isatty(fileno(stdout)) != 0;
#else
    return false;
#endif
}

} // namespace detail

/// Force-enable or force-disable styling, overriding all auto-detection.
inline void set_enabled(bool enabled) {
    detail::enabled_override() = enabled;
}

/// Remove any explicit override, reverting to NO_COLOR/FORCE_COLOR/TTY detection.
inline void reset_enabled() {
    detail::enabled_override().reset();
}

/// Whether styling output is currently enabled.
inline bool is_enabled() {
    if (detail::enabled_override().has_value()) {
        return *detail::enabled_override();
    }
    if (detail::env_var_set("NO_COLOR")) {
        return false;
    }
    if (detail::force_color_requested()) {
        return true;
    }
    return detail::stdout_is_tty();
}

} // namespace cx::console::chalk
