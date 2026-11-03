# Variant H (mock only): variant G with the MORPHEUS home chip rebuilt in CONTROLbot's chip style. Everything but his face
# is copied from CONTROLbot's own pixels, the way GIBSON's pins were copied from SCANbot in F: body, shading, diagonal, header
# wave, lime border and dashes, 'dL'/'Ld' side pins on a 3 px pitch, bottom pins, and the TINY3 label font in CONTROL's dark ink.
# The chips are the same size: CONTROL x165..209 maps onto MORPHEUS x31..75 (dx = -134). Column x75 is kept from G (trace).
# Face: G's locked MORPHEUS screen (23x21, with its glow), moved up 1 px into CONTROL's face-window rows 64..86, with a 1 px
# dark grey (84,88,98) bezel ring (x41..65, y64..86). Label: MORPHEUS in TINY3 on CONTROL's name row (y95..99), centred on
# CONTROL's label centre; CONTROL's own ink is erased to the body first.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_G.py').read().rsplit("if __name__ == '__main__':", 1)[0])
DX = -134; HB = (31, 51, 74, 111)                    # rebuilt columns x31..74 (x75 trace column kept)
H_LABEL = 'MORPHEUS'; H_LY = 95
def hframe(Gf):
    a = Gf.copy(); x0, y0, x1, y1 = HB
    a[y0:y1 + 1, x0:x1 + 1] = Gf[y0:y1 + 1, x0 - DX:x1 - DX + 1]
    # erase CONTROL's label ink (dark, sum < 120) in the label block to the nearest body pixel in the same column
    inkbox = (slice(93, 109), slice(37, 71)); blk = a[inkbox].copy(); ink = blk.astype(int).sum(-1) < 120
    for j in range(blk.shape[1]):
        ok = np.nonzero(~ink[:, j])[0]
        for i in np.nonzero(ink[:, j])[0]: blk[i, j] = blk[ok[np.argmin(np.abs(ok - i))], j]
    inkc = tuple(int(v) for v in np.median(a[inkbox][ink], 0)); a[inkbox] = blk
    # face: G screen x42..64 y66..86 -> y65..85, then 1 px dark grey ring
    a[65:86, 42:65] = Gf[66:87, 42:65]
    a[64, 41:66] = BEZEL_GREY; a[86, 41:66] = BEZEL_GREY; a[64:87, 41] = BEZEL_GREY; a[64:87, 65] = BEZEL_GREY
    # label MORPHEUS, TINY3 4 px pitch, centred on CONTROL's label centre (x174..200 -> 40..66, centre 53)
    lw = len(H_LABEL) * 4 - 1; lx = 53 - lw // 2
    for i, chh in enumerate(H_LABEL):
        m = TINY3[10 + ord(chh) - 65]
        for b in range(15):
            if m & (1 << (14 - b)): a[H_LY + b // 3, lx + i * 4 + b % 3] = inkc
    return a, inkc, (lx, lx + lw - 1)
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Hs'; os.makedirs(fd, exist_ok=True)
    chipm = np.zeros((H, W), bool); x0, y0, x1, y1 = CHIP; chipm[y0:y1 + 1, x0:x1 + 1] = True
    bad = 0; bb = [W, H, -1, -1]; mx = 0; M0 = None
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, inkc, lspan = hframe(Gf)
        dm = (Hf != Gf).any(-1); bad += int((dm & ~chipm).any()); mx = max(mx, int(dm.sum()))
        ys, xs = np.nonzero(dm); bb = [min(bb[0], xs.min()), min(bb[1], ys.min()), max(bb[2], xs.max()), max(bb[3], ys.max())]
        Image.fromarray(up(Hf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Hf)).save('%s/home_tap_faceglow_H_steady_3x.png' % OUT); np.save('/tmp/htp_H_steady.npy', Hf); np.save('/tmp/htp_G_steady21.npy', Gf)
    enc(fd, '%s/home_tap_faceglow_H_steady_3x.mp4' % OUT)
    print('frames', NF, '| frames with H!=G outside MORPHEUS chip box', CHIP, ':', bad, '| changed bbox', bb, '| max px/frame', mx,
          '| label ink', inkc, 'x', lspan)
