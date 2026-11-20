#!/bin/bash
# docs/pullup-glitch/gate.sh REF_SHA : pixel gate (every finished frame, band rows 24 and 12) of HEAD vs REF_SHA, plus the
# device-timeline model (tools/livescan/dev_model.h, devrec) for HEAD and REF. Host sim only: builds no firmware.
# Run the sims one at a time: sim_live's recstart uses the shared temp file /tmp/.sim_rec_part.
set -e; REF=${1:-cd5e118}; W=$(git rev-parse --show-toplevel); T=/tmp/pg-gate; rm -rf $T; mkdir -p $T
git -C $W worktree prune; git -C $W worktree add -f --detach $T/ref $REF >/dev/null 2>&1
cp $W/tools/livescan/dev_model.h $T/ref/tools/livescan/; (cd $T/ref && git -C $W diff $REF HEAD -- tools/livescan/sim_live.cpp | git apply)   # same sim harness on the ref
for R in 24 12; do
  (cd $W && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_new$R tools/livescan/sim_live.cpp -lz)
  (cd $T/ref && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_ref$R tools/livescan/sim_live.cpp -lz)
  for s in base gate gib_close dl_close screens control info extra; do for b in new ref; do
    mkdir -p $T/$b$R/$s; (cd $W && $T/bin_$b$R $W/tools/livescan/scripts/pg_$s.txt $T/$b$R/$s > $T/$b$R/$s.log 2>&1); done; done
  python3 $W/docs/pullup-glitch/cmpg.py $T/ref$R $T/new$R | tail -1
done
export SIM_DEV_K=${SIM_DEV_K:-120.7}
for r in 1 2 3; do for b in ref new; do d=$T/dev/r$r/$b; mkdir -p $d; (cd $W && $T/bin_${b}24 $W/tools/livescan/scripts/pg_dev.txt $d > $d.log 2>&1); done; done
python3 $W/docs/pullup-glitch/st3.py $T/dev ref new
git -C $W worktree remove --force $T/ref
