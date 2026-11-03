# Variant K stills: 4x zoom of the four tappable faces (J above, K below) and a J-vs-K side-by-side of the 3x board (frame 21).
import numpy as np
from PIL import Image, ImageDraw
J = np.load('/tmp/htp_J_steady21.npy'); K = np.load('/tmp/htp_K_steady.npy'); OUT = '/workspace/out/home-tap-pulse'
def z(a, b, k=4): x0, y0, x1, y1 = b; return Image.fromarray(np.repeat(np.repeat(a[y0:y1 + 1, x0:x1 + 1], k, 0), k, 1))
FACES = [('MORPHEUSbot', (38, 61, 68, 89)), ('CONTROLbot', (173, 60, 201, 88)), ('SCANbot', (44, 174, 90, 220)), ('TERMINALbot', (150, 174, 196, 220))]
gap, top = 24, 22
cells = [[(z(A, b), '%s %s' % (v, n)) for n, b in FACES] for v, A in (('J', J), ('K', K))]
Wd = sum(c.width for c, _ in cells[0]) + gap * 5; rh = max(c.height for c, _ in cells[0]) + top
im = Image.new('RGB', (Wd, 2 * rh + gap * 2), (40, 40, 44)); d = ImageDraw.Draw(im)
for r, row in enumerate(cells):
    x = gap; y = gap // 2 + r * (rh + gap)
    for c, t in row: im.paste(c, (x, y + top)); d.text((x, y + 5), t, fill=(230, 230, 230)); x += c.width + gap
im.save('%s/home_tap_faceglow_K_faces_4x.png' % OUT)
up = lambda a: np.repeat(np.repeat(a, 3, 0), 3, 1)
s = Image.new('RGB', (720 * 2 + 12, 960 + 30), (40, 40, 44)); s.paste(Image.fromarray(up(J)), (0, 30)); s.paste(Image.fromarray(up(K)), (732, 30))
d = ImageDraw.Draw(s); d.text((8, 9), 'J  (subtle face glow)', fill=(230, 230, 230)); d.text((740, 9), 'K  (heavy face glow)', fill=(230, 230, 230))
s.save('%s/home_tap_faceglow_J_vs_K_3x.png' % OUT)
