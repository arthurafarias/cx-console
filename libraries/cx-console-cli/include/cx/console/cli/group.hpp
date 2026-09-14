#pragma once

// group: a command that dispatches to nested subcommands (commands or
// further nested groups), with alias resolution and optional command
// chaining (chain(true): multiple subcommand names may appear in one
// invocation and each runs in sequence).

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cx/console/chalk.hpp"
#include "cx/console/cli/command.hpp"

namespace cx::console::cli {

class group;
std::string render_help(const group& grp, const std::string& invocation_path);
std::string generate_completion_script(const group& grp, std::string_view shell);

class group {
  public:
    explicit group(std::string name) : name_(std::move(name)) {}

    group& help(std::string text) {
        help_ = std::move(text);
        return *this;
    }
    group& alias(std::string a) {
        aliases_.push_back(std::move(a));
        return *this;
    }
    /// Enable command chaining: multiple subcommands may be named in one
    /// invocation (e.g. "mytool lint build deploy"), each run in order.
    group& chain(bool value = true) {
        chain_ = value;
        return *this;
    }

    command& add_command(command cmd) {
        commands_.push_back(std::make_unique<command>(std::move(cmd)));
        return *commands_.back();
    }
    group& add_group(group grp) {
        subgroups_.push_back(std::make_unique<group>(std::move(grp)));
        return *subgroups_.back();
    }

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& help_text() const { return help_; }
    [[nodiscard]] const std::vector<std::string>& aliases() const { return aliases_; }
    [[nodiscard]] bool is_chained() const { return chain_; }
    [[nodiscard]] const std::vector<std::unique_ptr<command>>& commands() const { return commands_; }
    [[nodiscard]] const std::vector<std::unique_ptr<group>>& subgroups() const { return subgroups_; }

    /// Top-level entry point: `int main(int argc, char** argv) { return my_group.run(argc, argv); }`
    int run(int argc, char** argv) {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
        std::string prog = !name_.empty() ? name_ : (argc > 0 ? std::string(argv[0]) : std::string("cli"));
        return run(args, prog);
    }

    /// Dispatch `args` under an already-known invocation path (used for
    /// the top-level call above, and recursively for nested groups).
    int run(const std::vector<std::string>& args, const std::string& invocation_path) {
        if (args.empty()) {
            std::cout << render_help(*this, invocation_path);
            return 0;
        }

        const std::string& first = args[0];
        if (first == "--help" || first == "-h") {
            std::cout << render_help(*this, invocation_path);
            return 0;
        }
        if (first == "--generate-completion") {
            if (args.size() < 2) {
                print_error("Missing option '--generate-completion' value.");
                return 2;
            }
            std::cout << generate_completion_script(*this, args[1]);
            return 0;
        }

        if (chain_) {
            std::size_t i = 0;
            int last_result = 0;
            bool ran_any = false;
            while (i < args.size()) {
                const std::string& tok = args[i];
                command* cmd = find_command(tok);
                group* grp = find_group(tok);
                if (!cmd && !grp) {
                    print_error("No such command '" + tok + "'.");
                    return 2;
                }
                ++i;
                std::size_t start = i;
                while (i < args.size() && !is_known_subcommand(args[i])) ++i;
                std::vector<std::string> slice(args.begin() + static_cast<long>(start), args.begin() + static_cast<long>(i));
                int result = cmd ? cmd->run(slice, invocation_path + " " + tok)
                                  : grp->run(slice, invocation_path + " " + tok);
                ran_any = true;
                if (result != 0) return result;
                last_result = result;
            }
            return ran_any ? last_result : 0;
        }

        const std::string& tok = args[0];
        command* cmd = find_command(tok);
        group* grp = find_group(tok);
        if (!cmd && !grp) {
            print_error("No such command '" + tok + "'.");
            return 2;
        }
        std::vector<std::string> rest(args.begin() + 1, args.end());
        return cmd ? cmd->run(rest, invocation_path + " " + tok) : grp->run(rest, invocation_path + " " + tok);
    }

  private:
    [[nodiscard]] command* find_command(std::string_view token) const {
        for (const auto& c : commands_) {
            if (c->matches(token)) return c.get();
        }
        return nullptr;
    }
    [[nodiscard]] group* find_group(std::string_view token) const {
        for (const auto& g : subgroups_) {
            if (g->name() == token) return g.get();
            for (const auto& a : g->aliases()) {
                if (a == token) return g.get();
            }
        }
        return nullptr;
    }
    [[nodiscard]] bool is_known_subcommand(std::string_view token) const {
        return find_command(token) != nullptr || find_group(token) != nullptr;
    }

    static void print_error(const std::string& detail) {
        std::cerr << chalk::red("Error: " + detail) << "\n";
    }

    std::string name_;
    std::string help_;
    std::vector<std::string> aliases_;
    bool chain_ = false;
    std::vector<std::unique_ptr<command>> commands_;
    std::vector<std::unique_ptr<group>> subgroups_;
};

} // namespace cx::console::cli
