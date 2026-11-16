#!/usr/bin/env python3
"""CONTROLbot Color UI v6 — our CYD ticker banner, tinted to the selected theme (mock only).

Mobandy 2026-11-16 12:38 PM ET: use our ticker banner (lv_scene_ticker in lv_core.h @049ff59),
with stripes and text following the selected theme and the alert flash staying red.
Not SECURITYbot's banners. Ticker text is the firmware default lv_tk_msg.
"""
import hashlib, json, subprocess, sys
from pathlib import Path
import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
OUTD = HERE.parent
STILL = OUTD / 'stills'
STILL.mkdir(parents=True, exist_ok=True)
W = Path('/tmp/cbticker_v6'); W.mkdir(parents=True, exist_ok=True)
ZIG = '/home/box/.local/lib/python3.13/site-packages/ziglang/zig'
FW = Path('/workspace/cyd-portrait-wt')
BODY = Path('/tmp/cbcolor_build_v4/frames.npy')
MASKP = Path('/tmp/cbcolor_build_v4/mask.npy')
SYS_V4 = Path('/workspace/out/controlbot-colorui-1115/v4-24themes/sim')
sys.path.insert(0, str(SYS_V4))
import render_security_color_themes_v2 as LAB

HY0, HY1, HX0, HX1 = 1, 24, 1, 239
ORDER = [23, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 25, 13, 14, 24, 16, 17, 18, 19, 20, 21, 22, 26]
NAMES = ['CIRCUIT', 'HI-VIS BLUE', 'NV GREEN BLOOM', 'NV WHITE PHOSPHOR', 'IR IRONBOW', 'IR RAINBOW HEAT',
         'IR WHITE-HOT', 'IR RED-HOT', 'ALARM STROBE', 'POLICE SIREN', 'PERIMETER GRID', 'NEON GRAPE',
         'HOLO FOIL', 'TOXIC', 'ICE', 'SONAR', 'BLAZE ORANGE', 'GOLD RUSH', 'SYNTHWAVE', 'AURORA',
         'PRISM', 'GLITCH', 'HOT ROSE', 'DIGI CAMO']
SRC = ['NEW (was SEC LIME slot)'] + ['SECURITYbot v2'] * 10 + ['CYD', 'NEW (was HOT PINK slot)', 'CYD', 'CYD',
       'NEW (was CYD BLUE slot)'] + ['CYD'] * 7 + ['NEW extra']
MSG = 'NO THREATS - TAP SCAN'          # lv_core.h @049ff59 line 234 default
MSG_LINE = 234
STILL_NOW, STILL_OFF = 2000, 220       # tx = 239-220 = 19; text readable in the bar
TICK_W = 238
MSG_W = len(MSG) * 12                  # b_text size 2 advances 12 px
OFF_WRAP = TICK_W + MSG_W              # firmware: off > (x1-x0+1)+w then lv_tk_next -> off=0
ALERT_SLOT = 18                        # SYNTHWAVE: red flash reads against a non-red theme
FEATURED = [0, 1, 12, 18, 23]          # CIRCUIT, HI-VIS, HOLO, SYNTHWAVE, DIGI CAMO

def sha8(p: Path) -> str:
    h = hashlib.sha256(p.read_bytes()).hexdigest()
    return h[:8]

def nn3(a: np.ndarray) -> np.ndarray:
    return np.repeat(np.repeat(a, 3, 0), 3, 1)

def compose(body, hdr):
    out = body.copy()
    out[HY0:HY1, HX0:HX1] = hdr
    return out

def build():
    exe = W / 'cyd_ticker'
    subprocess.check_call([
        ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-w',
        '-I/workspace/cyd-portrait-wt/src/live',
        '-I/workspace/securitybot/mocks/security-color-themes-v2',
        '-I' + str(HERE), '-o', str(exe), str(HERE / 'cyd_ticker.cpp')])
    return exe

def inc_matches_firmware():
    inc = (HERE / 'lv_scene_ticker_049ff59.inc').read_bytes()
    src = subprocess.check_output(['git', '-C', str(FW), 'show', '049ff59:src/live/lv_core.h'])
    # lines 880-901 of the header (1-indexed)
    chunk = b'\n'.join(src.splitlines()[879:901]) + b'\n'
    return inc == chunk, len(inc), len(chunk)

def load_hdr(path):
    return np.fromfile(path, np.uint8).reshape(23, 238, 3)

def main():
    exe = build()
    st = subprocess.check_output([str(exe), 'selftest'], text=True).strip()
    print(st)
    selftest_ok = 'fail=0' in st

    body = np.load(BODY)
    mask = np.load(MASKP)
    if mask.ndim == 3:
        mask = mask[..., 0] > 0
    hdr_m = np.zeros((320, 240), bool)
    hdr_m[HY0:HY1, HX0:HX1] = True

    inc_ok, inc_n, fw_n = inc_matches_firmware()

    # --- untinted clone check: verbatim fw function vs tint path with firmware palette ---
    cases = []
    for tag, now, off, alert in (
        ('still_normal', STILL_NOW, STILL_OFF, 0),
        ('t0_off0', 0, 0, 0),
        ('scroll', 1600, 100, 0),
        ('alert_on', 350, STILL_OFF, 1),    # now/350 = 1, flash on
        ('alert_off', 700, STILL_OFF, 1),   # now/350 = 2, flash off
    ):
        p1, p2 = W / f'fw_{tag}.rgb', W / f'tintfw_{tag}.rgb'
        subprocess.check_call([str(exe), 'one', 'fw', '-1', str(now), str(off), str(alert), str(p1), MSG])
        subprocess.check_call([str(exe), 'one', 'tintfw', '-1', str(now), str(off), str(alert), str(p2), MSG])
        a, b = load_hdr(p1), load_hdr(p2)
        d = int(np.any(a != b, 2).sum())
        cases.append({'tag': tag, 'now': now, 'off': off, 'alert': alert, 'px_diff': d})
        if tag == 'still_normal':
            fw_still, tintfw_still = a, b
        if tag == 'alert_on':
            fw_alert = a

    # compose firmware ticker onto ICE body for a full-frame clone proof image
    ice = body[14]
    clone_fw = compose(ice, fw_still)
    clone_tintfw = compose(ice, tintfw_still)
    Image.fromarray(clone_fw).save(OUTD / '00_clone_fw_untinted@1x.png')
    Image.fromarray(clone_tintfw).save(OUTD / '00_clone_tintpath_firmware_palette@1x.png')
    Image.fromarray(nn3(clone_fw)).save(OUTD / '00_clone_fw_untinted@3x.png')
    diff = np.zeros_like(clone_fw)
    dmap = np.any(clone_fw != clone_tintfw, 2)
    diff[dmap] = (255, 0, 0)
    Image.fromarray(nn3(diff)).save(OUTD / '00_clone_diff@3x.png')
    Image.fromarray(nn3(fw_still)).save(OUTD / '00_clone_fw_header@3x.png')
    Image.fromarray(nn3(fw_alert)).save(OUTD / '00_clone_fw_alert_header@3x.png')

    # --- 24 theme stills (normal) + alert still ---
    plan = W / 'stills.plan'
    with plan.open('w') as f:
        for t in ORDER:
            f.write(f'tint {t} {STILL_NOW} {STILL_OFF} 0\n')
        f.write(f'tint {ORDER[ALERT_SLOT]} {STILL_NOW} {STILL_OFF} 1\n')  # flash: 2000/350=5 odd -> on
    batch = W / 'stills.rgb'
    subprocess.check_call([str(exe), 'batch', str(plan), str(batch)])
    raw = np.fromfile(batch, np.uint8).reshape(25, 23, 238, 3)
    finals = []
    proof_final = []
    for s, t in enumerate(ORDER):
        fr = compose(body[s], raw[s])
        finals.append(fr)
        rec = {
            'slot': s + 1, 'theme': NAMES[s],
            'changed_vs_body_outside_header': int((np.any(fr != body[s], 2) & ~hdr_m).sum()),
            'changed_vs_body_outside_header_and_mask': int((np.any(fr != body[s], 2) & ~hdr_m & ~mask).sum()),
        }
        proof_final.append(rec)
        Image.fromarray(fr).save(STILL / f'{s + 1:02d}_{NAMES[s].lower().replace(" ", "_")}@1x.png')
    alert_fr = compose(body[ALERT_SLOT], raw[24])
    Image.fromarray(alert_fr).save(STILL / f'alert_{NAMES[ALERT_SLOT].lower().replace(" ", "_")}@1x.png')
    Image.fromarray(nn3(alert_fr)).save(STILL / f'alert_{NAMES[ALERT_SLOT].lower().replace(" ", "_")}@3x.png')
    for s in FEATURED:
        Image.fromarray(nn3(finals[s])).save(STILL / f'{s + 1:02d}_{NAMES[s].lower().replace(" ", "_")}@3x.png')

    # --- 24-theme header grid @3x (normal) ---
    # 6 x 4 tiles: labelled header crops, nearest-neighbor x3
    hw, hh, lh, pad, cols = 238 * 3, 23 * 3, 56, 20, 6
    grid = Image.new('RGB', (cols * hw + (cols + 1) * pad, 4 * (hh + lh) + 5 * pad), (6, 6, 8))
    for s in range(24):
        cx = pad + (s % cols) * (hw + pad)
        cy = pad + (s // cols) * (hh + lh + pad)
        grid.paste(Image.fromarray(nn3(raw[s])), (cx, cy + lh))
        new = 'NEW' in SRC[s]
        LAB.big_text(grid, cx + 2, cy + 4, f'{s + 1:02d} {NAMES[s]}', (120, 255, 200) if new else (255, 255, 255), 3)
        LAB.big_text(grid, cx + 2, cy + 30, SRC[s][:28], (160, 170, 180), 2)
    grid.save(OUTD / '00_24themes_headers@3x.png')

    # --- mp4: 32 frames / theme @ 30 fps, ticker scrolls continuously, one alert flash on SYNTHWAVE ---
    # firmware tick: now += 30, off += (lv_speed=1)+1 = 2, wrap like lv_ticker_tick / lv_tk_next
    nfr = 32
    plan_v = W / 'video.plan'
    frames_meta = []
    now, off = 0, 0
    with plan_v.open('w') as f:
        for s in range(24):
            for j in range(nfr):
                # mid-slot SYNTHWAVE: alert for frames 8..31 so a ~0.8 s flash is obvious
                alert = 1 if (s == ALERT_SLOT and j >= 8) else 0
                f.write(f'tint {ORDER[s]} {now} {off} {alert}\n')
                frames_meta.append((s, j, now, off, alert))
                now += 30
                off += 2
                if off > OFF_WRAP:
                    off = 0
    vraw = W / 'video_hdr.rgb'
    subprocess.check_call([str(exe), 'batch', str(plan_v), str(vraw)])
    vhs = np.fromfile(vraw, np.uint8).reshape(24 * nfr, 23, 238, 3)

    # tap glow (video overlay only, same recipe as v4) — face box from ICE cyan chip
    ice = body[14]
    fb = (np.abs(ice.astype(int) - np.array([0, 255, 255])).sum(2) < 60)
    ys, xs = np.nonzero(fb[60:230, 40:200])
    fy0, fy1, fx0, fx1 = ys.min() + 60, ys.max() + 60, xs.min() + 40, xs.max() + 40
    face = np.zeros((320, 240), bool); face[fy0:fy1 + 1, fx0:fx1 + 1] = True
    eyes = np.all(ice < 24, 2) & face
    face &= ~eyes
    yy, xx = np.mgrid[0:320, 0:240]
    dist = np.maximum(np.maximum(fx0 - xx, xx - fx1), np.maximum(fy0 - yy, yy - fy1))
    ring = (dist > 0) & (dist <= 4) & ~hdr_m
    cx, cy = (fx0 + fx1) // 2, (fy0 + fy1) // 2

    def tap(f, c, af, ag, r):
        g = f.astype(np.float32)
        col = np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], np.float32)
        g[face] = g[face] * (1 - af) + (col * 0.6 + 255 * 0.4) * af
        w = (ag * (1 - (dist - 1) / 4.0))[..., None] * ring[..., None]
        g = g * (1 - w) + col * w
        if r:
            d = np.hypot(xx - cx, yy - cy)
            m = (d >= r - 1.5) & (d <= r + 0.5) & ~eyes
            g[m] = g[m] * 0.3 + 255 * 0.7
        return np.clip(g, 0, 255).astype(np.uint8)

    # tap colour = theme HDR-ish mid-header pixel of the still (bright ink)
    acc = {}
    for s, t in enumerate(ORDER):
        px = raw[s][11, 119]
        acc[t] = int(px[0]) << 16 | int(px[1]) << 8 | int(px[2])

    raw_vid = W / 'v6.rgb'
    bad = 0
    k = 0
    samples = {}
    with open(raw_vid, 'wb') as fo:
        for s in range(24):
            for j in range(nfr):
                ss = s if j < 29 else (s + 1) % 24
                # after frame 28, snap to next theme body (tap transition) but keep this frame's header from plan
                # Plan was written with theme ORDER[s] for all 32 frames. For j>=29 use next theme's header+body.
                if j < 29:
                    fr = compose(body[s], vhs[k])
                    pre = fr
                else:
                    # header for next theme at this now/off: stored? plan used ORDER[s] even for j>=29.
                    # Re-render consistency: use the next slot's still-time? Better: the batch plan already
                    # used ORDER[s]. For transition frames, compose next body with a header of next theme
                    # at the same now/off/alert=0. We'll take it from a side table built below if needed.
                    fr = compose(body[ss], vhs[k])  # temporary; may be wrong theme header
                    pre = fr
                # count pre-fx changes... for j>=29 this isn't the right header yet. Fix below.
                k += 1
                samples[f'tmp'] = fr  # placeholder, rewritten after fix
    # The transition issue: I'll rebuild video properly in a second pass that requests
    # next-theme headers for j>=29. Cleaner to emit those headers in the plan from the start.
    # Rewrite plan with ss theme for j>=29 (and no alert on the incoming theme).
    now, off = 0, 0
    with plan_v.open('w') as f:
        frames_meta = []
        for s in range(24):
            for j in range(nfr):
                ss = s if j < 29 else (s + 1) % 24
                alert = 1 if (s == ALERT_SLOT and j >= 8 and j < 29) else 0
                f.write(f'tint {ORDER[ss]} {now} {off} {alert}\n')
                frames_meta.append((ss, j, now, off, alert))
                now += 30
                off += 2
                if off > OFF_WRAP:
                    off = 0
    subprocess.check_call([str(exe), 'batch', str(plan_v), str(vraw)])
    vhs = np.fromfile(vraw, np.uint8).reshape(24 * nfr, 23, 238, 3)

    bad = 0
    k = 0
    samples = {}
    with open(raw_vid, 'wb') as fo:
        for s in range(24):
            for j in range(nfr):
                ss = s if j < 29 else (s + 1) % 24
                fr = compose(body[ss], vhs[k])
                bad += int((np.any(fr != body[ss], 2) & ~hdr_m).sum())
                if 24 <= j < 29:
                    fr = tap(fr, acc[ORDER[s]], [0.0, 0.25, 0.55, 0.45, 0.3][j - 24],
                             [0.5, 0.9, 1.0, 0.9, 0.8][j - 24], [6, 10, 14, 18, 22][j - 24])
                elif j >= 29:
                    fr = tap(fr, acc[ORDER[ss]], [0.25, 0.1, 0.0][j - 29],
                             [0.7, 0.4, 0.15][j - 29], 0)
                fo.write(fr.tobytes())
                if j in (10, 12, 20, 26):
                    samples[f'{s:02d}_{j}'] = fr
                k += 1
    mp4 = OUTD / 'tap_cycle_24themes_ticker@3x.mp4'
    subprocess.run([
        'ffmpeg', '-v', 'error', '-y',
        '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '240x320', '-framerate', '30',
        '-i', str(raw_vid),
        '-vf', 'scale=720:960:flags=neighbor',
        '-c:v', 'libx264', '-preset', 'slow', '-crf', '17', '-pix_fmt', 'yuv420p',
        '-movflags', '+faststart', str(mp4)
    ], check=True)
    np.savez_compressed(W / 'samples.npz', **{k: v for k, v in samples.items()})

    proof = {
        'ticker_text': {'msg': MSG, 'source': 'lv_core.h @049ff59 line 234, default lv_tk_msg', 'line': MSG_LINE},
        'inc_verbatim_vs_firmware_header_880_901': {'match': inc_ok, 'inc_bytes': inc_n, 'fw_bytes': fw_n},
        'selftest_tintpath_firmware_palette_vs_verbatim_function': {'log': st, 'ok': selftest_ok},
        'clone_check_cases': cases,
        'clone_check_all_zero': all(c['px_diff'] == 0 for c in cases),
        'clone_fullframe_ice_px_diff': int(np.any(clone_fw != clone_tintfw, 2).sum()),
        'final_stills': proof_final,
        'alert_still': {
            'theme': NAMES[ALERT_SLOT], 'now': STILL_NOW, 'off': STILL_OFF,
            'flash': (STILL_NOW // 350) & 1,
            'changed_vs_body_outside_header': int((np.any(alert_fr != body[ALERT_SLOT], 2) & ~hdr_m).sum()),
        },
        'summary': {
            'theme_order': [f'{s + 1:02d} {NAMES[s]} [{SRC[s]}]' for s in range(24)],
            'inc_verbatim': inc_ok,
            'selftest_ok': selftest_ok,
            'clone_check_all_zero': all(c['px_diff'] == 0 for c in cases),
            'stills_changed_outside_header_max': max(r['changed_vs_body_outside_header'] for r in proof_final),
            'mp4_prefx_frames_changed_outside_header_total_px': bad,
            'mp4_frames': 24 * nfr, 'mp4_fps': 30, 'mp4_scale': '720x960 nearest-neighbor',
            'alert_in_video': f'{NAMES[ALERT_SLOT]} frames 8..28 of its 32 (now steps of 30 ms, flash period 350 ms)',
            'still_now_off': [STILL_NOW, STILL_OFF],
            'ticker_text': MSG,
        }
    }
    (OUTD / 'diff_proof.json').write_text(json.dumps(proof, indent=1))
    # SHA256 of deliverables
    deliver = sorted([p for p in OUTD.rglob('*') if p.is_file() and p.name not in ('SHA256SUMS',) and 'sim' not in p.parts or p.suffix in ('.png', '.mp4', '.json', '.md') and p.parent == OUTD or p.parent == STILL])
    # cleaner list
    names = [
        '00_clone_fw_untinted@1x.png', '00_clone_fw_untinted@3x.png',
        '00_clone_tintpath_firmware_palette@1x.png', '00_clone_diff@3x.png',
        '00_clone_fw_header@3x.png', '00_clone_fw_alert_header@3x.png',
        '00_24themes_headers@3x.png', 'tap_cycle_24themes_ticker@3x.mp4',
        'diff_proof.json',
    ]
    lines = []
    for n in names:
        p = OUTD / n
        if p.exists():
            h = hashlib.sha256(p.read_bytes()).hexdigest()
            lines.append(f'{h}  {n}')
    for p in sorted(STILL.glob('*.png')):
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        lines.append(f'{h}  stills/{p.name}')
    (OUTD / 'SHA256SUMS').write_text('\n'.join(lines) + '\n')
    print(json.dumps(proof['summary'], indent=0))
    print('SHA8', {n: sha8(OUTD / n) for n in names if (OUTD / n).exists()})

if __name__ == '__main__':
    main()
