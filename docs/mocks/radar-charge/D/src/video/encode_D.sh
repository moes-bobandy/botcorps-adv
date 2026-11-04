#!/bin/bash
# Render + encode charge_D_3x.mp4 (same timeline as A/B/C, then HUNT tapped at the end of the settled lock + 3 s hunting).
# Sim ticks are 30 ms; output 30 fps, frame j shows the sim frame at j/30 s (floor), 3x nearest-neighbour.
set -e
cd /workspace && git -C cyd-portrait-wt show 3240b03:src/live/lv_radar.h > radar-charge-D-scratch/src/live/lv_radar.h
python3 out/radar-charge/src/apply_charge.py radar-charge-D-scratch/src/live/lv_radar.h && python3 out/radar-charge/D/src/apply_charge_D.py radar-charge-D-scratch/src/live/lv_radar.h
cd radar-charge-D-scratch && g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=4 -o /tmp/rv_sim4 tools/livescan/sim_live.cpp -lz
mkdir -p /tmp/rv; rm -rf /tmp/rv/o4; mkdir /tmp/rv/o4; /tmp/rv_sim4 /workspace/out/radar-charge/D/src/video/video_D.txt /tmp/rv/o4 > /tmp/rv/o4/out.txt 2>&1
mkdir -p /workspace/out/radar-charge/D/video
python3 - <<'P'
import subprocess, numpy as np
fr=np.fromfile('/tmp/rv/o4/charge.rgb',np.uint8).reshape(-1,320,240,3); F=len(fr); n=int(F*30*30/1000)
p=subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s','720x960','-r','30','-i','-','-c:v','libx264','-preset','slow','-crf','14','-pix_fmt','yuv420p','-movflags','+faststart','/workspace/out/radar-charge/D/video/charge_D_3x.mp4'],stdin=subprocess.PIPE)
for j in range(n): p.stdin.write(fr[min(F-1,int(j*1000/30/30))].repeat(3,0).repeat(3,1).tobytes())
p.stdin.close(); p.wait(); print('sim frames', F, 'video frames', n)
P
