"""cyd_livescan harness proof. Runs tools/livescan/sim_live (same lv_core.h as the firmware, FAKE radio)
on scripts/main.txt, ticker.txt, calbot.txt (no saved cal) and calbot_nvs.txt, writes PNGs to docs/livescan_verify/, and scores every dump of a
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
MASK = {2: TICK, 3: TICK + [(34, 272, 181, 305)], 4: TICK + [(36, 188, 203, 253), (32, 275, 182, 308)],
        5: TICK + [(82, 36, 112, 44), (76, 51, 112, 59), (82, 66, 112, 74), (178, 246, 198, 252)], 0: [], 1: []}
def png(a, p, s=2): Image.fromarray(a).resize((a.shape[1] * s, a.shape[0] * s), Image.NEAREST).save(p)
res = {}
for script, extra in (('main', []), ('ticker', []), ('calbot', ['nocal']), ('calbot_nvs', [])):
    out = subprocess.run([SIM, 'tools/livescan/scripts/%s.txt' % script, RAW] + extra, check=True, capture_output=True, text=True).stdout
    open('%s/sim_%s_log.txt' % (OUT, script), 'w').write(out)
    for m in re.finditer(r'DUMP (\S+) scr=(\d+) seg=(-?\d+) clipframe=(-?\d+)', out):
        nm, scr, seg, cf = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4))
        r = np.fromfile('%s/%s.rgb' % (RAW, nm), np.uint8).reshape(320, 240, 3)
        prefix = {'scan': '', 'term': '', 'ticker': ''}
        png(r, '%s/%s.png' % (OUT, nm))
        if scr == 7: res[nm] = {'screen': 'CALbot (procedural full-screen, no clip, no ticker)'}; continue
        if seg < 0: res[nm] = {'screen': 'TTY (live full-screen terminal, no clip)'}; continue
        ref = np.asarray(np.load(CACHE + clipfile(seg), mmap_mode='r')[cf])
        d = np.sqrt(((r.astype(np.int32) - ref.astype(np.int32)) ** 2).sum(2))
        keep = np.ones((320, 240), bool)
        for (x0, y0, x1, y1) in MASK[scr]: keep[y0:y1 + 1, x0:x1 + 1] = False
        close = float((d[keep] <= 40).mean() * 100)
        diff = (ref // 3).astype(np.uint8); diff[(d > 40) & keep] = [255, 0, 0]; diff[~keep] = diff[~keep] // 2 + np.array([0, 0, 90], np.uint8)
        sbs = np.concatenate([ref, np.full((320, 4, 3), 255, np.uint8), r, np.full((320, 4, 3), 255, np.uint8), diff], 1)
        png(sbs, '%s/%s_ref_render_diff.png' % (OUT, nm))
        res[nm] = {'seg': seg, 'clip': clipfile(seg), 'clip_frame': cf, 'close_pct_outside_overlays': round(close, 3), 'overlay_px': int((~keep).sum())}
        print('%-24s seg %2d frame %3d  close %.3f%%  (overlay px %d)' % (nm, seg, cf, close, (~keep).sum()))
json.dump(res, open(OUT + '/scores.json', 'w'), indent=1)
# CALbot power-on flow video: every 30 ms sim frame, x3 nearest, H.264 yuv420p faststart (same recipe as ticker@3x.mp4)
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-framerate', '100/3',
                '-i', RAW + '/calbot_flow.rgb', '-vf', 'scale=720:960:flags=neighbor', '-c:v', 'libx264', '-crf', '18',
                '-pix_fmt', 'yuv420p', '-movflags', '+faststart', OUT + '/calbot_flow@3x.mp4'], check=True)
