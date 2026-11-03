# Variant G (mock only): variant F with MORPHEUS's white face bezel recoloured dark grey. Nothing else changes.
# Bezel = the white ring of the MORPHEUS face window (1x box x40..66 y65..91 minus the screen/chin x42..64 y66..88).
# Dark grey = SEP (84,88,98), the board's existing quiet grey (MORPHEUS outline / lens drop shade, most common dark grey on the board).
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_F.py').read().rsplit("if __name__ == '__main__':", 1)[0])
BEZEL_GREY = SEP
def bezel_mask(F):
    ring = np.zeros((H, W), bool); ring[65:92, 40:67] = True; ring[66:89, 42:65] = False
    return ring & (F.astype(int).min(-1) > 200)
def gframe(F, m): G_ = F.copy(); G_[m] = BEZEL_GREY; return G_
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Gs'; os.makedirs(fd, exist_ok=True); M0 = None; bad = 0; maskvar = 0; bb = [W, H, -1, -1]; tot = set()
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0); m = bezel_mask(F)
        if M0 is None: M0 = m
        maskvar += int((m != M0).any()); Gf = gframe(F, M0)
        dm = (Gf != F).any(-1); bad += int((dm & ~M0).any()); tot.add(int(dm.sum()))
        ys, xs = np.nonzero(dm); bb = [min(bb[0], xs.min()), min(bb[1], ys.min()), max(bb[2], xs.max()), max(bb[3], ys.max())]
        Image.fromarray(up(Gf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Gf)).save('%s/home_tap_faceglow_G_steady_3x.png' % OUT); np.save('/tmp/htp_G_steady.npy', Gf); np.save('/tmp/htp_F_steady21.npy', F)
    enc(fd, '%s/home_tap_faceglow_G_steady_3x.mp4' % OUT)
    print('frames', NF, '| bezel px', int(M0.sum()), '| frames where white-bezel set differs from frame 0:', maskvar,
          '| frames with G!=F outside bezel:', bad, '| changed px/frame', sorted(tot), '| bbox', bb, '| grey', BEZEL_GREY)
