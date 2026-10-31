import sys, os; sys.path.insert(0, os.path.dirname(__file__)); from look import *
from PIL import Image, ImageDraw
R = '/workspace/out/morpheus-rename'; O = '/workspace/out/morpheus-look'
SRC = {'boot_f070': (R + '/boot_mock_frames/m070.png', 'boot'), 'home_f200': (R + '/home_mock_frames/m200.png', 'home'),
       'home_intro_f010': (R + '/home_mock_frames/m010.png', None),
       'ideaC_pulse_f0100': ('/workspace/out/morpheus-pills/ideaC/frames/f0100.png', 'boot')}
CROP = {'boot': (222, 170, 500, 550), 'home': (90, 150, 230, 340)}
def label(img, text):
    W = Image.new('RGB', (img.width, img.height + 30), (24, 24, 24)); W.paste(img, (0, 30))
    ImageDraw.Draw(W).text((8, 9), text, fill=(225, 225, 225)); return W
def row(imgs, gap=20):
    W = Image.new('RGB', (sum(i.width for i in imgs) + gap * (len(imgs) - 1), max(i.height for i in imgs)), (24, 24, 24)); x = 0
    for i in imgs: W.paste(i, (x, 0)); x += i.width + gap
    return W
res = {}
for v, lvl in (('M1', 1), ('M2', 2), ('M3', 3)):
    for key, (p, gk) in SRC.items():
        if gk is None: continue
        base = L(p); a = morph(base.copy(), GEO[gk], lvl)
        d = np.abs(a.astype(int) - base.astype(int)).sum(2) > 0; ys, xs = np.nonzero(d)
        x0, y0, x1, y1 = GEO[gk]['chip']
        assert xs.min() >= 3 * x0 and xs.max() < 3 * x1 + 3 and ys.min() >= 3 * y0 and ys.max() < 3 * y1 + 3, (v, key)
        im = Image.fromarray(a); res[(v, key)] = im
        im.save('%s/%s_%s_3x.png' % (O, v, key))
        if key in ('boot_f070', 'home_f200'):
            c = CROP[gk]; im.crop(c).resize(((c[2] - c[0]) * (2 if gk == 'home' else 1), (c[3] - c[1]) * (2 if gk == 'home' else 1)), Image.NEAREST).save('%s/%s_%s_chip_zoom.png' % (O, v, key))
        print(v, key, 'changed px', int(d.sum()), 'bbox 1x', xs.min() // 3, ys.min() // 3, xs.max() // 3, ys.max() // 3)
cur_b = Image.open(R + '/frames/b070.png').convert('RGB'); cur_h = Image.open(R + '/frames/g200.png').convert('RGB')
row([label(cur_b, 'CURRENT BOOTbot (locked clip f70)')] + [label(res[(v, 'boot_f070')], v + ' MORPHEUSbot') for v in ('M1', 'M2', 'M3')]).save(O + '/side_by_side_boot_f070_3x.png')
c = CROP['boot']
row([label(cur_b.crop(c), 'CURRENT BOOTbot')] + [label(res[(v, 'boot_f070')].crop(c), v) for v in ('M1', 'M2', 'M3')]).save(O + '/side_by_side_boot_chip_3x.png')
c = CROP['home']; z = lambda i: i.crop(c).resize((280, 380), Image.NEAREST)
row([label(z(cur_h), 'CURRENT home chip (f200)')] + [label(z(res[(v, 'home_f200')]), v) for v in ('M1', 'M2', 'M3')]).save(O + '/side_by_side_home_chip_6x.png')
row([label(cur_h, 'CURRENT home (gibson f200)')] + [label(res[(v, 'home_f200')], v) for v in ('M1', 'M2', 'M3')]).save(O + '/side_by_side_home_f200_3x.png')
row([label(Image.open('/workspace/out/morpheus-pills/ideaC/frames/f0100.png').convert('RGB'), 'Idea C f100 (rename look)')] + [label(res[(v, 'ideaC_pulse_f0100')], 'Idea C + ' + v) for v in ('M1', 'M2', 'M3')]).save(O + '/side_by_side_ideaC_pulse_3x.png')
