#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BINARY="$SCRIPT_DIR/build/pwm_cycle"
CYCLES=${1:-20}
MODE=${2:-clean}
LOG_DIR=${3:-"$SCRIPT_DIR/logs"}

if [ ! -x "$BINARY" ]; then
  echo "Missing $BINARY; run ./build_validation.sh first" >&2
  exit 2
fi
if [ "$MODE" != "clean" ] && [ "$MODE" != "abrupt" ]; then
  echo "Mode must be clean or abrupt" >&2
  exit 2
fi

mkdir -p "$LOG_DIR"
STAMP=$(date '+%Y%m%d_%H%M%S')
LOG="$LOG_DIR/repeated_${MODE}_${STAMP}.log"

echo "cycles=$CYCLES mode=$MODE log=$LOG" | tee "$LOG"
i=1
while [ "$i" -le "$CYCLES" ]; do
  echo "===== cycle $i/$CYCLES =====" | tee -a "$LOG"
  EXTRA=""
  if [ "$MODE" = "abrupt" ]; then EXTRA="--abrupt-exit"; fi
  if ! "$BINARY" --hold-ms=500 $EXTRA 2>&1 | tee -a "$LOG"; then
    echo "FAILED cycle=$i" | tee -a "$LOG"
    exit 1
  fi
  sleep 0.2
  i=$((i + 1))
done

echo "PASS completed=$CYCLES mode=$MODE" | tee -a "$LOG"
