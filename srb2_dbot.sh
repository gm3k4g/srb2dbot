#!/usr/bin/env bash
set -euo pipefail

# srb2_dbot.sh — minimal SRB2 server with Discord bridge Lua
# Run this alongside srb2dbot to test the chat bridge locally.

CONTENT_DIR="/run/media/einfoed/EXTERNAL1/games/free/srb2/CONTENT_SRB2"
LUA_DIR="${CONTENT_DIR}/HOSTING/SCRIPTS/SRB2_DiscordBot"
LUA_WAD="${LUA_DIR}/SRB2DiscordBot.wad"

PORT="${1:-5029}"
ROOM="${2:-38}"
SERVERNAME="${3:-srb2dbot test server}"

if [ ! -f "$LUA_WAD" ]; then
    echo "ERROR: Lua addon not found at $LUA_WAD" >&2
    echo "Expected compiled WAD from $LUA_DIR/" >&2
    exit 1
fi

cd "$HOME/.srb2"

# Ensure bridge directory exists (same path srb2dbot uses)
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
