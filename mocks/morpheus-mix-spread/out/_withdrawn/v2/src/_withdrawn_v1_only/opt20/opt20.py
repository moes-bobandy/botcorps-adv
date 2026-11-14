#!/usr/bin/env python3
"""Pick the new-8 drift permutations (pools as towers-mix v1) + the two slight style height factors, sx / z0 / rssi fixed (seed-15 base).
Score: worst-pillar emblem visibility first, then tap p10 and worst cell. Writes layout.json."""
import json, sys, random, numpy as np
sys.path.insert(0, __file__.rsplit('/',1)[0]); import model20 as M
NEW = dict(sx=[84.0,196.6,142.7,63.7,109.6,121.7,169.8,161.9], z0=[3.982,2.264,2.018,3.491,1.772,3.0,3.736,2.509], rssi=[-48,-72,-82,-44,-88,-52,-64,-77])
POOL = dict(ax=[0.105,0.125,0.135,0.145,0.155,0.165,0.175,0.185], wx=[0.35,0.39,0.42,0.49,0.54,0.59,0.65,0.71],
            az=[0.19,0.21,0.22,0.23,0.29,0.305,0.315,0.335], wz=[0.27,0.31,0.34,0.39,0.45,0.49,0.55,0.59], wh=[0.61,0.64,0.68,0.73,0.80,0.86,0.90,0.96])
ts = np.linspace(9.0, 9.0 + 0.3 * 319, 40)
def build(perm):
    L = {k: list(M.OLD[k]) for k in M.OLD}
    for k in ('sx','z0','rssi'): L[k] += NEW[k]
    for k in POOL: L[k] += [POOL[k][j] for j in perm[k]]
    return L
def score(L, h1, h2):
    hs = {1: h1, 2: h2, 3: 1.0}; sq, vis = M.evaluate(L, hs, ts, 4)
    vmean = vis.mean(0); p10 = np.percentile(sq, 10, axis=0)
    return (vmean.min(), np.minimum(p10, 20).mean(), sq.min()), vmean, p10, sq.min(0)
rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 15); best = None
N = int(sys.argv[2]) if len(sys.argv) > 2 else 60
for it in range(N):
    perm = {k: rng.sample(range(8), 8) for k in POOL}; h1 = rng.uniform(1.05, 1.15); h2 = rng.uniform(0.85, 0.95)
    L = build(perm); (vmin, p10m, wmin), vm, p10, mn = score(L, h1, h2)
    key = vmin * 2 + p10m / 20 + min(wmin, 12) / 24
    if best is None or key > best[0]:
        best = (key, perm, h1, h2, vmin, p10m, wmin); print(it, round(key,3), 'vis_min', round(vmin,3), 'p10m', round(p10m,2), 'worst', wmin, 'h', round(h1,3), round(h2,3), flush=True)
key, perm, h1, h2, *_ = best; L = build(perm)
out = {k: L[k][12:] for k in ('sx','z0','rssi','ax','wx','az','wz','wh')}; out.update(h1=round(h1,3), h2=round(h2,3), seed=rng.random() and int(sys.argv[1]) if len(sys.argv) > 1 else 15)
json.dump(out, open(__file__.rsplit('/',1)[0] + '/L_%d.json' % (int(sys.argv[1]) if len(sys.argv) > 1 else 15), 'w'), indent=1); print(json.dumps(out))
