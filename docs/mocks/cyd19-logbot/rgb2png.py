import sys, numpy as np
from PIL import Image
src,dst,scale=sys.argv[1],sys.argv[2],int(sys.argv[3])
d=np.fromfile(src,dtype=np.uint8)[:230400].reshape(320,240,3)
im=Image.fromarray(d)
if scale>1: im=im.resize((240*scale,320*scale),Image.NEAREST)
im.save(dst)
