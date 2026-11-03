# PREVIOUS version faceonly_v1 (b00670c, superseded 2026-11-03 ~6 PM ET): face lift on, AMAX 0.80, RC 7. Outputs carry _faceonly_v1.
# SCAN screen glow on the CURRENT WIFI / BLE faces, face-only tint (Mobandy 2026-11-03: "Tint only the WiFi and Bluetooth faces orange
# and blue, keep their frames grey so the glow shows."). MOCK ONLY: no firmware change, no flash.
# Base = LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4 (decoded by prep_locked.py into /tmp/lk/S). Only the WIFI / BLE face square
# (detect.py face mask: inside the black window ring, eyes included) is tinted; bezel, label, pins, dash ring and everything else are the
# locked pixels. Tint keeps the shading: out = COL * L / L_MID (per channel, clipped), L = (3R+6G+B)/10, L_MID = luminance of the locked
# face mid grey (105,119,131) = 116.0, so the mid grey becomes exactly COL, the highlight a lighter orange/blue, the shadow band a dark one,
# black eyes stay black. COL = LV_WIFI_COL 0xff7a00 / LV_BLE_COL 0x2f6bff (src/live/lv_core.h).
# Then the locked C glow (glow_screens.apply, masks from the untinted median) in each face's own new colour. 168 frames at 30 fps.
import sys, os, glob, json, subprocess, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glow_screens as G
COLS = {'WIFI': (0xff, 0x7a, 0x00), 'BLE': (0x2f, 0x6b, 0xff)}
L_MID = (105 * 3 + 119 * 6 + 131) / 10
NF, FPS, SRC_FPS = 168, 30, 100 / 3
def face_tint(px, col):
    p = px.astype(float); L = (p[..., 0] * 3 + p[..., 1] * 6 + p[..., 2]) / 10
    return (np.array(col, float) * (L / L_MID)[..., None]).round().clip(0, 255).astype(np.uint8)
def main():
    cfg = G.SCREENS['scan']; chips, region = G.masks(cfg)
    med = np.load('/tmp/lk/med_S.npy'); tmed = med.copy(); fm3 = {}
    for c in chips:
        if c['name'] in COLS:
            fm = c['face_m']; tmed[fm] = face_tint(med[fm], COLS[c['name']]); fm3[c['name']] = G.up3(fm)
            c['colour'] = G.dominant(tmed[c['lift']])
        c['lift3'] = G.up3(c['lift']); c['ring3'] = G.up3(c['ring']); c['alpha3'] = G.up3(c['alpha'])
    facem3 = np.any(list(fm3.values()), 0)
    region3 = G.up3(region); ringall = G.up3(np.any([c['ring'] for c in chips], 0)); liftall = G.up3(np.any([c['lift'] for c in chips], 0))
    src = sorted(glob.glob('/tmp/lk/S/f*.png')); bdir, gdir = '/tmp/sgf_base', '/tmp/sgf_glow'
    for d in (bdir, gdir): os.makedirs(d, exist_ok=True); [os.remove(f) for f in glob.glob(d + '/*.png')]
    st = dict(frames=NF, fps=FPS, src_frames_used=[], frames_outside_changed=0, px_outside_changed=0, base_px_changed_outside_faces=0,
              max_ring_px=0, max_lift_px=0, max_abs_delta=0)
    best = (-1, -1.0)
    for n in range(NF):
        k = int(math.floor(n * SRC_FPS / FPS + 1e-9)); st['src_frames_used'].append(k + 1)
        lk = np.array(Image.open(src[k]).convert('RGB')); base = lk.copy()
        for nm, m in fm3.items(): base[m] = face_tint(lk[m], COLS[nm])
        st['base_px_changed_outside_faces'] += int(((base != lk).any(-1) & ~facem3).sum())
        s = 0.5 - 0.5 * math.cos(2 * math.pi * (n / FPS) / G.PER); g = G.apply(base, chips, s)
        ch = (g != base).any(-1); o = int((ch & ~region3).sum()); st['px_outside_changed'] += o; st['frames_outside_changed'] += int(o > 0)
        st['max_ring_px'] = max(st['max_ring_px'], int((ch & ringall).sum())); st['max_lift_px'] = max(st['max_lift_px'], int((ch & liftall).sum()))
        st['max_abs_delta'] = max(st['max_abs_delta'], int(np.abs(g.astype(int) - base).max()))
        Image.fromarray(base).save('%s/f%03d.png' % (bdir, n + 1)); Image.fromarray(g).save('%s/f%03d.png' % (gdir, n + 1))
        if s > best[1]: best = (n, s)
    n = best[0]; st['peak_frame'] = n + 1; st['peak_s'] = round(best[1], 4); st['peak_src_frame'] = st['src_frames_used'][n]
    peak = np.array(Image.open('%s/f%03d.png' % (gdir, n + 1))); base = np.array(Image.open('%s/f%03d.png' % (bdir, n + 1)))
    Image.fromarray(peak).save('scan_glow_peak_3x_faceonly_v1.png'); Image.fromarray(base).save('scan_current_base_3x_faceonly_v1.png')
    prev = Image.open('scan_glow_peak_3x_wholechip_tint.png').convert('RGB')
    font = ImageFont.load_default(size=24); sbs = Image.new('RGB', (720 * 2 + 12, 960 + 36), (0, 0, 0))
    sbs.paste(prev, (0, 36)); sbs.paste(Image.fromarray(peak), (732, 36)); d = ImageDraw.Draw(sbs)
    d.text((8, 6), 'PREVIOUS: whole-chip tint, glow peak', fill=(230, 230, 230), font=font)
    d.text((740, 6), 'NEW: face-only tint, glow peak (frame %d)' % (n + 1), fill=(230, 230, 230), font=font); sbs.save('scan_wholechip_vs_faceonly_sbs_faceonly_v1.png')
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', gdir + '/f%03d.png', '-c:v', 'libx264', '-profile:v', 'main',
                    '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', 'scan_glow_3x_faceonly_v1.mp4'], check=True)
    st['loop_s'] = NF / FPS; st['loop_phase_jump_s'] = round(0.5 - 0.5 * math.cos(2 * math.pi * (NF / FPS) / G.PER), 4)
    st['face_tint'] = {k: dict(col='#%02x%02x%02x' % v, mid=list(face_tint(np.array([[105, 119, 131]]), v)[0].tolist()),
                               hi=face_tint(np.array([[165, 165, 165]]), v)[0].tolist(), shadow=face_tint(np.array([[40, 54, 64]]), v)[0].tolist()) for k, v in COLS.items()}
    st['chips'] = [dict(name=c['name'], face=c['face'], win=c['win'], colour=c['colour'], ring_px_1x=int(c['ring'].sum()), lift_px_1x=int(c['lift'].sum())) for c in chips]
    st['src_frames_used'] = '%d..%d' % (st['src_frames_used'][0], st['src_frames_used'][-1])
    json.dump(st, open('scan_stats_faceonly_v1.json', 'w'), indent=1); print(json.dumps(st))
if __name__ == '__main__': main()
