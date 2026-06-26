# AGENTS.md  -  srb2dbot AI Assistant Guide

## Project Overview

**srb2dbot** is a C++26 Discord bot using the **D++** library to remotely control Sonic Robo Blast 2 game servers. It manages server lifecycle through systemd, communicates via named FIFO pipes, and provides script editing and WAD management through Discord slash commands.

- **Language:** C++26 (GCC 14+ / Clang 18+)
- **Build:** CMake 3.14+
- **Test framework:** CTest (plain `main()` with custom CHECK macros)
- **Target OS:** Linux (NixOS and standard distros)

## Build & Test Commands

```bash
# Build (auto-detects NixOS vs Linux, defaults to 4 jobs)
./build.sh

# Release build
./build.sh --release

# With tests
./build.sh --test

# Manual build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -Dsrb2dbot_DEVELOPER_MODE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## Architecture

```
Discord → D++ library → main.cpp (command router) → 4 paths:
  1. Script path: bash config file I/O + validation (fork/exec bash -n)
  2. WAD path: directory listing, file download via HTTP
  3. Server pipe: FIFO write for console commands + systemctl for lifecycle
  4. Chat bridge: shared text file I/O + polling for real-time message relay
```

## Directory Structure

```
  source/          -  Source files
    main.cpp       -  Bot init, command handlers, POSIX-dependent anon-namespace functions
    utils.cpp      -  Pure utility functions (sanitize_filename, link_filename_str)
    script.cpp     -  Script manipulation logic (pure string I/O, testable)
    bridge.cpp     -  Chat bridge sanitization, file polling, emoji conversion
    version.h.cmake  -  CMake template for project version and command name constants
include/srb2dbot/
    utils.hpp      -  Utility declarations
    script.hpp     -  Script helper declarations
    bridge.hpp     -  Bridge helper declarations
test/
  CMakeLists.txt
  source/srb2dbot_test.cpp        -  66 tests (14 test functions, ~66 CHECK assertions)
  source/srb2dbot_bridge_e2e.cpp  -  11 end-to-end pipeline tests (polling lifecycle, event pipeline, bot restart)
cmake/           -  CMake modules, presets, dev-mode, linting
doc/             -  HTML documentation
```

## Key Conventions

- **Anonymous namespace** in `main.cpp`  -  functions that depend on POSIX/system APIs (file I/O, pipes, process management) remain here. Pure logic is extracted to `source/utils.cpp` and `source/script.cpp`.
- **Conventional Commits**  -  use `fix:`, `feat:`, `refactor:`, `docs:`, `build:` prefixes.
- **New features** should be developed on feature branches.
- **Tests** go in `test/source/` and are registered in `test/CMakeLists.txt`. Test names are descriptive strings.
- **Documentation** is updated in `doc/` HTML files after significant changes. `DOCS.md` is the index of documentation pages.

## Tools Scripts

| Script | Description |
|---|---|
| `generate_thumbnails.sh` | Extracts MAPxxP.lmp level select pictures from PK3 files and converts to PNG via ImageMagick |
| `generate_intermission.sh` | Generates SRB2-style intermission overlay PNG (transparent) with player rankings sorted by score. Supports FFA (single column) and Team (blue/red columns) modes with team scores. Compositable over map thumbnails. Configurable layout parameters (margin, row-height, column-gap, font sizes). Includes built-in hitbox validation that errors on rectangle overlaps |
| `test_intermission.sh` | Test harness that generates sample outputs for 2/4/8/12/16/18/24 players in both FFA and Team modes. Validates all outputs are transparent PNGs |
| `srb2_dbot.sh` | Launches a local SRB2 test server alongside the bot for development/testing |
| `build.sh` | CMake build wrapper with auto-detection (NixOS vs Linux) |
| `test_pipeline.sh` | CI pipeline test runner |

## Security Constraints

When modifying the codebase, maintain these invariants:

1. **Pipe input sanitization**  -  `pipe_srb2_server_do`, `pipe_srb2_server_say`, `pipe_srb2_kick_player`, `pipe_srb2_ban_player` must reject `\n`, `\r`, and control characters (except tab).
2. **Script validation**  -  `script_validate()` must use `fork()`/`execlp()` with argument arrays, never `system()` with shell string concatenation.
3. **Filename sanitization**  -  `sanitize_filename()` extracts only the filename component, rejects `.` and `..`.
4. **Upload size cap**  -  `MAX_WAD_SIZE` constant (100 MB) is enforced in both `addfile_upload` and `addfile_link`.
5. **JSON parsing**  -  must use try/catch with proper error messages.
6. **Password redaction**  -  lines containing "password" must be redacted in script display/output.

## Double-Post Fix (PlayerMsg)

The `PlayerMsg` hook in SRB2 can fire **multiple times** for a single chat message (engine-level behavior, not a script bug). The fires do **not** always occur at the same `leveltime` — they can span 1-3 different game ticks. This caused duplicate `[EVENT:CHAT]` or `[EVENT:SERVER_CHAT]` lines in `Messages.txt` and thus duplicate Discord posts.

**Root cause (verified via extensive debugging)**: When running a dedicated server on `localhost` with a separate SRB2 client connecting to it, **both** the server-side Lua and the client-side Lua load `SRB2DiscordBot-v0.1.35.lua` and write to the **same** `~/.srb2/luafiles/client/DiscordBot/Messages.txt`. The server emits events via its hooks, and the client also emits the same events via its hooks (since the client receives the same in-game events). This produces duplicate lines in `Messages.txt`. The fix isolates the server to a separate home directory (`~/.srb2_server/`) via the `HOME` environment variable, so the server's Lua writes go to `~/.srb2_server/.srb2/luafiles/` while the client's Lua writes go to `~/.srb2/luafiles/`. The C++ bot reads from the server's directory via the `SRB2DBOT_SRB2_HOME` env var.

**Verified via SRB2's dedicated interpreter with trace output**: SRB2's NetVar deep-copy replaces the **entire `DiscordBot`** global table (not just `DiscordBot.Data`) when a player joins. Trace confirmed two different table addresses alternating — each with its own empty `_join_emitted`, `_pending_joins`, and `_player_msg_cache`. Neither copy suppressed the other's events. `DiscordBot.Commands` holds CVars with `CV_NETVAR` flag; SRB2's NetVar sync deep-copies the CVar owner table — the entire `DiscordBot`.

The `PlayerJoin` hook in SRB2 fires **multiple times** during the connection handshake (once when the slot is reserved, again when the player fully syncs). On the second fire, the new `DiscordBot` copy had empty state, so the ThinkFrame re-emitted `PLAYER_JOIN` with a new `os.time()`.

### Fix applied in `scripts/SRB2DiscordBot-v0.1.35.lua`:
- **Double-load guard**: `if rawget(_G, "DiscordBot") and DiscordBot.version then return end` at the top prevents re-initialization if the script somehow executes more than once.
- **Dedup state in separate global (`_G.SRB2DBot_State`)**: `_player_msg_cache`, `_pending_joins`, `_join_emitted`, and `_join_times` are stored in a **separate** global table (`_G.SRB2DBot_State`) which has NO `CV_NETVAR` CVs and is therefore immune to NetVar deep-copy. All hooks read this table via `rawget(_G, "SRB2DBot_State")`.
- **Clear `_pending_joins` after processing**: `_pending_joins[#player]` is set to `nil` immediately after the ThinkFrame processes it. This ensures each `PlayerJoin` fire is processed exactly once.
- **PlayerMsg 5-tic dedup window**: Tracks `(player_node, type, target, msg)` combined with `leveltime` at the top of the hook. On a repeat fire within 5 tics (`leveltime - cache[key] <= 5`), returns `true` to suppress the duplicate without writing to `Messages.txt`. At 35 tics/second, 5 tics ≈ 143ms. The engine multi-fire spans 1-3 tics, well within the window. Human typing (even rapid "a" four times in a second) is ≈250ms apart (≈8.75 tics), safely outside the 5-tic window.
- **`server_log msg` handler**: Added `~= ''` guard to match `flush_msgsrb2()`, preventing spurious file open/write/close cycles when `msgsrb2` is empty.

### Fix applied in `source/main.cpp`:
- **Removed phantom newline**: No longer writes `\n` to `Messages.txt` on startup. The code already handles empty files correctly (`seek_start == seek_end == 0` → skip).
- **Removed content-based CHAT/SERVER_CHAT dedup**: The `seen_lines` map and `LINE_DEDUP_WINDOW` constant were removed. The previous content-based dedup (`"CHAT|player_name|message"`) with a 1-second window suppressed legitimate repeated messages from the same player within that window. With the Lua 5-tic dedup handling the engine multi-fire, the C++ content-based dedup is no longer needed.
- **Removed PLAYER_JOIN `seen_joins` dedup**: The `seen_joins` map, `JOIN_DEDUP_WINDOW` constant, and associated cleanup loop were removed. This C++ dedup was silently dropping duplicate PLAYER_JOIN events (5-second window), masking the real root cause. With the Lua `_join_emitted` guard + `_pending_joins` clearing, the Lua guard properly prevents duplicate PLAYER_JOIN events at the source.
- **Seek-based Messages.txt reading**: Replaced the rename-delete pattern with `bridge_get_lines()` / `bridge_read_range()` so Messages.txt stays on disk for inspection. The bot tracks `lines_seen` and only reads new lines each 2-second poll cycle.
- **`SRB2DBOT_SRB2_HOME` env var**: `dir_srb2_str()` checks this env var before falling back to the password database. Set to `~/.srb2_server/.srb2` so the bot reads from the server's isolated data directory.

### Fix applied in `srb2_dbot.sh`:
- **Server home isolation**: The dedicated server runs with `HOME=~/.srb2_server` while the game client uses the default `~/.srb2/`. This prevents both the server-side and client-side Lua from writing to the same `Messages.txt`, which was the root cause of duplicate events when running client and server on the same machine.

## Current TODO List

From `source/main.cpp` comments:

- [ ] Replace code-block replies with Discord embeds/cards
- [x] Make admin commands invisible (ephemeral replies)
- [x] Chat log reading and printing in channel (chat bridge via `~/.srb2/luafiles/client/DiscordBot/`)
- [ ] Game stats command
- [ ] Configurable script name via CLI parameter (currently hardcoded `srb2b.sh`)
- [ ] Implement CLI argument parsing
- [ ] Case-insensitive search option for `find_line`

## Gametype System

Gametype names in bridge events are resolved through SRB2's internal `G_GetGametypeName()` Lua API in `scripts/SRB2DiscordBot-v0.1.35.lua:451`. This ensures:

1. **1-to-1 accuracy** with SRB2's in-game names:
   - `GT_COOP` → `"Co-op"` (not "Cooperative")
   - `GT_BATTLE` → `"Arena"` (not "Battle")
   - `GT_TEAMBATTLE` → `"Team Arena"` (not "Team Battle")
2. **Automatic WAD support**  -  custom gametypes registered via `G_AddGametype()` in WAD Lua scripts or SOC `Gametype` blocks are resolved automatically (e.g. "Survival" from battle mod)
3. **Safe fallback**  -  if `G_GetGametypeName` is unavailable, a corrected lookup table is used

## Event Delivery Pipeline

Events flow from SRB2 → Discord through this pipeline:

```
SRB2 Lua hook → DiscordBot.Data.msgsrb2 → flush_msgsrb2() → Messages.txt
                                                                      ↓
Discord embed ← main.cpp event handler ← bridge_parse_event() ← bot poll timer (2s)
```

### Flush Paths

| Path | Location | Trigger | Behavior |
|---|---|---|---|
| **Immediate (primary)** | `DiscordBot.Functions.flush_msgsrb2()` | Called after every event emission (SERVER_START, ROUND_START/END, PLAYER_JOIN/QUIT) | Opens file, writes buffer, closes, clears buffer |
| **Periodic (fallback)** | `bot_function()` in `SRB2DiscordBot-v0.1.35.lua:298` | Every 70 tics (~2s) inside `leveltime%70==35` gate | Same as immediate, acts as redundancy |
| **Message-triggered** | `server_log msg` on line 105 | `spamchatbug()` when `cv_messagedelay.value == 0` | Writes buffer but does NOT clear it (buffer cleared by flush_msgsrb2 instead) |

### Bot Startup Sync

On startup, the C++ bot:
1. Truncates `Messages.txt` (deletes stale events from previous session)
2. Sends `dbot_sync` FIFO command to SRB2
3. Lua script's `dbot_sync` handler calls `emit_server_start()` + re-emits ROUND_START if a round is active
4. Calls `flush_msgsrb2()` immediately so the bot sees current state on its next poll

This ensures events are visible within ~2 seconds of bot startup.

### Debug Builds

Debug prints (`[DEBUG]`) in the C++ code are guarded by `#ifndef NDEBUG` and only compile into Debug builds. The Lua script checks `DiscordBot.Data.debug` (default `false`) before printing; it's set to `true` by the `dbot_debug on` FIFO command, which the bot sends at startup in Debug builds only.

### `cv_messagedelay` Bug (Fixed)

Line 44 of `SRB2DiscordBot-v0.1.35.lua` previously compared `cv_messagedelay == 0`  -  comparing the CVar *table* to 0, which is always false. Fixed to `cv_messagedelay.value == 0`. This restores the immediate message-triggered flush path for users who disable messagedelay.

## Known Issues

From the previous audit:

| ID | Severity | Issue |
|---|---|---|
| M10 | Medium | FIFO open for write blocks if no reader (even with `O_NONBLOCK`) |
| L1-L17 | Low | Various naming, formatting issues |

## All Slash Commands

| Command | Category | Parameters |
|---|---|---|
| `/get_script` | Script |  -  |
| `/find_line` | Script | `target_string` |
| `/inspect_line` | Script | `line_num` |
| `/insert_line` | Script | `line_num`, `line_contents` |
| `/remove_line` | Script | `line_num` |
| `/change_line` | Script | `line_num`, `line_contents` |
| `/move_line` | Script | `old_line_num`, `new_line_num` |
| `/list_wads` | WADs |  -  |
| `/search_wads` | WADs | `target_filename` |
| `/addfile_upload` | WADs | `file` (attachment) |
| `/addfile_link` | WADs | `file_url` |
| `/restart_server` | Server |  -  |
| `/stop_server` | Server |  -  |
| `/server_do` | Server | `server_command` |
| `/server_say` | Players | `server_message` |
| `/kick_player` | Players | `player` |
| `/ban_player` | Players | `player` |
