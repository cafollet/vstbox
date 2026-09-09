#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

SDK_ROOT="${VST3_SDK_ROOT:-external/vst3sdk}"
BUILD_DIR="${MDA_BUILD_DIR:-build/vst3sdk-mda}"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=(-G Ninja)
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake is required." >&2
  exit 1
fi
if [[ ! -f "$SDK_ROOT/CMakeLists.txt" ]]; then
  echo "VST3 SDK not found at $SDK_ROOT. Run ./scripts/fetch_vst3_sdk.sh first." >&2
  exit 1
fi

# MDA is a useful controlled synth target because the bundle contains several
# instruments (DX10, EPiano, JX10, Piano) and does not require VSTGUI.
cmake -S "$SDK_ROOT" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=ON \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=ON \
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF

cmake --build "$BUILD_DIR" --config Release --target mda-vst3

PLUGIN="$(find "$BUILD_DIR" "$HOME/Library/Audio/Plug-Ins/VST3" -type d -name 'mda-vst3.vst3' -print -quit 2>/dev/null || true)"
if [[ -z "$PLUGIN" ]]; then
  echo "mda-vst3 built, but the .vst3 bundle could not be located automatically." >&2
  exit 1
fi

echo
echo "MDA VST3 bundle ready:"
echo "  $PLUGIN"
echo "Controlled instrument class: mda JX10"
