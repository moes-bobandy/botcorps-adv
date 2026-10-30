"""Review sheet: old 8x8 rain icons vs new 16x16 emoji icons, 1x and 4x, plus one rain frame."""
import sys, random
from PIL import Image, ImageDraw
sys.path.insert(0, 'tools/livescan')
import icons16 as I
def rgb(v): return I.rgb(v)
def new_img(name, s):
    p = I.pix(name); im = Image.new('RGB', (16 * s, 16 * s), (24, 24, 28)); d = ImageDraw.Draw(im)
    for y in range(16):
        for x in range(16):
            if p[y][x] is not None: d.rectangle([x*s, y*s, x*s+s-1, y*s+s-1], fill=rgb(p[y][x]))
    return im
def old_img(art, inks, s):
    im = Image.new('RGB', (8 * s, 8 * s), (24, 24, 28)); d = ImageDraw.Draw(im)
    for y in range(8):
        for x in range(8):
            ch = art[y][x]
            if ch != '.': d.rectangle([x*s, y*s, x*s+s-1, y*s+s-1], fill=rgb(inks['abc'.index(ch)]))
    return im
def rain_frame(seed=7):
    """240x320 device rain (16px cells) drawn with the new icons; index = hash % LV_IC_N (no % 10 wrap)."""
    W, H, C = 240, 320, 16
    im = Image.new('RGB', (W, H), (0, 0, 0)); rnd = random.Random(seed)
    n = len(I.ICONS); names = [t[0] for t in I.ICONS]
    for col in range(W // C):
        head = rnd.randrange(H // C + 6); tail = rnd.randrange(4, 9)
        for k in range(tail):
            row = head - k
            if not (0 <= row < H // C): continue
            h = (col * 11 + row * 5 + seed * 37) & 0xffffffff
            if k and (h // n) % 9 >= 4: continue
            ic = new_img(names[h % n], 1)
            f = 1.0 if k == 0 else max(0.25, 1 - k / tail)
            ic = Image.eval(ic, lambda v, f=f: int(v * f))
            im.paste(ic, (col * C, row * C))
    return im
def main(out):
    s1x, rows = 1, []
    newnames = [t[0] for t in I.ICONS]
    pairs = [(o, n) for o in I.OLD for n in [o[3]]]
    used = {o[3] for o in I.OLD}
    font = None
    W = 760; rowh = 74
    allrows = [(o[0], o, o[3]) for o in I.OLD] + [('(new)', None, n) for n in newnames if n not in used]
    rain = rain_frame(); rain2 = rain.resize((480, 640), Image.NEAREST)
    H = 40 + rowh * len(allrows) + 20
    sheet = Image.new('RGB', (W + 520, max(H, 700)), (40, 40, 46)); d = ImageDraw.Draw(sheet)
    d.text((10, 8), 'cyd_livescan device icons: OLD 8x8 rain art (1x, 4x)  ->  NEW 16x16 emoji (1x, 4x)', fill=(255, 255, 255))
    d.text((10, 22), 'old name        old1x  old4x                      new1x  new4x             new name', fill=(180, 180, 180))
    y = 40
    for oname, o, nname in allrows:
        d.text((10, y + 28), oname, fill=(220, 220, 220))
        if o:
            sheet.paste(old_img(o[1], o[2], 1), (110, y + 28)); sheet.paste(old_img(o[1], o[2], 4), (150, y + 16))
        else:
            d.text((150, y + 28), '(no old icon)', fill=(150, 150, 150))
        d.text((240, y + 28), '->', fill=(255, 200, 0))
        sheet.paste(new_img(nname, 1), (300, y + 24)); sheet.paste(new_img(nname, 4), (340, y + 2))
        d.text((420, y + 28), nname, fill=(255, 255, 255))
        y += rowh
    d.text((W - 10, 8), 'rain frame, new icons, 1x (240x320) and 2x', fill=(255, 255, 255))
    sheet.paste(rain, (W - 230, 40)); sheet.paste(rain2, (W + 20, 40))
    sheet.save(out)
    # distinctness at 1x
    px = {n: I.pix(n) for n in newnames}
    worst = min(((sum(1 for y in range(16) for x in range(16) if px[a][y][x] != px[b][y][x]), a, b)
                 for i, a in enumerate(newnames) for b in newnames[i + 1:]))
    print('min differing pixels between any two new icons at 1x:', worst)
if __name__ == '__main__':
    main(sys.argv[1])
