# Locked C breathing face glow (AMAX 0.80, RC 7 px, 1.4 s raised-cosine, lift gamma 0.15) on the LOCKED TERMINALbot,
# MORPHEUS glossary and SCANbot screens. MOCK ONLY: no firmware, no flash.
# Works on the decoded locked frames (3x). Masks are computed at 1x on the median frame and expanded to 3x blocks, so every
# pixel outside face+window is copied byte-for-byte from the decoded locked frame. Big main bots are deliberately untouched.
import sys, os, glob, math, subprocess, json
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import distance_transform_edt
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from detect import face_of, bbox
AMAX, RC, GAMMA, PER = 0.80, 7, 0.15, 1.4
LK = '/workspace/cyd-lock-store/LOCKED'
SCREENS = {
 'terminal': dict(key='T', src=LK + '/ui_terminalbot/terminalbot_bg@3x.mp4', fps='100/3', kind='solid', clip=(-5, -3, 7, 6),
   seeds={'DEAUTH': (51, 56), 'META': (187, 56), 'HACK': (51, 115), 'SKIMMER': (187, 115), 'FLOCK': (51, 172), 'FLIPPER': (187, 172),
          'DRONE': (51, 230), 'AIR TAG': (97, 230), 'KEYFOB': (141, 230), 'SAMSUNG': (187, 230)}),
 'morpheus': dict(key='M', src=LK + '/ui_glossary_morpheus_rain/chiprain_black_soft_final_big_morpheus_green.mp4', fps='8', kind='screen', clip=(-7, -3, 7, 4),
   seeds={'WIFI': (42, 43), 'DEAUTH': (180, 43), 'BLUETOOTH': (42, 108), 'HACK': (180, 108), 'FLOCK': (42, 173), 'DRONE': (88, 173),
          'AIR TAG': (134, 173), 'KEYFOB': (180, 173), 'META': (42, 243), 'SKIMMER': (88, 243), 'FLIPPER': (134, 243), 'SAMSUNG': (180, 243)}),
 'scan': dict(key='S', src=LK + '/ui_scanbot/scanbot_bg_themes_v2@3x.mp4', fps='100/3', kind='solid', clip=(-7, -4, 7, 6),
   seeds={'WIFI': (51, 60), 'BLE': (51, 123), 'GIB': (186, 92)}),
}
def dominant(px):
    v, c = np.unique(px.reshape(-1, 3), axis=0, return_counts=True); return tuple(int(x) for x in v[c.argmax()])
def masks(cfg):
    med = np.load('/tmp/lk/med_%s.npy' % cfg['key']); rng = np.load('/tmp/lk/rng_%s.npy' % cfg['key'])
    rng = rng.max(-1) if rng.ndim == 3 else rng
    m = med.astype(int); s = m.sum(-1); ptp = m.max(-1) - m.min(-1)
    dark = s < 90; greyb = (ptp < 40) & (s < 450) & ~dark; static = rng <= 40
    H, W = s.shape; chips = []; region = np.zeros((H, W), bool)
    for name, seed in cfg['seeds'].items():
        face, _ = face_of(med, seed, cfg['kind']); x0, y0, x1, y1 = bbox(face)
        lift = face & ~dark                                   # never the eyes / black screen
        sat = lift & (ptp >= 40) & (m.max(-1) >= 150)          # icon chips: brightest saturated icon colour, not its dark shading
        col = dominant(med[sat] if cfg['kind'] == 'screen' and sat.sum() >= 8 else med[lift])
        L, T, R, B = cfg['clip']; win = np.zeros((H, W), bool); win[y0 + T:y1 + B + 1, x0 + L:x1 + R + 1] = True
        d = distance_transform_edt(~face)
        ring = (d > 0) & (d <= RC) & win & (dark | greyb) & static & ~face
        alpha = np.where(ring, AMAX * ((RC + 1 - d) / RC) ** 2, 0.0)
        chips.append(dict(name=name, face=(x0, y0, x1, y1), win=(x0 + L, y0 + T, x1 + R, y1 + B), colour=col,
                          lift=lift, ring=ring, alpha=alpha, face_m=face, win_m=win))
        region |= face | win
    return chips, region
def up3(m): return np.repeat(np.repeat(m, 3, 0), 3, 1)
def breath(n, fps): t = n / fps; return 0.5 - 0.5 * math.cos(2 * math.pi * t / PER)
def apply(fr, chips3, s):
    a = fr.astype(float)
    for c in chips3:
        lm = c['lift3']; a[lm] = 255 * (a[lm] / 255) ** (1 / (1 + GAMMA * s))
        al = (c['alpha3'] * s)[..., None]; rm = c['ring3'][..., None]
        a = np.where(rm, a * (1 - al) + np.array(c['colour'], float) * al, a)
    return a.round().clip(0, 255).astype(np.uint8)
def run(name):
    cfg = SCREENS[name]; fps = eval(cfg['fps']); chips, region = masks(cfg)
    for c in chips: c['lift3'] = up3(c['lift']); c['ring3'] = up3(c['ring']); c['alpha3'] = up3(c['alpha'])
    region3 = up3(region); ringall = up3(np.any([c['ring'] for c in chips], 0)); liftall = up3(np.any([c['lift'] for c in chips], 0))
    src = sorted(glob.glob('/tmp/lk/%s/f*.png' % cfg['key'])); out = '/tmp/sg_%s' % name; os.makedirs(out, exist_ok=True)
    st = dict(frames=len(src), frames_outside_changed=0, px_outside_changed=0, max_ring_px=0, max_lift_px=0, max_abs_delta=0)
    best = (-1, 0)
    for n, f in enumerate(src):
        base = np.array(Image.open(f).convert('RGB')); s = breath(n, fps); g = apply(base, chips, s)
        ch = (g != base).any(-1)
        o = int((ch & ~region3).sum()); st['px_outside_changed'] += o; st['frames_outside_changed'] += int(o > 0)
        st['max_ring_px'] = max(st['max_ring_px'], int((ch & ringall).sum())); st['max_lift_px'] = max(st['max_lift_px'], int((ch & liftall).sum()))
        st['max_abs_delta'] = max(st['max_abs_delta'], int(np.abs(g.astype(int) - base).max()))
        Image.fromarray(g).save('%s/f%03d.png' % (out, n + 1))
        if s > best[1]: best = (n, s)
    n = best[0]; st['peak_frame'] = n + 1; st['peak_s'] = round(best[1], 4)
    peak = np.array(Image.open('%s/f%03d.png' % (out, n + 1))); lk = np.array(Image.open(src[n]).convert('RGB'))
    Image.fromarray(peak).save('%s_glow_peak_3x.png' % name)
    font = ImageFont.load_default(size=24); sbs = Image.new('RGB', (720 * 2 + 12, 960 + 36), (0, 0, 0))
    sbs.paste(Image.fromarray(lk), (0, 36)); sbs.paste(Image.fromarray(peak), (732, 36)); d = ImageDraw.Draw(sbs)
    d.text((8, 6), 'LOCKED %s (frame %d)' % (name.upper(), n + 1), fill=(230, 230, 230), font=font); d.text((740, 6), 'C FACE GLOW peak', fill=(230, 230, 230), font=font)
    sbs.save('%s_locked_vs_glow_sbs.png' % name)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', cfg['fps'], '-i', out + '/f%03d.png', '-c:v', 'libx264', '-profile:v', 'main',
                    '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', '%s_glow_3x.mp4' % name], check=True)
    st['loop_s'] = round(len(src) / fps, 3); st['loop_phase_jump_s'] = round(breath(len(src), fps), 3)
    st['chips'] = [dict(name=c['name'], face=c['face'], win=c['win'], colour=c['colour'], ring_px_1x=int(c['ring'].sum()), lift_px_1x=int(c['lift'].sum())) for c in chips]
    json.dump(st, open('%s_stats.json' % name, 'w'), indent=1); print(name, json.dumps(st)); return st
if __name__ == '__main__':
    for nm in sys.argv[1:] or ['terminal', 'morpheus', 'scan']: run(nm)
