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

mkdir -p benchmarks/results
vstbox-bench-vst3 \
  --plugin "$PLUGIN" \
  --sample-rate 48000 \
  --buffer 128 \
  --callbacks 20000 \
  --warmup 1000 \
  --no-midi \
  --json-out benchmarks/results/adelay_48k_128.json
