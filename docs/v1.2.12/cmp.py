import sys, numpy as np, os
A,B=sys.argv[1],sys.argv[2]; tot=0
for f in sorted(os.listdir(A)):
    if not f.endswith('.rgb') or not os.path.exists(B+'/'+f): continue
    a=np.fromfile(A+'/'+f,np.uint8); b=np.fromfile(B+'/'+f,np.uint8)
    if a.size!=b.size: print(f,'SIZE'); continue
    a=a.reshape(-1,320,240,3); b=b.reshape(-1,320,240,3); d=(a!=b).any(3); n=int(d.sum()); tot+=n
    if n:
        ys,xs=np.nonzero(d.any(0)); print(f'{f:14s} frames={a.shape[0]} diff_px={n} frames_diff={int(d.any((1,2)).sum())} box=x{xs.min()}-{xs.max()} y{ys.min()}-{ys.max()}')
    else: print(f'{f:14s} frames={a.shape[0]} diff_px=0')
print('TOTAL',tot)
