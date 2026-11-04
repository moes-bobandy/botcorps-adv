"""D4 pixel diff: each still vs the same run built with LV_RD_CG_NODRAW=1 (identical logic/state = the locked radar's drawing, none of
the new D drawing). Allowed mask = button rect (+2) | logged route pixels dilated 8 px (glyph 4 + halo/crackle) | locked/selected
tower box + label (+2). Reports changed pixels outside the mask (target 0). usage: diff_D.py /tmp/rd OUTDIR"""
import json, sys, re
import numpy as np
from PIL import Image
from scipy.ndimage import binary_dilation
R, OUT = sys.argv[1], sys.argv[2]; res = {}
tdump = {}
for line in open(f'{R}/sim4/out.txt'):
    m = re.match(r'DUMP (\w+) .* t=(\d+)', line)
    if m: tdump[m.group(1)] = int(m.group(2))
err = open(f'{R}/sim4/err.txt').read().splitlines()
for k, nm in (('sel', 'idle'), ('mid', 'mid'), ('full', 'full'), ('lockfire', 'lockfire'), ('locked', 'locked'), ('hunting', 'hunting')):
    t = tdump[k]
    a = np.frombuffer(open(f'{R}/sim4/{k}.rgb', 'rb').read(), np.uint8).reshape(320, 240, 3)
    b = np.frombuffer(open(f'{R}/base4/{k}.rgb', 'rb').read(), np.uint8).reshape(320, 240, 3)
    ch = (a != b).any(2); path = np.zeros((320, 240), bool); box = np.zeros((320, 240), bool)
    for l in err:
        if not l.startswith(('CG t=%d ' % t, 'CGP t=%d ' % t)): continue
        f = l.split()
        if f[0] == 'CGP':
            for p in f[4:]:
                x, y = map(int, p.split(','))
                if 0 <= x < 240 and 0 <= y < 320: path[y, x] = True
        else:
            x0, y0, x1, y1 = map(int, f[3:7]); box[max(0, y0 - 2):y1 + 3, max(0, x0 - 2):x1 + 3] = True
    mask = binary_dilation(path, structure=np.ones((3, 3), bool), iterations=8) if path.any() else path
    mask = mask | box; out = ch & ~mask
    res[f'D4_{nm}'] = dict(t_ms=t, changed=int(ch.sum()), changed_outside_mask=int(out.sum()), mask_px=int(mask.sum()), path_px=int(path.sum()))
    vis = (a // 3).copy(); vis[ch] = [255, 255, 0]; vis[out] = [255, 0, 0]
    Image.fromarray(vis).resize((720, 960), Image.NEAREST).save(f'{OUT}/diff_D4_{nm}@3x.png')
json.dump(res, open(f'{OUT}/diff.json', 'w'), indent=1); print(json.dumps(res, indent=1))
