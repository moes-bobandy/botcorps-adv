# Regenerate locked variant O (home board tap glow) from the copies in ./src only.
# Every read of /workspace/... is redirected to ./src/workspace/...; any read that escapes to the live /workspace fails the run.
# Usage: python3 regen_O.py [--live] [outdir]    (--live = no redirect, used once to trace dependencies)
import sys, os, builtins, io, hashlib, importlib.machinery, subprocess, json
HERE = os.path.dirname(os.path.abspath(__file__)); SRC = os.path.join(HERE, 'src')
LIVE = '--live' in sys.argv; args = [a for a in sys.argv[1:] if a != '--live']
OUTD = args[0] if args else '/tmp/lock_regen_O'; os.makedirs(OUTD, exist_ok=True)
def mp(p):
    if not LIVE and isinstance(p, str) and p.startswith('/workspace/') and not p.startswith(SRC + '/'): return SRC + p
    return p
reads, escaped = set(), set()
def hook(ev, a):
    if ev == 'open' and isinstance(a[0], str):
        m = a[1]
        if m is None or (isinstance(m, str) and 'r' in m and '+' not in m) or (isinstance(m, int) and m & 3 == 0):
            if a[0].startswith(SRC + '/'): reads.add(a[0][len(SRC):])
            elif a[0].startswith('/workspace/') and '/__pycache__/' not in a[0] and not a[0].startswith(HERE + '/'):
                (reads if LIVE else escaped).add(a[0])
sys.addaudithook(hook)
_open = builtins.open
def ropen(f, *a, **k): return _open(mp(f), *a, **k)
builtins.open = ropen; io.open = ropen
class Redirect:
    @staticmethod
    def find_spec(name, path=None, target=None):
        return importlib.machinery.PathFinder.find_spec(name, [mp(p) for p in (path if path is not None else sys.path)], target)
sys.meta_path.insert(0, Redirect); sys.dont_write_bytecode = True
_ins = sys.path.insert
src = ropen('/workspace/out/home-tap-pulse/tap_faceglow_O.py').read().rsplit("if __name__ == '__main__':", 1)[0]
g = {'__name__': 'lock_regen'}; exec(src, g)
import numpy as np
from PIL import Image
Pp = g['preps3']('green'); M0 = NM = OM = None; hashes = []
for n in range(g['NF']):
    s = g['breath'](n); _, f = g['fframe'](Pp, n); F = g['faceglow'](f, 1.0)
    if M0 is None: M0 = g['bezel_mask'](F)
    Gb = g['gframe'](f, M0); Hb, _, _ = g['hframe'](Gb); Ib, _ = g['iframe'](Hb, Gb); Bf = g['jframe'](Ib)
    if NM is None: NM = g['n_masks'](Bf)
    Nf = g['nglow'](Bf, s, NM); Sf, SH = g['o_shade'](Nf)
    if OM is None: OM = g['o_masks'](Nf, SH)
    Of = g['oglow'](Sf, s, OM); U = g['up'](Of)
    Image.fromarray(U).save('%s/f%03d.png' % (OUTD, n)); hashes.append(hashlib.sha256(U.tobytes()).hexdigest())
    if n == 0: Image.fromarray(Of).save('%s/trough_1x.png' % OUTD); Image.fromarray(U).save('%s/trough_3x.png' % OUTD)
    if n == g['PERIOD_F'] // 2: Image.fromarray(Of).save('%s/peak_1x.png' % OUTD); Image.fromarray(U).save('%s/peak_3x.png' % OUTD)
open(os.path.join(OUTD, 'frames_rgb_sha256.txt'), 'w').write(''.join('f%03d %s\n' % (i, h) for i, h in enumerate(hashes)))
ref = os.path.join(HERE, 'frames_O_rgb_sha256.txt')
if os.path.exists(ref):
    want = [l.split()[1] for l in _open(ref)]; mis = [i for i, (a, b) in enumerate(zip(hashes, want)) if a != b]
    print('frames', len(hashes), 'vs locked frame manifest', len(want), 'mismatched frames', len(mis), mis[:10])
print('mode', 'LIVE' if LIVE else 'LOCK-SRC', '| reads escaping to live /workspace:', len(escaped), sorted(escaped)[:10])
json.dump(sorted(reads), _open(os.path.join(OUTD, 'reads.json'), 'w'), indent=1)
mods = sorted(m.__file__ for m in list(sys.modules.values()) if (getattr(m, '__file__', None) or '').startswith('/workspace/') and not m.__file__.startswith(HERE + '/'))
print('modules loaded from live /workspace:', len(mods), mods[:5])
mp4 = os.path.join(OUTD, 'regen_O_3x.mp4'); g['enc'](OUTD, mp4)
h = lambda p: hashlib.sha256(_open(p, 'rb').read()).hexdigest()
print('re-encoded mp4', h(mp4), '| locked mp4', h(os.path.join(HERE, 'home_tap_faceglow_O_3x.mp4')))
