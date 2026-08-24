#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SUITE_DIR="$SCRIPT_DIR/logs/suite_$(date '+%Y%m%d_%H%M%S')"
mkdir -p "$SUITE_DIR"

echo "Validation output: $SUITE_DIR"
"$SCRIPT_DIR/preflight.sh" | tee "$SUITE_DIR/preflight.log"

echo "WARNING: remove the propeller or disconnect thruster power."
echo "Press Enter to run clean, abrupt-exit, and recovery sequences."
read -r _

"$SCRIPT_DIR/run_repeated.sh" 30 clean "$SUITE_DIR"
"$SCRIPT_DIR/run_repeated.sh" 30 abrupt "$SUITE_DIR"
"$SCRIPT_DIR/run_repeated.sh" 10 clean "$SUITE_DIR"

echo "PASS validation suite completed"
