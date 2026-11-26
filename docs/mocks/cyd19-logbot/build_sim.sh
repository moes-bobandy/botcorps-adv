# usage: build_sim.sh OUT_BIN   ->  host sim with the LOGbot page. Run sims from the repo root: LB_FINISH=0|1|2 SIM_THEME=22 OUT_BIN script outdir
# LB_FINISH: 0 chrome blue, 1 metallic copper, 2 oil-slick (host sim only). Tap the face/panel (tap 120 255) flips SCAN LOG <-> SYSTEM LOG.
cd /workspace/v1222-build-wt && g++ -O2 -std=gnu++17 -w -DLV_CYD19_LOGBOT=1 -o $1 tools/livescan/sim_live.cpp -lz
