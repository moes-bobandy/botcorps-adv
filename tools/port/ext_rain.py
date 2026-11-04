# Extract the LOCKED ui_main_bot_rain (gaps3) chips at 1x: base input frames + the 10 rain steps (T0..T0+9), green and orange.
# Every /workspace/out and /tmp/mbr_fr read is redirected into LOCKED/ui_main_bot_rain/repro (the lock's own copies).
# Firmware sources (lv_font.h, lv_icons.h, botcorps_rain.cpp) are read from this repo (unchanged since the lock's source commit 87d27d4).
# Usage: python ext_rain.py LOCKDIR OUT.npz
import sys, os, builtins, io, importlib.machinery
LOCK = os.path.abspath(sys.argv[1]); OUT = os.path.abspath(sys.argv[2]); RP = os.path.join(LOCK, 'ui_main_bot_rain', 'repro')
MAP = [('/workspace/out/morpheus-rename/frames', RP + '/inputs/morpheus-rename/frames'), ('/tmp/mbr_fr', RP + '/inputs/mbr_fr'), ('/workspace/out', RP)]
def mp(p):
    if isinstance(p, str):
        for a, b in MAP:
            if p == a or p.startswith(a + '/'): return b + p[len(a):]
    return p
_open = builtins.open
def ropen(f, *a, **k): return _open(mp(f), *a, **k)
builtins.open = ropen; io.open = ropen
class Redirect:
    @staticmethod
    def find_spec(name, path=None, target=None):
        return importlib.machinery.PathFinder.find_spec(name, [mp(p) for p in (path if path is not None else sys.path)], target)
sys.meta_path.insert(0, Redirect); sys.dont_write_bytecode = True
import PIL.Image as _I
_iopen = _I.open; _I.open = lambda fp, *a, **k: _iopen(mp(fp), *a, **k)
G3 = os.path.join(LOCK, 'ui_main_bot_rain', 'gaps3.py')
g = {'__name__': 'port_ext'}; exec(compile(_open(G3).read().rsplit("if __name__ == '__main__':", 1)[0], G3, 'exec'), g)
import numpy as np
res = {}
for th in ('green', 'orange'):
    Pp = g['preps3'](th); T0 = g['T0']
    st = [g['chips3'](Pp, T0 + i) for i in range(10)]
    assert all((a == b).all() for a, b in zip(g['chips3'](Pp, T0 + 10), st[0]))
    for j, nm in enumerate(('gib', 'scan', 'term')): res['%s_%s' % (nm, th)] = np.array([s[j] for s in st]).astype(np.uint8); res['%smask_%s' % (nm, th)] = Pp[j][0][1]
    res['gibin_%s' % th] = g['G'].load1x(g['G'].THEMES[th][0]).astype(np.uint8)
    res['scanin_%s' % th] = g['G'].load1x(g['SCAN_FR'][th]).astype(np.uint8); res['termin_%s' % th] = g['G'].load1x(g['TERM_FR'][th]).astype(np.uint8)
# check vs the locked 3-up still (green, step t=9)
from PIL import Image
ref = np.array(_iopen(os.path.join(LOCK, 'ui_main_bot_rain', 'threeup_gibson_scan_terminal_green_3x_gaps3.png')).convert('RGB'))
print('3-up still identical:', (g['tile']([res['gib_green'][0], res['scan_green'][0], res['term_green'][0]]) == ref).all())
np.savez_compressed(OUT, **res); print('ok', list(res))
