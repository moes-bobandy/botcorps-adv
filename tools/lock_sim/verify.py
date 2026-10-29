"""Score every segment of the lock player against its LOCKED clip.
Runs the host sim (same C++ player as the firmware) per segment, compares each
rendered frame with the clip frame (3x3 block centre of the @3x mp4), metric =
share of pixels within RGB Euclidean distance 40 (LOCKED/harness/gp score.py).
Writes ref|render|diff PNGs (red = distance > 40) for first/mid/last frames."""
import sys, os, json, subprocess, numpy as np
from PIL import Image
sys.path.insert(0, '/workspace/cydport/tools'); import lkenc
SIM = sys.argv[1]; OUT = sys.argv[2]; os.makedirs(OUT, exist_ok=True)
meta = json.load(open('src/lock/lock_blob.bin.json'))
seg = 0; res = {}
for clip, m in meta.items():
    fr = lkenc.frames(m['src'])
    for (a, b, loop, tag) in m['segs']:
        n = b - a + 1 + (1 if loop else 0)
        raw = '/workspace/cydport/seg.rgb'
        subprocess.run([SIM, 'seg', str(seg), raw], check=True)
        r = np.fromfile(raw, np.uint8).reshape(n, 320, 240, 3)
        refidx = list(range(a, b + 1)) + ([a] if loop else [])
        close = []
        for i, f in enumerate(refidx):
            d = np.sqrt(((r[i].astype(np.int32) - np.asarray(fr[f]).astype(np.int32)) ** 2).sum(2))
            close.append(float((d <= 40).mean() * 100))
        pngs = []
        for i in sorted({0, (b - a) // 2, b - a}):
            ref = np.asarray(fr[refidx[i]]); d = np.sqrt(((r[i].astype(np.int32) - ref.astype(np.int32)) ** 2).sum(2))
            diff = (ref // 3).astype(np.uint8); diff[d > 40] = [255, 0, 0]
            sbs = np.concatenate([ref, np.full((320, 4, 3), 255, np.uint8), r[i], np.full((320, 4, 3), 255, np.uint8), diff], 1)
            p = '%s/%s_f%03d_ref_render_diff.png' % (OUT, tag, refidx[i])
            Image.fromarray(sbs).resize((sbs.shape[1] * 2, 640), Image.NEAREST).save(p); pngs.append(p)
        res[tag] = {'clip': clip, 'frames': '%d-%d' % (a, b), 'loop': loop, 'mean_close_pct': round(float(np.mean(close)), 3),
                    'min_close_pct': round(float(np.min(close)), 3), 'min_at': refidx[int(np.argmin(close))], 'pngs': pngs}
        print(tag, res[tag]['mean_close_pct'], res[tag]['min_close_pct'], flush=True)
        seg += 1
json.dump(res, open(OUT + '/scores.json', 'w'), indent=1)
