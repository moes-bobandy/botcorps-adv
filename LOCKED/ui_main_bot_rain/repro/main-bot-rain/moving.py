# Animated 3-up of the current main-bot rain chips (mock only). Firmware cadence: botcorps_rain.cpp header "140 ms" per row step.
import sys, os, subprocess; sys.path.insert(0, '/workspace/out/main-bot-rain'); import mbr, numpy as np, glossary as G
from PIL import Image
exec(open('/workspace/out/main-bot-rain/oneline.py').read().split('JOBS =')[0])           # SPEC + oneline()
GIB_WHITE = [(mbr.GIB_ART, {'a': (255, 255, 255), 'e': (0, 0, 0)})]; WHITE_LEVELS = [1.00, 0.95, 0.92, 0.90, 0.88, 0.86]   # as white.py
SCAN_FR = {'green': '/tmp/mbr_fr/scanbot_bg_themes_v2_08.png', 'orange': '/tmp/mbr_fr/scanbot_bg_themes_v2_17.png'}
TERM_FR = {'green': '/tmp/mbr_fr/terminalbot_bg_13.png', 'orange': '/tmp/mbr_fr/terminalbot_bg_24.png'}
T0, N, STEP_MS = 9, 74, 140          # t=9 is the still; 74 = 2x the 37-glyph stream period
def preps(th):
    g = mbr.prep(oneline(G.load1x(G.THEMES[th][0]), 'GIBSONbot', 93), *SPEC)
    lb = g[3]; y0 = np.nonzero(lb.any(1))[0].min(); lb[y0:146] = lb[y0]                    # GIBSON dim band to chip bottom (as white.py)
    s = mbr.prep(oneline(G.load1x(SCAN_FR[th]), 'SCANbot', 95), *SPEC)
    t = mbr.prep(G.load1x(TERM_FR[th]), *SPEC)
    return [(g, GIB_WHITE, WHITE_LEVELS), (s, mbr.SCAN, mbr.LEVELS), (t, mbr.DEV, mbr.LEVELS)]
def frame(P, t):
    out = []; base = mbr.LEVELS
    for (a, mask, cells, lb), gl, lv in P:
        b = a.copy(); mbr.LEVELS = lv
        try: mbr.paint(b, mask, cells, lb, gl, t)
        finally: mbr.LEVELS = base
        out.append(G.up(b[40:156, 74:168].astype(np.uint8)))
    gap = np.zeros((116 * 3, 12, 3), np.uint8); return np.hstack([out[0], gap, out[1], gap, out[2]])
for th in ('green', 'orange'):
    P = preps(th); fd = '/tmp/mbr_mov_%s' % th; os.makedirs(fd, exist_ok=True)
    for i in range(N): Image.fromarray(frame(P, T0 + i)).save('%s/f%03d.png' % (fd, i))
    out = 'threeup_gibson_scan_terminal_%s_3x_moving.mp4' % th
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '50/7', '-i', fd + '/f%03d.png', '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
                    '-c:v', 'libx264', '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', out], check=True)
    if th == 'green':
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '50/7', '-i', fd + '/f%03d.png', '-vf', 'split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=none',
                        '-loop', '0', 'threeup_gibson_scan_terminal_green_3x_moving.gif'], check=True)
    print(th, 'ok')
