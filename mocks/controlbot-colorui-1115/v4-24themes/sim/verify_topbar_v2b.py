#!/usr/bin/env python3
"""Gates for mocks/topbar-v2b-colors (render_topbar_v2b.py). v2 gate set with the black/lime colour gate replaced by a
per-bar declared-palette gate, plus WCAG contrast, pairwise distinctness, v2-layout lock and police/strobe flash-rate gates."""
import sys
sys.dont_write_bytecode=True
import json,hashlib,subprocess,itertools
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage
sys.path.insert(0,str(Path(__file__).resolve().parent))
import render_topbar_v2b as V
import render_topbar_v2 as V2
R=V.R;OUT=V.OUT;W=V.W
G=[]
def ck(n,v,d=''):G.append({'check':n,'pass':bool(v),'detail':str(d)})
spec=json.loads((OUT/'TOPBAR_SPEC.json').read_text())
base=np.array(Image.open(R.V9C).convert('RGB'))
hdr=np.zeros((320,240),bool);hdr[R.HY0:R.HY1,R.HX0:R.HX1]=True
ck('v9c base matches commit a15182a',subprocess.run(['git','-C',str(R.ROOT),'diff','--quiet','a15182a','--',str(R.V9C.relative_to(R.ROOT))]).returncode==0)
for d in ('mocks/topbar-v1-matrix-security','mocks/topbar-v2','mocks/security-color-themes-v2'):
 ck(f'{d} untouched vs HEAD',subprocess.run(['git','-C',str(R.ROOT),'diff','--quiet','HEAD','--',d]).returncode==0 and
    subprocess.check_output(['git','-C',str(R.ROOT),'status','--porcelain','--',d]).strip()==b'')

def lum(c):
 c=np.asarray(c,float)/255;c=np.where(c<=0.04045,c/12.92,((c+0.055)/1.055)**2.4)
 return c[...,0]*0.2126+c[...,1]*0.7152+c[...,2]*0.0722
def contrast(a,b):
 la,lb=lum(a),lum(b);return (max(la,lb)+0.05)/(min(la,lb)+0.05)
def tri_dist(P,A,B):
 """distance of points P to triangle (0,A,B) = set {s*A+u*B : s,u>=0, s+u<=1}"""
 M=np.stack([A,B],1);sol=np.linalg.lstsq(M,P.T,rcond=None)[0];s,u=sol
 ok=(s>=0)&(u>=0)&(s+u<=1);best=np.where(ok,np.linalg.norm(P-(M@sol).T,axis=1),1e9)
 for E0,E1 in ((np.zeros(3),A),(np.zeros(3),B),(A,B)):
  d=E1-E0;t=np.clip(((P-E0)@d)/max(d@d,1e-9),0,1);best=np.minimum(best,np.linalg.norm(P-(E0+t[:,None]*d),axis=1))
 return best
from scipy.optimize import nnls
def palette_resid(px,pal):
 """distance of each pixel to the cone hull of black + any <=3 declared inks (weights>=0, sum<=1.05):
 covers AA edges, gradients between stops and ink-over-ink blends; evaluated on unique colours."""
 P=px.reshape(-1,3).astype(float);U,inv=np.unique(P,axis=0,return_inverse=True)
 pal=[np.array(c,float) for c in pal];combos=[c for k in (1,2,3) for c in itertools.combinations(range(len(pal)),k)]
 best=np.full(len(U),1e9)
 for j,u in enumerate(U):
  if u.max()==0:best[j]=0;continue
  for c in combos:
   M=np.stack([pal[i] for i in c],1);x,r=nnls(M,u)
   if x.sum()<=1.05 and r<best[j]:best[j]=r
   if best[j]<1:break
 return best[inv.ravel()]
def box_of(a,th=0.02):
 ys,xs=np.where(a>th);return (int(xs.min()),int(ys.min()),int(xs.max()),int(ys.max()))
def gap(b1,b2):
 dx=max(b2[0]-b1[2],b1[0]-b2[2])-1;dy=max(b2[1]-b1[3],b1[1]-b2[3])-1;return max(dx,dy)

bb=spec['bar_button']
ck('BAR ids: 11 bars, all <=5 chars, fit a 39px GLCD button value (<=29px)',len(bb['cycle'])==11 and all(len(i)<=5 and 6*len(i)-1<=29 for i in bb['cycle']),bb['glcd_value_px'])
MOTION={'CURTN':0.9,'TERM':0.2,'ZONE':0.9,'RADAR':0.99,'ARM':0.02,'LITEB':0.1,'BADGE':0.6,'NVG':0.99,'IRON':0.9,'ALARM':0.99,'PERIM':0.99}
stills={}
for vid,v in spec['variants'].items():
 pre=v['prefix'];pal=[tuple(c) for c in v['palette']]
 frames=np.load(W/f'{vid}_frames.npy')
 ck(f'{vid} 105 frames',frames.shape==(105,320,240,3))
 ck(f'{vid} outside header pixel-identical to v9c (all frames)',all(np.array_equal(f[~hdr],base[~hdr]) for f in frames))
 Hh=frames[:,R.HY0:R.HY1,R.HX0:R.HX1]
 ck(f'{vid} header RGB565-exact (all frames)',np.all(Hh[...,0]%8==0)&np.all(Hh[...,1]%4==0)&np.all(Hh[...,2]%8==0))
 ck(f'{vid} declared palette inks are RGB565-exact',all(c[0]%8==0 and c[1]%4==0 and c[2]%8==0 for c in pal),pal)
 res=max(palette_resid(f,pal).max() for f in Hh[::7])
 ck(f'{vid} header uses only its declared palette (+black, AA/gradient blends; resid<=14, every 7th frame)',res<=14,round(float(res),2))
 nb=Hh[V.STILL_T[vid]].reshape(-1,3).astype(float);nb=nb[nb.max(1)>40]
 hue_spread=len({tuple((c//64).astype(int)) for c in nb})
 ck(f'{vid} not single-hue (>=2 distinct inks in still)',hue_spread>=2 and len(pal)>=2,hue_spread)
 mv=np.mean([np.any(Hh[i]!=Hh[i+1]) for i in range(104)])
 ck(f'{vid} animates (moving pair fraction >= {MOTION[vid]})',mv>=MOTION[vid],round(mv,3))
 still=np.array(Image.open(OUT/f'{pre}@1x.png').convert('RGB'))
 ck(f'{vid} @1x == frame {v["still_frame"]}',still.shape==(320,240,3) and np.array_equal(still,frames[v['still_frame']]))
 sm=np.array(Image.open(OUT/f'{pre}@3x_smooth.png').convert('RGB'))
 ck(f'{vid} @3x_smooth 720x960 == Lanczos(@1x)',sm.shape==(960,720,3) and np.array_equal(sm,R.smooth(still)))
 ck(f'{vid} header crop == smooth rows 0..74',np.array_equal(np.array(Image.open(OUT/f'{pre}_header@3x_smooth.png').convert('RGB')),sm[:75]))
 h=still[R.HY0:R.HY1,R.HX0:R.HX1].astype(float);stills[vid]=h
 ss=v['still_spec'];els=ss['elements'];boxes={};allink=np.zeros((R.H,R.WD),bool)
 for e in els:
  a=R.text_alpha(e['text'],e['font'],e['px'],e['x'],e['baseline'])
  ua,P=R.text_alpha_unclipped(e['text'],e['font'],e['px'],e['x'],e['baseline'])
  ck(f'{vid} "{e["text"]}" not clipped',abs(ua.sum()-a.sum())<1e-6 and a.sum()>3,(round(ua.sum(),2),round(a.sum(),2)))
  if 'grad' in e:
   c0,c1,y0,y1=e['grad'];k=np.clip((np.arange(R.H)[:,None]-y0)/(y1-y0),0,1)[...,None]*np.ones((1,R.WD,1))
   colmap=np.array(c0,float)*(1-k)+np.array(c1,float)*k;inks=[c0,c1]
  else:colmap=np.broadcast_to(np.array(e['col'],float),(R.H,R.WD,3));inks=[e['col']]
  ring=ndimage.binary_dilation(a>0,iterations=2)&~(a>0)
  bgest=np.median(h[ring],axis=0) if ring.any() else np.zeros(3)
  want=a[...,None]*colmap+(1-a[...,None])*bgest;core=a>=0.6
  frac=np.mean(np.linalg.norm(h[core]-want[core],axis=1)<=28)
  ck(f'{vid} "{e["text"]}" ink present as declared (AA composite over local bg, >=95% of core px)',frac>=0.95,round(float(frac),4))
  b=box_of(a);boxes[e['text']]=(b,e);allink|=a>0
  ring=ndimage.binary_dilation(a>0,iterations=2)&~(a>0)
  bgpx=h[ring];bgl=np.percentile(lum(bgpx),95) if len(bgpx) else 0
  worst=min(contrast(c,np.array([0,0,0]))*0+((max(lum(np.array(c,float)),bgl)+0.05)/(min(lum(np.array(c,float)),bgl)+0.05)) for c in inks)
  ck(f'{vid} "{e["text"]}" contrast >=4.5:1 vs its own background (95th-pct bg lum in 2px ring)',worst>=4.5,round(float(worst),2))
  ck(f'{vid} "{e["text"]}" >=2px from header edges',b[0]>=2 and b[1]>=2 and b[2]<=R.WD-3 and b[3]<=R.H-3,b)
  ck(f'{vid} "{e["text"]}" >=9.5px (readable at 1x)',e['px']>=9.5,e['px'])
  if 'box' in e:
   x0,y0,x1,y1=e['box'];inner=(b[0]-x0,b[1]-y0,x1-1-b[2],y1-1-b[3])
   ck(f'{vid} "{e["text"]}" >=2px inside its chip',min(inner)>=2,inner)
 for name,ob in ss['obstacles'].items():
  for t,(b,e) in boxes.items():
   if 'box' in e and name=='chip':continue
   ck(f'{vid} "{t}" clears {name} by >=2px',gap(b,ob)>=2,(b,ob,gap(b,ob)))
 ks=list(boxes)
 for i in range(len(ks)):
  for j in range(i+1,len(ks)):
   ck(f'{vid} "{ks[i]}" vs "{ks[j]}" >=2px',gap(boxes[ks[i]][0],boxes[ks[j]][0])>=2,gap(boxes[ks[i]][0],boxes[ks[j]][0]))
 if ss.get('halo'):
  ring=ndimage.binary_dilation(allink,iterations=1)&~allink
  ck(f'{vid} 1px ring around text pure black',np.all(h[ring]==0),int(np.sum(np.any(h[ring]!=0,-1))))
 if ss.get('stray_check'):
  allowed=np.zeros((R.H,R.WD),bool)
  for (b,e) in boxes.values():allowed[max(0,b[1]-1):b[3]+2,max(0,b[0]-1):b[2]+2]=True
  for ob in ss['obstacles'].values():allowed[max(0,ob[1]-1):ob[3]+2,max(0,ob[0]-1):ob[2]+2]=True
  if ss.get('text_on_fill'):
   pxx=np.arange(R.WD)[None,:]+0.5;pyy=np.arange(R.H)[:,None]+0.5;allowed|=pxx<V.edge_x(pyy)+1
  stray=np.any(h!=0,-1)&~allowed
  ck(f'{vid} no stray pixels outside elements',not stray.any(),int(stray.sum()))
 titles=[e['text'] for e in els if e['kind']=='title']
 ck(f'{vid} still shows full "SECURITYbot EX.DATA" (exact case, EX.DATA visible)',titles==[R.TITLE])
 fsp=v['frame_specs']
 if vid=='TERM':ck('TERM still: fully typed + cursor on',ss['typed']==19 and ss['cursor']['on'])
 if vid=='ARM':ck('ARM still AWAY; clip cycles HOME/NIGHT/AWAY',ss['mode']=='AWAY' and {f['mode'] for f in fsp}=={'HOME','NIGHT','AWAY'})
 if vid=='CURTN':ck('CURTN still: wipe complete',ss['reveal_done'])
 if vid=='ZONE':ck('ZONE still all zones steady; clip pings Z3',not ss['ping'] and any(f['ping'] for f in fsp))
 if vid=='BADGE':ck('BADGE clip shows UNIT 01 and DISPATCH',{f['unit'] for f in fsp}=={'UNIT 01','DISPATCH'})
 # flash-rate gates (police light bar + alarm strobe): 2..4 Hz, never harsh
 def hz(sig):
  on=sig>(sig.min()+sig.max())/2;rises=np.sum(on[1:]&~on[:-1]);return rises/(104/30)
 if vid=='LITEB':
  x0,y0,x1,y1=V.POD_L;rs=Hh[:,y0+4:y1-4,x0+4:x1-4,0].mean(axis=(1,2))
  x0,y0,x1,y1=V.POD_R;bs=Hh[:,y0+4:y1-4,x0+4:x1-4,2].mean(axis=(1,2))
  ck('LITEB red pod flashes 1.5..4 Hz',1.5<=hz(rs)<=4,round(hz(rs),2));ck('LITEB blue pod flashes 1.5..4 Hz',1.5<=hz(bs)<=4,round(hz(bs),2))
  ck('LITEB red and blue alternate (never both lit)',not any(f['red_on'] and f['blue_on'] for f in fsp))
 if vid=='ALARM':
  rs=Hh[:,0,40:200,0].astype(float).mean(1);ck('ALARM strobe 1.5..4 Hz',1.5<=hz(rs)<=4,round(hz(rs),2))
 mp4=OUT/f'{pre}@3x.mp4'
 pr=json.loads(subprocess.check_output(['ffprobe','-v','error','-select_streams','v:0','-count_frames','-show_entries',
   'stream=codec_name,pix_fmt,width,height,r_frame_rate,nb_read_frames,color_space,color_primaries,color_transfer','-of','json',str(mp4)]))['streams'][0]
 ok=(pr['codec_name']=='h264' and pr['pix_fmt']=='yuv420p' and pr['width']==720 and pr['height']==960 and pr['r_frame_rate']=='30/1'
     and int(pr['nb_read_frames'])==105 and pr.get('color_space')=='bt709' and pr.get('color_primaries')=='bt709' and pr.get('color_transfer')=='bt709')
 ck(f'{vid} mp4 720x960 30fps h264 yuv420p bt709 105f (3.5s)',ok,pr)
 head=mp4.read_bytes()[:4096];ck(f'{vid} mp4 faststart',head.find(b'moov')!=-1 and (head.find(b'mdat')==-1 or head.find(b'moov')<head.find(b'mdat')))
 raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(mp4),'-vf','select=eq(n\\,52)','-vsync','0','-frames:v','1','-f','rawvideo','-pix_fmt','rgb24','-'])
 dec=np.frombuffer(raw,np.uint8).reshape(960,720,3).astype(float);ref=R.smooth(frames[52]).astype(float)
 psnr=10*np.log10(255**2/np.mean((dec-ref)**2));ck(f'{vid} mp4 frame 52 matches source (PSNR>=31)',psnr>=31,round(psnr,2))
 # v2 layout lock for the recoloured five: same geometry (non-black coverage) as v2 in every 5th frame
 if v['group']=='recolor':
  ex=np.zeros((R.H,R.WD),bool)
  if vid in('ZONE','RADAR'):     # enlarged labels: exclude old+new label boxes (dilated 2) from the lock
   for sp in (v['still_spec'],V2.header(vid,v['still_frame'])[1]):
    for e in sp['elements']:
     if e['kind']=='title':continue
     b=box_of(R.text_alpha(e['text'],e['font'],e['px'],e['x'],e['baseline']));ex[max(0,b[1]-2):b[3]+3,max(0,b[0]-2):b[2]+3]=True
   for t in range(0,105,5):
    for sp in (V.header(vid,t)[1],V2.header(vid,t)[1]):
     for e in sp['elements']:
      if e['kind']=='title':continue
      b=box_of(R.text_alpha(e['text'],e['font'],e['px'],e['x'],e['baseline']));ex[max(0,b[1]-2):b[3]+3,max(0,b[0]-2):b[2]+3]=True
  worst=0
  for t in range(0,105,5):
   m2=np.any(V2.header(vid,t)[0]>12,-1);mb=np.any(Hh[t]>12,-1)
   worst=max(worst,np.mean((m2!=mb)&~ex))
  ck(f'{vid} layout+animation locked to v2 (coverage mismatch <=1.0% outside resized labels)',worst<=0.01,round(float(worst),4))
# pairwise distinctness across all 11 stills
def sig(h):
 P=h.reshape(-1,3);m=P.max(1)>30;Q=P[m]
 if len(Q)==0:return np.zeros(12),0,0
 mx=Q.max(1);mn=Q.min(1);ch=(mx-mn)/255
 r,g,b=Q[:,0],Q[:,1],Q[:,2];hue=np.degrees(np.arctan2(np.sqrt(3)*(g-b),2*r-g-b))%360
 hh=np.histogram(hue,bins=12,range=(0,360),weights=ch+1e-6)[0];hh=hh/hh.sum()
 return hh,m.mean(),lum(h).mean()
ids=list(stills);mind=9;pair=None;rows=[]
for a,b in itertools.combinations(ids,2):
 ha,ca,la=sig(stills[a]);hb,cb,lb=sig(stills[b])
 d=0.5*np.abs(ha-hb).sum()+abs(ca-cb)+abs(la-lb)+np.abs(stills[a]-stills[b]).mean()/255
 rows.append((a,b,round(float(d),3)))
 if d<mind:mind,pair=d,(a,b)
ck('pairwise distinctness: every pair of the 11 stills differs (colour+layout signature >=0.35)',mind>=0.35,(pair,round(float(mind),3)))
for fn,n in (('00_topbar_compare_5up@3x_smooth.png',5),('00_topbar_compare_extra_6up@3x_smooth.png',6)):
 ck(f'{fn} 720 wide, {n} labelled crops',Image.open(OUT/fn).size==(720,n*(30+75)+(n-1)*12))
n_ok=sum(g['pass'] for g in G)
sums={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(OUT.iterdir()) if p.name not in('SHA256SUMS','VERIFICATION.json')}
(OUT/'VERIFICATION.json').write_text(json.dumps({'passed':n_ok,'total':len(G),'distinctness_pairs':rows,'checks':G},indent=1)+'\n')
(OUT/'SHA256SUMS').write_text(''.join(f'{h}  {n}\n' for n,h in sums.items()))
for g in G:
 if not g['pass']:print('FAIL',g['check'],g['detail'])
print(f'{n_ok}/{len(G)} gates pass')
sys.exit(0 if n_ok==len(G) else 1)
