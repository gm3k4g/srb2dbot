#!/usr/bin/env bash
# generate_intermission.sh  -  Generate SRB2-style intermission overlay image
#
# Creates a transparent PNG overlay that mimics SRB2's intermission screen,
# showing player rankings sorted by score. Supports Free-for-All (single
# column) and Team mode (blue/red columns) layouts with team scores.
#
# Usage:
#   ./generate_intermission.sh --gametype ffa --map MAP01 --title "..."
#     --players '[{"name":"P1","score":1500},{"name":"P2","score":1200}]'
#     --out intermission.png
#
#   ./generate_intermission.sh --gametype team --map MAP01
#     --blue-score 3 --red-score 5
#     --players '[{"name":"Sonic","score":4500,"team":"blue"},...]'
#     --thumb thumbnail.png --out team.png
#
# Dependencies: ImageMagick

set -euo pipefail

# ── Defaults ────────────────────────────────────────────────────────────────
GAMETYPE="ffa"
MAP=""
TITLE=""
BLUE_SCORE=0
RED_SCORE=0
PLAYERS_JSON=""
PLAYERS_FILE=""
THUMB=""
OUTPUT=""
WIDTH=640
HEIGHT=400
VERBOSE=false

# ── Colors (SRB2-inspired) ──────────────────────────────────────────────────
readonly C_BG="rgba(0,0,0,0.55)"
readonly C_BG_PANEL="rgba(0,0,0,0.50)"
readonly C_BG_ROW="rgba(255,255,255,0.06)"
readonly C_TITLE="#FFD700"
readonly C_BLUE="#4A9EFF"
readonly C_RED="#FF4A4A"
readonly C_PLAYER="#FFFFFF"
readonly C_SCORE="#00E5FF"
readonly C_RANK="#888888"
readonly C_SEP="rgba(255,255,255,0.12)"
readonly C_DIV="rgba(255,255,255,0.08)"

# ── Help ────────────────────────────────────────────────────────────────────
usage() {
    cat <<'USAGE'
Usage: generate_intermission.sh [options]

Options:
  --gametype <type>    "ffa" or "team" (default: ffa)
  --map <name>         Map number/code (e.g. MAP01)
  --title "<title>"    Map title/name
  --blue-score <n>     Blue team score (team mode only)
  --red-score <n>      Red team score (team mode only)
  --players "<json>"   JSON array of player objects
  --players-file <path> Read player JSON from file
  --thumb <path>       Map thumbnail for compositing behind overlay
  --out <path>         Output PNG path (required)
  --width <px>         Canvas width (default: 640)
  --height <px>        Canvas height (default: 400)
  --verbose            Print debug info
  --help               Show this help

Player JSON: [{"name":"P1","score":1500,"team":"blue"}, ...]
  team: "blue"/"red" (team mode) or omit (FFA)
USAGE
    exit 0
}

# ── Parse arguments ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --gametype)     GAMETYPE="$2"; shift 2 ;;
        --map)          MAP="$2"; shift 2 ;;
        --title)        TITLE="$2"; shift 2 ;;
        --blue-score)   BLUE_SCORE="$2"; shift 2 ;;
        --red-score)    RED_SCORE="$2"; shift 2 ;;
        --players)      PLAYERS_JSON="$2"; shift 2 ;;
        --players-file) PLAYERS_FILE="$2"; shift 2 ;;
        --thumb)        THUMB="$2"; shift 2 ;;
        --out)          OUTPUT="$2"; shift 2 ;;
        --width)        WIDTH="$2"; shift 2 ;;
        --height)       HEIGHT="$2"; shift 2 ;;
        --verbose)      VERBOSE=true; shift ;;
        --help)         usage ;;
        *)              echo "ERROR: Unknown option: $1" >&2; usage ;;
    esac
done

[[ -z "$OUTPUT" ]] && { echo "ERROR: --out is required" >&2; usage; }
[[ -n "$PLAYERS_FILE" && -n "$PLAYERS_JSON" ]] && { echo "ERROR: Use --players OR --players-file, not both" >&2; exit 1; }
[[ -n "$PLAYERS_FILE" ]] && PLAYERS_JSON=$(cat "$PLAYERS_FILE")
[[ "$GAMETYPE" != "ffa" && "$GAMETYPE" != "team" ]] && { echo "ERROR: --gametype must be 'ffa' or 'team'" >&2; exit 1; }

# ── Detect ImageMagick ──────────────────────────────────────────────────────
MAGICK=""
command -v magick &>/dev/null && MAGICK="magick" || { command -v convert &>/dev/null && MAGICK="convert"; }
[[ -z "$MAGICK" ]] && { echo "ERROR: ImageMagick not found" >&2; exit 1; }

P="$MAGICK"
$VERBOSE && echo "[intermission] using $P"

# ── Font detection (MVG font-size, not CLI pointsize) ──────────────────────
FONT_TITLE=""
FONT_MONO=""
FONT_LIST=$(mktemp /tmp/fonts_XXXXXX.txt)
"$P" -list font > "$FONT_LIST" 2>/dev/null || true
if grep -qi "DejaVu-Sans-Bold" "$FONT_LIST"; then
    FONT_TITLE="DejaVu-Sans-Bold"
elif grep -qi "DejaVu-Sans" "$FONT_LIST"; then
    FONT_TITLE="DejaVu-Sans"
fi
if grep -qi "DejaVu-Sans-Mono" "$FONT_LIST"; then
    FONT_MONO="DejaVu-Sans-Mono"
elif grep -qi "Liberation-Mono" "$FONT_LIST"; then
    FONT_MONO="Liberation-Mono"
fi
rm -f "$FONT_LIST"

$VERBOSE && echo "[intermission] title-font='$FONT_TITLE' mono-font='$FONT_MONO'"

# ── Parse player JSON ──────────────────────────────────────────────────────
PLAYER_NAMES=()
PLAYER_SCORES=()
PLAYER_TEAMS=()

if [[ -n "$PLAYERS_JSON" ]]; then
    flat=$(echo "$PLAYERS_JSON" | tr -d '\n\r\t')
    flat="${flat#\[}"
    flat="${flat%\]}"
    flat="${flat#"${flat%%[![:space:]]*}"}"
    while [[ -n "$flat" ]]; do
        flat="${flat#"${flat%%[![:space:]]*}"}"
        if [[ "$flat" =~ ^\{([^}]*)\}(.*)$ ]]; then
            obj="${BASH_REMATCH[1]}"
            flat="${BASH_REMATCH[2]#,}"
            flat="${flat#,}"
            pname=""; pscore=""; pteam=""
            [[ "$obj" =~ \"name\"[[:space:]]*:[[:space:]]*\"([^\"]*)\" ]] && pname="${BASH_REMATCH[1]}"
            [[ "$obj" =~ \"score\"[[:space:]]*:[[:space:]]*([0-9]+) ]] && pscore="${BASH_REMATCH[1]}"
            [[ "$obj" =~ \"team\"[[:space:]]*:[[:space:]]*\"([^\"]*)\" ]] && pteam="${BASH_REMATCH[1]}"
            [[ -n "$pname" && -n "$pscore" ]] && { PLAYER_NAMES+=("$pname"); PLAYER_SCORES+=("$pscore"); PLAYER_TEAMS+=("$pteam"); }
        else break; fi
    done
fi

NUM_PLAYERS=${#PLAYER_NAMES[@]}
$VERBOSE && echo "[intermission] parsed $NUM_PLAYERS players"

# ── Sorting helper (bubble sort indices by score descending) ────────────────
sort_desc() {
    local -n arr=$1 scores=${2:-PLAYER_SCORES}
    local len=${#arr[@]} t
    for ((i=0; i<len; i++)); do
        for ((j=0; j<len-i-1; j++)); do
            local a=${arr[$j]} b=${arr[$((j+1))]}
            if [[ ${scores[$a]} -lt ${scores[$b]} ]]; then
                t="${arr[$j]}"; arr[$j]="${arr[$((j+1))]}"; arr[$((j+1))]="$t"
            fi
        done
    done
}

# ── Generate MVG ────────────────────────────────────────────────────────────
gen_mvg() {
    local f
    f=$(mktemp /tmp/intermission_XXXXXX.mvg)
    local cw=$((WIDTH - 80))
    local cl=$(((WIDTH - cw) / 2))
    local pt=60
    local pb=$((HEIGHT - 15))

    # Begin graphic context
    echo "push graphic-context" > "$f"

    # ── Header bar ──
    echo "  fill '$C_BG'" >> "$f"
    echo "  rectangle 0,0 $WIDTH,48" >> "$f"
    echo "  fill '$C_SEP'" >> "$f"
    echo "  rectangle 0,48 $WIDTH,49" >> "$f"

    # ── Title text ──
    local title_text="$MAP${TITLE:+ - $TITLE}"
    [[ -z "$title_text" ]] && title_text="Round Results"
    [[ -n "$FONT_TITLE" ]] && echo "  font '$FONT_TITLE'" >> "$f"
    echo "  fill '$C_TITLE'" >> "$f"
    echo "  font-size 22" >> "$f"
    echo "  text-anchor middle" >> "$f"
    echo "  text $((WIDTH/2)),30 '$title_text'" >> "$f"
    echo "  text-anchor start" >> "$f"

    if [[ "$GAMETYPE" == "team" ]]; then
        gen_team "$f" "$pt" "$pb"
    else
        gen_ffa "$f" "$cl" "$cw" "$pt" "$pb"
    fi

    echo "pop graphic-context" >> "$f"
    echo "$f"
}

# ── Team mode ───────────────────────────────────────────────────────────────
gen_team() {
    local f=$1 pt=$2 pb=$3
    local hw=$((WIDTH/2))
    local cw=$((hw - 30))
    local lx=10 rx=$((hw + 10))
    local rs=$((pt + 55)) rh=26

    # Blue panel
    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $lx,$pt $((lx+cw)),$pb" >> "$f"

    # Blue header highlight
    echo "  fill 'rgba(74,158,255,0.15)'" >> "$f"
    echo "  rectangle $lx,$pt $((lx+cw)),$((pt+42))" >> "$f"

    [[ -n "$FONT_TITLE" ]] && echo "  font '$FONT_TITLE'" >> "$f"
    echo "  fill '$C_BLUE'" >> "$f"
    echo "  font-size 17" >> "$f"
    echo "  text $((lx+10)),$((pt+22)) 'BLUE'" >> "$f"
    echo "  fill '$C_SCORE'" >> "$f"
    echo "  font-size 15" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((lx+cw-8)),$((pt+22)) 'Score: $BLUE_SCORE'" >> "$f"
    echo "  text-anchor start" >> "$f"

    # Red panel
    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $rx,$pt $((rx+cw)),$pb" >> "$f"

    echo "  fill 'rgba(255,74,74,0.15)'" >> "$f"
    echo "  rectangle $rx,$pt $((rx+cw)),$((pt+42))" >> "$f"

    [[ -n "$FONT_TITLE" ]] && echo "  font '$FONT_TITLE'" >> "$f"
    echo "  fill '$C_RED'" >> "$f"
    echo "  font-size 17" >> "$f"
    echo "  text $((rx+10)),$((pt+22)) 'RED'" >> "$f"
    echo "  fill '$C_SCORE'" >> "$f"
    echo "  font-size 15" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((rx+cw-8)),$((pt+22)) 'Score: $RED_SCORE'" >> "$f"
    echo "  text-anchor start" >> "$f"

    # Separator line
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  line $hw,$pt $hw,$pb" >> "$f"

    # Split players by team
    local bi=() ri=()
    for ((i=0; i<NUM_PLAYERS; i++)); do
        case "${PLAYER_TEAMS[$i]}" in
            blue|1|2) bi+=("$i") ;;
            *)        ri+=("$i") ;;
        esac
    done
    sort_desc bi; sort_desc ri

    echo "  stroke none" >> "$f"

    # Render team column
    render_col() {
        local f=$1 x=$2 cw=$3 y=$4 rh=$5; shift 5
        local -n ids=$1
        local r=1
        [[ -n "$FONT_MONO" ]] && echo "  font '$FONT_MONO'" >> "$f"
        for idx in "${ids[@]}"; do
            [[ $y -ge $((pb - rh)) ]] && break
            if [[ $((r % 2)) -eq 0 ]]; then
                echo "  fill '$C_BG_ROW'" >> "$f"
                echo "  rectangle $((x+3)),$((y-20)) $((x+cw-3)),$((y+4))" >> "$f"
            fi
            echo "  fill '$C_RANK'" >> "$f"
            echo "  font-size 14" >> "$f"
            echo "  text $((x+10)),$y '$r.'" >> "$f"
            local name="${PLAYER_NAMES[$idx]}"
            [[ ${#name} -gt 16 ]] && name="${name:0:14}.."
            echo "  fill '$C_PLAYER'" >> "$f"
            echo "  text $((x+34)),$y '$name'" >> "$f"
            echo "  fill '$C_SCORE'" >> "$f"
            echo "  font-size 13" >> "$f"
            echo "  text-anchor end" >> "$f"
            echo "  text $((x+cw-8)),$y '${PLAYER_SCORES[$idx]}'" >> "$f"
            echo "  text-anchor start" >> "$f"
            y=$((y + rh)); r=$((r + 1))
        done
    }

    render_col "$f" "$lx" "$cw" "$rs" "$rh" bi
    render_col "$f" "$rx" "$cw" "$rs" "$rh" ri
}

# ── FFA mode ────────────────────────────────────────────────────────────────
gen_ffa() {
    local f=$1 cl=$2 cw=$3 pt=$4 pb=$5
    local rs=$((pt + 48)) rh=28

    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $cl,$pt $((cl+cw)),$pb" >> "$f"

    # Header row
    echo "  fill 'rgba(255,255,255,0.08)'" >> "$f"
    echo "  rectangle $cl,$pt $((cl+cw)),$((pt+38))" >> "$f"

    [[ -n "$FONT_MONO" ]] && echo "  font '$FONT_MONO'" >> "$f"
    echo "  fill '$C_RANK'" >> "$f"
    echo "  font-size 12" >> "$f"
    echo "  text $((cl+14)),$((pt+24)) '#'" >> "$f"
    echo "  text $((cl+42)),$((pt+24)) 'Player'" >> "$f"
    echo "  fill '$C_TITLE'" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((cl+cw-12)),$((pt+24)) 'Score'" >> "$f"
    echo "  text-anchor start" >> "$f"

    # Divider
    echo "  stroke '$C_DIV'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  line $((cl+6)),$((pt+38)) $((cl+cw-6)),$((pt+38))" >> "$f"
    echo "  stroke none" >> "$f"

    # Sort and render
    local all=()
    for ((i=0; i<NUM_PLAYERS; i++)); do all+=("$i"); done
    sort_desc all

    [[ -n "$FONT_MONO" ]] && echo "  font '$FONT_MONO'" >> "$f"
    local y=$rs r=1
    for idx in "${all[@]}"; do
        [[ $y -ge $((pb - rh)) ]] && break
        if [[ $((r % 2)) -eq 0 ]]; then
            echo "  fill '$C_BG_ROW'" >> "$f"
            echo "  rectangle $((cl+4)),$((y-22)) $((cl+cw-4)),$((y+4))" >> "$f"
        fi
        echo "  fill '$C_RANK'" >> "$f"
        echo "  font-size 14" >> "$f"
        echo "  text $((cl+16)),$y '$r.'" >> "$f"
        local name="${PLAYER_NAMES[$idx]}"
        [[ ${#name} -gt 24 ]] && name="${name:0:22}.."
        echo "  fill '$C_PLAYER'" >> "$f"
        echo "  text $((cl+48)),$y '$name'" >> "$f"
        echo "  fill '$C_SCORE'" >> "$f"
        echo "  font-size 13" >> "$f"
        echo "  text-anchor end" >> "$f"
        echo "  text $((cl+cw-12)),$y '${PLAYER_SCORES[$idx]}'" >> "$f"
        echo "  text-anchor start" >> "$f"
        y=$((y + rh)); r=$((r + 1))
    done
}

# ── Main ─────────────────────────────────────────────────────────────────────
MVG=$(gen_mvg)
$VERBOSE && echo "[intermission] MVG: $MVG"

TMP1=$(mktemp /tmp/overlay_XXXXXX.png)
TMP2=$(mktemp /tmp/overlay_bg_XXXXXX.png)

# Render overlay (transparent background)
"$P" -size "${WIDTH}x${HEIGHT}" xc:none \
    -fill none -stroke none \
    -draw "@$MVG" \
    -alpha on \
    "$TMP1"

# Composite with thumbnail if provided
if [[ -n "$THUMB" && -f "$THUMB" ]]; then
    tw=$("$P" "$THUMB" -format "%w" info: 2>/dev/null || echo 0)
    if [[ "$tw" -gt 0 ]]; then
        "$P" "$THUMB" -resize "${WIDTH}x${HEIGHT}^" \
            -gravity center -extent "${WIDTH}x${HEIGHT}" "$TMP2"
        "$P" "$TMP2" "$TMP1" -gravity center -composite "$OUTPUT"
    else
        cp "$TMP1" "$OUTPUT"
    fi
else
    cp "$TMP1" "$OUTPUT"
fi

rm -f "$TMP1" "$TMP2" "$MVG"

echo "Generated: $OUTPUT (${WIDTH}x${HEIGHT}, $NUM_PLAYERS players, $GAMETYPE)"
