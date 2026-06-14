#!/usr/bin/env bash
# Run the srb2dbot Lua hook test suite inside SRB2's Lua interpreter
# Usage: ./scripts/run_lua_tests.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SRB2_HOME="${HOME}/.srb2"
LUA_DIR="${SRB2_HOME}/luafiles"
TEST_RESULTS="${LUA_DIR}/test_results.txt"
TEST_MSGS="${LUA_DIR}/test_Messages.txt"

# Clean previous test artifacts
rm -f "$TEST_RESULTS" "$TEST_MSGS"

echo "=== Running Lua hook tests ==="
echo "Test script: ${SCRIPT_DIR}/test_hooks.lua"
echo ""

# Launch SRB2 dedicated with the test file
timeout 15 srb2 -dedicated -file "${SCRIPT_DIR}/test_hooks.lua" 2>/dev/null || true

echo ""

# Check results
if [ -f "$TEST_RESULTS" ]; then
    cat "$TEST_RESULTS"
    echo ""
    if grep -q "0 failed" "$TEST_RESULTS"; then
        echo "=== ALL TESTS PASSED ==="
        exit 0
    else
        echo "=== SOME TESTS FAILED ==="
        exit 1
    fi
else
    echo "ERROR: Test results file not found at ${TEST_RESULTS}"
    echo "SRB2 may have failed to start or the test script had errors."
    exit 1
fi
