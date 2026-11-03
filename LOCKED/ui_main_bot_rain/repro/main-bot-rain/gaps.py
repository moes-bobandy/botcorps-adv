# GIBSON rain with dark gaps (firmware-style drops) + SCAN/TERMINAL exactly as moving.py. Mock only.
import sys, os, random, subprocess; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, numpy as np, glossary as G
from PIL import Image
src = open('/workspace/out/main-bot-rain/moving.py').read().split('for th in')[0]; exec(src)    # preps(), frame pieces, T0
GIB_LV = [1.00, 0.92, 0.90, 0.88, 0.86]          # lead #FFFFFF, trail 92..86% white (never grey)
P = 10                                           # drop period in rows: run 3-5 + gap 5-7 (avg 6, firmware 'gap 6')
def paint_gib(a, mask, cells, lb, t, seed=7):
    r = random.Random(seed); col = {}
    for cx, cy in sorted(cells):
        k = cx // 8
        if k not in col: col[k] = (r.randrange(P), 3 + r.randrange(3))
        ph, L = col[k]; d = (t + ph - cy // 8) % P
        if d >= L: continue                      # dark gap
        lv = GIB_LV[d]
        for yy, line in enumerate(mbr.GIB_ART):
            for xx, ch in enumerate(line):
                x, y = cx + xx, cy + yy
                if ch == '.' or not mask[y, x]: continue
                f = lv * (mbr.LABEL_DIM if lb[y, x] else 1)
                a[y, x] = (0, 0, 0) if ch == 'e' else tuple(int(255 * f) for _ in range(3))
def chips(Pp, t):
    (g, _, _), s, tm = Pp; out = []
    b = g[0].copy(); paint_gib(b, g[1], g[2], g[3], t); out.append(b)
    base = mbr.LEVELS
    for (a, mask, cells, lb), gl, lv in (s, tm):
        b = a.copy(); mbr.LEVELS = lv
        try: mbr.paint(b, mask, cells, lb, gl, t)
        finally: mbr.LEVELS = base
        out.append(b)
    return out
def tile(cs):
    gap = np.zeros((116 * 3, 12, 3), np.uint8); u = [G.up(c[40:156, 74:168].astype(np.uint8)) for c in cs]
    return np.hstack([u[0], gap, u[1], gap, u[2]])
NSTEP, FPS = 70, 30; NF = NSTEP * 140 * FPS // 1000      # 70 steps x 140 ms = 9.8 s = 294 frames
if __name__ == '__main__':
    for th in ('green', 'orange'):
        Pp = preps(th); fd = '/tmp/mbr_gaps_%s' % th; os.makedirs(fd, exist_ok=True)
        steps = [tile(chips(Pp, T0 + i)) for i in range(NSTEP)]
        if th == 'green':
            Image.fromarray(steps[0]).save('threeup_gibson_scan_terminal_green_3x_gaps.png')
            sd = '/tmp/mbr_gaps_steps'; os.makedirs(sd, exist_ok=True)
            for i, s in enumerate(steps): Image.fromarray(s).save('%s/s%03d.png' % (sd, i))
        for n in range(NF): Image.fromarray(steps[n * 1000 // FPS // 140]).save('%s/f%03d.png' % (fd, n))
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', fd + '/f%03d.png', '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
                        '-c:v', 'libx264', '-profile:v', 'main', '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart',
                        'threeup_gibson_scan_terminal_%s_3x_gaps.mp4' % th], check=True)
        if th == 'green':
            subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '50/7', '-i', '/tmp/mbr_gaps_steps/s%03d.png', '-vf',
                            'split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=none', '-loop', '0',
                            'threeup_gibson_scan_terminal_green_3x_gaps.gif'], check=True)
        # motion: fraction of each chip's rain-area pixels that change between consecutive steps
        ms = []
        for i in range(NSTEP - 1):
            c0, c1 = chips(Pp, T0 + i), chips(Pp, T0 + i + 1); masks = [Pp[0][0][1], Pp[1][0][1], Pp[2][0][1]]
            ms.append([((np.abs(c0[j].astype(int) - c1[j].astype(int)).max(-1) > 40) & masks[j]).sum() / masks[j].sum() for j in range(3)])   # visible change (>40/255)
        ms = np.array(ms); print(th, 'motion mean GIB/SCAN/TERM', ms.mean(0).round(3), 'min', ms.min(0).round(3))
        c0, cN = chips(Pp, T0), chips(Pp, T0 + NSTEP); print(th, 'GIBSON loop seamless:', (c0[0] == cN[0]).all())
