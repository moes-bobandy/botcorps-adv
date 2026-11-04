# Generate src/live/lv_port_data.h: the "port layer" that puts three LOCKED mocks on the live UI over the clip:
#   HOME  ui_home_tap_glow (variant O): MORPHEUS chip, CONTROL chip + pins, GIBSON chip (cream face, small pins, main-bot rain),
#         breathing face glow (lift + ring) on MORPHEUS / CONTROL / GIBSON / SCAN / TERMINAL.
#   SCAN  ui_main_bot_rain SCANbot chip (rain + one-line label) + ui_scan_glow: WIFI / BLE face-only tint, halo glow WIFI / BLE / GIB.
#   TERM  ui_main_bot_rain TERMINALbot chip (rain + one-line label).
# Inputs: tools/port/ext_{home,rain,scan}.py outputs and sim clip dumps of the current firmware (all 12 themes) for the theme map.
# Usage: python gen_port.py EXTDIR CLIPDIR LOCKDIR OUT.h
import sys, math, collections
import numpy as np
from PIL import Image
EXT, CLIPD, LOCK, OUT = sys.argv[1:5]
SNAP = 16
def clip(n, th, k=0): return np.fromfile('%s/%s_%d_%02d.rgb' % (CLIPD, n, th, k), np.uint8).reshape(320, 240, 3)
def k565(a): a = np.asarray(a).astype(np.int64); return (a[..., 0] >> 3) << 11 | (a[..., 1] >> 2) << 5 | (a[..., 2] >> 3)
H = np.load(EXT + '/home.npz'); R = np.load(EXT + '/rain.npz'); S = np.load(EXT + '/scan.npz')
g200 = np.array(Image.open(LOCK + '/ui_home_tap_glow/src/workspace/out/morpheus-rename/frames/g200.png').convert('RGB'))[1::3, 1::3]
T0 = int(H['T0']); steps_of_n = [(T0 + n * 1000 // 30 // 140) % 10 for n in range(168)]
PAL = []; PALI = {}          # entries: (tuple of 6 RGB888 for themes 0..5, themed)
def pal_add(vals, themed):
    k = (tuple(vals), themed)
    if k not in PALI: PALI[k] = len(PAL); PAL.append(k)
    return PALI[k]
class Theme:
    """clip colour (theme 2) -> per theme colour, positional over the screen's own clip dumps (static px only)."""
    def __init__(s, nm):
        cs = [np.array([clip(nm, t, k) for k in range(16)]) for t in range(6)]
        stat = np.ones((320, 240), bool)
        for c in cs: stat &= ~(c != c[:1]).any(-1).any(0)
        stat[:24] = False; stat[270:] = False
        base = cs[2][0][stat]; s.keys = k565(base); s.cols = {}
        for kk, v in zip(*np.unique(s.keys, return_index=True)): s.cols[kk] = base[v]
        s.map = {}
        for t in range(6):
            tv = cs[t][0][stat]; tk = k565(tv)
            d = collections.defaultdict(collections.Counter)
            for a, b, i in zip(s.keys, tk, range(len(tk))): d[a][b] += 1
            for a, c in d.items(): s.map.setdefault(a, [None] * 6)[t] = c.most_common(1)[0][0]
        s.ck = np.array(list(s.cols)); s.cv = np.array([s.cols[k] for k in s.ck]).astype(int)
        s.themed = {a: any(m[t] != m[2] for t in range(6)) for a, m in s.map.items()}
    def entry(s, v, theme_val=None):
        v = tuple(int(x) for x in v)
        d = np.abs(s.cv - np.array(v)).max(-1); i = int(d.argmin())
        if d[i] <= SNAP and s.themed[s.ck[i]]:
            m = s.map[s.ck[i]]; ex = lambda k: (((k >> 11) & 31) * 255 // 31, ((k >> 5) & 63) * 255 // 63, (k & 31) * 255 // 31)
            vals = [ex(m[t]) for t in range(6)]; vals[2] = v
            return pal_add([(a << 16) | (b << 8) | c for a, b, c in vals], True)
        return pal_add([(v[0] << 16) | (v[1] << 8) | v[2]] * 6, False)
    def acc(s, v):
        e = PAL[s.entry(v)]; return e
OWN_S, OWN_R, OWN_G = 1, 2, 3
def build(nm, base, steps, glows, own_extra=None):
    """base: lock base 1x; steps: (10,320,240,3) lock board per rain step (None = no rain/static from base);
       glows: list of (mask, op, alpha, acc888, under?) op 0 lift, 1 ring, 2 face tint."""
    th = Theme(nm)
    code = np.zeros((320, 240), np.int64); rain = []; glow = []
    D = (steps != base[None]).any(-1).any(0) if steps is not None else np.zeros((320, 240), bool)
    if own_extra is not None: D |= own_extra
    V = (steps != steps[:1]).any(-1).any(0) if steps is not None else np.zeros((320, 240), bool)
    gm = np.zeros((320, 240), bool)
    for m, op, al, acc, _ in glows: assert not (gm & m).any(); gm |= m
    assert not (gm & V).any()
    ys, xs = np.nonzero(D & ~V & ~gm)
    for y, x in zip(ys, xs): code[y, x] = OWN_S << 14 | th.entry(steps[0][y, x])
    ys, xs = np.nonzero(V)
    for y, x in zip(ys, xs):
        code[y, x] = OWN_R << 14 | len(rain); rain.append([pal_add([(int(c[0]) << 16) | (int(c[1]) << 8) | int(c[2])] * 6, False) for c in steps[:, y, x]])
    accs = []
    for m, op, al, acc, _ in glows:
        acc = tuple(int(round(float(a))) for a in acc)
        ai = th.entry(acc) if op != 2 else pal_add([(acc[0] << 16) | (acc[1] << 8) | acc[2]] * 6, False)
        ys, xs = np.nonzero(m)
        for y, x in zip(ys, xs):
            b = 0xFFFF if not D[y, x] else th.entry(steps[0][y, x])
            code[y, x] = OWN_G << 14 | len(glow); glow.append((np.float32(al[y, x]) if op == 1 else np.float32(0), b, ai, op))
    assert len(rain) < 16384 and len(glow) < 16384
    own = code != 0
    # regions: bounding boxes of 8-connected groups (dilated by 6 px)
    from scipy.ndimage import label, find_objects, binary_dilation
    lab, n = label(binary_dilation(own, iterations=6), structure=np.ones((3, 3)))
    regs = []
    for sl in find_objects(lab):
        sub = own[sl]; yy, xx = np.nonzero(sub); y0, y1 = sl[0].start + yy.min(), sl[0].start + yy.max(); x0, x1 = sl[1].start + xx.min(), sl[1].start + xx.max()
        regs.append((x0, y0, x1, y1))
    # anim runs: rain px, glow px with op 0/1
    def runs(mask):
        out = []
        for y in range(320):
            row = mask[y]; x = 0
            while x < 240:
                if row[x]:
                    e = x
                    while e + 1 < 240 and row[e + 1]: e += 1
                    out.append((y, x, e - x + 1)); x = e + 1
                else: x += 1
        return out
    anim_g = np.zeros((320, 240), bool)
    for m, op, al, acc, _ in glows:
        if op != 2: anim_g |= m
    return dict(nm=nm, code=code, regs=regs, rain=rain, glow=glow, rr=runs(V), gr=runs(anim_g), own=own, D=D, V=V, G=gm, th=th)
# ---- HOME
X = H['X']; Xs = np.array([X[steps_of_n.index(k)] for k in range(10)])
ownx = np.zeros((320, 240), bool); ownx[45:114, 30:76] = True       # MORPHEUS replaces the grey BOOTbot chip rect (LV_R_BB_HOME) whole
hg = []
for i in range(len(H['names'])):
    hg.append((H['lift'][i], 0, None, H['acc'][i], True)); hg.append((H['ring'][i], 1, H['alpha'][i], H['acc'][i], True))
home = build('home', g200, Xs, hg, ownx)
# ---- SCAN (rain chip + glow lock); TERM (rain chip)
sg = []
for i, nmc in enumerate(S['names']):
    if nmc in ('WIFI', 'BLE'): sg.append((S['face'][i], 2, None, S['cols'][i], True))
    sg.append((S['ring'][i], 1, S['alpha'][i], S['colour'][i], True))
ab = lambda st: np.roll(st, T0 % 10, axis=0)   # rain.npz stacks are chips3(T0 + i): index i -> absolute step (T0 + i) % 10
scan = build('scan', R['scanin_green'], ab(R['scan_green']), sg)
term = build('term', R['termin_green'], ab(R['term_green']), [])
SC = [home, scan, term]
# ---- validation of the theme map against the locked ORANGE rain boards (theme 5)
for sc, key in ((scan, 'scan'), (term, 'term')):
    o = R[key + '_orange'][0]; m = sc['D'] & ~sc['V'] & ~sc['G']; ok = 0; tot = 0; md = []
    for y, x in zip(*np.nonzero(m)):
        v = PAL[sc['code'][y, x] & 16383][0][5]; pv = np.array([(v >> 16) & 255, (v >> 8) & 255, v & 255]); d = int(np.abs(pv - o[y, x]).max()); md.append(d); tot += 1; ok += d <= 16
    rv = (R[key + '_orange'][:, sc['V']] == R[key + '_green'][:, sc['V']]).all(-1).mean()
    print('%s theme map vs locked orange: static px %d, within 16: %.3f, p50/p95 %s; rain px green==orange: %.3f' % (key, tot, ok / tot, np.percentile(md, [50, 95]), rv))
# ---- emit
S42 = [np.float32(0.5 - 0.5 * math.cos(2 * math.pi * n / 42)) for n in range(42)]
assert all(np.float32(H['S'][n]) == S42[n % 42] for n in range(168))
L = ['/* GENERATED by tools/port/gen_port.py - do not edit. Port layer data for three LOCKED mocks (see lv_port.h). */', '#pragma once', '#include <stdint.h>']
L.append('#define LV_PORT_NPAL %d' % len(PAL))
L.append('static const uint32_t LV_PORT_PAL[%d][6] = {' % len(PAL)); L += ['{%s},' % ','.join('0x%06x' % v for v in e[0]) for e in PAL]; L.append('};')
L.append('static const uint8_t LV_PORT_PTHEMED[%d] = {%s};' % (len(PAL), ','.join(str(int(e[1])) for e in PAL)))
L.append('static const float LV_PORT_S[42] = {%s};' % ','.join(repr(float(s)) + 'f' for s in S42))
L.append('#define LV_PORT_GAMMA %sf' % repr(float(H['gamma'])))
L.append('#define LV_PORT_RAIN_T0 %d' % T0)
mx = 0
for i, sc in enumerate(SC):
    p = 'LV_PORT_%s_' % sc['nm'].upper()
    for j, (x0, y0, x1, y1) in enumerate(sc['regs']):
        c = sc['code'][y0:y1 + 1, x0:x1 + 1].ravel()
        L.append('static const uint16_t %sC%d[%d] = {%s};' % (p, j, len(c), ','.join(str(int(v)) for v in c)))
    L.append('static const LvPortReg %sREG[%d] = {%s};' % (p, len(sc['regs']), ','.join('{%d,%d,%d,%d,%sC%d}' % (x0, y0, x1, y1, p, j) for j, (x0, y0, x1, y1) in enumerate(sc['regs']))))
    L.append('static const uint16_t %sRAIN[%d][10] = {%s};' % (p, max(1, len(sc['rain'])), ','.join('{%s}' % ','.join(map(str, r)) for r in sc['rain']) or '{0}'))
    L.append('static const LvPortGlow %sGLOW[%d] = {%s};' % (p, max(1, len(sc['glow'])), ','.join('{%sf,%d,%d,%d}' % (repr(float(a)), b, ai, op) for a, b, ai, op in sc['glow']) or '{0,0,0,0}'))
    for nmr, rr in (('RR', sc['rr']), ('GR', sc['gr'])):
        L.append('static const LvPortRun %s%s[%d] = {%s};' % (p, nmr, max(1, len(rr)), ','.join('{%d,%d,%d}' % r for r in rr) or '{0,0,0}'))
    mx = max(mx, len(sc['glow']))
    print(sc['nm'], 'regions', sc['regs'], 'owned', int(sc['own'].sum()), 'static', int((sc['D'] & ~sc['V'] & ~sc['G']).sum()), 'rain', len(sc['rain']), 'glow', len(sc['glow']), 'runs r/g', len(sc['rr']), len(sc['gr']))
L.append('#define LV_PORT_MAXGLOW %d' % mx)
L.append('static const LvPortScr LV_PORT_SCR[3] = {%s};' % ','.join(
    '{LV_PORT_{0}_REG,{1},LV_PORT_{0}_RAIN,LV_PORT_{0}_GLOW,{2},LV_PORT_{0}_RR,{3},LV_PORT_{0}_GR,{4}}'.replace('{0}', sc['nm'].upper()).replace('{1}', str(len(sc['regs']))).replace('{2}', str(len(sc['glow']))).replace('{3}', str(len(sc['rr']))).replace('{4}', str(len(sc['gr']))) for sc in SC))
open(OUT, 'w').write('\n'.join(L) + '\n'); print('palette', len(PAL), 'themed', sum(e[1] for e in PAL), '->', OUT)
np.savez_compressed(EXT + '/port_meta.npz', **{'%s_%s' % (sc['nm'], k): sc[k] for sc in SC for k in ('own', 'D', 'V', 'G')})
