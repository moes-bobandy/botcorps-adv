import sys,numpy as np
base=sys.argv[1]; ks=sys.argv[2:]
def load(f): return [dict(kv.split('=') for kv in l.split()) for l in open(f)]
for n in ['gib_idle','gib_pull','dl_pull']:
    for k in ks:
        ms=[];cpu=[];step=[];kb=[];slow=0;fr=0
        for r in (1,2,3):
            L=load(f'{base}/r{r}/{k}/{n}.dev.txt'); m=[float(d['ms']) for d in L]; ms+=m; cpu+=[float(d['cpu']) for d in L]; step+=[float(d['step']) for d in L[1:]]; kb+=[int(d['px'])*2/1024 for d in L]
            slow+=sum(x>100 for x in m); fr+=len(m)
        ms=np.array(ms); step=np.array(step)
        print(f'{n:9s} {k}: ms/frame avg {ms.mean():6.1f} p95 {np.percentile(ms,95):6.1f} max {ms.max():6.1f} | CPU {np.mean(cpu):6.1f} | SPI {np.mean(kb):4.0f} KB/f | fps {1000/ms.mean():5.1f} | motion step max {step.max():6.1f} ms | frames >100 ms: {slow}/{fr}')
