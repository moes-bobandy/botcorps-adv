# screen-glow: locked C face glow on the TERMINALbot, MORPHEUS and SCANbot screens (MOCK ONLY)

Mobandy, 2026-11-03 5:16 PM ET: "Make the glow on the terminal screen and Morpheus screen"; 5:17 PM ET: "Same with scan bot".
Mock only: no firmware build, no flash. Built on the LOCKED screen videos from cyd-lock-store; nothing in LOCKED/ was modified.

## What is applied

The locked home-board C breathing face glow (as locked in LOCKED/ui_home_tap_glow, variant O): AMAX 0.80, ring radius RC = 7 px at 1x,
alpha = AMAX*((RC+1-d)/RC)^2*s, breath s = 0.5-0.5cos(2*pi*t/1.4 s) in sync for every chip, face lift 255*(x/255)^(1/(1+0.15 s)).
Each tappable device chip glows in its own face colour. The ring only touches black-window / grey-bezel pixels inside that chip's face window
(the bezel area between the face and the chip's dash ring: never the dash ring, lime/pink/purple dots, bevel, label, pins or anything animated).
The lift skips dark pixels, so eyes and black screens are untouched. **The big main bots (TERMINALbot, SCANbot, MORPHEUS) are not glowed.**

Method: the locked MP4s are lossy, so the glow is applied to the decoded locked frames (3x). Masks are built at 1x from the median frame
(`prep_locked.py`, `detect.py`) and expanded to 3x blocks. Every pixel outside face+window is copied byte-for-byte from the decoded locked frame.

Files: `prep_locked.py` (decode + median/range), `detect.py` (face + window per chip), `glow_screens.py` (apply, diff, stills, encode),
`verify_mp4.py` (codec/encode checks), `*_stats.json` (per-chip rects, colours, pixel counts).

MP4 encode: libx264 Main, yuv420p, CRF 12, +faststart (moov at byte 36), at the locked fps and frame count.

## TERMINAL

Locked source: LOCKED/ui_terminalbot/terminalbot_bg@3x.mp4 (sha256 2c6a04b0…, 720x960, 100/3 fps, 600 frames; locked 2026-10-28 "Looks good. Lock this ui in please")

| chip | face (1x x0,y0,x1,y1) | window | glow colour | ring px 1x | lift px 1x |
|---|---|---|---|---|---|
| DEAUTH | (41, 42, 62, 63) | (36, 39, 69, 69) | (221, 72, 81) | 432 | 452 |
| META | (176, 42, 197, 63) | (171, 39, 204, 69) | (148, 0, 254) | 432 | 452 |
| HACK | (41, 99, 62, 120) | (36, 96, 69, 126) | (23, 210, 65) | 432 | 452 |
| SKIMMER | (176, 99, 197, 120) | (171, 96, 204, 126) | (221, 72, 173) | 432 | 452 |
| FLOCK | (41, 156, 62, 177) | (36, 153, 69, 183) | (180, 218, 81) | 432 | 452 |
| FLIPPER | (176, 156, 197, 177) | (171, 153, 204, 183) | (247, 91, 24) | 432 | 452 |
| DRONE | (41, 213, 62, 234) | (36, 210, 69, 240) | (253, 253, 0) | 432 | 452 |
| AIR TAG | (87, 213, 108, 234) | (82, 210, 115, 240) | (253, 185, 73) | 432 | 452 |
| KEYFOB | (130, 213, 151, 234) | (125, 210, 158, 240) | (90, 254, 196) | 432 | 452 |
| SAMSUNG | (176, 213, 197, 234) | (171, 210, 204, 240) | (230, 92, 254) | 432 | 452 |

- Glowed: DEAUTH, META, HACK, SKIMMER, FLOCK, FLIPPER, DRONE, AIR TAG, KEYFOB, SAMSUNG (10 chips, solid faces, colour = face colour).
- Not glowed: big TERMINALbot (flag: is it tappable?), bottom terminal panel and its icons, pins, rain/trace area.
- Read: clear, even halo in each chip's colour on the bezel around the face; reads like the home board. Top side of each face has only 3 px before the bevel, so the halo is thicker on the sides and bottom.
- Diff vs decoded locked frames (PNG, before encode): **0 frames / 0 px changed outside face+window** over 600 frames; max changed ring px per frame 38880, lift px 38624 (3x); max |delta| 203.
- Peak still: frame 71 (s = 1.000). Loop: 18.00 s clip, 1.4 s breath kept, so the breath jumps at the loop point (s = 0 at the start, s = 0.188 at the wrap).
- Encode check: `terminal h264,Main,720,960,yuv420p,100/3,600 | moov at byte 36 | frames compared 600 | outside region: mean|d| 2.715, max 139, px>8 22336517 of 357804000 (6.2427%)`
- Files: `terminal_glow_3x.mp4`, `terminal_glow_peak_3x.png`, `terminal_locked_vs_glow_sbs.png`, `terminal_stats.json`.

## MORPHEUS

Locked source: LOCKED/ui_glossary_morpheus_rain/chiprain_black_soft_final_big_morpheus_green.mp4 (720x960, 8 fps, 74 frames; newest MORPHEUS lock)

| chip | face (1x x0,y0,x1,y1) | window | glow colour | ring px 1x | lift px 1x |
|---|---|---|---|---|---|
| WIFI | (40, 41, 62, 63) | (33, 38, 69, 67) | (253, 104, 0) | 439 | 86 |
| DEAUTH | (178, 41, 200, 63) | (171, 38, 207, 67) | (89, 32, 239) | 439 | 38 |
| BLUETOOTH | (40, 104, 62, 126) | (33, 101, 69, 130) | (47, 95, 255) | 442 | 120 |
| HACK | (178, 104, 200, 126) | (171, 101, 207, 130) | (206, 254, 0) | 435 | 109 |
| FLOCK | (40, 171, 62, 193) | (33, 168, 69, 197) | (173, 36, 247) | 443 | 75 |
| DRONE | (86, 171, 108, 193) | (79, 168, 115, 197) | (188, 188, 188) | 440 | 39 |
| AIR TAG | (132, 171, 154, 193) | (125, 168, 161, 197) | (255, 255, 0) | 441 | 122 |
| KEYFOB | (178, 171, 200, 193) | (171, 168, 207, 197) | (73, 79, 250) | 443 | 66 |
| META | (40, 241, 62, 263) | (33, 238, 69, 267) | (250, 41, 212) | 445 | 74 |
| SKIMMER | (86, 241, 108, 263) | (79, 238, 115, 267) | (36, 174, 255) | 440 | 141 |
| FLIPPER | (132, 241, 154, 263) | (125, 238, 161, 267) | (255, 141, 0) | 440 | 128 |
| SAMSUNG | (178, 241, 200, 263) | (171, 238, 207, 267) | (22, 75, 253) | 438 | 144 |

- Glowed: WIFI, DEAUTH, BLUETOOTH, HACK, FLOCK, DRONE, AIR TAG, KEYFOB, META, SKIMMER, FLIPPER, SAMSUNG (12 glossary chips).
- Interpretation (flag): these chips have a black icon screen, not a solid face. Face = the black icon screen; glow colour = the icon's brightest dominant colour; halo on the grey bezel inside the lime dash ring; lift on the icon pixels only. Alternative: glow inside the black screen around the icon.
- Not glowed: big MORPHEUS (flag: is it tappable?), the chip rain around him, lime dashes.
- Read: colourful and lively; AIR TAG (yellow) and HACK (lime) are the strongest, DEAUTH/KEYFOB/SAMSUNG blues are softer on grey. DRONE's icon is grey/white, so its glow is a neutral grey.
- Diff vs decoded locked frames (PNG, before encode): **0 frames / 0 px changed outside face+window** over 74 frames; max changed ring px per frame 47565, lift px 9259 (3x); max |delta| 195.
- Peak still: frame 29 (s = 1.000). Loop: 9.25 s clip, 1.4 s breath kept, so the breath jumps at the loop point (s = 0 at the start, s = 0.891 at the wrap).
- Encode check: `morpheus h264,Main,720,960,yuv420p,8/1,74 | moov at byte 36 | frames compared 74 | outside region: mean|d| 2.586, max 69, px>8 2716305 of 42277680 (6.4249%)`
- Files: `morpheus_glow_3x.mp4`, `morpheus_glow_peak_3x.png`, `morpheus_locked_vs_glow_sbs.png`, `morpheus_stats.json`.

## SCAN v2 (2026-11-03 ~6 PM ET: faces fixed, only a stronger halo breathes)

Mobandy, replying to the offer to keep the face colours fixed with only the halo breathing: "Yeah add a bit more". `glow_scan_current.py` v2:

- Base unchanged from faceonly_v1: the WIFI / BLE faces are tinted orange #ff7a00 and blue #2f6bff (same table below), frames grey, GIB as locked.
- **No face lift**: WIFI, BLE and GIB face pixels are identical to the base in all 168 frames (checked: 0 px). No brightening, no hue shift.
- Halo: AMAX **1.00** (was 0.80, +25%), RC **9 px** at 1x (was 7), same 1.4 s raised-cosine, alpha = AMAX*((RC+1-d)/RC)^2*s, a blend toward each
  face's own dominant colour (WIFI (255,122,0), BLE (47,107,255), GIB (247,242,230)), so never whiter than the face.
- Halo px per chip 657 at 1x (was 538): the C window (-7,-4,7,6) black/grey px as before, plus 87 px of grey bezel in a (-9,-9,9,9) box. Dark px outside
  the window are never touched, so the WIFI / BLE / GIB labels, pins and purple dash ring stay locked. Big SCANbot, tabs, FINDS, MORE not glowed.
- Diff (PNG, before encode): **face px changed 0**, **0 px changed outside face+window+halo**, base vs locked 0 px outside the two face masks. Peak frame 22 (s = 1.000, locked frame 24).
- Encode check: `scan h264,Main,720,960,yuv420p,30/1,168 | moov at byte 36 | frames compared 168 | outside region vs current base: mean|d| 2.670, max 121, px>8 6.4333%`
  (re-encode noise; `verify_scan_current.py` uses the C face+window region, so the 87 halo px outside the window count there too).
- Read (honest): at peak the black border round each face is fully the face colour (WIFI (0,0,0) -> (255,122,0), BLE -> (47,107,255), GIB -> cream), so the face
  edge merges into the halo, and a soft orange / blue / cream gradient fades over ~2-3 px of grey bezel (e.g. WIFI bezel below the face (105,118,133) -> (112,118,126)).
  Clearly visible, stronger than v1; labels, pins and dash ring unchanged. GIB's cream halo reads as a pale bloom on the grey.
- Files: `scan_glow_3x.mp4`, `scan_glow_peak_3x.png`, `scan_current_base_3x.png`, `scan_faceonly_v1_vs_v2_sbs.png` (b00670c peak vs v2 peak), `scan_stats.json`.
- b00670c (faceonly_v1: lift on, AMAX 0.80, RC 7) kept as `*_faceonly_v1.*`: `glow_scan_faceonly_v1.py`, `scan_glow_3x_faceonly_v1.mp4`, `scan_glow_peak_3x_faceonly_v1.png`,
  `scan_current_base_3x_faceonly_v1.png`, `scan_wholechip_vs_faceonly_sbs_faceonly_v1.png`, `scan_stats_faceonly_v1.json`.

### SCAN faceonly_v1 (b00670c, superseded)


Mobandy, 2026-11-03: "The WiFi and Bluetooth bot don't have grey faces that's old", then picked: "Tint only the WiFi and Bluetooth faces
orange and blue, keep their frames grey so the glow shows." The LOCKED SCANbot clip (2026-10-29) still has grey faces (105,119,131); the firmware
(`src/live/lv_core.h`, 156f359) tints the whole chip rect at run time (`LV_TM_COLOR` amt 5/8, muted). This mock does NOT follow that firmware tint:
it is a design mock of the face-only look. Firmware not changed.

Base (`glow_scan_current.py`) = decoded LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4 with only the WIFI / BLE face square (detect.py face mask,
inside the black window ring) tinted: `out = COL * L / 116`, L = (3R+6G+B)/10, 116 = L of the locked face mid grey, per channel, clipped.
Shading kept, eyes stay black. Bezel, label, pins, dash ring, everything else = locked pixels (**0 px changed outside the two face masks**, all 168 frames).

| chip | COL | face mid | highlight | shadow band | glow colour |
|---|---|---|---|---|---|
| WIFI | #ff7a00 (LV_WIFI_COL) | (255,122,0) | (255,174,0) | (112,53,0) | (255,122,0) |
| BLE | #2f6bff (LV_BLE_COL) | (47,107,255) | (67,152,255) | (21,47,112) | (47,107,255) |
| GIB | unchanged | | | | (247,242,230) |

Rects (1x): WIFI face (42,44,64,66) window (35,40,71,72); BLE face (42,107,64,129) window (35,103,71,135); GIB face (176,74,198,96) window (169,70,205,102); ring 538 px, lift 497/497/476 px.

- Same C glow (AMAX 0.80, RC 7, 1.4 s, gamma 0.15), masks from the untinted locked median. Big SCANbot, tabs, FINDS, MORE not glowed.
- 168 frames at 30 fps (5.6 s = 4 breaths, breath loops cleanly). Output frame n uses locked frame floor(n*(100/3)/30)+1 (frames 1..186 of 300);
  the locked animation jumps at the loop point.
- Diff vs base (PNG, before encode): **0 frames / 0 px changed outside face+window**; max ring px 14526, lift px 12912 (3x); max |delta| 204.
- Peak still: frame 22 (s = 1.000, locked frame 24).
- Read (honest): faces are clear orange and clear blue with the highlight strip, dark bottom band and black eyes; frames grey. At peak the black border
  round each face fills with the face colour (WIFI border (0,0,0) -> (150,72,0), BLE -> (28,63,150)) and a soft halo spills onto the grey bezel: clearly visible.
  The C lift (per-channel gamma) brightens the face a little at peak, WIFI (255,122,0) -> (255,134,0) (hue 28.7 -> 31.5 deg), BLE (47,107,255) -> (59,120,255);
  no whitening.
- Encode check (`verify_scan_current.py`): `scan h264,Main,720,960,yuv420p,30/1,168 | moov at byte 36 | frames compared 168 | outside region vs current base: mean|d| 2.661, max 121, px>8 6.3822%` (re-encode noise, as below).
- Files (now `*_faceonly_v1.*`): mp4, peak, base, wholechip-vs-faceonly sbs, stats.
- Previous whole-chip firmware-tint version kept as `*_wholechip_tint.*` (`glow_scan_wholechip_tint.py`, `scan_glow_3x_wholechip_tint.mp4`,
  `scan_glow_peak_3x_wholechip_tint.png`, `scan_locked_vs_glow_sbs_wholechip_tint.png`, `scan_current_base_3x_wholechip_tint.png`, `scan_stats_wholechip_tint.json`).
  Old grey-face versions kept as `scan_glow_3x_old_grey.mp4`, `scan_glow_peak_3x_old_grey.png`, `scan_locked_vs_glow_sbs_old_grey.png`, `scan_stats_old_grey.json`.

## Codec note (honest)

The 0-pixel diff is on the PNG frames. Any re-encode of the already-lossy locked MP4 adds yuv420 chroma noise everywhere. A control re-encode of
the *unmodified* locked TERMINAL frames gave the same numbers (mean |d| 2.69, 6.6% px > 8). So the MP4-level noise outside the glow comes from the re-encode, not the glow.

## Also flagged

- `LOCKED/ui_main_bot_rain` (gaps3 main-bot chips) exists only as 3-up chip clips and is not composed into these full screens; untouched here.
