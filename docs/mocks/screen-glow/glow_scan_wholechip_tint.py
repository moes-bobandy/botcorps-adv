# PREVIOUS version (superseded 2026-11-03 5:51 PM ET): whole-chip firmware tint. Outputs now carry the _wholechip_tint suffix.
# SCAN screen glow, re-done on the CURRENT WIFI / BLE faces (Mobandy 2026-11-03: "The WiFi and Bluetooth bot don't have grey faces that's old").
# Base = LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4 (decoded by prep_locked.py into /tmp/lk/S) + the firmware-current always-on
# SCAN tints from src/live/lv_core.h (commit 156f359, unchanged at HEAD): lv_tint_setup() LV_SC_SCAN adds LV_T_SWIFI on {32,30,74,92} and
# LV_T_SBLE on {32,93,74,152}; lv_tint_state() gives LV_TM_COLOR amt 5 (idle) toward LV_WIFI_COL 0xff7a00 / LV_BLE_COL 0x2f6bff; lv_tx()
# LV_TM_COLOR is reproduced here bit-for-bit in RGB565 (C truncating division). Masks (face / window / ring) come from the untinted median,
# exactly as glow_screens.py; glow colour = the tinted face's own dominant colour. Big SCANbot untouched. MOCK ONLY: no firmware, no flash.
# Output: 168 frames at 30 fps (4 x 1.4 s breaths, seamless breath loop); source frame for output n = floor(n * (100/3) / 30).
import sys, os, glob, json, subprocess, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import glow_screens as G
TINTS = {'WIFI': ((32, 30, 74, 92), 0xff7a00), 'BLE': ((32, 93, 74, 152), 0x2f6bff)}   # lv_core.h lv_tint_setup / LV_WIFI_COL / LV_BLE_COL
AMT, NF, FPS, SRC_FPS = 5, 168, 30, 100 / 3
def cdiv(a, b): return np.trunc(a / b).astype(np.int64)   # C integer division (toward zero)
def lv_tm_color(px, col, amt=AMT):
    """lv_core.h lv_tx() LV_TM_COLOR on 8-bit RGB pixels: to RGB565, colourize keeping the shading, mix amt/8, back to 8-bit like sim dump()."""
    p = px.astype(np.int64); r5, g6, b5 = p[..., 0] >> 3, p[..., 1] >> 2, p[..., 2] >> 3
    R, Gc, Bl = r5 << 3, g6 << 2, b5 << 3; L = (R * 3 + Gc * 6 + Bl) // 10
    tr = np.minimum(((col >> 16) & 255) * L // 150, 255); tg = np.minimum(((col >> 8) & 255) * L // 150, 255); tb = np.minimum((col & 255) * L // 150, 255)
    t5, t6, t7 = tr >> 3, tg >> 2, tb >> 3                       # lv_rgb(): (r & 0xF8), (g & 0xFC), b >> 3
    o5 = r5 + cdiv((t5 - r5) * amt, 8); o6 = g6 + cdiv((t6 - g6) * amt, 8); o7 = b5 + cdiv((t7 - b5) * amt, 8)
    return np.stack([(o5 << 3) | (o5 >> 2), (o6 << 2) | (o6 >> 4), (o7 << 3) | (o7 >> 2)], -1).astype(np.uint8)
def tint_frame(fr):
    out = fr.copy()
    for (x0, y0, x1, y1), col in TINTS.values():
        sl = np.s_[y0 * 3:(y1 + 1) * 3, x0 * 3:(x1 + 1) * 3]; out[sl] = lv_tm_color(fr[sl], col)
    return out
def tintmask3():
    m = np.zeros((960, 720), bool)
    for (x0, y0, x1, y1), _ in TINTS.values(): m[y0 * 3:(y1 + 1) * 3, x0 * 3:(x1 + 1) * 3] = True
    return m
def main():
    cfg = G.SCREENS['scan']; chips, region = G.masks(cfg)             # geometry from the untinted locked median (same as before)
    med = np.load('/tmp/lk/med_S.npy'); tmed = med.copy()
    for (x0, y0, x1, y1), col in TINTS.values(): tmed[y0:y1 + 1, x0:x1 + 1] = lv_tm_color(med[y0:y1 + 1, x0:x1 + 1], col)
    dark = med.astype(int).sum(-1) < 90
    for c in chips:
        if c['name'] in TINTS: c['colour'] = G.dominant(tmed[c['lift']])   # the current face's own colour
        c['lift3'] = G.up3(c['lift']); c['ring3'] = G.up3(c['ring']); c['alpha3'] = G.up3(c['alpha'])
    region3 = G.up3(region); tm3 = tintmask3(); ringall = G.up3(np.any([c['ring'] for c in chips], 0)); liftall = G.up3(np.any([c['lift'] for c in chips], 0))
    src = sorted(glob.glob('/tmp/lk/S/f*.png')); bdir, gdir = '/tmp/sgw_base', '/tmp/sgw_glow'; os.makedirs(bdir, exist_ok=True); os.makedirs(gdir, exist_ok=True)
    st = dict(frames=NF, fps=FPS, src_frames_used=[], frames_outside_changed=0, px_outside_changed=0, base_px_changed_outside_tint_rects=0,
              max_ring_px=0, max_lift_px=0, max_abs_delta=0)
    best = (-1, -1.0)
    for n in range(NF):
        k = int(math.floor(n * SRC_FPS / FPS + 1e-9)); st['src_frames_used'].append(k + 1)
        lk = np.array(Image.open(src[k]).convert('RGB')); base = tint_frame(lk)
        st['base_px_changed_outside_tint_rects'] += int(((base != lk).any(-1) & ~tm3).sum())
        s = 0.5 - 0.5 * math.cos(2 * math.pi * (n / FPS) / G.PER); g = G.apply(base, chips, s)
        ch = (g != base).any(-1); o = int((ch & ~region3).sum()); st['px_outside_changed'] += o; st['frames_outside_changed'] += int(o > 0)
        st['max_ring_px'] = max(st['max_ring_px'], int((ch & ringall).sum())); st['max_lift_px'] = max(st['max_lift_px'], int((ch & liftall).sum()))
        st['max_abs_delta'] = max(st['max_abs_delta'], int(np.abs(g.astype(int) - base).max()))
        Image.fromarray(base).save('%s/f%03d.png' % (bdir, n + 1)); Image.fromarray(g).save('%s/f%03d.png' % (gdir, n + 1))
        if s > best[1]: best = (n, s)
    n = best[0]; st['peak_frame'] = n + 1; st['peak_s'] = round(best[1], 4); st['peak_src_frame'] = st['src_frames_used'][n]
    peak = np.array(Image.open('%s/f%03d.png' % (gdir, n + 1))); base = np.array(Image.open('%s/f%03d.png' % (bdir, n + 1)))
    Image.fromarray(peak).save('scan_glow_peak_3x_wholechip_tint.png'); Image.fromarray(base).save('scan_current_base_3x_wholechip_tint.png')
    font = ImageFont.load_default(size=24); sbs = Image.new('RGB', (720 * 2 + 12, 960 + 36), (0, 0, 0))
    sbs.paste(Image.fromarray(base), (0, 36)); sbs.paste(Image.fromarray(peak), (732, 36)); d = ImageDraw.Draw(sbs)
    d.text((8, 6), 'CURRENT SCAN base (orange WIFI / blue BLE), frame %d' % (n + 1), fill=(230, 230, 230), font=font)
    d.text((740, 6), 'C FACE GLOW peak', fill=(230, 230, 230), font=font); sbs.save('scan_locked_vs_glow_sbs_wholechip_tint.png')
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-framerate', str(FPS), '-i', gdir + '/f%03d.png', '-c:v', 'libx264', '-profile:v', 'main',
                    '-crf', '12', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', 'scan_glow_3x_wholechip_tint.mp4'], check=True)
    st['loop_s'] = NF / FPS; st['loop_phase_jump_s'] = round(0.5 - 0.5 * math.cos(2 * math.pi * (NF / FPS) / G.PER), 4)
    st['tints'] = {k: dict(rect=v[0], col='#%06x' % v[1], amt='%d/8' % AMT) for k, v in TINTS.items()}
    st['chips'] = [dict(name=c['name'], face=c['face'], win=c['win'], colour=c['colour'], ring_px_1x=int(c['ring'].sum()), lift_px_1x=int(c['lift'].sum())) for c in chips]
    st['src_frames_used'] = '%d..%d' % (st['src_frames_used'][0], st['src_frames_used'][-1])
    json.dump(st, open('scan_stats_wholechip_tint.json', 'w'), indent=1); print(json.dumps(st))
if __name__ == '__main__': main()
