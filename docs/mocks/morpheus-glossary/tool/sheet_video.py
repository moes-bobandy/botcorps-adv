"""contact sheet of the 12 description pages + tap-through MP4 (grid -> tap chip -> page -> tap crown BACK -> grid)."""
import sys, os, json, subprocess, shutil
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from glossary import grid, desc, DEVICES, up, overlay, OUT
from PIL import Image, ImageDraw
import numpy as np
for theme in ('green', 'orange'):
    pages = [Image.open('%s/desc/desc_%s_%s_3x.png' % (OUT, n.replace(' ', '_').lower(), theme)) for n, _ in DEVICES]
    W, H = 360, 480; pad = 8
    sheet = Image.new('RGB', (4 * W + 5 * pad, 3 * H + 4 * pad), (20, 20, 20))
    for i, p in enumerate(pages):
        sheet.paste(p.resize((W, H), Image.LANCZOS), (pad + (i % 4) * (W + pad), pad + (i // 4) * (H + pad)))
    sheet.save('%s/contact_sheet_desc_%s.png' % (OUT, theme))
# video (green theme)
tmp = '/tmp/mg_vid'; shutil.rmtree(tmp, ignore_errors=True); os.makedirs(tmp)
g, rects, crown, *_ = grid('green')
k = 0
def emit(img, n):
    global k
    for _ in range(n): Image.fromarray(img).save('%s/%05d.png' % (tmp, k)); k += 1
G = up(g); emit(G, 12)
for name, _ in DEVICES:
    emit(overlay(g, [rects[name]], (255, 255, 255)), 4)
    d, cr, _ = desc('green', name); D = up(d)
    emit(D, 22)
    emit(overlay(d, [cr], (255, 255, 255)), 4)
    emit(G, 6)
subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '10', '-i', tmp + '/%05d.png', '-c:v', 'libx264',
                '-pix_fmt', 'yuv420p', '-movflags', '+faststart', OUT + '/tapthrough_green.mp4'], check=True)
print(k, 'frames')
