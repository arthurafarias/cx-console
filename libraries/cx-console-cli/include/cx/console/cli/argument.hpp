#pragma once

// Positional arguments: argument<T> plus the type-erased argument_base
// used internally by command to store a heterogeneous list of them.

#include <algorithm>
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

/// Type-erased base so `command` can hold a heterogeneous list of
/// `argument<T>` for different T.
class argument_base {
  public:
    virtual ~argument_base() = default;

    [[nodiscard]] virtual const std::string& name() const = 0;
    [[nodiscard]] virtual const std::string& help_text() const = 0;
    [[nodiscard]] virtual bool is_required() const = 0;
    [[nodiscard]] virtual bool is_variadic() const = 0;
    [[nodiscard]] virtual bool has_default() const = 0;
    [[nodiscard]] virtual std::string type_hint() const = 0;
    [[nodiscard]] virtual std::string default_display() const = 0;
    [[nodiscard]] virtual const std::vector<std::string>& allowed_choices() const = 0;

    /// Store this argument's default value into the context (used when the
    /// argument was not provided on the command line and is not required).
    virtual void store_default(context& ctx) const = 0;

    /// Parse and store a single raw token (non-variadic argument).
    virtual void store_one(context& ctx, std::string_view raw) const = 0;

    /// Parse and store a batch of raw tokens as a vector (variadic argument).
    virtual void store_many(context& ctx, const std::vector<std::string>& raws) const = 0;

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
class argument : public argument_base {
  public:
    explicit argument(std::string name) : name_(std::move(name)) {}

    argument& help(std::string text) {
        help_ = std::move(text);
        return *this;
    }
    argument& required(bool value = true) {
        required_ = value;
        return *this;
    }
    argument& default_value(T value) {
        default_ = std::move(value);
        required_ = false;
        return *this;
    }
    argument& variadic(bool value = true) {
        variadic_ = value;
        return *this;
    }
    /// Restrict accepted values to a fixed set (compared as strings).
    argument& choices(std::vector<std::string> allowed) {
        choices_ = std::move(allowed);
        return *this;
    }

    [[nodiscard]] const std::string& name() const override { return name_; }
    [[nodiscard]] const std::string& help_text() const override { return help_; }
    [[nodiscard]] bool is_required() const override { return required_ && !default_.has_value(); }
    [[nodiscard]] bool is_variadic() const override { return variadic_; }
    [[nodiscard]] bool has_default() const override { return default_.has_value(); }
    [[nodiscard]] const std::vector<std::string>& allowed_choices() const override { return choices_; }

    [[nodiscard]] std::string type_hint() const override {
        return variadic_ ? (detail::type_name<T>() + std::string("...")) : detail::type_name<T>();
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
        if (default_.has_value()) {
            ctx.set<T>(name_, *default_, false);
        }
    }

    void store_one(context& ctx, std::string_view raw) const override {
        ctx.set<T>(name_, parse_checked(raw), true);
    }

    void store_many(context& ctx, const std::vector<std::string>& raws) const override {
        std::vector<T> values;
        values.reserve(raws.size());
        for (const auto& raw : raws) values.push_back(parse_checked(raw));
        ctx.set<std::vector<T>>(name_, std::move(values), true);
    }

  private:
    [[nodiscard]] T parse_checked(std::string_view raw) const {
        if (!choices_.empty()) {
            if (std::ranges::find(choices_, std::string(raw)) == choices_.end()) {
                throw usage_error("Invalid value for '" + name_ + "': " + format_choice_error(raw, choices_));
            }
        }
        try {
            return detail::parse_value<T>(raw);
        } catch (const detail::value_parse_error& e) {
            throw usage_error("Invalid value for '" + name_ + "': " + e.detail);
        }
    }

    std::string name_;
    std::string help_;
    bool required_ = true;
    bool variadic_ = false;
    std::optional<T> default_;
    std::vector<std::string> choices_;
};

} // namespace cx::console::cli
