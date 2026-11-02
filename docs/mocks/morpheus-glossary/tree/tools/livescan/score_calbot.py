#!/usr/bin/env python3
"""Score the CALbot screen against the approved reference, pixel by pixel.
  score_calbot.py SIM [refs_dir]
Runs SIM on scripts/calbot_gif.txt (no saved cal; presses at the reference flow's times, one dump per 100 ms
reference frame), compares every dump with the reference frame (229cd344 GIF, label patched to CALbot as in
make_calbot_art.py) and the CAL OK dump with b0a422d0 at 3x. Reference colours are put through the panel's
RGB565 (what the CYD can show) before the exact compare. Writes docs/livescan_verify/calbot_v2_*.png (3x),
ref_vs_render_ok.png and calbot_v2_scores.json."""
import sys, os, json, subprocess, numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_calbot_art as art
SIM = sys.argv[1]
if len(sys.argv) > 2: art.REFS = sys.argv[2]
OUT = 'docs/livescan_verify'; RAW = '/workspace/calscratch/raw' if os.path.isdir('/workspace/calscratch') else '/tmp/lvcal'
os.makedirs(OUT, exist_ok=True); os.makedirs(RAW, exist_ok=True)
log = subprocess.run([SIM, 'tools/livescan/scripts/calbot_gif.txt', RAW, 'nocal'], check=True, capture_output=True, text=True).stdout
open(OUT + '/sim_calbot_gif_log.txt', 'w').write(log)
F, ok = art.load_frames()
def q565(a):
    a = a.astype(np.int32); r, g, b = a[..., 0] >> 3, a[..., 1] >> 2, a[..., 2] >> 3
    return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], -1)
def load(f): return np.fromfile('%s/gif_%03d.rgb' % (RAW, f), np.uint8).reshape(320, 240, 3).astype(np.int32)
def x3(a): return np.repeat(np.repeat(a.astype(np.uint8), 3, 0), 3, 1)
per = []
for f in range(len(F)):
    r, ref = load(f), q565(F[f])
    eq = (r == ref).all(2); d = np.sqrt(((r - ref) ** 2).sum(2))
    per.append({'frame': f, 'exact_pct': round(float(eq.mean() * 100), 4), 'close40_pct': round(float((d <= 40).mean() * 100), 4), 'diff_px': int((~eq).sum())})
STATES = {'cal1': 0, 'cal1_holding': 31, 'cal1_locked': 39, 'cal2': 48, 'cal2_holding': 56, 'cal3': 73, 'cal3_holding': 81,
          'cal4': 98, 'cal4_holding': 106, 'ok': 122}
res = {'per_frame': per, 'states': {}}
for nm, f in STATES.items():
    Image.fromarray(x3(load(f))).save('%s/calbot_v2_%s.png' % (OUT, nm))
    res['states'][nm] = dict(per[f], png='calbot_v2_%s.png' % nm)
# CAL OK vs the approved 720x960 b0a422d0 itself (3x nearest render vs the PNG as stored)
okref = np.asarray(Image.open(os.path.join(art.REFS, art.OKP)).convert('RGB')).astype(np.int32)
okr = x3(load(STATES['ok'])).astype(np.int32)
eq3 = (okr == q565(okref)).all(2); d3 = np.sqrt(((okr - okref) ** 2).sum(2))
res['ok_vs_b0a422d0_3x'] = {'exact_pct_565': round(float(eq3.mean() * 100), 4), 'close40_pct': round(float((d3 <= 40).mean() * 100), 4),
                            'raw_exact_pct': round(float((okr == okref).all(2).mean() * 100), 4), 'diff_px_565': int((~eq3).sum())}
diff = (okref // 3).astype(np.uint8); diff[~eq3] = [255, 0, 0]
sep = np.full((960, 12, 3), 255, np.uint8)
Image.fromarray(np.concatenate([okref.astype(np.uint8), sep, okr.astype(np.uint8), sep, diff], 1)).save(OUT + '/ref_vs_render_ok.png')
mean = float(np.mean([p['exact_pct'] for p in per]))
res['all_frames_exact_mean_pct'] = round(mean, 4)
res['frames_below_100'] = [(p['frame'], p['exact_pct']) for p in per if p['exact_pct'] < 100]
json.dump(res, open(OUT + '/calbot_v2_scores.json', 'w'), indent=1)
for nm, s in res['states'].items(): print('%-14s frame %3d exact %.4f%%  close40 %.4f%%  diff px %d' % (nm, s['frame'], s['exact_pct'], s['close40_pct'], s['diff_px']))
print('CAL OK vs b0a422d0 @3x:', res['ok_vs_b0a422d0_3x'])
print('all %d frames: mean exact %.4f%%; below 100%%: %s' % (len(per), mean, res['frames_below_100']))
