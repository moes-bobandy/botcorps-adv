# Verify encoded glow MP4s: container/codec facts, and decoded-glow-MP4 vs decoded-LOCKED-MP4 outside the face+window region (codec noise only).
import subprocess, sys, numpy as np
sys.path.insert(0, '.'); import glow_screens as G
def frames(p):
    pr = subprocess.Popen(['ffmpeg', '-v', 'error', '-i', p, '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'], stdout=subprocess.PIPE)
    while True:
        b = pr.stdout.read(960 * 720 * 3)
        if len(b) < 960 * 720 * 3: break
        yield np.frombuffer(b, np.uint8).reshape(960, 720, 3)
for nm in sys.argv[1:] or ['terminal', 'morpheus', 'scan']:
    cfg = G.SCREENS[nm]; _, region = G.masks(cfg); out = ~G.up3(region)
    pr = subprocess.run(['ffprobe', '-v', 'error', '-select_streams', 'v', '-count_frames', '-show_entries', 'stream=codec_name,profile,pix_fmt,width,height,r_frame_rate,nb_read_frames', '-of', 'csv=p=0', nm + '_glow_3x.mp4'], capture_output=True, text=True).stdout.strip()
    moov = open(nm + '_glow_3x.mp4', 'rb').read(8192).find(b'moov'); n = 0; s = 0.0; mx = 0; big = 0; tot = 0
    for a, b in zip(frames(nm + '_glow_3x.mp4'), frames(cfg['src'])):
        d = np.abs(a.astype(np.int16) - b.astype(np.int16)).max(-1)[out]; n += 1; s += d.sum(); mx = max(mx, int(d.max())); big += int((d > 8).sum()); tot += d.size
    print(nm, pr, '| moov at byte', moov, '| frames compared', n, '| outside region: mean|d| %.3f, max %d, px>8 %d of %d (%.4f%%)' % (s / tot, mx, big, tot, 100 * big / tot))
