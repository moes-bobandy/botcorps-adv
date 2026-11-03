# Variant N (mock only, Mobandy 2026-11-03, replying to the variant C video: "Use this one but with right bot ui" +
# "Make sure to use the right morpheus"). N = C's breathing face glow, unchanged, on today's board (variant J, 1a24a83).
#  - Base: the J board with its own (E) steady face-fill glow left off, i.e. J's chips exactly (GIBSON small pins, the right
#    MORPHEUSbot: locked black face + shades + glint, dark grey (84,88,98) face bezel, CONTROL-style body/border/label ink,
#    MORPHEUS + 'bot' label, TERMINAL-style pins; CONTROLbot with TERMINAL-style pins) and the raw face art. No BOOTbot anywhere.
#  - Glow = variant C's code and numbers: alpha = AMAX * ((R+1-d)/R)^2 * s, R = 7 px, AMAX 0.80 (TERMINAL 0.95 as in C),
#    s = C's 1.4 s raised-cosine breath, ring = within 7 px of the face square, inside the chip, on dark/grey pixels only,
#    never lime, never the header strip; face midtone lift x -> 255*(x/255)^(1/(1+0.15 s)). Colours as C: CONTROL lime
#    (147,254,0), SCAN gold (245,180,73), TERMINAL ice-silver (226,236,250).
#  - MORPHEUS (replaces C's BOOTbot entry): same ring formula from his face square (43,66,62,83) in his shades/glint colour
#    (222,232,252) = shades rim (132,138,150) lifted to the glint level 252, same hue (as in M). C's midtone lift is applied
#    only to his shades rim/bridge/glint pixels, so the black face panel stays exactly J's: nothing on the face turns grey-blue.
#  - Protected: chip outline, pins, labels, header and GIBSON are never in the ring (checked per frame).
exec(open('/workspace/out/home-tap-pulse/tap_faceglow_J.py').read().rsplit("if __name__ == '__main__':", 1)[0])
CNS = {}; exec(open('/workspace/out/home-tap-pulse/tap_faceglow_C.py').read().rsplit("if __name__ == '__main__':", 1)[0], CNS)  # C, untouched
N_FACES = dict(CNS['FACES']); del N_FACES['BOOTbot']
N_FACES = {'MORPHEUSbot': ((43, 66, 62, 83), (222, 232, 252), 0.80), **N_FACES}
N_CHIP = {'MORPHEUSbot': 'BOOTbot', 'CONTROLbot': 'CONTROLbot', 'SCANbot': 'SCANbot', 'TERMINALbot': 'TERMINALbot'}
N_WIN = {'MORPHEUSbot': (41, 64, 65, 86), 'CONTROLbot': (176, 64, 198, 86), 'SCANbot': (44, 175, 90, 221), 'TERMINALbot': (150, 175, 196, 221)}
RC, GAMMA = CNS['RC'], CNS['GAMMA']
def n_masks(B):
    fl = B.astype(int)
    lime = (fl[..., 1] > 200) & (fl[..., 2] < 60) & (fl[..., 0] < 170)
    dark = fl.sum(-1) < 90; greyb = (np.ptp(fl, -1) < 40) & (fl.sum(-1) < 450) & ~dark
    out = {}
    for k, (b, acc, am) in N_FACES.items():
        ck = N_CHIP[k]; fm = rect(b); d = distance_transform_edt(~fm)
        ring = (d > 0) & (d <= RC) & chipm[ck] & (dark | greyb) & ~lime
        ring &= ~rect((CHIPS[ck][0][0], CHIPS[ck][0][1], CHIPS[ck][0][2], CHIPS[ck][0][1] + 9))
        lift = fm.copy()
        if k == 'MORPHEUSbot': lift = fm & (fl.sum(-1) >= 330)          # shades rim / bridge / glint only
        out[k] = (fm, lift, ring, np.where(ring, am * ((RC + 1 - d) / RC) ** 2, 0.0), np.array(acc, float))
    return out
def nglow(B, s, NM):
    a = B.astype(float)
    for k, (fm, lift, ring, alpha, acc) in NM.items():
        a[lift] = 255 * (a[lift] / 255) ** (1 / (1 + GAMMA * s))
        al = (alpha * s)[..., None]; a = np.where(ring[..., None], a * (1 - al) + acc * al, a)
    return a.round().astype(np.uint8)
if __name__ == '__main__':
    Pp = preps3('green'); fd = '/tmp/htp_Ns'; os.makedirs(fd, exist_ok=True); M0 = None; NM = None
    bots = list(N_FACES); st = {k: dict(face=0, window=0, body=0) for k in bots}; outside = 0; prot = 0; jmis = 0
    dlt = {k: dict(px=0, mx=0, same=0) for k in bots if k != 'MORPHEUSbot'}; pf = {k: [] for k in bots}
    for n in range(NF):
        s = breath(n)
        _, f = fframe(Pp, n); F = faceglow(f, 1.0)
        if M0 is None: M0 = bezel_mask(F)
        Gf = gframe(F, M0); Hf, _, _ = hframe(Gf); If, _ = iframe(Hf, Gf); Jf = jframe(If)              # J (reference)
        jmis += int((up(Jf) != np.array(Image.open('/tmp/htp_Js/f%03d.png' % n))).any(-1).sum())
        Gb = gframe(f, M0); Hb, _, _ = hframe(Gb); Ib, _ = iframe(Hb, Gb); Bf = jframe(Ib)              # J board, E glow off
        if NM is None:
            NM = n_masks(Bf); lime = (Bf[..., 1] > 200) & (Bf[..., 2] < 60) & (Bf[..., 0] < 170)
            PROT = np.zeros((H, W), bool)                                                                   # outline, pins, labels, header
            for k in bots:
                ck = N_CHIP[k]; x0, y0, x1, y1 = CHIPS[ck][0]; cm = chipm[ck]
                wx0, wy0, wx1, wy1 = N_WIN[k]; below = rect((x0, wy1 + 1, x1, y1)); inner = rect((x0 + 4, y0 + 10, x1 - 4, y1 - 4))
                PROT |= cm & ~rect(N_WIN[k]) & (lime | ~inner | (below & (Bf.astype(int).sum(-1) < 200)))   # lime dashes/pins (not the lime face)
                PROT |= rect((x0, y0, x1, y0 + 9))
            ALLOW = np.zeros((H, W), bool)
            for k in bots: ALLOW |= chipm[N_CHIP[k]]
        Nf = nglow(Bf, s, NM)
        dm = (Nf != Jf).any(-1); outside += int((dm & ~ALLOW).sum())
        gm = (Nf != Bf).any(-1); prot += int((gm & PROT).sum()) + int((dm & PROT).sum())
        for k in bots:
            fm = NM[k][0]; win = rect(N_WIN[k]) & ~fm; body = chipm[N_CHIP[k]] & ~fm & ~win
            st[k]['face'] += int((dm & fm).sum()); st[k]['window'] += int((dm & win).sum()); st[k]['body'] += int((dm & body).sum())
            pf[k].append(int((dm & chipm[N_CHIP[k]]).sum()))
        cb = CNS['board'](Pp, n); cc = CNS['faceglow'](cb, s)                                            # C's own frame
        dC = cc.astype(int) - cb.astype(int); dN = Nf.astype(int) - Bf.astype(int)
        for k in dlt:
            cm = chipm[N_CHIP[k]]; e = np.abs(dC[cm] - dN[cm]).max(-1); nz = (dC[cm] != 0).any(-1) | (dN[cm] != 0).any(-1)
            dlt[k]['px'] += int(nz.sum()); dlt[k]['same'] += int((nz & (e == 0)).sum()); dlt[k]['mx'] = max(dlt[k]['mx'], int(e.max()))
        Image.fromarray(up(Nf)).save('%s/f%03d.png' % (fd, n))
        if n == PERIOD_F // 2: Image.fromarray(up(Nf)).save('%s/home_tap_faceglow_N_peak_3x.png' % OUT); np.save('/tmp/htp_N_peak.npy', Nf); np.save('/tmp/htp_C_peak.npy', cc)
        if n == 0: np.save('/tmp/htp_N_trough.npy', Nf); np.save('/tmp/htp_J0.npy', Jf)
    enc(fd, '%s/home_tap_faceglow_N_3x.mp4' % OUT)
    print('J regen mismatches', jmis, '| ring px', {k: int(NM[k][2].sum()) for k in bots}, '| lift px', {k: int(NM[k][1].sum()) for k in bots})
    print('N vs J (168 frames): outside the four chips', outside, '| on outline/pins/labels/header (vs J or vs base)', prot)
    print('per bot changed px (sum over frames) face/window/body', st)
    print('per bot changed px per frame min..max', {k: (min(v), max(v)) for k, v in pf.items()})
    print('glow delta N vs C (pixels where either changed; identical; max channel diff)', dlt)
