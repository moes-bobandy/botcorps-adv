#!/bin/bash
# D4 stills: scratch copy of locked lv_radar.h @3240b03 + apply_charge.py (A/B/C) + apply_charge_D4.py; sim (D) and NODRAW baseline.
set -e
cd /workspace && git -C cyd-portrait-wt show 3240b03:src/live/lv_radar.h > radar-charge-D4-scratch/src/live/lv_radar.h
python3 out/radar-charge/src/apply_charge.py radar-charge-D4-scratch/src/live/lv_radar.h && python3 out/radar-charge/D4/src/apply_charge_D4.py radar-charge-D4-scratch/src/live/lv_radar.h
cd radar-charge-D4-scratch
g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=4 -DLV_RD_CG_LOG -o /tmp/rd_sim7 tools/livescan/sim_live.cpp -lz &
g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=4 -DLV_RD_CG_LOG -DLV_RD_CG_NODRAW=1 -o /tmp/rd_base7 tools/livescan/sim_live.cpp -lz & wait
mkdir -p /tmp/rd4; for b in sim base; do rm -rf /tmp/rd4/${b}4; mkdir /tmp/rd4/${b}4; /tmp/rd_${b}7 /workspace/out/radar-charge/D4/src/scripts/charge_D4.txt /tmp/rd4/${b}4 > /tmp/rd4/${b}4/out.txt 2> /tmp/rd4/${b}4/err.txt; done
mkdir -p /workspace/out/radar-charge/D4/stills
python3 - <<'P'
from PIL import Image
for k, nm in (('sel','idle'),('mid','mid'),('full','full'),('lockfire','lockfire'),('locked','locked'),('hunting','hunting')):
    d = open(f'/tmp/rd4/sim4/{k}.rgb','rb').read()
    Image.frombytes('RGB',(240,320),d).resize((720,960),Image.NEAREST).save(f'/workspace/out/radar-charge/D4/stills/D4_{nm}@3x.png')
P
