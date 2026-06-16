#!/usr/bin/env bash
# test_intermission.sh  -  Test generate_intermission.sh with various configurations
# Generates sample outputs for varied player counts, team splits, and spectators.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GEN="$SCRIPT_DIR/generate_intermission.sh"
[[ -x "$GEN" ]] || { echo "ERROR: $GEN not found"; exit 1; }
OUT_DIR="${1:-/tmp/intermission_tests}"
mkdir -p "$OUT_DIR"

PASS=0; FAIL=0

run() {
    local label="$1"; shift
    local out="$OUT_DIR/${label}.png"
    echo -n "[$label] "
    local result
    result=$("$GEN" "$@" --out "$out" 2>&1) || true
    if [[ -f "$out" ]]; then
        local fsize=$(identify -format "%b" "$out" 2>/dev/null || echo "?")
        if echo "$result" | grep -qi "ERROR" >/dev/null 2>&1; then
            echo "FAIL"
            echo "$result" | grep -i "ERROR" | sed 's/^/  /'
            FAIL=$((FAIL+1))
        else
            echo "OK  ${fsize}"
            echo "$result" | grep -i "WARNING" | sed 's/^/  /' || true
            PASS=$((PASS+1))
        fi
    else
        echo "FAIL (no output)"
        echo "$result" | tail -1 | sed 's/^/  /'
        FAIL=$((FAIL+1))
    fi
}

echo "=========================================="
echo "  TEST HARNESS — intermission overlay"
echo "=========================================="
echo ""

echo "── FFA baseline (2-24 players) ──"
run "ffa_2p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":200},{"name":"P2","score":100}]'
run "ffa_4p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":400},{"name":"P2","score":300},{"name":"P3","score":200},{"name":"P4","score":100}]'
run "ffa_8p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":800},{"name":"P2","score":700},{"name":"P3","score":600},{"name":"P4","score":500},{"name":"P5","score":400},{"name":"P6","score":300},{"name":"P7","score":200},{"name":"P8","score":100}]'
run "ffa_12p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":1200},{"name":"P2","score":1100},{"name":"P3","score":1000},{"name":"P4","score":900},{"name":"P5","score":800},{"name":"P6","score":700},{"name":"P7","score":600},{"name":"P8","score":500},{"name":"P9","score":400},{"name":"P10","score":300},{"name":"P11","score":200},{"name":"P12","score":100}]'
run "ffa_16p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":1600},{"name":"P2","score":1500},{"name":"P3","score":1400},{"name":"P4","score":1300},{"name":"P5","score":1200},{"name":"P6","score":1100},{"name":"P7","score":1000},{"name":"P8","score":900},{"name":"P9","score":800},{"name":"P10","score":700},{"name":"P11","score":600},{"name":"P12","score":500},{"name":"P13","score":400},{"name":"P14","score":300},{"name":"P15","score":200},{"name":"P16","score":100}]'
run "ffa_18p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":1800},{"name":"P2","score":1700},{"name":"P3","score":1600},{"name":"P4","score":1500},{"name":"P5","score":1400},{"name":"P6","score":1300},{"name":"P7","score":1200},{"name":"P8","score":1100},{"name":"P9","score":1000},{"name":"P10","score":900},{"name":"P11","score":800},{"name":"P12","score":700},{"name":"P13","score":600},{"name":"P14","score":500},{"name":"P15","score":400},{"name":"P16","score":300},{"name":"P17","score":200},{"name":"P18","score":100}]'
run "ffa_24p" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":2400},{"name":"P2","score":2300},{"name":"P3","score":2200},{"name":"P4","score":2100},{"name":"P5","score":2000},{"name":"P6","score":1900},{"name":"P7","score":1800},{"name":"P8","score":1700},{"name":"P9","score":1600},{"name":"P10","score":1500},{"name":"P11","score":1400},{"name":"P12","score":1300},{"name":"P13","score":1200},{"name":"P14","score":1100},{"name":"P15","score":1000},{"name":"P16","score":900},{"name":"P17","score":800},{"name":"P18","score":700},{"name":"P19","score":600},{"name":"P20","score":500},{"name":"P21","score":400},{"name":"P22","score":300},{"name":"P23","score":200},{"name":"P24","score":100}]'

echo ""
echo "── FFA with spectators ──"
run "ffa_2p_4spec" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":100},{"name":"P2","score":50}]' \
    --spectators '["Spec1","Spec2","Spec3","Spec4"]'
run "ffa_9p_3spec" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"P1","score":900},{"name":"P2","score":800},{"name":"P3","score":700},{"name":"P4","score":600},{"name":"P5","score":500},{"name":"P6","score":400},{"name":"P7","score":300},{"name":"P8","score":200},{"name":"P9","score":100}]' \
    --spectators '["Spec1","Spec2","Spec3"]'
run "ffa_1p_10spec" --gametype ffa --map MAP01 --title "Test" \
    --players '[{"name":"Solo","score":999}]' \
    --spectators '["S1","S2","S3","S4","S5","S6","S7","S8","S9","S10"]'
run "ffa_0p_5spec" --gametype ffa --map MAP01 --title "Test" \
    --players '[]' \
    --spectators '["Alice","Bob","Charlie","Diana","Eve"]'

echo ""
echo "── Team baseline (2-24 players) ──"
run "team_2p" --gametype team --map MAP01 --title "Test" --blue-score 1 --red-score 1 \
    --players '[{"name":"BP1","score":100,"team":"blue"},{"name":"RP1","score":50,"team":"red"}]'
run "team_4p" --gametype team --map MAP01 --title "Test" --blue-score 2 --red-score 2 \
    --players '[{"name":"BP1","score":200,"team":"blue"},{"name":"BP2","score":100,"team":"blue"},{"name":"RP1","score":80,"team":"red"},{"name":"RP2","score":40,"team":"red"}]'
run "team_8p" --gametype team --map MAP01 --title "Test" --blue-score 4 --red-score 4 \
    --players '[{"name":"BP1","score":800,"team":"blue"},{"name":"BP2","score":700,"team":"blue"},{"name":"BP3","score":600,"team":"blue"},{"name":"BP4","score":500,"team":"blue"},{"name":"RP1","score":400,"team":"red"},{"name":"RP2","score":300,"team":"red"},{"name":"RP3","score":200,"team":"red"},{"name":"RP4","score":100,"team":"red"}]'
run "team_12p" --gametype team --map MAP01 --title "Test" --blue-score 6 --red-score 6 \
    --players '[{"name":"BP1","score":1200,"team":"blue"},{"name":"BP2","score":1100,"team":"blue"},{"name":"BP3","score":1000,"team":"blue"},{"name":"BP4","score":900,"team":"blue"},{"name":"BP5","score":800,"team":"blue"},{"name":"BP6","score":700,"team":"blue"},{"name":"RP1","score":600,"team":"red"},{"name":"RP2","score":500,"team":"red"},{"name":"RP3","score":400,"team":"red"},{"name":"RP4","score":300,"team":"red"},{"name":"RP5","score":200,"team":"red"},{"name":"RP6","score":100,"team":"red"}]'
run "team_16p" --gametype team --map MAP01 --title "Test" --blue-score 8 --red-score 8 \
    --players '[{"name":"BP1","score":1600,"team":"blue"},{"name":"BP2","score":1500,"team":"blue"},{"name":"BP3","score":1400,"team":"blue"},{"name":"BP4","score":1300,"team":"blue"},{"name":"BP5","score":1200,"team":"blue"},{"name":"BP6","score":1100,"team":"blue"},{"name":"BP7","score":1000,"team":"blue"},{"name":"BP8","score":900,"team":"blue"},{"name":"RP1","score":800,"team":"red"},{"name":"RP2","score":700,"team":"red"},{"name":"RP3","score":600,"team":"red"},{"name":"RP4","score":500,"team":"red"},{"name":"RP5","score":400,"team":"red"},{"name":"RP6","score":300,"team":"red"},{"name":"RP7","score":200,"team":"red"},{"name":"RP8","score":100,"team":"red"}]'
run "team_18p" --gametype team --map MAP01 --title "Test" --blue-score 9 --red-score 9 \
    --players '[{"name":"BP1","score":1800,"team":"blue"},{"name":"BP2","score":1700,"team":"blue"},{"name":"BP3","score":1600,"team":"blue"},{"name":"BP4","score":1500,"team":"blue"},{"name":"BP5","score":1400,"team":"blue"},{"name":"BP6","score":1300,"team":"blue"},{"name":"BP7","score":1200,"team":"blue"},{"name":"BP8","score":1100,"team":"blue"},{"name":"BP9","score":1000,"team":"blue"},{"name":"RP1","score":900,"team":"red"},{"name":"RP2","score":800,"team":"red"},{"name":"RP3","score":700,"team":"red"},{"name":"RP4","score":600,"team":"red"},{"name":"RP5","score":500,"team":"red"},{"name":"RP6","score":400,"team":"red"},{"name":"RP7","score":300,"team":"red"},{"name":"RP8","score":200,"team":"red"},{"name":"RP9","score":100,"team":"red"}]'
run "team_24p" --gametype team --map MAP01 --title "Test" --blue-score 12 --red-score 12 \
    --players '[{"name":"BP1","score":2400,"team":"blue"},{"name":"BP2","score":2300,"team":"blue"},{"name":"BP3","score":2200,"team":"blue"},{"name":"BP4","score":2100,"team":"blue"},{"name":"BP5","score":2000,"team":"blue"},{"name":"BP6","score":1900,"team":"blue"},{"name":"BP7","score":1800,"team":"blue"},{"name":"BP8","score":1700,"team":"blue"},{"name":"BP9","score":1600,"team":"blue"},{"name":"BP10","score":1500,"team":"blue"},{"name":"BP11","score":1400,"team":"blue"},{"name":"BP12","score":1300,"team":"blue"},{"name":"RP1","score":1200,"team":"red"},{"name":"RP2","score":1100,"team":"red"},{"name":"RP3","score":1000,"team":"red"},{"name":"RP4","score":900,"team":"red"},{"name":"RP5","score":800,"team":"red"},{"name":"RP6","score":700,"team":"red"},{"name":"RP7","score":600,"team":"red"},{"name":"RP8","score":500,"team":"red"},{"name":"RP9","score":400,"team":"red"},{"name":"RP10","score":300,"team":"red"},{"name":"RP11","score":200,"team":"red"},{"name":"RP12","score":100,"team":"red"}]'

echo ""
echo "── Team with spectators ──"
run "team_8r8b_8spec" --gametype team --map MAP01 --title "Test" --blue-score 8 --red-score 8 \
    --players '[{"name":"BP1","score":800,"team":"blue"},{"name":"BP2","score":700,"team":"blue"},{"name":"BP3","score":600,"team":"blue"},{"name":"BP4","score":500,"team":"blue"},{"name":"BP5","score":400,"team":"blue"},{"name":"BP6","score":300,"team":"blue"},{"name":"BP7","score":200,"team":"blue"},{"name":"BP8","score":100,"team":"blue"},{"name":"RP1","score":80,"team":"red"},{"name":"RP2","score":70,"team":"red"},{"name":"RP3","score":60,"team":"red"},{"name":"RP4","score":50,"team":"red"},{"name":"RP5","score":40,"team":"red"},{"name":"RP6","score":30,"team":"red"},{"name":"RP7","score":20,"team":"red"},{"name":"RP8","score":10,"team":"red"}]' \
    --spectators '["S1","S2","S3","S4","S5","S6","S7","S8"]'
run "team_2r8b_8spec" --gametype team --map MAP01 --title "Test" --blue-score 8 --red-score 2 \
    --players '[{"name":"BP1","score":800,"team":"blue"},{"name":"BP2","score":700,"team":"blue"},{"name":"BP3","score":600,"team":"blue"},{"name":"BP4","score":500,"team":"blue"},{"name":"BP5","score":400,"team":"blue"},{"name":"BP6","score":300,"team":"blue"},{"name":"BP7","score":200,"team":"blue"},{"name":"BP8","score":100,"team":"blue"},{"name":"RP1","score":80,"team":"red"},{"name":"RP2","score":70,"team":"red"}]' \
    --spectators '["S1","S2","S3","S4","S5","S6","S7","S8"]'
run "team_12r0b_5spec" --gametype team --map MAP01 --title "Test" --blue-score 0 --red-score 12 \
    --players '[{"name":"RP1","score":120,"team":"red"},{"name":"RP2","score":110,"team":"red"},{"name":"RP3","score":100,"team":"red"},{"name":"RP4","score":90,"team":"red"},{"name":"RP5","score":80,"team":"red"},{"name":"RP6","score":70,"team":"red"},{"name":"RP7","score":60,"team":"red"},{"name":"RP8","score":50,"team":"red"},{"name":"RP9","score":40,"team":"red"},{"name":"RP10","score":30,"team":"red"},{"name":"RP11","score":20,"team":"red"},{"name":"RP12","score":10,"team":"red"}]' \
    --spectators '["S1","S2","S3","S4","S5"]'
run "team_0r10b_10spec" --gametype team --map MAP01 --title "Test" --blue-score 10 --red-score 0 \
    --players '[{"name":"BP1","score":100,"team":"blue"},{"name":"BP2","score":90,"team":"blue"},{"name":"BP3","score":80,"team":"blue"},{"name":"BP4","score":70,"team":"blue"},{"name":"BP5","score":60,"team":"blue"},{"name":"BP6","score":50,"team":"blue"},{"name":"BP7","score":40,"team":"blue"},{"name":"BP8","score":30,"team":"blue"},{"name":"BP9","score":20,"team":"blue"},{"name":"BP10","score":10,"team":"blue"}]' \
    --spectators '["S1","S2","S3","S4","S5","S6","S7","S8","S9","S10"]'
run "team_1r1b_12spec" --gametype team --map MAP01 --title "Test" --blue-score 1 --red-score 1 \
    --players '[{"name":"BP1","score":100,"team":"blue"},{"name":"RP1","score":50,"team":"red"}]' \
    --spectators '["L","M","N","O","P","Q","R","S","T","U","V","W"]'
run "team_16r16b_0spec" --gametype team --map MAP01 --title "Test" --blue-score 16 --red-score 16 \
    --players '[{"name":"BP1","score":160,"team":"blue"},{"name":"BP2","score":150,"team":"blue"},{"name":"BP3","score":140,"team":"blue"},{"name":"BP4","score":130,"team":"blue"},{"name":"BP5","score":120,"team":"blue"},{"name":"BP6","score":110,"team":"blue"},{"name":"BP7","score":100,"team":"blue"},{"name":"BP8","score":90,"team":"blue"},{"name":"BP9","score":80,"team":"blue"},{"name":"BP10","score":70,"team":"blue"},{"name":"BP11","score":60,"team":"blue"},{"name":"BP12","score":50,"team":"blue"},{"name":"BP13","score":40,"team":"blue"},{"name":"BP14","score":30,"team":"blue"},{"name":"BP15","score":20,"team":"blue"},{"name":"BP16","score":10,"team":"blue"},{"name":"RP1","score":160,"team":"red"},{"name":"RP2","score":150,"team":"red"},{"name":"RP3","score":140,"team":"red"},{"name":"RP4","score":130,"team":"red"},{"name":"RP5","score":120,"team":"red"},{"name":"RP6","score":110,"team":"red"},{"name":"RP7","score":100,"team":"red"},{"name":"RP8","score":90,"team":"red"},{"name":"RP9","score":80,"team":"red"},{"name":"RP10","score":70,"team":"red"},{"name":"RP11","score":60,"team":"red"},{"name":"RP12","score":50,"team":"red"},{"name":"RP13","score":40,"team":"red"},{"name":"RP14","score":30,"team":"red"},{"name":"RP15","score":20,"team":"red"},{"name":"RP16","score":10,"team":"red"}]'

echo ""
echo "=========================================="
echo "  RESULTS: $PASS passed, $FAIL failed"
echo "=========================================="
echo ""
ls -lh "$OUT_DIR"/*.png 2>/dev/null | awk '{print $5, $9}' | column -t
echo ""
echo "Outputs: $OUT_DIR/"
