# All three chips on GIBSON's gaps model (10-row cycle, runs 3-5, gaps 5-7, 140 ms/row). GIBSON identical to gaps.py. Mock only.
import sys, os, random, subprocess; sys.path.insert(0, '/workspace/out/main-bot-rain')
exec(open('/workspace/out/main-bot-rain/gaps.py').read().split("if __name__ == '__main__':")[0])   # preps, paint_gib, P, tile, NSTEP, FPS, NF
SEEDS = (7, 47, 22)                              # GIBSON's seed from gaps.py; SCAN/TERM seeds picked so mean lit share = GIBSON's 13.5/40
CLR_LV = [1.00, 0.80, 0.65, 0.60, 0.55]          # SCAN/TERMINAL: lead 100%, trail = their current top steps (80/65/60/55%)
def paint_gap(a, mask, cells, lb, glyphs, t, seed=4):
    r = random.Random(seed); col = {}
    for cx, cy in sorted(cells):
        k = cx // 8
        if k not in col: col[k] = (r.randrange(P), 3 + r.randrange(3), [r.randrange(len(glyphs)) for _ in range(P)])
        ph, L, seq = col[k]; row = cy // 8; d = (t + ph - row) % P
        if d >= L: continue
        lv = CLR_LV[d]; art, ink = glyphs[seq[(t - row) % P]]          # icon rides down with its drop; period 10 -> seamless
        for yy, line in enumerate(art):
            for xx, ch in enumerate(line):
                x, y = cx + xx, cy + yy
                if ch == '.' or not mask[y, x]: continue
                f = lv * (mbr.LABEL_DIM if lb[y, x] else 1)
                a[y, x] = [int(v * f) for v in ink[ch]]
def preps3(th):
    Pp = preps(th)
    for (a, mask, cells, lb), _, _ in Pp[1:]:
        y0 = np.nonzero(lb.any(1))[0].min(); lb[y0:146] = lb[y0]        # same dim band as GIBSON: name line to chip bottom
    return Pp
def chips3(Pp, t):
    (g, _, _), (s, sg, _), (tm, tg, _) = Pp
    b0 = g[0].copy(); paint_gib(b0, g[1], g[2], g[3], t)
    b1 = s[0].copy(); paint_gap(b1, s[1], s[2], s[3], sg, t, SEEDS[1])
    b2 = tm[0].copy(); paint_gap(b2, tm[1], tm[2], tm[3], tg, t, SEEDS[2])
    return [b0, b1, b2]
def lit(Pp, t):                                   # icons painted per chip at step t
    out = []
    for i, ((a, mask, cells, lb), gl, _) in enumerate(Pp):
        r = random.Random(SEEDS[i]); col = {}; n = 0
        for cx, cy in sorted(cells):
            k = cx // 8
            if k not in col: col[k] = (r.randrange(P), 3 + r.randrange(3)) if i == 0 else (r.randrange(P), 3 + r.randrange(3), [r.randrange(len(gl)) for _ in range(P)])
            n += (t + col[k][0] - cy // 8) % P < col[k][1]
        out.append((n, len(cells)))
    return out
if __name__ == '__main__':
    for th in ('green', 'orange'):
        Pp = preps3(th); fd = '/tmp/mbr_gaps3_%s' % th; os.makedirs(fd, exist_ok=True)
        steps = [tile(chips3(Pp, T0 + i)) for i in range(NSTEP)]
        if th == 'green':
            Image.fromarray(steps[0]).save('threeup_gibson_scan_terminal_green_3x_gaps3.png')
            sd = '/tmp/mbr_gaps3_steps'; os.makedirs(sd, exist_ok=True)
            for i, s in enumerate(steps): Image.fromarray(s).save('%s/s%03d.png' % (sd, i))
        for n in range(NF): Image.fromarray(steps[n * 1000 // FPS // 140]).save('%s/f%03d.png' % (fd, n))
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', fd + '/f%03d.png', '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
                        '-c:v', 'libx264', '-profile:v', 'main', '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart',
                        'threeup_gibson_scan_terminal_%s_3x_gaps3.mp4' % th], check=True)
        if th == 'green':
            subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '50/7', '-i', '/tmp/mbr_gaps3_steps/s%03d.png', '-vf',
                            'split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=none', '-loop', '0',
                            'threeup_gibson_scan_terminal_green_3x_gaps3.gif'], check=True)
        ms = []; masks = [p[0][1] for p in Pp]
        for i in range(NSTEP - 1):
            c0, c1 = chips3(Pp, T0 + i), chips3(Pp, T0 + i + 1)
            ms.append([((np.abs(c0[j].astype(int) - c1[j].astype(int)).max(-1) > 40) & masks[j]).sum() / masks[j].sum() for j in range(3)])
        ms = np.array(ms); print(th, 'motion mean GIB/SCAN/TERM', ms.mean(0).round(3), 'min', ms.min(0).round(3), 'max', ms.max(0).round(3))
        L = np.array([[n for n, _ in lit(Pp, T0 + i)] for i in range(NSTEP)]); print(th, 'cells', [c for _, c in lit(Pp, T0)], 'lit mean', L.mean(0).round(1), 'min', L.min(0), 'max', L.max(0))
        c0, cN = chips3(Pp, T0), chips3(Pp, T0 + NSTEP); print(th, 'seamless', [(c0[j] == cN[j]).all() for j in range(3)])
        cg = chips3(Pp, T0 + 5)[0]; print(th, 'GIB identical to gaps.py', (cg == chips(preps(th), T0 + 5)[0]).all())
