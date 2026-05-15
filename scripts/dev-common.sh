#!/usr/bin/env bash
set -euo pipefail

# Shared implementation for dev-debug.sh / dev-release.sh.
# Usage: dev-common.sh <Debug|Release>

CONFIG="${1:-}"

if [[ "$CONFIG" != "Debug" && "$CONFIG" != "Release" ]]; then
  echo "dev-common.sh: first arg must be Debug or Release (got '$CONFIG')" >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$SCRIPT_DIR/common.sh"

command -v cmake >/dev/null
command -v ninja >/dev/null

EXTRA_CONFIGURE=()

if [[ -n "${HWB_PRESET:-}" ]]; then
  PRESET="$HWB_PRESET"
  BUILD_DIR="$ROOT_DIR/build/$PRESET"
else
  case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
      if [[ "$CONFIG" == "Debug" ]]; then PRESET="windows-msvc-debug"; else PRESET="windows-msvc-release"; fi
      BUILD_DIR="$ROOT_DIR/build/$PRESET"
      ;;
    Linux*)
      BASE_PRESET="linux-gcc-release"
      if [[ "$CONFIG" == "Debug" ]]; then
        PRESET="$BASE_PRESET"
        BUILD_DIR="$ROOT_DIR/build/${BASE_PRESET}-debug"
        EXTRA_CONFIGURE=(-B "$BUILD_DIR" "-DCMAKE_BUILD_TYPE=Debug")
      else
        PRESET="$BASE_PRESET"
        BUILD_DIR="$ROOT_DIR/build/$BASE_PRESET"
      fi
      ;;
    Darwin*)
      BASE_PRESET="macos-clang-release"
      if [[ "$CONFIG" == "Debug" ]]; then
        PRESET="$BASE_PRESET"
        BUILD_DIR="$ROOT_DIR/build/${BASE_PRESET}-debug"
        EXTRA_CONFIGURE=(-B "$BUILD_DIR" "-DCMAKE_BUILD_TYPE=Debug")
      else
        PRESET="$BASE_PRESET"
        BUILD_DIR="$ROOT_DIR/build/$BASE_PRESET"
      fi
      ;;
    *)
      echo "Unsupported platform: $(uname -s). Set HWB_PRESET manually." >&2
      exit 1
      ;;
  esac
fi

echo "==> Setup ($CONFIG) — preset=$PRESET build=$BUILD_DIR"
cmake --preset "$PRESET" -S "$ROOT_DIR" "${EXTRA_CONFIGURE[@]}"

echo "==> Build ($CONFIG)"
cmake --build "$BUILD_DIR" --config "$CONFIG"

echo "==> Test ($CONFIG)"
ctest --test-dir "$BUILD_DIR" --output-on-failure -C "$CONFIG"

EXE="$BUILD_DIR/hwbench-c"
if [[ -f "$BUILD_DIR/$CONFIG/hwbench-c.exe" ]]; then
  EXE="$BUILD_DIR/$CONFIG/hwbench-c.exe"
elif [[ -f "$BUILD_DIR/hwbench-c.exe" ]]; then
  EXE="$BUILD_DIR/hwbench-c.exe"
fi

CONFIG_LOWER="$(echo "$CONFIG" | tr '[:upper:]' '[:lower:]')"
TMP_OUT="$BUILD_DIR/hwbench-results.${CONFIG_LOWER}.tmp.json"
echo "==> Run ($CONFIG) — $EXE"
trap 'rm -f "$TMP_OUT"' EXIT
"$EXE" --suite quick --samples 5 --warmup-ms 100 --min-sample-ms 100 --out "$TMP_OUT"
