#!/usr/bin/env python3
"""mock/v1220 hit-list gate: base vs new host-sim dumps (240x320 RGB, last frame of NAME.rgb).
usage: v1220_gate.py BASEDIR NEWDIR 'name:x0,y0,x1,y1[;x0,y0,x1,y1]' ...   (rects inclusive = allowed change area;
name with no rects = must be 0 px). Prints inside/outside/total + change bbox; exit = number of FAILs."""
import sys, os
import numpy as np
B, N = sys.argv[1], sys.argv[2]
fails = 0
print(f"{'screen':16s} {'inside':>7s} {'outside':>8s} {'total':>7s}  result  change bbox")
for spec in sys.argv[3:]:
    name, _, rs = spec.partition(':')
    def ld(d):
        b = np.fromfile(os.path.join(d, name + '.rgb'), np.uint8)
        return b[-240*320*3:].reshape(320, 240, 3)
    a, n = ld(B), ld(N)
    diff = (a != n).any(axis=2)
    m = np.zeros_like(diff)
    for r in filter(None, rs.split(';')):
        x0, y0, x1, y1 = map(int, r.split(','))
        m[y0:y1+1, x0:x1+1] = True
    ins, outs = int((diff & m).sum()), int((diff & ~m).sum())
    ok = outs == 0
    fails += not ok
    ys, xs = np.nonzero(diff)
    bb = f"({xs.min()},{ys.min()})-({xs.max()},{ys.max()})" if len(xs) else ''
    print(f"{name:16s} {ins:7d} {outs:8d} {ins+outs:7d}  {'PASS' if ok else 'FAIL'}    {bb}  allowed={rs or 'none'}")
sys.exit(fails)
