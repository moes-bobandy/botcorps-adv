set -e
cd /workspace && git -C cyd-portrait-wt show 3240b03:src/live/lv_radar.h > radar-charge-scratch/src/live/lv_radar.h && python3 out/radar-charge/src/apply_charge.py radar-charge-scratch/src/live/lv_radar.h
cd radar-charge-scratch
for v in 1 2 3; do g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=$v -DLV_RD_CG_LOG -o /tmp/rc_sim$v tools/livescan/sim_live.cpp -lz & g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=$v -DLV_RD_CG_NODRAW=1 -o /tmp/rc_base$v tools/livescan/sim_live.cpp -lz & done; wait
for v in 1 2 3; do for b in sim base; do rm -rf /tmp/rc/$b$v; mkdir /tmp/rc/$b$v; /tmp/rc_$b$v /tmp/rc/s$v.txt /tmp/rc/$b$v > /tmp/rc/$b$v/out.txt 2> /tmp/rc/$b$v/err.txt; done; done
cd /tmp/rc && python3 - <<'P'
from PIL import Image
for v in (1,2,3):
    for k in ('sel','mid','full','locked'):
        for b in ('sim','base'):
            d=open(f'/tmp/rc/{b}{v}/{k}.rgb','rb').read()
            Image.frombytes('RGB',(240,320),d).resize((720,960),Image.NEAREST).save(f'/tmp/rc/{b}{v}_{k}.png')
P
