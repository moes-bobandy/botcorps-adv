#!/usr/bin/env python3
"""MOCK ONLY. v3.1: fill each bot face to CONTROLbot's 110x110 THEME_FACE
and remove on-screen name labels so they match CONTROLbot (no label).
Start from v3 stills so bottom panels and everything outside the face+name
pad stay byte-identical. CONTROLbot is copied unchanged.
"""
import hashlib, json, subprocess, sys
from pathlib import Path
import numpy as np
from PIL import Image

HERE = Path('/tmp/bigbot-1116-build')
V3 = Path('/workspace/out/bigbot-1116/v3')
OUT = Path('/workspace/out/bigbot-1116/v3.1')
STILL = OUT / 'stills'
THEME = OUT / 'themes'
SIM = OUT / 'sim'
for d in (STILL, THEME, SIM):
    d.mkdir(parents=True, exist_ok=True)

V6 = Path('/workspace/out/controlbot-colorui-1115/v6-cyd-ticker/stills')
HOME3 = Path('/workspace/cyd-fw-3locks-wt/docs/builds/fw-3locks-1116/renders/th0_home_3x.png')
EXE = HERE / 'bigbot_v3_compose'  # only used to rasterize contact-sheet captions
W, H = 240, 320
FACE = (65, 88, 174, 197)  # inclusive THEME_FACE, 110x110
BOT  = (54, 79, 185, 210)
PAD  = (54, 198, 185, 210)  # black frame pad under the face, inclusive
PANEL = (45, 220, 196, 290)
NAME_Y = 200
BOTS = ['botpheus', 'gibson', 'control', 'hackbot', 'terminal']
LABELS = ['BOTpheus', 'GIBSONbot', 'CONTROLbot', 'HACKbot', 'TERMINALbot']
OTHER = [('01_circuit', 'CIRCUIT', 0), ('15_ice', 'ICE', 14), ('18_gold_rush', 'GOLD RUSH', 17)]

def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def nn3(a):
    return np.repeat(np.repeat(a, 3, 0), 3, 1)

def save_pngs(a, stem: Path):
    Image.fromarray(a).save(stem.with_name(stem.name + '@1x.png'))
    Image.fromarray(nn3(a)).save(stem.with_name(stem.name + '@3x.png'))

def nn_fill(src, ds):
    """Nearest-neighbour scale that fills ds x ds exactly (no letterbox)."""
    sh, sw = src.shape[:2]
    yy = (np.arange(ds) * sh // ds)
    xx = (np.arange(ds) * sw // ds)
    return src[yy][:, xx].copy()

def morpheus_content():
    sys.path[:0] = [
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-rename/tool',
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-simple/tool',
        '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-glossary/tool',
    ]
    import variant_big_morpheus as V
    a = np.zeros((320, 240, 3), np.uint8)
    V.morpheus_chip(a, a.copy(), **V.SIZES['glossary_big'])
    sx0, sy0, sx1, sy1 = V.SIZES['glossary_big']['screen']
    src = a[sy0:sy1 + 1, sx0:sx1 + 1]
    ink = np.any(src != 0, axis=2)
    ys, xs = np.where(ink)
    return src[ys.min():ys.max() + 1, xs.min():xs.max() + 1].copy()

def gibson_content():
    home3 = np.array(Image.open(HOME3).convert('RGB'))
    home1 = home3[::3, ::3]
    # same tight cream+bevel crop as v1–v3, 49x49
    return home1[56:105, 96:145].copy()

def put_face(im, face110):
    x0, y0, x1, y1 = FACE
    assert face110.shape == (110, 110, 3)
    im = im.copy()
    im[y0:y1 + 1, x0:x1 + 1] = face110
    return im

def clear_name(im, reference):
    """Restore the under-face pad from the reference (CONTROLbot / locked body)."""
    im = im.copy()
    x0, y0, x1, y1 = PAD
    im[y0:y1 + 1, x0:x1 + 1] = reference[y0:y1 + 1, x0:x1 + 1]
    return im

def face_bounds(im):
    """Bounds of non-zero (and, for cream/gold/silver, any face-edge) tiles
    inside THEME_FACE. Also report whether every edge of the 110 slot is
    occupied by face art (no letterbox margin)."""
    x0, y0, x1, y1 = FACE
    slot = im[y0:y1 + 1, x0:x1 + 1]
    # letterbox margin is pure black touching the slot edge. Eyes are pure
    # black but interior. A filled face has no pure-black run along an edge
    # covering the whole side, and the face art reaches all four edges.
    edge = {
        'N': slot[0], 'S': slot[-1], 'W': slot[:, 0], 'E': slot[:, -1],
    }
    edge_black = {k: int(np.all(v == 0, axis=1).sum()) for k, v in edge.items()}
    # bounds of pixels that are not a letterbox: treat a pixel as content if
    # it's non-black OR it's interior black (eyes). Equivalent: the 110 slot
    # itself once letterbox is gone. We also compute bbox of non-black, then
    # union with interior blacks only if edges are filled.
    filled_edges = all(edge_black[k] < 110 for k in edge_black) and all(
        not np.all(edge[k] == 0) for k in edge
    )
    # tighter: each edge has at least one non-black pixel at both ends? Control
    # has non-black on the entire edge except... control edges are all non-black.
    # After fill, BOTpheus/Gibson edges should all be non-black (no eyes on edge).
    # Hack/Terminal/Control: all edge pixels non-black (eyes are inset).
    return {
        'THEME_FACE_inclusive': list(FACE),
        'slot_size': [110, 110],
        'edge_black_px': edge_black,
        'edge_all_face_art': all(edge_black[k] == 0 for k in edge_black),
        'fills_slot': bool(np.array_equal(slot.shape[:2], (110, 110))) and all(edge_black[k] == 0 for k in edge_black),
    }

def diff_bbox(a, b):
    d = np.any(a != b, axis=2)
    if not d.any():
        return None, 0, d
    ys, xs = np.where(d)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())), int(d.sum()), d

def allowed_mask():
    m = np.zeros((H, W), bool)
    x0, y0, x1, y1 = FACE
    m[y0:y1 + 1, x0:x1 + 1] = True
    px0, py0, px1, py1 = PAD
    m[py0:py1 + 1, px0:px1 + 1] = True
    return m

def glcd_line(text, sc=3):
    # rasterize a caption via the v3 composer name path on a black frame
    if not EXE.exists():
        return np.zeros((8 * sc, 8, 3), np.uint8)
    black = np.zeros((H, W, 3), np.uint8)
    bp, op = HERE / 'v31_cap_in.rgb', HERE / 'v31_cap_out.rgb'
    Path(bp).write_bytes(np.ascontiguousarray(black, np.uint8).tobytes())
    subprocess.check_call([str(EXE), str(bp), str(op), 'none', 'none', '11', text])
    tmp = np.fromfile(op, np.uint8).reshape(H, W, 3)
    band = tmp[NAME_Y:NAME_Y + 8]
    ink = np.any(band != 0, axis=2)
    if not ink.any():
        return np.zeros((8 * sc, 8, 3), np.uint8)
    xs = np.where(ink.any(0))[0]
    crop = band[:, xs.min():xs.max() + 1]
    return np.repeat(np.repeat(crop, sc, 0), sc, 1)

def main():
    v3 = {b: np.array(Image.open(V3 / 'stills' / f'{b}@1x.png').convert('RGB')) for b in BOTS}
    control = v3['control']
    morph = morpheus_content()
    gibson = gibson_content()
    print('morph content', morph.shape, 'gibson content', gibson.shape)
    morph110 = nn_fill(morph, 110)
    gibson110 = nn_fill(gibson, 110)
    print('filled', morph110.shape, gibson110.shape)

    allow = allowed_mask()
    proof = {
        'note': 'v3.1 starts from v3 stills. Only THEME_FACE and the under-face name pad change. CONTROLbot is byte-identical.',
        'control_face_measured': face_bounds(control),
        'bots': [],
        'vs_v3': [],
    }
    frames = {}
    for b in BOTS:
        src = v3[b]
        if b == 'control':
            im = src.copy()
        else:
            im = clear_name(src, control)
            if b == 'botpheus':
                im = put_face(im, morph110)
            elif b == 'gibson':
                im = put_face(im, gibson110)
            # hackbot / terminal: face already fills 110x110; only the name comes off
        assert im.shape == (H, W, 3)
        fb = face_bounds(im)
        bb, n, d = diff_bbox(im, src)
        outside = int((d & ~allow).sum()) if n else 0
        # also vs control, for the pad identity
        rec = {
            'id': b,
            'face_bounds': fb,
            'matches_control_slot': fb['fills_slot'] and fb['THEME_FACE_inclusive'] == list(FACE),
            'vs_v3_changed_px': n,
            'vs_v3_diff_bbox': bb,
            'vs_v3_outside_face_or_name_pad': outside,
        }
        if b == 'control':
            rec['identical_to_v3'] = bool(np.array_equal(im, src))
            rec['identical_to_locked_grape'] = sha256(V3 / 'stills' / 'control@1x.png') == '48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779'
        proof['bots'].append(rec)
        frames[b] = im
        save_pngs(im, STILL / b)
        print(rec)
        assert outside == 0, rec
        assert fb['fills_slot'], rec
        if b == 'control':
            assert np.array_equal(im, src)

    # contact sheet
    pads, labh, cw, ch = 16, 48, 720, 960
    sheet = Image.new('RGB', (pads + 5 * (cw + pads), ch + labh + 2 * pads), (8, 8, 10))
    for i, (lab, b) in enumerate(zip(LABELS, BOTS)):
        x = pads + i * (cw + pads)
        sheet.paste(Image.fromarray(nn3(frames[b])), (x, pads + labh))
        g = glcd_line(lab, 4)
        gx = x + (cw - g.shape[1]) // 2
        gy = pads + (labh - 4 - g.shape[0]) // 2
        sheet.paste(Image.fromarray(g), (gx, gy))
    sheet.save(OUT / 'contact_sheet_5bots@3x.png')
    print('contact', sheet.size)

    # TERMINALbot 3 themes: same face (already 110), names off, pad from the locked theme body
    theme_frames = []
    for stem, tname, slot in OTHER:
        v3t = np.array(Image.open(V3 / 'themes' / f'terminal_{stem}@1x.png').convert('RGB'))
        body = np.array(Image.open(V6 / f'{stem}@1x.png').convert('RGB'))
        im = clear_name(v3t, body)
        save_pngs(im, THEME / f'terminal_{stem}')
        bb, n, d = diff_bbox(im, v3t)
        outside = int((d & ~allow).sum()) if n else 0
        fb = face_bounds(im)
        print('theme', tname, 'vs v3', n, bb, 'outside', outside, 'fills', fb['fills_slot'])
        assert outside == 0 and fb['fills_slot']
        theme_frames.append((tname, im))
        proof.setdefault('themes', []).append({
            'theme': tname, 'slot': slot, 'vs_v3_changed_px': n, 'vs_v3_diff_bbox': bb,
            'outside': outside, 'fills_slot': fb['fills_slot'],
        })

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

    (OUT / 'diff_proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    print('proof written')

if __name__ == '__main__':
    main()
