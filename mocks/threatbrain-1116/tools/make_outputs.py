"""THREAT BRAIN mock outputs + diff gate. Inputs: raw2/ from sim_tb (base = real firmware lv_core.h frame,
tb = same frame + net layer, mask = px the layer was allowed to touch). Writes stills @3x, mp4 @3x, sheets, diff proof."""
import sys, os, json, hashlib, subprocess, numpy as np
from PIL import Image, ImageDraw, ImageFont
RAW, LOCKREF, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(OUT, exist_ok=True)
FS = 230400
TH = {0: 'NEON GRAPE', 6: 'GOLD RUSH', 23: 'SONAR'}
VAR = {'A': 'THREAT BRAIN', 'B': 'NEURAL TRACKS'}
# independent re-statement of the protected rects (bots, hub, rails/ticker/terminal = outside the area)
NODE = [(32, 29, 72, 82), (167, 29, 207, 82), (32, 86, 72, 139), (167, 86, 207, 139), (32, 143, 72, 196), (167, 143, 207, 196),
        (32, 200, 72, 253), (78, 200, 118, 253), (122, 200, 162, 253), (167, 200, 207, 253)]
HUB = (76, 28, 164, 152); AREA = (28, 26, 211, 257)
prot = np.ones((320, 240), bool); x0, y0, x1, y1 = AREA; prot[y0:y1 + 1, x0:x1 + 1] = False
for (x0, y0, x1, y1) in NODE + [HUB]: prot[y0:y1 + 1, x0:x1 + 1] = True
def frames(f): return np.fromfile(f, np.uint8).reshape(-1, 320, 240, 3)
def x3(a): return np.repeat(np.repeat(a, 3, 0), 3, 1)
def sha(p): return hashlib.sha256(open(p, 'rb').read()).hexdigest()
def save(a, p): Image.fromarray(a).save(p); return p
try: FONT = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', 22)
except Exception: FONT = ImageFont.load_default()
rep = {'inputs': {'renderer': 'tools/livescan/sim_live.cpp @9bc3569 (src c1b5659) + lv_core.h + locked clip blob; mock harness sim_tb.cpp'}, 'variants': {}}
files = []
for V in 'AB':
    vr = rep['variants'][V] = {'name': VAR[V], 'themes': {}}
    for T in (0, 6, 23):
        base, tb = frames(f'{RAW}/{V}_clip_{T}_base.rgb'), frames(f'{RAW}/{V}_clip_{T}_tb.rgb')
        mask = np.fromfile(f'{RAW}/{V}_clip_{T}_mask.bin', np.uint8).reshape(-1, 320, 240).astype(bool)
        ib, it = frames(f'{RAW}/{V}_idle_{T}_base.rgb')[0], frames(f'{RAW}/{V}_idle_{T}_tb.rgb')[0]
        im = np.fromfile(f'{RAW}/{V}_idle_{T}_mask.bin', np.uint8).reshape(320, 240).astype(bool)
        # 1) the harness base == the plain locked render (same sim, no layer code path) frame for frame
        lock = frames(f'{LOCKREF}/termrec_{T}.rgb')
        base_vs_lock = int((base != lock).any(-1).sum())
        lock_idle = frames(f'{LOCKREF}/term_lock_{T}.rgb')[0]
        idle_base_vs_lock = int((ib != lock_idle).any(-1).sum())
        # 2) changed px must be inside the allowed mask, never on a protected rect, never on a lit (non-background) base px
        allb, allt, allm = np.concatenate([base, ib[None]]), np.concatenate([tb, it[None]]), np.concatenate([mask, im[None]])
        ch = (allb != allt).any(-1)
        lit = (allb.astype(int).max(-1) > 40) | (allb == 24).all(-1)
        outside_mask = int((ch & ~allm).sum()); on_prot = int((ch & prot[None]).sum()); on_lit = int((ch & lit).sum())
        per = ch.reshape(ch.shape[0], -1).sum(1)
        k = int(np.argmax(per[60:240]) + 60)   # busy still = the frame with the most net light between 1.8 s and 7.2 s
        tr = {'frames': int(base.shape[0]), 'base_vs_locked_render_px': base_vs_lock, 'idle_base_vs_locked_render_px': idle_base_vs_lock,
              'changed_px_total_all_frames': int(ch.sum()), 'changed_px_idle': int(per[-1]), 'busy_frame': k, 'busy_t_s': round((k + 1) * 0.03, 2),
              'changed_px_busy': int(per[k]), 'changed_px_outside_net_layer': outside_mask, 'changed_px_on_bot_hub_rail_text_rects': on_prot,
              'changed_px_on_lit_locked_px': on_lit, 'PASS': outside_mask == 0 and on_prot == 0 and on_lit == 0 and base_vs_lock == 0 and idle_base_vs_lock == 0}
        vr['themes'][TH[T]] = tr
        tag = f'{V}_{VAR[V].lower().replace(" ", "")}_{TH[T].lower().replace(" ", "")}'
        p1 = save(x3(it), f'{OUT}/{tag}_idle@3x.png'); p2 = save(x3(tb[k]), f'{OUT}/{tag}_busy@3x.png')
        save(it, f'{OUT}/{tag}_idle@1x.png'); save(tb[k], f'{OUT}/{tag}_busy@1x.png')
        assert (np.asarray(Image.open(p2)) == x3(np.asarray(Image.open(f'{OUT}/{tag}_busy@1x.png')))).all()
        # diff proof image: locked | mock | changed px (cyan = net layer px, red = any change outside it)
        for nm, b, t, m in (('idle', ib, it, im), ('busy', base[k], tb[k], mask[k])):
            c = (b != t).any(-1); d = (b // 4).astype(np.uint8); d[c & m] = [0, 255, 255]; d[c & ~m] = [255, 0, 0]
            sbs = np.concatenate([b, np.full((320, 4, 3), 255, np.uint8), t, np.full((320, 4, 3), 255, np.uint8), d], 1)
            save(x3(sbs), f'{OUT}/proof_{tag}_{nm}_locked_mock_diff@3x.png')
        if T == 0:
            mp4 = f'{OUT}/{V}_{VAR[V].lower().replace(" ", "")}_neongrape_9s@3x.mp4'
            subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '100/3', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-i', f'{RAW}/{V}_clip_{T}_tb.rgb',
                            '-vf', 'scale=720:960:flags=neighbor', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '17', '-movflags', '+faststart', mp4], check=True)
            pr = json.loads(subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v:0', '-count_frames', '-show_entries',
                            'stream=codec_name,pix_fmt,width,height,nb_read_frames,duration', '-of', 'json', mp4], capture_output=True, text=True).stdout)['streams'][0]
            mot = float(np.abs(tb[1:].astype(int) - tb[:-1].astype(int)).sum(-1).astype(bool).sum((1, 2)).mean())
            vr['mp4'] = {'path': mp4, 'sha256': sha(mp4), 'probe': pr, 'mean_changed_px_per_frame': round(mot, 1)}
            vr['idle_still'] = {'path': p1, 'sha256': sha(p1)}; vr['busy_still'] = {'path': p2, 'sha256': sha(p2)}
# sheets
def label(img, txt, h=40):
    W = img.shape[1]; c = Image.new('RGB', (W, h), (12, 12, 16)); ImageDraw.Draw(c).text((8, 8), txt, fill=(235, 235, 235), font=FONT)
    return np.concatenate([np.asarray(c), img], 0)
def row(ims, gap=12): return np.concatenate(sum([[i, np.full((i.shape[0], gap, 3), 40, np.uint8)] for i in ims], [])[:-1], 1)
def col(ims, gap=12): return np.concatenate(sum([[i, np.full((gap, i.shape[1], 3), 40, np.uint8)] for i in ims], [])[:-1], 0)
for V in 'AB':
    cells = []
    for T in (0, 6, 23):
        tag = f'{V}_{VAR[V].lower().replace(" ", "")}_{TH[T].lower().replace(" ", "")}'
        cells.append(col([label(np.asarray(Image.open(f'{OUT}/{tag}_idle@3x.png')), f'{TH[T]} - idle'), label(np.asarray(Image.open(f'{OUT}/{tag}_busy@3x.png')), f'{TH[T]} - busy (EX.DATA)')]))
    sh = label(row(cells), f'{V}  {VAR[V]}  - 3 CONTROLbot themes (mock, real TERMINALbot render, example data)', 50)
    p = save(sh, f'{OUT}/{V}_{VAR[V].lower().replace(" ", "")}_3theme_sheet.png'); rep['variants'][V]['sheet'] = {'path': p, 'sha256': sha(p)}
ab = []
for V in 'AB':
    tag = f'{V}_{VAR[V].lower().replace(" ", "")}_neongrape'
    ab.append(col([label(np.asarray(Image.open(f'{OUT}/{tag}_idle@3x.png')), f'{V} {VAR[V]} - idle'), label(np.asarray(Image.open(f'{OUT}/{tag}_busy@3x.png')), f'{V} {VAR[V]} - busy (EX.DATA)')]))
p = save(label(row(ab), 'A THREAT BRAIN  |  B NEURAL TRACKS  - NEON GRAPE (mock)', 50), f'{OUT}/AB_contact_sheet.png'); rep['ab_sheet'] = {'path': p, 'sha256': sha(p)}
rep['PASS'] = all(t['PASS'] for v in rep['variants'].values() for t in v['themes'].values())
json.dump(rep, open(f'{OUT}/diff_proof.json', 'w'), indent=1)
print(json.dumps(rep, indent=1))
