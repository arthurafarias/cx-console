#pragma once

// Click-style help screen rendering for a single command or a group
// (with its "Commands:" section). Section headers are bold and option
// flags cyan/bold via cx-console-chalk, degrading to plain text when
// chalk is disabled.

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "cx/console/chalk.hpp"
#include "cx/console/cli/command.hpp"
#include "cx/console/cli/group.hpp"

namespace cx::console::cli {

namespace detail {

inline int terminal_width() {
    if (const char* cols = std::getenv("COLUMNS"); cols != nullptr && cols[0] != '\0') {
        try {
            int w = std::stoi(cols);
            if (w > 0) return w;
        } catch (...) {
        }
    }
#if defined(__unix__) || defined(__APPLE__)
    struct winsize ws {};
    if (ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
#endif
    return 80;
}

/// Wrap `text` to `width`, returning lines (word-wrapped, no hyphenation).
inline std::vector<std::string> wrap_text(const std::string& text, int width) {
    std::vector<std::string> lines;
    if (width < 10) width = 10;
    std::istringstream iss(text);
    std::string word;
    std::string current;
    while (iss >> word) {
        if (current.empty()) {
            current = word;
        } else if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
            current += " " + word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) lines.push_back(current);
    if (lines.empty()) lines.push_back("");
    return lines;
}

/// Left-pad `label` to `col` width for a two-column help row, then
/// append (word-wrapped) help text starting at column `col`.
inline void append_two_column_row(std::string& out, const std::string& label, const std::string& text,
                                   std::size_t col, int width) {
    out += "  " + label;
    if (text.empty()) {
        out += "\n";
        return;
    }
    std::size_t pad_to = col;
    std::size_t current_len = 2 + label.size();
    if (current_len + 2 > pad_to) {
        out += "\n";
        out += std::string(pad_to, ' ');
    } else {
        out += std::string(pad_to - current_len, ' ');
    }
    int text_width = std::max(20, width - static_cast<int>(pad_to));
    auto lines = wrap_text(text, text_width);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) out += std::string(pad_to, ' ');
        out += lines[i];
        out += "\n";
    }
}

inline std::string option_flag_display(const option_base& opt) {
    std::string s;
    if (!opt.short_name().empty()) {
        s += "-" + opt.short_name() + ", ";
    }
    s += "--" + opt.long_name();
    std::string hint = opt.type_hint();
    if (!hint.empty()) {
        s += " " + hint;
    }
    return s;
}

inline std::string option_help_suffix(const option_base& opt) {
    std::string extra;
    if (!opt.allowed_choices().empty()) {
        extra += " [choices: ";
        const auto& choices = opt.allowed_choices();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            if (i != 0) extra += ", ";
            extra += choices[i];
        }
        extra += "]";
    }
    if (opt.has_default() && !opt.default_display().empty()) {
        extra += " [default: " + opt.default_display() + "]";
    }
    if (!opt.env_var().empty()) {
        extra += " [env: " + opt.env_var() + "]";
    }
    if (opt.is_required()) {
        extra += " [required]";
    }
    return extra;
}

inline std::string argument_help_suffix(const argument_base& arg) {
    std::string extra;
    if (!arg.allowed_choices().empty()) {
        extra += " [choices: ";
        const auto& choices = arg.allowed_choices();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            if (i != 0) extra += ", ";
            extra += choices[i];
        }
        extra += "]";
    }
    if (arg.has_default() && !arg.default_display().empty()) {
        extra += " [default: " + arg.default_display() + "]";
    }
    if (arg.is_required()) {
        extra += " [required]";
    }
    return extra;
}

/// Usage line's "ARG1 ARG2..." fragment for a command's positionals.
inline std::string usage_arguments_fragment(const command& cmd) {
    std::string s;
    for (const auto& arg : cmd.positionals()) {
        s += " ";
        std::string token = arg->is_required() ? arg->name() : ("[" + arg->name() + "]");
        if (arg->is_variadic()) token += "...";
        s += token;
    }
    return s;
}

} // namespace detail

inline std::string render_help(const command& cmd, const std::string& invocation_path) {
    const int width = detail::terminal_width();
    std::ostringstream out;

    out << chalk::bold("Usage:") << " " << invocation_path << " [OPTIONS]" << detail::usage_arguments_fragment(cmd)
        << "\n";

    if (!cmd.help_text().empty()) {
        out << "\n";
        for (const auto& line : detail::wrap_text(cmd.help_text(), width)) out << line << "\n";
    }

    if (!cmd.positionals().empty()) {
        out << "\n" << chalk::bold("Arguments:") << "\n";
        std::string body;
        for (const auto& arg : cmd.positionals()) {
            std::string label = chalk::cyan(arg->name());
            std::string help_text = arg->help_text() + detail::argument_help_suffix(*arg);
            // Column alignment uses the plain (unstyled) label width, not
            // the ANSI-escaped one.
            detail::append_two_column_row(body, label, help_text, arg->name().size() + 2 + 2 + 2, width);
        }
        out << body;
    }

    if (!cmd.options().empty()) {
        out << "\n" << chalk::bold("Options:") << "\n";
        std::size_t widest = 0;
        for (const auto& opt : cmd.options()) {
            widest = std::max(widest, detail::option_flag_display(*opt).size());
        }
        std::string body;
        for (const auto& opt : cmd.options()) {
            std::string plain_label = detail::option_flag_display(*opt);
            std::string styled_label = chalk::cyan(chalk::bold(plain_label));
            std::string help_text = opt->help_text() + detail::option_help_suffix(*opt);
            std::size_t col = 2 + widest + 2;
            // Build the row manually so column math uses plain_label's
            // length while the printed text uses styled_label.
            body += "  " + styled_label;
            std::size_t current_len = 2 + plain_label.size();
            if (current_len + 2 > col) {
                body += "\n" + std::string(col, ' ');
            } else {
                body += std::string(col - current_len, ' ');
            }
            int text_width = std::max(20, width - static_cast<int>(col));
            auto lines = detail::wrap_text(help_text, text_width);
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (i != 0) body += std::string(col, ' ');
                body += lines[i];
                body += "\n";
            }
        }
        out << body;
    }

    return out.str();
}

inline std::string render_help(const group& grp, const std::string& invocation_path) {
    const int width = detail::terminal_width();
    std::ostringstream out;

    out << chalk::bold("Usage:") << " " << invocation_path << " [OPTIONS] COMMAND [ARGS]...\n";

    if (!grp.help_text().empty()) {
        out << "\n";
        for (const auto& line : detail::wrap_text(grp.help_text(), width)) out << line << "\n";
    }

    if (!grp.commands().empty() || !grp.subgroups().empty()) {
        out << "\n" << chalk::bold("Commands:") << "\n";
        std::size_t widest = 0;
        for (const auto& c : grp.commands()) widest = std::max(widest, c->name().size());
        for (const auto& g : grp.subgroups()) widest = std::max(widest, g->name().size());

        std::string body;
        for (const auto& c : grp.commands()) {
            detail::append_two_column_row(body, chalk::cyan(c->name()), c->help_text(), c->name().size() + 2 + 2 + 2,
                                           width);
        }
        for (const auto& g : grp.subgroups()) {
            detail::append_two_column_row(body, chalk::cyan(g->name()), g->help_text(), g->name().size() + 2 + 2 + 2,
                                           width);
        }
        out << body;
    }

    out << "\n"
        << chalk::bold("Options:") << "\n"
        << "  " << chalk::cyan(chalk::bold("-h, --help")) << "  Show this message and exit.\n";

    return out.str();
}

inline std::string generate_completion_script(const group& grp, std::string_view shell) {
    std::string commands;
    for (const auto& command : grp.commands()) {
        if (!commands.empty()) commands += " ";
        commands += command->name();
    }
    for (const auto& subgroup : grp.subgroups()) {
        if (!commands.empty()) commands += " ";
        commands += subgroup->name();
    }

    const std::string program = grp.name();
    if (shell == "bash") {
        return "_" + program + "_complete() {\n"
               "  COMPREPLY=( $(compgen -W '" + commands + "' -- \"${COMP_WORDS[COMP_CWORD]}\") )\n"
               "}\ncomplete -F _" + program + "_complete " + program + "\n";
    }
    if (shell == "zsh") {
        return "#compdef " + program + "\n_arguments '1:command:(" + commands + ")'\n";
    }
    if (shell == "fish") {
        std::string out;
        for (const auto& command : grp.commands())
            out += "complete -c " + program + " -f -a " + command->name() +
                   " -d '" + command->help_text() + "'\n";
        for (const auto& subgroup : grp.subgroups())
            out += "complete -c " + program + " -f -a " + subgroup->name() +
                   " -d '" + subgroup->help_text() + "'\n";
        return out;
    }
    throw usage_error("Unsupported completion shell '" + std::string(shell) +
                      "' (expected bash, zsh, or fish).");
}

} // namespace cx::console::cli
