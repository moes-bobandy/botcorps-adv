#!/usr/bin/env python3
"""v5 (Mobandy 11:51 AM ET 'all wood with different colored device emblem'): one saturated emblem colour per device.
18 saturated colours far apart in CIE Lab (see PALETTE), dealt in SCREEN order so towers next to each other on screen get far-apart colours. Detail ink: dark wood-black on light colours, white on dark ones. Outline: 0x1c120c."""
import colorsys, json, os
D = os.path.dirname(os.path.abspath(__file__))
# 18 colours picked by greedy farthest-point search in CIE Lab over saturated HSV candidates (S >= 0.55, V >= 0.8) that are all
# >= 45 deltaE from the wood tones and the outline; min pairwise deltaE 36.4, every hue distinct (rose moved 340 -> 352 deg so it no longer shares crimson's hue) (v5 first try, 20-deg HSV wheel, had 9.5 between two greens)
PALETTE = [("ff4000", "red-orange"), ("ff9500", "orange"), ("ffd400", "yellow"), ("ccc35c", "khaki"), ("ace600", "lime"), ("78cc5c", "leaf green"),
           ("00ff6a", "green"), ("26ffc9", "aquamarine"), ("3dcccc", "teal"), ("00bfff", "sky blue"), ("0080ff", "azure"), ("0033cc", "royal blue"),
           ("0000ff", "blue"), ("b95ccc", "orchid"), ("ff00ff", "magenta"), ("ff00aa", "hot pink"), ("ff0055", "crimson"), ("cc5c6a", "rose")]
NAMES = ["DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
def rgb(h): return tuple(int(h[k:k + 2], 16) for k in (0, 2, 4))
def lab(c):
    v = [x / 255.0 for x in c]; v = [((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in v]
    X = (0.4124 * v[0] + 0.3576 * v[1] + 0.1805 * v[2]) / 0.95047; Y = 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2]; Z = (0.0193 * v[0] + 0.1192 * v[1] + 0.9505 * v[2]) / 1.08883
    f = lambda t: t ** (1 / 3) if t > 0.008856 else 7.787 * t + 16 / 116
    return (116 * f(Y) - 16, 500 * (f(X) - f(Y)), 200 * (f(Y) - f(Z)))
def dE(a, b): return sum((x - y) ** 2 for x, y in zip(lab(rgb(a)), lab(rgb(b)))) ** 0.5
def lum(c): r, g, b = [v / 255.0 for v in c]; return 0.299 * r + 0.587 * g + 0.114 * b
def neighbours(L):
    """v5b: tower pairs that sit next to each other on screen at ANY of 64 sampled idle times (adjacent in screen-x order of the face centres,
    or face centres within 40 px), from the projection model opt18/model18.py (same camera sway + drift as the sim)"""
    import sys, numpy as np; sys.path.insert(0, D + '/opt18'); import model18 as M
    H = {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}; P = set()
    for t in np.linspace(9.0, 9.0 + 0.3 * 319, 64):
        tw = M.place(L['sx'], L['z0'], H, t); c = sorted(((T['x0'] + T['x1']) / 2, T['i']) for T in tw)
        for k in range(len(c) - 1): P.add(tuple(sorted((c[k][1], c[k + 1][1]))))
        for a in c:
            for b in c:
                if a[1] < b[1] and abs(a[0] - b[0]) <= 40: P.add((a[1], b[1]))
    return sorted(P)
def colors(layout=None):
    """v5b: deal the palette so that screen neighbours (neighbours()) get far-apart colours: maximise the smallest neighbour deltaE, then the
    sum of the 5 smallest (deterministic swap search, seed 7). v5a dealt greedily in sx order, which left META / SKIMMER (36.4) side by side."""
    import random
    L = layout or json.load(open(D + '/layout18.json')); NB = neighbours(L); rng = random.Random(7)
    DM = [[dE(PALETTE[a][0], PALETTE[b][0]) for b in range(18)] for a in range(18)]
    def sc(asg): v = sorted(DM[asg[a]][asg[b]] for a, b in NB); return (round(v[0], 3), round(sum(v[:5]), 3))
    best = list(range(18)); bs = sc(best)
    for r in range(40):
        asg = list(range(18)); rng.shuffle(asg); cs = sc(asg)
        for it in range(3000):
            a, b = rng.sample(range(18), 2); asg[a], asg[b] = asg[b], asg[a]; ns = sc(asg)
            if ns >= cs: cs = ns
            else: asg[a], asg[b] = asg[b], asg[a]
        if cs > bs: bs, best = cs, asg[:]
    out = [None] * 18
    for i in range(18):
        k = best[i]; c = rgb(PALETTE[k][0]); det = (0x1c, 0x12, 0x0c) if lum(c) > 0.55 else (0xff, 0xff, 0xff)
        out[i] = dict(device=NAMES[i], slot=k, hue_deg=int(round(colorsys.rgb_to_hsv(*[v / 255.0 for v in c])[0] * 360)), name=PALETTE[k][1], body=PALETTE[k][0], detail='%02x%02x%02x' % det)
    out[0]['_neighbour_min_dE'] = bs[0]; return out
def c_tables(cols):
    return ('static const uint32_t MO_EMB_COL[MO_N] = {%s};   /* v5: per-device emblem colour (src/emb_colors.py) */\n' % ', '.join('0x' + c['body'] for c in cols) +
            'static const uint32_t MO_EMB_DET[MO_N] = {%s};\n' % ', '.join('0x' + c['detail'] for c in cols))
if __name__ == '__main__':
    for c in colors(): print(c)
