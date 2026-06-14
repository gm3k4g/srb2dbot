#!/usr/bin/env bash
set -euo pipefail

# srb2_dbot.sh  -  minimal SRB2 server with Discord bridge Lua
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
        "$SCRIPT_DIR/scripts/SRB2DiscordBot.lua"
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

mkdir -p "$HOME/.srb2/luafiles/client/DiscordBot"

# Deploy script to DOWNLOAD so SRB2 auto-loads it once
cp "$LUA_WAD" "$HOME/.srb2/DOWNLOAD/SRB2DiscordBot.lua"

echo "=== srb2_dbot ==="
echo "Server: $SERVERNAME"
echo "Port:   $PORT"
echo "Room:   $ROOM"
echo "Lua:    $LUA_WAD"
echo ""

echo "[srb2_dbot] Launching srb2dbot..."
"$SCRIPT_DIR/build/srb2dbot" &
BOT_PID=$!

echo "[srb2_dbot] Waiting 3 seconds for bot to connect..."
sleep 3

echo "[srb2_dbot] Starting SRB2 server..."
cd "$HOME/.srb2"
exec srb2 \
    -dedicated \
    -port "$PORT" \
    -room "$ROOM" \
    -servername "$SERVERNAME" \
    -file "$HOME/.srb2/DOWNLOAD/SRB2DiscordBot.lua" \
    -file "$HOME/.srb2/DOWNLOAD/Ba_CommunityPack-v13_2HOTFIX.pk3" \
    +rejointimeout 0
    #</dev/null