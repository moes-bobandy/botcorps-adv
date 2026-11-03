# 4x zooms for variant F (mock only): (1) GIBSON pins old (E) vs new (F) next to a SCANbot pin edge, (2) new MORPHEUS chip
# next to CONTROLbot, plus the E chip for reference. Reads the n=21 1x frames saved by tap_faceglow_F.py.
import numpy as np
from PIL import Image, ImageDraw
OUT = '/workspace/out/home-tap-pulse'
E = np.load('/tmp/htp_E_steady21.npy'); F = np.load('/tmp/htp_F_steady.npy')
def z(a, box, k=4): x0, y0, x1, y1 = box; return Image.fromarray(np.repeat(np.repeat(a[y0:y1 + 1, x0:x1 + 1], k, 0), k, 1))
def sheet(cells, gap=12, top=22):
    W = sum(c.width for c, _ in cells) + gap * (len(cells) + 1); H = max(c.height for c, _ in cells) + top + gap
    im = Image.new('RGB', (W, H), (40, 40, 44)); d = ImageDraw.Draw(im); x = gap
    for c, t in cells: im.paste(c, (x, top)); d.text((x, 5), t, fill=(230, 230, 230)); x += c.width + gap
    return im
GB = (70, 112, 121, 157); GR = (118, 112, 169, 157); SB = (26, 214, 77, 259)
a = sheet([(z(E, GB), 'E old GIBSON bottom-left'), (z(F, GB), 'F new GIBSON bottom-left'), (z(F, GR), 'F new GIBSON bottom-right'),
           (z(F, SB), 'SCANbot bottom-left (ref)')])
GS = (70, 52, 90, 112); SS = (26, 172, 46, 232); GRS = (150, 52, 170, 112)
b = sheet([(z(E, GS), 'E GIBSON L'), (z(F, GS), 'F GIBSON L'), (z(F, GRS), 'F GIBSON R'), (z(F, SS), 'SCANbot L (ref)')])
w = max(a.width, b.width); s = Image.new('RGB', (w, a.height + b.height), (40, 40, 44)); s.paste(a, (0, 0)); s.paste(b, (0, a.height))
s.save(OUT + '/home_tap_faceglow_F_gibson_pins_4x.png')
CH = (28, 48, 78, 114); CC = (162, 48, 212, 114)
sheet([(z(E, CH), 'E MORPHEUS (rain body, old)'), (z(F, CH), 'F MORPHEUS (base body + locked face)'), (z(F, CC), 'CONTROLbot (ref)')]).save(OUT + '/home_tap_faceglow_F_morpheus_vs_control_4x.png')
