import re,sys
src=open('/workspace/src/ui/botcorps_pal_tables.inc').read()
def arr(name):
    m=re.search(name+r'\[256\] = \{(.*?)\};',src,re.S); return [int(x,0) for x in re.findall(r'0x[0-9A-Fa-f]+|\d+',m.group(1))]
res=arr('k_is_res'); nat=arr('k_exact_nat')
fit=[int(x) for x in re.findall(r'\d+',re.search(r'k_fit4\[4096\] = \{(.*?)\};',open('/workspace/src/ui/botcorps_pal_fit.inc').read(),re.S).group(1))]
def p332(i):
    r3,g3,b2=(i>>5)&7,(i>>2)&7,i&3
    r8=(((r3<<3)+r3)<<2)+(r3>>1); g8=(((g3<<3)+g3)<<2)+(g3>>1); b8=b2*0x55
    return ((r8&0xF8)<<8)|((g8&0xFC)<<3)|(b8>>3)
def ch(p): r5,g6,b5=(p>>11)&31,(p>>5)&63,p&31; return ((r5<<3)|(r5>>2),(g6<<2)|(g6>>4),(b5<<3)|(b5>>2))
def d2(a,b): return sum((x-y)**2 for x,y in zip(ch(a),ch(b)))
def to565(h): h=h.lstrip('#'); r,g,b=int(h[0:2],16),int(h[2:4],16),int(h[4:6],16); return ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
def mix(fg,bg,a):
    f=[int(fg.lstrip('#')[i:i+2],16) for i in (0,2,4)]; b=[int(bg.lstrip('#')[i:i+2],16) for i in (0,2,4)]
    return '#%02x%02x%02x'%tuple(round(a*x+(1-a)*y) for x,y in zip(f,b))
bins=[]
for key in range(4096):
    r4,g4,b4=key>>8,(key>>4)&15,key&15
    bins.append([ch(((r4*2+rl)<<11)|((g4*4+gl)<<5)|(b4*2+bl)) for rl in range(2) for gl in range(4) for bl in range(2)])
def shownlist(res,nat): return [ch(nat[i] if res[i] else p332(i)) for i in range(256)]
def floor(res,nat):
    S=shownlist(res,nat); W=0; tbl=[]
    for B in bins:
        best=None
        for i,s in enumerate(S):
            w=max(sum((x-y)**2 for x,y in zip(q,s)) for q in B)
            if best is None or w<best[0]: best=(w,i)
        tbl.append(best[1]); W=max(W,best[0])
    return W,tbl
