# SCAN screen glow on the CURRENT WIFI / BLE faces, face-only tint (Mobandy 2026-11-03: "Tint only the WiFi and Bluetooth faces orange
# and blue, keep their frames grey so the glow shows."). MOCK ONLY: no firmware change, no flash.
# Base = LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4 (decoded by prep_locked.py into /tmp/lk/S). Only the WIFI / BLE face square
# (detect.py face mask: inside the black window ring, eyes included) is tinted; bezel, label, pins, dash ring and everything else are the
# locked pixels. Tint keeps the shading: out = COL * L / L_MID (per channel, clipped), L = (3R+6G+B)/10, L_MID = luminance of the locked
# face mid grey (105,119,131) = 116.0, so the mid grey becomes exactly COL, the highlight a lighter orange/blue, the shadow band a dark one,
# black eyes stay black. COL = LV_WIFI_COL 0xff7a00 / LV_BLE_COL 0x2f6bff (src/live/lv_core.h).
# v2 (Mobandy 2026-11-03 ~6 PM ET, "Yeah add a bit more"): face pixels of WIFI / BLE / GIB are NEVER changed by the glow (no lift, no hue shift);
# only the halo breathes, a bit stronger than C: AMAX 1.00 (was 0.80), RC 9 px at 1x (was 7), same 1.4 s raised-cosine, same alpha curve
# AMAX*((RC+1-d)/RC)^2*s, colour = each face's own dominant colour (a blend toward it, so never whiter than the face).
# Halo px: within RC of the face, static in the locked clip, and either (a) inside the C window (-7,-4,7,6) and black/grey like before, or
# (b) in the wider (-9,-9,9,9) box and grey bezel only (never dark px outside the window, so WIFI/BLE/GIB label text, pins and the
# purple dash ring are not touched). 168 frames at 30 fps. Previous version: glow_scan_faceonly_v1.py (b00670c).
import sys, os, glob, json, subprocess, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glow_screens as G
COLS = {'WIFI': (0xff, 0x7a, 0x00), 'BLE': (0x2f, 0x6b, 0xff)}
L_MID = (105 * 3 + 119 * 6 + 131) / 10
NF, FPS, SRC_FPS = 168, 30, 100 / 3
AMAX2, RC2, WIDE = 1.00, 9, (-9, -9, 9, 9)
from scipy.ndimage import distance_transform_edt
def face_tint(px, col):
    p = px.astype(float); L = (p[..., 0] * 3 + p[..., 1] * 6 + p[..., 2]) / 10
    return (np.array(col, float) * (L / L_MID)[..., None]).round().clip(0, 255).astype(np.uint8)
def halo_masks(chips):
    med = np.load('/tmp/lk/med_S.npy').astype(int); rng = np.load('/tmp/lk/rng_S.npy'); rng = rng.max(-1) if rng.ndim == 3 else rng
    s = med.sum(-1); ptp = med.max(-1) - med.min(-1); dark = s < 90; greyb = (ptp < 40) & (s < 450) & ~dark; static = rng <= 40
    H, W = s.shape; region = np.zeros((H, W), bool)
    for c in chips:
        x0, y0, x1, y1 = c['face']; L, T, R, B = WIDE; wide = np.zeros((H, W), bool); wide[y0 + T:y1 + B + 1, x0 + L:x1 + R + 1] = True
        face = c['face_m']; d = distance_transform_edt(~face)
        ring = (d > 0) & (d <= RC2) & static & ~face & ((c['win_m'] & (dark | greyb)) | (wide & greyb))
        c['ring'] = ring; c['alpha'] = np.where(ring, AMAX2 * ((RC2 + 1 - d) / RC2) ** 2, 0.0).clip(0, 1)
        region |= face | c['win_m'] | ring
    return region
def apply_halo(fr, chips, s):
    a = fr.astype(float)
    for c in chips:
        al = (c['alpha3'] * s)[..., None]; a = np.where(c['ring3'][..., None], a * (1 - al) + np.array(c['colour'], float) * al, a)
    return a.round().clip(0, 255).astype(np.uint8)
def main():
    cfg = G.SCREENS['scan']; chips, _ = G.masks(cfg); region = halo_masks(chips)
    med = np.load('/tmp/lk/med_S.npy'); tmed = med.copy(); fm3 = {}
    for c in chips:
        if c['name'] in COLS:
            fm = c['face_m']; tmed[fm] = face_tint(med[fm], COLS[c['name']]); fm3[c['name']] = G.up3(fm)
            c['colour'] = G.dominant(tmed[c['lift']])
        c['lift3'] = G.up3(c['lift']); c['ring3'] = G.up3(c['ring']); c['alpha3'] = G.up3(c['alpha'])
    facem3 = np.any(list(fm3.values()), 0); allface3 = G.up3(np.any([c['face_m'] for c in chips], 0))
    region3 = G.up3(region); ringall = G.up3(np.any([c['ring'] for c in chips], 0)); liftall = G.up3(np.any([c['lift'] for c in chips], 0))
    src = sorted(glob.glob('/tmp/lk/S/f*.png')); bdir, gdir = '/tmp/sgc_base', '/tmp/sgc_glow'
    for d in (bdir, gdir): os.makedirs(d, exist_ok=True); [os.remove(f) for f in glob.glob(d + '/*.png')]
    st = dict(settings=dict(AMAX=AMAX2, RC=RC2, period_s=G.PER, lift='none', wide_box=WIDE, window=cfg['clip']), face_px_changed=0, frames=NF, fps=FPS, src_frames_used=[], frames_outside_changed=0, px_outside_changed=0, base_px_changed_outside_faces=0,
              max_ring_px=0, max_lift_px=0, max_abs_delta=0)
    best = (-1, -1.0)
    for n in range(NF):
        k = int(math.floor(n * SRC_FPS / FPS + 1e-9)); st['src_frames_used'].append(k + 1)
        lk = np.array(Image.open(src[k]).convert('RGB')); base = lk.copy()
        for nm, m in fm3.items(): base[m] = face_tint(lk[m], COLS[nm])
        st['base_px_changed_outside_faces'] += int(((base != lk).any(-1) & ~facem3).sum())
        s = 0.5 - 0.5 * math.cos(2 * math.pi * (n / FPS) / G.PER); g = apply_halo(base, chips, s)
        st['face_px_changed'] += int(((g != base).any(-1) & allface3).sum())
        ch = (g != base).any(-1); o = int((ch & ~region3).sum()); st['px_outside_changed'] += o; st['frames_outside_changed'] += int(o > 0)
        st['max_ring_px'] = max(st['max_ring_px'], int((ch & ringall).sum())); st['max_lift_px'] = max(st['max_lift_px'], int((ch & liftall).sum()))
        st['max_abs_delta'] = max(st['max_abs_delta'], int(np.abs(g.astype(int) - base).max()))
        Image.fromarray(base).save('%s/f%03d.png' % (bdir, n + 1)); Image.fromarray(g).save('%s/f%03d.png' % (gdir, n + 1))
        if s > best[1]: best = (n, s)
    n = best[0]; st['peak_frame'] = n + 1; st['peak_s'] = round(best[1], 4); st['peak_src_frame'] = st['src_frames_used'][n]
    peak = np.array(Image.open('%s/f%03d.png' % (gdir, n + 1))); base = np.array(Image.open('%s/f%03d.png' % (bdir, n + 1)))
    Image.fromarray(peak).save('scan_glow_peak_3x.png'); Image.fromarray(base).save('scan_current_base_3x.png')
    prev = Image.open('scan_glow_peak_3x_faceonly_v1.png').convert('RGB')
    font = ImageFont.load_default(size=24); sbs = Image.new('RGB', (720 * 2 + 12, 960 + 36), (0, 0, 0))
    sbs.paste(prev, (0, 36)); sbs.paste(Image.fromarray(peak), (732, 36)); d = ImageDraw.Draw(sbs)
    d.text((8, 6), 'b00670c faceonly_v1 peak (lift, 0.80, 7px)', fill=(230, 230, 230), font=font)
    d.text((740, 6), 'NEW v2 peak (frame %d): faces fixed, 1.00, 9px' % (n + 1), fill=(230, 230, 230), font=font); sbs.save('scan_faceonly_v1_vs_v2_sbs.png')
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', gdir + '/f%03d.png', '-c:v', 'libx264', '-profile:v', 'main',
                    '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', 'scan_glow_3x.mp4'], check=True)
    st['loop_s'] = NF / FPS; st['loop_phase_jump_s'] = round(0.5 - 0.5 * math.cos(2 * math.pi * (NF / FPS) / G.PER), 4)
    st['face_tint'] = {k: dict(col='#%02x%02x%02x' % v, mid=list(face_tint(np.array([[105, 119, 131]]), v)[0].tolist()),
                               hi=face_tint(np.array([[165, 165, 165]]), v)[0].tolist(), shadow=face_tint(np.array([[40, 54, 64]]), v)[0].tolist()) for k, v in COLS.items()}
    st['chips'] = [dict(name=c['name'], face=c['face'], win=c['win'], colour=c['colour'], ring_px_1x=int(c['ring'].sum()), ring_px_outside_window_1x=int((c['ring'] & ~c['win_m']).sum()), lift_px_1x=int(c['lift'].sum())) for c in chips]
    st['src_frames_used'] = '%d..%d' % (st['src_frames_used'][0], st['src_frames_used'][-1])
    json.dump(st, open('scan_stats.json', 'w'), indent=1); print(json.dumps(st))
if __name__ == '__main__': main()
