#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PYTHON_CMD="${PYTHON_CMD:-python3}"

"$PYTHON_CMD" -m pytest -q
vstbox-machine --json-out benchmarks/profiles/current_machine.json
./scripts/build_native.sh
vstbox-bench-synthetic \
  --sample-rate 48000 \
  --buffer 128 \
  --voices 16 \
  --callbacks 20000 \
  --warmup 1000 \
  --json-out benchmarks/results/synthetic_16v_48k_128.json

echo
echo "v0.2 smoke test complete."
