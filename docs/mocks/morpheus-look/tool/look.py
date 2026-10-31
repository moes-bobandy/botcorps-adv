"""MORPHEUSbot 'Morpheus look' chip variants M1/M2/M3 (mock only). Edits only inside the chip box."""
import sys, os
sys.path.insert(0, '/workspace/out/morpheus-rename/tool'); from mlib import *
from PIL import Image
OUT = '/workspace/out/morpheus-look'
def L(p): return np.asarray(Image.open(p).convert('RGB')).copy()
def mix(c, d, t): return tuple(int(round(c[i] * (1 - t) + d[i] * t)) for i in range(3))

SKIN = dict(spec=(186, 132, 98), hi=(150, 98, 70), base=(106, 66, 46), mid=(80, 48, 32), sh=(58, 34, 23))
RIM, LENS, GLINT, BRIDGE = (74, 78, 86), (6, 6, 8), (252, 252, 252), (30, 20, 15)
SHIRT = (12, 15, 13)
LEATHER = dict(hi=(98, 34, 38), mid=(64, 20, 24), lo=(38, 10, 14), dark=(24, 6, 9), edge=(128, 50, 54))
GREEN, GLOW, HDRK = (52, 232, 112), (18, 70, 38), (12, 16, 14)
GUN, GUNO = (52, 56, 60), (86, 92, 98)

LENS_BIG = ["..RRRRR..", ".RKKKKKR.", "RKGGKKKKR", "RKGKKKKKR", "RKKKKKKKR", "RKKKKKKKR", "RKKKKKKKR", ".RKKKKKR.", "..RRRRR.."]
LENS_SMALL = [".RRR.", "RGKKR", "RKKKR", "RKKKR", ".RRR."]

GEO = {
 'boot': dict(chip=(76, 58, 161, 181), face=(96, 87, 143, 132), bezel=(90, 82, 148, 139),
              head=dict(cx=119.5, top=89, rx=15.0, ry=17.0), lens=LENS_BIG, lc=(112, 127), ltop=99, brow=97,
              mouth=(116, 115, 124), neck=(113, 126, 120), coat=dict(sy=125, sw=10, vt=6.0), ears=(103, 136, 103, 110),
              win=(81, 78, 158, 175), refs=dict(win=(120, 150), bev=[(84, 100), (90, 80), (82, 85)], edge=[(81, 100)], slant=[(155, 170)]),
              hdr=(78, 60, 161, 77), mrows=(63, 73), mcols=(82, 157), frame=[(78, 78, 80, 181), (159, 78, 161, 181), (78, 176, 161, 181)],
              lapel=(92, 140, 113, 153), big=True),
 'home': dict(chip=(31, 52, 74, 111), face=(42, 66, 64, 86), bezel=(40, 65, 66, 91),
              head=dict(cx=53.0, top=67, rx=7.5, ry=8.0), lens=LENS_SMALL, lc=(49, 57), ltop=72, brow=None,
              mouth=(80, 51, 55), neck=(50, 56, 82), coat=dict(sy=84, sw=4, vt=2.5), ears=(45, 61, 73, 76),
              win=(35, 61, 70, 108), refs=dict(win=(40, 107), bev=[(35, 70), (37, 70)], edge=[], slant=[(70, 105)]),
              hdr=None, mrows=(53, 58), mcols=(36, 69), frame=[], lapel=None, big=False),
}

def face_layer(g, lvl):
    d = {}; x0, y0, x1, y1 = g['face']; h = g['head']; cx = h['cx']; cy = h['top'] + h['ry']
    ex0, ex1, ey0, ey1 = g['ears']                     # ears behind head
    for ex in (ex0, ex1):
        for y in range(ey0, ey1 + 1):
            for x in (ex, ex + (1 if ex == ex0 else -1)) if g['big'] else (ex,):
                d[(x, y)] = SKIN['mid'] if ex == ex0 else SKIN['sh']
    c = g['coat']                                      # coat / shoulders + V-neck shirt
    for y in range(c['sy'] - 3, y1 + 1):
        for x in range(x0, x1 + 1):
            ax = abs(x + 0.5 - (cx + 0.5)) if False else abs(x - cx)
            top = c['sy'] + max(0, int((ax - c['sw']) * 0.5))
            if y < top or (g['big'] and False): continue
            vw = c['vt'] - (y - c['sy']) * 0.55
            d[(x, y)] = SHIRT if ax < vw else (LEATHER['dark'] if lvl >= 2 else (22, 22, 24))
    if lvl >= 2:                                       # lapel highlight: coat px next to the shirt V; shoulder top edge
        for (x, y), col in list(d.items()):
            if col == LEATHER['dark']:
                if d.get((x - 1, y)) == SHIRT or d.get((x + 1, y)) == SHIRT: d[(x, y)] = LEATHER['edge']
                elif (x, y - 1) not in d: d[(x, y)] = LEATHER['mid']
    nx0, nx1, ny = g['neck']                           # neck
    for y in range(ny, c['sy'] + 1):
        for x in range(nx0, nx1 + 1):
            if d.get((x, y)) in (None, SHIRT) or y < c['sy']: d[(x, y)] = SKIN['sh'] if y == ny else SKIN['mid']
    for y in range(y0, y1 + 1):                        # head (bald, lit from top-left)
        for x in range(x0, x1 + 1):
            u = (x - cx) / h['rx']; v = (y + 0.5 - cy) / h['ry']
            if u * u + v * v > 1.0: continue
            light = -0.6 * u - 0.8 * v; rr = u * u + v * v
            col = SKIN['base']
            if light > 0.62: col = SKIN['hi']
            if rr > 0.72 and u > 0.25: col = SKIN['sh']
            elif light < -0.45: col = SKIN['mid']
            if g['big'] and -0.5 <= u <= -0.15 and -0.92 <= v <= -0.72: col = SKIN['spec']
            d[(x, y)] = col
    if not g['big']: d[(int(cx) - 3, h['top'] + 1)] = SKIN['spec']
    if g['brow']:
        for lcx in g['lc']:
            for x in range(lcx - 3, lcx + 4): d[(x, g['brow'])] = SKIN['mid']
    my, mx0, mx1 = g['mouth']
    for x in range(mx0, mx1 + 1): d[(x, my)] = (50, 27, 19)
    if g['big']:
        for y in (107, 108, 109): d[(119, y)] = SKIN['hi']; d[(120, y)] = SKIN['hi']
        d[(118, 111)] = SKIN['sh']; d[(121, 111)] = SKIN['sh']
        for x in (119, 120): d[(x, 112)] = SKIN['mid']
    else:
        d[(53, 77)] = SKIN['mid']
    lens = g['lens']; n = len(lens)                    # solid black pince-nez
    for lcx in g['lc']:
        for r, row in enumerate(lens):
            for k, ch in enumerate(row):
                if ch != '.': d[(lcx - n // 2 + k, g['ltop'] + r)] = {'R': RIM, 'K': LENS, 'G': GLINT}[ch]
    by = g['ltop'] + (2 if g['big'] else 1)
    for x in range(g['lc'][0] + n // 2 + 1, g['lc'][1] - n // 2): d[(x, by)] = BRIDGE
    return {k: v for k, v in d.items() if x0 <= k[0] <= x1 and y0 <= k[1] <= y1}

def near(p, refs, thr=48):
    best = None
    for name, cols in refs.items():
        for c in cols:
            dd = float(np.sqrt(((p - c) ** 2).sum()))
            if dd < thr and (best is None or dd < best[0]): best = (dd, name)
    return best[1] if best else None

def body_layer(s, g, lvl):
    d = {}
    if lvl < 2: return d
    R = {k: [s[y, x] for (x, y) in (v if isinstance(v, list) else [v])] for k, v in g['refs'].items()}
    R = {k: v for k, v in R.items() if v}
    bx0, by0, bx1, by1 = g['bezel']; wx0, wy0, wx1, wy1 = g['win']
    for y in range(wy0, wy1 + 1):
        for x in range(wx0, wx1 + 1):
            if bx0 <= x <= bx1 and by0 <= y <= by1: continue
            k = near(s[y, x], R)
            if k == 'win':
                t = ((x - wx0) + (y - wy0) * 0.6) / ((wx1 - wx0) + (wy1 - wy0) * 0.6)
                d[(x, y)] = LEATHER['hi'] if t < 0.2 else LEATHER['mid'] if t < 0.72 else LEATHER['lo']
            elif k == 'bev': d[(x, y)] = LEATHER['dark']
            elif k == 'edge': d[(x, y)] = (16, 4, 6)
            elif k == 'slant': d[(x, y)] = LEATHER['edge']
    if lvl >= 3 and g['lapel']:
        lx0, ly0, lx1, ly1 = g['lapel']; mid2 = g['face'][0] + g['face'][2]
        for y in range(ly0, ly1 + 1):
            x = lx0 + round((y - ly0) * (lx1 - lx0) / (ly1 - ly0))
            for (px, c2) in ((x, LEATHER['edge']), (x + 1, LEATHER['dark']), (mid2 - x, LEATHER['edge']), (mid2 - x - 1, LEATHER['dark'])):
                if (px, y) in d and d[(px, y)] in (LEATHER['hi'], LEATHER['mid'], LEATHER['lo']): d[(px, y)] = c2
    if lvl >= 3:
        if g['hdr']:                                   # header -> black with Matrix-green meander + glow
            hx0, hy0, hx1, hy1 = g['hdr']; m = set()
            for y in range(hy0, hy1 + 1):
                for x in range(hx0, hx1 + 1):
                    p = s[y, x]
                    if g['mrows'][0] <= y <= g['mrows'][1] and g['mcols'][0] <= x <= g['mcols'][1] and p[2] > p[0] + 50: m.add((x, y))
            for y in range(hy0, hy1 + 1):
                for x in range(hx0, hx1 + 1):
                    if (x, y) in m: d[(x, y)] = GREEN
                    elif s[y, x].sum() > 520:
                        d[(x, y)] = GLOW if any((x + a, y + b) in m for a in (-1, 0, 1) for b in (-1, 0, 1)) else HDRK
            for (fx0, fy0, fx1, fy1) in g['frame']:
                for y in range(fy0, fy1 + 1):
                    for x in range(fx0, fx1 + 1):
                        if s[y, x].sum() > 520 and abs(int(s[y, x][2]) - int(s[y, x][0])) < 40: d[(x, y)] = HDRK
        else:
            for y in range(g['mrows'][0], g['mrows'][1] + 1):
                for x in range(g['mcols'][0], g['mcols'][1] + 1):
                    if s[y, x].sum() > 250: d[(x, y)] = GREEN
        bx0, by0, bx1, by1 = g['bezel']; fx0, fy0, fx1, fy1 = g['face']   # gunmetal bezel, green inner glow
        for y in range(by0, by1 + 1):
            for x in range(bx0, bx1 + 1):
                if fx0 <= x <= fx1 and fy0 <= y <= fy1: continue
                if s[y, x].sum() < 600: continue
                inner = fx0 - 1 <= x <= fx1 + 1 and fy0 - 1 <= y <= fy1 + 1
                outer = x in (bx0, bx1) or y in (by0, by1)
                d[(x, y)] = GREEN if inner else GUNO if outer else GUN
    return d

def morph(a, g, lvl):
    s = s1(a).astype(int); lay = body_layer(s, g, lvl)
    fx0, fy0, fx1, fy1 = g['face']; bg = tuple(int(v) for v in s[fy0 + 1, fx0 + 1])   # clear screen (old glasses)
    for y in range(fy0, fy1 + 1):
        for x in range(fx0, fx1 + 1): lay[(x, y)] = bg
    lay.update(face_layer(g, lvl))
    x0, y0, x1, y1 = g['chip']
    for (x, y), c in lay.items():
        assert x0 <= x <= x1 and y0 <= y <= y1
        put(a, x, y, c)
    return a
