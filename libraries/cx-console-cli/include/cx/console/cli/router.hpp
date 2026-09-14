#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cx/console/cli/group.hpp"

namespace cx::console::cli {

// Transport-neutral registry of the operations exposed by an application.
// A router owns the command metadata and controller callbacks; views only
// decide how that registry is presented and invoked.
class router {
public:
  explicit router(std::string name) : routes_(std::move(name)) {}

  router &help(std::string text) {
    routes_.help(std::move(text));
    return *this;
  }

  command &add(command route) { return routes_.add_command(std::move(route)); }
  group &add(group route_group) {
    return routes_.add_group(std::move(route_group));
  }

  [[nodiscard]] const std::string &name() const { return routes_.name(); }
  [[nodiscard]] const std::string &help_text() const {
    return routes_.help_text();
  }
  [[nodiscard]] const std::vector<std::unique_ptr<command>> &commands() const {
    return routes_.commands();
  }
  [[nodiscard]] const std::vector<std::unique_ptr<group>> &groups() const {
    return routes_.subgroups();
  }

private:
  friend class view;
  group routes_;
};

} // namespace cx::console::cli
