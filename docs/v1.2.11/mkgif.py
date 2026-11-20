import sys, numpy as np
from PIL import Image
# mkgif.py in.rgb out.gif frame_ms [idx:extra_ms ...]  -- exact native frames, nearest 2x, true device timing
a=np.fromfile(sys.argv[1],np.uint8).reshape(-1,320,240,3); ms=int(sys.argv[3]); d=[ms]*len(a)
for kv in sys.argv[4:]:
    i,e=kv.split(':'); d[int(i)]+=int(e)
fr=[Image.fromarray(f.repeat(2,0).repeat(2,1)) for f in a]
fr[0].save(sys.argv[2],save_all=True,append_images=fr[1:],duration=d,loop=0,optimize=False)
print(sys.argv[2],len(a),'frames',sum(d),'ms')
