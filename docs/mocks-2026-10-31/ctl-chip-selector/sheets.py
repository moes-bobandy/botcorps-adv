import sys; sys.path.insert(0,'.')
from sel import *
from PIL import ImageFont
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',28)
Fs=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',20)
O='/workspace/out/ctl-chip-selector/'
Z=lambda im:im.resize((im.width*3,im.height*3),Image.NEAREST)
# before/after
ship=ld(0); A=compose(0,{},True); B=compose(0,{},False)
W=720; s=Image.new('RGB',(3*W+80,960+120),(24,24,28)); d=ImageDraw.Draw(s)
d.text((20,10),'CTL chip-selector mock (MOCK ONLY - not in firmware)  theme GRAPE, default choices, 3x',font=F,fill='white')
for i,(im,t) in enumerate(((ship,'BEFORE: shipping sim frame (value boxes)'),(A,'AFTER A: tinted plate in chip'),(B,'AFTER B: art brightness only'))):
    x=20+i*(W+20); d.text((x,60),t,font=Fs,fill=(255,220,120)); s.paste(Z(im),(x,100))
s.save(O+'ctl_selector_before_after_3x.png')
# all states
def crop(k):
    rs=TAP[k]; x0=min(r[0] for r in rs);y0=min(r[1] for r in rs);x1=max(r[2] for r in rs);y1=max(r[3] for r in rs)
    if k=='THEME': return (35,33,113,50)
    return (x0,y0,x1+1,y1+1)
rows=[]
for k in ['THEME','ORIENT','RAIN','BGSTYLE','RAINCOL','BGCOL','RMODE','MASHUP','MOTION','MIX','MULTI','FOCUS']:
    base={'BGSTYLE':'RAIN','BGCOL':'RAINCOL'}.get(k,k); bgA=k in('BGSTYLE','BGCOL')
    for plate in (False,True):
        tiles=[]
        for ci,cn in enumerate(CHOICES[k]):
            t=ci if k=='THEME' else 0
            st={base:ci} if not bgA else {k:ci}
            im=compose(t,st,plate,bgA).crop(crop(base)); tiles.append((cn,Z(im)))
        rows.append((f"{k}{' (A-steer)' if bgA else ''} {'UI' if base in UI else 'non-UI, dimmed'} - {'A plate' if plate else 'B brightness'}",tiles))
Wt=1500; y=70; H=sum(max(t[1].height for t in r[1])+60 for r in rows)
# wrap THEME 12 tiles into rows of 6
out=Image.new('RGB',(Wt,H+800),(24,24,28)); d=ImageDraw.Draw(out)
d.text((20,15),'CTL chip-selector: every chip, every choice, both variants (3x crops of the chip tap area)',font=F,fill='white')
for title,tiles in rows:
    d.text((20,y),title,font=Fs,fill=(255,220,120)); y+=28; x=20; rh=0
    for cn,im in tiles:
        if x+im.width>Wt-10: x=20; y+=rh+26; rh=0
        out.paste(im,(x,y)); d.text((x,y+im.height+2),cn,font=Fs,fill='white'); x+=im.width+16; rh=max(rh,im.height)
    y+=rh+40
out.crop((0,0,Wt,y)).save(O+'ctl_selector_all_states_3x.png'); print(y)
