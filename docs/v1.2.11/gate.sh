#!/bin/bash
# gate.sh NAME : build sim from the worktree, run gate.txt, compare every recorded frame with g_base (v1.2.10 e11c7f2)
set -e
N=${1:-new}; W=/workspace/cyd-dojo-fix-wt; O=/tmp/pf/g_$N; mkdir -p $O
cd $W && g++ -O2 -std=gnu++17 -w -DLV_HOST_BAND_ROWS=${ROWS:-12} -I/tmp/zinc -o /tmp/pf/sim_$N tools/livescan/sim_live.cpp /usr/lib/x86_64-linux-gnu/libz.so.1
/tmp/pf/sim_$N ${GATE:-/tmp/pf/gate.txt} $O > /tmp/pf/g_$N.log 2>&1
python3 - $O <<'PY'
import sys, numpy as np, os
o = sys.argv[1]; tot = 0; bad = 0
for f in sorted(os.listdir('/tmp/pf/g_base')):
    a = np.fromfile('/tmp/pf/g_base/' + f, np.uint8); b = np.fromfile(o + '/' + f, np.uint8) if os.path.exists(o + '/' + f) else None
    if b is None or a.size != b.size: print(f, 'MISSING/SIZE'); bad += 1; continue
    a = a.reshape(-1, 320, 240, 3); b = b.reshape(-1, 320, 240, 3); d = (a != b).any(3)
    n = int(d.sum()); tot += n; fr = int(d.any((1, 2)).sum())
    print(f'{f:16s} frames={a.shape[0]:4d} diff_px={n:8d} frames_diff={fr}')
print('TOTAL diff px', tot, 'missing', bad)
PY
