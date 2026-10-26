from PIL import Image
import itertools,sys
POS={'L':{'homeA':(123,34),'homeB':(123,34),'homeC':(123,34),'term':(123,30),'scan':(123,30),'ctrl':(123,38)},
     'P':{'homeA':(78,78),'homeB':(78,78),'homeC':(78,78),'term':(78,78),'scan':(78,30),'ctrl':(78,86)}}
GEO={'L':dict(w=74,h=96,face=(10,19,54,54),lab=(3,73,68,19)),'P':dict(w=84,h=120,face=(10,19,64,64),lab=(3,85,78,21))}
def inr(x,y,r): return r[0]<=x<r[0]+r[2] and r[1]<=y<r[1]+r[3]
res=[]
for o in 'LP':
    G=GEO[o]; crops={}
    for n,(x,y) in POS[o].items():
        crops[n]=Image.open(f'{o}/{n}.png').convert('RGB').crop((x,y,x+G['w'],y+G['h']))
    ref='ctrl'; R=crops[ref].load()
    for n,c in crops.items():
        C=c.load(); diff=0; tot=0; first=None
        for yy in range(G['h']):
            for xx in range(G['w']):
                if inr(xx,yy,G['face']) or inr(xx,yy,G['lab']): continue
                tot+=1
                if C[xx,yy]!=R[xx,yy]:
                    diff+=1; first=first or (xx,yy)
        res.append(f"{o} {n:6s} @({POS[o][n][0]},{POS[o][n][1]},{G['w']},{G['h']}): frame px differing vs ctrl = {diff}/{tot}" + (f" first {first}" if first else ""))
    # side-by-side native + 3x
    W=sum(c.width+6 for c in crops.values()); sb=Image.new('RGB',(W,G['h']),(40,40,40)); xo=0
    for n in ['homeA','term','scan','ctrl']:
        sb.paste(crops[n],(xo,0)); xo+=crops[n].width+6
    sb=sb.crop((0,0,xo,G['h'])); sb.save(f'mainchip_{o}_native.png'); sb.resize((sb.width*3,sb.height*3),Image.NEAREST).save(f'mainchip_{o}_3x.png')
print('\n'.join(res))
