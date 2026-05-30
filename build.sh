#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-$(nproc)}"
WITH_TESTS="${WITH_TESTS:-ON}"
RUN_TESTS="${RUN_TESTS:-OFF}"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

is_nixos() {
    [ -f /etc/NIXOS ] 2>/dev/null || command -v nixos-version &>/dev/null
}

cmake_args=(
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if [ "$WITH_TESTS" = "ON" ]; then
    cmake_args+=(-Dsrb2dbot_DEVELOPER_MODE=ON)
fi

if is_nixos; then
    echo -e "${GREEN}[nix-shell] building srb2dbot (${BUILD_TYPE})${NC}"

    nix-shell --run "
        cmake ${cmake_args[*]} &&
        cmake --build '$BUILD_DIR' -j ${JOBS}
    "

    if [ "$RUN_TESTS" = "ON" ] && [ "$WITH_TESTS" = "ON" ]; then
        nix-shell --run "cd '$BUILD_DIR' && ctest --output-on-failure"
    fi
else
    echo -e "${GREEN}[linux] building srb2dbot (${BUILD_TYPE})${NC}"

    cmake "${cmake_args[@]}" || {
        echo -e "${RED}cmake configure failed. Check that dpp and nlohmann_json are installed.${NC}"
        exit 1
    }

    cmake --build "$BUILD_DIR" -j "$JOBS"

    if [ "$RUN_TESTS" = "ON" ] && [ "$WITH_TESTS" = "ON" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure
    fi
fi

echo -e "${GREEN}done${NC}"
