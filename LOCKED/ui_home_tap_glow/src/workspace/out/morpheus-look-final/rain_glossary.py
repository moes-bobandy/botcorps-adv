"""Big-MORPHEUSbot glossary screen + the firmware device-icon rain (src/rain/botcorps_rain.cpp DEV_ART / DEV_NEON,
device-only mode like the Terminal rain, 8px cells, 30x40 portrait grid) BEHIND the chips, high-visibility shade:
full DEV_NEON ink on the lead, trail kept bright (100/90/80/70 %). Mock only."""
import sys, os, random, subprocess; sys.path.insert(0, '/workspace/out/morpheus-glossary/tool')
import numpy as np; from PIL import Image
import variant_big_morpheus as V, glossary as G
ART = [l.split('"')[1::2] for l in open('/workspace/cyd-portrait-wt/src/rain/botcorps_rain.cpp').read().split('DEV_ART[DEV_N][8][9] = {')[1].split('};')[0].strip().splitlines()]
NEON = [int(v, 16) for v in open('/workspace/cyd-portrait-wt/src/rain/botcorps_rain.cpp').read().split('DEV_NEON[DEV_N][3] = {')[1].split('};')[0].replace('{', '').replace('}', '').replace('\n', '').split(',') if v.strip()]
assert len(ART) == 10 and all(len(a) == 8 for a in ART) and len(NEON) == 30
def c565(v): return ((v >> 11 & 31) * 255 // 31, (v >> 5 & 63) * 255 // 63, (v & 31) * 255 // 31)
INK = [[c565(NEON[i * 3 + k]) for k in range(3)] for i in range(10)]
FADE = [1.0, 0.9, 0.8, 0.7]
COLS, ROWS = 30, 40
def drops(seed=7):
    r = random.Random(seed); D = []
    for c in range(COLS):
        for d in range(2):
            L = r.randint(3, 6); D.append([c, r.randint(-ROWS, ROWS), L, [r.randrange(10) for _ in range(ROWS + 12)], r.randint(1, 2)])
    return D
def paint(a, mask, D):
    for c, h, L, gl, sp in D:
        for k in range(L):
            row = h - k
            if not 0 <= row < ROWS: continue
            if not mask[row * 8:row * 8 + 8, c * 8:c * 8 + 8].all(): continue   # whole icons only, no slivers in chip gaps
            i = gl[(row) % len(gl)]; s = FADE[min(k, 3)]
            for yy, line in enumerate(ART[i]):
                for xx, ch in enumerate(line):
                    if ch == '.': continue
                    x, y = c * 8 + xx, row * 8 + yy
                    if mask[y, x]: a[y, x] = [int(v * s) for v in INK[i]['abc'.index(ch)]]
def step(D):
    for d in D:
        d[1] += 1 if d[4] == 1 else (1 if random.random() < .5 else 0)
        if d[1] - d[2] > ROWS: d[1] = -random.randint(0, 12)
def base(theme):
    a, rects, m, crown = V.render(theme); a = np.array(a, np.uint8)
    keep = np.zeros(a.shape[:2], bool)
    for (x, y, w, h) in list(rects.values()) + [m]: keep[y - 1:y + h + 1, x - 1:x + w + 1] = True
    mask = (a.astype(int).sum(2) < 60) & ~keep
    mask[:22] = False; mask[313:] = False; mask[:, :30] = False; mask[:, 210:] = False
    return a, mask
if __name__ == '__main__':
    O = 'rain'; os.makedirs(O, exist_ok=True)
    for theme in G.THEMES:
        a, mask = base(theme); D = drops()
        for _ in range(25): step(D)
        b = a.copy(); paint(b, mask, D); Image.fromarray(G.up(b)).save('%s/grid_big_morpheus_rain_%s_3x.png' % (O, theme))
    a, mask = base('green'); D = drops(); random.seed(3); fd = '/tmp/mlf_rain'; os.makedirs(fd, exist_ok=True)
    for t in range(80):
        b = a.copy(); paint(b, mask, D); Image.fromarray(G.up(b)).save('%s/f%03d.png' % (fd, t)); step(D)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '10', '-i', fd + '/f%03d.png', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', O + '/rain_big_morpheus_green.mp4'], check=True)
