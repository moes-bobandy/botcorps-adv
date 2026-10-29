#!/usr/bin/env python3
"""Sim touch-tour for the CYD plate prompt + keyboard (every keyboard screen, P and L).
Each tap goes through the sim's raw touch -> firmware calibration -> read_key release-commit
-> portrait_touch -> pt_hit path. Oracle reads the live typed line / mode / sym after each tap."""
import subprocess, re, sys, os, json
SIM = '/workspace/cyd-sim-v3/build/cydsim_poke'
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/tour'
os.makedirs(OUT, exist_ok=True)
SCREENS = [('hub', 'poke screen 1'), ('wifi', 'cmd scan wifi'), ('ble', 'cmd scan ble'),
           ('gib', 'cmd hunt'), ('gchat', 'cmd defend')]
JIT = [(0, 0), (6, 0), (-6, 0), (0, 6), (0, -6), (6, 6), (-6, -6), (6, -6), (-6, 6)]
HOLD = [30, 80, 150]
KIND = {1: 'CHAR', 2: 'SPACE', 3: 'BKSP', 4: 'ENTER', 5: 'TAB', 6: 'SYM', 7: 'HIDE'}

def run(lines, land, tag):
    p = os.path.join(OUT, tag + '.txt')
    open(p, 'w').write('\n'.join(lines) + '\n')
    args = [SIM, '--headless', '--script', p, '--out', OUT] + (['--landscape'] if land else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=600)
    return (r.stderr + r.stdout).split('\n')

def head(setup):
    return ['until 7600', 'poke screen 1', 'wait 400', setup, 'wait 300']

BUF = re.compile(r"buf mode=(\d+) sym=(\d+) screen=(\d+) sheet=(\d+)/(\d+) buf='(.*)' log='(.*)'$")
def parse_buf(l):
    m = BUF.search(l)
    return dict(mode=int(m[1]), sym=int(m[2]), screen=int(m[3]), sheet=(int(m[4]), int(m[5])), buf=m[6], log=m[7])

def discover(setup, land, tag):
    out = run(head(setup) + ['state'], land, tag + '_d0')
    st = [l for l in out if ' state screen=' in l and 'pt=' in l][-1]
    x, y, w, h = map(int, re.search(r'pt=(\d+),(\d+),(\d+),(\d+)', st).groups())
    scr = int(re.search(r'screen=(\d+)', st).group(1))
    ax, gx, by = x + w - 39, x + w - 76, y + h - 29
    abc = (ax + 17, by + 14); eq = (gx + 17, by + 14); line = ((x + gx - 2) // 2, by + 14)
    out = run(head(setup) + ['tap %d %d' % abc, 'wait 100', 'keys'], land, tag + '_d1')
    keys = []
    ov = None
    for l in out:
        m = re.search(r'\[sim\] key (\d+) (-?\d+) (-?\d+) (\d+) (\d+) kind=(\d+) a=(\d+) s=(\d+)', l)
        if m:
            i, kx, ky, kw, kh, kind, a, s = map(int, m.groups())
            keys.append(dict(i=i, x=kx, y=ky, w=kw, h=kh, kind=kind, a=chr(a) if a else '', s=chr(s) if s else ''))
        m = re.search(r'overlay (\d+) (\d+) (\d+) (\d+)', l)
        if m: ov = tuple(map(int, m.groups()))
    return dict(scr=scr, plate=(x, y, w, h), abc=abc, eq=eq, line=line, keys=keys, ov=ov)

def ctr(k): return (k['x'] + k['w'] // 2, k['y'] + k['h'] // 2)

def tour(setup, land, tag, D):
    L = head(setup)
    ev = []   # (test, key-index or name, expect-kind, extra)
    n = [0]
    def tap(pt, j, hold):
        L.append('tap %d %d %d' % (pt[0] + j[0], pt[1] + j[1], hold))
        L.append('wait 40')
    def mark(test, key, extra=None):
        L.append('echo M %d' % n[0]); L.append('buf'); ev.append((test, key, extra)); n[0] += 1
    keys = D['keys']
    hide = [k for k in keys if k['kind'] == 7][0]
    # baseline
    mark('base', None)
    # prompt button (abc): 9 jittered taps, each must open the keys on that first tap; BACK closes (HIDE key test)
    for t, j in enumerate(JIT):
        tap(D['abc'], j, HOLD[t % 3]); mark('open_abc', None)
        tap(ctr(hide), j, HOLD[(t + 1) % 3]); mark('key', hide['i'])
    for t, j in enumerate(JIT[:5]):
        tap(D['line'], j, HOLD[t % 3]); mark('open_line', None)
        tap(ctr(hide), (0, 0), 80); mark('close', None)
    ox, oy, ow, oh = D['ov']
    bx2, by2, bw2, bh2 = ox + 2, oy + 15, ow - 4, oh - 17
    bk_h = 26 if bh2 > 90 else 12
    back = (bx2 + 2 * (bw2 // 3) + (bw2 // 3) // 2, by2 + bh2 - bk_h // 2)
    for t, j in enumerate(JIT[:5]):
        tap(D['eq'], j, HOLD[t % 3]); mark('open_list', None)
        tap(back, (0, 0), 80); mark('close', None)
    # open keyboard for the key tour
    L.append('put '); tap(D['abc'], (0, 0), 80); mark('reopen', None)
    order = [k for k in keys if k['kind'] in (1, 2)] + [k for k in keys if k['kind'] == 3] + \
            [k for k in keys if k['kind'] == 6] + [k for k in keys if k['kind'] == 5] + [k for k in keys if k['kind'] == 4]
    for k in order:
        if k['kind'] == 3: L.append('put abcdefghijklmnopqr'); mark('pre', None)
        if k['kind'] == 4: pass
        for t, j in enumerate(JIT):
            if k['kind'] in (1, 2) and t == 0: L.append('put '); mark('pre', None)
            if k['kind'] == 4:
                L.append('put zz'); mark('pre', None)
            tap(ctr(k), j, HOLD[t % 3]); mark('key', k['i'])
            if k['kind'] == 4:   # ENTER may close the keys / move screens: get back to the keys
                L.append('poke screen 1'); L.append('wait 200'); L.append(setup); L.append('wait 300')
                tap(D['abc'], (0, 0), 80); mark('reopen', None)
        if k['kind'] == 6 and False: pass
        if k['kind'] == 5: L.append('nosheet'); L.append('wait 100'); mark('pre', None)
    out = run(L, land, tag + '_tour')
    bufs = [parse_buf(l) for l in out if ' buf mode=' in l]
    assert len(bufs) == len(ev), (len(bufs), len(ev))
    return ev, bufs

def score(D, ev, bufs):
    K = {k['i']: k for k in D['keys']}
    res = {}   # name -> [taps, correct, miss, wrong, double]
    def add(name, c=0, m=0, w=0, d=0):
        r = res.setdefault(name, [0, 0, 0, 0, 0]); r[0] += 1; r[1] += c; r[2] += m; r[3] += w; r[4] += d
    fails = []
    for idx, (test, key, _) in enumerate(ev):
        if test in ('base', 'pre', 'close', 'reopen'): continue
        b0, b1 = bufs[idx - 1], bufs[idx]
        if test == 'open_abc':
            add('prompt abc', c=b1['mode'] == 1, m=b1['mode'] != 1)
        elif test == 'open_line':
            add('prompt line', c=b1['mode'] == 1, m=b1['mode'] != 1)
        elif test == 'open_list':
            add('prompt =', c=b1['mode'] == 2, m=b1['mode'] != 2)
        elif test == 'key':
            k = K[key]; kind = k['kind']
            name = {1: repr(k['a']), 2: 'SPC'}.get(kind, KIND[kind])
            ok = miss = wrong = dbl = 0
            if kind in (1, 2):
                want = ' ' if kind == 2 else (k['s'] if b0['sym'] else k['a'])
                if b1['buf'] == b0['buf'] + want: ok = 1
                elif b1['buf'] == b0['buf'] + want + want: dbl = 1
                elif b1['buf'] == b0['buf']: miss = 1
                else: wrong = 1
            elif kind == 3:
                if b1['buf'] == b0['buf'][:-1]: ok = 1
                elif b1['buf'] == b0['buf'][:-2]: dbl = 1
                elif b1['buf'] == b0['buf']: miss = 1
                else: wrong = 1
            elif kind == 6:
                if b1['sym'] != b0['sym'] and b1['buf'] == b0['buf'] and b1['mode'] == 1: ok = 1
                elif b1['sym'] == b0['sym'] and b1['buf'] == b0['buf']: miss = 1   # a double flips back too
                else: wrong = 1
            elif kind == 7:
                if b1['mode'] == 0 and b0['mode'] == 1 and b1['buf'] == b0['buf']: ok = 1
                elif b1['mode'] == 1: miss = 1
                else: wrong = 1
            elif kind == 5:
                same = b1['buf'] == b0['buf'] and b1['mode'] == b0['mode'] and b1['sym'] == b0['sym']
                if b1['screen'] == 10:   # Defend: TAB opens / steps the command sheet
                    s0, s1 = b0['sheet'], b1['sheet']
                    exp = (1, 0) if not s0[0] else (1, None)
                    if same and s1[0] == 1 and (s0[0] == 0 and s1[1] == 0 or s0[0] == 1 and s1[1] != s0[1]): ok = 1
                    elif same and s1 == s0: miss = 1
                    else: wrong = 1
                else:                    # hub / scan screens: TAB is a firmware no-op; nothing else may change
                    ok = 1 if same else 0; wrong = 0 if same else 1
            elif kind == 4:
                if b0['buf'] == 'zz' and b1['buf'] != 'zz' and b1['buf'] != 'z': ok = 1
                elif b1['buf'] == 'zz': miss = 1
                else: wrong = 1
            add(name, ok, miss, wrong, dbl)
            if not ok: fails.append((name, b0, b1))
    return res, fails

if __name__ == '__main__':
    allres = {}
    for land in (False, True):
        o = 'L' if land else 'P'
        for nm, setup in SCREENS:
            tag = '%s_%s' % (o, nm)
            D = discover(setup, land, tag)
            ev, bufs = tour(setup, land, tag, D)
            res, fails = score(D, ev, bufs)
            allres[tag] = dict(D=D, res=res, fails=[(f[0], f[1]['buf'], f[2]['buf'], f[1]['mode'], f[2]['mode']) for f in fails])
            tot = [sum(r[i] for r in res.values()) for i in range(5)]
            print(tag, 'screen', D['scr'], 'plate', D['plate'], 'ov', D['ov'], 'keys', len(D['keys']), 'taps/correct/miss/wrong/double', tot, flush=True)
            for f in fails[:8]: print('   FAIL', f[0], f[1], '->', f[2])
    json.dump(allres, open(os.path.join(OUT, 'tour.json'), 'w'), indent=1)
