"""MORPHEUSbot simple crew-style chip face + round Morpheus shades (MOCK ONLY).
Edits only inside the chip box. Pins + header wave take the active UI theme color."""
import sys, os
sys.path.insert(0, '/workspace/out/morpheus-rename/tool'); from mlib import *
from PIL import Image
def L(p): return np.asarray(Image.open(p).convert('RGB')).copy()

BRONZE = dict(hi=(214, 160, 104), base=(170, 118, 70), dark=(122, 80, 44), top=(240, 204, 156), bot=(96, 62, 36), drop=(26, 18, 12))
BLACK = dict(hi=(52, 55, 62), base=(30, 32, 36), dark=(16, 17, 20), top=(96, 100, 110), bot=(8, 8, 10), drop=(0, 0, 0))
BLACK_FRAME, BLACK_FRAME_SMALL, BLACK_LENS = (132, 138, 150), (70, 74, 84), (2, 2, 3)
FRAME_SMALL = (34, 36, 42)
POP_LIT, POP_SHADE, POP_BRIDGE = (128, 134, 146), (84, 88, 98), (108, 114, 124)
FRAME, LENS, GLINT = (64, 68, 76), (6, 6, 8), (252, 252, 252)
MFRAME, MGLINT = (36, 128, 70), (196, 255, 214)
GRAPH = dict(win=(74, 78, 76), bev1=(54, 56, 62), bev2=(58, 62, 62), slant=(118, 122, 120))
THEMES = {'purple': (180, 34, 255), 'green': (146, 255, 0), 'blue': (48, 96, 255), 'orange': (255, 106, 0)}

LENS_BIG = ["...FFFFF...", ".FFKKKKKFF.", ".FKGGKKKKF.", "FKGGKKKKKKF", "FKGKKKKKKKF", "FKKKKKKKKKF",
            "FKKKKKKKKKF", "FKKKKKKKKKF", ".FKKKKKKKF.", ".FFKKKKKFF.", "...FFFFF..."]
LENS_SMALL = [".FFF.", "FGKKF", "FKKKF", "FKKKF", ".FFF."]
GEO = {
 'boot': dict(chip=(76, 58, 163, 183), screen=(96, 87, 143, 132), panel=(98, 89, 141, 130), lens=LENS_BIG, lc=(112, 127), ltop=98,
              bridge=1, win=(81, 78, 158, 175), bezel=(90, 82, 148, 139),
              refs=dict(win=[(120, 150)], bev1=[(84, 100)], bev2=[(86, 100), (88, 100)], slant=[(155, 170)])),
 'home': dict(chip=(31, 52, 74, 111), screen=(42, 66, 64, 86), panel=(43, 67, 63, 85), lens=LENS_SMALL, lc=(49, 57), ltop=71,
              bridge=1, win=(35, 61, 70, 108), bezel=(40, 65, 66, 91),
              refs=dict(win=[(40, 107)], bev1=[(35, 70)], bev2=[(37, 70)], slant=[(70, 105)])),
}

def face(g, lvl, panel='bronze', pop=False):
    P = BLACK if panel == 'black' else BRONZE
    d = {}; sx0, sy0, sx1, sy1 = g['screen']; x0, y0, x1, y1 = g['panel']; W = x1 - x0
    for y in range(sy0, sy1 + 1):
        for x in range(sx0, sx1 + 1): d[(x, y)] = (0, 0, 0)
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            t = ((x - x0) + (y - y0)) / W
            c = P['hi'] if t < 0.45 or 0.85 <= t < 1.02 else P['dark'] if t >= 1.42 else P['base']
            if y == y0 or x == x0: c = P['top']
            if y == y1 or x == x1: c = P['bot']
            d[(x, y)] = c
    for y in range(y0 + 1, y1 + 2):
        if y <= sy1 and x1 + 1 <= sx1: d[(x1 + 1, y)] = P['drop']
    for x in range(x0 + 1, x1 + 2):
        if y1 + 1 <= sy1 and x <= sx1: d[(x, y1 + 1)] = P['drop']
    n = len(g['lens'])
    fr, gl = (MFRAME, MGLINT) if lvl >= 3 else (FRAME if n > 5 else FRAME_SMALL, GLINT)
    lens = LENS
    if panel == 'black': fr, lens = (BLACK_FRAME if n > 5 else BLACK_FRAME_SMALL), BLACK_LENS
    for lcx in g['lc']:
        for r, row in enumerate(g['lens']):
            for k, ch in enumerate(row):
                if ch != '.':
                    c = {'F': fr, 'K': lens, 'G': gl}[ch]
                    if pop and n <= 5 and ch == 'F': c = POP_LIT if (r + k) <= 3 else POP_SHADE   # shaded rim: lit top-left
                    d[(lcx - n // 2 + k, g['ltop'] + r)] = c
    by = g['ltop'] + g['bridge'] + (1 if n > 5 else 0)
    for x in range(g['lc'][0] + n // 2 + 1, g['lc'][1] - n // 2): d[(x, by)] = POP_BRIDGE if (pop and n <= 5) else fr
    return d

def themed(s, g, theme):
    """Pins, inner pads and the header square wave: fixed blue -> active theme color (keeps the blue's shading)."""
    d = {}; x0, y0, x1, y1 = g['chip']; T = np.array(THEMES[theme], float); ref = 173.0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            r, gg, b = s[y, x]
            if b > r + 50 and b > gg + 30 and b > 110:
                k = min(1.0, b / ref); d[(x, y)] = tuple(int(v) for v in np.clip(T * k, 0, 255))
    return d

def body(s, g, lvl):
    d = {}
    if lvl < 2: return d
    R = {k: [s[y, x] for (x, y) in v] for k, v in g['refs'].items()}
    bx0, by0, bx1, by1 = g['bezel']; wx0, wy0, wx1, wy1 = g['win']
    for y in range(wy0, wy1 + 1):
        for x in range(wx0, wx1 + 1):
            if bx0 <= x <= bx1 and by0 <= y <= by1: continue
            r_, g_, b_ = s[y, x]
            if b_ > r_ + 50 and b_ > g_ + 30: continue          # pins/pads stay theme-colored
            best = None
            for k, cols in R.items():
                for c in cols:
                    dd = float(np.sqrt(((s[y, x] - c) ** 2).sum()))
                    if dd < 40 and (best is None or dd < best[0]): best = (dd, k)
            if best: d[(x, y)] = GRAPH[best[1]]
    return d

def morph(a, gk, lvl, theme=None, panel='bronze', pop=False):
    g = GEO[gk]; s = s1(a).astype(int); lay = {}
    if theme: lay.update(themed(s, g, theme))
    lay.update(body(s, g, lvl)); lay.update(face(g, lvl, panel, pop))
    x0, y0, x1, y1 = g['chip']
    for (x, y), c in lay.items():
        assert x0 <= x <= x1 and y0 <= y <= y1
        put(a, x, y, c)
    return a
