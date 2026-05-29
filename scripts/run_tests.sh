#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

PASS=0
FAIL=0
SKIP=0

for test_bin in "$BUILD_DIR"/test_*; do
    [ -x "$test_bin" ] || continue
    name=$(basename "$test_bin")
    printf "%-25s " "$name"
    if "$test_bin" > /tmp/nav_test_out_$$ 2>&1; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        status=$?
        if [ $status -eq 2 ]; then
            echo "SKIP"
            SKIP=$((SKIP + 1))
        else
            echo "FAIL"
            cat /tmp/nav_test_out_$$
            FAIL=$((FAIL + 1))
        fi
    fi
    rm -f /tmp/nav_test_out_$$
done

echo ""
echo "================================"
echo "PASS: $PASS  FAIL: $FAIL  SKIP: $SKIP"
echo "================================"
[ $FAIL -eq 0 ]
