#!/usr/bin/env bash
set -euo pipefail

# srb2_dbot.sh  -  minimal SRB2 server with Discord bridge Lua
# Run this alongside srb2dbot to test the chat bridge locally.
#
# Usage: ./srb2_dbot.sh [--release] [port] [room] [servername] [lua_wad_path]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_ARGS="-r"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) BUILD_ARGS=""; shift ;;
        --release) BUILD_ARGS="-r"; shift ;;
        *) break ;;
    esac
done
PORT="${1:-5029}"
ROOM="${2:-38}"
SERVERNAME="${3:-srb2dbot test server}"

PID_DIR="$HOME/.srb2dbot"
mkdir -p "$PID_DIR"
BOT_PID_FILE="$PID_DIR/srb2dbot.pid"
SRB2_PID_FILE="$PID_DIR/srb2.pid"

kill_pid_file() {
    local pidfile="$1" name="$2"
    if [[ -f "$pidfile" ]]; then
        local old_pid
        old_pid=$(cat "$pidfile" 2>/dev/null || true)
        if [[ -n "$old_pid" ]] && kill -0 "$old_pid" 2>/dev/null; then
            echo "[srb2_dbot] Stopping old $name (PID $old_pid)..."
            kill "$old_pid" 2>/dev/null || true
            sleep 1
            kill -0 "$old_pid" 2>/dev/null && kill -9 "$old_pid" 2>/dev/null || true
        fi
        rm -f "$pidfile"
    fi
}

kill_pid_file "$BOT_PID_FILE" "srb2dbot"
kill_pid_file "$SRB2_PID_FILE" "SRB2"

LUA_WAD="$SCRIPT_DIR/scripts/SRB2DiscordBot-v0.1.45.lua"
if [[ ! -f "$LUA_WAD" ]]; then
    echo "ERROR: SRB2DiscordBot-v0.1.45.lua not found at $LUA_WAD" >&2
    exit 1
fi

SRB2_SERVER_HOME="$HOME/.srb2_server"
# SRB2 appends .srb2 to HOME, so the actual data directory is:
SRB2_DATA_DIR="$SRB2_SERVER_HOME/.srb2"
echo "[srb2_dbot] Setting up server home: $SRB2_SERVER_HOME (data: $SRB2_DATA_DIR)"
mkdir -p "$SRB2_DATA_DIR"
mkdir -p "$SRB2_DATA_DIR/luafiles/client/DiscordBot"
mkdir -p "$SRB2_DATA_DIR/DOWNLOAD"

# Symlink IWAD files so SRB2 can find its assets
for asset in srb2.pk3 zones.pk3 player.dta patch.pk3 music.dta; do
    if [[ -f "$HOME/.srb2/$asset" ]]; then
        [[ -e "$SRB2_DATA_DIR/$asset" ]] || ln -s "$HOME/.srb2/$asset" "$SRB2_DATA_DIR/$asset"
    fi
done

# Deploy script to server's DOWNLOAD/
cp "$LUA_WAD" "$SRB2_DATA_DIR/DOWNLOAD/SRB2DiscordBot-v0.1.45.lua"

echo "=== srb2_dbot ==="
[[ -n "$BUILD_ARGS" ]] && echo "Mode:   Release"
echo "Server: $SERVERNAME"
echo "Port:   $PORT"
echo "Room:   $ROOM"
echo "Lua:    $LUA_WAD"
echo ""

echo "[srb2_dbot] Launching srb2dbot..."
if [[ ! -f "$SCRIPT_DIR/build/srb2dbot" ]]; then
    echo "[srb2_dbot] Binary not found, building..."
    "$SCRIPT_DIR/build.sh" $BUILD_ARGS || { echo "ERROR: Build failed" >&2; exit 1; }
fi
SRB2DBOT_SRB2_HOME="$SRB2_DATA_DIR" "$SCRIPT_DIR/build/srb2dbot" &
BOT_PID=$!
echo "$BOT_PID" > "$BOT_PID_FILE"
echo "[srb2_dbot] Bot started (PID $BOT_PID)"

echo "[srb2_dbot] Waiting 3 seconds for bot to connect..."
sleep 3

echo "[srb2_dbot] Starting SRB2 server (HOME=$SRB2_SERVER_HOME)..."
command -v srb2 &>/dev/null || export PATH="$HOME/.local/bin:$PATH"
echo "$$" > "$SRB2_PID_FILE"
HOME="$SRB2_SERVER_HOME" exec srb2 \
    -dedicated \
    -port "$PORT" \
    -room "$ROOM" \
    -servername "$SERVERNAME" \
    -warp MAPF0 \
    -gametype 7 \
    -file "$SRB2_DATA_DIR/DOWNLOAD/SRB2DiscordBot-v0.1.45.lua" \
    "ZBa_Battlemod-v10BETA-2.1.8.pk3" \
    +rejointimeout 0
    #</dev/null
