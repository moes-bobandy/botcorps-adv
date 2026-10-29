import numpy as np,sys
from PIL import Image
f=int(sys.argv[1]); x0,y0,x1,y1=map(int,sys.argv[2:6]); d=sys.argv[6] if len(sys.argv)>6 else 'hb/gp/out/v8'
r=np.array(Image.open('g8q/q%04d.png'%f).convert('RGB'))
m=np.array(Image.open('%s/f%04d.png'%(d,f)).convert('RGB'))
for y in range(y0,y1):
    print(y,' '.join('%02x%02x%02x/%02x%02x%02x'%(tuple(r[y,x])+tuple(m[y,x])) for x in range(x0,x1)))
