# AGENTS.md — srb2dbot AI Assistant Guide

## Project Overview

**srb2dbot** is a C++26 Discord bot using the **D++** library to remotely control Sonic Robo Blast 2 game servers. It manages server lifecycle through systemd, communicates via named FIFO pipes, and provides script editing and WAD management through Discord slash commands.

- **Language:** C++26 (GCC 14+ / Clang 18+)
- **Build:** CMake 3.14+
- **Test framework:** CTest (plain `main()` with custom CHECK macros)
- **Target OS:** Linux (NixOS and standard distros)

## Build & Test Commands

```bash
# Build (auto-detects NixOS vs Linux)
./build.sh

# With tests
BUILD_DIR=build BUILD_TYPE=Debug RUN_TESTS=ON ./build.sh

# Manual build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -Dsrb2dbot_DEVELOPER_MODE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## Architecture

```
Discord → D++ library → main.cpp (command router) → 3 paths:
  1. Script path: bash config file I/O + validation (fork/exec bash -n)
  2. WAD path: directory listing, file download via HTTP
  3. Server pipe: FIFO write for console commands + systemctl for lifecycle
```

## Directory Structure

```
source/         — Source files
  main.cpp      — Bot init, command handlers, POSIX-dependent anon-namespace functions
  utils.cpp     — Pure utility functions (sanitize_filename, link_filename_str)
  script.cpp    — Script manipulation logic (pure string I/O, testable)
include/srb2dbot/
  utils.hpp     — Utility declarations
  script.hpp    — Script helper declarations
test/
  CMakeLists.txt
  source/srb2dbot_test.cpp  — 28 tests
cmake/          — CMake modules, presets, dev-mode, linting
doc/            — HTML documentation
```

## Key Conventions

- **Anonymous namespace** in `main.cpp` — functions that depend on POSIX/system APIs (file I/O, pipes, process management) remain here. Pure logic is extracted to `source/utils.cpp` and `source/script.cpp`.
- **Conventional Commits** — use `fix:`, `feat:`, `refactor:`, `docs:`, `build:` prefixes.
- **New features** should be developed on feature branches.
- **Tests** go in `test/source/` and are registered in `test/CMakeLists.txt`. Test names are descriptive strings.
- **Documentation** is updated in `doc/` HTML files after significant changes. `DOCS.md` is the index of documentation pages.

## Security Constraints

When modifying the codebase, maintain these invariants:

1. **Pipe input sanitization** — `pipe_srb2_server_do`, `pipe_srb2_server_say`, `pipe_srb2_kick_player`, `pipe_srb2_ban_player` must reject `\n`, `\r`, and control characters (except tab).
2. **Script validation** — `script_validate()` must use `fork()`/`execlp()` with argument arrays, never `system()` with shell string concatenation.
3. **Filename sanitization** — `sanitize_filename()` extracts only the filename component, rejects `.` and `..`.
4. **Upload size cap** — `MAX_WAD_SIZE` constant (100 MB) is enforced in both `addfile_upload` and `addfile_link`.
5. **JSON parsing** — must use try/catch with proper error messages.
6. **Password redaction** — lines containing "password" must be redacted in script display/output.

## Current TODO List

From `source/main.cpp` comments:

- [ ] Replace code-block replies with Discord embeds/cards
- [ ] Make admin commands invisible (ephemeral replies)
- [ ] Chat log reading and printing in channel
- [ ] Game stats command
- [ ] Implement `CMD_REMOVE_FILE` / `CMD_RENAME_FILE` (defined but not wired up)
- [ ] Implement CLI argument parsing (`-r`, `-v`, `-c`, `-n`, `-e`) and print `USAGE`
- [ ] Configurable script name via `-c` parameter (currently hardcoded `srb2b.sh`)
- [ ] Case-insensitive search option for `find_line`

## Known Issues

From the previous audit:

| ID | Severity | Issue |
|---|---|---|
| M5 | Medium | `file(GLOB SOURCES)` — new source files silently ignored by CMake |
| M10 | Medium | FIFO open for write blocks if no reader (even with `O_NONBLOCK`) |
| L14 | Low | CI preset forces C++17 but project uses C++26 |
| L1-L17 | Low | Various naming, formatting, and dead code issues |

## All Slash Commands

| Command | Category | Parameters |
|---|---|---|
| `/get_script` | Script | — |
| `/find_line` | Script | `target_string` |
| `/inspect_line` | Script | `line_number` |
| `/insert_line` | Script | `line_content`, `line_number` |
| `/remove_line` | Script | `line_number` |
| `/change_line` | Script | `new_content`, `line_number` |
| `/move_line` | Script | `old_line`, `new_line` |
| `/list_wads` | WADs | — |
| `/search_wads` | WADs | `target_string` |
| `/addfile_upload` | WADs | `file` (attachment) |
| `/addfile_link` | WADs | `file_url` |
| `/restart_server` | Server | — |
| `/stop_server` | Server | — |
| `/server_do` | Server | `command` |
| `/server_say` | Players | `message` |
| `/kick_player` | Players | `player` |
| `/ban_player` | Players | `player` |
