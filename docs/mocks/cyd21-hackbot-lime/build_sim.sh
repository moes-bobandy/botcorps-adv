# usage: build_sim.sh OUT_BIN   ->  host sim with the CYD-21 HACKbot lime plate. Run from the worktree: SIM_THEME=22 ./sim script outdir
# Compile with the v1.2.23 home flags (CYD-17 panels + CYD-19 LOGbot) plus LV_CYD21_HACK_LIME=1. Flag default in source is 0.
cd "$(dirname "$0")/../../.." && g++ -O2 -std=gnu++17 -w -DLV_CYD17_PANEL_TERM=1 -DLV_CYD19_LOGBOT=1 -DLV_CYD21_HACK_LIME=1 -o "$1" tools/livescan/sim_live.cpp -lz
