#!/usr/bin/env python3
"""Pixel-diff the old-terminal prompt + buttons against each new plate (mode 0).
Region = the plate's bottom 31 rows: left frame strip (5 px) + right 80 px (cmdline right end, "=",
"abc", frames), anchored to the plate's bottom-left / bottom-right like the old paint_pterm.
Masked: the cmdline interior (typed text / prompt text / caret). Silk is above the region."""
import sys
from PIL import Image
OLD = ('/tmp/oldP/term.png', (10, 249, 220, 53))
def regions(x, y, w, h):
    gx, by = x + w - 76, y + h - 29
    rows = range(by - 2, y + h)
    return gx, by, [(c, r) for r in rows for c in list(range(x, x + 5)) + list(range(gx - 4, x + w))]
def masked(x, y, w, h, c, r):
    gx, by = x + w - 76, y + h - 29
    return x + 3 <= c <= gx - 4 and by + 9 <= r <= by + 19
def cells(img, rect):
    x, y, w, h = rect
    gx, by, pts = regions(*rect)
    out = []
    for (c, r) in pts:
        rel = ('L', c - x, r - (y + h)) if c < x + 5 else ('R', c - (x + w), r - (y + h))
        v = None if masked(x, y, w, h, c, r) else img.getpixel((c, r))[:3]
        out.append((rel, v))
    return out
if __name__ == '__main__':
    old = Image.open(OLD[0]).convert('RGB'); oc = dict(cells(old, OLD[1]))
    for arg in sys.argv[1:]:
        path, rect = arg.split('@'); rect = tuple(map(int, rect.split(',')))
        im = Image.open(path).convert('RGB'); nc = dict(cells(im, rect))
        n = sum(1 for k, v in nc.items() if v is not None)
        d = sum(1 for k, v in nc.items() if v is not None and oc.get(k) != v)
        print('%s compared=%d diff=%d' % (path, n, d))
