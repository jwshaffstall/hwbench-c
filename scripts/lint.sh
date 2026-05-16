#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
echo "Linting C files (headers are checked when included)..."
if [ "${1:-}" != "" ]; then
    find src apps tests -type f -name '*.c' -exec clang-tidy -p "$1" {} +
else
    find src apps tests -type f -name '*.c' -exec clang-tidy {} -- -Iinclude \;
fi
echo "Done."
