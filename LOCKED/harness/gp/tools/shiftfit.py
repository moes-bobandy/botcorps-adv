import numpy as np,sys
from PIL import Image
f=int(sys.argv[1]); x0,y0,x1,y1=map(int,sys.argv[2:6]); d=sys.argv[6] if len(sys.argv)>6 else 'hb/gp/out/v8'
r=np.array(Image.open('g8ref/f%04d.png'%f).convert('RGB')).astype(int)
m=np.array(Image.open('%s/f%04d.png'%(d,f)).convert('RGB')).astype(int)
res=[]
for dy in range(-3,4):
  for dx in range(-3,4):
    R=r[y0:y1,x0:x1]; M=m[y0+dy:y1+dy,x0+dx:x1+dx]
    res.append(((np.sqrt(((R-M)**2).sum(2))<=40).mean(),dx,dy))
res.sort(reverse=True); print(res[:4])
