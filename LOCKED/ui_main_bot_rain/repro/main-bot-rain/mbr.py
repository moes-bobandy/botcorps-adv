"""Main-bot chip rain (mock only). Black chip body (grey body area only), rain of whole 8x8 icons inside it
(lead 80%, trails 65..45%, 30% on label rows), 1px dim grey outline (SEP 84,88,98). Mobandy 2026-11-02 4:04/4:05 PM ET."""
import sys, os, random, subprocess
sys.path.insert(0, '/workspace/out/morpheus-glossary/tool'); sys.path.insert(0, '/workspace/out/morpheus-look-final')
import numpy as np; from PIL import Image
from scipy.ndimage import binary_fill_holes, binary_erosion, binary_dilation, label as cc
import glossary as G, rain_glossary as R
SEP = (84, 88, 98); SHADOW = (24, 24, 26); LEVELS = [0.80, 0.65, 0.60, 0.55, 0.50, 0.45]; LABEL_DIM = 0.30
c565 = R.c565
# --- glyph sets: list of (8x8 art rows, {'a':rgb,'b':rgb,'c':rgb,'e':rgb}) ---
DEV = [(R.ART[i], dict(zip('abc', R.INK[i]))) for i in range(10)]          # firmware DEV_ART + DEV_NEON
SPR_ROUND = ["...1...", ".11111.", "1111111", "1221221", "1221221", "1111111", ".11111."]   # src/ui/botcorps_sprite.cpp
GIB_ART = ["........"] + ["." + r.replace('1', 'a').replace('2', 'e').replace('0', '.') for r in SPR_ROUND]
GIB_IVORY = c565(0xF79C)                                                      # botcorps_rain.cpp GIB_IVORY_N
GIBSON = [(GIB_ART, {'a': GIB_IVORY, 'e': (0, 0, 0)})]
ORANGE = [(255, 106, 0), (110, 46, 0), (255, 196, 140)]; BLUE = [(48, 96, 255), (16, 34, 110), (170, 196, 255)]  # glossary WIFI / BLUETOOTH inks
def half(rows):   # 16x16 glossary art -> 8x8 (top-left of each 2x2, ink letters only)
    out = []
    for y in range(0, 16, 2):
        r = ''
        for x in range(0, 16, 2):
            q = [rows[y + dy][x + dx] for dy in (0, 1) for dx in (0, 1)]
            ink = [c for c in q if c in 'abcw']; r += (max(set(ink), key=ink.count) if ink else '.')
        out.append(r)
    return out
# No 8x8 Bluetooth rain icon exists in firmware (DEV_ART has none). 8x8 redraw of the glossary BLE rune (IC_ART 'BLE', BLUE inks):
BLE8 = ["aaawaaa.", "aaawwaa.", "awawawa.", "aawwwaa.", "aawwwaa.", "awawawa.", "aaawwaa.", "aaawaaa."]
SCAN = [(R.ART[0], dict(zip('abc', ORANGE))), (BLE8, {'a': BLUE[0], 'b': BLUE[1], 'c': BLUE[2], 'w': (240, 244, 255)})]
def grey(p): p = p.astype(int); return (np.ptp(p, axis=-1) < 40) & (p.sum(-1) > 120) & (p.sum(-1) < 720)
def prep(f, box, panel, label_box):
    """f: 1x frame. box: loose body bbox (x0,y0,x1,y1 incl). panel: black face panel rect incl. label_box: label rows box."""
    a = f.copy().astype(np.uint8); H, W = a.shape[:2]
    inb = np.zeros((H, W), bool); inb[box[1]:box[3] + 1, box[0]:box[2] + 1] = True
    pan = np.zeros((H, W), bool); pan[panel[1] - 1:panel[3] + 2, panel[0] - 1:panel[2] + 2] = True
    g = grey(a) & inb & ~pan
    lab, n = cc(g); body = lab == (np.bincount(lab.ravel())[1:].argmax() + 1); body = g & binary_fill_holes(body)   # incl. grey inside letter counters
    lb = np.zeros((H, W), bool); lb[label_box[1]:label_box[3] + 1, label_box[0]:label_box[2] + 1] = True
    text = lb & ~pan & (a.astype(int).sum(-1) < 100) & binary_fill_holes(body) & ~body
    lt = lb & ~body & ~(a.astype(int).sum(-1) < 30) & binary_fill_holes(body)   # any label ink (dark name + theme sub-label)
    ty = np.nonzero(lt.any(1) & (lt.sum(1) > 3))[0]
    if len(ty): lb[:] = False; lb[ty.min() - 1:ty.max() + 2, label_box[0]:label_box[2] + 1] = True   # 30% only on the actual label rows
    a[body] = 0; a[text] = G.LABEL_INK
    sh = binary_dilation(text) & ~text; a[sh] = SHADOW
    # SEP ring 1px outside the face panel (as on the locked MORPHEUS rain)
    x0, y0, x1, y1 = panel; a[y0 - 1, x0 - 1:x1 + 2] = SEP; a[y1 + 1, x0 - 1:x1 + 2] = SEP; a[y0 - 1:y1 + 2, x0 - 1] = SEP; a[y0 - 1:y1 + 2, x1 + 1] = SEP
    sil = binary_fill_holes(body | text | pan & inb); edge = sil & ~binary_erosion(sil); a[edge] = SEP
    mask = body & ~edge & ~sh & ~pan
    # whole 8x8 cells: per 8-row band, runs of columns >=75% covered, cells evenly spread across each run
    ys = np.nonzero(mask.any(1))[0]; cells = []
    for cy in range(ys.min(), ys.max() - 6, 8):
        cov = mask[cy:cy + 8].mean(0) >= 0.5; x = 0
        while x < W:
            if not cov[x]: x += 1; continue
            x2 = x
            while x2 < W and cov[x2]: x2 += 1
            L = x2 - x; n = L // 8
            for j in range(n):
                cx = x + (round(j * (L - 8) / (n - 1)) if n > 1 else (L - 8) // 2)
                if mask[cy:cy + 8, cx:cx + 8].mean() >= 0.85: cells.append((cx, cy))
            x = x2
    return a, mask, cells, lb
def paint(a, mask, cells, lb, glyphs, t, seed=4):
    r = random.Random(seed); col = {}
    for cx, cy in cells:
        k = cx // 8
        if k not in col: col[k] = ([r.randrange(len(glyphs)) for _ in range(37)], r.randrange(6))
        seq, ph = col[k]; row = cy // 8
        lv = LEVELS[(t - row + ph) % len(LEVELS)]; art, ink = glyphs[seq[(t - row) % 37]]
        for yy, line in enumerate(art):
            for xx, ch in enumerate(line):
                x, y = cx + xx, cy + yy
                if ch == '.' or not mask[y, x]: continue
                f = lv * (LABEL_DIM if lb[y, x] else 1)
                a[y, x] = [int(v * f) for v in ink[ch]]
def render(f, spec, glyphs, t=9):
    a, mask, cells, lb = prep(f, *spec); b = a.copy(); paint(b, mask, cells, lb, glyphs, t)
    return b, cells
def sbs(cur, new): return np.hstack([G.up(cur.astype(np.uint8)), np.zeros((cur.shape[0] * 3, 12, 3), np.uint8), G.up(new)])
def zoom(a, box, m=2):
    x0, y0, x1, y1 = box; c = a[y0 - m:y1 + m + 1, x0 - m:x1 + m + 1].astype(np.uint8)
    return np.array(Image.fromarray(c).resize((c.shape[1] * 8, c.shape[0] * 8), Image.NEAREST))
