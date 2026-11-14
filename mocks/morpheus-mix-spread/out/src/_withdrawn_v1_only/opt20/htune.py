#!/usr/bin/env python3
"""Steering 2: tune the slight style height factors (T1 h1 in [1.05,1.15], T2 h2 in [0.85,0.95], T3 1.00) for unobstructed idle emblems."""
import json, sys, numpy as np
sys.path.insert(0, __file__.rsplit('/',1)[0]); import model20 as M
J = json.load(open(__file__.rsplit('/',1)[0] + '/../layout.json'))
L = {k: list(M.OLD[k]) + J[k] for k in M.OLD}; ts = np.linspace(9.0, 9.0 + 0.3 * 319, 80); rows = []
for h1 in np.linspace(1.05, 1.15, 6):
    for h2 in np.linspace(0.85, 0.95, 6):
        sq, vis = M.evaluate(L, {1: h1, 2: h2, 3: 1.0}, ts, 4); vm = vis.mean(0)
        rows.append((vm.mean() + 0.5 * vm.min(), round(h1,3), round(h2,3), round(vm.min(),3), round(vm.mean(),3), float(np.percentile(sq,10,axis=0).min()), float(sq.min())))
rows.sort(reverse=True)
for r in rows[:6]: print(r)
sq, vis = M.evaluate(L, {1: 1, 2: 1, 3: 1.0}, ts, 4); print('no style heights: vis min/mean', round(vis.mean(0).min(),3), round(vis.mean(0).mean(),3))
b = rows[0]; J['h1'], J['h2'] = b[1], b[2]; json.dump(J, open(__file__.rsplit('/',1)[0] + '/../layout.json', 'w'), indent=1); print('chosen', b)
