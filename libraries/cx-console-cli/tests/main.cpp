#include <cx/console/cli.hpp>

#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace cli = cx::console::cli;

int main() {
  cli::context context;
  context.set("count", 7, true);
  context.set("path", std::filesystem::path("output.txt"), false);
  context.set("names", std::vector<std::string>{"one", "two"}, true);
  assert(context.get<int>("count") == 7);
  assert(context.get<std::filesystem::path>("path") == "output.txt");
  assert((context.get<std::vector<std::string>>("names") ==
          std::vector<std::string>{"one", "two"}));
  assert(context.was_provided("count"));
  assert(!context.was_provided("path"));

  cli::command command("run");
  command.add_option(cli::option<int>("count", "c").required());
  bool before = false;
  bool after = false;
  command.on_before_run() += [&](const cli::context &ctx) {
    before = ctx.get<int>("count") == 3;
  };
  command.on_after_run() += [&](const cli::context &, int result) {
    after = result == 9;
  };
  command.set_action([](cli::context &ctx) { return ctx.get<int>("count") * 3; });
  assert(command.run({"--count", "3"}, "tool run") == 9);
  assert(before && after);

  std::ostringstream help_output;
  auto *old_buffer = std::cout.rdbuf(help_output.rdbuf());
  assert(command.run({"--help"}, "tool run") == 0);
  std::cout.rdbuf(old_buffer);
  assert(help_output.str().find("Usage: tool run") != std::string::npos);
}
