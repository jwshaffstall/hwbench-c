#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$SCRIPT_DIR/common.sh"

DURATION="${1:-30}"
OUT_PATH="${2:-$ROOT_DIR/hwbench-stress.json}"

if [[ "$DURATION" != "10" && "$DURATION" != "30" && "$DURATION" != "60" ]]; then
  echo "Usage: $0 [10|30|60] [out_path]" >&2
  exit 1
fi

PRESET="$(pick_preset)"
BUILD_DIR="$(build_dir_for_preset "$PRESET")"

cmake --preset "$PRESET" -S "$ROOT_DIR"
cmake --build "$ROOT_DIR/$BUILD_DIR" --config Release

EXE="$ROOT_DIR/$BUILD_DIR/hwbench-c"
if [[ -f "$ROOT_DIR/$BUILD_DIR/Release/hwbench-c.exe" ]]; then
  EXE="$ROOT_DIR/$BUILD_DIR/Release/hwbench-c.exe"
elif [[ -f "$ROOT_DIR/$BUILD_DIR/hwbench-c.exe" ]]; then
  EXE="$ROOT_DIR/$BUILD_DIR/hwbench-c.exe"
fi

CMD=("$EXE" --stress "$DURATION" --out "$OUT_PATH")
if [[ -n "${HWB_STRESS_THREADS:-}" ]]; then
  CMD+=("--threads" "$HWB_STRESS_THREADS")
fi

echo "Running stress mode for ${DURATION}s via $EXE"
echo "${CMD[@]}"
"${CMD[@]}"
