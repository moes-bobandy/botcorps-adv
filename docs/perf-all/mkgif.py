# docs/perf-all/mkgif.py REFDIR NEWDIR OUT.gif CLIP[,CLIP...] [LABEL_REF] [LABEL_NEW]
# Side-by-side device-timeline GIF from devrec NAME.cam.rgb (the panel every 20 ms of modelled device time). SIM@2x. HOST SIM ONLY.
import numpy as np, sys
from PIL import Image, ImageDraw
R, N, out, clips = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4].split(',')
la = sys.argv[5] if len(sys.argv) > 5 else 'BEFORE cda0390 (v1.2.17)'; lb = sys.argv[6] if len(sys.argv) > 6 else 'AFTER fix/dojo-glitch'
def cam(d, n): return np.fromfile(f'{d}/{n}.cam.rgb', np.uint8).reshape(-1, 320, 240, 3)
import os; fr = []; step = int(os.environ.get('STEP', '2'))
for c in clips:
    A, B = cam(R, c), cam(N, c); n = max(len(A), len(B))
    for i in range(0, n, step):
        a = A[min(i, len(A) - 1)]; b = B[min(i, len(B) - 1)]
        im = np.zeros((340, 484, 3), np.uint8); im[20:, 0:240] = a; im[20:, 244:] = b; im[:, 240:244] = 40
        if i >= len(A): im[20:, 0:240] //= 3
        if i >= len(B): im[20:, 244:] //= 3
        p = Image.fromarray(im); d = ImageDraw.Draw(p)
        d.text((3, 4), la, fill=(255, 90, 90)); d.text((247, 4), lb, fill=(90, 255, 140)); d.text((150, 4), f'{c} {i*20/1000:4.2f}s', fill=(220, 220, 220))
        fr.append(p.resize((968, 680), Image.NEAREST).convert('P', palette=Image.ADAPTIVE, colors=255))
fr[0].save(out, save_all=True, append_images=fr[1:], duration=20 * step, loop=0, optimize=False); print(out, len(fr), 'frames')
