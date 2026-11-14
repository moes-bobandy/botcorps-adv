# MORPHEUS MIX SPREAD v5f (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number comes from the real 240x320 RGB565 sim (same pipeline as v4).
Earlier versions are in `_withdrawn/v1` .. `v4` (v4 = the 10:15 AM ET renders, rejected 11:51 AM ET; v5a = the first v5 render, rejected 12:42 PM ET, see `_withdrawn/v5a/NOTE.txt`; v5b = commit 512fbb3, superseded 2:05 PM ET, `_withdrawn/v5b/`; v5c = commit d68abf4, superseded 2:34 PM ET, `_withdrawn/v5c/`; v5d = commit bf90bda, open screen rejected 3:23 PM ET, `_withdrawn/v5d/`; v5e = commit d0fbab7, superseded 3:48 PM ET, `_withdrawn/v5e/`).

## What changed in v5f (Mobandy 3:24 / 3:48 PM ET)

**Folder squares redesigned as digital instruments.** The HACKbot-shaped Morpheus scroll frame is kept: tab, opaque double outline, and the search-then-drop order. Every element maps to a real value:

| Scroll | Element | Value it shows |
|---|---|---|
| All | Header: device emblem + real folder name | Unchanged |
| All | Fact line | MO_FOLD_ACT |
| SCROLL 1 (what it is) | 7-segment readout | The device's dBm (EX.DATA rssi) |
| SCROLL 1 | 10-LED signal meter | Lit to the Gibson level of that dBm: (rssi + 100, 4..60) x 10 / 60 |
| SCROLL 1 | Threat: warning triangle + 3 segments | HIGH = 3, MEDIUM = 2, LOW = 1, in the threat colour |
| SCROLL 2 (how spotted) | Lit RADIO badge | MO_FOLD_TAG radio: WIFI / BLE / WI+BT |
| SCROLL 2 | Lit band badge | From the device's GHZ item (e.g. 2.4G) |
| SCROLL 2 | Up to 2 numbers as 7-segment digits, labelled | The device's numeric items (e.g. DEAUTH SUBTYPE 12, DISASOC 10; FLOCK OUI 49) |
| SCROLL 3 (protect) | Shield-check icon + DO THIS | Marks the header line as the action to take |

**Removed:** the progress bar, LOADED / %, the landing light sweep and the idle glow cycle. None of them showed data.

**Rain on the 5 black towers is brighter.** It is the same lv_radar.h code-streak law (x, length, speed, head), now with:
- denser streams (face width / 2);
- a continuous trail instead of every 2nd pixel;
- a bright 0xd8ffe6 head;
- trail 0x10d858 then 0x00a83c, full strength for the first half, then fading.

The rain is still drawn behind the file rows and the emblem.

**Unchanged from v5e:** idle placement, solid wood, attached emblems (fixed size, inset), and the left tower.

## What changed in v5e (Mobandy 3:23 - 3:25 PM ET)

**Withdrawn.** The v5d open screen was rejected because the HACKbot UI sat on top of the Morpheus screen and the stat bars and strips meant nothing. All of it is gone: the Gibson lock brackets, the HACKbot key bar, the 11-band meter, the folder chips, the meters and the load bars.

**Open screen.** This is v5c's Morpheus open state again, code for code:
- the camera push;
- the charcoal search bar hung from the tower by timber posts;
- three green scrolls on DOWNLOAD's eased stagger, with the index line.

**Search step.** Only the file-search step changed. The scrolls now drop 200 ms after the search bar has finished typing the device name, so the search comes first and the files follow. In v5c they started at 300 ms while the name was still typing.
- Every element of the search bar is real: the typed text is the device name, and "n/3 SCROLLS" counts the scrolls that have landed.
- Gate, from the sim's own state: DEAUTH at 0.54 s: 0 scrolls, 5 letters typed; at 1.8 s: 3 scrolls, 6 letters; AIR TAG at 0.54 s: 0 scrolls, 5 letters typed; at 1.8 s: 3 scrolls, 7 letters; FLOCK at 0.54 s: 0 scrolls, 5 letters typed; at 1.8 s: 3 scrolls, 5 letters; RAVEN at 0.54 s: 0 scrolls, 5 letters typed; at 1.8 s: 3 scrolls, 5 letters.

**Solid wood.** The 13 wood towers are opaque again: front grain, sides and cap are drawn at full alpha.

**Rain on the 5 black Matrix towers.** The towers are DEAUTH, HACK, SKIMMER, FLIPPER and EVIL TWIN.
- The old dim glyph or 1 px code inside the glass is replaced by the CYD code-streak rain from `lv_radar.h`, the radar field's "code streaks" (`lv_rd_bg` case 0, RAIN), with the same law:

  | part | value |
  |---|---|
  | x | (s*53+17) mod width |
  | length | 10+(s*7) mod 18 |
  | speed | 40+(s*29) mod 70 px/s |
  | head | (t*speed + s*37) mod (height+length) |
  | pixels lit | every 2nd |
  | head colour | 0x10d858 |
  | tail colour | 0x00a83c |
  | fade | 8/8 at the head, then 6-k*5/len |

- It is mapped into each tower's own face: 18 streams in the close-up, width/3 when idle, offset per tower.
- It is drawn before the file rows and the emblem, so both stay opaque on top.

**Emblems bolted to the face.** Each tower now has one fixed idle emblem size: its narrowest drifting face width over 320 real-sim samples minus 3, so there is at least 1 px inset on each side (`src/emb_fix_v5e.json`).
- The emblem is centred on its own face with the same transform, so it never changes size or jumps while idle. It switches to 40 px only in the open close-up.
- Gate over 320 samples:
  - 0 rects fall outside their face;
  - centre-to-face-centre x offset varies by at most 0.5 px;
  - top offset varies by at most 1 px.

**Unchanged from v5d:** the idle room (shorter near towers, Gibson depth, independent drift, colours, 5 black and 13 wood), the camera, and tw 1.35.

### v5d height factors (kept)

The shortest tower (RING) is 47.3% of the tallest on-screen tower. The minimum is 7 idle folder rows.

| tower (near -> far) | Z | hk | H | median on-screen px | idle folder rows |
|---|---|---|---|---|---|
| HACK | 1.71 | 0.602 | 1.566 | 110 px | 21 |
| CAMERA | 1.83 | 0.609 | 1.266 | 83 px | 13 |
| FLIPPER | 1.94 | 0.619 | 1.610 | 100 px | 19 |
| AXON | 2.06 | 0.633 | 1.317 | 76 px | 13 |
| SKIMMER | 2.17 | 0.651 | 1.692 | 94 px | 14 |
| GOOGLE TAG | 2.29 | 0.671 | 1.395 | 73 px | 10 |
| DRONE | 2.40 | 0.694 | 1.118 | 56 px | 8 |
| FLOCK | 2.63 | 0.746 | 1.203 | 55 px | 8 |
| META | 2.86 | 0.805 | 1.674 | 71 px | 12 |
| EVIL TWIN | 2.97 | 0.836 | 2.174 | 87 px | 16 |
| RAVEN | 3.43 | 0.964 | 1.554 | 55 px | 9 |
| KEYFOB | 3.54 | 0.995 | 2.070 | 69 px | 14 |
| IBEACON | 3.71 | 1.040 | 1.676 | 54 px | 7 |
| DEAUTH | 3.83 | 1.068 | 2.776 | 87 px | 16 |
| ALPR | 4.00 | 1.106 | 1.783 | 53 px | 8 |
| AIR TAG | 4.17 | 1.140 | 2.370 | 68 px | 11 |
| RING | 4.34 | 1.167 | 1.881 | 52 px | 7 |
| SAMSUNG | 4.57 | 1.191 | 2.478 | 65 px | 10 |

The black towers are DEAUTH, HACK, SKIMMER, FLIPPER, EVIL TWIN.

## What changed in v5 (Mobandy 11:51 / 11:52 AM ET)

1. **All wood (v5, changed in v5d):** in v5 / v5c all 18 bodies were see-through wood (T3). v5d makes the 5 HIGH towers black Matrix code glass (see above); the other 13 stay wood.
   - The opaque emblem, on-tower folder glyphs and file line-items are unchanged; only the body is see-through.
2. **Group heights as v4:** green group 1.00 (2.6), wood group 0.80 (2.08), white group 0.62 (1.612). v5d multiplies each by the fixed depth factor hk (above). No breathing.
3. **A colour per device.** Each emblem gets its own saturated colour (`src/emb_colors.py`):
   - 18 colours with 18 distinct hues (min hue step 5 deg; v5a had crimson and rose both at 340 deg, rose is now #cc5c6a at 352 deg). They are dealt by a swap search that maximises the smallest deltaE between towers that are screen neighbours at any of 64 sampled idle times.
   - Emblems are still crisp binary masks with a 1 px dark outline, drawn pixel-aligned and opaque.
   - Min pairwise deltaE (CIE76) = 36.4 (FLOCK / IBEACON). Min between screen neighbours = 58.0 (SAMSUNG / CAMERA). Min distance from the wood tones = 36.3.
4. **Width reverted:** `tw` is back to **1.35** (v4 had 2.2, which was unrequested).
5. **Spread (11:52 AM / 12:42 PM ET) - front floor NOT filled.** Columns re-picked (`src/opt18/opt18e.py`, seed 952, depth fixed by item 6) on a corrected occlusion model (a nearer pillar's box now starts at min(front top, back top); v5a missed the front-face top strip, which also caused the false emblem 'flicker'), with an emblem visibility floor, empty-floor and screen-third balance terms.
   - Largest empty floor square, whole visible floor (rows 140-285): v4 idle **107 px**, v5a idle **71 px**, v5b idle **83 px** (median over 320 samples 87.0, worst 97).
   - Standable floor (rows 140-251; a base lower than y 251 needs z0 < 0.95 and the tower top leaves the field): v5a idle **43 px**, v5b idle **63 px** (median 61.0, worst 73). Bases per screen third L/M/R: v5a 6/8/4, v5b 2/8/8; front-row bases per third 1/0/1.
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

My own look (v5f, 4:02 PM ET).

**Idle and video.** I stepped through idle_then_open_3x.mp4 (8 frames 12 video frames apart, cropped to the towers).
- In every frame the emblems (HACK skull, FLIPPER dolphin, SKIMMER, FLOCK and the others) stay on their own tower's top face. They keep the same size and move with the tower. None floats off or changes size.
- The wood towers are solid.
- The 5 black towers now show visible green rain streaks with bright heads. At idle size they still read mostly black with green streaks, not a dense curtain.

**Open screen (sim preview: DEAUTH, FLOCK, AIR TAG).**
- SCROLL 1: 7-segment dBm readout (DEAUTH -79), a 10-LED signal meter lit to that level, and the threat as a warning triangle plus 3 red segments for HIGH (FLOCK LOW = 1 lit, AIR TAG MEDIUM = 2 lit, amber).
- SCROLL 2: lit WIFI and 2.4G badges plus SUBTYPE 12 and DISASOC 10 as 7-segment digits for DEAUTH. FLOCK shows a WI+BT badge and OUI 49. AIR TAG shows a BLE badge and TYPE 12.
- SCROLL 3: shield-check icon with DO THIS under the action header.
- No progress bars or strips remain. The 7-segment digits keep their unlit segments dim, which looks like a real LED display but reads slightly busy at 1x.

## Idle still

`idle_1x.png` / `idle_3x.png` = idle sample **261 of 320** (median of per-sample quality (mean emblem visible fraction + mean tap square/60) over the samples where all 18 pillars show some emblem and keep a tap cell; not the best sample; quality 1.1031, median 1.1031).

## Per pillar (real sim)

| # | device | group x hk = H | sx | z0 | emblem S | idle px | vis % | tap cell % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | 1.00 x 1.068 = 2.776 | 186.9 | 3.829 | 6 | 36 | 99 | 100.0 | 35 | 39 |
| 1 | HACK | 1.00 x 0.602 = 1.566 | 34.0 | 1.714 | 18 | 324 | 86 | 100.0 | 21 | 37 |
| 2 | FLOCK | 0.62 x 0.746 = 1.203 | 96.0 | 2.629 | 10 | 100 | 64 | 99.7 | 0 | 9 |
| 3 | DRONE | 0.62 x 0.694 = 1.118 | 60.3 | 2.400 | 11 | 121 | 54 | 99.7 | 0 | 9 |
| 4 | AIR TAG | 0.80 x 1.140 = 2.370 | 68.8 | 4.171 | 5 | 0 | 55 | 97.8 | 0 | 9 |
| 5 | KEYFOB | 0.80 x 0.995 = 2.070 | 187.4 | 3.543 | 6 | 0 | 46 | 100.0 | 1 | 9 |
| 6 | META | 0.80 x 0.805 = 1.674 | 116.3 | 2.857 | 9 | 81 | 56 | 98.7 | 0 | 9 |
| 7 | SKIMMER | 1.00 x 0.651 = 1.692 | 194.2 | 2.171 | 13 | 169 | 96 | 100.0 | 7 | 15 |
| 8 | FLIPPER | 1.00 x 0.619 = 1.610 | 132.7 | 1.943 | 15 | 225 | 98 | 100.0 | 3 | 21 |
| 9 | SAMSUNG | 0.80 x 1.191 = 2.478 | 127.6 | 4.571 | 4 | 16 | 57 | 99.1 | 0 | 15 |
| 10 | AXON | 0.80 x 0.633 = 1.317 | 202.5 | 2.057 | 14 | 28 | 73 | 100.0 | 1 | 15 |
| 11 | RAVEN | 0.62 x 0.964 = 1.554 | 164.8 | 3.429 | 7 | 0 | 37 | 99.4 | 0 | 5 |
| 12 | ALPR | 0.62 x 1.106 = 1.783 | 36.2 | 4.000 | 6 | 24 | 34 | 97.2 | 0 | 5 |
| 13 | CAMERA | 0.80 x 0.609 = 1.266 | 141.9 | 1.829 | 17 | 289 | 92 | 100.0 | 11 | 31 |
| 14 | RING | 0.62 x 1.167 = 1.881 | 77.0 | 4.343 | 5 | 0 | 36 | 97.5 | 0 | 5 |
| 15 | GOOGLE TAG | 0.80 x 0.671 = 1.395 | 69.1 | 2.286 | 13 | 129 | 70 | 100.0 | 7 | 11 |
| 16 | EVIL TWIN | 1.00 x 0.836 = 2.174 | 52.1 | 2.971 | 9 | 81 | 69 | 100.0 | 17 | 35 |
| 17 | IBEACON | 0.62 x 1.040 = 1.676 | 206.0 | 3.714 | 6 | 12 | 37 | 99.4 | 0 | 5 |

## Gate: 24/32 (`gate.json`)

- FAIL: emblem visibility (idle still): every pillar shows emblem pixels
- FAIL: emblem visibility (320 idle samples): every pillar mean visible fraction >= 0.5
- FAIL: tap: every pillar keeps a tap cell in every idle sample (100%)
- FAIL: tap: p10 largest square >= 20 px for every pillar
- FAIL: floor coverage: largest empty standable-floor square <= 60 px (idle + 320-sample median), whole floor <= 75 px, 5-7 bases per screen third and >= 1 front-row base (y >= 200) in each third (v4 107 px, v5a 71 px whole floor)
- FAIL: emblem colours: 18 distinct saturated colours with 18 distinct hues (min hue step >= 5 deg), min pairwise CIE76 deltaE >= 30, screen-adjacent towers deltaE >= 40, every colour deltaE >= 30 from the wood tones, >= 16 of 18 drawn in the idle still
- FAIL: folder content: all 18 devices have real text (3 folders ID / DETECT / ACTION + 8 KEY / VALUE line-items); no EX.DATA / lorem / fake KB sizes; no folder name, detail line or line-item block shared across devices; every string fits its slot; tables wired into the renderer; close-up folder panels all differ
- FAIL: emblems: idle emblem as large as the face allows (S from face width - 4 to face width, 2 px steps with hysteresis; was 8 / 10 / 16 px steps)

Temporal stability: 256 pairs, mean |dY| 0.011, worst 2.8. Opaque UI: 330972208 UI pixels, 0 translucent. The tap rule was not changed.

## Files

| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still |
| `closeup_deauth_high_3x.png` | close-up, height group 1.00, black Matrix body with code rain, scrolls open |
| `closeup_airtag_medium_3x.png` | close-up, height group 0.80 |
| `closeup_flock_low_3x.png` | close-up, height group 0.62 |
| `emblem_colors.png` | 18 emblems in their colours on wood (16 / 24 / 40 px) |
| `idle_then_open_3x.mp4` | idle, then DEAUTH, AIR TAG, FLOCK, RAVEN opened (search, then scrolls); 720x960 h264 |
| `open_sequence_deauth_3x.png` | DEAUTH open: 0.18 s push, 0.54 s search typing (no scrolls), 1.8 s scrolls landed, 3.15 s |
| `folders.md` | folder text (unchanged from v4) |
| `gate.json`, `SHA256SUMS` | gates, checksums |
| `src/`, `tree_patch/` | generator sources, patched headers |
