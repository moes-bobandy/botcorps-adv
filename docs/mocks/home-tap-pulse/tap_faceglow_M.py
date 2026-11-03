# Variant M (mock only, Mobandy 2026-11-03): L's recolouring rejected -> "Try again without changing the color".
# M = J (1a24a83) + a heavy halo only. Every face pixel keeps EXACTLY its J colour (no brighten/whiten/hue shift).
#  - Halo colour per bot is sampled from his J face (frame 0): median of the bright face pixels for CONTROL (lime), SCAN (gold),
#    TERMINAL (silver); for MORPHEUS his J shades-rim colour (132,138,150) carried up to his J glint level (252) in the same hue.
#  - The halo fills only the black face-window ring between face plate and bezel: I(d) = A * exp(-(d-1)^2 / 2 (hs*sz)^2),
#    d = 1x distance to the face plate (1 = touching), so it is strongest at the face and fades to black.
#  - Steady: A fixed every frame; only the halo size breathes (sz = 0.92..1.08 on the 1.4 s cycle).
#  - Hard clip: only window-ring pixels may change; face plates, bezel, border, outline, pins, labels, body, GIBSON untouched.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_L.py').read().rsplit("if __name__ == '__main__':", 1)[0])
M_P = {'CONTROLbot': dict(A=0.95, hs=1.1), 'SCANbot': dict(A=0.95, hs=2.8),
       'TERMINALbot': dict(A=0.95, hs=2.8), 'MORPHEUSbot': dict(A=0.95, hs=1.1)}
def m_colours(Jf):
    a = Jf.astype(float); out = {}
    for k, (x0, y0, x1, y1) in L_FACE.items():
        px = a[y0:y1 + 1, x0:x1 + 1].reshape(-1, 3)
        if k == 'MORPHEUSbot':
            rim = np.array([132, 138, 150.]); assert (np.abs(px - rim).sum(-1) == 0).sum() > 0      # J shades rim present
            glint = px[px.sum(-1) > 600].max(0)                                                        # J glint (252,252,252)
            out[k] = tuple(np.round(rim * glint.max() / rim.max()).astype(int))
        else:
            out[k] = tuple(np.median(px[px.sum(-1) >= 200], 0).round().astype(int))
    return out
def mglow(Jf, n, masks, cols):
    a = Jf.astype(float); sz = 0.92 + 0.16 * breath(n)
    for k, (_, _, _, ring, dface) in masks.items():
        P = M_P[k]; I = P['A'] * np.exp(-(dface[ring] - 1) ** 2 / (2 * (P['hs'] * sz) ** 2))[:, None]
        a[ring] = np.maximum(a[ring], np.array(cols[k], float) * I)
    return np.clip(a.round(), 0, 255).astype(np.uint8)
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Ms'; os.makedirs(fd, exist_ok=True); M0 = None; LM = None; COL = None
    allow = l_allowed(); bots = list(L_WIN); out = tot = face = jmis = 0; halo = {k: [] for k in bots}; other = {k: 0 for k in bots}
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, _ = iframe(Hf, Gf); Jf = jframe(If)
        jmis += int((up(Jf) != np.array(Image.open('/tmp/htp_Js/f%03d.png' % n))).any(-1).sum())   # J regenerated == J frames
        if LM is None: LM = l_masks(Jf); COL = m_colours(Jf)
        Mf = mglow(Jf, n, LM, COL); dm = (Mf != Jf).any(-1)
        out += int((dm & ~allow).sum()); tot += int(dm.sum())
        for k in bots:
            x0, y0, x1, y1 = L_FACE[k]; fb = np.zeros((H, W), bool); fb[y0:y1 + 1, x0:x1 + 1] = True
            ring = LM[k][3]; wx0, wy0, wx1, wy1 = L_WIN[k]; win = np.zeros((H, W), bool); win[wy0:wy1 + 1, wx0:wx1 + 1] = True
            face += int((dm & fb).sum()); halo[k].append(int((dm & ring).sum())); other[k] += int((dm & win & ~fb & ~ring).sum())
        Image.fromarray(up(Mf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Mf)).save('%s/home_tap_faceglow_M_steady_3x.png' % OUT); np.save('/tmp/htp_M_steady.npy', Mf); np.save('/tmp/htp_J_steady21.npy', Jf)
    enc(fd, '%s/home_tap_faceglow_M_steady_3x.mp4' % OUT)
    print('halo colours', COL, '| ring px', {k: int(LM[k][3].sum()) for k in bots})
    print('J regen mismatches %d | M vs J: face-plate px changed %d | outside windows %d | total %d | halo px/frame min..max %s | sum %s | other in-window %s' % (
        jmis, face, out, tot, {k: (min(v), max(v)) for k, v in halo.items()}, {k: sum(v) for k, v in halo.items()}, other))
