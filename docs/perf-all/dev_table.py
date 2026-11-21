import sys, os, glob, numpy as np
T = sys.argv[1]; print(f"{'clip':28s} {'':4s} {'avg ms':>7s} {'p95':>7s} {'max':>7s} {'CPU':>6s} {'KB/f':>5s} {'fps':>5s} {'>100ms':>9s}")
for s in sorted(d for d in os.listdir(T) if os.path.isdir(f'{T}/{d}') and d != 'ref'):
    clips = sorted(os.path.basename(f)[:-8] for f in glob.glob(f'{T}/{s}/r1/ref/*.dev.txt'))
    for c in clips:
        for b in ('ref', 'new'):
            ms = []; cpu = []; kb = []
            for r in sorted(glob.glob(f'{T}/{s}/r*')):
                L = [dict(kv.split('=') for kv in l.split()) for l in open(f'{r}/{b}/{c}.dev.txt')]
                ms += [float(d['ms']) for d in L]; cpu += [float(d['cpu']) for d in L]; kb += [int(d['px']) * 2 / 1024 for d in L]
            ms = np.array(ms); print(f"{s+'/'+c:28s} {b:4s} {ms.mean():7.1f} {np.percentile(ms,95):7.1f} {ms.max():7.1f} {np.mean(cpu):6.1f} {np.mean(kb):5.0f} {1000/ms.mean():5.1f} {int((ms>100).sum()):4d}/{len(ms):<4d}")
