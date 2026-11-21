#!/bin/bash
# docs/v1.2.17/gate1217.sh : v1.2.17 host-sim pixel gates (HOST SIM ONLY, builds no firmware).
#  A) every finished frame of the pg_* scripts, band rows 24 and 12: HEAD vs cd5e118 (v1.2.16), vs 7fe1b4b (HACKbot
#     intro/pause lock), vs 66659af (pull-up fix head).
#  B) locked INFO-tower mock 726a5d3: pg_base / pg_gib_close / pg_dl_close (same set as docs/pullup-glitch/mockcmp.txt).
#  C) HACKbot intro/pause lock 7fe1b4b: docs/mocks/hackbot-intro-saves gate.txt / stills.txt / pause.txt.
#  D) devrec device-timeline model, 3 runs: HEAD vs cd5e118 (SIM_DEV_K=120.7).
# Refs get the HEAD dev_model.h and the 66659af devrec harness (git diff cd5e118 66659af -- sim_live.cpp) when missing.
# Sims run one at a time (recstart shares /tmp/.sim_rec_part).
set -e; W=$(git rev-parse --show-toplevel); T=${T:-/tmp/g1217}; rm -rf $T; mkdir -p $T
CMP=$W/docs/pullup-glitch/cmpg.py
mk() { local s=$1 d=$T/ref-$1; git -C $W worktree add -f --detach $d $s >/dev/null 2>&1
  if ! grep -q '"devrec"' $d/tools/livescan/sim_live.cpp; then cp $W/tools/livescan/dev_model.h $d/tools/livescan/
    (cd $d && git -C $W diff cd5e118 66659af -- tools/livescan/sim_live.cpp | git apply); fi; }
run() { local bin=$1 scr=$2 out=$3; mkdir -p $out; (cd $W && $bin $scr $out > $out.log 2>&1); }
git -C $W worktree prune
for s in cd5e118 7fe1b4b 66659af 726a5d3; do mk $s; done
PG="base gate gib_close dl_close screens control info extra"
for R in 24 12; do
  (cd $W && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_head$R tools/livescan/sim_live.cpp -lz)
  for s in cd5e118 7fe1b4b 66659af 726a5d3; do (cd $T/ref-$s && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=$R -o $T/bin_$s$R tools/livescan/sim_live.cpp -lz); done
  for b in head cd5e118 7fe1b4b 66659af; do for p in $PG; do run $T/bin_$b$R $W/tools/livescan/scripts/pg_$p.txt $T/A$R/$b/$p; done; done
  for b in head 726a5d3; do for p in base gib_close dl_close; do run $T/bin_$b$R $W/tools/livescan/scripts/pg_$p.txt $T/B$R/$b/$p; done; done
  for b in head 7fe1b4b; do for p in gate stills pause; do run $T/bin_$b$R $W/docs/mocks/hackbot-intro-saves/$p.txt $T/C$R/$b/$p; done; done
  for r in cd5e118 7fe1b4b 66659af; do echo "== A R=$R HEAD vs $r"; python3 $CMP $T/A$R/$r $T/A$R/head; done
  echo "== B R=$R HEAD vs locked INFO-tower mock 726a5d3"; python3 $CMP $T/B$R/726a5d3 $T/B$R/head
  echo "== C R=$R HEAD vs locked HACKbot intro/pause 7fe1b4b"; python3 $CMP $T/C$R/7fe1b4b $T/C$R/head
done
export SIM_DEV_K=${SIM_DEV_K:-120.7}; echo "== D devrec SIM_DEV_K=$SIM_DEV_K (ref = v1.2.16 cd5e118, new = v1.2.17 HEAD)"
for r in 1 2 3; do for b in cd5e118 head; do d=$T/dev/r$r; mkdir -p $d; run $T/bin_${b}24 $W/tools/livescan/scripts/pg_dev.txt $d/$([ $b = head ] && echo new || echo ref); done; done
python3 $W/docs/pullup-glitch/st3.py $T/dev ref new
for s in cd5e118 7fe1b4b 66659af 726a5d3; do git -C $W worktree remove --force $T/ref-$s; done
