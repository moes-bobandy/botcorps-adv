# Variant E (Mobandy, 2026-11-03: "The boarder around the chip is glowing. Make only the bot color right beyond the
# glasses pulse" -> "Not pulse but glow"), on top of the D brief ("Morpheus needs to be the new Morpheus ui we locked in").
# MOCK ONLY, no firmware.
#  1. Top-left home chip (old BOOTbot look, MORPHEUSbot's slot, 1x x31..75 y51..111) redrawn in the LOCKED MORPHEUSbot look
#     (LOCKED/ui_glossary_morpheus_rain = rain_chip_black_soft_final on top of LOCKED/ui_glossary_morpheus), fitted to the
#     same footprint: pins/header wave/top+bottom lime dashes kept where they are, grey body -> black, 1px quiet grey
#     outline (84,88,98) on the body edge and round the face window, black shaded face panel + small round black shades with
#     bridge (morpheus_chip() art, LENS_SMALL), toned-down device rain (firmware DEV_ART/DEV_NEON, half scale 4x4 cells,
#     lead 80 % trails 65..45 %, 30 % behind the label, whole icons only), label "MORPHEUS" (the locked chip's name) in the
#     home 3x5 silk font (TINY3, same as TERMINALbot's label) in the locked label ink.
#  2. No outward glow anywhere. Only each bot's own face-colour fill pixels glow (inner, brighter toward the eyes, colour
#     true): CONTROL lime (screen with lime light), SCAN gold (hue-locked gain, max channel <= 250), TERMINAL silver
#     (hue-locked gain, max channel <= 232 so never near-white), MORPHEUS black glass (additive light in his own panel
#     rim slate 96,100,110). Eyes, glasses, glints, pure-white highlights unchanged.
#     steady: constant glow. breathe: same glow gently 75..100 % on the 1.4 s cycle.
import sys, os, math, random, subprocess
exec(open('/workspace/out/home-tap-pulse/tap_pulse.py').read().rsplit("if __name__ == '__main__':", 1)[0])
sys.path.insert(0, '/workspace/out/morpheus-simple/tool')
from simple import BLACK, BLACK_FRAME, BLACK_LENS, GLINT, LENS_SMALL
src = open('/workspace/cyd-portrait-wt/src/rain/botcorps_rain.cpp').read()
ART = [l.split('"')[1::2] for l in src.split('DEV_ART[DEV_N][8][9] = {')[1].split('};')[0].strip().splitlines()]
NEON = [int(v, 16) for v in src.split('DEV_NEON[DEV_N][3] = {')[1].split('};')[0].replace('{', '').replace('}', '').replace('\n', '').split(',') if v.strip()]
def c565(v): return ((v >> 11 & 31) * 255 // 31, (v >> 5 & 63) * 255 // 63, (v & 31) * 255 // 31)
INK = [[c565(NEON[i * 3 + k]) for k in range(3)] for i in range(10)]
def half(art):   # 8x8 -> 4x4: a 2x2 block is ink if >=2 of its 4 px are ink; colour = most common ink letter (ties -> darker 'a')
    out = []
    for r in range(4):
        row = ''
        for c in range(4):
            px = [art[2 * r + i][2 * c + j] for i in (0, 1) for j in (0, 1)]; ink = [p for p in px if p != '.']
            row += '.' if len(ink) < 2 else max('abc', key=lambda k: (ink.count(k), -'abc'.index(k)))
        out.append(row)
    return out
ART4 = [half(a) for a in ART]
_s = open('/workspace/cyd-portrait-wt/src/ui/botcorps_ui.cpp').read().split('TINY3[37] = {')[1].split('}')[0]
TINY3 = [int(v, 16) for v in _s.split(',')]
SEP = (84, 88, 98); LABEL_INK = (232, 236, 249); SHADOW = (24, 24, 26)
LEVELS = [0.80, 0.65, 0.60, 0.55, 0.50, 0.45]; LABEL_DIM = 0.30
CHIP = (31, 51, 75, 111)                     # top-left chip footprint incl. pins (unchanged)
BODY = (35, 61, 71, 108)                     # grey body (old BOOTbot) -> black
WIN = (40, 65, 66, 91)                       # face window = old BOOT white bezel square, same place/size
PANEL = (43, 68, 63, 88)                     # black shaded face square (locked big chip insets scaled ~0.43)
LCS, LTOP = (49, 57), 73                     # shades: LENS_SMALL, centres 8 px apart, top at 25 % of the panel
LABEL_Y, LABEL = 102, 'MORPHEUS'
def morpheus_base(fr):
    a = fr.copy().astype(np.uint8); x0, y0, x1, y1 = BODY
    sub = a[y0:y1 + 1, x0:x1 + 1].astype(int)
    lime = (sub[..., 1] > 200) & (sub[..., 2] < 90) & (sub[..., 0] < 200)
    side = np.zeros(lime.shape, bool); side[:, 0:5] = True; side[:, -5:] = True        # side dashes inside the body: locked chip has rain there
    keep = lime & ~(side & (np.arange(y0, y1 + 1)[:, None] >= 64) & (np.arange(y0, y1 + 1)[:, None] <= 91))
    body = np.zeros((H, W), bool); body[y0:y1 + 1, x0:x1 + 1] = ~keep
    a[body] = 0
    edge = np.zeros((H, W), bool); edge[y0, x0:x1 + 1] = edge[y1, x0:x1 + 1] = True; edge[y0:y1 + 1, x0] = edge[y0:y1 + 1, x1] = True
    edge &= body; a[edge] = SEP
    wx0, wy0, wx1, wy1 = WIN; a[wy0:wy1 + 1, wx0:wx1 + 1] = SEP; a[wy0 + 1:wy1, wx0 + 1:wx1] = 0
    px0, py0, px1, py1 = PANEL; Wp = px1 - px0
    for y in range(py0, py1 + 1):                 # morpheus_chip() panel shading
        for x in range(px0, px1 + 1):
            t = ((x - px0) + (y - py0)) / Wp
            c = BLACK['hi'] if t < 0.45 or 0.85 <= t < 1.02 else BLACK['dark'] if t >= 1.42 else BLACK['base']
            if y == py0 or x == px0: c = BLACK['top']
            if y == py1 or x == px1: c = BLACK['bot']
            a[y, x] = c
    a[py0 + 1:py1 + 2, px1 + 1] = BLACK['drop']; a[py1 + 1, px0 + 1:px1 + 2] = BLACK['drop']
    lens = np.zeros((H, W), bool); n = len(LENS_SMALL); h = n // 2
    for lcx in LCS:
        for r, row in enumerate(LENS_SMALL):
            for k, ch in enumerate(row):
                if ch == '.': continue
                c = {'F': BLACK_FRAME, 'K': BLACK_LENS, 'G': GLINT}[ch]
                if ch == 'F' and (r + k) > n - 2: c = SEP
                a[LTOP + r, lcx - h + k] = c; lens[LTOP + r, lcx - h + k] = True
    for x in range(LCS[0] + h + 1, LCS[1] - h): a[LTOP + max(1, n // 5), x] = BLACK_FRAME; lens[LTOP + max(1, n // 5), x] = True
    rainm = body & ~edge; rainm[wy0:wy1 + 1, wx0:wx1 + 1] = False; rainm[py1 + 1, :] &= True
    # label (TINY3, 4 px pitch) + 1px shadow, like RC.label
    ink = np.zeros((H, W), bool); lw = len(LABEL) * 4 - 1; lx = (x0 + x1 + 1) // 2 - (lw + 1) // 2
    for i, chh in enumerate(LABEL):
        m = TINY3[10 + ord(chh) - 65]
        for b in range(15):
            if m & (1 << (14 - b)): ink[LABEL_Y + b // 3, lx + i * 4 + b % 3] = True
    sh = np.zeros_like(ink); sh[1:, 1:] |= ink[:-1, :-1]; sh[1:, :] |= ink[:-1, :]; sh[:, 1:] |= ink[:, :-1]; sh &= ~ink & rainm
    fill = np.zeros((H, W), bool); fill[py0 + 1:py1, px0 + 1:px1] = True; fill &= ~lens   # his face fill round the shades (panel rim excluded)
    return a, rainm, ink, sh, fill, (lx, lw)
# rain cells (4x4, whole icons only): side strips while beside the window, 8 re-spaced columns under it
CELLS = []
for y in range(62, 105, 4):
    xs = [36, 67] if y + 3 < 92 else [36 + round(j * 31 / 7) for j in range(8)]
    for c, x in enumerate(xs): CELLS.append((x, y, c if y + 3 < 92 else 10 + c))
STEPS = 42                                   # 5.6 s loop / 42 = 133 ms per rain step (locked mp4 runs 125 ms), seamless
_r = random.Random(4); SEQ = {k: [_r.randrange(10) for _ in range(STEPS)] for k in list(range(2)) + list(range(10, 18))}
PH = {k: _r.randrange(6) for k in SEQ}
def rain(a, rainm, ink, sh, t):
    for x, y, c in CELLS:
        if rainm[y:y + 4, x:x + 4].mean() < 0.85: continue
        row = (y - 62) // 4; k = (t - row + PH[c]) % len(LEVELS); i = SEQ[c][(t - row) % STEPS]
        for yy, line in enumerate(ART4[i]):
            for xx, chh in enumerate(line):
                X, Y = x + xx, y + yy
                if chh == '.' or not rainm[Y, X]: continue
                f = LEVELS[k] * (LABEL_DIM if LABEL_Y - 1 <= Y <= LABEL_Y + 5 else 1)
                a[Y, X] = [int(v * f) for v in INK[i]['abc'.index(chh)]]
    a[sh] = SHADOW; a[ink] = LABEL_INK
# ---- face fills + glow fields
base0 = board(preps3('green'), 0).astype(np.uint8)
MB, RAINM, LINK, LSH, MFILL, LBL = morpheus_base(base0)
def facefill(fr, box):
    x0, y0, x1, y1 = box; f = fr.astype(int); m = np.zeros((H, W), bool)
    sub = f[y0:y1 + 1, x0:x1 + 1]; m[y0:y1 + 1, x0:x1 + 1] = (sub.sum(-1) >= 60) & (sub.min(-1) < 240)
    eyes = np.zeros((H, W), bool); eyes[y0:y1 + 1, x0:x1 + 1] = sub.sum(-1) < 60
    return m, eyes
FACES = {}
for k, box, mode in (('CONTROLbot', (178, 65, 196, 83), 'screen'), ('SCANbot', (48, 178, 86, 216), 'gain'), ('TERMINALbot', (154, 178, 192, 216), 'gain')):
    m, eyes = facefill(base0, box)
    ey, ex = np.nonzero(eyes); cy, cx = ey.mean(), ex.mean()
    FACES[k] = (m, box, (cy, cx), mode)
FACES['MORPHEUSbot'] = (MFILL, PANEL, (LTOP + 2, (LCS[0] + LCS[1]) / 2), 'add')
YY, XX = np.mgrid[0:H, 0:W]
GP = dict(screen=dict(light=np.array([0.55, 1.0, 0.25]), A=0.70), gain=dict(K={'SCANbot': 0.28, 'TERMINALbot': 0.18}),
         add=dict(light=np.array([96, 100, 110], float), A=0.28))
CAP = {'SCANbot': 250, 'TERMINALbot': 232}
def field(k):
    m, (x0, y0, x1, y1), (cy, cx), mode = FACES[k]; sig = 0.42 * (x1 - x0 + 1)
    return 0.30 + 0.70 * np.exp(-((YY - cy) ** 2 + (XX - cx) ** 2) / (2 * sig ** 2))
WF = {k: field(k) for k in FACES}
def faceglow(fr, g):
    a = fr.astype(float)
    for k, (m, box, ctr, mode) in FACES.items():
        w = (WF[k] * g)[m][:, None]; c = a[m]
        if mode == 'screen': c = 255 - (255 - c) * (1 - GP['screen']['light'] * GP['screen']['A'] * w)
        elif mode == 'gain':
            mx = c.max(1, keepdims=True); gmax = np.maximum(1, CAP[k] / np.maximum(mx, 1))
            c = c * np.minimum(1 + GP['gain']['K'][k] * w, gmax)
        else: c = c + GP['add']['light'] * GP['add']['A'] * w
        a[m] = c
    return np.clip(a.round(), 0, 255).astype(np.uint8)
FILL = np.any([v[0] for v in FACES.values()], 0)
def dframe(Pp, n):     # D/E base: locked board + locked-look MORPHEUSbot chip, no glow
    fr = board(Pp, n); x0, y0, x1, y1 = CHIP; a = fr.copy(); a[y0:y1 + 1, x0:x1 + 1] = MB[y0:y1 + 1, x0:x1 + 1]
    rain(a, RAINM, LINK, LSH, (n * STEPS) // NF); return a
if __name__ == '__main__':
    Pp = preps3('green'); dirs = {'steady': '/tmp/htp_Es', 'breathe': '/tmp/htp_Eb'}
    for d in dirs.values(): os.makedirs(d, exist_ok=True)
    chipm = np.zeros((H, W), bool); x0, y0, x1, y1 = CHIP; chipm[y0:y1 + 1, x0:x1 + 1] = True
    bad_d = bad_e = 0; morph_change = 0
    for n in range(NF):
        fr = board(Pp, n); d = dframe(Pp, n)
        bad_d += int((d[~chipm] != fr[~chipm]).any())
        for v, g in (('steady', 1.0), ('breathe', 0.75 + 0.25 * breath(n))):
            e = faceglow(d, g); bad_e += int((e[~FILL] != d[~FILL]).any())
            Image.fromarray(up(e)).save('%s/f%03d.png' % (dirs[v], n))
            if n == PERIOD_F // 2: Image.fromarray(up(e)).save('%s/home_tap_faceglow_E_%s_3x.png' % (OUT, v)); np.save('/tmp/htp_E_%s.npy' % v, e)
        if n == PERIOD_F // 2: np.save('/tmp/htp_D_21.npy', d)
    for v, dd in dirs.items(): enc(dd, '%s/home_tap_faceglow_E_%s_3x.mp4' % (OUT, v))
    print('D frames changed outside top-left chip:', bad_d, '/', NF, '| E frames changed outside face fills:', bad_e, '/', 2 * NF,
          '| fill px', {k: int(v[0].sum()) for k, v in FACES.items()}, '| label x', LBL)
