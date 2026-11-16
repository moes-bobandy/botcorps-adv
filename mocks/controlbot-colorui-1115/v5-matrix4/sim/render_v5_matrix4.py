#!/usr/bin/env python3
"""CONTROLbot v5: exactly 4 Matrix banners tinted to the selected theme (Mobandy 2026-11-16 12:17 PM ET "Just 4 Matrix-style banners that tint to the selected theme").
Banners, rendered by VERBATIM copies of the SECURITYbot drafts (branch backup/securitybot/gibson-tower-home-mocks, read through a worktree at /tmp/sbwt):
  CURTN, TERM  <- render_topbar_v2b.py  (mocks/topbar-v2b-colors, locked BAR cycle 05d2a48)
  RAIN (M1), DECOD (M2) <- render_topbar_v1.py (mocks/topbar-v1-matrix-security, a05184a)
Tint = v3 rule without the alert-red exception (none of the 4 has red ink): black stays black, whiteness and brightness kept, colour -> theme ink.
Body frames: the v4 24-theme list (render_controlbot_colorui.py ORDER) -> /tmp/cbcolor_build_v5/frames.npy."""
import sys, json, subprocess
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image
HERE = Path(__file__).resolve().parent; OUTD = HERE.parent
sys.path.insert(0, str(HERE))
import render_alertbars_v3 as A           # paths to the securitybot worktree + v2b module set up there
R, V = A.R, A.V
V.W = Path('/tmp/cbbars_v5'); R.W = V.W; V.W.mkdir(parents=True, exist_ok=True)
import render_v4_24themes as V4           # ORDER / NAMES / SRC / inks_all (same 24-theme list)
V4.V.W = V.W
ORDER, NAMES, SRC = V4.ORDER, V4.NAMES, V4.SRC
LOCK_V1 = A.SB / 'mocks/topbar-v1-matrix-security'
# (id shown, source module, source id, resolved still frame = the locked still of each draft)
BANNERS = [('CURTN', 'v2b', 'CURTN', V.STILL_T['CURTN']), ('TERM', 'v2b', 'TERM', V.STILL_T['TERM']),
           ('RAIN', 'v1', 'M1', R.STILL_T['M1']), ('DECOD', 'v1', 'M2', R.STILL_T['M2'])]
LOCKED = {'CURTN': A.LOCKD / '01_CURTN_matrix_curtain@1x.png', 'TERM': A.LOCKD / '02_TERM_amber_terminal@1x.png',
          'RAIN': LOCK_V1 / '01_M1_matrix_rain@1x.png', 'DECOD': LOCK_V1 / '02_M2_matrix_decode@1x.png'}
_cache = {}
def banner(b, t):
    key = (b, t)
    if key not in _cache:
        _, src, sid, _ = next(x for x in BANNERS if x[0] == b)
        _cache[key] = V.header(sid, t)[0] if src == 'v2b' else R.header(sid, t)[0]
    return _cache[key]

def tint(h, T):
    a = h.astype(float); mx = a.max(2); mn = a.min(2); w = np.where(mx > 0, mn / np.maximum(mx, 1), 0)[..., None]
    o = (T * (1 - w) + 255 * w) * (mx[..., None] / 255)
    o = np.where((mx < 8)[..., None], a, o)
    return R.to565(np.clip(np.round(o), 0, 255).astype(np.uint8))

def main():
    body = np.load('/tmp/cbcolor_build_v5/frames.npy'); mask = np.load('/tmp/cbcolor_build_v5/mask.npy'); inks = V4.inks_all()
    hdr = np.zeros((320, 240), bool); hdr[A.HY0:A.HY1, A.HX0:A.HX1] = True
    base9 = np.array(Image.open(R.V9C).convert('RGB'))
    proof = {'clone_check_untinted': [], 'red_ink_px_in_banner_stills': {}, 'stills': [], 'sec_clone_check': []}
    for b, src, sid, st in BANNERS:
        full = R.compose(base9, banner(b, st)); lock = np.array(Image.open(LOCKED[b]).convert('RGB'))
        proof['clone_check_untinted'].append({'banner': b, 'source': f'{src}:{sid}', 'frame': st, 'locked_png': str(LOCKED[b].relative_to(A.SB)),
                                              'full_frame_px_diff': int(np.any(full != lock, 2).sum())})
        h = banner(b, st); proof['red_ink_px_in_banner_stills'][b] = int(A.is_alert_red(h).sum())
    # grid: 24 theme rows x 4 banner columns of smooth @3x header crops at each banner's resolved still frame
    def lab(txt, w): return R.label_strip(txt, w, 75)
    rows = [np.concatenate([R.label_strip('THEME \\ BANNER', 380)] + [np.concatenate([np.full((30, 12, 3), 6, np.uint8), R.label_strip(f'{b}  ({src} {sid}, frame {st})', 720)], 1) for b, src, sid, st in BANNERS], 1)]
    for s, t in enumerate(ORDER):
        cells = [lab(f'{s + 1:02d} {NAMES[s]}', 380)]
        for b, src, sid, st in BANNERS:
            fr = R.compose(body[s], tint(banner(b, st), inks[t]))
            rec = {'slot': s + 1, 'theme': NAMES[s], 'banner': b, 'changed_vs_body_outside_header': int((np.any(fr != body[s], 2) & ~hdr).sum())}
            if t < 11:
                ref = np.array(Image.open(A.SEC_SPEC / f'{t + 1:02d}_{A.THEMES[t][7:]}@1x.png').convert('RGB'))
                rec['changed_vs_locked_sec_v2_outside_header_and_mask'] = int((np.any(fr != ref, 2) & ~hdr & ~mask).sum())
            proof['stills'].append(rec)
            cells += [np.full((75, 12, 3), 6, np.uint8), R.smooth(fr)[:75]]
        rows += [np.full((8, rows[0].shape[1], 3), 24, np.uint8), np.concatenate(cells, 1)]
    Image.fromarray(np.concatenate(rows, 0)).save(OUTD / '00_matrix4_x_24themes_headers@3x.png')
    for s, b in ((0, 'CURTN'), (5, 'RAIN'), (18, 'DECOD'), (15, 'TERM')):
        st = next(x[3] for x in BANNERS if x[0] == b); fr = R.compose(body[s], tint(banner(b, st), inks[ORDER[s]]))
        Image.fromarray(fr.repeat(3, 0).repeat(3, 1)).save(OUTD / f'stills/full_{s + 1:02d}_{NAMES[s].lower().replace(" ", "_")}__{b}@3x.png')
    for b, src, sid, st in BANNERS:
        Image.fromarray(R.compose(base9, banner(b, st)).repeat(3, 0).repeat(3, 1)[:75]).save(OUTD / f'stills/clone_untinted_{b}_header@3x.png')
    # mp4: 24 themes x 32 frames @30 fps; banner = BANNERS[s % 4]; each theme plays the banner's last 29 frames up to its resolved still, so it is fully resolved at the tap
    acc = {t: int(inks[t][11, 119, 0]) << 16 | int(inks[t][11, 119, 1]) << 8 | int(inks[t][11, 119, 2]) for t in ORDER}
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
    raw = V.W / 'v5.rgb'; bad = 0; samples = {}
    with open(raw, 'wb') as fo:
        for s in range(24):
            b, src, sid, st = BANNERS[s % 4]
            for j in range(32):
                ss = s if j < 29 else (s + 1) % 24
                tb = max(0, st - 28 + min(j, 28))
                fr = R.compose(body[ss], tint(banner(b, tb), inks[ORDER[ss]]))
                bad += int((np.any(fr != body[ss], 2) & ~hdr).sum())
                if 24 <= j < 29: fr = tap(fr, acc[ORDER[s]], [0.0, 0.25, 0.55, 0.45, 0.3][j - 24], [0.5, 0.9, 1.0, 0.9, 0.8][j - 24], [6, 10, 14, 18, 22][j - 24])
                elif j >= 29: fr = tap(fr, acc[ORDER[ss]], [0.25, 0.1, 0.0][j - 29], [0.7, 0.4, 0.15][j - 29], 0)
                fo.write(R.smooth(fr).tobytes())
                if j in (8, 23): samples[f'{s:02d}_{j}'] = fr
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', '720x960', '-framerate', '30', '-i', str(raw),
                    '-c:v', 'libx264', '-preset', 'slow', '-crf', '16', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(OUTD / 'tap_cycle_24themes_matrix4@3x.mp4')], check=True)
    np.savez_compressed(V.W / 'samples.npz', **samples)
    bp = json.loads((OUTD / 'diff_proof_body.json').read_text())
    proof['summary'] = {'banners': [f'{b} <- {src}:{sid} (frame {st})' for b, src, sid, st in BANNERS],
        'clone_untinted_match': f"{sum(c['full_frame_px_diff'] == 0 for c in proof['clone_check_untinted'])}/4",
        'body_changed_outside_coilpin_mask_max': max(p['changed_outside_mask'] for p in bp['per_theme']),
        'stills_changed_outside_header_vs_body_max': max(r['changed_vs_body_outside_header'] for r in proof['stills']),
        'mp4_frames_changed_outside_header_total_px_before_tap_overlay': bad,
        'sec_10_vs_locked_v2_outside_header_and_mask_max': max(r.get('changed_vs_locked_sec_v2_outside_header_and_mask', 0) for r in proof['stills']),
        'sec_10_no_coilpin_pipeline_equals_locked_v2': all(p.get('pipeline_equals_locked_v2', True) for p in bp['per_theme']),
        'red_ink_px': proof['red_ink_px_in_banner_stills']}
    (OUTD / 'diff_proof.json').write_text(json.dumps(proof, indent=1)); print(json.dumps(proof['summary'], indent=0))

if __name__ == '__main__': main()
