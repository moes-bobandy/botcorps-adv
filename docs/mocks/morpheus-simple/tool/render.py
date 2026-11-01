import sys, os; sys.path.insert(0, os.path.dirname(__file__)); from simple import *
from PIL import ImageDraw
R = '/workspace/out/morpheus-rename'; O = '/workspace/out/morpheus-simple'
VAR = [('S1', 1), ('S2', 2), ('S3', 3)]
def label(img, text):
    W = Image.new('RGB', (img.width, img.height + 28), (24, 24, 24)); W.paste(img, (0, 28))
    ImageDraw.Draw(W).text((6, 8), text, fill=(225, 225, 225)); return W
def row(imgs, gap=16):
    W = Image.new('RGB', (sum(i.width for i in imgs) + gap * (len(imgs) - 1), max(i.height for i in imgs)), (24, 24, 24)); x = 0
    for i in imgs: W.paste(i, (x, 0)); x += i.width + gap
    return W
def col(imgs, gap=16):
    W = Image.new('RGB', (max(i.width for i in imgs), sum(i.height for i in imgs) + gap * (len(imgs) - 1)), (24, 24, 24)); y = 0
    for i in imgs: W.paste(i, (0, y)); y += i.height + gap
    return W
def check(a, base, gk):
    d = np.abs(a.astype(int) - base.astype(int)).sum(2) > 0; ys, xs = np.nonzero(d); x0, y0, x1, y1 = GEO[gk]['chip']
    assert xs.min() >= 3 * x0 and xs.max() < 3 * x1 + 3 and ys.min() >= 3 * y0 and ys.max() < 3 * y1 + 3
    return int(d.sum()), (xs.min() // 3, ys.min() // 3, xs.max() // 3, ys.max() // 3)
BC = (222, 168, 498, 556); HC = (90, 150, 230, 340)
def main():
  boot_rows = []
  for theme in ('purple', 'green'):
      base = L(R + '/boot_mock_frames/m070.png'); cells = [label(Image.open(R + '/frames/b070.png').convert('RGB').crop(BC), 'CURRENT BOOTbot')]
      for v, lvl in VAR:
          a = morph(base.copy(), 'boot', lvl, theme); n, bb = check(a, base, 'boot'); print(v, 'boot', theme, n, bb)
          Image.fromarray(a).save('%s/%s_boot_f070_%s_3x.png' % (O, v, theme)); cells.append(label(Image.fromarray(a).crop(BC), '%s  theme=%s' % (v, theme)))
      boot_rows.append(row(cells))
  col(boot_rows).save(O + '/side_by_side_boot_chip_3x.png')
  home_rows = []
  for theme, fi in (('green', 200), ('orange', 380)):
      base = L(R + '/home_mock_frames/m%03d.png' % fi); z = lambda im: im.crop(HC).resize((280, 380), Image.NEAREST)
      cells = [label(z(Image.open(R + '/frames/g%03d.png' % fi).convert('RGB')), 'CURRENT BOOTbot f%d' % fi)]
      for v, lvl in VAR:
          a = morph(base.copy(), 'home', lvl, None); n, bb = check(a, base, 'home'); print(v, 'home', fi, n, bb)
          Image.fromarray(a).save('%s/%s_home_f%03d_%s_3x.png' % (O, v, fi, theme)); cells.append(label(z(Image.fromarray(a)), '%s  theme=%s' % (v, theme)))
      home_rows.append(row(cells))
  col(home_rows).save(O + '/side_by_side_home_chip_6x.png')

if __name__ == "__main__": main()
