"""Score the scripted nav session (sim flow tools/lock_sim/flow.txt) against the LOCKED clips.
usage: flow_score.py <sim log> <dump dir> <out dir>"""
import json, re, sys, os, numpy as np
from PIL import Image
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lock_enc')); import lkenc
log, dd, out = sys.argv[1:4]
meta = json.load(open('src/lock/lock_blob.bin.json'))
segclip = [(c, m['src']) for c, m in meta.items() for s in m['segs']]
rows = []; tiles = []
for l in open(log):
    m = re.match(r'dump (\S+) scr=(\d+) seg=(\d+) frame=(-?\d+) clipframe=(\d+)', l)
    if not m: continue
    n, scr, s, k, f = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))
    r = np.fromfile('%s/%s.rgb' % (dd, n), np.uint8).reshape(320, 240, 3)
    ref = np.asarray(lkenc.frames(segclip[s][1])[f])
    d = np.sqrt(((r.astype(int) - ref.astype(int)) ** 2).sum(2)); pct = (d <= 40).mean() * 100
    rows.append('%s clip=%s seg=%d clipframe=%d close=%.3f%%' % (n, segclip[s][0], s, f, pct)); tiles.append(r)
    diff = (ref // 3).astype(np.uint8); diff[d > 40] = [255, 0, 0]
    w = np.full((320, 4, 3), 255, np.uint8)
    Image.fromarray(np.concatenate([ref, w, r, w, diff], 1)).save('%s/flow_%s_ref_render_diff.png' % (out, n))
open(out + '/flow_scores.txt', 'w').write('\n'.join(rows) + '\n'); print('\n'.join(rows))
C = 8; z = np.zeros_like(tiles[0])
sheet = [np.concatenate(tiles[i:i + C] + [z] * (C - len(tiles[i:i + C])), 1) for i in range(0, len(tiles), C)]
Image.fromarray(np.concatenate(sheet, 0)).save(out + '/flow_sheet.png')
