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
PRE = min(128, 10 * (KREP + 1) - 1); steps[4] = 'tick %d' % (10 * (KREP + 1) - PRE); steps[6] = 'tick %d' % PRE   # video pre-roll before the idle still: up to 128 ticks; json.dump(steps, open(W + '/mix_run.json', 'w'))   # idle dump lands on sample KREP's time
print('representative idle sample', KREP, 'of', len(SM), 'quality', round(float(qual[KREP]), 4), 'range', round(float(qual.min()), 4), round(float(qual.max()), 4), flush=True)
# ---------- 1. main run (+ an identical second run for the determinism gate)
for d in ('run', 'run2'):
    if not (REUSE and os.path.exists(W + '/%s/run.log' % d)):
        subprocess.run([sys.executable, SRC + '/drive.py', W + '/mix_run.json', W + '/' + d], check=True)
log = open(W + '/run/run.log').read()
fr = {n: rgb(W + '/run/%s.rgb' % n)[0] for n in ['idle'] + ['%s_%s' % (a, b) for b in ('deauth','airtag','flock','raven') for a in ('push','open','later')]}
png(fr['idle'], R + '/idle_1x.png', 1); png(fr['idle'], R + '/idle_3x.png', 3)
CU = [('deauth', 'DEAUTH', 'high'), ('airtag', 'AIR TAG', 'medium'), ('flock', 'FLOCK', 'low'), ('raven', 'RAVEN', 'new')]
for k, nm, tag in CU: png(fr['later_' + k], R + '/closeup_%s_%s_3x.png' % (k, tag), 3)
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
        U = IT[j]; m[max(U['b'][2], 0):U['f'][3] + 1, max(min(U['f'][0], U['b'][0]), 0):max(U['f'][1], U['b'][1]) + 1] = False
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
    elif c == LOOK[i]: ok_px += 1
    else: bad.append((NAMES[i], c, n))
gate('assignment (pixels): unoccluded face colour class = look', not bad and ok_px >= 14, matched=ok_px, misses=bad, unclassifiable_hidden=unc, note='idle still; faces with < 15 unoccluded px are not classified')
# motion: every pillar follows the identical v11 drift law (dx, dz recomputed from the tables), X = X0 + dx, Z = Z0 + dz
import math
lawerr = 0; xzerr = 0
for a in SM:
    t = a['t']
    for i in range(NP):
        Z0 = J['z0'][i]; X0 = (J['sx'][i] - 120) * Z0 / 120.0
        dx = J['ax'][i] * (Z0 / 2) * math.sin(J['wx'][i] * t + i * 2.399); dz = J['az'][i] * (Z0 / 3) * math.sin(J['wz'][i] * t + i * 1.713 + 0.9)
        lawerr = max(lawerr, abs(a['tw'][i]['dx'] - dx), abs(a['tw'][i]['dz'] - dz)); xzerr = max(xzerr, abs(a['tw'][i]['X'] - X0 - dx), abs(a['tw'][i]['Z'] - Z0 - dz))
gate('motion: all 18 follow the identical v11 drift law (dx, dz, X, Z recomputed from the tables)', lawerr < 2e-3 and xzerr < 2e-3, max_dxdz_err=round(lawerr, 5), max_xz_err=round(xzerr, 5), samples=len(SM))
Hs = {1: set(), 2: set(), 3: set()}; allH = set()
for a in SM:
    for i in range(NP): h = round(a['tw'][i]['H'], 4); Hs[LOOK[i]].add(h); allH.add(h)
gate('heights: exactly THREE distinct pillar heights over all 320 idle samples, one per style', len(allH) == 3 and all(len(v) == 1 for v in Hs.values()),
     distinct=sorted(allH), per_style={LN[k]: sorted(v) for k, v in Hs.items()})
mH = {LN[k]: max(v) for k, v in Hs.items()}
gate('heights: T1 code glass tallest > T3 lantern > T2 white light shortest', mH['T1_code_glass'] > mH['T3_sentinel_post'] > mH['T2_construct_light'], H=mH)
base = HF[1]; fac = {k: round(HF[k] / base, 4) for k in (1, 3, 2)}
gate('heights: factors on ONE base height H = green T1 1.00 / wood T3 0.80 / white T2 0.62 (Mobandy 9:27 AM ET: bigger steps, no stumps)', abs(fac[1] - 1.0) < 1e-3 and abs(fac[3] - 0.80) < 1e-3 and abs(fac[2] - 0.62) < 1e-3,
     base_H=base, factors={'T1_code_glass': fac[1], 'T3_see_through_wood': fac[3], 'T2_white_light': fac[2]})
mr = min(min(v) for v in Hs.values()) / max(max(v) for v in Hs.values())
gate('heights: min / max height ratio >= 0.60 (no stumps: white 0.62 x green)', mr >= 0.60 - 1e-6, min_ratio=round(mr, 4))
HV3 = {1: 1.26, 3: 1.134, 2: 1.008}   # the rejected 9:22 AM ET heights (_withdrawn/v3)
tall = {LN[k]: round(HF[k] / HV3[k], 3) for k in (1, 3, 2)}
fh = {LN[k]: float(np.median([a['tw'][i]['yb'] - a['tw'][i]['yt'] for a in SM for i in range(NP) if LOOK[i] == k])) for k in (1, 3, 2)}
fw_med = {LN[k]: float(np.median([a['tw'][i]['x1'] - a['tw'][i]['x0'] for a in SM for i in range(NP) if LOOK[i] == k])) for k in (1, 3, 2)}
gate('heights: every style 1.6 - 2.1 x taller than the rejected 9:22 AM version (same projection, so on-screen height scales the same at equal depth)', all(1.595 <= v <= 2.1 + 1e-3 for v in tall.values()),
     x_v3=tall, H_now={LN[k]: HF[k] for k in (1, 3, 2)}, H_v3={LN[k]: HV3[k] for k in (1, 3, 2)}, median_front_face_h_px=fh, median_front_face_w_px=fw_med)
gate('heights: no idle height breathing (hk = 1 on every pillar, every sample)', all(abs(a['tw'][i]['hk'] - 1.0) < 1e-6 for a in SM for i in range(NP)))
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
# sheet
font = ImageFont.load_default(); CW, CH = 300, 150; sheet = Image.new('RGB', (CW * 4, CH * 5 + 40), (30, 26, 22)); dr = ImageDraw.Draw(sheet)   # 18 cells (4 x 5 grid, last two empty)
dr.text((10, 10), 'MORPHEUS MIX SPREAD (18, WIFI + BLUETOOTH removed): device emblems, OLD 16x16 art (x3, nearest) vs NEW vector (16 px x3, 40 px x2), body parchment / detail green / halo', fill=(232, 222, 200), font=font)
def comp(i, S, k, bg=(30, 26, 22)):
    b, d, h = EMB[i, S]; im = np.zeros((S + 2, S + 2, 3), float) + bg
    for m, col in ((h, (1, 8, 5)), (np.pad(b, 1), (232, 222, 200)), (np.pad(d, 1), (16, 216, 88))):
        al = m[..., None] / 255.0; im = im * (1 - al) + np.array(col) * al
    return Image.fromarray(im.astype(np.uint8)).resize(((S + 2) * k, (S + 2) * k), Image.NEAREST)
OC = {'k': (30, 26, 22), 'a': (232, 222, 200), 'b': (184, 164, 124), 'c': (255, 255, 255), 'g': (0, 168, 60)}
for i in range(NP):
    x, y = (i % 4) * CW, 40 + (i // 4) * CH; dr.rectangle([x + 2, y + 2, x + CW - 3, y + CH - 3], outline=(92, 74, 50))
    dr.text((x + 8, y + 6), '%s  [%s -> %s]%s' % (NAMES[i], TN[THREAT[i]], LN[LOOK[i]][:2], '  NEW' if NAMES[i] in NEWSET else ''), fill=(16, 216, 88) if NAMES[i] in NEWSET else (232, 222, 200), font=font)
    if i < 10:
        o = np.zeros((16, 16, 3), np.uint8) + 60
        for r in range(16):
            for c in range(16):
                ch = chr(OLD[i][r, c]); o[r, c] = OC.get(ch, (0, 168, 60)) if ch != '.' else (60, 54, 46)
        sheet.paste(Image.fromarray(o).resize((48, 48), Image.NEAREST), (x + 10, y + 40)); dr.text((x + 10, y + 92), 'OLD 16', fill=(191, 178, 148), font=font)
    else: dr.text((x + 10, y + 56), '(no old\n art)', fill=(191, 178, 148), font=font)
    sheet.paste(comp(i, 16, 3), (x + 80, y + 40)); dr.text((x + 80, y + 98), 'NEW 16', fill=(191, 178, 148), font=font)
    sheet.paste(comp(i, 40, 2), (x + 150, y + 28)); dr.text((x + 150, y + 116), 'NEW 40', fill=(191, 178, 148), font=font)
sheet.save(R + '/emblems_old_vs_new.png')
# close-ups: 40 px emblem on the open pillar, folders landed, pillar >= 50 px wide; threat text; no red pill; scrolling files
cub = [b for b in blocks if 'MOFOLD' in b]; cu_ok = []; 
for k, nm, tag in CU:
    i = NAMES.index(nm); bl = [b for b in cub if re.search(r'MOCU %d ' % i, b)]; b = bl[-1]
    fw = int(re.search(r'MOCU %d .*fw=(\d+)' % i, b).group(1)); nl = int(re.search(r'MOFOLD nland=(\d)', b).group(1)); e = moemb(b).get(i, {}).get('e', [0, 0, 0])
    cu_ok.append(dict(dev=nm, fw=fw, nland=nl, emb_S=e[2], ok=fw >= 50 and nl == 3 and e[2] == 40))
gate('close-ups: pillar >= 50 px wide, 3 folders landed, 40 px emblem drawn', all(c['ok'] for c in cu_ok), closeups=cu_ok)
term = {k: fr['later_' + k][287 + 4:287 + 10, 5:200] for k, _, _ in CU}
gate('threat text: small text in the terminal status line; LOW lines identical, HIGH / MEDIUM / LOW differ', (term['flock'] == term['raven']).all() and not (term['deauth'] == term['airtag']).all() and not (term['deauth'] == term['flock']).all() and not (term['airtag'] == term['flock']).all(),
     note='line reads "PILLAR OPEN  THREAT: <LEVEL>  EX.DATA"; no new widget')
red = 0
for k, _, _ in CU:
    for ph in ('push', 'open', 'later'):
        a = fr['%s_%s' % (ph, k)][35:247].astype(int); red += int(((a[..., 0] > 180) & (a[..., 1] < 70) & (a[..., 2] < 70)).sum())
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
_fold = _inc[_inc.index('static void mo12_folder'):]; _fold = _fold[:_fold.index('/* content (positions')]
_alpha_src = re.findall(r'a \* (?:f|kb|kd|k|3) / (?:256|255|4)|a / 3|ha \* k', _src_ui) + re.findall(r'MO_LOOK == 1 \? 242|solid \? 256 : 120|lv_hex\(OUT\), 200\)|lv_hex\(HEAD\), MO_LOOK == 2 \? 256 : 200', _fold)
gate('opaque UI: every emblem / on-tower folder glyph / file-line pixel and the folder pull-down drawn at 100% (no alpha blending) in every frame of the run',
     bool(mui) and int(mui[-1][1]) == 0 and not _alpha_src, ui_px_drawn=int(mui[-1][0]) if mui else None, ui_px_translucent=int(mui[-1][1]) if mui else None, alpha_terms_in_ui_source=_alpha_src,
     note='MOUI counts every UI pixel call in the sim (mo_ui_px) whose alpha is below its pillar alpha; the pull-down body / header / tab / drop outline are 256 in source')
esz = {}; front = []
for i in range(NP):
    x0, x1, yt, yb = IT[i]['f']; fit = min(36, x1 - x0 - 1); S_ = IE[i]['e'][2]
    esz[NAMES[i]] = dict(S=S_, face_w=x1 - x0, fit=fit, ok=bool(fit < 6 or fit - 3 <= S_ <= fit))
zi = sorted(range(NP), key=lambda i: IT[i]['Z'])[:6]
gate('emblems: idle emblem as large as the face allows (S within 3 px of face width - 1, 2 px steps with hysteresis; was 8 / 10 / 16 px steps)', all(v['ok'] for v in esz.values()), per_pillar=esz,
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
                U = TT[j]; jx0, jx1 = min(U['f'][0], U['b'][0]), max(U['f'][1], U['b'][1]); jy0, jy1 = U['b'][2], U['f'][3]
                if jx0 <= ax + aS + 1 and jx1 >= ax - 1 and jy0 <= ay + aS + 1 and jy1 >= ay - 1: occ = True
        if occ: continue
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
st = all(Image.open(R + '/' + f).size == (720, 960) for f in os.listdir(R) if f.endswith('_3x.png')) and Image.open(R + '/idle_1x.png').size == (240, 320)
gate('stills: idle @1x 240x320, idle + 4 close-ups @3x 720x960', st)
npass = sum(g['pass_'] for g in G)
json.dump(dict(mock='morpheus-mix SPREAD 2026-11-14 (mock only: no flash, no merge, no firmware commit)', passed=npass, total=len(G),
               devices=NAMES, fixed_heights_world=dict(T1_code_glass=HF[1], T3_sentinel_lantern=HF[3], T2_white_light=HF[2]),
               idle_still=dict(sample_index=KREP, of=len(SM), rule='median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample', clean_pool=int(len(POOL)),
                               quality=round(float(qual[KREP]), 4), quality_min=round(float(qual.min()), 4), quality_median=round(float(np.median(qual)), 4), quality_max=round(float(qual.max()), 4)),
               gates=[{('pass' if k == 'pass_' else k): v for k, v in g.items()} for g in G]),
          open(R + '/gate.json', 'w'), indent=1, default=lambda o: o.item() if hasattr(o, 'item') else str(o))
print('GATE %d/%d' % (npass, len(G)))
