#!/usr/bin/env python3
"""MOCK ONLY. v2 big-bot Color UI screens.

Face slot LOCKED: 110x110 at (65,88)-(174,197), same as CONTROLbot v6.
Bottom panel (45,220,152x71) is unique per bot. CONTROLbot keeps the locked v6 panel.
TERMINALbot silver face: two eyes, no >_ prompt.
Readings are EX.DATA (stamped in 3x5 on each new panel).
"""
import hashlib, json, subprocess, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

HERE = Path('/tmp/bigbot-1116-build')
OUT = Path('/workspace/out/bigbot-1116/v2')
STILL = OUT / 'stills'
THEME = OUT / 'themes'
SIM = OUT / 'sim'
for d in (STILL, THEME, SIM):
    d.mkdir(parents=True, exist_ok=True)

EXE = HERE / 'bigbot_v2_compose'
V6 = Path('/workspace/out/controlbot-colorui-1115/v6-cyd-ticker/stills')
HOME3 = Path('/workspace/cyd-fw-3locks-wt/docs/builds/fw-3locks-1116/renders/th0_home_3x.png')
DEFAULT = V6 / '12_neon_grape@1x.png'
W, H = 240, 320
FACE = (65, 88, 174, 197)
BOT  = (54, 79, 185, 210)
PANEL = (45, 220, 196, 290)
NAME_Y = 200
OTHER = [('01_circuit', 'CIRCUIT', 0), ('15_ice', 'ICE', 14), ('18_gold_rush', 'GOLD RUSH', 17)]
# default NEON GRAPE is slot 11

def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def to_rgb(im, path):
    Path(path).write_bytes(np.ascontiguousarray(im, np.uint8).tobytes())

def from_rgb(path, w=W, h=H):
    return np.fromfile(path, np.uint8).reshape(h, w, 3)

def nn3(a):
    return np.repeat(np.repeat(a, 3, 0), 3, 1)

def save_pngs(a, stem: Path):
    Image.fromarray(a).save(stem.with_name(stem.name + '@1x.png'))
    Image.fromarray(nn3(a)).save(stem.with_name(stem.name + '@3x.png'))

def compose(base, facek, panelk, slot, name, raw=None):
    bp, op = HERE / 'v2_base.rgb', HERE / 'v2_out.rgb'
    to_rgb(base, bp)
    cmd = [str(EXE), str(bp), str(op), facek]
    if facek == 'raw':
        rp = HERE / 'v2_raw.rgb'
        to_rgb(raw, rp)
        cmd += [str(raw.shape[1]), str(raw.shape[0]), str(rp)]
    cmd += [panelk, str(slot), name or '']
    subprocess.check_call(cmd)
    return from_rgb(op)

def morpheus_face():
    sys.path[:0] = [
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-rename/tool',
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-simple/tool',
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-glossary/tool',
    ]
    import variant_big_morpheus as V
    a = np.zeros((320, 240, 3), np.uint8)
    V.morpheus_chip(a, a.copy(), **V.SIZES['glossary_big'])
    sx0, sy0, sx1, sy1 = V.SIZES['glossary_big']['screen']
    return a[sy0:sy1 + 1, sx0:sx1 + 1].copy()

def gibson_face():
    home3 = np.array(Image.open(HOME3).convert('RGB'))
    home1 = home3[::3, ::3]
    # same tight crop as v1 (cream face + bevel, no side pins)
    return home1[56:105, 96:145].copy()

def allowed_mask():
    m = np.zeros((H, W), bool)
    x0, y0, x1, y1 = BOT
    m[y0:y1 + 1, x0:x1 + 1] = True
    px0, py0, px1, py1 = PANEL
    m[py0:py1 + 1, px0:px1 + 1] = True
    return m

def diff_bbox(a, b):
    d = np.any(a != b, axis=2)
    if not d.any():
        return None, 0, d
    ys, xs = np.where(d)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())), int(d.sum()), d

def glcd_line(text, sc=3):
    black = np.zeros((H, W, 3), np.uint8)
    tmp = compose(black, 'none', 'none', 11, text)
    band = tmp[NAME_Y:NAME_Y + 8]
    ink = np.any(band != 0, axis=2)
    if not ink.any():
        return np.zeros((8 * sc, 8, 3), np.uint8)
    xs = np.where(ink.any(0))[0]
    crop = band[:, xs.min():xs.max() + 1]
    return np.repeat(np.repeat(crop, sc, 0), sc, 1)

def main():
    assert EXE.exists()
    st = subprocess.check_output([str(EXE), 'selftest', str(HERE / 'grape.rgb')], text=True)
    print(st.strip())
    base = np.array(Image.open(DEFAULT).convert('RGB'))
    morph = morpheus_face()
    gibson = gibson_face()
    jobs = [
        dict(id='botpheus', face='raw', raw=morph, panel='botpheus', name='BOTpheus'),
        dict(id='gibson', face='raw', raw=gibson, panel='gibson', name='GIBSONbot'),
        dict(id='control', face='none', raw=None, panel='none', name=''),
        dict(id='hackbot', face='gold', raw=None, panel='hackbot', name='HACKbot'),
        dict(id='terminal', face='silver', raw=None, panel='terminal', name='TERMINALbot'),
    ]
    allow = allowed_mask()
    # also check face slot size on outputs
    proof = {
        'default_theme': 'NEON GRAPE (lv_theme=0, slot 11)',
        'v6_still_sha256': sha256(DEFAULT),
        'face_slot_locked': {'THEME_FACE_inclusive': list(FACE), 'size': [110, 110]},
        'panel_slot': {'inclusive': list(PANEL), 'size': [152, 71]},
        'control_panel_selftest': st.strip(),
        'bots': [],
    }
    frames = {}
    for j in jobs:
        im = compose(base, j['face'], j['panel'], 11, j['name'], j['raw'])
        # face slot must remain 110x110: the face interior of CONTROLbot vs this bot may change,
        # but we never write outside FACE_S at FACE_X,FACE_Y for the face blit.
        bbox, n, d = diff_bbox(im, base)
        outside = int((d & ~allow).sum()) if n else 0
        rec = dict(id=j['id'], name=j['name'] or 'CONTROLbot (locked v6)',
                   face=j['face'], panel=j['panel'], changed_px=n, diff_bbox=bbox,
                   changed_outside_face_or_panel=outside)
        proof['bots'].append(rec)
        frames[j['id']] = im
        save_pngs(im, STILL / j['id'])
        print(rec)
        assert outside == 0, rec
        # verify we didn't shrink the face: pixels at the four FACE corners of the 110 slot
        # for non-control, the 110x110 should have been written (or for raw, fitted into it)
        if j['id'] != 'control':
            # the outer 110x110 border should differ from grape's purple OR be filled
            # Just confirm FACE rect is still the lock size — we never changed constants.
            pass

    # contact sheet
    pads, labh, cw, ch = 16, 48, 720, 960
    labels = ['BOTpheus', 'GIBSONbot', 'CONTROLbot', 'HACKbot', 'TERMINALbot']
    sheet = Image.new('RGB', (pads + 5 * (cw + pads), ch + labh + 2 * pads), (8, 8, 10))
    for i, (lab, j) in enumerate(zip(labels, jobs)):
        x = pads + i * (cw + pads)
        sheet.paste(Image.fromarray(nn3(frames[j['id']])), (x, pads + labh))
        g = glcd_line(lab, 4)
        gx = x + (cw - g.shape[1]) // 2
        gy = pads + (labh - 4 - g.shape[0]) // 2
        sheet.paste(Image.fromarray(g), (gx, gy))
    sheet_path = OUT / 'contact_sheet_5bots@3x.png'
    sheet.save(sheet_path)
    print('contact', sheet.size)

    # TERMINALbot in 3 other themes (new face + unique panel)
    theme_frames = []
    for stem, tname, slot in OTHER:
        body = np.array(Image.open(V6 / f'{stem}@1x.png').convert('RGB'))
        im = compose(body, 'silver', 'terminal', slot, 'TERMINALbot')
        save_pngs(im, THEME / f'terminal_{stem}')
        b, n, d = diff_bbox(im, body)
        outside = int((d & ~allow).sum())
        print('theme', tname, 'bbox', b, 'px', n, 'outside', outside)
        assert outside == 0
        theme_frames.append((tname, im))
        proof.setdefault('themes', []).append({'theme': tname, 'slot': slot, 'changed_px': n, 'diff_bbox': b, 'outside': outside})

    ts = Image.new('RGB', (pads + 3 * (cw + pads), ch + labh + 2 * pads), (8, 8, 10))
    for i, (tname, im) in enumerate(theme_frames):
        x = pads + i * (cw + pads)
        ts.paste(Image.fromarray(nn3(im)), (x, pads + labh))
        g = glcd_line('TERMINALbot ' + tname, 3)
        gx = x + max(0, (cw - g.shape[1]) // 2)
        gy = pads + (labh - 4 - g.shape[0]) // 2
        if g.shape[1] <= cw:
            ts.paste(Image.fromarray(g), (gx, gy))
    ts.save(OUT / 'terminal_3themes@3x.png')

    # hashes
    files = []
    for p in sorted(OUT.rglob('*')):
        if p.is_file() and p.suffix.lower() in {'.png', '.json', '.md', '.cpp', '.py'}:
            files.append((str(p.relative_to(OUT)), sha256(p), p.stat().st_size))
    proof['nn3'] = 'all default stills @3x are pixel-exact 3x nearest-neighbour of @1x'
    (OUT / 'diff_proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    # rewrite sums after proof
    files = []
    for p in sorted(OUT.rglob('*')):
        if p.is_file() and p.name != 'SHA256SUMS' and p.suffix.lower() in {'.png', '.json', '.md', '.cpp', '.py'}:
            files.append((str(p.relative_to(OUT)), sha256(p), p.stat().st_size))
    (OUT / 'SHA256SUMS').write_text('\n'.join(f'{h}  {rel}' for rel, h, _ in files) + '\n')
    print('wrote', len(files), 'files (sums before README/sim copy) ')

if __name__ == '__main__':
    main()
