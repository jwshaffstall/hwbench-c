#!/usr/bin/env bash
set -euo pipefail

pick_preset() {
  if [[ -n "${HWB_PRESET:-}" ]]; then
    echo "$HWB_PRESET"
    return
  fi

  case "$(uname -s)" in
    Linux*) echo "linux-gcc-release" ;;
    Darwin*) echo "macos-clang-release" ;;
    MINGW*|MSYS*|CYGWIN*) echo "windows-msvc-release" ;;
    *)
      echo "Unsupported platform: $(uname -s). Set HWB_PRESET manually." >&2
      return 1
      ;;
  esac
}

build_dir_for_preset() {
  local preset="$1"
  echo "build/${preset}"
}
