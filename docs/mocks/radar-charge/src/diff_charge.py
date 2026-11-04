"""Pixel diff of each charge still vs the same run with LV_RD_CG_NODRAW=1 (identical logic/state, none of the new drawing).
Allowed mask = button rect (+2) | current path pixels from the LV_RD_CG_LOG geometry (dilated) | selected tower box (+label, +2)
| for C the info panel's top frame row (the gauge). Reports changed pixels outside the mask (target 0)."""
import json, sys, re
import numpy as np
from PIL import Image
from scipy.ndimage import binary_dilation
R, OUT = sys.argv[1], sys.argv[2]
res = {}
for v, name in ((1, 'A'), (2, 'B'), (3, 'C')):
    tdump = {}
    for line in open(f'{R}/sim{v}/out.txt'):
        m = re.match(r'DUMP (\w+) .* t=(\d+)', line)
        if m: tdump[m.group(1)] = int(m.group(2))
    err = open(f'{R}/sim{v}/err.txt').read().splitlines()
    for k in ('mid', 'full', 'locked'):
        t = tdump[k]
        a = np.frombuffer(open(f'{R}/sim{v}/{k}.rgb', 'rb').read(), np.uint8).reshape(320, 240, 3)
        b = np.frombuffer(open(f'{R}/base{v}/{k}.rgb', 'rb').read(), np.uint8).reshape(320, 240, 3)
        ch = (a != b).any(2)
        path = np.zeros((320, 240), bool); box = np.zeros((320, 240), bool)
        for l in err:
            if not l.startswith(('CG t=%d ' % t, 'CGP t=%d ' % t)): continue
            f = l.split()
            if f[0] == 'CGP':
                for p in f[4:]:
                    x, y = map(int, p.split(','))
                    if 0 <= x < 240 and 0 <= y < 320: path[y, x] = True
            else:
                x0, y0, x1, y1 = map(int, f[3:7]); pad = 2
                box[max(0, y0 - pad):y1 + pad + 1, max(0, x0 - pad):x1 + pad + 1] = True
        r = 7 if v == 3 else 3
        mask = binary_dilation(path, structure=np.ones((3, 3), bool), iterations=r) | box
        if v == 3: mask[260:263, 2:238] = True
        out = ch & ~mask
        res[f'{name}_{k}'] = dict(t_ms=t, changed=int(ch.sum()), changed_outside_mask=int(out.sum()), mask_px=int(mask.sum()))
        vis = (a // 3).copy(); vis[ch] = [255, 255, 0]; vis[out] = [255, 0, 0]
        Image.fromarray(vis).resize((720, 960), Image.NEAREST).save(f'{OUT}/diff_{name}_{k}@3x.png')
json.dump(res, open(f'{OUT}/diff.json', 'w'), indent=1); print(json.dumps(res, indent=1))
