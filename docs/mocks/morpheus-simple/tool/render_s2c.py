import sys, os; sys.path.insert(0, os.path.dirname(__file__)); from render import *
O3 = O + '/S2c'; os.makedirs(O3, exist_ok=True)
rows = []
for theme, fi in (('green', 200), ('orange', 380)):
    base = L(R + '/home_mock_frames/m%03d.png' % fi); z = lambda im: im.crop(HC).resize((280, 380), Image.NEAREST)
    cells = [label(z(Image.open(R + '/frames/g%03d.png' % fi).convert('RGB')), 'CURRENT BOOTbot f%d' % fi)]
    for v, pop in (('S2b', False), ('S2c', True)):
        a = morph(base.copy(), 'home', 2, None, 'black', pop); n, bb = check(a, base, 'home'); print(v, fi, n, bb)
        if v == 'S2c': Image.fromarray(a).save('%s/S2c_home_f%03d_%s_3x.png' % (O3, fi, theme))
        cells.append(label(z(Image.fromarray(a)), '%s  theme=%s' % (v, theme)))
    rows.append(row(cells))
col(rows).save(O3 + '/side_by_side_home_chip_6x.png')
