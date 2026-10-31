from mock import *
import os
O='/workspace/out/ctl-pictures'; os.makedirs(O,exist_ok=True)
ROWS=[('THEME 31x9  [UI: brighter plate]',[(TNAME[t],theme_chip(t)) for t in range(12)]),
 ('FOCUS 31x9  [non-UI: dim]',[(n,focus_chip(i)) for i,n in enumerate(['ALL','WIFI','BLE','ALERTS'])]),
 ('MIX 34x14  [non-UI: dim]',[(n,mix_chip(i)) for i,n in enumerate(['ALL','WIFI','BLE'])]),
 ('RAIN COL 34x14  [UI]',[(n,col_chip(c)) for n,c in (('GREEN',0x20ff40),('CYAN',0x00ffff),('THEME(GRAPE)',ACC[0]))]),
 ('MASHUP (ticker speed) 34x14  [UI]',[(n,speed_chip(i)) for i,n in enumerate(['SLOW','MED','FAST'])]),
 ('RAIN 39x14  [UI]',[('ON',rain_chip(1)),('OFF',rain_chip(0))]),
 ('RAIN MODE 39x14  [UI]',[(n,rmode_chip(i)) for i,n in enumerate(['DRIFT','SWEEP','STILL'])]),
 ('MULTI auto re-scan 26x14 (+ LED kept)  [non-UI]',[(n,multi_chip(i)) for i,n in enumerate(['OFF','30S','60S'])]),
 ('ORIENT 71x15  [UI]',[('PORT',orient_chip(0)),('FLIP',orient_chip(1))]),
 ('BG MOTION 21x7  [UI]',[('ON',motion_chip(1)),('OFF',motion_chip(0))]),
 ('(A) BG STYLE in RAIN rect 39x14  [UI, proposal]',[(n,bgstyle_chip(i)) for i,n in enumerate(['RAIN','BOTS','GIBSON','FLYBY','BOUNCE'])]),
 ('(A) BG COLOR in RAIN COL rect 34x14  [UI, proposal]',[(n,bgcol_chip(i)) for i,n in enumerate(['MULTI','GREEN','CYAN','PURPLE','AMBER'])]),
]
def chipsheet(S,out):
    tw=max(c.size[0] for _,r in ROWS for _,c in r)*S+30; th=max(c.size[1] for _,r in ROWS for _,c in r)*S+40
    W=40+12*tw; H=80+len(ROWS)*(th+40)
    im=Image.new('RGB',(W,H),(24,24,24)); d=ImageDraw.Draw(im)
    d.text((20,20),f'CONTROLbot picture chips, every state, {S}x (mock, not firmware). UI-changing = tinted plate + full-bright art; non-UI = grey plate, dimmed art. No white selection border.',fill=W_,font=Fs)
    y=70
    for name,row in ROWS:
        d.text((20,y),name,fill=(255,220,120),font=Fs); y+=30
        for i,(n,c) in enumerate(row):
            x=20+i*tw; im.paste(c.resize((c.size[0]*S,c.size[1]*S),Image.NEAREST),(x,y)); d.text((x,y+c.size[1]*S+2),n,fill=(200,200,200),font=Fs)
        y+=th+10
    im.save(out); return im
W_=(255,255,255)
chipsheet(3,O+'/chips_all_states_3x.png'); chipsheet(6,'/workspace/scratch-p1/cp/chips6.png')
def lab(img,t):
    c=Image.new('RGB',(720,1010),(24,24,24)); c.paste(img.resize((720,960),Image.NEAREST),(0,50)); ImageDraw.Draw(c).text((10,8),t,fill=W_,font=F); return c
def grid(items,cols,out,title):
    w,h=740,1030; im=Image.new('RGB',(20+cols*w,80+((len(items)+cols-1)//cols)*h),(24,24,24)); ImageDraw.Draw(im).text((20,20),title,fill=W_,font=F)
    for i,c in enumerate(items): im.paste(c,(20+(i%cols)*w,80+(i//cols)*h))
    im.save(out); return im
ba=grid([lab(ld(0),'BEFORE (shipping): text value rects'),lab(compose(0,{}),'AFTER: picture per choice (defaults)'),
         lab(compose(0,{'focus':2,'mix':1,'rcol':1,'speed':2,'rain':0,'rmode':1,'auto':2,'flip':1,'motion':0}),'AFTER: every chip on its 2nd/3rd choice'),
         lab(compose(0,{'bgs':2,'bgc':3},True),'AFTER (A): RAIN->BG STYLE GIBSON, RAIN COL->BG PURPLE')],4,O+'/ctl_before_after_3x.png','CONTROLbot: before / after (sim frame + mock chips)')
th=grid([lab(compose(t,{}),f'theme {t}: {TNAME[t]}') for t in range(12)],4,O+'/ctl_theme_chip_all_themes_3x.png','THEME chip: one picture per theme, in place (12 themes)')
ba.resize((ba.width//3,ba.height//3)).save('ba_prev.png'); th.resize((th.width//4,th.height//4)).save('th_prev.png')
