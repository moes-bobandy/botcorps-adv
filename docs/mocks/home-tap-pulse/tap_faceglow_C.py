# Variant C (Mobandy, 2026-11-02: "have the bot face glow a bit stronger"). MOCK ONLY.
# ONLY the face square of BOOTbot, CONTROLbot, SCANbot, TERMINALbot glows; B's outer chip halo removed.
#  - glow: soft breathing halo in the face's own colour radiating OUT from the face-square edge over the surrounding
#    black face panel / grey bezel, inside the chip only. alpha = AMAX * ((R+1-d)/R)^2 * s, R = 7 px (1x), never on
#    lime pin/decor dots, labels or the chip outline. Same 1.4 s in-sync raised-cosine breath s as A/B.
#  - face: mild midtone lift x -> 255*(x/255)^(1/(1+0.15 s)); 0 and 255 fixed, so shading ramps, eyes, highlights keep identity.
import sys, os, math, subprocess
exec(open('/workspace/out/home-tap-pulse/tap_pulse.py').read().rsplit("if __name__ == '__main__':", 1)[0])  # board, breath, glow(B), up, enc
FACES = {  # face square (x0, y0, x1, y1 incl, 1x), glow colour (from the face), peak alpha
    'BOOTbot':     ((40, 65, 66, 91),    (147, 254, 0),   0.80),   # white-framed screen; lime per brief
    'CONTROLbot':  ((178, 65, 196, 83),  (147, 254, 0),   0.80),   # lime face
    'SCANbot':     ((48, 178, 86, 216),  (245, 180, 73),  0.80),   # gold highlight
    'TERMINALbot': ((154, 178, 192, 216), (226, 236, 250), 0.95),  # face highlight silver (B's 197,205,212 read faintest)
}
RC, GAMMA = 7, 0.15
fl = base0 = board(preps3('green'), 0).astype(int)
lime = (fl[..., 1] > 200) & (fl[..., 2] < 60) & (fl[..., 0] < 170)
dark = fl.sum(-1) < 90; greyb = (np.ptp(fl, -1) < 40) & (fl.sum(-1) < 450) & ~dark
FG = []; facem = np.zeros((H, W), bool)
for k, (b, acc, am) in FACES.items():
    fm = rect(b); facem |= fm; d = distance_transform_edt(~fm)
    ring = (d > 0) & (d <= RC) & chipm[k] & (dark | greyb) & ~lime
    ring &= ~rect((CHIPS[k][0][0], CHIPS[k][0][1], CHIPS[k][0][2], CHIPS[k][0][1] + 9))     # never the header bezel/waveform
    FG.append((ring, np.where(ring, am * ((RC + 1 - d) / RC) ** 2, 0.0), np.array(acc, float)))
region = facem | np.any([g[0] for g in FG], 0)          # the only pixels C may change
def faceglow(base, s):
    a = base.astype(float)
    a[facem] = 255 * (a[facem] / 255) ** (1 / (1 + GAMMA * s))
    for ring, alpha, acc in FG:
        al = (alpha * s)[..., None]; a = np.where(ring[..., None], a * (1 - al) + acc * al, a)
    return a.round().astype(np.uint8)
if __name__ == '__main__':
    Pp = preps3('green'); fc, fs = '/tmp/htp_C', '/tmp/htp_SBS_BC'
    for d in (fc, fs): os.makedirs(d, exist_ok=True)
    from PIL import ImageFont; font = ImageFont.load_default(size=24)
    bad = 0
    for n in range(NF):
        base = board(Pp, n); s = breath(n); c1 = faceglow(base, s); C = up(c1); B = up(glow(base, s))
        bad += int((c1[~region] != base[~region]).any())
        Image.fromarray(C).save('%s/f%03d.png' % (fc, n))
        sbs = np.zeros((996, 1452, 3), np.uint8); sbs[36:, :720] = B; sbs[36:, 732:] = C
        im = Image.fromarray(sbs); dr = ImageDraw.Draw(im)
        dr.text((8, 6), 'B  GLOW (outer chip halo)', fill=(230, 230, 230), font=font); dr.text((740, 6), 'C  FACE GLOW (face only)', fill=(230, 230, 230), font=font)
        im.save('%s/f%03d.png' % (fs, n))
        if n == PERIOD_F // 2: Image.fromarray(C).save(OUT + '/home_tap_faceglow_C_peak_3x.png')
    enc(fc, OUT + '/home_tap_faceglow_C_3x.mp4'); enc(fs, OUT + '/home_tap_sbs_B_C_3x.mp4')
    print('frames with any change outside face-glow regions:', bad, '/', NF, '| ring px', [int(g[0].sum()) for g in FG], '| face px', int(facem.sum()), '| region px', int(region.sum()))
