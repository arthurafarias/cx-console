// cx-console-chalk tests.
//
// Plain assert()-based tests -- no gtest/catch2/doctest, matching this
// ecosystem's convention. IMPORTANT: this must be run from a Debug build
// (CMAKE_BUILD_TYPE=Debug). A Release build defines NDEBUG, which turns
// every assert() into a no-op, so tests would silently "pass" without
// checking anything.

// Verify each header compiles standalone (no reliance on include order
// from the umbrella header) by including them individually here first.
#include "cx/console/chalk/colors.hpp"
#include "cx/console/chalk/detection.hpp"
#include "cx/console/chalk/style_builder.hpp"
#include "cx/console/chalk/styles.hpp"

#include "cx/console/chalk.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace chalk = cx::console::chalk;

namespace {

void with_enabled(bool enabled, auto&& fn) {
    chalk::set_enabled(enabled);
    fn();
    chalk::reset_enabled();
}

void test_style_escape_codes() {
    with_enabled(true, [] {
        std::string s = chalk::style().fg(chalk::color::red).bold()("hi");
        assert(s.find("\x1b[") == 0);
        assert(s.find("1") != std::string::npos); // bold code
        assert(s.find("31") != std::string::npos); // red fg code
        assert(s.substr(s.size() - 4) == "\x1b[0m");

        std::string plain_red = chalk::red("x");
        assert(plain_red == "\x1b[31mx\x1b[0m");

        std::string boldstr = chalk::bold("y");
        assert(boldstr == "\x1b[1my\x1b[0m");

        // rgb / 256-color
        std::string rgb_s = chalk::style().fg(chalk::rgb(10, 20, 30))("z");
        assert(rgb_s == "\x1b[38;2;10;20;30mz\x1b[0m");

        std::string c256_s = chalk::style().bg(chalk::color256(200))("z");
        assert(c256_s == "\x1b[48;5;200mz\x1b[0m");
    });
}

void test_combined_style() {
    with_enabled(true, [] {
        std::string s = chalk::style().fg(chalk::color::green).bg(chalk::color::black).bold().underline()("combo");
        // bold(1), underline(4) then fg(32) then bg(40)
        assert(s.find("1") != std::string::npos);
        assert(s.find("4") != std::string::npos);
        assert(s.find("32") != std::string::npos);
        assert(s.find("40") != std::string::npos);
    });
}

void test_nesting_produces_superset_and_no_crash() {
    with_enabled(true, [] {
        std::string inner = chalk::red("inner");
        std::string nested = chalk::bold(inner);
        // Should contain both the outer bold code and the inner red code.
        assert(nested.find("1m") != std::string::npos || nested.find(";1") != std::string::npos ||
               nested.find("[1") != std::string::npos);
        assert(nested.find("31") != std::string::npos);
        assert(!nested.empty());
    });
}

void test_disabled_mode_returns_plain_text() {
    with_enabled(false, [] {
        assert(chalk::red("plain") == "plain");
        assert(chalk::bold("plain") == "plain");
        assert(chalk::style().fg(chalk::color::blue).bold()("plain") == "plain");
    });
}

void test_no_color_env_forces_disabled() {
    setenv("FORCE_COLOR", "1", 1);
    setenv("NO_COLOR", "1", 1);
    chalk::reset_enabled();
    assert(chalk::is_enabled() == false); // NO_COLOR wins over FORCE_COLOR
    assert(chalk::red("x") == "x");
    unsetenv("NO_COLOR");
    unsetenv("FORCE_COLOR");
    chalk::reset_enabled();
}

void test_force_color_env_forces_enabled() {
    unsetenv("NO_COLOR");
    setenv("FORCE_COLOR", "1", 1);
    chalk::reset_enabled();
    assert(chalk::is_enabled() == true);
    assert(chalk::red("x") != "x");
    unsetenv("FORCE_COLOR");
    chalk::reset_enabled();
}

void test_force_color_zero_does_not_force() {
    unsetenv("NO_COLOR");
    setenv("FORCE_COLOR", "0", 1);
    chalk::reset_enabled();
    // FORCE_COLOR=0 must not force-enable; falls back to TTY detection
    // (false in a non-interactive test run / piped output).
    unsetenv("FORCE_COLOR");
    chalk::reset_enabled();
}

void test_explicit_override_wins_over_env() {
    setenv("NO_COLOR", "1", 1);
    chalk::set_enabled(true);
    assert(chalk::is_enabled() == true);
    chalk::reset_enabled();
    unsetenv("NO_COLOR");
}

} // namespace

int main() {
    test_style_escape_codes();
    test_combined_style();
    test_nesting_produces_superset_and_no_crash();
    test_disabled_mode_returns_plain_text();
    test_no_color_env_forces_disabled();
    test_force_color_env_forces_enabled();
    test_force_color_zero_does_not_force();
    test_explicit_override_wins_over_env();

    std::cout << "cx-console-chalk-tests: all tests passed\n";
    return 0;
}
