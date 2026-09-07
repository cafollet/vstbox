#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build/native}"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=(-G Ninja)
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required. On macOS with Homebrew: brew install cmake ninja" >&2
  exit 1
fi

cmake -S native -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

echo
echo "Native benchmark built:"
echo "  $BUILD_DIR/vstbox_synthetic_bench"
