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
MOCKDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'release', 'gate', 'mock')
MOCKREF = {'home-wifi': 'wifi', 'home-ble': 'bluetooth', 'home-bot': 'bot', 'bot-radar': 'bots'}
HOME_REG = {'ticker': (0, 0, 320, 15), 'ring': (98, 12, 222, 126), 'plate': (80, 128, 240, 166),
            'pill': (100, 168, 220, 188), 'btnL': (10, 131, 74, 163), 'btnR': (246, 131, 278, 163)}
RADAR_REG = {'core': (100, 52, 220, 164)}

def mock_img(scr):
    p = os.path.join(MOCKDIR, MOCKREF[scr] + '.png')
    if not os.path.exists(p): return None
    m = Image.open(p).convert('RGB')
    return m.crop((0, 0, 640, 480)).resize((W, H), Image.BOX) if m.size[0] >= 640 else m.resize((W, H))

def reg_stats(im, r):
    p = im.load(); n = 0; acc = [0, 0, 0]; bright = 0
    for y in range(r[1], r[3]):
        for x in range(r[0], r[2]):
            c = p[x, y]; n += 1
            for i in range(3): acc[i] += c[i]
            if luma(c) > 90: bright += 1
    return [a / n for a in acc], bright / n

def coloured(c):
    return luma(c) > 40 and not (c[1] > c[0] + 20 and c[1] > c[2] + 20) and not (c[0] > 200 and c[1] > 200 and c[2] > 200) \
        and max(c) - min(c) > 60

def mock_rain_colour(m, regs):
    p = m.load(); n = 0
    for y in range(H):
        for x in range(W):
            if any(r[0] <= x < r[2] and r[1] <= y < r[3] for r in regs): continue
            if coloured(p[x, y]): n += 1
    return n

def rep(scr, item, ok, detail=''):
    results.append((scr, item, ok, detail))

OVERLAYS = ('found-alert',)   # full-screen takeover overlay (mock): rain/crown not applicable
SOLID = ('CHIP', 'BTN', 'TILE', 'PART', 'CROWN', 'HUBCORE', 'PROMPT', 'PLATE', 'PILL')
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
    rep(scr, 'G1 no element/text bbox overlap', not bad, '; '.join(bad[:6]) + (f' (+{len(bad)-6})' if len(bad) > 6 else ''))
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
order = ['intro', 'boot', 'hub', 'hub-amber', 'terminal', 'terminal-typed', 'terminal-tab', 'home-wifi', 'home-ble', 'home-bot',
         'scan-wifi', 'scan-ble', 'bot-radar', 'sbox-selected', 'sbox-info', 'sbox-hunt', 'sbox-glossary', 'found-alert']
screens = [s for s in order if s in screens] + [s for s in screens if s not in order]
stats = {}
for scr in sorted(set(frozen) - set(screens)):
    rep(scr, 'G7 regression vs approved render', False, 'frozen screen was not rendered')
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
        gchip = [b for b in boxes if b['k'] == 'CHIP']
        rep(scr, 'H1 centre = animated rings core, not a chip', bool(core) and not gchip, f'core={len(core)} chips={len(gchip)}')
        navs = [b for b in boxes if b['k'] == 'BTN' and b['n'] == 'hubnav']
        lit = 0; det = []
        p = im.load()
        for b in navs:
            px = [p[x, y] for y in range(b['y'] + 1, b['y'] + b['h'] - 1) for x in range(b['x'] + 1, b['x'] + b['w'] - 1)]
            m = [sum(c[i] for c in px) / len(px) for i in range(3)]
            edge = [p[b['x'] + b['w'] // 2, b['y']], p[b['x'], b['y'] + b['h'] // 2]]
            lum = luma(m)
            det.append(f'{int(m[0])},{int(m[1])},{int(m[2])}')
            if lum >= 28 and max(luma(e) for e in edge) >= 120: lit += 1
        rep(scr, 'H2 three lit nav buttons (filled face + bright edge)', len(navs) == 3 and lit == 3, 'mean ' + ' | '.join(det))
        rep(scr, 'H3 rain carries bots/devices (>=150 coloured px)', col >= 150, f'colour_px={col}')
        tiles = [b for b in boxes if b['k'] == 'TILE' and b['n'] == 'hubbot']
        rep(scr, 'H4 12 bot side tiles', len(tiles) == 12, f'{len(tiles)}')
    if scr in MOCKREF:
        m = mock_img(scr)
        regs = HOME_REG if scr.startswith('home') else RADAR_REG
        if m is None:
            rep(scr, 'M1 matches mock regions', False, 'mock reference missing')
        else:
            bad, det = [], []
            for k, r in regs.items():
                (fm, fb), (mm, mb) = reg_stats(nr, r), reg_stats(m, r)
                dist = sum(abs(fm[i] - mm[i]) for i in range(3))
                ratio = (fb + 0.01) / (mb + 0.01)
                ok = dist <= 60 and 0.4 <= ratio <= 2.5
                det.append(f'{k}:d{int(dist)} b{ratio:.2f}')
                if not ok: bad.append(k)
            rep(scr, 'M1 chrome regions match mock (mean colour d<=60, bright-px ratio 0.4..2.5)', not bad, ' '.join(det) + (f' FAIL={bad}' if bad else ''))
            # M2 rain density vs the mock: coloured (bot/device) rain px outside the chrome >= 50% of the mock's
            # (rain px = rain render differs from no-rain render; mock px = coloured px outside the same regions)
            pa, pb = im.load(), nr.load(); fw = 0
            for y in range(H):
                for x in range(W):
                    if any(r[0] <= x < r[2] and r[1] <= y < r[3] for r in regs.values()): continue
                    c, d = pa[x, y], pb[x, y]
                    if abs(c[0] - d[0]) + abs(c[1] - d[1]) + abs(c[2] - d[2]) > 24 and coloured(c): fw += 1
            mk = mock_rain_colour(m, list(regs.values()))
            rep(scr, 'M2 rain bot/device density >= 50% of mock', fw >= 0.5 * mk, f'fw={fw} mock={mk} ({fw / max(1, mk):.0%})')
        if scr.startswith('home'):
            need = {'HUBCORE': 1, 'PLATE': 1, 'PILL': 1}
            have = {k: len([b for b in boxes if b['k'] == k]) for k in need}
            rep(scr, 'M3 mock structure: glow ring + name plate + WATCHING pill', all(have[k] >= 1 for k in need), str(have))
            strs = ' '.join(t['s'] for t in texts) + ' ' + ' '.join(b['n'].replace('_', ' ') for b in boxes if b['k'] == 'MARQUEE')
            want = ['RX', 'LV 01', 'UNIQUE FINDS', 'WATCHING', 'OBSERVE ONLY', 'ARMED', 'LOG']
            miss = [w for w in want if w not in strs]
            rep(scr, 'M4 status bar + plate + pill strings drawn', not miss, f'missing={miss}' if miss else 'all')
        else:
            core = [b for b in boxes if b['k'] == 'HUBCORE']; board = [b for b in boxes if b['k'] == 'BOARD']
            rep(scr, 'M3 Gibson rings core, no bus board', bool(core) and not board, f'core={len(core)} board={len(board)}')
        btns = [b for b in boxes if b['k'] == 'BTN' and b['n'] == 'litbtn']
        p = im.load(); lit = 0; det = []
        for b in btns:
            px = [p[x, y] for y in range(b['y'] + 2, b['y'] + b['h'] - 2) for x in range(b['x'] + 2, b['x'] + b['w'] - 2)]
            mf = [sum(c[i] for c in px) / len(px) for i in range(3)]
            rim = [p[b['x'] + b['w'] // 2, b['y'] + 1], p[b['x'] + 1, b['y'] + b['h'] // 2], p[b['x'] + b['w'] - 2, b['y'] + b['h'] // 2]]
            peak = max(luma(c) for c in px)
            ok = luma(mf) >= 20 and min(luma(e) for e in rim) >= 50 and peak >= 150
            lit += ok; det.append(f'face{int(luma(mf))}/rim{int(min(luma(e) for e in rim))}/icon{int(peak)}')
        want_n = 3 if scr.startswith('home') else 1
        rep(scr, f'L1 {want_n} lit button(s) like the mock capture (face luma>=20, rim>=50, lit icon>=150)', len(btns) == want_n and lit == want_n, ' '.join(det))
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
                    tot += 1; c = p[x, y]; n += (c[2] > 150 and c[1] > 150 and c[0] < 120) if scr != 'settings-amber' else (c[0] > 150)
            ok = n >= tot * 0.8; det = f'{n}/{tot} outline px accent'
        rep(scr, 'P4 selection outline in UI accent (cyan)', ok, det)
        tr = [b for b in boxes if b['k'] == 'TRACE']
        ok = False; det = 'no TRACE'
        if tr:
            p = im.load(); n = 0; tot = 0
            for t in tr:
                for y in range(t['y'], t['y'] + t['h']):
                    for x in range(t['x'], t['x'] + t['w']):
                        tot += 1; c = p[x, y]; n += (c[2] > 60 and c[1] > 60 and c[0] < c[2]) if scr != 'settings-amber' else (c[0] > 60 and c[0] > c[2])
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
if os.path.exists(fp):
    for ln in open(fp):
        f = ln.rstrip('\n').split('\t')
        if f[0] == 'ESC':
            rep('functional', f'F1 Esc goes back from {f[1]}', f[2] == f[3], f'got {f[2]} want {f[3]}')
        elif f[0] == 'SET':
            rep('functional', f'F2 setting {f[1]} edits by keyboard', f[4] == '1', f'{f[2]} -> {f[3]}')
            if f[1] in ('3', '4'):
                rep('functional', f'F3 setting {f[1]} writes NVS', int(f[5]) > 0, f'puts={f[5]}')

fails = 0; cur = None
for scr, item, ok, det in results:
    if scr != cur:
        print(f'== {scr}'); cur = scr
    print(f"  {'N/A ' if ok is None else 'PASS' if ok else 'FAIL'}  {item}" + (f'  [{det}]' if det else ''))
    fails += 0 if (ok or ok is None) else 1
na = sum(1 for r in results if r[2] is None)
print(f'GATE: {len(results) - fails - na}/{len(results) - na} PASS, {fails} FAIL, {na} N/A -> {"PASS" if fails == 0 else "FAIL"}')
sys.exit(min(fails, 255))
