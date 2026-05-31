#!/usr/bin/env bash
# generate_thumbnails.sh — Extract map level select pictures from PK3 files
# Usage: ./generate_thumbnails.sh [maps_directory] [output_directory]
#
# Scans for MAPxxP.lmp lumps in extracted PK3 _FILES directories
# and converts them to PNG using ImageMagick.
# SRB2 lumps are 160x100 raw 8-bit indexed images with SRB2 palette.

set -euo pipefail

MAPPACKS="${1:-$HOME/.srb2}"
OUTDIR="${2:-$HOME/.srb2/luafiles/client/DiscordBot/thumbnails}"

mkdir -p "$OUTDIR"

command -v magick &>/dev/null && MAGICK="magick" || MAGICK="convert"

echo "Scanning $MAPPACKS for level select pictures..."
echo "Output: $OUTDIR"
echo ""

COUNT=0
while IFS= read -r -d '' lmp; do
    mapname=$(basename "$lmp" .lmp)
    png="$OUTDIR/$mapname.png"

    [[ -f "$png" ]] && continue

    "$MAGICK" -size 160x100 -depth 8 "GRAY:$lmp" -auto-level "$png" 2>/dev/null && {
        echo "  $mapname → $png"
        COUNT=$((COUNT+1))
    } || true
done < <(find "$MAPPACKS" -name "MAP*P.lmp" -print0 2>/dev/null || true)

echo ""
echo "Generated $COUNT thumbnails."
if [[ $COUNT -eq 0 ]]; then
    echo "No lumps found. Specify a maps directory:"
    echo "  $0 /path/to/extracted/pk3/files"
    echo ""
    echo "To extract PK3 files first:"
    echo "  find /path/to/addons -name '*.pk3' -exec unzip -o {} -d {}_FILES 'Level select pictures/*' \;"
fi
