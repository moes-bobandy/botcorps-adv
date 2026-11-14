#!/usr/bin/env python3
"""v5c (Mobandy 2:05 PM ET): (1) depth like the Gibson scan - closer device closer, further further; (2) towers not locked in movement.
DEPTH. Reused Gibson code (tree/src/live/lv_radar.h, verbatim semantics):
  - lv_rd_level(f):  v = f.rssi + 100, clamped 4..60                       (signal level the Gibson scan uses for tower height / meters)
  - lv_rd_place():   T.Z = 1.6f + u * 3.2f   (Gibson's depth band 1.6 .. 4.8; Gibson's u is a MAC hash byte / 255)
                     T.X = (u2 * 2 - 1) * 0.72f * T.Z   (spread across the width at every depth -> screen x 120 +- 86 px)
  NOTE: the real Gibson scan does NOT derive depth from RSSI (its Z is a MAC hash). Here u = (60 - level) / 56 from the EXAMPLE rssi, so a
  strong (close) example signal stands at Z 1.6 and a weak one at 4.8, inside Gibson's own band. Then the existing top-clip rule (tower top
  >= field row 38 at the nearest drift point) pushes a tall tower back only as far as needed.
DRIFT (per tower, independent): own phase, speed and amplitude in X, Z and a faster lateral sway; speeds kept away from the camera sway
(0.45 rad/s) so no tower rides along with the camera. dx = ax (Z0/2) sin(wx t + px) + as Z0 sin(ws t + ps); dz = az (Z0/3) sin(wz t + pz).
EXAMPLE DATA: the rssi values are example data (EX.DATA), not a live scan."""
import json, os, random, math
D = os.path.dirname(os.path.abspath(__file__)); F, HOR, CAMH = 120.0, 138, 0.9
def lv_rd_level(rssi): v = rssi + 100; return 4 if v < 4 else 60 if v > 60 else v      # lv_radar.h lv_rd_level, verbatim
def gibson_z(rssi): u = (60 - lv_rd_level(rssi)) / 56.0; return 1.6 + u * 3.2              # lv_rd_place depth band 1.6f + u * 3.2f
def spread(vals, lo, hi, gap, rng, avoid=None):
    out = []
    while len(out) < 18:
        v = round(rng.uniform(lo, hi), 3)
        if all(abs(v - o) >= gap for o in out) and not (avoid and abs(v - avoid) < 0.07): out.append(v)
    return out
def make(L, seed=2051):
    rng = random.Random(seed); H = {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}
    import sys; sys.path.insert(0, D + '/opt18'); import model18 as M
    L['wx'] = spread(None, 0.16, 0.95, 0.025, rng, 0.45); L['ax'] = [round(rng.uniform(0.22, 0.36), 3) for _ in range(18)]
    L['px'] = [round(rng.uniform(0, 2 * math.pi), 3) for _ in range(18)]
    L['ws'] = spread(None, 0.9, 2.0, 0.025, rng); L['as'] = [round(rng.uniform(0.035, 0.07), 3) for _ in range(18)]
    L['ps'] = [round(rng.uniform(0, 2 * math.pi), 3) for _ in range(18)]
    L['wz'] = spread(None, 0.12, 0.75, 0.02, rng, 0.45); L['az'] = [round(rng.uniform(0.18, 0.34), 3) for _ in range(18)]
    L['pz'] = [round(rng.uniform(0, 2 * math.pi), 3) for _ in range(18)]
    zg = [gibson_z(r) for r in L['rssi']]; z0 = []
    for i in range(18):
        h = H[M.LOOKOF[i]]; zmin = F * (h - CAMH) / (HOR - 38) / (1 - L['az'][i] / 3)   # top-clip rule at the nearest drift point
        z0.append(round(max(zg[i], zmin + 0.005), 4))
    L['z0'] = z0; L['z_gibson'] = [round(z, 4) for z in zg]; L['drift_law'] = 'v5c'; L['drift_seed'] = seed
    return L
if __name__ == '__main__':
    L = json.load(open(D + '/layout18.json')); L = make(L)
    json.dump(L, open(D + '/layout18.json', 'w'), indent=1)
    o = sorted(range(18), key=lambda i: L['z0'][i])
    for i in o: print('%-10s rssi %4d (EX.DATA) level %2d  Gibson Z %.2f  -> z0 %.2f' % (L['names'][i], L['rssi'][i], lv_rd_level(L['rssi'][i]), L['z_gibson'][i], L['z0'][i]))
