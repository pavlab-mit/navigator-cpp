#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$PROJECT_DIR/bin"

# Build with unit tests
"$PROJECT_DIR/build.sh" --unit_tests || exit 1

PASS=0
FAIL=0

for test_bin in "$BIN_DIR"/test_*; do
    [ -x "$test_bin" ] || continue
    name=$(basename "$test_bin")
    [ "$name" = "test_rc" ] && continue
    printf "%-25s " "$name"
    if "$test_bin" > /tmp/nav_test_out_$$ 2>&1; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        cat /tmp/nav_test_out_$$
        FAIL=$((FAIL + 1))
    fi
    rm -f /tmp/nav_test_out_$$
done

echo ""
echo "================================"
echo "PASS: $PASS  FAIL: $FAIL"
echo "================================"
[ $FAIL -eq 0 ]
