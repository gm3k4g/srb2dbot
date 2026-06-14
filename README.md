# srb2dbot

> Control SRB2 servers via Discord.

A Discord bot that remotely manages [Sonic Robo Blast 2](https://www.srb2.org/) game servers through slash commands. Edit server config scripts, manage WAD addon files, send console commands, kick/ban players  -  all from Discord. Also bridges chat between Discord and SRB2 in real time.

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
| **Chat Bridge** | Real-time Discord↔SRB2 message relay - gametype-aware, resolves WAD custom gametypes via `G_GetGametypeName` |
| **Event Cards** | Configurable Discord embeds for player join, quit, kick, ban, flag events, round start/end, chat, server announcements, and map thumbnails |
| **Module System** | Toggle and customize individual features via `modules.json` - enable/disable cards, set embed titles with placeholder fields |

## How It Works

The bot communicates with SRB2 through a **named FIFO pipe** (`~/.srb2/srb2_servers.d/srb2b.d/srb2b.fifo`) for console commands, and **systemd** for service lifecycle management. Script editing operates on a bash config file validated with `bash -n` before each write.

The **chat bridge** forwards Discord messages to SRB2 via `~/.srb2/luafiles/client/DiscordBot/discordmessage.txt`, and polls `Messages.txt` every 2 seconds to relay SRB2 chat back to Discord with emoji conversion and mention suppression. Gametype names use SRB2's internal `G_GetGametypeName` API, ensuring exact 1-to-1 in-game names (e.g. "Co-op", "Arena") and automatic support for custom WAD gametypes (e.g. "Survival").

Events are written to `Messages.txt` immediately when they occur (SERVER_START, ROUND_START/END, player join/quit) via a shared helper, rather than waiting for the periodic flush cycle. On bot startup, a `dbot_sync` command is sent to the SRB2 server to request re-emission of the current server state, ensuring events are visible within the next poll cycle even after a restart.

## Configuration

Create `secret.json` from the template and fill in these fields:

```json
{
    "bot_token":     "YOUR_BOT_TOKEN",
    "bot_id":        "YOUR_BOT_USER_ID",
    "guild_id":      "YOUR_GUILD_ID",
    "channel_id":    "YOUR_BRIDGE_CHANNEL_ID",
    "service_name":  "srb2@srb2b"
}
```

| Field | Required | Description |
|---|---|---|
| `bot_token` | **Yes** | Discord bot token from the [Developer Portal](https://discord.com/developers/applications) → Bot → Token |
| `bot_id` | **Yes** | The bot's own Discord user ID. Used by the chat bridge to prevent echoing its own messages back. Find under General Information → Application ID |
| `guild_id` | **Yes** | Discord server (guild) ID where slash commands are registered and the bot operates |
| `channel_id` | For bridge | Discord channel ID for chat relay. Messages in this channel are forwarded to SRB2. Optional  -  if unset or `"0"`, the bridge is disabled |
| `service_name` | For server control | systemd user service name for restart/stop commands. Defaults to `srb2@srb2b` |

**After creating `secret.json`, you must also enable** the Message Content Intent in the Discord Developer Portal under Bot → Privileged Gateway Intents for the chat bridge to read message content.

## Dependencies

- **C++26** compiler (GCC 14+ / Clang 18+)
- **CMake** 3.14+
- **[D++](https://dpp.dev)**  -  Discord API library
- **[nlohmann/json](https://github.com/nlohmann/json)**  -  JSON parsing
- **systemd** + **bash** (runtime)

## Testing

77 tests (66 unit + 11 end-to-end pipeline tests) covering utilities, filename sanitization, script manipulation, chat bridge functions (sanitization, file polling, emoji conversion, event parsing), polling lifecycle, and full event pipeline simulation (dbot_sync replay, ROUND_END with teams, CTF batch detection, bot restart recovery):

```bash
BUILD_DIR=build BUILD_TYPE=Debug RUN_TESTS=ON ./build.sh
```

## Documentation

- [Full Documentation](DOCS.md)  -  table of contents for `doc/`
- [Commands Reference](doc/commands.html)
- [Build Instructions](doc/building.html)
- [Architecture Overview](doc/architecture.html)

## Author

**gm3k4g**  -  [GitHub](https://github.com/gm3k4g/srb2dbot)
