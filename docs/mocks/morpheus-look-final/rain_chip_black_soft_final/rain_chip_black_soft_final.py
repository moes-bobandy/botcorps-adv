"""TRY (mock only, not locked): big MORPHEUSbot grey body -> black, dense continuous device rain (firmware
botcorps_rain.cpp DEV_ART + DEV_NEON) clipped to the body. Mobandy 2026-11-02 3:43 PM ET: "Turn the grey to black and thicken the device rain"."""
import sys, os, random, subprocess
sys.path.insert(0, '/workspace/out/morpheus-look-final/rain_chip'); sys.path.insert(0, '/workspace/out/morpheus-look-final'); sys.path.insert(0, '/workspace/out/morpheus-glossary/tool')
import numpy as np; from PIL import Image
import rain_chip as RC, rain_glossary as R, variant_big_morpheus as V, glossary as G
from simple import BLACK_FRAME
O = os.path.dirname(os.path.abspath(__file__))
BODY_BLACK = (0, 0, 0)
LABEL_DIM_SOFT = 0.30   # rain behind MORPHEUS
SEP = (84, 88, 98)          # existing shaded-rim colour of his glasses; 1px ring so the face square stays its own shape
LEVELS = [0.80, 0.65, 0.60, 0.55, 0.50, 0.45]      # toned down (Mobandy 3:45 PM ET): lead 80%, trails 65..45%
def base(theme):
    a, mask = RC.base(theme)
    a[mask] = BODY_BLACK
    sub = a[48:146, 81:162].astype(int); grey = (np.ptp(sub, axis=2) < 30) & (sub.sum(2) > 120) & (sub.sum(2) < 600)
    sx0, sy0, sx1, sy1 = V.SIZES['glossary_big']['screen']; scr = np.zeros(grey.shape, bool); scr[sy0 - 48:sy1 - 47, sx0 - 81:sx1 - 80] = True
    a[48:146, 81:162][grey & ~scr] = BODY_BLACK          # body bevel/corner greys too
    sx0, sy0, sx1, sy1 = V.SIZES['glossary_big']['screen']
    a[sy0 - 1, sx0 - 1:sx1 + 2] = SEP; a[sy1 + 1, sx0 - 1:sx1 + 2] = SEP
    a[sy0 - 1:sy1 + 2, sx0 - 1] = SEP; a[sy0 - 1:sy1 + 2, sx1 + 1] = SEP
    mask[sy0 - 1:sy1 + 2, sx0 - 1:sx1 + 2] = False
    # 1px quiet grey outline on the outer edge of the black body (Mobandy 3:45 PM ET: "slight grey line boarder")
    from scipy.ndimage import binary_fill_holes, binary_erosion
    body = np.zeros(mask.shape, bool); body[48:146, 81:162] = (a[48:146, 81:162] == 0).all(2)
    body = binary_fill_holes(body); edge = body & ~binary_erosion(body)
    a[edge] = SEP; mask[edge] = False
    return a, mask
BOTTOM_X = [82 + round(j * 68 / 8) for j in range(9)]   # FINAL: 9 bottom columns re-spaced across the full interior x82..157 (fills the 4px gap)
BOTTOM_C = [0, 1, 2, 3, 4, 5, 6, 7, 11]                 # rightmost bottom column continues the right-strip stream
def paint(a, mask, t, seqs, phase):
    for c in range(12):
        for row in range(13):
            cy = 49 + row * 8                             # grid moved 1px in so it sits inside the grey outline
            if cy + 8 > 110:                               # bottom zone under the face
                if c not in BOTTOM_C: continue
                cx = BOTTOM_X[BOTTOM_C.index(c)]
            else:
                cx = 82 + c * 8 if c < 11 else 150
            if mask[cy:cy + 8, cx:cx + 8].mean() < 0.85: continue
            k = (t - row + phase[c]) % len(LEVELS); i = seqs[c][(t - row) % len(seqs[c])]
            for yy, line in enumerate(R.ART[i]):
                for xx, ch in enumerate(line):
                    x, y = cx + xx, cy + yy
                    if ch == '.' or not mask[y, x]: continue
                    f = LEVELS[k] * (LABEL_DIM_SOFT if RC.LABEL_ROWS[0] <= y <= RC.LABEL_ROWS[1] else 1)
                    a[y, x] = [int(v * f) for v in R.INK[i]['abc'.index(ch)]]
if __name__ == '__main__':
    r = random.Random(4); seqs = [[r.randrange(10) for _ in range(37)] for _ in range(12)]; phase = [r.randrange(6) for _ in range(12)]
    for theme in G.THEMES:
        a, mask = base(theme); b = a.copy(); paint(b, mask, 9, seqs, phase); RC.label(b)
        Image.fromarray(G.up(b)).save('%s/grid_big_morpheus_chiprain_black_soft_final_%s_3x.png' % (O, theme))
        x, y, w, h = 74, 30, 91, 123
        Image.fromarray(b[y - 2:y + h + 2, x - 2:x + w + 2]).resize(((w + 4) * 8, (h + 4) * 8), Image.NEAREST).save('%s/zoom_morpheus_chiprain_black_soft_final_%s_8x.png' % (O, theme))
    a, mask = base('green'); fd = '/tmp/mlf_crbsf'; os.makedirs(fd, exist_ok=True)
    for t in range(74):
        b = a.copy(); paint(b, mask, t, seqs, phase); RC.label(b); Image.fromarray(G.up(b)).save('%s/f%03d.png' % (fd, t))
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '8', '-i', fd + '/f%03d.png', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', O + '/chiprain_black_soft_final_big_morpheus_green.mp4'], check=True)
