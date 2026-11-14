#!/usr/bin/env python3
# v5b (12:42 PM ET): + optional hard-ish emblem visibility floor (VMW weight below VMIN). v5 (Mobandy 11:52 AM ET): fill the floor - same as opt18d.py + a penalty on the largest empty floor square (floor18.py), per-look depth bounds
# (white T2 may come as close as z0 0.95, wood T3 1.45, green-group T1 2.1; the top-clip rule still holds), MO_TW=1.35
"""Spread search v2 (simulated annealing). Start: a stadium in three depth tiers (tall T1 code glass at the back, T3 lanterns mid,
short T2 white lights in front) so each tier owns its own band of the tap field; columns spread across the 240 px room.
Moves: one pillar's sx / z0, swap two pillars of a tier, or one height (T1 >= 1.2 T3 >= 1.44 T2, T2 >= 0.28).
Score: worst-pillar tap p10 (cap 28) first, then worst emblem visible fraction, means; zero-cell samples penalised.
Usage: opt18b.py SEED ITERS OUT.json"""
import json, sys, random, math, os, numpy as np
sys.path.insert(0, __file__.rsplit('/', 1)[0]); import model18 as M, floor18 as FL
seed = int(sys.argv[1]); iters = int(sys.argv[2]); outp = sys.argv[3]; rng = random.Random(seed)
HFIX = [float(x) for x in sys.argv[4].split(",")] if len(sys.argv) > 4 else None   # fixed [T1, T3, T2] (v3: 1.00 / 0.90 / 0.80 x base)
ts = np.linspace(9.0, 9.0 + 0.3 * 319, int(os.environ.get('NTS', '96'))); STEP = 2
def topok(p):
    return all(p['z0'][i] * (1 - M.AZ[i] / 3) >= M.F * ({1: p['H'][0], 3: p['H'][1], 2: p['H'][2]}[M.LOOKOF[i]] * M.HK[i] - M.CAMH) / (M.HOR - 38) for i in range(M.N))
def score(p):
    if not topok(p): return (-1e9, np.zeros(M.N), np.zeros(M.N), 0, 999)
    sq, vf = M.evaluate(p['sx'], p['z0'], {1: p['H'][0], 3: p['H'][1], 2: p['H'][2]}, ts, STEP)
    p10 = np.percentile(sq, 10, axis=0); vm = vf.mean(0); zero = (sq == 0).sum()
    _pl = [M.place(p['sx'], p['z0'], {1: p['H'][0], 3: p['H'][1], 2: p['H'][2]}, t) for t in ts[::4]]
    emp = np.mean([FL.empty_square(FL.boxes_model(tw)) for tw in _pl[::2]])
    bal = 0.0   # v5b (12:42 PM ET): bases spread over the screen thirds (5..7 each) with >= 1 front-row base (y >= 200) per third, at every sampled time
    for tw in _pl:
        cx = [((T['x0'] + T['x1']) / 2, T['yb']) for T in tw]
        th = [sum(1 for x, y in cx if lo <= x < hi) for lo, hi in ((-999, 80), (80, 160), (160, 999))]
        fr = [sum(1 for x, y in cx if lo <= x < hi and y >= 200) for lo, hi in ((-999, 80), (80, 160), (160, 999))]
        bal += sum(max(5 - n, 0) + max(n - 7, 0) for n in th) + sum(1 for n in fr if n == 0)
    emp += float(os.environ.get('BALW', '0')) * bal / len(_pl)
    off = sum(max(0, -T['x0']) + max(0, T['x1'] - 239) for tw in _pl for T in tw) / len(_pl)   # v5d: px of pillar faces off the 240 px screen (drift at the screen edges hid HACK / AXON emblems)
    emp += float(os.environ.get('OFFW', '0')) * off
    return min(p10.min(), 28) * 3 + np.minimum(p10, 34).mean() * 0.5 + min(vm.min(), 0.9) * 40 + vm.mean() * 10 - zero * 25 - float(os.environ.get('FLW', '0.6')) * max(emp - 40, 0) - float(os.environ.get('VMW', '0')) * max(float(os.environ.get('VMIN', '0.5')) - vm.min(), 0), p10, vm, zero, emp
TIER = {1: (4.15, 0.15), 3: (2.7, 0.3), 2: (1.72, 0.06)} if not HFIX else {1: (rng.uniform(1.8, 3.8), 1.0), 3: (rng.uniform(1.8, 3.8), 1.0), 2: (rng.uniform(1.8, 3.8), 1.0)}
sx = [0.0] * M.N; z0 = [0.0] * M.N
for l in (1, 3, 2):
    ids = [i for i in range(M.N) if M.LOOKOF[i] == l]; rng.shuffle(ids); n = len(ids); off = rng.uniform(-6, 6)
    for k, i in enumerate(ids): sx[i] = 30 + 180 * (k + 0.5) / n + off; z0[i] = TIER[l][0] + rng.uniform(-TIER[l][1], TIER[l][1])
import os
if os.environ.get('INIT'): _I = json.load(open(os.environ['INIT'])); sx = [x + rng.gauss(0, 4) for x in _I['sx']]; z0 = list(_I['z0']) if os.environ.get('ZFIX') else [min(4.35, max(1.65, z + rng.gauss(0, 0.08))) for z in _I['z0']]
for _i in range(M.N if not os.environ.get('ZFIX') else 0):
    while M.LOOKOF[_i] == 1 and z0[_i] * (1 - M.AZ[_i] / 3) < M.F * (HFIX[0] - M.CAMH) / (M.HOR - 38): z0[_i] += 0.05
p = dict(sx=sx, z0=z0, H=HFIX if HFIX else [rng.uniform(2.1, 2.4), rng.uniform(1.0, 1.25), rng.uniform(0.32, 0.45)]); cur = score(p); best = (cur, p)
print('start', round(cur[0], 2), cur[1].min(), round(cur[2].min(), 3), cur[3], round(cur[4], 1), flush=True)
for it in range(iters):
    T = 0.25 * (1 - it / iters) + 0.01
    q = json.loads(json.dumps(p)); r = rng.random()
    if r < 0.45: i = rng.randrange(M.N); q['sx'][i] = min(float(os.environ.get('SXMAX', '215')), max(float(os.environ.get('SXMIN', '25')), q['sx'][i] + rng.gauss(0, 6)))   # v5b: SXMIN/SXMAX env (8..232 lets towers stand at the screen edges)
    elif r < 0.8 and os.environ.get('ZFIX'): continue   # v5c: depth fixed by the Gibson mapping of the example rssi
    elif r < 0.8: i = rng.randrange(M.N); q['z0'][i] = min(4.35, max({1: 2.1, 3: 1.45, 2: 0.95}[M.LOOKOF[i]], q['z0'][i] + rng.gauss(0, 0.2)))
    elif r < 0.9:
        l = rng.choice((1, 3, 2)); ids = [i for i in range(M.N) if M.LOOKOF[i] == l]; i, j = rng.sample(ids, 2); q['sx'][i], q['sx'][j] = q['sx'][j], q['sx'][i]
    elif HFIX: continue
    else:
        k = rng.randrange(3); q['H'][k] += rng.gauss(0, 0.06)
        if not (q['H'][0] >= 1.2 * q['H'][1] and q['H'][1] >= 1.2 * q['H'][2] and q['H'][2] >= 0.28 and q['H'][0] <= 2.6): continue
    s = score(q)
    if s[0] >= cur[0] or rng.random() < math.exp((s[0] - cur[0]) / T):
        p, cur = q, s
        if cur[0] > best[0][0]:
            best = (cur, json.loads(json.dumps(p))); b = best[0]
            print(it, round(b[0], 2), 'p10min', b[1].min(), 'vismin', round(b[2].min(), 3), 'zero', b[3], 'empty', round(b[4], 1), 'H', [round(h, 3) for h in p['H']], flush=True)
            json.dump(dict(best[1], score=b[0], p10=b[1].tolist(), vis=b[2].tolist(), zero=int(b[3]), seed=seed), open(outp, 'w'), indent=1)
print('done', best[0][0])
