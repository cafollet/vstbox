#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

SDK_DIR="${VST3_SDK_DIR:-external/vst3sdk}"
SDK_URL="https://github.com/steinbergmedia/vst3sdk.git"

if ! command -v git >/dev/null 2>&1; then
  echo "git is required" >&2
  exit 1
fi

if [[ -d "$SDK_DIR/.git" ]]; then
  echo "Updating existing VST3 SDK checkout at $SDK_DIR"
  git -C "$SDK_DIR" pull --ff-only
  git -C "$SDK_DIR" submodule update --init --recursive
else
  echo "Cloning Steinberg VST3 SDK into $SDK_DIR"
  git clone --recursive "$SDK_URL" "$SDK_DIR"
fi

echo
git -C "$SDK_DIR" describe --tags --always --dirty 2>/dev/null || true
