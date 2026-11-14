#!/usr/bin/env python3
"""Spread search v2 (simulated annealing). Start: a stadium in three depth tiers (tall T1 code glass at the back, T3 lanterns mid,
short T2 white lights in front) so each tier owns its own band of the tap field; columns spread across the 240 px room.
Moves: one pillar's sx / z0, swap two pillars of a tier, or one height (T1 >= 1.2 T3 >= 1.44 T2, T2 >= 0.28).
Score: worst-pillar tap p10 (cap 28) first, then worst emblem visible fraction, means; zero-cell samples penalised.
Usage: opt18b.py SEED ITERS OUT.json"""
import json, sys, random, math, numpy as np
sys.path.insert(0, __file__.rsplit('/', 1)[0]); import model18 as M
seed = int(sys.argv[1]); iters = int(sys.argv[2]); outp = sys.argv[3]; rng = random.Random(seed)
ts = np.linspace(9.0, 9.0 + 0.3 * 319, 40); STEP = 2
def score(p):
    sq, vf = M.evaluate(p['sx'], p['z0'], {1: p['H'][0], 3: p['H'][1], 2: p['H'][2]}, ts, STEP)
    p10 = np.percentile(sq, 10, axis=0); vm = vf.mean(0); zero = (sq == 0).sum()
    return min(p10.min(), 28) * 3 + np.minimum(p10, 34).mean() * 0.5 + min(vm.min(), 0.9) * 40 + vm.mean() * 10 - zero * 5, p10, vm, zero
TIER = {1: (4.15, 0.15), 3: (2.7, 0.3), 2: (1.72, 0.06)}
sx = [0.0] * M.N; z0 = [0.0] * M.N
for l in (1, 3, 2):
    ids = [i for i in range(M.N) if M.LOOKOF[i] == l]; rng.shuffle(ids); n = len(ids); off = rng.uniform(-6, 6)
    for k, i in enumerate(ids): sx[i] = 30 + 180 * (k + 0.5) / n + off; z0[i] = TIER[l][0] + rng.uniform(-TIER[l][1], TIER[l][1])
p = dict(sx=sx, z0=z0, H=[rng.uniform(2.1, 2.4), rng.uniform(1.0, 1.25), rng.uniform(0.32, 0.45)]); cur = score(p); best = (cur, p)
print('start', round(cur[0], 2), cur[1].min(), round(cur[2].min(), 3), cur[3], flush=True)
for it in range(iters):
    T = 0.6 * (1 - it / iters) + 0.02
    q = json.loads(json.dumps(p)); r = rng.random()
    if r < 0.45: i = rng.randrange(M.N); q['sx'][i] = min(215, max(25, q['sx'][i] + rng.gauss(0, 6)))
    elif r < 0.8: i = rng.randrange(M.N); q['z0'][i] = min(4.35, max(1.65, q['z0'][i] + rng.gauss(0, 0.15)))
    elif r < 0.9:
        l = rng.choice((1, 3, 2)); ids = [i for i in range(M.N) if M.LOOKOF[i] == l]; i, j = rng.sample(ids, 2); q['sx'][i], q['sx'][j] = q['sx'][j], q['sx'][i]
    else:
        k = rng.randrange(3); q['H'][k] += rng.gauss(0, 0.06)
        if not (q['H'][0] >= 1.2 * q['H'][1] and q['H'][1] >= 1.2 * q['H'][2] and q['H'][2] >= 0.28 and q['H'][0] <= 2.6): continue
    s = score(q)
    if s[0] >= cur[0] or rng.random() < math.exp((s[0] - cur[0]) / T):
        p, cur = q, s
        if cur[0] > best[0][0]:
            best = (cur, json.loads(json.dumps(p))); b = best[0]
            print(it, round(b[0], 2), 'p10min', b[1].min(), 'vismin', round(b[2].min(), 3), 'zero', b[3], 'H', [round(h, 3) for h in p['H']], flush=True)
            json.dump(dict(best[1], score=b[0], p10=b[1].tolist(), vis=b[2].tolist(), zero=int(b[3]), seed=seed), open(outp, 'w'), indent=1)
print('done', best[0][0])
