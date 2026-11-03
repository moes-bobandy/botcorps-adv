# Variant O stills: 4x zoom of GIBSONbot next to SCANbot and TERMINALbot (rows: N trough, O trough frame 0, O peak frame 21)
# and N-vs-O side-by-side (peak, 3x).
import numpy as np
from PIL import Image, ImageDraw
OUT = '/workspace/out/home-tap-pulse'
NT = np.load('/tmp/htp_N_trough.npy'); OT = np.load('/tmp/htp_O_trough.npy'); OP = np.load('/tmp/htp_O_peak.npy'); NP = np.load('/tmp/htp_N_peak.npy')
CH = [('GIBSONbot', (76, 40, 164, 155)), ('SCANbot', (29, 156, 106, 258)), ('TERMINALbot', (135, 156, 212, 258))]
z = lambda a, b: Image.fromarray(np.repeat(np.repeat(a[b[1]:b[3] + 1, b[0]:b[2] + 1], 4, 0), 4, 1))
gap, top = 20, 22; rows = [('N trough', NT), ('O trough', OT), ('O peak', OP)]
cw = [z(NT, b).width for _, b in CH]; rh = max(z(NT, b).height for _, b in CH) + top
im = Image.new('RGB', (sum(cw) + gap * 4, len(rows) * (rh + gap) + gap), (40, 40, 44)); d = ImageDraw.Draw(im)
for r, (lab, A) in enumerate(rows):
    x = gap; y = gap + r * (rh + gap)
    for (n, b), w in zip(CH, cw): im.paste(z(A, b), (x, y + top)); d.text((x, y + 5), '%s  %s' % (lab, n), fill=(230, 230, 230)); x += w + gap
im.save(OUT + '/home_tap_faceglow_O_gibson_4x.png')
up = lambda a: np.repeat(np.repeat(a, 3, 0), 3, 1)
s = Image.new('RGB', (720 * 2 + 12, 990), (40, 40, 44)); s.paste(Image.fromarray(up(NP)), (0, 30)); s.paste(Image.fromarray(up(OP)), (732, 30))
d = ImageDraw.Draw(s); d.text((8, 9), 'N  peak (GIBSON flat, no glow)', fill=(230, 230, 230)); d.text((740, 9), 'O  peak (GIBSON shaded + C glow)', fill=(230, 230, 230))
s.save(OUT + '/home_tap_faceglow_N_vs_O_3x.png')
