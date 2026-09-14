#pragma once

// The token-matching algorithm shared by command::parse(): given a list
// of registered options/arguments and a raw argv slice, populate a
// context or throw usage_error. Split out from command.hpp to keep the
// latter focused on command/group structure.

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cx/console/cli/argument.hpp"
#include "cx/console/cli/context.hpp"
#include "cx/console/cli/errors.hpp"
#include "cx/console/cli/option.hpp"

namespace cx::console::cli::detail {

/// Find a registered option by its "--long" or "-x" spelling (including
/// the leading dashes). Returns nullptr if not found.
inline const option_base* find_long_option(const std::vector<std::unique_ptr<option_base>>& options,
                                             std::string_view long_name) {
    for (const auto& opt : options) {
        if (opt->long_name() == long_name) return opt.get();
    }
    return nullptr;
}

inline const option_base* find_short_option(const std::vector<std::unique_ptr<option_base>>& options,
                                              char short_name) {
    for (const auto& opt : options) {
        if (opt->short_name().size() == 1 && opt->short_name()[0] == short_name) return opt.get();
    }
    return nullptr;
}

/// Parses `args` (already excluding the program name / command path)
/// against `options` and `positionals`, populating `ctx`. Throws
/// usage_error on any user-facing parse problem.
inline void parse_arguments(const std::vector<std::string>& args,
                             const std::vector<std::unique_ptr<option_base>>& options,
                             const std::vector<std::unique_ptr<argument_base>>& positionals, context& ctx,
                             bool allow_leftover) {
    // Establish baseline defaults first (flags -> false, counting -> 0,
    // multiple -> empty/default vector, single-value -> default if any)
    // so later "was this ever set" checks only need to look at ctx.has().
    for (const auto& opt : options) {
        opt->store_default(ctx);
    }

    std::vector<std::string> positional_tokens;
    bool only_positional = false;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& tok = args[i];

        if (!only_positional && tok == "--") {
            only_positional = true;
            continue;
        }

        if (!only_positional && tok.size() >= 2 && tok[0] == '-' && tok[1] == '-') {
            // --long or --long=value
            std::string name_part = tok.substr(2);
            std::string inline_value;
            bool has_inline_value = false;
            if (auto eq = name_part.find('='); eq != std::string::npos) {
                inline_value = name_part.substr(eq + 1);
                name_part = name_part.substr(0, eq);
                has_inline_value = true;
            }
            const option_base* opt = find_long_option(options, name_part);
            if (!opt) {
                throw usage_error("No such option: --" + name_part);
            }
            if (opt->is_flag()) {
                opt->store_flag(ctx);
                continue;
            }
            if (opt->is_counting()) {
                int current = ctx.has(opt->long_name()) ? ctx.get<int>(opt->long_name()) : 0;
                opt->store_count(ctx, current + 1);
                continue;
            }
            std::string value;
            if (has_inline_value) {
                value = inline_value;
            } else {
                if (i + 1 >= args.size()) {
                    throw usage_error("Option '--" + name_part + "' requires an argument.");
                }
                value = args[++i];
            }
            if (opt->is_multiple()) {
                opt->append_one(ctx, value);
            } else {
                opt->store_one(ctx, value);
            }
            continue;
        }

        if (!only_positional && tok.size() >= 2 && tok[0] == '-' && tok[1] != '-') {
            // -x, -xvalue, or combined flags -vvv
            std::string rest = tok.substr(1);

            // Single short option that takes a value: -x value or -xvalue
            if (rest.size() >= 1) {
                const option_base* first = find_short_option(options, rest[0]);
                if (!first) {
                    throw usage_error("No such option: -" + std::string(1, rest[0]));
                }
                if (!first->is_flag() && !first->is_counting()) {
                    std::string value;
                    if (rest.size() > 1) {
                        value = rest.substr(1);
                    } else {
                        if (i + 1 >= args.size()) {
                            throw usage_error("Option '-" + std::string(1, rest[0]) + "' requires an argument.");
                        }
                        value = args[++i];
                    }
                    if (first->is_multiple()) {
                        first->append_one(ctx, value);
                    } else {
                        first->store_one(ctx, value);
                    }
                    continue;
                }
            }

            // Combined boolean/counting short flags, e.g. -vvv or -ab
            for (char c : rest) {
                const option_base* opt = find_short_option(options, c);
                if (!opt) {
                    throw usage_error("No such option: -" + std::string(1, c));
                }
                if (opt->is_counting()) {
                    int current = ctx.has(opt->long_name()) ? ctx.get<int>(opt->long_name()) : 0;
                    opt->store_count(ctx, current + 1);
                } else if (opt->is_flag()) {
                    opt->store_flag(ctx);
                } else {
                    throw usage_error("No such option: -" + std::string(1, c));
                }
            }
            continue;
        }

        positional_tokens.push_back(tok);
    }

    // Match positional_tokens against `positionals`, in order. The last
    // positional may be variadic and collects the remainder.
    std::size_t token_index = 0;
    for (std::size_t p = 0; p < positionals.size(); ++p) {
        const auto& arg = *positionals[p];
        bool is_last = (p + 1 == positionals.size());

        if (arg.is_variadic() && is_last) {
            std::vector<std::string> rest(positional_tokens.begin() + static_cast<long>(token_index),
                                           positional_tokens.end());
            if (rest.empty()) {
                if (arg.is_required()) {
                    throw usage_error("Missing argument '" + arg.name() + "'.");
                }
                arg.store_default(ctx);
            } else {
                arg.store_many(ctx, rest);
            }
            token_index = positional_tokens.size();
            continue;
        }

        if (token_index >= positional_tokens.size()) {
            if (arg.is_required()) {
                throw usage_error("Missing argument '" + arg.name() + "'.");
            }
            arg.store_default(ctx);
            continue;
        }

        arg.store_one(ctx, positional_tokens[token_index]);
        ++token_index;
    }

    if (token_index < positional_tokens.size()) {
        if (!allow_leftover) {
            throw usage_error("Got unexpected extra argument (" + positional_tokens[token_index] + ")");
        }
        for (; token_index < positional_tokens.size(); ++token_index) {
            ctx.add_leftover(positional_tokens[token_index]);
        }
    }

    // Env var fallback + required-option validation, for options never
    // seen on the command line. Flags/counting flags are always defaulted
    // (false/0) up front and can't be "missing", so they're skipped here.
    for (const auto& opt : options) {
        if (opt->is_flag() || opt->is_counting()) continue;
        if (ctx.was_provided(opt->long_name())) continue;

        if (!opt->env_var().empty()) {
            const char* env_value = std::getenv(opt->env_var().c_str());
            if (env_value != nullptr && env_value[0] != '\0') {
                opt->store_from_env(ctx, env_value);
                continue;
            }
        }

        if (!ctx.has(opt->long_name()) && opt->is_required()) {
            throw usage_error("Missing option '--" + opt->long_name() + "'.");
        }
    }
}

} // namespace cx::console::cli::detail
