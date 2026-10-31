from PIL import Image
def hx(c): return ((c>>16)&255,(c>>8)&255,c&255)
def mul(c,k): return tuple(min(255,int(v*k)) for v in c)
ACC=[0xb324fd,0xdd00ac,0x92ff02,0x00ffff,0x345efb,0xfd6b00,0xffda00,0xff24fd,0x20fe01,0x00fe02,0xfe0001,0xff0052]
TNAME=["GRAPE","MAGENTA","TOXIC","ICE","BLUE","ORANGE","GOLD","SYNTH","AURA","PRISM","GLTCH","ROSE"]
TPAL={7:[0xff24ff,0xff5a8c,0xff8918],8:[0x00daff,0x20ff00,0xb400ff],9:[0xff2020,0xff9000,0xffff00,0x20ff20,0x2080ff,0xb030ff],10:[0xfe0001,0x00ffff,0xfe0001]}
def grad(pal,t):
    if len(pal)==1: return hx(pal[0])
    t=max(0,min(0.9999,t))*(len(pal)-1); i=int(t); f=t-i; a,b=hx(pal[i]),hx(pal[i+1])
    return tuple(int(a[k]+(b[k]-a[k])*f) for k in range(3))
def plate(w,h,ui,acc=0xb324fd):
    im=Image.new('RGB',(w,h),(0,0,0)); p=im.load()
    a=hx(acc)
    fill=mul(a,0.22) if ui else (20,20,20); fr=mul(a,0.55) if ui else (44,44,44)
    for y in range(h):
        for x in range(w): p[x,y]=fr if x in(0,w-1) or y in(0,h-1) else fill
    return im
def bm(im,x0,y0,rows,cmap,k=1.0):
    p=im.load()
    for y,r in enumerate(rows):
        for x,ch in enumerate(r):
            if ch in cmap and 0<=x0+x<im.width and 0<=y0+y<im.height:
                c=cmap[ch]; p[x0+x,y0+y]=mul(c if isinstance(c,tuple) else hx(c),k)
W=(255,255,255)
ICON7={
 'GRAPE':["...+...","..+....",".##.##.","##.##.#",".##.##.","..##...","...#..."],
 'MAGENTA':[".##.##.","#######","#######",".#####.","..###..","...#...","......."],
 'TOXIC':["...#...","..###..",".#####.","##.#.##","#######",".#####.","..###.."],
 'ICE':["#..#..#",".#.#.#.","..###..","#######","..###..",".#.#.#.","#..#..#"],
 'BLUE':[".......","..##...",".#..#..","#....##",".......","..##...",".#..###"],
 'ORANGE':["...#...","..##...","..###..",".####+.","##+++##","#++#++#",".#####."],
 'GOLD':[".#####.","##+++##","#+###+#","#+#++.#","#+###+#","##+++##",".#####."],
 'SYNTH':["..###..",".#####.","#######",".......","#######",".......","#######"],
 'AURA':["#......",".#...#.","..#.#.#","...#...","#.....#",".#...#.","..###.."],
 'PRISM':["...#...","...#...","..#.#..","..#.#..",".#...#.",".#...#.","#######"],
 'GLTCH':["###.##.",".......","#.####.","..##...","##.####",".......",".###.##"],
 'ROSE':["..###..",".#+#+#.","#+###+#",".#+#+#.","..###..","...+...","..++..."],
}
def theme_chip(t,w=31,h=9):
    im=plate(w,h,True,ACC[t]); pal=TPAL.get(t,[ACC[t]])
    icon=ICON7[TNAME[t]]; p=im.load()
    for y,r in enumerate(icon):
        for x,ch in enumerate(r):
            if ch=='#': p[2+x,1+y]=grad(pal,y/6) if t in TPAL else hx(ACC[t])
            elif ch=='+': p[2+x,1+y]=W if t not in TPAL else mul(grad(pal,1-y/6),1.0)
    for x in range(11,w-2):   # the theme's own colour ribbon
        for y in range(2,h-2):
            c=grad(pal,(x-11)/(w-14)) if t!=10 else (hx(0x00ffff) if (x//3)%3==1 else hx(0xfe0001))
            p[x,y]=c
    return im
WIFI5=["..###..",".#...#.","#.###.#","..#.#..","...#..."]
BT9=["..#..","..##.","#.#.#",".###.","..#..",".###.","#.#.#","..##.","..#.."]
BT7=["..#..","#.##.",".###.","..#..",".###.","#.##.","..#.."]
def focus_chip(i,w=31,h=9):
    im=plate(w,h,False); k=0.7; c=0x40d0ff; b=0x4080ff
    if i==0: bm(im,6,2,WIFI5,{'#':c},k); bm(im,19,1,BT7,{'#':b},k)
    if i==1: bm(im,12,2,WIFI5,{'#':c},k)
    if i==2: bm(im,13,1,BT7,{'#':b},k)
    if i==3: bm(im,12,1,["...#...","..#.#..","..#.#..",".#.#.#.",".#...#.","#..#..#","#######"],{'#':0xff3048},k)
    return im
def mix_chip(i,w=34,h=14):   # SCANbot mix ALL / WIFI / BLE (non-UI)
    im=plate(w,h,False); k=0.7
    WIFI=["...#####...",".##.....##.","#..#####..#","..#.....#..","....###....",".....#....."]
    if i in(0,1): bm(im,4 if i==0 else 11,4,WIFI,{'#':0x40d0ff},k)
    if i in(0,2): bm(im,22 if i==0 else 14,2,BT9,{'#':0x4080ff},k)
    return im
def col_chip(c,w=34,h=14,ui=True,multi=False):   # falling code columns in a colour
    im=plate(w,h,ui,0x20ff40 if multi else c); p=im.load()
    cols=[(4,1),(9,4),(14,0),(19,3),(24,2),(29,5)]
    MC=[0xff24ff,0x20ff40,0x00ffff,0xffb000,0xb400ff,0xff3048]
    for j,(x,off) in enumerate(cols):
        base=hx(MC[j] if multi else c)
        for y in range(2,h-2):
            d=(y+off)%7
            if d<4: p[x,y]=mul(base,0.35+0.2*d) if d<3 else W if not multi else mul(base,1.3)
            if d<4: p[x+1,y]=p[x,y] if d==3 else mul(base,0.25+0.15*d)
    return im
def speed_chip(i,w=34,h=14):   # MASHUP = ticker speed SLOW/MED/FAST: 1/2/3 chevrons
    im=plate(w,h,True,0xffb000); n=i+1; CH=["#...","##..",".##.","..##",".##.","##..","#..."]
    x0=(w-(n*6-2))//2
    for j in range(n): bm(im,x0+j*6,3,CH,{'#':0xffc830})
    return im
def rain_chip(on,w=39,h=14):
    im=col_chip(0x20ff40,w,h,True)
    if not on:
        im=plate(w,h,True,0x20ff40); p=im.load()
        for x in (6,12,18,24,30):
            for y in range(3,h-3): p[x,y]=(40,60,44)
        bm(im,15,3,["#.....#",".#...#.","..#.#..","...#...","..#.#..",".#...#.","#.....#"],{'#':0xff3048})
    return im
def rmode_chip(i,w=39,h=14):  # radar camera DRIFT / SWEEP / STILL
    im=plate(w,h,True,0x00ffff); p=im.load(); c=hx(0x00e8ff)
    if i==0:  # drift: grid that slides + small side arrow
        for x in range(3,w-3): p[x,10]=mul(c,0.6)
        for x in (6,12,18,24,30): 
            for y in range(4,10): p[x,y]=c
        bm(im,31,3,["#..","##.","###","##.","#.."],{'#':0xffffff})
    if i==1:  # sweep: radar arc + beam
        import math
        cx,cy,r=19,12,9
        for a in range(0,181,6):
            x=int(round(cx+r*math.cos(math.radians(a)))); y=int(round(cy-r*math.sin(math.radians(a))))
            if 0<x<w-1 and 0<y<h-1: p[x,y]=mul(c,0.6)
        for t in range(0,9): 
            x=int(round(cx+t*math.cos(math.radians(55)))); y=int(round(cy-t*math.sin(math.radians(55))))
            if 0<y<h-1: p[x,y]=W
    if i==2:  # still: grid + pause bars
        for x in range(3,w-3): p[x,10]=mul(c,0.6)
        for x in (6,12,24,30):
            for y in range(4,10): p[x,y]=mul(c,0.7)
        bm(im,16,3,["##.##","##.##","##.##","##.##","##.##","##.##"],{'#':0xffffff})
    return im
def multi_chip(i,w=26,h=14):  # auto re-scan OFF / 30S / 60S: clock pie (non-UI)
    im=plate(w,h,False); p=im.load(); import math; k=0.7
    cx,cy,r=12,6.5,5.2; c=hx(0x20ff40) if i else hx(0x909090)
    for y in range(1,h-1):
        for x in range(1,w-1):
            dx,dy=x-cx,y-cy; d=math.hypot(dx,dy)
            if abs(d-r)<0.6: p[x,y]=mul(c,k)
            elif d<r-0.6:
                a=(math.degrees(math.atan2(dx,-dy))+360)%360
                if i==1 and a<180 or i==2: p[x,y]=mul(c,0.45*k/0.7)
    if i==0: bm(im,10,4,["#...#",".#.#.","..#..",".#.#.","#...#"],{'#':0xff3048},k)
    return im
PH=[".####.","#....#","#.##.#","#.##.#","#.##.#","#....#",".####."]
def orient_half(which,active,w=35,h=15):
    im=plate(w,h,active,0x00ffff) if active else plate(w,h,False)
    k=1.0 if active else 0.45; c=0x00ffff
    body=["#########","#.......#","#.......#","#.......#","#.......#","#.......#","#.......#","#.......#","#########"]
    bm(im,13,3,body,{'#':c},k)
    usb=["#####",".###."] if which==0 else [".###.","#####"]
    bm(im,15,12 if which==0 else 1,usb,{'#':0xffffff},k)
    return im
def orient_chip(flip):
    im=Image.new('RGB',(71,15),(0,0,0)); im.paste(orient_half(0,not flip),(0,0)); im.paste(orient_half(1,flip),(36,0)); return im
def motion_chip(on,w=21,h=7):
    im=plate(w,h,True,0x00ffff)
    if on: bm(im,4,1,["#...#...#...","##..##..##..",".##..##..##.","##..##..##..","#...#...#..."],{'#':0x00ffff})
    else: bm(im,8,1,["##.##","##.##","##.##","##.##","##.##"],{'#':0x608080})
    return im
# (A) BG chips, proposed in the RAIN / RAIN COL rects
def bgstyle_chip(i,w=39,h=14):
    im=plate(w,h,True,0x20ff40); p=im.load()
    if i==0: return col_chip(0x20ff40,w,h,True,True)
    if i==1:  # BOTS: two bot faces
        F=["#######","#.....#","#.#.#.#","#.....#","#######"]
        bm(im,9,4,F,{'#':0xff24ff}); bm(im,22,4,F,{'#':0x20ff40})
    if i==2:  # GIBSON: towers on a grid
        for x in range(3,w-3): p[x,11]=hx(0xb400ff)
        for x0,ht in ((8,6),(14,9),(20,4),(26,7)):
            for y in range(11-ht,11):
                for x in range(x0,x0+3): p[x,y]=hx(0x00ffff) if x in(x0,x0+2) or y==11-ht else hx(0x003a40)
    if i==3:  # FLYBY: a ship with a trail
        bm(im,6,4,["......##....",".....####...","++++++++####","++++++++####",".....####...","......##...."],{'#':0xffffff,'+':0xff8918})
        bm(im,20,4,["........##","..........","##........"],{'#':0x808080})
    if i==4:  # BOUNCE: ball + bounce arc dots
        for x,y in ((5,10),(8,6),(11,4),(14,4),(17,6),(20,10)): p[x,y]=hx(0x808080)
        bm(im,23,6,[".###.","#####","#####",".###."],{'#':0xffb000})
        for x in range(3,w-3): p[x,11]=hx(0x404040)
    return im
BGC=[None,0x20ff40,0x00ffff,0xb400ff,0xffb000]
def bgcol_chip(i,w=34,h=14): return col_chip(BGC[i] or 0x20ff40,w,h,True,i==0)
