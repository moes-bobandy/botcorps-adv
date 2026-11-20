import numpy as np, sys
from PIL import Image, ImageDraw
def cam(d,n): return np.memmap(f'{d}/{n}.cam.rgb',np.uint8,'r').reshape(-1,320,240,3)
def gif(n, secs, out, scale=2, step=2):
    A=cam('V0',n); B=cam('V3',n); N=min(int(secs*1000/20), len(A), len(B))
    fr=[]
    for i in range(0,N,step):
        c=np.zeros((340,484,3),np.uint8); c[20:,0:240]=A[i]; c[20:,244:]=B[i]; c[:,240:244]=40
        im=Image.fromarray(c); dr=ImageDraw.Draw(im)
        dr.text((3,4),'BEFORE v1.2.16 fd8efa8',fill=(255,90,90)); dr.text((247,4),'AFTER pullup-glitch fix',fill=(90,255,140))
        dr.text((170,4),f'{i*20/1000:4.2f}s',fill=(200,200,200))
        if scale!=1: im=im.resize((484*scale,340*scale),Image.NEAREST)
        fr.append(im.convert('P',palette=Image.ADAPTIVE,colors=255))
    fr[0].save(out,save_all=True,append_images=fr[1:],duration=20*step,loop=0,optimize=False)
    print(out,len(fr))
O='/workspace/out/fw-bigbot-1117/pullup-glitch/'
gif('gib_pull',8.0,O+'gibson_pullup_DEVICE-TIMELINE_before-vs-after_SIM@2x.gif')
gif('dl_pull',8.0,O+'hackbot_pullup_DEVICE-TIMELINE_before-vs-after_SIM@2x.gif')
