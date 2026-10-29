#!/usr/bin/env python3
"""Type 'scan wifi' + OK by taps (key centers) on every keyboard screen, P and L; shot before/after OK."""
import json, os, sys, re
sys.argv = [sys.argv[0], sys.argv[1] if len(sys.argv) > 1 else '/tmp/typeproof']
exec(open(os.path.join(os.path.dirname(__file__), 'touch_tour.py')).read().split("if __name__ == '__main__':")[0])
for land in (False, True):
    o = 'L' if land else 'P'
    for nm, setup in SCREENS:
        tag = '%s_%s' % (o, nm)
        D = discover(setup, land, tag)
        K = {k['a']: k for k in D['keys'] if k['kind'] == 1}
        spc = [k for k in D['keys'] if k['kind'] == 2][0]
        ent = [k for k in D['keys'] if k['kind'] == 4][0]
        L = head(setup) + ['put ', 'tap %d %d' % D['abc'], 'wait 100']
        for ch in 'scan wifi':
            k = spc if ch == ' ' else K[ch]
            L += ['tap %d %d' % ctr(k), 'wait 40']
        L += ['buf', 'shot typed_' + tag, 'tap %d %d' % ctr(ent), 'wait 500', 'buf', 'state', 'shot after_' + tag]
        out = run(L, land, tag + '_type')
        b = [parse_buf(l) for l in out if ' buf mode=' in l]
        st = [l for l in out if ' state screen=' in l][-1]
        print('%-8s typed=%r  after OK: screen %d->%d buf=%r log=%r' % (tag, b[0]['buf'], b[0]['screen'], b[1]['screen'], b[1]['buf'], b[1]['log']))
