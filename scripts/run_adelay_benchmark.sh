#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGIN="${1:-}"
if [[ -z "$PLUGIN" ]]; then
  PLUGIN="$(find build/vst3sdk-plugin-smoke "$HOME/Library/Audio/Plug-Ins/VST3" -type d -name 'adelay.vst3' -print -quit 2>/dev/null || true)"
fi
if [[ -z "$PLUGIN" ]]; then
  echo "adelay.vst3 not found. Pass its path as the first argument." >&2
  exit 1
fi

if [[ -n "${VSTBOX_VST3_CLI:-}" ]]; then
  CLI="$VSTBOX_VST3_CLI"
elif [[ -x .venv/bin/vstbox-bench-vst3 ]]; then
  CLI=.venv/bin/vstbox-bench-vst3
elif command -v vstbox-bench-vst3 >/dev/null 2>&1; then
  CLI="$(command -v vstbox-bench-vst3)"
else
  echo "vstbox-bench-vst3 was not found. Activate .venv and run: python3 -m pip install -e '.[dev]'" >&2
  exit 1
fi

NATIVE_EXE="${VSTBOX_VST3_NATIVE_EXE:-build/native-vst3/vstbox_vst3_bench}"
if [[ ! -x "$NATIVE_EXE" ]]; then
  echo "Native VST3 benchmark is missing: $NATIVE_EXE" >&2
  echo "Run ./scripts/build_vst3_benchmark.sh first." >&2
  exit 1
fi

mkdir -p benchmarks/results
"$CLI" \
  --native-exe "$NATIVE_EXE" \
  --plugin "$PLUGIN" \
  --sample-rate 48000 \
  --buffer 128 \
  --callbacks 20000 \
  --warmup 1000 \
  --no-midi \
  --json-out benchmarks/results/adelay_48k_128.json
