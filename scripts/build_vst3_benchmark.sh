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

if [[ "$(uname -s)" == "Darwin" ]]; then
  if ! command -v xcrun >/dev/null 2>&1 || ! xcrun --find clang++ >/dev/null 2>&1; then
    echo "A working Xcode/AppleClang toolchain is required on macOS." >&2
    exit 1
  fi
  if ! command -v xcodebuild >/dev/null 2>&1; then
    echo "Full Xcode is required for the Steinberg macOS build." >&2
    exit 1
  fi
fi

SDK_ABS="$(cd "$SDK_ROOT" && pwd)"

echo "== VSTBox VST3 benchmark build =="
echo "SDK:       $SDK_ABS"
echo "Build dir: $BUILD_DIR"
echo "CMake:     $(cmake --version | head -n 1)"
if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "Xcode:     $(xcodebuild -version | paste -sd ' ' -)"
  echo "Clang++:   $(xcrun --find clang++)"
fi
if [[ -d "$SDK_ROOT/.git" ]]; then
  echo "SDK rev:   $(git -C "$SDK_ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
fi

cmake -S native -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVSTBOX_ENABLE_VST3=ON \
  -DVST3_SDK_ROOT="$SDK_ABS"

cmake --build "$BUILD_DIR" --config Release --target vstbox_vst3_bench

EXE="$BUILD_DIR/vstbox_vst3_bench"
if [[ ! -x "$EXE" ]]; then
  echo "Build completed but expected executable was not found: $EXE" >&2
  exit 1
fi

echo
echo "VST3 benchmark built:"
echo "  $EXE"
