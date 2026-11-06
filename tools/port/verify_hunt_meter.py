"""Verify the firmware hunt meter port (LOCKED ui_gibson_radar_hunt_meter A + ui_gibson_radar_hunt_meter_interact V2) in sim_live.
usage: verify_hunt_meter.py SIMDIR LOCK_V2 OUT
 SIMDIR holds the sim binaries (built from the repo root, see tools/port/hunt_meter_sims.sh):
   new      this tree, -DLV_RD_CG_LOG            base     3b8b3de tree, -DLV_RD_CG_LOG
   new_occ  this tree, -DLV_RD_CG_LOG -DLV_RD_HM_OCC=1 (opt-in: meter behind nearer towers; reported, not shipped)
   new_hm0  this tree, -DLV_RD_HUNT_METER=0      new_cg0 / base_cg0  -DLV_RD_CHARGE=0 (this tree / 3b8b3de)
   new_nolog / base_nolog  default flags (other locked screens)
 LOCK_V2 = cyd-lock-store LOCKED/ui_gibson_radar_hunt_meter_interact (src/render_interact.py + stills/ are read, never written).
 Checks: pre-hunt D4 dumps vs 3b8b3de (byte), 200 hunt frames vs the locked V2 mock re-rendered on the 3b8b3de frames (RGB565,
 @1x = the mock's centre samples = the locked @1x stills), changed px outside the meter masks, flag-off byte identity, and the
 other locked screens (verify_port.py's render set) byte-identical to 3b8b3de. Writes cmp PNGs, fw stills, fw_hunt_V2_3x.mp4."""
import sys, os, json, shutil, subprocess, importlib, tempfile
import numpy as np
from PIL import Image, ImageDraw
SIMD, LOCK, OUT = sys.argv[1:4]; os.makedirs(OUT, exist_ok=True)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TMP = tempfile.mkdtemp(prefix='hmv_'); sys.dont_write_bytecode = True
q = lambda a: np.stack([a[..., 0] >> 3, a[..., 1] >> 2, a[..., 2] >> 3], -1)
ld = lambda p: np.fromfile(p, np.uint8).reshape(-1, 320, 240, 3)
def run(sim, script, tag):
    d = f'{TMP}/{tag}'; os.makedirs(d, exist_ok=True)
    with open(f'{d}/out.txt', 'w') as o, open(f'{d}/err.txt', 'w') as e:
        r = subprocess.run([f'{SIMD}/{sim}', script, d], cwd=REPO, stdout=o, stderr=e)
    assert r.returncode == 0, (sim, script, r.returncode); return d
S0 = f'{LOCK}/src/hunt_current_meter.txt'
S1 = f'{TMP}/hunt_hmsched.txt'
open(S1, 'w').write(open(S0).read().replace('dump hunting\n', 'dump hunting\nhmsched 26640\n'))   # replay the mock's fill schedule from its first frame
rep = {}
B = run('base', S0, 'base'); N = run('new', S1, 'new'); NO = run('new_occ', S1, 'occ')
# 1. pre-hunt charge-lock moments vs 3b8b3de
rep['prehunt_vs_3b8b3de_px'] = {k: int((ld(f'{B}/{k}.rgb')[0] != ld(f'{N}/{k}.rgb')[0]).any(2).sum()) for k in ('sel', 'mid', 'full', 'lockfire', 'locked')}
# 2. the locked V2 mock, re-rendered by its own code on the 3b8b3de frames
md = f'{TMP}/mock'; shutil.copytree(f'{LOCK}/src', md, ignore=shutil.ignore_patterns('__pycache__'))
sys.argv = ['render_interact.py', f'{TMP}/mockout', B]; sys.path.insert(0, md); RI = importlib.import_module('render_interact')
fb, fn, fo = ld(f'{B}/hunt.rgb'), ld(f'{N}/hunt.rgb'), ld(f'{NO}/hunt.rgb'); assert len(fb) == len(fn) == len(fo) == 200
EV = RI.EV['V2']; m3 = [np.asarray(RI.render('V2', s, EV)) for s in range(200)]
# HM log: the hunted tower per frame (this tree, LV_RD_CG_LOG)
hm = {}
for l in open(f'{N}/err.txt'):
    if l.startswith('HM '):
        f = dict(kv.split('=') for kv in l.split()[1:]); hm.setdefault(int(f['t']), {k: int(v) for k, v in f.items()})
ts = RI.ts
def mask(t, cxy):
    h = hm[t]; m = np.zeros((320, 240), bool)
    m[min(h['yt'], h['byt']) - 1:h['yb'] + 2, h['L'] - 1:h['R'] + 2] = True
    cx, cy = cxy; m[cy - 3:cy + 4, cx - 3:cx + 4] = True
    m[6:22, 83:149] = True; return m        # the cleared top-meter slot (mock rect 250,18 - 445,66 at 3x)
per, tot1, totedge, outside, lumo, lumm, noocc, occ_only = [], 0, 0, 0, 0, 0, 0, 0
for s in range(200):
    c1 = m3[s][1::3, 1::3]; d = (q(c1) != q(fn[s])).any(2)
    do = (q(c1) != q(fo[s])).any(2); noocc += int(do.sum()); occ_only += int((do & (fo[s] == fb[s]).all(2)).sum())
    kept = d & (fn[s] == fb[s]).all(2)          # fw kept the 3b8b3de pixel: a nearer tower's edge line stays on top (occlusion)
    tot1 += int(d.sum()); totedge += int(kept.sum())
    ch = (fn[s] != fb[s]).any(2); mk = mask(ts[s], RI.contact(s)); outside += int((ch & ~mk).sum())
    h = hm[ts[s]]; yl = 0.299 * fn[s][..., 0] + 0.587 * fn[s][..., 1] + 0.114 * fn[s][..., 2]
    glass = np.zeros((320, 240), bool); glass[:, h['L']:h['R'] + 1] = True; glass[:24] = True
    ov = ch & ~glass; lumo = max(lumo, float(yl[ov].max(initial=0)))
    c1q = q(c1); c1e = np.stack([c1q[..., 0] << 3 | c1q[..., 0] >> 2, c1q[..., 1] << 2 | c1q[..., 1] >> 4, c1q[..., 2] << 3 | c1q[..., 2] >> 2], -1).astype(float)
    ym = 0.299 * c1e[..., 0] + 0.587 * c1e[..., 1] + 0.114 * c1e[..., 2]; om = (q(c1) != q(fb[s])).any(2) & ~glass; lumm = max(lumm, float(ym[om].max(initial=0)))
    if d.any(): per.append(dict(frame=s, t_ms=ts[s] - RI.T0, px=int(d.sum()), kept_edge_px=int(kept.sum()), cols=sorted(set(np.nonzero(d)[1].tolist()))))
rep['hunt_200_frames'] = dict(fw_vs_mock_1x_rgb565_px=tot1, frames_with_diff=len(per),
    optin_LV_RD_HM_OCC1_vs_mock_1x_rgb565_px=noocc, optin_occ_nearer_edge_kept_px=occ_only, changed_px_outside_mask=outside,
    max_luma_changed_px_outside_glass_fw=round(lumo, 1), max_luma_changed_px_outside_glass_mock_same_rgb565=round(lumm, 1), per_frame=per)
# 3. the locked stills
picks = {'idle': 40, 'mid': 120, 'full': 183, 'pulse': 119}
st = {}
for k, s in picks.items():
    L3 = np.array(Image.open(f'{LOCK}/stills/V2_{k}@3x.png').convert('RGB')); L1 = np.array(Image.open(f'{LOCK}/stills/V2_{k}@1x.png').convert('RGB'))
    F1 = fn[s]; F3 = F1.repeat(3, 0).repeat(3, 1)
    d1 = (q(L1) != q(F1)).any(2); d3 = (q(L3) != q(F3)).any(2)
    kept = d1 & (F1 == fb[s]).all(2)
    st[k] = dict(sim_frame=s, t_ms=ts[s] - RI.T0, lit=RI.lit_at(EV, ts[s]), locked3x_vs_mock_render_px=int((L3 != m3[s]).any(2).sum()),
        locked1x_is_centre_sample=bool((L1 == L3[1::3, 1::3]).all()), fw_vs_locked_1x_rgb565_px=int(d1.sum()), of_which_nearer_tower_edge_kept_px=int(kept.sum()),
        fw3x_vs_locked_3x_rgb565_subpx=int(d3.sum()), fw3x_vs_locked_3x_subpx_where_1x_centre_matches=int((d3 & np.repeat(np.repeat(~d1, 3, 0), 3, 1)).sum()))
    vis = (F3 // 3).copy(); vis[d3] = (255, 255, 0); vis[np.repeat(np.repeat(d1 & ~kept, 3, 0), 3, 1)] = (255, 0, 0); vis[np.repeat(np.repeat(kept, 3, 0), 3, 1)] = (0, 255, 255)
    sep = np.full((960, 12, 3), 255, np.uint8)
    Image.fromarray(np.hstack([L3, sep, F3, sep, vis])).save(f'{OUT}/cmp_V2_{k}@3x.png')
    Image.fromarray(F3).save(f'{OUT}/fw_V2_{k}@3x.png'); Image.fromarray(F1).save(f'{OUT}/fw_V2_{k}@1x.png')
rep['stills'] = st
# zoom: tower + contact, locked | fw for every moment (x4 of 3x)
rows = []
for k in ('idle', 'mid', 'pulse', 'full'):
    s = picks[k]; h = hm[ts[s]]; x0, y0 = 3 * h['L'] - 36, 3 * min(h['yt'], h['byt']) - 30
    L3 = np.array(Image.open(f'{LOCK}/stills/V2_{k}@3x.png').convert('RGB')); F3 = fn[s].repeat(3, 0).repeat(3, 1)
    cr = lambda a: Image.fromarray(a[y0:y0 + 168, x0:x0 + 96]).resize((288, 504), Image.NEAREST)
    rows.append((k, cr(L3), cr(F3)))
Z = Image.new('RGB', (4 * 600 + 20, 560), (10, 0, 12)); dr = ImageDraw.Draw(Z)
for i, (k, a, b) in enumerate(rows):
    Z.paste(a, (10 + i * 600, 40)); Z.paste(b, (10 + i * 600 + 300, 40)); dr.text((10 + i * 600, 12), f'{k}: LOCKED V2 | firmware', fill=(255, 176, 0))
Z.save(f'{OUT}/cmp_V2_zoom@3x.png')
# 4. firmware MP4 (6.0 s, 180 frames from the 200 sim frames, the mock's own mapping)
mp4 = f'{OUT}/fw_hunt_V2_3x.mp4'
p = subprocess.Popen(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '720x960', '-r', '30', '-i', '-', '-c:v', 'libx264',
    '-preset', 'slow', '-crf', '14', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', mp4], stdin=subprocess.PIPE)
for j in range(180): p.stdin.write(fn[min(199, int(j * 1000 / 30 / 30))].repeat(3, 0).repeat(3, 1).tobytes())
p.stdin.close(); assert p.wait() == 0
# 5. flag off: byte identity
def same(a, b, keys):
    return {k: int((ld(f'{a}/{k}.rgb') != ld(f'{b}/{k}.rgb')).any(3).sum()) for k in keys}
K = ('sel', 'mid', 'full', 'lockfire', 'locked', 'hunting', 'hunt')
H0 = run('new_hm0', S1, 'hm0'); rep['flag_off_LV_RD_HUNT_METER0_vs_3b8b3de_px'] = same(H0, B, K)
C0 = run('new_cg0', S0, 'cg0'); C0b = run('base_cg0', S0, 'cg0b'); rep['flag_off_LV_RD_CHARGE0_vs_3b8b3de_px'] = same(C0, C0b, K)
# 6. other locked screens: verify_port.py's render set, this tree vs 3b8b3de, byte for byte
def ms_of(n): return -(-100 * n // 3)
def screens(sim, tag):
    sc, names = [], []
    def add(scr, th, ns, nm):
        s = ['theme %d' % th, 'tick 257', 'go %d' % scr]
        for n in ns: s += ['tickto %d' % (257 * 30 + ms_of(n)), 'dump %s_%d' % (nm, n)]; names.append('%s_%d' % (nm, n))
        sc.append(s)
    add(2, 2, [7, 14, 21, 28, 35, 42, 63, 84, 105, 147], 'home')
    for scr, key in ((4, 'scan'), (3, 'term')):
        add(scr, 2, list(range(3, 45, 3)), key + '_green'); add(scr, 5, [21, 42], key + '_orange')
    add(4, 0, [21, 42], 'scan_glow')
    for scr, nm in ((2, 'home'), (4, 'scan'), (3, 'term')):
        for t in (0, 2, 5, 8): add(scr, t, [21], '%s_t%d' % (nm, t))
    d = f'{TMP}/{tag}'; os.makedirs(d, exist_ok=True)
    for i, s in enumerate(sc):
        p = f'{d}/s{i}.txt'; open(p, 'w').write('\n'.join(s) + '\n')
        r = subprocess.run([f'{SIMD}/{sim}', p, d, 'nocal'], cwd=REPO, capture_output=True); assert r.returncode == 0
    return d, names, len(sc)
dn, names, nrun = screens('new_nolog', 'scrN'); db, _, _ = screens('base_nolog', 'scrB')
diffs = {n: int((ld(f'{dn}/{n}.rgb') != ld(f'{db}/{n}.rgb')).any(3).sum()) for n in names}
rep['other_locked_screens'] = dict(renders=len(names), sim_runs=nrun, identical=sum(v == 0 for v in diffs.values()), diff_px_total=sum(diffs.values()))
json.dump(rep, open(f'{OUT}/verify.json', 'w'), indent=1)
print(json.dumps({k: v for k, v in rep.items() if k != 'hunt_200_frames'}, indent=1))
print(json.dumps({k: v for k, v in rep['hunt_200_frames'].items() if k != 'per_frame'}, indent=1))
shutil.rmtree(TMP)
