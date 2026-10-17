#!/bin/bash
# BOTcorps Adv render gate (SPEC-LOCKED.md). Builds the host renderer (real UI + real rain),
# renders EVERY screen (rain on + rain off), then tools/gate/gate.py prints PASS/FAIL per
# spec item per screen. Exit 0 only when every item passes.
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
exit $rc
