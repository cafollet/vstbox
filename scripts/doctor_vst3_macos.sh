#!/usr/bin/env bash
set -u
cd "$(dirname "$0")/.."

SDK_ROOT="${VST3_SDK_ROOT:-external/vst3sdk}"
BUILD_DIR="${BUILD_DIR:-build/native-vst3}"

echo "== VSTBox VST3 macOS doctor =="
echo "OS:          $(sw_vers -productVersion 2>/dev/null || uname -srm)"
echo "Arch:        $(uname -m)"
echo "CMake:       $(cmake --version 2>/dev/null | head -n1 || echo missing)"
echo "Ninja:       $(ninja --version 2>/dev/null || echo missing)"
echo "xcode-select: $(xcode-select -p 2>/dev/null || echo missing)"
echo "Xcode:       $(xcodebuild -version 2>/dev/null | paste -sd ' ' - || echo missing)"
echo "clang++:     $(xcrun --find clang++ 2>/dev/null || echo missing)"
echo "SDK path:    $SDK_ROOT"
if [[ -d "$SDK_ROOT/.git" ]]; then
  echo "SDK rev:     $(git -C "$SDK_ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
fi

echo
echo "CMake integration checks:"
if grep -q 'enable_language(OBJCXX)' native/CMakeLists.txt; then
  echo "  [OK] OBJCXX enabled at project scope"
else
  echo "  [!!] OBJCXX enablement missing"
fi
if grep -q 'module_mac.mm' native/CMakeLists.txt; then
  echo "  [OK] module_mac.mm included"
else
  echo "  [!!] module_mac.mm missing"
fi
if grep -q 'COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc' native/CMakeLists.txt; then
  echo "  [OK] ARC compile option configured"
else
  echo "  [!!] -fobjc-arc configuration missing"
fi
if grep -q '\-framework Foundation' native/CMakeLists.txt; then
  echo "  [OK] Foundation framework linked"
else
  echo "  [!!] Foundation framework missing"
fi

if [[ -f "$BUILD_DIR/build.ninja" ]]; then
  echo
echo "Generated build checks:"
  if grep -q -- '-fobjc-arc' "$BUILD_DIR/build.ninja"; then
    echo "  [OK] generated Ninja rules contain -fobjc-arc"
  else
    echo "  [!!] generated Ninja rules do not contain -fobjc-arc"
  fi
else
  echo
echo "No generated Ninja build found at $BUILD_DIR yet."
fi
