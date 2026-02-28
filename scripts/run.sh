#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$SCRIPT_DIR/common.sh"

PRESET="$(pick_preset)"
BUILD_DIR="$(build_dir_for_preset "$PRESET")"
OUT_PATH="${1:-$ROOT_DIR/hwbench-results.json}"

cmake --preset "$PRESET" -S "$ROOT_DIR"
cmake --build "$ROOT_DIR/$BUILD_DIR" --config Release

EXE="$ROOT_DIR/$BUILD_DIR/hwbench-c"
if [[ -f "$ROOT_DIR/$BUILD_DIR/Release/hwbench-c.exe" ]]; then
  EXE="$ROOT_DIR/$BUILD_DIR/Release/hwbench-c.exe"
elif [[ -f "$ROOT_DIR/$BUILD_DIR/hwbench-c.exe" ]]; then
  EXE="$ROOT_DIR/$BUILD_DIR/hwbench-c.exe"
fi

"$EXE" --suite quick --samples 5 --warmup-ms 100 --min-sample-ms 100 --out "$OUT_PATH"
