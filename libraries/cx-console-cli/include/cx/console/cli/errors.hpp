#pragma once

// Click-equivalent user-facing parse errors. A usage_error's what() is
// the message body only (no "Error: " prefix and no trailing newline);
// command::run()/group::run() add the "Error: " prefix, style it red via
// chalk, print it to stderr, and return exit code 2.

#include <stdexcept>
#include <string>

namespace cx::console::cli {

class usage_error : public std::runtime_error {
  public:
    explicit usage_error(std::string message) : std::runtime_error(std::move(message)) {}
};

} // namespace cx::console::cli
