#!/usr/bin/env python3
"""v5d (Mobandy 2:38 PM ET 'as close to the gibson tower pullup as possible'): side-by-side of the real Gibson radar tower lock as it ships
(LV_RD_CHARGE 1 = LOCKED charge lock D4: tap a tower to select it, hold HOLD TO LOCK 1 s -> lock; prefix from
tools/livescan/scripts/radar_lock.txt verbatim up to its 'rhold 97 140 1200', split into press / ticks / release / tap so frames can be dumped mid-hold: hold a tower 1.2 s -> LOCK) and the Morpheus open (time-driven clone of the same bracket
law, then the HACKbot search). Rows: Gibson | Morpheus. Columns: hold 25 / 50 / 75 %, lock snap +~90 ms, lock +~300 ms. Real sim frames.
Usage: pullup_compare.py OUT.png"""
import json, os, subprocess, sys, numpy as np
from PIL import Image, ImageDraw
D = os.path.dirname(os.path.abspath(__file__)); W = '/tmp/m18/pullup'
gib = ["tick 67", "tick 190", "tap 110 70", "tick 330", "tick 45", "tap 20 215", "tick 3", "tick 30", "tap 20 215", "tick 30", "tap 20 215", "tick 3", "tick 30",
       "tap 97 140", "tick 10", "press 200 255", "tick 10", "dump g1", "tick 10", "dump g2", "tick 10", "dump g3", "tick 6", "radarstate", "dump g4", "tick 7", "dump g5", "release", "radarstate"]
mor = ["tick 67", "tick 190", "go 13", "tick 2", "tick 552", "tick 128", "tower DEAUTH", "tick 10", "dump m1", "tick 10", "dump m2", "tick 10", "dump m3", "tick 6", "dump m4", "tick 7", "dump m5"]
def run(steps, out):
    json.dump(steps, open(out + '.json', 'w')); subprocess.run([sys.executable, D + '/drive.py', out + '.json', out], check=True, capture_output=True)
def rgb(p): return np.fromfile(p, dtype=np.uint8).reshape(320, 240, 3)
if __name__ == '__main__':
    os.makedirs(W, exist_ok=True); run(gib, W + '/gib'); run(mor, W + '/mor')
    cols = ['hold 0.3 s', 'hold 0.6 s', 'hold 0.9 s', 'hold 1.08 s (LOCK +0.08)', 'hold 1.29 s (LOCK +0.29)']
    S = 2; sheet = Image.new('RGB', (240 * S * 5 + 60, 320 * S * 2 + 40), (12, 12, 12)); d = ImageDraw.Draw(sheet)
    for r, (tag, pre) in enumerate((('GIBSON', 'gib/g'), ('MORPHEUS', 'mor/m'))):
        d.text((4, 40 + r * 320 * S + 300), tag, fill=(255, 255, 255))
        for c in range(5):
            im = Image.fromarray(rgb(W + '/%s%d.rgb' % (pre, c + 1))).resize((240 * S, 320 * S), Image.NEAREST); sheet.paste(im, (60 + c * 240 * S, 40 + r * 320 * S))
            if r == 0: d.text((60 + c * 240 * S + 4, 14), cols[c], fill=(255, 255, 255))
    sheet.save(sys.argv[1]); print(open(W + '/gib/run.log').read().count('lock=1'), 'gibson radarstate lock=1 lines (expect 2)')
