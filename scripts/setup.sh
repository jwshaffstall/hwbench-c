#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$SCRIPT_DIR/common.sh"

command -v cmake >/dev/null
command -v ninja >/dev/null

PRESET="$(pick_preset)"
echo "Configuring preset: $PRESET"
cmake --preset "$PRESET" -S "$ROOT_DIR"
