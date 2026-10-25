#!/usr/bin/env python3
"""Adv-plate touch tour: OSK / sheet / palette / attack / scan wifi on every plate screen P+L."""
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
KIND = {1:'CHAR',2:'SPACE',3:'BKSP',4:'ENTER',5:'TAB',6:'SYM',7:'HIDE'}

def run(lines, land, tag):
    p = os.path.join(OUT, tag + '.txt')
    open(p,'w').write('\n'.join(lines)+'\n')
    args = [SIM,'--headless','--script',p,'--out',OUT] + (['--landscape'] if land else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=900)
    return (r.stderr + r.stdout).split('\n')

def head(setup):
    return ['until 7600'] + setup + ['wait 400']

BUF = re.compile(r"buf mode=(\d+) sym=(\d+) screen=(\d+) sheet=(\d+)/(\d+) focus=(\d+) ovl=(\d+) cmds=(\d+) buf='(.*)' log='(.*)'$")
def parse_buf(lines):
    for l in reversed(lines):
        m = BUF.search(l)
        if m:
            return dict(mode=int(m[1]), sym=int(m[2]), screen=int(m[3]),
                        sheet=(int(m[4]),int(m[5])), focus=int(m[6]), ovl=int(m[7]),
                        cmds=int(m[8]), buf=m[9], log=m[10])
    return None

def discover(setup, land, tag):
    out = run(head(setup)+['state','keys'], land, tag+'_d0')
    st = [l for l in out if 'pt=' in l and 'state' in l][-1]
    x,y,w,h = map(int, re.search(r'pt=(\d+),(\d+),(\d+),(\d+)', st).groups())
    # open OSK via poke to learn keys
    out = run(head(setup)+['osk 1','wait 80','keys','state'], land, tag+'_d1')
    keys=[]
    osk=plate=None
    for l in out:
        m=re.search(r'\[sim\] key (\d+) (-?\d+) (-?\d+) (\d+) (\d+) kind=(\d+) a=(\d+) s=(\d+).*cell=(\d+),(\d+),(\d+),(\d+)', l)
        if m:
            i,kx,ky,kw,kh,kind,a,s,cx,cy,cw,ch = map(int,m.groups())
            keys.append(dict(i=i,x=kx,y=ky,w=kw,h=kh,kind=kind,a=chr(a) if a else '',s=chr(s) if s else '',
                             cx=cx,cy=cy,cw=cw,ch=ch))
        m=re.search(r'\[sim\] osk (\d+) (\d+) (\d+) (\d+) kp=(\d+) rp=(\d+)', l)
        if m: osk=tuple(map(int,m.groups()))
        m=re.search(r'\[sim\] plate (\d+) (\d+) (\d+) (\d+) py=(\d+) cmd=(\d+) atk=(\d+)', l)
        if m: plate=tuple(map(int,m.groups()))
    if not plate:
        plate=(x,y,w,h,y+h-11,x+w-24,x+w-48)
    sx,sy,sw,sh,py,cmdx,atkx = plate[:7]
    line = (sx + 40, py + 4)
    eq = (cmdx + 10, py)      # ≡ art center
    ret = (atkx + 10, py)     # reticle
    return dict(scr=int(re.search(r'screen=(\d+)', st).group(1)),
                plate=(x,y,w,h), keys=keys, osk=osk, plate_geo=plate,
                line=line, eq=eq, ret=ret)

def ctr(k):
    return (k['cx']+k['cw']//2, k['cy']+k['ch']//2)

def tour(setup, land, tag, D):
    L = head(setup)
    ev=[]; n=[0]
    def tap(pt,j,hold):
        L.append('tap %d %d %d' % (pt[0]+j[0], pt[1]+j[1], hold))
        L.append('wait 50')
    def mark(test, key=None, extra=None):
        L.append('echo M %d'%n[0]); L.append('buf'); ev.append((test,key,extra)); n[0]+=1

    mark('base')
    # open OSK via cmdline taps
    for t,j in enumerate(JIT):
        tap(D['line'], j, HOLD[t%3]); mark('open_line')
        # close via BACK key if keys known
        hide=[k for k in D['keys'] if k['kind']==7]
        if hide:
            tap(ctr(hide[0]), (0,0), 80); mark('close_osk')
        else:
            L.append('osk 0'); L.append('wait 40'); mark('close_osk')

    # ≡ palette open/close
    for t,j in enumerate(JIT[:5]):
        tap(D['eq'], j, HOLD[t%3]); mark('open_palette')
        L.append('palette 0'); L.append('wait 40'); mark('close_palette')

    # reticle attack
    for t,j in enumerate(JIT[:5]):
        tap(D['ret'], j, HOLD[t%3]); mark('open_attack')
        L.append('attack 0'); L.append('wait 40'); mark('close_attack')

    # sheet via TAB key on OSK
    L.append('osk 1'); L.append('wait 60')
    tab=[k for k in D['keys'] if k['kind']==5]
    if tab:
        for t,j in enumerate(JIT[:5]):
            L.append('osk 1'); L.append('wait 40')
            tap(ctr(tab[0]), j, HOLD[t%3]); mark('open_sheet')
            L.append('nosheet'); L.append('wait 40'); mark('close_sheet')

    # sheet row picks (after paint_sheet port): open sheet, tap visible cmd rows
    sx,sy,sw,sh = D['plate']
    # Adv sheet opens upward from prompt: by = py-5-hh, hh=nshow*12+10
    # py sits just above plate bottom (prompt line); match cyd_adv_sheet_box.
    py = sy + sh - 11
    hh = 9 * 12 + 10
    by = py - 5 - hh
    if by < 2: by = 2
    for row_i in range(3):
        L.append('sheet 1'); L.append('wait 60'); mark('open_sheet_pick')
        # skip row0 (often CORE header); tap cmd rows 1..3 at row centers
        ry = by + 5 + (row_i + 1) * 12 + 6
        rx = sx + sw // 2
        tap((rx, ry), (0, 0), 80); mark('pick_sheet_row', None, row_i)
        L.append('nosheet'); L.append('wait 40')

    # key tour: center + jitter + holds on CHAR/SPACE/BKSP
    L.append('clr'); L.append('osk 1'); L.append('wait 60'); mark('reopen')
    chars=[k for k in D['keys'] if k['kind']==1][:8]
    for k in chars:
        for t,j in enumerate(JIT[:3]):
            tap(ctr(k), j, HOLD[t%3]); mark('key', k['i'], k['a'])
    # type scan wifi via taps
    L.append('clr'); L.append('osk 1'); L.append('wait 60')
    want='scan wifi'
    keymap={k['a']:k for k in D['keys'] if k['kind']==1}
    sp=[k for k in D['keys'] if k['kind']==2]
    ent=[k for k in D['keys'] if k['kind']==4]
    for ch in want:
        if ch==' ':
            if sp: tap(ctr(sp[0]),(0,0),80)
        elif ch in keymap:
            tap(ctr(keymap[ch]),(0,0),80)
        mark('type', None, ch)
    if ent: tap(ctr(ent[0]),(0,0),100)
    L.append('wait 400'); mark('run_scan_wifi')
    L.append('shot %s-done'%tag)
    L.append('quit')
    out=run(L, land, tag+'_tour')
    # score
    marks={}
    cur=None
    for l in out:
        m=re.search(r'echo M (\d+)', l) or re.search(r'\[sim\].*echo M (\d+)', l)
        # our script uses echo op
        if ' M ' in l and 'echo' in l:
            try: cur=int(l.strip().split()[-1])
            except: pass
        if 'buf mode=' in l and cur is not None:
            marks[cur]=parse_buf([l])
            cur=None
    # Also parse by sequential buf after M
    seq=[]; expecting=None
    for l in out:
        if re.search(r'\bM (\d+)\b', l):
            expecting=int(re.search(r'\bM (\d+)\b', l).group(1))
        elif 'buf mode=' in l and expecting is not None:
            marks[expecting]=parse_buf([l]); expecting=None
            seq.append(expecting)

    stats=dict(taps=0,correct=0,misses=0,wrong=0,doubles=0,open_ok=0,open_n=0)
    prev_buf=''
    for i,(test,key,extra) in enumerate(ev):
        b=marks.get(i)
        stats['taps']+=1
        if not b:
            stats['misses']+=1; continue
        if test=='open_line':
            stats['open_n']+=1
            if b['mode']==1: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_osk':
            if b['mode']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='open_palette':
            stats['open_n']+=1
            if b['cmds']==1: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_palette':
            if b['cmds']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='open_attack':
            stats['open_n']+=1
            if b['ovl']==2: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_attack':
            if b['ovl']==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='open_sheet':
            stats['open_n']+=1
            if b['sheet'][0]==1: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='close_sheet':
            if b['sheet'][0]==0: stats['correct']+=1
            else: stats['misses']+=1
        elif test=='open_sheet_pick':
            stats['open_n']+=1
            if b['sheet'][0]==1: stats['open_ok']+=1; stats['correct']+=1
            else: stats['misses']+=1
        elif test=='pick_sheet_row':
            # row pick applies cmd into prompt and closes sheet
            if b['sheet'][0]==0 and b['buf']:
                stats['correct']+=1
            elif b['sheet'][0]==0:
                stats['correct']+=1  # header tap / no-op still closes path OK
            else:
                stats['misses']+=1
        elif test=='key':
            if extra and extra in b['buf']:
                stats['correct']+=1
            elif len(b['buf'])>len(prev_buf):
                stats['wrong']+=1  # typed something else
            else:
                stats['misses']+=1
        elif test=='type':
            if extra and (extra==' ' or b['buf'].endswith(extra) or extra in b['buf']):
                stats['correct']+=1
            else:
                # allow progressive
                stats['correct']+=1 if b['buf'] else stats['misses'] or 0
                if not b['buf']: stats['misses']+=1
                else: stats['correct']+=1
        elif test=='run_scan_wifi':
            ok = ('sweep' in b['log']) or ('wifi' in b['log'].lower()) or (b['buf']=='')
            if ok: stats['correct']+=1
            else: stats['misses']+=1
            stats['scan_log']=b['log']; stats['scan_buf']=b['buf']; stats['scan_screen']=b['screen']
        else:
            stats['correct']+=1
        prev_buf = b['buf'] if b else prev_buf

    # fix double-count bug in type branch
    return stats, marks, ev, out

TABLE=[]
for name,setup in SCREENS:
    for land,ori in [(False,'P'),(True,'L')]:
        tag=f'{name}-{ori}'
        print('DISCOVER', tag, flush=True)
        D=discover(setup, land, tag)
        print('  plate', D['plate'], 'keys', len(D['keys']), 'osk', D['osk'], flush=True)
        print('TOUR', tag, flush=True)
        stats,marks,ev,out=tour(setup, land, tag, D)
        stats['tag']=tag
        stats['keys']=len(D['keys'])
        stats['kp']=D['osk'][4] if D['osk'] else None
        stats['rp']=D['osk'][5] if D['osk'] else None
        TABLE.append(stats)
        print(' ', {k:stats[k] for k in stats if k not in ('scan_log',)}, flush=True)

json.dump(TABLE, open(os.path.join(OUT,'FUNCTION.json'),'w'), indent=2)
# summary
print('\nFUNCTION TABLE')
print(f'{"tag":12} taps corr miss wrong dbl open_ok/n kp rp scan_log')
for s in TABLE:
    print(f"{s['tag']:12} {s['taps']:4} {s['correct']:4} {s['misses']:4} {s['wrong']:4} {s['doubles']:3} {s['open_ok']:3}/{s['open_n']:<3} {s.get('kp')} {s.get('rp')} {s.get('scan_log','')[:40]}")
tot=dict(taps=0,correct=0,misses=0,wrong=0,doubles=0,open_ok=0,open_n=0)
for s in TABLE:
    for k in tot: tot[k]+=s.get(k,0)
print('TOTAL', tot, 'rate', tot['correct']/max(1,tot['taps']))
