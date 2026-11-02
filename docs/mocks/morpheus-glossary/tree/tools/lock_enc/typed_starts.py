import sys,os; import numpy as np
p=sys.argv[1]; fr=np.load('/workspace/cydport/cache/'+os.path.basename(p)+'.npy',mmap_mode='r')
reg=np.concatenate([fr[:,288:296,140:177].reshape(len(fr),-1,3),fr[:,297:305,38:177].reshape(len(fr),-1,3)],1)
w=(reg.min(2)>190).sum(1)
starts=[i for i in range(1,len(w)) if w[i]>0 and w[i-1]==0]
print(os.path.basename(p),'typing starts',starts)
