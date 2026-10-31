# CTL chip-selector mock: value rects removed (board fill), chip's own art = the selector.
from PIL import Image, ImageDraw
import math
ACC=[0xb324fd,0xdd00ac,0x92ff02,0x00ffff,0x345efb,0xfd6b00,0xffda00,0xff24fd,0x20fe01,0x00fe02,0xfe0001,0xff0052]
TN=['GRAPE','MAGENTA','TOXIC','ICE','BLUE','ORANGE','GOLD','SYNTH','AURA','PRISM','GLTCH','ROSE']
def rgb(c): return ((c>>16)&255,(c>>8)&255,c&255)
def sc(c,f): return tuple(max(0,min(255,int(v*f))) for v in c)
def ld(t): return Image.frombytes('RGB',(240,320),open(f'/workspace/scratch-p1/cp/o/ctl_t{t}.rgb','rb').read())
# value rects that are REMOVED (all of them, incl. MULTI LED+value, ORIENT PORT/FLIP pair)
REMOVED={'THEME':(82,36,112,44),'FOCUS':(82,66,112,74),'MIX':(36,134,69,147),'RAINCOL':(36,180,69,193),
 'MASHUP':(36,244,69,257),'RAIN':(166,125,204,138),'RMODE':(166,159,204,172),'MULTI':(166,220,207,237),
 'ORIENT':(90,242,160,256),'MOTION':(178,246,198,252)}
# chip own-art areas (where the per-choice picture goes)
ART={'THEME':(39,46,109,50),'FOCUS':(39,76,109,80),'MIX':(40,93,65,118),'RAINCOL':(42,148,63,169),
 'MASHUP':(40,205,65,230),'RAIN':(174,85,199,110),'RMODE':(176,137,198,147),'MULTI':(174,182,199,207),
 'ORIENT':(97,228,158,241),'MOTION':(168,239,212,251)}
# tap rects = chip art (+pins/label) padded; THEME also keeps the CONTROLbot face (its art re-inks per theme)
TAP={'THEME':[(35,33,113,50),(75,82,164,205)],'FOCUS':[(35,66,113,81)],'MIX':[(33,86,72,132)],
 'RAINCOL':[(33,144,72,179)],'MASHUP':[(33,197,72,247)],'RAIN':[(167,78,206,127)],'RMODE':[(167,134,210,162)],
 'MULTI':[(167,175,206,221)],'ORIENT':[(92,219,162,245)],'MOTION':[(165,236,213,255)]}
UI={'THEME','ORIENT','RAIN','RAINCOL','RMODE','MASHUP','MOTION'}
CHOICES={'THEME':TN,'FOCUS':['ALL','WIFI','BLE','ALERTS'],'MIX':['ALL','WIFI','BLE'],'RAINCOL':['GREEN','CYAN','THEME'],
 'MASHUP':['SLOW','MED','FAST'],'RAIN':['ON','OFF'],'RMODE':['DRIFT','SWEEP','STILL'],'MULTI':['OFF','30S','60S'],
 'ORIENT':['PORT','FLIP'],'MOTION':['ON','OFF'],'BGSTYLE':['RAIN','BOTS','GIBSON','FLYBY','BOUNCE'],
 'BGCOL':['MULTI','GREEN','CYAN','PURPLE','AMBER']}
BOARD=(0,0,0); BODY=(0x18,0x18,0x18); SEGOFF=(0x21,0x0c,0x31); CAP=(0x73,0xdb,0x08)
def fill_board(im,r):
    p=im.load(); x0,y0,x1,y1=r
    for y in range(y0,y1+1):
        for x in range(x0,x1+1): p[x,y]=BOARD
    # continue straight traces that enter/leave the removed area on opposite sides
    for x in range(x0,x1+1):
        a=p[x,y0-1]; b=p[x,y1+1]
        if a==b and sum(a)>40 and a not in ((255,255,255),):
            for y in range(y0,y1+1): p[x,y]=a
    for y in range(y0,y1+1):
        a=p[x0-1,y]; b=p[x1+1,y]
        if a==b and sum(a)>40 and a not in ((255,255,255),):
            for x in range(x0,x1+1): p[x,y]=a
def seg_meter(d,x0,y0,lit,col,dimc):
    # 12 segments, 5px wide, 1px gap, starting x0 (39), rows y0..y0+4
    for i in range(12):
        sx=x0+i*6; d.rectangle((sx,y0,sx+4,y0+4),fill=col[i] if lit[i] else dimc)
def wifi(d,cx,cy,r,c):
    for k,rr in enumerate((r,r*2//3,r//3)):
        d.arc((cx-rr,cy-rr,cx+rr,cy+rr),225,315,fill=c,width=2)
    d.rectangle((cx-1,cy-1,cx,cy),fill=c)
def ble(d,cx,cy,h,c):
    w=h//3
    pts=[(cx-w,cy-h//4),(cx+w,cy+h//4),(cx,cy+h//2),(cx,cy-h//2),(cx+w,cy-h//4),(cx-w,cy+h//4)]
    d.line(pts,fill=c,width=1)
def bot(d,x,y,c,s=1):
    d.rectangle((x,y,x+7*s,y+6*s),fill=c); d.rectangle((x+2*s,y+2*s,x+2*s+s-1,y+3*s),fill=(0,0,0)); d.rectangle((x+5*s,y+2*s,x+5*s+s-1,y+3*s),fill=(0,0,0))
def tile(k,ch,t,plate,bgA=False):
    """picture for chip k showing choice ch; returns (img, origin)"""
    x0,y0,x1,y1=ART[k]; W,H=x1-x0+1,y1-y0+1; acc=rgb(ACC[t])
    base=BODY if k in('RAINCOL','ORIENT') else BOARD
    if plate and k in UI and k not in('THEME','MOTION'): base=sc(acc,0.30)
    if plate and k=='MOTION': base=sc(acc,0.30)
    im=Image.new('RGB',(W,H),base); d=ImageDraw.Draw(im)
    if k=='THEME':
        im=Image.new('RGB',(W,H),BOARD); d=ImageDraw.Draw(im)
        seg_meter(d,0,0,[i==ch for i in range(12)],[acc]*12,sc(acc,0.22) if plate else SEGOFF)
    elif k=='FOCUS':
        im=Image.new('RGB',(W,H),BOARD); d=ImageDraw.Draw(im)
        cols=[(0xde,0x92,0xff)]*3+[(0,0xdb,0xff)]*3+[(0x34,0x5e,0xfb)]*3+[(0xff,0x30,0x30)]*3
        lit=[True]*12 if ch==0 else [i//3==ch for i in range(12)]
        if ch==0: cols=[(0xde,0x92,0xff)]*12
        seg_meter(d,0,0,lit,cols,SEGOFF)
    elif k=='MIX':
        if ch==0: wifi(d,8,17,9,(0,0xdb,0xff)); ble(d,19,13,14,(0x34,0x9e,0xff))
        elif ch==1: wifi(d,12,20,13,(0,0xdb,0xff))
        else: ble(d,12,12,20,(0x34,0x9e,0xff))
    elif k=='RAINCOL':
        # keep "CP" lettering row, recolour the bar; THEME = accent, (A) MULTI = 4 stripes
        src=ld(t).crop((x0,y0,x1,y1+1)); im.paste(src.crop((0,0,W,12)),(0,0))
        bar=(4,13,17,17)
        if bgA:
            cs=[None,(0x20,0xff,0x40),(0,0xff,0xff),(0xb3,0x24,0xfd),(0xff,0xb0,0x00)][ch]
            if cs is None:
                for i,c in enumerate([(0x20,0xff,0x40),(0,0xff,0xff),(0xb3,0x24,0xfd),(0xff,0xb0,0x00)]):
                    d.rectangle((4+i*4,13,7+i*4,17),fill=c)
            else: d.rectangle(bar,fill=cs)
        else:
            d.rectangle(bar,fill=[(0x20,0xff,0x40),(0,0xff,0xff),acc][ch])
        if plate: # plate = tint of the grey die only around the bar
            pass
    elif k=='MASHUP':
        g=(0x73,0xdb,0x08); bot(d,1,7,g,2)
        for i in range(ch+1):
            x=18+i*3 if ch<2 else 17+i*3; d.line([(x,9),(x+3,13),(x,17)],fill=g,width=1)
        for i in range(ch):   # speed lines under the bot
            d.line([(1,23-i*0),(1+5*(ch+1),23)],fill=sc(g,0.6)) if i==0 else d.line([(4,25),(4+4*(ch+1),25)],fill=sc(g,0.6))
    elif k=='RAIN':
        gc=(0x20,0xff,0x40)
        if bgA:
            if ch==0:
                for x,y in ((6,4),(12,8),(18,3),(9,14),(16,15)): d.line([(x,y),(x,y+5)],fill=gc)
            elif ch==1:
                bot(d,2,4,(0xff,0x24,0xfd)); bot(d,14,9,(0,0xff,0xff)); bot(d,6,16,(0xff,0xda,0))
            elif ch==2:
                for x in range(0,26,5): d.line([(13,9),(x,25)],fill=(0,0xdb,0xff))
                for y in (13,17,22): d.line([(0,y),(25,y)],fill=(0,0x8e,0xad))
            elif ch==3:
                bot(d,13,5,(0xff,0xda,0)); 
                for i,y in enumerate((7,9,11)): d.line([(2+i,y),(11,y)],fill=sc((0xff,0xda,0),0.6))
            else:
                for i,(x,y) in enumerate(((2,20),(6,12),(10,7),(14,6),(18,9))): d.point((x,y),fill=sc((0,0xff,0xff),0.5))
                d.ellipse((19,15,24,20),fill=(0,0xff,0xff))
        else:
            if ch==0:
                for x,y in ((5,3),(10,9),(15,2),(20,8),(7,15),(17,16)): d.line([(x,y),(x,y+5)],fill=gc)
            else:
                d.line([(3,20),(22,20)],fill=(0x5a,0x61,0x4a))
                d.line([(6,4),(19,17)],fill=(0xff,0x30,0x48),width=2); d.line([(19,4),(6,17)],fill=(0xff,0x30,0x48),width=2)
    elif k=='RMODE':
        # 3-position slide switch replacing the yellow connector; knob position = mode, knob glyph = motion
        y=(0xff,0xdb,0); d.rectangle((0,3,22,7),fill=(0xb5,0x92,0)); 
        for i in range(3): d.point((3+i*8,5),fill=(0,0,0))
        kx=ch*8; d.rectangle((kx,0,kx+6,10),fill=y)
        if ch==0: d.line([(kx+1,7),(kx+5,3)],fill=(0,0,0))
        elif ch==1: d.line([(kx+3,1),(kx+3,9)],fill=(0,0,0))
        else: d.rectangle((kx+2,4,kx+4,6),fill=(0,0,0))
    elif k=='MULTI':
        c=(0xff,0xda,0); 
        d.ellipse((2,2,23,23),outline=sc(c,0.8))
        if ch==0: d.line([(5,20),(20,5)],fill=(0xff,0x30,0x48),width=2)
        elif ch==1: d.pieslice((4,4,21,21),-90,90,fill=c)
        else: d.ellipse((4,4,21,21),fill=c)
        d.line([(12,12),(12,5)],fill=(0,0,0))
    elif k=='ORIENT':
        im=ld(t).crop((x0,y0,x1+1,y1+1)); d=ImageDraw.Draw(im)
        d.rectangle((11,1,51,12),fill=sc(acc,0.30) if plate else BODY)   # track (old pill+knob area)
        kx=12 if ch==0 else 25; cy=(0,0xdb,0xff)
        d.rectangle((kx,2,kx+25,11),fill=cy)
        # tiny phone glyph: notch up (PORT) or down (FLIP)
        px=kx+10; d.rectangle((px,3,px+5,10),fill=(0,0x40,0x50))
        if ch==0: d.rectangle((px+2,4,px+3,4),fill=(255,255,255))
        else: d.rectangle((px+2,9,px+3,9),fill=(255,255,255))
    elif k=='MOTION':
        src=ld(t).crop((x0,y0,x1+1,y0+6)); 
        on=ch==0; lab=(0xff,0xff,0xff) if on else (0x50,0x50,0x50)
        sp=src.load()
        for yy in range(6):
            for xx in range(W):
                if sum(sp[xx,yy])>200: im.putpixel((xx,yy),lab)
        if on:
            for xx in range(W): im.putpixel((xx,9+int(round(1.5*math.sin(xx/2.2)))),(0,0xdb,0xff))
        else:
            d.line([(0,9),(W-1,9)],fill=(0x40,0x40,0x40))
    if k not in UI:
        im=Image.eval(im,lambda v:int(v*0.6))
    return im
def compose(t,st,plate,bgA=False):
    im=ld(t).copy()
    for r in REMOVED.values(): fill_board(im,r)
    for k in ART:
        kk={'RAIN':'BGSTYLE' if bgA else 'RAIN','RAINCOL':'BGCOL' if bgA else 'RAINCOL'}.get(k,k)
        ch=t if k=='THEME' else st.get(k,0)
        if bgA and k in('RAIN','RAINCOL'): ch=st.get(kk,0)
        im.paste(tile(k,ch,t,plate,bgA),ART[k][:2])
    return im

KEEP={'ticker':(0,0,239,20),'uptime':(35,51,113,65),'bottom-bar+tty+back':(0,256,239,319)}
