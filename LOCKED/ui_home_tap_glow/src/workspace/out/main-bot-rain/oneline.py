import sys; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, numpy as np, glossary as G
from PIL import Image
SPEC = ((78, 45, 164, 150), (91, 52, 148, 109), (83, 114, 158, 144))
def oneline(f, name, x0):
    a = f.copy(); r = a[125:132, 81:159].reshape(-1, 3); ink = r[r.astype(int).sum(1).argmin()].copy()   # original dark name ink
    a[124:146, 81:159] = a[120, 100]             # erase both label lines with body grey (outline cols untouched)
    G.text(a, name, x0, 135, ink); return a      # TERMINALbot's line y135-141, same horizontal start as before
JOBS = [('home', 'gibson', 'GIBSONbot', 93, mbr.GIBSON, {t: G.THEMES[t][0] for t in G.THEMES}),
        ('scanbot', 'scan', 'SCANbot', 95, mbr.SCAN, {'green': '/tmp/mbr_fr/scanbot_bg_themes_v2_08.png', 'orange': '/tmp/mbr_fr/scanbot_bg_themes_v2_17.png'})]
three = []
for scr, nm, name, x0, gl, fr in JOBS:
    for th, p in fr.items():
        f = G.load1x(p); _, c0 = mbr.render(f, SPEC, gl); b, c1 = mbr.render(oneline(f, name, x0), SPEC, gl)
        Image.fromarray(mbr.sbs(f, b)).save('sbs_%s_%s_%s_3x_oneline.png' % (scr, nm, th))
        Image.fromarray(mbr.zoom(b, (78, 110, 164, 148))).save('zoom_%s_%s_%s_8x_oneline.png' % (scr, nm, th))
        print(nm, th, len(c0), '->', len(c1), 'gain', len(c1) - len(c0))
        if th == 'green': three.append(b)
t = G.load1x('/tmp/mbr_fr/terminalbot_bg_13.png'); bt, _ = mbr.render(t, SPEC, mbr.DEV); three.append(bt)
crop = lambda a: G.up(a[40:156, 74:168].astype(np.uint8)); gap = np.zeros((116 * 3, 12, 3), np.uint8)
Image.fromarray(np.hstack([crop(three[0]), gap, crop(three[1]), gap, crop(three[2])])).save('threeup_gibson_scan_terminal_green_3x_oneline.png')
