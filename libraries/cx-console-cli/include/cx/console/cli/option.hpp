#pragma once

// Options: --long-name/-x with a typed value, flags, counting flags,
// multiple(), required(), default_value(), env_var() fallback.

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "cx/console/cli/context.hpp"
#include "cx/console/cli/detail/value_parse.hpp"
#include "cx/console/cli/errors.hpp"

namespace cx::console::cli {

class option_base {
  public:
    virtual ~option_base() = default;

    [[nodiscard]] virtual const std::string& long_name() const = 0;   // without "--"
    [[nodiscard]] virtual const std::string& short_name() const = 0;  // without "-", may be empty
    [[nodiscard]] virtual const std::string& help_text() const = 0;
    [[nodiscard]] virtual bool is_required() const = 0;
    [[nodiscard]] virtual bool is_flag() const = 0;
    [[nodiscard]] virtual bool is_counting() const = 0;
    [[nodiscard]] virtual bool is_multiple() const = 0;
    [[nodiscard]] virtual bool has_default() const = 0;
    [[nodiscard]] virtual std::string type_hint() const = 0;
    [[nodiscard]] virtual std::string default_display() const = 0;
    [[nodiscard]] virtual const std::string& env_var() const = 0;
    [[nodiscard]] virtual const std::vector<std::string>& allowed_choices() const = 0;

    virtual void store_default(context& ctx) const = 0;
    virtual void store_flag(context& ctx) const = 0;
    virtual void store_count(context& ctx, int count) const = 0;
    virtual void store_one(context& ctx, std::string_view raw) const = 0;
    virtual void append_one(context& ctx, std::string_view raw) const = 0;
    /// Parse the option's env var fallback value (only called when
    /// env_var() is non-empty and set in the environment, and the option
    /// was not supplied on the command line).
    virtual void store_from_env(context& ctx, std::string_view raw) const = 0;

  protected:
    [[nodiscard]] static std::string format_choice_error(std::string_view raw,
                                                           const std::vector<std::string>& choices) {
        std::string msg = "'" + std::string(raw) + "' is not one of ";
        for (std::size_t i = 0; i < choices.size(); ++i) {
            if (i != 0) msg += ", ";
            msg += "'" + choices[i] + "'";
        }
        msg += ".";
        return msg;
    }
};

template <class T>
class option : public option_base {
  public:
    explicit option(std::string long_name, std::string short_name = {})
        : long_name_(std::move(long_name)), short_name_(std::move(short_name)) {}

    option& help(std::string text) {
        help_ = std::move(text);
        return *this;
    }
    option& required(bool value = true) {
        required_ = value;
        return *this;
    }
    option& default_value(T value) {
        default_ = std::move(value);
        return *this;
    }
    option& env_var(std::string name) {
        env_var_ = std::move(name);
        return *this;
    }
    /// Boolean presence flag: no value is consumed, presence -> true.
    option& is_flag(bool value = true) {
        flag_ = value;
        return *this;
    }
    /// Counting flag (e.g. -vvv -> 3). Implies is_flag-like parsing (no
    /// value token consumed); each occurrence increments the stored count.
    option& counting(bool value = true) {
        counting_ = value;
        return *this;
    }
    /// Repeatable option: collects every occurrence into a vector<T>.
    option& multiple(bool value = true) {
        multiple_ = value;
        return *this;
    }
    option& choices(std::vector<std::string> allowed) {
        choices_ = std::move(allowed);
        return *this;
    }

    [[nodiscard]] const std::string& long_name() const override { return long_name_; }
    [[nodiscard]] const std::string& short_name() const override { return short_name_; }
    [[nodiscard]] const std::string& help_text() const override { return help_; }
    [[nodiscard]] bool is_required() const override { return required_; }
    [[nodiscard]] bool is_flag() const override { return flag_; }
    [[nodiscard]] bool is_counting() const override { return counting_; }
    [[nodiscard]] bool is_multiple() const override { return multiple_; }
    [[nodiscard]] bool has_default() const override { return default_.has_value(); }
    [[nodiscard]] const std::string& env_var() const override { return env_var_; }
    [[nodiscard]] const std::vector<std::string>& allowed_choices() const override { return choices_; }

    [[nodiscard]] std::string type_hint() const override {
        if (flag_) return {};
        if (counting_) return {};
        std::string hint = detail::type_name<T>();
        if (multiple_) hint += "...";
        return hint;
    }

    [[nodiscard]] std::string default_display() const override {
        if (!default_.has_value()) return {};
        if constexpr (std::is_same_v<T, std::string>) {
            return *default_;
        } else if constexpr (std::is_same_v<T, bool>) {
            return *default_ ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
            return default_->string();
        } else {
            return std::to_string(*default_);
        }
    }

    void store_default(context& ctx) const override {
        if (flag_) {
            bool default_val = false;
            if constexpr (std::is_same_v<T, bool>) {
                if (default_.has_value()) default_val = *default_;
            }
            ctx.set<bool>(long_name_, default_val, false);
        } else if (counting_) {
            ctx.set<int>(long_name_, 0, false);
        } else if (multiple_) {
            ctx.set<std::vector<T>>(long_name_, default_.has_value() ? std::vector<T>{*default_} : std::vector<T>{}, false);
        } else if (default_.has_value()) {
            ctx.set<T>(long_name_, *default_, false);
        }
    }

    void store_flag(context& ctx) const override { ctx.set<bool>(long_name_, true, true); }

    void store_count(context& ctx, int count) const override { ctx.set<int>(long_name_, count, true); }

    void store_one(context& ctx, std::string_view raw) const override {
        ctx.set<T>(long_name_, parse_checked(raw), true);
    }

    void append_one(context& ctx, std::string_view raw) const override {
        std::vector<T> values;
        if (ctx.has(long_name_)) {
            values = ctx.get<std::vector<T>>(long_name_);
        }
        values.push_back(parse_checked(raw));
        ctx.set<std::vector<T>>(long_name_, std::move(values), true);
    }

    void store_from_env(context& ctx, std::string_view raw) const override {
        if (multiple_) {
            ctx.set<std::vector<T>>(long_name_, std::vector<T>{parse_checked(raw)}, true);
        } else {
            ctx.set<T>(long_name_, parse_checked(raw), true);
        }
    }

  private:
    [[nodiscard]] T parse_checked(std::string_view raw) const {
        if (!choices_.empty()) {
            if (std::ranges::find(choices_, std::string(raw)) == choices_.end()) {
                throw usage_error("Invalid value for '--" + long_name_ + "': " + format_choice_error(raw, choices_));
            }
        }
        try {
            return detail::parse_value<T>(raw);
        } catch (const detail::value_parse_error& e) {
            throw usage_error("Invalid value for '--" + long_name_ + "': " + e.detail);
        }
    }

    std::string long_name_;
    std::string short_name_;
    std::string help_;
    bool required_ = false;
    bool flag_ = false;
    bool counting_ = false;
    bool multiple_ = false;
    std::optional<T> default_;
    std::string env_var_;
    std::vector<std::string> choices_;
};

} // namespace cx::console::cli
