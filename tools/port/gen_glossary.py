# Generate src/live/lv_glossary_data.h: the LOCKED MORPHEUSbot Device Glossary (item 2, fw-fix7-1106).
#   LV_SC_GLOSS       LOCKED/ui_glossary_morpheus_rain grid_big_morpheus_chiprain_black_soft_final_{green,orange}_3x.png
#                     + the chip rain inside MORPHEUSbot's black body, procedural and exact: the mock's own generator
#                     (docs/mocks/morpheus-look-final/rain_chip_black_soft_final/rain_chip_black_soft_final.py paint()) is
#                     reproduced from its tables (12 columns x 37-step device sequences, 6 levels, label dim), so step t is
#                     the mock's frame t (the locked still is t = 9; the mock mp4 is t = 0..73, period 222).
#   LV_SC_GLOSS_DESC  LOCKED/ui_glossary desc_{wifi..samsung}_{green,orange}_3x.png, verbatim.
# Pixels: every locked 3x PNG is a nearest 3x of the 240x320 1x board ([1::3, 1::3], asserted). Per pixel g = green,
# o = orange (RGB565). g == o: unthemed (all 6 slots g). g != o: themed, slot 2 = g, slot 5 = o verbatim, slots 0 1 3 4 from
# the home clip theme map (sim dumps of LV_SC_HOME in themes 0..5, positional majority, like gen_port.py Theme('home')),
# falling back to the accent hue map when g is not a home-clip colour. Themes 6..11 use slot 3 + lv_multi_ink (themed only).
# Encoding: a base layer (desc_wifi, all 320 rows) + one patch layer per screen (grid, 11 descs) over its bbox of pixels that
# differ from the base, scanline RLE tokens uint16 = pal_id << 6 | (len - 1), pal_id 1023 = keep the base pixel.
# Usage: python gen_glossary.py CLIPDIR OUT.h [REPORT.json]   (CLIPDIR: sim_live dumps home_{t}_{kk}.rgb, t 0..5, kk 00..15)
import sys, os, json, random, collections
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
LOCK = '/workspace/cyd-lock-store/LOCKED'
FRAMES = LOCK + '/ui_main_bot_rain/repro/inputs/morpheus-rename/frames'
M = REPO + '/docs/mocks'
for p in ('morpheus-rename/tool', 'morpheus-simple/tool', 'morpheus-look-final', 'morpheus-glossary/tool', 'morpheus-look-final/rain_chip',
          'morpheus-look-final/rain_chip_black_soft_final'): sys.path.insert(0, os.path.join(M, p))
import glossary as G
G.THEMES = {'green': (FRAMES + '/g200.png', G.THEMES['green'][1]), 'orange': (FRAMES + '/g380.png', G.THEMES['orange'][1])}
import rain_chip as RC, rain_glossary as RG, rain_chip_black_soft_final as RF
CLIPD, OUT = sys.argv[1:3]; REP = sys.argv[3] if len(sys.argv) > 3 else None
NAMES = ['WIFI', 'BLUETOOTH', 'DEAUTH', 'HACK', 'FLOCK', 'DRONE', 'AIR TAG', 'KEYFOB', 'META', 'SKIMMER', 'FLIPPER', 'SAMSUNG']
FILES = [n.lower().replace(' ', '_') for n in NAMES]
def k565(a): a = np.asarray(a).astype(np.int64); return (a[..., 0] >> 3) << 11 | (a[..., 1] >> 2) << 5 | (a[..., 2] >> 3)
def x888(k): k = int(k); return (((k >> 11) & 31) * 255 // 31, ((k >> 5) & 63) * 255 // 63, (k & 31) * 255 // 31)
def ld(p):
    a = np.array(Image.open(p).convert('RGB')); s = a[1::3, 1::3]
    assert s.shape == (320, 240, 3) and (np.repeat(np.repeat(s, 3, 0), 3, 1) == a).all(), p
    return s
GRID = LOCK + '/ui_glossary_morpheus_rain/grid_big_morpheus_chiprain_black_soft_final_%s_3x.png'
DESC = LOCK + '/ui_glossary/desc_%s_%s_3x.png'
SCR = [(ld(GRID % 'green'), ld(GRID % 'orange'))] + [(ld(DESC % (f, 'green')), ld(DESC % (f, 'orange'))) for f in FILES]
# ---- rain: reproduce the locked still from the mock generator, then take its tables
rr = random.Random(4); SEQS = [[rr.randrange(10) for _ in range(37)] for _ in range(12)]; PHASE = [rr.randrange(6) for _ in range(12)]
T_STILL = 9
A, MASK = {}, {}
for th, lk in (('green', SCR[0][0]), ('orange', SCR[0][1])):
    a, mask = RF.base(th); b = a.copy(); RF.paint(b, mask, T_STILL, SEQS, PHASE); RC.label(b)
    assert (b.astype(np.uint8) == lk).all(), 'mock generator does not reproduce the locked grid (%s)' % th
    A[th], MASK[th] = a, mask
assert (MASK['green'] == MASK['orange']).all()
mask = MASK['green']
z = np.zeros((320, 240, 3), np.int64); RC.label(z); w = np.full((320, 240, 3), 255, np.int64); RC.label(w)
labelpix = (z != 0).any(2) | (w != 255).any(2)
meff = mask & ~labelpix                         # pixels the rain owns (the label is drawn over the rain, constant)
ys, xs = np.nonzero(meff); RX0, RY0, RX1, RY1 = int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())
assert 74 <= RX0 and RX1 <= 164 and 30 <= RY0 and RY1 <= 152, (RX0, RY0, RX1, RY1)   # inside morpheus_rect [74, 30, 91, 123]
CELLS = []                                      # paint order (c major), eligibility exactly as paint()
for c in range(12):
    for row in range(13):
        cy = 49 + row * 8
        if cy + 8 > 110:
            if c not in RF.BOTTOM_C: continue
            cx = RF.BOTTOM_X[RF.BOTTOM_C.index(c)]
        else: cx = 82 + c * 8 if c < 11 else 150
        if mask[cy:cy + 8, cx:cx + 8].mean() < 0.85: continue
        CELLS.append((cx, cy, c, row))
CELLS_BY_ROW = sorted(CELLS, key=lambda t: (t[3], t[2]))    # within a row, c order = overwrite order; rows never overlap
ROWIDX = [0] * 14
for r in range(13): ROWIDX[r + 1] = ROWIDX[r] + sum(1 for t in CELLS if t[3] == r)
ART = [[sum(('.abc'.index(ch)) << (2 * xx) for xx, ch in enumerate(line)) for line in RG.ART[i]] for i in range(10)]
LR0, LR1 = RC.LABEL_ROWS
RCOL = [[[[int(k565([int(v * (RF.LEVELS[k] * (RF.LABEL_DIM_SOFT if d else 1))) for v in RG.INK[i][ch]])) for d in range(2)] for k in range(6)] for ch in range(3)] for i in range(10)]
def rain_px(t, x, y):                           # mirror of lv_gl_rain_px (C)
    r = (y - 49) // 8; out = 0
    if 0 <= r < 13:
        for cx, cy, c, row in CELLS_BY_ROW[ROWIDX[r]:ROWIDX[r + 1]]:
            if not (cx <= x < cx + 8): continue
            ch = (ART[SEQS[c][(t - row) % 37]][y - cy] >> (2 * (x - cx))) & 3
            if ch: out = RCOL[SEQS[c][(t - row) % 37]][ch - 1][(t - row + PHASE[c]) % 6][1 if LR0 <= y <= LR1 else 0]
    return out
def rain_frame(t):
    a = A['green'].copy(); b = a.copy(); RF.paint(b, mask, t, SEQS, PHASE); RC.label(b); return k565(b)
for t in list(range(0, 74)) + [221]:
    fr = rain_frame(t); ys, xs = np.nonzero(meff)
    model = np.array([rain_px(t, x, y) for y, x in zip(ys, xs)])
    assert (model == fr[ys, xs]).all(), 'rain model differs at t=%d' % t
    if t == 0: base_out = fr.copy()
    assert ((fr == base_out) | meff).all(), 'rain touches a pixel outside meff at t=%d' % t
print('rain: %d cells, %d owned px (bbox %d,%d..%d,%d), model == mock generator for t = 0..73 and 221 (period 222)' % (len(CELLS), int(meff.sum()), RX0, RY0, RX1, RY1))
# ---- theme map from the home clip (positional, static pixels), as gen_port.py Theme('home')
SNAP = 16
cs = [np.array([np.fromfile('%s/home_%d_%02d.rgb' % (CLIPD, t, k), np.uint8).reshape(320, 240, 3) for k in range(16)]) for t in range(6)]
stat = np.ones((320, 240), bool)
for c in cs: stat &= ~(c != c[:1]).any(-1).any(0)
stat[:24] = False; stat[270:] = False
basek = k565(cs[2][0][stat]); tmap = {}
for t in range(6):
    d = collections.defaultdict(collections.Counter)
    for a, b in zip(basek, k565(cs[t][0][stat])): d[int(a)][int(b)] += 1
    for a, c in d.items(): tmap.setdefault(a, [None] * 6)[t] = c.most_common(1)[0][0]
tkeys = np.array(sorted(tmap)); tcols = np.array([x888(k) for k in tkeys])
LV_ACC = [0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00]
def hue(g, t):                                  # fallback: the green accent's shading carried to theme t's accent
    r, gg, b = x888(g); mx = max(r, gg, b, 1); W = min(r, gg, b) * 255 // mx; T = LV_ACC[t]
    return int(k565([((((T >> (16 - 8 * k)) & 255) * (255 - W) + 255 * W) // 255) * mx // 255 for k in range(3)]))
stats = collections.Counter()
# positional: a board pixel the home clip draws identically (static over 16 frames) in theme 2 = g and theme 5 = o takes
# the clip's own colour in every theme at that position (rails, pads, pins, silk: what the clip itself themes)
C565 = [k565(c[0]) for c in cs]
statp = np.ones((320, 240), bool)
for c in cs: statp &= ~(c != c[:1]).any(-1).any(0)
def accentish(g, o):                            # green accent shading in green, orange accent shading in orange
    gr, gg, gb = x888(g); orr, og, ob = x888(o)
    return gg > gr + 24 and gg > gb + 24 and orr > og + 24 and orr > ob + 24
def slots(g, o, x, y):
    if g == o: return (g,) * 6, 0
    if statp[y, x] and int(C565[2][y, x]) == g and int(C565[5][y, x]) == o:
        stats['positional'] += 1; return tuple(int(C565[t][y, x]) for t in range(6)), 1
    d = np.abs(tcols - np.array(x888(g))).max(-1); i = int(d.argmin())
    m = tmap[int(tkeys[i])] if d[i] <= SNAP else None
    if m is not None and any(m[t] != m[2] for t in range(6)) and np.abs(np.array(x888(m[5])) - np.array(x888(o))).max() <= 24:
        v = [int(m[t]) for t in range(6)]; stats['clip_colour'] += 1
    elif accentish(g, o): v = [hue(g, t) for t in range(6)]; stats['accent_hue'] += 1
    else: v = [g] * 6; stats['neutral'] += 1   # tan / grey / near-black board noise that only differs between the two mock frames
    v[2], v[5] = g, o
    return tuple(v), 1
PAL, PALI = [], {}
def pid(g, o, x=0, y=0):
    key = slots(int(g), int(o), x, y)
    if key not in PALI: PALI[key] = len(PAL); PAL.append(key)
    return PALI[key]
BLACK = pid(0, 0)
IDS = []
for s, (g, o) in enumerate(SCR):
    G5, O5 = k565(g), k565(o); ids = np.zeros((320, 240), np.int64)
    for y in range(320):
        for x in range(240): ids[y, x] = pid(int(G5[y, x]), int(O5[y, x]), x, y)
    if s == 0: ids[meff] = BLACK                 # rain-owned pixels are drawn by the rain model at runtime
    ids[1:24, 1:239] = ids[1:24, :1]             # ticker interior (runtime lv_ticker_tick): never blitted, compress it away
    IDS.append(ids)
NPAL = len(PAL); TRANSP = 1023; assert NPAL < TRANSP, NPAL
print('palette %d entries (%d themed); themed px by source: %s' % (NPAL, sum(p[1] for p in PAL), dict(stats)))
def enc_rows(ids, y0, y1, x0, x1, keep=None):
    off, tok = [], []
    for y in range(y0, y1 + 1):
        off.append(len(tok)); x = x0
        while x <= x1:
            v = TRANSP if keep is not None and keep[y, x] else int(ids[y, x]); e = x
            while e + 1 <= x1 and e - x + 1 < 64 and (TRANSP if keep is not None and keep[y, e + 1] else int(ids[y, e + 1])) == v: e += 1
            tok.append(v << 6 | (e - x)); x = e + 1
    off.append(len(tok)); assert len(tok) < 65536
    return off, tok
BASE = IDS[1]
LAY = [('BASE', 0, 319, 0, 239) + enc_rows(BASE, 0, 319, 0, 239)]
for s in range(13):
    d = IDS[s] != BASE
    if not d.any(): LAY.append(('P%d' % s, 1, 0, 0, 0, [0], [])); continue
    ys, xs = np.nonzero(d); y0, y1, x0, x1 = int(ys.min()), int(ys.max()), int(xs.min()), int(xs.max())
    assert y0 >= 24
    LAY.append(('P%d' % s, y0, y1, x0, x1) + enc_rows(IDS[s], y0, y1, x0, x1, keep=~d))
# ---- byte codec per row (random access by row offsets), 3-colour MRU reset per row to the layer's 3 commonest ids:
#   b = sel << 6 | (len - 1): sel 0 = MRU[0] again (long runs), 1 / 2 = MRU[1] / MRU[2] (moved to front), len 1..64
#   sel 3: explicit id, 2 bytes: 0xC0 | id >> 4, (id & 15) << 4 | (len - 1), len 1..16
def row_runs(tok):
    out = []
    for t in tok:
        v, n = t >> 6, (t & 63) + 1
        if out and out[-1][0] == v: out[-1][1] += n
        else: out.append([v, n])
    return out
def enc_bytes(rr, init):
    mru = list(init); out = []
    for v, n in rr:
        first = True
        while n > 0:
            if first and v in mru[1:3]:
                m = min(n, 64); out.append(mru.index(v) << 6 | (m - 1)); mru.remove(v); mru.insert(0, v)
            elif first and mru[0] != v:
                m = min(n, 16); out += [0xC0 | (v >> 4), ((v & 15) << 4) | (m - 1)]
                if v in mru: mru.remove(v)
                mru.insert(0, v); mru = mru[:3]
            else: m = min(n, 64); out.append(m - 1)
            first = False; n -= m
    return out
BL = []
for nm, y0, y1, x0, x1, off, tok in LAY:
    c = collections.Counter()
    for t in tok: c[t >> 6] += (t & 63) + 1
    init = [v for v, _ in c.most_common(3)]; init += [BLACK, TRANSP, 0][:3 - len(init)]
    dat, boff = [], []
    for r in range(len(off) - 1):
        boff.append(len(dat)); dat += enc_bytes(row_runs(tok[off[r]:off[r + 1]]), init)
    boff.append(len(dat)); assert len(dat) < 65536
    BL.append((nm, y0, y1, x0, x1, init, boff, dat))
def dec_row(li, y, out, slot):
    nm, y0, y1, x0, x1, init, boff, dat = BL[li]
    if not (y0 <= y <= y1): return
    m0, m1, m2 = init; x = x0; i = boff[y - y0]; e = boff[y - y0 + 1]
    while i < e:
        b = dat[i]; i += 1; sel = b >> 6
        if sel == 3:
            c = dat[i]; i += 1; v = (b & 63) << 4 | c >> 4; n = (c & 15) + 1
            if v == m1: m1, m0 = m0, v
            else: m2, m1, m0 = m1, m0, v
        else:
            n = (b & 63) + 1
            if sel == 1: v = m1; m1, m0 = m0, v
            elif sel == 2: v = m2; m2, m1, m0 = m1, m0, v
            else: v = m0
        if v != TRANSP: out[y, x:x + n] = PAL[v][0][slot]
        x += n
# ---- decode check (python mirror of lv_gl_row) against the locked 1x pixels, slots 2 and 5
def decode(s, slot):
    out = np.zeros((320, 240), np.int64)
    for y in range(320):
        dec_row(0, y, out, slot); dec_row(1 + s, y, out, slot)
    if s == 0:
        ys, xs = np.nonzero(meff); out[ys, xs] = [rain_px(T_STILL, x, y) for y, x in zip(ys, xs)]
    return out
rep = {}
for s in range(13):
    for slot, th in ((2, 'green'), (5, 'orange')):
        ref = k565(SCR[s][0 if slot == 2 else 1]); dec = decode(s, slot)
        m = np.ones((320, 240), bool); m[1:24, 1:239] = False
        rep['%s_%s' % ('grid' if s == 0 else FILES[s - 1], th)] = int((dec != ref)[m].sum())
assert not any(rep.values()), rep
print('decode == locked (RGB565, every pixel but the ticker interior) on all 26 screens')
# ---- emit
ntok = sum(len(l[7]) for l in BL); nrow = sum(len(l[6]) for l in BL)
L = ['/* GENERATED by tools/port/gen_glossary.py - do not edit. LOCKED MORPHEUSbot Device Glossary (item 2, fw-fix7-1106):',
     ' * ui_glossary_morpheus_rain grid + chip rain, ui_glossary 12 description pages. Flash only (const), 0 B RAM. See lv_glossary.h. */',
     '#pragma once', '#include <stdint.h>',
     '#define LV_GL_NPAL %d' % NPAL, '#define LV_GL_TRANSP %d' % TRANSP, '#define LV_GL_T_STILL %d' % T_STILL, '#define LV_GL_RAIN_PERIOD 222',
     '/* palette: RGB565 per theme slot 0..5 (slot 2 = locked green, slot 5 = locked orange); themed bit: re-inked on 6..11 */',
     'static const uint16_t LV_GL_PAL[%d][6] = {%s};' % (NPAL, ','.join('{%s}' % ','.join('0x%04x' % v for v in p[0]) for p in PAL)),
     'static const uint8_t LV_GL_THEMED[%d] = {%s};' % ((NPAL + 7) // 8, ','.join(str(sum(PAL[i][1] << b for b, i in enumerate(range(j, min(j + 8, NPAL))))) for j in range(0, NPAL, 8))),
     'struct LvGlLayer { int16_t y0, y1; uint8_t x0, x1; uint16_t m0, m1, m2; const uint16_t *off; const uint8_t *dat; };']
for nm, y0, y1, x0, x1, init, boff, dat in BL:
    L.append('static const uint16_t LV_GL_%s_OFF[%d] = {%s};' % (nm, len(boff), ','.join(map(str, boff))))
    L.append('static const uint8_t LV_GL_%s_DAT[%d] = {%s};' % (nm, max(1, len(dat)), ','.join(map(str, dat)) or '0'))
L.append('static const LvGlLayer LV_GL_LAYER[14] = {%s};  /* 0 base (desc_wifi), 1 grid, 2.. desc 0..11 */' % ','.join(
    '{%d,%d,%d,%d,%d,%d,%d,LV_GL_%s_OFF,LV_GL_%s_DAT}' % (y0, y1, x0, x1, init[0], init[1], init[2], nm, nm) for nm, y0, y1, x0, x1, init, boff, dat in BL))
mr = []
for y in range(320):
    x = 0
    while x < 240:
        if meff[y, x]:
            e = x
            while e + 1 < 240 and meff[y, e + 1]: e += 1
            mr.append((y, x, e - x + 1)); x = e + 1
        else: x += 1
L += ['/* chip rain (procedural, = rain_chip_black_soft_final.py paint() + label): owned runs, cells per row in paint order */',
      '#define LV_GL_RX0 %d' % RX0, '#define LV_GL_RY0 %d' % RY0, '#define LV_GL_RX1 %d' % RX1, '#define LV_GL_RY1 %d' % RY1,
      '#define LV_GL_LROW0 %d' % LR0, '#define LV_GL_LROW1 %d' % LR1,
      'struct LvGlRun { uint8_t y, x, n; };',
      '#define LV_GL_NRUN %d' % len(mr),
      'static const LvGlRun LV_GL_RUN[%d] = {%s};' % (len(mr), ','.join('{%d,%d,%d}' % r for r in mr)),
      'struct LvGlCell { uint8_t cx, cy, c, row; };',
      'static const LvGlCell LV_GL_CELL[%d] = {%s};' % (len(CELLS_BY_ROW), ','.join('{%d,%d,%d,%d}' % c for c in CELLS_BY_ROW)),
      'static const uint8_t LV_GL_CROW[14] = {%s};' % ','.join(map(str, ROWIDX)),
      'static const uint8_t LV_GL_SEQ[12][37] = {%s};' % ','.join('{%s}' % ','.join(map(str, s)) for s in SEQS),
      'static const uint8_t LV_GL_PHASE[12] = {%s};' % ','.join(map(str, PHASE)),
      'static const uint16_t LV_GL_ART[10][8] = {%s};' % ','.join('{%s}' % ','.join(map(str, a)) for a in ART),
      'static const uint16_t LV_GL_RCOL[10][3][6][2] = {%s};' % ','.join('{%s}' % ','.join('{%s}' % ','.join('{%s}' % ','.join('0x%04x' % v for v in d) for d in k) for k in ch) for ch in RCOL)]
tr = json.load(open(LOCK + '/ui_glossary_morpheus_rain/tap_rects_big_morpheus.json'))
CH = [tr['chip_tap_rects_1x_xywh'][n] for n in NAMES]
L.append('static const LvRect LV_GL_CHIP[12] = {%s};  /* tap_rects_big_morpheus.json, drawn chips (inclusive) */' % ','.join('{%d,%d,%d,%d}' % (x, y, x + w - 1, y + h - 1) for x, y, w, h in CH))
L.append('static const char *const LV_GLOSS_NAME[12] = {%s};' % ','.join('"%s"' % n for n in NAMES))
open(OUT, 'w').write('\n'.join(L) + '\n')
flash = NPAL * 12 + (NPAL + 7) // 8 + ntok + 2 * nrow + 14 * 16 + 3 * len(mr) + 4 * len(CELLS) + 14 + 12 * 37 + 12 + 160 + 720 + 12 * 8
print('codec bytes %d, row offsets %d, rain runs %d, cells %d -> ~%d bytes const data -> %s' % (ntok, nrow, len(mr), len(CELLS), flash, OUT))
if REP: json.dump(dict(npal=NPAL, themed=sum(p[1] for p in PAL), tokens=ntok, rows=nrow, rain_runs=len(mr), cells=len(CELLS), flash_est=flash, decode_diff=rep, theme_map=dict(stats)), open(REP, 'w'), indent=1)
