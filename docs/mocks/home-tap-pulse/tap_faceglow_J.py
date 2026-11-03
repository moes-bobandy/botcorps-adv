# Variant J (mock only): variant I with the side and bottom pins of the two top chips (MORPHEUS, CONTROLbot) redrawn in
# TERMINALbot's pin style, stamped from TERMINAL's own pixels (as F stamped GIBSON's pins from SCANbot).
# TERMINAL (1x): side pins = 1 dark shade + 2 lime, 2 tall, 5 px pitch (left x137..139, right x205..207, rows 175+5k);
# bottom pins = 2 wide x 3 tall, 5 px pitch, no shade (x145+5k, rows 253..255).
# Old top-chip pins: side 1 dark + 1 lime, 2 tall, 3 px pitch (16 per side, rows 62..108); bottom 2x2, pitch 3 (12, rows 110..111).
# The top chips' pin strips are 2 px wide (sides) / 2 rows (bottom), so the new 3 px pin sits inside the footprint as
# shade on black + 2 lime over the strip (lime abuts the body, like TERMINAL's right side). Bottom pins cover rows 109..111.
# Rows/columns are chosen so every trace lands on a pin: side rows 65..105 (9 per side, incl. 95-96), bottom x = trace - 15 + 5k.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_I.py').read().rsplit("if __name__ == '__main__':", 1)[0])
T_L = [175 + 5 * k for k in range(16)]; T_B = [145 + 5 * k for k in range(12)]
J_ROWS = [65 + 5 * k for k in range(9)]; J_SIDE = (60, 108); J_BROWS = (109, 111)
J_CHIPS = {  # name: (left cols outer->inner, right cols inner->outer, bottom body x-range, bottom pin x list)
    'MORPHEUS': ((31, 32, 33), (72, 73, 74), (34, 73), [37 + 5 * k for k in range(7)]),
    'CONTROL':  ((165, 166, 167), (206, 207, 208), (168, 207), [171 + 5 * k for k in range(7)])}
def isgreen(p): p = p.astype(int); return (p[..., 1] > 100) & (p[..., 2] < 40) & (p[..., 0] < 200)
def j_bands():
    m = np.zeros((H, W), bool)
    for L, R, (bx0, bx1), _ in J_CHIPS.values():
        m[J_SIDE[0]:J_SIDE[1] + 1, L[0]:L[2] + 1] = True; m[J_SIDE[0]:J_SIDE[1] + 1, R[0]:R[2] + 1] = True
        m[J_BROWS[0]:J_BROWS[1] + 1, bx0:bx1 + 1] = True
    return m
J_BANDS = j_bands()
def jframe(If):
    a = If.copy(); src = If.copy(); g = isgreen(src)
    for L, R, (bx0, bx1), BX in J_CHIPS.values():
        for x in L + R:   # side: erase old pins column-wise to the nearest non-pin row of the same column
            y0, y1 = J_SIDE; ok = np.nonzero(~g[y0:y1 + 1, x])[0] + y0
            for y in np.nonzero(g[y0:y1 + 1, x])[0] + y0: a[y, x] = src[ok[np.argmin(np.abs(ok - y))], x]
        for y in range(J_BROWS[0], J_BROWS[1] + 1):   # bottom: row-wise
            ok = np.nonzero(~g[y, bx0:bx1 + 1])[0] + bx0
            for x in np.nonzero(g[y, bx0:bx1 + 1])[0] + bx0: a[y, x] = src[y, ok[np.argmin(np.abs(ok - x))]]
        for k, y in enumerate(J_ROWS):   # stamp TERMINAL pins
            sy = T_L[k % len(T_L)]
            a[y:y + 2, L[0]:L[2] + 1] = src[sy:sy + 2, 137:140]; a[y:y + 2, R[0]:R[2] + 1] = src[sy:sy + 2, 205:208]
        for k, x in enumerate(BX):
            sx = T_B[k % len(T_B)]; a[109:112, x:x + 2] = src[253:256, sx:sx + 2]
    return a
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Js'; os.makedirs(fd, exist_ok=True); M0 = None
    bad = 0; bb = [W, H, -1, -1]; mx = 0
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, _ = iframe(Hf, Gf); Jf = jframe(If)
        dm = (Jf != If).any(-1); bad += int((dm & ~J_BANDS).any()); mx = max(mx, int(dm.sum()))
        ys, xs = np.nonzero(dm); bb = [min(bb[0], xs.min()), min(bb[1], ys.min()), max(bb[2], xs.max()), max(bb[3], ys.max())]
        Image.fromarray(up(Jf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Jf)).save('%s/home_tap_faceglow_J_steady_3x.png' % OUT); np.save('/tmp/htp_J_steady.npy', Jf); np.save('/tmp/htp_I_steady21.npy', If)
    enc(fd, '%s/home_tap_faceglow_J_steady_3x.mp4' % OUT)
    print('frames', NF, '| frames with J!=I outside the two chips\' pin bands:', bad, '| changed bbox', bb, '| max px/frame', mx)
