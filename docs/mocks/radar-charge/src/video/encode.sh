#!/bin/bash
# Render + encode the three charge videos. Sim ticks are 30 ms (33.3 fps); output is 30 fps,
# each output frame j shows the sim frame at time j/30 s (floor), scaled 3x nearest-neighbour.
set -e
cd /workspace && git -C cyd-portrait-wt show 3240b03:src/live/lv_radar.h > radar-charge-scratch/src/live/lv_radar.h && python3 out/radar-charge/src/apply_charge.py radar-charge-scratch/src/live/lv_radar.h
cd radar-charge-scratch; mkdir -p /tmp/rv
for v in 1 2 3; do L=$(echo _ABC | cut -c$((v+1))); g++ -O2 -std=gnu++17 -w -DLV_RD_CHARGE=$v -o /tmp/rv_sim$v tools/livescan/sim_live.cpp -lz
  rm -rf /tmp/rv/o$v; mkdir /tmp/rv/o$v; /tmp/rv_sim$v /workspace/out/radar-charge/src/video/video_$L.txt /tmp/rv/o$v > /tmp/rv/o$v/out.txt 2>&1; done
python3 - <<'P'
import subprocess, numpy as np
for v,L in ((1,'A'),(2,'B'),(3,'C')):
    fr=np.fromfile(f'/tmp/rv/o{v}/charge.rgb',np.uint8).reshape(-1,320,240,3); F=len(fr); n=int(F*30*30/1000)
    p=subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s','720x960','-r','30','-i','-','-c:v','libx264','-preset','slow','-crf','14','-pix_fmt','yuv420p','-movflags','+faststart',f'/workspace/out/radar-charge/video/charge_{L}_3x.mp4'],stdin=subprocess.PIPE)
    for j in range(n): p.stdin.write(fr[min(F-1,int(j*1000/30/30))].repeat(3,0).repeat(3,1).tobytes())
    p.stdin.close(); p.wait()
P
