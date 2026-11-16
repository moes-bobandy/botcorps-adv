#!/usr/bin/env python3
"""CONTROLbot v3 alert bars (mock, Mobandy 2026-11-16 11:18-11:20 AM ET: "Clone all 11 bars but tint them to follow the selected Color UI theme").
The locked SECURITYbot BAR cycle (mocks/topbar-v2b-colors, lock 05d2a48) is rendered by VERBATIM copies of render_topbar_v1/v2b.py.
Only the ink colour of each header pixel is remapped afterwards: black stays black, white highlights stay white (whiteness kept),
brightness (max channel) kept, ALERT RED kept red; every other ink takes the selected theme's ink at that pixel (bar_theme_ink.cpp).
Frames: our v2 CONTROLbot Color UI frame per theme with header rows y1..23 x1..238 replaced. Nothing else changes."""
import sys, json, subprocess, hashlib
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image
HERE = Path(__file__).resolve().parent; OUTD = HERE.parent; V2D = OUTD.parent / 'v2'
SB = Path('/tmp/sbwt')                         # git worktree of securitybot origin/backup/securitybot/gibson-tower-home-mocks (read only)
LOCKD = SB / 'mocks/topbar-v2b-colors'
sys.path.insert(0, str(HERE))
import render_topbar_v1 as R
R.ROOT = SB; R.V9C = SB / 'mocks/controlbot-bigchip-v9c-glasses/01_panel@1x.png'
import render_topbar_v2b as V
V.ROOT = SB; V.W = Path('/tmp/cbbars_v3'); R.W = V.W; V.W.mkdir(parents=True, exist_ok=True)
ZIG = '/home/box/.local/lib/python3.13/site-packages/ziglang/zig'
SEC_SPEC = Path('/workspace/securitybot/mocks/security-color-themes-v2')
THEMES = ['01_sec_lime', '02_sec_hivis', '03_sec_nvg', '04_sec_nvwht', '05_sec_iron', '06_sec_heat', '07_sec_whhot', '08_sec_redht', '09_sec_alarm', '10_sec_siren', '11_sec_perim',
          '12_cyd_grape', '13_cyd_magenta', '14_cyd_toxic', '15_cyd_ice', '16_cyd_blue', '17_cyd_orange', '18_cyd_gold', '19_cyd_synth', '20_cyd_aura', '21_cyd_prism', '22_cyd_gltch', '23_cyd_rose']
SAMPLE = [1, 4, 11, 18]   # SEC HI-VIS BLUE, SEC IR IRONBOW, CYD NEON GRAPE, CYD SYNTHWAVE
HY0, HY1, HX0, HX1 = R.HY0, R.HY1, R.HX0, R.HX1

def build():
    exe = V.W / 'bar_theme_ink'
    subprocess.check_call([ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-w', '-I/workspace/cyd-portrait-wt/src/live', '-I' + str(SEC_SPEC), '-I' + str(HERE),
                           '-o', str(exe), str(HERE / 'bar_theme_ink.cpp')])
    inks = []
    for t in range(23):
        p = V.W / f'ink{t}.rgb'; subprocess.check_call([str(exe), str(t), str(p)])
        inks.append(np.fromfile(p, np.uint8).reshape(23, 238, 3).astype(float))
    return inks

def is_alert_red(h):
    r, g, b = h[..., 0].astype(float), h[..., 1].astype(float), h[..., 2].astype(float)
    return (r >= 40) & (g <= 0.35 * r) & (b <= 0.45 * r)

def tint(h, T):
    """h: 23x238 RGB565-exact bar header; T: theme ink. Keeps black, whiteness, brightness, alert red."""
    a = h.astype(float); mx = a.max(2); mn = a.min(2); w = np.where(mx > 0, mn / np.maximum(mx, 1), 0)[..., None]
    o = (T * (1 - w) + 255 * w) * (mx[..., None] / 255)
    o = np.where(((mx < 8) | is_alert_red(h))[..., None], a, o)
    return R.to565(np.clip(np.round(o), 0, 255).astype(np.uint8))

def bar_header(vid, t): return V.header(vid, t)[0]

def main():
    inks = build()
    v2f = [np.array(Image.open(V2D / f'{n}@1x.png').convert('RGB')) for n in THEMES]
    mask = np.array(Image.open(V2D / '00_coil_pin_mask@1x.png')) > 0
    hdr = np.zeros((320, 240), bool); hdr[HY0:HY1, HX0:HX1] = True
    base9 = np.array(Image.open(R.V9C).convert('RGB'))
    proof = {'clone_check': [], 'tinted': []}
    # (1) untinted clone check vs the LOCKED v2b stills
    for i, (vid, slug, name, grp) in enumerate(V.VARIANTS, 1):
        h = bar_header(vid, V.STILL_T[vid]); full = R.compose(base9, h)
        lock = np.array(Image.open(LOCKD / f'{i:02d}_{vid}_{slug}@1x.png').convert('RGB'))
        proof['clone_check'].append({'bar': vid, 'still_frame': V.STILL_T[vid], 'header_px_diff_vs_locked': int(np.any(full[HY0:HY1, HX0:HX1] != lock[HY0:HY1, HX0:HX1], 2).sum()),
                                     'full_frame_px_diff_vs_locked': int(np.any(full != lock, 2).sum())})
    # (2) grid: 11 bars x 4 sample themes (header crops, R.smooth @3x like the locked header crops) + full stills
    rows = []
    hdr_lbl = [R.label_strip('BAR / THEME', 300)] + [R.label_strip(THEMES[t][3:].upper(), 720) for t in SAMPLE]
    W3 = 300 + 4 * 720 + 4 * 12
    def hcat(parts):
        out = [parts[0]]
        for p in parts[1:]: out += [np.full((p.shape[0], 12, 3), 40, np.uint8), p]
        return np.concatenate(out, 1)
    rows.append(hcat([np.pad(x, ((0, max(y.shape[0] for y in hdr_lbl) - x.shape[0]), (0, 0), (0, 0))) for x in hdr_lbl]))
    for i, (vid, slug, name, grp) in enumerate(V.VARIANTS, 1):
        h = bar_header(vid, V.STILL_T[vid]); cells = [np.pad(R.label_strip(f'{vid}', 300), ((0, 0), (0, 0), (0, 0)))]
        crops = []
        for t in SAMPLE:
            th = tint(h, inks[t]); fr = R.compose(v2f[t], th)
            d = np.any(fr != v2f[t], 2)
            rec = {'bar': vid, 'theme': THEMES[t], 'changed_outside_header': int((d & ~hdr).sum())}
            if t < 11:
                lk = np.array(Image.open(SEC_SPEC / f'{t + 1:02d}_{THEMES[t][7:]}@1x.png').convert('RGB'))
                rec['changed_vs_locked_sec_v2_outside_header_and_coilpin_mask'] = int((np.any(fr != lk, 2) & ~hdr & ~mask).sum())
            proof['tinted'].append(rec)
            Image.fromarray(fr).save(OUTD / f'stills/{i:02d}_{vid}__{THEMES[t]}@1x.png')
            crops.append(R.smooth(fr)[:75])
        lab = R.label_strip(f'{i:02d} {vid}', 300); lab = np.pad(lab, ((0, max(0, 75 - lab.shape[0])), (0, 0), (0, 0)))[:75]
        rows.append(hcat([lab] + crops))
    sep = np.full((10, W3, 3), 20, np.uint8); g = [rows[0]]
    for r in rows[1:]: g += [sep, r]
    Image.fromarray(np.concatenate(g, 0)).save(OUTD / '00_alertbars_11x4_grid@3x.png')
    # untinted clone grid (locked colours) for reference
    Image.fromarray(np.concatenate([R.smooth(R.compose(base9, bar_header(v[0], V.STILL_T[v[0]])))[:75] for v in V.VARIANTS], 0)).save(OUTD / '00_clone_untinted_11bars@3x.png')
    # (3) mp4: 23 themes, each 32 frames @30fps (24 hold + tap press 5 + release 3), BAR cycles one step per theme; bar animation runs on its own 105-frame clock
    acc = [int(x[2:], 16) for x in __import__('re').findall(r'0x[0-9a-fA-F]{6}', __import__('re').search(r'SEC_ACC\[[^\]]*\]\s*=\s*\{([^}]*)\}', (SEC_SPEC / 'themes_spec_v2.h').read_text()).group(1))]
    acc += [int(x[2:], 16) for x in __import__('re').findall(r'0x[0-9a-fA-F]{6}', __import__('re').search(r'LV_ACC\[[^\]]*\]\s*=\s*\{([^}]*)\}', (HERE / 'cyd_themes_lv_core_copy.h').read_text()).group(1))]
    # face box from the v2 frame: role-1 face = the big theme-coloured square; reuse v2's tap geometry (face bbox 65..175 x 88..198 approx) measured from the frame
    fb = (np.abs(v2f[14].astype(int) - np.array([0, 255, 255])).sum(2) < 60); ys, xs = np.nonzero(fb[60:230, 40:200]); fy0, fy1, fx0, fx1 = ys.min() + 60, ys.max() + 60, xs.min() + 40, xs.max() + 40
    face = np.zeros((320, 240), bool); face[fy0:fy1 + 1, fx0:fx1 + 1] = True
    eyes = np.all(v2f[14] < 24, 2) & face; face &= ~eyes                                   # v2 flaw fixed here: eyes not tinted by the flash
    yy, xx = np.mgrid[0:320, 0:240]; dist = np.maximum(np.maximum(fx0 - xx, xx - fx1), np.maximum(fy0 - yy, yy - fy1)); ring = (dist > 0) & (dist <= 4) & ~hdr
    cx, cy = (fx0 + fx1) // 2, (fy0 + fy1) // 2
    def tap(f, c, af, ag, r):
        g = f.astype(np.float32); col = np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], np.float32)
        g[face] = g[face] * (1 - af) + (col * 0.6 + 255 * 0.4) * af
        w = (ag * (1 - (dist - 1) / 4.0))[..., None] * ring[..., None]; g = g * (1 - w) + col * w
        if r: d = np.hypot(xx - cx, yy - cy); m = (d >= r - 1.5) & (d <= r + 0.5) & ~eyes; g[m] = g[m] * 0.3 + 255 * 0.7
        return np.clip(g, 0, 255).astype(np.uint8)
    raw = V.W / 'v3bars.rgb'; k = 0; samples = {}
    with open(raw, 'wb') as fo:
        for t in range(23):
            vid = V.VARIANTS[t % 11][0]
            for j in range(32):
                tt = t if j < 29 else (t + 1) % 23
                fr = R.compose(v2f[tt], tint(bar_header(vid, k % V.NFR), inks[tt]))
                if 24 <= j < 29: fr = tap(fr, acc[t], [0.0, 0.25, 0.55, 0.45, 0.3][j - 24], [0.5, 0.9, 1.0, 0.9, 0.8][j - 24], [6, 10, 14, 18, 22][j - 24])
                elif j >= 29: fr = tap(fr, acc[tt], [0.25, 0.1, 0.0][j - 29], [0.7, 0.4, 0.15][j - 29], 0)
                fo.write(R.smooth(fr).tobytes()); k += 1
                if j in (10, 26): samples[f'{t:02d}_{j}'] = fr
    mp4 = OUTD / 'alertbars_theme_tap_cycle@3x.mp4'
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', '720x960', '-framerate', '30', '-i', str(raw),
                    '-c:v', 'libx264', '-preset', 'slow', '-crf', '16', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(mp4)], check=True)
    np.savez_compressed(V.W / 'samples.npz', **samples)
    proof['summary'] = {'clone_header_all_zero': all(c['header_px_diff_vs_locked'] == 0 for c in proof['clone_check']),
                        'clone_full_all_zero': all(c['full_frame_px_diff_vs_locked'] == 0 for c in proof['clone_check']),
                        'max_changed_outside_header_vs_v2_controlbot': max(r['changed_outside_header'] for r in proof['tinted']),
                        'max_changed_vs_locked_sec_v2_outside_header_and_mask': max(r.get('changed_vs_locked_sec_v2_outside_header_and_coilpin_mask', 0) for r in proof['tinted'])}
    (OUTD / 'diff_proof.json').write_text(json.dumps(proof, indent=1))
    print(json.dumps(proof['summary']), [c['header_px_diff_vs_locked'] for c in proof['clone_check']])

if __name__ == '__main__': main()
