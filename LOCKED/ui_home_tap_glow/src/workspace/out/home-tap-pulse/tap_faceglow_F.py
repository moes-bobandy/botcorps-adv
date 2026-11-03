# Variant F (Mobandy, 2026-11-03), on top of variant E. MOCK ONLY, no firmware.
#  1. "Make the pin ui for Gibson look like the rest on the page": GIBSONbot's outer pins (old: 5 px deep x 4 px on a 6 px
#     pitch at the sides, 4 px wide x 5 px on a 6 px pitch at the bottom) are erased to the local board background (each
#     pin pixel copied from the nearest non-pin pixel of the same band column/row) and redrawn in the big-chip
#     (SCANbot / TERMINALbot) style: side pins 2 tall on a 5 px pitch, 3 deep = 1 px dark shade outside + 2 px lime, lime
#     ending 2 px short of the body outline; bottom pins 2 wide x 3 tall on a 5 px pitch, 1 px gap below the outline. The
#     pin pixels themselves are copied from SCANbot's own pins (exact colours incl. the dark shade). Same chip footprint;
#     pins sit so the existing traces (rows 95-96 both sides, columns 112-113 / 126-127 below) still land on a pin, the
#     trace is extended through the freed background pixels so it stays connected. Header wave + inner dashes untouched.
#  2. "I only wanted you to change the face ui not the board to have the bot rain": the top-left chip goes back to the
#     base (g200/hub12) home-board chip exactly (grey body, header wave, lime side pins/dashes, border, white bezel), and
#     ONLY its face screen (1x x42..64 y66..86) gets the locked MORPHEUSbot face (black shaded square, round black shades
#     with lit rim + glint + bridge; same art as variant E, fitted to the morpheus-simple 'home' panel 43..63 x 67..85).
#     Label BOOTbot -> MORPHEUS in the same TINY3 3x5 silk font + light ink the base label uses. No device rain.
#  Face-fill glow (variant E, steady) on all four faces, face colour only.
import sys, os
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_E.py').read().rsplit("if __name__ == '__main__':", 1)[0])
from simple import GEO
MG = GEO['home']; SCR = MG['screen']; PAN = MG['panel']; LC2, LT2 = MG['lc'], MG['ltop']
LBL_ROWS = (101, 107)                          # rows bracketing the base BOOTbot label (ink rows 102..106)
# ---- GIBSON pin geometry (1x)
GL = (76, 80); GR = (159, 163); GYS = (52, 145)       # side bands x, rows (old pins)
GB = (147, 151); GBX = (85, 154)                      # bottom band rows, x
NEW_SIDE_ROWS = [55 + 5 * k for k in range(18)]      # 55..140 (2 tall): pin at 95-96 meets the side traces
NEW_BOT_X = [86 + 5 * k for k in range(14)]          # 86..151 (2 wide): pins at 111-112 / 126-127 meet the traces
L_COLS = (77, 78, 79); R_COLS = (162, 161, 160)      # outer(dark) -> inner(lime), mirrored about x=119.5
B_ROWS = (147, 148, 149)
SCAN_L = [(175 + 5 * k) for k in range(16)]          # SCANbot left pins rows (x31..33), right (x99..101 mirrored)
SCAN_B = [39 + 5 * k for k in range(12)]             # SCANbot bottom pins x (rows 253..255)
def band_mask():
    m = np.zeros((H, W), bool)
    m[GYS[0]:GYS[1] + 1, GL[0]:GL[1] + 1] = True; m[GYS[0]:GYS[1] + 1, GR[0]:GR[1] + 1] = True
    m[GB[0]:GB[1] + 1, GBX[0]:GBX[1] + 1] = True; return m
BANDS = band_mask()
def islime(p): p = p.astype(int); return (p[..., 1] > 200) & (p[..., 2] < 40) & (p[..., 0] < 200)
def istrace(p): p = p.astype(int); return (p[..., 1] > 100) & (p[..., 1] <= 200) & (p[..., 0] < 110) & (p[..., 2] < 40)
def gibson_pins(a):
    a = a.copy(); src = a.copy(); lime = islime(src); bg = ~lime & ~istrace(src)   # background samples: never pin, never trace
    # erase: side bands column-wise from the nearest non-lime row in the band, bottom band row-wise from nearest non-lime column
    for x0, x1 in (GL, GR):
        for x in range(x0, x1 + 1):
            col = lime[GYS[0]:GYS[1] + 1, x]; ok = np.nonzero(bg[GYS[0]:GYS[1] + 1, x])[0] + GYS[0]
            for y in np.nonzero(col)[0] + GYS[0]: a[y, x] = src[ok[np.argmin(np.abs(ok - y))], x]
    for y in range(GB[0], GB[1] + 1):
        row = lime[y, GBX[0]:GBX[1] + 1]; ok = np.nonzero(bg[y, GBX[0]:GBX[1] + 1])[0] + GBX[0]
        for x in np.nonzero(row | (istrace(src[y, GBX[0]:GBX[1] + 1]) & (y >= 150)))[0] + GBX[0]: a[y, x] = src[y, ok[np.argmin(np.abs(ok - x))]]   # old trace stubs re-laid below
    # traces into the pins: extend through the freed pixels with the trace's own colour (sampled just outside)
    for y in (95, 96): a[y, 76] = src[y, 75]; a[y, 163] = src[y, 164]
    for x in (112, 113, 126, 127):
        for y in (150, 151): a[y, x] = src[153, x]
    # redraw in SCANbot's pin style, pixels copied from SCANbot's own pins
    for k, y in enumerate(NEW_SIDE_ROWS):
        sy = SCAN_L[k % len(SCAN_L)]
        for dy in (0, 1):
            for i in range(3): a[y + dy, L_COLS[i]] = src[sy + dy, 31 + i]; a[y + dy, R_COLS[i]] = src[sy + dy, 101 - i]
    for k, x in enumerate(NEW_BOT_X):
        sx = SCAN_B[k % len(SCAN_B)]
        for i, y in enumerate(B_ROWS):
            for dx in (0, 1): a[y, x + dx] = src[253 + i, sx + dx]
    return a
# ---- top-left chip: base body + locked MORPHEUS face + MORPHEUS label
def morpheus_face(fr):
    a = fr.copy(); lens = np.zeros((H, W), bool)
    sx0, sy0, sx1, sy1 = SCR; a[sy0:sy1 + 1, sx0:sx1 + 1] = 0
    px0, py0, px1, py1 = PAN; Wp = px1 - px0
    for y in range(py0, py1 + 1):
        for x in range(px0, px1 + 1):
            t = ((x - px0) + (y - py0)) / Wp
            c = BLACK['hi'] if t < 0.45 or 0.85 <= t < 1.02 else BLACK['dark'] if t >= 1.42 else BLACK['base']
            if y == py0 or x == px0: c = BLACK['top']
            if y == py1 or x == px1: c = BLACK['bot']
            a[y, x] = c
    a[py0 + 1:min(py1 + 1, sy1) + 1, px1 + 1] = BLACK['drop']; a[py1 + 1, px0 + 1:px1 + 2] = BLACK['drop']
    n = len(LENS_SMALL); h = n // 2
    for lcx in LC2:
        for r, row in enumerate(LENS_SMALL):
            for k, ch in enumerate(row):
                if ch == '.': continue
                c = {'F': BLACK_FRAME, 'K': BLACK_LENS, 'G': GLINT}[ch]
                if ch == 'F' and (r + k) > n - 2: c = SEP
                a[LT2 + r, lcx - h + k] = c; lens[LT2 + r, lcx - h + k] = True
    for x in range(LC2[0] + h + 1, LC2[1] - h): a[LT2 + max(1, n // 5), x] = BLACK_FRAME; lens[LT2 + max(1, n // 5), x] = True
    # label: erase base ink (BOOTbot) to the body right above/below it, write MORPHEUS (TINY3) in the same ink
    y0, y1 = LBL_ROWS; sub = fr[y0:y1 + 1, 36:71].astype(int); inkm = sub.sum(-1) > 650
    ink_c = np.median(sub[inkm], 0).round().astype(np.uint8)
    for yy, xx in zip(*np.nonzero(inkm)):
        y, x = yy + y0, xx + 36; a[y, x] = fr[y0 if y <= (y0 + y1) // 2 else y1, x]
    lw = len(LABEL) * 4 - 1; lx = 37
    for i, chh in enumerate(LABEL):
        m = TINY3[10 + ord(chh) - 65]
        for b in range(15):
            if m & (1 << (14 - b)): a[y0 + 1 + b // 3, lx + i * 4 + b % 3] = ink_c
    fill = np.zeros((H, W), bool); fill[py0 + 1:py1, px0 + 1:px1] = True; fill &= ~lens
    return a, fill, (lx, lw, tuple(int(v) for v in ink_c), int(inkm.sum()))
_, MFILL2, LBL2 = morpheus_face(base0)
FACES['MORPHEUSbot'] = (MFILL2, PAN, (LT2 + 2, (LC2[0] + LC2[1]) / 2), 'add'); WF['MORPHEUSbot'] = field('MORPHEUSbot')
FILL = np.any([v[0] for v in FACES.values()], 0)
def fframe(Pp, n):
    fr = board(Pp, n); a, _, _ = morpheus_face(fr); return fr, gibson_pins(a)
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Fs'; os.makedirs(fd, exist_ok=True)
    chipm = np.zeros((H, W), bool); x0, y0, x1, y1 = CHIP; chipm[y0:y1 + 1, x0:x1 + 1] = True
    allow = chipm | BANDS; bb = [W, H, -1, -1]; bad = 0; bad_base = 0; nchg = 0; checked = 0
    for n in range(NF):
        fr, f = fframe(Pp, n); e = faceglow(dframe(Pp, n), 1.0); F = faceglow(f, 1.0)
        dm = (F != e).any(-1); bad += int((dm & ~allow).any()); nchg = max(nchg, int(dm.sum())); checked += 1
        ys, xs = np.nonzero(dm & ~chipm)
        if len(xs): bb = [min(bb[0], xs.min()), min(bb[1], ys.min()), max(bb[2], xs.max()), max(bb[3], ys.max())]
        bad_base += int(((F != fr).any(-1) & ~allow & ~FILL).any())
        Image.fromarray(up(F)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(F)).save('%s/home_tap_faceglow_F_steady_3x.png' % OUT); np.save('/tmp/htp_F_steady.npy', F); np.save('/tmp/htp_E_steady21.npy', e)
    enc(fd, '%s/home_tap_faceglow_F_steady_3x.mp4' % OUT)
    print('frames checked', checked, '| frames with F!=E outside (MORPHEUS chip box U GIBSON pin bands):', bad,
          '| F!=E bbox outside chip box (x0,y0,x1,y1):', bb, '| max changed px/frame', nchg,
          '| frames with F!=base board outside chip+bands+face fills:', bad_base, '| label', LBL2, '| morpheus fill px', int(MFILL2.sum()))
