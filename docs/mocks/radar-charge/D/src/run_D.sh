#!/bin/bash
# D stills: scratch copy of locked lv_radar.h @3240b03 + apply_charge.py (A/B/C) + apply_charge_D.py; sim (D) and NODRAW baseline.
set -e
cd /workspace && git -C cyd-portrait-wt show 3240b03:src/live/lv_radar.h > radar-charge-D-scratch/src/live/lv_radar.h
python3 out/radar-charge/src/apply_charge.py radar-charge-D-scratch/src/live/lv_radar.h && python3 out/radar-charge/D/src/apply_charge_D.py radar-charge-D-scratch/src/live/lv_radar.h
cd radar-charge-D-scratch
g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=4 -DLV_RD_CG_LOG -o /tmp/rd_sim4 tools/livescan/sim_live.cpp -lz &
g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=4 -DLV_RD_CG_LOG -DLV_RD_CG_NODRAW=1 -o /tmp/rd_base4 tools/livescan/sim_live.cpp -lz & wait
mkdir -p /tmp/rd; for b in sim base; do rm -rf /tmp/rd/${b}4; mkdir /tmp/rd/${b}4; /tmp/rd_${b}4 /workspace/out/radar-charge/D/src/scripts/charge_D.txt /tmp/rd/${b}4 > /tmp/rd/${b}4/out.txt 2> /tmp/rd/${b}4/err.txt; done
mkdir -p /workspace/out/radar-charge/D/stills
python3 - <<'P'
from PIL import Image
for k, nm in (('sel','idle'),('mid','mid'),('full','full'),('locked','locked'),('hunting','hunting')):
    d = open(f'/tmp/rd/sim4/{k}.rgb','rb').read()
    Image.frombytes('RGB',(240,320),d).resize((720,960),Image.NEAREST).save(f'/workspace/out/radar-charge/D/stills/D_{nm}@3x.png')
P
