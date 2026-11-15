import re, numpy as np, math
H_=open('/tmp/t3c2/tree/src/live/lv_morpheus_v13.h').read()
def arr(n):
    return np.array([float(v.rstrip('f')) for v in re.search(n+r'\[MO_N\] = \{([^}]*)\}', H_).group(1).split(', ')])
SX,Z0,AX,AZ,WX,WZ,PX,PZ,AS,WS,PS,HK=[arr(n) for n in 'MO11_SX MO11_Z0 MO11_AX MO11_AZ MO11_WX MO11_WZ MO11_PX MO11_PZ MO11_AS MO11_WS MO11_PS MO11_HK'.split()]
F=120.; HOR=138; CAMH=0.9; W=0.12*1.35; D=0.24
NEAR=[1,13,8,10,7]; KZ=0.6; MINFW=10.5
S_=[6,18,10,11,5,6,9,13,15,4,14,7,6,17,5,13,9,6]
Hs={}
for l in open('/tmp/t3c2/run_EB2/run.log'):
    m=re.search(r'MOTW (\d+) X=(\S+) Z=(\S+) H=(\S+) .*dx=(\S+) dz=(\S+)',l)
    if m and int(m.group(1)) not in Hs: Hs[int(m.group(1))]=list(map(float,m.groups()[1:]))
HH=np.array([Hs[i][2]/(KZ if i in NEAR else 1) for i in range(18)])
def geom(t, c, dsx=None, dz0=None, comp=True):
    dsx = np.zeros(18) if dsx is None else dsx; dz0=np.zeros(18) if dz0 is None else dz0
    out=[]
    for i in range(18):
        z0=Z0[i]+dz0[i]; X0=(SX[i]+dsx[i]-120)*z0/F
        dx=AX[i]*(z0/2)*math.sin(WX[i]*t+PX[i])+AS[i]*z0*math.sin(WS[i]*t+PS[i])
        dz=AZ[i]*(z0/3)*math.sin(WZ[i]*t+PZ[i])
        X=X0+dx; Z=z0+dz; Hh=HH[i]; w=W; d=D
        if i in NEAR:
            k=KZ; X*=k; Z*=k; Hh*=k; w*=k; d*=k
            if comp: X+=c*(1-k)
        wmin=MINFW*Z/(2*F)
        if w<wmin: d*=wmin/w; w=wmin
        px=lambda X,Z: round(120+F*(X-c)/Z); py=lambda Y,Z: round(HOR+F*(CAMH-Y)/Z)
        out.append(dict(i=i,Z=Z,x0=px(X-w,Z),x1=px(X+w,Z),yt=py(Hh,Z),yb=py(0,Z),bx0=px(X-w,Z+d),bx1=px(X+w,Z+d),byt=py(Hh,Z+d),byb=py(0,Z+d)))
    return out
def parts(T):
    fw=T['x1']-T['x0']; m=4 if fw>=40 else 2; bh=4 if fw>=40 else 2 if fw>=14 else 1
    bt0=T['yt']+m; bb0=T['yb']-(10 if fw>=40 else 3)-bh; S=S_[T['i']]
    ey0=bt0+bh+2*m; cx=(T['x0']+T['x1'])//2
    emb=(cx-S//2, ey0, cx-S//2+S-1, ey0+S-1)
    lw=4 if fw>=40 else 3 if fw>=20 else 2
    led=(T['x1']-2-lw-1, ey0+S+2, T['x1']-2, bb0)
    return emb, led
def silo(T): return (min(T['x0'],T['bx0']), min(T['yt'],T['byt']), max(T['x1'],T['bx1']), max(T['yb'],T['byb']))
def vis(G):
    """fraction visible of emblem and LED column per tower (occluders = nearer towers' silhouettes, screen x 0..239)"""
    order=sorted(G,key=lambda T:-T['Z']); res={}
    for n,T in enumerate(order):
        m=np.zeros((330,260),bool)
        for U in order[n+1:]:
            a,b,c_,d_=silo(U); m[max(0,b):max(0,d_+1), max(0,a+10):max(0,c_+11)]=True
        m[:, :10]=True; m[:, 250:]=True   # off-screen x<0 or x>239 (offset 10)
        r=[]
        for (a,b,c_,d_) in parts(T):
            if c_<a or d_<b: r.append(0.); continue
            sub=m[max(0,b):d_+1, a+10:c_+11]; r.append(1-sub.mean() if sub.size else 0.)
        res[T['i']]=r
    return res
