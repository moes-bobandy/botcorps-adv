from mock import *
import subprocess
seq=[]; st={}
def add(t,s,k=None,bgA=False,n=6):
    for i in range(n): seq.append((t,dict(s),k if i<2 else None,bgA))
add(0,st,n=10)
for t in range(1,12): add(t,st,'THEME')
add(0,st,'THEME')
for key,rk,vals in (('focus','FOCUS',[1,2,3,0]),('mix','MIX',[1,2,0]),('rcol','RAINCOL',[1,2,0]),('speed','MASHUP',[2,0,1]),('rain','RAIN',[0,1]),
                    ('rmode','RMODE',[1,2,0]),('auto','MULTI',[1,2,0]),('flip','ORIENT',[1,0]),('motion','MOTION',[0,1])):
    for v in vals: st[key]=v; add(0,st,rk)
for v in [1,2,3,4,0]: st['bgs']=v; add(0,st,'RAIN',True)
for v in [1,2,3,4,0]: st['bgc']=v; add(0,st,'RAINCOL',True)
add(0,st,n=10)
p=subprocess.Popen(['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s','720x960','-r','10','-i','-','-c:v','libx264','-pix_fmt','yuv420p','-movflags','+faststart','-crf','18','/workspace/out/ctl-pictures/ctl_pictures.mp4'],stdin=subprocess.PIPE)
for t,s,k,bgA in seq:
    im=compose(t,s,bgA).resize((720,960),Image.NEAREST)
    if k:
        x0,y0,x1,y1=R[k]; cx,cy=(x0+x1)*3//2,(y0+y1)*3//2; d=ImageDraw.Draw(im); d.ellipse((cx-14,cy-14,cx+14,cy+14),outline=(255,255,0),width=3)
    p.stdin.write(im.tobytes())
p.stdin.close(); p.wait(); print(len(seq))
