"""Hit-list evidence crops (nearest 3x / 6x) + numeric checks -> docs/livescan_verify/evidence/.
usage: evidence_sheets.py AFTER_DIR BEFORE_DIR FWPAL1_DIR FWPAL2_DIR I11_BEFORE I11_AFTER BOOTFLOW_BEFORE BOOTFLOW_AFTER RADAR_DIR
AFTER/BEFORE: evidence.txt + evidence_ctl.txt (+ evidence_themes.txt for AFTER) dumps from this tree's sim / the d9f0461 sim."""
import sys, os, colorsys, numpy as np
from PIL import Image, ImageDraw, ImageFont
A, B, F1, F2, I11B, I11A, BFB, BFA, RD = sys.argv[1:10]
OUT = 'docs/livescan_verify/evidence'; os.makedirs(OUT, exist_ok=True)
try: FN = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 22)
except Exception: FN = ImageFont.load_default()
LOG = []
def say(*a): s = ' '.join(str(x) for x in a); LOG.append(s); print(s)
def ld(d, n): return np.fromfile('%s/%s.rgb' % (d, n), np.uint8).reshape(320, 240, 3)
def frame(path, k): return np.fromfile(path, np.uint8, 230400, offset=k * 230400).reshape(320, 240, 3)
def up(a, s, crop=None):
    if crop: x0, y0, x1, y1 = crop; a = a[y0:y1 + 1, x0:x1 + 1]
    return Image.fromarray(np.ascontiguousarray(a)).resize((a.shape[1] * s, a.shape[0] * s), Image.NEAREST)
def sheet(name, items, cols=None):
    cols = cols or len(items); rows = (len(items) + cols - 1) // cols
    w = max(i.width for _, i in items); h = max(i.height for _, i in items)
    S = Image.new('RGB', (cols * (w + 16) + 16, rows * (h + 44) + 16), (24, 24, 28)); D = ImageDraw.Draw(S)
    for k, (lab, im) in enumerate(items):
        x = 16 + (k % cols) * (w + 16); y = 16 + (k // cols) * (h + 44)
        D.text((x, y), lab, fill=(255, 255, 255), font=FN); S.paste(im, (x, y + 30))
    S.save('%s/%s.png' % (OUT, name)); say('wrote', name + '.png', S.size)
def bbox(m):
    ys, xs = np.nonzero(m)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())) if len(xs) else None
def pad(r, p): return (max(0, r[0] - p), max(0, r[1] - p), min(239, r[2] + p), min(319, r[3] + p))
def lum(a): return a[..., 0] * .299 + a[..., 1] * .587 + a[..., 2] * .114

# ---- item 5: press = the element's own artwork brightened, nothing outside it, hue kept ----
say('== item 5 press brighten (after = this tree, before = d9f0461 haze)')
P5 = [('chip (HOME SCANbot)', 'e_home', 'e_p_chip', (33, 156, 101, 255)), ('list row', 'e_list', 'e_p_row', None),
      ('CTL toggle RAIN', 'c_ctl', 'c_p_rain', (165, 80, 205, 130)), ('BACKbot', 'e_detail_wifi', 'e_p_backbot', (221, 301, 239, 319))]
items = []
for lab, n0, n1, drawn in P5:
    a0, a1, b0, b1 = ld(A, n0), ld(A, n1), ld(B, n0), ld(B, n1)
    da = (a0 != a1).any(2); db = (b0 != b1).any(2)
    ra = bbox(da); rb = bbox(db); r = pad((min(ra[0], rb[0]), min(ra[1], rb[1]), max(ra[2], rb[2]), max(ra[3], rb[3])), 4)
    if drawn is None: drawn = ra
    ins = np.zeros_like(da); ins[drawn[1]:drawn[3] + 1, drawn[0]:drawn[2] + 1] = True
    live = np.zeros_like(da); live[51:60, 76:113] = True   # CTL uptime value: a live clock mask (m:76,51,112,59), redrawn by the RAIN event, not the press
    out_px = int((da & ~ins & ~live).sum())
    m = da & ins; l0 = lum(a0[m].astype(float)); l1 = lum(a1[m].astype(float))
    ratio = (l1.sum() / max(l0.sum(), 1))
    unc = a0[m].max(1) <= 188   # below lv_hi's headroom knee: the full x1.35 applies
    ratio_u = lum(a1[m][unc].astype(float)).sum() / max(lum(a0[m][unc].astype(float)).sum(), 1)
    hs = []
    for p0, p1 in zip(a0[m][::3], a1[m][::3]):
        h0, s0, v0 = colorsys.rgb_to_hsv(*(p0 / 255.)); h1, s1, v1 = colorsys.rgb_to_hsv(*(p1 / 255.))
        if s0 > .25 and v0 > .15: d = abs(h0 - h1); hs.append(min(d, 1 - d) * 360)
    cols0 = len({tuple(p) for p in a0[ins].reshape(-1, 3)}); cols1 = len({tuple(p) for p in a1[ins].reshape(-1, 3)})
    say('  %-20s drawn %s  changed px %d  outside drawn %d  luma x%.2f (x%.2f on px below the white knee)  hue drift mean %.1f deg max %.1f  distinct colours %d -> %d  (d9 haze changed %d px, bbox %s)'
        % (lab, drawn, int(da.sum()), out_px, ratio, ratio_u, np.mean(hs) if hs else 0, max(hs) if hs else 0, cols0, cols1, int(db.sum()), rb))
    items += [(lab + ' d9 idle', up(b0, 6, r)), ('d9 PRESSED (haze)', up(b1, 6, r)), ('now idle', up(a0, 6, r)), ('now PRESSED', up(a1, 6, r))]
sheet('item5_press_before_after_6x', items, 4)

# ---- item 2: TTY BACKbot shade removed ----
r = (190, 0, 239, 30)
sheet('item2_tty_backbot_before_after_6x', [('TTY d9f0461', up(ld(B, 'c_tty'), 6, r)), ('TTY now', up(ld(A, 'c_tty'), 6, r))])
# ---- item 3: CTL toggles visibly change ----
r = (160, 76, 210, 175)
sheet('item3_ctl_toggle_on_off_6x', [('d9 RAIN on', up(ld(B, 'c_rain_on'), 6, r)), ('d9 RAIN off', up(ld(B, 'c_rain_off'), 6, r)),
                                     ('now RAIN on', up(ld(A, 'c_rain_on'), 6, r)), ('now RAIN off', up(ld(A, 'c_rain_off'), 6, r))], 4)
r = (30, 80, 75, 145)
sheet('item3_ctl_mix_6x', [('d9 MIX 0', up(ld(B, 'c_rain_on'), 6, r)), ('d9 MIX 1', up(ld(B, 'c_mix1'), 6, r)),
                           ('now MIX 0', up(ld(A, 'c_rain_on'), 6, r)), ('now MIX 1', up(ld(A, 'c_mix1'), 6, r))], 4)
for st, n in (('RAIN on', 'c_rain_on'), ('RAIN off', 'c_rain_off')):
    say('item3 %s toggle region px differing d9 vs now: %d' % (st, int((ld(A, n)[76:176, 160:211] != ld(B, n)[76:176, 160:211]).any(2).sum())))
say('item3 now RAIN on->off px changed in toggle: %d (d9: %d)' % (int((ld(A, 'c_rain_on')[76:176, 160:211] != ld(A, 'c_rain_off')[76:176, 160:211]).any(2).sum()),
                                                                   int((ld(B, 'c_rain_on')[76:176, 160:211] != ld(B, 'c_rain_off')[76:176, 160:211]).any(2).sum())))
# ---- item 4: CONTROLbot face tap = theme, stays on CTL ----
sheet('item4_ctl_face_theme_3x', [('d9: face tap -> TTY', up(ld(B, 'c_face_tap_settled'), 3)), ('now before tap', up(ld(A, 'c_mix1'), 3)),
                                  ('now face tap -> next theme', up(ld(A, 'c_face_tap_settled'), 3)), ('now theme bar tap', up(ld(A, 'e_ctl_bar_theme2'), 3))], 4)
# ---- item 6: palette cache fix ----
d = (ld(F1, 'scan_detail') != ld(F2, 'scan_detail')).any(2)
say('item6 SIM_FWPAL=1 (pre-fix cache) vs SIM_FWPAL=2 (fixed) scan_detail: %d px differ, bbox %s' % (int(d.sum()), bbox(d)))
sheet('item6_fwpal_scan_detail_3x', [('SIM_FWPAL=1 pre-fix', up(ld(F1, 'scan_detail'), 3)), ('SIM_FWPAL=2 fixed', up(ld(F2, 'scan_detail'), 3))])
# ---- item 7: BOOTbot white -> grey ----
say('== item 7 BOOTbot white->grey')
it = []
for lab, k in (('boot', 30), ('intro', 120), ('home', 300)):
    b0, a0 = frame(BFB, k), frame(BFA, k); dd = (b0 != a0).any(2); dd[258:319, 26:213] = False   # terminal box: item 11, judged separately
    r = bbox(dd)
    newc = {tuple(p) for p in a0[dd].reshape(-1, 3)}; oldc = {tuple(p) for p in b0[dd].reshape(-1, 3)}
    say('  %s frame %d: %d px changed, bbox %s, before colours %s -> after colours %s' % (lab, k, int(dd.sum()), r, sorted(tuple(int(v) for v in c) for c in oldc), sorted(tuple(int(v) for v in c) for c in newc)))
    rr = pad(r, 2); s = 6 if (rr[2] - rr[0]) < 90 else 3
    it += [('%s before (%dx)' % (lab, s), up(b0, s, rr)), ('%s after' % lab, up(a0, s, rr))]
sheet('item7_bootbot_grey_before_after', it, 2)
# ---- item 9: HUNT on every tracked list ----
sheet('item9_hunt_on_lists_3x', [('WIFI list detail', up(ld(A, 'e_detail_wifi'), 3)), ('BLE list detail', up(ld(A, 'e_detail_ble'), 3)),
                                 ('tile 3 list detail', up(ld(A, 'e_detail_cat3'), 3))], 3)
# ---- item 11: terminal buttons in the box ----
BOX = (26, 258, 212, 318)
it = []
for n in ('home', 'term', 'scan', 'ctl'):
    b0, a0 = ld(I11B, n), ld(I11A, n)
    o = np.ones((320, 240), bool); o[BOX[1]:BOX[3] + 1, BOX[0]:BOX[2] + 1] = False
    say('item11 %s: px changed outside the terminal box %d (before = aee6495 WIP render)' % (n, int(((b0 != a0).any(2) & o).sum())))
    it += [('%s before' % n, up(b0, 3, BOX)), ('%s after' % n, up(a0, 3, BOX))]
sheet('item11_terminal_box_before_after_3x', it, 2)
sheet('item11_terminal_box_after_6x', [('HOME', up(ld(I11A, 'home'), 6, BOX)), ('CTL', up(ld(I11A, 'ctl'), 6, BOX))], 1)
sheet('item11_box_all_themes_3x', [('%s t%d' % (s, t), up(ld(A, 'th_%s%d' % (s, t)), 3, BOX)) for t in range(6) for s in ('home', 'ctl')], 4)
# ---- items 8 / 13: radar key frames (firmware sim) ----
ks = ['k1_base', 'k2_cycle_glow', 'k3_cycle_3', 'k4_locked', 'k5_watch', 'k6_hunt', 'k7_hunt_ticker', 'k8_ignore']
sheet('item8_radar_keyframes_3x', [(k, up(ld(RD, k), 3)) for k in ks], 4)
open('%s/evidence_checks.txt' % OUT, 'w').write('\n'.join(LOG) + '\n')
