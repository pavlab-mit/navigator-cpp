#!/bin/bash
set -eu

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
NAVIGATOR_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" \
  -DNAVIGATOR_CPP_ROOT="$NAVIGATOR_ROOT" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$SCRIPT_DIR/build" --parallel

echo "Built $SCRIPT_DIR/build/pwm_cycle"
