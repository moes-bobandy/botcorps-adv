# 4x zoom for variant H: G MORPHEUS, H MORPHEUS, CONTROLbot (all from frame 21).
import numpy as np
from PIL import Image, ImageDraw
G = np.load('/tmp/htp_G_steady21.npy'); Hh = np.load('/tmp/htp_H_steady.npy')
def z(a, b, k=4): x0, y0, x1, y1 = b; return Image.fromarray(np.repeat(np.repeat(a[y0:y1 + 1, x0:x1 + 1], k, 0), k, 1))
cells = [(z(G, (28, 48, 78, 114)), 'G MORPHEUS'), (z(Hh, (28, 48, 78, 114)), 'H MORPHEUS (CONTROL body)'), (z(Hh, (162, 48, 212, 114)), 'CONTROLbot (ref)')]
W = sum(c.width for c, _ in cells) + 48; Ht = max(c.height for c, _ in cells) + 34
im = Image.new('RGB', (W, Ht), (40, 40, 44)); d = ImageDraw.Draw(im); x = 12
for c, t in cells: im.paste(c, (x, 22)); d.text((x, 5), t, fill=(230, 230, 230)); x += c.width + 12
im.save('/workspace/out/home-tap-pulse/home_tap_faceglow_H_morpheus_G_vs_H_vs_control_4x.png')
