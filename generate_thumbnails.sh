#!/usr/bin/env bash
# generate_thumbnails.sh  -  Extract map level select pictures from PK3 files
# Usage: ./generate_thumbnails.sh [maps_directory] [output_directory]
#
# Extracts MAPxxP.lmp lumps from PK3 files and converts them to PNG
# using Python with Pillow. Handles SRB2's Doom picture format.

set -euo pipefail

MAPPACKS="${1:-$HOME/.srb2}"
OUTDIR="${2:-$HOME/.srb2/luafiles/client/DiscordBot}"

mkdir -p "$OUTDIR"

echo "Scanning $MAPPACKS for level select pictures..."
echo "Output: $OUTDIR"
echo ""

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

while IFS= read -r -d '' pk3; do
    unzip -o "$pk3" 'Level select pictures/*' -d "$TMPDIR" 2>/dev/null || true
done < <(find "$MAPPACKS" -name '*.pk3' -print0 2>/dev/null || true)

export PY_SRCDIR="$TMPDIR"
export PY_OUTDIR="$OUTDIR"
export PY_MAPPACKS="$MAPPACKS"

python3 "$HOME/.srb2dbot/lmp2png.py" || true

echo ""
echo "Done."
