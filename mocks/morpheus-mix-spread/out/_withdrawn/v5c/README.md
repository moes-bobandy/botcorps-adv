# MORPHEUS MIX SPREAD v5c (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number comes from the real 240x320 RGB565 sim (same pipeline as v4).
Earlier versions are in `_withdrawn/v1` .. `v4` (v4 = the 10:15 AM ET renders, rejected 11:51 AM ET; v5a = the first v5 render, rejected 12:42 PM ET, see `_withdrawn/v5a/NOTE.txt`; v5b = commit 512fbb3, superseded 2:05 PM ET, `_withdrawn/v5b/`).

## What changed in v5 (Mobandy 11:51 / 11:52 AM ET)

1. **All wood.** All 18 pillar bodies use the see-through wood (T3) body. There is no green code glass or white light body any more.
   - In `mo11_tower_fx`, the body look is forced to 3. `MO_LOOKOF` now only picks the height group.
   - The opaque emblem, on-tower folder glyphs and file line-items are unchanged; only the body is see-through.
2. **Heights are as v4:** H = 2.6. Former green group 1.00 (2.6), former wood group 0.80 (2.08), former white group 0.62 (1.612). No breathing.
3. **A colour per device.** Each emblem gets its own saturated colour (`src/emb_colors.py`):
   - 18 colours with 18 distinct hues (min hue step 5 deg; v5a had crimson and rose both at 340 deg, rose is now #cc5c6a at 352 deg). They are dealt by a swap search that maximises the smallest deltaE between towers that are screen neighbours at any of 64 sampled idle times.
   - Emblems are still crisp binary masks with a 1 px dark outline, drawn pixel-aligned and opaque.
   - Min pairwise deltaE (CIE76) = 36.4 (GOOGLE TAG / IBEACON). Min between screen neighbours = 45.7 (GOOGLE TAG / HACK). Min distance from the wood tones = 36.3.
4. **Width reverted:** `tw` is back to **1.35** (v4 had 2.2, which was unrequested).
5. **Spread (11:52 AM / 12:42 PM ET) - front floor NOT filled.** Columns re-picked (`src/opt18/opt18e.py`, seed 904, depth fixed by item 6) on a corrected occlusion model (a nearer pillar's box now starts at min(front top, back top); v5a missed the front-face top strip, which also caused the false emblem 'flicker'), with an emblem visibility floor, empty-floor and screen-third balance terms.
   - Largest empty floor square, whole visible floor (rows 140-285): v4 idle **107 px**, v5a idle **71 px**, v5b idle **93 px** (median over 320 samples 95.0, worst 101).
   - Standable floor (rows 140-251; a base lower than y 251 needs z0 < 0.95 and the tower top leaves the field): v5a idle **43 px**, v5b idle **67 px** (median 65.0, worst 73). Bases per screen third L/M/R: v5a 6/8/4, v5b 7/7/4; front-row bases per third 0/0/0.
   - The front floor stays bare: towers on it are 35-45 px wide and hide the emblems behind them. With camera, heights, tw and drift locked, ~40 optimizer runs never got the whole-floor square below ~70 px while keeping every emblem >= 0.5 visible.
6. **Depth like the Gibson scan (2:06 PM ET).** Each device's depth comes from its **example** signal (EX.DATA rssi) through reused Gibson code in `tree/src/live/lv_radar.h`: `lv_rd_level()` (level = rssi + 100, 4..60) and the depth band of `lv_rd_place()` (`T.Z = 1.6f + u * 3.2f`), with u = (60 - level) / 56, so strong = near (Z 1.6) and weak = far (Z 4.8). The real Gibson scan takes u from a MAC hash, not from rssi: the rssi -> u step is new. Columns stay inside Gibson's width spread (`T.X = +-0.72 * T.Z`, screen 34..206). Tall towers are pushed back only as far as the top-clip rule needs. Code: `src/gibson_depth_drift.py`.
   - Near -> far: CAMERA (-44 dBm, Z 1.83), AXON (-48 dBm, Z 2.06), HACK (-42 dBm, Z 2.21), FLIPPER (-46 dBm, Z 2.26), SKIMMER (-50 dBm, Z 2.28), GOOGLE TAG (-52 dBm, Z 2.29), DRONE (-54 dBm, Z 2.40), FLOCK (-58 dBm, Z 2.63), META (-62 dBm, Z 2.86), EVIL TWIN (-64 dBm, Z 2.97), RAVEN (-72 dBm, Z 3.43), KEYFOB (-74 dBm, Z 3.54), IBEACON (-77 dBm, Z 3.71), DEAUTH (-79 dBm, Z 3.83), ALPR (-82 dBm, Z 4.00), AIR TAG (-85 dBm, Z 4.17), RING (-88 dBm, Z 4.34), SAMSUNG (-92 dBm, Z 4.57).
7. **Not locked in movement (2:05 PM ET).** Every tower has its own phase, speed and amplitude in X and Z plus a faster lateral sway (`dx = ax (Z0/2) sin(wx t + px) + as Z0 sin(ws t + ps)`, `dz = az (Z0/3) sin(wz t + pz)`), speeds kept away from the camera sway (0.45 rad/s, unchanged). Motion gate: raw screen-motion mean |r| 0.293 (v5b 0.853), own-drift mean |r| 0.058 / max 0.495, depth mean |r| 0.085 / max 0.78, 153 of 153 pairs change spacing by >= 8 px (median 81.9 px).
   - The camera sway is unchanged.

## Emblem colours

| device | threat / height | colour | hex | hue |
|---|---|---|---|---|
| DEAUTH | HIGH / 1.00 | royal blue | #0033cc | 225 deg |
| HACK | HIGH / 1.00 | lime | #ace600 | 75 deg |
| FLOCK | LOW / 0.62 | crimson | #ff0055 | 340 deg |
| DRONE | LOW / 0.62 | orange | #ff9500 | 35 deg |
| AIR TAG | MEDIUM / 0.80 | magenta | #ff00ff | 300 deg |
| KEYFOB | MEDIUM / 0.80 | rose | #cc5c6a | 352 deg |
| META | MEDIUM / 0.80 | green | #00ff6a | 145 deg |
| SKIMMER | HIGH / 1.00 | hot pink | #ff00aa | 320 deg |
| FLIPPER | HIGH / 1.00 | orchid | #b95ccc | 290 deg |
| SAMSUNG | MEDIUM / 0.80 | azure | #0080ff | 210 deg |
| AXON | MEDIUM / 0.80 | teal | #3dcccc | 180 deg |
| RAVEN | LOW / 0.62 | red-orange | #ff4000 | 15 deg |
| ALPR | LOW / 0.62 | sky blue | #00bfff | 195 deg |
| CAMERA | MEDIUM / 0.80 | yellow | #ffd400 | 50 deg |
| RING | LOW / 0.62 | blue | #0000ff | 240 deg |
| GOOGLE TAG | MEDIUM / 0.80 | khaki | #ccc35c | 55 deg |
| EVIL TWIN | HIGH / 1.00 | aquamarine | #26ffc9 | 165 deg |
| IBEACON | LOW / 0.62 | leaf green | #78cc5c | 105 deg |

## What the idle still looks like

v5c idle_3x, looked at by the executor ~2:52 PM ET: all 18 bodies see-through wood with opaque green folder rows; emblems crisp and steady (stability 0.0). Depth now reads like the Gibson scan: a few strong-signal towers (HACK skull, CAMERA, AXON, FLIPPER) stand big and near at the left and centre, the weak-signal ones are small and deep in the room near the back wall.
The cost is plain in the still: the near towers cover much of the room behind them - FLOCK, FLIPPER, RAVEN and RING show 0 own-colour emblem px in this still, DEAUTH and AIR TAG only 8 px - and because even Gibson's nearest depth (Z 1.6) stands at screen y ~205, the whole front floor (lower ~40% of the field) is bare; it is emptier than v5b (largest empty square 93 px vs 75).
Video (frames 1 s apart): the towers no longer slide as one block with the camera; neighbours drift at different speeds and directions, gaps open and close, and towers pass in front of each other.
Emblem colours were re-dealt for the new screen neighbours (same 18-colour palette): the two dark blues now sit on DEAUTH (royal blue) and RING (blue) and are the weakest on the dark grain; the dark second parts of the DEAUTH and EVIL TWIN emblems still nearly vanish into the wood.

## Idle still

`idle_1x.png` / `idle_3x.png` = idle sample **211 of 320** (median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample; quality 0.9445, median 0.944).

## Per pillar (real sim)

| # | device | height | sx | z0 | emblem S | idle px | vis % | tap cell % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | 1.00 | 205.7 | 3.829 | 8 | 8 | 49 | 97.5 | 0 | 7 |
| 1 | HACK | 1.00 | 25.3 | 2.211 | 16 | 256 | 73 | 98.7 | 0 | 11 |
| 2 | FLOCK | 0.62 | 72.0 | 2.629 | 12 | 0 | 32 | 98.7 | 0 | 5 |
| 3 | DRONE | 0.62 | 59.3 | 2.400 | 14 | 196 | 53 | 99.7 | 0 | 7 |
| 4 | AIR TAG | 0.80 | 74.8 | 4.171 | 8 | 8 | 28 | 97.2 | 0 | 5 |
| 5 | KEYFOB | 0.80 | 186.9 | 3.543 | 12 | 144 | 27 | 97.2 | 0 | 5 |
| 6 | META | 0.80 | 116.3 | 2.857 | 12 | 144 | 79 | 99.1 | 0 | 13 |
| 7 | SKIMMER | 1.00 | 182.2 | 2.281 | 18 | 207 | 79 | 100.0 | 21 | 25 |
| 8 | FLIPPER | 1.00 | 54.0 | 2.263 | 14 | 0 | 86 | 100.0 | 9 | 25 |
| 9 | SAMSUNG | 0.80 | 100.0 | 4.571 | 6 | 30 | 30 | 98.1 | 0 | 3 |
| 10 | AXON | 0.80 | 199.3 | 2.057 | 16 | 144 | 82 | 100.0 | 9 | 17 |
| 11 | RAVEN | 0.62 | 172.8 | 3.429 | 10 | 0 | 30 | 98.1 | 0 | 3 |
| 12 | ALPR | 0.62 | 34.0 | 4.000 | 8 | 32 | 24 | 95.0 | 0 | 3 |
| 13 | CAMERA | 0.80 | 150.4 | 1.829 | 20 | 400 | 100 | 100.0 | 55 | 59 |
| 14 | RING | 0.62 | 117.7 | 4.343 | 8 | 0 | 32 | 96.9 | 0 | 3 |
| 15 | GOOGLE TAG | 0.80 | 55.4 | 2.286 | 18 | 108 | 61 | 99.4 | 0 | 11 |
| 16 | EVIL TWIN | 1.00 | 112.6 | 2.971 | 12 | 144 | 81 | 99.7 | 0 | 21 |
| 17 | IBEACON | 0.62 | 204.4 | 3.714 | 8 | 64 | 29 | 97.5 | 0 | 3 |

## Gate: 26/32 (`gate.json`)

- FAIL: emblem visibility (idle still): every pillar shows emblem pixels
- FAIL: emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5
- FAIL: tap: every pillar keeps a tap cell in every idle sample (100%)
- FAIL: tap: p10 largest square >= 20 px for every pillar
- FAIL: floor coverage: largest empty standable-floor square <= 60 px (idle + 320-sample median), whole floor <= 75 px, 5-7 bases per screen third and >= 1 front-row base (y >= 200) in each third (v4 107 px, v5a 71 px whole floor)
- FAIL: emblem colours: 18 distinct saturated colours with 18 distinct hues (min hue step >= 5 deg), min pairwise CIE76 deltaE >= 30, screen-adjacent towers deltaE >= 40, every colour deltaE >= 30 from the wood tones, >= 16 of 18 drawn in the idle still

Temporal stability: 210 pairs, mean |dY| 0.0, worst 0.0. Opaque UI: 422387910 UI pixels, 0 translucent. The tap rule was not changed.

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
