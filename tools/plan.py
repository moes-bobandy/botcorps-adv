exec(open('pal.py').read())
names={}
import re
for m in re.finditer(r'(PAL_\w+) = (\d+)',open('/workspace/src/ui/botcorps_pal_enum.inc').read()): names.setdefault(int(m.group(2)),m.group(1))
reserved=[i for i in range(256) if res[i]]
def hx(p): return '#%02x%02x%02x'%ch(p)
want=[]
for term in ['#ff2bd6','#00f0ff']:
    who={'#ff2bd6':'cyan+mint UI','#00f0ff':'amber UI'}[term]
    want+= [(term,f'term accent ({who}) solid'),
      (mix(term,'#070c11',.28),f'hair .28 ({term})'),(mix(term,'#070c11',.22),f'rule/hunt .22 ({term})'),
      (mix(term,'#070c11',.18),f'selected .18 ({term})'),(mix(term,'#090d12',.35),f'neon .35 ({term})'),
      (mix(term,'#070c11',.10),f'tile breathe low .10 ({term})')]
want+=[('#f1eee6','cream tile sel'),('#e2dfd6','cream tile'),('#aaa69b','light edge'),('#bdb9ad','light trace'),('#84807a','light antenna'),('#8a929a','light pad'),('#16191d','light footprint/hole'),('#aab2b9','light fp pad'),('#3b4046','light silk'),('#c9c5ba','light led off'),
 ('#1c2127','dark base sel'),('#121519','dark base'),('#323a42','dark edge'),('#3f4b57','dark trace sel'),('#262d35','dark trace'),('#6a7681','dark antenna'),('#bcc3ca','dark pad'),('#07090b','dark fp'),('#9aa3ab','dark fp pad'),('#a9b1b8','dark silk'),('#2a2f35','dark led off'),('#05070a','dark hole'),
 ('#F4F0E2','crown'),('#C9C3B2','crown lower'),('#26323f','bar/rx off'),('#8d99a6','battery shell'),('#0b0f14','battery inner'),('#ffb000','battery cells amber (UI.amber)'),('#8dff9a','battery cells OK'),('#000000','silk sel light'),('#ffffff','silk sel dark')]
pool=[(i,nat[i]) for i in reserved]+[(0,0),(255,0xFFFF)]
new=[]; reuse=[]
for h,why in want:
    p=to565(h)
    best=min(pool,key=lambda s:d2(p,s[1]))
    dd=d2(p,best[1])
    if dd<100: reuse.append((h,why,best[0],names.get(best[0],'new' if best[0]>=230 and not res[best[0]] else '?'),dd))
    else:
        code=[i for i in range(230,255) if not res[i] and i not in [c for c,_ in pool]]
        if not code: print('OUT OF CODES',h); continue
        c=code[0]; pool.append((c,p)); names[c]='NEW:'+why; new.append((h,'0x%04X'%p,c,why))
print('NEW'); [print(x) for x in new]
print('REUSE'); [print(x) for x in reuse]
r2=res[:]; n2=nat[:]
for h,p,c,_ in new: r2[c]=1; n2[c]=int(p,16)
same=all(r2[i]==res[i] and n2[i]==nat[i] for i in reserved)
W,t=floor(r2,n2); print('k_fit4 floor',W,'71 identical',same, 'total reserved',sum(r2))
