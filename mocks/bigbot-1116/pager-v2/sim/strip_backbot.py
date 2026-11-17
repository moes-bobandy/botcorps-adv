#!/usr/bin/env python3
"""Restore the bottom-right mounting-hole chrome under BACKbot.

BACKbot is painted only in LV_R_BACKBOT {221,301,239,319}. Every fill pixel
is copied from the same still's already-rendered chrome:
  - 5V pad body from the 3V3 pad (same kind-0 power pad, +28 px = two pin pitches)
  - 5V left nub column continued from the visible 5V nubs already on the still
  - hole ring from the left mounting hole, horizontally mirrored (cx 8 <-> 231)
  - inner rule, theme side-edge, and gold rim from the matching clean rows
    of this still (left bottom corner is the intact model for y=316-319)
No freehand redraw of the board.
"""
import numpy as np

BACK = (221, 301, 239, 319)  # inclusive


def _c(im, y, x):
    return im[y, x].copy()


def strip_backbot(im: np.ndarray) -> np.ndarray:
    out = im.copy()
    x0, y0, x1, y1 = BACK
    gold = _c(im, 319, 100)
    rule = _c(im, 290, 237)
    edge = _c(im, 290, 239)          # theme-tinted right rim, mid-board
    black = np.array([0, 0, 0], dtype=np.uint8)
    nub = _c(im, 300, 222)           # visible 5V nub (x=222, y=299-300)

    # Clear the formal rect (icon + pins sit only here).
    out[y0:y1 + 1, x0:x1 + 1] = black

    # 5V pad body: 3V3 pad is 28 px above (pin pitch 14, two steps).
    # Visible 5V rows y=299-300 already match the 3V3 pad body; copy the
    # covered rows y=301-304 from 3V3 y=273-276, x=227-233 only.
    out[301:305, 227:234] = im[273:277, 227:234]

    # 5V left nub: visible rows are x=222 only (not 3V3's x=221+222 pair).
    # Continue that column through the pad face (y=301-303); y=304 is the
    # solid bottom row, no nub, matching 3V3 y=276.
    for y in range(301, 304):
        out[y, 222] = nub

    # Right hole ring: exact left ring, mirrored about x=119.5 (x' = 239-x).
    # Left ring lives in x=5-11, y=308-314; copy a 1-px black margin too.
    for y in range(308, 315):
        for x in range(4, 13):
            dx = 239 - x
            if x0 <= dx <= x1:
                out[y, dx] = im[y, x]

    # Right / bottom chrome inside the formal rect, modelled on the intact
    # left corner of this still and on the clean right rail at y=290.
    for y in range(301, 316):
        out[y, 237] = rule
        out[y, 238] = black
        out[y, 239] = edge
    # y=316: gold corner begins on the left at x=0; mirror to x=239.
    out[316, 237] = rule
    out[316, 238] = black
    out[316, 239] = gold
    # y=317: inner rule runs x=2..237; gold at x=239; black at x=238.
    out[317, x0:238] = rule
    out[317, 238] = black
    out[317, 239] = gold
    # y=318: black, gold at x=239.
    out[318, x0:239] = black
    out[318, 239] = gold
    # y=319: gold rim.
    out[319, x0:x1 + 1] = gold
    return out


def diff_report(before, after):
    d = np.any(before != after, axis=2)
    if not d.any():
        return {'changed_px': 0, 'bbox': None, 'outside_formal': 0}
    ys, xs = np.where(d)
    formal = np.zeros(d.shape, dtype=bool)
    formal[301:320, 221:240] = True
    return {
        'changed_px': int(d.sum()),
        'bbox': (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())),
        'outside_formal': int((d & ~formal).sum()),
        'in_formal': int((d & formal).sum()),
    }


if __name__ == '__main__':
    import sys
    from PIL import Image
    from pathlib import Path
    src, dst = Path(sys.argv[1]), Path(sys.argv[2])
    a = np.array(Image.open(src).convert('RGB'))
    b = strip_backbot(a)
    Image.fromarray(b).save(dst)
    print(diff_report(a, b))
