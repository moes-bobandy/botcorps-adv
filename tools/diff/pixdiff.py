import sys
from PIL import Image
# masked pixel-diff: firmware vs mock, excluding Dirt rule-override rects.
# usage: pixdiff.py fw.png mock.png label x0,y0,x1,y1 [more masks...]
def load(p): return Image.open(p).convert('RGB').resize((320,240))
fw=load(sys.argv[1]); mk=load(sys.argv[2]); label=sys.argv[3]
masks=[]
for a in sys.argv[4:]:
    masks.append(tuple(int(v) for v in a.split(',')))
fp=fw.load(); mp=mk.load()
def masked(x,y):
    for (x0,y0,x1,y1) in masks:
        if x0<=x<x1 and y0<=y<y1: return True
    return False
tot=0; diff=0; dist=0.0
TH=48  # per-channel sum threshold for "different"
for y in range(240):
    for x in range(320):
        if masked(x,y): continue
        r1,g1,b1=fp[x,y]; r2,g2,b2=mp[x,y]
        d=abs(r1-r2)+abs(g1-g2)+abs(b1-b2); dist+=d; tot+=1
        if d>TH: diff+=1
print(f"{label}: diff {100.0*diff/tot:.1f}% of {tot} px (masked {320*240-tot}), mean dist {dist/tot:.1f}/765")
