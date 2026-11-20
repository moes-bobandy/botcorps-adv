#!/bin/bash
# qmeasure.sh NAME : build env cyd_qemu from the worktree, run the QEMU tour (no sampling), print [fps] lines
set -e; N=$1; cd /workspace/cyd-dojo-fix-wt; export PATH=$HOME/.platformio/penv/bin:$PATH
pio run -e cyd_qemu > /tmp/pf/pio_$N.log 2>&1 || { tail -30 /tmp/pf/pio_$N.log; exit 1; }
/tmp/pf/mkflash.sh .pio/build/cyd_qemu /tmp/pf/q_$N.bin > /dev/null 2>&1
cd /tmp/pf && timeout 170 python3 qrun.py q_$N.bin q_$N 140 || true
grep -a "fps\]\|tour\]\|GFX-RAM\|Guru\|abort" /tmp/pf/q_$N.log | head -70 > /tmp/pf/q_$N.fps; cat /tmp/pf/q_$N.fps
