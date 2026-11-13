# MORPHEUS MIX (mock, 2026-11-13)

**Mock only.** Nothing was flashed, merged, or committed to main as firmware. Every number below comes from the real 240x320 RGB565 sim (`tools/livescan/sim_live.cpp`, Gibson engine). EX.DATA only.

## What this is
- The Morpheus inside screen has **20 device pillars**: the original 12 plus 8 new devices from the SquachWatch README (AXON, RAVEN, ALPR, CAMERA, RING, GOOGLE TAG, EVIL TWIN, IBEACON).
- Each pillar uses one of the three towers-3 looks, chosen by threat level:
  - HIGH: T1 code glass
  - MEDIUM: T3 sentinel lantern
  - LOW: T2 construct white light
  - This split is provisional; any device can be moved by editing `MO_THREAT` in `patch_tree.py`.
- The threat level appears only as small text in the existing terminal status line, which reads `PILLAR OPEN  THREAT: HIGH  EX.DATA`. No new widgets were added.
- Device icon art is replaced by crisp, anti-aliased vector emblems (`src/mo_emblem.h`), each with its own silhouette:
  - idle pillars: 8 to 16 px
  - close-up: 40 px
  - panel chip: 16 px
  - folder header: 10 px
- Unchanged towers-3 design: folders, files scrolling inside the tower, no square pane grid, no red pill, no lock bracket.

## Per-style height scale
The scale is applied on top of the engine height, in `mo11_drift`, as `T.H *= MO_HSC_LOOK[look]`. The Gibson H law is unchanged.

| style | factor |
|---|---|
| T1 code glass (HIGH) | **x1.05** |
| T3 sentinel lantern (MEDIUM) | **x1.00** |
| T2 construct light (LOW) | **x0.95** |

**How the factors were chosen:** the factors were tuned for idle emblem visibility on the real sim. I ran a 3x3 grid over 320 idle samples each (`src/htune_sim.py`) and picked the best score. Per-style mean H is {'T1_code_glass': 1.5823, 'T2_construct_light': 0.99, 'T3_sentinel_post': 1.2191}, so the order T1 > T3 > T2 holds.

**Honest finding:** within the "slight" range, the style heights barely change emblem visibility. Mean visible fraction is 0.81 with the style heights and 0.814 without them. Taller T1 pillars tend to hide the emblems behind them, so the smallest T1 boost tested (x1.05) scored best.

| h1 (T1) | h2 (T2) | emblem vis frac mean | worst pillar frac | vis px mean | tap p10 mean |
|---|---|---|---|---|---|
| 1.05 | 0.85 | 0.795 | 0.260 | 92.8 | 19.7 |
| 1.05 | 0.90 | 0.803 | 0.260 | 93.5 | 19.6 |
| 1.05 | 0.95 | 0.810 | 0.260 | 94.3 | 19.6 |
| 1.10 | 0.85 | 0.797 | 0.231 | 92.9 | 20.7 |
| 1.10 | 0.90 | 0.803 | 0.260 | 93.4 | 20.7 |
| 1.10 | 0.95 | 0.808 | 0.260 | 94.0 | 20.6 |
| 1.15 | 0.85 | 0.797 | 0.211 | 92.9 | 20.9 |
| 1.15 | 0.90 | 0.803 | 0.242 | 93.3 | 20.9 |
| 1.15 | 0.95 | 0.807 | 0.260 | 93.8 | 20.9 |

## Device -> threat -> look
"Idle emblem px" is the count of visible emblem pixels in the idle still. "Vis frac" is the mean visible fraction over 320 idle samples. "Tap p10" is the 10th-percentile largest tap square in px.

| # | device | threat | look | height | idle emblem px | vis frac | tap p10 |
|---|---|---|---|---|---|---|---|
| 0 | WIFI | LOW | T2_construct_light | x0.95 | 100 | 1.00 | 9.0 |
| 1 | BLUETOOTH | LOW | T2_construct_light | x0.95 | 76 | 0.69 | 9.0 |
| 2 | DEAUTH | HIGH | T1_code_glass | x1.05 | 256 | 0.88 | 35.0 |
| 3 | HACK | HIGH | T1_code_glass | x1.05 | 64 | 0.90 | 38.800000000000004 |
| 4 | FLOCK | LOW | T2_construct_light | x0.95 | 64 | 0.95 | 15.0 |
| 5 | DRONE | LOW | T2_construct_light | x0.95 | 32 | 0.74 | 9.0 |
| 6 | AIR TAG | MEDIUM | T3_sentinel_post | x1.00 | 256 | 0.26 | 9.0 |
| 7 | KEYFOB | MEDIUM | T3_sentinel_post | x1.00 | 100 | 0.97 | 9.0 |
| 8 | META | MEDIUM | T3_sentinel_post | x1.00 | 64 | 0.31 | 11.0 |
| 9 | SKIMMER | HIGH | T1_code_glass | x1.05 | 100 | 0.98 | 32.800000000000004 |
| 10 | FLIPPER | HIGH | T1_code_glass | x1.05 | 64 | 0.69 | 21.0 |
| 11 | SAMSUNG | MEDIUM | T3_sentinel_post | x1.00 | 100 | 0.92 | 33.0 |
| 12 | AXON (new) | MEDIUM | T3_sentinel_post | x1.00 | 64 | 0.67 | 13.0 |
| 13 | RAVEN (new) | LOW | T2_construct_light | x0.95 | 100 | 0.58 | 9.0 |
| 14 | ALPR (new) | LOW | T2_construct_light | x0.95 | 100 | 1.00 | 19.0 |
| 15 | CAMERA (new) | MEDIUM | T3_sentinel_post | x1.00 | 64 | 0.89 | 19.0 |
| 16 | RING (new) | LOW | T2_construct_light | x0.95 | 256 | 1.00 | 57.0 |
| 17 | GOOGLE TAG (new) | MEDIUM | T3_sentinel_post | x1.00 | 100 | 0.94 | 11.0 |
| 18 | EVIL TWIN (new) | HIGH | T1_code_glass | x1.05 | 60 | 0.95 | 19.0 |
| 19 | IBEACON (new) | LOW | T2_construct_light | x0.95 | 60 | 0.87 | 13.0 |

## Layout of the new 8
- They use the same Gibson scatter, depth and height rules (`src/layout.json`, chosen by `src/opt/opt20.py` with seed 15).
- Their motion is the identical v11 drift law (gate-checked).
- The original 12 keep their exact v13.1 motion: X, Z, drift and breathing all match the v13.1 reference sim at 0 error. Their H differs only by the style factor.

## Gate: 18/21 pass (`gate.json`)
- FAIL: emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5
- FAIL: tap: every pillar keeps a tap cell in every idle sample
- FAIL: tap: p10 largest square >= 20 px for every pillar

These three failures are structural:
- The original 12 must keep their motion, and in that layout AIR TAG drifts behind DEAUTH and META drifts behind others.
- Twenty pillars on a 240 px screen crowd the centre cluster.

Emblem visibility over 320 idle samples:
- per-pillar visible px: min 0.0, mean 94.3
- worst pillar: AIR TAG (fraction 0.26)
- the idle still shows emblem pixels on all 20 pillars (min 32 px, mean 104.0 px)

Pillars that lose their tap cell entirely in some samples: BLUETOOTH, DRONE, AIR TAG, META, FLIPPER, AXON, GOOGLE TAG. In each case this happens in at most 1.6% of samples.

## Files
| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still (sample 67 of 320, where all 20 emblems are visible) |
| `closeup_deauth_high_3x.png`, `closeup_airtag_medium_3x.png`, `closeup_wifi_low_3x.png`, `closeup_raven_new_3x.png` | the four close-ups |
| `emblems_old_vs_new.png` | 20 emblems: old 16x16 art vs new vector at 16 and 40 px |
| `idle_then_open_3x.mp4` | idle, then each of the 4 pillars opening, @3x, h264 yuv420p, 30 fps (100/3) |
| `gate.json`, `SHA256SUMS` | gate results and checksums |
| `src/` | sources: `patch_tree.py` (v6-wt v13.1 base + towers-3 renames), `make_mix_inc.py` -> `mo_mix.inc`, `mo_emblem.h`, `drive.py`, `mix.json`, `touch20.py`, `package.py`, `emb_sheet.cpp`, `opt/`, `layout.json` |
| `tree_patch/` | the two patched headers (`lv_morpheus_v13.h`, `lv_inside_towers.h`) |
