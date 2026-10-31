"""Touch audit report (hit-list item 1). Runs sim_live on scripts/touch.txt: every screen is tapped on a 2 px grid
(each tap in a forked child, no side effects) and dumped. For each screen this writes docs/livescan_verify/touch/
<screen>_touch@2x.png = the render with every tap region tinted in its own colour (black = dead, tap does nothing)
and every DRAWN pressable element outlined in white; and touch_audit.txt = per drawn element: the action expected,
the share of its own pixels that fire it, any other action that fires inside it (mismatch), the tap-region bbox
size, and the dead count (drawn elements whose centre tap does nothing)."""
import sys, os, re, subprocess, numpy as np
from PIL import Image, ImageDraw
SIM = sys.argv[1]; OUT = 'docs/livescan_verify/touch'; RAW = '/tmp/lvtouch'
os.makedirs(OUT, exist_ok=True); os.makedirs(RAW, exist_ok=True)
log = subprocess.run([SIM, 'tools/livescan/scripts/touch.txt', RAW], check=True, capture_output=True, text=True).stdout
open(OUT + '/sim_touch_log.txt', 'w').write(log)
def mt(T, scan=False, ctl=False):   # mirrors lv_mt_btn / lv_mt_prompt_draw in lv_core.h
    if ctl: xs = [(36 + 34 * i, 66 + 34 * i) for i in range(5)]
    elif scan: xs = [(35, 88), (93, 146), (151, 204)]
    else: xs = [(36, 88), (93, 145), (150, 202)]
    return [(a, T + 11, b, T + 23) for a, b in xs], ((32, T + 24, 206, 308) if scan else (34, T + 24, 204, 305))
BACK = (221, 301, 239, 319)
def box(T, names, scan=False, ctl=False):
    b, p = mt(T, scan, ctl); return [(n, r) for n, r in zip(names, b)] + [(('tty',), p)]
HUNT = ('deauth', 'meta', 'hack', 'skimmer', 'flock', 'flipper', 'airtag', 'keyfob', 'drone', 'samsung')
H3 = ('home-crosshair-scan',), ('home-list',), ('home-menu-help',)
# drawn pressable elements per screen: (accepted action names, drawn rect)
DRAWN = {
 't_home': [(('boot-selftest',), (31, 52, 74, 112)), (('gibson-jackin-radar',), (78, 30, 160, 150)), (('ctl',), (165, 51, 208, 110)),
            (('scan',), (33, 156, 101, 255)), (('term',), (139, 156, 207, 255)), (('ticker->scan',), (1, 1, 238, 23))]
           + [(n, r) for n, r in box(272, H3)][:3] + [(('home-tty',), mt(272)[1])],
 't_scan': [(('wifi-chip-scan',), (32, 30, 74, 92)), (('ble-chip-scan',), (32, 93, 74, 152)), (('scanbot-scan-mix', 'scanbot-stop'), (78, 30, 165, 150)),
            (('gib-home',), (166, 58, 208, 122)), (('scan-button', 'stop'), (174, 165, 203, 206)), (('more-types', 'more-rows', 'detail-hunt'), (174, 210, 203, 252)),
            (('backbot',), BACK), (('ticker->scan',), (1, 1, 238, 23))]
           + box(275, [('crosshair-scan-all',), ('list',), ('menu-help',)], scan=True),
 't_term': [(HUNT, r) for r in [(35, 30, 70, 80), (170, 30, 205, 80), (35, 87, 70, 137), (170, 87, 205, 137), (35, 145, 70, 195),
            (170, 145, 205, 195), (35, 200, 70, 250), (80, 200, 115, 250), (124, 200, 160, 250), (170, 200, 205, 250)]]
           + [(('tty',), (75, 30, 165, 150)), (('backbot',), BACK), (('ticker->scan',), (1, 1, 238, 23))]
           + box(272, [('term-crosshair-scan',), ('term-list',), ('term-menu-help',)]),
 't_ctl': [(('theme',), (35, 33, 113, 48)), (('uptime',), (35, 49, 113, 63)), (('focus',), (35, 64, 113, 79)), (('multi',), (165, 175, 207, 237)),
           (('bg-motion',), (160, 238, 208, 258)), (('mix',), (35, 85, 70, 140)), (('rain-col',), (35, 145, 70, 190)), (('mashup',), (35, 195, 70, 250)),
           (('rain',), (165, 80, 205, 130)), (('rain-mode',), (170, 131, 205, 170)), (('orient', 'orientation'), (90, 212, 159, 258)),
           (('ctl-face-theme',), (75, 82, 164, 205)), (('backbot',), BACK), (('ticker->scan',), (1, 1, 238, 23))]
          + box(272, [('go-ble',), ('go-scan',), ('ctl-crosshair-scan',), ('ctl-list',), ('ctl-menu-help',)], ctl=True),
 't_radar_lock': [(('radar-ignore',), (5, 250, 76, 260)), (('radar-watch', 'radar-unwatch'), (84, 250, 155, 260)), (('radar-hunt', 'radar-hunt-stop'), (163, 250, 234, 260)),
                  (('backbot',), BACK)],
 't_tty': [(('backbot',), (204, 0, 239, 23))],
}
DRAWN['t_scan_found'] = DRAWN['t_scan']
DRAWN['t_list'] = [d for d in DRAWN['t_scan'] if d[1] != (174, 210, 203, 252)] + [(('more-rows', 'detail-hunt'), (174, 210, 203, 252))]
DRAWN['t_radar'] = [(('backbot',), BACK)]
def load_audit(nm):
    L = open('%s/%s.audit.txt' % (RAW, nm)).read().split('\n'); st = int(L[0]); i = L.index('--')
    names = L[1:i]; g = np.array([[int(v) for v in r.split()] for r in L[i + 1:] if r.strip()]); return st, names, g
def ok(acc, a): return any(a == n or a.startswith(n) for n in acc)
rep = []; dead_total = 0
pal = np.array([[230, 25, 75], [60, 180, 75], [255, 225, 25], [0, 130, 200], [245, 130, 48], [145, 30, 180], [70, 240, 240], [240, 50, 230],
                [210, 245, 60], [250, 190, 212], [0, 128, 128], [220, 190, 255], [170, 110, 40], [255, 250, 200], [128, 0, 0], [170, 255, 195],
                [128, 128, 0], [255, 215, 180], [0, 0, 128], [128, 128, 128]], np.uint8)
for nm in re.findall(r'^AUDIT (\S+)', log, re.M):
    st, names, g = load_audit(nm)
    r = np.fromfile('%s/%s.rgb' % (RAW, nm), np.uint8).reshape(320, 240, 3)
    full = np.kron(g, np.ones((st, st), int))[:320, :240]
    col = np.zeros((320, 240, 3), np.uint8); m = full >= 0
    col[m] = pal[full[m] % len(pal)]
    img = (r.astype(float) * 0.45 + col.astype(float) * 0.55).astype(np.uint8); img[~m] = (r[~m] * 0.25).astype(np.uint8)
    im = Image.fromarray(img).resize((480, 640), Image.NEAREST); d = ImageDraw.Draw(im)
    rep.append('== %s  (grid step %d px; %d actions)' % (nm, st, len(names)))
    for k, n in enumerate(names):
        ys, xs = np.nonzero(full == k)
        rep.append('   region %-22s bbox {%d,%d,%d,%d}  %dx%d%s' % (n, xs.min(), ys.min(), xs.max(), ys.max(), xs.max() - xs.min() + 1, ys.max() - ys.min() + 1, '' if min(xs.max() - xs.min(), ys.max() - ys.min()) + 1 >= 30 else '   (< 30 px)'))
    for acc, (x0, y0, x1, y1) in DRAWN.get(nm, []):
        d.rectangle([x0 * 2, y0 * 2, x1 * 2 + 1, y1 * 2 + 1], outline=(255, 255, 255))
        sub = g[-(-y0 // st):y1 // st + 1, -(-x0 // st):x1 // st + 1]; tot = sub.size   # grid taps that land inside the drawn rect
        hit = sum(int((sub == k).sum()) for k, n in enumerate(names) if ok(acc, n))
        other = {names[k]: int((sub == k).sum()) for k in set(sub.flatten()) if k >= 0 and not ok(acc, names[k])}
        ndead = int((sub < 0).sum())
        cx, cy = (x0 + x1) // 2 // st, (y0 + y1) // 2 // st; c = g[cy, cx]
        cdead = c < 0 or not ok(acc, names[c])
        dead_total += cdead
        rep.append('   drawn %-24s {%d,%d,%d,%d}  own taps fire %5.1f%%  dead taps %d  other %s  centre -> %s%s' % (
            '/'.join(acc), x0, y0, x1, y1, 100.0 * hit / tot, ndead, other or '-', names[c] if c >= 0 else 'none', '   <<< DEAD' if cdead else ''))
    for k, n in enumerate(names):
        ys, xs = np.nonzero(full == k); d.text((int(xs.mean()) * 2 - 20, int(ys.mean()) * 2 - 5), n[:14], fill=(255, 255, 255))
    im.save('%s/%s_touch@2x.png' % (OUT, nm))
rep.append('DEAD drawn targets (centre tap does nothing or the wrong thing): %d' % dead_total)
open(OUT + '/touch_audit.txt', 'w').write('\n'.join(rep) + '\n'); print('\n'.join(rep))
