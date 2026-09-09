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
CALLBACKS="${VSTBOX_MATRIX_CALLBACKS:-10000}"
WARMUP="${VSTBOX_MATRIX_WARMUP:-1000}"
VOICES=(1 4 8 16 32)
BUFFERS=(256 128 64)

if [[ ! -x "$CLI" ]]; then
  echo "VSTBox Python CLI missing at $CLI. Run: python3 -m pip install -e '.[dev]'" >&2
  exit 1
fi
if [[ ! -x "$NATIVE_EXE" ]]; then
  echo "Native VST3 benchmark missing at $NATIVE_EXE. Run ./scripts/build_vst3_benchmark.sh" >&2
  exit 1
fi

mkdir -p benchmarks/results/mda_jx10_matrix
CYCLE=$((WARMUP + CALLBACKS + 2))
GATE=$((WARMUP + CALLBACKS + 1))

echo "== VSTBox controlled synth matrix =="
echo "Plugin:     $PLUGIN"
echo "Class:      mda JX10"
echo "Callbacks:  $CALLBACKS (+ $WARMUP warmup)"
echo "Voices:     ${VOICES[*]}"
echo "Buffers:    ${BUFFERS[*]}"
echo

for buffer in "${BUFFERS[@]}"; do
  for voices in "${VOICES[@]}"; do
    out="benchmarks/results/mda_jx10_matrix/mda_jx10_${voices}v_48k_${buffer}.json"
    printf 'Running %2d voices @ %3d samples ... ' "$voices" "$buffer"
    "$CLI" \
      --native-exe "$NATIVE_EXE" \
      --plugin "$PLUGIN" \
      --class "mda JX10" \
      --sample-rate 48000 \
      --buffer "$buffer" \
      --callbacks "$CALLBACKS" \
      --warmup "$WARMUP" \
      --voices "$voices" \
      --midi-note 48 \
      --midi-note-step 1 \
      --midi-velocity 0.8 \
      --midi-cycle "$CYCLE" \
      --midi-gate "$GATE" \
      --json-out "$out" \
      >/dev/null
    python3 - "$out" <<'PY'
import json, sys
p=sys.argv[1]
d=json.load(open(p))
t=d["timing"]
print(f'p99={t["p99_ms"]:.4f} ms  worst={t["worst_ms"]:.4f} ms  misses={t["deadline_misses"]}')
PY
  done
done

echo
echo "Matrix complete. Summary:"
python3 scripts/summarize_vst3_matrix.py benchmarks/results/mda_jx10_matrix
