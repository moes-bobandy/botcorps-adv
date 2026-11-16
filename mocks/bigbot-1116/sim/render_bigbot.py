#!/usr/bin/env python3
"""MOCK ONLY. Each home-spot bot as a big-center CONTROLbot Color UI v6 screen.

Clones the locked v6 still (a87fe22, default theme NEON GRAPE = lv_theme 0) and swaps
only the 110x110 center face (THEME_FACE 65,88..174,197) and an optional name in the
black-frame pad under the face. Board, coil, pins, ticker, and control rows stay
byte-identical to the locked still.
"""
import hashlib, json, subprocess, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

HERE = Path('/tmp/bigbot-1116-build')
OUT = Path('/workspace/out/bigbot-1116')
STILL = OUT / 'stills'
THEME = OUT / 'themes'
OVER = OUT / 'overlay'
SIM = OUT / 'sim'
for d in (STILL, THEME, OVER, SIM):
    d.mkdir(parents=True, exist_ok=True)

ZIG = '/home/box/.local/lib/python3.13/site-packages/ziglang/zig'
EXE = HERE / 'bigbot_compose'
LIVE = Path('/workspace/cyd-portrait-wt/src/live')
V6 = Path('/workspace/out/controlbot-colorui-1115/v6-cyd-ticker/stills')
HOME3 = Path('/workspace/cyd-fw-3locks-wt/docs/builds/fw-3locks-1116/renders/th0_home_3x.png')
FW3 = Path('/workspace/cyd-fw-3locks-wt')

# v6 geometry
FACE = (65, 88, 174, 197)   # inclusive THEME_FACE
BOT  = (54, 79, 185, 210)   # inclusive LV_CB_BOT
NAME_Y = 200
W, H = 240, 320

# Home spots from src/live/lv_core.h @ c1b5659 (and ada2253 v1.1 rewires)
# LV_HOME_CHIP[5] = {{31, 52, 74, 112}, {78, 30, 160, 150}, {165, 51, 208, 110}, {33, 156, 101, 255}, {139, 156, 207, 255}}
HOME_SPOTS = [
    dict(i=0, id='botpheus', label='BOTpheus', home_name_c1b='MORPHEUS / bot',
         rect=(31, 52, 74, 112),
         route_c1b5659='LV_SC_GLOSS (morpheus-glossary)',
         route_v11='LV_SC_MO BOTpheus EB3 (botpheus-eb3, ada2253)'),
    dict(i=1, id='gibson', label='GIBSONbot', home_name_c1b='GIBSONbot',
         rect=(78, 30, 160, 150),
         route_c1b5659='LV_SC_RADAR (gibson-jackin-radar)',
         route_v11='unchanged'),
    dict(i=2, id='control', label='CONTROLbot', home_name_c1b='CONTROL / bot',
         rect=(165, 51, 208, 110),
         route_c1b5659='LV_SC_CONTROLBOT (ctl, v6)',
         route_v11='unchanged'),
    dict(i=3, id='hackbot', label='HACKbot', home_name_c1b='SCANbot',
         rect=(33, 156, 101, 255),
         route_c1b5659='LV_SC_SCAN (scan)',
         route_v11='LV_SC_DL HACKbot game (hackbot-game, ada2253); name pixels SCANbot->HACKbot'),
    dict(i=4, id='terminal', label='TERMINALbot', home_name_c1b='TERMINALbot',
         rect=(139, 156, 207, 255),
         route_c1b5659='LV_SC_TERM (term)',
         route_v11='unchanged'),
]

DEFAULT = V6 / '12_neon_grape@1x.png'
OTHER_THEMES = [
    ('01_circuit', 'CIRCUIT'),
    ('15_ice', 'ICE'),
    ('18_gold_rush', 'GOLD RUSH'),
]

def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def to_rgb(im: np.ndarray, path: Path):
    path.write_bytes(np.ascontiguousarray(im, np.uint8).tobytes())

def from_rgb(path: Path, w=W, h=H) -> np.ndarray:
    return np.fromfile(path, np.uint8).reshape(h, w, 3)

def nn3(a: np.ndarray) -> np.ndarray:
    return np.repeat(np.repeat(a, 3, 0), 3, 1)

def save_pngs(a: np.ndarray, stem: Path):
    Image.fromarray(a).save(stem.with_name(stem.name + '@1x.png'))
    Image.fromarray(nn3(a)).save(stem.with_name(stem.name + '@3x.png'))

def compose(base: np.ndarray, kind: str, name: str, raw=None) -> np.ndarray:
    bp, op = HERE / 'base.rgb', HERE / 'out.rgb'
    to_rgb(base, bp)
    cmd = [str(EXE), str(bp), str(op), kind]
    if kind == 'raw':
        rw, rh = raw.shape[1], raw.shape[0]
        rp = HERE / 'raw.rgb'
        to_rgb(raw, rp)
        cmd += [str(rw), str(rh), str(rp)]
    if name:
        cmd.append(name)
    subprocess.check_call(cmd)
    return from_rgb(op)

def morpheus_face() -> np.ndarray:
    """Locked MORPHEUSbot / BOTpheus face from variant_big_morpheus.morpheus_chip
    at the glossary_big designed size, cropped to the screen window."""
    sys.path.insert(0, '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-rename/tool')
    sys.path.insert(0, '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-simple/tool')
    sys.path.insert(0, '/workspace/cyd-fw-3locks-wt/docs/mocks/morpheus-glossary/tool')
    import variant_big_morpheus as V
    a = np.zeros((320, 240, 3), np.uint8)
    f = a.copy()
    V.morpheus_chip(a, f, **V.SIZES['glossary_big'])
    sx0, sy0, sx1, sy1 = V.SIZES['glossary_big']['screen']
    face = a[sy0:sy1 + 1, sx0:sx1 + 1].copy()
    Image.fromarray(face.repeat(4, 0).repeat(4, 1)).save(HERE / 'morph_src_x4.png')
    return face

def gibson_face() -> np.ndarray:
    """Locked GIBSONbot face from the fw-3locks-1116 home render (exact 3x NN, theme-0 file).
    Crop is the cream monitor interior + shades, measured on the 1x home pixels."""
    home3 = np.array(Image.open(HOME3).convert('RGB'))
    # verified 3x NN (every 3x3 block uniform)
    home1 = home3[::3, ::3]
    # chip (78,30,160,150). Cream face + its own bevel, no side-pin purple,
    # no outer black chassis (the v6 chip already has LV_CB_BOT's black frame).
    # Measured on 1x home: cream (98,58)-(142,102), +2 px bevel.
    x0, y0, x1, y1 = 96, 56, 144, 104
    face = home1[y0:y1 + 1, x0:x1 + 1].copy()
    Image.fromarray(face.repeat(4, 0).repeat(4, 1)).save(HERE / 'gibson_src_x4.png')
    return face

def diff_bbox(a: np.ndarray, b: np.ndarray):
    d = np.any(a != b, axis=2)
    if not d.any():
        return None, 0, d
    ys, xs = np.where(d)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())), int(d.sum()), d

def allowed_mask():
    """Center bot (LV_CB_BOT) + name pad (already inside LV_CB_BOT y198..210)."""
    m = np.zeros((H, W), bool)
    x0, y0, x1, y1 = BOT
    m[y0:y1 + 1, x0:x1 + 1] = True
    return m

def main():
    # compile already done; rebuild if needed
    if not EXE.exists():
        subprocess.check_call([
            ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-w',
            '-DLV_HOST=1', '-I' + str(LIVE), '-o', str(EXE), str(HERE / 'bigbot_compose.cpp')])

    base = np.array(Image.open(DEFAULT).convert('RGB'))
    assert base.shape == (H, W, 3)
    morph = morpheus_face()
    gibson = gibson_face()

    jobs = [
        dict(id='botpheus', kind='raw', raw=morph, name='BOTpheus'),
        dict(id='gibson', kind='raw', raw=gibson, name='GIBSONbot'),
        dict(id='control', kind='none', raw=None, name=''),  # locked v6, no name on the lock
        dict(id='hackbot', kind='gold', raw=None, name='HACKbot'),
        dict(id='terminal', kind='silver', raw=None, name='TERMINALbot'),
    ]

    allow = allowed_mask()
    proof = {
        'default_theme': 'NEON GRAPE (lv_theme=0, LV_CB slot 11, stills/12_neon_grape@1x.png)',
        'v6_still_sha256': sha256(DEFAULT),
        'face_slot': {'THEME_FACE_inclusive': list(FACE), 'LV_CB_BOT_inclusive': list(BOT)},
        'bots': [],
        'outside_bot_px': {},
    }
    frames = {}
    for j in jobs:
        im = compose(base, j['kind'], j['name'], j['raw'])
        bbox, n, d = diff_bbox(im, base)
        outside = int((d & ~allow).sum()) if n else 0
        rec = dict(id=j['id'], name=j['name'] or 'CONTROLbot (locked v6, no name on still)',
                   kind=j['kind'], changed_px=n, diff_bbox=bbox, changed_outside_LV_CB_BOT=outside)
        proof['bots'].append(rec)
        proof['outside_bot_px'][j['id']] = outside
        frames[j['id']] = im
        save_pngs(im, STILL / j['id'])
        print(rec)

    # contact sheet: 5 @3x side by side with labels
    pads, lw, labh = 16, 4, 48
    tiles = [nn3(frames[j['id']]) for j in jobs]
    cw, ch = 720, 960
    sheet = Image.new('RGB', (pads + 5 * (cw + pads), ch + labh + 2 * pads), (8, 8, 10))
    dr = ImageDraw.Draw(sheet)
    # crude pixel labels via our 1x text later; for now paste and write with default
    labels = ['BOTpheus', 'GIBSONbot', 'CONTROLbot', 'HACKbot', 'TERMINALbot']
    for i, (t, lab) in enumerate(zip(tiles, labels)):
        x = pads + i * (cw + pads)
        y = pads + labh
        sheet.paste(Image.fromarray(t), (x, y))
        # label bar
        dr.rectangle([x, pads, x + cw, pads + labh - 4], fill=(20, 20, 24))
    # draw GLCD labels via compose helper: render a tiny 240x16 strip per name
    # Use simple upsample of a 1x text row
    def glcd_line(text, col=(231, 239, 247), sc=3):
        # render at 1x into 240x16 via C++ name on a black frame
        black = np.zeros((H, W, 3), np.uint8)
        # put a dummy base and only use the name band
        tmp = compose(black, 'none', text)
        band = tmp[NAME_Y:NAME_Y + 8]
        # crop to text width
        ink = np.any(band != 0, axis=2)
        if not ink.any():
            return np.zeros((8 * sc, 8, 3), np.uint8)
        xs = np.where(ink.any(0))[0]
        crop = band[:, xs.min():xs.max() + 1]
        return np.repeat(np.repeat(crop, sc, 0), sc, 1)
    for i, lab in enumerate(labels):
        g = glcd_line(lab, sc=4)
        x = pads + i * (cw + pads)
        gx = x + (cw - g.shape[1]) // 2
        gy = pads + (labh - 4 - g.shape[0]) // 2
        sheet.paste(Image.fromarray(g), (gx, gy))
    sheet_path = OUT / 'contact_sheet_5bots@3x.png'
    sheet.save(sheet_path)
    print('contact', sheet_path, sheet.size)

    # one bot (HACKbot) in 3 other themes
    theme_frames = []
    for stem, tname in OTHER_THEMES:
        body = np.array(Image.open(V6 / f'{stem}@1x.png').convert('RGB'))
        im = compose(body, 'gold', 'HACKbot')
        save_pngs(im, THEME / f'hackbot_{stem}')
        theme_frames.append((tname, im))
        b, n, d = diff_bbox(im, body)
        print('theme', tname, 'bbox', b, 'px', n, 'outside', int((d & ~allow).sum()))

    # theme contact
    ts = Image.new('RGB', (pads + 3 * (cw + pads), ch + labh + 2 * pads), (8, 8, 10))
    for i, (tname, im) in enumerate(theme_frames):
        x = pads + i * (cw + pads)
        ts.paste(Image.fromarray(nn3(im)), (x, pads + labh))
        g = glcd_line('HACKbot ' + tname, sc=3)
        gx = x + (cw - g.shape[1]) // 2
        gy = pads + (labh - 4 - g.shape[0]) // 2
        ts.paste(Image.fromarray(g), (gx, gy))
    ts_path = OUT / 'hackbot_3themes@3x.png'
    ts.save(ts_path)

    # concept overlay: swipe-strip on a copy of CONTROLbot, clearly marked
    overlay_sketch(frames, labels, glcd_line)

    # swipe animation BOTpheus -> GIBSONbot (HOME_CHIP order, swipe left = next)
    swipe_anim(frames['botpheus'], frames['gibson'])

    # hashes + proof
    files = []
    for p in sorted(OUT.rglob('*')):
        if p.is_file() and p.suffix.lower() in {'.png', '.mp4', '.gif', '.json', '.md', '.cpp', '.py'}:
            files.append((str(p.relative_to(OUT)), sha256(p), p.stat().st_size))
    proof['files'] = [{'path': a, 'sha256': b, 'bytes': c} for a, b, c in files]
    (OUT / 'diff_proof.json').write_text(json.dumps(proof, indent=2) + '\n')
    lines = []
    for a, b, c in files:
        lines.append(f'{b}  {a}')
    (OUT / 'SHA256SUMS').write_text('\n'.join(lines) + '\n')
    print('done', len(files), 'files')
    return proof

def overlay_sketch(frames, labels, glcd_line):
    """Concept-only overlay: bot-order strip just under the ticker, current highlighted.
    Not a change to the locked look — delivered as a separate sketch."""
    cur = 2  # CONTROLbot, HOME_CHIP index
    shorts = ['BOTph', 'GIBSON', 'CONTROL', 'HACK', 'TERM']
    a = frames['control'].copy()
    # strip occupies y24..36 (first body rows under LV_R_TICK y1..23) — concept only
    y0, y1 = 24, 37
    a[y0:y1, 1:239] = (8, 8, 16)
    a[y0, 1:239] = (180, 40, 255)
    a[y1 - 1, 1:239] = (180, 40, 255)
    a[y0:y1, 1] = (180, 40, 255)
    a[y0:y1, 238] = (180, 40, 255)
    inner = 236
    cell = inner // 5
    # helper: blit 1x GLCD into array
    def blit_text(arr, text, x, y, col, sc=1):
        # reuse compose name-band
        black = np.zeros((H, W, 3), np.uint8)
        tmp = compose(black, 'none', text)
        band = tmp[NAME_Y:NAME_Y + 8]
        ink = np.any(band != 0, axis=2)
        if not ink.any():
            return
        xs = np.where(ink.any(0))[0]
        crop = band[:, xs.min():xs.max() + 1]
        if sc > 1:
            crop = np.repeat(np.repeat(crop, sc, 0), sc, 1)
        # recolor non-black to col
        m = np.any(crop != 0, axis=2)
        h, w = crop.shape[:2]
        if y + h > arr.shape[0] or x + w > arr.shape[1] or x < 0:
            return
        arr[y:y + h, x:x + w][m] = col
        return w, h

    for i, lab in enumerate(shorts):
        cx0 = 2 + i * cell
        cx1 = cx0 + cell - 2
        if i == cur:
            a[y0 + 2:y1 - 2, cx0:cx1] = (168, 32, 240)
            a[y0 + 2, cx0:cx1] = (231, 239, 247)
            a[y1 - 3, cx0:cx1] = (231, 239, 247)
            a[y0 + 2:y1 - 2, cx0] = (231, 239, 247)
            a[y0 + 2:y1 - 2, cx1 - 1] = (231, 239, 247)
            ink = (255, 255, 255)
        else:
            a[y0 + 2:y1 - 2, cx0:cx1] = (24, 20, 36)
            ink = (200, 200, 210)
        # center short name in the cell (5x7)
        tw = len(lab) * 6 - 1
        tx = cx0 + (cx1 - cx0 - tw) // 2
        ty = y0 + 3
        blit_text(a, lab, tx, ty, ink, 1)

    # expand canvas with a concept caption and swipe chevrons on the ticker
    cap_h = 24
    canvas = np.zeros((H + cap_h, W, 3), np.uint8)
    canvas[:, :] = (12, 12, 16)
    canvas[:cap_h] = (32, 16, 8)
    canvas[cap_h:] = a
    blit_text(canvas, 'CONCEPT ONLY', 4, 2, (255, 220, 160), 1)
    blit_text(canvas, 'swipe ticker L/R to flip bots', 4, 12, (255, 220, 160), 1)
    # chevrons on ticker (ticker at y cap_h+1..23)
    def chev(arr, x, y, direc, col=(255, 220, 80)):
        h, w = 11, 8
        for i in range(h):
            span = i if i < h // 2 else h - 1 - i
            if direc < 0:
                arr[y + i, x + (w - 1 - span):x + w] = col
            else:
                arr[y + i, x:x + span + 1] = col
    chev(canvas, 3, cap_h + 6, -1)
    chev(canvas, W - 11, cap_h + 6, +1)
    Image.fromarray(nn3(canvas)).save(OVER / 'nav_strip_overlay_control@3x.png')
    Image.fromarray(canvas).save(OVER / 'nav_strip_overlay_control@1x.png')
    # standalone strip card (not covering a locked screen)
    card = np.zeros((80, 240, 3), np.uint8)
    card[:] = (12, 12, 16)
    blit_text(card, 'SWIPE TICKER  bot order', 8, 6, (255, 220, 160), 1)
    blit_text(card, 'current = highlight', 8, 16, (180, 180, 200), 1)
    y0, y1 = 32, 56
    cell = 236 // 5
    for i, lab in enumerate(shorts):
        cx0 = 2 + i * cell
        cx1 = cx0 + cell - 2
        if i == cur:
            card[y0:y1, cx0:cx1] = (168, 32, 240)
            ink = (255, 255, 255)
        else:
            card[y0:y1, cx0:cx1] = (32, 28, 44)
            ink = (200, 200, 210)
        tw = len(lab) * 6 - 1
        blit_text(card, lab, cx0 + (cx1 - cx0 - tw) // 2, y0 + 8, ink, 1)
    # arrows
    blit_text(card, 'L', 4, 64, (255, 220, 80), 1)
    blit_text(card, 'R', 228, 64, (255, 220, 80), 1)
    Image.fromarray(nn3(card)).save(OVER / 'nav_strip_legend@3x.png')
    Image.fromarray(card).save(OVER / 'nav_strip_legend@1x.png')
    print('overlay written')

def swipe_anim(a: np.ndarray, b: np.ndarray):
    """Full-frame horizontal slide, swipe-left on the banner: A out to the left, B in from the right."""
    n = 24  # 0.8s at 30fps
    frames = []
    for i in range(n + 1):
        t = i / n
        # ease in-out
        e = t * t * (3 - 2 * t)
        off = int(round(e * W))
        fr = np.zeros_like(a)
        if off < W:
            fr[:, :W - off] = a[:, off:]
        if off > 0:
            fr[:, W - off:] = b[:, :off]
        frames.append(fr)
    raw = HERE / 'swipe.rgb'
    with raw.open('wb') as f:
        for fr in frames:
            f.write(np.ascontiguousarray(fr, np.uint8).tobytes())
    mp4 = OUT / 'swipe_botpheus_to_gibson@3x.mp4'
    subprocess.check_call([
        'ffmpeg', '-y', '-framerate', '30', '-f', 'rawvideo', '-pix_fmt', 'rgb24',
        '-s', '240x320', '-i', str(raw),
        '-vf', 'scale=720:960:flags=neighbor',
        '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '17', '-movflags', '+faststart',
        str(mp4)
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # gif @3x (short, palette)
    gif = OUT / 'swipe_botpheus_to_gibson@3x.gif'
    pal = HERE / 'pal.png'
    subprocess.check_call([
        'ffmpeg', '-y', '-framerate', '30', '-f', 'rawvideo', '-pix_fmt', 'rgb24',
        '-s', '240x320', '-i', str(raw),
        '-vf', 'scale=720:960:flags=neighbor,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse=dither=none',
        str(gif)
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print('swipe', mp4, gif, 'frames', len(frames))

if __name__ == '__main__':
    main()
