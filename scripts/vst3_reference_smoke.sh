#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 /path/to/Plugin.vst3" >&2
  exit 2
fi

PLUGIN="$1"
BUILD_DIR="${VST3_BUILD_DIR:-build/vst3sdk}"

if [[ ! -e "$PLUGIN" ]]; then
  echo "Plugin not found: $PLUGIN" >&2
  exit 1
fi

AUDIOHOST="$(find "$BUILD_DIR" -type f -name audiohost -perm -111 -print 2>/dev/null | head -n 1 || true)"
if [[ -z "$AUDIOHOST" ]]; then
  echo "audiohost was not found under $BUILD_DIR" >&2
  echo "Run ./scripts/build_vst3_reference_host.sh first." >&2
  exit 1
fi

echo "Reference host: $AUDIOHOST"
echo "Plugin:         $PLUGIN"
echo
exec "$AUDIOHOST" "$PLUGIN"
