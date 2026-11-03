# 4x zoom for variant J: before (I) / after (J) pin edges of MORPHEUS and CONTROL, with TERMINALbot's pin edges for reference.
import numpy as np
from PIL import Image, ImageDraw
I = np.load('/tmp/htp_I_steady21.npy'); J = np.load('/tmp/htp_J_steady.npy')
def z(a, b, k=4): x0, y0, x1, y1 = b; return Image.fromarray(np.repeat(np.repeat(a[y0:y1 + 1, x0:x1 + 1], k, 0), k, 1))
def sheet(cells, gap=28, top=22):
    W = sum(c.width for c, _ in cells) + gap * (len(cells) + 1); Ht = max(c.height for c, _ in cells) + top + gap
    im = Image.new('RGB', (W, Ht), (40, 40, 44)); d = ImageDraw.Draw(im); x = gap
    for c, t in cells: im.paste(c, (x, top)); d.text((x, 5), t, fill=(230, 230, 230)); x += c.width + gap
    return im
ML, MR, CL, CR = (26, 58, 44, 114), (62, 58, 80, 114), (160, 58, 178, 114), (196, 58, 214, 114)
TL, TR = (132, 170, 150, 226), (196, 170, 214, 226)
a = sheet([(z(I, ML), 'I MORPH L'), (z(J, ML), 'J MORPH L'), (z(I, MR), 'I MORPH R'), (z(J, MR), 'J MORPH R'),
           (z(I, CL), 'I CTRL L'), (z(J, CL), 'J CTRL L'), (z(I, CR), 'I CTRL R'), (z(J, CR), 'J CTRL R'),
           (z(J, TL), 'TERMINAL L'), (z(J, TR), 'TERMINAL R')])
MB, CB, TB = (30, 104, 76, 114), (164, 104, 210, 114), (136, 246, 182, 256)
b = sheet([(z(I, MB), 'I MORPH bottom'), (z(J, MB), 'J MORPH bottom'), (z(I, CB), 'I CTRL bottom'), (z(J, CB), 'J CTRL bottom'), (z(J, TB), 'TERMINAL bottom')])
w = max(a.width, b.width); s = Image.new('RGB', (w, a.height + b.height), (40, 40, 44)); s.paste(a, (0, 0)); s.paste(b, (0, a.height))
s.save('/workspace/out/home-tap-pulse/home_tap_faceglow_J_pins_before_after_4x.png')
