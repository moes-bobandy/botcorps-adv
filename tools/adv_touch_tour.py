#!/usr/bin/env python3
"""CYD Adv-plate touch tour (no letter OSK): cmdline/≡ open list, list build,
<WORD, RUN, reticle, old-OSK spots must stay mode=0, doubles via fires. P+L."""
import subprocess, re, os, json, sys
SIM = '/workspace/cyd-sim-v3/build/cydsim_poke'
OUT = sys.argv[1] if len(sys.argv) > 1 else '/tmp/adv-tour'
os.makedirs(OUT, exist_ok=True)
SCREENS = [
    ('hub', ['poke screen 1']),
    ('scan', ['poke screen 7']),
    ('gchat', ['poke screen 10']),
]
JIT = [(0,0),(6,0),(-6,0),(0,6),(0,-6),(6,6),(-6,-6),(6,-6),(-6,6)]
HOLD = [30, 80, 150]

def run(lines, land, tag):
    p = os.path.join(OUT, tag + '.txt')
    open(p,'w').write('\n'.join(lines)+'\n')
    args = [SIM,'--headless','--script',p,'--out',OUT] + (['--landscape'] if land else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=900)
    return (r.stderr + r.stdout).split('\n')

def head(setup):
    return ['until 7600'] + setup + ['wait 400']

BUF = re.compile(
    r"buf mode=(\d+) sym=(\d+) screen=(\d+) sheet=(\d+)/(\d+) focus=(\d+) ovl=(\d+) "
    r"cmds=(\d+) cl=(\d+) lvl=(\d+) top=(\d+) n=(\d+) fires=(\d+) buf='(.*)' log='(.*)'$")

def parse_buf(lines):
    for l in reversed(lines):
        m = BUF.search(l)
        if m:
            return dict(mode=int(m[1]), sym=int(m[2]), screen=int(m[3]),
                        sheet=(int(m[4]),int(m[5])), focus=int(m[6]), ovl=int(m[7]),
                        cmds=int(m[8]), cl=int(m[9]), lvl=int(m[10]), top=int(m[11]),
                        n=int(m[12]), fires=int(m[13]), buf=m[14], log=m[15])
    return None

def discover(setup, land, tag):
    out = run(head(setup)+['state','keys'], land, tag+'_d0')
    plate = eq = ret = None
    for l in out:
        m = re.search(r'pt=(\d+),(\d+),(\d+),(\d+)', l)
        if m: plate = tuple(map(int, m.groups()))
        m = re.search(r'\[sim\] plate (\d+) (\d+) (\d+) (\d+) py=(\d+)', l)
        if m:
            sx,sy,sw,sh,py = map(int, m.groups())
            plate_geo = (sx,sy,sw,sh,py)
        m = re.search(r'\[sim\] pad eq (\d+) (\d+) (\d+) (\d+)', l)
        if m: eq = tuple(map(int, m.groups()))
        m = re.search(r'\[sim\] pad ret (\d+) (\d+) (\d+) (\d+)', l)
        if m: ret = tuple(map(int, m.groups()))
    out2 = run(head(setup)+['cl 1','wait 80','clkeys','buf'], land, tag+'_d1')
    rows=[]; btns=[]; box=None
    for l in out2:
        m = re.search(r'\[sim\] clkeys box (\d+) (\d+) (\d+) (\d+) row_y (\d+) lvl (\d+) n (\d+) top (\d+) vis (\d+)', l)
        if m: box = tuple(map(int, m.groups()))
        m = re.search(r'\[sim\] clrow (\d+) (\d+) (\d+) (\d+) (\d+) (\S+)', l)
        if m: rows.append(dict(i=int(m[1]),x=int(m[2]),y=int(m[3]),w=int(m[4]),h=int(m[5]),tok=m[6]))
        m = re.search(r'\[sim\] clbtn (\d+) (\d+) (\d+) (\d+) (\d+)', l)
        if m: btns.append(dict(i=int(m[1]),x=int(m[2]),y=int(m[3]),w=int(m[4]),h=int(m[5])))
    padchk = {
        'eq_ok': eq is not None and eq[2]>=28 and eq[3]>=28,
        'ret_ok': ret is not None and ret[2]>=28 and ret[3]>=28,
        'eq': eq, 'ret': ret,
        'btn_ok': all(b['w']>=28 and b['h']>=28 for b in btns) and len(btns)==5,
        'row_ok': all(r['h']>=28 for r in rows) if rows else False,
    }
    # old OSK trigger spots (pre-removal): centre of plate body, and mid of old letter row band
    sx,sy,sw,sh = plate[0],plate[1],plate[2],plate[3]
    old_osk_spots = [
        (sx + sw//2, sy + sh//2),          # plate centre (was open-OSK)
        (sx + 24, sy + sh - 20),           # old letter-row left
        (sx + sw//2, sy + sh - 20),        # old letter-row mid
        (sx + sw - 40, sy + sh - 20),      # old letter-row right (near ≡)
    ]
    return dict(plate=plate, plate_geo=locals().get('plate_geo'),
                eq=eq, ret=ret, box=box, rows=rows, btns=btns, padchk=padchk,
                old_osk=old_osk_spots, land=land)

def ctr_rect(r):
    return (r[0]+r[2]//2, r[1]+r[3]//2)

def ctr_btn(b):
    return (b['x']+b['w']//2, b['y']+b['h']//2)

def tour(setup, land, tag, D):
    L = head(setup)
    ev=[]; n=[0]; pend=[0]
    def tap(pt, j=(0,0), hold=80):
        L.append('tap %d %d %d' % (pt[0]+j[0], pt[1]+j[1], hold))
        L.append('wait 50'); pend[0]+=1
    def mark(test, key=None, extra=None):
        L.append('echo M %d'%n[0]); L.append('buf')
        ev.append((test,key,extra,pend[0])); n[0]+=1; pend[0]=0

    mark('base')
    # --- open via cmdline (plate body, away from ≡/reticle) ---
    sx,sy,sw,sh = D['plate']
    line_pt = (sx + sw//2 - 20, sy + sh - 8)   # near cmdline
    for j in JIT:
        tap(line_pt, j, 80); mark('open_line')
        # close via BACK
        if D['btns']:
            tap(ctr_btn(D['btns'][0]), (0,0), 80); mark('close_list')
        else:
            L.append('cl 0'); L.append('wait 40'); mark('close_list')

    # --- open via ≡ ---
    if D['eq']:
        for j in JIT[:5]:
            tap(ctr_rect(D['eq']), j, 80); mark('open_eq')
            tap(ctr_btn(D['btns'][0]), (0,0), 80); mark('close_list')

    # --- old OSK spots must NOT open letter keyboard (mode stays 0); may open cl ---
    for pt in D['old_osk']:
        for j in JIT[:3]:
            tap(pt, j, 80); mark('no_osk')
            L.append('cl 0'); L.append('wait 30')   # reset if list opened

    # --- list build: scan → wifi, then <WORD, then RUN ---
    L.append('clr'); L.append('cl 1'); L.append('wait 60'); mark('cl_open')
    # find scan row among discovered (level 0); if not in top, scroll DN
    scan_row = next((r for r in D['rows'] if r['tok']=='scan'), None)
    if scan_row is None and D['btns']:
        # scroll DN a few times hoping to find it
        for _ in range(4):
            tap(ctr_btn(D['btns'][3]), (0,0), 60)
        L.append('clkeys'); mark('scroll')
        # re-discover rows mid-tour via put+open is safer: put empty and use known geometry
        # Use absolute: row 0 of level 0 is scan (verified by gen.py)
        if D['box']:
            bx,by,bw,bh,rowy,lvl,nn,top,vis = D['box']
            scan_pt = (bx + bw//2, rowy - (vis-1)*28 + 14)  # first visible row centre approx
            # Prefer: row_y is bottom of last visible; first row y = row_y - (vis-1)*28
            # From clkeys: clrow 0 y=68 for P. Use that if we have it.
    if scan_row:
        for j in JIT[:5]:
            L.append('clr'); L.append('cl 1'); L.append('wait 40')
            tap((scan_row['x']+scan_row['w']//2, scan_row['y']+scan_row['h']//2), j, 80)
            mark('pick_scan')
    else:
        # fall back: put "scan " then open
        L.append('clr'); L.append('put scan '); L.append('cl 1'); L.append('wait 40'); mark('pick_scan')

    # level 1: tap wifi (first row after reopen with "scan ")
    L.append('clr'); L.append('put scan '); L.append('cl 1'); L.append('wait 60')
    L.append('clkeys')
    # wifi is first token at level 1 — tap first visible row via geometry
    if D['box']:
        bx,by,bw,bh,rowy,lvl,nn,top,vis = D['box']
        # first list row: by + CYD_CL_HEAD(14) + 2; centre +14. row_y is the cmdline, not this.
        wifi_pt = (bx + bw//2, by + 16 + 14)
        # keep vertical jitter inside the 28px row (±6 is ok at centre; skip corners that clip)
        for j in [(0,0),(6,0),(-6,0),(0,4),(0,-4)]:
            L.append('clr'); L.append('put scan '); L.append('cl 1'); L.append('wait 40')
            tap(wifi_pt, j, 80); mark('pick_wifi')

    # <WORD after "scan wifi "
    L.append('clr'); L.append('put scan wifi '); L.append('cl 1'); L.append('wait 40')
    if D['btns']:
        for j in JIT[:5]:
            L.append('put scan wifi '); L.append('cl 1'); L.append('wait 30')
            tap(ctr_btn(D['btns'][1]), j, 80); mark('delword')

    # RUN after "scan wifi "
    L.append('clr'); L.append('put scan wifi '); L.append('cl 1'); L.append('wait 40')
    if D['btns']:
        for j in JIT[:5]:
            L.append('put scan wifi '); L.append('cl 1'); L.append('wait 30')
            tap(ctr_btn(D['btns'][4]), j, 80); mark('run_scan_wifi')

    # --- reticle open/close attack ---
    if D['ret']:
        L.append('cl 0'); L.append('wait 30')
        for j in JIT[:5]:
            tap(ctr_rect(D['ret']), j, 80); mark('open_attack')
            tap(ctr_rect(D['ret']), (0,0), 80); mark('close_attack')

    # --- doubles / drops: rapid taps on ≡ ---
    if D['eq']:
        L.append('cl 0'); L.append('wait 40')
        for hold in HOLD:
            for j in JIT[:3]:
                tap(ctr_rect(D['eq']), j, hold)
            mark('rapid_eq')
            L.append('cl 0'); L.append('wait 40')

    L.append('shot ' + tag + '-done')
    L.append('buf'); mark('done')

    out = run(L, land, tag+'_tour')
    # parse marks
    marks={}; expecting=None; seq=[]
    for l in out:
        m = re.search(r'\[sim\] echo M (\d+)', l) or re.search(r'^\[sim\] M (\d+)', l)
        if m is None:
            m = re.search(r'\[sim\] M (\d+)$', l)
        if 'echo M' in l or (l.startswith('[sim]') and ' M ' in l):
            mm = re.search(r'M (\d+)', l)
            if mm: expecting = int(mm.group(1))
        if expecting is not None and 'buf mode=' in l:
            marks[expecting]=parse_buf([l]); expecting=None

    stats=dict(taps=0,correct=0,misses=0,wrong=0,doubles=0,dropped=0,open_ok=0,open_n=0)
    prev_buf=''; prev_fires=None
    for i,(test,key,extra,ntap) in enumerate(ev):
        b=marks.get(i)
        stats['taps']+=1
        if not b:
            stats['misses']+=1; continue
        if prev_fires is not None:
            d=b['fires']-prev_fires
            if d>ntap: stats['doubles']+=d-ntap
            elif d<ntap: stats['dropped']+=ntap-d
        prev_fires=b['fires']
        if test in ('open_line','open_eq'):
            stats['open_n']+=1
            if b['cl']==1 and b['mode']==0:
                stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_list':
            if b['cl']==0 and b['mode']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='no_osk':
            # must never open letter OSK
            if b['mode']==0: stats['correct']+=1
            else: stats['wrong']+=1  # OSK opened — fail hard
        elif test=='pick_scan':
            if b['cl']==1 and (b['buf'].startswith('scan') or b['lvl']>=1): stats['correct']+=1
            else: stats['misses']+=1
        elif test=='pick_wifi':
            if 'wifi' in b['buf'] and b['mode']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='delword':
            # after <WORD on "scan wifi ", expect "scan " and lvl 1
            if b['buf'].rstrip() in ('scan','scan ') and b['mode']==0: stats['correct']+=1
            elif b['lvl']==1 and 'wifi' not in b['buf']: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='run_scan_wifi':
            ok = b['cl']==0 and (b['buf']=='' or 'sweep' in b['log'] or 'wifi' in b['log'].lower() or 'ok' in b['log'])
            if ok: stats['correct']+=1
            else: stats['misses']+=1
            stats['scan_log']=b['log']; stats['scan_buf']=b['buf']
        elif test=='open_attack':
            stats['open_n']+=1
            if b['ovl']==2: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_attack':
            if b['ovl']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test in ('base','cl_open','scroll','rapid_eq','done'):
            if b['mode']==0: stats['correct']+=1
            else: stats['wrong']+=1
        else:
            stats['correct']+=1
        prev_buf = b['buf'] if b else prev_buf
    return stats, marks, ev, out

TABLE=[]
for name,setup in SCREENS:
    for land,ori in [(False,'P'),(True,'L')]:
        tag=f'{name}-{ori}'
        print('DISCOVER', tag, flush=True)
        D=discover(setup, land, tag)
        print('  plate', D['plate'], 'eq', D['eq'], 'ret', D['ret'],
              'btns', len(D['btns']), 'rows', len(D['rows']), flush=True)
        print('TOUR', tag, flush=True)
        stats,marks,ev,out=tour(setup, land, tag, D)
        stats['tag']=tag
        stats['padchk']=D['padchk']
        stats['n_btns']=len(D['btns'])
        print('  pads', D['padchk'], flush=True)
        TABLE.append(stats)
        print(' ', {k:stats[k] for k in stats if k not in ('scan_log','padchk')}, flush=True)

json.dump(TABLE, open(os.path.join(OUT,'FUNCTION.json'),'w'), indent=2)
print('\nFUNCTION TABLE')
print(f'{"tag":12} taps corr miss wrong dbl drop open_ok/n scan_log')
for s in TABLE:
    print(f"{s['tag']:12} {s['taps']:4} {s['correct']:4} {s['misses']:4} {s['wrong']:4} "
          f"{s['doubles']:3} {s['dropped']:4} {s['open_ok']:3}/{s['open_n']:<3} "
          f"{s.get('scan_log','')[:40]}")
tot=dict(taps=0,correct=0,misses=0,wrong=0,doubles=0,dropped=0,open_ok=0,open_n=0)
for s in TABLE:
    for k in tot: tot[k]+=s.get(k,0)
print('TOTAL', tot, 'rate', round(tot['correct']/max(1,tot['taps']),4))
pads_ok = all(s['padchk']['eq_ok'] and s['padchk']['ret_ok'] and s['padchk']['btn_ok'] for s in TABLE)
print('PADS', 'all ok' if pads_ok else 'FAIL')
# mode must never be 1
osk_leaks = sum(s['wrong'] for s in TABLE)
print('OSK_LEAKS (mode!=0 counted as wrong)', osk_leaks)
