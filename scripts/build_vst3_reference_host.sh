#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

SDK_DIR="${VST3_SDK_DIR:-external/vst3sdk}"
BUILD_DIR="${VST3_BUILD_DIR:-build/vst3sdk}"

if [[ ! -f "$SDK_DIR/CMakeLists.txt" ]]; then
  echo "VST3 SDK not found at $SDK_DIR" >&2
  echo "Run ./scripts/fetch_vst3_sdk.sh first." >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required. On macOS: brew install cmake ninja" >&2
  exit 1
fi

GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=(-G Ninja)
fi

cmake -S "$SDK_DIR" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=ON \
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF

cmake --build "$BUILD_DIR" --target audiohost validator --config Release

echo
echo "Reference VST3 host build complete. Candidate binaries:"
find "$BUILD_DIR" -type f \( -name audiohost -o -name validator \) -perm -111 -print 2>/dev/null || true
