import sys; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, numpy as np, glossary as G
from PIL import Image
src = open('/workspace/out/main-bot-rain/oneline.py').read().split('JOBS =')[0]; exec(src)   # SPEC + oneline()
GIB_WHITE = [(mbr.GIB_ART, {'a': (255, 255, 255), 'e': (0, 0, 0)})]
WHITE_LEVELS = [1.00, 0.95, 0.92, 0.90, 0.88, 0.86]       # lead #FFFFFF, trails >= 86% white
def gib_white(f):
    old = mbr.LEVELS
    a, mask, cells, lb = mbr.prep(f, *SPEC); b = a.copy()
    y0 = np.nonzero(lb.any(1))[0].min(); lb[y0:146] = lb[y0]   # dim band runs from the name line to the chip bottom: no full-white 2px tails under the label
    mbr.LEVELS = WHITE_LEVELS
    try: mbr.paint(b, mask, cells, lb, GIB_WHITE, 9)
    finally: mbr.LEVELS = old
    return b, cells
grn = None
for th in G.THEMES:
    f = oneline(G.load1x(G.THEMES[th][0]), 'GIBSONbot', 93)
    old, c0 = mbr.render(f, SPEC, mbr.GIBSON); new, c1 = gib_white(f); assert [c for c in c0] == [c for c in c1]
    gap = np.zeros((960, 12, 3), np.uint8)
    Image.fromarray(np.hstack([G.up(old), gap, G.up(new)])).save('sbs_home_gibson_%s_3x_white.png' % th)
    Image.fromarray(mbr.zoom(new, (74, 30, 167, 152))).save('zoom_home_gibson_%s_8x_white.png' % th); print(th, len(c1))
    if th == 'green': grn = new
s = G.load1x('/tmp/mbr_fr/scanbot_bg_themes_v2_08.png'); bs, _ = mbr.render(oneline(s, 'SCANbot', 95), SPEC, mbr.SCAN)
t = G.load1x('/tmp/mbr_fr/terminalbot_bg_13.png'); bt, _ = mbr.render(t, SPEC, mbr.DEV)
crop = lambda a: G.up(a[40:156, 74:168].astype(np.uint8)); gap = np.zeros((116 * 3, 12, 3), np.uint8)
Image.fromarray(np.hstack([crop(grn), gap, crop(bs), gap, crop(bt)])).save('threeup_gibson_scan_terminal_green_3x_white.png')
