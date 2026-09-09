#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGIN="${1:-}"
if [[ -z "$PLUGIN" ]]; then
  PLUGIN="$(find build/vst3sdk-mda "$HOME/Library/Audio/Plug-Ins/VST3" -type d -name 'mda-vst3.vst3' -print -quit 2>/dev/null || true)"
fi
if [[ -z "$PLUGIN" ]]; then
  echo "mda-vst3.vst3 not found. Run ./scripts/build_mda_synth_sample.sh first." >&2
  exit 1
fi

CLI="${VSTBOX_VST3_CLI:-.venv/bin/vstbox-bench-vst3}"
NATIVE_EXE="${VSTBOX_VST3_NATIVE_EXE:-build/native-vst3/vstbox_vst3_bench}"
if [[ ! -x "$CLI" ]]; then
  echo "VSTBox Python CLI missing at $CLI. Run: python3 -m pip install -e '.[dev]'" >&2
  exit 1
fi
if [[ ! -x "$NATIVE_EXE" ]]; then
  echo "Native VST3 benchmark missing at $NATIVE_EXE. Run ./scripts/build_vst3_benchmark.sh" >&2
  exit 1
fi

CALLBACKS=200
WARMUP=50
CYCLE=$((WARMUP + CALLBACKS + 2))
GATE=$((WARMUP + CALLBACKS + 1))
mkdir -p benchmarks/results

"$CLI" \
  --native-exe "$NATIVE_EXE" \
  --plugin "$PLUGIN" \
  --class "mda JX10" \
  --sample-rate 48000 \
  --buffer 128 \
  --callbacks "$CALLBACKS" \
  --warmup "$WARMUP" \
  --voices 4 \
  --midi-note 48 \
  --midi-note-step 1 \
  --midi-velocity 0.8 \
  --midi-cycle "$CYCLE" \
  --midi-gate "$GATE" \
  --json-out benchmarks/results/mda_jx10_smoke_4v_48k_128.json
