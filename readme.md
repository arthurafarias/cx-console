# cx-console

A header-only C++23 pair of libraries:

- **cx-console-chalk** — a Chalk-equivalent terminal styling library
  (ANSI colors, styles, TTY/`NO_COLOR` detection).
- **cx-console-cli** — a Click-equivalent CLI framework (typed options
  and arguments, command groups, auto-generated `--help`, and shell
  completion).

`cx-console-cli` uses the shared `cx-core` object/variant model for parsed
values and its event primitive for command lifecycle notifications. Clone with
submodules enabled, or run `git submodule update --init --recursive` before
configuring the project.

Work in progress.
