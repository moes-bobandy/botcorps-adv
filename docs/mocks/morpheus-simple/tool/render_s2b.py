import sys, os; sys.path.insert(0, os.path.dirname(__file__)); from render import *
O2 = O + '/S2b'; os.makedirs(O2, exist_ok=True)
rows = []
for theme in ('purple', 'green'):
    base = L(R + '/boot_mock_frames/m070.png'); cells = [label(Image.open(R + '/frames/b070.png').convert('RGB').crop(BC), 'CURRENT BOOTbot')]
    for v, pn in (('S2', 'bronze'), ('S2b', 'black')):
        a = morph(base.copy(), 'boot', 2, theme, pn); n, bb = check(a, base, 'boot'); print(v, 'boot', theme, n, bb)
        if v == 'S2b': Image.fromarray(a).save('%s/S2b_boot_f070_%s_3x.png' % (O2, theme))
        cells.append(label(Image.fromarray(a).crop(BC), '%s  theme=%s' % (v, theme)))
    rows.append(row(cells))
col(rows).save(O2 + '/side_by_side_boot_chip_3x.png')
rows = []
for theme, fi in (('green', 200), ('orange', 380)):
    base = L(R + '/home_mock_frames/m%03d.png' % fi); z = lambda im: im.crop(HC).resize((280, 380), Image.NEAREST)
    cells = [label(z(Image.open(R + '/frames/g%03d.png' % fi).convert('RGB')), 'CURRENT BOOTbot f%d' % fi)]
    for v, pn in (('S2', 'bronze'), ('S2b', 'black')):
        a = morph(base.copy(), 'home', 2, None, pn); n, bb = check(a, base, 'home'); print(v, 'home', fi, n, bb)
        if v == 'S2b': Image.fromarray(a).save('%s/S2b_home_f%03d_%s_3x.png' % (O2, fi, theme))
        cells.append(label(z(Image.fromarray(a)), '%s  theme=%s' % (v, theme)))
    rows.append(row(cells))
col(rows).save(O2 + '/side_by_side_home_chip_6x.png')
