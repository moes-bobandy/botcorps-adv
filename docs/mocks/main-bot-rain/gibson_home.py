import sys; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, numpy as np, glossary as G
from PIL import Image
SPEC = ((78, 45, 164, 150), (91, 52, 148, 109), (83, 114, 158, 144))
for th in G.THEMES:
    f = G.load1x(G.THEMES[th][0]); b, cells = mbr.render(f, SPEC, mbr.GIBSON)
    Image.fromarray(mbr.sbs(f, b)).save('sbs_home_gibson_%s_3x.png' % th)
    Image.fromarray(mbr.zoom(b, (74, 30, 167, 152))).save('zoom_home_gibson_%s_8x.png' % th); print(th, len(cells))
