# MORPHEUS MIX SPREAD v5b (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number comes from the real 240x320 RGB565 sim (same pipeline as v4).
Earlier versions are in `_withdrawn/v1` .. `v4` (v4 = the 10:15 AM ET renders, rejected 11:51 AM ET; v5a = the first v5 render, rejected 12:42 PM ET, see `_withdrawn/v5a/NOTE.txt`).

## What changed in v5 (Mobandy 11:51 / 11:52 AM ET)

1. **All wood.** All 18 pillar bodies use the see-through wood (T3) body. There is no green code glass or white light body any more.
   - In `mo11_tower_fx`, the body look is forced to 3. `MO_LOOKOF` now only picks the height group.
   - The opaque emblem, on-tower folder glyphs and file line-items are unchanged; only the body is see-through.
2. **Heights are as v4:** H = 2.6. Former green group 1.00 (2.6), former wood group 0.80 (2.08), former white group 0.62 (1.612). No breathing.
3. **A colour per device.** Each emblem gets its own saturated colour (`src/emb_colors.py`):
   - 18 colours with 18 distinct hues (min hue step 5 deg; v5a had crimson and rose both at 340 deg, rose is now #cc5c6a at 352 deg). They are dealt by a swap search that maximises the smallest deltaE between towers that are screen neighbours at any of 64 sampled idle times.
   - Emblems are still crisp binary masks with a 1 px dark outline, drawn pixel-aligned and opaque.
   - Min pairwise deltaE (CIE76) = 36.4 (DEAUTH / META). Min between screen neighbours = 61.3 (FLOCK / SAMSUNG). Min distance from the wood tones = 36.3.
4. **Width reverted:** `tw` is back to **1.35** (v4 had 2.2, which was unrequested).
5. **Spread (11:52 AM / 12:42 PM ET) - NOT achieved.** The layout was re-picked (`src/opt18/opt18e.py`, seed 803) on a corrected occlusion model (a nearer pillar's box now starts at min(front top, back top); v5a missed the front-face top strip, which also caused the false emblem 'flicker'), with an emblem visibility floor, empty-floor and screen-third balance terms.
   - Largest empty floor square, whole visible floor (rows 140-285): v4 idle **107 px**, v5a idle **71 px**, v5b idle **75 px** (median over 320 samples 79.0, worst 83).
   - Standable floor (rows 140-251; a base lower than y 251 needs z0 < 0.95 and the tower top leaves the field): v5a idle **43 px**, v5b idle **53 px** (median 51.0, worst 65). Bases per screen third L/M/R: v5a 6/8/4, v5b 7/6/5; front-row bases per third 1/1/1.
   - The front floor stays bare: towers on it are 35-45 px wide and hide the emblems behind them. With camera, heights, tw and drift locked, ~40 optimizer runs never got the whole-floor square below ~70 px while keeping every emblem >= 0.5 visible.
   - The height group order (green-group back, white-group front), the camera, sway and the v11 drift law are unchanged.

## Emblem colours

| device | threat / height | colour | hex | hue |
|---|---|---|---|---|
| DEAUTH | HIGH / 1.00 | leaf green | #78cc5c | 105 deg |
| HACK | HIGH / 1.00 | aquamarine | #26ffc9 | 165 deg |
| FLOCK | LOW / 0.62 | royal blue | #0033cc | 225 deg |
| DRONE | LOW / 0.62 | red-orange | #ff4000 | 15 deg |
| AIR TAG | MEDIUM / 0.80 | azure | #0080ff | 210 deg |
| KEYFOB | MEDIUM / 0.80 | sky blue | #00bfff | 195 deg |
| META | MEDIUM / 0.80 | khaki | #ccc35c | 55 deg |
| SKIMMER | HIGH / 1.00 | blue | #0000ff | 240 deg |
| FLIPPER | HIGH / 1.00 | orange | #ff9500 | 35 deg |
| SAMSUNG | MEDIUM / 0.80 | magenta | #ff00ff | 300 deg |
| AXON | MEDIUM / 0.80 | yellow | #ffd400 | 50 deg |
| RAVEN | LOW / 0.62 | crimson | #ff0055 | 340 deg |
| ALPR | LOW / 0.62 | orchid | #b95ccc | 290 deg |
| CAMERA | MEDIUM / 0.80 | teal | #3dcccc | 180 deg |
| RING | LOW / 0.62 | hot pink | #ff00aa | 320 deg |
| GOOGLE TAG | MEDIUM / 0.80 | lime | #ace600 | 75 deg |
| EVIL TWIN | HIGH / 1.00 | green | #00ff6a | 145 deg |
| IBEACON | LOW / 0.62 | rose | #cc5c6a | 352 deg |

## What the idle still looks like

v5b idle_3x, looked at by the executor 2:12 PM ET: all 18 bodies are see-through brown wood with the opaque green folder pull-down and white file lines; emblems are crisp, flat, one strong colour each (red-orange drone, magenta key-style AIR-TAG-row emblem, blue target, lime pin, rose bag, cyan skull, etc.) and none of them shimmer in the 40-frame stability run (mean dY 0.0).
NOT FIXED - the spread: the towers still stand in a band across the middle of the room. The whole front floor (roughly the lower third of the field, below the tower bases at 1x y ~215) is bare from left to right, and the right edge beside the last tower is bare too. The left half is denser than the right. Reason: every layout that puts towers on that front floor needs them very close to the camera, where they are 35-45 px wide and cover the emblems of the towers behind them, or (closer than z0 0.95) their tops leave the field. With the camera, heights, tw 1.35 and drift locked, ~40 optimizer runs on the corrected model never got the largest empty floor square below ~70 px while keeping every emblem >= 0.5 visible.
Emblems: SAMSUNG shows 0 own-colour px in this still (behind a nearer tower's see-through body: it reads as a tinted silhouette through the wood, not as its own colour); DEAUTH, ALPR, AXON and EVIL TWIN are partly covered. Over 320 idle samples every pillar averages >= 0.546 visible (worst AIR TAG).
Emblem colour sheet: all 18 shapes crisp on wood; the two dark blues (FLOCK royal blue #0033cc, SKIMMER blue #0000ff) are the weakest against the dark grain, and the dark secondary parts of DEAUTH (the x) and EVIL TWIN (the second tower) almost vanish into the wood.

## Idle still

`idle_1x.png` / `idle_3x.png` = idle sample **306 of 320** (median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample; quality 1.1552, median 1.1052).

## Per pillar (real sim)

| # | device | height | sx | z0 | emblem S | idle px | vis % | tap cell % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | 1.00 | 217.3 | 2.450 | 14 | 28 | 60 | 100.0 | 1 | 13 |
| 1 | HACK | 1.00 | 25.2 | 2.497 | 14 | 116 | 58 | 99.7 | 0 | 11 |
| 2 | FLOCK | 0.62 | 87.9 | 1.744 | 20 | 180 | 85 | 100.0 | 11 | 17 |
| 3 | DRONE | 0.62 | 154.4 | 1.945 | 20 | 400 | 69 | 99.7 | 0 | 11 |
| 4 | AIR TAG | 0.80 | 178.3 | 2.147 | 16 | 240 | 55 | 100.0 | 7 | 9 |
| 5 | KEYFOB | 0.80 | 73.4 | 2.202 | 16 | 66 | 64 | 100.0 | 9 | 13 |
| 6 | META | 0.80 | 128.8 | 1.857 | 18 | 162 | 56 | 100.0 | 11 | 15 |
| 7 | SKIMMER | 1.00 | 182.0 | 2.431 | 14 | 86 | 69 | 100.0 | 23 | 25 |
| 8 | FLIPPER | 1.00 | 51.6 | 2.460 | 14 | 196 | 60 | 100.0 | 19 | 23 |
| 9 | SAMSUNG | 0.80 | 101.9 | 1.989 | 18 | 0 | 56 | 100.0 | 5 | 9 |
| 10 | AXON | 0.80 | 203.2 | 2.150 | 16 | 46 | 56 | 99.4 | 0 | 11 |
| 11 | RAVEN | 0.62 | 55.5 | 2.061 | 18 | 198 | 69 | 99.1 | 0 | 11 |
| 12 | ALPR | 0.62 | 16.0 | 1.577 | 22 | 22 | 65 | 100.0 | 11 | 18.8 |
| 13 | CAMERA | 0.80 | 151.9 | 2.144 | 16 | 166 | 63 | 100.0 | 5 | 15 |
| 14 | RING | 0.62 | 194.6 | 1.646 | 24 | 576 | 96 | 100.0 | 25 | 33 |
| 15 | GOOGLE TAG | 0.80 | 39.0 | 2.075 | 16 | 146 | 60 | 100.0 | 3 | 9 |
| 16 | EVIL TWIN | 1.00 | 105.6 | 2.315 | 14 | 47 | 63 | 100.0 | 19 | 23 |
| 17 | IBEACON | 0.62 | 119.8 | 1.373 | 28 | 784 | 100 | 100.0 | 51 | 53 |

## Gate: 26/31 (`gate.json`)

- FAIL: emblem visibility (idle still): every pillar shows emblem pixels
- FAIL: tap: every pillar keeps a tap cell in every idle sample (100%)
- FAIL: tap: p10 largest square >= 20 px for every pillar
- FAIL: floor coverage: largest empty standable-floor square <= 60 px (idle + 320-sample median), whole floor <= 75 px, 5-7 bases per screen third and >= 1 front-row base (y >= 200) in each third (v4 107 px, v5a 71 px whole floor)
- FAIL: emblem colours: 18 distinct saturated colours with 18 distinct hues (min hue step >= 5 deg), min pairwise CIE76 deltaE >= 30, screen-adjacent towers deltaE >= 40, every colour deltaE >= 30 from the wood tones, >= 16 of 18 drawn in the idle still

Temporal stability: 157 pairs, mean |dY| 0.0, worst 0.0. Opaque UI: 812046158 UI pixels, 0 translucent. The tap rule was not changed.

## Files

| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still |
| `closeup_deauth_high_3x.png` | close-up, height group 1.00 |
| `closeup_airtag_medium_3x.png` | close-up, height group 0.80 |
| `closeup_flock_low_3x.png` | close-up, height group 0.62 |
| `emblem_colors.png` | 18 emblems in their colours on wood (16 / 24 / 40 px) |
| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened; 720x960 h264 |
| `folders.md` | folder text (unchanged from v4) |
| `gate.json`, `SHA256SUMS` | gates, checksums |
| `src/`, `tree_patch/` | generator sources, patched headers |
