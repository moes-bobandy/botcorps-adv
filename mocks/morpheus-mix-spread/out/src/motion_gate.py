#!/usr/bin/env python3
"""v5c (Mobandy 2:05 PM ET 'they should not be locked in movement with one another'): pairwise motion independence on the real-sim
320 idle samples (0.3 s apart). Per tower: screen centre x (raw, includes the shared camera sway), camera-free screen x
(120 + F X / Z: the tower's own drift only) and depth Z. Motion = sample-to-sample difference. Reports the Pearson correlation of the
motion over all 153 pairs, and how many pairs change their camera-free screen spacing (|u_i - u_j|) by >= 8 px over the samples."""
import numpy as np
F = 120.0
def motion(SM, n=18):
    cx = np.array([[(a['tw'][i]['x0'] + a['tw'][i]['x1']) / 2.0 for i in range(n)] for a in SM])
    u = np.array([[120 + F * a['tw'][i]['X'] / a['tw'][i]['Z'] for i in range(n)] for a in SM])
    Z = np.array([[a['tw'][i]['Z'] for i in range(n)] for a in SM])
    def corr(M):
        d = np.diff(M, axis=0); C = np.corrcoef(d.T); iu = np.triu_indices(n, 1); r = np.abs(C[iu]); return r
    rr, ru, rz = corr(cx), corr(u), corr(Z)
    iu = np.triu_indices(n, 1); sp = np.abs(u[:, iu[0]] - u[:, iu[1]]); rng = sp.max(0) - sp.min(0)   # camera-free spacing (the camera sway alone also changes spacing by parallax)
    k = int(np.argmax(ru))
    return dict(pairs=len(ru), own_drift_mean_abs_r=round(float(ru.mean()), 3), own_drift_max_abs_r=round(float(ru.max()), 3), own_drift_max_pair=[int(iu[0][k]), int(iu[1][k])],
                depth_mean_abs_r=round(float(rz.mean()), 3), depth_max_abs_r=round(float(rz.max()), 3),
                raw_screen_mean_abs_r=round(float(rr.mean()), 3), raw_screen_max_abs_r=round(float(rr.max()), 3),
                spacing_pairs_changing_8px=int((rng >= 8).sum()), spacing_median_change_px=round(float(np.median(rng)), 1), spacing_min_change_px=round(float(rng.min()), 1))
if __name__ == '__main__':
    import json, sys
    SM = json.load(open(sys.argv[1]), object_hook=lambda d: {int(k) if k.isdigit() else k: v for k, v in d.items()})
    print(json.dumps(motion(SM)))
