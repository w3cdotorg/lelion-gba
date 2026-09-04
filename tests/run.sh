#!/usr/bin/env bash
# Build the libmgba harness (if needed), run every scenario in tests/scripts against the debug ROM
# (test hooks compiled in), then check the release ROM ignores the hooks.
set -euo pipefail
cd "$(dirname "$0")/.."
DEBUG_ROM=${1:-lelion-debug.gba}
RELEASE_ROM=${2:-lelion.gba}
CFLAGS_EXTRA=${HARNESS_CFLAGS:-"-I/opt/homebrew/include -L/opt/homebrew/lib"}
if [ ! -x tests/harness ] || [ tests/harness.c -nt tests/harness ]; then
  cc -O2 -o tests/harness tests/harness.c $CFLAGS_EXTRA -lmgba
fi
mkdir -p tests/out
echecs=0
for script in tests/scripts/*.txt; do
  echo "--- $(basename "$script") [$DEBUG_ROM]"
  if ! tests/harness "$DEBUG_ROM" "$script" tests/out; then echecs=$((echecs + 1)); fi
done
if [ -f "$RELEASE_ROM" ]; then
  echo "--- release_no_hooks.txt [$RELEASE_ROM]"
  if ! tests/harness "$RELEASE_ROM" tests/release/no_hooks.txt tests/out; then echecs=$((echecs + 1)); fi
fi
echo "== $echecs failing script(s) =="
[ "$echecs" -eq 0 ]
