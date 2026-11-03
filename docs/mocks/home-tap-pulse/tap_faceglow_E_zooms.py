import numpy as np
from PIL import Image, ImageDraw, ImageFont
font = ImageFont.load_default(size=18)
d = np.load('/tmp/htp_D_21.npy'); e = np.load('/tmp/htp_E_steady.npy')
def z(a, b, s): x0, y0, x1, y1 = b; return Image.fromarray(a[y0:y1 + 1, x0:x1 + 1].astype(np.uint8)).resize(((x1 - x0 + 1) * s, (y1 - y0 + 1) * s), Image.NEAREST)
mz = z(e, (29, 49, 77, 113), 4); mz.save('home_tap_faceglow_E_morpheus_chip_4x.png')
L = Image.open('/workspace/cyd-lock-store/LOCKED/ui_glossary_morpheus_rain/zoom_morpheus_chiprain_black_soft_final_green_8x.png')
L = L.resize((round(L.width * mz.height / L.height), mz.height), Image.NEAREST)
c = Image.new('RGB', (L.width + mz.width + 20, mz.height + 54)); c.paste(L, (0, 54)); c.paste(mz, (L.width + 20, 54))
dr = ImageDraw.Draw(c); dr.text((4, 4), 'LOCKED MORPHEUSbot', fill=(230,230,230), font=font); dr.text((4, 26), '(rain final, big chip)', fill=(230,230,230), font=font); dr.text((L.width + 24, 4), 'E home chip, 4x', fill=(230,230,230), font=font)
c.save('home_tap_faceglow_E_morpheus_vs_locked.png')
B = {'MORPHEUS': (39, 64, 67, 92), 'CONTROL': (174, 61, 200, 87), 'SCAN': (42, 172, 92, 222), 'TERMINAL': (148, 172, 198, 222)}
tiles = [[z(d, b, 4) for b in B.values()], [z(e, b, 4) for b in B.values()]]
th = max(t.height for t in tiles[0]); Wd = sum(t.width for t in tiles[0]) + 50
c = Image.new('RGB', (Wd, th * 2 + 90), (16, 16, 16)); dr = ImageDraw.Draw(c)
for r, (row, lab) in enumerate(zip(tiles, ('no glow (locked base + Morpheus swap)', 'E steady inner face glow'))):
    y = 30 + r * (th + 30); x = 10; dr.text((10, y - 24), lab, fill=(230, 230, 230), font=font)
    for t in row: c.paste(t, (x, y)); x += t.width + 10
c.save('home_tap_faceglow_E_faces_4x.png')
