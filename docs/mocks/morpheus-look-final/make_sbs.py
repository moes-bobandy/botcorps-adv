"""Side-by-sides: current frame (left) vs MORPHEUSbot chip in the locked big-Morpheus look (right). Mock only."""
import sys; sys.path.insert(0, '/workspace/out/morpheus-glossary/tool')
import numpy as np; from PIL import Image
import variant_big_morpheus as V, glossary as G
F = '/workspace/out/morpheus-rename/frames/'
JOBS = [('home', 'g200', 'home_green'), ('home', 'g380', 'home_orange'), ('boot', 'b070', 'boot_f070')]
for size, fr, out in JOBS:
    f = G.load1x(F + fr + '.png'); a = f.copy()
    V.morpheus_chip(a, f, **V.SIZES[size])
    gap = np.zeros((960, 12, 3), np.uint8)
    Image.fromarray(np.hstack([G.up(f), gap, G.up(a)])).save('sbs_%s_3x.png' % out)
    x0, y0, x1, y1 = V.SIZES[size]['chip']
    Image.fromarray(a.astype(np.uint8)[y0-2:y1+3, x0-2:x1+3]).resize(((x1-x0+5)*8, (y1-y0+5)*8), Image.NEAREST).save('zoom_%s_8x.png' % out)
