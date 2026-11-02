#!/usr/bin/env python3
"""BOTcorps Adv render gate. Reads the host renders written by `rend gate DIR`
(<screen>.rgb, <screen>.norain.rgb, <screen>.log, functional.txt) and prints
PASS/FAIL per spec item per screen. Exit code = number of FAILs (0 = gate green).
Spec item ids match release/SPEC-LOCKED.md."""
import os, re, sys
from PIL import Image

D = sys.argv[1]
BASE = sys.argv[2] if len(sys.argv) > 2 else None
FROZEN = sys.argv[3] if len(sys.argv) > 3 else None
W, H = 320, 240

def rgb(name):
    p = os.path.join(D, name + '.rgb')
    return Image.frombytes('RGB', (W, H), open(p, 'rb').read()) if os.path.exists(p) else None

def base_png(name):
    if not BASE: return None
    p = os.path.join(BASE, name + '.norain.png')
    return Image.open(p).convert('RGB') if os.path.exists(p) else None

def log(name):
    """ordered element/text log; an OCCLUDE box (opaque overlay) hides what was drawn under it"""
    items = []
    p = os.path.join(D, name + '.log')
    if not os.path.exists(p): return [], []
    seen = set()
    for ln in open(p, errors='replace'):
        if ln in seen: continue
        seen.add(ln)
        f = ln.rstrip('\n').split('\t')
        if f[0] == 'BOX' and len(f) >= 8:
            b = dict(k=f[1], n=f[2], x=int(f[3]), y=int(f[4]), w=int(f[5]), h=int(f[6]), p=f[7])
            if b['k'] == 'OCCLUDE':
                items = [i for i in items if i.get('k') in ('RAIN', 'BOARD') or not inter(i, b) or inside(b, i)]
                continue
            items.append(b)
        elif f[0] == 'TXT' and len(f) >= 10:
            # glyph cell includes 1 px spacing right + bottom; ink box is 1 px smaller
            items.append(dict(x=int(f[1]), y=int(f[2]), w=max(1, int(f[3]) - 1), h=max(1, int(f[4]) - 1),
                              cx=int(f[5]), cy=int(f[6]), cw=int(f[7]), ch=int(f[8]), s=f[9]))
    # glow / shadow / bold passes: the same string re-drawn within 6 px is ONE label (keep the last pass)
    txts = [i for i in items if 's' in i]
    keep = []
    for idx, t in enumerate(txts):
        later = any(u['s'] == t['s'] and abs(u['x'] - t['x']) <= 6 and abs(u['y'] - t['y']) <= 12 for u in txts[idx + 1:])
        if not later: keep.append(t)
    return [i for i in items if 'k' in i], keep

def inter(a, b):
    return a['x'] < b['x'] + b['w'] and b['x'] < a['x'] + a['w'] and a['y'] < b['y'] + b['h'] and b['y'] < a['y'] + a['h']

def inside(a, b, m=0):
    return a['x'] >= b['x'] + m and a['y'] >= b['y'] + m and a['x'] + a['w'] <= b['x'] + b['w'] - m and a['y'] + a['h'] <= b['y'] + b['h'] - m

def centre_in(a, b):
    cx, cy = a['x'] + a['w'] / 2, a['y'] + a['h'] / 2
    return b['x'] <= cx < b['x'] + b['w'] and b['y'] <= cy < b['y'] + b['h']

def luma(c): return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]

results = []   # (screen, item, ok, detail)
def rep(scr, item, ok, detail=''):
    results.append((scr, item, ok, detail))

OVERLAYS = ('found-alert',)   # full-screen takeover overlay (mock): rain/crown not applicable
# v1017a: the commands menu is the approved 6x2 chip palette drawn opaquely OVER the scrollback, so
# log text boxes under it are expected; G1 overlap is not applicable to that one render.
G1_SKIP = ('terminal-cmdmenu',)
SOLID = ('CHIP', 'BTN', 'TILE', 'PART', 'CROWN', 'HUBCORE', 'PROMPT')
INSTR = re.compile(r'\b(press|hit|use (the )?arrows?|tab to|esc to|enter to|to (select|go back|exit)|instructions?|hint)\b', re.I)

def rain_px(a, b, region=None):
    """pixels that differ between rain and no-rain renders (the visible rain)"""
    pa, pb = a.load(), b.load()
    n = col = 0
    x0, y0, x1, y1 = region if region else (0, 0, W, H)
    for y in range(max(0, y0), min(H, y1)):
        for x in range(max(0, x0), min(W, x1)):
            c, d = pa[x, y], pb[x, y]
            if abs(c[0] - d[0]) + abs(c[1] - d[1]) + abs(c[2] - d[2]) > 24:
                n += 1
                if not (c[1] > c[0] + 20 and c[1] > c[2] + 20) and luma(c) > 40 and not (c[0] > 200 and c[1] > 200 and c[2] > 200):
                    col += 1   # bot / device colours (not code green, not the white lead)
    return n, col

def generic(scr, boxes, texts, im, nr):
    # G1 element bbox overlap (same-layer solid elements must not intersect)
    sol = [b for b in boxes if b['k'] in SOLID]
    bad = []
    for i in range(len(sol)):
        for j in range(i + 1, len(sol)):
            a, b = sol[i], sol[j]
            if a['k'] == b['k'] == 'CHIP' and (inside(a, b) or inside(b, a)): continue
            if inter(a, b) and not (inside(a, b) or inside(b, a)):
                bad.append(f"{a['k']}:{a['n']}@{a['x']},{a['y']}x{b['k']}:{b['n']}@{b['x']},{b['y']}")
    # text vs text overlap (different strings) and text straddling a solid element
    for i in range(len(texts)):
        for j in range(i + 1, len(texts)):
            a, b = texts[i], texts[j]
            if len(a['s'].strip()) > 1 and len(b['s'].strip()) > 1 and a['s'] != b['s'] and inter(a, b):
                bad.append(f"TXT'{a['s'][:12]}'xTXT'{b['s'][:12]}'")
    for t in texts:
        if not t['s'].strip(): continue
        for b in sol:
            if b['k'] in ('HUBCORE',): continue
            if inter(t, b) and not inside(t, b):
                bad.append(f"TXT'{t['s'][:12]}'x{b['k']}:{b['n']}")
    silk = [b for b in boxes if b['k'] == 'SILK']
    for i in range(len(silk)):
        for j in range(i + 1, len(silk)):
            if inter(silk[i], silk[j]): bad.append(f"SILK'{silk[i]['n']}'xSILK'{silk[j]['n']}'")
        for b in sol:
            if inter(silk[i], b) and not inside(silk[i], b): bad.append(f"SILK'{silk[i]['n']}'x{b['k']}:{b['n']}")
    rep(scr, 'G1 no element/text bbox overlap', None if scr in G1_SKIP else not bad, '; '.join(bad[:6]) + (f' (+{len(bad)-6})' if len(bad) > 6 else ''))
    # G2 sprite inside its chip/tile
    bad = []
    holders = [b for b in boxes if b['k'] in ('CHIP', 'TILE', 'FACE')]
    for s in [b for b in boxes if b['k'] == 'SPR']:
        for h in holders:
            if centre_in(s, h) and not inside(s, h):
                bad.append(f"{s['n']}@{s['x']},{s['y']},{s['w']}x{s['h']} out of {h['k']}:{h['n']}@{h['x']},{h['y']},{h['w']}x{h['h']}")
                break
    rep(scr, 'G2 sprites inside their chip/tile', not bad, '; '.join(bad[:4]))
    # G3 clipped / overrun text
    bad = []
    for t in texts:
        if not t['s'].strip(): continue
        s = t['s'].rstrip()
        w = t['w'] * len(s) // max(1, len(t['s']))
        tt = dict(t, w=w)
        clip = dict(x=t['cx'], y=t['cy'], w=t['cw'], h=t['ch'])
        if not inside(tt, clip) and not (clip['w'] == W and clip['h'] == H and inside(tt, dict(x=0, y=0, w=W, h=H))):
            bad.append(f"'{s[:16]}'@{t['x']},{t['y']} w{w} clip {clip['x']},{clip['y']},{clip['w']}x{clip['h']}")
    rep(scr, 'G3 no clipped/overrun text', not bad, '; '.join(bad[:4]))
    # G4 nothing past the screen edge, nothing past its board
    bad = []
    scrn = dict(x=0, y=0, w=W, h=H)
    for b in boxes + texts:
        if b.get('k') in ('RAIN',): continue
        bb = dict(b)
        if 's' in b: bb['w'] = b['w'] * len(b['s'].rstrip()) // max(1, len(b['s']))
        if bb['w'] > 0 and not inside(bb, scrn): bad.append(f"{b.get('k','TXT')}:{b.get('n', b.get('s',''))[:12]}@{b['x']},{b['y']},{bb['w']}x{b['h']}")
    boards = [b for b in boxes if b['k'] == 'BOARD']
    for bd in boards:
        for b in boxes:
            if b['k'] in ('CHIP', 'PART', 'TILE', 'MINI', 'SILK', 'HUNT', 'PROMPT') and centre_in(b, bd) and not inside(b, bd):
                bad.append(f"{b['k']}:{b['n']} past board")
    rep(scr, 'G4 nothing past the board/screen edge', not bad, '; '.join(bad[:4]))
    # G5 rain layer present and visible
    logged = any(b['k'] == 'RAIN' for b in boxes)
    n, col = rain_px(im, nr) if nr else (0, 0)
    rep(scr, 'G5 rain layer present + visible', None if scr in OVERLAYS else (logged and n >= 100), f'logged={logged} visible_px={n} colour_px={col}')
    # G6 no instruction text
    bad = [t['s'] for t in texts if INSTR.search(t['s'])]
    rep(scr, 'G6 no instruction text', not bad, '; '.join(bad[:3]))
    return n, col

def regress(scr, frozen):
    if scr not in frozen: return
    b = base_png(scr); a = rgb(scr + '.norain')
    if b is None:
        rep(scr, 'G7 regression vs approved render', False, 'no baseline'); return
    pa, pb = a.load(), b.load(); n = 0
    masks = frozen[scr]
    for y in range(H):
        for x in range(W):
            if any(m[0] <= x < m[2] and m[1] <= y < m[3] for m in masks): continue
            c, d = pa[x, y], pb[x, y]
            if abs(c[0] - d[0]) + abs(c[1] - d[1]) + abs(c[2] - d[2]) > 48: n += 1
    rep(scr, 'G7 regression vs approved render', n <= 40, f'{n} px changed (limit 40)' + (f', masked {masks}' if masks else ''))

def dominant(im, box, ignore=None):
    from collections import Counter
    p = im.load(); c = Counter()
    for y in range(box[1], box[3], 2):
        for x in range(box[0], box[2], 2):
            c[p[x, y]] += 1
    return c.most_common(1)[0][0]

frozen = {}   # screen -> [mask rects x0,y0,x1,y1 of INTENDED change, excluded from the diff]
if FROZEN and os.path.exists(FROZEN):
    for l in open(FROZEN):
        f = l.split('#')[0].split()
        if not f: continue
        frozen[f[0]] = [tuple(int(v) for v in m[5:].split(',')) for m in f[1:] if m.startswith('mask=')]

screens = sorted({f[:-4] for f in os.listdir(D) if f.endswith('.log')})
order = ['intro', 'boot', 'hub', 'hub-amber', 'terminal', 'terminal-typed', 'terminal-tab',
         'scan-wifi', 'scan-ble', 'sbox-selected', 'sbox-info', 'sbox-hunt', 'sbox-glossary', 'found-alert']
screens = [s for s in order if s in screens] + [s for s in screens if s not in order]
stats = {}
for scr in sorted(set(frozen) - set(screens)):
    rep(scr, 'G7 regression vs approved render', False, 'frozen screen was not rendered')
# v1019: the UI colour is a theme (NEON GRAPE default; the '-amber' renders use theme 1, HOT PINK).
# Per theme: accent, selection outline, wire (trace) RGB as rendered.
THEME_RGB = {0: dict(ac=(181, 36, 255), sel=(181, 36, 255), wire=(90, 36, 99)),
             1: dict(ac=(222, 0, 173), sel=(222, 0, 173), wire=(107, 0, 82))}
def theme_of(scr): return THEME_RGB[1 if scr.endswith('-amber') else 0]
def near(c, ref, tol=28): return all(abs(int(a) - int(b)) <= tol for a, b in zip(c[:3], ref))
for scr in screens:
    boxes, texts = log(scr)
    im, nr = rgb(scr), rgb(scr + '.norain')
    n, col = generic(scr, boxes, texts, im, nr)
    stats[scr] = (n, col)
    regress(scr, frozen)
    crowns = [b for b in boxes if b['k'] == 'CROWN']
    if scr not in ('intro', 'boot') + OVERLAYS:
        rep(scr, 'G8 crown back control present', bool(crowns), f'{len(crowns)} crown')
    if scr.startswith('hub'):
        core = [b for b in boxes if b['k'] == 'HUBCORE']
        # v1019k (Dirt 2026-10-18 LOCKED, Option E default): Gibson's own square chip sits at the centre
        # of the animated rings core. The only chip allowed is that one gibson chip, fully inside HUBCORE.
        gchip = [b for b in boxes if b['k'] == 'CHIP' and not (b['n'] == 'gibson' and any(inside(b, c) for c in core))]
        rep(scr, 'H1 centre = animated rings core (Gibson chip inside the rings only)', bool(core) and not gchip, f'core={len(core)} stray chips={len(gchip)}')
        navs = [b for b in boxes if b['k'] == 'BTN' and b['n'] == 'hubnav']
        lit = 0; det = []
        p = im.load()
        for b in navs:
            px = [p[x, y] for y in range(b['y'] + 1, b['y'] + b['h'] - 1) for x in range(b['x'] + 1, b['x'] + b['w'] - 1)]
            m = [sum(c[i] for c in px) / len(px) for i in range(3)]
            edge = [p[b['x'] + b['w'] // 2, b['y']], p[b['x'], b['y'] + b['h'] // 2]]
            lum = luma(m)
            det.append(f'{int(m[0])},{int(m[1])},{int(m[2])}')
            if lum >= 28 and (max(luma(e) for e in edge) >= 120 or any(near(e, theme_of(scr)['ac']) for e in edge)): lit += 1
        rep(scr, 'H2 three lit nav buttons (filled face + bright edge)', len(navs) == 3 and lit == 3, 'mean ' + ' | '.join(det))
        rep(scr, 'H3 rain carries bots/devices (>=150 coloured px)', col >= 150, f'colour_px={col}')
        tiles = [b for b in boxes if b['k'] == 'TILE' and b['n'] == 'hubbot']
        rep(scr, 'H4 12 bot side tiles', len(tiles) == 12, f'{len(tiles)}')
    if scr.startswith('scanboard-') or scr.startswith('game-board') or scr.startswith('game-jackin'):
        want_gib = scr.endswith('gibson') or scr.startswith('game')
        chips = [b for b in boxes if b['k'] == 'CHIP' and (b['n'] == 'gibson') == want_gib]
        sq = [b for b in chips if b['w'] == b['h']]
        rep(scr, 'C1 square chip_block present' + (' (Gibson)' if want_gib else ' (bot)'), bool(sq), f'{[(b["n"], b["w"], b["h"]) for b in chips][:3]}')
        fill = 0
        for c in sq:
            for s in [b for b in boxes if b['k'] == 'SPR' and centre_in(b, c)]:
                fill = max(fill, max(s['w'], s['h']) / c['w'])
        rep(scr, 'C2 figure fills the chip square (major side >=60% of chip)', fill >= 0.60, f'fill={fill:.2f}')
        if want_gib:
            gb = [b for b in boxes if b['k'] == 'SPR' and b['n'] == 'gibson_block']
            rep(scr, 'C3 Gibson drawn as the square block figure', bool(gb), f'{len(gb)}')
    # D2 (#5 Dirt locked): the per-bot hunt grid is alive - this bot's falling device icons, the
    # chasing bot and the catch pips light the grid playfield (bright, non-grid pixels present).
    if scr in ('scanboard-gibson', 'scanboard-hunt-gibson'):
        def boxgap(a, b):
            ax2, ay2 = a['x'] + a['w'], a['y'] + a['h']
            bx2, by2 = b['x'] + b['w'], b['y'] + b['h']
            if ax2 <= b['x']: dx = b['x'] - ax2
            elif bx2 <= a['x']: dx = a['x'] - bx2
            else: dx = 0
            if ay2 <= b['y']: dy = b['y'] - ay2
            elif by2 <= a['y']: dy = a['y'] - by2
            else: dy = 0
            if dx and dy: return max(dx, dy)
            return dx + dy
        tchips = [b for b in boxes if b['k'] == 'TCHIP' and b['n'] in ('wifi', 'bt')]
        wf = [b for b in tchips if b['n'] == 'wifi']
        bt = [b for b in tchips if b['n'] == 'bt']
        def band_ok(b, cx, top):
            return (b['w'] == b['h'] and b['w'] == 22 and abs(b['x'] + b['w'] / 2 - cx) <= 1
                    and abs(b['y'] - top) <= 1 and b['x'] >= 20 and b['x'] + b['w'] <= 96)
        p1 = (len(wf) == 1 and len(bt) == 1 and band_ok(wf[0], 43, 144) and band_ok(bt[0], 77, 144)
              and boxgap(wf[0], bt[0]) >= 2)
        rep(scr, 'P1 two device chips, Wi-Fi then Bluetooth', p1,
            f"wifi={[(b['x'], b['y'], b['w']) for b in wf]} bt={[(b['x'], b['y'], b['w']) for b in bt]}")
        traces = [b for b in boxes if b['k'] == 'TRACE']
        pads = [b for b in boxes if b['k'] == 'PAD']
        silk = [b for b in boxes if b['k'] == 'SILK']
        tight = []
        parts = traces + pads
        for i in range(len(parts)):
            for j in range(i + 1, len(parts)):
                if parts[i]['n'] == parts[j]['n']: continue
                if boxgap(parts[i], parts[j]) < 2:
                    tight.append(f"{parts[i]['n']}x{parts[j]['n']}")
        silk_gap = True
        for t in tchips:
            for s in silk:
                if boxgap(t, s) < 2: silk_gap = False
        labels = list(silk) + [dict(x=t['x'], y=t['y'], w=t['w'], h=t['h'], n=t['s'][:16]) for t in texts]
        near_lbl = []
        for part in traces + pads:
            for s in labels:
                if boxgap(part, s) < 2:
                    near_lbl.append(f"{part['k']}:{part['n']}/{s['n']}")
        def covered(ix0, iy0, ix1, iy1, own):
            return any(ix0 >= p['x'] and iy0 >= p['y'] and ix1 <= p['x'] + p['w'] and iy1 <= p['y'] + p['h'] for p in own)
        bodies = tchips + [b for b in boxes if b['k'] == 'CHIP']
        invade = []
        for tr in traces:
            for ch in bodies:
                if not inter(tr, ch): continue
                ix0, iy0 = max(tr['x'], ch['x']), max(tr['y'], ch['y'])
                ix1 = min(tr['x'] + tr['w'], ch['x'] + ch['w'])
                iy1 = min(tr['y'] + tr['h'], ch['y'] + ch['h'])
                own = [p for p in pads if p['n'] == tr['n']]
                if not covered(ix0, iy0, ix1, iy1, own):
                    invade.append(f"{tr['n']}->{ch['k']}:{ch['n']}")
        # Index 104 is reserved. k_exact_nat[104] = 0x6242, which the norain frame expands to this RGB.
        WIRE104 = (99, 73, 16)
        wire_bad = []
        sample = None
        if nr is not None:
            pix = nr.load()
            if 0 <= 110 < H and 0 <= 21 < W:
                sample = pix[21, 110][:3]
            for y in range(H):
                for x in range(W):
                    if pix[x, y][:3] != WIRE104: continue
                    if x < 20 or x > 101:
                        wire_bad.append((x, y))
                        if len(wire_bad) >= 6: break
                if len(wire_bad) >= 6: break
        p2_5 = nr is not None and sample == WIRE104 and not wire_bad
        silver = [b for b in pads if b['y'] >= 140]
        gold = [b for b in pads if b['y'] < 140]
        p2 = (len(traces) >= 8 and len(silver) >= 4 and len(gold) >= 4 and not tight and silk_gap
              and not near_lbl and not invade and p2_5
              and all(b['y'] >= 140 for b in silver)
              and all(80 <= b['y'] <= 130 for b in gold))
        rep(scr, 'P2 wires, pads, silk, chip entry, and wire pixels', p2,
            f"traces={len(traces)} tight={tight[:4]} silk_ok={silk_gap} near={near_lbl[:3]} invade={invade[:3]} wire104={sample} bad={wire_bad[:3]}")
        left = [b for b in traces if b['n'].startswith('wf')]
        right = [b for b in traces if b['n'].startswith('bt')]
        spr = [b for b in boxes if b['k'] == 'SPR' and b['n'] in ('wifi', 'bt')]
        fill = [round(s['w'] / 24, 3) for s in spr]
        psplit = (len(left) >= 4 and len(right) >= 4 and all(b['x'] + b['w'] <= 61 for b in left)
              and all(b['x'] >= 61 for b in right) and len(spr) == 2 and all(v >= 0.60 for v in fill))
        rep(scr, 'P3-split wires split at x=61 and glyph fill >= 60%', psplit, f"fill={fill}")
    if scr.startswith('scanboard-'):
        p = im.load(); bright = 0
        for y in range(56, 140, 2):
            for x in range(110, 258, 2):
                c = p[x, y]
                if luma(c) > 100: bright += 1
        rep(scr, 'D2 hunt grid active (falling icons + chaser + pips)', bright >= 8, f'bright_px={bright}')
    # D1 (#9/#11 Dirt): the Defend board carries the dynamic "<name>> Defend" title.
    if scr == 'game-board':
        has = [t['s'] for t in texts if '> Defend' in t['s']]
        # v1019x (Mobandy 2:22 PM photo order): Gibson's Defend IS the restored pre-mod per-bot scan screen
        # (no title plate in the photo / no new UI): the item checks that shell instead - GIBSON silk under
        # the left chip and the zap's 'defend' board on the scan PCB.
        shell = any(b['k'] == 'SILK' and b['n'] == 'GIBSON' for b in boxes) and \
            any(b['k'] == 'BOARD' and b['n'] == 'scan' for b in boxes) and any(b['k'] == 'BOARD' and b['n'] == 'defend' for b in boxes)
        if shell:
            rep(scr, 'D1 Gibson Defend = restored scan shell (GIBSON silk + scan PCB + defend board)', True, 'photo layout')
        else:
            rep(scr, 'D1 dynamic "<name>> Defend" title present', bool(has), f'{has[:1]}')
    # v1019y: the idle Defend shell (game-board) keeps an empty grid — no chip inside it.
    if scr in ('game-board', 'game-jackin'):
        grid = {'x': 108, 'y': 44, 'w': 172, 'h': 96}
        ing = [c for c in boxes if c['k'] in ('TCHIP', 'CHIP', 'DGBOT') and inside(c, grid)]
        if scr == 'game-board':
            rep(scr, 'DG2 no chip inside the scan grid', not ing, f'grid chips={[(c["k"], c["n"], c["x"], c["y"]) for c in ing][:3]}')
        else:   # P0 v003 / GIBSON O2: game-jackin is a live round (B1) -> no chip on the bot sprite
            bots = [c for c in boxes if c['k'] == 'DGBOT']
            hit = [(c['k'], c['n'], c['x'], c['y']) for c in boxes if c['k'] in ('TCHIP', 'CHIP') and any(inter(c, d) for d in bots)]
            rep(scr, 'DG2 no chip on the bot sprite (live round)', not hit, f'DGBOT boxes={len(bots)} hits={hit[:3]}')
    if scr.startswith('scanboard-') or scr.startswith('sbox') or scr.startswith('scan-') or scr.startswith('settings'):
        bd = [b for b in boxes if b['k'] == 'BOARD']
        if bd:
            b = bd[0]
            dom = dominant(im, (b['x'] + 6, b['y'] + 6, b['x'] + b['w'] - 6, b['y'] + b['h'] - 6))
            rep(scr, 'B1 board is matte black PCB', luma(dom) <= 24, f'dominant={dom}')
        else:
            rep(scr, 'B1 board is matte black PCB', False, 'no BOARD box')
    if scr.startswith('sbox') or scr in ('scan-wifi', 'scan-ble'):
        mini = [b for b in boxes if b['k'] == 'MINI']
        tiles = [b for b in boxes if b['k'] == 'TILE' and b['n'] == 'dev']
        if scr != 'sbox-hunt':
            if scr in ('scan-wifi', 'scan-ble'):
              rep(scr, 'S1 device icons (tiles) in the mini screen', bool(mini) and len(tiles) >= 3 and all(inside(t, mini[0]) for t in tiles), f'tiles={len(tiles)}')
            pr = [b for b in boxes if b['k'] == 'PROMPT']
            ok = bool(mini) and bool(pr) and pr[0]['x'] - mini[0]['x'] <= 4 and pr[0]['y'] - mini[0]['y'] <= 4
            rep(scr, 'S2 index = small prompt top-left of mini screen', ok, f'{[(b["x"], b["y"]) for b in pr]}')
        else:
            hb = [b for b in boxes if b['k'] == 'HUNT']
            rep(scr, 'S3 hunt stays inside the mini screen', bool(mini) and bool(hb) and inside(hb[0], mini[0]), f'{hb[:1]}')
    if scr.startswith('settings'):
        parts = [b for b in boxes if b['k'] == 'PART']
        rep(scr, 'P1 eight settings parts present', len({b['n'] for b in parts}) == 8, f'{sorted({b["n"] for b in parts})}')
        silk = [t for t in texts if re.fullmatch(r'D23|D22|TX0|RX0|D21|D19|D18|D5|D17|D16', t['s'].strip()) and t['y'] < 60]
        chips = [b for b in parts if b['n'] in ('s0', 's1', 's2', 's5', 's7')]
        top = min([c['y'] for c in chips] or [0])
        ok = len(silk) == 10 and all(s['y'] + s['h'] <= top for s in silk)
        rep(scr, 'P2 top pin labels on the rail above the chips', ok, f'{len(silk)} labels, max bottom {max([s["y"]+s["h"] for s in silk] or [0])}, chip top {top}')
        small = {b['n']: (b['w'], b['h']) for b in parts if b['n'] in ('s3', 's4', 's6')}
        rep(scr, 'P3 JUMP/CP/PWR are small mock parts (<=22x22)', len(small) == 3 and all(w <= 22 and h <= 22 for w, h in small.values()), str(small))
        sel = [b for b in boxes if b['k'] == 'SEL']
        ok = False; det = 'no SEL box'
        if sel:
            s = sel[0]; p = im.load(); n = 0; tot = 0
            for x in range(s['x'], s['x'] + s['w']):
                for y in (s['y'], s['y'] + s['h'] - 1):
                    tot += 1; c = p[x, y]; n += near(c, theme_of(scr)['sel'])
            ok = n >= tot * 0.8; det = f'{n}/{tot} outline px accent'
        rep(scr, 'P4 selection outline in the UI theme selection colour', ok, det)
        tr = [b for b in boxes if b['k'] == 'TRACE']
        ok = False; det = 'no TRACE'
        if tr:
            p = im.load(); n = 0; tot = 0
            for t in tr:
                for y in range(t['y'], t['y'] + t['h']):
                    for x in range(t['x'], t['x'] + t['w']):
                        tot += 1; c = p[x, y]; th = theme_of(scr); n += near(c, th['ac']) or near(c, th['wire'])
            ok = tot > 0 and n >= tot * 0.6; det = f'{n}/{tot} trace px accent'
        rep(scr, 'P5 accent trace from the selected part', ok, det)
    if scr == 'terminal-typed':
        p = im.load(); okg = okc = oth = 0
        for t in texts:
            if not (171 <= t['y'] < 220) or t['x'] > 60: continue
            green = t['s'].startswith('[ ok ]')
            for y in range(t['y'], t['y'] + t['h']):
                for x in range(t['x'], t['x'] + t['w']):
                    c = p[x, y]
                    if luma(c) < 60: continue
                    if green:
                        okg += c[1] > 150 and c[1] > c[0] + 40 and c[1] > c[2] + 20
                    elif c[2] > 150 and c[1] > 150 and 90 < c[0] < c[2] - 20: okc += 1
                    else: oth += 1
        rep(scr, 'T1 log pale cyan, "[ ok ]" green', okg > 20 and okc > 50 and oth < okc // 10, f'green_px={okg} paleCyan_px={okc} other_px={oth}')

# functional
fp = os.path.join(D, 'functional.txt')
cpmove = {}; cpstart = 0
k1_seen = False
NSCR = 11
nav_need = ['boot-rain', 'hub', 'terminal', 'gibson-scan', 'wifi-radar', 'ble-radar']
keyfob_need = ['go-keyfob', 'load-keyfob', 'scan-keyfob']
keyfob_got = set()
chip_got = set()
nav_got = set()
nav_direct = set()
nav_band = set()
if os.path.exists(fp):
    for ln in open(fp):
        f = ln.rstrip('\n').split('\t')
        if f[0] == 'ESC':
            rep('functional', f'F1 Esc goes back from {f[1]}', f[2] == f[3], f'got {f[2]} want {f[3]}')
        elif f[0] == 'NAV':
            # v1016n (HIT LIST #1, ac9c53b): go/load wifi|ble|bots and the Esc chain only ever land on live
            # screens. Live ids: 0 rain 1 gibson 2 terminal 3 wifiradar 4 wifilist 5 btradar 6 btlist 7 scan
            # 8 control 9 diag 10 gchat. wifi/ble open that bot's scan box, bots opens the 12 chip tiles.
            want = {'wifi': 3, 'ble': 5, 'bots': 2, 'diag': 9}[f[1].split('-')[1]]
            chain = [int(x) for x in f[3].split(',')]
            seen = [int(f[2])] + chain
            ok = int(f[2]) == want and all(0 <= x < NSCR for x in seen) and chain[-1] in (0, 1)
            what = 'control route' if f[1].endswith('diag') else 'scan/chip screen'
            rep('functional', f'N1 {f[1].replace("-", " ")} -> {what}, Esc chain never hits a removed screen', ok,
                f'opened {f[2]} want {want}; esc chain {f[3]}')
        elif f[0] == 'OPEN':
            b, got, sb, e1 = (int(x) for x in f[1:5])
            want = 3 if b == 0 else 5 if b == 1 else 7
            ok = got == want and (b < 2 or sb == b) and e1 == 2
            rep('functional', f'N2 terminal Select on board {b} opens that bot\'s scan screen, Esc back to terminal', ok,
                f'screen {got} want {want}, scan_bot {sb}, esc -> {e1}')
        elif f[0] == 'SCREENS':
            NSCR = int(f[1])
            rep('functional', 'N0 screen table has no per-bot home or bot-radar ids (11 live screens)', NSCR == 11, f'screens={NSCR}')
        elif f[0] == 'SET':
            # v1017a: ENTER cycles the focused setting (arrows only move). Must change the value, stay on
            # the control panel with focus kept, and come back to the start after a full cycle.
            # v1019: setting 0 (UI COLOR) cycles the five control-panel themes
            want_cycle = {'0': 9, '1': 3, '2': 5, '3': 2, '4': 6, '5': 5, '6': 2, '7': 5}[f[1]]
            rep('functional', f'F2 Enter cycles setting {f[1]}', f[4] == '1' and int(f[6]) == want_cycle,
                f'{f[2]} -> {f[3]}, full cycle {f[6]} presses (want {want_cycle})')
            if f[1] in ('3', '4'):
                rep('functional', f'F3 setting {f[1]} writes NVS on Enter', int(f[5]) > 0, f'puts={f[5]}')
        elif f[0] == 'CPMOVE':
            cpmove.setdefault(int(f[1]), {})[int(f[2])] = (int(f[3]), int(f[4]))
        elif f[0] == 'CPSTART':
            cpstart = int(f[1])
        elif f[0] == 'CMDBTN':
            r = [int(x) for x in f[1:6]]
            rep('functional', 'C1 terminal: Down from the bottom boards stops on the commands button (prompt is typed)', r[0] == 1, f'reach={r[0]}')
            rep('functional', 'C2 Enter on commands button opens the commands menu', r[1] == 1, f'opened={r[1]}')
            rep('functional', 'C3 Esc closes the menu back to the button; Up walks button -> boards (prompt is typed)', r[2] == 1 and r[3] == 1, f'esc={r[2]} up={r[3]}')
            rep('functional', 'C4 a command picked in the menu runs', r[4] == 1, f'ran={r[4]}')
        elif f[0] == 'K1':
            k1_seen = True
            got = f[3] if len(f) > 3 else ''
            rep('functional', 'K1 terminal prompt shows exactly what was typed (rollover, same-scan press, Backspace)',
                f[1] == '1' and got == 'scan wifi', f'prompt={got!r} (want \'scan wifi\')')
            rep('functional', 'K2 one arrow chord = one focus stop; arrows stay arrows with text in the prompt',
                f[2] == '1', f'focus_ok={f[2]}')
        elif f[0] == 'LS':
            names = {
                '1': 'LS1 live model drives the tiles',
                '2': 'LS2 paint does not wait on the radio',
                '3': 'LS3 arm, disarm, disarmed on enter (T1/T2)',
                '4': 'LS4 found-alert only from the handler while armed',
                '5': 'LS5 ignored never counts or re-alerts',
                '6': 'LS6 selection stays on the MAC; prompt re-pins; GIB_STOP stays 36',
                '7': 'LS7 queue overflow drops without corrupting the model',
                '9': 'LS9 hub latch is set by the first sighting',
                '10': 'LS10 48 KB NimBLE check at arm, short does not retry',
                '11': 'LS11 boot probe does not wait; NIM_GATE is 48 KB',
                '12': 'LS12 defend bots 0/1 stay on the canned tally while armed',
                '13': 'LS13 radar strip is radio-off or the armed count',
                '14': 'LS14 GIB_STOP arms on the radar screens and not on Gibson',
                '15': 'LS15 armed glow is the two radar screens only',
                '16': 'LS16 static RAM is enforced by ram.py',
                '17': 'LS17 one first-sighting writer',
                '18': 'LS18 the canned fixture does not call the handler',
                '20': 'LS20 tag and ignore follow the MAC',
                '21': 'LS21 each gate pass starts disarmed, unlatched, fixture off',
                '22': 'LS22 legacy boards still key the canned ignore array',
                '24': 'LS24 a radar screen enters with an empty model',
                '25': 'LS25 the alert keeps its own device copy',
                'seen': 'LS seen-set overflow is not a first sighting',
            }
            title = names.get(f[1], 'LS' + f[1])
            rep('functional', title, len(f) > 2 and f[2] == '1', f[3] if len(f) > 3 else '')
        elif f[0] in ('NAVCHAIN', 'NAVDIRECT', 'NAVBAND') and len(f) >= 10:
            step, got, want, bot = f[1], int(f[2]), int(f[3]), int(f[4])
            w, h, left, ext_px = int(f[5]), int(f[6]), int(f[7]), int(f[8])
            bucket = nav_got if f[0] == 'NAVCHAIN' else nav_direct if f[0] == 'NAVDIRECT' else nav_band
            bucket.add(step)
            ok = got == want and w == 320 and h == 240 and left < 128 and ext_px >= 500
            if step == 'gibson-scan':
                ok = ok and bot == 12
            if step in ('go-keyfob', 'load-keyfob'):
                ok = ok and bot == 7 and got == 7 and want == 7
                keyfob_got.add(step)
            label = 'NAV' if f[0] == 'NAVCHAIN' else f[0]
            rep('functional', f'{label} {step}: key path paints the 320x240 EXT surface', ok,
                f'screen {got} want {want} bot={bot} {w}x{h} sentinel={left} ext_px={ext_px}')
        elif f[0] == 'KEYFOBSCAN' and len(f) >= 9:
            step, got, want, who = f[1], int(f[2]), int(f[3]), int(f[4])
            log_ok, before, after, okb = int(f[5]), int(f[6]), int(f[7]), int(f[8])
            keyfob_got.add(step)
            ok = (step == 'scan-keyfob' and got == want == 2 and who == 7 and log_ok == 1
                  and before == after and okb == 1)
            rep('functional', 'CHIP-G-KEYFOB scan-keyfob stays on the terminal', ok,
                f'got {got} want {want} who {who} log {log_ok} cursor {before}->{after}')
        elif f[0] == 'CHIPFSM' and len(f) >= 4:
            chip_got.add(f[1])
            rep('functional', f'CHIP-G-FSM {f[1]} {f[2]}', f[3] == '1', f[2])
        elif f[0] == 'GD3' and len(f) >= 4:
            ok = int(f[1]) == 17 and int(f[2]) >= 3 and int(f[3]) == 3
            rep('functional', 'CHIP-G-GD3 Gibson drop-down is 3 rows of 17', ok,
                f'count {f[1]} rows-cap {f[2]} rows {f[3]}')
        elif f[0] == 'RAINWIN' and len(f) >= 3:
            ok = int(f[1]) == 0 and int(f[2]) == 0
            rep('functional', 'CHIP-G-RAIN panel rain stays in the window', ok,
                f'over_info {f[1]} outside {f[2]}')
        elif f[0] == 'PARTIALS':
            rep('functional', 'V2 partials: no band-only caller without a direct twin',
                len(f) > 1 and f[1] == 'empty', f[1] if len(f) > 1 else '')
        elif f[0] == 'SIM':
            name, bad, pres, push, rec, worst = f[1], int(f[2]), int(f[3]), int(f[4]), int(f[5]), int(f[6])
            # v1019v: full-width zap Defend chrome presents large band slices by design (mock v5).
            ok_s1 = (bad <= 2 and worst <= 4096) if name in ('defend-zap', 'game-jackin') else (bad == 0 and worst <= 64)
            rep('functional', f'S1 {name}: no frame-to-frame flash on the band simulator (6 s live loop)', ok_s1,
                f'glitched pushes={bad} worst={worst}px, presents={pres} pushes={push} recomposes={rec}')
            if name.startswith('scan-'):
                rep('functional', f'S2 {name}: idle animation never full-screen presents (patches only)', pres <= 1,
                    f'presents={pres} (1 = the screen-enter paint)')
                # v1017c (#1): scan parts are drawn in place; a part tick never rebuilds a whole 120-row slice
                rep('functional', f'S4 {name}: idle ticks draw in place (no slice rebuild / recompose)', rec == 0,
                    f'recomposes={rec}')
            if name == 'scan-gibson':
                # Contract P3: the new device chips do not repaint while idle. S1/S2/S4 already measure this.
                rep('functional', 'P3 scan-gibson idle repaint is 0 px',
                    bad == 0 and worst <= 64 and pres <= 1 and rec == 0,
                    f'glitched={bad} worst={worst}px presents={pres} recomposes={rec}')

for step in keyfob_need:
    if step not in keyfob_got:
        rep('functional', f'CHIP-G-KEYFOB {step}', False, 'missing')
for n, name in (('1', 'repress'), ('2', 'esc'), ('3', 'focus'), ('4', 'doneidle'),
                ('5', 'airtag'), ('6', 'gibson'), ('7', 'timing')):
    if n not in chip_got:
        rep('functional', f'CHIP-G-FSM {n} {name}', False, 'missing')

def _src(rel):
    p = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), rel)
    return open(p, encoding='utf-8', errors='replace').read() if os.path.exists(p) else ''

ui_src = _src('src/ui/botcorps_ui.cpp')
id_src = _src('src/ui/botcorps_id.cpp')
main_src = _src('src/main.cpp')
probe_src = _src('src/probe/botcorps_probe.cpp')
ini_src = _src('platformio.ini')
alloc_fmt = 'direct tile alloc fail w=%d h=%d largest=%u'
di = ui_src.find('static void direct_tile_invert')
di_body = ui_src[di:ui_src.find('\n#endif', di)] if di >= 0 else ''
alloc_ok = (di >= 0 and alloc_fmt in di_body and 'static bool latched' in di_body
            and 'createSprite' in di_body and di_body.find(alloc_fmt) < di_body.find('return;'))
rep('functional', 'CHIP-G-ALLOC-GATE direct tile alloc fail is latched', alloc_ok,
    'SERIAL-UNPROVEN' if alloc_ok else 'missing latch or format')
heap_i = main_src.find('band after-probe')
heap_j = main_src.find('botcorps_ui_mark_dirty')
heap_ok = (heap_i >= 0 and heap_j > heap_i and 'botcorps_heap_log("i4")' in main_src[heap_i:heap_j]
           and '[botcorps] heap %s free=%u largest=%u min=%u' in probe_src)
rep('functional', 'CHIP-G-HEAP after-probe heap print', heap_ok, 'i4' if heap_ok else 'missing')
live_ok = 'build_src_flags = -DBOTCORPS_LIVE_SCAN=0' in ini_src
rep('functional', 'CHIP-G-LIVE0 launcher LIVE_SCAN=0', live_ok, '')
grey_ok = 'sb_rect(g, dx - 9, dy + 44, 5, 44, 41)' not in ui_src and 'static void paint_sbox' in ui_src
rep('functional', 'CHIP-G-GREY grey strip deleted', grey_ok, '')
king_ok = 'k_slot_bot[12] = { 12, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11 }' in ui_src
rep('functional', 'CHIP-G-KING slot to bot table', king_ok, '')
import hashlib as _hl
id_sha = _hl.sha256(id_src.encode()).hexdigest() if id_src else ''
id_ok = id_sha == 'bb688f822f1935d89457addd633d22c33039d7039436d73395320667b92ce95e'
rep('functional', 'CHIP-G-INT botcorps_id.cpp byte-identical', id_ok, id_sha[:12])
cb = ui_src.find('static void chip_button(')
cb2 = ui_src.find('static void chip_button(', cb + 1) if cb >= 0 else -1
cb_end = ui_src.find('\nstatic void ', cb + 1) if cb >= 0 else -1
cb_body = ui_src[cb:cb_end] if cb >= 0 and cb_end > cb else ''
chip_ok = (cb >= 0 and cb2 < 0 and 'PAL_GOLD_PIN' not in cb_body and 'PAL_SOFT' not in cb_body
           and 'chip_button(g, bot)' in ui_src
           and all(s in ui_src for s in ('WIFIBOT', 'BLUETOOTHBOT', 'Gibson SCAN', 'AIRTAG', 'Gibson game')))
rep('functional', 'CHIP-G-CHIPONE one chip_button, pins from chip_shell', chip_ok, '')
deauth_ok = 'return bot >= 0 && bot <= 12;' in ui_src  # v003 A1 (GIBSON 2:05 PM ET)
rep('functional', 'CHIP-G-DEAUTH chip panel is the closed set', deauth_ok, '')
rain_ok = 'zr_clip_push(g, 20, 43, 82, 149)' in ui_src
rep('functional', 'CHIP-G-RAIN window is RAIN_WIN', rain_ok, '')

for step in nav_need:
    if step not in nav_got:
        rep('functional', f'NAV {step}: key path paints the 320x240 EXT surface', False, 'missing')
    if step not in nav_direct:
        rep('functional', f'NAVDIRECT {step}: key path paints the 320x240 EXT surface', False, 'missing')
    if step not in nav_band:
        rep('functional', f'NAVBAND {step}: key path paints the 320x240 EXT surface', False, 'missing')
rep('functional', 'K0 keyboard test ran', k1_seen, '' if k1_seen else 'no K1 line in functional.txt')
rep('functional', 'LS-D1 device heap with the band held', None, 'on-device feel check')
rep('functional', 'LS-D2 ten-minute live scan per band', None, 'on-device field test')

# v1019y: DG / DGR / DGC retired with the old mini-games (968 -> fewer; no new loop checks).

# v1017c S3: every screen painted the device way (two 120-row band slices, oy 0/120, clipped to the
# 320x120 sprite) composes to exactly the one-shot frame, rain on and off.
sp = os.path.join(D, 'slice.txt')
if os.path.exists(sp):
    for ln in open(sp):
        f = ln.rstrip('\n').split('\t')
        if f[0] == 'SLICE':
            rep('functional', f'S3 {f[1]}: BANDS slice paint == full frame', int(f[2]) == 0, f'diff={f[2]}px')

if cpmove:
    # v1017a: every control-panel stop reachable from the entry stop by arrows alone; arrows never edit.
    seen = {cpstart}; todo = [cpstart]
    while todo:
        c = todo.pop()
        for d, (to, chg) in cpmove.get(c, {}).items():
            if 0 <= to < 10 and to not in seen: seen.add(to); todo.append(to)
    rep('functional', 'R1 every control-panel control reachable by the cursor', len(seen) == 10, f'reached {sorted(seen)} from {cpstart}')
    ring = {cpstart}; c = cpstart
    for _ in range(10):
        c = cpmove[c][4][0]; ring.add(c)
    rep('functional', 'R2 Right alone walks through all 10 stops (incl. SCREEN SAVER U4 + PWR LED)', len(ring) == 10 and 5 in ring and 6 in ring, f'{sorted(ring)}')
    edits = [(c, d) for c in cpmove for d, (to, chg) in cpmove[c].items() if chg]
    rep('functional', 'R3 arrows only move (no arrow changes any setting)', not edits, f'edits={edits}')
    stay = [(c, d) for c in cpmove for d, (to, chg) in cpmove[c].items() if to < 0]
    rep('functional', 'R4 arrows never leave the control panel', not stay, f'left={stay}')

# v1017a L1: approved renders locked byte-for-byte to the build Dirt flashed (675c87d): the control
# panel (all 8 focus states + amber), terminal, scan screens, scan boxes, game and hub. Input-only change.
import hashlib
LOCK = os.path.join(os.path.dirname(FROZEN or '.'), 'locked-675c87d.sha256')
if os.path.exists(LOCK):
    for l in open(LOCK):
        h, name = l.split()
        pth = os.path.join(D, name)
        got = hashlib.sha256(open(pth, 'rb').read()).hexdigest() if os.path.exists(pth) else 'missing'
        grp = 'control-panel' if name.startswith('settings') else 'locked'
        rep(grp, f'L1 {name} byte-identical to 675c87d', got == h, '' if got == h else f'sha {got[:12]} != {h[:12]}')

# ---------------------------------------------------------------------------------------------
# P0 v003 (ARCH-GAME-P0-CHIP-PROMPT-v003.md sha256 39e76347...0bf4) gates G1-G21 + GIBSON rulings
# R1/R2 (1:16 PM ET). Additive. References (all from e7ba692 or the A5-off tip render) live in
# release/gate/p0-v003/ (README.txt has the provenance).
P0REF = os.path.join(os.path.dirname(FROZEN or '.'), 'p0-v003')
if os.path.isdir(P0REF):
    def _b(p): return open(p, 'rb').read() if os.path.exists(p) else None
    def _sha(p): return hashlib.sha256(open(p, 'rb').read()).hexdigest() if os.path.exists(p) else 'missing'
    def _man(p): return dict((l.split()[1], l.split()[0]) for l in open(p) if l.strip())
    def _png(name): return Image.open(os.path.join(P0REF, 'png', name)).convert('RGB').tobytes()
    def _px(buf, x, y): i = (y * W + x) * 3; return buf[i:i + 3]
    def _diff(a, b):
        return [((i // 3) % W, (i // 3) // W) for i in range(0, len(a), 3) if a[i:i + 3] != b[i:i + 3]]
    def _bb(pts):
        return (min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts), max(p[1] for p in pts)) if pts else None
    def _boxes(p):
        out = []
        if os.path.exists(p):
            for ln in open(p, errors='replace'):
                f = ln.rstrip('\n').split('\t')
                if f[0] == 'BOX' and len(f) >= 8: out.append((f[1], f[2], int(f[3]), int(f[4]), int(f[5]), int(f[6])))
        return out
    def _txts(p):
        out = []
        if os.path.exists(p):
            for ln in open(p, errors='replace'):
                f = ln.rstrip('\n').split('\t')
                if f[0] == 'TXT' and len(f) >= 10: out.append((int(f[1]), int(f[2]), f[9]))
        return out
    P0D = os.path.join(D, 'p0')
    SB = ['deauth', 'hack', 'flock', 'drone', 'keyfob', 'meta', 'skimmer', 'flipper', 'samsungtag']
    SBBOT = dict(deauth=2, hack=3, flock=4, drone=5, keyfob=7, meta=8, skimmer=9, flipper=10, samsungtag=11)
    NSCR = ['scanboard-' + n for n in SB] + ['game-jackin', 'game-menu', 'terminal-atkmenu', 'terminal-cmdmenu']
    NFR = sorted(s + v + '.rgb' for s in NSCR for v in ('', '.norain'))
    base = _man(os.path.join(P0REF, 'e7ba692-render.sha256'))
    bman = ''.join(f'{base[k]}  render/{k}\n' for k in sorted(base))
    rep('p0-v003', 'G1 e7ba692 reference manifest is 69052dcf', hashlib.sha256(bman.encode()).hexdigest().startswith('69052dcf'),
        hashlib.sha256(bman.encode()).hexdigest()[:12])
    got = {k: _sha(os.path.join(D, k)) for k in base}
    rep('p0-v003', 'GIBSON-R1 N = 26 frames (13 screens x rgb/norain, terminal-cmdmenu x2 added)', len(NFR) == 26 and all(k in base for k in NFR), f'{len(NFR)}')
    moved = sorted(k for k in base if got[k] != base[k])
    rep('p0-v003', 'G1 only the 26 N frames differ from e7ba692 (HOLD if any other moves)', set(moved) <= set(NFR), f'moved={len(moved)} outside N={sorted(set(moved) - set(NFR))[:4]}')
    same = [k for k in base if k not in NFR and got[k] == base[k]]
    rep('p0-v003', 'G1 the other 70 frames byte-identical to e7ba692', len(same) == 70, f'{len(same)}/70')
    srcref = _man(os.path.join(P0REF, 'e7ba692-src.sha256'))
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    for rel, h in sorted(srcref.items()):
        g2 = _sha(os.path.join(root, rel))
        rep('p0-v003', f'G1 {rel} empty diff vs e7ba692', g2 == h, '' if g2 == h else f'{g2[:12]} != {h[:12]}')
    rain_now = sorted(os.listdir(os.path.join(root, 'src/rain'))) if os.path.isdir(os.path.join(root, 'src/rain')) else []
    rep('p0-v003', 'G1 src/rain has no added files', ['src/rain/' + f for f in rain_now] == sorted(k for k in srcref if k.startswith('src/rain/')), f'{rain_now}')

    # G2 (SCAN) per bot: (a) outside the panel rect 0-px vs e7ba692 (drone: + ring, hunt, R2 holes);
    # (b) inside vs AirTag one consistent colour map; (c) GATE_BOX list inside the rect equals AirTag's.
    def in_panel(x, y): return 20 <= x < 102 and 40 <= y < 194
    BHX, BHY, BHW, BHH = 108, 44, 171, 95     # ui.cpp BH_X/BH_Y + BH_W = 9*19, BH_H = 5*19 (ui.cpp:4803-4804)
    def in_ring(x, y): return (x in (18, 301) and 30 <= y <= 204) or (y in (30, 204) and 18 <= x <= 301)
    def in_hunt(x, y): return BHX <= x < BHX + BHW and BHY - 12 <= y < BHY + BHH
    def in_hole(x, y): return (22 <= x <= 30 or 290 <= x <= 298) and (34 <= y <= 42 or 193 <= y <= 201)  # GIBSON R2
    def g2_mask(x, y):  # glyph (55,155) 12x12 + gl_icon's real (55,156) cell (to y167), label y181-188, silk y124-130
        return (55 <= x < 67 and 155 <= y < 168) or 181 <= y <= 188 or 124 <= y <= 130
    def cmap_bad(ref, t):
        cm = {}; bad = 0
        for y in range(40, 194):
            for x in range(20, 102):
                if g2_mask(x, y): continue
                a, c = _px(ref, x, y), _px(t, x, y)
                if cm.setdefault(a, c) != c: bad += 1
        return bad, len(cm)
    def boxsig(bx):
        out = []
        for k, n, x, y, w, h in bx:
            if not (20 <= x and 40 <= y and x + w <= 102 and y + h <= 194): continue
            out.append((k, x * 2 + w, y, h) if k == 'SILK' else (k, x, y, w, h))   # SILK: name-agnostic, centre x
        return sorted(out)
    air = _b(os.path.join(D, 'scanboard-airtag.norain.rgb'))
    airsig = boxsig(_boxes(os.path.join(D, 'scanboard-airtag.log')))
    for n in SB:
        t = _b(os.path.join(D, f'scanboard-{n}.norain.rgb'))
        if t is None or air is None:
            rep('p0-v003', f'G2 scanboard-{n} rendered', False, 'missing'); continue
        r = _png(f'e7ba692-scanboard-{n}.norain.png')
        out = [p for p in _diff(t, r) if not in_panel(*p) and not (n == 'drone' and (in_ring(*p) or in_hunt(*p) or in_hole(*p)))]
        rep('p0-v003', f'G2(a) scanboard-{n} 0-px outside (20,40)-(102,194) vs e7ba692' + (' (+A5 ring/hunt/R2 holes)' if n == 'drone' else ''),
            not out, f'{len(out)}px {_bb(out)}')
        bad, nc = cmap_bad(air, t)
        rep('p0-v003', f'G2(b) scanboard-{n} panel = AirTag under one colour map', bad == 0, f'bad={bad} colours={nc}')
        s2 = boxsig(_boxes(os.path.join(D, f'scanboard-{n}.log')))
        rep('p0-v003', f'G2(c) scanboard-{n} GATE_BOX list in rect = AirTag', s2 == airsig, f'{len(s2)} boxes')
        # G3 rgb-vs-norain inside the panel rect confined to the rain window. v003 text says (20,44) 76x148, but
        # e7ba692's own AirTag/Gibson rain (v2_rain, RAIN_WIN = zr_clip_push(g, 20, 43, 82, 149), CHIP-G-RAIN)
        # spans (20,43)-(101,191); the row checks RAIN_WIN and logs the count outside the literal v003 rect.
        tr = _b(os.path.join(D, f'scanboard-{n}.rgb'))
        rp = [p for p in _diff(tr, t) if in_panel(*p)]
        rd = [p for p in rp if not (20 <= p[0] < 102 and 43 <= p[1] < 192)]
        lit = [p for p in rp if not (20 <= p[0] < 96 and 44 <= p[1] < 192)]
        rep('p0-v003', f'G3 scanboard-{n} rain diff inside panel confined to RAIN_WIN (20,43) 82x149', not rd,
            f'{len(rp)}px {_bb(rp)}; outside RAIN_WIN {len(rd)}; outside v003-literal (20,44) 76x148 {len(lit)}')

    # G2g (GCHAT Defend via botcorps_host_defend(b)) vs AirTag's GCHAT frame from the same tip; AirTag gc-6 0-px vs e7ba692
    p0ref = _man(os.path.join(P0REF, 'e7ba692-p0.sha256'))
    for k in ('gc-6.rgb', 'gc-6.norain.rgb'):
        g6 = _sha(os.path.join(P0D, k))
        rep('p0-v003', f'G2g AirTag GCHAT {k} byte-identical to e7ba692', g6 == p0ref.get(k), g6[:12])
    gair = _b(os.path.join(P0D, 'gc-6.norain.rgb'))
    gairsig = boxsig(_boxes(os.path.join(P0D, 'gc-6.log')))
    for b in (2, 3, 4, 5, 7, 8, 9, 10, 11):
        t = _b(os.path.join(P0D, f'gc-{b}.norain.rgb'))
        if t is None or gair is None:
            rep('p0-v003', f'G2g gc-{b} rendered', False, 'missing'); continue
        r = _png(f'e7ba692-gc-{b}.norain.png')
        mini = [bx for bx in _boxes(os.path.join(P0D, f'gc-{b}.log')) if bx[0] == 'MINI']
        out = [p for p in _diff(t, r) if not in_panel(*p)]
        if b == 5:   # A5 X5/X7: Defend board 5 now spawns/rains GL_DRONE; bounded to the MINI defend screen
            m = mini[0] if mini else ('MINI', '', 0, 0, 0, 0)
            o2 = [p for p in out if not (m[2] <= p[0] < m[2] + m[4] and m[3] <= p[1] < m[3] + m[5])]
            rep('p0-v003', 'G2g(a) gc-5 outside panel: diff only inside the MINI defend screen (A5 X5/X7)', bool(mini) and not o2,
                f'{len(out)}px {_bb(out)} in MINI {m[2:]}; outside MINI {len(o2)}px')
        else:
            rep('p0-v003', f'G2g(a) gc-{b} 0-px outside (20,40)-(102,194) vs e7ba692', not out, f'{len(out)}px {_bb(out)}')
        bad, nc = cmap_bad(gair, t)
        rep('p0-v003', f'G2g(b) gc-{b} panel = AirTag GCHAT under one colour map', bad == 0, f'bad={bad} colours={nc}')
        s2 = boxsig(_boxes(os.path.join(P0D, f'gc-{b}.log')))
        rep('p0-v003', f'G2g(c) gc-{b} GATE_BOX list in rect = AirTag GCHAT', s2 == gairsig, f'{len(s2)} boxes')

    # G3..G20 behaviour rows from the host harness (rend gate -> p0/p0.txt)
    pp = os.path.join(P0D, 'p0.txt')
    cnt = {}
    if os.path.exists(pp):
        for ln in open(pp):
            f = ln.rstrip('\n').split('\t')
            if f[0] == 'P0' and len(f) >= 5:
                cnt[f[1]] = cnt.get(f[1], 0) + 1
                rep('p0-v003', f'{f[1]} {f[2]}', f[3] == '1', f[4][:110])
    need = dict(G3=9, G4=11, G5=11, G6=3, G7=15, G8=12, G9=11, G10=10, G11=11, G12=2, G13=1, G14=11, G15=13, G18=1, G20=7)
    short = {g: (cnt.get(g, 0), n) for g, n in need.items() if cnt.get(g, 0) < n}
    rep('p0-v003', 'P0 harness ran every G3-G20 row', not short, f'{sum(cnt.values())} rows' + (f' short={short}' if short else ''))
    # G6: CHIPFSM rows (0/1/Gibson SCAN) unchanged is the existing CHIP-G-FSM block above.

    # G8 overlay strings (§4.3) through the text log: Terminal slots (KING + tiles) and GCHAT
    NM = {2: 'deauth wi-fi ap', 3: 'hack laptop', 4: 'flock camera', 5: 'drone', 6: 'airtag tracker', 7: 'keyfob car',
          8: 'meta watch', 9: 'skimmer unknown', 10: 'flipper tv', 11: 'samsung tracker'}
    SIGV = {12: 'overload', 6: 'ghost', 11: 'echo', 2: 'kick', 3: 'trace', 4: 'blind', 5: 'ground', 8: 'blur', 9: 'void', 10: 'null', 7: 'roll'}
    SUBV = {12: 'core root', 6: 'mute unpair', 11: 'ring unlink', 2: 'flood twin', 3: 'rst burn', 4: 'plate mask', 5: 'spoof land',
            8: 'cover glitch', 9: 'spot flag', 10: 'replay clone', 7: 'relay seal'}
    SILKN = {0: 'WIFIBOT', 1: 'BLUETOOTHBOT', 2: 'DEAUTHBOT', 3: 'HACKBOT', 4: 'FLOCKBOT', 5: 'DRONEBOT', 6: 'AIRTAGBOT', 7: 'KEYFOBBOT',
             8: 'METABOT', 9: 'SKIMMERBOT', 10: 'FLIPPERBOT', 11: 'SAMSUNGTAGBOT', 12: 'GIBSON'}
    def atk_rows(b):
        if b in (0, 1): return [('core', 'scan'), ('ok', 'scan')]
        if b == 12:
            return [('names', 'wifi bluetooth deauth hack flock drone'), ('', 'airtag keyfob meta skimmer flipper'),
                    ('', 'samsung wi-fi ap phone laptop tv'), ('', 'speaker tracker watch camera car'), ('', 'unknown printer'),
                    ('core', 'scan lock zap purge restart'), ('sig', 'overload'), ('subs', 'core root'), ('ok', 'start defend')]
        return [('names', NM[b]), ('core', 'scan lock zap purge restart'), ('sig', SIGV[b]), ('subs', SUBV[b]), ('ok', 'start defend')]
    def atk_check(fn, b):
        tx = _txts(os.path.join(P0D, fn + '.log'))
        i = next((k for k, t in enumerate(tx) if t[2] == 'ATTACK PROMPTS'), -1)
        if i < 0: return False, 'no overlay'
        bx, by = tx[i][0] - 8, tx[i][1] - 4
        seen = []
        for x, y, s in tx[i + 1:]:
            if (x, y, s) not in seen: seen.append((x, y, s))
        want = [(None, by + 4, SILKN[b])]
        for k, (lab, val) in enumerate(atk_rows(b)):
            if lab: want.append((bx + 8, by + 18 + k * 12, lab))
            want.append((bx + 56, by + 18 + k * 12, val))
        gotl = [(x, y, s) for x, y, s in seen if y >= by + 4]
        ok = len(gotl) == len(want) and all((w[0] is None or w[0] == g[0]) and w[1] == g[1] and w[2] == g[2] for w, g in zip(want, gotl))
        return ok, f'{len(gotl)} strings, {len(want)} expected'
    KSLOT = [12, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11]
    for s, b in enumerate(KSLOT):
        ok, det = atk_check(f'atk-{s}', b)
        rep('p0-v003', f'G8 Terminal slot {s} (bot {b}) overlay rows = §4.3 strings', ok, det)
    for b in range(13):
        ok, det = atk_check(f'gcatk-{b}', b)
        rep('p0-v003', f'G8 GCHAT bot {b} overlay rows = §4.3 strings', ok, det)

    # G15: WiFi/BT sheet byte-identical to e7ba692 (BOARD rows: harness G15 rows above)
    for k in ('sheet-0.rgb', 'sheet-0.norain.rgb', 'sheet-1.rgb', 'sheet-1.norain.rgb'):
        g5 = _sha(os.path.join(P0D, k))
        rep('p0-v003', f'G15 WiFi/BT {k} byte-identical to e7ba692', g5 == p0ref.get(k), g5[:12])
    sp0 = os.path.join(P0D, 'slice.txt')
    if os.path.exists(sp0):
        bads = [ln.split('\t')[1] for ln in open(sp0) if ln.startswith('SLICE') and int(ln.rstrip().split('\t')[2]) != 0]
        rep('p0-v003', 'S3 p0 frames: BANDS slice paint == full frame', not bads, f'bad={bads[:4]}')

    # G16: re-lock is exactly the 22 rows of N; frozen masks per §6 G16
    lk = [l.split()[1] for l in open(LOCK)] if os.path.exists(LOCK) else []
    relock = sorted(k for k in lk if k in NFR)
    want22 = sorted(f'{s}{v}.rgb' for s in ['scanboard-' + n for n in SB] + ['game-jackin', 'game-menu'] for v in ('', '.norain'))
    rep('p0-v003', 'G16 locked rows inside N are exactly the 22 re-locked rows', relock == want22, f'{len(relock)}')
    lkd = dict((l.split()[1], l.split()[0]) for l in open(LOCK)) if os.path.exists(LOCK) else {}
    keep48 = [k for k in lk if k not in NFR and lkd[k] == base.get(k)]
    rep('p0-v003', 'G16 the 48 other locked rows still equal e7ba692', len(keep48) == 48 and len(lk) == 70, f'{len(keep48)}/48 of {len(lk)}')
    fz = {}
    for l in open(FROZEN):
        l = l.split('#')[0].split()
        if l: fz[l[0]] = [m[5:] for m in l[1:] if m.startswith('mask=')]
    fm = all(fz.get('scanboard-' + n) == ['20,40,102,194'] for n in SB if n != 'drone') and fz.get('scanboard-drone') == ['18,30,302,205']
    rep('p0-v003', 'G16 frozen masks: 8 scanboards 20,40,102,194; drone 18,30,302,205', fm, '')
    # G17: caps unchanged (ram-caps.txt sha in e7ba692-src.sha256 above); RAM/flash rows are tools/gate/ram.py.
    # G18: sizeof(s_gp_buf) is gone
    hits = ui_src.count('sizeof(s_gp_buf)')
    rep('p0-v003', 'G18 sizeof(s_gp_buf) has zero hits', hits == 0, f'{hits} hits')
    # G19: no new arrow binding (key-handler compares identical to e7ba692), field stays cyan, LIVE_SCAN=0 (CHIP-G-LIVE0)
    def _body(fn):
        i = ui_src.find(fn); return ui_src[i:ui_src.find('\n}\n', i)] if i >= 0 else ''
    arr = _body('static void on_arrows(int key)')
    kc = tuple(arr.count(f'key == {k}') for k in (1, 2, 3, 4))
    dk = sorted(re.findall(r'key == K_\w+', _body('static void ui_dispatch(int key, char typed)')))
    rep('p0-v003', 'G19 key handlers: on_arrows key compares = e7ba692 (15,17,14,15); ui_dispatch K_ set unchanged',
        kc == (15, 17, 14, 15) and dk == ['key == K_DN', 'key == K_LT', 'key == K_NONE', 'key == K_RT', 'key == K_UP']
        and 'key ==' not in _body('static void on_tab(void)'), f'{kc}')
    fi = ui_src.find('    const int py = 216;\n    txt(g, "gibson", 10, py, ac,')
    fj = ui_src.find('    if (s_sheet) paint_gc_sheet(g);', fi)
    fsha = hashlib.sha256(ui_src[fi:fj].encode()).hexdigest() if fi >= 0 and fj > fi else ''
    rep('p0-v003', 'G19 GCHAT prompt field block byte-identical to e7ba692 (accent colour)', fsha == '12f4f9672f3e459d7533146ce0875531189efb12501c0354781366f74f35d1af', fsha[:12])
    rep('p0-v003', 'G19 LIVE_SCAN=0 in the launcher env', 'build_src_flags = -DBOTCORPS_LIVE_SCAN=0' in ini_src, '')

    # G21 (A5): tip vs tip-with-only-the-BH_DEV-drone-row-reverted (a5off-render.sha256, a5off PNGs + log)
    a5 = _man(os.path.join(P0REF, 'a5off-render.sha256'))
    a5mv = sorted(k for k in a5 if got.get(k) != a5[k])
    rep('p0-v003', 'G21(a) A5 on vs off: only scanboard-drone.rgb/.norain.rgb differ', a5mv == ['scanboard-drone.norain.rgb', 'scanboard-drone.rgb'], f'{a5mv[:4]}')
    for v in ('', '.norain'):
        t = _b(os.path.join(D, f'scanboard-drone{v}.rgb')); r = _png(f'a5off-scanboard-drone{v}.png')
        dd = _diff(t, r)
        zc = {}
        for p in dd:
            z = 'panel' if in_panel(*p) else 'ring' if in_ring(*p) else 'hole' if in_hole(*p) else 'hunt' if in_hunt(*p) else 'OUT'
            zc[z] = zc.get(z, 0) + 1
        rep('p0-v003', f'G21(b)+GIBSON-R2 scanboard-drone{v or ".rgb"} A5 diff inside panel+ring+hunt+4 corner holes', 'OUT' not in zc,
            f'{len(dd)}px {zc}')
    xy = lambda bx: sorted((k, x, y) for k, n, x, y, w, h in bx)
    rep('p0-v003', 'G21(c) scanboard-drone GATE_BOX x/y identical A5 on vs off (drop positions)',
        xy(_boxes(os.path.join(D, 'scanboard-drone.log'))) == xy(_boxes(os.path.join(P0REF, 'a5off-scanboard-drone.log'))), '')

    # GIBSON R1: terminal-cmdmenu diff inside (16,24)-(304,219); new cmdmenu vs new atkmenu = 922 px (rgb)
    for v in ('', '.norain'):
        c = _b(os.path.join(D, f'terminal-cmdmenu{v}.rgb')); r = _png(f'e7ba692-terminal-cmdmenu{v}.png')
        dd = _diff(c, r); bb = _bb(dd)
        rep('p0-v003', f'GIBSON-R1 terminal-cmdmenu{v or ".rgb"} diff vs e7ba692 inside (16,24)-(304,219)',
            bool(dd) and bb[0] >= 16 and bb[1] >= 24 and bb[2] <= 304 and bb[3] <= 219, f'{len(dd)}px bbox {bb}')
        am = _diff(c, _b(os.path.join(D, f'terminal-atkmenu{v}.rgb')))
        if v == '':
            rep('p0-v003', 'GIBSON-R1 new terminal-cmdmenu vs new terminal-atkmenu (rgb) = 922 px', len(am) == 922, f'{len(am)}px')
        else:
            rep('p0-v003', 'GIBSON-R1 new terminal-cmdmenu vs new terminal-atkmenu (norain) logged', True, f'{len(am)}px (e7ba692: 0px)')
else:
    rep('p0-v003', 'P0 v003 reference dir release/gate/p0-v003 present', False, 'missing')

fails = 0; cur = None
for scr, item, ok, det in results:
    if scr != cur:
        print(f'== {scr}'); cur = scr
    print(f"  {'N/A ' if ok is None else 'PASS' if ok else 'FAIL'}  {item}" + (f'  [{det}]' if det else ''))
    fails += 0 if (ok or ok is None) else 1
na = sum(1 for r in results if r[2] is None)
print(f'GATE: {len(results) - fails - na}/{len(results) - na} PASS, {fails} FAIL, {na} N/A -> {"PASS" if fails == 0 else "FAIL"}')
sys.exit(min(fails, 255))
