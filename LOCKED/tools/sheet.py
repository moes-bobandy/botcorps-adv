#!/usr/bin/env python3
"""Contact sheet: sheet.py OUT.png img1.png img2.png ...  (labels = file stems)"""
import sys, os
from PIL import Image, ImageDraw
out, files = sys.argv[1], sys.argv[2:]
cols = min(6, len(files)); rows = (len(files) + cols - 1) // cols
W, H, pad, lab = 240, 320, 8, 14
s = Image.new('RGB', (cols * (W + pad) + pad, rows * (H + pad + lab) + pad), (40, 40, 40))
d = ImageDraw.Draw(s)
for k, f in enumerate(files):
    im = Image.open(f).convert('RGB')
    if im.size != (W, H): im = im.resize((W, int(im.height * W / im.width)))
    x = pad + (k % cols) * (W + pad); y = pad + (k // cols) * (H + pad + lab)
    d.text((x, y), os.path.splitext(os.path.basename(f))[0][:38], fill=(230, 230, 230))
    s.paste(im, (x, y + lab))
s.save(out)
