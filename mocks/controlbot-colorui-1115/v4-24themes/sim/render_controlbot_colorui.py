#!/usr/bin/env python3
"""CONTROLbot Color UI (mock, Mobandy 2026-11-15 9:36 PM ET): SECURITYbot Color UI v2 (locked 2b79d83) verbatim, ONE change:
the coil (square-wave strip at the top of the chip) and the pins (side + bottom pin pads, pin labels) take the selected theme's
colour (SEC_ACC from the locked themes_spec_v2.h). Grey board, face, control box, top bar: untouched v2 pixels.
The v2 renderer module is imported verbatim (copy in this folder, same sha256); only its output paths / zig path are pointed elsewhere."""
import sys, importlib.util, json, subprocess
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image
HERE = Path(__file__).resolve().parent; OUTD = HERE.parent
import scipy.ndimage as ndi
sp = importlib.util.spec_from_file_location('v2', HERE / 'render_security_color_themes_v2.py'); v2 = importlib.util.module_from_spec(sp); sp.loader.exec_module(v2)
v2.ROOT = Path('/workspace/securitybot'); v2.B = Path('/tmp/cbcolor_build_v4'); v2.ZIG = sys.argv[1]
v2.SPEC = v2.ROOT / 'mocks/security-color-themes-v2/themes_spec_v2.h'
v2.OUT = v2.ROOT / 'mocks/security-color-themes-v2'   # -I for themes_spec_v2.h (read only)
LOCK = v2.ROOT / 'mocks/security-color-themes-v2'
v2.TOPBAR = v2.ROOT / 'mocks/topbar-v1-matrix-security'
v2.v9c.V5PNG = v2.ROOT / 'mocks/controlbot-bigchip-v5-reference/01_bigchip@1x.png'

def coil_rect():
    m = np.zeros((320, 240), bool); m[28:64, 30:210] = True; return m

def coil_pin_mask(base):
    """gold coil + gold pin pads / castellations + grey pin labels, from the v2 base (theme-independent pixels)."""
    a = base.astype(int); R, G, Bl = a[..., 0], a[..., 1], a[..., 2]
    gold = (R > G) & (G > Bl) & (R - Bl >= 40) & (R >= 90)
    m = np.zeros((320, 240), bool)
    m[28:62, 30:210] |= gold[28:62, 30:210]                                  # coil strip
    side = np.zeros_like(m); side[25:316, 0:31] = True; side[25:316, 209:240] = True
    bottom = np.zeros_like(m); bottom[292:316, 31:209] = True
    m |= gold & (side | bottom)                                               # pin pads (gold only; red 3V3 / grey GND / purple used pins keep their meaning)
    lab = (abs(R - G) < 20) & (abs(G - Bl) < 16) & (Bl > R) & (R >= 120) & (R < 250)   # grey pin-label ink
    m |= lab & side
    return m

CYD_NAMES = ["GRAPE", "MAGENTA", "TOXIC", "ICE", "BLUE", "ORANGE", "GOLD", "SYNTH", "AURA", "PRISM", "GLTCH", "ROSE", "CIRC", "SONAR", "HOLO", "CAMO"]   # lv_core.h LV_THEME_NAME
CYD_FULL = ["NEON GRAPE", "HOT PINK", "TOXIC", "ICE", "BLUE", "BLAZE ORANGE", "GOLD RUSH", "SYNTHWAVE", "AURORA", "PRISM", "GLITCH", "HOT ROSE", "CIRCUIT", "SONAR", "HOLO FOIL", "DIGI CAMO"]

ORDER = [23, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 25, 13, 14, 24, 16, 17, 18, 19, 20, 21, 22, 26]

def build():
    v2.B.mkdir(parents=True, exist_ok=True)
    subprocess.check_call([v2.ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-w', '-I' + v2.LIVE, '-I' + str(LOCK), '-I' + str(HERE),
                           '-o', str(v2.B / 'reink_v2'), str(HERE / 'controlbot_colorui_reink.cpp')])

def main():
    build()
    names, full, tab = v2.parse_spec(v2.SPEC); n = len(names)
    base, role, samp0, _, _ = v2.base_and_roles(); samp = samp0
    gold_lab = coil_pin_mask(base)
    a = base.astype(int); R, G, Bl = a[..., 0], a[..., 1], a[..., 2]
    gold = (R > G) & (G > Bl) & (R - Bl >= 40) & (R >= 90)
    role8 = gold_lab & gold & (role == 0); role9 = gold_lab & ~gold & (role == 0)
    # v2: drop the bottom-left gold corner ring (stays v2)
    lab8, n8 = ndi.label(role8, structure=np.ones((3, 3)))
    for k in range(1, n8 + 1):
        ys, xs = np.nonzero(lab8 == k)
        if xs.mean() < 20 and ys.mean() > 305: role8[lab8 == k] = False
    role9 &= ~((np.arange(240)[None, :] < 20) & (np.arange(320)[:, None] > 305))
    # v2: one sample point per coil tooth / pad; labels take their pad's sample
    samp = samp.copy(); lab8, n8 = ndi.label(role8 & ~coil_rect(), structure=np.ones((3, 3))); cents = []
    for k in range(1, n8 + 1):
        ys, xs = np.nonzero(lab8 == k); cx, cy = int(round(xs.mean())), int(round(ys.mean())); samp[ys, xs] = (cx, cy); cents.append((cx, cy, xs.size))
    ys, xs = np.nonzero(role8 & coil_rect())
    seg = (xs - 30) * 12 // 180; samp[ys, xs] = np.stack([30 + (seg * 15 + 7), np.full_like(xs, 45)], 1)
    pads = [c for c in cents if c[2] >= 16]
    for y, x in zip(*np.nonzero(role9)):
        side = [c for c in pads if (c[0] < 120) == (x < 120) and c[1] < 292]
        c = min(side, key=lambda c: abs(c[1] - y)); samp[y, x] = (c[0], c[1])
    role_cp = role.copy(); role_cp[role8] = 8; role_cp[role9] = 9
    mask = role8 | role9
    samp_cp = samp
    allnames = [nm for nm in names] + CYD_NAMES; allfull = list(full) + CYD_FULL
    proof = []; frames = []
    for slot, t in enumerate(ORDER, 1):   # v4: our CYD CONTROLbot 24-theme list (LIME->CIRCUIT, HOT PINK->HOLO, CYD BLUE->SONAR, +CAMO)
        im0 = v2.reink(base, role, samp0, t, tag='p')      # same pipeline without the coil/pin roles
        im = v2.reink(base, role_cp, samp_cp, t, tag='c')
        d = np.any(im != im0, axis=2)
        rec = {'theme': allnames[t], 'source': 'SECURITYbot v2' if t < n else ('v3 NEW' if t >= 23 else 'CYD lv_core.h'), 'changed_px_vs_no_coilpin': int(d.sum()), 'changed_outside_mask': int((d & ~mask).sum())}
        if t < n:
            lock = np.array(Image.open(LOCK / f'{t + 1:02d}_{names[t].lower()}@1x.png').convert('RGB'))
            dl = np.any(im != lock, axis=2)
            rec.update({'pipeline_equals_locked_v2': bool((im0 == lock).all()), 'changed_px_vs_locked_v2': int(dl.sum()), 'changed_outside_mask_vs_locked_v2': int((dl & ~mask).sum())})
        proof.append(rec)
        rec['slot'] = slot; rec['src_index'] = t
        frames.append(im)
    # board check: pixels with role 0 outside the coil/pin mask are identical in every theme (the grey board never changes)
    keep = (role_cp == 0); same = all((f[keep] == frames[0][keep]).all() for f in frames)
    lock = np.array(Image.open(OUTD.parent / 'v2' / '01_sec_lime@1x.png').convert('RGB'))
    same_v2 = all((f[~mask] == lock[~mask]).sum() >= 0 and bool((f[keep] == lock[keep]).all()) for f in frames)
    (OUTD / 'diff_proof_body.json').write_text(json.dumps({'mask_px': int(mask.sum()), 'board_role0_outside_mask_identical_to_v2_lime': bool(same_v2),
        'board_identical_across_all_24': bool(same), 'per_theme': proof}, indent=1))
    print(json.dumps(proof), same, same_v2)
    np.save(v2.B / 'frames.npy', np.stack(frames)); np.save(v2.B / 'mask.npy', mask)

if __name__ == '__main__':
    main()
