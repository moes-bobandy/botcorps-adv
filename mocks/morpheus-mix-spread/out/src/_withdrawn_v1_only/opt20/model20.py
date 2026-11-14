#!/usr/bin/env python3
"""Python model of the Gibson engine placement for the 20-pillar Morpheus room (lv_rd_place + mo11_drift, idle camera 0.5 sin(0.45 t)),
mo_hit tap cells (nearest centre line, front first) and a coarse idle emblem-visibility estimate. Used only to PICK the new-8 layout;
the gate re-measures everything from the real sim (touch20.py)."""
import numpy as np
F, HOR, CAMH, Y0, Y1, FY0, FY1 = 120.0, 138, 0.9, 34, 285, 35, 285
TW = 1.35; W = 0.12 * TW; D = 0.24
OLD = dict(
 sx=[125.8,144.0,201.2,219.9,196.0,93.9,199.1,94.2,41.1,35.7,125.8,36.0],
 z0=[2.386,3.123,1.650,4.105,3.859,4.350,1.895,2.877,3.368,2.632,3.614,2.141],
 rssi=[-70,-66,-79,-42,-58,-54,-85,-74,-62,-50,-46,-92],
 ax=[0.13,0.16,0.20,0.12,0.15,0.18,0.10,0.17,0.14,0.11,0.19,0.16],
 wx=[0.41,0.57,0.33,0.69,0.47,0.36,0.62,0.52,0.44,0.73,0.38,0.55],
 az=[0.20,0.26,0.30,0.32,0.24,0.34,0.18,0.28,0.31,0.27,0.33,0.25],
 wz=[0.29,0.43,0.51,0.26,0.37,0.58,0.32,0.47,0.61,0.35,0.53,0.40],
 wh=[0.71,0.83,0.62,0.94,0.77,0.66,0.88,0.59,0.98,0.74,0.69,0.91])
THREAT = [2,2,0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]   # 0 HIGH 1 MEDIUM 2 LOW (MO_DEV order)
LOOKOF = [{0:1,1:3,2:2}[t] for t in THREAT]
def embS(fw): return 40 if fw>=50 else 32 if fw>=44 else 16 if fw>=20 else 10 if fw>=12 else 8 if fw>=9 else 0
def place(L, t, hs):
    n = len(L['sx']); cam = 0.5*np.sin(0.45*t); out = []
    for i in range(n):
        lvl = min(60, max(4, L['rssi'][i]+100)); H = (0.12 + lvl/60*1.9) * hs[LOOKOF[i]]
        Z0 = L['z0'][i]; X0 = (L['sx'][i]-120)*Z0/F
        dx = L['ax'][i]*(Z0/2)*np.sin(L['wx'][i]*t + i*2.399); dz = L['az'][i]*(Z0/3)*np.sin(L['wz'][i]*t + i*1.713 + 0.9)
        H *= 1 + 0.06*np.sin(L['wh'][i]*t + i*0.77); X = X0+dx; Z = Z0+dz
        px = lambda X_, Z_: int(round(120 + F*(X_-cam)/Z_)); py = lambda Y_, Z_: int(round(HOR + F*(CAMH-Y_)/Z_))
        T = dict(i=i, Z=Z, H=H, x0=px(X-W,Z), x1=px(X+W,Z), yt=py(H,Z), yb=py(0,Z), bx0=px(X-W,Z+D), bx1=px(X+W,Z+D), byt=py(H,Z+D))
        out.append(T)
    out.sort(key=lambda T: -T['Z'])   # far -> near
    return out
def cells(tw, step=2):
    xs = np.arange(0, 240, step); ys = np.arange(FY0, FY1+1, step); X, Y = np.meshgrid(xs, ys)
    best = np.full(X.shape, -1); bd = np.full(X.shape, np.inf)
    for T in reversed(tw):   # nearest first; strict < keeps the nearer on ties
        hy0 = max(FY0, T['byt']-3); hy1 = min(FY1, T['yb']+3)
        dx2 = 2*X - T['x0'] - T['x1']; dy = np.where(Y < hy0, hy0-Y, np.where(Y > hy1, Y-hy1, 0))
        d = dx2.astype(float)**2 + 4.0*dy**2; m = d < bd; bd[m] = d[m]; best[m] = T['i']
    return best
def maxsq(mask):
    h, w = mask.shape; dp = np.zeros((h+1, w+1), int); b = 0
    for r in range(h):
        row = mask[r]
        for c in range(w):
            if row[c]: v = 1 + min(dp[r][c+1], dp[r+1][c], dp[r][c]); dp[r+1][c+1] = v; b = max(b, v)
    return b
def maxsq_all(lab, n):
    """largest all-one-label square per label (single DP pass)"""
    h, w = lab.shape; dp = np.zeros((h, w), int); out = np.zeros(n); L = lab.tolist(); P = [[0]*w for _ in range(h)]
    for r in range(h):
        Lr = L[r]; Pr = P[r]; Pu = P[r-1] if r else None; Lu = L[r-1] if r else None
        for c in range(w):
            k = Lr[c]
            if k < 0: continue
            if r and c and Lu[c] == k and Lr[c-1] == k and Lu[c-1] == k: v = 1 + min(Pu[c], Pr[c-1], Pu[c-1])
            else: v = 1
            Pr[c] = v
            if v > out[k]: out[k] = v
    return out
def emb_vis(tw):
    """fraction of each pillar's emblem rect not covered by nearer pillars' boxes (front + side + cap)"""
    res = {}
    for k, T in enumerate(tw):
        fw = T['x1'] - T['x0']; S = embS(fw)
        if S == 0: S = 6
        ex0 = (T['x0']+T['x1'])//2 - S//2; ey0 = T['yt'] + 4; ex1, ey1 = ex0+S-1, ey0+S-1
        m = np.ones((S, S), bool)
        for U in tw[k+1:]:
            ux0 = min(U['x0'], U['bx0']); ux1 = max(U['x1'], U['bx1']); uy0 = U['byt']; uy1 = U['yb']
            a0, a1 = max(ex0, ux0), min(ex1, ux1); b0, b1 = max(ey0, uy0), min(ey1, uy1)
            if a0 <= a1 and b0 <= b1: m[b0-ey0:b1-ey0+1, a0-ex0:a1-ex0+1] = False
        cx0, cx1, cy0, cy1 = max(ex0,0), min(ex1,239), max(ey0,Y0+1), min(ey1,Y1)
        on = np.zeros_like(m)
        if cx0 <= cx1 and cy0 <= cy1: on[cy0-ey0:cy1-ey0+1, cx0-ex0:cx1-ex0+1] = True
        res[T['i']] = float((m & on).mean())
    return res
def evaluate(L, hs, ts, step=4):
    n = len(L['sx']); sq = np.zeros((len(ts), n)); vis = np.zeros((len(ts), n))
    for a, t in enumerate(ts):
        tw = place(L, t, hs); c = cells(tw, step)
        sq[a] = maxsq_all(c, n) * step
        v = emb_vis(tw)
        for i in range(n): vis[a, i] = v[i]
    return sq, vis
