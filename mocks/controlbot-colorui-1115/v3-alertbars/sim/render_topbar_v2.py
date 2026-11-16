#!/usr/bin/env python3
"""Top-bar v2 (Mobandy 2026-11-09 08:26 ET: "5 different ones with only two matrix ui variation").
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
W=Path('/tmp/secbot_build/topbar_v2')
OUT=ROOT/'mocks/topbar-v2'
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
 comp(img,cur_l*hf,LIME);comp(img,cur_w*hf,WHITE)
 comp(img,ta*rv[None,:],LIME)
 # settled glyph seam: rows 16..21, tiny glyphs every 6px, one white 'scanner' glyph walks across
 seam=np.zeros((H,WD));seamw=np.zeros((H,WD))
 for i,c in enumerate(CCOLS):
  if c['x']+CU_CW/2>front:continue
  rnd=random.Random(c['sd']+int(t//6))
  ch=rnd.choice(KANA+DIGS);key='kana' if ch in KANA else 'jbb'
  spr=glyph_sprite(ch,CU_CW,6,6.5 if key=='kana' else 6.0,mirror=(key=='kana'),key=key)
  hot=(int(t*0.75)%len(CCOLS))==i
  blit(seamw if hot else seam,spr,c['x'],SEAM_Y0,0.9 if hot else 0.38)
 comp(img,seam,LIME);comp(img,seamw,WHITE)
 mask_line(img,22,0,WD-1,LIME,1.0)
 done=front>WD+5
 ib=ink_box(ta)
 spec={'elements':[{'kind':'title','text':TITLE,'font':'jb','px':CU_PX,'x':x,'baseline':bl,'col':LIME,'bg':BLACK}],
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
 comp(img,pa,WHITE)
 tx=x0+(len(PROMPT)+1)*adv
 s=TITLE[:typed]
 ta=text_alpha(s,'jb',TE_PX,tx,bl) if s else np.zeros((H,WD))
 comp(img,ta,LIME)
 # block cursor after last typed char (solid while typing, blinks 8f on/8f off when idle)
 idle=typed>=n
 on=True if not idle else ((t//8)%2==0)
 cxp=tx+typed*adv
 b=R.font('jb',TE_PX).getbbox('M',anchor='ls');ct,cb=bl+b[1]/SS,bl+b[3]/SS
 cur=(cxp+2.5,ct,cxp+adv+1.5,cb)   # >=2px clear of the last glyph
 if on:
  comp(img,rrect(cur[0],cur[1],cur[2],cur[3],0.01),WHITE)
 # terminal chrome: dim lime frame top/bottom ticks + status dot row
 mask_line(img,21,0,WD-1,LIME,0.45);mask_line(img,22,0,WD-1,LIME,1.0)
 spec={'elements':[{'kind':'title','text':s,'font':'jb','px':TE_PX,'x':tx,'baseline':bl,'col':LIME,'bg':BLACK},
                   {'kind':'text','text':PROMPT,'font':'jb','px':TE_PX,'x':x0,'baseline':bl,'col':WHITE,'bg':BLACK}],
       'cursor':{'box':cur,'on':bool(on)},
       'obstacles':{'rules':(0,21,WD-1,22),'cursor':(int(math.floor(cur[0])),int(math.floor(cur[1])),int(math.ceil(cur[2]))-1,int(math.ceil(cur[3]))-1)},
       'stray_check':True,'typed':int(typed)}
 return img,spec

# ---------------- ZONE ----------------
ZO_PX=12.0
ZONES=['Z1','Z2','Z3','Z4']
def render_ZONE(t):
 img=np.zeros((H,WD,3))
 x=4.0
 bl=round(baseline_for_center(TITLE,'ox',ZO_PX,1,15)*SS)/SS
 ta=text_alpha(TITLE,'ox',ZO_PX,x,bl);comp(img,ta,LIME)
 els=[{'kind':'title','text':TITLE,'font':'ox','px':ZO_PX,'x':x,'baseline':bl,'col':LIME,'bg':BLACK}]
 obst={}
 # zone chips right-aligned: dot r=2.4 + label (Oxanium 9)
 lw=text_w('Z1','ox',9.0);cell=5.6+3.6+lw+6
 zx0=WD-4-cell*4+5
 zbl=round(baseline_for_center('Z1','ox',9.0,1,15)*SS)/SS
 ping=(t//5)%2==0 and 30<=t<90
 for i,z in enumerate(ZONES):
  cx=zx0+i*cell+2.8;cy=8.0
  if i==2 and ping:   # Z3 ping: hollow + white core flash
   comp(img,disc(cx,cy,2.8),LIME);comp(img,disc(cx,cy,1.8),BLACK);comp(img,disc(cx,cy,1.0),WHITE)
  else:
   comp(img,disc(cx,cy,2.8),LIME)
  lx=round((cx+2.8+3.6)*SS)/SS
  comp(img,text_alpha(z,'ox',9.0,lx,zbl),LIME)
  els.append({'kind':'text','text':z,'font':'ox','px':9.0,'x':lx,'baseline':zbl,'col':LIME,'bg':BLACK})
  obst[f'dot{z}']=(int(math.floor(cx-2.8)),int(math.floor(cy-2.8)),int(math.ceil(cx+2.8)),int(math.ceil(cy+2.8)))
 # segmented perimeter map rows 18..21: 4 segments with 3px gaps, end caps; Z3 segment pulses in ping
 segw=(WD-4-3*3)/4
 for i in range(4):
  s0=2+i*(segw+3);s1=s0+segw
  a=0.35 if (i==2 and ping) else 1.0
  m=rrect(s0,18,s1,21,0.8)*a;comp(img,m,LIME)
  # inner tick per segment centre (white accent on active Z1)
 px_=2+((t*2.2)%(WD-8));comp(img,rrect(px_,18,px_+4,21,0.8),WHITE)   # patrol marker sweeps the perimeter
 obst['perimeter']=(0,18,WD-1,20)
 spec={'elements':els,'obstacles':obst,'stray_check':True,'ping':bool(ping)}
 return img,spec

# ---------------- RADAR ----------------
RA_C=(12.5,11.0);RA_R=9.6
RA_PX=13.5
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
 comp(img,red(wedge),LIME);comp(img,red(cross),LIME);comp(img,red(ring2),LIME);comp(img,red(ring),LIME)
 # sweep arm
 arm=((lag<5)|(lag>357))&inside
 comp(img,red(arm.astype(float))*0.9,LIME)
 # blip at bearing 50deg r=5: lights when sweep passes, decays
 bang=50.0;bx=cx+5.2*math.cos(math.radians(bang));by=cy+5.2*math.sin(math.radians(bang))
 since=(ang-bang)%360;bi=max(0.25,1-since/300)
 comp(img,disc(bx,by,1.5)*bi,WHITE)
 obst={'radar':(int(cx-RA_R),int(cy-RA_R),int(math.ceil(cx+RA_R)),int(math.ceil(cy+RA_R)))}
 x=27.0
 bl=round(baseline_for_center(TITLE,'saira',RA_PX,2,11)*SS)/SS
 ta=text_alpha(TITLE,'saira',RA_PX,x,bl);comp(img,ta,LIME)
 # bearing readout under title + range ticks
 brg='BRG %03d  RNG 12m'%int(ang)
 rbl=round(baseline_for_center('BRG 000  RNG 12m','saira',8.5,14,19)*SS)/SS
 ra=text_alpha(brg,'saira',8.5,x,rbl);comp(img,ra,LIME)
 # right: thin horizontal scan line tracking the sweep (ticks) from x 120..234
 tick=np.zeros((H,WD))
 for k,xx_ in enumerate(range(124,235,6)):tick[19:21 if k%5 else 22,xx_]=0.6 if k%5 else 1.0
 comp(img,tick,LIME)
 hx=124+int(((ang/360)*110)//1)
 comp(img,disc(hx+0.5,20.0,1.4),WHITE)
 obst['ticks']=(122,17,236,22)
 nd='NODES 4/4';nw=text_w(nd,'saira',9.0);nx=round((WD-4-nw)*SS)/SS
 nbl=round(baseline_for_center(nd,'saira',9.0,3,10)*SS)/SS
 comp(img,text_alpha(nd,'saira',9.0,nx,nbl),WHITE)
 els=[{'kind':'title','text':TITLE,'font':'saira','px':RA_PX,'x':x,'baseline':bl,'col':LIME,'bg':BLACK},
      {'kind':'text','text':nd,'font':'saira','px':9.0,'x':nx,'baseline':nbl,'col':WHITE,'bg':BLACK,'note':'mock EX.DATA'},
      {'kind':'text','text':brg,'font':'saira','px':8.5,'x':x,'baseline':rbl,'col':LIME,'bg':BLACK,'note':'mock EX.DATA'}]
 return img,{'elements':els,'obstacles':obst,'stray_check':True,'sweep_deg':ang}

# ---------------- ARM ----------------
AR_PX=13.5
MODES=['HOME','NIGHT','AWAY']
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
 comp(img,red(sh),LIME);comp(img,red(inner),BLACK)
 # padlock inside shield: body + shackle
 comp(img,rrect(7.2,9.6,13.8,15.2,0.8),LIME)
 yy,xx=np.mgrid[0:H*SS,0:WD*SS];px=(xx+0.5)/SS;py=(yy+0.5)/SS
 dd=np.hypot(px-10.5,py-9.6);shk=(dd<=2.6)&(dd>=1.5)&(py<=9.8)
 comp(img,red(shk),LIME)
 comp(img,disc(10.5,12.0,0.9),BLACK)
 x=23.0
 bl=round(baseline_for_center(TITLE,'exo',AR_PX,1,19)*SS)/SS
 ta=text_alpha(TITLE,'exo',AR_PX,x,bl);comp(img,ta,LIME)
 mi=2 if t<0 else [2,0,1,2][min(3,t//27)]   # clip: AWAY -> HOME -> NIGHT -> AWAY; still uses AWAY
 mode=MODES[mi]
 x0,y0,x1,y1=CHIP
 comp(img,rrect(x0,y0,x1,y1,2.0),LIME);comp(img,rrect(x0+1,y0+1,x1-1,y1-1,1.2),BLACK)
 mw=text_w(mode,'exo',9.5);mx=round(((x0+x1)/2-mw/2)*SS)/SS
 mbl=round(baseline_for_center(mode,'exo',9.5,y0,y1-1)*SS)/SS
 comp(img,text_alpha(mode,'exo',9.5,mx,mbl),WHITE)
 # mode pips under chip rows 18..20
 pc=[(x0+x1)/2-8,(x0+x1)/2,(x0+x1)/2+8]
 for i,p in enumerate(pc):
  if i==mi:comp(img,rrect(p-3,18,p+3,21,1.0),LIME)
  else:comp(img,rrect(p-3,18,p+3,21,1.0)*0.30,LIME)
 spec={'elements':[{'kind':'title','text':TITLE,'font':'exo','px':AR_PX,'x':x,'baseline':bl,'col':LIME,'bg':BLACK},
                   {'kind':'chip','text':mode,'font':'exo','px':9.5,'x':mx,'baseline':mbl,'col':WHITE,'bg':BLACK,'box':(x0+1,y0+1,x1-1,y1-1)}],
       'obstacles':{'shield':(3,2,18,20),'chip':(x0,y0,x1-1,y1-1),'pips':(int(pc[0]-3),18,int(math.ceil(pc[2]+3)),20)},
       'stray_check':True,'mode':mode}
 return img,spec

RENDER={'CURTN':render_CURTN,'TERM':render_TERM,'ZONE':render_ZONE,'RADAR':render_RADAR,'ARM':render_ARM}
STILL_T={'CURTN':100,'TERM':96,'ZONE':0,'RADAR':14,'ARM':0}
def header(vid,t):
 img,spec=RENDER[vid](t);return to565(img),spec

README='''# SECURITYbot top bar v2: 2 Matrix + 3 Security, black + lime (+white accent)

Mobandy 2026-11-09 08:26 ET: "Let's do 5 different ones with only two matrix ui variation".
All five are new designs. None reuses v1's rain, decode, ticker, hazard or HUD bars. The CONTROLbot BAR button cycles them by id (5 chars or fewer, fits a 39px button value):

| BAR id | prefix | look |
|--------|--------|------|
| CURTN | 01_CURTN_matrix_curtain | mirrored kana curtain covers the bar; a white-hot wipe front sweeps L->R and the curtain drops away to reveal the lime title; a settled glyph seam keeps ticking along the bottom with a white scanner glyph |
| TERM | 02_TERM_matrix_terminal | terminal prompt `>` (white) types `SECURITYbot EX.DATA` in lime mono; white block cursor solid while typing, blinking when idle |
| ZONE | 03_ZONE_security_zone_strip | lime title + Z1..Z4 zone status dots with labels + 4-segment perimeter map with a white patrol marker; clip pings Z3 (hollow dot, white core, dimmed segment) |
| RADAR | 04_RADAR_security_radar_sweep | rotating radar icon (fading sweep wedge, crosshair, white blip, 3 s/rev), title, mock `BRG/RNG` readout, white `NODES 4/4`, bearing tick scale with white marker |
| ARM | 05_ARM_security_arm_mode | shield+padlock icon, title, outlined arming-mode chip (white text) cycling AWAY->HOME->NIGHT->AWAY with 3 mode pips |

Base: `controlbot-bigchip-v9c-glasses/01_panel@1x.png` (a15182a). Only rows y1..23 x1..238 change; row 24 hairline and the gold rim are untouched.
Palette: black, lime (120,252,0) = 0x7FFF00, white (248,252,248) + AA blends; RGB565-exact. Text is smooth AA (x4 supersampled coverage, VLW-style).
Fonts: JetBrains Mono (CURTN/TERM), Oxanium Bold (ZONE), Saira Semi Condensed Bold (RADAR), Exo 2 Bold (ARM), Noto CJK half-width kana (glyphs).
All readings (BRG/RNG, zone states, mode) are mock EX.DATA.

Per bar: `@1x.png`, `@3x_smooth.png` (Lanczos), `_header@3x_smooth.png` (rows 0..24), `@3x.mp4` (720x960 30fps H.264 yuv420p crf14 BT.709 faststart, 105 frames = 3.5 s).
`00_topbar_compare_5up@3x_smooth.png` stacks the 5 header crops labelled by BAR id.
Render `python3 sim/render_topbar_v2.py`; verify `python3 sim/verify_topbar_v2.py`. Mock only.
'''

def main():
 W.mkdir(parents=True,exist_ok=True)
 if OUT.exists():
  assert OUT.name=='topbar-v2';shutil.rmtree(OUT)
 OUT.mkdir(parents=True)
 base=np.array(Image.open(R.V9C).convert('RGB'))
 specs={};crops=[]
 for i,(vid,slug,name) in enumerate(VARIANTS,1):
  pre=f'{i:02d}_{vid}_{slug}'
  frames=[];fs=[]
  for t in range(NFR):
   h,s=header(vid,t);frames.append(R.compose(base,h));fs.append(s)
  frames=np.array(frames);np.save(W/f'{vid}_frames.npy',frames)
  st=STILL_T[vid];still=frames[st]
  Image.fromarray(still).save(OUT/f'{pre}@1x.png')
  sm=R.smooth(still);Image.fromarray(sm).save(OUT/f'{pre}@3x_smooth.png')
  Image.fromarray(sm[:75]).save(OUT/f'{pre}_header@3x_smooth.png');crops.append((vid,name,sm[:75]))
  R.encode_smooth(frames,OUT/f'{pre}@3x.mp4')
  specs[vid]={'bar_id':vid,'name':name,'prefix':pre,'still_frame':st,'still_spec':fs[st],'frames':NFR,
              'frame_specs':[{k:v for k,v in f.items() if k in ('mode','ping','typed','reveal_done','sweep_deg')} for f in fs]}
 parts=[]
 for vid,name,crop in crops:
  parts+= [R.label_strip(f'BAR {vid}  {name}',720),crop,np.full((12,720,3),40,np.uint8)]
 Image.fromarray(np.concatenate(parts[:-1],0)).save(OUT/'00_topbar_compare_5up@3x_smooth.png')
 (OUT/'TOPBAR_SPEC.json').write_text(json.dumps({'header_region_exclusive':{'y':[R.HY0,R.HY1],'x':[R.HX0,R.HX1]},'lime':LIME,'white':WHITE,
   'base':str(R.V9C.relative_to(ROOT)),'base_commit':'a15182a','bar_button':{'label':'BAR','cycle':[v[0] for v in VARIANTS],'value_max_chars':5,'glcd_value_px':{v[0]:6*len(v[0])-1 for v in VARIANTS}},
   'variants':specs},indent=1,default=float)+'\n')
 (OUT/'README.md').write_text(README)
 subprocess.run([sys.executable,str(ROOT/'sim/verify_topbar_v2.py')],check=True)

if __name__=='__main__':main()
