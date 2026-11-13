#!/usr/bin/env python3
"""morpheus-mix package: run the mix script on the patched sim, render stills / video / emblem sheet, run the gates, write gate.json.
Sequential on purpose (one sim at a time). REUSE=1 skips the sim runs if their outputs exist."""
import json, os, re, subprocess, sys, hashlib, numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__))); import touch20 as T
R = '/workspace/out/morpheus-mix'; SRC = R + '/src'; W = '/tmp/mmix'; REUSE = os.environ.get('REUSE') == '1'
NAMES = ["WIFI","BLUETOOTH","DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
THREAT = [2,2,0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TN = ["HIGH","MEDIUM","LOW"]; LOOK = [{0:1,1:3,2:2}[t] for t in THREAT]
LN = {1: 'T1_code_glass', 2: 'T2_construct_light', 3: 'T3_sentinel_post'}
J = json.load(open(SRC + '/layout.json')); G = []
def gate(name, ok, **kw): G.append(dict(name=name, pass_=bool(ok), **kw)); print(('PASS ' if ok else 'FAIL ') + name, json.dumps(kw)[:300], flush=True)
def rgb(p): return np.frombuffer(open(p, 'rb').read(), np.uint8).reshape(-1, 320, 240, 3)
def png(a, p, k=1): Image.fromarray(a).resize((240 * k, 320 * k), Image.NEAREST).save(p)
# ---------- 1. main run
if not (REUSE and os.path.exists(W + '/run/run.log')):
    subprocess.run([sys.executable, SRC + '/drive.py', SRC + '/mix.json', W + '/run'], check=True)
log = open(W + '/run/run.log').read()
fr = {n: rgb(W + '/run/%s.rgb' % n)[0] for n in ['idle'] + ['%s_%s' % (a, b) for b in ('deauth','airtag','wifi','raven') for a in ('push','open','later')]}
png(fr['idle'], R + '/idle_1x.png', 1); png(fr['idle'], R + '/idle_3x.png', 3)
CU = [('deauth', 'DEAUTH', 'high'), ('airtag', 'AIR TAG', 'medium'), ('wifi', 'WIFI', 'low'), ('raven', 'RAVEN', 'new')]
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
gate('presence: 20 pillars placed in idle', len(IT) == 20 and len(IE) == 20, n=len(IT))
gate('assignment (sim read-back): look = threat table for all 20', all(IE[i]['look'] == LOOK[i] and IE[i]['threat'] == TN[THREAT[i]] for i in range(20)))
# pixel look classifier on the idle frames (best of the idle still + two touch-run frames): face pixels outside the emblem rect
def classify(a, i, IT, ordr):
    """median colour of pillar i's front face pixels that no nearer pillar's box covers (None if < 15 such pixels)"""
    x0, x1, yt, yb = IT[i]['f']; m = np.zeros((320, 240), bool); m[max(yt + 2, 36):min(yb - 1, 284), max(x0 + 2, 0):min(x1 - 1, 239) + 1] = True
    for j in ordr[ordr.index(i) + 1:]:
        U = IT[j]; m[max(U['b'][2], 0):U['f'][3] + 1, max(min(U['f'][0], U['b'][0]), 0):max(U['f'][1], U['b'][1]) + 1] = False
    reg = a[m].astype(int)
    if len(reg) < 15: return None, len(reg)
    r, g, b = np.median(reg, 0); lum = (r + g + b) / 3
    if lum > 170 and abs(r - g) < 40: return 2, len(reg)
    if g > r and g >= b and lum < 110: return 1, len(reg)
    if r > g: return 3, len(reg)
    return 0, len(reg)
# ---------- 2. idle samples: main, no-style-height A/B, reference v13.1 (original 12 motion)
def samp(sim, cwd, tag):
    p = W + '/s_%s.json' % tag
    if REUSE and os.path.exists(p): return json.load(open(p), object_hook=lambda d: {int(k) if k.isdigit() else k: v for k, v in d.items()})
    S = T.sample(sim, cwd, W + '/t_' + tag); json.dump(S, open(p, 'w')); return json.load(open(p), object_hook=lambda d: {int(k) if k.isdigit() else k: v for k, v in d.items()})
SM = samp(W + '/sim', R + '/tree', 'main'); SN = samp(W + '/sim_nohs', W + '/tree_nohs', 'nohs'); SR = samp(W + '/sim_ref', '/workspace/cyd-morpheus-v6-wt', 'ref')
ok_px = 0; bad = []; unc = []; IORD = [int(x) for x in re.search(r'ord=([\d,]+)', idle_blk).group(1).split(',')]
for i in range(20):
    c, n = classify(fr['idle'], i, IT, IORD)
    if c is None: unc.append(NAMES[i])
    elif c == LOOK[i]: ok_px += 1
    else: bad.append((NAMES[i], c, n))
gate('assignment (pixels): unoccluded face colour class = look', not bad and ok_px >= 16, matched=ok_px, misses=bad, unclassifiable_hidden=unc, note='idle still; faces with < 15 unoccluded px are not classified')
# motion: originals identical to v13.1 except H x style factor; new 8 follow the same v11 law (checked vs no-style-height build)
mx = 0; hr = 0; nmx = 0
for a, b in zip(SM, SR):
    for i in range(12):
        A, B = a['tw'][i], b['tw'][i]; mx = max(mx, abs(A['X'] - B['X']), abs(A['Z'] - B['Z']), abs(A['dx'] - B['dx']), abs(A['dz'] - B['dz']), abs(A['hk'] - B['hk']))
        hr = max(hr, abs(A['H'] / B['H'] - (J['h1'] if LOOK[i] == 1 else J['h2'] if LOOK[i] == 2 else 1.0)))
gate('motion: original 12 X/Z/drift/breathing identical to v13.1, H ratio = style factor', mx < 2e-4 and hr < 2e-3, max_xz_err=mx, max_h_ratio_err=round(hr, 5), samples=len(SM))
for a, b in zip(SM, SN):
    for i in range(20):
        A, B = a['tw'][i], b['tw'][i]; nmx = max(nmx, abs(A['X'] - B['X']), abs(A['Z'] - B['Z']))
import math
lawerr = 0
for a in SM:
    t = a['t']
    for i in range(12, 20):
        k = i - 12; Z0 = J['z0'][k]; dx = J['ax'][k] * (Z0 / 2) * math.sin(J['wx'][k] * t + i * 2.399); dz = J['az'][k] * (Z0 / 3) * math.sin(J['wz'][k] * t + i * 1.713 + 0.9)
        lawerr = max(lawerr, abs(a['tw'][i]['dx'] - dx), abs(a['tw'][i]['dz'] - dz))
gate('motion: new 8 follow the identical v11 drift law (dx, dz recomputed from the tables)', lawerr < 2e-3 and nmx < 2e-4, max_err=round(lawerr, 5), style_height_changes_xz=nmx)
Hs = {1: [], 2: [], 3: []}
for a in SM:
    for i in range(20): Hs[LOOK[i]].append(a['tw'][i]['H'])
mH = {LN[k]: round(float(np.mean(v)), 4) for k, v in Hs.items()}
gate('heights: per-style mean H ordered T1 > T3 > T2', mH['T1_code_glass'] > mH['T3_sentinel_post'] > mH['T2_construct_light'], mean_H=mH)
gate('heights: style factors slight (T1 1.05..1.15, T3 1.00, T2 0.85..0.95)', 1.05 <= J['h1'] <= 1.15 and 0.85 <= J['h2'] <= 0.95, h1=J['h1'], h3=1.0, h2=J['h2'])
sq, vp, vf = T.analyse(SM); sqn, vpn, vfn = T.analyse(SN)
idle_vis = {}
for i in range(20):
    s = dict(ord=[int(x) for x in re.search(r'ord=([\d,]+)', idle_blk).group(1).split(',')], tw={k: dict(x0=v['f'][0], x1=v['f'][1], yt=v['f'][2], yb=v['f'][3], bx0=v['b'][0], bx1=v['b'][1], byt=v['b'][2], byb=v['b'][3]) for k, v in IT.items()},
             em={k: dict(x=v['e'][0], y=v['e'][1], S=v['e'][2]) for k, v in IE.items()})
    idle_vis[NAMES[i]] = T.vis(s, i)[0]
vm = vf.mean(0)
gate('emblem visibility (idle still): every pillar shows emblem pixels', min(idle_vis.values()) > 0, px=idle_vis, min=min(idle_vis.values()), mean=round(float(np.mean(list(idle_vis.values()))), 1))
gate('emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5', vm.min() >= 0.5, px_min=float(vp.min()), px_mean=round(float(vp.mean()), 1),
     frac_mean=round(float(vm.mean()), 3), worst=NAMES[int(vm.argmin())], worst_frac=round(float(vm.min()), 3),
     per_pillar={NAMES[i]: dict(frac=round(float(vm[i]), 3), px_mean=round(float(vp[:, i].mean()), 1), px_min=int(vp[:, i].min())) for i in range(20)},
     without_style_heights=dict(frac_mean=round(float(vfn.mean(0).mean()), 3), worst_frac=round(float(vfn.mean(0).min()), 3), px_mean=round(float(vpn.mean()), 1)))
p10 = np.percentile(sq, 10, axis=0)
gate('tap: every pillar keeps a tap cell in every idle sample', sq.min() > 0, zero_cell=[NAMES[i] for i in range(20) if sq[:, i].min() == 0],
     zero_frac={NAMES[i]: round(float((sq[:, i] == 0).mean()), 3) for i in range(20) if sq[:, i].min() == 0})
gate('tap: p10 largest square >= 20 px for every pillar', p10.min() >= 20, p10={NAMES[i]: float(p10[i]) for i in range(20)}, below_20=[NAMES[i] for i in range(20) if p10[i] < 20],
     median={NAMES[i]: float(np.median(sq[:, i])) for i in range(20)})
# ---------- 3. emblems
raw = subprocess.run([W + '/emb_sheet'], capture_output=True, check=True).stdout; off = 0; EMB = {}
for i in range(20):
    for S in (16, 32, 40):
        b = np.frombuffer(raw[off:off + S * S], np.uint8).reshape(S, S); off += S * S; d = np.frombuffer(raw[off:off + S * S], np.uint8).reshape(S, S); off += S * S
        h = np.frombuffer(raw[off:off + (S + 2) ** 2], np.uint8).reshape(S + 2, S + 2); off += (S + 2) ** 2; EMB[i, S] = (b, d, h)
OLD = [np.frombuffer(raw[off + k * 256: off + (k + 1) * 256], np.uint8).reshape(16, 16) for k in range(12)]
aa = []
for i in range(20):
    c = EMB[i, 40][0].astype(int) + EMB[i, 40][1].astype(int); part = ((c > 0) & (c < 255)).sum(); full = (c >= 250).sum(); aa.append(part / max(1, part + full))
gate('emblems: anti-aliased (partial-coverage edge pixels on every 40 px emblem)', min(aa) > 0.1, min_partial_frac=round(min(aa), 3))
sil = [(EMB[i, 32][0].astype(int) + EMB[i, 32][1] > 127) for i in range(20)]; mi = 0; pair = None
for a in range(20):
    for b in range(a + 1, 20):
        iou = (sil[a] & sil[b]).sum() / max(1, (sil[a] | sil[b]).sum())
        if iou > mi: mi, pair = iou, (NAMES[a], NAMES[b])
gate('emblems: unique silhouettes (max pairwise IoU at 32 px < 0.75)', mi < 0.75, max_iou=round(float(mi), 3), pair=pair)
# sheet
font = ImageFont.load_default(); CW, CH = 300, 150; sheet = Image.new('RGB', (CW * 4, CH * 5 + 40), (30, 26, 22)); dr = ImageDraw.Draw(sheet)
dr.text((10, 10), 'MORPHEUS MIX: device emblems, OLD 16x16 art (x3, nearest) vs NEW vector (16 px x3, 40 px x2), body parchment / detail green / halo', fill=(232, 222, 200), font=font)
def comp(i, S, k, bg=(30, 26, 22)):
    b, d, h = EMB[i, S]; im = np.zeros((S + 2, S + 2, 3), float) + bg
    for m, col in ((h, (1, 8, 5)), (np.pad(b, 1), (232, 222, 200)), (np.pad(d, 1), (16, 216, 88))):
        al = m[..., None] / 255.0; im = im * (1 - al) + np.array(col) * al
    return Image.fromarray(im.astype(np.uint8)).resize(((S + 2) * k, (S + 2) * k), Image.NEAREST)
OC = {'k': (30, 26, 22), 'a': (232, 222, 200), 'b': (184, 164, 124), 'c': (255, 255, 255), 'g': (0, 168, 60)}
for i in range(20):
    x, y = (i % 4) * CW, 40 + (i // 4) * CH; dr.rectangle([x + 2, y + 2, x + CW - 3, y + CH - 3], outline=(92, 74, 50))
    dr.text((x + 8, y + 6), '%s  [%s -> %s]%s' % (NAMES[i], TN[THREAT[i]], LN[LOOK[i]][:2], '  NEW' if i >= 12 else ''), fill=(16, 216, 88) if i >= 12 else (232, 222, 200), font=font)
    if i < 12:
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
gate('threat text: small text in the terminal status line; LOW lines identical, HIGH / MEDIUM / LOW differ', (term['wifi'] == term['raven']).all() and not (term['deauth'] == term['airtag']).all() and not (term['deauth'] == term['wifi']).all() and not (term['airtag'] == term['wifi']).all(),
     note='line reads "PILLAR OPEN  THREAT: <LEVEL>  EX.DATA"; no new widget')
red = 0
for k, _, _ in CU:
    for ph in ('push', 'open', 'later'):
        a = fr['%s_%s' % (ph, k)][35:247].astype(int); red += int(((a[..., 0] > 180) & (a[..., 1] < 70) & (a[..., 2] < 70)).sum())
gate('no red pill (no saturated red pixels in the field on any close-up frame)', red == 0, red_px=red)
txt_bad = int(re.findall(r'txt_bad_all=(\d+)', log)[-1]); txt_all = int(re.findall(r'txt_lines_all=(\d+)', log)[-1])
gate('text audit: every audited text line inside its box', txt_bad == 0, lines=txt_all, bad=txt_bad)
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
v = rgb(W + '/run/mix.rgb'); fz = sum(1 for a in range(1, len(v)) if (v[a] == v[a - 1]).all())
gate('video: no frozen (identical consecutive) frames', fz == 0, frames=len(v), frozen=fz)
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-framerate', '100/3', '-i', W + '/run/mix.rgb',
                '-vf', 'scale=720:960:flags=neighbor', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '16', '-movflags', '+faststart', R + '/idle_then_open_3x.mp4'], check=True)
pr = json.loads(subprocess.run(['ffprobe', '-v', 'error', '-show_streams', '-of', 'json', R + '/idle_then_open_3x.mp4'], capture_output=True, text=True).stdout)['streams'][0]
gate('video: h264 yuv420p 720x960 @3x', pr['codec_name'] == 'h264' and pr['pix_fmt'] == 'yuv420p' and pr['width'] == 720 and pr['height'] == 960, codec=pr['codec_name'], pix_fmt=pr['pix_fmt'], size=[pr['width'], pr['height']], frames=pr.get('nb_frames'), seconds=pr.get('duration'))
st = all(Image.open(R + '/' + f).size == (720, 960) for f in os.listdir(R) if f.endswith('_3x.png')) and Image.open(R + '/idle_1x.png').size == (240, 320)
gate('stills: idle @1x 240x320, idle + 4 close-ups @3x 720x960', st)
npass = sum(g['pass_'] for g in G)
json.dump(dict(mock='morpheus-mix 2026-11-13 (mock only: no flash, no merge, no firmware commit)', passed=npass, total=len(G), style_heights=dict(T1=J['h1'], T3=1.0, T2=J['h2']),
               htune_sim=json.load(open(SRC + '/htune_sim.json')) if os.path.exists(SRC + '/htune_sim.json') else None, gates=[{('pass' if k == 'pass_' else k): v for k, v in g.items()} for g in G]),
          open(R + '/gate.json', 'w'), indent=1, default=lambda o: o.item() if hasattr(o, 'item') else str(o))
print('GATE %d/%d' % (npass, len(G)))
