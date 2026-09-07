#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

echo "== VSTBox macOS bootstrap =="

if ! command -v git >/dev/null 2>&1; then
  echo "Missing required command: git" >&2
  echo "Install Apple's Command Line Tools with: xcode-select --install" >&2
  exit 1
fi

# Respect an explicitly supplied interpreter first. Otherwise choose the newest
# supported Python we can find before falling back to python3.
if [[ -n "${PYTHON_BIN:-}" ]]; then
  CANDIDATE="$PYTHON_BIN"
else
  CANDIDATE=""
  for py in python3.13 python3.12 python3.11 python3.10 python3; do
    if command -v "$py" >/dev/null 2>&1; then
      if "$py" - <<'PY' >/dev/null 2>&1
import sys
raise SystemExit(0 if sys.version_info >= (3, 10) else 1)
PY
      then
        CANDIDATE="$(command -v "$py")"
        break
      fi
    fi
  done
fi

if [[ -z "$CANDIDATE" ]] || [[ ! -x "$CANDIDATE" ]]; then
  echo "Python 3.10+ is required, but no suitable interpreter was found." >&2
  echo "If Python is installed, rerun with e.g.:" >&2
  echo "  PYTHON_BIN=/usr/local/bin/python3.13 ./scripts/bootstrap_macos.sh" >&2
  exit 1
fi

PYTHON_BIN="$CANDIDATE"

"$PYTHON_BIN" - <<'PY'
import sys
if sys.version_info < (3, 10):
    raise SystemExit(f"Python 3.10+ is required; found {sys.version.split()[0]}")
print(f"Python: {sys.version.split()[0]}")
print(f"Interpreter: {sys.executable}")
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
