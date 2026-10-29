#!/usr/bin/env python3
"""pxdiff REF.png OUT.png -> prints 'diff N px bbox x0,y0-x1,y1' (or 'diff 0 px'); exit 0 only on zero diff.
Size mismatch = exit 1. Exact RGB compare, no tolerance (lock rule: any non-zero diff is a FAIL)."""
import sys
import numpy as np
from PIL import Image
a = np.asarray(Image.open(sys.argv[1]).convert('RGB'))
b = np.asarray(Image.open(sys.argv[2]).convert('RGB'))
if a.shape != b.shape:
    print(f"size {a.shape[1]}x{a.shape[0]} vs {b.shape[1]}x{b.shape[0]}"); sys.exit(1)
m = (a != b).any(2); n = int(m.sum())
if n == 0:
    print("diff 0 px"); sys.exit(0)
ys, xs = np.nonzero(m)
print(f"diff {n} px bbox {xs.min()},{ys.min()}-{xs.max()},{ys.max()}"); sys.exit(1)
