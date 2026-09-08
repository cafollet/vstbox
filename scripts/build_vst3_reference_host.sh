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

# Steinberg VST3 SDK 3.8.1's macOS audiohost sample uses Foundation classes
# but its sample CMake target may only link CoreFoundation. Apply a narrow,
# idempotent local patch to the downloaded SDK; external/ remains untracked.
if [[ "$(uname -s)" == "Darwin" ]]; then
  AUDIOHOST_CMAKE="$SDK_DIR/public.sdk/samples/vst-hosting/audiohost/CMakeLists.txt"
  if [[ -f "$AUDIOHOST_CMAKE" ]] && ! grep -q '\-framework Foundation' "$AUDIOHOST_CMAKE"; then
    python3 - "$AUDIOHOST_CMAKE" <<'PY2'
from pathlib import Path
import sys
p = Path(sys.argv[1])
text = p.read_text()
old = 'set(audiohost_PLATFORM_LIBS "-framework CoreFoundation")'
new = 'set(audiohost_PLATFORM_LIBS "-framework CoreFoundation" "-framework Foundation")'
if old in text:
    p.write_text(text.replace(old, new))
    print("Applied local Foundation.framework patch to Steinberg audiohost.")
PY2
  fi
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
