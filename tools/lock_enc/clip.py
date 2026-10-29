import subprocess, numpy as np, sys
def load(path, sample=True):
    """return uint8 array [N,320,240,3] sampled at 3x3 block centres (and full-res if sample=False)"""
    p = subprocess.run(['ffmpeg','-v','error','-i',path,'-f','rawvideo','-pix_fmt','rgb24','-'],capture_output=True,check=True)
    a = np.frombuffer(p.stdout,np.uint8).reshape(-1,960,720,3)
    return a[:,1::3,1::3,:].copy() if sample else a
