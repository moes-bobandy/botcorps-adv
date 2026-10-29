#!/usr/bin/env python3
"""refq: turn a 3x reference clip frame (720x960 H.264) back into native 240x320 panel pixels.
Each 3x3 block is averaged, then snapped to the nearest colour the firmware palette can emit
(pal.txt from the poke harness `ivdump`). Snapping removes most codec noise; `dist` reports how far
each block was from its snapped colour so codec-damaged blocks can be masked.
usage: refq.py PAL.txt IN.png OUT.png [OUT_dist.npy]"""
import sys, numpy as np
from PIL import Image
def pal_rgb(path):
    out = []
    for l in open(path):
        p = l.split()
        v = int(p[1], 16)
        r5, g6, b5 = v >> 11, (v >> 5) & 63, v & 31
        out.append(((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2)))
    return np.array(sorted(set(out)), dtype=np.float32)
def quant(pal, img3):
    a = np.asarray(img3.convert('RGB'), dtype=np.float32)
    h, w = a.shape[0] // 3, a.shape[1] // 3
    blk = a[:h * 3, :w * 3].reshape(h, 3, w, 3, 3).mean(axis=(1, 3))
    d = ((blk[:, :, None, :] - pal[None, None, :, :]) ** 2).sum(-1)
    k = d.argmin(-1)
    return pal[k].astype(np.uint8), np.sqrt(d.min(-1))
if __name__ == '__main__':
    pal = pal_rgb(sys.argv[1])
    q, dist = quant(pal, Image.open(sys.argv[2]))
    Image.fromarray(q).save(sys.argv[3])
    if len(sys.argv) > 4: np.save(sys.argv[4], dist)
