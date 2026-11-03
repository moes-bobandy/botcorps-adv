# Variant K (mock only, Mobandy 2026-11-03: J "They did not glow" -> "Heavy glow"): variant J with a HEAVY, steady face glow
# on the four tappable chips (MORPHEUSbot, CONTROLbot, SCANbot, TERMINALbot). Nothing but the glow changes; GIBSON untouched.
#  - Glow seed = each bot's own eyes (MORPHEUS: his shades). Weight w = exp(-d^2 / (2 sig^2)), d = 1x distance to the nearest
#    eye/shade pixel, sig = SIGF x face width (wide falloff), w = 1 on the pixels touching the eyes (d-1 used). Strength is constant every frame; only the
#    halo size breathes gently (sig x 0.92..1.08 on the 1.4 s cycle).
#  - Colour = the bot's own face colour, brightened ("screen" with a light in that colour, monotonic per channel so ramps keep
#    their order): CONTROL hot lime, SCAN bright gold (ramp kept, blue never < J so never flat yellow), TERMINAL bright
#    silver/white-blue, MORPHEUS additive slate-blue light round the shades (face stays dark at the edges).
#  - Clipped to the face-colour pixels inside each face window (the non-dark component bounded by the black window frame),
#    eyes/shades/glints/pure-white highlights excluded -> border, bezel, outline, pins, label and body cannot change.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_J.py').read().rsplit("if __name__ == '__main__':", 1)[0])
from scipy.ndimage import label as _label
K_WIN = {'MORPHEUSbot': (41, 64, 65, 86), 'CONTROLbot': (176, 63, 198, 85),      # face windows incl. black frame (1x, inclusive)
         'SCANbot': (46, 176, 88, 218), 'TERMINALbot': (152, 176, 194, 218)}
K_SEED = {'MORPHEUSbot': (53, 75), 'CONTROLbot': (187, 75), 'SCANbot': (67, 205), 'TERMINALbot': (173, 205)}  # a face pixel
K_MODE = {  # light colour (0..1 per channel) x strength A for screen; MORPHEUS additive rgb
    'CONTROLbot':  ('screen', np.array([0.90, 1.00, 0.85]), 0.97),
    'SCANbot':     ('screen', np.array([1.00, 0.88, 0.55]), 0.88),
    'TERMINALbot': ('screen', np.array([0.88, 0.95, 1.00]), 1.00),
    'MORPHEUSbot': ('add',    np.array([110., 128., 175.]), 1.00)}
SIGF = {'MORPHEUSbot': 0.14, 'CONTROLbot': 0.28, 'SCANbot': 0.28, 'TERMINALbot': 0.28}
def k_masks(Jf):
    a = Jf.astype(int); out = {}
    for k, (x0, y0, x1, y1) in K_WIN.items():
        nd = np.zeros((H, W), bool); nd[y0:y1 + 1, x0:x1 + 1] = a[y0:y1 + 1, x0:x1 + 1].sum(-1) >= 60
        lab, _ = _label(nd); comp = lab == lab[K_SEED[k][1], K_SEED[k][0]]
        win = np.zeros((H, W), bool); win[y0:y1 + 1, x0:x1 + 1] = True
        if k == 'MORPHEUSbot':
            fill = np.roll(MFILL2, -1, axis=0) & comp                     # H moved his screen up 1 px; fill excludes shades
            eyes = np.zeros((H, W), bool)                                   # seed = the shades (lens art + bridge), shifted up 1
            n_ = len(LENS_SMALL); h_ = n_ // 2
            for lcx in LC2:
                for r, row in enumerate(LENS_SMALL):
                    for kk, ch in enumerate(row):
                        if ch != '.': eyes[LT2 + r - 1, lcx - h_ + kk] = True
            for x in range(LC2[0] + h_ + 1, LC2[1] - h_): eyes[LT2 + max(1, n_ // 5) - 1, x] = True
        else:
            fill = comp & (a.min(-1) < 240)                                 # pure-white highlights stay as they are
            eyes = win & ~comp & (a.sum(-1) < 60); eyes[y0:y0 + 3, :] = False; eyes[y1 - 2:y1 + 1, :] = False
            eyes[:, x0:x0 + 3] = False; eyes[:, x1 - 2:x1 + 1] = False     # eyes = black pixels inside, not the window frame
        out[k] = (fill, distance_transform_edt(~eyes), x1 - x0 - 3)
    return out
def kglow(Jf, n, masks):
    a = Jf.astype(float); sz = 0.92 + 0.16 * breath(n)
    for k, (fill, d, wid) in masks.items():
        sig = SIGF[k] * wid * sz; w = np.exp(-np.maximum(d[fill] - 1, 0) ** 2 / (2 * sig ** 2))[:, None]; c = a[fill]
        mode, L, A = K_MODE[k]
        c = 255 - (255 - c) * (1 - L * A * w) if mode == 'screen' else c + L * A * w
        a[fill] = c
    return np.clip(a.round(), 0, 255).astype(np.uint8)
def k_allowed():
    m = np.zeros((H, W), bool)
    for x0, y0, x1, y1 in K_WIN.values(): m[y0 + 2:y1 - 1, x0 + 2:x1 - 1] = True   # inside the black window frame
    return m
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Ks'; os.makedirs(fd, exist_ok=True); M0 = None; KM = None
    allow = k_allowed(); stats = dict(frames=0, out_win=0, out_fill=0, j_png_mismatch=0, px=[], per_bot={k: 0 for k in K_WIN})
    for n in range(NF):
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, _ = iframe(Hf, Gf); Jf = jframe(If)
        jp = np.array(Image.open('/tmp/htp_Js/f%03d.png' % n))[::3, ::3]; stats['j_png_mismatch'] += int((jp != Jf).any())
        masks = k_masks(Jf)
        if KM is None: KM = masks
        Kf = kglow(Jf, n, masks)
        dm = (Kf != Jf).any(-1); fillany = np.any([v[0] for v in masks.values()], 0)
        stats['frames'] += 1; stats['out_win'] += int((dm & ~allow).sum()); stats['out_fill'] += int((dm & ~fillany).sum()); stats['px'].append(int(dm.sum()))
        for k, v in masks.items(): stats['per_bot'][k] = max(stats['per_bot'][k], int((dm & v[0]).sum()))
        Image.fromarray(up(Kf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2:
            Image.fromarray(up(Kf)).save('%s/home_tap_faceglow_K_steady_3x.png' % OUT); np.save('/tmp/htp_K_steady.npy', Kf); np.save('/tmp/htp_J_steady21.npy', Jf)
        if n == 0: np.save('/tmp/htp_K_f0.npy', Kf)
    enc(fd, '%s/home_tap_faceglow_K_steady_3x.mp4' % OUT)
    print('frames', stats['frames'], '| regenerated J == J PNG frames mismatches:', stats['j_png_mismatch'],
          '| changed px outside face windows (all frames):', stats['out_win'], '| changed px outside face-fill masks:', stats['out_fill'],
          '| changed px/frame min..max', min(stats['px']), max(stats['px']), 'total', sum(stats['px']),
          '| max changed px per bot', stats['per_bot'], '| fill px', {k: int(v[0].sum()) for k, v in KM.items()})
