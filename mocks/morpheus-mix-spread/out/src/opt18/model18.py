#!/usr/bin/env python3
"""morpheus-mix spread (2026-11-14): Python model of the 18-pillar room (WIFI + BLUETOOTH removed) used only to PICK the spread
placement (screen column sx, depth z0) and the three fixed style heights. Same Gibson projection, idle camera 0.5 sin(0.45 t), v11 drift
law (dx, dz; drift tables unchanged per device), no height breathing (three fixed heights). Tap cells = mo_hit (nearest centre line,
front first). The gate re-measures everything on the real sim (touch20.py)."""
import numpy as np
from scipy.ndimage import distance_transform_cdt
F, HOR, CAMH, FY0, FY1 = 120.0, 138, 0.9, 35, 285
import os as _os
W = 0.12 * float(_os.environ.get("MO_TW", "1.35")); D = 0.24   # v4: MO_TW=1.8 (wider pillar faces for bigger emblems)
NAMES = ["DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
THREAT = [0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]          # 0 HIGH 1 MEDIUM 2 LOW
LOOKOF = [{0:1,1:3,2:2}[t] for t in THREAT]                # 1 T1 code glass, 3 T3 lantern, 2 T2 white light
# per-device drift tables (v1 values for the same devices, WIFI/BLUETOOTH rows dropped)
AX = [0.20,0.12,0.15,0.18,0.10,0.17,0.14,0.11,0.19,0.16, 0.135,0.165,0.125,0.185,0.155,0.175,0.105,0.145]
WX = [0.33,0.69,0.47,0.36,0.62,0.52,0.44,0.73,0.38,0.55, 0.54,0.39,0.35,0.71,0.65,0.42,0.49,0.59]
AZ = [0.30,0.32,0.24,0.34,0.18,0.28,0.31,0.27,0.33,0.25, 0.335,0.22,0.21,0.19,0.23,0.305,0.315,0.29]
WZ = [0.51,0.26,0.37,0.58,0.32,0.47,0.61,0.35,0.53,0.40, 0.55,0.49,0.34,0.45,0.31,0.27,0.39,0.59]
WH = [0.62,0.94,0.77,0.66,0.88,0.59,0.98,0.74,0.69,0.91, 0.73,0.96,0.68,0.8,0.61,0.64,0.9,0.86]
N = 18
def embS(fw): return 40 if fw >= 50 else 32 if fw >= 44 else 16 if fw >= 20 else 10 if fw >= 12 else 8 if fw >= 8 else 0
def place(sx, z0, H, t):
    cam = 0.5 * np.sin(0.45 * t); out = []
    r = lambda v: int(np.floor(v + 0.5)) if v >= 0 else -int(np.floor(-v + 0.5))
    for i in range(N):
        Z0 = z0[i]; X0 = (sx[i] - 120) * Z0 / F
        dx = AX[i] * (Z0 / 2) * np.sin(WX[i] * t + i * 2.399); dz = AZ[i] * (Z0 / 3) * np.sin(WZ[i] * t + i * 1.713 + 0.9)
        X = X0 + dx; Z = Z0 + dz; h = H[LOOKOF[i]]
        px = lambda X_, Z_: r(120 + F * (X_ - cam) / Z_); py = lambda Y_, Z_: r(HOR + F * (CAMH - Y_) / Z_)
        out.append(dict(i=i, Z=Z, x0=px(X - W, Z), x1=px(X + W, Z), yt=py(h, Z), yb=py(0, Z), bx0=px(X - W, Z + D), bx1=px(X + W, Z + D), byt=py(h, Z + D)))
    out.sort(key=lambda T: -T['Z']); return out
def cells(tw, step=2):
    ys, xs = np.mgrid[FY0:FY1 + 1:step, 0:240:step]; lab = np.full(xs.shape, -1); bd = np.full(xs.shape, np.inf)
    for T in reversed(tw):
        hy0 = max(FY0, T['byt'] - 3); hy1 = min(FY1, T['yb'] + 3); dx2 = 2 * xs - T['x0'] - T['x1']
        dy = np.where(ys < hy0, hy0 - ys, np.where(ys > hy1, ys - hy1, 0)); d = dx2.astype(float) ** 2 + 4.0 * dy ** 2
        m = d < bd; bd[m] = d[m]; lab[m] = T['i']
    return lab
def squares(lab, step=2):
    out = np.zeros(N)
    for i in range(N):
        m = lab == i
        if m.any(): out[i] = (2 * distance_transform_cdt(np.pad(m, 1), metric='chessboard').max() - 1) * step
    return out
def vis(tw):
    res = np.zeros(N)
    for k, T in enumerate(tw):
        S = embS(T['x1'] - T['x0']) or 6; ex0 = (T['x0'] + T['x1']) // 2 - S // 2; ey0 = T['yt'] + 3
        yy, xx = np.mgrid[ey0:ey0 + S, ex0:ex0 + S]; m = (xx >= 0) & (xx <= 239) & (yy >= 35) & (yy <= 285)
        for U in tw[k + 1:]:
            m &= ~((xx >= min(U['x0'], U['bx0'])) & (xx <= max(U['x1'], U['bx1'])) & (yy >= U['byt']) & (yy <= U['yb']))
        res[T['i']] = m.mean()
    return res
def evaluate(sx, z0, H, ts, step=2):
    sq = np.zeros((len(ts), N)); vf = np.zeros((len(ts), N))
    for a, t in enumerate(ts):
        tw = place(sx, z0, H, t); sq[a] = squares(cells(tw, step), step); vf[a] = vis(tw)
    return sq, vf
