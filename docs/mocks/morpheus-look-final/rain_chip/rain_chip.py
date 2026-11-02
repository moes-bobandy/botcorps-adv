"""TRY (mock only, not locked): firmware device rain (botcorps_rain.cpp DEV_ART + DEV_NEON) inside the big
MORPHEUSbot chip's grey body only. Mobandy 2026-11-02 3:39 PM ET: "Yes the grey area only"."""
import sys, os, random, subprocess
sys.path.insert(0, '/workspace/out/morpheus-glossary/tool'); sys.path.insert(0, '/workspace/out/morpheus-look-final')
sys.path.insert(0, '/workspace/out/morpheus-simple/tool')
import numpy as np; from PIL import Image
import variant_big_morpheus as V, glossary as G, rain_glossary as R
from simple import GRAPH
SHADOW = (24, 24, 26); AX, AY = 81, 48              # 8px cell grid anchored on the body's top-left
LABEL_ROWS = (126, 138); LABEL_DIM = 0.45           # rain behind the MORPHEUS letters dimmed so the label stays on top
O = os.path.dirname(os.path.abspath(__file__))
def base(theme):
    a, rects, m, crown = V.render(theme); a = np.array(a, np.uint8)
    x, y, w, h = m; mask = np.zeros(a.shape[:2], bool)
    sub = a[y:y + h, x:x + w]; mk = np.zeros(sub.shape[:2], bool)
    for k in ('win', 'slant'): mk |= (sub == np.array(GRAPH[k])).all(2)
    mk |= (sub == np.array(G.LABEL_INK)).all(2)       # label ink is on the grey body; redrawn on top below
    mask[y:y + h, x:x + w] = mk
    return a, mask
def label(a):
    (lx0, ly0, lx1, ly1), text, ty = V.SIZES['glossary_big']['label']
    w = len(text) * 6 - 1; x0 = (lx0 + lx1 + 1) // 2 - (w + 1) // 2
    b = np.zeros_like(a); G.text(b, text, x0, ty, G.LABEL_INK); ink = b.any(2)
    sh = np.zeros_like(ink); sh[1:, 1:] = ink[:-1, :-1]; sh2 = np.zeros_like(ink); sh2[1:, :] |= ink[:-1, :]; sh2[:, 1:] |= ink[:, :-1]
    a[(sh | sh2) & ~ink] = SHADOW; a[ink] = G.LABEL_INK
def paint(a, mask, D):
    for c, h, L, gl, sp in D:
        for k in range(L):
            row = h - k
            if not 0 <= row < 13 or c > 10: continue
            cx, cy = AX + c * 8, AY + row * 8
            if mask[cy:cy + 8, cx:cx + 8].mean() < 0.85: continue      # no slivers at the face/pin edges
            i = gl[row % len(gl)]; s = R.FADE[min(k, 3)]
            for yy, line in enumerate(R.ART[i]):
                for xx, ch in enumerate(line):
                    x, y = cx + xx, cy + yy
                    if ch == '.' or not mask[y, x]: continue
                    f = s * (LABEL_DIM if LABEL_ROWS[0] <= y <= LABEL_ROWS[1] else 1)
                    a[y, x] = [int(v * f) for v in R.INK[i]['abc'.index(ch)]]
def drops(seed):
    r = random.Random(seed); return [[c, r.randint(-13, 13), r.randint(2, 4), [r.randrange(10) for _ in range(20)], 1] for c in range(11) for _ in range(2)]
def step(D):
    for d in D:
        d[1] += 1
        if d[1] - d[2] > 13: d[1] = -random.randint(0, 6)
if __name__ == '__main__':
    random.seed(5)
    for theme in G.THEMES:
        a, mask = base(theme); D = drops(11)
        for _ in range(9): step(D)
        b = a.copy(); paint(b, mask, D); label(b)
        Image.fromarray(G.up(b)).save('%s/grid_big_morpheus_chiprain_%s_3x.png' % (O, theme))
        x, y, w, h = V.SIZES['glossary_big']['chip'][0], 30, 91, 123
        Image.fromarray(b[y - 2:y + h + 2, x - 2:x + w + 2]).resize(((w + 4) * 8, (h + 4) * 8), Image.NEAREST).save('%s/zoom_morpheus_chiprain_%s_8x.png' % (O, theme))
    a, mask = base('green'); D = drops(11); fd = '/tmp/mlf_chiprain'; os.makedirs(fd, exist_ok=True)
    for t in range(60):
        b = a.copy(); paint(b, mask, D); label(b); Image.fromarray(G.up(b)).save('%s/f%03d.png' % (fd, t)); step(D)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '8', '-i', fd + '/f%03d.png', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', O + '/chiprain_big_morpheus_green.mp4'], check=True)
