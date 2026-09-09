#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

SDK_ROOT="${VST3_SDK_ROOT:-external/vst3sdk}"
BUILD_DIR="${BUILD_DIR:-build/native-vst3}"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=(-G Ninja)
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required. On macOS with Homebrew: brew install cmake ninja" >&2
  exit 1
fi
if [[ ! -f "$SDK_ROOT/CMakeLists.txt" ]]; then
  echo "VST3 SDK not found at $SDK_ROOT" >&2
  echo "Run ./scripts/fetch_vst3_sdk.sh first." >&2
  exit 1
fi

cmake -S native -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVSTBOX_ENABLE_VST3=ON \
  -DVST3_SDK_ROOT="$(cd "$SDK_ROOT" && pwd)"

cmake --build "$BUILD_DIR" --config Release --target vstbox_vst3_bench

echo
echo "VST3 benchmark built:"
echo "  $BUILD_DIR/vstbox_vst3_bench"
