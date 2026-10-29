import numpy as np,sys
from PIL import Image
d=sys.argv[1]; TH=float(sys.argv[2]) if len(sys.argv)>2 else 40
REG={'topbar':(0,0,240,26),'pins_L':(0,26,30,268),'pins_R':(208,26,240,268),'field':(30,26,208,128),
     'controlF+chip':(30,128,208,240),'rainrow':(30,240,208,268),'plate':(30,268,210,310),'bottom':(0,310,240,320)}
fs=list(range(1,631))
acc={k:[0,0,0] for k in REG}; tot=[0,0,0]; worst=[]
for f in fs:
    r=np.array(Image.open('g8ref/f%04d.png'%f).convert('RGB')).astype(int)
    q=np.array(Image.open('g8q/q%04d.png'%f).convert('RGB')).astype(int)
    m=np.array(Image.open('%s/f%04d.png'%(d,f)).convert('RGB')).astype(int)
    dist=np.sqrt(((r-m)**2).sum(2)); close=dist<=TH; ex=(q==m).all(2)
    worst.append((close.mean(),f))
    for k,(x0,y0,x1,y1) in REG.items():
        a=acc[k]; a[0]+=close[y0:y1,x0:x1].sum(); a[1]+=ex[y0:y1,x0:x1].sum(); a[2]+=close[y0:y1,x0:x1].size
    tot[0]+=close.sum(); tot[1]+=ex.sum(); tot[2]+=close.size
print('region close%%(RGB<=%g vs raw ref) exact%%(vs palette-snapped ref)'%TH)
for k,a in acc.items(): print('%-14s %6.2f %6.2f'%(k,100*a[0]/a[2],100*a[1]/a[2]))
print('%-14s %6.2f %6.2f'%('ALL',100*tot[0]/tot[2],100*tot[1]/tot[2]))
worst.sort(); print('worst frames',[(f,round(100*c,2)) for c,f in worst[:12]])
