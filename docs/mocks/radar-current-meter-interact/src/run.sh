#!/bin/bash
# MOCK ONLY. Build the host sim from THIS tree (src/live/lv_radar.h, LV_RD_CHARGE=1 = LOCKED D4 port 3b8b3de), record 6 s of
# the hunt screen, then draw the locked A meter + the current/meter interaction on top. No firmware build, no flash.
# Box has no g++: zig c++ (pip ziglang 0.13.0) + zlib 1.3.1 headers, linked against the system libz.so.1.
set -e
REPO=${REPO:-/workspace/cyd-portrait-wt}; HERE=$(cd "$(dirname "$0")" && pwd); W=${W:-/tmp/rci}
mkdir -p $W/o2
cd $REPO && $W/zig/ziglang/zig c++ -target x86_64-linux-gnu.2.36 -O2 -std=gnu++17 -w -DLV_RD_CG_LOG -I$W/zinc -o $W/sim tools/livescan/sim_live.cpp /usr/lib/x86_64-linux-gnu/libz.so.1
$W/sim $HERE/hunt_current_meter.txt $W/o2 > $W/o2/out.txt 2> $W/o2/err.txt
python3 $HERE/render_interact.py "${OUT:-$(dirname $HERE)}" $W/o2
