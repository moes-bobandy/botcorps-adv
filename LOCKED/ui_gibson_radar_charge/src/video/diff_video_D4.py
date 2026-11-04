"""Per-frame outside-mask diff of the D4 video run vs the same script built with LV_RD_CG_NODRAW=1 (same mask as diff_D4.py).
usage: diff_video_D4.py SIMDIR BASEDIR T_LAST_MS   (frame k is at T_LAST - 30*(F-1-k); SIMDIR has charge.rgb + err.txt from an LV_RD_CG_LOG build)"""
import sys, json, collections
import numpy as np
from scipy.ndimage import binary_dilation
S, Bd, TL = sys.argv[1], sys.argv[2], int(sys.argv[3])
a = np.fromfile(f'{S}/charge.rgb', np.uint8).reshape(-1, 320, 240, 3); b = np.fromfile(f'{Bd}/charge.rgb', np.uint8).reshape(-1, 320, 240, 3)
F = len(a); L = collections.defaultdict(list)
for l in open(f'{S}/err.txt'):
    if l.startswith(('CG ', 'CGP ')): L[int(l.split()[1][2:])].append(l.split())
bad = []; tot = 0; chg = 0
for k in range(F):
    t = TL - 30 * (F - 1 - k); ch = (a[k] != b[k]).any(2); path = np.zeros((320, 240), bool); box = np.zeros((320, 240), bool)
    for f in L.get(t, []):
        if f[0] == 'CGP':
            for p in f[4:]:
                x, y = map(int, p.split(','))
                if 0 <= x < 240 and 0 <= y < 320: path[y, x] = True
        else:
            x0, y0, x1, y1 = map(int, f[3:7]); box[max(0, y0 - 2):y1 + 3, max(0, x0 - 2):x1 + 3] = True
    m = (binary_dilation(path, np.ones((3, 3), bool), iterations=8) if path.any() else path) | box
    o = int((ch & ~m).sum()); tot += o; chg += int(ch.sum())
    if o: bad.append((k, t, o))
print(json.dumps(dict(sim_frames=F, frames_with_outside_mask_px=len(bad), outside_mask_px_total=tot, changed_px_total=chg, bad=bad[:20])))
