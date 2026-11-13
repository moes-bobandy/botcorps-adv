#!/usr/bin/env python3
"""v11 touch targets, read back from sim_live itwstate (320 idle samples every 90 ms = 28.8 s, two full Gibson sways, with every pillar drifting).
Hit rule = mo_hit: every point of the field rows 35..285 belongs to the pillar whose centre line (its front-face centre column,
from lv_rd_box pad 3 top to bottom) is nearest, front-first on ties. Reported: per pillar the side of the largest square it owns, worst / 10th pct / median over all samples."""
import re,subprocess,json,numpy as np,os
W='/tmp/morpheus_v13'; os.makedirs(W+'/touch',exist_ok=True); FY0,FY1=35,285
L=['tick 67','tick 190','go 13','tick 2']+sum([['tick 3','itwstate'] for _ in range(320)],[])
open(W+'/touch/s.txt','w').write('\n'.join(L)+'\n')
out=subprocess.run([W+'/sim_new',W+'/touch/s.txt',W+'/touch'],cwd='/workspace/cyd-morpheus-v6-wt',capture_output=True,text=True,check=True).stdout
open(W+'/touch/out.txt','w').write(out)
blks=out.split('ITW scr=')[1:]
def maxsq(A):
    I=np.pad(A.astype(np.int32).cumsum(0).cumsum(1),((1,0),(1,0))); lo=0
    for s in range(1,120):
        W_=I[s:,s:]-I[:-s,s:]-I[s:,:-s]+I[:-s,:-s]
        if (W_==s*s).any(): lo=s
        else: break
    return lo
yy,xx=np.mgrid[0:320,0:240]
def owner(T,o,FC):
    A=np.full((320,240),-1); best=np.full((320,240),1e18)
    for i in reversed(o):   # front first; strict < keeps the front pillar on ties (as mo_hit)
        n,s,(x0,y0,x1,y1)=T[i]
        if not s: continue
        dx2=(2*xx-FC[i]).astype(float); dy=np.maximum(np.maximum(y0-yy,yy-y1),0); d=dx2*dx2+4.0*dy*dy
        sel=(d<best)&(yy>=FY0)&(yy<=FY1); A[sel]=i; best=np.where(sel,d,best)
    return A
worst={}; samples=[]; face_ok={}; face_n={}
for b in blks:
    T={}
    FC={}
    for m in re.finditer(r'T(\d+) (.{12}) icon=-?\d+ seen=(\d) hit=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\] face=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]',b):
        g=list(map(int,m.groups()[3:])); T[int(m.group(1))]=(m.group(2).strip(),m.group(3)=='1',tuple(g[:4])); FC[int(m.group(1))]=g[4]+g[6]
    o=list(map(int,re.search(r'ord=([\d,]+)',b).group(1).split(',')))
    A=owner(T,o,FC); row={}
    for m in re.finditer(r'T(\d+) (.{12}) icon=-?\d+ seen=(\d) hit=\[[-\d,]+\] face=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]',b):
        i=int(m.group(1)); n=m.group(2).strip(); fx0,fyt,fx1,fyb=map(int,m.groups()[3:]); cx=(fx0+fx1)//2; cy=max(35,min(285,(fyt+fyb)//2))
        if 0<=cx<240: face_n[n]=face_n.get(n,0)+1; face_ok[n]=face_ok.get(n,0)+int(A[cy,cx]==i)
    for i,(n,s,_) in T.items():
        v=maxsq(A==i) if s else 0; row[n]=int(v); worst[n]=min(worst.get(n,999),v)
    samples.append(row)
names=list(worst)
p10={n:int(np.percentile([r[n] for r in samples],10)) for n in names}; med={n:int(np.median([r[n] for r in samples])) for n in names}
res={'what':__doc__.strip(),'samples':len(blks),'square_px_worst':{k:int(v) for k,v in worst.items()},'square_px_p10':p10,'square_px_median':med,
 'min_worst':int(min(worst.values())),'face_centre_tap_hits_own_pillar':{n:'%d/%d'%(face_ok.get(n,0),face_n[n]) for n in face_n},'face_centre_hit_rate_min':round(min(face_ok.get(n,0)/face_n[n] for n in face_n),3),'samples_all_ge_36':sum(all(v>=36 for v in r.values()) for r in samples),
 'towers_btn':{'rect':[170,247,239,285],'w':70,'h':39},'every_tower_tappable_every_sample':min(worst.values())>=1}
json.dump(res,open(W+'/touch_targets.json','w'),indent=1)
print(json.dumps({k:res[k] for k in ('face_centre_tap_hits_own_pillar','min_worst','samples_all_ge_36','square_px_worst','square_px_median')}))
