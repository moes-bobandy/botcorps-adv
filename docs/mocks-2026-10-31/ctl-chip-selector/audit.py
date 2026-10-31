import sys; sys.path.insert(0,'.')
from sel import *
O='/workspace/out/ctl-chip-selector/'
rects=[(k,r) for k,rs in TAP.items() for r in rs]+[(k,r) for k,r in KEEP.items()]
hits=[[[] for x in range(240)] for y in range(320)]
for k,(x0,y0,x1,y1) in rects:
    for y in range(y0,y1+1):
        for x in range(x0,x1+1): hits[y][x].append(k)
L=['CTL chip-selector mock - tap-rect audit (every pixel of the 240x320 screen tested)','','Tap rects (x0,y0,x1,y1 inclusive):']
for k,rs in TAP.items(): L.append(f'  {k:8s} {rs}   choices: {"/".join(CHOICES[k])}')
for k,r in KEEP.items(): L.append(f'  keep {k:20s} {r} (existing non-selector route, unchanged)')
ov=sum(1 for y in range(320) for x in range(240) if len(hits[y][x])>1)
L.append(''); L.append(f'overlapping pixels (hit by >1 rect): {ov}')
dead=0; tot=0
for k,(x0,y0,x1,y1) in ART.items():
    for y in range(y0,y1+1):
        for x in range(x0,x1+1):
            tot+=1
            if k not in hits[y][x] or len(hits[y][x])!=1: dead+=1
# chip bodies incl. pins
PINS={'MIX':(36,89,69,122),'RAINCOL':(40,148,65,171),'MASHUP':(36,201,69,234),'RAIN':(170,81,203,114),'MULTI':(170,178,203,211),'THEME':(75,82,164,205)}
for k,(x0,y0,x1,y1) in PINS.items():
    for y in range(y0,y1+1):
        for x in range(x0,x1+1):
            tot+=1
            if k not in hits[y][x] or len(hits[y][x])!=1: dead+=1
L.append(f'chip-art + chip-body pixels tested: {tot}, dead or ambiguous: {dead}')
rem=[r for r in REMOVED.values()]
L.append('removed value rects (now board fill): '+str(list(REMOVED.items())))
open(O+'tap_audit.txt','w').write('\n'.join(L)+'\n'); print('\n'.join(L))
assert ov==0 and dead==0
