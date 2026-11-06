# Items 2 + 4 (fw-fix7-1106) proof on the firmware's own lv_core.h (sim_live):
#   item 4: LV_SC_BOOT frames 0..84 vs the generator's target (raw clip + MORPHEUSbot rule, gen_boot_morpheus.py), BOOT -> HOME
#           (theme 5) with no HOME_INTRO frame, PNGs.
#   item 2: 26 screens (grid + 12 descs, theme 2 green / theme 5 orange) vs the LOCKED 1x pixels in RGB565, y 24..319 outside
#           BACKbot {221,301,239,319}; tap tour HOME -> MORPHEUSbot -> GLOSS -> 12 chips -> DESC -> BACKbot ... -> HOME; rain motion.
# Usage (repo root): python tools/port/verify_glossary.py SIM BOOTTARGET.npy OUTDIR_ITEM4 OUTDIR_ITEM2 REPORT.json
import sys, os, json, subprocess, re
import numpy as np
from PIL import Image
SIM, BT, O4, O2, REP = sys.argv[1:6]
for d in (O4, O2): os.makedirs(d, exist_ok=True)
TMP = '/tmp/gl/ver'; os.makedirs(TMP, exist_ok=True)
LOCK = '/workspace/cyd-lock-store/LOCKED'
NAMES = ['WIFI', 'BLUETOOTH', 'DEAUTH', 'HACK', 'FLOCK', 'DRONE', 'AIR TAG', 'KEYFOB', 'META', 'SKIMMER', 'FLIPPER', 'SAMSUNG']
FILES = [n.lower().replace(' ', '_') for n in NAMES]
CHIP = {n: v for n, v in json.load(open(LOCK + '/ui_glossary_morpheus_rain/tap_rects_big_morpheus.json'))['chip_tap_rects_1x_xywh'].items()}
def k565(a): a = np.asarray(a).astype(np.int64); return (a[..., 0] >> 3) << 11 | (a[..., 1] >> 2) << 5 | (a[..., 2] >> 3)
def run(name, lines):
    p = '%s/%s.txt' % (TMP, name); open(p, 'w').write('\n'.join(lines) + '\n')
    r = subprocess.run([SIM, p, TMP, 'nocal'], capture_output=True, text=True); assert r.returncode == 0, r.stderr[-2000:]
    return r.stdout
def img(n): return np.fromfile('%s/%s.rgb' % (TMP, n), np.uint8).reshape(320, 240, 3)
def dumps(out): return {m.group(1): dict(scr=int(m.group(2)), seg=int(m.group(3)), cf=int(m.group(4)), t=int(m.group(5))) for m in re.finditer(r'DUMP (\S+) scr=(-?\d+) seg=(-?\d+) clipframe=(-?\d+) t=(\d+)', out)}
def up3(a): return np.repeat(np.repeat(a, 3, 0), 3, 1)
def save3(a, p): Image.fromarray(up3(a)).save(p); return p
def lock1(p): a = np.array(Image.open(p).convert('RGB')); return a[1::3, 1::3]
rep = {}
# ================= item 4
tgt = np.load(BT)
out = run('boot', ['theme 5', 'go 0'] + ['tick 1\ndump b%03d' % i for i in range(88)])
D = dumps(out)
bad = {i: int((k565(img('b%03d' % i)) != tgt[i]).sum()) for i in range(85)}
scrs = [D['b%03d' % i]['scr'] for i in range(88)]
rep['item4'] = dict(frames_vs_target_px=bad, frames_all_zero=not any(bad.values()), screens_seen=sorted(set(scrs)),
                    home_intro_frames=scrs.count(1), boot_frames=scrs.count(0),
                    first_home_dump={k: v for k, v in D.items() if v['scr'] == 2 and v['cf'] >= 0 and k == min(kk for kk, vv in D.items() if vv['scr'] == 2 and vv['cf'] >= 0)})
# boot -> home: the last boot frame, then the first HOME frame (theme 5)
last = max(i for i in range(88) if scrs[i] == 0); h0 = min(i for i in range(88) if scrs[i] == 2 and D['b%03d' % i]['cf'] >= 0)
ref = run('home5', ['theme 5', 'go 2', 'tick 1', 'dump home5_f0'])
hm = np.ones((320, 240), bool); hm[:24] = False; hm[272:306, 34:205] = False   # live ticker + terminal box (time-dependent overlays)
rep['item4']['home_frame0_seg'] = D['b%03d' % h0]['seg']
rep['item4']['home_frame0_vs_direct_theme5_home_px'] = int(((img('b%03d' % h0) != img('home5_f0')).any(-1) & hm).sum())
rep['item4']['home_frame0_equals_direct_theme5_home'] = rep['item4']['home_frame0_vs_direct_theme5_home_px'] == 0
rep['item4']['between_last_boot_and_home0'] = [D['b%03d' % i] for i in range(last, h0 + 1)]
p4 = {}
p4['boot_f015'] = save3(img('b015'), O4 + '/boot_f015_3x.png')
p4['boot_f070'] = save3(img('b070'), O4 + '/boot_f070_3x.png')
p4['boot_last'] = save3(img('b%03d' % last), O4 + '/boot_last_3x.png')
gap = np.full((320, 4, 3), 255, np.uint8)
p4['boot_to_home_orange'] = save3(np.concatenate([img('b%03d' % last), gap, img('b%03d' % h0)], 1), O4 + '/boot_to_home_orange_3x.png')
# wipe strip (frames 72..80) for the record
p4['boot_wipe_f072_f080'] = save3(np.concatenate(sum([[img('b%03d' % i), gap] for i in range(72, 81, 2)], [])[:-1], 1), O4 + '/boot_wipe_f072_f080_3x.png')
rep['item4']['png'] = p4
# ================= item 2: 26 screens, pixel diff
m = np.zeros((320, 240), bool); m[24:, :] = True; m[301:320, 221:240] = False
def cxy(n): x, y, w, h = CHIP[n]; return x + w // 2, y + h // 2
tab = {}
for th, t in (('green', 2), ('orange', 5)):
    L = ['theme %d' % t, 'motion 0', 'go 9', 'tick 3', 'dump grid_%s' % th]
    for d, n in enumerate(NAMES):
        L += ['tap %d %d' % cxy(n), 'tick 6', 'dump desc_%s_%s' % (FILES[d], th), 'tap 230 310', 'tick 6']
    out = run('diff_' + th, L); D = dumps(out)
    g = lock1(LOCK + '/ui_glossary_morpheus_rain/grid_big_morpheus_chiprain_black_soft_final_%s_3x.png' % th)
    tab['grid_' + th] = dict(scr=D['grid_' + th]['scr'], diff=int(((k565(img('grid_' + th)) != k565(g)) & m).sum()),
                             rim_row0_diff=int((k565(img('grid_' + th))[0] != k565(g)[0]).sum() + (k565(img('grid_' + th))[1:24, [0, 239]] != k565(g)[1:24, [0, 239]]).sum()))
    for d, f in enumerate(FILES):
        l = lock1(LOCK + '/ui_glossary/desc_%s_%s_3x.png' % (f, th)); k = 'desc_%s_%s' % (f, th)
        tab[k] = dict(scr=D[k]['scr'], diff=int(((k565(img(k)) != k565(l)) & m).sum()),
                      rim_row0_diff=int((k565(img(k))[0] != k565(l)[0]).sum() + (k565(img(k))[1:24, [0, 239]] != k565(l)[1:24, [0, 239]]).sum()))
rep['item2_pixel_diff'] = tab
rep['item2_all_26_zero'] = all(v['diff'] == 0 for v in tab.values()) and len(tab) == 26
p2 = {}
p2['gloss_grid_green'] = save3(img('grid_green'), O2 + '/gloss_grid_green_3x.png')
p2['gloss_grid_orange'] = save3(img('grid_orange'), O2 + '/gloss_grid_orange_3x.png')
p2['desc_air_tag_green'] = save3(img('desc_air_tag_green'), O2 + '/desc_air_tag_green_3x.png')
p2['desc_wifi_orange'] = save3(img('desc_wifi_orange'), O2 + '/desc_wifi_orange_3x.png')
p2['desc_bluetooth_green'] = save3(img('desc_bluetooth_green'), O2 + '/desc_bluetooth_green_3x.png')
out = run('multi', ['theme 8', 'motion 0', 'go 9', 'tick 3', 'dump grid_multi', 'tap %d %d' % cxy('AIR TAG'), 'tick 6', 'dump desc_air_tag_multi',
                    'theme 0', 'go 9', 'tick 3', 'dump grid_grape'])
p2['gloss_grid_multi'] = save3(img('grid_multi'), O2 + '/gloss_grid_multi_3x.png')
p2['desc_air_tag_multi'] = save3(img('desc_air_tag_multi'), O2 + '/desc_air_tag_multi_3x.png')
p2['gloss_grid_grape'] = save3(img('grid_grape'), O2 + '/gloss_grid_grape_3x.png')
rep['item2_multi_theme8_px_differ_from_ice'] = None
# ================= item 2: tap tour + rain motion + press flash
T = ['theme 2', 'motion 1', 'go 2', 'tick 10', 'dump tour_home', 'tap 52 82', 'dump tour_home_pressed', 'tick 6', 'dump tour_g0']
for k in range(1, 6): T += ['tick 5', 'dump tour_g%d' % k]   # 5 x 150 ms: one rain step each (140 ms)
for d, n in enumerate(NAMES):
    T += ['tap %d %d' % cxy(n), 'dump tour_press_%d' % d, 'tick 6', 'dump tour_desc_%d' % d, 'tap 230 310', 'dump tour_backpress_%d' % d, 'tick 6', 'dump tour_back_%d' % d]
T += ['tap 230 310', 'tick 6', 'dump tour_home_again', 'motion 0', 'tap 52 82', 'tick 6', 'dump tour_still0']
for k in range(1, 6): T += ['tick 5', 'dump tour_still%d' % k]
T += ['motion 1', 'tap 120 10', 'tick 6', 'dump tour_ticker_scan', 'tap 230 310', 'tick 6', 'tap 52 82', 'tick 6', 'tap %d %d' % cxy('META'), 'tick 6', 'tap 120 10', 'tick 6', 'dump tour_desc_ticker_scan']
out = run('tour', T); D = dumps(out); taps = re.findall(r't=(\d+) tap (\d+),(\d+) -> (\S+(?: TAG)?) \(scr=(\d+)', out)
rain = np.zeros((320, 240), bool); rain[30:153, 74:165] = True
tick = np.zeros((320, 240), bool); tick[1:24, 1:239] = True
steps = []
for k in range(1, 6):
    a, b = k565(img('tour_g%d' % (k - 1))), k565(img('tour_g%d' % k))
    steps.append(dict(rain_px_changed=int(((a != b) & rain).sum()), other_px_changed=int(((a != b) & ~rain & ~tick).sum()), t=D['tour_g%d' % k]['t']))
still = []
for k in range(1, 6):
    a, b = k565(img('tour_still0')), k565(img('tour_still%d' % k))
    still.append(dict(rain_px_changed=int(((a != b) & rain).sum()), other_px_changed=int(((a != b) & ~tick).sum())))
gl_g = lock1(LOCK + '/ui_glossary_morpheus_rain/grid_big_morpheus_chiprain_black_soft_final_green_3x.png')
tour = []
for d, n in enumerate(NAMES):
    l = lock1(LOCK + '/ui_glossary/desc_%s_green_3x.png' % FILES[d])
    x, y, w, h = CHIP[n]; cm = np.zeros((320, 240), bool); cm[y:y + h, x:x + w] = True
    pa, pre = k565(img('tour_press_%d' % d)), k565(img('tour_back_%d' % (d - 1)) if d else img('tour_g5'))
    bb = np.zeros((320, 240), bool); bb[301:320, 221:240] = True
    bpa, bpre = k565(img('tour_backpress_%d' % d)), k565(img('tour_desc_%d' % d))
    tour.append(dict(device=n, tap_result=[tp[3] for tp in taps if (int(tp[1]), int(tp[2])) == cxy(n)][0] if True else None,
                     press_px_brightened_in_chip=int(((pa != pre) & cm).sum()), press_px_changed_outside_chip_rain_ticker=int(((pa != pre) & ~cm & ~rain & ~tick).sum()),
                     desc_scr=D['tour_desc_%d' % d]['scr'], desc_diff_vs_lock=int(((k565(img('tour_desc_%d' % d)) != k565(l)) & m).sum()),
                     backbot_press_px_in_backbot=int(((bpa != bpre) & bb).sum()), backbot_press_px_outside=int(((bpa != bpre) & ~bb & ~tick).sum()),
                     back_scr=D['tour_back_%d' % d]['scr']))
rep['item2_tour'] = dict(home_scr=D['tour_home']['scr'], tap_morpheus=[tp[3] for tp in taps][0], gloss_scr=D['tour_g0']['scr'],
                         home_press_px_changed=int((k565(img('tour_home_pressed')) != k565(img('tour_home'))).sum()),
                         rain_motion_on=steps, rain_motion_off=still, devices=tour,
                         final_back_scr=D['tour_home_again']['scr'], ticker_from_gloss_scr=D['tour_ticker_scan']['scr'],
                         ticker_from_desc_scr=D['tour_desc_ticker_scan']['scr'], taps=[(int(a), int(b), int(c), r, int(s)) for a, b, c, r, s in taps])
rep['item2_png'] = p2
# rain motion strip
save3(np.concatenate(sum([[img('tour_g%d' % k)[26:157, 70:169], np.full((131, 3, 3), 255, np.uint8)] for k in range(6)], [])[:-1], 1), O2 + '/gloss_rain_steps_green_3x.png')
rep['item2_png']['gloss_rain_steps_green'] = O2 + '/gloss_rain_steps_green_3x.png'
json.dump(rep, open(REP, 'w'), indent=1)
print(json.dumps(dict(item4_zero=rep['item4']['frames_all_zero'], home_intro_frames=rep['item4']['home_intro_frames'], home0_ok=rep['item4']['home_frame0_equals_direct_theme5_home'],
                      item2_26_zero=rep['item2_all_26_zero'], diffs={k: v['diff'] for k, v in tab.items()}, rims={k: v['rim_row0_diff'] for k, v in tab.items()},
                      rain_on=steps, rain_off=still, tour=[(t['device'], t['tap_result'], t['desc_scr'], t['desc_diff_vs_lock'], t['press_px_brightened_in_chip'], t['press_px_changed_outside_chip_rain_ticker'], t['backbot_press_px_in_backbot'], t['backbot_press_px_outside'], t['back_scr']) for t in tour],
                      tour_misc={k: rep['item2_tour'][k] for k in ('home_scr', 'tap_morpheus', 'gloss_scr', 'home_press_px_changed', 'final_back_scr', 'ticker_from_gloss_scr', 'ticker_from_desc_scr')}), indent=None))
