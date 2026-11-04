# Verify scan_glow_3x.mp4 (current WIFI/BLE faces): codec facts, and decoded MP4 vs the current base PNGs (/tmp/sgc_base) outside face+window.
import subprocess, sys, glob, numpy as np
from PIL import Image
sys.path.insert(0, '.'); import glow_screens as G
_, region = G.masks(G.SCREENS['scan']); out = ~G.up3(region); P = 'scan_glow_3x.mp4'
pr = subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v', '-count_frames', '-show_entries', 'stream=codec_name,profile,pix_fmt,width,height,r_frame_rate,nb_read_frames', '-of', 'csv=p=0', P], capture_output=True, text=True).stdout.strip()
moov = open(P, 'rb').read(8192).find(b'moov')
p = subprocess.Popen(['ffmpeg', '-v', 'error', '-i', P, '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'], stdout=subprocess.PIPE)
n = 0; s = 0.0; mx = 0; big = 0; tot = 0
for f in sorted(glob.glob('/tmp/sgc_base/f*.png')):
    a = np.frombuffer(p.stdout.read(960 * 720 * 3), np.uint8).reshape(960, 720, 3); b = np.array(Image.open(f).convert('RGB'))
    d = np.abs(a.astype(np.int16) - b.astype(np.int16)).max(-1)[out]; n += 1; s += d.sum(); mx = max(mx, int(d.max())); big += int((d > 8).sum()); tot += d.size
print('scan', pr, '| moov at byte', moov, '| frames compared', n, '| outside region vs current base: mean|d| %.3f, max %d, px>8 %d of %d (%.4f%%)' % (s / tot, mx, big, tot, 100 * big / tot))
