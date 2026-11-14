#!/usr/bin/env python3
"""320 idle samples (0.3 s apart) from a sim: per sample, MOTW boxes + MOEMB rects. Tap cells (mo_hit: nearest front-face centre line,
front first on ties) on the full 1 px field, largest square per pillar (chessboard distance transform); idle emblem visibility = emblem rect
pixels inside the field and not under any NEARER pillar's lv_rd_box (front + side + cap); per-style mean H."""
import re, subprocess, os, numpy as np
from scipy.ndimage import distance_transform_cdt
SCRIPT = ['tick 67', 'tick 190', 'go 13', 'tick 2'] + ['tick 10', 'itwstate'] * 320
def sample(sim, cwd, out):
    os.makedirs(out, exist_ok=True); p = out + '/touch.txt'; open(p, 'w').write('\n'.join(SCRIPT) + '\n')
    txt = subprocess.run([sim, p, out], cwd=cwd, capture_output=True, text=True, timeout=1200).stdout
    S = []
    for blk in txt.split('MOCAM')[1:]:
        ordr = [int(x) for x in re.search(r'ord=([\d,]+)', blk).group(1).split(',')]; tw = {}; em = {}
        t = float(re.search(r' t=([\d.]+)', blk).group(1))
        for m in re.finditer(r'MOTW (\d+) X=(-?[\d.]+) Z=(-?[\d.]+) H=(-?[\d.]+) f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] b=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] halo=\d+ dx=(-?[\d.]+) dz=(-?[\d.]+) hk=(-?[\d.]+)', blk):
            g = m.groups(); i = int(g[0]); v = list(map(int, g[4:12]))
            tw[i] = dict(X=float(g[1]), Z=float(g[2]), H=float(g[3]), x0=v[0], x1=v[1], yt=v[2], yb=v[3], bx0=v[4], bx1=v[5], byt=v[6], byb=v[7], dx=float(g[12]), dz=float(g[13]), hk=float(g[14]))
        for m in re.finditer(r'MOEMB (\d+) look=(\d) threat=(\w+) hs=([\d.]+) e=\[(-?\d+),(-?\d+),(-?\d+)\]', blk):
            g = m.groups(); em[int(g[0])] = dict(look=int(g[1]), threat=g[2], hs=float(g[3]), x=int(g[4]), y=int(g[5]), S=int(g[6]))
        S.append(dict(t=t, ord=ordr, tw=tw, em=em))
    return S
Y, X = np.mgrid[35:286, 0:240]
def cells(s):
    lab = np.full(X.shape, -1); bd = np.full(X.shape, np.inf)
    for i in reversed(s['ord']):
        T = s['tw'][i]; hy0 = max(35, T['byt'] - 3); hy1 = min(285, T['yb'] + 3)
        dx2 = 2 * X - T['x0'] - T['x1']; dy = np.where(Y < hy0, hy0 - Y, np.where(Y > hy1, Y - hy1, 0))
        d = dx2.astype(float) ** 2 + 4.0 * dy ** 2; m = d < bd; bd[m] = d[m]; lab[m] = i
    return lab
def square(lab, i):
    m = lab == i
    if not m.any(): return 0
    d = distance_transform_cdt(np.pad(m, 1), metric='chessboard'); return int(2 * d.max() - 1)
def vis(s, i):
    e = s['em'].get(i); T = s['tw'][i]
    if not e or e['S'] <= 0: return 0, 1
    S = e['S']; m = np.zeros((S, S), bool); ex, ey = e['x'], e['y']
    yy, xx = np.mgrid[ey:ey + S, ex:ex + S]; m[(xx >= 0) & (xx <= 239) & (yy >= 35) & (yy <= 285)] = True
    k = s['ord'].index(i)
    for j in s['ord'][k + 1:]:
        U = s['tw'][j]; ux0 = min(U['x0'], U['bx0']); ux1 = max(U['x1'], U['bx1'])
        m &= ~((xx >= ux0) & (xx <= ux1) & (yy >= min(U['yt'], U['byt'])) & (yy <= U['yb']))   # v5 fix: box top = min(front top, back top); was byt only (missed the front face top for tall pillars)
    return int(m.sum()), S * S
def analyse(S, n=18):
    sq = np.zeros((len(S), n)); vp = np.zeros((len(S), n)); vf = np.zeros((len(S), n))
    for a, s in enumerate(S):
        lab = cells(s)
        for i in range(n):
            sq[a, i] = square(lab, i); p, tot = vis(s, i); vp[a, i] = p; vf[a, i] = p / tot
    return sq, vp, vf
