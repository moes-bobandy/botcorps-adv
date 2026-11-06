# Generate src/live/lv_boot_morpheus.h: the MORPHEUSbot boot chip (item 4, fw-fix7-1106) as a per-pixel colour
# substitution over the LOCKED LK_S_BOOT clip (the blob is untouched).
#   Look: docs/mocks/morpheus-look-final/variant_big_morpheus.py morpheus_chip(**SIZES['boot']) (dark grey body, black
#   shaded face, round shades + bridge, CALbot/BOOTbot board + pins + wave untouched), the chip name "BOOTbot" respelled
#   "MORPHEUSbot" (GLCD 5x7 from src/live/lv_font.h, centred in the chip, same white ink; green BOOTING row kept) and the
#   bottom-right silk "BOOTBOT REV C" -> "MORPHEUS REV C" (tiny 3x5, right aligned like the original; the left BOOT
#   tact-switch silk is the button and stays, PORT_NOTE.md).
#   Rule (runtime, lv_clip_draw on LV_SC_BOOT): a pixel whose CLIP colour equals a token's E becomes that token's O. The
#   tokens are learnt from the settled chip frames 0..CHIP_LAST; the clip's own wipe out of the chip (frames 76..80) then
#   reveals the new art pixel by pixel exactly where the old art leaves (target there: T_75 where the clip still shows
#   the frame-75 chip pixel, else the clip pixel) - no pop-in, no left-over pixels. Checked here over all 85 frames.
# Usage: python gen_boot_morpheus.py FRAMEDIR OUT.h   (FRAMEDIR: sim_live dumps bNNN.rgb of LK_S_BOOT frames 0..84,
#        rendered with -DLV_BOOT_MORPHEUS=0, i.e. raw clip pixels)
import sys, os, collections
import numpy as np
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'docs', 'mocks')
for p in ('morpheus-rename/tool', 'morpheus-simple/tool', 'morpheus-glossary/tool'): sys.path.insert(0, os.path.join(R, p))
import variant_big_morpheus as V
from mlib import glcd_pts, tiny_pts
FD, OUT = sys.argv[1:3]
NF, CHIP_LAST = 85, 75
F = [np.fromfile('%s/b%03d.rgb' % (FD, n), np.uint8).reshape(320, 240, 3).astype(np.int32) for n in range(NF)]
def k565(a): a = np.asarray(a).astype(np.int64); return (a[..., 0] >> 3) << 11 | (a[..., 1] >> 2) << 5 | (a[..., 2] >> 3)
def x888(k): return np.stack([((k >> 11) & 31) * 255 // 31, ((k >> 5) & 63) * 255 // 63, (k & 31) * 255 // 31], -1)
OLD_INK = (231, 239, 247)
LAB_Y, LAB_TXT = 155, 'MORPHEUSbot'
cx0, cy0, cx1, cy1 = V.SIZES['boot']['chip']
LAB_X = cx0 + ((cx1 - cx0 + 1) - (len(LAB_TXT) * 6 - 1)) // 2          # centred in the chip: 87..151
SILK_Y, SILK_TXT = 310, 'MORPHEUS REV C'
def transform(f):
    a = f.copy()
    V.morpheus_chip(a, f, **V.SIZES['boot'])
    # chip name: erase the old white name ink, draw MORPHEUSbot (same ink) centred in the chip
    box = a[LAB_Y - 1:LAB_Y + 8, LAB_X - 1:LAB_X + 66]
    old = (box == np.array(OLD_INK)).all(-1)
    if old.sum() == 105:                                                 # "BOOTbot" fully drawn
        box[old] = V.GRAPH['win']
        for (x, y) in glcd_pts(LAB_TXT, LAB_X, LAB_Y): a[y, x] = OLD_INK
    # bottom-right silk
    s = f[309:316, 150:224]; ink = (s.sum(-1) > 300)
    if ink.sum() == 94:
        col = tuple(int(v) for v in f[309:316, 150:224][ink][0]); plate = tuple(int(v) for v in f[309, 171])
        w = len(SILK_TXT) * 4 - 1; x0 = 240 - 17 - w
        a[309:316, x0 - 1:x0 + w + 1] = plate
        for (x, y) in tiny_pts(SILK_TXT, x0, SILK_Y): a[y, x] = col
    return a
K = [k565(f) for f in F]
REG = [((cx0, cy0, cx1, cy1)), ((150, 309, 224, 315))]          # chip, bottom-right silk
inreg = np.zeros((320, 240), bool)
for x0, y0, x1, y1 in REG: inreg[y0:y1 + 1, x0:x1 + 1] = True
# the clip's wipe out of BOOTbot moves up in bands (frames 74, 76, 78, 80); a row's art leaves at the first wipe frame
# that changes any of its region pixels. Before that the row shows MORPHEUSbot, from it on the clip's own pixels.
WIPE0 = 74
assert all(not ((K[n] != K[n - 1]) & inreg).any() for n in range(CHIP_LAST + 1 - 9, WIPE0)), 'chip not settled before the wipe'
LEAVE = np.full(320, 255, np.int32)
for y in range(320):
    for n in range(WIPE0, NF):
        if ((K[n][y] != K[n - 1][y]) & inreg[y]).any(): LEAVE[y] = n; break
T = []
for n in range(NF):
    t = k565(transform(F[n]))
    live = (n < LEAVE)[:, None] & inreg
    T.append(np.where(live, t, K[n]))
# learn tokens (pixel, E) -> O from the frames where the row still shows the chip
rule = {}
for n in range(NF):
    ys, xs = np.nonzero(T[n] != K[n])
    for y, x in zip(ys, xs):
        assert n < LEAVE[y]
        key = (int(y), int(x), int(K[n][y, x])); o = int(T[n][y, x])
        assert rule.setdefault(key, o) == o, ('inconsistent', key, rule[key], o)
byp = collections.defaultdict(dict)
for (y, x, e), o in rule.items(): byp[(y, x)][e] = o
def apply(k, n):
    out = k.copy()
    for (y, x), m in byp.items():
        v = int(k[y, x])
        if n < LEAVE[y] and v in m: out[y, x] = m[v]
    return out
# panel model: the clip pushes only the pixels that change (frame 0: all); fixups repaint, at a row's leave frame, the
# pixels the clip does not push but that still show MORPHEUSbot art
panel = None; FIX = []; bad = []
for n in range(NF):
    drawn = np.ones((320, 240), bool) if n == 0 else (K[n] != K[n - 1])
    a = apply(K[n], n)
    panel = a.copy() if panel is None else np.where(drawn, a, panel)
    fx = (panel != T[n]) & (LEAVE[:, None] == n)
    for y, x in zip(*np.nonzero(fx)): FIX.append((n, int(y), int(x), int(T[n][y, x])))
    panel = np.where(fx, T[n], panel)
    bad.append(int((panel != T[n]).sum()))
assert not any(bad), bad
print('panel model == target on all %d frames (0 px off); leave frames per band %s; fixup px %d' % (NF, sorted(set(LEAVE[LEAVE < 255].tolist())), len(FIX)))
print('changed px per frame (vs raw clip):', [(n, int((T[n] != K[n]).sum())) for n in (0, 15, 50, 70, 73, 74, 75, 76, 77, 78, 79, 80, 84)])
# emit: rows y0..y1, tokens (x, n, e, o) runs of identical (E, O) in x order
ys = sorted({y for (y, x) in byp} | {f[1] for f in FIX}); Y0, Y1 = ys[0], ys[-1]
EP, OP = sorted({e for m in byp.values() for e in m}), sorted({o for m in byp.values() for o in m.values()})
assert len(EP) <= 256 and len(OP) <= 256
EI, OI = {v: i for i, v in enumerate(EP)}, {v: i for i, v in enumerate(OP)}
FC = sorted({f[3] for f in FIX}); assert len(FC) <= 256; FR = []
for f, y, x, c in sorted(FIX):
    if FR and FR[-1][0] == f and FR[-1][1] == y - Y0 and FR[-1][2] + FR[-1][3] == x and FC[FR[-1][4]] == c and FR[-1][3] < 255: FR[-1] = (f, y - Y0, FR[-1][2], FR[-1][3] + 1, FR[-1][4])
    else: FR.append((f, y - Y0, x, 1, FC.index(c)))
assert Y1 - Y0 < 256
rows = []; tok = []
for y in range(Y0, Y1 + 1):
    rows.append(len(tok))
    ent = sorted((x, EI[e], OI[o]) for (yy, x), m in byp.items() if yy == y for e, o in m.items())
    # group into runs of consecutive x with equal (e, o); several E at one pixel give several tokens
    used = [False] * len(ent)
    for i, (x, e, o) in enumerate(ent):
        if used[i]: continue
        used[i] = True; n = 1
        while True:
            j = next((j for j in range(len(ent)) if not used[j] and ent[j] == (x + n, e, o)), None)
            if j is None or n == 255: break
            used[j] = True; n += 1
        tok.append((x, n, e, o))
rows.append(len(tok))
# the tokens must be sorted by x within a row for the runtime early-out
for r in range(len(rows) - 1): tok[rows[r]:rows[r + 1]] = sorted(tok[rows[r]:rows[r + 1]])
L = ['/* GENERATED by tools/port/gen_boot_morpheus.py - do not edit. MORPHEUSbot boot chip (item 4, fw-fix7-1106): clip-colour',
     ' * substitution over the LOCKED LK_S_BOOT clip, see lv_boot_morpheus apply in lv_clip_draw. Flash only, 0 B RAM. */',
     '#pragma once', '#include <stdint.h>',
     '#define LV_BM_Y0 %d' % Y0, '#define LV_BM_Y1 %d' % Y1, '#define LV_BM_NTOK %d' % len(tok),
     'struct LvBmTok { uint8_t x, n, e, o; };',
     'static const uint16_t LV_BM_E[%d] = {%s};' % (len(EP), ','.join('0x%04x' % v for v in EP)),
     'static const uint16_t LV_BM_O[%d] = {%s};' % (len(OP), ','.join('0x%04x' % v for v in OP)),
     'static const uint16_t LV_BM_ROW[%d] = {%s};' % (len(rows), ','.join(map(str, rows))),
     'static const LvBmTok LV_BM_TOK[%d] = {%s};' % (len(tok), ','.join('{%d,%d,%d,%d}' % t for t in tok)),
     '/* per row y0..y1: the clip frame from which the row shows its own pixels again (the clip wipe reached it) */',
     'static const uint8_t LV_BM_LEAVE[%d] = {%s};' % (Y1 - Y0 + 1, ','.join(str(int(min(LEAVE[y], 255))) for y in range(Y0, Y1 + 1))),
     '/* at frame f, repaint runs (x..x+n-1, Y0+dy) with LV_BM_FC[c]: pixels the clip does not push at its wipe but that still show MORPHEUSbot art */',
     'struct LvBmFix { uint8_t f, dy, x, n, c; };',
     'static const uint16_t LV_BM_FC[%d] = {%s};' % (len(FC), ','.join('0x%04x' % v for v in FC)),
     '#define LV_BM_NFIX %d' % len(FR),
     'static const LvBmFix LV_BM_FIX[%d] = {%s};' % (max(1, len(FR)), ','.join('{%d,%d,%d,%d,%d}' % f for f in FR) or '{255,0,0,0,0}')]
open(OUT, 'w').write('\n'.join(L) + '\n')
flash = 2 * len(EP) + 2 * len(OP) + 2 * len(rows) + 4 * len(tok) + (Y1 - Y0 + 1) + 2 * len(FC) + 5 * max(1, len(FR))
print('fixup runs %d, ' % len(FR), end=''); print('rows %d..%d, tokens %d, E %d, O %d, rule px %d -> flash %d bytes -> %s' % (Y0, Y1, len(tok), len(EP), len(OP), len(rule), flash, OUT))
np.save(os.path.join(FD, 'target565.npy'), np.array(T))
