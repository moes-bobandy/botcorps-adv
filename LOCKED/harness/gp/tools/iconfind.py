import numpy as np,sys
from PIL import Image
ic=np.array(Image.open('hb/gp/out/dbg/icons.png').convert('RGB')).astype(int)
N=15
T=[]
for i in range(N):
    x=4+16*(i%14); y=4+20*(i//14)
    t=ic[y:y+12,x:x+12]; T.append(t)
names='AP PHONE LAPTOP TV SPEAKER WATCH? TRACKER CAMERA CAR UNKNOWN DRONE DEVBOARD PRINTER'.split()
def find(f):
    q=np.array(Image.open('g8q/q%04d.png'%f).convert('RGB')).astype(int)
    out=[]
    for i,t in enumerate(T):
        m=t.sum(2)>0
        if m.sum()==0: continue
        for y in range(34,112):
            for x in range(42,190):
                R=q[y:y+12,x:x+12]
                if R.shape[:2]!=(12,12): continue
                d=np.abs(R-t).sum(2)[m]
                if (d<40).mean()>0.9: out.append((i,x,y,round((d<40).mean(),3)))
    return out
for f in map(int,sys.argv[1:]):
    print(f,find(f))
