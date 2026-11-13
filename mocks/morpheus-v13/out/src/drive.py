#!/usr/bin/env python3
"""Deterministic sim_live script builder for the MORPHEUSbot v7 mock. Steps: plain sim lines, or 'tower NAME' -> tap the centre of the
largest square of NAME's own tap cell at that sim instant (v11 mo_hit rule); 'towers' -> < TOWERS."""
import re, subprocess, sys, os, json
SIM = os.environ.get('SIM', '/tmp/morpheus_v13/sim_new'); TREE = '/workspace/cyd-morpheus-v6-wt'
def run(lines, outdir):
    os.makedirs(outdir, exist_ok=True); p = os.path.join(outdir, 'script.txt'); open(p, 'w').write('\n'.join(lines) + '\n')
    return subprocess.run([SIM, p, outdir], cwd=TREE, capture_output=True, text=True, check=True).stdout
def parse(blk):
    T = {}
    for m in re.finditer(r'T(\d+) (.{12}) icon=-?\d+ seen=(\d) hit=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] face=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]', blk):
        g = list(map(int, m.groups()[3:])); T[int(m.group(1))] = dict(name=m.group(2).strip(), seen=m.group(3) == '1', box=tuple(g[:4]), fc=g[4] + g[6])
    ord_ = list(map(int, re.search(r'ord=([\d,]+)', blk).group(1).split(',')))
    return T, ord_
def owner_map(T, ord_):   # v11 mo_hit: nearest pillar centre line (front-face centre column, box top..bottom), front-first ties, field rows 35..285
    import numpy as np
    yy, xx = np.mgrid[0:320, 0:240]; M = np.full((320, 240), -1, int); best = np.full((320, 240), 1e18)
    for i in reversed(ord_):
        if not T[i]['seen']: continue
        x0, y0, x1, y1 = T[i]['box']; dx2 = (2 * xx - T[i]['fc']).astype(float); dy = np.maximum(np.maximum(y0 - yy, yy - y1), 0)
        d = dx2 * dx2 + 4.0 * dy * dy; sel = (d < best) & (yy >= 35) & (yy <= 285); M[sel] = i; best = np.where(sel, d, best)
    return M
def max_square(M, i):
    import numpy as np
    A = (M == i).astype(int); S = np.zeros_like(A); best = (0, 0, 0)
    for y in range(A.shape[0]):
        for x in range(A.shape[1]):
            if A[y, x]: S[y, x] = 1 + (min(S[y - 1, x], S[y, x - 1], S[y - 1, x - 1]) if x and y else 0)
            if S[y, x] > best[0]: best = (S[y, x], x, y)
    return best   # side, bottom-right x, y
def build(steps, outdir):
    lines = []
    for s in steps:
        m = re.match(r'tower (.+)$', s)
        if m:
            for _w in range(20):   # v12: if the pillar is hidden behind a passing one right now (cell < 20 px), wait 90 ms and look again
                out = run(lines + ['itwstate'], outdir + '/_probe'); blk = out[out.rfind('ITW scr='):]
                T, o = parse(blk); i = [k for k, v in T.items() if v['name'] == m.group(1)][0]
                side, bx, by = max_square(owner_map(T, o), i); x, y = bx - side // 2, by - side // 2
                if side >= 20: break
                lines.append('tick 3')
            lines += ['tap %d %d' % (x, y), 'itwstate']
        elif s == 'towers': lines += ['tap 204 266', 'itwstate']
        else: lines.append(s)
    return lines
if __name__ == '__main__':
    steps = json.load(open(sys.argv[1])); outdir = sys.argv[2]
    lines = build(steps, outdir); out = run(lines, outdir); open(os.path.join(outdir, 'out.txt'), 'w').write(out)
    open(os.path.join(outdir, 'resolved_script.txt'), 'w').write('\n'.join(lines) + '\n')
    print('\n'.join(l for l in out.splitlines() if ' tap ' in l or l.startswith('DUMP') or 'MOCAM' in l))
