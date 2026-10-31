"""cyd_livescan harness proof. Runs tools/livescan/sim_live (same lv_core.h as the firmware, FAKE radio)
on scripts/main.txt, ticker.txt, calbot.txt (no saved cal), calbot_nvs.txt and calbot_recal.txt, writes PNGs to docs/livescan_verify/, and scores every dump of a
LOCKED screen against its clip frame on the pixels OUTSIDE the live overlay rects (metric = share of pixels
within RGB distance 40, same as tools/lock_sim/verify.py). ref|render|diff PNGs: red = distance > 40,
blue tint = live overlay area (not scored)."""
import sys, os, re, json, subprocess, numpy as np
from PIL import Image
SIM = sys.argv[1]; OUT = 'docs/livescan_verify'; RAW = '/tmp/lvraw'
os.makedirs(OUT, exist_ok=True); os.makedirs(RAW, exist_ok=True)
CACHE = '/workspace/cydport/cache/'
CLIP = {0: 'bootbot_bg_v2@3x.mp4', 1: 'gibsonbot_bg@3x.mp4', 2: 'terminalbot_bg@3x.mp4', 3: 'scanbot_bg_themes_v2@3x.mp4', 4: '8cc9946c'}
def clipfile(seg):
    c = 0 if seg == 0 else 1 if seg <= 7 else 2 if seg <= 13 else 3 if seg <= 19 else 4
    if c == 4: return [f for f in os.listdir(CACHE) if f.startswith('8cc9946c')][0]
    return CLIP[c] + '.npy'
TICK = [(24, 1, 127, 23), (164, 1, 236, 23)]
TTERM = (34, 272, 204, 305)   # item 11: the small terminal (buttons + prompt) is live on HOME/TERM/CTL; SCAN box below
MASK = {2: TICK + [TTERM], 3: TICK + [TTERM], 4: TICK + [(36, 188, 203, 253), (32, 275, 206, 308)],
        5: TICK + [(82, 36, 112, 44), (76, 51, 112, 59), (82, 66, 112, 74), (178, 246, 198, 252), TTERM], 0: [], 1: []}
OLD_TICK_ONLY = TICK   # strict = the baseline masks above (pre-hitlist overlays only)
def ovl_rects(out, nm):  # overlay-aware = strict masks + every live overlay the sim reports for that dump (masks, lit tints, BACKbot, press flash)
    m = re.search(r'^OVL %s((?: \S+)*)$' % re.escape(nm), out, re.M)
    if not m: return []
    return [(tok.split(':')[0],) + tuple(int(v) for v in tok.split(':')[1].split(',')) for tok in m.group(1).split()]
TAG = {'t1': 'WiFi chip lit', 't2': 'BLE chip lit', 't3': 'SCANbot lit', 't4': 'CONTROLbot face theme', 't5': 'CONTROLbot name plate',
       't6': 'BACKbot', 't7': 'BOOTbot black+grey (item 7)', 't8': 'terminal pin rail under old icon column (item 11)', 'f': 'press flash'}
def tagname(t, r):
    if t == 'm':
        return {(1, 1, 238, 23): 'ticker full width (crown+GIBSON icon removed)', (33, 153, 206, 256): 'SCAN emoji grid (4 tabs removed)'}.get(r, 'live value mask %s' % (r,))
    return TAG.get(t, 'hunt chip lit' if t.startswith('t') else t)
def png(a, p, s=2): Image.fromarray(a).resize((a.shape[1] * s, a.shape[0] * s), Image.NEAREST).save(p)
res = {}
for script, extra in (('main', []), ('ticker', []), ('calbot', ['nocal']), ('calbot_nvs', []), ('calbot_recal', [])):
    out = subprocess.run([SIM, 'tools/livescan/scripts/%s.txt' % script, RAW] + extra, check=True, capture_output=True, text=True).stdout
    open('%s/sim_%s_log.txt' % (OUT, script), 'w').write(out)
    for m in re.finditer(r'DUMP (\S+) scr=(\d+) seg=(-?\d+) clipframe=(-?\d+)', out):
        nm, scr, seg, cf = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4))
        r = np.fromfile('%s/%s.rgb' % (RAW, nm), np.uint8).reshape(320, 240, 3)
        prefix = {'scan': '', 'term': '', 'ticker': ''}
        png(r, '%s/%s.png' % (OUT, nm))
        if scr == 7: res[nm] = {'screen': 'CALbot (approved-board art + live terminal lines; scored by score_calbot.py)'}; continue
        if seg < 0: res[nm] = {'screen': 'TTY (live full-screen terminal, no clip)'}; continue
        ref = np.asarray(np.load(CACHE + clipfile(seg), mmap_mode='r')[cf])
        d = np.sqrt(((r.astype(np.int32) - ref.astype(np.int32)) ** 2).sum(2))
        keep = np.ones((320, 240), bool)
        for (x0, y0, x1, y1) in MASK[scr]: keep[y0:y1 + 1, x0:x1 + 1] = False
        close = float((d[keep] <= 40).mean() * 100)
        keep2 = keep.copy()
        why = {}; bad = (d > 40) & keep
        for (t, x0, y0, x1, y1) in ovl_rects(out, nm):
            k = int(bad[y0:y1 + 1, x0:x1 + 1].sum()); bad[y0:y1 + 1, x0:x1 + 1] = False
            if k: why[tagname(t, (x0, y0, x1, y1))] = why.get(tagname(t, (x0, y0, x1, y1)), 0) + k
            keep2[y0:y1 + 1, x0:x1 + 1] = False
        close2 = float((d[keep2] <= 40).mean() * 100)
        diff = (ref // 3).astype(np.uint8); diff[(d > 40) & keep] = [255, 0, 0]; diff[~keep] = diff[~keep] // 2 + np.array([0, 0, 90], np.uint8)
        sbs = np.concatenate([ref, np.full((320, 4, 3), 255, np.uint8), r, np.full((320, 4, 3), 255, np.uint8), diff], 1)
        png(sbs, '%s/%s_ref_render_diff.png' % (OUT, nm))
        res[nm] = {'seg': seg, 'clip': clipfile(seg), 'clip_frame': cf, 'close_pct_outside_overlays': round(close, 3), 'overlay_px': int((~keep).sum()),
                   'close_pct_overlay_aware': round(close2, 3), 'overlay_aware_px': int((~keep2).sum()),
                   'strict_drop_px_by_overlay': why, 'strict_drop_px_unexplained': int(bad.sum())}
        print('%-24s seg %2d frame %3d  strict %.3f%%  (masked px %d)  overlay-aware %.3f%%  (masked px %d)' % (nm, seg, cf, close, (~keep).sum(), close2, (~keep2).sum()))
        if why: print('    strict drop px: ' + ', '.join('%s %d' % kv for kv in why.items()) + ', unexplained %d' % bad.sum())
json.dump(res, open(OUT + '/scores.json', 'w'), indent=1)
# CALbot power-on flow video: every 30 ms sim frame, x3 nearest, H.264 yuv420p faststart (same recipe as ticker@3x.mp4)
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-framerate', '100/3',
                '-i', RAW + '/calbot_flow.rgb', '-vf', 'scale=720:960:flags=neighbor', '-c:v', 'libx264', '-crf', '18',
                '-pix_fmt', 'yuv420p', '-movflags', '+faststart', OUT + '/calbot_flow@3x.mp4'], check=True)
