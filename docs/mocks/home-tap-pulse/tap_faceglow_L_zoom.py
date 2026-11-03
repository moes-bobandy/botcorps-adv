# Variant L stills: 4x zoom of the four tappable faces incl. their black face windows (K above, L below) and a K-vs-L side-by-side (3x, frame 21).
import numpy as np
from PIL import Image, ImageDraw
K = np.load('/tmp/htp_K_steady21.npy'); L = np.load('/tmp/htp_L_steady.npy'); OUT = '/workspace/out/home-tap-pulse'
def z(a, b, k=4): x0, y0, x1, y1 = b; return Image.fromarray(np.repeat(np.repeat(a[y0:y1 + 1, x0:x1 + 1], k, 0), k, 1))
FACES = [('MORPHEUSbot', (38, 61, 68, 89)), ('CONTROLbot', (173, 60, 201, 89)), ('SCANbot', (41, 172, 93, 224)), ('TERMINALbot', (147, 172, 199, 224))]
gap, top = 24, 22
cells = [[(z(A, b), '%s %s' % (v, n)) for n, b in FACES] for v, A in (('K', K), ('L', L))]
Wd = sum(c.width for c, _ in cells[0]) + gap * 5; rh = max(c.height for c, _ in cells[0]) + top
im = Image.new('RGB', (Wd, 2 * rh + gap * 2), (40, 40, 44)); d = ImageDraw.Draw(im)
for r, row in enumerate(cells):
    x = gap; y = gap // 2 + r * (rh + gap)
    for c, t in row: im.paste(c, (x, y + top)); d.text((x, y + 5), t, fill=(230, 230, 230)); x += c.width + gap
im.save('%s/home_tap_faceglow_L_faces_4x.png' % OUT)
up = lambda a: np.repeat(np.repeat(a, 3, 0), 3, 1)
s = Image.new('RGB', (720 * 2 + 12, 960 + 30), (40, 40, 44)); s.paste(Image.fromarray(up(K)), (0, 30)); s.paste(Image.fromarray(up(L)), (732, 30))
d = ImageDraw.Draw(s); d.text((8, 9), 'K  (rejected: whitened)', fill=(230, 230, 230)); d.text((740, 9), 'L  (own colour brightened + coloured halo)', fill=(230, 230, 230))
s.save('%s/home_tap_faceglow_K_vs_L_3x.png' % OUT)
