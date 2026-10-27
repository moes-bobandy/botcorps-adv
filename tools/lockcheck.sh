#!/bin/bash
# lockcheck: sim-side lock registry check (NOT tools/gate.py). Renders every locked/approved portrait
# screen listed in LOCKED/locks.tsv from a firmware build, at the registry's fixed sim state/time,
# and pixel-diffs it against the reference PNG. ANY non-zero diff = FAIL. Exit 0 only if all PASS.
#   usage: tools/lockcheck.sh [REV|WORKTREE_DIR]   (default: this worktree incl. uncommitted edits)
#   env:   LOCKED=<registry dir>  PY=<python with PIL+numpy>  LGFX=<LovyanGFX checkout>  WORK=<scratch>
set -u
REPO=$(cd "$(dirname "$0")/.." && pwd)
LOCKED=${LOCKED:-/workspace/out/cyd-full-board-build/LOCKED}
PY=${PY:-/tmp/cydpaint-venv/bin/python}
LGFX=${LGFX:-/workspace/cyd-sim/LovyanGFX}
SRC=${1:-$REPO}
WORK=${WORK:-/tmp/lockcheck-$(basename "$SRC")-$$}
rm -rf "$WORK"; mkdir -p "$WORK/fw"
if [ -d "$SRC" ]; then
  (cd "$SRC" && git ls-files -co --exclude-standard | grep -v '^checkpoints/' | tar -cf - -T -) | tar -xf - -C "$WORK/fw"
  TAG="$(git -C "$SRC" rev-parse --short=7 HEAD)$(git -C "$SRC" diff --quiet HEAD -- src || echo -dirty)"
else
  git -C "$REPO" archive "$SRC" | tar -xf - -C "$WORK/fw"; TAG=$(git -C "$REPO" rev-parse --short=7 "$SRC")
fi
for h in poke gp; do
  cp -r "$LOCKED/harness/$h" "$WORK/$h"; ln -s "$LGFX" "$WORK/$h/LovyanGFX"; ln -s "$WORK/fw" "$WORK/$h/src"
  cmake -S "$WORK/$h" -B "$WORK/$h/build" -DCMAKE_BUILD_TYPE=RelWithDebInfo >"$WORK/$h.cmake.log" 2>&1 &&
  timeout 1200 make -C "$WORK/$h/build" -j4 cydsim_$h >"$WORK/$h.build.log" 2>&1 || { echo "LOCKCHECK: $h sim build FAILED ($WORK/$h.build.log)"; exit 2; }
done
fail=0; pass=0
printf "LOCKCHECK %s  (registry %s)\n" "$TAG" "$LOCKED"
printf "%-28s %-6s %s\n" screen result diff
while IFS=$'\t' read -r id h script shot ref; do
  [ "$id" = id ] && continue
  out="$WORK/r/$h-${script%.script}"
  if [ ! -d "$out" ]; then mkdir -p "$out"; timeout 300 "$WORK/$h/build/cydsim_$h" --headless --script "$LOCKED/scripts/$script" --out "$out" >"$out.log" 2>&1; fi
  if [ -f "$out/$shot.png" ]; then r=$("$PY" "$LOCKED/pxdiff.py" "$LOCKED/$ref" "$out/$shot.png"); rc=$?; else r="no render"; rc=1; fi
  if [ $rc -eq 0 ]; then st=PASS; pass=$((pass+1)); else st=FAIL; fail=$((fail+1)); fi
  printf "%-28s %-6s %s\n" "$id" "$st" "$r"
done < "$LOCKED/locks.tsv"
echo "LOCKCHECK: $pass PASS, $fail FAIL  (renders in $WORK/r)"
[ $fail -eq 0 ]
