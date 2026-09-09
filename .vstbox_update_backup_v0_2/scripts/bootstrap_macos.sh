#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

echo "== VSTBox macOS bootstrap =="

for cmd in git python3; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing required command: $cmd" >&2
    if [[ "$cmd" == "git" ]]; then
      echo "Install Apple's Command Line Tools with: xcode-select --install" >&2
    fi
    exit 1
  fi
done

PYTHON_BIN="${PYTHON_BIN:-python3}"

"$PYTHON_BIN" - <<'PY'
import sys
if sys.version_info < (3, 10):
    raise SystemExit(f"Python 3.10+ is required; found {sys.version.split()[0]}")
print(f"Python: {sys.version.split()[0]}")
PY

if [[ ! -d .venv ]]; then
  echo "Creating .venv..."
  "$PYTHON_BIN" -m venv .venv
fi

source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e '.[dev]'

echo "Running test suite..."
python -m pytest -q

echo
echo "Setup complete. Activate the environment later with:"
echo "  source .venv/bin/activate"
echo
echo "Try the simulator with:"
echo "  vstbox-sim configs/native_arm_baseline.json"
