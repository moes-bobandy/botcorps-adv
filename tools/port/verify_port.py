# Render the ported screens from the firmware (sim_live, same lv_core.h) at lock-aligned times and pixel-diff in RGB565
# against the LOCKED frames. Usage: python verify_port.py SIM EXTDIR LOCKDIR OUTDIR
import sys, os, math, subprocess, json
import numpy as np
from PIL import Image
SIM, EXT, LOCK, OUT = sys.argv[1:5]; os.makedirs(OUT, exist_ok=True); TMP = '/tmp/fwp/ver'; os.makedirs(TMP, exist_ok=True)
H = np.load(EXT + '/home.npz'); R = np.load(EXT + '/rain.npz'); M = np.load(EXT + '/port_meta.npz'); SG = np.load(EXT + '/scan.npz')
T0 = int(H['T0'])
def q(a): a = np.asarray(a).astype(int); return np.stack([a[..., 0] >> 3, a[..., 1] >> 2, a[..., 2] >> 3], -1)
def ms_of(n):
    m = -(-100 * n // 3); assert m * 30 // 1000 == n and m // 140 == (n * 1000 // 30) // 140, n; return m
def render(scr, theme, ns, tag):
    sc = ['theme %d' % theme, 'tick 257', 'go %d' % scr]; t0 = 257 * 30; out = []
    for n in ns: sc += ['tickto %d' % (t0 + ms_of(n)), 'dump %s_%d' % (tag, n)]
    p = TMP + '/%s.txt' % tag; open(p, 'w').write('\n'.join(sc) + '\n')
    r = subprocess.run([SIM, p, TMP, 'nocal'], capture_output=True, text=True); assert r.returncode == 0, r.stderr
    d = [l for l in r.stdout.splitlines() if l.startswith('DUMP')]
    for n in ns: out.append(np.fromfile('%s/%s_%d.rgb' % (TMP, tag, n), np.uint8).reshape(320, 240, 3))
    return out, d
def sbs(lock, fw, mask, name, note):
    d = (q(lock) != q(fw)).any(-1); heat = np.zeros_like(fw); heat[mask] = (60, 60, 60); heat[d & mask] = (255, 0, 0); heat[d & ~mask] = (90, 0, 90)
    im = np.concatenate([lock, np.full((320, 4, 3), 255, np.uint8), fw, np.full((320, 4, 3), 255, np.uint8), heat], 1)
    Image.fromarray(im).resize((im.shape[1] * 2, 640), Image.NEAREST).save(OUT + '/' + name + '.png'); Image.fromarray(fw).save(OUT + '/' + name + '_fw.png')
    return dict(png=OUT + '/' + name + '.png', fw_png=OUT + '/' + name + '_fw.png', note=note)
def stats(lock, fw, mask):
    d = np.abs(q(lock) - q(fw)).max(-1); m = mask
    return dict(px=int(m.sum()), eq565=int((d[m] == 0).sum()), gt1=int((d[m] > 1).sum()), gt4=int((d[m] > 4).sum()), mean_abs565=round(float(d[m].mean()), 3), max565=int(d[m].max()))
rep = {}
# HOME vs locked O frames (theme 2 = the locked green)
ns = [7, 14, 21, 28, 35, 42, 63, 84, 105, 147]
fw, d = render(2, 2, ns, 'home'); own = M['home_own']; G = M['home_G']; Dm = M['home_D']
under = G & ~Dm; ownb = own & ~under
for n, f in zip(ns, fw):
    L = H['O'][n]
    rep['home_n%d' % n] = dict(owned_base=stats(L, f, ownb), glow_over_clip=stats(L, f, under), outside=stats(L, f, ~own))
    if n in (42, 21, 105): rep['home_n%d' % n].update(sbs(L, f, own, 'home_n%03d' % n, 'LOCKED O frame %d | firmware sim | diff (grey owned, red owned diff, purple diff outside the port)' % n))
# SCAN rain chip vs gaps3 (green = theme 2, orange = theme 5); TERM likewise
for scr, key in ((4, 'scan'), (3, 'term')):
    for th, t in (('green', 2), ('orange', 5)):
        ns = list(range(3, 45, 3)) if th == 'green' else [21, 42]
        fw, _ = render(scr, t, ns, '%s_%s' % (key, th)); V = M['%s_V' % key]; Dm = M['%s_D' % key] & ~M['%s_G' % key]
        for n, f in zip(ns, fw):
            k = (T0 + ms_of(n) // 140) % 10 - T0 % 10; L = R['%s_%s' % (key, th)][k % 10]
            box = np.zeros((320, 240), bool); box[45:151, 78:165] = True
            rep['%s_%s_n%d' % (key, th, n)] = dict(step=int((T0 + ms_of(n) // 140) % 10), rain_chip_owned=stats(L, f, Dm), rain_px=stats(L, f, V), chip_box_not_owned=stats(L, f, box & ~Dm))
            if n in (21, 42): rep['%s_%s_n%d' % (key, th, n)].update(sbs(L, f, Dm, '%s_rain_%s_n%03d' % (key, th, n), 'LOCKED main-bot-rain gaps3 %s (%s) | firmware sim | diff' % (key, th)))
# SCAN glow vs LOCKED ui_scan_glow (theme 0 GRAPE base): peak (s=1) and trough (s=0)
pk = np.array(Image.open(LOCK + '/ui_scan_glow/scan_glow_peak_3x.png').convert('RGB'))[1::3, 1::3]
bs = np.array(Image.open(LOCK + '/ui_scan_glow/scan_current_base_3x.png').convert('RGB'))[1::3, 1::3]
fw, _ = render(4, 0, [21, 42], 'scan_glow')
names = list(SG['names']); face = SG['face']; ring = SG['ring']
for n, f, L in ((21, fw[0], pk), (42, fw[1], bs)):
    r = {}
    for i, nm in enumerate(names):
        r[nm + '_face'] = stats(L, f, face[i]); r[nm + '_halo'] = stats(L, f, ring[i])
    win = np.zeros((320, 240), bool)
    for i in range(3):
        x0, y0, x1, y1 = [int(v) for v in np.argwhere(ring[i] | face[i])[:, ::-1].min(0).tolist() + np.argwhere(ring[i] | face[i])[:, ::-1].max(0).tolist()]; win[y0:y1 + 1, x0:x1 + 1] = True
    r['chip_windows_rest'] = stats(L, f, win & ~(ring.any(0) | face.any(0)))
    rep['scanglow_n%d' % n] = r
    rep['scanglow_n%d' % n].update(sbs(L, f, ring.any(0) | face.any(0), 'scan_glow_%s' % ('peak' if n == 21 else 'trough'), 'LOCKED ui_scan_glow %s (1x of 3x) | firmware sim theme 0 | diff' % ('peak' if n == 21 else 'trough')))
# themes: full renders for a look at every theme family
for scr, nm in ((2, 'home'), (4, 'scan'), (3, 'term')):
    for t in (0, 2, 5, 8):
        f, _ = render(scr, t, [21], '%s_t%d' % (nm, t)); Image.fromarray(f[0]).resize((480, 640), Image.NEAREST).save(OUT + '/theme_%s_t%02d_peak.png' % (nm, t))
json.dump(rep, open(OUT + '/verify.json', 'w'), indent=1); print(json.dumps(rep, indent=0)[:6000])
