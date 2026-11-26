cd /workspace/v1222-build-wt && g++ -O2 -std=gnu++17 -w -DLV_CYD17_PANEL_TERM=1 $2 -o $1 tools/livescan/sim_live.cpp -lz
