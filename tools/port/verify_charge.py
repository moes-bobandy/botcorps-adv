"""Verify the firmware charge-lock port against the LOCKED ui_gibson_radar_charge D4 mock.
usage: verify_charge.py FW_SIM FW_BASE MOCK_SIM MOCK_BASE STILLS OUTDIR
 FW_SIM/FW_BASE: sim_live dumps of scripts/charge_D4.txt with this tree (LV_RD_CG_LOG, and LV_RD_CG_NODRAW=1 for base).
 MOCK_SIM/MOCK_BASE: same from the locked mock (3240b03 + apply_charge.py + apply_charge_D4.py, LV_RD_CHARGE=4).
 STILLS: LOCKED/ui_gibson_radar_charge/stills. Reports per moment:
  still_vs_mock  : locked @3x still (1::3 sampled) vs mock render (sanity, expect 0)
  fw_vs_still    : full-frame fw vs locked still (includes HEAD radar drift outside the charge layer)
  overlay_*      : charge layer as (sim != base) for fw and mock; xor = pixels drawn by one but not the other
  common_base_*  : on pixels where fw base == mock base, fw vs mock differences (the port's own error)
  outside_mask   : fw changed pixels outside the D4 allowed mask (route dilated 8 + boxes +2), target 0"""
import json, sys, re
import numpy as np
from PIL import Image
from scipy.ndimage import binary_dilation
FS, FB, MS, MB, ST, OUT = sys.argv[1:7]
ld = lambda d, k: np.frombuffer(open(f'{d}/{k}.rgb', 'rb').read(), np.uint8).reshape(320, 240, 3)
def rgb565(a): return ((a[..., 0].astype(np.uint16) >> 3) << 11) | ((a[..., 1].astype(np.uint16) >> 2) << 5) | (a[..., 2] >> 3)
tdump = {}
for line in open(f'{FS}/out.txt'):
    m = re.match(r'DUMP (\w+) .* t=(\d+)', line)
    if m: tdump[m.group(1)] = int(m.group(2))
err = open(f'{FS}/err.txt').read().splitlines(); res = {}
for k, nm in (('sel', 'idle'), ('mid', 'mid'), ('full', 'full'), ('lockfire', 'lockfire'), ('locked', 'locked'), ('hunting', 'hunting')):
    t = tdump[k]; fs, fb, ms, mb = ld(FS, k), ld(FB, k), ld(MS, k), ld(MB, k)
    st3 = np.array(Image.open(f'{ST}/D4_{nm}@3x.png').convert('RGB')); st = st3[1::3, 1::3]
    blocks_ok = all((st3[i::3, j::3] == st).all() for i in range(3) for j in range(3))
    path = np.zeros((320, 240), bool); box = np.zeros((320, 240), bool)
    for l in err:
        if not l.startswith(('CG t=%d ' % t, 'CGP t=%d ' % t)): continue
        f = l.split()
        if f[0] == 'CGP':
            for p in f[4:]:
                x, y = map(int, p.split(','))
                if 0 <= x < 240 and 0 <= y < 320: path[y, x] = True
        else:
            x0, y0, x1, y1 = map(int, f[3:7]); box[max(0, y0 - 2):y1 + 3, max(0, x0 - 2):x1 + 3] = True
    mask = (binary_dilation(path, np.ones((3, 3), bool), iterations=8) if path.any() else path) | box
    df = (fs != fb).any(2); dm = (ms != mb).any(2); cb = (fb == mb).all(2); dfm = (fs != ms).any(2)
    res[nm] = dict(t_ms=t, still_3x_blocks_uniform=bool(blocks_ok),
        still_vs_mock=int((st != ms).any(2).sum()),
        fw_vs_still=int((fs != st).any(2).sum()), fw_vs_still_rgb565=int((rgb565(fs) != rgb565(st)).sum()),
        fw_base_vs_mock_base=int((~cb).sum()),
        overlay_fw=int(df.sum()), overlay_mock=int(dm.sum()), overlay_xor=int((df ^ dm).sum()),
        overlay_xor_on_common_base=int(((df ^ dm) & cb).sum()),
        common_base_fw_vs_mock=int((dfm & cb).sum()),
        common_base_fw_vs_mock_rgb565=int(((rgb565(fs) != rgb565(ms)) & cb).sum()),
        common_base_max_abs=int(np.abs(fs.astype(int) - ms.astype(int))[cb].max(initial=0)),
        outside_mask=int((df & ~mask).sum()), mask_px=int(mask.sum()))
    # comparison: LOCKED still | firmware | diff (yellow = fw!=still, red = fw!=mock on common base)
    vis = (fs // 3).copy(); vis[(fs != st).any(2)] = [255, 255, 0]; vis[dfm & cb] = [255, 0, 0]
    sep = np.full((320, 4, 3), 255, np.uint8)
    Image.fromarray(np.hstack([st, sep, fs, sep, vis])).resize(((240 * 3 + 8) * 3, 960), Image.NEAREST).save(f'{OUT}/cmp_D4_{nm}@3x.png')
    Image.fromarray(fs).resize((720, 960), Image.NEAREST).save(f'{OUT}/fw_D4_{nm}@3x.png')
json.dump(res, open(f'{OUT}/verify_charge.json', 'w'), indent=1); print(json.dumps(res, indent=1))
