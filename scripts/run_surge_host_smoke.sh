#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGIN="${1:-}"
if [[ -z "$PLUGIN" ]]; then
  PLUGIN="$(find external/surge/build -type d -name 'Surge XT.vst3' -print -quit 2>/dev/null || true)"
fi
if [[ -z "$PLUGIN" ]]; then
  echo "Surge XT.vst3 not found under external/surge/build." >&2
  exit 1
fi

CLI="${VSTBOX_VST3_CLI:-.venv/bin/vstbox-bench-vst3}"
NATIVE_EXE="${VSTBOX_VST3_NATIVE_EXE:-build/native-vst3/vstbox_vst3_bench}"
OUT="benchmarks/results/surge_host_smoke_48k_128.json"

if [[ ! -x "$CLI" ]]; then
  echo "VSTBox Python CLI missing at $CLI. Run: python3 -m pip install -e '.[dev]'" >&2
  exit 1
fi
if [[ ! -x "$NATIVE_EXE" ]]; then
  echo "Native VST3 benchmark missing at $NATIVE_EXE. Run ./scripts/build_vst3_benchmark.sh" >&2
  exit 1
fi

mkdir -p benchmarks/results

"$CLI" \
  --native-exe "$NATIVE_EXE" \
  --plugin "$PLUGIN" \
  --class "Surge XT" \
  --sample-rate 48000 \
  --buffer 128 \
  --callbacks 500 \
  --warmup 100 \
  --voices 4 \
  --midi-note 48 \
  --midi-note-step 3 \
  --midi-velocity 0.8 \
  --midi-cycle 602 \
  --midi-gate 601 \
  --json-out "$OUT" \
  >/dev/null

python3 - "$OUT" <<'PY'
import json, sys
p=sys.argv[1]
d=json.load(open(p))
plugin=d["plugin"]
t=d["timing"]
o=d.get("output", {})
count=len(plugin.get("parameters", []))
print("== Surge XT host-completeness smoke ==")
print(f'parameters:           {count}')
print(f'controller present:   {plugin.get("controller_present")}')
print(f'controller connected: {plugin.get("controller_connected")}')
print(f'component state sync: {plugin.get("component_state_synced")}')
print(f'process context:      {plugin.get("process_context_provided")}')
print(f'output RMS:           {o.get("rms", 0.0):.8f}')
print(f'output peak:          {o.get("peak", 0.0):.8f}')
print(f'p99:                  {t["p99_ms"]:.6f} ms')
print(f'worst:                {t["worst_ms"]:.6f} ms')
print(f'deadline misses:      {t["deadline_misses"]}')

fail=[]
if count < 1000: fail.append(f"expected a large Surge parameter set, got {count}")
if not plugin.get("controller_present"): fail.append("controller not present")
if not plugin.get("controller_connected"): fail.append("controller not connected")
if not plugin.get("component_state_synced"): fail.append("component state not synchronized to controller")
if not plugin.get("process_context_provided"): fail.append("process context not provided")
if o.get("peak", 0.0) <= 1e-8: fail.append("main output is silent")
if o.get("rms", 0.0) <= 1e-10: fail.append("main output RMS is effectively zero")
if t.get("deadline_misses", 1) != 0: fail.append("deadline miss observed in smoke test")
if fail:
    print("\nFAIL:")
    for item in fail: print(f"- {item}")
    raise SystemExit(2)
print("\nPASS: controller, context, output, and timing checks all passed.")
PY

echo "Result: $OUT"
