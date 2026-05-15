#!/bin/bash
cd "$(dirname "$0")/.."
echo "Formatting C/C++ files..."
find src include apps tests -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
echo "Done."
