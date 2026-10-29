import numpy as np,sys
from PIL import Image
f=int(sys.argv[1]); x0,y0,x1,y1=map(int,sys.argv[2:6]); s=int(sys.argv[6]); d=sys.argv[7] if len(sys.argv)>7 else 'hb/gp/out/v8'
r=np.array(Image.open('g8ref/f%04d.png'%f).convert('RGB')).astype(int)[y0:y1,x0:x1]
m=np.array(Image.open('%s/f%04d.png'%(d,f)).convert('RGB')).astype(int)[y0:y1,x0:x1]
bad=np.sqrt(((r-m)**2).sum(2))>40
dv=m//4; dv[bad]=[255,0,0]
sep=np.full((r.shape[0],2,3),255)
img=np.concatenate([r,sep,m,sep,dv],1).astype(np.uint8)
Image.fromarray(img).resize((img.shape[1]*s,img.shape[0]*s),Image.NEAREST).save('crop_%d_%d_%d.png'%(f,x0,y0))
print('close %.2f%%'%(100*(1-bad.mean())), 'crop_%d_%d_%d.png'%(f,x0,y0))
