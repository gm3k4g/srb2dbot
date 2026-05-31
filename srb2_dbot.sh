#!/usr/bin/env bash
set -euo pipefail

# srb2_dbot.sh — minimal SRB2 server with Discord bridge Lua
# Run this alongside srb2dbot to test the chat bridge locally.
#
# Usage: ./srb2_dbot.sh [port] [room] [servername] [lua_wad_path]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-5029}"
ROOM="${2:-38}"
SERVERNAME="${3:-srb2dbot test server}"

# Search for the WAD/Lua in multiple locations
find_wad() {
    for loc in \
        "${4:-}" \
        "$SCRIPT_DIR/scripts/SRB2DiscordBot.wad" \
        "$SCRIPT_DIR/scripts/SRB2DiscordBot.lua" \
        "$HOME/.srb2/SRB2DiscordBot.wad" \
        "$HOME/.srb2/luafiles/client/SRB2DiscordBot.wad" \
        "$HOME/.srb2/addons/SRB2DiscordBot.wad" \
        "$SCRIPT_DIR/../CONTENT_SRB2/HOSTING/SCRIPTS/SRB2_DiscordBot/SRB2DiscordBot.wad"
    do
        [[ -n "$loc" && -f "$loc" ]] && { echo "$loc"; return 0; }
    done
    return 1
}

LUA_WAD=$(find_wad) || {
    echo "ERROR: SRB2DiscordBot.wad not found." >&2
    echo "Place it in one of:" >&2
    echo "  $SCRIPT_DIR/scripts/" >&2
    echo "  \$HOME/.srb2/" >&2
    echo "Or pass the path as the 4th argument." >&2
    exit 1
}

cd "$HOME/.srb2"
mkdir -p luafiles/client/DiscordBot

echo "=== srb2_dbot ==="
echo "Server: $SERVERNAME"
echo "Port:   $PORT"
echo "Room:   $ROOM"
echo "Lua:    $LUA_WAD"
echo ""

exec srb2 \
    -dedicated \
    -port "$PORT" \
    -room "$ROOM" \
    -servername "$SERVERNAME" \
    -file "$LUA_WAD"
