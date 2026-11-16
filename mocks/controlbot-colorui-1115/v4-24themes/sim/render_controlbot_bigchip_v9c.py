#!/usr/bin/env python3
"""v9c: v9 panel + polished eye-box glasses filling grey void; left theme-bars SCAN/VOL/BRIGHT;
right 2x2 single-cycle badges: CHIME, TIME, GLASSES, TONE.
GLASSES: NONE->MORPH->SMITH->SURF->INTEL. Lenses REPLACE eyes; iconic silhouettes free of 20x20 box; centroids on eye centres, AA supersampled. Panel = v9."""
import sys
sys.dont_write_bytecode=True
import json,hashlib,subprocess,re,shutil,math
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
W=Path('/tmp/secbot_build/controlbot_v9c')
OUT=ROOT/'mocks/controlbot-bigchip-v9c-glasses'
V5PNG=ROOT/'mocks/controlbot-bigchip-v5-reference/01_bigchip@1x.png'
V5MP4=ROOT/'mocks/controlbot-bigchip-v5-reference/01_bigchip_theme_tap@3x.mp4'
FONT=Path('/workspace/cyd-portrait-wt/src/live/lv_font.h')
FACE=(47,72,194,218)
# Grey void inscribed (fully grey under face): x43-198 y218-292.
# Fill with even 2px margins -> panel exclusive (45,220,197,291) = 152x71
VOID=(43,218,199,293)          # exclusive-ish grey bounds used for margin math
PANEL=(45,220,197,291)         # exclusive end; painted region
COL_LABEL=(148,73,173)
COL_VALUE=(222,146,255)
COL_SEG_OFF=(33,12,49)
COL_SEG_CAP=(140,154,165)
COL_PLATE=(8,8,8)
COL_KEY=(148,73,173)
SCANBG_CYCLE=['B.RAIN','S.RAIN','BOTS','GIBSON','FLYBY','BOUNCE']
GLASSES_CYCLE=['NONE','MORPH','SMITH','SURF','INTEL']  # short EX.DATA values that fit
TONE_CYCLE=['BEEP','8BIT','SIREN']
CHIME_CYCLE=['ON','OFF']
TIME_CYCLE=['RTC','NTP']
# Native eye squares on big face (from v5 CPP): 20x20 at (88,109) and (132,109)
EYE_L=(88,109,108,129)   # inclusive
EYE_R=(132,109,152,129)
EYE_BAND=(65,100,174,145)  # traced silhouette region inside face (inclusive)
# v16 guard palette
SHEEN=(150,166,190); DARK=[(46,54,70),(22,26,36),(8,10,16)]; STREAK=(78,90,112); GLINT=(255,255,255)

# Layout inside PANEL 152x71 (x45-196, y220-290):
# gap 2 between cols; left bars 74 wide; right 2x2 buttons fill remaining
# PANEL paint: x0=45,y0=220,w=152,h=71
# Left col x46..119 (74), right x122..195 (74), 2px gap at 120-121
# 3 bars h=22 with 1px gaps: y221,244,267
# 2x2 buttons: each ~36x33 — aim fill
TARGETS={
 'THEME_FACE':(65,88,174,197),
 'SCANBG':(46,221,113,242),   # 68x22
 'VOL':(46,244,113,265),
 'BRIGHT':(46,267,113,288),
 'CHIME':(116,221,154,254),   # 39x34
 'TIME':(157,221,195,254),    # 39x34
 'GLASSES':(116,256,154,289),
 'TONE':(157,256,195,289),
}

def fonts():
 t=FONT.read_text()
 g=t.split('LV_GLCD[95 * 5]',1)[1].split('{',1)[1].split('};',1)[0]
 f=t.split('LV_F35[95]',1)[1].split('{',1)[1].split('};',1)[0]
 return [int(v,16) for v in re.findall(r'0x[0-9a-fA-F]+',g)],[int(v,16) for v in re.findall(r'0x[0-9a-fA-F]+',f)]
GL,F35=fonts()

def glyph(s,font='glcd'):
 if font=='glcd':
  m=np.zeros((7,6*len(s)-1),bool)
  for i,ch in enumerate(s):
   for cx,bits in enumerate(GL[(ord(ch)-32)*5:(ord(ch)-31)*5]):
    for cy in range(7):
     if bits>>cy&1:m[cy,6*i+cx]=True
 else:
  m=np.zeros((5,4*len(s)-1),bool)
  for i,ch in enumerate(s):
   g=F35[ord(ch)-32]
   for k in range(15):
    if g>>(14-k)&1:m[k//3,4*i+k%3]=True
 return m

def text(im,x,y,s,col,font='glcd'):
 m=glyph(s,font);h,w=m.shape
 assert 0<=x and x+w<=240 and 0<=y and y+h<=320,(x,y,s,w,h)
 yy,xx=np.where(m);im[y+yy,x+xx]=col
 return True

def to_rgb565(im):
 r=(im[:,:,0].astype(np.uint16)>>3)<<3
 g=(im[:,:,1].astype(np.uint16)>>2)<<2
 b=(im[:,:,2].astype(np.uint16)>>3)<<3
 out=im.copy();out[:,:,0]=r;out[:,:,1]=g;out[:,:,2]=b
 return out

def load_bases():
 ice=np.array(Image.open(V5PNG).convert('RGB'))
 raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(V5MP4),'-f','rawvideo','-pix_fmt','rgb24','-'])
 dec=np.frombuffer(raw,np.uint8).reshape(-1,960,720,3)
 blue=dec[60][::3,::3].copy()
 np.save(W/'v5_ice.npy',ice);np.save(W/'v5_blue.npy',blue)
 return ice,blue

def draw_meter(im,x,y,w,h,lit_n,n_seg=8):
 im[y:y+h,x:x+w]=0
 im[y:y+h,x:x+2]=COL_SEG_CAP;im[y:y+h,x+w-2:x+w]=COL_SEG_CAP
 inner_x=x+3;inner_w=w-6;gap=1;seg_w=max(2,(inner_w-(n_seg-1)*gap)//n_seg)
 for i in range(n_seg):
  sx=inner_x+i*(seg_w+gap)
  im[y:y+h,sx:sx+seg_w]=COL_VALUE if i<lit_n else COL_SEG_OFF

def draw_theme_bar(im,x,y,w,h,label,value,lit_n,n_seg=8,value_box=False,accent=None):
 assert h>=20 and w>=40,(w,h)
 im[y:y+h,x:x+w]=COL_PLATE
 im[y,x:x+w]=COL_KEY;im[y+h-1,x:x+w]=COL_KEY;im[y:y+h,x]=COL_KEY;im[y:y+h,x+w-1]=COL_KEY
 ty=y+3
 lw=glyph(label).shape[1];vw=glyph(value).shape[1]
 assert 3+lw+3+vw+3<=w, f'{label}/{value} need {3+lw+3+vw+3} in {w}'
 text(im,x+3,ty,label,COL_LABEL)
 vx=x+w-3-vw
 if value_box and accent is not None:
  im[ty:ty+7,vx-1:vx+vw+1]=accent
  text(im,vx,ty,value,(255,255,255))
 else:
  text(im,vx,ty,value,COL_VALUE)
 draw_meter(im,x+3,y+12,w-6,5,lit_n,n_seg=n_seg)

def draw_cycle_btn(im,x,y,w,h,label,value,on=True):
 """Single tap-to-cycle button: label + one lv_badge value (native badge chrome)."""
 assert h>=30 and w>=30,(w,h)
 im[y:y+h,x:x+w]=COL_PLATE
 im[y,x:x+w]=COL_KEY;im[y+h-1,x:x+w]=COL_KEY;im[y:y+h,x]=COL_KEY;im[y:y+h,x+w-1]=COL_KEY
 lw=glyph(label).shape[1]
 assert 3+lw+3<=w, f'label {label} need {3+lw+3} in {w}'
 text(im,x+(w-lw)//2,y+3,label,COL_LABEL)
 # badge
 by=y+12;bh=h-12-3;bx=x+3;bw=w-6
 fill=(222,146,255) if on else (32,32,32)
 frame=(255,255,255) if on else (96,96,96)
 ink=(255,255,255) if on else (176,176,176)
 im[by:by+bh,bx:bx+bw]=fill
 im[by,bx:bx+bw]=frame;im[by+bh-1,bx:bx+bw]=frame;im[by:by+bh,bx]=frame;im[by:by+bh,bx+bw-1]=frame
 vw=glyph(value).shape[1]
 assert bw>=vw+4 and bh>=7+4, f'value {value} {vw}px in badge {bw}x{bh}'
 text(im,bx+(bw-vw)//2,by+(bh-7)//2,value,ink)

# --- glasses drawn only in EYE_BAND, centered on native eye centers ---



def eye_centers():
 lx=(EYE_L[0]+EYE_L[2])//2; ly=(EYE_L[1]+EYE_L[3])//2
 rx=(EYE_R[0]+EYE_R[2])//2; ry=(EYE_R[1]+EYE_R[3])//2
 return (lx,ly),(rx,ry)

def _face_fill_at(base,x,y):
 for dy,dx in ((-8,0),(-4,0),(0,-12),(0,12),(8,0),(-12,10)):
  yy,xx=y+dy,x+dx
  if 0<=yy<320 and 0<=xx<240:
   c=base[yy,xx]
   if not (c[0]<20 and c[1]<20 and c[2]<20):
    return tuple(int(v) for v in c)
 return (0,250,255)

def _erase_eyes(im,base):
 for (x0,y0,x1,y1) in (EYE_L,EYE_R):
  fill=_face_fill_at(base,(x0+x1)//2,y0-2)
  im[y0:y1+1,x0:x1+1]=fill

# Silhouette traces from refs (built by /tmp/secbot_build/controlbot_v9c/build_silhouettes.py)
SIL_DIR=Path(__file__).resolve().parent/'assets'/'glasses_v9c'
# Mapping: MORPH=TL curved gloss, SMITH=BR flattop, SURF=BL diagonal streaks, INTEL=round open rims
SIL_KEYS={'MORPH':'MORPH','SMITH':'SMITH','SURF':'SURF','INTEL':'INTEL'}

def _load_sil(style):
 z=np.load(SIL_DIR/f'{style}_blit.npz')
 return z

def apply_glasses(im, style):
 """Blit traced ref silhouettes (solid black + crisp white highlights). INTEL = open rims, cyan shows through."""
 if style=='NONE':
  return im
 out=im.copy(); base=im
 _erase_eyes(out, base)
 if style not in SIL_KEYS:
  return out
 z=_load_sil(SIL_KEYS[style])
 body=z['body']; hi=z['hi']; y0=int(z['y0']); x0=int(z['x0'])
 open_rims=bool(z['open_rims']) if 'open_rims' in z.files else (style=='INTEL')
 SS=int(z['ss']) if 'ss' in z.files else 4
 body_ss=z['body_ss']; hi_ss=z['hi_ss']
 # Supersampled coverage -> soft AA edge when downsampling
 h,w=body.shape
 # rebuild SS from stored; downsample mean for AA alpha
 cov_b=body_ss.astype(np.float64)/255.0
 cov_h=hi_ss.astype(np.float64)/255.0
 # if shapes are HxW at SS, reshape
 if cov_b.ndim==2:
  Hh,Ww=cov_b.shape
  assert Hh==h*SS and Ww==w*SS,(Hh,Ww,h,w,SS)
  alpha_b=cov_b.reshape(h,SS,w,SS).mean(axis=(1,3))
  alpha_h=cov_h.reshape(h,SS,w,SS).mean(axis=(1,3))
 else:
  alpha_b=body.astype(np.float64); alpha_h=hi.astype(np.float64)

 # Drop shadow: shift body alpha down-right, darken face under
 sh=np.zeros_like(alpha_b)
 sh[1:,1:]=alpha_b[:-1,:-1]*0.45
 fill=np.array(_face_fill_at(base,98,115),np.float64)

 # Compose onto face region
 fy0,fx0=y0,x0
 # Clip to face
 face=np.zeros((320,240),bool); face[88:198,65:175]=True
 for yy in range(h):
  for xx in range(w):
   Y,X=fy0+yy,fx0+xx
   if not (0<=Y<320 and 0<=X<240): continue
   if not face[Y,X]: continue
   # shadow first
   a=float(sh[yy,xx])
   if a>0.02:
    out[Y,X]=np.clip(out[Y,X].astype(np.float64)*(1-0.35*a),0,255)
   ab=float(alpha_b[yy,xx]); ah=float(alpha_h[yy,xx])
   if ab>0.02:
    # solid black body
    col=np.array([0,0,0],np.float64)
    out[Y,X]=np.clip(out[Y,X].astype(np.float64)*(1-ab)+col*ab,0,255)
   if ah>0.02:
    col=np.array([255,255,255],np.float64)
    out[Y,X]=np.clip(out[Y,X].astype(np.float64)*(1-ah)+col*ah,0,255)

 # RGB565 quantize face fill only
 out[88:198,65:175]=to_rgb565(out[88:198,65:175])
 # re-stamp crisp white highlights after quantize so they survive RGB565
 for yy in range(h):
  for xx in range(w):
   Y,X=fy0+yy,fx0+xx
   if not (0<=Y<320 and 0<=X<240): continue
   if not face[Y,X]: continue
   if float(alpha_h[yy,xx])>=0.5:
    out[Y,X]=np.array([248,252,248],np.uint8)  # RGB565 white
 return out

def paint(base,scan_idx,chime_on,vol,bright,time_src,glasses,tone,blue=False):
 im=base.copy()
 # glasses first (face region) then panel
 im=apply_glasses(im,glasses)
 x0,y0,x1,y1=PANEL
 im[y0:y1,x0:x1]=COL_PLATE
 im[y0,x0:x1]=COL_KEY;im[y1-1,x0:x1]=COL_KEY;im[y0:y1,x0]=COL_KEY;im[y0:y1,x1-1]=COL_KEY
 scan_accent=(222,146,255)
 BH=22;BW=68
 draw_theme_bar(im,46,221,BW,BH,'SCAN',SCANBG_CYCLE[scan_idx],scan_idx+1,n_seg=8,value_box=True,accent=scan_accent)
 draw_theme_bar(im,46,244,BW,BH,'VOL',f'{vol}%',max(1,int(round(8*vol/100))),n_seg=8)
 draw_theme_bar(im,46,267,BW,BH,'BRIGHT',f'{bright}%',max(1,int(round(8*bright/100))),n_seg=8)
 # 2x2 cells 39x34. GLASSES label needs 47 > 40 — use SPECS (report).
 label_glasses='SPECS'  # GLASSES needs 47 > 39
 draw_cycle_btn(im,116,221,39,34,'CHIME','ON' if chime_on else 'OFF',on=chime_on)
 draw_cycle_btn(im,157,221,39,34,'TIME',time_src,on=True)
 draw_cycle_btn(im,116,256,39,34,label_glasses,glasses,on=(glasses!='NONE'))
 draw_cycle_btn(im,157,256,39,34,'TONE',tone,on=True)
 im[y0:y1,x0:x1]=to_rgb565(im[y0:y1,x0:x1])
 # glasses region also RGB565 for CYD honesty (face outside eye band untouched for NONE)
 return im

def encode_smooth(frames,mp4):
 ups=[np.array(Image.fromarray(f).resize((720,960),Image.Resampling.LANCZOS)) for f in frames]
 raw=W/'clip.rgb';raw.write_bytes(np.stack(ups).tobytes())
 subprocess.run(['ffmpeg','-v','error','-y','-f','rawvideo','-pixel_format','rgb24','-video_size','720x960','-framerate','30','-i',str(raw),
  '-c:v','libx264','-preset','slow','-crf','14','-pix_fmt','yuv420p',
  '-colorspace','bt709','-color_primaries','bt709','-color_trc','bt709',
  '-x264-params','colorprim=bt709:transfer=bt709:colormatrix=bt709',
  '-movflags','+faststart',str(mp4)],check=True)

README='''# CONTROLbot v9c — iconic glasses silhouettes

Panel/chrome pixel-identical to v9b. Only the SPECS face glasses change (bigger free silhouettes).

## Layout
- Grey void ~x43-198 y218-292. Panel (45,220)-(197,291): margins L/R=2, T/B=2.
- Left: SCAN / VOL / BRIGHT theme-bars (68x22).
- Right 2x2 cycle buttons (39x34): CHIME, TIME, SPECS, TONE.

## SPECS (glasses) — v9c traced iconic silhouettes
Native eyes erased to face cyan; silhouettes traced from refs (solid black + crisp white interior highlights), supersampled x4 AA, centroids on native eye centres (<=1px), ~82% face width (90px), inside face square.

Label **SPECS** (GLASSES too wide). Values: NONE / MORPH / SMITH / SURF / INTEL.
- NONE: native v5 eyes (face pixel-identical)
- MORPH: magnific ref top-right AVIATOR (double bridge, teardrop lenses)
- SMITH: magnific ref bottom-right flat-top wayfarer (hero still)
- SURF: magnific ref bottom-left two-diagonal-streak wayfarer
- INTEL: vecteezy round open rims (clear lenses, cyan shows through)

## Global NVS
SPECS choice is global NVS -- every bot on every screen wears it. (Home-board mock not in this pass.)

## Other cycles
CHIME ON/OFF; TIME RTC/NTP; TONE BEEP/8BIT/SIREN; SCAN B.RAIN..BOUNCE.

## Font / CYD
LV_GLCD 5x7. Panel RGB565. Procedural draw + prebuilt silhouette blit masks.

## Limitations
Button cells 39x34. Label SPECS because GLASSES needs 47px. Bars 68x22. Scripted mock. No commit/push.
'''

def main():
 W.mkdir(parents=True,exist_ok=True)
 if OUT.exists():
  for p in OUT.iterdir():
   if p.is_dir():shutil.rmtree(p)
   else:p.unlink()
 else:OUT.mkdir(parents=True)
 ice,blue=load_bases()
 # Still NONE / ICE
 still=paint(ice,0,True,40,80,'RTC','SMITH','BEEP',blue=False)  # hero shows non-NONE
 Image.fromarray(still).save(OUT/'01_panel@1x.png')
 Image.fromarray(still.repeat(3,0).repeat(3,1)).save(OUT/'01_panel@3x.png')
 Image.fromarray(still).resize((720,960),Image.Resampling.LANCZOS).save(OUT/'01_panel@3x_smooth.png')
 # Stills per glasses style (ICE theme)
 glasses_stills=[]
 for g in GLASSES_CYCLE:
  fr=paint(ice,0,True,40,80,'RTC',g,'BEEP',blue=False)
  tag='02_glasses_'+g.lower()
  Image.fromarray(fr).save(OUT/f'{tag}@1x.png')
  Image.fromarray(fr).resize((720,960),Image.Resampling.LANCZOS).save(OUT/f'{tag}@3x_smooth.png')
  glasses_stills.append(fr)
 # 1x5 compare strip @3x nearest of face/panel crop? Full frames side by side is huge;
 # use face+panel crop scaled 2x for strip readability, but brief says 1x5 compare strip — use @1x faces concatenated at 1x then nearest 3x of strip
 crops=[]
 for fr in glasses_stills:
  crops.append(fr[72:291,47:197])  # face through panel
 gap=np.zeros((crops[0].shape[0],4,3),np.uint8)
 strip=crops[0]
 for c in crops[1:]:
  strip=np.concatenate([strip,gap,c],1)
 Image.fromarray(strip).save(OUT/'00_glasses_compare_1x5@1x.png')
 Image.fromarray(strip.repeat(3,0).repeat(3,1)).save(OUT/'00_glasses_compare_1x5@3x.png')
 Image.fromarray(strip).resize((strip.shape[1]*3,strip.shape[0]*3),Image.Resampling.LANCZOS).save(OUT/'00_glasses_compare_1x5@3x_smooth.png')
 # zoomed face-only closeup strip
 faces=[]
 for fr in glasses_stills:
  faces.append(fr[88:198,65:175])
 gapf=np.zeros((faces[0].shape[0],4,3),np.uint8)
 fstrip=faces[0]
 for c in faces[1:]:
  fstrip=np.concatenate([fstrip,gapf,c],1)
 Image.fromarray(fstrip).resize((fstrip.shape[1]*3,fstrip.shape[0]*3),Image.Resampling.LANCZOS).save(OUT/'00_glasses_closeup_1x5@3x_smooth.png')
 Image.fromarray(fstrip.repeat(3,0).repeat(3,1)).save(OUT/'00_glasses_closeup_1x5@3x.png')
 # Clip: ICE hold -> BLUE theme (NONE) -> cycle NONE,MORPH,SMITH,SURF,INTEL on BLUE (96 frames)
 # 0-23 ICE NONE, 24-39 BLUE NONE, then 40-95 cycle 5 styles ~11 frames each
 frames=[]
 for n in range(96):
  is_blue=n>=24
  base=blue if is_blue else ice
  if n<40: g='NONE'
  else:
   g=GLASSES_CYCLE[min(4,(n-40)//11)]
  frames.append(paint(base,0,True,40,80,'RTC',g,'BEEP',blue=is_blue))
 fr=np.array(frames);np.save(W/'clip.npy',fr)
 encode_smooth(fr,OUT/'01_panel_theme_and_glasses@3x.mp4')
 Image.fromarray(frames[32]).save(OUT/'03_theme_blue_none@1x.png')
 # Margins
 vx0,vy0,vx1,vy1=VOID
 px0,py0,px1,py1=PANEL
 margins={'left':px0-vx0,'right':vx1-px1,'top':py0-vy0,'bottom':vy1-py1}
 (OUT/'INTERACTION_SPEC.json').write_text(json.dumps({
  'prototype_only':True,
  'void':VOID,'panel_exclusive':PANEL,'margins_px':margins,
  'targets_inclusive':TARGETS,
  'glasses_label':'SPECS (GLASSES GLCD 47px > 39px cell)',
  'glasses_cycle':GLASSES_CYCLE,
  'tone_cycle':TONE_CYCLE,'scan_bg_cycle':SCANBG_CYCLE,
  'eye_centers':{'L':((EYE_L[0]+EYE_L[2])//2,(EYE_L[1]+EYE_L[3])//2),'R':((EYE_R[0]+EYE_R[2])//2,(EYE_R[1]+EYE_R[3])//2)},
  'eye_band_inclusive':EYE_BAND,
  'lens_sizes_px':{'MORPH':'aviator ~90x36 frame','SMITH':'flattop wayfarer ~90x33','SURF':'streak wayfarer ~90x32','INTEL':'round open ~90x38'},
  'theme':'center face tap ICE->BLUE; no THEME control',
  'demo':'theme tap then GLASSES NONE->MORPH->SMITH->SURF->INTEL',
  'font':'LV_GLCD 5x7',
  'target_heights_honest':{k:{'w':TARGETS[k][2]-TARGETS[k][0]+1,'h':TARGETS[k][3]-TARGETS[k][1]+1} for k in TARGETS if k!='THEME_FACE'},
  'resistive_ideal':36,
  'global_nvs_glasses':'SPECS choice is global NVS — every bot on every screen wears it (Home-board mock not in this pass).',
  'silhouette_mapping':{'MORPH':'magnific aviator TR','SMITH':'magnific flattop BR','SURF':'magnific diagonal-streak BL','INTEL':'vecteezy round open rims'},
 },indent=2)+'\n')
 (OUT/'README.md').write_text(README)
 subprocess.run([sys.executable,str(ROOT/'sim/verify_controlbot_bigchip_v9c.py')],check=True)

if __name__=='__main__':main()
