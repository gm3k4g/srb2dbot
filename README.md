# srb2dbot

> Control SRB2 servers via Discord.

A Discord bot that remotely manages [Sonic Robo Blast 2](https://www.srb2.org/) game servers through slash commands. Edit server config scripts, manage WAD addon files, send console commands, kick/ban players — all from Discord. Also bridges chat between Discord and SRB2 in real time.

## Quick Start

```bash
# Clone
git clone https://github.com/gm3k4g/srb2dbot.git
cd srb2dbot

# Build (auto-detects NixOS vs Linux)
./build.sh

# Or manually
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Configure
cp secret.json.default secret.json
# Edit with your bot token, guild ID, channel ID, and service name
```

## Features

| Category | Commands |
|---|---|
| **Script Editing** | `get_script`, `find_line`, `inspect_line`, `insert_line`, `remove_line`, `change_line`, `move_line` |
| **WAD Management** | `list_wads`, `search_wads`, `addfile_upload`, `addfile_link` |
| **Server Control** | `restart_server`, `stop_server`, `server_do` |
| **Player Management** | `server_say`, `kick_player`, `ban_player` |
| **Chat Bridge** | Real-time Discord↔SRB2 message relay via shared text files |

## How It Works

The bot communicates with SRB2 through a **named FIFO pipe** (`~/.srb2/srb2_servers.d/srb2b.d/srb2b.fifo`) for console commands, and **systemd** for service lifecycle management. Script editing operates on a bash config file validated with `bash -n` before each write.

The **chat bridge** forwards Discord messages to SRB2 via `~/.srb2/luafiles/client/DiscordBot/discordmessage.txt`, and polls `Messages.txt` every second to relay SRB2 chat back to Discord with emoji conversion and mention suppression.

## Dependencies

- **C++26** compiler (GCC 14+ / Clang 18+)
- **CMake** 3.14+
- **[D++](https://dpp.dev)** — Discord API library
- **[nlohmann/json](https://github.com/nlohmann/json)** — JSON parsing
- **systemd** + **bash** (runtime)

## Testing

47 unit tests covering utilities, filename sanitization, script manipulation, and chat bridge functions:

```bash
BUILD_DIR=build BUILD_TYPE=Debug RUN_TESTS=ON ./build.sh
```

## Documentation

- [Full Documentation](DOCS.md) — table of contents for `doc/`
- [Commands Reference](doc/commands.html)
- [Build Instructions](doc/building.html)
- [Architecture Overview](doc/architecture.html)

## Author

**gm3k4g** — [GitHub](https://github.com/gm3k4g/srb2dbot)
