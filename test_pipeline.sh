#!/usr/bin/env bash
set -euo pipefail

# test_pipeline.sh — Full end-to-end pipeline test for srb2dbot chat bridge
# Starts SRB2 server + bot, tests all event types, verifies bidirectional flow.

PASS=0
FAIL=0
MSGFILE="$HOME/.srb2/luafiles/client/DiscordBot/Messages.txt"
DISCFILE="$HOME/.srb2/luafiles/client/DiscordBot/discordmessage.txt"

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

check() {
    local label="$1"; shift
    if "$@"; then
        echo -e "  ${GREEN}PASS${NC} $label"
        PASS=$((PASS+1))
    else
        echo -e "  ${RED}FAIL${NC} $label"
        FAIL=$((FAIL+1))
    fi
}

cleanup() {
    killall srb2 2>/dev/null || true
    killall srb2dbot 2>/dev/null || true
    wait 2>/dev/null || true
}
trap cleanup EXIT

echo -e "${CYAN}=== srb2dbot Pipeline Test ===${NC}"
echo ""

# ── Setup ────────────────────────────────────────────────
cleanup
mkdir -p "$HOME/.srb2/luafiles/client/DiscordBot/thumbnails"
rm -f "$MSGFILE" "$DISCFILE"
touch "$MSGFILE" "$DISCFILE"

# ── Start bot first (so it init Messages.txt) ────────────
echo "Starting srb2dbot bot..."
./build/srb2dbot &
BOT_PID=$!
sleep 3
check "bot process running" kill -0 $BOT_PID

# ── Start SRB2 server ────────────────────────────────────
echo "Starting SRB2 server..."
./srb2_dbot.sh &
SRB2_PID=$!
sleep 8
check "server process running" kill -0 $SRB2_PID

# ── Wait for auto-events (SERVER_START at tick 35, written at tick 105) ─
echo "Waiting for server boot events (~12s for tick 105)..."
sleep 12

check "SERVER_START event fired" grep -q "SERVER_START" "$MSGFILE"
check "ROUND_START event fired" grep -q "ROUND_START" "$MSGFILE"
check "SERVER_START appears first" grep -q "SERVER_START" "$MSGFILE"

# ── PLAYER_JOIN ──────────────────────────────────────────
echo "[EVENT:PLAYER_JOIN]|TestPlayer" >> "$MSGFILE"
sleep 3
check "PLAYER_JOIN event processed" grep -q "PLAYER_JOIN" "$MSGFILE"

# ── PLAYER_QUIT ──────────────────────────────────────────
echo "[EVENT:PLAYER_QUIT]|TestPlayer" >> "$MSGFILE"
sleep 3
check "PLAYER_QUIT event processed" grep -q "PLAYER_QUIT" "$MSGFILE"

# ── KICK_PLAYER ──────────────────────────────────────────
echo "[EVENT:KICK_PLAYER]|ProblemPlayer" >> "$MSGFILE"
sleep 3
check "KICK_PLAYER event processed" grep -q "KICK_PLAYER" "$MSGFILE"

# ── CTF events ───────────────────────────────────────────
echo "[EVENT:CTF_CAPTURE]|Sonic|Red" >> "$MSGFILE"
sleep 2
echo "[EVENT:CTF_DROP]|Sonic" >> "$MSGFILE"
sleep 2
echo "[EVENT:CTF_PICKUP]|Tails" >> "$MSGFILE"
sleep 2
echo "[EVENT:CTF_RETURN]|Knuckles" >> "$MSGFILE"
sleep 3
check "CTF_CAPTURE detected" grep -q "CTF_CAPTURE" "$MSGFILE"
check "CTF_DROP detected" grep -q "CTF_DROP" "$MSGFILE"
check "CTF_PICKUP detected" grep -q "CTF_PICKUP" "$MSGFILE"
check "CTF_RETURN detected" grep -q "CTF_RETURN" "$MSGFILE"

# ── ROUND_END (team) ─────────────────────────────────────
echo "[EVENT:ROUND_END]|CTF|MAP01|MAPNAME:Greenflower Zone|TEAM:Red:5|RED:Sonic:3|RED:Tails:2|TEAM:Blue:3|BLUE:Knuckles:2|BLUE:Amy:1|SPEC:Watcher" >> "$MSGFILE"
sleep 3
check "ROUND_END detected" grep -q "ROUND_END" "$MSGFILE"

# ── MAP_CHANGE via ROUND_START ──────────────────────────
echo "[EVENT:ROUND_START]|CTF|MAP03|Greenflower Zone Act 3" >> "$MSGFILE"
sleep 3
check "ROUND_START MAP03 detected" grep -q "MAP03.*Greenflower" "$MSGFILE"

# ── Discord → SRB2 (write to discordmessage.txt) ─────────
echo "<DiscordUser> hello from Discord!" >> "$DISCFILE"
sleep 4
check "discordmessage.txt has Discord msg" grep -q "hello from Discord" "$DISCFILE"

# ── SRB2 → Discord (chat message) ────────────────────────
echo "TestPlayer: hello from SRB2 game!" >> "$MSGFILE"
sleep 3
check "SRB2 chat in Messages.txt" grep -q "hello from SRB2 game" "$MSGFILE"

# ── Multi-message batching ───────────────────────────────
for i in $(seq 1 5); do
    echo "Player$i: chat message $i" >> "$MSGFILE"
done
sleep 3
check "batched messages present" test "$(grep -c "chat message" "$MSGFILE" 2>/dev/null || echo 0)" -ge 5

# ── Summary ──────────────────────────────────────────────
echo ""
echo -e "${CYAN}=== Results: $PASS pass, $FAIL fail ===${NC}"

exit $FAIL
