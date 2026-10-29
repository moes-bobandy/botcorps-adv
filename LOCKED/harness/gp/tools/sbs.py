import numpy as np,sys,os
from PIL import Image, ImageDraw
d=sys.argv[1]; od=sys.argv[2]; os.makedirs(od,exist_ok=True)
for f in map(int,sys.argv[3:]):
    r=np.array(Image.open('g8ref/f%04d.png'%f).convert('RGB')).astype(int)
    m=np.array(Image.open('%s/f%04d.png'%(d,f)).convert('RGB')).astype(int)
    bad=np.sqrt(((r-m)**2).sum(2))>40
    dv=m//4; dv[bad]=[255,0,0]
    sep=np.full((320,4,3),255)
    img=np.concatenate([r,sep,m,sep,dv],1).astype(np.uint8)
    im=Image.fromarray(img).resize((img.shape[1]*2,640),Image.NEAREST)
    can=Image.new('RGB',(im.width,664),'black'); can.paste(im,(0,24))
    ImageDraw.Draw(can).text((4,6),'f%04d  ref (3x mp4 -> 240x320) | rebuild | diff (red = RGB dist > 40)   close %.2f%%'%(f,100*(1-bad.mean())),fill='white')
    can.save('%s/sbs_f%04d.png'%(od,f))
