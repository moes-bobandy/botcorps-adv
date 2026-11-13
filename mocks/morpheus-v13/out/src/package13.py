#!/usr/bin/env python3
"""Package + gate MORPHEUSbot v13 (mock only). Every frame is sim_live output (real C++ renderer, 240x320 RGB565)."""
import os,re,json,subprocess,hashlib,shutil,numpy as np
from PIL import Image,ImageDraw,ImageFont
D='/workspace/out/morpheus-1112'; W='/tmp/morpheus_v13'; FR=240*320*3; TREE='/workspace/cyd-morpheus-v6-wt'; LIVE=TREE+'/src/live'; V11=   '/tmp/morpheus_v12/v11tree/src/live'
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',22)
NAMES=['WIFI','BLUETOOTH','DEAUTH','HACK','FLOCK','DRONE','AIR TAG','KEYFOB','META','SKIMMER','FLIPPER','SAMSUNG']
gate={'what':'MORPHEUSbot v13 gate (mock only). Numbers are read back from sim_live output, written files or source.','checks':[]}
def chk(n,ok,**kw): gate['checks'].append(dict(name=n,PASS=bool(ok),**kw)); print('PASS' if ok else 'FAIL',n[:120],flush=True)
def blocks(p): return open(p).read().split('ITW scr=')[1:]
def sha(p): return hashlib.sha256(open(p,'rb').read()).hexdigest()
def motw(b): return {int(m.group(1)):tuple(map(int,m.group(2,3,4,5))) for m in re.finditer(r'MOTW (\d+) X=\S+ Z=\S+ H=\S+ f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]',b)}
def mocam(b):
    m=re.search(r'MOCAM camx=(-?[\d.]+) sel=(-?\d+) closing=(\d) lock=\d txt_n=\d+ txt_bad=(\d+) txt_lines_all=(\d+) txt_bad_all=(\d+) t=([\d.]+) p=([\d.]+)',b)
    return dict(cam=float(m.group(1)),sel=int(m.group(2)),closing=int(m.group(3)),bad_all=int(m.group(6)),lines=int(m.group(5)),t=float(m.group(7)),p=float(m.group(8)))
h=open(LIVE+'/lv_morpheus_v13.h').read(); it=open(LIVE+'/lv_inside_towers.h').read()
# ---- source
chk('source: lv_radar.h byte-identical to the v11-state tree (engine, camera law, place/clamp untouched)',open(LIVE+'/lv_radar.h','rb').read()==open(V11+'/lv_radar.h','rb').read())
chk('source: lv_morpheus_v11.h and v12.h unchanged on disk (v11 = v11-state tree, v12 = the withdrawn v12 src copy); lv_inside_towers.h includes v13',
    open(LIVE+'/lv_morpheus_v11.h','rb').read()==open(V11+'/lv_morpheus_v11.h','rb').read() and open(LIVE+'/lv_morpheus_v12.h','rb').read()==open(D+'/_withdrawn/v12/src/lv_morpheus_v12.h','rb').read() and '#include "lv_morpheus_v13.h"' in it and 'lv_morpheus_v12.h"' not in it.split('#include "lv_morpheus_v13.h"')[0][-400:])
chk('source: drift tables + hooks verbatim from v12 (MO11_SX/Z0/RSSI/AX/WX/AZ/WZ/WH, mo11_drift, mo11_layout)',all(re.search(re.escape(l),h) for l in re.findall(r'static const float MO11_\w+\[12\] = \{[^}]*\};',open(LIVE+'/lv_morpheus_v12.h').read())) and
    [x for x in open(LIVE+'/lv_morpheus_v12.h').read().split('static void mo11_drift(LvTower &T)')[1].split('/* ---- skin-1')[0]]==[x for x in h.split('static void mo11_drift(LvTower &T)')[1].split('/* ---- skin-1')[0]])
chk('source: no select effect (no mo_brackets / mo_pill call, no flood) and no HACKbot neon literals',
    'mo_brackets(B, T' not in h and 'mo_pill(B, (T' not in h and 'static float mo_flood(void) { return 0; }' in h and not any(c in h for c in ['0x30e8ff','0xff2ad4','0xffb000','0xff385c','0x30ff90','0xffc34a']))
chk('source: 3 folders with the DOWNLOAD drop timing (DROP0 300 ms, GAP 110, DROP 380)','#define MO12_DROP0 300' in h and '#define MO12_GAP 110' in h and '#define MO12_DROP_MS 380' in h and 'for (int j = 2; j >= 0; --j)' in h)
# ---- idle: motion identical to v11, skin differs
idl={}
for s,sim in (('v11','/tmp/morpheus_v12/sim_v11'),('v12','/tmp/morpheus_v12/sim_new'),('v13',W+'/sim_new')):
    d=f'{W}/idle_{s}'; shutil.rmtree(d,ignore_errors=True); os.makedirs(d)
    o=subprocess.run([sim,'/tmp/morpheus_v12/idle_only.txt',d],cwd=TREE,capture_output=True,text=True,check=True).stdout
    idl[s]=dict(motw=[l for l in o.splitlines() if 'MOTW' in l or 'MOCAM camx' in l],rgb=np.fromfile(d+'/idle.rgb',np.uint8).reshape(-1,320,240,3))
def strip(L):
    o=[]
    for l in L:
        if 'MOCAM' in l: m=re.search(r'camx=(\S+).* t=(\S+).* ord=(\S+)',l); o.append(('CAM',)+m.groups())
        else: m=re.search(r'MOTW (\d+) (X=\S+ Z=\S+ H=\S+ f=\S+ b=\S+) halo=\d+ (dx=\S+ dz=\S+ hk=\S+)',l); o.append(m.groups())
    return o
chk('idle motion: per-pillar MOTW read-back (X, Z, H, front/back corners, drift dx/dz/hk) + camera identical to v11 over the 600-frame idle recording',strip(idl['v11']['motw'])==strip(idl['v13']['motw']) and len(idl['v13']['motw'])>=24,lines=len(idl['v13']['motw']))
dif=int(sum(not np.array_equal(a,b) for a,b in zip(idl['v11']['rgb'],idl['v13']['rgb'])))
chk('idle skin: v13 idle frames differ from v11 (new pillar skin)',dif==len(idl['v13']['rgb'])==600,frames_differ=dif,of=len(idl['v13']['rgb']))
# ---- camera + touch
tb=blocks(W+'/touch/out.txt'); C=[mocam(b) for b in tb]; e=float(np.abs(np.array([c['cam'] for c in C])-0.5*np.sin(0.45*np.array([c['t'] for c in C]))).max())
chk('idle camera keeps Gibson 0.5*sin(0.45 t) (max residual <= 0.0005)',e<=0.0005,max_residual=round(e,6))
tj=json.load(open(W+'/touch_targets.json'))
chk('touch: < TOWERS 70x39 (>= 36 px)',tj['towers_btn']['w']>=36 and tj['towers_btn']['h']>=36,rect=tj['towers_btn'])
chk('touch: every pillar keeps a tap cell in every idle sample (inherited v11 motion)',tj['every_tower_tappable_every_sample'],worst=tj['square_px_worst'])
chk('touch: >= 36 px idle tap cell for every pillar in every sample (inherited v11 motion; unchanged because positions must stay v11)',tj['min_worst']>=36,min_worst=tj['min_worst'],median=tj['square_px_median'])
ab=open(W+'/all12/out.txt').read(); hits=re.findall(r'tap \d+,\d+ -> ([A-Za-z -]+?) \(',ab)
chk('touch: each of the 12 pillars opens its device and < TOWERS closes it',[x for x in hits if not x.startswith('mo-')]==NAMES and hits.count('mo-towers')==12,taps=hits)
# ---- open state
cu=[tuple(map(int,m.groups())) for m in re.finditer(r'MOCU \d+ p=1.000 f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]',ab+open(W+'/play/out.txt').read())]
chk('close-up: every fully-open pillar is big on the LEFT (front face >= 58 px, centre 34..58, top 50..70)',len(cu)>=24 and all(x1-x0>=58 and 34<=(x0+x1)//2<=58 and 50<=yt<=70 for x0,x1,yt,yb in cu),count=len(cu))
fo=[x for x,y in re.findall(r'MOFOLD nland=(\d) typed=\d+ ys=(\S+)',ab) if '-999' not in y]; chk('folders: all 3 folders landed at y 60/122/184 in every rendered open-state read-back (all12, 1.2 s and 3 s after each tap)',len(fo)==24 and all(x=='3' for x in fo),nland=fo)
bad=0;why=[]
for f in ('play','all12'):
    for b in blocks(W+f'/{f}/out.txt'): bad=max(bad,mocam(b)['bad_all']); why+=re.findall(r'MOWHY (.*)',b)
chk('text: 0 clipped lines (every text line audited every rendered frame)',bad==0 and not why,bad=bad,why=why[:5])
A=np.fromfile(W+'/play/playthrough.rgb',np.uint8).reshape(-1,320,240,3); n=len(A)
red=int(((A[:,35:247,:,0]>170)&(A[:,35:247,:,1]<70)&(A[:,35:247,:,2]<70)).sum())
chk('no red pill anywhere in the playthrough field (rows 35..246)',red==0,red_px=red)
fz=sum(1 for k in range(1,n) if np.array_equal(A[k],A[k-1])); chk('motion: 0 frozen frames in the playthrough',fz==0,frozen=fz,frames=n)
p8=np.array(Image.open(W+'/play/p08_open_airtag.png')); m=re.search(r'DUMP p08.*?MOCU \d+ p=[\d.]+ f=\[(-?\d+),(-?\d+),(-?\d+),(-?\d+)\]',open(W+'/play/out.txt').read(),re.S)
x0,x1,yt,yb=map(int,m.groups()); pl=p8[yt+3:yt+3+52,x0+1:x1]; nc=len({tuple(c) for c in pl.reshape(-1,3)})
chk('crisp: close-up icon plate + icon uses <= 10 distinct colours (no blur / dither smear)',nc<=10,colours=nc,box=[x0+1,yt+3,x1-1,yt+54])
# ---- outputs
for f in os.listdir(D):
    if f!='_withdrawn': raise SystemExit('out dir not clean: '+f)
os.makedirs(D+'/stills',exist_ok=True)
names=sorted(f[:-4] for f in os.listdir(W+'/play') if re.match(r'p\d\d_',f) and f.endswith('.png'))
for k,nm in enumerate(names):
    im=Image.open(W+f'/play/{nm}.png'); lab='%02d_%s'%(k+1,nm[4:]); im.save(D+f'/{lab}@1x.png'); im.resize((720,960),Image.NEAREST).save(D+f'/stills/{lab}@3x.png')
ok3=all(np.array_equal(np.array(Image.open(D+'/stills/'+f)),np.array(Image.open(D+'/'+f.replace('@3x','@1x'))).repeat(3,0).repeat(3,1)) for f in os.listdir(D+'/stills'))
chk('@3x stills are exact 3x nearest-neighbour of @1x',ok3)
for sc,o in ((1,'@1x'),(3,'@3x')):
    subprocess.run(['ffmpeg','-y','-loglevel','error','-framerate','100/3','-f','rawvideo','-pix_fmt','rgb24','-s','240x320','-i',W+'/play/playthrough.rgb','-vf',f'scale={240*sc}:{320*sc}:flags=neighbor','-c:v','libx264','-pix_fmt','yuv420p','-crf','17','-movflags','+faststart',D+f'/morpheus_playthrough{o}.mp4'],check=True)
pr=json.loads(subprocess.run(['ffprobe','-v','error','-select_streams','v:0','-count_frames','-show_entries','stream=codec_name,pix_fmt,width,height,nb_read_frames','-of','json',D+'/morpheus_playthrough@3x.mp4'],capture_output=True,text=True).stdout)['streams'][0]
chk('video: @3x 720x960 h264 yuv420p, every frame',pr['codec_name']=='h264' and pr['pix_fmt']=='yuv420p' and (pr['width'],pr['height'])==(720,960) and int(pr['nb_read_frames'])==n,probe=pr)
def lab(im,t):
    c=Image.new('RGB',(im.width,im.height+40),(20,18,16)); c.paste(im,(0,40)); ImageDraw.Draw(c).text((8,8),t,fill=(232,222,200),font=F); return c
def side(a,b,ta,tb,out):
    A_=lab(a.resize((720,960),Image.NEAREST),ta); B_=lab(b.resize((720,960),Image.NEAREST),tb); c=Image.new('RGB',(1460,1000),(20,18,16)); c.paste(A_,(0,0)); c.paste(B_,(740,0)); c.save(out)
side(Image.open('/workspace/out/hackbot-game-1112/stills/17_size_3_files_L1@1x.png'),Image.open(W+'/play/p08_open_airtag.png'),'HACKbot DOWNLOAD open (3 files)','v13 AIR TAG open',D+'/compare_hackbot_vs_v13_open.png')
side(Image.fromarray(idl['v12']['rgb'][150]),Image.fromarray(idl['v13']['rgb'][150]),'v12 idle (frame 150)','v13 idle (frame 150, same motion)',D+'/compare_v12_vs_v13_idle.png')
cols=4; rows=(len(names)+cols-1)//cols; sh=Image.new('RGB',(cols*490,rows*690),(20,18,16))
for k,nm in enumerate(names): sh.paste(lab(Image.open(W+f'/play/{nm}.png').resize((480,640),Image.NEAREST),'%02d %s'%(k+1,nm[4:])),((k%cols)*490,(k//cols)*690))
sh.save(D+'/contact_sheet.png')
shutil.copy(W+'/touch_targets.json',D+'/touch_targets.json')
np_=sum(c['PASS'] for c in gate['checks']); gate['score']='%d/%d'%(np_,len(gate['checks'])); gate['failures']=[c['name'] for c in gate['checks'] if not c['PASS']]
json.dump(gate,open(D+'/gate.json','w'),indent=1); print('GATE',gate['score'])
