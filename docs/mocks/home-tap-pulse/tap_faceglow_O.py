# Variant O (mock only, Mobandy 2026-11-03), on top of N (7917cf7): "Gibson also" / "He's clickable" / "And make him like
# the other bots with shade". GIBSONbot is tappable, so:
#  1. Shading: his flat cream face square (1x x96..143 y56..103) gets the same multi-tone ramp as SCAN/TERMINAL. The ramp
#     map is TERMINALbot's own 39x39 face (light from the top-left: near-white bevel on the top rows/left column, light
#     upper-left field, diagonal light stripe, mid field, shadow triangle bottom-right, deep shadow at the right/bottom edge,
#     dark edge column/row), nearest-resampled to 48x48 (TERMINAL's eye holes filled with the nearest tone), and each of
#     TERMINAL's tones replaced by the cream tone at the same step: warm grey shadow -> cream -> near-white highlight.
#     His black shades (every face pixel in rows 0..40 with RGB sum < 650: lenses, bridge, rim, glint) stay pixel-identical.
#  2. Glow: variant C's face glow, unchanged (alpha = 0.80 * ((7+1-d)/7)^2 * s, same breath, in sync), in his face colour
#     cream (247,242,230), from the face square outward; C's midtone lift on the face (shades excluded). Clipped to the
#     black pixels of his face window inside its (84,88,98) frame (x91..148 y52..109): rain never enters there in any
#     frame (checked); the frame line, lime dashes, rain lanes, gaps, label, dim band, pins and border are never touched.
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_N.py').read().rsplit("if __name__ == '__main__':", 1)[0])
G_FACE = (96, 56, 143, 103); G_WIN = (91, 52, 148, 109); G_COL = (247, 242, 230); G_AMAX = 0.80
T_FACE = (154, 178, 192, 216)
# TERMINAL tone anchors -> GIBSON cream tones (same step on the ramp)
T_ANCH = np.array([(255, 254, 255), (230, 238, 247), (197, 205, 212), (139, 153, 165), (105, 119, 131), (40, 54, 66)], float)
G_TONE = np.array([(255, 254, 250), (252, 248, 238), (240, 233, 217), (208, 197, 178), (176, 165, 147), (120, 111, 98)], float)
def o_shade(B):
    a = B.astype(int); tx0, ty0, tx1, ty1 = T_FACE; t = a[ty0:ty1 + 1, tx0:tx1 + 1].astype(float)
    eye = t.sum(-1) < 60
    _, (iy, ix) = distance_transform_edt(eye, return_indices=True); t = t[iy, ix]                  # fill TERMINAL's eye holes
    tone = np.abs(t[..., None, :] - T_ANCH[None, None]).sum(-1).argmin(-1)                          # 39x39 tone index
    gx0, gy0, gx1, gy1 = G_FACE; n = gx1 - gx0 + 1; idx = (np.arange(n) * 39) // n
    shade = G_TONE[tone[idx][:, idx]]                                                                 # 48x48 cream ramp
    g = a[gy0:gy1 + 1, gx0:gx1 + 1]; shades = np.zeros((n, n), bool); shades[:41] = g[:41].sum(-1) < 650
    out = a.copy(); sub = np.where(shades[..., None], g, shade.round().astype(int)); out[gy0:gy1 + 1, gx0:gx1 + 1] = sub
    sm = np.zeros((H, W), bool); sm[gy0:gy1 + 1, gx0:gx1 + 1] = shades
    return out.astype(np.uint8), sm
def o_masks(B, shadesm):
    fl = B.astype(int); fm = rect(G_FACE); d = distance_transform_edt(~fm)
    ring = (d > 0) & (d <= RC) & rect(G_WIN) & (fl.sum(-1) < 90)                                       # black window pixels only
    return fm, fm & ~shadesm, ring, np.where(ring, G_AMAX * ((RC + 1 - d) / RC) ** 2, 0.0)
def oglow(S, s, OM):
    fm, lift, ring, alpha = OM; a = S.astype(float)
    a[lift] = 255 * (a[lift] / 255) ** (1 / (1 + GAMMA * s))
    al = (alpha * s)[..., None]; a = np.where(ring[..., None], a * (1 - al) + np.array(G_COL, float) * al, a)
    return a.round().astype(np.uint8)
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Os'; os.makedirs(fd, exist_ok=True); M0 = None; NM = None; OM = None
    nmis = 0; outside = 0; shade_px = []; glow_face = []; glow_ring = []; prot = 0; shades_chg = 0
    for n in range(NF):
        s = breath(n)
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gb = gframe(f, M0); Hb, _, _ = hframe(Gb); Ib, _ = iframe(Hb, Gb); Bf = jframe(Ib)
        if NM is None: NM = n_masks(Bf)
        Nf = nglow(Bf, s, NM)                                                                             # N (reference)
        nmis += int((up(Nf) != np.array(Image.open('/tmp/htp_Ns/f%03d.png' % n))).any(-1).sum())
        Sf, SH = o_shade(Nf)
        if OM is None:
            OM = o_masks(Nf, SH); fm, lift, ring, _ = OM; okm = fm | ring
            gx0, gy0, gx1, gy1 = G_WIN; PROTG = rect((76, 40, 164, 155)) & ~rect(G_WIN)                    # frame, dashes, rain, label, band, pins, border
        Of = oglow(Sf, s, OM)
        dm = (Of != Nf).any(-1); outside += int((dm & ~okm).sum()); prot += int((dm & PROTG).sum()); shades_chg += int((dm & SH).sum())
        shade_px.append(int(((Sf != Nf).any(-1)).sum())); glow_face.append(int(((Of != Sf).any(-1) & fm).sum())); glow_ring.append(int(((Of != Sf).any(-1) & ring).sum()))
        Image.fromarray(up(Of)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2: Image.fromarray(up(Of)).save('%s/home_tap_faceglow_O_peak_3x.png' % OUT); np.save('/tmp/htp_O_peak.npy', Of); np.save('/tmp/htp_N_peak.npy', Nf)
        if n == 0: np.save('/tmp/htp_O_trough.npy', Of); np.save('/tmp/htp_N_trough.npy', Nf)
    enc(fd, '%s/home_tap_faceglow_O_3x.mp4' % OUT)
    fm, lift, ring, _ = OM
    print('N regen mismatches', nmis, '| G face px', int(fm.sum()), 'shades px kept', int(SH.sum()), 'lift px', int(lift.sum()), 'ring px', int(ring.sum()))
    print('O vs N (168 fr): outside GIBSON face+ring', outside, '| on frame/dashes/rain/gaps/label/band/pins/border', prot, '| on shades', shades_chg)
    print('face shading px/frame', (min(shade_px), max(shade_px)), 'sum', sum(shade_px), '| glow (C lift) face px/frame', (min(glow_face), max(glow_face)), 'sum', sum(glow_face),
          '| glow ring px/frame', (min(glow_ring), max(glow_ring)), 'sum', sum(glow_ring))
