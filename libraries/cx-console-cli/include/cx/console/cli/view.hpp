#pragma once

#include <functional>
#include <vector>

#include "cx/console/cli/router.hpp"

namespace cx::console::cli {

// CLI adapter for a router. Other front ends can consume the same public
// route metadata without coupling application controllers to argv/stdout.
class view {
public:
  explicit view(router &routes) : routes_(routes) {}

  int run(int argc, char **argv) {
    return routes_.get().routes_.run(argc, argv);
  }

  int run(const std::vector<std::string> &args) {
    return routes_.get().routes_.run(args, routes_.get().name());
  }

  [[nodiscard]] router &routes() const { return routes_.get(); }

private:
  std::reference_wrapper<router> routes_;
};

} // namespace cx::console::cli
