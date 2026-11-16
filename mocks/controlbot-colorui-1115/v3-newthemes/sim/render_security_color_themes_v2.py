#!/usr/bin/env python3
"""SECURITYbot Color UI themes v2 (mock). THEME_LOCK 8:18 AM ET: 11 themes, names + order + designs locked.
v2 = v1 + the 4 approved fixes (ALARM calmer / SCAN value box taller / IR heat palette in the panel / PERIM off ICE cyan)
+ BAR button (cycles top-bar styles) replacing SPECS; faces show native eyes. Re-ink = themes_spec_v2.h compiled with
zig c++ against the native lv_draw.h. Gates live in sim/verify_security_color_themes_v2.py."""
import sys, re, json, subprocess, importlib.util
sys.dont_write_bytecode = True
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'mocks/security-color-themes-v2'
V1 = ROOT / 'mocks/security-color-themes-v1'
B = Path('/tmp/secbot_build/themes_v2')
SPEC = OUT / 'themes_spec_v2.h'
CPP = ROOT / 'sim/security_themes_reink_v2.cpp'
ZIG = '/tmp/rci/zig/ziglang/zig'
LIVE = '/workspace/cyd-portrait-wt/src/live'
TOPBAR = ROOT / 'mocks/topbar-v1-matrix-security'
LIME = (120, 252, 0)
TXT = (200, 0, 0)  # sentinel for white text in badges / value box
BAR_CYCLE = ['RAIN', 'DECOD', 'TICK', 'HAZRD', 'HUD']   # M1 code rain, M2 decode, M3 ticker, S1 hazard+ARMED, S2 HUD+REC
BAR_DEFAULT = 'HAZRD'   # default top bar = S1 hazard+ARMED (direct successor of the v1 black+lime hazard bar)
SCAN_BOX_PAD = 1        # v2 fix 2: value box = glyph + 1 px pad all sides (v1: glyph rows only, 1 px sides)

_s = importlib.util.spec_from_file_location('v9c', ROOT / 'sim/render_controlbot_bigchip_v9c.py')
v9c = importlib.util.module_from_spec(_s); _s.loader.exec_module(v9c)
GL = v9c.glyph

def parse_spec(path=SPEC):
    t = Path(path).read_text()
    arr = lambda n: re.search(n + r'\[SEC_NTHEME\]\s*=\s*\{([^}]*)\}', t).group(1)
    names = re.findall(r'"([^"]+)"', arr('SEC_THEME_NAME')); full = re.findall(r'"([^"]+)"', arr('SEC_THEME_FULL'))
    tab = {k: [int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]+', arr(k))] for k in ('SEC_ACC', 'SEC_ACCD', 'SEC_HDR', 'SEC_FRAME')}
    return names, full, tab

def scan_box_rect(x=46, y=221, w=68, value='B.RAIN'):
    """v2 fix 2: taller SCAN value box (exclusive end). Glyph rows y+3..y+9 -> box rows y+2..y+10 (9 tall);
    glyph x unchanged from v1; 1 px pad on all four sides, 3 px clear of the label, 1 px clear of the frame and meter."""
    vw = GL(value).shape[1]; vx = x + w - 3 - vw; ty = y + 3
    return (vx - SCAN_BOX_PAD, ty - SCAN_BOX_PAD, vx + vw + SCAN_BOX_PAD, ty + 7 + SCAN_BOX_PAD), vx, ty

def draw_scan_bar(im, x, y, w, h, label, value, lit_n):
    im[y:y + h, x:x + w] = v9c.COL_PLATE
    im[y, x:x + w] = v9c.COL_KEY; im[y + h - 1, x:x + w] = v9c.COL_KEY; im[y:y + h, x] = v9c.COL_KEY; im[y:y + h, x + w - 1] = v9c.COL_KEY
    v9c.text(im, x + 3, y + 3, label, v9c.COL_LABEL)
    (bx0, by0, bx1, by1), vx, ty = scan_box_rect(x, y, w, value)
    im[by0:by1, bx0:bx1] = (222, 146, 255)
    v9c.text(im, vx, ty, value, (255, 255, 255))
    v9c.draw_meter(im, x + 3, y + 12, w - 6, 5, lit_n, n_seg=8)

def paint_v2(ice, bar_value=BAR_DEFAULT, scan_idx=0, vol=40, bright=80):
    im = ice.copy()     # native eyes: no glasses anywhere
    x0, y0, x1, y1 = v9c.PANEL
    im[y0:y1, x0:x1] = v9c.COL_PLATE
    im[y0, x0:x1] = v9c.COL_KEY; im[y1 - 1, x0:x1] = v9c.COL_KEY; im[y0:y1, x0] = v9c.COL_KEY; im[y0:y1, x1 - 1] = v9c.COL_KEY
    draw_scan_bar(im, 46, 221, 68, 22, 'SCAN', v9c.SCANBG_CYCLE[scan_idx], scan_idx + 1)
    v9c.draw_theme_bar(im, 46, 244, 68, 22, 'VOL', f'{vol}%', max(1, int(round(8 * vol / 100))), n_seg=8)
    v9c.draw_theme_bar(im, 46, 267, 68, 22, 'BRIGHT', f'{bright}%', max(1, int(round(8 * bright / 100))), n_seg=8)
    v9c.draw_cycle_btn(im, 116, 221, 39, 34, 'CHIME', 'ON', on=True)
    v9c.draw_cycle_btn(im, 157, 221, 39, 34, 'TIME', 'RTC', on=True)
    v9c.draw_cycle_btn(im, 116, 256, 39, 34, 'BAR', bar_value, on=True)    # fix 5: BAR replaces SPECS
    v9c.draw_cycle_btn(im, 157, 256, 39, 34, 'TONE', 'BEEP', on=True)
    im[y0:y1, x0:x1] = v9c.to_rgb565(im[y0:y1, x0:x1])
    return im

HDR_MAP = {(41, 4, 0): (0, 0, 0), (231, 0, 0): (0, 0, 0), (255, 203, 49): LIME, (255, 178, 0): LIME, (33, 0, 0): (24, 60, 0)}
def lime_header(base):
    """Fixed BLACK + LIME top bar (rows 1..23 inside the gold rim), as v1. Never re-inked by any theme."""
    im = base.copy(); reg = im[1:24, 1:239]
    text_mask = np.all(reg == (255, 203, 49), axis=2)
    out = reg.copy(); done = np.zeros(reg.shape[:2], bool)
    for src, dst in HDR_MAP.items():
        m = np.all(reg == src, axis=2); out[m] = dst; done |= m
    assert done.all(), 'unmapped header colours'
    im[1:24, 1:239] = out
    return im, text_mask

BTN = {'CHIME': (116, 221, 39, 34), 'TIME': (157, 221, 39, 34), 'BAR': (116, 256, 39, 34), 'TONE': (157, 256, 39, 34)}
def topbar_header(style):
    """rows 1..23, x 1..238 of the topbar-v1-matrix-security @1x render for this BAR value (None if absent)."""
    tb = topbar_renders()
    if not tb: return None
    return np.array(Image.open(tb[style]).convert('RGB'))[1:24, 1:239].copy()

def base_and_roles(bar_value=BAR_DEFAULT):
    ice = np.array(Image.open(v9c.V5PNG).convert('RGB'))
    base = paint_v2(ice, bar_value)
    orig_text = v9c.text
    v9c.text = lambda im, x, y, s, col, font='glcd': orig_text(im, x, y, s, TXT if tuple(col) == (255, 255, 255) else col, font)
    try: sen = paint_v2(ice, bar_value)
    finally: v9c.text = orig_text
    base, hdr_text_mask = lime_header(base)
    hdr = topbar_header(bar_value)
    if hdr is not None: base[1:24, 1:239] = hdr      # BAR value picks the top-bar style (fix 5)
    a = base.astype(int); s = sen.astype(int)
    role = np.zeros((320, 240), np.uint8)
    R, G, Bl = a[..., 0], a[..., 1], a[..., 2]
    role[88:198, 65:175] = 1
    purple = (np.maximum(R, Bl) - G >= 60) & (Bl >= 80)
    chip = np.zeros_like(purple); chip[25:316, 22:218] = True
    px0, py0, px1, py1 = v9c.PANEL
    pan = np.zeros_like(purple); pan[py0:py1, px0:px1] = True
    role[purple & chip & ~pan] = 2
    eq = lambda c: (a[..., 0] == c[0]) & (a[..., 1] == c[1]) & (a[..., 2] == c[2])
    role[pan & eq((144, 72, 168))] = 3
    role[pan & eq((216, 144, 248))] = 4
    role[pan & eq((32, 12, 48))] = 5
    role[pan & (s[..., 0] == 200) & (s[..., 1] == 0) & (s[..., 2] == 0)] = 6
    role[:25] = 0
    samp = np.zeros((320, 240, 2), np.int16)
    rects = [(x + 3, y + 12, x + w - 3, y + h - 3) for (x, y, w, h) in BTN.values()]
    rects.append(scan_box_rect()[0])
    for x0, y0, x1, y1 in rects:
        sub = role[y0:y1, x0:x1]; sub[sub == 4] = 7
        samp[y0:y1, x0:x1] = ((x0 + x1) // 2, (y0 + y1) // 2)
    return base, role, samp, hdr_text_mask, rects

def build():
    B.mkdir(parents=True, exist_ok=True)
    subprocess.check_call([ZIG, 'c++', '-target', 'x86_64-linux-gnu.2.36', '-O2', '-std=gnu++17', '-Wall', '-Wno-unused-function',
                           '-I' + LIVE, '-I' + str(OUT), '-o', str(B / 'reink_v2'), str(CPP)])

def reink(base, role, samp, t, tag='m'):
    (B / f'base_{tag}.rgb').write_bytes(base.tobytes()); (B / f'role_{tag}.u8').write_bytes(role.tobytes()); (B / f'samp_{tag}.i16').write_bytes(samp.tobytes())
    subprocess.check_call([str(B / 'reink_v2'), str(t), str(B / f'base_{tag}.rgb'), str(B / f'role_{tag}.u8'), str(B / f'out_{tag}{t}.rgb'), str(B / f'samp_{tag}.i16')])
    return np.frombuffer((B / f'out_{tag}{t}.rgb').read_bytes(), np.uint8).reshape(320, 240, 3).copy()

def big_text(img, x, y, s, col, k):
    m = GL(s); d = ImageDraw.Draw(img)
    for yy, xx in zip(*np.nonzero(m)):
        d.rectangle([x + xx * k, y + yy * k, x + xx * k + k - 1, y + yy * k + k - 1], fill=col)
    return m.shape[1] * k

def to565hex(c):
    r, g, b = (c >> 16) & 255, (c >> 8) & 255, c & 255
    return '0x%04X' % (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

def topbar_renders():
    """Return {style: 240x25 rgb} if mocks/topbar-v1-matrix-security has per-style @1x renders, else None."""
    if not TOPBAR.is_dir(): return None
    found = {}
    for key, pats in {'RAIN': ('m1',), 'DECOD': ('m2',), 'TICK': ('m3',), 'HAZRD': ('s1',), 'HUD': ('s2',)}.items():
        c = sorted(p for p in TOPBAR.glob('*@1x.png') if any(p.name.lower().startswith(q) or f'_{q}' in p.name.lower() for q in pats))
        if c: found[key] = c[0]
    return found if len(found) == 5 else None

def main():
    tracked = subprocess.run(['git', '-C', str(ROOT), 'ls-files', str(OUT)], capture_output=True, text=True).stdout
    assert '.png' not in tracked, 'refusing to overwrite a committed mock folder'
    build()
    names, full, tab = parse_spec(); n = len(names)
    base, role, samp, _, _ = base_and_roles()
    for t in range(n):
        im = reink(base, role, samp, t)
        tag = f'{t + 1:02d}_{names[t].lower()}'
        Image.fromarray(im).save(OUT / f'{tag}@1x.png')
        Image.fromarray(im.repeat(3, 0).repeat(3, 1)).save(OUT / f'{tag}@3x.png')
        Image.fromarray(im).resize((720, 960), Image.Resampling.LANCZOS).save(OUT / f'{tag}@3x_smooth.png')
    cw, ch, lh, pad, cols, rows = 720, 960, 76, 24, 4, 3
    grid = Image.new('RGB', (cols * cw + (cols + 1) * pad, rows * (ch + lh) + (rows + 1) * pad), (6, 6, 8))
    for t in range(n):
        cx, cy = pad + (t % cols) * (cw + pad), pad + (t // cols) * (ch + lh + pad)
        grid.paste(Image.open(OUT / f'{t + 1:02d}_{names[t].lower()}@3x_smooth.png'), (cx, cy + lh))
        big_text(grid, cx + 4, cy + 6, f'{t + 1:02d} {full[t]}', (255, 255, 255), 4)
        big_text(grid, cx + 4, cy + 42, f'{names[t]}  {"SOLID" if t < 2 else "MULTI"}', (160, 170, 180), 4)
    cx, cy = pad + 3 * (cw + pad), pad + 2 * (ch + lh + pad)
    lines = ['SECURITYbot', 'COLOR UI v2', 'EX.DATA MOCK', '', 'THEME LOCK', '11 THEMES', '', 'TAP FACE', 'TO CYCLE', '', 'BAR = TOP BAR', 'NOT FIRMWARE']
    for i, s in enumerate(lines):
        if s: big_text(grid, cx + 40, cy + 100 + i * 66, s, LIME if i == 0 else (200, 205, 210), 6)
    grid.save(OUT / '00_themes_grid@3x_smooth.png')
    tb = topbar_renders()
    if tb:   # 1x5 strip: LIME theme, BAR value + matching top bar for each style
        cells = []
        for v in BAR_CYCLE:
            b2, r2, s2, _, _ = base_and_roles(v)
            im = reink(b2, r2, s2, 0, tag='bar')
            Image.fromarray(im).save(B / f'bar_{v}@1x.png')
            cells.append(np.array(Image.fromarray(im).resize((720, 960), Image.Resampling.LANCZOS)))
        gap = np.full((960 + 76, 24, 3), 6, np.uint8)
        cols_ = []
        for v, c in zip(BAR_CYCLE, cells):
            lab = Image.new('RGB', (720, 76), (6, 6, 8)); big_text(lab, 4, 20, f'BAR = {v}', (255, 255, 255), 5)
            cols_ += [np.vstack([np.array(lab), c]), gap]
        Image.fromarray(np.hstack(cols_[:-1])).save(OUT / '00_bar_styles_1x5@3x_smooth.png')
        st = np.hstack([np.array(Image.open(B / f'bar_{v}@1x.png')) for v in BAR_CYCLE])
        Image.fromarray(st).save(OUT / '00_bar_styles_1x5@1x.png')
    (B / 'topbar_status.json').write_text(json.dumps({'topbar_renders': {k: str(v) for k, v in tb.items()} if tb else None}))
    rows_md = [f'| {t + 1} | `{names[t]}` | {full[t]} | {"solid" if t < 2 else "multi"} | ' +
               ' | '.join(f'`{tab[k][t]:06X}` / `{to565hex(tab[k][t])}`' for k in ('SEC_ACC', 'SEC_ACCD', 'SEC_HDR', 'SEC_FRAME')) + ' |' for t in range(n)]
    lock = '\n'.join(f'{t + 1}. `{names[t]}` - {full[t]}' for t in range(n))
    (OUT / 'README.md').write_text(README.format(rows='\n'.join(rows_md), lock=lock,
        topbar=('top bar = `mocks/topbar-v1-matrix-security/` @1x header rows 1..23 (x 1..238) for the BAR value; every theme shows the default '
        '**HAZRD (S1 hazard + ARMED)**. `00_bar_styles_1x5@3x_smooth.png` / `@1x.png`: LIME theme, BAR = RAIN / DECOD / TICK / HAZRD / HUD with the matching bar. '
        'Source hashes in VERIFICATION.json.') if tb else
        '**`mocks/topbar-v1-matrix-security/` did not exist at render time**, so every theme keeps the fixed v1 black+lime hazard bar, BAR shows `HAZRD` (closest match), and the 1x5 BAR strip was NOT rendered.'))
    print('rendered', n, 'themes; topbar renders:', bool(tb))

README = """# SECURITYbot Color UI themes v2 (mock, EX.DATA)

CONTROLbot big-chip panel (v9c layout, native eyes, no glasses) re-inked in every theme. Tap the big face to cycle.
Mock only - nothing is wired into firmware. Gates: `VERIFICATION.json` (from `sim/verify_security_color_themes_v2.py`).

## THEME_LOCK (Mobandy, 2026-11-09 8:18 AM ET)
Names, order and designs of these 11 themes are LOCKED. v2 changes only the 4 approved fixes below.
{lock}

## v2 changes (approved 8:18 AM ET)
1. **ALARM STROBE calmer**: face / chip-frame hazard bands 14 px (v1: 8 px); panel chrome flat alarm red, so labels never sit on stripes.
2. **SCAN value box taller**: 9 px (v1: 7 px = glyph height): 1 px pad on all sides, 3 px clear of the label, 1 px clear of the frame and meter. Theme-bar size, panel rect and 2 px void margins unchanged.
3. **IR themes (IRON, HEAT, WHHOT, REDHT)**: each left bar (SCAN / VOL / BRIGHT) runs the FULL heat palette cold -> hot left -> right (thermal scale; meter segments step through it) and the 4 buttons take 4 rising heat steps (`sec_panel_at` -> `sec_ir_at`). Faces unchanged.
4. **PERIMETER GRID** off native ICE cyan: UV laser-tripwire violet rings B050FF / 6A20C0 + F0D8FF grid (v1: 00E8FF / 00A0B8 / C8FFFF). Same ring + grid design.
5. **BAR button replaces SPECS** (Mobandy 8:18 AM: top announcement bar interchangeable from that button). Values cycle RAIN / DECOD / TICK / HAZRD / HUD (M1 code rain, M2 decode, M3 ticker, S1 hazard+ARMED, S2 HUD+REC). Glasses removed; faces show native eyes.
   Top bar: {topbar}

Top bar stays black + lime (+ white highlights / AA from the top-bar styles) in every theme, never re-inked.

| # | NAME (31 px) | FULL | kind | ACC | ACCD | HDR | FRAME |
|---|---|---|---|---|---|---|---|
{rows}

Values are `0xRRGGBB` / RGB565; on multi themes they are lv_blit_theme sentinels and the real ink is `sec_theme_at` (face, chip frame) / `sec_panel_at` (panel).

## Files
- `00_themes_grid@3x_smooth.png` (all 11, full names), `00_bar_styles_1x5@3x_smooth.png` + `@1x.png` (if top-bar renders present), `NN_<name>@1x.png` / `@3x.png` (nearest) / `@3x_smooth.png` (Lanczos)
- `themes_spec_v2.h` SPEC, `VERIFICATION.json`, `SHA256SUMS`
- source: `sim/render_security_color_themes_v2.py`, `sim/security_themes_reink_v2.cpp`, `sim/verify_security_color_themes_v2.py`

videoReview: not run (no clip in this pass).
"""

if __name__ == '__main__':
    main()
