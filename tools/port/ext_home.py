# Extract the LOCKED ui_home_tap_glow (variant O) layers at 1x: pre-glow board X(n) (rain steps) + glow pixel tables.
# Runs the locked generator from the lock store copy (regen_O.py redirect), reads nothing from live /workspace/out.
# Usage: python ext_home.py LOCKDIR OUT.npz
import sys, os, math
LOCK = os.path.abspath(sys.argv[1]); OUT = os.path.abspath(sys.argv[2])
R = os.path.join(LOCK, 'ui_home_tap_glow', 'regen_O.py')
pre = open(R).read().split("Pp = g['preps3']")[0]
sys.argv = [R, '/tmp/fwp/ext/regen_O_scratch']; __file__ = R
exec(compile(pre, R, 'exec'))
import numpy as np
from PIL import Image
Pp = g['preps3']('green'); M0 = NM = OM = None
X = []; O = []; S = []
for n in range(g['NF']):
    s = g['breath'](n); _, f = g['fframe'](Pp, n); F = g['faceglow'](f, 1.0)
    if M0 is None: M0 = g['bezel_mask'](F)
    Gb = g['gframe'](f, M0); Hb, _, _ = g['hframe'](Gb); Ib, _ = g['iframe'](Hb, Gb); Bf = g['jframe'](Ib)
    if NM is None: NM = g['n_masks'](Bf)
    Nf = g['nglow'](Bf, s, NM); Sf, SH = g['o_shade'](Nf)
    if OM is None: OM = g['o_masks'](Nf, SH)
    Of = g['oglow'](Sf, s, OM)
    x = Bf.copy(); fm = OM[0]; x[fm] = Sf[fm]          # GIBSON face: O's cream ramp (shades = J pixels)
    X.append(x); O.append(Of); S.append(s)
glows = []   # (lift mask, ring mask, alpha, acc)
for k, (fm, lift, ring, alpha, acc) in NM.items(): glows.append((k, lift, ring, alpha, acc))
fm, lift, ring, alpha = OM; glows.append(('GIBSONbot', lift, ring, alpha, np.array(g['G_COL'], float)))
np.savez_compressed(OUT, X=np.array(X), O=np.array(O), S=np.array(S), names=np.array([q[0] for q in glows]),
                    lift=np.array([q[1] for q in glows]), ring=np.array([q[2] for q in glows]), alpha=np.array([q[3] for q in glows]),
                    acc=np.array([q[4] for q in glows]), gamma=g['GAMMA'], T0=g['T0'])
print('ok', len(X), [q[0] for q in glows], 'GAMMA', g['GAMMA'])
