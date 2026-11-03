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

## SCAN

Locked source: LOCKED/ui_scanbot/scanbot_bg_themes_v2@3x.mp4 (sha256 a8d2f40c…, 720x960, 100/3 fps, 300 frames; locked 2026-10-29 3:22 PM ET)

| chip | face (1x x0,y0,x1,y1) | window | glow colour | ring px 1x | lift px 1x |
|---|---|---|---|---|---|
| WIFI | (42, 44, 64, 66) | (35, 40, 71, 72) | (105, 119, 131) | 538 | 497 |
| BLE | (42, 107, 64, 129) | (35, 103, 71, 135) | (105, 119, 131) | 538 | 497 |
| GIB | (176, 74, 198, 96) | (169, 70, 205, 102) | (247, 242, 230) | 538 | 476 |

- Glowed: WIFI and BLE scan chips (face colour grey 105,119,131) and GIB (cream 247,242,230).
- Not glowed (flag, ask Mobandy): ALL / WIFI / BT / DEAUTH tab buttons, the FINDS tiles and MORE. They have no bot face, so the face glow has nowhere to start.
- Not glowed: big SCANbot (flag: is it tappable?).
- Read: GIB glows clearly. The WIFI and BLE faces are grey on a grey bezel, so in their own colour there is no visible coloured halo. At peak the grey fills the 2 px black window, so the face's black frame fades and the face blends into the bezel (it looks washed out, not glowing). If Mobandy wants a visible glow here, it needs a different colour (e.g. the theme accent), which breaks the 'own face colour' rule. Ask him.
- Diff vs decoded locked frames (PNG, before encode): **0 frames / 0 px changed outside face+window** over 300 frames; max changed ring px per frame 12260, lift px 12937 (3x); max |delta| 198.
- Peak still: frame 71 (s = 1.000). Loop: 9.00 s clip, 1.4 s breath kept, so the breath jumps at the loop point (s = 0 at the start, s = 0.950 at the wrap).
- Encode check: `scan h264,Main,720,960,yuv420p,100/3,300 | moov at byte 36 | frames compared 300 | outside region: mean|d| 2.625, max 124, px>8 12510596 of 197469900 (6.3354%)`
- Files: `scan_glow_3x.mp4`, `scan_glow_peak_3x.png`, `scan_locked_vs_glow_sbs.png`, `scan_stats.json`.

## Codec note (honest)

The 0-pixel diff is on the PNG frames. Any re-encode of the already-lossy locked MP4 adds yuv420 chroma noise everywhere. A control re-encode of
the *unmodified* locked TERMINAL frames gave the same numbers (mean |d| 2.69, 6.6% px > 8). So the MP4-level noise outside the glow comes from the re-encode, not the glow.

## Also flagged

- `LOCKED/ui_main_bot_rain` (gaps3 main-bot chips) exists only as 3-up chip clips and is not composed into these full screens; untouched here.
