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
        rep(scr, 'D1 dynamic "<name>> Defend" title present', bool(has), f'{has[:1]}')
    # v1019j (Dirt LOCKED): the four Defend mini-games. Matte-black board, the active bot (v1019l: as its round
    # sprite, Gibson round ivory on his maze), and the game's own pieces on the board (reused icons + 0/1 bits).
    if scr.startswith('defend-'):
        DGW = {'defend-raincatch': ('bot', ('icon', 'bit'), 1), 'defend-beaconsort': ('bot', ('beacon',), 1),
               'defend-firewall': ('bot', ('block', 'iot', 'packet'), 30), 'defend-maze': ('gibson', ('shield', 'jammer'), 4),
               # v1019m: the eight added games (same bots per category, Gibson on the two Rogue ones)
               'defend-eviltwin': ('bot', ('twin', 'ap'), 2), 'defend-channelhop': ('bot', ('noise',), 1),
               'defend-rangering': ('bot', ('ble',), 1), 'defend-pairlock': ('bot', ('pin', 'rogue'), 2),
               'defend-patchrun': ('bot', ('iot', 'vuln', 'down'), 15), 'defend-botnet': ('bot', ('iot', 'infected', 'drip'), 6),
               'defend-signaltrace': ('gibson', ('heat', 'rogue'), 1), 'defend-jamdodge': ('gibson', ('jammer', 'noise'), 2),
               'defend-zap': ('gibson', ('trace', 'null', 'device'), 4)}[scr]
        bd = [b for b in boxes if b['k'] == 'BOARD' and b['n'] == 'defend']
        if bd:
            b = bd[0]
            # zap board is full-width with device-rain + copper; sample the dark centre well
            box = (b['x'] + 6, b['y'] + 6, b['x'] + b['w'] - 6, b['y'] + b['h'] - 6)
            if scr == 'defend-zap':
                box = (24, 100, 70, 130)  # dark grid well left of centre chip
            dom = dominant(im, box)
            lim = 48 if scr == 'defend-zap' else 24
            rep(scr, 'DG1 Defend board is matte black', luma(dom) <= lim, f'dominant={dom}')
        else:
            rep(scr, 'DG1 Defend board is matte black', False, 'no BOARD box')
        # v1019l (Dirt concept: games = software side): the active bot plays as its ROUND sprite, no chip on the board.
        rb = [c for c in boxes if c['k'] == 'DGBOT' and c['n'] == DGW[0] and bd and inside(c, bd[0])]
        ch = [c for c in boxes if c['k'] in ('TCHIP', 'CHIP') and bd and inside(c, bd[0])]
        if scr == 'defend-zap':
            # v1019v mock: centre defender is the square 5-pin gold Gibson chip (not round).
            ok = bool(bd) and any(c['k'] == 'CHIP' and c['n'] == 'gibson' and inside(c, bd[0]) for c in boxes)
            rep(scr, 'DG2 centre Gibson is square chip (zap mock)', ok, f'chips={[(c["n"], c["w"]) for c in ch][:3]}')
        else:
            rep(scr, 'DG2 active bot is its round sprite on the board, no chip' + (' (Gibson)' if DGW[0] == 'gibson' else ''), len(rb) == 1 and not ch, f'round={[(c["n"], c["x"], c["y"], c["w"]) for c in rb]} chips={len(ch)}')
        ob = [o for o in boxes if o['k'] == 'DGOBJ' and o['n'] in DGW[1] and bd and inside(o, bd[0])]
        if scr == 'defend-zap':
            # Zap marks traces/devices via CHIP/DGBOT + board; no DGOBJ pellets required.
            rep(scr, 'DG3 zap playfield has centre chip + board', bool(bd) and len(ch) >= 1, f'board={bool(bd)} chips={len(ch)}')
        else:
            rep(scr, 'DG3 game pieces on the board (reused icons / 0-1 bits)', len(ob) >= DGW[2], f'{len(ob)} pieces {[o["n"] for o in ob][:6]}')
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
            rep('functional', 'C1 terminal: Down from the bottom boards stops on the prompt, then the commands button', r[0] == 1, f'reach={r[0]}')
            rep('functional', 'C2 Enter on commands button opens the commands menu', r[1] == 1, f'opened={r[1]}')
            rep('functional', 'C3 Esc closes the menu back to the button; Up walks button -> prompt -> boards', r[2] == 1 and r[3] == 1, f'esc={r[2]} up={r[3]}')
            rep('functional', 'C4 a command picked in the menu runs', r[4] == 1, f'ran={r[4]}')
        elif f[0] == 'K1':
            k1_seen = True
            got = f[3] if len(f) > 3 else ''
            rep('functional', 'K1 terminal prompt shows exactly what was typed (rollover, same-scan press, Backspace)',
                f[1] == '1' and got == 'scan wifi', f'prompt={got!r} (want \'scan wifi\')')
            rep('functional', 'K2 one arrow chord = one focus stop; arrows stay arrows with text in the prompt',
                f[2] == '1', f'focus_ok={f[2]}')
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

rep('functional', 'K0 keyboard test ran', k1_seen, '' if k1_seen else 'no K1 line in functional.txt')

# v1019j DG / DGR: Defend mini-game logic (win + lose on the real step code) and launch routes.
if os.path.exists(fp):
    WANT = {'raincatch-play': (0, 1), 'raincatch-idle': (0, 2), 'beaconsort-play': (1, 1), 'beaconsort-idle': (1, 2),
            'firewall-play': (2, 1), 'firewall-idle': (2, 2), 'maze-clear': (3, 1), 'maze-idle': (3, 2),
            'eviltwin-play': (4, 1), 'eviltwin-idle': (4, 2), 'channelhop-play': (5, 1), 'channelhop-idle': (5, 2),
            'rangering-play': (6, 1), 'rangering-idle': (6, 2), 'pairlock-play': (7, 1), 'pairlock-idle': (7, 2),
            'patchrun-play': (8, 1), 'patchrun-idle': (8, 2), 'botnet-play': (9, 1), 'botnet-idle': (9, 2),
            'signaltrace-play': (10, 1), 'signaltrace-idle': (10, 2), 'jamdodge-play': (11, 1), 'jamdodge-idle': (11, 2)}
    RULE = {'raincatch-play': lambda sc, mi, ju: sc >= 20 and mi < 3 and ju < 3, 'raincatch-idle': lambda sc, mi, ju: mi >= 3 or ju >= 3,
            'beaconsort-play': lambda sc, mi, ju: sc >= 25 and mi < 3, 'beaconsort-idle': lambda sc, mi, ju: mi >= 3,
            'firewall-play': lambda sc, mi, ju: sc == 32 and mi < 3, 'firewall-idle': lambda sc, mi, ju: mi >= 3,
            'maze-clear': lambda sc, mi, ju: mi < 3, 'maze-idle': lambda sc, mi, ju: mi >= 3}
    # v1019m rules (concept doc): WIN by the game's goal with < 3 strikes, LOST by 3 strikes (or the rule's lose)
    RULE.update({'eviltwin-play': lambda sc, mi, ju: sc >= 15 and mi < 3, 'eviltwin-idle': lambda sc, mi, ju: mi >= 3,
                 'channelhop-play': lambda sc, mi, ju: mi < 3, 'channelhop-idle': lambda sc, mi, ju: mi >= 3,
                 'rangering-play': lambda sc, mi, ju: sc == 24 and mi < 3, 'rangering-idle': lambda sc, mi, ju: mi >= 3,
                 'pairlock-play': lambda sc, mi, ju: sc >= 10 and mi < 3, 'pairlock-idle': lambda sc, mi, ju: mi >= 3,
                 'patchrun-play': lambda sc, mi, ju: mi < 3, 'patchrun-idle': lambda sc, mi, ju: mi >= 3,
                 'botnet-play': lambda sc, mi, ju: True, 'botnet-idle': lambda sc, mi, ju: mi >= 3,
                 'signaltrace-play': lambda sc, mi, ju: sc >= 3 and mi < 3, 'signaltrace-idle': lambda sc, mi, ju: mi >= 3,
                 'jamdodge-play': lambda sc, mi, ju: mi < 3, 'jamdodge-idle': lambda sc, mi, ju: mi >= 3})
    # timed games must also have run their clock: Channel Hop / Botnet 60 s, Patch Run 90 s, Jam Dodge 45 s
    TIMED = {'channelhop-play': 60000, 'botnet-play': 60000, 'patchrun-play': 90000, 'jamdodge-play': 45000}
    GN = ['Rain Catch', 'Beacon Sort', 'Firewall Wall', 'Gibson Maze', 'Evil Twin', 'Channel Hop', 'Range Ring',
          'Pair Lock', 'Patch Run', 'Botnet Breaker', 'Signal Trace', 'Jam Dodge', 'Night Shift', 'Sweep', 'Hackers Zap']
    SETS = {0: [0, 4, 5], 1: [1, 6, 7], 2: [2, 8, 9], 3: [3, 10, 11]}
    seen_dg = set()
    # route w: 0 hub defend (Gibson) 1 hub defend wifi 2 terminal defend airtag 3 hub defend flock 4 scan board flipper
    #          5 scan board Gibson 6 game-chat prompt defend (Gibson)
    # v1019v: Gibson Defend entry launches HACKERS ZAP (14); other bots keep category slot-0 games.
    RW = {0: 14, 1: 0, 2: 1, 3: 2, 4: 2, 5: 14, 6: 14}
    for ln in open(fp):
        f = ln.rstrip('\n').split('\t')
        if f[0] == 'DG':
            nm = f[1]; gm, st, sc, mi, ju, ms = (int(x) for x in f[2:8]); seen_dg.add(nm)
            wg, ws = WANT[nm]
            rep('functional', f'DG {GN[wg]} {nm.split("-")[1]}: ends {"WIN" if ws == 1 else "LOST"} by its rule',
                gm == wg and st == ws and RULE[nm](sc, mi, ju) and ms >= TIMED.get(nm, 0), f'game={gm} state={st} score={sc} miss={mi} bits={ju} time={ms/1000:.1f}s')
        elif f[0] == 'DGR':
            w, gm, scr_, e1, e1g, e2 = (int(x) for x in f[1:7])
            rep('functional', f'DGR route {w}: Defend launches {GN[RW[w]]}; backtick quits to the Defend board; Esc again goes back',
                gm == RW[w] and scr_ == 10 and e1 == 10 and e1g == -1 and e2 != 10, f'game={gm} screen={scr_} esc1={e1}/{e1g} esc2={e2}')
        elif f[0] == 'DGC':
            bot, cat = int(f[1]), int(f[2]); ids = [int(x) for x in f[3:9]]
            s3 = SETS.get(cat, [])
            want = [s3[0], s3[1], s3[2], s3[0], s3[1], s3[0]] if s3 else []
            rep('functional', f'DGC category {cat}: Defend starts {GN[s3[0]] if s3 else "?"}; Tab walks its 3 games and wraps; / , step on LOST',
                ids == want, f'bot={bot} seen={ids} want={want}')
            seen_dg.add(f'cycle-{cat}')
    rep('functional', 'DG0 Defend logic + cycle tests ran (24 + 4)', len(seen_dg) == 28, f'{len(seen_dg)} {sorted(seen_dg)[:6]}...')

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

fails = 0; cur = None
for scr, item, ok, det in results:
    if scr != cur:
        print(f'== {scr}'); cur = scr
    print(f"  {'N/A ' if ok is None else 'PASS' if ok else 'FAIL'}  {item}" + (f'  [{det}]' if det else ''))
    fails += 0 if (ok or ok is None) else 1
na = sum(1 for r in results if r[2] is None)
print(f'GATE: {len(results) - fails - na}/{len(results) - na} PASS, {fails} FAIL, {na} N/A -> {"PASS" if fails == 0 else "FAIL"}')
sys.exit(min(fails, 255))
