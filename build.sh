#!/usr/bin/env bash
set -euo pipefail

# ── defaults ──────────────────────────────────────────────
BUILD_DIR="build"
BUILD_TYPE="Debug"
JOBS="$(nproc)"
RUN_TESTS=0
CLEAN=0
RUN_BOT=0

# ── colors ────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# ── helpers ───────────────────────────────────────────────
usage() {
    cat <<EOF
${BOLD}srb2dbot build script${NC}

Usage: $0 [options]

Options:
  -h, --help      Show this help message
  -c, --clean     Remove build directory and reconfigure from scratch
  -r, --release   Build in Release mode (default: Debug)
  -j, --jobs N    Number of parallel build jobs (default: 2)
  -t, --test      Run tests after building
  --run           Run the bot after building
  -B, --build-dir DIR  Set build directory (default: build)

Examples:
  $0                         Debug build
  $0 --release               Release build
  $0 --clean --release       Clean release build
  $0 --test                  Debug build + run tests
  $0 --release --run         Release build + start bot

EOF
}

die() { echo -e "${RED}[FATAL]${NC} $*" >&2; exit 1; }
info() { echo -e "${CYAN}[INFO]${NC} $*"; }
ok() { echo -e "${GREEN}[ OK ]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }

is_nixos() {
    [[ -f /etc/NIXOS ]] 2>/dev/null || command -v nixos-version &>/dev/null
}

# ── parse args ────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)      usage; exit 0 ;;
        -c|--clean)     CLEAN=1 ;;
        -r|--release)   BUILD_TYPE="Release" ;;
        -t|--test)      RUN_TESTS=1 ;;
        --run)          RUN_BOT=1 ;;
        -j|--jobs)      JOBS="$2"; shift ;;
        -B|--build-dir) BUILD_DIR="$2"; shift ;;
        *)              die "Unknown option: $1" ;;
    esac
    shift
done

DEVELOPER_MODE=$([[ "$RUN_TESTS" -eq 1 ]] && echo "ON" || echo "OFF")

# ── banner ────────────────────────────────────────────────
echo ""
echo -e "${BOLD}${GREEN}  srb2dbot${NC}  -  ${BUILD_TYPE} build  -  $(nproc) jobs"
echo ""

# ── clean if requested ────────────────────────────────────
if [[ "$CLEAN" -eq 1 ]] && [[ -d "$BUILD_DIR" ]]; then
    info "Cleaning build directory '$BUILD_DIR'"
    rm -rf "$BUILD_DIR"
fi

# ── dependency check ──────────────────────────────────────
check_dep() {
    local name="$1"
    local pkg="${2:-$1}"
    local critical="${3:-yes}"
    if command -v "$name" &>/dev/null || [[ -f "$name" ]]; then
        ok "Found: $name"
    else
        if [[ "$critical" == "yes" ]]; then
            die "$name not found. Install with: sudo apt install $pkg"
        else
            warn "$name not found (optional: sudo apt install $pkg)"
        fi
    fi
}

check_header() {
    local header="$1" name="$2" pkg="$3"
    if echo "#include <$header>" | g++ -fsyntax-only -x c++ - 2>/dev/null; then
        ok "Found: $name ($header)"
    else
        die "$name not found. Install with: sudo apt install $pkg"
    fi
}

if is_nixos; then
    command -v nix-shell &>/dev/null || die "nix-shell not found"
    info "NixOS detected  -  using nix-shell"
else
    echo ""
    info "Checking build dependencies..."
    command -v cmake &>/dev/null || die "cmake not found. Install: sudo apt install cmake"
    ok "Found: cmake"
    command -v g++ &>/dev/null || command -v clang++ &>/dev/null || \
        die "No C++ compiler found. Install: sudo apt install build-essential"
    ok "Found: C++ compiler ($(command -v g++ 2>/dev/null || command -v clang++))"
    check_header "dpp/dpp.h" "D++" "libdpp-dev"
    check_header "nlohmann/json.hpp" "nlohmann/json" "nlohmann-json3-dev"
    echo ""
    info "Checking runtime dependencies..."
    check_dep "convert" "imagemagick" "no"
    check_dep "magick" "imagemagick" "no"
    if ! command -v convert &>/dev/null && ! command -v magick &>/dev/null; then
        warn "ImageMagick not found  -  thumbnails will not be generated"
        warn "  Install: sudo apt install imagemagick"
    fi
    check_dep "unzip" "unzip" "no"
    if ! command -v unzip &>/dev/null; then
        warn "unzip not found  -  thumbnail extraction will fail"
    fi
    check_dep "bash" "bash"
    echo ""
fi

# ── build ─────────────────────────────────────────────────
SECONDS=0
cmake_args=(
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -Dsrb2dbot_DEVELOPER_MODE="$DEVELOPER_MODE"
)

do_configure() {
    cmake "${cmake_args[@]}"
}

do_build() {
    cmake --build "$BUILD_DIR" -j "$JOBS"
}

do_test() {
    info "Running tests..."
    ctest --test-dir "$BUILD_DIR" --output-on-failure \
        && ok "All tests passed" \
        || die "Tests failed"
}

if is_nixos; then
    nix-shell --run "
        cmake -B '$BUILD_DIR' -DCMAKE_BUILD_TYPE='$BUILD_TYPE' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Dsrb2dbot_DEVELOPER_MODE='$DEVELOPER_MODE' \
        && cmake --build '$BUILD_DIR' -j '$JOBS'
    " || die "Build failed"

    if [[ "$RUN_TESTS" -eq 1 ]]; then
        nix-shell --run "cd '$BUILD_DIR' && ctest --output-on-failure" \
            && ok "All tests passed" \
            || die "Tests failed"
    fi
else
    do_configure || die "CMake configure failed. Check that dpp and nlohmann_json are installed."
    do_build || die "Build failed. Check the errors above."

    if [[ "$RUN_TESTS" -eq 1 ]]; then
        do_test
    fi
fi

# ── summary ───────────────────────────────────────────────
elapsed="$SECONDS"
echo ""
echo -e "${BOLD}${GREEN}  Build complete in ${elapsed}s${NC}"
echo -e "  Binary: ${CYAN}${BUILD_DIR}/srb2dbot${NC}"
echo ""

# ── run if requested ──────────────────────────────────────
if [[ "$RUN_BOT" -eq 1 ]]; then
    if [[ ! -f secret.json ]]; then
        die "secret.json not found. Copy secret.json.default and fill in your credentials."
    fi
    warn "Checks:"
    warn "  - Message Content Intent enabled in Discord Developer Portal?"
    warn "  - Have you put valid credentials in secret.json?"
    echo ""
    exec "${BUILD_DIR}/srb2dbot"
fi
