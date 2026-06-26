#!/usr/bin/env bash
set -euo pipefail

# ── srb2dbot deploy script ────────────────────────────────
# Produces a portable tarball of the srb2dbot binary with
# bundled shared libraries, suitable for deployment to
# standard Linux distributions (Debian, Ubuntu, Arch, etc.).
#
# NixOS builds have /nix/store paths baked into the ELF
# interpreter and RPATH; this script uses patchelf to
# rewrite those to portable locations so the binary runs
# on systems without Nix.
# ────────────────────────────────────────────────────────────

RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
die() { echo -e "${RED}[FATAL]${NC} $*" >&2; exit 1; }
info() { echo -e "${CYAN}[INFO]${NC} $*"; }
ok() { echo -e "${GREEN}[ OK ]${NC} $*"; }

usage() {
    cat <<EOF
${BOLD}srb2dbot deploy script${NC}

Produces a portable tarball (srb2dbot-<version>-<arch>.tar.gz)
with bundled libraries for deployment to standard Linux.

Usage: $0 [options]

Options:
  -h, --help       Show this help message
  -o, --output DIR Output directory (default: ./deploy)
  -b, --build DIR  Build directory (default: ./build)
  -j, --jobs N     Parallel build jobs (default: 4)

EOF
}

OUTDIR="deploy"
BUILD_DIR="build"
JOBS="4"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)      usage; exit 0 ;;
        -o|--output)    OUTDIR="$2"; shift ;;
        -b|--build)     BUILD_DIR="$2"; shift ;;
        -j|--jobs)      JOBS="$2"; shift ;;
        *)              die "Unknown option: $1" ;;
    esac
    shift
done

is_nixos() {
    [[ -f /etc/NIXOS ]] 2>/dev/null || command -v nixos-version &>/dev/null
}

# On NixOS, batch all nix-shell-requiring commands into one script
PATCHELF_SH=$(mktemp)
cat > "$PATCHELF_SH" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
SCRIPT

patchelf_wrap() {
    echo "$@" >> "$PATCHELF_SH"
}

if is_nixos; then
    command -v nix-shell &>/dev/null || die "nix-shell not found (required on NixOS)"
    info "NixOS detected  -  using nix-shell for patchelf, native for build/libraries"
    RUN_BATCH="nix-shell -p patchelf --run 'bash $PATCHELF_SH'"
    USE_BATCH=1
else
    command -v patchelf &>/dev/null || die "patchelf is required. Install: sudo apt install patchelf"
    command -v cmake &>/dev/null || die "cmake is required"
    # Direct invocation on standard Linux
    patchelf_wrap() { "$@"; }
    RUN_BATCH=""
    USE_BATCH=0
fi

# ── Build release binary ──
if [[ ! -f "$BUILD_DIR/srb2dbot" ]]; then
    info "Building srb2dbot (Release)..."
    if is_nixos; then
        nix-shell --run "
            cmake -B '$BUILD_DIR' -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            cmake --build '$BUILD_DIR' -j '$JOBS'
        " || die "Build failed"
    else
        cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        cmake --build "$BUILD_DIR" -j "$JOBS"
    fi
else
    info "Binary already exists at $BUILD_DIR/srb2dbot (skipping build)"
fi

BINARY="$BUILD_DIR/srb2dbot"
[[ -f "$BINARY" ]] || die "Build failed: $BINARY not found"

# ── Gather metadata ──
VERSION="0.1.45"
ARCH="$(uname -m)"
DEPLOY_NAME="srb2dbot-${VERSION}-${ARCH}"
DEPLOY_DIR="$OUTDIR/$DEPLOY_NAME"
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/lib"

# ── Find all needed shared libraries ──
info "Bundling shared libraries..."
declare -A SEEN
COPIED=()
LIBC_SRC=""  # track original libc source path for ld-linux discovery

collect_libs() {
    local binary="$1"
    while IFS= read -r line; do
        [[ "$line" == *"not found"* ]] && { warn "Missing library in $line"; continue; }
        [[ "$line" != *"=>"* ]] && continue
        local lib_path
        lib_path="$(echo "$line" | awk -F'=> ' '{print $2}' | awk '{print $1}')"
        local lib_name
        lib_name="$(basename "$lib_path")"
        [[ "$lib_name" == "linux-vdso"* ]] && continue
        [[ "$lib_name" == "ld-linux"* ]] && continue
        [[ -n "${SEEN[$lib_name]:-}" ]] && continue
        SEEN["$lib_name"]=1
        if [[ -f "$lib_path" ]]; then
            cp -L "$lib_path" "$DEPLOY_DIR/lib/$lib_name"
            COPIED+=("$lib_name")
            if [[ "$lib_name" == libc.so.* ]] && [[ -z "$LIBC_SRC" ]]; then
                LIBC_SRC="$(readlink -f "$lib_path")"
            fi
            collect_libs "$lib_path"
        fi
    done < <(ldd "$binary" 2>/dev/null || true)
}

collect_libs "$BINARY"

ok "Bundled ${#COPIED[@]} libraries"

# ── Set portable RPATH and interpreter ──
info "Patching ELF for portability..."
cp "$BINARY" "$DEPLOY_DIR/srb2dbot"

# Find and bundle ld-linux from the same directory as glibc
LDLINUX=""
if [[ -n "$LIBC_SRC" ]]; then
    LIBC_DIR="$(dirname "$LIBC_SRC")"
    for ld in "$LIBC_DIR"/ld-linux*; do
        if [[ -f "$ld" ]]; then
            LDLINUX="$(basename "$ld")"
            cp -L "$ld" "$DEPLOY_DIR/lib/$LDLINUX"
            ok "Copied ld-linux ($LDLINUX) from $ld"
            break
        fi
    done
fi
if [[ -z "$LDLINUX" ]]; then
    # Fallback: search Nix store for glibc
    for d in /nix/store/*glibc*/lib/; do
        for ld in "$d"/ld-linux*; do
            if [[ -f "$ld" ]]; then
                LDLINUX="$(basename "$ld")"
                cp -L "$ld" "$DEPLOY_DIR/lib/$LDLINUX"
                ok "Copied ld-linux ($LDLINUX) from $ld (fallback)"
                break 2
            fi
        done
    done
fi

# Queue patchelf operations
patchelf_wrap "patchelf --set-rpath '\$ORIGIN/lib' '$PWD/$DEPLOY_DIR/srb2dbot'"
patchelf_wrap "patchelf --set-interpreter '\$ORIGIN/lib/$LDLINUX' '$PWD/$DEPLOY_DIR/srb2dbot'"

# Run all patchelf commands
if [[ "$USE_BATCH" -eq 1 ]]; then
    info "Running patchelf via nix-shell..."
    nix-shell -p patchelf --run "bash '$PATCHELF_SH'" || die "patchelf failed"
else
    bash "$PATCHELF_SH" || die "patchelf failed"
fi

rm -f "$PATCHELF_SH"

ok "Binary patched for portability"

# ── Copy config templates ──
cp secret.json.default "$DEPLOY_DIR/" 2>/dev/null || true
cp modules.json "$DEPLOY_DIR/" 2>/dev/null || true

# ── Create tarball ──
info "Creating tarball..."
(cd "$OUTDIR" && tar czf "${DEPLOY_NAME}.tar.gz" "$DEPLOY_NAME/")

SIZE="$(du -sh "$OUTDIR/${DEPLOY_NAME}.tar.gz" | cut -f1)"
ok "Deploy package: $OUTDIR/${DEPLOY_NAME}.tar.gz ($SIZE)"

echo ""
echo -e "${BOLD}Deploy to remote server:${NC}"
echo "  scp $OUTDIR/${DEPLOY_NAME}.tar.gz user@server:/home/user/.srb2dbot/"
echo "  ssh user@server 'cd ~/.srb2dbot && tar xzf ${DEPLOY_NAME}.tar.gz --strip-components=1 && ./srb2dbot'"
echo ""
echo -e "${BOLD}Or deploy directly from local deployment directory:${NC}"
echo "  rsync -av $DEPLOY_DIR/ user@server:/home/user/.srb2dbot/"
echo "  ssh user@server 'cd ~/.srb2dbot && ./srb2dbot'"
echo ""
