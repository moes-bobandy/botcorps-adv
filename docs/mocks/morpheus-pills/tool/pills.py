"""MORPHEUSbot boot-entrance mocks A/B/C (mock only). Base = renamed boot frames (name + pince-nez).
Sequence: MORPHEUS spot powers on -> RED + BLUE chip bots side by side next to him -> RED selected
(red brightens, blue dims) -> boot continues (original ACCESS GRANTED + wipe to GIBSON)."""
import sys, os, subprocess
sys.path.insert(0, '/workspace/out/morpheus-rename/tool'); from mlib import *
from PIL import Image
BASE = '/workspace/out/morpheus-rename/boot_mock_frames/m%03d.png'
OUTR = '/workspace/out/morpheus-pills'
def load(n): return np.asarray(Image.open(BASE % n).convert('RGB')).copy()

GOLD, GOLDD = (214, 170, 72), (140, 104, 36)
FRAME, HDR, WHITE, BLACK = (18, 18, 22), (214, 218, 226), (236, 240, 250), (6, 8, 10)
THEME = {'red':  dict(hi=(255, 112, 112), mid=(206, 36, 48), lo=(118, 14, 24)),
         'blue': dict(hi=(120, 176, 255), mid=(40, 92, 214), lo=(18, 40, 122))}
def mix(c, d, t): return tuple(int(round(c[i] * (1 - t) + d[i] * t)) for i in range(3))
def scale(c, p): return tuple(min(255, int(c[i] * p)) for i in range(3))

def chipbot(x0, y0, W, H, theme, label, power=1.0, state='', pinw=2):
    """CYD chip bot at 1x: gold pins, dark frame, light header w/ meander, metallic tinted window,
    black face w/ white outline + two eyes, GLCD label. Returns {(x,y): rgb}. x0,y0 = body top-left."""
    T = THEME[theme]; d = {}
    for y in range(y0 + 2, y0 + H - 2):                       # side pins (2 on / 1 off)
        if (y - y0) % 3 != 2:
            for k in range(pinw):
                d[(x0 - 1 - k, y)] = GOLDD if k == pinw - 1 else GOLD
                d[(x0 + W + k, y)] = GOLDD if k == pinw - 1 else GOLD
    for x in range(x0 + 2, x0 + W - 2):                       # bottom pins
        if (x - x0) % 3 != 2: d[(x, y0 + H)] = GOLD; d[(x, y0 + H + 1)] = GOLDD
    for y in range(y0, y0 + H):
        for x in range(x0, x0 + W):
            i, j = x - x0, y - y0
            if i in (0, W - 1) or j in (0, H - 1, 4): c = FRAME
            elif j <= 3:                                       # header + meander
                c = HDR
                if j == 2 or (j in (1, 3) and ((i - 2) % 4 == 0 if j == 1 else (i - 4) % 4 == 0)):
                    c = T['mid'] if 2 <= i <= W - 3 else HDR
                if j == 2 and (i - 3) % 4 in (1, 2): c = HDR
            else:                                              # metallic window, diagonal 3-tone
                t = (i + (j - 5) * 1.6) / (W + (H - 6) * 1.6)
                c = T['hi'] if t < 0.22 else T['mid'] if t < 0.74 else T['lo']
                if j == 5 and 1 <= i <= W - 2: c = mix(T['hi'], WHITE, 0.35)   # specular edge
            d[(x, y)] = c
    fw, fh = 14, 10; fx, fy = x0 + (W - fw) // 2, y0 + 6     # face
    for y in range(fy, fy + fh):
        for x in range(fx, fx + fw):
            d[(x, y)] = WHITE if x in (fx, fx + fw - 1) or y in (fy, fy + fh - 1) else BLACK
    for ex in (fx + 3, fx + fw - 5):
        for dx in (0, 1):
            for dy in (3, 4): d[(ex + dx, fy + dy)] = T['hi'] if theme == 'red' else T['hi']
    for x in range(fx + 5, fx + fw - 5): d[(x, fy + 7)] = mix(T['hi'], BLACK, 0.3)
    w = len(label) * 6 - 1; ly = fy + fh + 2
    for (x, y) in glcd_pts(label, x0 + (W - w) // 2, ly):
        d[(x + 1, y + 1)] = mix(T['lo'], BLACK, 0.5); d[(x, y)] = WHITE
    for k in d:                                                # state
        c = d[k]
        if state == 'sel': c = scale(c, 1.22)
        if state == 'dim': c = mix(c, (46, 48, 54), 0.68)
        d[k] = scale(c, power)
    if state == 'sel' and power >= 1:                          # halo
        for r, t in ((pinw + 1, 0.0), (pinw + 2, 0.55)):
            for x in range(x0 - r, x0 + W + r):
                for y in (y0 - r + 1, y0 + H + 1 + r):
                    d.setdefault((x, y), mix(T['hi'], BLACK, t))
            for y in range(y0 - r + 1, y0 + H + 2 + r):
                for x in (x0 - r, x0 + W - 1 + r):
                    d.setdefault((x, y), mix(T['hi'], BLACK, t))
    return d

CURSOR = ["K       ", "KK      ", "KWK     ", "KWWK    ", "KWWWK   ", "KWWWWK  ", "KWWWWWK ", "KWWKKKK ", "KWK     ", "KK      "]
def cursor(tx, ty):
    return {(tx + c, ty + r): (WHITE if ch == 'W' else BLACK) for r, row in enumerate(CURSOR) for c, ch in enumerate(row) if ch != ' '}
def ring(cx, cy, r, col):
    out = {}
    for k in range(64):
        a = 2 * np.pi * k / 64
        out[(int(round(cx + r * np.cos(a))), int(round(cy + r * np.sin(a))))] = col
    return out

def apply(a, layer):
    for (x, y), c in layer.items():
        if 0 <= x < 240 and 0 <= y < 320: put(a, x, y, c)
    return a

SPOT = (74, 52, 166, 181)        # MORPHEUS chip spot (1x box) for the power-on
def spot(a, f):
    x0, y0, x1, y1 = SPOT
    r = a[3 * y0:3 * y1, 3 * x0:3 * x1].astype(float) * f
    a[3 * y0:3 * y1, 3 * x0:3 * x1] = np.clip(r, 0, 255).astype(np.uint8); return a
POWER = [0.10] * 8 + [0.55, 0.10, 0.85, 0.30, 1.12, 1.05]

PLATE_IN = (46, 186, 193, 219); PLATE_BG = (24, 24, 24)
def plate_clear():
    x0, y0, x1, y1 = PLATE_IN
    return {(x, y): PLATE_BG for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}
TRACE = (150, 64, 210)
def plate_bus(col=TRACE):
    d = {}
    for x in range(78, 162): d[(x, 187)] = col
    for y in (188, 189): d[(78, y)] = col; d[(161, y)] = col
    for y in range(180, 187): d[(119, y)] = col          # stub from MORPHEUS' bottom edge
    return d
PATH_RED = [(119, y) for y in range(178, 188)] + [(x, 187) for x in range(118, 77, -1)] + [(78, 188), (78, 189)]

def layout(kind, pr, pb, sr='', sb=''):
    if kind == 'plate':
        L = plate_clear(); L.update(plate_bus())
        L.update(chipbot(56, 190, 44, 27, 'red', 'ENTER', pr, sr))
        L.update(chipbot(139, 190, 44, 27, 'blue', 'TBD', pb, sb))
    else:   # flank: red left of MORPHEUS, blue right of him
        L = chipbot(42, 60, 33, 37, 'red', 'ENTER', pr, sr, pinw=1)
        L.update(chipbot(166, 60, 33, 37, 'blue', 'TBD', pb, sb, pinw=1))
    return L
RAMP = [0.1, 0.1, 0.5, 0.15, 0.8, 0.4, 1.0, 1.0]

def build(idea):
    out = []                                          # list of (array, tag)
    for f in POWER: out.append((spot(load(0), f), 'power'))
    for n in range(0, 50): out.append((load(n), 'orig'))
    kind = 'flank' if idea == 'A' else 'plate'
    tgt = (58, 82) if kind == 'flank' else (78, 205)  # red chip centre (1x)
    for k in range(60):
        a = load(49); p = RAMP[k] if k < len(RAMP) else 1.0
        if idea == 'C':
            if k < 30: L = layout(kind, p, p)
            elif k < 42:
                L = layout(kind, 1, 1); i = int((k - 30) / 11 * (len(PATH_RED) - 1))
                for q in PATH_RED[:i]: L[q] = (236, 52, 60)                 # trail lights red
                hx, hy = PATH_RED[i]
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1): L[(hx + dx, hy + dy)] = (255, 236, 200) if dx == dy == 0 else (255, 120, 110)
            else:
                L = layout(kind, 1, 1, 'sel', 'dim')
                for q in PATH_RED: L[q] = (236, 52, 60)
        else:
            sel = k >= 44
            L = layout(kind, p, p, 'sel' if sel else '', 'dim' if sel else '')
            if 28 <= k < 50:
                t = min(1.0, (k - 28) / 12); cx = int(220 + (tgt[0] - 220) * t); cy = int(300 + (tgt[1] - 300) * t)
                if k >= 42: L.update(ring(tgt[0], tgt[1], 3 + (k - 42) * 2, WHITE) if k < 47 else {})
                L.update(cursor(cx, cy))
        out.append((apply(a, L), 'choice'))
    for n in range(50, 85):
        a = load(n)
        if kind == 'flank' and n < 74: a = apply(a, layout(kind, 1, 1, 'sel', 'dim'))
        out.append((a, 'orig'))
    return out

if __name__ == '__main__':
    for idea in sys.argv[1:] or ['A', 'B', 'C']:
        d = '%s/idea%s/frames' % (OUTR, idea); os.makedirs(d, exist_ok=True)
        for f in os.listdir(d): os.remove(os.path.join(d, f))
        seq = build(idea)
        for i, (a, tag) in enumerate(seq): Image.fromarray(a).save('%s/f%04d.png' % (d, i))
        print(idea, len(seq), 'frames; choice starts at', [t for _, t in seq].index('choice'))
