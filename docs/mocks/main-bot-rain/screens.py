import sys; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, glossary as G
from PIL import Image
SPEC = ((78, 45, 164, 150), (91, 52, 148, 109), (83, 114, 158, 144))
JOBS = [('scan', 'scanbot', mbr.SCAN, {'green': '/tmp/mbr_fr/scanbot_bg_themes_v2_08.png', 'orange': '/tmp/mbr_fr/scanbot_bg_themes_v2_17.png'}),
        ('terminal', 'terminalbot', mbr.DEV, {'green': '/tmp/mbr_fr/terminalbot_bg_13.png', 'orange': '/tmp/mbr_fr/terminalbot_bg_24.png'})]
for name, scr, glyphs, fr in JOBS:
    if len(sys.argv) > 1 and name not in sys.argv[1:]: continue
    for th, p in fr.items():
        f = G.load1x(p); b, cells = mbr.render(f, SPEC, glyphs)
        Image.fromarray(mbr.sbs(f, b)).save('sbs_%s_%s_%s_3x.png' % (scr, name, th))
        Image.fromarray(mbr.zoom(b, (74, 30, 167, 152))).save('zoom_%s_%s_%s_8x.png' % (scr, name, th)); print(name, th, len(cells))
