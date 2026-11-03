"""VARIANT (look-only, NOT locked): big MORPHEUSbot at top centre in the GIBSON slot of the home board, S2c look
(dark grey body, black shaded face, round solid black shades with lit rim + glint, theme pins/wave), with the 12
device chips re-fitted around and under him. Uses the S2c palette/lens art from morpheus-simple/tool/simple.py."""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, '/workspace/out/morpheus-simple/tool')
import numpy as np
from PIL import Image
import glossary as G
from simple import BLACK, BLACK_FRAME, BLACK_LENS, GLINT, GRAPH, LENS_BIG, LENS_SMALL
GB = (74, 30, 164, 152)                      # GIBSON chip box on the home frame (1x, incl. pins)
SCREEN = (91, 52, 148, 109); PANEL = (96, 56, 143, 103)
LAB = (83, 122, 157, 143)                    # GIBSONbot / JACK IN label band
COLS = [29, 75, 121, 167]
SIDE = [(29, 28), (29, 91), (167, 28), (167, 91)]                 # flanking chips (2 each side)
LOW = [(x, y) for y in (158, 228) for x in COLS]                   # 2 rows of 4 under him
SLOTS = SIDE[:2] + SIDE[2:] + LOW

def morpheus_chip(a, f, chip, screen, panel, lens, lcs, ltop, label=None):
    """THE MORPHEUSbot chip look (Mobandy 2026-11-02 3:24 PM ET: keep it exactly like this, every size):
    dark grey body, theme pins/wave kept, black screen window, black shaded face square with black border,
    round solid black shades (lit rim top-left, shaded rim bottom-right, glint) joined by a bridge."""
    x0, y0, x1, y1 = chip
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            p = f[y, x]
            if G.is_board(p): continue
            r, g, b = (int(v) for v in p)
            grey = abs(r - g) < 30 and abs(g - b) < 30
            if grey and r >= 220: a[y, x] = p                        # keep light label ink
            elif grey and r > 150: a[y, x] = GRAPH['slant']
            elif grey and r > 85: a[y, x] = GRAPH['win']
            else: a[y, x] = p
    if label:
        (lx0, ly0, lx1, ly1), text, ty = label
        a[ly0:ly1 + 1, lx0:lx1 + 1] = GRAPH['win']
        w = len(text) * 6 - 1; G.text(a, text, (lx0 + lx1 + 1) // 2 - (w + 1) // 2, ty, G.LABEL_INK)
    sx0, sy0, sx1, sy1 = screen; a[sy0:sy1 + 1, sx0:sx1 + 1] = 0
    px0, py0, px1, py1 = panel; W = px1 - px0
    for y in range(py0, py1 + 1):
        for x in range(px0, px1 + 1):
            t = ((x - px0) + (y - py0)) / W
            c = BLACK['hi'] if t < 0.45 or 0.85 <= t < 1.02 else BLACK['dark'] if t >= 1.42 else BLACK['base']
            if y == py0 or x == px0: c = BLACK['top']
            if y == py1 or x == px1: c = BLACK['bot']
            a[y, x] = c
    a[py0 + 1:py1 + 2, px1 + 1] = BLACK['drop']; a[py1 + 1, px0 + 1:px1 + 2] = BLACK['drop']
    n = len(lens); h = n // 2
    for lcx in lcs:
        for r, row in enumerate(lens):
            for k, ch in enumerate(row):
                if ch == '.': continue
                c = {'F': BLACK_FRAME, 'K': BLACK_LENS, 'G': GLINT}[ch]
                if ch == 'F' and (r + k) > n - 2: c = (84, 88, 98)
                a[ltop + r, lcx - h + k] = c
    for x in range(lcs[0] + h + 1, lcs[1] - h): a[ltop + max(1, n // 5), x] = BLACK_FRAME
    return (x0, y0, x1 - x0 + 1, y1 - y0 + 1)

SIZES = {   # same look, three sizes
 'glossary_big': dict(chip=GB, screen=SCREEN, panel=PANEL, lens=LENS_BIG, lcs=(110, 130), ltop=68, label=(LAB, 'MORPHEUS', 129)),
 'home': dict(chip=(31, 52, 74, 111), screen=(42, 66, 64, 86), panel=(43, 67, 63, 85), lens=LENS_SMALL, lcs=(49, 57), ltop=72),
 'boot': dict(chip=(76, 58, 163, 183), screen=(96, 87, 143, 132), panel=(98, 89, 141, 130), lens=LENS_BIG, lcs=(110, 130), ltop=100),
}

def big_morpheus(a, f): return morpheus_chip(a, f, **SIZES['glossary_big'])

def render(theme):
    fp, th = G.THEMES[theme]
    f = G.load1x(fp); tpl = G.chip_template(f)
    a, plus = G.board(f, ((155, 225), (74, 166)))
    m = big_morpheus(a, f)
    rects = {}
    for (name, _), (x, y) in zip(G.DEVICES, SLOTS):
        a[y - 1:y + G.CHH + 1, x - 1:x + G.CW + 1] = G.BASE
        rects[name] = G.draw_chip(a, f, tpl, x, y, name)
    # overlap check
    R = list(rects.values()) + [m]
    for i in range(len(R)):
        for j in range(i + 1, len(R)):
            ax, ay, aw, ah = R[i]; bx, by, bw, bh = R[j]
            assert ax + aw <= bx or bx + bw <= ax or ay + ah <= by or by + bh <= ay, (R[i], R[j])
    return a, rects, m, G.crown_rect(f)

if __name__ == '__main__':
    O = G.OUT + '/variant_big_morpheus'; os.makedirs(O, exist_ok=True); meta = {}
    for theme in G.THEMES:
        a, rects, m, crown = render(theme)
        Image.fromarray(G.up(a)).save('%s/grid_big_morpheus_%s_3x.png' % (O, theme))
        Image.fromarray(G.overlay(a, list(rects.values()) + [m, crown])).save('%s/grid_big_morpheus_%s_tapzones_3x.png' % (O, theme))
        meta = {'chip_tap_rects_1x_xywh': rects, 'morpheus_rect_1x_xywh': m, 'back_rect_1x_xywh': crown}
    json.dump(meta, open(O + '/tap_rects_big_morpheus.json', 'w'), indent=1); print(json.dumps(meta))
