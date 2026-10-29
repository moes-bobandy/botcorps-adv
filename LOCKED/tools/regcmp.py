#!/usr/bin/env python3
"""Region compare: rebuild (native 240x320 sim PNG) vs a reference frame downscaled to 240x320
by 3x3 block mean (no palette snap, so codec noise near black is not mis-snapped).
close = % pixels with RGB euclidean distance <= T (default 40) at the same position
tol1  = % pixels within T of the ref at the same position OR at one of its 8 neighbours
        (1px edge tolerance for H.264 ringing around 3x-upscaled edges)
Usage: regcmp.py REGIONS.tsv REF.png REB.png [T]   (REGIONS.tsv: name x0 y0 x1 y1, exclusive)"""
import sys, numpy as np
from PIL import Image
def load(p):
    a=np.array(Image.open(p).convert('RGB')).astype(np.float32)
    if a.shape[0]==960: a=a.reshape(320,3,240,3,3).mean((1,3))
    return a
def cmp(ref, reb, T=40.0):
    H,W,_=ref.shape; p=np.pad(ref,((1,1),(1,1),(0,0)),mode='edge')
    d0=np.sqrt(((ref-reb)**2).sum(2)); dm=d0.copy()
    for dy in range(3):
        for dx in range(3):
            dm=np.minimum(dm,np.sqrt(((p[dy:dy+H,dx:dx+W]-reb)**2).sum(2)))
    return d0<=T, dm<=T, d0
def regions(p):
    out=[]
    for l in open(p):
        l=l.split('#')[0].split()
        if len(l)==5: out.append((l[0],*map(int,l[1:])))
    return out
def report(R, close, tol):
    rows=[]
    for n,x0,y0,x1,y1 in R:
        c=close[y0:y1,x0:x1]; t=tol[y0:y1,x0:x1]
        rows.append((n,100*c.mean(),100*t.mean(),int((~t).sum()),c.size))
    return rows
if __name__=='__main__':
    T=float(sys.argv[4]) if len(sys.argv)>4 else 40.0
    c,t,_=cmp(load(sys.argv[2]),load(sys.argv[3]),T)
    for n,a,b,m,s in report(regions(sys.argv[1]),c,t):
        print(f"{n:12s} close {a:6.2f}%  tol1 {b:6.2f}%  miss {m}/{s}")
