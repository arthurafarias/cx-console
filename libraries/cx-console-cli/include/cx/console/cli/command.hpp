#pragma once

// command: a single leaf CLI command -- name, help, arguments, options,
// and an action callback. Parses its own argv slice into a context and
// either runs the action or prints a Click-style error.

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <cxcore/events/event.hpp>

#include "cx/console/chalk.hpp"
#include "cx/console/cli/argument.hpp"
#include "cx/console/cli/context.hpp"
#include "cx/console/cli/errors.hpp"
#include "cx/console/cli/option.hpp"
#include "cx/console/cli/parser.hpp"

namespace cx::console::cli {

// Forward declaration; help.hpp includes command.hpp and defines these.
class command;
std::string render_help(const command& cmd, const std::string& invocation_path);

class command {
  public:
    explicit command(std::string name) : name_(std::move(name)) { add_help_option(); }

    command& help(std::string text) {
        help_ = std::move(text);
        return *this;
    }
    command& alias(std::string a) {
        aliases_.push_back(std::move(a));
        return *this;
    }
    command& aliases(std::vector<std::string> a) {
        aliases_ = std::move(a);
        return *this;
    }
    /// Allow trailing positional tokens beyond the declared arguments to
    /// be collected instead of raising "unexpected extra argument".
    command& allow_extra_args(bool value = true) {
        allow_extra_args_ = value;
        return *this;
    }

    template <class T>
    command& add_argument(argument<T> arg) {
        positionals_.push_back(std::make_unique<argument<T>>(std::move(arg)));
        return *this;
    }

    template <class T>
    command& add_option(option<T> opt) {
        options_.push_back(std::make_unique<option<T>>(std::move(opt)));
        return *this;
    }

    command& set_action(std::function<int(context&)> action) {
        action_ = std::move(action);
        return *this;
    }

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& help_text() const { return help_; }
    [[nodiscard]] const std::vector<std::string>& aliases() const { return aliases_; }
    [[nodiscard]] const std::vector<std::unique_ptr<argument_base>>& positionals() const { return positionals_; }
    [[nodiscard]] const std::vector<std::unique_ptr<option_base>>& options() const { return options_; }

    cx::core::event<const context&>& on_before_run() { return *before_run_; }
    cx::core::event<const context&, int>& on_after_run() { return *after_run_; }
    cx::core::event<const std::string&>& on_error() { return *error_; }

    [[nodiscard]] bool matches(std::string_view token) const {
        if (name_ == token) return true;
        for (const auto& a : aliases_) {
            if (a == token) return true;
        }
        return false;
    }

    /// Parse `args` and either run the action (returning its exit code)
    /// or print a parse error / help screen. `invocation_path` is the
    /// program+subcommand path shown in usage/help (e.g. "prog sub").
    int run(const std::vector<std::string>& args, const std::string& invocation_path) {
        if (std::ranges::find(args, "--help") != args.end() ||
            std::ranges::find(args, "-h") != args.end()) {
            std::cout << render_help(*this, invocation_path);
            return 0;
        }

        context ctx;
        try {
            detail::parse_arguments(args, options_, positionals_, ctx, allow_extra_args_);
        } catch (const usage_error& e) {
            error_->emit(e.what());
            print_usage_error(e.what());
            return 2;
        }

        if (ctx.has("help") && ctx.get<bool>("help")) {
            std::cout << render_help(*this, invocation_path);
            return 0;
        }

        if (!action_) {
            std::cout << render_help(*this, invocation_path);
            return 0;
        }

        try {
            before_run_->emit(ctx);
            int result = action_(ctx);
            after_run_->emit(ctx, result);
            return result;
        } catch (const std::exception& e) {
            error_->emit(e.what());
            print_runtime_error(e.what());
            return 1;
        } catch (...) {
            const std::string detail = "unknown error";
            error_->emit(detail);
            print_runtime_error(detail);
            return 1;
        }
    }

  private:
    void add_help_option() {
        option<bool> help_opt("help", "h");
        help_opt.help("Show this message and exit.").is_flag(true);
        options_.push_back(std::make_unique<option<bool>>(std::move(help_opt)));
    }

    static void print_usage_error(const std::string& detail) {
        std::cerr << chalk::red("Error: " + detail) << "\n";
    }
    static void print_runtime_error(const std::string& detail) {
        std::cerr << chalk::red("Error: " + detail) << "\n";
    }

    std::string name_;
    std::string help_;
    std::vector<std::string> aliases_;
    std::vector<std::unique_ptr<argument_base>> positionals_;
    std::vector<std::unique_ptr<option_base>> options_;
    std::function<int(context&)> action_;
    bool allow_extra_args_ = false;
    std::unique_ptr<cx::core::event<const context&>> before_run_ =
        std::make_unique<cx::core::event<const context&>>();
    std::unique_ptr<cx::core::event<const context&, int>> after_run_ =
        std::make_unique<cx::core::event<const context&, int>>();
    std::unique_ptr<cx::core::event<const std::string&>> error_ =
        std::make_unique<cx::core::event<const std::string&>>();
};

} // namespace cx::console::cli
