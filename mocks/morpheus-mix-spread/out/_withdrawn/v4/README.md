# MORPHEUS MIX SPREAD v4 (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number below comes from the real 240x320 RGB565 sim
(`tree/tools/livescan/sim_live.cpp`, Gibson engine, built with zig c++). Earlier versions: `_withdrawn/v1/` (20 pillars), `_withdrawn/v2/`
(stump heights), `_withdrawn/v3a/` (first 1.00/0.90/0.80 run, never delivered), `_withdrawn/v3/` (the 9:22 AM ET renders Mobandy rejected at 9:27 AM ET).

## What changed in v4 (Mobandy 9:27 - 9:29 AM ET)

1. **Taller pillars.** ONE base height H = 2.60 (was 1.26).
   - Factors are bigger steps but no stumps: green T1 1.00 H, wood T3 0.80 H, white T2 0.62 H.
   - World heights: T1 2.600, T3 2.080, T2 1.612.
   - Against the rejected 9:22 heights: T1 x2.06, T3 x1.83, T2 x1.60. On-screen height scales the same at equal depth.
   - Median front-face height over 320 idle samples: T1 77 px, T3 68 px, T2 61 px (at 1x).
2. **Sharper pillar definition.**
   - Removed the soft outer glows on T1 and T2, the blurred reflection band on T1, the soft rising light band on T2, and the blurred sheen and warm glow on T3.
   - T1 glass is opaque with one crisp highlight column. T3 grain is two-tone. Edges are 1 px solid lines.
3. **Bigger, simpler, crisp emblems.**
   - Emblems are binary masks rasterised at the final size and drawn pixel-aligned and fully opaque: a 1 px outline, a body and a detail. No anti-aliasing or blur.
   - Each idle emblem is as large as the face allows (face width - 1), in 2 px steps with hysteresis.
   - New silhouettes: HACK is a skull (was a padlock), FLIPPER is the dolphin (was the pocket tool), KEYFOB is a car key fob with a flip blade (was a Tile square). The others were already recognisable.
   - Idle emblem sizes in the still (px at 1x): DEAUTH 12, HACK 14, FLOCK 16, DRONE 20, AIR TAG 16, KEYFOB 16, META 14, SKIMMER 12, FLIPPER 14, SAMSUNG 14, AXON 16, RAVEN 16, ALPR 16, CAMERA 14, RING 20, GOOGLE TAG 14, EVIL TWIN 12, IBEACON 22.
   - The six nearest pillars: IBEACON 22, RING 20, DRONE 20, AXON 16, ALPR 16, RAVEN 16. Median 15 px.
   - **Not met: ~22-28 px.** The faces are only 16 / 17 / 20 px wide (median, T1 / T3 / T2) at the depths that keep 18 pillars apart and their tops inside the field.
   - To get this far, the pillar width scale `MO11_TW` was raised from 1.35 to 2.2, the same width the CT skin uses. This is a change outside the named list. Revert it in `layout18.json` (`tw`) if unwanted.
4. **No flicker.**
   - Pillar and emblem positions are integer pixels. The emblem has no bob and no alpha ramps, and it is drawn again after the edge lines so an edge can never cut it.
   - The idle emblem size only changes when the face width leaves a 3 px band.
   - New gate, temporal stability: 40 consecutive idle frames, 220 pillar-frame pairs. Mean |dY| of emblem pixels = 0.0; worst pair 0.0 (None). Size changes in 40 frames: {'FLIPPER': 1}.
5. **White T2 is see-through too.** The white shell is translucent (front ~47%, sides ~59%, cap ~66%), so the room and the pillars behind show through. Its edges are crisp opaque green. Green T1 is unchanged in style.
6. **Opaque UI.**
   - Only the wood and white shells are translucent. Emblems, on-tower folder glyphs and file line-items are drawn at 100%: hard clip, no fade rows.
   - On the see-through shells the text has a 1 px opaque outline (white on T2, dark on T3) so it stays readable.
   - The right-hand folder pull-down is now 100% opaque: body, header, tab and the drop-in outline.
   - New gate: the sim counts every UI pixel. 547638742 drawn, 0 below full alpha.
7. **Placement re-picked** for the new heights and width by `src/opt18/opt18d.py`, seed 64. Every pillar top stays at y >= 38. The v11 drift law, camera sway and breathing-off are unchanged.

Held as is (per the 9:27 order): the folder text from `folders18.py` (`folders.md`), the tap rule, and the chrome text (it still reads `HEIGHT = EXAMPLE SIGNAL` / `EX.DATA`).

## Idle still

`idle_1x.png` / `idle_3x.png` show **idle sample 162 of 320** (0.3 s apart).
- **How it was picked:** median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample.
- **Quality:** 1.2454 (median 1.2329, range 1.1244 to 1.331). The pool has 208 of the 320 samples.

## Per pillar (real sim)

| # | device | threat | look | H | sx | z0 | idle emblem S | idle px | vis % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | HIGH | T1 code glass (green) | 2.600 | 113.2 | 4.198 | 12 | 108 | 84 | 11 | 23 |
| 1 | HACK | HIGH | T1 code glass (green) | 2.600 | 152.6 | 3.861 | 14 | 168 | 79 | 15 | 35 |
| 2 | FLOCK | LOW | T2 see-through white | 1.612 | 37.0 | 3.447 | 16 | 240 | 98 | 11 | 16.8 |
| 3 | DRONE | LOW | T2 see-through white | 1.612 | 81.9 | 3.044 | 20 | 400 | 99 | 7 | 29 |
| 4 | AIR TAG | MEDIUM | T3 see-through wood | 2.080 | 85.2 | 3.605 | 16 | 61 | 47 | 3 | 9 |
| 5 | KEYFOB | MEDIUM | T3 see-through wood | 2.080 | 214.9 | 3.394 | 16 | 32 | 63 | 5 | 9 |
| 6 | META | MEDIUM | T3 see-through wood | 2.080 | 110.3 | 3.773 | 14 | 42 | 53 | 5 | 9 |
| 7 | SKIMMER | HIGH | T1 code glass (green) | 2.600 | 212.2 | 3.912 | 12 | 111 | 77 | 29 | 33 |
| 8 | FLIPPER | HIGH | T1 code glass (green) | 2.600 | 76.6 | 3.934 | 14 | 196 | 89 | 11 | 31 |
| 9 | SAMSUNG | MEDIUM | T3 see-through wood | 2.080 | 18.5 | 3.823 | 14 | 196 | 72 | 0 | 7 |
| 10 | AXON | MEDIUM | T3 see-through wood | 2.080 | 143.9 | 3.257 | 16 | 179 | 53 | 3 | 9 |
| 11 | RAVEN | LOW | T2 see-through white | 1.612 | 113.5 | 3.307 | 16 | 256 | 84 | 7 | 13 |
| 12 | ALPR | LOW | T2 see-through white | 1.612 | 168.6 | 3.300 | 16 | 128 | 83 | 5 | 11 |
| 13 | CAMERA | MEDIUM | T3 see-through wood | 2.080 | 52.0 | 4.005 | 14 | 20 | 57 | 3 | 11 |
| 14 | RING | LOW | T2 see-through white | 1.612 | 203.7 | 2.958 | 20 | 400 | 96 | 17 | 29 |
| 15 | GOOGLE TAG | MEDIUM | T3 see-through wood | 2.080 | 177.9 | 3.860 | 14 | 96 | 46 | 7 | 15 |
| 16 | EVIL TWIN | HIGH | T1 code glass (green) | 2.600 | 25.8 | 4.350 | 12 | 94 | 80 | 13 | 23 |
| 17 | IBEACON | LOW | T2 see-through white | 1.612 | 142.3 | 2.801 | 22 | 484 | 92 | 0 | 31 |

## Gate: 26/29 (`gate.json`)

- FAIL: emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5
- FAIL: tap: every pillar keeps a tap cell in every idle sample (100%)
- FAIL: tap: p10 largest square >= 20 px for every pillar

The tap failures are structural, as in v2 and v3. The tap rule (`mo_hit`, nearest centre line) was not changed, by order.
The middle depth band gets only a horizontal slice of the field between the tall back tops and the near bottoms. With 18 pillars across 240 px, the 10th-percentile tap square for those pillars sits around 7 to 15 px.
Per-pillar numbers for the 3 failing gates (also in `gate.json` under `per_pillar_numbers`, from the same 320 samples):
- Emblem mean visible fraction below 0.5: AIR TAG 0.469, GOOGLE TAG 0.462. The table above lists all 18.
- Samples with a tap cell below 100%: SAMSUNG 98.1%, IBEACON 99.7%. Every other pillar is at 100%.
Below 20 px p10: FLOCK 16.8, AIR TAG 9, KEYFOB 9, META 9, SAMSUNG 7, AXON 9, RAVEN 13, ALPR 11, CAMERA 11, GOOGLE TAG 15.

## Files

| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still (sample 162) |
| `closeup_deauth_high_3x.png` | T1 code glass close-up |
| `closeup_airtag_medium_3x.png` | see-through wood close-up (AIR TAG) |
| `closeup_flock_low_3x.png` | see-through white close-up (FLOCK) |
| `closeup_raven_new_3x.png` | see-through white close-up (RAVEN) |
| `emblems_old_vs_new.png` | 18 emblems: old 16x16 art vs the new crisp masks at 16 / 32 / 40 px |
| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened; @3x 720x960 h264 |
| `folders.md` | all 18 devices' folder + line-item text (held as is) |
| `gate.json`, `SHA256SUMS` | gates and checksums |
| `src/` | `patch_tree.py`, `make_mix_inc.py` -> `mo_mix.inc`, `mo_emblem.h`, `folders18.py`, `layout18.json`, `opt18/`, `package.py`, `drive.py`, `touch20.py`, `mix.json`, `emb_sheet.cpp`, `write_readme.py` |
| `tree_patch/` | the two patched headers |
