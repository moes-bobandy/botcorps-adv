from model import *
import random, json
from opt import axon_ok
T0=78.66; C0=-0.372; ph=math.asin(C0/0.5)
samples=[(T0,C0,10.0)]+[(T0+dt,0.5*math.sin(ph+0.45*dt),0.3) for dt in (-2,-1,1,2)]
def score(dsx,dz):
    s=0
    for (t,c,wt) in samples:
        V=vis(geom(t,c,dsx,dz))
        for i in range(18):
            e,l=V[i]; s+=wt*(3*max(0,0.97-e)+max(0,0.6-l)/0.6)
    return s+0.004*np.abs(dsx).sum()+0.5*np.abs(dz).sum()
random.seed(7)
dsx=np.array(json.load(open('dsx.json'))); dz=np.zeros(18); amin=-37
best=score(dsx,dz); print('start',best,flush=True)
for it in range(2500):
    c=dsx.copy(); z=dz.copy(); i=random.randrange(18)
    if random.random()<0.25 and i not in NEAR: z[i]=max(-0.25,min(0.25,z[i]+random.choice([-0.1,-0.05,0.05,0.1])))
    else:
        c[i]+=random.choice([-10,-6,-3,-2,2,3,6,10]); c[i]=max(-40,min(40,c[i])) if i!=10 else min(amin,max(amin-30,c[i]))
    sc=score(c,z)
    if sc<best:
        if i==10 and not axon_ok(c): continue
        best=sc; dsx=c; dz=z; print(it,round(best,3),flush=True)
        json.dump({'dsx':list(dsx),'dz':list(dz)},open('best2.json','w'))
print('DONE')
