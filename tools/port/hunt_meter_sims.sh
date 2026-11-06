#!/bin/bash
# Build the sim_live binaries verify_hunt_meter.py needs: this tree and the 3b8b3de baseline (git archive), each with the
# flags it is checked under. usage: tools/port/hunt_meter_sims.sh OUTDIR. Compiler: $CXX, else g++, else zig c++ (pip ziglang).
set -e
cd "$(dirname "$0")/../.."; O=${1:?outdir}; mkdir -p $O
if [ -n "$CXX" ]; then C="$CXX"; elif command -v g++ >/dev/null; then C=g++; else C="${ZIG:-/tmp/rci/zig/ziglang/zig} c++ -target x86_64-linux-gnu.2.36 -I${ZINC:-/tmp/rci/zinc}"; fi
F="-O2 -std=gnu++17 -w"; L=$(ls /usr/lib/x86_64-linux-gnu/libz.so.1 2>/dev/null || echo -lz)
B=$O/tree3b8b3de; rm -rf $B; mkdir -p $B; git archive 3b8b3de src tools/livescan | tar -x -C $B
$C $F -DLV_RD_CG_LOG -o $O/new tools/livescan/sim_live.cpp $L &
$C $F -DLV_RD_CG_LOG -DLV_RD_HM_OCC=1 -o $O/new_occ tools/livescan/sim_live.cpp $L &
$C $F -DLV_RD_HUNT_METER=0 -o $O/new_hm0 tools/livescan/sim_live.cpp $L &
$C $F -DLV_RD_CHARGE=0 -o $O/new_cg0 tools/livescan/sim_live.cpp $L &
$C $F -o $O/new_nolog tools/livescan/sim_live.cpp $L &
( cd $B && $C $F -DLV_RD_CG_LOG -o $O/base tools/livescan/sim_live.cpp $L ) &
( cd $B && $C $F -DLV_RD_CHARGE=0 -o $O/base_cg0 tools/livescan/sim_live.cpp $L ) &
( cd $B && $C $F -o $O/base_nolog tools/livescan/sim_live.cpp $L ) &
wait; ls -la $O
