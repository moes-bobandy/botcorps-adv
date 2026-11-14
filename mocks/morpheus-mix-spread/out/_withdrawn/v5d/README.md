# MORPHEUS MIX SPREAD v5d (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number comes from the real 240x320 RGB565 sim (same pipeline as v4).
Earlier versions are in `_withdrawn/v1` .. `v4` (v4 = the 10:15 AM ET renders, rejected 11:51 AM ET; v5a = the first v5 render, rejected 12:42 PM ET, see `_withdrawn/v5a/NOTE.txt`; v5b = commit 512fbb3, superseded 2:05 PM ET, `_withdrawn/v5b/`; v5c = commit d68abf4, superseded 2:34 PM ET, `_withdrawn/v5c/`).

## What changed in v5d (Mobandy 2:34 - 2:38 PM ET)

1. **Shorter near towers (2:34 PM ET).** Each tower's world height is its group height times a fixed depth factor:
   - `hk(Z) = 0.6 + (1.2 - 0.6) * smoothstep((Z - 1.6) / 3.2)`, from Gibson depth Z (`src/v5d_heights.py`). It is fixed per tower, so nothing breathes.
   - Group order and ratios 1.00 / 0.80 / 0.62 hold at every depth.
   - **Far factor is 1.2, not 1.0.** With 1.0 the far white towers could not keep >= 6 folder rows while the near towers also stayed >= 45% of the tallest on-screen tower. So the far towers are 20% taller than v5c, and the near ones are about 40% shorter.
   - HACK, FLIPPER and SKIMMER are back at their true Gibson depths (1.71 / 1.94 / 2.17); their tops now fit.
   - The front floor still can't take a base. The nearest Gibson depth (Z 1.6) puts a base at about y 205, so the band below is unreachable without leaving Gibson depth.
   - Columns were re-picked for the new heights (`src/opt18/opt18e.py`, depth fixed).
   - No stumps: the shortest tower (RING) is 47.3% of the tallest on-screen tower, median over 320 samples.
   - Every tower shows its emblem plus at least 6 on-tower folder rows.

   | tower (near -> far) | Z | hk | H | median on-screen px | idle folder rows |
   |---|---|---|---|---|---|
| HACK | 1.71 | 0.602 | 1.566 | 110 px | 18 |
| CAMERA | 1.83 | 0.609 | 1.266 | 83 px | 11 |
| FLIPPER | 1.94 | 0.619 | 1.610 | 100 px | 18 |
| AXON | 2.06 | 0.633 | 1.317 | 76 px | 11 |
| SKIMMER | 2.17 | 0.651 | 1.692 | 94 px | 17 |
| GOOGLE TAG | 2.29 | 0.671 | 1.395 | 73 px | 10 |
| DRONE | 2.40 | 0.694 | 1.118 | 56 px | 7 |
| FLOCK | 2.63 | 0.746 | 1.203 | 55 px | 6 |
| META | 2.86 | 0.805 | 1.674 | 71 px | 10 |
| EVIL TWIN | 2.97 | 0.836 | 2.174 | 87 px | 16 |
| RAVEN | 3.43 | 0.964 | 1.554 | 55 px | 7 |
| KEYFOB | 3.54 | 0.995 | 2.070 | 69 px | 10 |
| IBEACON | 3.71 | 1.040 | 1.676 | 54 px | 9 |
| DEAUTH | 3.83 | 1.068 | 2.776 | 87 px | 18 |
| ALPR | 4.00 | 1.106 | 1.783 | 53 px | 7 |
| AIR TAG | 4.17 | 1.140 | 2.370 | 68 px | 11 |
| RING | 4.34 | 1.167 | 1.881 | 52 px | 6 |
| SAMSUNG | 4.57 | 1.191 | 2.478 | 65 px | 10 |

2. **Black Matrix towers (2:35 PM ET).** The 5 HIGH towers (DEAUTH, HACK, SKIMMER, FLIPPER, EVIL TWIN) use the existing T1 code-glass branch of `mo11_tower_fx`, the same code path v4 used for the green group.
   - The falling Matrix code is unchanged. Only the glass base is darkened toward black: front `mo3_mx(0x03120a, 0x000201)`, side `0x05160d`, highlight `0x03120a -> 0x3a8a58`.
   - The other 13 stay see-through wood (T3).
   - Emblems use each device's own colour; folders are opaque.

3. **Open sequence (2:36 - 2:38 PM ET).** A tap still pushes the camera in, as in v5c. Then, in order:
   1. **Gibson lock (0 - 1.2 s).** This is the hold / lock bracket branch of `lv_radar.h lv_rd_fx`, line for line, calling the real `lv_rd_box`, `lv_rd_brackets`, `lv_rd_rect_blend`, `lv_dim` and `b_text35`:
      - Brackets close in from pad 15 to pad 3 and brighten over 1 s.
      - Then magenta brackets pulse at 1.6 Hz, with a tick and a LOCK label.
      - The white snap flash is gated by the same `LV_RD_CHARGE` condition as Gibson, so it is off with the shipped charge lock.
      - The hold is driven by the time since the tap, because Morpheus has no HOLD TO LOCK bar.
      - The shipped Gibson locks by charging a HOLD TO LOCK bar, and the locked tower lights in its own colour (`lv_rd_cg_lit`). That part is not used here; `pullup_gibson_vs_morpheus_2x.png` shows both.
   2. **HACKbot search (1.2 s - about 2.6 s).** This uses `lv_scanbot_download.h lv_dl_drop`'s key bar through the real `lvi_rect` / `lvi_box` / `lvi_line` / `lvi_txt` helpers:
      - Dark fill, with the frame blinking amber / cyan and a cyan underline.
      - Amber 3x5 text `[SEARCH // DEAUTH_ // n/11]` types the device name.
      - Then lv_dl_drop's 11-band tower meter (`lvi_band` inks, rows 238 - k*18) lights band by band up the opened tower.
   3. **Folders.** These use lv_dl_drop's folder chrome via the `lvi_*` helpers: tab, neon frame, scanlines, drop shadow, the cubic ease-out drop and the stagger, and the cyan rail at x 84 / 86. Each folder adds, inside that chrome and all from folders18:
      - an `lvi_icon` detection icon and the file name;
      - a HIGH / MED / LOW risk chip in the threat colour;
      - the device tag, the EX.DATA dBm and an 11-step signal meter from the device's level;
      - the action line;
      - a HACKbot-style load bar with `n%` -> `GOT`;
      - a `T+n.nS` timestamp since the tap.
   - The idle on-tower folders are unchanged.
   - Gate, from the sim's own state: DEAUTH search: 0 folders, typed 6, 4/11 lit; open: 3 folders, 11/11; AIR TAG search: 0 folders, typed 7, 3/11 lit; open: 3 folders, 11/11; FLOCK search: 0 folders, typed 5, 4/11 lit; open: 3 folders, 11/11; RAVEN search: 0 folders, typed 5, 4/11 lit; open: 3 folders, 11/11.

## What changed in v5 (Mobandy 11:51 / 11:52 AM ET)

1. **All wood (v5, changed in v5d):** in v5 / v5c all 18 bodies were see-through wood (T3). v5d makes the 5 HIGH towers black Matrix code glass (see above); the other 13 stay wood.
   - The opaque emblem, on-tower folder glyphs and file line-items are unchanged; only the body is see-through.
2. **Group heights as v4:** green group 1.00 (2.6), wood group 0.80 (2.08), white group 0.62 (1.612). v5d multiplies each by the fixed depth factor hk (above). No breathing.
3. **A colour per device.** Each emblem gets its own saturated colour (`src/emb_colors.py`):
   - 18 colours with 18 distinct hues (min hue step 5 deg; v5a had crimson and rose both at 340 deg, rose is now #cc5c6a at 352 deg). They are dealt by a swap search that maximises the smallest deltaE between towers that are screen neighbours at any of 64 sampled idle times.
   - Emblems are still crisp binary masks with a 1 px dark outline, drawn pixel-aligned and opaque.
   - Min pairwise deltaE (CIE76) = 36.4 (FLOCK / IBEACON). Min between screen neighbours = 48.0 (DRONE / EVIL TWIN). Min distance from the wood tones = 36.3.
4. **Width reverted:** `tw` is back to **1.35** (v4 had 2.2, which was unrequested).
5. **Spread (11:52 AM / 12:42 PM ET) - front floor NOT filled.** Columns re-picked (`src/opt18/opt18e.py`, seed 952, depth fixed by item 6) on a corrected occlusion model (a nearer pillar's box now starts at min(front top, back top); v5a missed the front-face top strip, which also caused the false emblem 'flicker'), with an emblem visibility floor, empty-floor and screen-third balance terms.
   - Largest empty floor square, whole visible floor (rows 140-285): v4 idle **107 px**, v5a idle **71 px**, v5b idle **89 px** (median over 320 samples 87.0, worst 97).
   - Standable floor (rows 140-251; a base lower than y 251 needs z0 < 0.95 and the tower top leaves the field): v5a idle **43 px**, v5b idle **61 px** (median 61.0, worst 73). Bases per screen third L/M/R: v5a 6/8/4, v5b 6/6/6; front-row bases per third 1/0/0.
   - The front floor stays bare: towers on it are 35-45 px wide and hide the emblems behind them. With camera, heights, tw and drift locked, ~40 optimizer runs never got the whole-floor square below ~70 px while keeping every emblem >= 0.5 visible.
6. **Depth like the Gibson scan (2:06 PM ET).** Each device's depth comes from its **example** signal (EX.DATA rssi) through reused Gibson code in `tree/src/live/lv_radar.h`: `lv_rd_level()` (level = rssi + 100, 4..60) and the depth band of `lv_rd_place()` (`T.Z = 1.6f + u * 3.2f`), with u = (60 - level) / 56, so strong = near (Z 1.6) and weak = far (Z 4.8). The real Gibson scan takes u from a MAC hash, not from rssi: the rssi -> u step is new. Columns stay inside Gibson's width spread (`T.X = +-0.72 * T.Z`, screen 34..206). Tall towers are pushed back only as far as the top-clip rule needs. Code: `src/gibson_depth_drift.py`.
   - Near -> far: HACK (-42 dBm, Z 1.71), CAMERA (-44 dBm, Z 1.83), FLIPPER (-46 dBm, Z 1.94), AXON (-48 dBm, Z 2.06), SKIMMER (-50 dBm, Z 2.17), GOOGLE TAG (-52 dBm, Z 2.29), DRONE (-54 dBm, Z 2.40), FLOCK (-58 dBm, Z 2.63), META (-62 dBm, Z 2.86), EVIL TWIN (-64 dBm, Z 2.97), RAVEN (-72 dBm, Z 3.43), KEYFOB (-74 dBm, Z 3.54), IBEACON (-77 dBm, Z 3.71), DEAUTH (-79 dBm, Z 3.83), ALPR (-82 dBm, Z 4.00), AIR TAG (-85 dBm, Z 4.17), RING (-88 dBm, Z 4.34), SAMSUNG (-92 dBm, Z 4.57).
7. **Not locked in movement (2:05 PM ET).** Every tower has its own phase, speed and amplitude in X and Z plus a faster lateral sway (`dx = ax (Z0/2) sin(wx t + px) + as Z0 sin(ws t + ps)`, `dz = az (Z0/3) sin(wz t + pz)`), speeds kept away from the camera sway (0.45 rad/s, unchanged). Motion gate: raw screen-motion mean |r| 0.301 (v5b 0.853), own-drift mean |r| 0.057 / max 0.54, depth mean |r| 0.085 / max 0.78, 153 of 153 pairs change spacing by >= 8 px (median 81.2 px).
   - The camera sway is unchanged.

## Emblem colours

| device | threat / height | colour | hex | hue |
|---|---|---|---|---|
| DEAUTH | HIGH / 1.00 | teal | #3dcccc | 180 deg |
| HACK | HIGH / 1.00 | lime | #ace600 | 75 deg |
| FLOCK | LOW / 0.62 | khaki | #ccc35c | 55 deg |
| DRONE | LOW / 0.62 | crimson | #ff0055 | 340 deg |
| AIR TAG | MEDIUM / 0.80 | aquamarine | #26ffc9 | 165 deg |
| KEYFOB | MEDIUM / 0.80 | orchid | #b95ccc | 290 deg |
| META | MEDIUM / 0.80 | green | #00ff6a | 145 deg |
| SKIMMER | HIGH / 1.00 | yellow | #ffd400 | 50 deg |
| FLIPPER | HIGH / 1.00 | azure | #0080ff | 210 deg |
| SAMSUNG | MEDIUM / 0.80 | blue | #0000ff | 240 deg |
| AXON | MEDIUM / 0.80 | rose | #cc5c6a | 352 deg |
| RAVEN | LOW / 0.62 | red-orange | #ff4000 | 15 deg |
| ALPR | LOW / 0.62 | royal blue | #0033cc | 225 deg |
| CAMERA | MEDIUM / 0.80 | magenta | #ff00ff | 300 deg |
| RING | LOW / 0.62 | sky blue | #00bfff | 195 deg |
| GOOGLE TAG | MEDIUM / 0.80 | orange | #ff9500 | 35 deg |
| EVIL TWIN | HIGH / 1.00 | hot pink | #ff00aa | 320 deg |
| IBEACON | LOW / 0.62 | leaf green | #78cc5c | 105 deg |

## What the idle still looks like

My own look at idle_3x.png (v5d, 3:22 PM ET):
- **Black Matrix towers.** They read black, with green code rails and an outline. FLIPPER (blue dolphin, centre), DEAUTH (cyan wifi, right back) and EVIL TWIN (left back) are clearly black.
- **HACK is off screen.** HACK sits at the far left edge: in this still its face is at x -28..-4, so only a black sliver shows at the left border and its emblem is not visible.
- **Wood towers.** The other towers read as see-through wood with opaque coloured emblems and green/white folder rows.
- **Not all logos are visible.** HACK is off screen. META, SAMSUNG and RAVEN are hidden behind nearer towers in this still (0 emblem px). The other 14 emblems are visible.
- **No stumps.** The shortest on-screen towers are still mid-height pillars with an emblem and 6+ rows; nothing reads as a stump.
- **Near towers are visibly shorter than in v5c.** The far towers behind show emblems above them.
- **The front floor (bottom ~40%) is still empty.**

## Idle still

`idle_1x.png` / `idle_3x.png` = idle sample **202 of 320** (median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample; quality 1.0887, median 1.0882).

## Per pillar (real sim)

| # | device | group x hk = H | sx | z0 | emblem S | idle px | vis % | tap cell % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | 1.00 x 1.068 = 2.776 | 186.9 | 3.829 | 8 | 64 | 97 | 100.0 | 35 | 39 |
| 1 | HACK | 1.00 x 0.602 = 1.566 | 34.0 | 1.714 | 22 | 0 | 86 | 100.0 | 21 | 37 |
| 2 | FLOCK | 0.62 x 0.746 = 1.203 | 96.0 | 2.629 | 14 | 196 | 62 | 99.7 | 0 | 9 |
| 3 | DRONE | 0.62 x 0.694 = 1.118 | 60.3 | 2.400 | 18 | 324 | 55 | 99.7 | 0 | 9 |
| 4 | AIR TAG | 0.80 x 1.140 = 2.370 | 68.8 | 4.171 | 8 | 64 | 51 | 97.8 | 0 | 9 |
| 5 | KEYFOB | 0.80 x 0.995 = 2.070 | 187.4 | 3.543 | 10 | 40 | 43 | 100.0 | 1 | 9 |
| 6 | META | 0.80 x 0.805 = 1.674 | 116.3 | 2.857 | 12 | 0 | 55 | 98.7 | 0 | 9 |
| 7 | SKIMMER | 1.00 x 0.651 = 1.692 | 194.2 | 2.171 | 16 | 256 | 93 | 100.0 | 7 | 15 |
| 8 | FLIPPER | 1.00 x 0.619 = 1.610 | 132.7 | 1.943 | 18 | 324 | 95 | 100.0 | 3 | 21 |
| 9 | SAMSUNG | 0.80 x 1.191 = 2.478 | 127.6 | 4.571 | 6 | 0 | 56 | 99.1 | 0 | 15 |
| 10 | AXON | 0.80 x 0.633 = 1.317 | 202.5 | 2.057 | 18 | 324 | 73 | 100.0 | 1 | 15 |
| 11 | RAVEN | 0.62 x 0.964 = 1.554 | 164.8 | 3.429 | 10 | 0 | 36 | 99.4 | 0 | 5 |
| 12 | ALPR | 0.62 x 1.106 = 1.783 | 36.2 | 4.000 | 8 | 8 | 33 | 97.2 | 0 | 5 |
| 13 | CAMERA | 0.80 x 0.609 = 1.266 | 141.9 | 1.829 | 20 | 400 | 92 | 100.0 | 11 | 31 |
| 14 | RING | 0.62 x 1.167 = 1.881 | 77.0 | 4.343 | 8 | 36 | 32 | 97.5 | 0 | 5 |
| 15 | GOOGLE TAG | 0.80 x 0.671 = 1.395 | 69.1 | 2.286 | 16 | 96 | 70 | 100.0 | 7 | 11 |
| 16 | EVIL TWIN | 1.00 x 0.836 = 2.174 | 52.1 | 2.971 | 12 | 144 | 66 | 100.0 | 17 | 35 |
| 17 | IBEACON | 0.62 x 1.040 = 1.676 | 206.0 | 3.714 | 8 | 64 | 36 | 99.4 | 0 | 5 |

## Gate: 23/31 (`gate.json`)

- FAIL: bodies (v5d): the 5 HIGH towers (DEAUTH, HACK, SKIMMER, FLIPPER, EVIL TWIN) read as black Matrix code glass (T1), the other 13 as see-through wood (T3); pixel class from each face's own opaque outline / median pixels
- FAIL: emblem visibility (idle still): every pillar shows emblem pixels
- FAIL: emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5
- FAIL: tap: every pillar keeps a tap cell in every idle sample (100%)
- FAIL: tap: p10 largest square >= 20 px for every pillar
- FAIL: floor coverage: largest empty standable-floor square <= 60 px (idle + 320-sample median), whole floor <= 75 px, 5-7 bases per screen third and >= 1 front-row base (y >= 200) in each third (v4 107 px, v5a 71 px whole floor)
- FAIL: emblem colours: 18 distinct saturated colours with 18 distinct hues (min hue step >= 5 deg), min pairwise CIE76 deltaE >= 30, screen-adjacent towers deltaE >= 40, every colour deltaE >= 30 from the wood tones, >= 16 of 18 drawn in the idle still
- FAIL: stills: idle @1x 240x320, idle + 4 close-ups @3x 720x960

Temporal stability: 243 pairs, mean |dY| 0.0, worst 0.0. Opaque UI: 478575846 UI pixels, 0 translucent. The tap rule was not changed.

## Files

| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still |
| `closeup_deauth_high_3x.png` | close-up, height group 1.00, black Matrix body, folders open |
| `closeup_airtag_medium_3x.png` | close-up, height group 0.80 |
| `closeup_flock_low_3x.png` | close-up, height group 0.62 |
| `emblem_colors.png` | 18 emblems in their colours on wood (16 / 24 / 40 px) |
| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened (lock, HACKbot search, folders); 720x960 h264 |
| `open_sequence_deauth_3x.png` | DEAUTH open: 0.18 s lock brackets, 1.8 s HACKbot search (no folders), 5.16 s folders landed, 6.5 s |
| `pullup_gibson_vs_morpheus_2x.png` | top: real Gibson radar charge lock; bottom: Morpheus open; same ms since the touch |
| `folders.md` | folder text (unchanged from v4) |
| `gate.json`, `SHA256SUMS` | gates, checksums |
| `src/`, `tree_patch/` | generator sources, patched headers |
