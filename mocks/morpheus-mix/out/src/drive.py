#!/usr/bin/env python3
"""Run a sim_live step list (JSON) for the morpheus-mix tree. Extra steps: "tower NAME" taps the named pillar at a point that mo_hit
(nearest centre line, front first) gives to it, chosen from this frame's itwstate; "towers" taps < TOWERS. Usage: drive.py steps.json OUTDIR"""
import json, os, re, subprocess, sys
SIM = os.environ.get('SIM', '/tmp/mmix/sim'); TREE = '/workspace/out/morpheus-mix/tree'
NAMES = ["WIFI","BLUETOOTH","DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
def run(lines, out):
    os.makedirs(out, exist_ok=True); p = out + '/_script.txt'; open(p, 'w').write('\n'.join(lines) + '\n')
    r = subprocess.run([SIM, p, out], cwd=TREE, capture_output=True, text=True, timeout=900); return r.stdout + r.stderr
def state(txt):
    blk = txt.rsplit('MOCAM', 1)[1]; ordr = [int(x) for x in re.search(r'ord=([\d,]+)', blk).group(1).split(',')]; tw = {}
    for m in re.finditer(r'MOTW (\d+) .*?f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] b=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]', blk):
        i = int(m.group(1)); v = list(map(int, m.groups()[1:])); tw[i] = dict(x0=v[0], x1=v[1], yt=v[2], yb=v[3], bx0=v[4], bx1=v[5], byt=v[6], byb=v[7])
    return ordr, tw
def mo_hit(x, y, ordr, tw):
    if y < 35 or y > 285: return -1
    best, bd = -1, 1 << 60
    for i in reversed(ordr):
        T = tw[i]; hy0 = max(35, T['byt'] - 3); hy1 = min(285, T['yb'] + 3)
        dx2 = 2 * x - T['x0'] - T['x1']; dy = hy0 - y if y < hy0 else y - hy1 if y > hy1 else 0
        d = dx2 * dx2 + 4 * dy * dy
        if d < bd: bd, best = d, i
    return best
def pick(i, ordr, tw):
    """the point of i's cell farthest (Chebyshev, 1 px grid) from any other cell"""
    T = tw[i]; best = None
    for y in range(max(36, T['yt']), min(284, T['yb']) + 1, 2):
        for x in range(max(1, T['x0'] - 6), min(238, T['x1'] + 6) + 1):
            if mo_hit(x, y, ordr, tw) != i: continue
            r = 0
            while r < 12 and all(mo_hit(x + a, y + b, ordr, tw) == i for a, b in ((r+1,0),(-r-1,0),(0,r+1),(0,-r-1),(r+1,r+1),(-r-1,-r-1),(r+1,-r-1),(-r-1,r+1))): r += 1
            if best is None or r > best[0]: best = (r, x, y)
    return best
def expand(steps, out):
    done = []
    for s in steps:
        if s.startswith('tower '):
            nm = s[6:]; i = NAMES.index(nm); pre = [x for x in done if not x.startswith('rec')] + ['itwstate']
            txt = run(pre, out + '/_probe'); ordr, tw = state(txt); b = pick(i, ordr, tw)
            assert b, ('no tap point', nm); done.append('tap %d %d' % (b[1], b[2])); print('tower', nm, '-> tap', b[1], b[2], 'margin', b[0], file=sys.stderr)
        elif s == 'towers': done.append('tap 205 266')
        else: done.append(s)
    return done
if __name__ == '__main__':
    steps = json.load(open(sys.argv[1])); out = sys.argv[2]; lines = expand(steps, out); txt = run(lines, out)
    open(out + '/run.log', 'w').write(txt); print(out + '/run.log')
