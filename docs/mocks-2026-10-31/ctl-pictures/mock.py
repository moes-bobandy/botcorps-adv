import sys; sys.path.insert(0,'.'); sys.path.insert(0,'/workspace/scratch-p1/cs')
from chips import *
from PIL import Image, ImageDraw, ImageFont
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',30)
Fs=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',22)
def ld(t): return Image.frombytes('RGB',(240,320),open(f'o/ctl_t{t}.rgb','rb').read())
R={'THEME':(82,36,112,44),'FOCUS':(82,66,112,74),'MIX':(36,134,69,147),'RAINCOL':(36,180,69,193),'MASHUP':(36,244,69,257),
   'RAIN':(166,125,204,138),'RMODE':(166,159,204,172),'MULTI':(182,222,207,235),'ORIENT':(90,242,160,256),'MOTION':(178,246,198,252)}
def compose(t,st,bgA=False):
    im=ld(t).copy()
    def put(k,c):
        x0,y0,x1,y1=R[k]; assert c.size==(x1-x0+1,y1-y0+1),(k,c.size); im.paste(c,(x0,y0))
    put('THEME',theme_chip(t)); put('FOCUS',focus_chip(st.get('focus',0))); put('MIX',mix_chip(st.get('mix',0)))
    put('RAINCOL',bgcol_chip(st.get('bgc',0)) if bgA else col_chip([0x20ff40,0x00ffff,ACC[t]][st.get('rcol',0)]))
    put('MASHUP',speed_chip(st.get('speed',1)))
    put('RAIN',bgstyle_chip(st.get('bgs',0)) if bgA else rain_chip(st.get('rain',1)))
    put('RMODE',rmode_chip(st.get('rmode',0))); put('MULTI',multi_chip(st.get('auto',0)))
    # MULTI LED (unchanged, part of the shipping chip) - redraw so the state matches
    p=im.load(); led=(0x20,0xff,0x40) if st.get('auto',0) else (0xff,0x30,0x48)
    for y in range(222,234):
        for x in range(168,180): p[x,y]=led
    put('ORIENT',orient_chip(st.get('flip',0))); put('MOTION',motion_chip(st.get('motion',1)))
    return im
