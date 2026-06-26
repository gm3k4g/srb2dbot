#!/usr/bin/env bash
set -euo pipefail

# ── srb2dbot Docker portable builder ──────────────────────
# Builds the binary inside a Debian 12 container so it links
# against glibc 2.36 (Debian 12 compatible).
# Extracts binary + libdpp to ./build-portable/.
# ───────────────────────────────────────────────────────────

RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
die() { echo -e "${RED}[FATAL]${NC} $*" >&2; exit 1; }
info() { echo -e "${CYAN}[INFO]${NC} $*"; }
ok() { echo -e "${GREEN}[ OK ]${NC} $*"; }

command -v docker &>/dev/null || die "Docker is required"

OUTDIR="build-portable"
IMAGE_TAG="srb2dbot-builder:latest"

info "Building portable binary in Debian 12 container..."
docker build -t "$IMAGE_TAG" . 2>&1 || die "Docker build failed"

mkdir -p "$OUTDIR"

info "Extracting binary and libs..."
CID=$(docker create "$IMAGE_TAG")
docker cp "$CID:/usr/local/bin/srb2dbot" "$OUTDIR/srb2dbot"
docker cp "$CID:/usr/local/lib/libdpp.so.10.1.5" "$OUTDIR/libdpp.so.10.1.5" 2>/dev/null || true
docker rm "$CID" > /dev/null

# Set RPATH to find libdpp next to the binary
patchelf --set-rpath '$ORIGIN' "$OUTDIR/srb2dbot" 2>/dev/null || true

ok "Portable binary: $OUTDIR/srb2dbot"
ls -lh "$OUTDIR/"
echo ""
echo "  scp -r $OUTDIR/* user@server:~/.srb2dbot/"
echo "  # On server: sudo apt install libssl3 libopus0 ca-certificates"
echo ""
