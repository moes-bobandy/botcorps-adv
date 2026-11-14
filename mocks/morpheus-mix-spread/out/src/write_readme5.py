#!/usr/bin/env python3
"""Write README.md for the v5 morpheus-mix outputs from gate.json + layout18.json + emb_colors.py (numbers only from the real-sim gates)."""
import json, sys, os
R = '/workspace/out/morpheus-mix'; sys.path.insert(0, R + '/src'); import emb_colors as EC
g = json.load(open(R + '/gate.json')); L = json.load(open(R + '/src/layout18.json')); G = {x['name']: x for x in g['gates']}
def find(pfx): return next(v for k, v in G.items() if k.startswith(pfx))
NAMES = g['devices']; THREAT = [0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TN = ["HIGH","MEDIUM","LOW"]; GRP = {0: 1.00, 1: 0.80, 2: 0.62}
vis = find('emblem visibility (320'); tap0 = find('tap: every pillar keeps'); p10 = find('tap: p10'); idl = find('emblem visibility (idle still)')
esz = find('emblems: idle emblem'); stab = find('temporal stability'); ui = find('opaque UI'); col = find('emblem colours'); fl = find('floor coverage'); st = g['idle_still']
fails = [x['name'] for x in g['gates'] if not x['pass']]; COLS = EC.colors(L); NOTE = open(R + '/src/visual_v5.txt').read().strip() if os.path.exists(R + '/src/visual_v5.txt') else ''
o = ['# MORPHEUS MIX SPREAD v5b (mock, 2026-11-14)', '',
     '**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number comes from the real 240x320 RGB565 sim (same pipeline as v4).',
     'Earlier versions are in `_withdrawn/v1` .. `v4` (v4 = the 10:15 AM ET renders, rejected 11:51 AM ET; v5a = the first v5 render, rejected 12:42 PM ET, see `_withdrawn/v5a/NOTE.txt`).', '',
     '## What changed in v5 (Mobandy 11:51 / 11:52 AM ET)', '',
     '1. **All wood.** All 18 pillar bodies use the see-through wood (T3) body. There is no green code glass or white light body any more.',
     '   - In `mo11_tower_fx`, the body look is forced to 3. `MO_LOOKOF` now only picks the height group.',
     '   - The opaque emblem, on-tower folder glyphs and file line-items are unchanged; only the body is see-through.',
     '2. **Heights are as v4:** H = 2.6. Former green group 1.00 (2.6), former wood group 0.80 (2.08), former white group 0.62 (1.612). No breathing.',
     '3. **A colour per device.** Each emblem gets its own saturated colour (`src/emb_colors.py`):',
     '   - 18 colours with 18 distinct hues (min hue step %s deg; v5a had crimson and rose both at 340 deg, rose is now #cc5c6a at 352 deg). They are dealt by a swap search that maximises the smallest deltaE between towers that are screen neighbours at any of 64 sampled idle times.' % col['min_hue_step_deg'],
     '   - Emblems are still crisp binary masks with a 1 px dark outline, drawn pixel-aligned and opaque.',
     '   - Min pairwise deltaE (CIE76) = %s (%s). Min between screen neighbours = %s (%s). Min distance from the wood tones = %s.' % (col['min_pair_deltaE'], ' / '.join(col['min_pair']), col['min_screen_adjacent_deltaE'], ' / '.join(col['adjacent_pair']), col['min_deltaE_to_wood']),
     '4. **Width reverted:** `tw` is back to **%s** (v4 had 2.2, which was unrequested).' % L.get('tw', 1.35),
     "5. **Spread (11:52 AM / 12:42 PM ET) - NOT achieved.** The layout was re-picked (`src/opt18/opt18e.py`, seed %s) on a corrected occlusion model (a nearer pillar's box now starts at min(front top, back top); v5a missed the front-face top strip, which also caused the false emblem 'flicker'), with an emblem visibility floor, empty-floor and screen-third balance terms." % L.get('seed'),
     '   - Largest empty floor square, whole visible floor (rows 140-285): v4 idle **107 px**, v5a idle **71 px**, v5b idle **%s px** (median over 320 samples %s, worst %s).' % (fl['whole_floor_idle_px'], fl['whole_floor_median_px'], fl['whole_floor_max_px']),
     '   - Standable floor (rows 140-251; a base lower than y 251 needs z0 < 0.95 and the tower top leaves the field): v5a idle **43 px**, v5b idle **%s px** (median %s, worst %s). Bases per screen third L/M/R: v5a 6/8/4, v5b %s; front-row bases per third %s.' % (fl['standable_floor_idle_px'], fl['standable_floor_median_px'], fl['standable_floor_max_px'], '/'.join(map(str, fl['bases_per_third_LMR'])), '/'.join(map(str, fl['front_row_bases_per_third_LMR']))),
     '   - The front floor stays bare: towers on it are 35-45 px wide and hide the emblems behind them. With camera, heights, tw and drift locked, ~40 optimizer runs never got the whole-floor square below ~70 px while keeping every emblem >= 0.5 visible.',
     '   - The height group order (green-group back, white-group front), the camera, sway and the v11 drift law are unchanged.', '',
     '## Emblem colours', '', '| device | threat / height | colour | hex | hue |', '|---|---|---|---|---|']
for c, t in zip(COLS, THREAT): o.append('| %s | %s / %.2f | %s | #%s | %d deg |' % (c['device'], TN[t], GRP[t], c['name'], c['body'], c['hue_deg']))
o += ['', '## What the idle still looks like', '', NOTE, '',
      '## Idle still', '', '`idle_1x.png` / `idle_3x.png` = idle sample **%d of %d** (%s; quality %s, median %s).' % (st['sample_index'], st['of'], st['rule'], st['quality'], st['quality_median']), '',
      '## Per pillar (real sim)', '', '| # | device | height | sx | z0 | emblem S | idle px | vis %% | tap cell %% | min sq | p10 |'.replace('%%', '%'), '|---|---|---|---|---|---|---|---|---|---|---|']
for i, n in enumerate(NAMES):
    o.append('| %d | %s | %.2f | %.1f | %.3f | %d | %d | %d | %.1f | %d | %g |' % (i, n, GRP[THREAT[i]], L['sx'][i], L['z0'][i], esz['per_pillar'][n]['S'], idl['px'][n], round(100 * vis['per_pillar'][n]['frac']),
             100 - 100 * tap0['zero_frac'].get(n, 0), tap0['min_square'][n], p10['p10'][n]))
o += ['', '## Gate: %d/%d (`gate.json`)' % (g['passed'], g['total']), ''] + (['- FAIL: ' + f for f in fails] or ['- all pass']) + ['',
      'Temporal stability: %d pairs, mean |dY| %s, worst %s. Opaque UI: %s UI pixels, %s translucent. The tap rule was not changed.' % (stab['pairs'], stab['mean_abs_dY'], stab['worst_pair_mean_dY'], ui['ui_px_drawn'], ui['ui_px_translucent']), '',
      '## Files', '', '| file | content |', '|---|---|', '| `idle_1x.png`, `idle_3x.png` | idle still |',
      '| `closeup_deauth_high_3x.png` | close-up, height group 1.00 |', '| `closeup_airtag_medium_3x.png` | close-up, height group 0.80 |', '| `closeup_flock_low_3x.png` | close-up, height group 0.62 |',
      '| `emblem_colors.png` | 18 emblems in their colours on wood (16 / 24 / 40 px) |', '| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened; 720x960 h264 |',
      '| `folders.md` | folder text (unchanged from v4) |', '| `gate.json`, `SHA256SUMS` | gates, checksums |', '| `src/`, `tree_patch/` | generator sources, patched headers |', '']
open(R + '/README.md', 'w').write('\n'.join(o)); print('README.md written', len(o))
