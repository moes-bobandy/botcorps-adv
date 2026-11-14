#!/usr/bin/env python3
"""v5d (Mobandy 2:34 PM ET 'Shorten the near towers'): world height H_i = H_group_i * k(Z_i), a smooth fixed depth factor (no breathing):
k(Z) = k_near + (1 - k_near) * smoothstep((Z - 1.6) / 3.2) over Gibson's depth band 1.6..4.8 (lv_radar.h lv_rd_place), so a near tower is
shorter and the emblems behind show above its top; the group ratios 1.00 / 0.80 / 0.62 hold at every depth. Depth = the Gibson map of the
EX.DATA rssi (src/gibson_depth_drift.py); a tall tower is pushed back from its Gibson depth only if its top would still leave the field.
Checks per tower (model, camera centred, mean depth): on-screen height >= 45% of the tallest on-screen tower; >= 6 folder rows under the
emblem (mo_mix.inc T3 geometry: brass bands, emblem S = face width - 1 up to 36, idle row pitch 8 px when the inner face is >= 22 px, else 4)."""
import json, os, sys, numpy as np
D = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, D + '/opt18'); sys.path.insert(0, D)
import model18 as M, gibson_depth_drift as G
F, HOR, CAMH = 120.0, 138, 0.9
def kfun(Z, kn, kf=1.0):
    u = min(max((Z - 1.6) / 3.2, 0.0), 1.0); s = u * u * (3 - 2 * u); return kn + (kf - kn) * s
def rows(T, S):
    fw = T['x1'] - T['x0']; bh = 4 if fw >= 40 else 2 if fw >= 14 else 1; m = 5 if fw >= 40 else 2 if fw >= 12 else 1
    bt0 = T['yt'] + 2; bb0 = T['yb'] - 3 - bh; py0 = bt0 + bh + 2; py1 = bb0 - 2; iy = py0 + 2; ly0 = iy + S + 2 if S else py0 + 1; ly1 = py1 - 1
    inner = (T['x1'] - m) - (T['x0'] + m) - 2 + 1; rowh = 8 if inner >= 22 else 4
    return max(0, (ly1 - ly0 - 2) // rowh + 1) if ly1 - ly0 >= 6 else 0
def design(L, kn, kf=1.0):
    Hg = {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}; zg = L['z_gibson']; z0 = []; hk = []
    for i in range(18):
        Z = zg[i]
        for _ in range(200):
            k = kfun(Z, kn, kf); h = Hg[M.LOOKOF[i]] * k
            if Z * (1 - L['az'][i] / 3) >= F * (h - CAMH) / (HOR - 38): break
            Z += 0.01
        z0.append(round(Z, 4)); hk.append(round(kfun(Z, kn, kf), 4))
    return z0, hk
def check(L, z0, hk):
    M.HK[:] = hk; Hg = {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}
    tw = sorted(M.place(L['sx'], z0, Hg, 9.0 + 0.3 * 160), key=lambda T: T['i'])
    hs = [F * Hg[M.LOOKOF[i]] * hk[i] / z0[i] for i in range(18)]
    S = [min(36, max(0, tw[i]['x1'] - tw[i]['x0'] - 1)) for i in range(18)]
    rw = [rows(tw[i], S[i]) for i in range(18)]
    return hs, rw
if __name__ == '__main__':
    L = json.load(open(D + '/layout18.json')); ts = np.linspace(9.0, 9.0 + 0.3 * 319, 64)
    for kn in [1.0, 0.8, 0.7, 0.6, 0.55, 0.5, 0.45, 0.4]:
        z0, hk = design(L, kn); hs, rw = check(L, z0, hk)
        sq, vf = M.evaluate(L['sx'], z0, {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}, ts, 2); v = vf.mean(0)
        print('k_near %.2f  min on-screen h %.0f%% of max  min rows %d  vis min %.3f mean %.3f  z0 HACK/FLIPPER/SKIMMER %s' % (kn, 100 * min(hs) / max(hs), min(rw), v.min(), v.mean(), [z0[1], z0[8], z0[7]]))
