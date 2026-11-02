#!/bin/bash
# BOTcorps Adv gate (SPEC-LOCKED.md). Builds the host renderer (real UI + real rain),
# renders EVERY screen (rain on + rain off), then tools/gate/gate.py prints PASS/FAIL per
# spec item per screen. tools/gate/ram.py then checks static RAM against
# release/gate/ram-caps.txt (PASS at or under, FAIL above). Exit 0 only when every
# render item and every RAM row passes. RAM fails add to the render fail count.
#   usage: tools/gate.sh [render_dir]
set -e
cd "$(dirname "$0")/.."
OUT=${OUT:-/tmp/botcorps-gate}
DIR=${1:-$OUT/render}
export OUT
./tools/host/build.sh >"$OUT.build.log" 2>&1 || { echo "GATE: host build FAILED (see $OUT.build.log)"; exit 2; }
rm -rf "$DIR"; mkdir -p "$DIR"
"$OUT/rend" gate "$DIR" >/dev/null
set +e
python3 tools/gate/gate.py "$DIR" release/gate/baseline release/gate/frozen.txt
rc=$?
python3 tools/gate/ram.py
rr=$?
total=$((rc + rr))
if [ "$total" -gt 255 ]; then total=255; fi
exit $total
