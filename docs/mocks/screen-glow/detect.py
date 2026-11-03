# Locate each tappable chip's face and its glow-allowed window on a locked screen (1x, median of the decoded locked frames).
import numpy as np
from scipy import ndimage as ndi
def bbox(m): ys, xs = np.nonzero(m); return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max()))
def dark_cc(med, seed):
    """walk left from the seed to the first dark pixel whose dark component is big (window ring or icon screen)."""
    s = med.astype(int).sum(-1); dark = s < 90; lab, _ = ndi.label(dark); x, y = seed
    while x > 0:
        if dark[y, x] and (lab == lab[y, x]).sum() >= 30: return lab == lab[y, x]
        x -= 1
    raise ValueError(seed)
def face_of(med, seed, kind):
    cc = dark_cc(med, seed); x0, y0, x1, y1 = bbox(cc); box = np.zeros_like(cc); box[y0:y1 + 1, x0:x1 + 1] = True
    face = box if kind == 'screen' else box & ~cc            # solid face: inside the black window ring; icon screen: the whole black screen
    return face, (x0, y0, x1, y1)
