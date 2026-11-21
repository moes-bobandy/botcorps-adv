#!/bin/bash
# docs/perf-all/dev_time.sh [REF] [RUNS] : devrec device-timeline timing (tools/livescan/dev_model.h, SIM_DEV_K=120.7), HEAD vs REF,
# every tools/livescan/scripts/*_dev.txt script, RUNS runs each (default 3). Prints one row per devrec clip. HOST SIM ONLY.
set -e; REF=${1:-cda0390}; N=${2:-3}; W=$(git rev-parse --show-toplevel); T=${T:-/tmp/dev-time}; rm -rf $T; mkdir -p $T
git -C $W worktree prune; git -C $W worktree add -f --detach $T/ref $REF >/dev/null 2>&1
(cd $W && g++ -O2 -std=gnu++17 -w -o $T/bin_new tools/livescan/sim_live.cpp -lz); (cd $T/ref && g++ -O2 -std=gnu++17 -w -o $T/bin_ref tools/livescan/sim_live.cpp -lz)
export SIM_DEV_K=${SIM_DEV_K:-120.7}
for s in $(cd $W/tools/livescan/scripts && ls *_dev.txt | sed 's/\.txt$//'); do for r in $(seq 1 $N); do for b in ref new; do
  d=$T/$s/r$r/$b; mkdir -p $d; (cd $W && $T/bin_$b $W/tools/livescan/scripts/$s.txt $d > $d.log 2>&1); done; done; done
python3 $W/docs/perf-all/dev_table.py $T
git -C $W worktree remove --force $T/ref
