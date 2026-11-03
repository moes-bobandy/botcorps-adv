# Variant I (mock only): variant H plus a second label line "bot" under MORPHEUS, copied from CONTROLbot's own "bot" glyph
# pixels (ink, row and centring relative to the chip): CONTROL's ink pixels (sum < 120) in 1x rows 101..107, x170..204 are
# copied with dx = -134 (same offset as H's body copy), so "bot" sits exactly where CONTROL has it on his chip. Nothing else changes.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_H.py').read().rsplit("if __name__ == '__main__':", 1)[0])
def bot_mask(Gf):
    m = np.zeros((H, W), bool); blk = Gf[101:108, 170:205].astype(int).sum(-1) < 120; m[101:108, 170 + DX:205 + DX] = blk; return m
def iframe(Hf, Gf):
    a = Hf.copy(); m = bot_mask(Gf); src = np.roll(Gf, DX, axis=1); a[m] = src[m]; return a, m
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Is'; os.makedirs(fd, exist_ok=True); M0 = None
    bad = 0; mvar = 0; bb = [W, H, -1, -1]; cnt = set()
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, bm = iframe(Hf, Gf)
        if n == 0: BM0 = bm
        mvar += int((bm != BM0).any())
        dm = (If != Hf).any(-1); bad += int((dm & ~bm).any()); cnt.add(int(dm.sum()))
        ys, xs = np.nonzero(dm); bb = [min(bb[0], xs.min()), min(bb[1], ys.min()), max(bb[2], xs.max()), max(bb[3], ys.max())]
        Image.fromarray(up(If)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(If)).save('%s/home_tap_faceglow_I_steady_3x.png' % OUT); np.save('/tmp/htp_I_steady.npy', If); np.save('/tmp/htp_H_steady21.npy', Hf)
            cm = BM0.copy(); cm = np.roll(cm, -DX, axis=1); ctrl_eq = bool((If[bm] == Gf[cm]).all())
    enc(fd, '%s/home_tap_faceglow_I_steady_3x.mp4' % OUT)
    ys, xs = np.nonzero(BM0)
    print('frames', NF, '| bot glyph px', int(BM0.sum()), 'bbox', (xs.min(), ys.min(), xs.max(), ys.max()), '| glyph mask varies in', mvar,
          'frames | frames with I!=H outside glyph px:', bad, '| changed px/frame', sorted(cnt), '| changed bbox', bb, '| frame21 ink == CONTROL ink:', ctrl_eq)
