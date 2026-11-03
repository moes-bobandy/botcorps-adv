# Variant L (mock only, Mobandy 2026-11-03): K was rejected ("Why does it just whiten") -> "Brighter color plus a colored halo
# of the bot's color spilling around the face". L = J with the glow redone; nothing else changes, GIBSON untouched.
#  - No blend toward white anywhere. The earlier glows (E screen/gain/add, still inside J, and K's screen) are dropped: L starts
#    from the un-glowed faces (same pipeline as J with faceglow off) and brightens each face in HSV: value up, saturation up,
#    hue kept (CONTROL neon lime, SCAN rich gold with his ramp kept since V and S are raised monotonically per pixel) or pulled
#    to the glow hue (TERMINAL ice-blue/cyan, MORPHEUS slate/cyan-blue round his shades; his face edges stay black).
#    Face weight w = exp(-max(d-1,0)^2 / 2 sig^2), d = 1x distance to the eyes / shades, sig = SIGF x face width.
#  - Halo: the black face-window pixels between face and bezel (the dark frame ring, eyes excluded) get the bot's glow colour x
#    I(d) = A * exp(-(d-1)^2 / 2 s^2), d = distance to the face (1 = touching): strong next to the face, fading to black.
#  - Steady: strengths fixed every frame; only the face and halo sizes breathe (x 0.92..1.08 on the 1.4 s cycle).
#  - Hard clip: only face-colour pixels and the window's dark ring change; bezel, border, outline, pins, label, body untouched.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_K.py').read().rsplit("if __name__ == '__main__':", 1)[0])
L_WIN = {'MORPHEUSbot': (42, 65, 64, 85), 'CONTROLbot': (176, 64, 198, 86),        # black face windows (1x, inclusive; inside the bezel)
         'SCANbot': (44, 175, 90, 221), 'TERMINALbot': (150, 175, 196, 221)}
L_FACE = {'MORPHEUSbot': (43, 66, 62, 83), 'CONTROLbot': (178, 65, 196, 83),          # face plates inside them
          'SCANbot': (48, 178, 86, 216), 'TERMINALbot': (154, 178, 192, 216)}
def rgb2hsv(c):
    c = c / 255.0; mx = c.max(-1); mn = c.min(-1); d = mx - mn; h = np.zeros_like(mx); nz = d > 1e-9
    r, g, b = c[..., 0], c[..., 1], c[..., 2]
    i = nz & (mx == r); h[i] = ((g - b)[i] / d[i]) % 6
    i = nz & (mx == g) & (mx != r); h[i] = (b - r)[i] / d[i] + 2
    i = nz & (mx == b) & (mx != r) & (mx != g); h[i] = (r - g)[i] / d[i] + 4
    return h * 60, np.where(mx > 0, d / np.maximum(mx, 1e-9), 0), mx * 255
def hsv2rgb(h, s, v):
    h = (h % 360) / 60; i = np.floor(h).astype(int) % 6; f = h - np.floor(h)
    p, q, t = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    sel = [(v, t, p), (q, v, p), (p, v, t), (p, q, v), (t, p, v), (v, p, q)]
    out = np.zeros(h.shape + (3,))
    for k, (a, b, c) in enumerate(sel): m = i == k; out[m] = np.stack([a[m], b[m], c[m]], -1)
    return out
L_P = {  # face: target hue (None = keep own), hue pull, target S, S gain, target V, V gain, sigma factor; halo: rgb, A, sigma (1x px)
    'CONTROLbot':  dict(hue=None, hp=0.0,  S=1.00, sa=1.0,  V=255, va=1.0,  sig=0.30, halo=(120, 255, 0),  A=0.95, hs=1.3),
    'SCANbot':     dict(hue=None, hp=0.0,  S=0.88, sa=0.85, V=255, va=0.95, sig=0.30, halo=(255, 170, 20), A=0.95, hs=2.2),
    'TERMINALbot': dict(hue=192,  hp=1.0,  S=0.55, sa=1.0,  V=255, va=1.0,  sig=0.30, halo=(40, 200, 255), A=0.95, hs=2.2),
    'MORPHEUSbot': dict(hue=200,  hp=1.0,  S=0.85, sa=1.0,  V=215, va=1.0,  sig=0.10, halo=(30, 150, 255), A=0.90, hs=1.1)}
def l_masks(B):
    km = k_masks(B); a = B.astype(int); out = {}
    for k, (x0, y0, x1, y1) in L_WIN.items():
        fill, deye, wid = km[k]
        win = np.zeros((H, W), bool); win[y0:y1 + 1, x0:x1 + 1] = True
        dark = win & (a.sum(-1) < 60); lab, _ = _label(dark)
        edge_ids = set(np.unique(np.concatenate([lab[y0, x0:x1 + 1], lab[y1, x0:x1 + 1], lab[y0:y1 + 1, x0], lab[y0:y1 + 1, x1]]))) - {0}
        x0f, y0f, x1f, y1f = L_FACE[k]; face = np.zeros((H, W), bool); face[y0f:y1f + 1, x0f:x1f + 1] = True
        ring = np.isin(lab, list(edge_ids)) & ~face                          # dark frame ring between face plate and bezel
        dface = distance_transform_edt(~face)                                # distance to the face plate
        out[k] = (fill, deye, wid, ring, dface)
    return out
def lglow(B, n, masks):
    a = B.astype(float); sz = 0.92 + 0.16 * breath(n)
    for k, (fill, deye, wid, ring, dface) in masks.items():
        P = L_P[k]; sig = P['sig'] * wid * sz
        w = np.exp(-np.maximum(deye[fill] - 1, 0) ** 2 / (2 * sig ** 2))
        h, s, v = rgb2hsv(a[fill])
        if P['hue'] is not None: dh = ((P['hue'] - h + 180) % 360) - 180; h = h + dh * P['hp'] * w
        s = s + np.maximum(P['S'] - s, 0) * P['sa'] * w; v = v + np.maximum(P['V'] - v, 0) * P['va'] * w
        a[fill] = hsv2rgb(h, s, v)
        I = P['A'] * np.exp(-(dface[ring] - 1) ** 2 / (2 * (P['hs'] * sz) ** 2))[:, None]
        a[ring] = np.maximum(a[ring], np.array(P['halo'], float) * I)       # colour x I over black: fades to black, never white
    return np.clip(a.round(), 0, 255).astype(np.uint8)
def l_allowed():
    m = np.zeros((H, W), bool)
    for x0, y0, x1, y1 in L_WIN.values(): m[y0:y1 + 1, x0:x1 + 1] = True
    return m
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Ls'; os.makedirs(fd, exist_ok=True); M0 = None; LM = None
    allow = l_allowed(); bots = list(L_WIN)
    st = {ref: dict(out=0, tot=0, face={k: [] for k in bots}, halo={k: [] for k in bots}, other={k: 0 for k in bots}) for ref in ('J', 'K')}
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, _ = iframe(Hf, Gf); Jf = jframe(If)    # J (reference)
        Kf = kglow(Jf, n, k_masks(Jf))                                                            # K (reference)
        Gb = gframe(f, M0); Hb, _, _ = hframe(Gb); Ib, _ = iframe(Hb, Gb); Bf = jframe(Ib)     # J with no face glow (L base)
        if LM is None: LM = l_masks(Bf)
        Lf = lglow(Bf, n, LM)
        for ref, R in (('J', Jf), ('K', Kf)):
            dm = (Lf != R).any(-1); s = st[ref]; s['out'] += int((dm & ~allow).sum()); s['tot'] += int(dm.sum())
            for k in bots:
                fill, _, _, ring, _ = LM[k]; x0, y0, x1, y1 = L_WIN[k]; win = np.zeros((H, W), bool); win[y0:y1 + 1, x0:x1 + 1] = True
                s['face'][k].append(int((dm & fill).sum())); s['halo'][k].append(int((dm & ring).sum())); s['other'][k] += int((dm & win & ~fill & ~ring).sum())
        Image.fromarray(up(Lf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Lf)).save('%s/home_tap_faceglow_L_steady_3x.png' % OUT); np.save('/tmp/htp_L_steady.npy', Lf); np.save('/tmp/htp_K_steady21.npy', Kf)
    enc(fd, '%s/home_tap_faceglow_L_steady_3x.mp4' % OUT)
    print('masks: fill px', {k: int(LM[k][0].sum()) for k in bots}, '| halo ring px', {k: int(LM[k][3].sum()) for k in bots})
    for ref, s in st.items():
        print('L vs %s: changed px outside face windows (all frames) %d | total %d | per bot face px/frame min..max %s | halo px/frame min..max %s | other in-window px (eyes/shades/highlights) %s' % (
            ref, s['out'], s['tot'], {k: (min(v), max(v)) for k, v in s['face'].items()}, {k: (min(v), max(v)) for k, v in s['halo'].items()}, s['other']))
