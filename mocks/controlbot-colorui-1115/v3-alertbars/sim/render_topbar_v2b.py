#!/usr/bin/env python3
"""Top-bar v2b COLORS (Mobandy 09:18 ET "Don't make them All green"; 09:19 ET "Do a couple police looking ones and ones after the color ui you designed").
Layouts/animations of the 5 v2 bars are kept exactly (geometry copied from render_topbar_v2.py @9d83341); only inks change,
except ZONE Z-labels and RADAR readouts which were enlarged to >=10px. Plus 6 new bars: LITEB, BADGE (police) and NVG, IRON, ALARM, PERIM
(after the locked Color UI themes, palettes from mocks/security-color-themes-v2/themes_spec_v2.h @2b79d83).
--- original v2 docstring ---
Five NEW black+lime (+white accent) header bars on the v9c CONTROLbot panel (a15182a), cycled by a BAR button:
  CURTN  matrix glyph-curtain wipe reveals the title; settled glyph seam ticks along the bottom
  TERM   matrix terminal prompt types 'SECURITYbot EX.DATA' with a blinking white block cursor
  ZONE   perimeter zone strip: Z1..Z4 status dots + segmented perimeter map (Z3 ping in clip)
  RADAR  rotating radar-sweep icon with blip + title + bearing readout
  ARM    shield/lock icon + arming-mode chip cycling HOME/NIGHT/AWAY with mode pips
Only header rows y1..23, x1..238 change. Helpers (AA text, RGB565, Lanczos, encoder) come from render_topbar_v1."""
import sys
sys.dont_write_bytecode=True
import json,shutil,math,random,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
sys.path.insert(0,str(Path(__file__).resolve().parent))
import render_topbar_v1 as R
from render_topbar_v1 import H,WD,SS,LIME,WHITE,TITLE,KANA,DIGS,text_alpha,text_w,ink_box,baseline_for_center,comp,to565,disc,rrect,glyph_sprite,blit,halo_factor

ROOT=R.ROOT
W=Path('/tmp/secbot_build/topbar_v2b')
OUT=ROOT/'mocks/topbar-v2b-colors'
R.W=W   # encoder scratch -> v2 scratch
GF=R.GF
R.FONTS.update({
 'ox':(GF+'Oxanium/Oxanium-VariableFont_wght.ttf','Bold'),
 'saira':(GF+'Saira Semi Condensed/SairaSemiCondensed-Bold.ttf',None),
 'exo':(GF+'Exo 2/Exo2-VariableFont_wght.ttf','Bold'),
})
NFR=105
VARIANTS=[('CURTN','matrix_curtain','MATRIX CURTAIN WIPE'),('TERM','matrix_terminal','MATRIX TERMINAL PROMPT'),
          ('ZONE','security_zone_strip','SECURITY ZONE STRIP'),('RADAR','security_radar_sweep','SECURITY RADAR SWEEP'),
          ('ARM','security_arm_mode','SECURITY ARM MODE')]
BLACK=(0,0,0)
# ---- RGB565-exact inks (r,b %8==0, g %4==0) ----
MGREEN=(0,200,64)          # matrix rain green
AMBER=(248,176,0);ORANGE=(248,124,0)
RED=(248,28,40)            # SEC_ACC SIREN ff1e2e -> 565
BLUE=(40,104,248)          # siren blue 2f6bff -> 565
DBLUE=(24,72,200)          # deep cruiser blue (white text 7:1)
LBLUE=(104,160,248)        # legible blue for text
CYAN=(0,148,248)           # HIVIS acc 0096ff
LCYAN=(0,224,248)
HIVIS_HDR=(96,192,248)     # HIVIS hdr 66c0ff
NVG_S=[(240,252,216),(176,252,80),(104,240,40),(40,152,24),(8,72,8)]                      # f2ffd8 b4ff50 6cf02a 2c9a18 0e4a0a
IRON_S=[(16,4,24),(72,12,120),(160,20,136),(232,48,88),(248,120,16),(248,192,48),(248,244,192)]  # 14051e..fff6c0
ALARM_RED=(248,20,24)      # ff1418
PERIM_HI=(176,80,248);PERIM_LO=(104,32,192);PERIM_PALE=(240,216,248)   # b050ff 6a20c0 f0d8ff
def gradcol(c0,c1,y0,y1):
 """per-pixel vertical ink gradient c0 (row y0) -> c1 (row y1)"""
 k=np.clip((np.arange(H)[:,None]-y0)/(y1-y0),0,1)[...,None]*np.ones((1,WD,1))
 return np.array(c0,float)*(1-k)+np.array(c1,float)*k
def L(a): return tuple(int(v) for v in np.array(LIME)*a)

def mask_line(img,y,x0,x1,col,a=1.0):
 m=np.zeros((H,WD));m[y,x0:x1+1]=a;comp(img,m,col)

def tel(e):return e
# ---------------- CURTN ----------------
CU_PX=15.5
CU_CW,CU_CH=6,8
SEAM_Y0=16       # seam glyph strip rows 16..21 (inclusive); title ink above
def curtain_cols():
 rnd=random.Random(4242);cols=[]
 for x in range(1,WD-CU_CW+1,CU_CW):
  cols.append(dict(x=x,ph=rnd.uniform(0,8),sp=rnd.uniform(0.9,1.6),g=[rnd.choice(KANA+DIGS) for _ in range(30)],seam=rnd.choice(KANA+DIGS),sd=rnd.randint(0,9999)))
 return cols
CCOLS=curtain_cols()
WIPE0,WIPE1=6,66
def render_CURTN(t):
 img=np.zeros((H,WD,3))
 tw=text_w(TITLE,'jb',CU_PX);x=round((WD-tw)/2*SS)/SS
 bl=round(baseline_for_center(TITLE,'jb',CU_PX,2,13)*SS)/SS
 ta=text_alpha(TITLE,'jb',CU_PX,x,bl)
 front=(t-WIPE0)/(WIPE1-WIPE0)*(WD+30)-15     # wipe front x (px)
 cur_l=np.zeros((H,WD));cur_w=np.zeros((H,WD));reveal=np.zeros(WD)
 for c in CCOLS:
  cx=c['x']+CU_CW/2
  behind=cx<front            # curtain already dropped here
  if behind:
   reveal[c['x']:c['x']+CU_CW]=1
   drop=(front-cx)*0.9       # stream falls away below
  else:drop=0
  # stream (full height curtain) scrolling down
  for k in range(-1,4):
   gy=int(math.floor((c['ph']+c['sp']*t)%CU_CH+k*CU_CH-CU_CH+drop))
   if gy>H or gy+CU_CH<0:continue
   ch=c['g'][(k+int(t*c['sp']/CU_CH))%30];key='kana' if ch in KANA else 'jbb'
   spr=glyph_sprite(ch,CU_CW,CU_CH,8.5 if key=='kana' else 7.5,mirror=(key=='kana'),key=key)
   if not behind:
    blit(cur_l,spr,c['x'],gy,0.68)
    if abs(cx-front)<CU_CW*1.5:blit(cur_w,spr,c['x'],gy,0.85)   # white-hot front edge
   elif drop<H:
    blit(cur_l,spr,c['x'],gy,0.40*max(0,1-drop/H))
 # title revealed behind the front (soft 3px edge)
 xs=np.arange(WD)+0.5
 rv=np.clip((front-xs)/3,0,1)
 hf=1-rv[None,:]*(1-halo_factor(ta))   # halo only where the title is revealed (no title ghost in the curtain)
 cur_l[SEAM_Y0-2:]=0;cur_w[SEAM_Y0-2:]=0     # curtain rows 0..13; the seam owns rows 16..21
 comp(img,cur_l*hf,MGREEN);comp(img,cur_w*hf,WHITE)
 tb=ink_box(ta);TG=gradcol(WHITE,LIME,tb[1]+1,tb[3]-1)   # title white (top) -> lime (bottom)
 comp(img,ta*rv[None,:],TG)
 # settled glyph seam: rows 16..21, tiny glyphs every 6px, one white 'scanner' glyph walks across
 seam=np.zeros((H,WD));seamw=np.zeros((H,WD))
 for i,c in enumerate(CCOLS):
  if c['x']+CU_CW/2>front:continue
  rnd=random.Random(c['sd']+int(t//6))
  ch=rnd.choice(KANA+DIGS);key='kana' if ch in KANA else 'jbb'
  spr=glyph_sprite(ch,CU_CW,6,6.5 if key=='kana' else 6.0,mirror=(key=='kana'),key=key)
  hot=(int(t*0.75)%len(CCOLS))==i
  blit(seamw if hot else seam,spr,c['x'],SEAM_Y0,0.9 if hot else 0.38)
 comp(img,seam,MGREEN);comp(img,seamw,WHITE)
 mask_line(img,22,0,WD-1,LIME,1.0)
 done=front>WD+5
 ib=ink_box(ta)
 spec={'elements':[{'kind':'title','text':TITLE,'font':'jb','px':CU_PX,'x':x,'baseline':bl,'col':LIME,'grad':[WHITE,LIME,tb[1]+1,tb[3]-1],'bg':BLACK}],
       'obstacles':{'seam':(0,SEAM_Y0,WD-1,SEAM_Y0+5),'bottom_rule':(0,22,WD-1,22)},
       'bg_effect':'glyph curtain (wipe) rows 0..14; settled seam rows 16..21','halo':True,'stray_check':False,'reveal_done':bool(done)}
 return img,spec

# ---------------- TERM ----------------
TE_PX=16.0
PROMPT='>'
TYPE0,TYPE_STEP=8,3
def render_TERM(t):
 img=np.zeros((H,WD,3))
 adv=text_w('M','jb',TE_PX)
 n=len(TITLE)
 typed=max(0,min(n,(t-TYPE0)//TYPE_STEP+1)) if t>=TYPE0 else 0
 total=(len(PROMPT)+1+n+1)*adv
 x0=round((WD-total)/2*SS)/SS
 bl=round(baseline_for_center(TITLE,'jb',TE_PX,1,19)*SS)/SS
 pa=text_alpha(PROMPT,'jb',TE_PX,x0,bl)
 comp(img,pa,ORANGE)
 tx=x0+(len(PROMPT)+1)*adv
 s=TITLE[:typed]
 ta=text_alpha(s,'jb',TE_PX,tx,bl) if s else np.zeros((H,WD))
 comp(img,ta,AMBER)
 # block cursor after last typed char (solid while typing, blinks 8f on/8f off when idle)
 idle=typed>=n
 on=True if not idle else ((t//8)%2==0)
 cxp=tx+typed*adv
 b=R.font('jb',TE_PX).getbbox('M',anchor='ls');ct,cb=bl+b[1]/SS,bl+b[3]/SS
 cur=(cxp+2.5,ct,cxp+adv+1.5,cb)   # >=2px clear of the last glyph
 if on:
  comp(img,rrect(cur[0],cur[1],cur[2],cur[3],0.01),WHITE)
 # terminal chrome: dim lime frame top/bottom ticks + status dot row
 mask_line(img,21,0,WD-1,AMBER,0.45);mask_line(img,22,0,WD-1,ORANGE,1.0)
 spec={'elements':[{'kind':'title','text':s,'font':'jb','px':TE_PX,'x':tx,'baseline':bl,'col':AMBER,'bg':BLACK},
                   {'kind':'text','text':PROMPT,'font':'jb','px':TE_PX,'x':x0,'baseline':bl,'col':ORANGE,'bg':BLACK}],
       'cursor':{'box':cur,'on':bool(on)},
       'obstacles':{'rules':(0,21,WD-1,22),'cursor':(int(math.floor(cur[0])),int(math.floor(cur[1])),int(math.ceil(cur[2]))-1,int(math.ceil(cur[3]))-1)},
       'stray_check':True,'typed':int(typed)}
 return img,spec

# ---------------- ZONE ----------------
ZO_PX=12.0
ZONES=['Z1','Z2','Z3','Z4']
ZL_PX=10.5
ZDOT=[LIME,AMBER,RED,BLUE];ZLAB=[LIME,AMBER,RED,LBLUE]   # Z1 ok / Z2 warn / Z3 alert / Z4 info
def render_ZONE(t):
 img=np.zeros((H,WD,3))
 x=4.0
 bl=round(baseline_for_center(TITLE,'ox',ZO_PX,1,15)*SS)/SS
 ta=text_alpha(TITLE,'ox',ZO_PX,x,bl);comp(img,ta,WHITE)
 els=[{'kind':'title','text':TITLE,'font':'ox','px':ZO_PX,'x':x,'baseline':bl,'col':WHITE,'bg':BLACK}]
 obst={}
 # zone chips right-aligned: dot r=2.4 + label (Oxanium 9)
 lw=text_w('Z1','ox',ZL_PX);cell=5.6+3.6+lw+4.0
 zx0=WD-4-cell*4+5
 zbl=round(baseline_for_center('Z1','ox',ZL_PX,1,15)*SS)/SS
 ping=(t//5)%2==0 and 30<=t<90
 for i,z in enumerate(ZONES):
  cx=zx0+i*cell+2.8;cy=8.0
  if i==2 and ping:   # Z3 ping: hollow + white core flash
   comp(img,disc(cx,cy,2.8),ZDOT[i]);comp(img,disc(cx,cy,1.8),BLACK);comp(img,disc(cx,cy,1.0),WHITE)
  else:
   comp(img,disc(cx,cy,2.8),ZDOT[i])
  lx=round((cx+2.8+3.6)*SS)/SS
  comp(img,text_alpha(z,'ox',ZL_PX,lx,zbl),ZLAB[i])
  els.append({'kind':'text','text':z,'font':'ox','px':ZL_PX,'x':lx,'baseline':zbl,'col':ZLAB[i],'bg':BLACK})
  obst[f'dot{z}']=(int(math.floor(cx-2.8)),int(math.floor(cy-2.8)),int(math.ceil(cx+2.8)),int(math.ceil(cy+2.8)))
 # segmented perimeter map rows 18..21: 4 segments with 3px gaps, end caps; Z3 segment pulses in ping
 segw=(WD-4-3*3)/4
 for i in range(4):
  s0=2+i*(segw+3);s1=s0+segw
  m=rrect(s0,18,s1,21,0.8)
  if i==2 and ping:comp(img,m,RED)      # Z3 alert segment turns red while pinging
  else:comp(img,m,LCYAN)
  # inner tick per segment centre (white accent on active Z1)
 px_=2+((t*2.2)%(WD-8));comp(img,rrect(px_,18,px_+4,21,0.8),WHITE)   # patrol marker sweeps the perimeter
 obst['perimeter']=(0,18,WD-1,20)
 spec={'elements':els,'obstacles':obst,'stray_check':True,'ping':bool(ping)}
 return img,spec

# ---------------- RADAR ----------------
RA_C=(12.5,11.0);RA_R=9.6
RA_PX=13.5
RD_PX=10.0     # readouts enlarged from 8.5/9 px
def render_RADAR(t):
 img=np.zeros((H,WD,3))
 cx,cy=RA_C
 ang=(t*4.0)%360           # sweep 3 s/rev (90 frames)
 yy,xx=np.mgrid[0:H*SS,0:WD*SS];px=(xx+0.5)/SS;py=(yy+0.5)/SS
 d=np.hypot(px-cx,py-cy);th=(np.degrees(np.arctan2(py-cy,px-cx))+360)%360
 lag=(ang-th)%360
 inside=d<=RA_R-1.2
 wedge=np.where(inside&(lag<70),np.clip(1-lag/70,0,1)**1.6*0.55,0)
 ring=((d<=RA_R)&(d>=RA_R-1.1)).astype(float)
 ring2=((d<=RA_R*0.5)&(d>=RA_R*0.5-0.9)).astype(float)*0.4
 cross=(((np.abs(px-cx)<0.45)|(np.abs(py-cy)<0.45))&inside).astype(float)*0.3
 red=lambda m:m.reshape(H,SS,WD,SS).mean(axis=(1,3))
 comp(img,red(wedge),CYAN);comp(img,red(cross),CYAN);comp(img,red(ring2),CYAN);comp(img,red(ring),LCYAN)
 # sweep arm
 arm=((lag<5)|(lag>357))&inside
 comp(img,red(arm.astype(float))*0.9,LCYAN)
 # blip at bearing 50deg r=5: lights when sweep passes, decays
 bang=50.0;bx=cx+5.2*math.cos(math.radians(bang));by=cy+5.2*math.sin(math.radians(bang))
 since=(ang-bang)%360;bi=max(0.25,1-since/300)
 comp(img,disc(bx,by,1.6)*bi,RED)
 obst={'radar':(int(cx-RA_R),int(cy-RA_R),int(math.ceil(cx+RA_R)),int(math.ceil(cy+RA_R)))}
 x=27.0
 bl=round(baseline_for_center(TITLE,'saira',RA_PX,2,11)*SS)/SS
 ta=text_alpha(TITLE,'saira',RA_PX,x,bl);comp(img,ta,HIVIS_HDR)
 # bearing readout under title + range ticks
 brg='BRG %03d  RNG 12m'%int(ang)
 rbl=round(baseline_for_center('BRG 000  RNG 12m','saira',RD_PX,14,20)*SS)/SS
 ra=text_alpha(brg,'saira',RD_PX,x,rbl);comp(img,ra,WHITE)
 # right: thin horizontal scan line tracking the sweep (ticks) from x 120..234
 tick=np.zeros((H,WD))
 for k,xx_ in enumerate(range(124,235,6)):tick[19:21 if k%5 else 22,xx_]=0.6 if k%5 else 1.0
 comp(img,tick,CYAN)
 hx=124+int(((ang/360)*110)//1)
 comp(img,disc(hx+0.5,20.0,1.5),RED)
 obst['ticks']=(122,17,236,22)
 nd='NODES 4/4';nw=text_w(nd,'saira',RD_PX);nx=round((WD-4-nw)*SS)/SS
 nbl=round(baseline_for_center(nd,'saira',RD_PX,3,10)*SS)/SS
 comp(img,text_alpha(nd,'saira',RD_PX,nx,nbl),WHITE)
 els=[{'kind':'title','text':TITLE,'font':'saira','px':RA_PX,'x':x,'baseline':bl,'col':HIVIS_HDR,'bg':BLACK},
      {'kind':'text','text':nd,'font':'saira','px':RD_PX,'x':nx,'baseline':nbl,'col':WHITE,'bg':BLACK,'note':'mock EX.DATA'},
      {'kind':'text','text':brg,'font':'saira','px':RD_PX,'x':x,'baseline':rbl,'col':WHITE,'bg':BLACK,'note':'mock EX.DATA'}]
 return img,{'elements':els,'obstacles':obst,'stray_check':True,'sweep_deg':ang}

# ---------------- ARM ----------------
AR_PX=13.5
MODES=['HOME','NIGHT','AWAY']
MODE_COL={'HOME':LIME,'NIGHT':BLUE,'AWAY':RED}
CHIP=(178,2,232,15)   # exclusive, fixed width (fits NIGHT)
def shield(cx,top,w,h):
 yy,xx=np.mgrid[0:H*SS,0:WD*SS];px=(xx+0.5)/SS;py=(yy+0.5)/SS
 u=(px-cx)/(w/2);v=(py-top)/h
 half=np.where(v<0.55,1.0,np.sqrt(np.clip(1-((v-0.55)/0.45)**2,0,1)))
 m=(v>=0)&(v<=1)&(np.abs(u)<=half)
 return m
def render_ARM(t):
 img=np.zeros((H,WD,3))
 red=lambda m:m.astype(float).reshape(H,SS,WD,SS).mean(axis=(1,3))
 sh=shield(10.5,2.0,15.0,18.0)
 inner=shield(10.5,3.6,11.6,14.6)
 SPL=np.zeros((H,WD,3));SPL[:,:]=RED;SPL[:,int(10.5):]=BLUE;SPL[:,10]=np.array(RED)*0.5+np.array(BLUE)*0.5   # SECURITYbot red | blue split
 comp(img,red(sh),SPL);comp(img,red(inner),BLACK)
 # padlock inside shield: body + shackle
 comp(img,rrect(7.2,9.6,13.8,15.2,0.8),WHITE)
 yy,xx=np.mgrid[0:H*SS,0:WD*SS];px=(xx+0.5)/SS;py=(yy+0.5)/SS
 dd=np.hypot(px-10.5,py-9.6);shk=(dd<=2.6)&(dd>=1.5)&(py<=9.8)
 comp(img,red(shk),WHITE)
 comp(img,disc(10.5,12.0,0.9),BLACK)
 x=23.0
 bl=round(baseline_for_center(TITLE,'exo',AR_PX,1,19)*SS)/SS
 ta=text_alpha(TITLE,'exo',AR_PX,x,bl);comp(img,ta,WHITE)
 mi=2 if t<0 else [2,0,1,2][min(3,t//27)]   # clip: AWAY -> HOME -> NIGHT -> AWAY; still uses AWAY
 mode=MODES[mi]
 x0,y0,x1,y1=CHIP
 MC=MODE_COL[mode]
 comp(img,rrect(x0,y0,x1,y1,2.0),MC);comp(img,rrect(x0+1,y0+1,x1-1,y1-1,1.2),BLACK)
 mw=text_w(mode,'exo',9.5);mx=round(((x0+x1)/2-mw/2)*SS)/SS
 mbl=round(baseline_for_center(mode,'exo',9.5,y0,y1-1)*SS)/SS
 comp(img,text_alpha(mode,'exo',9.5,mx,mbl),WHITE)
 # mode pips under chip rows 18..20
 pc=[(x0+x1)/2-8,(x0+x1)/2,(x0+x1)/2+8]
 for i,p in enumerate(pc):
  if i==mi:comp(img,rrect(p-3,18,p+3,21,1.0),MC)
  else:comp(img,rrect(p-3,18,p+3,21,1.0)*0.30,MODE_COL[MODES[i]])
 spec={'elements':[{'kind':'title','text':TITLE,'font':'exo','px':AR_PX,'x':x,'baseline':bl,'col':WHITE,'bg':BLACK},
                   {'kind':'chip','text':mode,'font':'exo','px':9.5,'x':mx,'baseline':mbl,'col':WHITE,'bg':BLACK,'box':(x0+1,y0+1,x1-1,y1-1)}],
       'obstacles':{'shield':(3,2,18,20),'chip':(x0,y0,x1-1,y1-1),'pips':(int(pc[0]-3),18,int(math.ceil(pc[2]+3)),20)},
       'stray_check':True,'mode':mode}
 return img,spec

# =====================================================================================================
# v2b additions: 2 police bars (LITEB, BADGE) + 4 Color-UI-theme bars (NVG, IRON, ALARM, PERIM)
# =====================================================================================================
R.FONTS.update({'chakra':(GF+'Chakra Petch/ChakraPetch-Bold.ttf',None)})
def mixc(a,b,k):return tuple(a[i]*(1-k)+b[i]*k for i in range(3))
def lut(S,k):
 """== sec_lut: k 0..255 across len(S) stops (vectorised)."""
 k=np.clip(np.asarray(k,float),0,255);n=len(S);seg=255//(n-1)
 i=np.minimum((k//seg).astype(int),n-2);f=((k-i*seg)/seg)[...,None]
 S=np.array(S,float);return S[i]*(1-f)+S[i+1]*f
def ss_grid():
 yy,xx=np.mgrid[0:H*SS,0:WD*SS];return (xx+0.5)/SS,(yy+0.5)/SS
def down(m):return m.astype(float).reshape(H,SS,WD,SS).mean(axis=(1,3))
def text_el(img,s,key,px,x,bl,col,bg=BLACK,kind='text',**kw):
 a=text_alpha(s,key,px,x,bl);comp(img,a,col)
 e={'kind':kind,'text':s,'font':key,'px':px,'x':x,'baseline':bl,'col':col,'bg':bg};e.update(kw);return a,e
def cx_text(s,key,px,x0,x1):return round(((x0+x1)/2-text_w(s,key,px)/2)*SS)/SS
def bl_c(s,key,px,a,b):return round(baseline_for_center(s,key,px,a,b)*SS)/SS

# ---------------- LITEB: police light bar ----------------
POD_L=(2,2,42,21);POD_R=(196,2,236,21)
def lite_state(t):
 p=t%15;return p<7,7<=p<14          # red on 7f, blue on 7f, 1f gap -> each side 2 Hz, alternating
def pod(img,box,col,on):
 x0,y0,x1,y1=box;I=1.0 if on else 0.20
 if on:   # soft glow 3px around the pod
  px,py=ss_grid();dx=np.maximum(np.maximum(x0-px,px-x1),0);dy=np.maximum(np.maximum(y0-py,py-y1),0)
  d=np.hypot(dx,dy);g=np.where((d>0)&(d<3.0),(1-d/3.0)**2*0.45,0);comp(img,down(g>0)*0+down_np(g),col)
 comp(img,rrect(x0,y0,x1,y1,2.5),mixc(BLACK,col,I))
 for i in range(3):   # 3 LED modules with a white-hot core when lit
  cx0=x0+3+i*12;comp(img,rrect(cx0,y0+3,cx0+10,y1-3,1.5),mixc(BLACK,mixc(col,WHITE,0.22) if on else col,0.35 if not on else 1.0))
def down_np(g):return g.reshape(H,SS,WD,SS).mean(axis=(1,3))
def render_LITEB(t):
 img=np.zeros((H,WD,3))
 ron,bon=lite_state(t)
 pod(img,POD_L,RED,ron);pod(img,POD_R,BLUE,bon)
 # lens strip under title: red half | blue half
 comp(img,rrect(48,19,119,21,0.8),mixc(BLACK,RED,1.0 if ron else 0.25));comp(img,rrect(119,19,190,21,0.8),mixc(BLACK,BLUE,1.0 if bon else 0.25))
 px=12.5;x=cx_text(TITLE,'chakra',px,46,192);bl=bl_c(TITLE,'chakra',px,2,16)
 ta,e=text_el(img,TITLE,'chakra',px,x,bl,WHITE,kind='title')
 return img,{'elements':[e],'obstacles':{'podL':(0,0,45,23-1),'podR':(193,0,WD-1,22),'lens':(48,19,190,20)},
             'stray_check':True,'red_on':bool(ron),'blue_on':bool(bon),
             'palette':[WHITE,RED,BLUE],'flash':{'red_box':POD_L,'blue_box':POD_R,'hz_each':2.0}}

# ---------------- BADGE: cruiser livery ----------------
def edge_x(py):return 150+(22-py)*0.55
def render_BADGE(t):
 img=np.zeros((H,WD,3));px,py=ss_grid()
 blk=px<edge_x(py)
 comp(img,down(blk),DBLUE)
 # moving sheen across the blue block (before the title), off-screen in the still
 sx=-40+t*2.4;sh=np.clip(1-np.abs((px+py*0.6)-sx)/7,0,1)*0.22*blk
 comp(img,down_np(sh),WHITE)
 e0=edge_x(py)
 comp(img,down((px>=e0+3)&(px<e0+6)),WHITE);comp(img,down((px>=e0+8)&(px<e0+11)),RED)
 tpx=13.5;x=6.0;bl=bl_c(TITLE,'exo',tpx,2,19)
 ta,e=text_el(img,TITLE,'exo',tpx,x,bl,WHITE,bg=DBLUE,kind='title')
 unit='UNIT 01' if t<53 else 'DISPATCH'
 upx=10.5;ux=round((WD-4-text_w(unit,'exo',upx))*SS)/SS;ubl=bl_c('DISPATCH','exo',upx,2,11)
 ua,ue=text_el(img,unit,'exo',upx,ux,ubl,WHITE)
 # status LEDs under the unit tag: red / blue alternating at 1 Hz
 rb=(t//15)%2==0
 comp(img,disc(214.5,17.0,2.2),mixc(BLACK,RED,1.0 if rb else 0.25));comp(img,disc(222.5,17.0,2.2),mixc(BLACK,BLUE,0.25 if rb else 1.0))
 return img,{'elements':[e,ue],'obstacles':{'livery_stripes':(int(edge_x(22)+3),0,int(math.ceil(edge_x(0)+11)),22),'leds':(212,14,225,20)},
             'stray_check':True,'unit':unit,'palette':[DBLUE,WHITE,RED,BLUE],'text_on_fill':True}

# ---------------- NVG: night-vision bloom (SEC_NVG) ----------------
def nvg_bg(t):
 X=np.arange(WD)[None,:]+1;Y=np.arange(H)[:,None]+1        # screen coords (header x1..238, y1..23)
 d=np.minimum((np.abs(X-120)*5+np.abs(Y-160)*4)//4,255)     # == sec_bloom
 c=lut(NVG_S,d)
 br=0.42*(1+0.08*math.sin(2*math.pi*t/30))                   # tube dimmed for legibility + 1 Hz breathing
 c=c*br
 c[(Y[:,0]%4)==3]*=0.65                                        # every 4th row scanline (35% darker, as theme)
 return c
def render_NVG(t):
 img=nvg_bg(t)
 tpx=14.0;x=cx_text(TITLE,'jb',tpx,0,WD);bl=bl_c(TITLE,'jb',tpx,2,18)
 ta=text_alpha(TITLE,'jb',tpx,x,bl)
 g1=text_alpha('GEN3','jbb',10.0,3.0,bl_c('GEN3','jbb',10.0,2,18));g2s='AGC';g2x=round((WD-3-text_w(g2s,'jbb',10.0))*SS)/SS
 g2=text_alpha(g2s,'jbb',10.0,g2x,bl_c('AGC','jbb',10.0,2,18))
 hf=halo_factor(np.maximum(np.maximum(ta,g1),g2),1.0,1.5);img*=(0.25+0.75*hf)[...,None]
 rnd=random.Random(t*31+7)
 for _ in range(10):   # photon sparkle in the tube, away from text halos
  sx,sy=rnd.randrange(1,WD-1),rnd.randrange(1,H-1)
  if hf[sy,sx]>=1:img[sy,sx]=img[sy,sx]*0.3+np.array(NVG_S[0])*0.7
 comp(img,ta,NVG_S[0]);comp(img,g1,NVG_S[1]);comp(img,g2,NVG_S[1])
 els=[{'kind':'title','text':TITLE,'font':'jb','px':tpx,'x':x,'baseline':bl,'col':NVG_S[0],'bg':'textured'},
      {'kind':'text','text':'GEN3','font':'jbb','px':10.0,'x':3.0,'baseline':bl_c('GEN3','jbb',10.0,2,18),'col':NVG_S[1],'bg':'textured'},
      {'kind':'text','text':'AGC','font':'jbb','px':10.0,'x':g2x,'baseline':bl_c('AGC','jbb',10.0,2,18),'col':NVG_S[1],'bg':'textured'}]
 return img,{'elements':els,'obstacles':{},'stray_check':False,'palette':NVG_S,'bg':'SEC_NVG bloom x0.42, scanline every 4th row'}

# ---------------- IRON: ironbow heat (SEC_IRON) ----------------
def render_IRON(t):
 X=np.arange(WD)[None,:]*np.ones((H,1))
 img=lut(IRON_S,X/237*55)                       # cold violet field (k 0..55)
 # hot target blob (heat signature) drifting at right
 bx=214+3*math.sin(2*math.pi*t/52);by=9.5+0.8*math.sin(2*math.pi*t/35)
 px,py=ss_grid();r2=((px-bx)/15)**2+((py-by)/8)**2
 k=np.clip(255*(1-r2),0,255);hot=lut(IRON_S,np.maximum(k,55*px/237))
 m=(r2<1).astype(float)
 img=img*(1-down_np(m)[...,None])+down_np(hot*m[...,None]*1.0) if False else img
 hb=hot.reshape(H,SS,WD,SS,3).mean(axis=(1,3));ma=down_np(m)[...,None]
 img=img*(1-ma)+hb*ma
 # thermal scale bar rows 19..21 (full palette cold->hot) + max marker above it
 sc=lut(IRON_S,np.clip((np.arange(WD)-2)/(WD-5)*255,0,255))
 msk=rrect(2,19,WD-2,22,0.8)
 img=img*(1-msk[...,None])+sc[None,:,:]*msk[...,None]
 temp=38.4+0.5*(0.5+0.5*math.sin(2*math.pi*t/70))
 mxp=2+(temp-30)/10*(WD-5)
 comp(img,disc(mxp,17.0,1.2),IRON_S[6])
 tpx=13.5;x=4.0;bl=bl_c(TITLE,'saira',tpx,2,13)
 ta=text_alpha(TITLE,'saira',tpx,x,bl)
 rd='MAX %.1fC'%temp;rbl=bl_c('MAX 38.6C','saira',10.0,2,13);rx=round((x+text_w(TITLE,'saira',tpx)+8)*SS)/SS
 ra=text_alpha(rd,'saira',10.0,rx,rbl)
 hf=halo_factor(np.maximum(ta,ra),1.0,1.5)
 img=img*hf[...,None]+np.array(IRON_S[0])*(1-hf[...,None])      # cold halo behind text
 comp(img,ta,IRON_S[6]);comp(img,ra,IRON_S[5])
 els=[{'kind':'title','text':TITLE,'font':'saira','px':tpx,'x':x,'baseline':bl,'col':IRON_S[6],'bg':'textured'},
      {'kind':'text','text':rd,'font':'saira','px':10.0,'x':rx,'baseline':rbl,'col':IRON_S[5],'bg':'textured','note':'mock EX.DATA'}]
 return img,{'elements':els,'obstacles':{'scale':(2,16,WD-3,21),'blob':(int(bx-16),0,WD-1,int(by+9))},'stray_check':False,'palette':IRON_S,
             'bg':'SEC_IRON ironbow LUT: cold field k0..55, hot blob full ramp, scale bar k0..255'}

# ---------------- ALARM: strobe + caution stripes (SEC_ALARM) ----------------
def render_ALARM(t):
 img=np.zeros((H,WD,3))
 on=(t%15)<8                                    # 2 Hz strobe
 if on:comp(img,rrect(36,1,202,22,0.5)*0.16,ALARM_RED)
 px,py=ss_grid();sh=t*0.5
 band=(((px+py+sh)//5)%3==2)
 for x0,x1 in ((2,34),(204,236)):
  inn=(px>=x0)&(px<x1)&(py>=2)&(py<21)
  comp(img,down(inn&~band),ALARM_RED);comp(img,down(inn&band),WHITE)
 comp(img,(np.arange(H)[:,None]==0)*np.ones((1,WD))*(1.0 if on else 0.3),ALARM_RED)
 comp(img,(np.arange(H)[:,None]==22)*np.ones((1,WD))*(1.0 if on else 0.3),ALARM_RED)
 tpx=13.0;x=cx_text(TITLE,'chakra',tpx,36,202);bl=bl_c(TITLE,'chakra',tpx,2,20)
 ta,e=text_el(img,TITLE,'chakra',tpx,x,bl,WHITE,kind='title')
 return img,{'elements':[e],'obstacles':{'stripesL':(2,2,33,20),'stripesR':(204,2,235,20),'rules':(0,0,WD-1,0),'rule_b':(0,22,WD-1,22)},
             'stray_check':False,'strobe_on':bool(on),'palette':[ALARM_RED,WHITE],'flash':{'rule_row':0,'hz':2.0}}

# ---------------- PERIM: violet laser tripwire grid (SEC_PERIM) ----------------
def render_PERIM(t):
 img=np.zeros((H,WD,3))
 g=np.zeros((H,WD));g[:,::10]=0.40;g[6,:]=0.40;g[16,:]=0.40;g[19:,:]=0
 tpx=13.0;x=4.0;bl=bl_c(TITLE,'ox',tpx,1,15)
 ta=text_alpha(TITLE,'ox',tpx,x,bl)
 tr='TRIP 0/4';tx=round((WD-4-text_w(tr,'ox',10.0))*SS)/SS;tbl=bl_c(tr,'ox',10.0,1,15)
 tra=text_alpha(tr,'ox',10.0,tx,tbl)
 hf=halo_factor(np.maximum(ta,tra),1.5,1.0)
 comp(img,g*hf,PERIM_LO)
 # two tripwire beams with travelling pulses + emitter dots
 xs=np.arange(WD)+0.5
 for row,dirn,ph in ((19,1,0),(21,-1,90)):
  b=np.zeros((H,WD));b[row,5:WD-5]=0.85;comp(img,b,PERIM_HI)
  c=((t*3.0*dirn+ph)%(WD+40))-20 if dirn>0 else (WD+20)-((t*3.0+ph)%(WD+40))
  pu=np.zeros((H,WD));pu[row,5:WD-5]=np.exp(-((xs[5:WD-5]-c)/6)**2);comp(img,pu,PERIM_PALE)
 for ex in (2.5,WD-2.5):
  for row in (19,21):comp(img,disc(ex,row+0.5,1.2),PERIM_PALE)
 comp(img,ta,PERIM_PALE);comp(img,tra,PERIM_HI)
 els=[{'kind':'title','text':TITLE,'font':'ox','px':tpx,'x':x,'baseline':bl,'col':PERIM_PALE,'bg':BLACK},
      {'kind':'text','text':tr,'font':'ox','px':10.0,'x':tx,'baseline':tbl,'col':PERIM_HI,'bg':BLACK,'note':'mock EX.DATA'}]
 return img,{'elements':els,'obstacles':{'beams':(0,18,WD-1,22)},'stray_check':False,'halo':True,'palette':[PERIM_PALE,PERIM_HI,PERIM_LO],
             'bg':'violet 10px tripwire grid (alpha .40), cleared 1.5px around text'}

# ---------------- registry ----------------
PALETTE={'CURTN':[WHITE,LIME,MGREEN],'TERM':[AMBER,ORANGE,WHITE],'ZONE':[WHITE,LIME,AMBER,RED,BLUE,LBLUE,LCYAN],
         'RADAR':[HIVIS_HDR,CYAN,LCYAN,RED,WHITE],'ARM':[RED,BLUE,WHITE,LIME]}
RENDER={'CURTN':render_CURTN,'TERM':render_TERM,'ZONE':render_ZONE,'RADAR':render_RADAR,'ARM':render_ARM,
        'LITEB':render_LITEB,'BADGE':render_BADGE,'NVG':render_NVG,'IRON':render_IRON,'ALARM':render_ALARM,'PERIM':render_PERIM}
STILL_T={'CURTN':100,'TERM':96,'ZONE':0,'RADAR':14,'ARM':0,'LITEB':3,'BADGE':0,'NVG':0,'IRON':0,'ALARM':2,'PERIM':10}
VARIANTS=[('CURTN','matrix_curtain','MATRIX CURTAIN','recolor'),('TERM','amber_terminal','AMBER TERMINAL','recolor'),
          ('ZONE','zone_status','ZONE STATUS','recolor'),('RADAR','hivis_radar','HI-VIS RADAR','recolor'),('ARM','redblue_arm','RED/BLUE ARM MODE','recolor'),
          ('LITEB','police_lightbar','POLICE LIGHT BAR','police'),('BADGE','police_livery','POLICE CRUISER LIVERY','police'),
          ('NVG','nvg_bloom','NV GREEN BLOOM','theme'),('IRON','ir_ironbow','IR IRONBOW','theme'),('ALARM','alarm_strobe','ALARM STROBE','theme'),
          ('PERIM','perim_tripwire','PERIMETER TRIPWIRE','theme')]
def header(vid,t):
 img,spec=RENDER[vid](t)
 spec.setdefault('palette',PALETTE.get(vid))
 return to565(img),spec

README='''# SECURITYbot top bar v2b: colours (no more all-green) + police + Color-UI-theme bars

Mobandy 2026-11-09 09:18 ET: "Don't make them All green". 09:19 ET: "Do a couple police looking ones and ones after the color ui you designed".
Base: `controlbot-bigchip-v9c-glasses/01_panel@1x.png` (a15182a). Only header rows y1..23, x1..238 change.

## Set A: the 5 v2 bars recoloured (layout and animation unchanged from v2 9d83341)
The 5-up is `00_topbar_compare_5up@3x_smooth.png`.

| BAR | look |
|-----|------|
| CURTN | matrix-green kana curtain with a white leading edge; title white (top) to lime (bottom); lime rule |
| TERM | amber phosphor terminal: orange `>` prompt, amber typing, white block cursor, amber/orange rules |
| ZONE | white title; Z1 lime OK, Z2 amber, Z3 red alert, Z4 blue; cyan perimeter bar (Z3 segment goes red on ping); white patrol marker. Z labels 9 -> 10.5px |
| RADAR | hi-vis blue title, cyan sweep/ring/ticks, red blip and red bearing marker, white readouts (8.5/9 -> 10px) |
| ARM | red/blue split shield with white padlock, white title; mode chip and pips per mode: AWAY red, HOME lime, NIGHT blue |

## Set B: 6 new bars (`00_topbar_compare_extra_6up@3x_smooth.png`)

| BAR | look |
|-----|------|
| LITEB | police light bar: red pod left and blue pod right, each with 3 LED modules and a glow, alternating at 2 Hz per side; red/blue lens strip; white title |
| BADGE | cruiser door livery: deep-blue slanted block with white title, white + red pinstripes, `UNIT 01` -> `DISPATCH` tag, red/blue status LEDs alternating at 1 Hz, slow sheen |
| NVG | SEC_NVG bloom LUT (f2ffd8..0e4a0a) dimmed to 42% with 1 Hz breathing and every-4th-row scanlines, photon sparkle, `GEN3` and `AGC` tags |
| IRON | SEC_IRON ironbow LUT (14051e..fff6c0): cold violet field, drifting hot target blob, full cold-to-hot scale bar with max marker, mock `MAX 38.xC` |
| ALARM | SEC_ALARM ff1418/white: red/red/white diagonal caution stripes at both ends (slow crawl), red top and bottom rules strobing at 2 Hz with a red wash |
| PERIM | SEC_PERIM violet: 6a20c0 10px tripwire grid, two b050ff laser beams with f0d8ff pulses, emitter dots, `TRIP 0/4` |

All ids are 5 chars or fewer. Text is smooth anti-aliased (x4 supersampled), with RGB565-exact inks. All numbers and states are mock EX.DATA.
Per bar: `@1x.png`, `@3x_smooth.png`, `_header@3x_smooth.png`, `@3x.mp4` (720x960 30fps H.264 yuv420p crf14 BT.709 faststart, 105 frames).
Render with `python3 sim/render_topbar_v2b.py` and verify with `python3 sim/verify_topbar_v2b.py`. Mock only.
'''

def main():
 W.mkdir(parents=True,exist_ok=True)
 if OUT.exists():
  assert OUT.name=='topbar-v2b-colors';shutil.rmtree(OUT)
 OUT.mkdir(parents=True)
 base=np.array(Image.open(R.V9C).convert('RGB'))
 specs={};crops={}
 for i,(vid,slug,name,grp) in enumerate(VARIANTS,1):
  pre=f'{i:02d}_{vid}_{slug}'
  frames=[];fs=[]
  for t in range(NFR):
   h,s=header(vid,t);frames.append(R.compose(base,h));fs.append(s)
  frames=np.array(frames);np.save(W/f'{vid}_frames.npy',frames)
  st=STILL_T[vid];still=frames[st]
  Image.fromarray(still).save(OUT/f'{pre}@1x.png')
  sm=R.smooth(still);Image.fromarray(sm).save(OUT/f'{pre}@3x_smooth.png')
  Image.fromarray(sm[:75]).save(OUT/f'{pre}_header@3x_smooth.png');crops[vid]=(name,sm[:75])
  R.encode_smooth(frames,OUT/f'{pre}@3x.mp4')
  keep=('mode','ping','typed','reveal_done','sweep_deg','red_on','blue_on','strobe_on','unit')
  specs[vid]={'bar_id':vid,'group':grp,'name':name,'prefix':pre,'still_frame':st,'still_spec':fs[st],'frames':NFR,
              'palette':fs[st]['palette'],'frame_specs':[{k:v for k,v in f.items() if k in keep} for f in fs]}
 for fn,ids in (('00_topbar_compare_5up@3x_smooth.png',[v[0] for v in VARIANTS if v[3]=='recolor']),
                ('00_topbar_compare_extra_6up@3x_smooth.png',[v[0] for v in VARIANTS if v[3]!='recolor'])):
  parts=[]
  for vid in ids:
   name,crop=crops[vid];parts+=[R.label_strip(f'BAR {vid}  {name}',720),crop,np.full((12,720,3),40,np.uint8)]
  Image.fromarray(np.concatenate(parts[:-1],0)).save(OUT/fn)
 (OUT/'TOPBAR_SPEC.json').write_text(json.dumps({'header_region_exclusive':{'y':[R.HY0,R.HY1],'x':[R.HX0,R.HX1]},
   'base':str(R.V9C.relative_to(ROOT)),'base_commit':'a15182a','layout_source':'sim/render_topbar_v2.py @9d83341','theme_source':'mocks/security-color-themes-v2/themes_spec_v2.h @2b79d83',
   'bar_button':{'label':'BAR','cycle':[v[0] for v in VARIANTS],'value_max_chars':5,'glcd_value_px':{v[0]:6*len(v[0])-1 for v in VARIANTS}},
   'variants':specs},indent=1,default=float)+'\n')
 (OUT/'README.md').write_text(README)
 subprocess.run([sys.executable,str(ROOT/'sim/verify_topbar_v2b.py')],check=True)

if __name__=='__main__':main()
