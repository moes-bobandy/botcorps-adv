#!/bin/bash
# docs/perf-all/ir_dojo.sh [REF] : deterministic CPU work of the Dojo scene (valgrind callgrind instruction count inside
# lv_scene_radar*, the dj_gate.txt walk with rec -> tick), HEAD vs REF (default cda0390). Same input = same count every run. HOST SIM ONLY.
set -e; REF=${1:-cda0390}; W=$(git rev-parse --show-toplevel); T=${T:-/tmp/ir-dojo}; rm -rf $T; mkdir -p $T
git -C $W worktree prune; git -C $W worktree add -f --detach $T/ref $REF >/dev/null 2>&1
(cd $W && g++ -O2 -g -std=gnu++17 -w -o $T/bin_new tools/livescan/sim_live.cpp -lz); (cd $T/ref && g++ -O2 -g -std=gnu++17 -w -o $T/bin_ref tools/livescan/sim_live.cpp -lz)
sed 's/^rec \([0-9]*\) .*/tick \1/' $W/tools/livescan/scripts/dj_gate.txt > $T/walk.txt
for b in ref new; do (cd $T/ref && valgrind --tool=callgrind --toggle-collect='lv_scene_radar*' --callgrind-out-file=$T/$b.out $T/bin_$b $T/walk.txt $T >/dev/null 2>&1); echo "$b $(callgrind_annotate $T/$b.out 2>/dev/null | grep 'PROGRAM TOTALS')"; done
git -C $W worktree remove --force $T/ref
