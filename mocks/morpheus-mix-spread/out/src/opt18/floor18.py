"""v5 (Mobandy 11:52 AM ET 'large area of ground with no towers'): largest empty FLOOR square. Floor = rows 140..285 (below the wall base), x 0..239.
A floor pixel is covered if any pillar's projected box (front + back face, cap to base, +2 px for the contact shadow) contains it.
Returns the side (px, 1x) of the largest axis-aligned square of uncovered floor."""
import numpy as np
from scipy.ndimage import distance_transform_cdt
import os
FY0, FY1 = 140, int(os.environ.get('FLOOR_Y1', '285'))   # v5b: FLOOR_Y1=251 = the standable floor (a base lower than y 251 needs z0 < 0.95, which cuts the tower top off the field)
def empty_square(boxes):
    m = np.ones((FY1 - FY0 + 1, 240), bool)
    for (x0, x1, y0, y1) in boxes:
        a, b = max(y0, FY0) - FY0, min(y1 + 2, FY1) - FY0
        if b >= a: m[a:b + 1, max(x0 - 1, 0):min(x1 + 1, 239) + 1] = False
    if not m.any(): return 0
    return int(distance_transform_cdt(np.pad(m, 1), metric='chessboard').max() * 2 - 1)
def boxes_model(tw): return [(min(T['x0'], T['bx0']), max(T['x1'], T['bx1']), min(T['yt'], T['byt']), T['yb']) for T in tw]
