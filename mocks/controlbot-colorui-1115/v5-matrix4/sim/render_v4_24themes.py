#!/usr/bin/env python3
"""CONTROLbot v4: 24-theme list (Mobandy 2026-11-16 11:26-11:27 AM ET) with the v3 alert-bar tint UNCHANGED (tint/is_alert_red imported from render_alertbars_v3.py).
Body frames come from render_controlbot_colorui.py (same v2 pipeline, ORDER below) -> /tmp/cbcolor_build_v4/frames.npy."""
import sys, json, subprocess
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image
HERE = Path(__file__).resolve().parent; OUTD = HERE.parent
sys.path.insert(0, str(HERE))
import render_alertbars_v3 as A
R, V = A.R, A.V
V.W = Path('/tmp/cbbars_v4'); R.W = V.W; V.W.mkdir(parents=True, exist_ok=True)
ORDER = [23, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 25, 13, 14, 24, 16, 17, 18, 19, 20, 21, 22, 26]   # same as render_controlbot_colorui.ORDER
NAMES = ['CIRCUIT', 'HI-VIS BLUE', 'NV GREEN BLOOM', 'NV WHITE PHOSPHOR', 'IR IRONBOW', 'IR RAINBOW HEAT', 'IR WHITE-HOT', 'IR RED-HOT', 'ALARM STROBE', 'POLICE SIREN',
         'PERIMETER GRID', 'NEON GRAPE', 'HOLO FOIL', 'TOXIC', 'ICE', 'SONAR', 'BLAZE ORANGE', 'GOLD RUSH', 'SYNTHWAVE', 'AURORA', 'PRISM', 'GLITCH', 'HOT ROSE', 'DIGI CAMO']
SRC = ['NEW (was SEC LIME slot)'] + ['SECURITYbot v2'] * 10 + ['CYD', 'NEW (was HOT PINK slot)', 'CYD', 'CYD', 'NEW (was CYD BLUE slot)'] + ['CYD'] * 7 + ['NEW extra']
GRID_BAR = 'CURTN'

def inks_all():
    exe = V.W / 'bar_theme_ink'
    subprocess.check_call([A.ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-w', '-I/workspace/cyd-portrait-wt/src/live', '-I' + str(A.SEC_SPEC), '-I' + str(HERE),
                           '-o', str(exe), str(HERE / 'bar_theme_ink.cpp')])
    out = {}
    for t in ORDER:
        p = V.W / f'ink{t}.rgb'; subprocess.check_call([str(exe), str(t), str(p)]); out[t] = np.fromfile(p, np.uint8).reshape(23, 238, 3).astype(float)
    return out

def main():
    body = np.load('/tmp/cbcolor_build_v4/frames.npy'); mask = np.load('/tmp/cbcolor_build_v4/mask.npy'); inks = inks_all()
    hdr = np.zeros((320, 240), bool); hdr[A.HY0:A.HY1, A.HX0:A.HX1] = True
    base9 = np.array(Image.open(R.V9C).convert('RGB'))
    proof = {'bar_clone_check': [], 'final': [], 'sec_clone_check': []}
    for i, (vid, slug, name, grp) in enumerate(V.VARIANTS, 1):
        full = R.compose(base9, A.bar_header(vid, V.STILL_T[vid])); lock = np.array(Image.open(A.LOCKD / f'{i:02d}_{vid}_{slug}@1x.png').convert('RGB'))
        proof['bar_clone_check'].append({'bar': vid, 'full_frame_px_diff_vs_locked': int(np.any(full != lock, 2).sum())})
    hb = A.bar_header(GRID_BAR, V.STILL_T[GRID_BAR]); finals = []
    for s, t in enumerate(ORDER):
        fr = R.compose(body[s], A.tint(hb, inks[t])); finals.append(fr)
        ref = np.array(Image.open(A.SEC_SPEC / f'{t + 1:02d}_{A.THEMES[t][7:]}@1x.png').convert('RGB')) if t < 11 else None
        rec = {'slot': s + 1, 'theme': NAMES[s], 'changed_vs_body_outside_header': int((np.any(fr != body[s], 2) & ~hdr).sum())}
        if ref is not None:
            rec['changed_vs_locked_sec_v2_outside_header_and_mask'] = int((np.any(fr != ref, 2) & ~hdr & ~mask).sum())
            proof['sec_clone_check'].append({'slot': s + 1, 'theme': NAMES[s], 'locked_png': f'{t + 1:02d}_{A.THEMES[t][7:]}@1x.png', 'px_diff_outside_header_and_mask': rec['changed_vs_locked_sec_v2_outside_header_and_mask']})
        proof['final'].append(rec)
        Image.fromarray(fr).save(OUTD / f'stills/{s + 1:02d}_{NAMES[s].lower().replace(" ", "_")}@1x.png')
    # grid @3x: 6 x 4, nearest x3, labelled
    cw, ch, lh, pad, cols = 720, 960, 76, 24, 6
    grid = Image.new('RGB', (cols * cw + (cols + 1) * pad, 4 * (ch + lh) + 5 * pad), (6, 6, 8))
    for s in range(24):
        cx, cy = pad + (s % cols) * (cw + pad), pad + (s // cols) * (ch + lh + pad)
        grid.paste(Image.fromarray(finals[s].repeat(3, 0).repeat(3, 1)), (cx, cy + lh))
        new = 'NEW' in SRC[s]
        A_ = __import__('render_security_color_themes_v2'); A_.big_text(grid, cx + 4, cy + 6, f'{s + 1:02d} {NAMES[s]}', (120, 255, 200) if new else (255, 255, 255), 4)
        A_.big_text(grid, cx + 4, cy + 42, SRC[s], (160, 170, 180), 4)
    grid.save(OUTD / '00_24themes_grid@3x.png')
    # mp4: same structure as v3-alertbars (32 frames per theme @30fps, tap at 24..31, BAR steps one per theme, bar runs on its own 105-frame clock)
    acc = {}
    for t in ORDER:
        T = inks[t]; acc[t] = int(T[11, 119, 0]) << 16 | int(T[11, 119, 1]) << 8 | int(T[11, 119, 2])   # tap glow = theme ink mid-bar (lifted), covers new themes too
    ice = body[ORDER.index(14)]
    fb = (np.abs(ice.astype(int) - np.array([0, 255, 255])).sum(2) < 60); ys, xs = np.nonzero(fb[60:230, 40:200]); fy0, fy1, fx0, fx1 = ys.min() + 60, ys.max() + 60, xs.min() + 40, xs.max() + 40
    face = np.zeros((320, 240), bool); face[fy0:fy1 + 1, fx0:fx1 + 1] = True; eyes = np.all(ice < 24, 2) & face; face &= ~eyes
    yy, xx = np.mgrid[0:320, 0:240]; dist = np.maximum(np.maximum(fx0 - xx, xx - fx1), np.maximum(fy0 - yy, yy - fy1)); ring = (dist > 0) & (dist <= 4) & ~hdr
    cx, cy = (fx0 + fx1) // 2, (fy0 + fy1) // 2
    def tap(f, c, af, ag, r):
        g = f.astype(np.float32); col = np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], np.float32)
        g[face] = g[face] * (1 - af) + (col * 0.6 + 255 * 0.4) * af
        w = (ag * (1 - (dist - 1) / 4.0))[..., None] * ring[..., None]; g = g * (1 - w) + col * w
        if r: d = np.hypot(xx - cx, yy - cy); m = (d >= r - 1.5) & (d <= r + 0.5) & ~eyes; g[m] = g[m] * 0.3 + 255 * 0.7
        return np.clip(g, 0, 255).astype(np.uint8)
    raw = V.W / 'v4.rgb'; k = 0; samples = {}; bad = 0
    with open(raw, 'wb') as fo:
        for s in range(24):
            vid = V.VARIANTS[s % 11][0]
            for j in range(32):
                ss = s if j < 29 else (s + 1) % 24
                fr = R.compose(body[ss], A.tint(A.bar_header(vid, k % V.NFR), inks[ORDER[ss]]))
                bad += int((np.any(fr != body[ss], 2) & ~hdr).sum())
                if 24 <= j < 29: fr = tap(fr, acc[ORDER[s]], [0.0, 0.25, 0.55, 0.45, 0.3][j - 24], [0.5, 0.9, 1.0, 0.9, 0.8][j - 24], [6, 10, 14, 18, 22][j - 24])
                elif j >= 29: fr = tap(fr, acc[ORDER[ss]], [0.25, 0.1, 0.0][j - 29], [0.7, 0.4, 0.15][j - 29], 0)
                fo.write(R.smooth(fr).tobytes()); k += 1
                if j in (10, 26): samples[f'{s:02d}_{j}'] = fr
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', '720x960', '-framerate', '30', '-i', str(raw),
                    '-c:v', 'libx264', '-preset', 'slow', '-crf', '16', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(OUTD / 'tap_cycle_24themes_alertbars@3x.mp4')], check=True)
    np.savez_compressed(V.W / 'samples.npz', **samples)
    body_proof = json.loads((OUTD / 'diff_proof_body.json').read_text())
    proof['summary'] = {'theme_order': [f'{s + 1:02d} {NAMES[s]} [{SRC[s]}]' for s in range(24)],
        'body_changed_outside_coilpin_mask_max': max(p['changed_outside_mask'] for p in body_proof['per_theme']),
        'grid_stills_changed_outside_header_vs_body_max': max(r['changed_vs_body_outside_header'] for r in proof['final']),
        'mp4_prefx_frames_changed_outside_header_total_px': bad,
        'sec_10_clone_outside_header_and_mask_max': max(r['px_diff_outside_header_and_mask'] for r in proof['sec_clone_check']),
        'sec_10_no_coilpin_pipeline_equals_locked_v2': all(p.get('pipeline_equals_locked_v2', True) for p in body_proof['per_theme']),
        'bar_clone_all_zero': all(c['full_frame_px_diff_vs_locked'] == 0 for c in proof['bar_clone_check'])}
    (OUTD / 'diff_proof.json').write_text(json.dumps(proof, indent=1)); print(json.dumps(proof['summary'], indent=0))

if __name__ == '__main__': main()
