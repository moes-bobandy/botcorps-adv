# Variant N stills: 4x zoom of the four tappable chips (rows: J, N trough frame 0, N peak frame 21) and C-vs-N side-by-side (peak, 3x).
import numpy as np
from PIL import Image, ImageDraw
OUT = '/workspace/out/home-tap-pulse'
J = np.load('/tmp/htp_J0.npy'); T = np.load('/tmp/htp_N_trough.npy'); P = np.load('/tmp/htp_N_peak.npy'); C = np.load('/tmp/htp_C_peak.npy')
CH = [('MORPHEUSbot', (29, 49, 77, 113)), ('CONTROLbot', (163, 49, 211, 113)), ('SCANbot', (29, 156, 106, 258)), ('TERMINALbot', (135, 156, 212, 258))]
z = lambda a, b: Image.fromarray(np.repeat(np.repeat(a[b[1]:b[3] + 1, b[0]:b[2] + 1], 4, 0), 4, 1))
gap, top = 20, 22; rows = [('J', J), ('N trough', T), ('N peak', P)]
cw = [z(J, b).width for _, b in CH]; rh = max(z(J, b).height for _, b in CH) + top
im = Image.new('RGB', (sum(cw) + gap * 5, len(rows) * (rh + gap) + gap), (40, 40, 44)); d = ImageDraw.Draw(im)
for r, (lab, A) in enumerate(rows):
    x = gap; y = gap + r * (rh + gap)
    for (n, b), w in zip(CH, cw): im.paste(z(A, b), (x, y + top)); d.text((x, y + 5), '%s  %s' % (lab, n), fill=(230, 230, 230)); x += w + gap
im.save(OUT + '/home_tap_faceglow_N_chips_4x.png')
up = lambda a: np.repeat(np.repeat(a, 3, 0), 3, 1)
s = Image.new('RGB', (720 * 2 + 12, 990), (40, 40, 44)); s.paste(Image.fromarray(up(C)), (0, 30)); s.paste(Image.fromarray(up(P)), (732, 30))
d = ImageDraw.Draw(s); d.text((8, 9), 'C  (old g200 board, BOOTbot) peak', fill=(230, 230, 230)); d.text((740, 9), 'N  (C glow on J board, right MORPHEUS) peak', fill=(230, 230, 230))
s.save(OUT + '/home_tap_faceglow_C_vs_N_3x.png')
