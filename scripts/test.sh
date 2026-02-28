#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$SCRIPT_DIR/common.sh"

PRESET="$(pick_preset)"
BUILD_DIR="$(build_dir_for_preset "$PRESET")"

cmake --preset "$PRESET" -S "$ROOT_DIR"
cmake --build "$ROOT_DIR/$BUILD_DIR" --config Release
ctest --test-dir "$ROOT_DIR/$BUILD_DIR" --output-on-failure -C Release
