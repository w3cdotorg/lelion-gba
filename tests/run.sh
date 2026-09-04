#!/usr/bin/env bash
# Build the libmgba harness (if needed) and run every script in tests/scripts against lelion.gba.
set -euo pipefail
cd "$(dirname "$0")/.."
ROM=${1:-lelion.gba}
CFLAGS_EXTRA=${HARNESS_CFLAGS:-"-I/opt/homebrew/include -L/opt/homebrew/lib"}
if [ ! -x tests/harness ] || [ tests/harness.c -nt tests/harness ]; then
  cc -O2 -o tests/harness tests/harness.c $CFLAGS_EXTRA -lmgba
fi
mkdir -p tests/out
echecs=0
for script in tests/scripts/*.txt; do
  echo "--- $(basename "$script")"
  if ! tests/harness "$ROM" "$script" tests/out; then echecs=$((echecs + 1)); fi
done
echo "== $echecs failing script(s) =="
[ "$echecs" -eq 0 ]
