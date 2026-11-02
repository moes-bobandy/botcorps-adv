"""MORPHEUSbot GLOSSARY mock (CYD 240x320 portrait). MOCK ONLY: composites a new screen from the
home portrait board reference frames (g200 green, g380 orange), the CONTROLbot crew chip as the chip
template, firmware icon art (src/live/lv_icons.h, DEV_DEAUTH in src/ui/botcorps_ui.cpp) and firmware
fonts (GLCD 5x7 from src/live/lv_font.h, tiny 3x5). Writes 3x PNGs, tap-zone overlays, rects JSON."""
import sys, os, re, json
import numpy as np
from PIL import Image
sys.path.insert(0, '/workspace/out/morpheus-rename/tool')
from mlib import glcd_pts, tiny_pts
REPO = '/workspace/cyd-portrait-wt'
FR = '/workspace/out/morpheus-rename/frames'
OUT = sys.argv[1] if len(sys.argv) > 1 else '/workspace/out/morpheus-glossary'
THEMES = {'green': (FR + '/g200.png', (146, 255, 0)), 'orange': (FR + '/g380.png', (255, 106, 0))}
def bm(rows): return [(c, r) for r, row in enumerate(rows) for c, ch in enumerate(row) if ch == '#']
LOWER = {'b': bm(['#..', '#..', '###', '#.#', '###']), 'o': bm(['...', '...', '###', '#.#', '###']),
         't': bm(['.#.', '###', '.#.', '.#.', '.##'])}   # the clip's own tiny lowercase (BOOTbot/CONTROLbot chip)
BASE = (0, 7, 7); WHITE = (247, 242, 230); TAN = (180, 144, 82)

def c565(v):
    r, g, b = v >> 11 & 31, v >> 5 & 63, v & 31
    return (r * 255 // 31, g * 255 // 63, b * 255 // 31)

# ---- firmware icon art --------------------------------------------------------------
_ic = open(REPO + '/src/live/lv_icons.h').read()
IC_NAMES = re.search(r'LV_ICON_NAME\[LV_IC_N\] = \{([^}]*)\}', _ic).group(1).replace('"', '').replace(' ', '').split(',')
IC_INK = [tuple(int(x, 16) for x in t) for t in re.findall(r'\{(0x\w+),(0x\w+),(0x\w+)\}', _ic.split('LV_ICON_INK')[1].split(';')[0])]
_art = _ic.split('LV_ICON_ART')[1]
IC_ART = {}
for name in IC_NAMES:
    m = re.search(r'/\* %s \*/((?:\s*"[^"]*",?)+)' % name, _art)
    IC_ART[name] = re.findall(r'"([^"]*)"', m.group(1))
FIXED = {'w': 0xFFFF, 'r': 0xF8A2, 'g': 0x2FE4, 'y': 0xFFE0, 's': 0xBDF7, 'n': 0x2124, 'o': 0xFC80, 'u': 0x23DF, 'k': 0x0841}
def lv_icon(name, inks=None):
    i = IC_NAMES.index(name)
    ink = inks or [c565(v) for v in IC_INK[i]]
    px = {}
    for y, row in enumerate(IC_ART[name]):
        for x, ch in enumerate(row):
            if ch == '.': continue
            px[(x, y)] = ink['abc'.index(ch)] if ch in 'abc' else c565(FIXED[ch])
    return px, 16
# DEV_DEAUTH 12x12, src/ui/botcorps_ui.cpp DEV_ROW[DEV_DEAUTH], DEV_COL = {PAL_BAC7F 0x591D, idx33 0x2087, PAL_INK 0xE77E}
DEAUTH_ROWS = ["...aaaaaa...", ".aa......aa.", "a..aaaaaa..a", "..aa....aa..", "...a.aa.a...", ".....aa.....",
               ".....aa.....", "........c..c", ".........cc.", ".........cc.", "........c..c", "............"]
def deauth_icon():
    col = {'a': c565(0x591D), 'b': c565(0x2087), 'c': c565(0xE77E)}
    return {(x, y): col[ch] for y, r in enumerate(DEAUTH_ROWS) for x, ch in enumerate(r) if ch != '.'}, 12

ORANGE = [(255, 106, 0), (110, 46, 0), (255, 196, 140)]   # clear orange a/b/c
BLUE = [(48, 96, 255), (16, 34, 110), (170, 196, 255)]     # clear blue a/b/c
# grid order = BOTS table order, src/ui/botcorps_ui.cpp ~190 (ids 0..11)
DEVICES = [
    ('WIFI', lambda: lv_icon('AP', ORANGE)), ('BLUETOOTH', lambda: lv_icon('BLE', BLUE)), ('DEAUTH', deauth_icon),
    ('HACK', lambda: lv_icon('LAPTOP')), ('FLOCK', lambda: lv_icon('CCTV')), ('DRONE', lambda: lv_icon('DRONE')),
    ('AIR TAG', lambda: lv_icon('AIRTAG')), ('KEYFOB', lambda: lv_icon('KEYFOB')), ('META', lambda: lv_icon('GLASSES')),
    ('SKIMMER', lambda: lv_icon('SKIMMER')), ('FLIPPER', lambda: lv_icon('FLIPPER')), ('SAMSUNG', lambda: lv_icon('GALAXYTAG')),
]
TEXT = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'glossary_text.json')))

# ---- frame + chip template ----------------------------------------------------------
CH_X0, CH_Y0, CW, CHH = 165, 51, 44, 61          # CONTROLbot chip on the home frame (1x)
FACE = (176, 64, 198, 86)                         # black face window (incl.), template coords absolute
LABEL = (168, 94, 205, 107)                       # label rows on the chip body
def load1x(p): return np.array(Image.open(p).convert('RGB'))[1::3, 1::3].astype(np.int32)
def is_board(p): return int(p[0]) + int(p[1]) + int(p[2]) < 40 and p[0] < 12

def chip_template(f):
    t = f[CH_Y0:CH_Y0 + CHH, CH_X0:CH_X0 + CW].copy()
    mask = np.array([[not is_board(t[y, x]) for x in range(CW)] for y in range(CHH)])
    # face window -> black
    t[FACE[1] - CH_Y0:FACE[3] - CH_Y0 + 1, FACE[0] - CH_X0:FACE[2] - CH_X0 + 1] = 0
    # erase CONTROL / bot label ink: dark pixels on the grey body -> nearest body pixel to the left
    for y in range(LABEL[1] - CH_Y0, LABEL[3] - CH_Y0 + 1):
        for x in range(LABEL[0] - CH_X0, LABEL[2] - CH_X0 + 1):
            if t[y, x].sum() < 200:
                k = x - 1
                while k > 0 and t[y, k].sum() < 200: k -= 1
                t[y, x] = t[y, k]
    ink = (24, 24, 26)
    for y in range(LABEL[1], LABEL[3] + 1):
        for x in range(LABEL[0], LABEL[2] + 1):
            p = f[y, x]
            if p.sum() < 120: ink = tuple(int(v) for v in p); break
    return t, mask, ink

def draw_chip(a, f, tpl, x0, y0, name):
    t, mask, ink = tpl
    reg = a[y0:y0 + CHH, x0:x0 + CW]
    reg[mask] = t[mask]
    px, n = dict(DEVICES)[name]()
    fx0 = x0 + FACE[0] - CH_X0; fy0 = y0 + FACE[1] - CH_Y0; fw = FACE[2] - FACE[0] + 1
    ox = fx0 + (fw - n) // 2; oy = fy0 + (fw - n) // 2
    for (x, y), c in px.items(): a[oy + y, ox + x] = c
    lx0 = x0 + LABEL[0] - CH_X0; lw = LABEL[2] - LABEL[0] + 1
    for word, yy in ((name, 95), ('bot', 101)):
        w = len(word) * 4 - 1
        for (x, y) in tiny_pts(word, lx0 + (lw - w + 1) // 2, y0 + yy - CH_Y0, LOWER): a[y, x] = ink
    return (x0, y0, CW, CHH)

def board(f, marks=((105, 175, 245), (36, 92, 148, 204))):
    """keep top bar (y<26), side pin columns (x<30, x>=210) and bottom edge (y>=313); clear the middle"""
    a = f.copy()
    plus = tuple(int(v) for v in f[38, 54])
    a[26:313, 30:210] = BASE
    for gy in marks[0]:                           # silk '+' marks in the gaps between chips
        for gx in marks[1]:
            for d in (-2, -1, 0, 1, 2): a[gy, gx + d] = plus; a[gy + d, gx] = plus
    return a, plus

def text(a, s, x, y, col, scale=1):
    for (px, py) in glcd_pts(s, 0, 0):
        a[y + py * scale:y + py * scale + scale, x + px * scale:x + px * scale + scale] = col

COLS = [42, 98, 154]; ROWS = [40, 110, 180, 250]
def crown_rect(f):
    """the crown plate in the top bar (crown = BACK, src/ui/botcorps_cyd_edge.inc:184); bbox of non-black pixels"""
    ys, xs = np.where(f[2:23, 125:185].sum(2) > 25)
    x0, y0, x1, y1 = 125 + int(xs.min()), 2 + int(ys.min()), 125 + int(xs.max()), 2 + int(ys.max())
    return (x0, y0, x1 - x0 + 1, y1 - y0 + 1)

def grid(theme):
    fp, th = THEMES[theme]
    f = load1x(fp); tpl = chip_template(f)
    a, plus = board(f)
    half = tuple(v // 2 for v in th)
    # traces: column buses between rows, 2px wide, half theme (home trace style)
    for cx in COLS:
        for r in range(3):
            a[ROWS[r] + CHH:ROWS[r + 1], cx + 21:cx + 23] = half
    w = 8 * 6 - 1
    text(a, 'GLOSSARY', 120 - (w + 1) // 2, 30, WHITE)
    rects = {}
    for i, (name, _) in enumerate(DEVICES):
        x0, y0 = COLS[i % 3], ROWS[i // 3]
        a[y0 - 1:y0 + CHH + 1, x0 - 1:x0 + CW + 1] = BASE
        rects[name] = draw_chip(a, f, tpl, x0, y0, name)
    return a, rects, crown_rect(f), f, tpl, th, plus

def wrap(s, n):
    out, cur = [], ''
    for w in s.split():
        if len(cur) + len(w) + (1 if cur else 0) <= n: cur = (cur + ' ' + w).strip()
        else: out.append(cur); cur = w
    out.append(cur); return out

PANEL = (32, 112, 207, 252)
def desc(theme, name):
    fp, th = THEMES[theme]
    f = load1x(fp); tpl = chip_template(f)
    a, plus = board(f, ((40, 66, 280, 302), (56, 84, 156, 184)))
    x0 = 120 - CW // 2
    a[29:29 + CHH + 2, x0 - 1:x0 + CW + 1] = BASE
    draw_chip(a, f, tpl, x0, 30, name)
    w = len(name) * 12 - 2
    text(a, name, 120 - (w + 1) // 2 + 1, 94, WHITE, 2) if w < 176 else None
    X0, Y0, X1, Y1 = PANEL
    a[Y0:Y1 + 1, X0:X1 + 1] = TAN; a[Y0 + 1:Y1, X0 + 1:X1] = th; a[Y0 + 2:Y1 - 1, X0 + 2:X1 - 1] = 0
    t = TEXT[name]; y = Y0 + 6; lines = 0
    for head, key in (('WHAT IT IS', 'what'), ('HOW IT IS SPOTTED', 'spot'), ('HOW TO PROTECT YOURSELF', 'protect')):
        text(a, head, X0 + 6, y, th); y += 10
        for ln in wrap(t[key], 27):
            text(a, ln, X0 + 6, y, WHITE); y += 10; lines += 1
        y += 6
    assert y <= Y1 - 8, (name, y)
    tag = 'DRAFT TEXT'
    for (px, py) in tiny_pts(tag, X1 - 4 - (len(tag) * 4 - 1), Y1 - 8): a[py, px] = (146, 145, 138)
    return a, crown_rect(f), lines

def up(a): return np.repeat(np.repeat(a.astype(np.uint8), 3, 0), 3, 1)
def overlay(a, rects, col=(255, 0, 200)):
    b = up(a).copy()
    for (x, y, w, h) in rects:
        X0, Y0, X1, Y1 = 3 * x, 3 * y, 3 * (x + w) - 1, 3 * (y + h) - 1
        for k in range(2):
            b[Y0 + k, X0:X1 + 1] = col; b[Y1 - k, X0:X1 + 1] = col; b[Y0:Y1 + 1, X0 + k] = col; b[Y0:Y1 + 1, X1 - k] = col
    return b

if __name__ == '__main__':
    os.makedirs(OUT + '/desc', exist_ok=True); os.makedirs(OUT + '/overlay', exist_ok=True)
    meta = {'grid_tap_rects_1x_xywh': {}, 'back_rect_1x_xywh': None, 'desc_lines': {}}
    for theme in THEMES:
        a, rects, crown, *_ = grid(theme)
        Image.fromarray(up(a)).save('%s/grid_%s_3x.png' % (OUT, theme))
        Image.fromarray(overlay(a, list(rects.values()) + [crown])).save('%s/overlay/grid_%s_tapzones_3x.png' % (OUT, theme))
        meta['grid_tap_rects_1x_xywh'] = rects; meta['back_rect_1x_xywh'] = crown
        for name, _ in DEVICES:
            d, crown, n = desc(theme, name)
            fn = name.replace(' ', '_').lower()
            Image.fromarray(up(d)).save('%s/desc/desc_%s_%s_3x.png' % (OUT, fn, theme))
            meta['desc_lines'][name] = n
            if name in ('WIFI', 'SKIMMER'):
                Image.fromarray(overlay(d, [crown])).save('%s/overlay/desc_%s_%s_backzone_3x.png' % (OUT, fn, theme))
    json.dump(meta, open(OUT + '/tap_rects.json', 'w'), indent=1)
    print(json.dumps(meta))
