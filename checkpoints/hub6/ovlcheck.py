import sys,glob,os
def rd(f):
    R=[]
    for l in open(f):
        p=l.rstrip('\n').split('\t')
        if len(p)<5: continue
        R.append((p[0],int(p[1]),int(p[2]),int(p[3]),int(p[4]),p[5] if len(p)>5 else ''))
    return R
def inter(a,b):
    return a[1]<b[1]+b[3] and b[1]<a[1]+a[3] and a[2]<b[2]+b[4] and b[2]<a[2]+a[4]
def inside(a,b):
    return a[1]>=b[1] and a[2]>=b[2] and a[1]+a[3]<=b[1]+b[3] and a[2]+a[4]<=b[2]+b[4]
tot=0;out=[]
for f in sorted(sys.argv[1:]):
    R=list(set(rd(f)))
    C=[r for r in R if r[0]=='CHIP']; T=[r for r in R if r[0]=='TEXT' and r[2]+r[4]>26]
    PL=[r for r in R if r[0]=='PLATE']
    hits=[]
    for i,a in enumerate(C):
        for b in C[i+1:]:
            if inter(a,b) and not inside(a,b) and not inside(b,a): hits.append(('chip/chip',a,b))
    for p in PL:
        q=('P',p[1]-4,p[2]-4,p[3]+8,p[4]+8,p[5])
        W=320 if any(r[1]+r[3]>240 for r in R) else 240
        for a in C+T:
            if a[0]=='TEXT' and (a[1]+a[3]<=31 or a[1]>=W-31): continue   # pin-header column labels
            if inter(a,q): hits.append(('plate',a,p))
    for i,a in enumerate(T):
        if a[2]<27: hits.append(('topbar',a,a))
        for b in T[i+1:]:
            if inter(a,b) and not inside(a,b) and not inside(b,a): hits.append(('text/text',a,b))
        for c in C:
            if inter(a,c) and not inside(a,c): hits.append(('text/chip',a,c))
    tot+=len(hits)
    out.append(f"{os.path.relpath(f)}: {len(hits)} hits ({len(C)} chips, {len(T)} text, {len(PL)} plate)")
    for h in hits: out.append(f"   {h[0]}: {h[1]}  vs  {h[2]}")
print('\n'.join(out)); print('TOTAL',tot)
