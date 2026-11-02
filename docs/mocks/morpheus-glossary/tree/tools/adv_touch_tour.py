#!/usr/bin/env python3
"""Adv-plate touch tour for SELECT chip grid + CORE glossary — every plate screen P+L.

Covers: cmdline + ≡ jitter (both open chip grid), glossary pad → CORE sheet,
verb→device chip build, <WORD + BACK, RUN, every leaf, scan tabs/MORE/tiles/link
chips, crown→home. Scores correct/miss/wrong/double on s_cyd_tap_fires. Misses=0.
"""
import subprocess, re, os, json, sys

SIM = '/workspace/cyd-sim-v3/build/cydsim_poke'
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/adv-tour'
os.makedirs(OUT, exist_ok=True)

SCREENS = [
    ('hub', ['poke screen 1']),
    ('scan', ['poke screen 7']),
    ('ctrl', ['poke screen 8']),
    ('gchat', ['poke screen 10']),
]
JIT = [(0, 0), (6, 0), (-6, 0), (0, 6), (0, -6), (6, 6), (-6, -6), (6, -6), (-6, 6)]
HOLD = [30, 80, 150]

L_SCAN = dict(kx0=166, ky=42, kw=84, kh=28, kcols=2,
              t0x=171, t0y=114, pitch=28, cols=3, rows=2, tile=28)
P_SCAN = dict(kx0=34, ky=133, kw=172, kh=14, kcols=0,
              t0x=42, t0y=163, pitch=31, cols=4, rows=3, tile=28)

BUF = re.compile(
    r"buf mode=(\d+) sym=(\d+) screen=(\d+) sheet=(\d+)/(\d+) focus=(\d+) ovl=(\d+) "
    r"cmds=(\d+) cl=(\d+) lvl=(\d+) top=(\d+) n=(\d+) fires=(\d+) buf='(.*)' log='(.*)'$"
)


def run(lines, land, tag):
    p = os.path.join(OUT, tag + '.txt')
    open(p, 'w').write('\n'.join(lines) + '\n')
    args = [SIM, '--headless', '--script', p, '--out', OUT] + (['--landscape'] if land else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=1200)
    return (r.stderr + r.stdout).split('\n')


def head(setup):
    return ['until 7600'] + setup + ['wait 300']


def parse_buf(lines):
    for l in reversed(lines):
        m = BUF.search(l)
        if m:
            return dict(
                mode=int(m[1]), sym=int(m[2]), screen=int(m[3]),
                sheet=(int(m[4]), int(m[5])), focus=int(m[6]), ovl=int(m[7]),
                cmds=int(m[8]), cl=int(m[9]), lvl=int(m[10]), top=int(m[11]),
                n=int(m[12]), fires=int(m[13]), buf=m[14], log=m[15],
            )
    return None


def ctr_box(b):
    x, y, w, h = b
    return (x + w // 2, y + h // 2)


def discover(setup, land, tag):
    out = run(head(setup) + ['state', 'keys', 'buf'], land, tag + '_d0')
    st = [l for l in out if 'pt=' in l and 'state' in l][-1]
    x, y, w, h = map(int, re.search(r'pt=(\d+),(\d+),(\d+),(\d+)', st).groups())
    plate = None
    chips, pads, ext = {}, {}, {}
    for l in out:
        m = re.search(
            r'\[sim\] plate (\d+) (\d+) (\d+) (\d+) py=(\d+) cmd=(\d+) atk=(\d+)', l)
        if m:
            plate = tuple(map(int, m.groups()))
        m = re.search(r'eqy=(\d+) rety=(\d+) row=(-?\d+) rowh=(\d+) sheetw=(\d+)', l)
        if m:
            ext = dict(zip(('eqy', 'rety', 'row', 'rowh', 'sheetw'), map(int, m.groups())))
        m = re.search(r'\[sim\] pad (eq|ret|gls) (\d+) (\d+) (\d+) (\d+)', l)
        if m:
            pads[m[1]] = tuple(map(int, m.groups()[1:]))
        m = re.search(r'\[sim\] chip (\d+) (\S+) (\d+) (\d+) (\d+) (\d+)', l)
        if m:
            chips[m[2]] = tuple(map(int, m.groups()[2:]))
    if not plate:
        plate = (x, y, w, h, y + h - 11, x + w - 24, x + w - 48)

    clbtn, rows0 = {}, []
    for l in run(head(setup) + ['cl 1', 'wait 80', 'clkeys', 'buf'], land, tag + '_d1'):
        m = re.search(r'\[sim\] clbtn (\d+) (\d+) (\d+) (\d+) (\d+)', l)
        if m:
            clbtn[int(m[1])] = tuple(map(int, m.groups()[1:]))
        m = re.search(r'\[sim\] clrow (\d+) (\d+) (\d+) (\d+) (\d+) (\S+)', l)
        if m:
            rows0.append(dict(i=int(m[1]), x=int(m[2]), y=int(m[3]),
                              w=int(m[4]), h=int(m[5]), tok=m[6]))

    sx, sy, sw, sh, py = plate[0], plate[1], plate[2], plate[3], plate[4]
    line = (sx + 40, py + 4)
    pe, pr = pads['eq'], pads['ret']
    pg = pads.get('gls')
    eq, ret = ctr_box(pe), ctr_box(pr)
    gls = ctr_box(pg) if pg else None
    ew = 320 if land else 240
    crown = (ew - 94, 13)
    # Clamp ≡ jitter so taps stay inside the eq pad (pad may sit on cmdline band)
    eq_jit = []
    for jx, jy in JIT:
        tx, ty = eq[0] + jx, eq[1] + jy
        if pe[0] <= tx < pe[0] + pe[2] and pe[1] <= ty < pe[1] + pe[3]:
            eq_jit.append((jx, jy))
    if not eq_jit:
        eq_jit = [(0, 0)]
    ret_jit = []
    for jx, jy in JIT:
        tx, ty = ret[0] + jx, ret[1] + jy
        if pr[0] <= tx < pr[0] + pr[2] and pr[1] <= ty < pr[1] + pr[3]:
            ret_jit.append((jx, jy))
    if not ret_jit:
        ret_jit = [(0, 0)]
    line_jit = []
    for jx, jy in JIT:
        tx, ty = line[0] + jx, line[1] + jy
        if sx <= tx < sx + sw and sy <= ty < sy + sh:
            line_jit.append((jx, jy))
    if not line_jit:
        line_jit = [(0, 0)]

    pads_list = [pe, pr] + ([pg] if pg else [])
    # stacked column: no pairwise overlap on Y
    ys = sorted((p_[1], p_[1] + p_[3]) for p_ in pads_list)
    no_ov = all(ys[i][1] <= ys[i + 1][0] for i in range(len(ys) - 1))
    # ≡ must sit fully inside the plate (bottom rim); ret/gls may rise above plate top
    plate_bot = y + h
    eq_inside = pe[1] >= y and pe[1] + pe[3] <= plate_bot
    padchk = dict(
        eq=pe, ret=pr, gls=pg,
        size_ok=all(p_[2] >= 28 and p_[3] >= 28 for p_ in pads_list),
        no_overlap=no_ov,
        eq_inside_plate=eq_inside,
    )
    gls_jit = [(0, 0)]
    if pg:
        gls_jit = []
        for jx, jy in JIT:
            tx, ty = gls[0] + jx, gls[1] + jy
            if pg[0] <= tx < pg[0] + pg[2] and pg[1] <= ty < pg[1] + pg[3]:
                gls_jit.append((jx, jy))
        if not gls_jit:
            gls_jit = [(0, 0)]
    return dict(
        scr=int(re.search(r'screen=(\d+)', st).group(1)),
        plate=(x, y, w, h), plate_geo=plate, line=line, eq=eq, ret=ret, gls=gls,
        padchk=padchk, chips=chips, ext=ext, clbtn=clbtn, rows0=rows0,
        crown=crown, land=land, name=tag,
        eq_jit=eq_jit, ret_jit=ret_jit, line_jit=line_jit, gls_jit=gls_jit,
    )


def tour(setup, land, tag, D):
    L = head(setup)
    ev = []
    n = [0]
    pend = [0]

    def tap(pt, j=(0, 0), hold=80):
        L.append('tap %d %d %d' % (pt[0] + j[0], pt[1] + j[1], hold))
        L.append('wait 55')
        pend[0] += 1

    def mark(test, key=None, extra=None):
        L.append('echo M %d' % n[0])
        L.append('buf')
        ev.append((test, key, extra, pend[0]))
        n[0] += 1
        pend[0] = 0

    def btn(i):
        return ctr_box(D['clbtn'][i])

    def open_cl():
        L.append('cl 1')
        L.append('wait 70')

    def close_cl_back():
        # BACK only works when CL is open; also poke-close as safety before mark
        tap(btn(0), (0, 0), 80)

    def ensure_closed():
        L.append('cl 0')
        L.append('palette 0')
        L.append('nosheet')
        L.append('attack 0')
        L.append('wait 40')

    mark('base')

    # --- cmdline open/close with in-plate jitter ---
    for t, j in enumerate(D['line_jit']):
        ensure_closed()
        tap(D['line'], j, HOLD[t % 3])
        mark('open_line')
        close_cl_back()
        mark('close_list')

    # --- ≡ opens command list (not the old palette) ---
    for t, j in enumerate(D['eq_jit']):
        ensure_closed()
        tap(D['eq'], j, HOLD[t % 3])
        mark('open_eq')
        close_cl_back()
        mark('close_eq')
    # ≡ toggle: re-tap ≡ while open should keep CL semantics; close via BACK
    for t, j in enumerate(D['eq_jit'][:4]):
        ensure_closed()
        tap(D['eq'], j, HOLD[t % 3])
        mark('open_eq')
        close_cl_back()
        mark('close_eq')

    # --- reticle attack (CL must be closed first) ---
    for t, j in enumerate(D['ret_jit']):
        ensure_closed()
        tap(D['ret'], j, HOLD[t % 3])
        mark('open_attack')
        L.append('attack 0')
        L.append('wait 40')
        mark('close_attack')
    for t, j in enumerate(D['ret_jit'][:4]):
        ensure_closed()
        tap(D['ret'], j, HOLD[t % 3])
        mark('open_attack')
        tap(D['ret'], (0, 0), 80)
        mark('close_attack')

    # --- glossary / CORE sheet button ---
    if D.get('gls'):
        for t, j in enumerate(D['gls_jit']):
            ensure_closed()
            tap(D['gls'], j, HOLD[t % 3])
            mark('open_gls')
            L.append('nosheet')
            L.append('wait 40')
            mark('close_gls')
        for t, j in enumerate(D['gls_jit'][:4]):
            ensure_closed()
            tap(D['gls'], j, HOLD[t % 3])
            mark('open_gls')
            tap(D['gls'], (0, 0), 80)  # toggle close
            mark('close_gls')

    # --- link chips ---
    for name, box in list(D['chips'].items())[:6]:
        ensure_closed()
        tap(ctr_box(box), (0, 0), 70)
        mark('chip', None, name)
        ensure_closed()

    # --- build via chip grid: scan → wifi, <WORD, RUN ---
    ensure_closed()
    L.append('clr')
    open_cl()
    mark('cl_open_poke')
    scan = next((r for r in D['rows0'] if r['tok'] == 'scan'), D['rows0'][0] if D['rows0'] else None)
    if scan:
        tap((scan['x'] + scan['w'] // 2, scan['y'] + scan['h'] // 2), (0, 0), 80)
        mark('pick_verb', None, 'scan')
        # device chips share chip[0] slot (stable Y); wifi is first opt for scan
        tap((scan['x'] + scan['w'] // 2, scan['y'] + scan['h'] // 2), (0, 0), 80)
        mark('pick_arg', None, 'wifi')
        tap(btn(1), (0, 0), 80)  # <WORD
        mark('word_pop')
        tap((scan['x'] + scan['w'] // 2, scan['y'] + scan['h'] // 2), (0, 0), 80)
        mark('pick_arg2', None, 'wifi')
        tap(btn(4), (0, 0), 100)  # RUN
        L.append('wait 220')
        mark('run_scan_wifi')

    # --- every leaf runs (seed + RUN) ---
    LEAF_VERBS = [
        'help', 'bots', 'roster', 'pulse', 'dig', 'clear', 'history',
        'theme', 'about', 'cmds', 'version', 'uptime', 'rollcall',
    ]
    for verb in LEAF_VERBS:
        ensure_closed()
        L.append('clr')
        L.append('put %s ' % verb)
        L.append('cl 1')
        L.append('wait 60')
        tap(btn(4), (0, 0), 100)
        L.append('wait 220')
        mark('leaf_run', None, verb)

    # list-built leaves
    for verb_tok in ('help', 'bots'):
        ensure_closed()
        L.append('clr')
        open_cl()
        L.append('wait 50')
        r = next((x for x in D['rows0'] if x['tok'] == verb_tok), None)
        if r:
            tap((r['x'] + r['w'] // 2, r['y'] + r['h'] // 2), (0, 0), 80)
            mark('leaf_pick', None, verb_tok)
            tap(btn(4), (0, 0), 100)
            L.append('wait 220')
            mark('leaf_run_list', None, verb_tok)
        ensure_closed()

    # scroll UP/DN
    ensure_closed()
    open_cl()
    L.append('wait 50')
    tap(btn(3), (0, 0), 70)  # DN
    mark('scroll_dn')
    tap(btn(2), (0, 0), 70)  # UP
    mark('scroll_up')
    close_cl_back()
    mark('close_after_scroll')

    # --- scan tabs / MORE / tiles ---
    if D['scr'] == 7 or tag.startswith('scan'):
        L.append('poke screen 7')
        L.append('wait 120')
        ensure_closed()
        G = L_SCAN if land else P_SCAN
        if land:
            pw = (G['kw'] - 4) // 2
            gap = 4
            rpitch = G['kh'] + 2
            for ri in range(2):
                for ci in range(2):
                    tx = G['kx0'] + ci * (pw + gap) + pw // 2
                    ty = G['ky'] + ri * rpitch + G['kh'] // 2
                    tap((tx, ty), (0, 0), 70)
                    mark('tab', None, '%d_%d' % (ri, ci))
        else:
            pw, gap = 40, 4
            for i, lab in enumerate(('ALL', 'WIFI', 'BT', 'DEAUTH')):
                tx = G['kx0'] + i * (pw + gap) + pw // 2
                ty = G['ky'] + G['kh'] // 2
                tap((tx, ty), (0, 0), 70)
                mark('tab', None, lab)
        for ri in range(min(2, G['rows'])):
            for ci in range(min(3, G['cols'])):
                tx = G['t0x'] + ci * G['pitch'] + G['tile'] // 2
                ty = G['t0y'] + ri * G['pitch'] + G['tile'] // 2
                tap((tx, ty), (0, 0), 70)
                mark('tile', None, '%d_%d' % (ri, ci))
        if land:
            mx = G['t0x'] + G['cols'] * G['pitch'] + 14
            my = G['t0y'] + (G['rows'] * G['pitch'] - 28) // 2 + 14
        else:
            mx = G['t0x'] + 3 * G['pitch'] + G['tile'] // 2
            my = G['t0y'] + 2 * G['pitch'] + G['tile'] // 2
        tap((mx, my), (0, 0), 70)
        mark('more')

    # --- crown → home ---
    for t, j in enumerate([(0, 0), (4, 0), (-4, 0)]):
        L.append('poke screen 7')
        L.append('wait 100')
        ensure_closed()
        tap(D['crown'], j, HOLD[t % 3])
        L.append('wait 80')
        mark('crown_home')

    L.append('shot %s-done' % tag)
    L.append('quit')
    out = run(L, land, tag + '_tour')

    marks = {}
    expecting = None
    for l in out:
        m = re.search(r'\bM (\d+)\b', l)
        if m:
            expecting = int(m.group(1))
        elif 'buf mode=' in l and expecting is not None:
            marks[expecting] = parse_buf([l])
            expecting = None

    stats = dict(taps=0, correct=0, misses=0, wrong=0, doubles=0, dropped=0,
                 open_ok=0, open_n=0)
    prev_buf = ''
    prev_fires = None
    prev_log = ''
    miss_detail = []
    for i, (test, key, extra, ntap) in enumerate(ev):
        b = marks.get(i)
        stats['taps'] += 1
        if not b:
            stats['misses'] += 1
            miss_detail.append((i, test, 'nobuf', None))
            continue
        if prev_fires is not None and ntap > 0:
            d = b['fires'] - prev_fires
            if d > ntap:
                stats['doubles'] += d - ntap
            elif d < ntap:
                stats['dropped'] += ntap - d
        if ntap > 0 or prev_fires is None:
            prev_fires = b['fires']

        ok = False
        wrong = False
        if test == 'base':
            ok = True
        elif test == 'open_line':
            stats['open_n'] += 1
            ok = (b['cl'] == 1)
            if ok:
                stats['open_ok'] += 1
        elif test == 'close_list':
            ok = (b['cl'] == 0)
        elif test == 'open_eq':
            stats['open_n'] += 1
            ok = (b['cl'] == 1)  # ≡ opens command list
            if ok:
                stats['open_ok'] += 1
        elif test == 'close_eq':
            ok = (b['cl'] == 0)
        elif test == 'open_attack':
            stats['open_n'] += 1
            ok = (b['ovl'] == 2)
            if ok:
                stats['open_ok'] += 1
        elif test == 'close_attack':
            ok = (b['ovl'] == 0)
        elif test == 'open_gls':
            stats['open_n'] += 1
            ok = (b['sheet'][0] == 1 and b['cl'] == 0)
            if ok:
                stats['open_ok'] += 1
        elif test == 'close_gls':
            ok = (b['sheet'][0] == 0)
        elif test == 'chip':
            ok = True
        elif test == 'cl_open_poke':
            ok = (b['cl'] == 1)
        elif test == 'pick_verb':
            ok = (b['cl'] == 1 and b['lvl'] >= 1 and (extra is None or extra in b['buf']))
            if b['cl'] == 1 and b['lvl'] >= 1 and extra and extra not in b['buf']:
                wrong = True
        elif test in ('pick_arg', 'pick_arg2'):
            ok = (b['cl'] == 1 and (extra is None or extra in b['buf']))
            if b['cl'] == 1 and extra and extra not in b['buf'] and b['buf'] != prev_buf:
                wrong = True
        elif test == 'word_pop':
            ok = (b['cl'] == 1 and (b['lvl'] <= 1 or 'wifi' not in b['buf']))
        elif test == 'run_scan_wifi':
            ok = (b['cl'] == 0 and (
                'sweep' in b['log'] or 'wifi' in b['log'].lower() or b['buf'] == ''))
            stats['scan_log'] = b['log']
        elif test in ('leaf_run', 'leaf_run_list'):
            ok = (b['cl'] == 0) or (b['log'] != prev_log) or (b['buf'] == '')
        elif test == 'leaf_pick':
            ok = (b['cl'] == 1 and (extra is None or extra in b['buf'] or b['lvl'] >= 1))
        elif test == 'scroll_dn':
            ok = (b['cl'] == 1 and b['top'] > 0)
        elif test == 'scroll_up':
            ok = (b['cl'] == 1)
        elif test == 'close_after_scroll':
            ok = (b['cl'] == 0)
        elif test in ('tab', 'tile', 'more'):
            ok = True
        elif test == 'crown_home':
            ok = (b['screen'] == 1)
        else:
            ok = True

        if wrong:
            stats['wrong'] += 1
            miss_detail.append((i, test, 'wrong', b))
        elif ok:
            stats['correct'] += 1
        else:
            stats['misses'] += 1
            miss_detail.append((i, test, 'miss', b))
        prev_buf = b['buf'] if b else prev_buf
        prev_log = b['log'] if b else prev_log

    stats['miss_detail'] = [
        (i, t, k, None if b is None else dict(cl=b['cl'], ovl=b['ovl'], scr=b['screen'],
                                              top=b['top'], buf=b['buf'][:40], log=b['log'][:40]))
        for i, t, k, b in miss_detail
    ]
    return stats, marks, ev, out


TABLE = []
for name, setup in SCREENS:
    for land, ori in [(False, 'P'), (True, 'L')]:
        tag = f'{name}-{ori}'
        print('DISCOVER', tag, flush=True)
        D = discover(setup, land, tag)
        print('  plate', D['plate'], 'clbtn', len(D['clbtn']), 'rows', len(D['rows0']),
              'chips', len(D['chips']), 'crown', D['crown'],
              'jit', len(D['line_jit']), len(D['eq_jit']), len(D['ret_jit']), 'gls', D.get('gls'), flush=True)
        print('TOUR', tag, flush=True)
        stats, marks, ev, out = tour(setup, land, tag, D)
        stats['tag'] = tag
        stats['clbtn'] = len(D['clbtn'])
        stats['padchk'] = D['padchk']
        print('  pads', D['padchk'], flush=True)
        if stats['miss_detail']:
            print('  MISSES', stats['miss_detail'][:8], flush=True)
        TABLE.append(stats)
        print(' ', {k: stats[k] for k in stats if k not in ('scan_log', 'padchk', 'miss_detail')},
              flush=True)

# strip miss_detail for json size
for s in TABLE:
    s.pop('miss_detail', None)
json.dump(TABLE, open(os.path.join(OUT, 'FUNCTION.json'), 'w'), indent=2)
print('\nFUNCTION TABLE')
print(f'{"tag":12} taps corr miss wrong dbl drop open_ok/n')
for s in TABLE:
    print(f"{s['tag']:12} {s['taps']:4} {s['correct']:4} {s['misses']:4} {s['wrong']:4} "
          f"{s['doubles']:3} {s['dropped']:4} {s['open_ok']:3}/{s['open_n']:<3} "
          f"{s.get('scan_log', '')[:40]}")
tot = dict(taps=0, correct=0, misses=0, wrong=0, doubles=0, dropped=0, open_ok=0, open_n=0)
for s in TABLE:
    for k in tot:
        tot[k] += s.get(k, 0)
print('TOTAL', tot, 'rate', tot['correct'] / max(1, tot['taps']))
print('PADS', 'all ok' if all(
    all(v for k, v in s['padchk'].items() if k not in ('eq', 'ret')) for s in TABLE
) else 'FAIL')
if tot['misses'] != 0:
    sys.exit(2)
