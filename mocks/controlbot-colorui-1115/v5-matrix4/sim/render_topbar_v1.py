#!/usr/bin/env python3
"""Top-bar v1: SECURITYbot header goes BLACK + LIME (fixed across every theme).
Five variants rendered in-place on the v9c CONTROLbot panel (SMITH hero still):
  M1 MATRIX RAIN, M2 MATRIX DECODE, M3 MATRIX TICKER, S1 SECURITY HAZARD, S2 SECURITY HUD.
Only header rows y1..23, x1..238 change (inside the gold rim). Everything else is v9c pixel-identical.
Smooth AA text = supersampled x4 coverage (LovyanGFX smooth/VLW-style alpha), quantised to RGB565."""
import sys
sys.dont_write_bytecode=True
import json,hashlib,subprocess,shutil,math,random
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from scipy import ndimage

ROOT=Path(__file__).resolve().parents[1]
W=Path('/tmp/secbot_build/topbar_v1')
OUT=ROOT/'mocks/topbar-v1-matrix-security'
V9C=ROOT/'mocks/controlbot-bigchip-v9c-glasses/01_panel@1x.png'
HY0,HY1,HX0,HX1=1,24,1,239          # header region, exclusive ends -> 23 rows x 238 cols
H,WD=HY1-HY0,HX1-HX0
SS=4
LIME=(120,252,0)                     # 0x7FFF00, RGB565-exact; same lime as security-color-themes-v1
WHITE=(248,252,248)                  # RGB565 white
GF='/usr/share/fonts/truetype/sand-box/google/'
FONTS={
 'jb':(GF+'JetBrains Mono/JetBrainsMono-VariableFont_wght.ttf','ExtraBold'),
 'jbb':(GF+'JetBrains Mono/JetBrainsMono-VariableFont_wght.ttf','Bold'),
 'chakra':(GF+'Chakra Petch/ChakraPetch-Bold.ttf',None),
 'raj':(GF+'Rajdhani/Rajdhani-Bold.ttf',None),
 'kana':('/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc',None),
}
TITLE='SECURITYbot EX.DATA'
TICKER='SECURITYbot // PERIMETER ARMED // 4 NODES ONLINE // EX.DATA // '
KANA='ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝ'
DIGS='0123457891Z:=*+<>'
NFR=105                              # 3.5 s @ 30 fps
VARIANTS=[('M1','matrix_rain','MATRIX RAIN'),('M2','matrix_decode','MATRIX DECODE'),('M3','matrix_ticker','MATRIX TICKER'),
          ('S1','security_hazard','SECURITY HAZARD'),('S2','security_hud','SECURITY HUD')]

_fc={}
def font(key,px):
 k=(key,px)
 if k not in _fc:
  p,var=FONTS[key]
  f=ImageFont.truetype(p,int(round(px*SS)),index=0) if p.endswith('.ttc') else ImageFont.truetype(p,int(round(px*SS)))
  if var:f.set_variation_by_name(var)
  _fc[k]=f
 return _fc[k]

def text_alpha(s,key,px,x,baseline,shape=(H,WD)):
 """Coverage (0..1) at 1x for text s, left edge x, baseline (1x header coords). Float positions OK (1/SS)."""
 L=Image.new('L',(shape[1]*SS,shape[0]*SS),0)
 ImageDraw.Draw(L).text((x*SS,baseline*SS),s,fill=255,font=font(key,px),anchor='ls')
 return np.asarray(L.reduce(SS),np.float64)/255.0

def text_alpha_unclipped(s,key,px,x,baseline):
 """Same text on a padded canvas: used to prove nothing was clipped by the header bounds."""
 P=40
 a=text_alpha(s,key,px,x+P,baseline+P,shape=(H+2*P,WD+2*P))
 return a,P

def text_w(s,key,px): return font(key,px).getlength(s)/SS
def ink_box(a,th=0.02):
 ys,xs=np.where(a>th)
 return (int(xs.min()),int(ys.min()),int(xs.max()),int(ys.max())) if len(xs) else None

def baseline_for_center(s,key,px,top,bot):
 """Baseline so the ink of s is vertically centred in rows [top,bot] (inclusive)."""
 b=font(key,px).getbbox(s,anchor='ls');t,bt=b[1]/SS,b[3]/SS
 return (top+bot+1)/2-(t+bt)/2

def comp(img,a,col):
 a=np.clip(a,0,1)[...,None];img[:]=img*(1-a)+np.array(col,np.float64)*a

def mix(c1,c2,t): return tuple(c1[i]*(1-t)+c2[i]*t for i in range(3))

def to565(im):
 im=np.clip(np.round(im),0,255).astype(np.uint8)
 r=(im[...,0]>>3)<<3;g=(im[...,1]>>2)<<2;b=(im[...,2]>>3)<<3
 return np.stack([r,g,b],-1).astype(np.uint8)

def hline(img,y,x0,x1,col,a=1.0):
 m=np.zeros((H,WD));m[y,x0:x1+1]=a;comp(img,m,col)

def disc(cx,cy,r):
 yy,xx=np.mgrid[0:H*SS,0:WD*SS]
 d=np.hypot((xx+0.5)/SS-cx,(yy+0.5)/SS-cy)
 m=(d<=r).astype(np.float64)
 return m.reshape(H,SS,WD,SS).mean(axis=(1,3))

def rrect(x0,y0,x1,y1,r=1.5):
 """AA rounded rect coverage, x1/y1 exclusive edges in 1x px."""
 yy,xx=np.mgrid[0:H*SS,0:WD*SS]
 px=(xx+0.5)/SS;py=(yy+0.5)/SS
 cx=np.clip(px,x0+r,x1-r);cy=np.clip(py,y0+r,y1-r)
 m=(np.hypot(px-cx,py-cy)<=r)&(px>=x0)&(px<=x1)&(py>=y0)&(py<=y1)
 return m.astype(np.float64).reshape(H,SS,WD,SS).mean(axis=(1,3))

# ---------------- kana sprites (mirrored, like the film) ----------------
_spr={}
def glyph_sprite(ch,cell_w,cell_h,px,mirror=True,key='kana'):
 """Coverage sprite cell_h x cell_w at 1x, glyph centred, hard-clipped to its own cell."""
 k=(ch,cell_w,cell_h,px,mirror,key)
 if k in _spr:return _spr[k]
 f=font(key,px)
 L=Image.new('L',(cell_w*SS,cell_h*SS),0)
 bb=f.getbbox(ch,anchor='ls')
 gw,gh=bb[2]-bb[0],bb[3]-bb[1]
 x=(cell_w*SS-gw)/2-bb[0];y=(cell_h*SS-gh)/2-bb[1]
 ImageDraw.Draw(L).text((x,y),ch,fill=255,font=f,anchor='ls')
 if mirror:L=L.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
 a=np.asarray(L.reduce(SS),np.float64)/255.0
 _spr[k]=a;return a

def blit(acc,spr,x,y,alpha):
 h,w=spr.shape
 y0,x0=max(0,y),max(0,x);y1,x1=min(acc.shape[0],y+h),min(acc.shape[1],x+w)
 if y1<=y0 or x1<=x0:return
 acc[y0:y1,x0:x1]=np.maximum(acc[y0:y1,x0:x1],spr[y0-y:y1-y,x0-x:x1-x]*alpha)

# ---------------- shared matrix frame ----------------
MT_TITLE_PX=19.5
M1_TITLE_PX=15.5
def matrix_title_geom(px=MT_TITLE_PX):
 tw=text_w(TITLE,'jb',px)
 x=(WD-tw)/2
 x=round(x*SS)/SS
 bl=round(baseline_for_center(TITLE,'jb',px,1,20)*SS)/SS
 return x,bl,tw

def matrix_rules(img):
 hline(img,0,0,WD-1,LIME,0.35)     # top hairline
 hline(img,21,0,WD-1,LIME,1.0)     # bottom rule
 hline(img,22,0,WD-1,LIME,0.30)

# ---------------- M1 rain ----------------
RAIN_CW,RAIN_CH,RAIN_PX=6,8,8.5
def rain_cols():
 rnd=random.Random(1999)
 cols=[]
 for i,x in enumerate(range(1,WD-RAIN_CW+1,RAIN_CW)):
  cols.append(dict(x=x,speed=rnd.uniform(0.35,0.9),phase=rnd.uniform(0,60),trail=rnd.randint(2,4),
                   glyphs=[rnd.choice(KANA+DIGS) for _ in range(40)],flip=rnd.random()))
 return cols
RAIN=rain_cols()
def rain_layer(t):
 """Returns (lime_alpha, white_alpha) at 1x across the content band rows 1..20."""
 la=np.zeros((H,WD));wa=np.zeros((H,WD))
 period=20+RAIN_CH*4+6                          # short loop + 2 drops per column -> dense but faint
 for c in RAIN:
  for drop in (0.0,period/2):
   head=(c['phase']+drop+c['speed']*t)%period-RAIN_CH   # head glyph top y (float)
   draw_trail(la,wa,c,head,t)
 la[0]=0;la[21:]=0;wa[0]=0;wa[21:]=0     # rain lives only in content band rows 1..20
 return la,wa

def draw_trail(la,wa,c,head,t):
  for k in range(c['trail']+1):
   gy=head-k*RAIN_CH
   yi=int(math.floor(gy))
   if yi+RAIN_CH<1 or yi>20:continue
   gi=(k+int(t*c['flip']*0.25)+int(c['phase']))%len(c['glyphs'])   # occasional glyph swaps
   ch=c['glyphs'][gi]
   key='kana' if ch in KANA else 'jbb'
   spr=glyph_sprite(ch,RAIN_CW,RAIN_CH,RAIN_PX if key=='kana' else 7.5,mirror=(key=='kana'),key=key)
   if k==0:
    blit(la,spr,c['x'],yi,0.70);blit(wa,spr,c['x'],yi,0.55)
   else:
    blit(la,spr,c['x'],yi,0.55*(0.72**(k-1)))

def halo_factor(title_a,r_full=2.0,r_soft=1.5):
 d=ndimage.distance_transform_edt(title_a<=0)
 return np.clip((d-r_full)/r_soft,0,1)

def render_M1(t):
 img=np.zeros((H,WD,3))
 x,bl,_=matrix_title_geom(M1_TITLE_PX)
 ta=text_alpha(TITLE,'jb',M1_TITLE_PX,x,bl)
 hf=halo_factor(ta)
 # behind the title's span the rain is dimmed to 25% so inter-letter fragments never read as glyphs (e.g. '.'->':')
 bx=ink_box(ta);xs=np.arange(WD)+0.5
 span=np.clip(np.minimum(xs-(bx[0]-3),(bx[2]+4)-xs)/3,0,1)
 hf=hf*(1-0.75*span[None,:])
 la,wa=rain_layer(t)
 comp(img,la*hf,LIME);comp(img,wa*hf,WHITE)
 matrix_rules(img)
 comp(img,ta,LIME)
 spec={'elements':[{'kind':'title','text':TITLE,'font':'jb','px':M1_TITLE_PX,'x':x,'baseline':bl,'color':'lime'}],
       'rules':[0,21,22],'bg_effect_rows':[1,20],'bg_effect':'katakana rain, halo-cleared 2px around title'}
 return img,spec

# ---------------- M2 decode ----------------
def decode_cells():
 x,bl,tw=matrix_title_geom()
 adv=text_w('M','jb',MT_TITLE_PX)
 return x,bl,adv
DEC_START,DEC_END=8,74       # lead glyph walks across 19 cells between these frames
def render_M2(t):
 img=np.zeros((H,WD,3))
 x,bl,adv=decode_cells()
 matrix_rules(img)
 n=len(TITLE)
 lead=(t-DEC_START)/(DEC_END-DEC_START)*n
 rnd=random.Random(t*7919+13)
 cell_top=int(math.floor(bl-15.5));cell_h=17
 lime_a=np.zeros((H,WD));white_a=np.zeros((H,WD))
 for i,ch in enumerate(TITLE):
  cx=x+i*adv
  if ch==' ':continue
  if i<int(lead):                       # locked: final lime glyph
   lime_a=np.maximum(lime_a,text_alpha(ch,'jb',MT_TITLE_PX,cx,bl))
  elif i==int(lead) and lead>=0:          # leading glyph: bright white, the final char
   white_a=np.maximum(white_a,text_alpha(ch,'jb',MT_TITLE_PX,cx,bl))
  else:                                   # scrambled: dim lime random kana/digit, fades in 0..8
   g=rnd.choice(KANA+DIGS)
   key='kana' if g in KANA else 'jbb'
   cw=int(math.floor(adv))-1
   spr=glyph_sprite(g,cw,cell_h,17 if key=='kana' else 15,mirror=(key=='kana'),key=key)
   fade=min(1.0,(t+1)/8)
   blit(lime_a,spr,int(round(cx)),cell_top,0.55*fade)
 comp(img,lime_a,LIME);comp(img,white_a,WHITE)
 done=lead>=n
 spec={'elements':[{'kind':'title','text':TITLE,'font':'jb','px':MT_TITLE_PX,'x':x,'baseline':bl,'color':'lime'}],
       'rules':[0,21,22],'decode_done':bool(done),'lead_index':int(lead) if lead>=0 else -1}
 return img,spec

# ---------------- M3 ticker ----------------
TAG_PX=10.0;TICK_PX=10.5
TAG_BOX=(2,2,74,20)          # x0,y0,x1,y1 exclusive  (72x18)
WIN=(88,236)                 # ticker window x0,x1 exclusive
FADE=8
def ticker_offset(t):
 """px scrolled; t=0 (the still) centres '// EX.DATA' in the window."""
 pre=text_w(TICKER[:TICKER.index('EX.DATA')],'jbb',TICK_PX);ew=text_w('EX.DATA','jbb',TICK_PX)
 return WIN[0]+pre+ew/2-(WIN[0]+WIN[1])/2+2.0*t
def render_M3(t):
 img=np.zeros((H,WD,3))
 matrix_rules(img)
 x0,y0,x1,y1=TAG_BOX
 comp(img,rrect(x0,y0,x1,y1,1.5),LIME)
 tw=text_w('SECURITYbot','jb',TAG_PX)
 tx=round(((x0+x1)/2-tw/2)*SS)/SS
 tbl=round(baseline_for_center('SECURITYbot','jb',TAG_PX,y0,y1-1)*SS)/SS
 tag_a=text_alpha('SECURITYbot','jb',TAG_PX,tx,tbl)
 comp(img,tag_a,(0,0,0))
 # separator chevrons
 cv=text_alpha('»','jbb',TICK_PX,x1+3.0,baseline_for_center('»','jbb',TICK_PX,y0,y1-1))
 # ticker
 L=text_w(TICKER,'jbb',TICK_PX)
 off=ticker_offset(t)%L
 tbl2=round(baseline_for_center('SECURITYbot // PERIMETER','jbb',TICK_PX,y0,y1-1)*SS)/SS
 ta=np.zeros((H,WD))
 for rep in (-1,0,1,2):
  sx=WIN[0]-off+rep*L
  if sx>WIN[1] or sx+L<WIN[0]:continue
  ta=np.maximum(ta,text_alpha(TICKER,'jbb',TICK_PX,sx,tbl2))
 xs=np.arange(WD)+0.5
 win=np.clip(np.minimum(xs-WIN[0],WIN[1]-xs)/FADE,0,1)
 win[(xs<WIN[0])|(xs>WIN[1])]=0
 ta*=win[None,:]
 comp(img,ta,LIME);comp(img,cv,LIME)
 spec={'elements':[{'kind':'title','text':'SECURITYbot','font':'jb','px':TAG_PX,'x':tx,'baseline':tbl,'color':'black_on_lime','box':TAG_BOX},
                   {'kind':'ticker','text':TICKER,'font':'jbb','px':TICK_PX,'baseline':tbl2,'window':WIN,'offset':off,'fade_px':FADE}],
       'rules':[0,21,22]}
 return img,spec

# ---------------- S1 hazard ----------------
S1_TITLE_PX=15.0
CHIP=(193,3,235,15)          # ARMED chip exclusive
LED_C=(184.5,9.0);LED_R=3.0
def hazard(img,y0,y1,period=8.0):
 yy,xx=np.mgrid[0:H*SS,0:WD*SS]
 px=(xx+0.5)/SS;py=(yy+0.5)/SS
 m=(((px+py)%period)<period/2)&(py>=y0)&(py<y1)
 a=m.astype(np.float64).reshape(H,SS,WD,SS).mean(axis=(1,3))
 comp(img,a,LIME)
def render_S1(t):
 img=np.zeros((H,WD,3))
 hazard(img,17,23)
 x=5.0
 bl=round(baseline_for_center(TITLE,'chakra',S1_TITLE_PX,1,15)*SS)/SS
 ta=text_alpha(TITLE,'chakra',S1_TITLE_PX,x,bl)
 comp(img,ta,LIME)
 # blinking LED: on 0.6 s, off 0.4 s; glow ring when on
 on=(t%30)<18
 cx,cy=LED_C
 if on:
  glow=np.clip(disc(cx,cy,LED_R+1.6)-disc(cx,cy,LED_R),0,1)*0.30
  comp(img,glow,LIME);comp(img,disc(cx,cy,LED_R),LIME)
  comp(img,disc(cx-0.9,cy-0.9,1.0),WHITE)
 else:
  comp(img,disc(cx,cy,LED_R),LIME)
  comp(img,disc(cx,cy,LED_R-1.0),(0,0,0))
 x0,y0,x1,y1=CHIP
 comp(img,rrect(x0,y0,x1,y1,1.5),LIME)
 cw=text_w('ARMED','chakra',9.0)
 cx_=round(((x0+x1)/2-cw/2)*SS)/SS
 cbl=round(baseline_for_center('ARMED','chakra',9.0,y0,y1-1)*SS)/SS
 comp(img,text_alpha('ARMED','chakra',9.0,cx_,cbl),(0,0,0))
 spec={'elements':[{'kind':'title','text':TITLE,'font':'chakra','px':S1_TITLE_PX,'x':x,'baseline':bl,'color':'lime'},
                   {'kind':'chip','text':'ARMED','font':'chakra','px':9.0,'x':cx_,'baseline':cbl,'box':CHIP},
                   {'kind':'led','center':LED_C,'r':LED_R+1.6,'on':bool(on)}],
       'stripe_rows':[17,22]}
 return img,spec

# ---------------- S2 HUD ----------------
S2_TITLE_PX=15.0
def brackets(img,x0,y0,x1,y1,arm_x=7,arm_y=4):
 m=np.zeros((H,WD))
 for (cx,cy,dx,dy) in ((x0,y0,1,1),(x1,y0,-1,1),(x0,y1,1,-1),(x1,y1,-1,-1)):
  for i in range(arm_x):m[cy,cx+dx*i]=1
  for i in range(arm_y):m[cy+dy*i,cx]=1
 comp(img,m,LIME)
TICKS=(12,226)
def render_S2(t):
 img=np.zeros((H,WD,3))
 # faint scanlines (every other row) + moving scan band
 sl=np.zeros((H,WD));sl[1:21:2,:]=0.07
 band_y=1+((t*0.6)%22)
 yy=np.arange(H)[:,None]+0.5
 band=np.clip(1-np.abs(yy-band_y)/1.6,0,1)*0.16*np.ones((1,WD))
 band[0]=0;band[21:]=0
 comp(img,sl,LIME);comp(img,band,LIME)
 brackets(img,1,1,WD-2,H-2)
 # perimeter ticks on the bottom edge (minor 1px every 6, major 3px every 30)
 tk=np.zeros((H,WD))
 for i,x in enumerate(range(TICKS[0],TICKS[1]+1,6)):
  ln=3 if (x-TICKS[0])%30==0 else 1
  tk[21-ln+1:22,x]=0.85 if ln==3 else 0.55
 comp(img,tk,LIME)
 x=11.0
 bl=round(baseline_for_center(TITLE,'raj',S2_TITLE_PX,2,16)*SS)/SS
 ta=text_alpha(TITLE,'raj',S2_TITLE_PX,x,bl)
 # clear scan texture right under/around title so it stays crisp
 hf=halo_factor(ta,1.0,1.0)
 img[:]=img*hf[...,None]
 comp(img,ta,LIME)
 # right block: REC dot + REC + time
 tstr='08:13:%02d'%(7+int(t//30))
 # layout fixed from the reference string so REC/dot never jitter as digits change; time right-aligned
 tw0=text_w('08:13:07','raj',12.0);rw=text_w('REC','raj',12.0)
 rx=WD-12-tw0-5-rw
 tx=rx+rw+5          # time left-aligned at a fixed gap after REC
 dot=(rx-6.0,9.0)
 rbl=round(baseline_for_center('REC 08:13:07','raj',12.0,2,16)*SS)/SS
 tx=round(tx*SS)/SS;rx=round(rx*SS)/SS
 on=(t%30)<20
 ra=text_alpha('REC','raj',12.0,rx,rbl);tia=text_alpha(tstr,'raj',12.0,tx,rbl)
 for a in (ra,tia):
  img[:]=img*halo_factor(a,1.0,1.0)[...,None]
 if on:comp(img,disc(dot[0],dot[1],2.6),LIME)
 else:
  comp(img,disc(dot[0],dot[1],2.6),LIME);comp(img,disc(dot[0],dot[1],1.6),(0,0,0))
 comp(img,ra,LIME);comp(img,tia,LIME)
 spec={'elements':[{'kind':'title','text':TITLE,'font':'raj','px':S2_TITLE_PX,'x':x,'baseline':bl,'color':'lime'},
                   {'kind':'text','text':'REC','font':'raj','px':12.0,'x':rx,'baseline':rbl},
                   {'kind':'text','text':tstr,'font':'raj','px':12.0,'x':tx,'baseline':rbl,'note':'mock clock EX.DATA'},
                   {'kind':'led','center':dot,'r':2.6,'on':bool(on)}],
       'brackets':True,'ticks':TICKS,'bg_effect':'scanlines rows 1..20 (alpha<=0.23), halo-cleared 1px around text'}
 return img,spec

RENDER={'M1':render_M1,'M2':render_M2,'M3':render_M3,'S1':render_S1,'S2':render_S2}

def header(vid,t):
 img,spec=RENDER[vid](t)
 return to565(img),spec

def compose(base,hdr):
 out=base.copy();out[HY0:HY1,HX0:HX1]=hdr;return out

def smooth(a):return np.array(Image.fromarray(a).resize((a.shape[1]*3,a.shape[0]*3),Image.Resampling.LANCZOS))

def encode_smooth(frames,mp4):
 raw=W/'clip.rgb'
 with open(raw,'wb') as f:
  for fr in frames:f.write(smooth(fr).tobytes())
 subprocess.run(['ffmpeg','-v','error','-y','-f','rawvideo','-pixel_format','rgb24','-video_size','720x960','-framerate','30','-i',str(raw),
  '-c:v','libx264','-preset','slow','-crf','14','-pix_fmt','yuv420p',
  '-colorspace','bt709','-color_primaries','bt709','-color_trc','bt709',
  '-x264-params','colorprim=bt709:transfer=bt709:colormatrix=bt709',
  '-movflags','+faststart',str(mp4)],check=True)
 raw.unlink()

STILL_T={'M1':37,'M2':NFR-1,'M3':0,'S1':0,'S2':12}
README='''# SECURITYbot top bar v1 — BLACK + LIME, 3 Matrix + 2 Security styles

Mobandy 2026-11-09 08:12-08:13 ET: top bar becomes black + lime green, fixed in every theme; "more matrix themed";
"three different ui of the top bar with a matrix style and two different security styles".

Every variant is rendered in place on the v9c CONTROLbot panel (`controlbot-bigchip-v9c-glasses/01_panel@1x.png`, SMITH hero).
Only the header region (rows y1..23, x1..238, inside the gold rim) changes; every other pixel is v9c-identical (gated).
Row 24 (v9c red->blue gradient hairline under the bar) and the gold rim are untouched, same as security-color-themes-v1.

Lime = 0x7FFF00 / RGB565 (120,252,0), same lime as security-color-themes-v1. Header palette = black, lime, white + AA blends only.
Text is smooth anti-aliased (x4 supersampled coverage, LovyanGFX smooth/VLW style), quantised to RGB565.

| id | file prefix | look |
|----|-------------|------|
| M1 | 01_M1_matrix_rain | faint mirrored katakana rain (white-hot heads) falling behind crisp lime title; 2px black halo keeps title legible |
| M2 | 02_M2_matrix_decode | title decodes from scrambled lime kana/digits, bright-white leading glyph walking left->right; still = resolved title |
| M3 | 03_M3_matrix_ticker | fixed lime `SECURITYbot` tag, `»`, right->left lime announcement ticker with edge fades |
| S1 | 04_S1_security_hazard | lime title, blinking lime status LED + ARMED chip, lime/black diagonal hazard stripe beneath |
| S2 | 05_S2_security_hud | HUD bracket corners, faint scanlines + sweeping scan band, perimeter ticks, blinking lime REC dot + mock time |

Fonts (VLW candidates): M1/M2 JetBrains Mono ExtraBold 19.5px, M3 JetBrains Mono, S1 Chakra Petch Bold 14px, S2 Rajdhani Bold 16px,
rain/scramble glyphs Noto Sans CJK half-width katakana (mirrored). All numbers are mock EX.DATA.

Per variant: `@1x.png`, `@3x_smooth.png` (Lanczos of @1x), `_header@3x_smooth.png` (rows 0..24 crop of the smooth frame), `@3x.mp4`
(720x960 30fps H.264 yuv420p crf14 BT.709 faststart, 3.5 s). M2 also has `_midframe@3x_smooth.png`.
`00_topbar_compare_5up@3x_smooth.png` stacks the five header crops labelled M1-M3, S1-S2.

Render: `python3 sim/render_topbar_v1.py`  Verify: `python3 sim/verify_topbar_v1.py`  (scratch /tmp/secbot_build/topbar_v1). Mock only; not firmware.
'''

def label_strip(txt,w,h=30):
 im=Image.new('RGB',(w,h),(18,18,18));d=ImageDraw.Draw(im)
 f=ImageFont.truetype(FONTS['jbb'][0],18);f.set_variation_by_name('Bold')
 d.text((10,h//2),txt,fill=(220,220,220),font=f,anchor='lm')
 return np.asarray(im)

def main():
 W.mkdir(parents=True,exist_ok=True)
 if OUT.exists():
  # only ever our own folder; refuse to clobber anything else
  assert OUT.name=='topbar-v1-matrix-security'
  shutil.rmtree(OUT)
 OUT.mkdir(parents=True)
 base=np.array(Image.open(V9C).convert('RGB'))
 np.save(W/'v9c_base.npy',base)
 specs={};crops=[]
 for i,(vid,slug,name) in enumerate(VARIANTS,1):
  pre=f'{i:02d}_{vid}_{slug}'
  frames=[];fspecs=[]
  for t in range(NFR):
   h,s=header(vid,t);frames.append(compose(base,h));fspecs.append(s)
  frames=np.array(frames);np.save(W/f'{vid}_frames.npy',frames)
  st=STILL_T[vid];still=frames[st]
  Image.fromarray(still).save(OUT/f'{pre}@1x.png')
  sm=smooth(still);Image.fromarray(sm).save(OUT/f'{pre}@3x_smooth.png')
  crop=sm[0:75];Image.fromarray(crop).save(OUT/f'{pre}_header@3x_smooth.png');crops.append((vid,name,crop))
  if vid=='M2':
   Image.fromarray(smooth(frames[40])).save(OUT/f'{pre}_midframe@3x_smooth.png')
  encode_smooth(frames,OUT/f'{pre}@3x.mp4')
  specs[vid]={'name':name,'prefix':pre,'still_frame':st,'still_spec':fspecs[st],'frames':NFR,
              'frame_specs':[{'t':t,**({'decode_done':s.get('decode_done')} if vid=='M2' else {})} for t,s in enumerate(fspecs)]}
 parts=[]
 for vid,name,crop in crops:
  parts.append(label_strip(f'{vid}  {name}',crop.shape[1]));parts.append(crop);parts.append(np.full((12,crop.shape[1],3),40,np.uint8))
 Image.fromarray(np.concatenate(parts[:-1],0)).save(OUT/'00_topbar_compare_5up@3x_smooth.png')
 (OUT/'TOPBAR_SPEC.json').write_text(json.dumps({'header_region_exclusive':{'y':[HY0,HY1],'x':[HX0,HX1]},'lime':LIME,'white':WHITE,
   'base':str(V9C.relative_to(ROOT)),'base_commit':'a15182a','variants':specs},indent=1,default=float)+'\n')
 (OUT/'README.md').write_text(README)
 subprocess.run([sys.executable,str(ROOT/'sim/verify_topbar_v1.py')],check=True)

if __name__=='__main__':main()
