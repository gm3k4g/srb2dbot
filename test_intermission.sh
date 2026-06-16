#!/usr/bin/env bash
# test_intermission.sh  -  Test generate_intermission.sh with various player counts
# Generates sample outputs for 2, 4, 8, 12, 16, 18, 24 players in both modes.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GEN="$SCRIPT_DIR/generate_intermission.sh"
[[ -x "$GEN" ]] || { echo "ERROR: $GEN not found"; exit 1; }
OUT_DIR="${1:-/tmp/intermission_tests}"

mkdir -p "$OUT_DIR"

# Generate a player JSON array with N sequential players
players_json() {
    local n=$1 prefix=${2:-Player}
    local team=${3:-ffa}
    echo -n '['
    for ((i=1; i<=n; i++)); do
        local score=$(( (n - i + 1) * 100 + RANDOM % 99 ))
        local name="${prefix}${i}"
        if [[ "$team" == "ffa" ]]; then
            echo -n '{"name":"'"$name"'","score":'"$score"'}'
        else
            local t="red"
            [[ $((i % 2)) -eq 0 ]] && t="blue"
            echo -n '{"name":"'"$name"'","score":'"$score"',"team":"'"$t"'"}'
        fi
        [[ $i -lt $n ]] && echo -n ','
    done
    echo ']'
}

# Test a specific player count
test_n() {
    local n=$1 mode=$2
    local suffix="${mode}_${n}p"
    local out="$OUT_DIR/${suffix}.png"
    local extra=""

    if [[ "$mode" == "team" ]]; then
        extra="--blue-score $((n/4 + 1)) --red-score $((n/4))"
    fi

    players_json "$n" "P" "$mode" > /tmp/_intermission_test_players.json

    echo -n "[$mode ${n}p] "
    if "$GEN" --gametype "$mode" $extra \
        --players-file /tmp/_intermission_test_players.json \
        --width 640 --height 400 \
        --out "$out" 2>&1; then
        local size
        size=$(identify -format "%b" "$out" 2>/dev/null || echo "?")
        echo "  OK  ${size}"
    else
        echo "  FAILED"
        return 1
    fi
}

echo "=== FFA Mode Tests ==="
for n in 2 4 8 12 16 18 24; do
    test_n "$n" "ffa"
done

echo ""
echo "=== Team Mode Tests ==="
for n in 2 4 8 12 16 18 24; do
    test_n "$n" "team"
done

echo ""
echo "=== Summary ==="
ls -lh "$OUT_DIR"/*.png 2>/dev/null | awk '{print $5, $NF}' | column -t
echo ""
echo "Test outputs: $OUT_DIR/"
