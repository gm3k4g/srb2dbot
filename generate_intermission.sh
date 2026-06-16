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
ROUND_TIME=""
BLUE_SCORE=0
RED_SCORE=0
PLAYERS_JSON=""
PLAYERS_FILE=""
THUMB=""
OUTPUT=""
VERBOSE=false

# Canvas
WIDTH=640
HEIGHT=400

# Layout parameters (all configurable via CLI)
MARGIN=40              # horizontal margin: canvas edge → panel edge
HEADER_H=58            # header bar height at top
HEADER_GAP=9           # gap between header bar bottom → panel top
BOTTOM_GAP=15          # gap between panel bottom → canvas bottom
ROW_H_FFA=28           # per-player row height (FFA)
ROW_H_TEAM=26          # per-player row height (team)
COL_GAP=30             # gap between columns (team mode only)
PANEL_PAD=4            # inset: panel inner edge → row background edge

# Font sizes
FONT_SZ_TITLE=22
FONT_SZ_HDR=12
FONT_SZ_ROW=14
FONT_SZ_SCORE=13
FONT_SZ_TEAM=17
FONT_SZ_TSCORE=15

# ── Colors (SRB2-inspired) ──────────────────────────────────────────────────
readonly C_BG="rgba(0,0,0,0.55)"
readonly C_BG_PANEL="rgba(0,0,0,0.50)"
readonly C_BG_ROW="rgba(255,255,255,0.06)"
readonly C_HDR="rgba(255,255,255,0.08)"
readonly C_HDR_BLUE="rgba(74,158,255,0.15)"
readonly C_HDR_RED="rgba(255,74,74,0.15)"
readonly C_TITLE="#FFD700"
readonly C_BLUE="#4A9EFF"
readonly C_RED="#FF4A4A"
readonly C_PLAYER="#FFFFFF"
readonly C_SCORE="#00E5FF"
readonly C_RANK="#888888"
readonly C_SEP="rgba(255,255,255,0.12)"
readonly C_DIV="rgba(255,255,255,0.08)"

# ── Hitbox tracker ──────────────────────────────────────────────────────────
HITBOXES=()

hitbox_check() {
    local err=0
    for ((i=0; i<${#HITBOXES[@]}; i++)); do
        IFS=',' read -r ax1 ay1 ax2 ay2 alabel <<< "${HITBOXES[$i]}"
        for ((j=i+1; j<${#HITBOXES[@]}; j++)); do
            IFS=',' read -r bx1 by1 bx2 by2 blabel <<< "${HITBOXES[$j]}"
            # Skip if one contains the other (intentional nesting)
            if (( ax1 >= bx1 && ay1 >= by1 && ax2 <= bx2 && ay2 <= by2 )); then continue; fi
            if (( bx1 >= ax1 && by1 >= ay1 && bx2 <= ax2 && by2 <= ay2 )); then continue; fi
            if (( ax2 > bx1 && bx2 > ax1 && ay2 > by1 && by2 > ay1 )); then
                echo "ERROR: Hitbox overlap: '$alabel' ($ax1,$ay1,$ax2,$ay2)  ✕  '$blabel' ($bx1,$by1,$bx2,$by2)" >&2
                err=1
            fi
        done
    done
    if (( err )); then
        echo "ERROR: Hitbox validation failed — overlapping rectangles detected" >&2
        exit 1
    fi
    if $VERBOSE; then echo "[intermission] hitbox validation: ${#HITBOXES[@]} rectangles, 0 overlaps"; fi
}

# ── Help ────────────────────────────────────────────────────────────────────
usage() {
    cat <<'USAGE'
Usage: generate_intermission.sh [options]

Required:
  --out <path>              Output PNG path

Game type:
  --gametype <type>         "ffa" or "team" (default: ffa)

Map info:
  --map <name>              Map number/code (e.g. MAP01)
  --title "<title>"         Map title/name

Scores (team mode):
  --blue-score <n>          Blue team score
  --red-score <n>           Red team score

Player data (one required):
  --players "<json>"        JSON array of player objects
  --players-file <path>     Read player JSON from file

Compositing:
  --thumb <path>            Map thumbnail to composite behind overlay

Canvas:
  --width <px>              Canvas width (default: 640)
  --height <px>             Canvas height (default: 400)

Layout parameters:
  --margin <px>             Horizontal margin: canvas edge → panel (default: 40)
  --header-h <px>           Top header bar height (default: 48)
  --header-gap <px>         Gap header bar → panel top (default: 11)
  --bottom-gap <px>         Gap panel bottom → canvas bottom (default: 15)
  --row-h-ffa <px>          Per-player row height FFA (default: 28)
  --row-h-team <px>         Per-player row height team (default: 26)
  --col-gap <px>            Gap between team columns (default: 30)
  --panel-pad <px>          Panel inner edge → row bg inset (default: 4)

Font sizes:
  --font-title <px>         Title text (default: 22)
  --font-header <px>        Column header labels (default: 12)
  --font-row <px>           Player name (default: 14)
  --font-score <px>         Score number (default: 13)
  --font-team <px>          Team label (default: 17)
  --font-tscore <px>        Team score (default: 15)

Other:
  --verbose                 Print debug info
  --help                    Show this help

Map info:
  --map <name>              Map number/code (e.g. MAP01)
  --title "<name>"          Map name (e.g. "Green Flower Zone")
  --round-time "<string>"   Round duration shown in title bar (e.g. "2:30")

Scores (team mode):
  --blue-score <n>          Blue team score
  --red-score <n>           Red team score

Player data (one required):
  --players "<json>"        JSON array of player objects
  --players-file <path>     Read player JSON from file

Title bar (two lines):
  Line 1: {gametype} - {round_time}    (e.g. "FFA - 3:45")
  Line 2: {title} ({map})              (e.g. "Green Flower Zone (MAP01)")

Player JSON format:
  [{"name":"P1","score":1500,"team":"blue"}, ...]
  team: "blue"/"red" (team) or omit (FFA).
USAGE
    exit 0
}

# ── Parse arguments ─────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --gametype)     GAMETYPE="$2"; shift 2 ;;
        --map)          MAP="$2"; shift 2 ;;
        --title)        TITLE="$2"; shift 2 ;;
        --round-time)   ROUND_TIME="$2"; shift 2 ;;
        --blue-score)   BLUE_SCORE="$2"; shift 2 ;;
        --red-score)    RED_SCORE="$2"; shift 2 ;;
        --players)      PLAYERS_JSON="$2"; shift 2 ;;
        --players-file) PLAYERS_FILE="$2"; shift 2 ;;
        --thumb)        THUMB="$2"; shift 2 ;;
        --out)          OUTPUT="$2"; shift 2 ;;
        --width)        WIDTH="$2"; shift 2 ;;
        --height)       HEIGHT="$2"; shift 2 ;;
        --margin)       MARGIN="$2"; shift 2 ;;
        --header-h)     HEADER_H="$2"; shift 2 ;;
        --header-gap)   HEADER_GAP="$2"; shift 2 ;;
        --bottom-gap)   BOTTOM_GAP="$2"; shift 2 ;;
        --row-h-ffa)    ROW_H_FFA="$2"; shift 2 ;;
        --row-h-team)   ROW_H_TEAM="$2"; shift 2 ;;
        --col-gap)      COL_GAP="$2"; shift 2 ;;
        --panel-pad)    PANEL_PAD="$2"; shift 2 ;;
        --font-title)   FONT_SZ_TITLE="$2"; shift 2 ;;
        --font-header)  FONT_SZ_HDR="$2"; shift 2 ;;
        --font-row)     FONT_SZ_ROW="$2"; shift 2 ;;
        --font-score)   FONT_SZ_SCORE="$2"; shift 2 ;;
        --font-team)    FONT_SZ_TEAM="$2"; shift 2 ;;
        --font-tscore)  FONT_SZ_TSCORE="$2"; shift 2 ;;
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
if $VERBOSE; then echo "[intermission] using $P"; fi

# ── Font detection ──────────────────────────────────────────────────────────
FONT_TITLE=""; FONT_MONO=""
FL=$(mktemp /tmp/fonts_XXXXXX.txt)
"$P" -list font > "$FL" 2>/dev/null || true
grep -qi "DejaVu-Sans-Bold" "$FL" && FONT_TITLE="DejaVu-Sans-Bold" || { grep -qi "DejaVu-Sans" "$FL" && FONT_TITLE="DejaVu-Sans"; }
grep -qi "DejaVu-Sans-Mono" "$FL" && FONT_MONO="DejaVu-Sans-Mono" || { grep -qi "Liberation-Mono" "$FL" && FONT_MONO="Liberation-Mono"; }
rm -f "$FL"
if $VERBOSE; then echo "[intermission] title-font='$FONT_TITLE' mono-font='$FONT_MONO'"; fi

# ── Parse player JSON ──────────────────────────────────────────────────────
PLAYER_NAMES=(); PLAYER_SCORES=(); PLAYER_TEAMS=()
if [[ -n "$PLAYERS_JSON" ]]; then
    flat=$(echo "$PLAYERS_JSON" | tr -d '\n\r\t')
    flat="${flat#\[}"; flat="${flat%\]}"
    flat="${flat#"${flat%%[![:space:]]*}"}"
    while [[ -n "$flat" ]]; do
        flat="${flat#"${flat%%[![:space:]]*}"}"
        if [[ "$flat" =~ ^\{([^}]*)\}(.*)$ ]]; then
            obj="${BASH_REMATCH[1]}"; flat="${BASH_REMATCH[2]#,}"; flat="${flat#,}"
            pname=""; pscore=""; pteam=""
            [[ "$obj" =~ \"name\"[[:space:]]*:[[:space:]]*\"([^\"]*)\" ]] && pname="${BASH_REMATCH[1]}"
            [[ "$obj" =~ \"score\"[[:space:]]*:[[:space:]]*([0-9]+) ]] && pscore="${BASH_REMATCH[1]}"
            [[ "$obj" =~ \"team\"[[:space:]]*:[[:space:]]*\"([^\"]*)\" ]] && pteam="${BASH_REMATCH[1]}"
            [[ -n "$pname" && -n "$pscore" ]] && { PLAYER_NAMES+=("$pname"); PLAYER_SCORES+=("$pscore"); PLAYER_TEAMS+=("$pteam"); }
        else break; fi
    done
fi
NUM_PLAYERS=${#PLAYER_NAMES[@]}
if $VERBOSE; then echo "[intermission] parsed $NUM_PLAYERS players"; fi

# ── Sorting helper ──────────────────────────────────────────────────────────
sort_desc() {
    local -n arr=$1 scores=${2:-PLAYER_SCORES}
    local len=${#arr[@]} t
    for ((i=0; i<len; i++)); do for ((j=0; j<len-i-1; j++)); do
        local a=${arr[$j]} b=${arr[$((j+1))]}
        if [[ ${scores[$a]} -lt ${scores[$b]} ]]; then
            t="${arr[$j]}"; arr[$j]="${arr[$((j+1))]}"; arr[$((j+1))]="$t"
        fi
    done; done
}

# ── Generate MVG (writes to a file, returns path) ──────────────────────────
gen_mvg() {
    local f
    f=$(mktemp /tmp/intermission_XXXXXX.mvg)
    local pt=$((HEADER_H + HEADER_GAP))
    local pb=$((HEIGHT - BOTTOM_GAP))

    echo "push graphic-context" > "$f"

    echo "  fill '$C_BG'" >> "$f"
    echo "  rectangle 0,0 $WIDTH,$HEADER_H" >> "$f"

    echo "  fill '$C_SEP'" >> "$f"
    echo "  rectangle 0,$((HEADER_H-1)) $WIDTH,$HEADER_H" >> "$f"

    local gt_display="FFA"
    [[ "$GAMETYPE" == "team" ]] && gt_display="Team"
    local map_line="" gt_line=""
    if [[ -n "$TITLE" && -n "$MAP" ]]; then
        map_line="$TITLE ($MAP)"
    elif [[ -n "$TITLE" ]]; then
        map_line="$TITLE"
    elif [[ -n "$MAP" ]]; then
        map_line="$MAP"
    fi
    if [[ -n "$ROUND_TIME" ]]; then
        gt_line="$gt_display - $ROUND_TIME"
    else
        gt_line="$gt_display"
    fi
    [[ -z "$map_line" ]] && map_line="Round Results"

    # Line 1: gametype + round time (top, biggest font)
    [[ -n "$FONT_TITLE" ]] && echo "  font '$FONT_TITLE'" >> "$f"
    echo "  stroke none" >> "$f"
    echo "  fill '$C_TITLE'" >> "$f"
    echo "  font-size $FONT_SZ_TITLE" >> "$f"
    echo "  text-anchor middle" >> "$f"
    echo "  text $((WIDTH/2)),22 '$gt_line'" >> "$f"
    # Line 2: map name + number (bottom, smaller font)
    echo "  font-size 16" >> "$f"
    echo "  text $((WIDTH/2)),44 '$map_line'" >> "$f"
    echo "  text-anchor start" >> "$f"

    if [[ "$GAMETYPE" == "team" ]]; then
        gen_team_mvg "$f" "$pt" "$pb"
    else
        gen_ffa_mvg "$f" "$pt" "$pb"
    fi

    echo "pop graphic-context" >> "$f"
    echo "$f"
}

# ── Hitbox validation (computed in MAIN process, not subshell) ─────────────
_compute_hitboxes() {
    HITBOXES=()
    local pt=$((HEADER_H + HEADER_GAP))
    local pb=$((HEIGHT - BOTTOM_GAP))

    _hb "header-bar" 0 0 "$WIDTH" "$HEADER_H"

    if [[ "$GAMETYPE" == "team" ]]; then
        _comp_team_hb "$pt" "$pb"
    else
        _comp_ffa_hb "$pt" "$pb"
    fi
}

_hb() { HITBOXES+=("$2,$3,$4,$5,$1"); }

_comp_ffa_hb() {
    local pt=$1 pb=$2
    local cw cl hdr_h hdr_gap row_top rh pad
    cw=$((WIDTH - 2*MARGIN))
    cl=$MARGIN
    hdr_h=36; hdr_gap=14
    (( row_top = pt + hdr_h + hdr_gap ))
    rh=$ROW_H_FFA; pad=$PANEL_PAD

    _hb "ffa-panel" "$cl" "$pt" "$((cl+cw))" "$pb"
    _hb "ffa-hdr" "$cl" "$pt" "$((cl+cw))" "$((pt+hdr_h))"

    local all=()
    for ((i=0; i<NUM_PLAYERS; i++)); do all+=("$i"); done
    sort_desc all

    local rt=$row_top r=1
    for idx in "${all[@]}"; do
        [[ $rt -ge $((pb - rh)) ]] && break
        if [[ $((r % 2)) -eq 0 ]]; then
            _hb "ffa-row-$r" "$((cl+pad))" "$((rt+2))" "$((cl+cw-pad))" "$((rt+rh-2))"
        fi
        rt=$((rt + rh)); r=$((r + 1))
    done
}

_comp_team_hb() {
    local pt=$1 pb=$2
    local cw lx rx hdr_h hdr_gap row_top rh pad
    cw=$(( (WIDTH - COL_GAP) / 2 - MARGIN ))
    lx=$MARGIN
    (( rx = lx + cw + COL_GAP ))
    hdr_h=40; hdr_gap=16
    (( row_top = pt + hdr_h + hdr_gap ))
    rh=$ROW_H_TEAM; pad=$PANEL_PAD

    _hb "team-blue-panel" "$lx" "$pt" "$((lx+cw))" "$pb"
    _hb "team-blue-hdr" "$lx" "$pt" "$((lx+cw))" "$((pt+hdr_h))"
    _hb "team-red-panel" "$rx" "$pt" "$((rx+cw))" "$pb"
    _hb "team-red-hdr" "$rx" "$pt" "$((rx+cw))" "$((pt+hdr_h))"

    local bi=() ri=()
    for ((i=0; i<NUM_PLAYERS; i++)); do
        case "${PLAYER_TEAMS[$i]}" in blue|1|2) bi+=("$i") ;; *) ri+=("$i") ;; esac
    done
    sort_desc bi; sort_desc ri

    _comp_col_hb "$lx" "$cw" "$row_top" "$rh" "$pad" "$pb" bi
    _comp_col_hb "$rx" "$cw" "$row_top" "$rh" "$pad" "$pb" ri
}

_comp_col_hb() {
    local x=$1 cw=$2 row_top=$3 rh=$4 pad=$5 pb=$6; shift 6
    local -n ids=$1
    local rt=$row_top r=1
    for idx in "${ids[@]}"; do
        [[ $rt -ge $((pb - rh)) ]] && break
        if [[ $((r % 2)) -eq 0 ]]; then
            _hb "team-row-$r" "$((x+pad))" "$((rt+2))" "$((x+cw-pad))" "$((rt+rh-2))"
        fi
        rt=$((rt + rh)); r=$((r + 1))
    done
}

# ── Shared row rendering ────────────────────────────────────────────────────
# Row model:
#   row_top = start of the row area
#   text_baseline = row_top + rh - 4  (4px from bottom for descenders)
#   row background: row_top + 2  to  row_top + rh - 2  (2px inset each side)

_render_rows() {
    local f=$1 x=$2 cw=$3 row_top=$4 rh=$5 pad=$6 pb=$7; shift 7
    local -n ids=$1
    local r=1
    [[ -n "$FONT_MONO" ]] && echo "  font '$FONT_MONO'" >> "$f"
    for idx in "${ids[@]}"; do
        [[ $row_top -ge $((pb - rh)) ]] && break
        local tb=$(( row_top + rh - 4 ))
        local bg_y1=$(( row_top + 2 ))
        local bg_y2=$(( row_top + rh - 2 ))
        if [[ $((r % 2)) -eq 0 ]]; then
            echo "  fill '$C_BG_ROW'" >> "$f"
            echo "  rectangle $((x+pad)),$bg_y1 $((x+cw-pad)),$bg_y2" >> "$f"
        fi
        local name="${PLAYER_NAMES[$idx]}"
        local maxlen=24
        [[ "$GAMETYPE" == "team" ]] && maxlen=16
        [[ ${#name} -gt $maxlen ]] && name="${name:0:$((maxlen-2))}.."
        echo "  fill '$C_RANK'" >> "$f"
        echo "  font-size $FONT_SZ_ROW" >> "$f"
        echo "  text $((x+10)),$tb '$r.'" >> "$f"
        local name_x=$(( x + 34 ))
        [[ "$GAMETYPE" == "ffa" ]] && name_x=$(( x + 40 ))
        echo "  fill '$C_PLAYER'" >> "$f"
        echo "  text $name_x,$tb '$name'" >> "$f"
        echo "  fill '$C_SCORE'" >> "$f"
        echo "  font-size $FONT_SZ_SCORE" >> "$f"
        echo "  text-anchor end" >> "$f"
        echo "  text $((x+cw-10)),$tb '${PLAYER_SCORES[$idx]}'" >> "$f"
        echo "  text-anchor start" >> "$f"
        row_top=$((row_top + rh)); r=$((r + 1))
    done
}

# ── FFA mode ────────────────────────────────────────────────────────────────
gen_ffa_mvg() {
    local f=$1 pt=$2 pb=$3
    local cw=$((WIDTH - 2*MARGIN))
    local cl=$MARGIN
    local hdr_h=36
    local hdr_gap=14
    local row_top=$((pt + hdr_h + hdr_gap))
    local rh=$ROW_H_FFA
    local pad=$PANEL_PAD

    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $cl,$pt $((cl+cw)),$pb" >> "$f"

    echo "  stroke none" >> "$f"
    echo "  fill '$C_HDR'" >> "$f"
    echo "  rectangle $cl,$pt $((cl+cw)),$((pt+hdr_h))" >> "$f"

    local hdr_y=$((pt + hdr_h/2 + 4))
    [[ -n "$FONT_MONO" ]] && echo "  font '$FONT_MONO'" >> "$f"
    echo "  fill '$C_RANK'" >> "$f"
    echo "  font-size $FONT_SZ_HDR" >> "$f"
    echo "  text $((cl+14)),$hdr_y '#'" >> "$f"
    echo "  text $((cl+36)),$hdr_y 'Player'" >> "$f"
    echo "  fill '$C_TITLE'" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((cl+cw-14)),$hdr_y 'Score'" >> "$f"
    echo "  text-anchor start" >> "$f"

    echo "  stroke '$C_DIV'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  line $((cl+6)),$((pt+hdr_h)) $((cl+cw-6)),$((pt+hdr_h))" >> "$f"
    echo "  stroke none" >> "$f"

    local all=()
    for ((i=0; i<NUM_PLAYERS; i++)); do all+=("$i"); done
    sort_desc all
    _render_rows "$f" "$cl" "$cw" "$row_top" "$rh" "$pad" "$pb" all
}

# ── Team mode ───────────────────────────────────────────────────────────────
gen_team_mvg() {
    local f=$1 pt=$2 pb=$3
    local cw=$(( (WIDTH - COL_GAP) / 2 - MARGIN ))
    local lx=$MARGIN
    local rx=$(( lx + cw + COL_GAP ))
    local hw=$(( lx + cw + COL_GAP/2 ))
    local hdr_h=40
    local hdr_gap=16
    local row_top=$((pt + hdr_h + hdr_gap))
    local rh=$ROW_H_TEAM
    local pad=$PANEL_PAD

    # Blue column
    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $lx,$pt $((lx+cw)),$pb" >> "$f"
    echo "  stroke none" >> "$f"
    echo "  fill '$C_HDR_BLUE'" >> "$f"
    echo "  rectangle $lx,$pt $((lx+cw)),$((pt+hdr_h))" >> "$f"

    # Red column
    echo "  fill '$C_BG_PANEL'" >> "$f"
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  rectangle $rx,$pt $((rx+cw)),$pb" >> "$f"
    echo "  stroke none" >> "$f"
    echo "  fill '$C_HDR_RED'" >> "$f"
    echo "  rectangle $rx,$pt $((rx+cw)),$((pt+hdr_h))" >> "$f"

    # Separator
    echo "  stroke '$C_SEP'" >> "$f"
    echo "  stroke-width 1" >> "$f"
    echo "  line $hw,$pt $hw,$pb" >> "$f"

    # Team labels
    echo "  stroke none" >> "$f"
    [[ -n "$FONT_TITLE" ]] && echo "  font '$FONT_TITLE'" >> "$f"
    local team_y=$((pt + hdr_h/2 + 2))
    echo "  fill '$C_BLUE'" >> "$f"
    echo "  font-size $FONT_SZ_TEAM" >> "$f"
    echo "  text $((lx+10)),$team_y 'BLUE'" >> "$f"
    echo "  fill '$C_SCORE'" >> "$f"
    echo "  font-size $FONT_SZ_TSCORE" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((lx+cw-10)),$team_y 'Score: $BLUE_SCORE'" >> "$f"
    echo "  text-anchor start" >> "$f"

    echo "  fill '$C_RED'" >> "$f"
    echo "  font-size $FONT_SZ_TEAM" >> "$f"
    echo "  text $((rx+10)),$team_y 'RED'" >> "$f"
    echo "  fill '$C_SCORE'" >> "$f"
    echo "  font-size $FONT_SZ_TSCORE" >> "$f"
    echo "  text-anchor end" >> "$f"
    echo "  text $((rx+cw-10)),$team_y 'Score: $RED_SCORE'" >> "$f"
    echo "  text-anchor start" >> "$f"

    local bi=() ri=()
    for ((i=0; i<NUM_PLAYERS; i++)); do
        case "${PLAYER_TEAMS[$i]}" in blue|1|2) bi+=("$i") ;; *) ri+=("$i") ;; esac
    done
    sort_desc bi; sort_desc ri

    _render_rows "$f" "$lx" "$cw" "$row_top" "$rh" "$pad" "$pb" bi
    _render_rows "$f" "$rx" "$cw" "$row_top" "$rh" "$pad" "$pb" ri
}

# ── Main ─────────────────────────────────────────────────────────────────────
_compute_hitboxes
hitbox_check

# Warn if players exceed panel capacity
{ 
    pt=$((HEADER_H + HEADER_GAP))
    pb=$((HEIGHT - BOTTOM_GAP))
    if [[ "$GAMETYPE" == "ffa" ]]; then
        hdr_h=36; hdr_gap=14
        row_top=$((pt + hdr_h + hdr_gap))
        max_rows=$(( (pb - 2 - row_top) / ROW_H_FFA ))
        if [[ $NUM_PLAYERS -gt $max_rows ]]; then
            echo "WARNING: Only $max_rows of $NUM_PLAYERS players fit in panel ($((NUM_PLAYERS - max_rows)) clipped)" >&2
        fi
    else
        hdr_h=40; hdr_gap=16
        row_top=$((pt + hdr_h + hdr_gap))
        max_per_col=$(( (pb - 2 - row_top) / ROW_H_TEAM ))
        max_total=$(( max_per_col * 2 ))
        if [[ $NUM_PLAYERS -gt $max_total ]]; then
            echo "WARNING: Only $max_total of $NUM_PLAYERS players fit ($((NUM_PLAYERS - max_total)) clipped)" >&2
        fi
    fi
}

MVG=$(gen_mvg)
if $VERBOSE; then echo "[intermission] MVG: $MVG"; fi

TMP1=$(mktemp /tmp/overlay_XXXXXX.png)
TMP2=$(mktemp /tmp/overlay_bg_XXXXXX.png)

"$P" -size "${WIDTH}x${HEIGHT}" xc:none \
    -fill none -stroke none \
    -draw "@$MVG" \
    -alpha on \
    "$TMP1"

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
