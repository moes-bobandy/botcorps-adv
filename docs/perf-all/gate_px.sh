#!/bin/bash
# docs/perf-all/gate_px.sh [REF] : pixel gate, HEAD vs REF (default cda0390 = v1.2.17), every finished frame of every
# tools/livescan/scripts/{pg_,dj_,pa_}*.txt script that has no devrec, at band rows 24 and 12. HOST SIM ONLY.
set -e; REF=${1:-cda0390}; W=$(git rev-parse --show-toplevel); T=${T:-/tmp/gate-px}; rm -rf $T; mkdir -p $T
git -C $W worktree prune; git -C $W worktree add -f --detach $T/ref $REF >/dev/null 2>&1
S=$(cd $W/tools/livescan/scripts && grep -L devrec pg_*.txt dj_*.txt pa_*.txt 2>/dev/null | sed 's/\.txt$//')
for R in 24 12; do
  (cd $W && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_new$R tools/livescan/sim_live.cpp -lz)
  (cd $T/ref && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_ref$R tools/livescan/sim_live.cpp -lz)
  for s in $S; do for b in new ref; do mkdir -p $T/$b$R/$s; (cd $W && $T/bin_$b$R $W/tools/livescan/scripts/$s.txt $T/$b$R/$s > $T/$b$R/$s.log 2>&1); done; done
  echo "== band rows $R: HEAD vs $REF"; python3 $W/docs/pullup-glitch/cmpg.py $T/ref$R $T/new$R | grep -E "diff_px=[1-9]|MISSING|TOTAL"
done
git -C $W worktree remove --force $T/ref
