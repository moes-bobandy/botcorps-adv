#!/usr/bin/env python3
"""morpheus-mix SPREAD package (2026-11-14): 18 pillars (WIFI + BLUETOOTH removed), spread placement, three fixed style heights.
Samples 320 idle frames on the real sim, picks the idle still as the MEDIAN-representative sample (not the best one), runs the mix script,
renders stills / video / emblem sheet, runs the gates (incl. a second identical run for determinism), writes gate.json.
Sequential on purpose (one sim at a time). REUSE=1 skips the sim runs if their outputs exist."""
import json, os, re, subprocess, sys, hashlib, numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__))); import touch20 as T
R = '/workspace/out/morpheus-mix'; SRC = R + '/src'; W = '/tmp/m18'; REUSE = os.environ.get('REUSE') == '1'
NAMES = ["DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
NP = 18; NEWSET = {"AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"}
THREAT = [0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TN = ["HIGH","MEDIUM","LOW"]; LOOK = [{0:1,1:3,2:2}[t] for t in THREAT]
LN = {1: 'T1_code_glass', 2: 'T2_construct_light', 3: 'T3_sentinel_post'}
J = json.load(open(SRC + '/layout18.json')); G = []; HF = {1: J['H'][0], 3: J['H'][1], 2: J['H'][2]}
def gate(name, ok, **kw): G.append(dict(name=name, pass_=bool(ok), **kw)); print(('PASS ' if ok else 'FAIL ') + name, json.dumps(kw)[:300], flush=True)
def rgb(p): return np.frombuffer(open(p, 'rb').read(), np.uint8).reshape(-1, 320, 240, 3)
def png(a, p, k=1): Image.fromarray(a).resize((240 * k, 320 * k), Image.NEAREST).save(p)
def samp(sim, cwd, tag):
    p = W + '/s_%s.json' % tag
    if REUSE and os.path.exists(p): return json.load(open(p), object_hook=lambda d: {int(k) if k.isdigit() else k: v for k, v in d.items()})
    S = T.sample(sim, cwd, W + '/t_' + tag); json.dump(S, open(p, 'w')); return json.load(open(p), object_hook=lambda d: {int(k) if k.isdigit() else k: v for k, v in d.items()})
# ---------- 0. 320 idle samples (0.3 s apart) on the real sim, then the representative idle still = the MEDIAN sample
SM = samp(W + '/sim', R + '/tree', 'main'); sq, vp, vf = T.analyse(SM)
qual = vf.mean(1) + np.minimum(sq, 60).mean(1) / 60.0          # per sample: mean emblem visible fraction + mean tap square (capped)
CLEAN = np.where((vf.min(1) > 0) & (sq.min(1) > 0))[0]          # v3b: samples where every pillar shows some emblem and keeps a tap cell
POOL = CLEAN if len(CLEAN) >= 32 else np.arange(len(qual))
order = POOL[np.argsort(qual[POOL], kind='stable')]; KREP = int(order[len(order) // 2])   # the median sample of that pool (not the best)
steps = json.load(open(SRC + '/mix.json')); assert steps[4] == 'tick 552' and steps[6] == 'tick 128'
PRE = min(128, 10 * (KREP + 1) - 1); steps[4] = 'tick %d' % (10 * (KREP + 1) - PRE); steps[6] = 'tick %d' % PRE   # video pre-roll before the idle still: up to 128 ticks
json.dump(steps, open(W + '/mix_run.json', 'w'))   # idle dump lands on sample KREP's time (v5d fix: this write had been swallowed by the comment above since 8:22 AM ET, so v4 - v5c always ran the stale 08:22 file = sample 251)
print('representative idle sample', KREP, 'of', len(SM), 'quality', round(float(qual[KREP]), 4), 'range', round(float(qual.min()), 4), round(float(qual.max()), 4), flush=True)
# ---------- 1. main run (+ an identical second run for the determinism gate)
for d in ('run', 'run2'):
    if not (REUSE and os.path.exists(W + '/%s/run.log' % d)):
        subprocess.run([sys.executable, SRC + '/drive.py', W + '/mix_run.json', W + '/' + d], check=True)
log = open(W + '/run/run.log').read()
fr = {n: rgb(W + '/run/%s.rgb' % n)[0] for n in ['idle'] + ['%s_%s' % (a, b) for b in ('deauth','airtag','flock','raven') for a in ('push','search','open','later')]}
png(fr['idle'], R + '/idle_1x.png', 1); png(fr['idle'], R + '/idle_3x.png', 3)
CU = [('deauth', 'DEAUTH', 'high'), ('airtag', 'AIR TAG', 'medium'), ('flock', 'FLOCK', 'low'), ('raven', 'RAVEN', 'new')]
for k, nm, tag in CU:
    if k != 'raven': png(fr['later_' + k], R + '/closeup_%s_%s_3x.png' % (k, tag), 3)   # v5: one close-up per height group (RAVEN stays in the run / video)
blocks = log.split('MOCAM')[1:]
idle_blk = blocks[0]
def motw(blk):
    d = {}
    for m in re.finditer(r'MOTW (\d+) X=(-?[\d.]+) Z=(-?[\d.]+) H=(-?[\d.]+) f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] b=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]', blk):
        g = m.groups(); d[int(g[0])] = dict(X=float(g[1]), Z=float(g[2]), H=float(g[3]), f=list(map(int, g[4:8])), b=list(map(int, g[8:12])))
    return d
def moemb(blk): return {int(m.group(1)): dict(look=int(m.group(2)), threat=m.group(3), hs=float(m.group(4)), e=list(map(int, m.group(5, 6, 7)))) for m in re.finditer(r'MOEMB (\d+) look=(\d) threat=(\w+) hs=([\d.]+) e=\[(-?\d+),(-?\d+),(-?\d+)\]', blk)}
IT = motw(idle_blk); IE = moemb(idle_blk)
gate('presence: 18 pillars placed in idle (WIFI + BLUETOOTH removed)', len(IT) == NP and len(IE) == NP and 'WIFI' not in NAMES and 'BLUETOOTH' not in NAMES, n=len(IT))
gate('assignment (sim read-back): look = threat table for all 18', all(IE[i]['look'] == LOOK[i] and IE[i]['threat'] == TN[THREAT[i]] for i in range(NP)))
# pixel look classifier on the idle frames (best of the idle still + two touch-run frames): face pixels outside the emblem rect
def x565(c): r, g, b = c >> 19 & 31, c >> 10 & 63, c >> 3 & 31; return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))
def classify(a, i, IT, ordr):
    """median colour of pillar i's front face pixels that no nearer pillar's box covers (None if < 15 such pixels)"""
    x0, x1, yt, yb = IT[i]['f']; m = np.zeros((320, 240), bool); m[max(yt + 2, 36):min(yb - 1, 284), max(x0 + 2, 0):min(x1 - 1, 239) + 1] = True
    for j in ordr[ordr.index(i) + 1:]:
        U = IT[j]; m[max(min(U['f'][2], U['b'][2]), 0):U['f'][3] + 1, max(min(U['f'][0], U['b'][0]), 0):max(U['f'][1], U['b'][1]) + 1] = False
    reg = a[m].astype(int)
    if len(reg) < 15: return None, len(reg)
    # T3 is SEE-THROUGH wood since 2026-11-14 (whatever stands behind shows through the face), so it is identified by its own opaque
    # wood-grain / brass pixels (warm r > g > b, >= 25% of the unoccluded face) instead of the face median; T1 green glass and T2 white light have none
    wood = ((reg[:, 0] > reg[:, 1]) & (reg[:, 1] > reg[:, 2]) & (reg[:, 0] - reg[:, 2] >= 20) & (reg.sum(1) < 600)).mean()   # warm wood / brass pixels
    if wood >= 0.25: return 3, len(reg)
    # v4: T2 white and T3 wood are both SEE-THROUGH, so a face shows whatever stands behind it. Identify each pillar by its OWN opaque
    # pixels: the 1 px outline of its file text has an exact per-look colour (T1 0x010805, T2 0xffffff, T3 0x1c120c)
    oc = {k: int((reg == np.array(x565(c))).all(1).sum()) for k, c in ((1, 0x010805), (2, 0xffffff), (3, 0x1c120c))}
    kb = max(oc, key=oc.get)
    if oc[kb] >= 6 and oc[kb] >= 3 * max(v for k, v in oc.items() if k != kb): return kb, len(reg)
    r, g, b = np.median(reg, 0); lum = (r + g + b) / 3
    if g > r and g >= b and lum < 110: return 1, len(reg)
    # v4: T2 is SEE-THROUGH white light too: its translucent white front lifts whatever is behind toward white, and its file text
    # carries an opaque pure-white outline, so >= 15 % near-white pixels (min channel > 200) identify it
    if lum > 170 and abs(r - g) < 40: return 2, len(reg)
    if (reg.min(1) > 200).mean() >= 0.15: return 2, len(reg)
    if r > g: return 3, len(reg)
    return 0, len(reg)
# ---------- 2. look / motion / heights / tap / emblem gates on the 320 idle samples
ok_px = 0; bad = []; unc = []; IORD = [int(x) for x in re.search(r'ord=([\d,]+)', idle_blk).group(1).split(',')]
for i in range(NP):
    c, n = classify(fr['idle'], i, IT, IORD)
    if c is None: unc.append(NAMES[i])
    elif c == (1 if LOOK[i] == 1 else 3): ok_px += 1   # v5d: the 5 HIGH towers = T1 black Matrix code glass, the other 13 = see-through wood
    else: bad.append((NAMES[i], c, n))
_blk = [NAMES[i] for i in range(NP) if LOOK[i] == 1]
gate('bodies (v5d): the 5 HIGH towers (%s) read as black Matrix code glass (T1), the other 13 as see-through wood (T3); pixel class from each face\'s own opaque outline / median pixels' % ', '.join(_blk),
     not bad and ok_px >= 14 and len(_blk) == 5 and 'const int L = MO_LOOKOF[i] == 1 ? 1 : 3;' in open(SRC + '/mo_mix.inc').read() and 'mo3_mx(0x03120a, 0x000201, v)' in open(SRC + '/mo_mix.inc').read(),
     black_matrix=_blk, matched=ok_px, misses=bad, unclassifiable_hidden=unc, note='idle still; faces with < 15 unoccluded px are not classified')
# motion: every pillar follows the identical v11 drift law (dx, dz recomputed from the tables), X = X0 + dx, Z = Z0 + dz
import math
lawerr = 0; xzerr = 0
for a in SM:
    t = a['t']
    for i in range(NP):
        Z0 = J['z0'][i]; X0 = (J['sx'][i] - 120) * Z0 / 120.0
        if J.get('drift_law') == 'v5c':   # v5c: each tower its own phase / speed / amplitude in X, Z + lateral sway (src/gibson_depth_drift.py)
            dx = J['ax'][i] * (Z0 / 2) * math.sin(J['wx'][i] * t + J['px'][i]) + J['as'][i] * Z0 * math.sin(J['ws'][i] * t + J['ps'][i]); dz = J['az'][i] * (Z0 / 3) * math.sin(J['wz'][i] * t + J['pz'][i])
        else: dx = J['ax'][i] * (Z0 / 2) * math.sin(J['wx'][i] * t + i * 2.399); dz = J['az'][i] * (Z0 / 3) * math.sin(J['wz'][i] * t + i * 1.713 + 0.9)
        lawerr = max(lawerr, abs(a['tw'][i]['dx'] - dx), abs(a['tw'][i]['dz'] - dz)); xzerr = max(xzerr, abs(a['tw'][i]['X'] - X0 - dx), abs(a['tw'][i]['Z'] - Z0 - dz))
gate('motion: all 18 follow their own drift law exactly (v5c: own phase / speed / amplitude + sway per tower; dx, dz, X, Z recomputed from the tables)', lawerr < 2e-3 and xzerr < 2e-3, max_dxdz_err=round(lawerr, 5), max_xz_err=round(xzerr, 5), samples=len(SM))
# v5d (Mobandy 2:34 PM ET 'Shorten the near towers'): world height = group height x a fixed per-tower depth factor hk (src/v5d_heights.py)
sys.path.insert(0, SRC); import v5d_heights as VH
HK = J['hk']; herr = max(abs(a['tw'][i]['H'] - HF[LOOK[i]] * HK[i]) for a in SM for i in range(NP))
gate('heights (v5d): every tower H = its group height (1.00 / 0.80 / 0.62 x 2.6) x its fixed depth factor hk, in every sample (no breathing)', herr < 2e-3 and all(abs(a['tw'][i]['hk'] - 1.0) < 1e-6 for a in SM for i in range(NP)),
     max_err=round(herr, 5), hk={NAMES[i]: HK[i] for i in range(NP)}, H={NAMES[i]: round(HF[LOOK[i]] * HK[i], 3) for i in range(NP)})
_zs = sorted(range(NP), key=lambda i: J['z0'][i]); _ok = all(HK[_zs[k]] <= HK[_zs[k + 1]] + 1e-9 for k in range(NP - 1))
gate('heights (v5d): the depth factor never grows toward the camera (nearer tower -> smaller or equal hk); group ratios 1.00 / 0.80 / 0.62 kept at every depth', _ok and abs(HF[3] / HF[1] - 0.80) < 1e-3 and abs(HF[2] / HF[1] - 0.62) < 1e-3,
     near_to_far=[(NAMES[i], J['z0'][i], HK[i]) for i in _zs])
hs_med = {i: float(np.median([a['tw'][i]['yb'] - a['tw'][i]['yt'] for a in SM])) for i in range(NP)}; hmax = max(hs_med.values())
gate('no stumps (v5d): every tower\'s median on-screen height >= 45% of the tallest on-screen tower (320 samples)', min(hs_med.values()) >= 0.45 * hmax,
     min_pct=round(100 * min(hs_med.values()) / hmax, 1), shortest=NAMES[min(hs_med, key=hs_med.get)], per_tower_px={NAMES[i]: hs_med[i] for i in range(NP)})
_rows = {}
for i in range(NP):
    v = IT[i]; T_ = dict(x0=v['f'][0], x1=v['f'][1], yt=v['f'][2], yb=v['f'][3]); _rows[NAMES[i]] = VH.rows(T_, IE[i]['e'][2])
# v5e (Mobandy 3:25 PM ET 'icons hover ... come off the tower'): every emblem bolted to its own face, every sampled frame
_viol = []; _off = {}
for i in range(NP):
    ox, oy = [], []
    for a in SM:
        _g = lambda d: d.get(i, d.get(str(i))) if isinstance(d, dict) else d[i]; e = _g(a['em']); t_ = _g(a['tw'])
        if not e or not e['S']: continue
        if e['x'] < t_['x0'] + 1 or e['x'] + e['S'] > t_['x1'] or e['y'] < t_['yt'] or e['y'] + e['S'] > t_['yb']: _viol.append((NAMES[i], a['t']))
        ox.append(e['x'] + e['S'] / 2 - (t_['x0'] + t_['x1']) / 2); oy.append(e['y'] - t_['yt'])
    _off[NAMES[i]] = dict(dx_range=round(max(ox) - min(ox), 2) if ox else None, dy_range=(max(oy) - min(oy)) if oy else None, S=sorted(set((a['em'].get(i) or a['em'].get(str(i)) or {'S': 0})['S'] for a in SM)))
gate('emblems attached (v5e): in every one of the 320 idle samples each emblem rect lies inside its own tower face (>= 1 px inset left / right), its centre-to-face-centre x offset and its top-to-face-top y offset vary by <= 1 px, one fixed size per tower',
     not _viol and all(v['dx_range'] is None or (v['dx_range'] <= 1 and v['dy_range'] <= 1 and len(v['S']) == 1) for v in _off.values()), violations=len(_viol), first=_viol[:5], per_tower=_off)
gate('folder rows (v5d): every tower shows its emblem plus >= 6 on-tower folder rows in the idle still (mo_mix.inc T3 geometry, idle row pitch)', min(_rows.values()) >= 6, rows=_rows)
idle_vis = {}
for i in range(NP):
    s = dict(ord=[int(x) for x in re.search(r'ord=([\d,]+)', idle_blk).group(1).split(',')], tw={k: dict(x0=v['f'][0], x1=v['f'][1], yt=v['f'][2], yb=v['f'][3], bx0=v['b'][0], bx1=v['b'][1], byt=v['b'][2], byb=v['b'][3]) for k, v in IT.items()},
             em={k: dict(x=v['e'][0], y=v['e'][1], S=v['e'][2]) for k, v in IE.items()})
    idle_vis[NAMES[i]] = T.vis(s, i)[0]
vm = vf.mean(0)
gate('emblem visibility (idle still): every pillar shows emblem pixels', min(idle_vis.values()) > 0, px=idle_vis, min=min(idle_vis.values()), mean=round(float(np.mean(list(idle_vis.values()))), 1))
gate('emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5', vm.min() >= 0.5, px_min=float(vp.min()), px_mean=round(float(vp.mean()), 1),
     frac_mean=round(float(vm.mean()), 3), worst=NAMES[int(vm.argmin())], worst_frac=round(float(vm.min()), 3),
     per_pillar={NAMES[i]: dict(frac=round(float(vm[i]), 3), px_mean=round(float(vp[:, i].mean()), 1), px_min=int(vp[:, i].min())) for i in range(NP)})
p10 = np.percentile(sq, 10, axis=0)
gate('tap: every pillar keeps a tap cell in every idle sample (100%)', sq.min() > 0, zero_cell=[NAMES[i] for i in range(NP) if sq[:, i].min() == 0],
     zero_frac={NAMES[i]: round(float((sq[:, i] == 0).mean()), 3) for i in range(NP) if sq[:, i].min() == 0}, min_square={NAMES[i]: int(sq[:, i].min()) for i in range(NP)})
gate('tap: p10 largest square >= 20 px for every pillar', p10.min() >= 20, p10={NAMES[i]: float(p10[i]) for i in range(NP)}, below_20=[NAMES[i] for i in range(NP) if p10[i] < 20],
     median={NAMES[i]: float(np.median(sq[:, i])) for i in range(NP)})
# v5 (Mobandy 11:52 AM ET): no large empty floor patch. Largest empty floor square (pillar boxes + 2 px shadow, box top = min(front, back top))
# in the idle still and over the 320 samples. Two floors are reported: the whole visible floor (rows 140..285) and the standable floor
# (rows 140..251): a base lower than y 251 needs z0 < 0.95, which pushes the tower top off the field (top-clip rule), so rows 252..285 stay bare.
# v5b (12:42 PM ET 'lower-right and front floor empty, left and middle packed'): + base spread: pillar bases per screen-x third and per depth band.
sys.path.insert(0, SRC + '/opt18'); import floor18 as FL
def _emp(boxes, y1):
    o = FL.FY1; FL.FY1 = y1; r = FL.empty_square(boxes); FL.FY1 = o; return r
_bi = [(min(v['f'][0], v['b'][0]), max(v['f'][1], v['b'][1]), min(v['f'][2], v['b'][2]), v['f'][3]) for v in IT.values()]
_bs = [[(min(T_['x0'], T_['bx0']), max(T_['x1'], T_['bx1']), min(T_['yt'], T_['byt']), T_['yb']) for T_ in a_['tw'].values()] for a_ in SM]
emp_idle = _emp(_bi, 285); emp_s = [_emp(b, 285) for b in _bs]; st_idle = _emp(_bi, 251); st_s = [_emp(b, 251) for b in _bs]
_cx = [(v['f'][0] + v['f'][1]) / 2 for v in IT.values()]; _yb = [v['f'][3] for v in IT.values()]
thirds = [sum(1 for x in _cx if x < 80), sum(1 for x in _cx if 80 <= x < 160), sum(1 for x in _cx if x >= 160)]
_fr = [x for x, y in zip(_cx, _yb) if y >= 200]
front_thirds = [sum(1 for x in _fr if x < 80), sum(1 for x in _fr if 80 <= x < 160), sum(1 for x in _fr if x >= 160)]
gate('floor coverage: largest empty standable-floor square <= 60 px (idle + 320-sample median), whole floor <= 75 px, 5-7 bases per screen third and >= 1 front-row base (y >= 200) in each third (v4 107 px, v5a 71 px whole floor)',
     st_idle <= 60 and float(np.median(st_s)) <= 60 and emp_idle <= 75 and float(np.median(emp_s)) <= 75 and all(5 <= n <= 7 for n in thirds) and min(front_thirds) >= 1,
     whole_floor_idle_px=emp_idle, whole_floor_median_px=float(np.median(emp_s)), whole_floor_max_px=int(max(emp_s)),
     standable_floor_idle_px=st_idle, standable_floor_median_px=float(np.median(st_s)), standable_floor_max_px=int(max(st_s)),
     bases_per_third_LMR=thirds, front_row_bases_per_third_LMR=front_thirds, v4_idle_whole_floor_px=107, v5a_idle_whole_floor_px=71, v5a_idle_standable_floor_px=43, v5a_idle_bases_per_third_LMR=[6, 8, 4])
# v5c (Mobandy 2:05 PM ET 'they should not be locked in movement with one another'): pairwise motion independence, src/motion_gate.py
import motion_gate as MG
MOT = MG.motion(SM, NP); MOT['own_drift_max_pair'] = [NAMES[k] for k in MOT['own_drift_max_pair']]
MOT['v5b_before'] = dict(raw_screen_mean_abs_r=0.853, raw_screen_max_abs_r=0.959, own_drift_mean_abs_r=0.147, own_drift_max_abs_r=0.805, depth_max_abs_r=0.839)
gate('motion independence: per-tower motion correlation over 153 pairs - own drift mean |r| <= 0.15 and max <= 0.6, depth mean |r| <= 0.15, raw screen motion (incl. the shared camera sway) mean |r| <= 0.4, >= 90% of pairs change camera-free spacing by >= 8 px (v5b: raw mean 0.853)',
     MOT['own_drift_mean_abs_r'] <= 0.15 and MOT['own_drift_max_abs_r'] <= 0.6 and MOT['depth_mean_abs_r'] <= 0.15 and MOT['raw_screen_mean_abs_r'] <= 0.4 and MOT['spacing_pairs_changing_8px'] >= 0.9 * MOT['pairs'], **MOT)
# ---------- 3. emblems
raw = subprocess.run([W + '/emb_sheet'], capture_output=True, check=True).stdout; off = 0; EMB = {}
for i in range(NP):
    for S in (16, 32, 40):
        b = np.frombuffer(raw[off:off + S * S], np.uint8).reshape(S, S); off += S * S; d = np.frombuffer(raw[off:off + S * S], np.uint8).reshape(S, S); off += S * S
        h = np.frombuffer(raw[off:off + (S + 2) ** 2], np.uint8).reshape(S + 2, S + 2); off += (S + 2) ** 2; EMB[i, S] = (b, d, h)
OLD = [np.frombuffer(raw[off + k * 256: off + (k + 1) * 256], np.uint8).reshape(16, 16) for k in range(10)]
aa = []
for i in range(NP):
    for S in (16, 32, 40):
        for m in EMB[i, S]: aa.append(int(((m > 0) & (m < 255)).sum()))
gate('emblems: crisp (v4: binary masks drawn pixel-aligned at the final size; zero partial-coverage / blurred pixels at 16 / 32 / 40 px)', max(aa) == 0, partial_px_total=int(sum(aa)))
sil = [(EMB[i, 32][0].astype(int) + EMB[i, 32][1] > 127) for i in range(NP)]; mi = 0; pair = None
for a in range(NP):
    for b in range(a + 1, NP):
        iou = (sil[a] & sil[b]).sum() / max(1, (sil[a] | sil[b]).sum())
        if iou > mi: mi, pair = iou, (NAMES[a], NAMES[b])
gate('emblems: unique silhouettes (max pairwise IoU at 32 px < 0.75)', mi < 0.75, max_iou=round(float(mi), 3), pair=pair)
# v5: emblem COLOUR sheet + colour gates
sys.path.insert(0, SRC); import emb_colors as EC
COLS = EC.colors(J); RGBC = [tuple(int(c['body'][k:k + 2], 16) for k in (0, 2, 4)) for c in COLS]; RGBD = [tuple(int(c['detail'][k:k + 2], 16) for k in (0, 2, 4)) for c in COLS]
def lab(c):
    v = [x / 255.0 for x in c]; v = [((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in v]
    X = (0.4124 * v[0] + 0.3576 * v[1] + 0.1805 * v[2]) / 0.95047; Y = 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2]; Z = (0.0193 * v[0] + 0.1192 * v[1] + 0.9505 * v[2]) / 1.08883
    f = lambda t: t ** (1 / 3) if t > 0.008856 else 7.787 * t + 16 / 116
    return np.array([116 * f(Y) - 16, 500 * (f(X) - f(Y)), 200 * (f(Y) - f(Z))])
LB = [lab(c) for c in RGBC]; dE = np.array([[np.linalg.norm(LB[a] - LB[b]) for b in range(NP)] for a in range(NP)]); np.fill_diagonal(dE, 999)
ma, mb = np.unravel_index(dE.argmin(), dE.shape); hues = sorted(c['hue_deg'] for c in COLS); dh = min((hues[(k + 1) % 18] - hues[k]) % 360 for k in range(18))
scr = sorted(range(NP), key=lambda i: (IT[i]['f'][0] + IT[i]['f'][1]) / 2); adj = min(dE[scr[k], scr[k + 1]] for k in range(NP - 1))
ka = min(range(NP - 1), key=lambda k: dE[scr[k], scr[k + 1]])
WOODS = [lab(c) for c in ((0x8a, 0x54, 0x28), (0x4a, 0x2a, 0x12))]; wmin = min(np.linalg.norm(l - w) for l in LB for w in WOODS)
drawn = {}
for i in range(NP):
    ex, ey, eS = IE[i]['e']; blk_ = fr['idle'][max(ey, 0):ey + eS, max(ex, 0):ex + eS].reshape(-1, 3)
    drawn[NAMES[i]] = int((blk_ == np.array(x565(int(COLS[i]['body'], 16)))).all(1).sum())
gate('emblem colours: 18 distinct saturated colours with 18 distinct hues (min hue step >= 5 deg), min pairwise CIE76 deltaE >= 30, screen-adjacent towers deltaE >= 40, every colour deltaE >= 30 from the wood tones, >= 16 of 18 drawn in the idle still',
     len(set(c['body'] for c in COLS)) == 18 and len(set(c['hue_deg'] for c in COLS)) == 18 and dh >= 5 and dE.min() >= 30 and adj >= 40 and wmin >= 30 and sum(1 for v in drawn.values() if v > 0) >= 16,
     min_pair_deltaE=round(float(dE.min()), 1), min_pair=(NAMES[ma], NAMES[mb]), min_hue_step_deg=dh, min_screen_adjacent_deltaE=round(float(adj), 1), adjacent_pair=(NAMES[scr[ka]], NAMES[scr[ka + 1]]),
     min_deltaE_to_wood=round(float(wmin), 1), body_colour_px_in_idle=drawn, colours={c['device']: dict(hex='#' + c['body'], hue=c['hue_deg'], name=c['name']) for c in COLS})
raw24 = subprocess.run([W + '/emb_sheet', '24'], capture_output=True, check=True).stdout; E24 = {}; o_ = 0
for i in range(NP):
    b_ = np.frombuffer(raw24[o_:o_ + 576], np.uint8).reshape(24, 24); o_ += 576; d_ = np.frombuffer(raw24[o_:o_ + 576], np.uint8).reshape(24, 24); o_ += 576
    h_ = np.frombuffer(raw24[o_:o_ + 676], np.uint8).reshape(26, 26); o_ += 676; E24[i] = (b_, d_, h_)
font = ImageFont.load_default(); CW, CH = 300, 150; sheet = Image.new('RGB', (CW * 3, CH * 6 + 40), (30, 26, 22)); dr = ImageDraw.Draw(sheet)
dr.text((10, 10), 'MORPHEUS MIX v5: 18 device emblem colours on see-through wood (crisp binary masks; 16 / 24 / 40 px shown x3 / x3 / x2, nearest)', fill=(232, 222, 200), font=font)
def wood(w, h):
    a = np.zeros((h, w, 3), np.uint8); xs = np.arange(w)
    for y in range(h): a[y] = np.where(np.sin(xs * 0.9 + 1.6 * np.sin(y * 0.11)) > 0.15, 1, 0)[:, None] * np.array([0x8a, 0x54, 0x28]) + np.where(np.sin(xs * 0.9 + 1.6 * np.sin(y * 0.11)) > 0.15, 0, 1)[:, None] * np.array([0x4a, 0x2a, 0x12])
    return a
def comp2(i, m3, k):
    b, d, h = m3; S = b.shape[0]; im = wood(S + 6, S + 6); sub = im[2:S + 4, 2:S + 4]
    sub[h > 0] = (0x1c, 0x12, 0x0c); sub[1:-1, 1:-1][b > 0] = RGBC[i]; sub[1:-1, 1:-1][d > 0] = RGBD[i]
    return Image.fromarray(im).resize(((S + 6) * k, (S + 6) * k), Image.NEAREST)
for i in range(NP):
    x = (i % 3) * CW; y = 40 + (i // 3) * CH; dr.rectangle([x + 4, y + 4, x + CW - 4, y + CH - 4], outline=(90, 80, 66))
    c = COLS[i]; dr.text((x + 10, y + 8), '%s  #%s  %s (%d deg)  [%s]' % (NAMES[i], c['body'], c['name'], c['hue_deg'], TN[THREAT[i]]), fill=RGBC[i], font=font)
    sheet.paste(comp2(i, EMB[i, 16], 3), (x + 10, y + 30)); sheet.paste(comp2(i, E24[i], 3), (x + 80, y + 30)); sheet.paste(comp2(i, EMB[i, 40], 2), (x + 186, y + 30))
sheet.save(R + '/emblem_colors.png')
# close-ups: 40 px emblem on the open pillar, folders landed, pillar >= 50 px wide; threat text; no red pill; scrolling files
cub = [b for b in blocks if 'MOFOLD' in b]; cu_ok = []; 
for k, nm, tag in CU:
    i = NAMES.index(nm); bl = [b for b in cub if re.search(r'MOCU %d ' % i, b)]; b = bl[-1]
    fw = int(re.search(r'MOCU %d .*fw=(\d+)' % i, b).group(1)); nl = int(re.search(r'MOFOLD nland=(\d)', b).group(1)); e = moemb(b).get(i, {}).get('e', [0, 0, 0])
    cu_ok.append(dict(dev=nm, fw=fw, nland=nl, emb_S=e[2], ok=fw >= 50 and nl == 3 and e[2] == 40))
gate('close-ups: pillar >= 50 px wide, 3 folders landed, 40 px emblem drawn', all(c['ok'] for c in cu_ok), closeups=cu_ok)
# v5d (Mobandy 2:36 - 2:38 PM ET): open order = Gibson lock -> HACKbot search (bar types the device, 11-band meter sweeps the tower) -> folders.
# Each tower's itwstate after its search dump (1.8 s) and after its open dump (5.16 s): MOFOLD nland / typed / lit / drop0
sr = []
for k, nm, tag in CU:
    i = NAMES.index(nm); bl = [b for b in cub if re.search(r'MOCU %d ' % i, b)]
    m = [re.search(r'MOFOLD nland=(\d) typed=(\d+)', b).groups() + ('11', '0') for b in bl]
    a_, b_ = [int(v) for v in m[0]], [int(v) for v in m[1]]
    sr.append(dict(dev=nm, search=dict(nland=a_[0], typed=a_[1], lit=a_[2]), open=dict(nland=b_[0], typed=b_[1], lit=b_[2]), folders_drop_at_ms=a_[3],
                   ok=a_[0] == 0 and a_[1] > 0 and b_[0] == 3 and b_[1] == len(nm)))
gate('open order (v5e): search first (0.54 s frame: device being typed, 0 scrolls), scrolls only after the device is fully typed (1.8 s frame: 3 scrolls, full name)', all(x['ok'] for x in sr), towers=sr)
from PIL import Image as _Im
_seq = _Im.new('RGB', (720 * 4, 960))
for k_, nm_ in enumerate(('push_deauth', 'search_deauth', 'open_deauth', 'later_deauth')): _seq.paste(_Im.fromarray(fr[nm_]).resize((720, 960), _Im.NEAREST), (720 * k_, 0))
_seq.save(R + '/open_sequence_deauth_3x.png')   # v5e: 0.18 s push | 0.54 s search typing, no scrolls | 1.8 s scrolls landed | 3.15 s
pass   # v5e: the Gibson pull-up comparison belonged to the rejected v5d open screen   # v5d 2:38 PM: real Gibson charge lock vs the Morpheus open, same ms since the touch
term = {k: fr['later_' + k][287 + 4:287 + 10, 5:200] for k, _, _ in CU}
gate('threat text: small text in the terminal status line; LOW lines identical, HIGH / MEDIUM / LOW differ', (term['flock'] == term['raven']).all() and not (term['deauth'] == term['airtag']).all() and not (term['deauth'] == term['flock']).all() and not (term['airtag'] == term['flock']).all(),
     note='line reads "PILLAR OPEN  THREAT: <LEVEL>  EX.DATA"; no new widget')
red = 0
for k, _, _ in CU:
    for ph in ('push', 'open', 'later'):
        a = fr['%s_%s' % (ph, k)][35:247].astype(int); em_ = np.zeros(a.shape[:2], bool)
        for c_ in COLS: em_ |= (a == np.array(x565(int(c_['body'], 16)))).all(2)   # v5: the red-orange / crimson EMBLEM colours are not a pill
        red += int(((a[..., 0] > 180) & (a[..., 1] < 70) & (a[..., 2] < 70) & ~em_).sum())
gate('no red pill (no saturated red pixels in the field on any close-up frame)', red == 0, red_px=red)
txt_bad = int(re.findall(r'txt_bad_all=(\d+)', log)[-1]); txt_all = int(re.findall(r'txt_lines_all=(\d+)', log)[-1])
gate('text audit: every audited text line inside its box', txt_bad == 0, lines=txt_all, bad=txt_bad)
# 2026-11-14 (Mobandy 9:06 AM ET): real folder text, no placeholders, nothing shared across devices, every string fits its UI slot
import importlib.util as _ilu
_sp = _ilu.spec_from_file_location('folders18', SRC + '/folders18.py'); F18 = _ilu.module_from_spec(_sp); _sp.loader.exec_module(F18)
_ferr = F18.check(); _inc = open(SRC + '/mo_mix.inc').read()
_wired = all(k in _inc for k in ('MO_FOLD_NAME[i][j]', 'MO_FOLD_ACT[i][j]', 'MO_FOLD_TAG[i][j]', 'MO_ITEM_K[i][e]', 'MO_ITEM_V[i][e]')) and 'mo3_size(nm, sz)' not in _inc and 'MO13_ACT[j]' not in _inc and '"%d.%dKB"' not in _inc.split('MO_FOLD_TAG[i][j]')[1][:4000]
_allfold = [f for d in F18.DEV for f in d[3]]; _alltxt = [t for f in _allfold for t in f] + [t for d in F18.DEV for kv in d[4] for t in kv]
_ph = [t for t in _alltxt if F18.BAD.search(t) or 'EX.DATA' in t or 'LOREM' in t]
_dupf = len(set(f[0] for f in _allfold)) != len(_allfold) or len(set(f[1] for f in _allfold)) != len(_allfold)
_dupi = len(set(tuple(d[4]) for d in F18.DEV)) != len(F18.DEV)
_fp = {k: hashlib.sha256(np.ascontiguousarray(fr['later_' + k][60:240, 92:235]).tobytes()).hexdigest()[:12] for k, _, _ in CU}
gate('folder content: all 18 devices have real text (3 folders ID / DETECT / ACTION + 8 KEY / VALUE line-items); no EX.DATA / lorem / fake KB sizes; no folder name, detail line or line-item block shared across devices; every string fits its slot; tables wired into the renderer; close-up folder panels all differ',
     not _ferr and _wired and not _ph and not _dupf and not _dupi and len(set(_fp.values())) == len(_fp),
     devices=len(F18.DEV), folders=len(_allfold), items=sum(len(d[4]) for d in F18.DEV), errors=_ferr, placeholders=_ph, dup_folder_text=_dupf, dup_item_block=_dupi, wired=_wired, closeup_panel_hash=_fp,
     note='scope: the folder + line-item text tables. Chrome text (card / terminal line, incl. its EX.DATA word) is unchanged by order "do not change the chrome text".')
# ---------- v4 (Mobandy 9:27 - 9:29 AM ET): opaque UI, big idle emblems, temporal stability
mui = re.findall(r'MOUI tot=(\d+) part=(\d+)', log)
_src_ui = _inc[_inc.index('static void mo3_t35'):_inc.index('/* device icon with a per-look ink map')] + _inc[_inc.index('static void mo_emb_draw'):_inc.index('static inline int mo_embS(')]
_fold = _inc[_inc.index('static void mo12_folder'):]; _fold = _fold[:_fold.index('/* content (positions') if '/* content (positions' in _fold else _fold.index('static int mo12_drop0')]   # v5d: the HACKbot lv_dl_drop folder (lvi_* helpers, all opaque fills)
_alpha_src = re.findall(r'a \* (?:f|kb|kd|k|3) / (?:256|255|4)|a / 3|ha \* k', _src_ui) + re.findall(r'MO_LOOK == 1 \? 242|solid \? 256 : 120|lv_hex\(OUT\), 200\)|lv_hex\(HEAD\), MO_LOOK == 2 \? 256 : 200', _fold)
gate('opaque UI: every emblem / on-tower folder glyph / file-line pixel and the folder pull-down drawn at 100% (no alpha blending) in every frame of the run',
     bool(mui) and int(mui[-1][1]) == 0 and not _alpha_src, ui_px_drawn=int(mui[-1][0]) if mui else None, ui_px_translucent=int(mui[-1][1]) if mui else None, alpha_terms_in_ui_source=_alpha_src,
     note='MOUI counts every UI pixel call in the sim (mo_ui_px) whose alpha is below its pillar alpha; the pull-down body / header / tab / drop outline are 256 in source')
esz = {}; front = []
for i in range(NP):
    x0, x1, yt, yb = IT[i]['f']; fit = min(36, x1 - x0 - 1); S_ = IE[i]['e'][2]
    esz[NAMES[i]] = dict(S=S_, face_w=x1 - x0, fit=fit, ok=bool(fit < 6 or fit - 3 <= S_ <= fit + 1))
zi = sorted(range(NP), key=lambda i: IT[i]['Z'])[:6]
gate('emblems: idle emblem as large as the face allows (S from face width - 4 to face width, 2 px steps with hysteresis; was 8 / 10 / 16 px steps)', all(v['ok'] for v in esz.values()), per_pillar=esz,
     front6={NAMES[i]: esz[NAMES[i]]['S'] for i in zi}, median_S=float(np.median([v['S'] for v in esz.values()])))
# temporal stability: 40 consecutive idle frames (1 tick apart); per pillar, emblem ink pixels compared frame to frame after the
# emblem rect's own integer move; pairs where the rect moved <= 1 px and kept its size, and no nearer pillar overlaps it
STAB = W + '/stab'
if not (REUSE and os.path.exists(STAB + '/run.log')):
    st_steps = json.load(open(SRC + '/mix.json'))[:4] + ['tick 300']
    for k in range(40): st_steps += ['tick 1', 'dump st%02d' % k, 'itwstate']
    json.dump(st_steps, open(W + '/stab.json', 'w')); subprocess.run([sys.executable, SRC + '/drive.py', W + '/stab.json', STAB], check=True)
slog = open(STAB + '/run.log').read(); sblk = slog.split('MOCAM')[1:][-40:]
PAL = {1: [0xd8ffe6, 0x10d858, 0x010805], 2: [0x005a20, 0x00b843, 0xffffff], 3: [0xfff0c8, 0x10d858, 0x1c120c]}
SF = [rgb(STAB + '/st%02d.rgb' % k)[0].astype(int) for k in range(40)]; lumf = lambda a: 0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]
_MC = {}
def MASK(i, S):
    if S not in _MC:
        raw_ = subprocess.run([W + '/emb_sheet', str(S)], capture_output=True, check=True).stdout; o = 0; L_ = []
        for _ in range(NP):
            b_ = np.frombuffer(raw_[o:o + S * S], np.uint8).reshape(S, S); o += S * S; d_ = np.frombuffer(raw_[o:o + S * S], np.uint8).reshape(S, S); o += S * S
            h_ = np.frombuffer(raw_[o:o + (S + 2) ** 2], np.uint8).reshape(S + 2, S + 2); o += (S + 2) ** 2
            m_ = h_ > 0; m_[1:-1, 1:-1] |= (b_ > 0) | (d_ > 0); L_.append(m_)
        _MC[S] = L_
    return _MC[S][i]
pairs = 0; dmean = []; dmax = 0.0; worst = None; pops = {n: 0 for n in NAMES}
for k in range(39):
    TA, TB = motw(sblk[k]), motw(sblk[k + 1]); EA, EB = moemb(sblk[k]), moemb(sblk[k + 1]); OA = [int(x) for x in re.search(r'ord=([\d,]+)', sblk[k]).group(1).split(',')]
    for i in range(NP):
        (ax, ay, aS), (bx, by, bS) = EA[i]['e'], EB[i]['e']
        if aS != bS: pops[NAMES[i]] += 1; continue
        if aS < 6 or abs(bx - ax) > 1 or abs(by - ay) > 1: continue
        occ = False
        for j in OA[OA.index(i) + 1:]:
            for TT in (TA, TB):
                U = TT[j]; jx0, jx1 = min(U['f'][0], U['b'][0]), max(U['f'][1], U['b'][1]); jy0, jy1 = min(U['f'][2], U['b'][2]), U['f'][3]   # v5 fix: box top = min(front top, back top)
                if jx0 <= ax + aS + 1 and jx1 >= ax - 1 and jy0 <= ay + aS + 1 and jy1 >= ay - 1: occ = True
        if occ: continue
        if min(ax, ay, bx, by) < 1 or max(ax, bx) + aS + 1 > 240 or max(ay, by) + aS + 1 > 320: continue   # emblem touching the screen edge: skip
        A = SF[k][ay - 1:ay + aS + 1, ax - 1:ax + aS + 1]; Bm = SF[k + 1][by - 1:by + bS + 1, bx - 1:bx + bS + 1]
        if A.shape != Bm.shape or A.size == 0: continue
        ink = MASK(i, aS)   # the emblem's own outline + body + detail pixels (from mo_emblem.h at this size)
        if ink.sum() < 8: continue
        d = np.abs(lumf(A) - lumf(Bm))[ink]; pairs += 1; dmean.append(float(d.mean()))
        if d.mean() > dmax: dmax, worst = float(d.mean()), (NAMES[i], k)
gate('temporal stability: emblem ink luminance frame-to-frame delta ~0 when the pillar barely moves (40 consecutive idle frames)', pairs >= 100 and dmax <= 4.0 and float(np.mean(dmean)) <= 1.0,
     pairs=pairs, mean_abs_dY=round(float(np.mean(dmean)), 3) if dmean else None, worst_pair_mean_dY=round(dmax, 2), worst=worst, size_changes_in_40_frames=pops,
     rule='pairs where the emblem rect moved <= 1 px (compensated), same size, no nearer pillar overlapping; ink = the emblem mask (outline + body + detail) at its drawn size')
scr = []
for k, nm, _ in CU:
    i = NAMES.index(nm); bl = [b for b in cub if re.search(r'MOCU %d ' % i, b)]
    def crop(b, a):
        x0, x1, yt, yb = map(int, re.search(r'MOCU %d p=[\d.]+ f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]' % i, b).groups())
        c = a[yt + 75:min(yb, 244), max(x0 + 2, 0):x1 - 1].astype(int).mean(2); return np.abs(np.diff(c, axis=0)) > 30
    A, B = crop(bl[0], fr['open_' + k]), crop(bl[-1], fr['later_' + k]); h = min(len(A), len(B)); w_ = min(A.shape[1], B.shape[1]); A, B = A[:h, :w_], B[:h, :w_]
    same = (A == B).mean(); shifted = max(((A[s:] == B[:h - s]).mean() for s in range(1, 30)), default=0)
    scr.append(dict(dev=nm, same_pos_match=round(float(same), 3), best_shift_match=round(float(shifted), 3), moved=bool(same < 0.97)))
gate('files keep scrolling inside the tower (file-list edges move between open and later frames)', all(s['moved'] for s in scr), checks=scr)
# ---------- 4. video
# determinism: the second identical run gives byte-identical framebuffers (every dump + the whole video)
dumps = sorted(f for f in os.listdir(W + '/run') if f.endswith('.rgb')); same = [f for f in dumps if open(W + '/run/' + f, 'rb').read() == open(W + '/run2/' + f, 'rb').read()]
gate('determinism: two identical runs give byte-identical framebuffers (all dumps + video)', len(dumps) >= 14 and len(same) == len(dumps), dumps=len(dumps), identical=len(same),
     video_sha256=hashlib.sha256(open(W + '/run/mix.rgb', 'rb').read()).hexdigest()[:16])
# motion: idle frames before the first tap really move (no frozen stills)
v = rgb(W + '/run/mix.rgb'); fz = sum(1 for a in range(1, len(v)) if (v[a] == v[a - 1]).all())
gate('video: no frozen (identical consecutive) frames', fz == 0, frames=len(v), frozen=fz)
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-framerate', '100/3', '-i', W + '/run/mix.rgb',
                '-vf', 'scale=720:960:flags=neighbor', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '16', '-movflags', '+faststart', R + '/idle_then_open_3x.mp4'], check=True)
pr = json.loads(subprocess.run(['ffprobe', '-v', 'error', '-show_streams', '-of', 'json', R + '/idle_then_open_3x.mp4'], capture_output=True, text=True).stdout)['streams'][0]
gate('video: h264 yuv420p 720x960 @3x', pr['codec_name'] == 'h264' and pr['pix_fmt'] == 'yuv420p' and pr['width'] == 720 and pr['height'] == 960, codec=pr['codec_name'], pix_fmt=pr['pix_fmt'], size=[pr['width'], pr['height']], frames=pr.get('nb_frames'), seconds=pr.get('duration'))
st = all(Image.open(R + '/' + f).size == (720, 960) for f in os.listdir(R) if f.endswith('_3x.png') and not f.startswith('open_sequence')) and Image.open(R + '/idle_1x.png').size == (240, 320)
gate('stills: idle @1x 240x320, idle + 4 close-ups @3x 720x960', st)
npass = sum(g['pass_'] for g in G)
json.dump(dict(mock='morpheus-mix SPREAD 2026-11-14 (mock only: no flash, no merge, no firmware commit)', passed=npass, total=len(G),
               devices=NAMES, fixed_heights_world=dict(T1_code_glass=HF[1], T3_sentinel_lantern=HF[3], T2_white_light=HF[2]),
               idle_still=dict(sample_index=KREP, of=len(SM), rule='median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample', clean_pool=int(len(POOL)),
                               quality=round(float(qual[KREP]), 4), quality_min=round(float(qual.min()), 4), quality_median=round(float(np.median(qual)), 4), quality_max=round(float(qual.max()), 4)),
               gates=[{('pass' if k == 'pass_' else k): v for k, v in g.items()} for g in G]),
          open(R + '/gate.json', 'w'), indent=1, default=lambda o: o.item() if hasattr(o, 'item') else str(o))
print('GATE %d/%d' % (npass, len(G)))
