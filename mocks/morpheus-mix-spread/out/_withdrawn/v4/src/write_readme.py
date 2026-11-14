#!/usr/bin/env python3
"""Write README.md for the v4 morpheus-mix outputs from gate.json + layout18.json (numbers only from the real-sim gates)."""
import json, os
R = '/workspace/out/morpheus-mix'; g = json.load(open(R + '/gate.json')); L = json.load(open(R + '/src/layout18.json'))
G = {x['name']: x for x in g['gates']}
def find(pfx): return next(v for k, v in G.items() if k.startswith(pfx))
NAMES = g['devices']; THREAT = [0,0,2,2,1,1,1,0,0,1, 1,2,2,1,2,1,0,2]; TN = ["HIGH","MEDIUM","LOW"]; LOOK = [{0:1,1:3,2:2}[t] for t in THREAT]
LN = {1: 'T1 code glass (green)', 3: 'T3 see-through wood', 2: 'T2 see-through white'}; HF = {1: L['H'][0], 3: L['H'][1], 2: L['H'][2]}
vis = find('emblem visibility (320'); tap0 = find('tap: every pillar keeps'); p10 = find('tap: p10'); idl = find('emblem visibility (idle still)')
esz = find('emblems: idle emblem'); stab = find('temporal stability'); ui = find('opaque UI'); hgt = find('heights: every style'); st = g['idle_still']
fails = [x['name'] for x in g['gates'] if not x['pass']]
o = []
o += ['# MORPHEUS MIX SPREAD v4 (mock, 2026-11-14)', '',
      '**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number below comes from the real 240x320 RGB565 sim',
      '(`tree/tools/livescan/sim_live.cpp`, Gibson engine, built with zig c++). Earlier versions: `_withdrawn/v1/` (20 pillars), `_withdrawn/v2/`',
      '(stump heights), `_withdrawn/v3a/` (first 1.00/0.90/0.80 run, never delivered), `_withdrawn/v3/` (the 9:22 AM ET renders Mobandy rejected at 9:27 AM ET).', '',
      '## What changed in v4 (Mobandy 9:27 - 9:29 AM ET)', '',
      '1. **Taller pillars.** ONE base height H = %.2f (was 1.26).' % L['base_H'],
      '   - Factors are bigger steps but no stumps: green T1 1.00 H, wood T3 0.80 H, white T2 0.62 H.',
      '   - World heights: T1 %.3f, T3 %.3f, T2 %.3f.' % (HF[1], HF[3], HF[2]),
      '   - Against the rejected 9:22 heights: T1 x%.2f, T3 x%.2f, T2 x%.2f. On-screen height scales the same at equal depth.' % tuple(hgt['x_v3'][k] for k in ('T1_code_glass', 'T3_sentinel_post', 'T2_construct_light')),
      '   - Median front-face height over 320 idle samples: T1 %d px, T3 %d px, T2 %d px (at 1x).' % tuple(hgt['median_front_face_h_px'][k] for k in ('T1_code_glass', 'T3_sentinel_post', 'T2_construct_light')),
      '2. **Sharper pillar definition.**',
      '   - Removed the soft outer glows on T1 and T2, the blurred reflection band on T1, the soft rising light band on T2, and the blurred sheen and warm glow on T3.',
      '   - T1 glass is opaque with one crisp highlight column. T3 grain is two-tone. Edges are 1 px solid lines.',
      '3. **Bigger, simpler, crisp emblems.**',
      '   - Emblems are binary masks rasterised at the final size and drawn pixel-aligned and fully opaque: a 1 px outline, a body and a detail. No anti-aliasing or blur.',
      '   - Each idle emblem is as large as the face allows (face width - 1), in 2 px steps with hysteresis.',
      '   - New silhouettes: HACK is a skull (was a padlock), FLIPPER is the dolphin (was the pocket tool), KEYFOB is a car key fob with a flip blade (was a Tile square). The others were already recognisable.',
      '   - Idle emblem sizes in the still (px at 1x): ' + ', '.join('%s %d' % (k, v['S']) for k, v in esz['per_pillar'].items()) + '.',
      '   - The six nearest pillars: ' + ', '.join('%s %d' % kv for kv in esz['front6'].items()) + '. Median %d px.' % esz['median_S'],
      '   - **Not met: ~22-28 px.** The faces are only %s px wide (median, T1 / T3 / T2) at the depths that keep 18 pillars apart and their tops inside the field.' % ' / '.join('%d' % hgt['median_front_face_w_px'][k] for k in ('T1_code_glass', 'T3_sentinel_post', 'T2_construct_light')),
      '   - To get this far, the pillar width scale `MO11_TW` was raised from 1.35 to %.1f, the same width the CT skin uses. This is a change outside the named list. Revert it in `layout18.json` (`tw`) if unwanted.' % L['tw'],
      '4. **No flicker.**',
      '   - Pillar and emblem positions are integer pixels. The emblem has no bob and no alpha ramps, and it is drawn again after the edge lines so an edge can never cut it.',
      '   - The idle emblem size only changes when the face width leaves a 3 px band.',
      '   - New gate, temporal stability: 40 consecutive idle frames, %d pillar-frame pairs. Mean |dY| of emblem pixels = %s; worst pair %s (%s). Size changes in 40 frames: %s.' % (stab['pairs'], stab['mean_abs_dY'], stab['worst_pair_mean_dY'], stab['worst'], {k: v for k, v in stab['size_changes_in_40_frames'].items() if v}),
      '5. **White T2 is see-through too.** The white shell is translucent (front ~47%, sides ~59%, cap ~66%), so the room and the pillars behind show through. Its edges are crisp opaque green. Green T1 is unchanged in style.',
      '6. **Opaque UI.**',
      '   - Only the wood and white shells are translucent. Emblems, on-tower folder glyphs and file line-items are drawn at 100%: hard clip, no fade rows.',
      '   - On the see-through shells the text has a 1 px opaque outline (white on T2, dark on T3) so it stays readable.',
      '   - The right-hand folder pull-down is now 100% opaque: body, header, tab and the drop-in outline.',
      '   - New gate: the sim counts every UI pixel. %s drawn, %s below full alpha.' % (ui['ui_px_drawn'], ui['ui_px_translucent']),
      '7. **Placement re-picked** for the new heights and width by `src/opt18/opt18d.py`, seed %d. Every pillar top stays at y >= 38. The v11 drift law, camera sway and breathing-off are unchanged.' % L['seed'],
      '', 'Held as is (per the 9:27 order): the folder text from `folders18.py` (`folders.md`), the tap rule, and the chrome text (it still reads `HEIGHT = EXAMPLE SIGNAL` / `EX.DATA`).', '',
      '## Idle still', '',
      '`idle_1x.png` / `idle_3x.png` show **idle sample %d of %d** (0.3 s apart).' % (st['sample_index'], st['of']),
      '- **How it was picked:** %s.' % st['rule'],
      '- **Quality:** %s (median %s, range %s to %s). The pool has %d of the 320 samples.' % (st['quality'], st['quality_median'], st['quality_min'], st['quality_max'], st.get('clean_pool', 0)),
      '', '## Per pillar (real sim)', '',
      '| # | device | threat | look | H | sx | z0 | idle emblem S | idle px | vis % | min sq | p10 |', '|---|---|---|---|---|---|---|---|---|---|---|---|']
for i, n in enumerate(NAMES):
    o.append('| %d | %s | %s | %s | %.3f | %.1f | %.3f | %d | %d | %d | %d | %g |' % (i, n, TN[THREAT[i]], LN[LOOK[i]], HF[LOOK[i]], L['sx'][i], L['z0'][i], esz['per_pillar'][n]['S'], idl['px'][n],
             round(100 * vis['per_pillar'][n]['frac']), tap0['min_square'][n], p10['p10'][n]))
o += ['', '## Gate: %d/%d (`gate.json`)' % (g['passed'], g['total']), ''] + ['- FAIL: ' + f for f in fails] + ['',
      'The tap failures are structural, as in v2 and v3. The tap rule (`mo_hit`, nearest centre line) was not changed, by order.',
      'The middle depth band gets only a horizontal slice of the field between the tall back tops and the near bottoms. With 18 pillars across 240 px, the 10th-percentile tap square for those pillars sits around 7 to 15 px.',
      'Per-pillar numbers for the 3 failing gates (also in `gate.json` under `per_pillar_numbers`, from the same 320 samples):',
      '- Emblem mean visible fraction below 0.5: ' + ', '.join('%s %.3f' % (k, v['frac']) for k, v in vis['per_pillar'].items() if v['frac'] < 0.5) + '. The table above lists all 18.',
      '- Samples with a tap cell below 100%: ' + ', '.join('%s %.1f%%' % (k, 100 - 100 * v) for k, v in tap0['zero_frac'].items()) + '. Every other pillar is at 100%.',
      'Below 20 px p10: ' + ', '.join('%s %g' % (k, v) for k, v in p10['p10'].items() if v < 20) + '.', '',
      '## Files', '', '| file | content |', '|---|---|',
      '| `idle_1x.png`, `idle_3x.png` | idle still (sample %d) |' % st['sample_index'],
      '| `closeup_deauth_high_3x.png` | T1 code glass close-up |', '| `closeup_airtag_medium_3x.png` | see-through wood close-up (AIR TAG) |',
      '| `closeup_flock_low_3x.png` | see-through white close-up (FLOCK) |', '| `closeup_raven_new_3x.png` | see-through white close-up (RAVEN) |',
      '| `emblems_old_vs_new.png` | 18 emblems: old 16x16 art vs the new crisp masks at 16 / 32 / 40 px |',
      '| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened; @3x 720x960 h264 |',
      '| `folders.md` | all 18 devices\' folder + line-item text (held as is) |', '| `gate.json`, `SHA256SUMS` | gates and checksums |',
      '| `src/` | `patch_tree.py`, `make_mix_inc.py` -> `mo_mix.inc`, `mo_emblem.h`, `folders18.py`, `layout18.json`, `opt18/`, `package.py`, `drive.py`, `touch20.py`, `mix.json`, `emb_sheet.cpp`, `write_readme.py` |',
      '| `tree_patch/` | the two patched headers |', '']
open(R + '/README.md', 'w').write('\n'.join(o)); print('README.md written', len(o), 'lines')
