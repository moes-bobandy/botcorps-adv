#!/usr/bin/env python3
"""Hill-climb the 18-pillar spread: sx, z0 per pillar + 3 fixed heights (T1 > T3 > T2, each >= 1.2x the next).
Score: tap p10 (capped 26) worst + mean, emblem visible fraction worst + mean, zero-cell samples penalised. Usage: opt18.py SEED ITERS OUT.json"""
import json, sys, random, numpy as np
sys.path.insert(0, __file__.rsplit('/', 1)[0]); import model18 as M
seed = int(sys.argv[1]); iters = int(sys.argv[2]); outp = sys.argv[3]; rng = random.Random(seed)
ts = np.linspace(9.0, 9.0 + 0.3 * 319, 48)
def score(p):
    sq, vf = M.evaluate(p['sx'], p['z0'], {1: p['H'][0], 3: p['H'][1], 2: p['H'][2]}, ts, 3)
    p10 = np.percentile(sq, 10, axis=0); vm = vf.mean(0); zero = (sq == 0).sum()
    return min(p10.min(), 26) * 2 + np.minimum(p10, 30).mean() * 0.5 + min(vm.min(), 0.85) * 40 + vm.mean() * 10 - zero * 3, p10, vm, zero
# start: three depth rows, stadium (tall T1 at the back, T3 mid, short T2 in front), columns interleaved
idx = {l: [i for i in range(M.N) if M.LOOKOF[i] == l] for l in (1, 3, 2)}
sx = [0.0] * M.N; z0 = [0.0] * M.N
for l, zr, off in ((1, 3.4, 0.5), (3, 2.45, 0.0), (2, 1.75, 0.5)):
    ids = idx[l][:]; rng.shuffle(ids); n = len(ids)
    for k, i in enumerate(ids): sx[i] = 20 + (200) * (k + off) / (n - 1 + (0.5 if off else 0)) if n > 1 else 120; z0[i] = zr + rng.uniform(-0.15, 0.15)
p = dict(sx=sx, z0=z0, H=[1.9, 1.35, 0.75]); best = score(p); print('start', round(best[0], 2), best[1].min(), best[2].min(), best[3], flush=True)
for it in range(iters):
    q = json.loads(json.dumps(p)); r = rng.random()
    if r < 0.45: i = rng.randrange(M.N); q['sx'][i] = min(225, max(15, q['sx'][i] + rng.gauss(0, 12)))
    elif r < 0.85: i = rng.randrange(M.N); q['z0'][i] = min(4.35, max(1.65, q['z0'][i] + rng.gauss(0, 0.25)))
    elif r < 0.93: i, j = rng.sample(range(M.N), 2); q['sx'][i], q['sx'][j] = q['sx'][j], q['sx'][i]
    else:
        k = rng.randrange(3); q['H'][k] += rng.gauss(0, 0.1)
        if not (q['H'][0] >= 1.2 * q['H'][1] and q['H'][1] >= 1.2 * q['H'][2] and q['H'][2] >= 0.45 and q['H'][0] <= 2.1): continue
    s = score(q)
    if s[0] > best[0]:
        p, best = q, s; print(it, round(s[0], 2), 'p10min', s[1].min(), 'vismin', round(s[2].min(), 3), 'zero', s[3], 'H', [round(h, 3) for h in p['H']], flush=True)
        json.dump(dict(p, score=best[0], p10=best[1].tolist(), vis=best[2].tolist(), zero=int(best[3]), seed=seed), open(outp, 'w'), indent=1)
print('done', best[0])
