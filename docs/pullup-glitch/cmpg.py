import sys,os,numpy as np
A,B=sys.argv[1],sys.argv[2]; tot=0; nf=0
for root,_,fs in os.walk(A):
    for f in sorted(fs):
        if not f.endswith('.rgb') or f.endswith('.cam.rgb'): continue
        pa=os.path.join(root,f); pb=pa.replace(A,B,1)
        a=np.fromfile(pa,np.uint8); b=np.fromfile(pb,np.uint8) if os.path.exists(pb) else None
        if b is None or a.size!=b.size: print(pa,'MISSING/SIZE'); tot+=1; continue
        a=a.reshape(-1,320,240,3); b=b.reshape(-1,320,240,3); d=(a!=b).any(3); n=int(d.sum()); tot+=n; nf+=a.shape[0]
        print(f'{os.path.relpath(pa,A):28s} frames={a.shape[0]:4d} diff_px={n}')
print('TOTAL frames',nf,'diff px',tot)
