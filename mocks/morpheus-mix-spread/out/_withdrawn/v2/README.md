# MORPHEUS MIX SPREAD (mock, 2026-11-14)

**Mock only.** Nothing was flashed, merged, or committed as firmware. Every number below comes from the real 240x320 RGB565 sim
(`tree/tools/livescan/sim_live.cpp`, Gibson engine, built with zig c++). EX.DATA only. The previous 20-pillar mock (v1) is in `_withdrawn/v1/`.

## What changed (Mobandy, 2026-11-14 ET)
1. **7:52 AM, spread the towers out** so emblems and taps clear. New screen column (`sx`) and depth (`z0`) for every pillar, picked by
   simulated annealing on a Python model of the room (`src/opt18/`), then re-measured on the real sim. Layout: a stadium in three
   depth bands, tall green T1 at the back, wood T3 in the middle, short white T2 in front, columns spread across the room.
2. **7:53 AM, only three heights.** One fixed world height per style replaces the engine's rssi height. The idle height breathing
   (the 6% sway) is also off, so there are exactly three heights at all times (gate-checked over 320 samples). `MO_HFIX` in `lv_morpheus_v13.h`:

| style | threat | world height H |
|---|---|---|
| T1 code glass (green) | HIGH | **2.47** (tallest) |
| T3 see-through wood | MEDIUM | **1.16** (middle) |
| T2 white light | LOW | **0.58** (shortest) |

   As screen factors relative to T3, that is T1 x2.13, T3 x1.00, T2 x0.50.
3. **7:54 AM, WIFI and BLUETOOTH removed.** 18 pillars: 5 T1 (DEAUTH, HACK, SKIMMER, FLIPPER, EVIL TWIN), 7 T3 (AIR TAG, KEYFOB, SAMSUNG,
   GOOGLE TAG, META, CAMERA, AXON), 6 T2 (FLOCK, DRONE, ALPR, RAVEN, RING, IBEACON). Every per-device table lost those two rows; nothing else in them changed.
   The WIFI close-up is replaced by a FLOCK (white, LOW) close-up.
4. **8:08 AM, only the 7 wood towers become SEE-THROUGH WOOD.** The T3 body is now translucent wood: grain at about 55% alpha, so the room,
   the rain and the towers behind show through. It keeps thin brass bands, the green trim and a faint warm inner light. The emblem and the scrolling file line-items sit inside.
   The old opaque paper-lantern panel is gone from the pillar; the old code is in `_withdrawn/v1/src/mo_mix.inc`. The T1 and T2 drawing code is byte-identical to v1
   (only the T3 block and the device tables differ, `diff _withdrawn/v1/src/mo_mix.inc src/mo_mix.inc`).

Unchanged: the v11 drift law (dx, dz) with each device's own drift tables, the camera sway, the close-up open state with files scrolling inside
the tower (never a square grid), the outlined folders, the chrome, and the vector emblems (16 px idle when the pillar is wide enough, 40 px close-up). No red pill, no lock bracket.

Things the brief did not cover (flagged, not changed):
- The terminal chrome still reads `HEIGHT = EXAMPLE SIGNAL`, which is now untrue because height is fixed by style.
- The T3 close-up folders keep their v1 wood palette.

## Idle still
`idle_1x.png` / `idle_3x.png` show **idle sample 251 of 320** (0.3 s apart).
- **How it was picked:** the MEDIAN sample by quality (mean emblem visible fraction + mean tap square / 60). It is the median, not the best: quality 1.5074, range 1.394 to 1.5833.
- **Video:** `idle_then_open_3x.mp4` starts 128 ticks (3.8 s) before this sample, then opens DEAUTH, AIR TAG, FLOCK and RAVEN.

## Per pillar (real sim)
- **idle px:** emblem pixels visible in the idle still.
- **vis %:** mean visible fraction over 320 idle samples.
- **min sq:** the smallest tap square in any sample.
- **p10:** the 10th-percentile largest tap square, in px.

| # | device | threat | look | H | sx | z0 | idle px | vis % | min sq | p10 |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | DEAUTH | HIGH | T1 code glass | 2.47 | 25.0 | 4.023 | 64 | 93 | 17 | 35 |
| 1 | HACK | HIGH | T1 code glass | 2.47 | 74.4 | 3.997 | 64 | 100 | 37 | 41 |
| 2 | FLOCK | LOW | T2 white light | 0.58 | 139.0 | 1.788 | 256 | 98 | 15 | 39 |
| 3 | DRONE | LOW | T2 white light | 0.58 | 198.7 | 2.338 | 100 | 97 | 19 | 29 |
| 4 | AIR TAG | MEDIUM | T3 see-through wood | 1.16 | 207.4 | 3.135 | 64 | 97 | 5 | 13 |
| 5 | KEYFOB | MEDIUM | T3 see-through wood | 1.16 | 25.0 | 3.237 | 64 | 89 | 3 | 11 |
| 6 | META | MEDIUM | T3 see-through wood | 1.16 | 62.4 | 2.934 | 64 | 95 | 5 | 13 |
| 7 | SKIMMER | HIGH | T1 code glass | 2.47 | 129.0 | 3.828 | 64 | 100 | 41 | 43 |
| 8 | FLIPPER | HIGH | T1 code glass | 2.47 | 205.4 | 3.658 | 100 | 98 | 15 | 29 |
| 9 | SAMSUNG | MEDIUM | T3 see-through wood | 1.16 | 148.3 | 3.009 | 64 | 100 | 11 | 15 |
| 10 | AXON | MEDIUM | T3 see-through wood | 1.16 | 113.3 | 2.895 | 100 | 99 | 9 | 13 |
| 11 | RAVEN | LOW | T2 white light | 0.58 | 113.8 | 1.828 | 256 | 88 | 11 | 21 |
| 12 | ALPR | LOW | T2 white light | 0.58 | 162.4 | 2.609 | 100 | 91 | 7 | 13 |
| 13 | CAMERA | MEDIUM | T3 see-through wood | 1.16 | 176.6 | 3.102 | 100 | 97 | 7 | 9 |
| 14 | RING | LOW | T2 white light | 0.58 | 44.4 | 2.199 | 30 | 90 | 9 | 15 |
| 15 | GOOGLE TAG | MEDIUM | T3 see-through wood | 1.16 | 89.7 | 2.914 | 100 | 94 | 11 | 13 |
| 16 | EVIL TWIN | HIGH | T1 code glass | 2.47 | 174.7 | 3.861 | 64 | 100 | 25 | 29 |
| 17 | IBEACON | LOW | T2 white light | 0.58 | 71.2 | 1.650 | 256 | 100 | 31 | 49 |

## Gate: 21/22 (`gate.json`)
- FAIL: tap: p10 largest square >= 20 px for every pillar

The failure is honest and structural:
- **Which pillars fail:** AIR TAG, KEYFOB, META, SAMSUNG, AXON, ALPR, CAMERA, RING, GOOGLE TAG are below a 20 px p10.
- **Why:** the tap assist (`mo_hit`, nearest pillar centre line) gives each pillar a cell around a vertical segment, and every segment
  of a pillar farther than the front row crosses the horizon band. Only two depth bands can own a free strip of the field: the tallest
  tops above, and the nearest bottoms below. The third band (here the 7 wood pillars) only gets a horizontal slice between its neighbours.
  12 or more segments share 240 px there, which is about 20 px on average, and the drift plus the camera sway push the 10th percentile down to about 9 to 15 px.
- **What the search tried:** 8 hill-climb seeds plus 16 simulated-annealing seeds over sx / z0 / the three heights (8 of the annealing runs were stopped early when the second strategy took over). None reached 20 px for the
  middle band (`src/opt18/`). Fixing it needs a change outside this brief: a tap-assist rule, fewer pillars, or stubby same-height wood and white pillars.
- **What does pass:** every pillar keeps a tap cell in 100% of the 320 samples, and every emblem is at least 87.7% visible on average (worst RAVEN).

## Files
| file | content |
|---|---|
| `idle_1x.png`, `idle_3x.png` | idle still (sample 251) |
| `closeup_deauth_high_3x.png` | T1 code glass close-up |
| `closeup_airtag_medium_3x.png` | **see-through wood** close-up (AIR TAG) |
| `closeup_flock_low_3x.png` | T2 white close-up (replaces WIFI) |
| `closeup_raven_new_3x.png` | T2 white close-up, new device |
| `emblems_old_vs_new.png` | 18 emblems: old 16x16 art vs new vector at 16 and 40 px |
| `idle_then_open_3x.mp4` | 708 frames, 21.2 s, @3x 720x960 h264 yuv420p |
| `gate.json`, `SHA256SUMS` | gates and checksums |
| `src/` | `patch_tree.py`, `make_mix_inc.py` -> `mo_mix.inc`, `mo_emblem.h`, `layout18.json`, `opt18/`, `package.py`, `drive.py`, `touch20.py`, `mix.json`, `emb_sheet.cpp` (v1-only tuning files in `src/_withdrawn_v1_only/`) |
| `tree_patch/` | the two patched headers |
