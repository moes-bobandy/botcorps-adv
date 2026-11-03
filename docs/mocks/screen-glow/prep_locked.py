# Decode the three LOCKED screen MP4s to 3x PNG frames and compute the 1x median / per-pixel range used for the masks.
import os, subprocess, sys, numpy as np, glob
from PIL import Image
LK = '/workspace/cyd-lock-store/LOCKED'
SRC = {'T': LK + '/ui_terminalbot/terminalbot_bg@3x.mp4', 'S': LK + '/ui_scanbot/scanbot_bg_themes_v2@3x.mp4',
       'M': LK + '/ui_glossary_morpheus_rain/chiprain_black_soft_final_big_morpheus_green.mp4'}
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/lk'
for k, p in SRC.items():
    d = '%s/%s' % (OUT, k); os.makedirs(d, exist_ok=True)
    if not glob.glob(d + '/f*.png'): subprocess.run(['ffmpeg', '-v', 'error', '-i', p, '-start_number', '1', d + '/f%03d.png'], check=True)
    st = np.stack([np.array(Image.open(f).convert('RGB'))[1::3, 1::3] for f in sorted(glob.glob(d + '/f*.png'))])
    np.save('%s/med_%s.npy' % (OUT, k), np.median(st, 0).astype(np.uint8))
    np.save('%s/rng_%s.npy' % (OUT, k), (st.max(0).astype(int) - st.min(0).astype(int)).max(-1))   # per-pixel range, max over channels
