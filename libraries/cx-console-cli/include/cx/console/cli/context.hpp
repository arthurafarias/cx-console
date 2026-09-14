#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <cxcore/containers/object.hpp>

namespace cx::console::cli {

class context_error : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

namespace detail {

template <class> inline constexpr bool unsupported_context_type = false;

template <class T> cx::core::variant context_encode(T value) {
  using value_type = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<value_type, bool> ||
                std::is_same_v<value_type, std::string> ||
                std::is_same_v<value_type, double>) {
    return cx::core::variant(std::move(value));
  } else if constexpr (std::is_same_v<value_type, std::filesystem::path>) {
    return cx::core::variant(value.string());
  } else if constexpr (std::is_integral_v<value_type> && std::is_signed_v<value_type>) {
    return cx::core::variant(std::int64_t(value));
  } else if constexpr (std::is_integral_v<value_type>) {
    return cx::core::variant(std::uint64_t(value));
  } else if constexpr (requires { typename value_type::value_type; } &&
                       std::is_same_v<value_type, std::vector<typename value_type::value_type>>) {
    std::deque<cx::core::variant> result;
    for (const auto &item : value)
      result.push_back(context_encode<typename value_type::value_type>(item));
    return cx::core::variant(std::move(result));
  } else {
    static_assert(unsupported_context_type<T>, "unsupported CLI context type");
  }
}

template <class T> T context_decode(const cx::core::variant &value) {
  using value_type = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<value_type, bool> ||
                std::is_same_v<value_type, std::string> ||
                std::is_same_v<value_type, double>) {
    return std::get<value_type>(value);
  } else if constexpr (std::is_same_v<value_type, std::filesystem::path>) {
    return std::filesystem::path(std::get<std::string>(value));
  } else if constexpr (std::is_integral_v<value_type> && std::is_signed_v<value_type>) {
    return static_cast<value_type>(std::get<std::int64_t>(value));
  } else if constexpr (std::is_integral_v<value_type>) {
    return static_cast<value_type>(std::get<std::uint64_t>(value));
  } else if constexpr (requires { typename value_type::value_type; } &&
                       std::is_same_v<value_type, std::vector<typename value_type::value_type>>) {
    value_type result;
    for (const auto &item : std::get<std::deque<cx::core::variant>>(value))
      result.push_back(context_decode<typename value_type::value_type>(item));
    return result;
  } else {
    static_assert(unsupported_context_type<T>, "unsupported CLI context type");
  }
}

} // namespace detail

class context {
public:
  template <class T> [[nodiscard]] T get(std::string_view name) const {
    auto value = values_.property_get_variant(std::string(name));
    if (!value) {
      std::ostringstream message;
      message << "cx::console::cli::context::get: no value stored for '" << name << "'";
      throw context_error(message.str());
    }
    return detail::context_decode<T>(*value);
  }

  [[nodiscard]] bool has(std::string_view name) const {
    return values_.property_get_variant(std::string(name)).has_value();
  }

  [[nodiscard]] bool was_provided(std::string_view name) const {
    return provided_.contains(std::string(name));
  }

  [[nodiscard]] const std::vector<std::string> &leftover_args() const { return leftover_; }

  template <class T> void set(std::string_view name, T value, bool explicitly_provided) {
    values_.property_set(std::string(name), detail::context_encode(std::move(value)));
    if (explicitly_provided) provided_.insert(std::string(name));
  }

  void add_leftover(std::string arg) { leftover_.push_back(std::move(arg)); }

  cx::core::object &values() noexcept { return values_; }
  const cx::core::object &values() const noexcept { return values_; }

private:
  cx::core::object values_;
  std::unordered_set<std::string> provided_;
  std::vector<std::string> leftover_;
};

} // namespace cx::console::cli
