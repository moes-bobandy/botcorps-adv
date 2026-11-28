# usage: build_sim.sh OUT_BIN [extra -D flags]. ON build: bash build_sim.sh /tmp/sim_ls -DLV_CYD27_LOGSUB=1. Run sims from the repo root: OUT_BIN script outdir
# Flags = the v1.2.25 cyd_livescan set (platformio.ini). Frames: script shots_video.txt (tick 1 + dump per 30 ms) -> rgb -> ffmpeg 100/3 fps, scale=720:960:flags=neighbor.
g++ -O2 -std=gnu++17 -w -DLV_CYD17_PANEL_TERM=1 -DLV_CYD19_LOGBOT=1 -DLV_TPS_G2_ONLY=1 -DLV_CYD22_G2=1 -DLV_C22_VAR=6 -DLV_RD_CHARGE=1 -DLV_RD_HUNT_METER=1 -DLV_RD_INFO_TOWER_PULL=1 -DLV_C17_CTL=0 $2 -o $1 tools/livescan/sim_live.cpp -lz
