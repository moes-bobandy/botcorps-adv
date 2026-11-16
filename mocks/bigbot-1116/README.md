# Big-bot Color UI v6 mocks — each home-spot bot, really big (MOCK ONLY)

Mobandy 2026-11-16 4:48–4:50 PM ET: a mock screen of each spot on the locked CYD home portrait board as its own version of the locked CONTROLbot Color UI v6 screen, with that bot really big in the center. Swipe left or right on the top ticker to flip to the next bot's big screen. Tapping the center of each big bot opens that bot's own screen.

**Mock only. No firmware commits to the build branch, nothing flashed, nothing copied to an SD card.**

## Sources (verified)

- Firmware home spots: `src/live/lv_core.h` on `moes-bobandy/botcorps-adv`, worktree `/workspace/cyd-fw-3locks-wt` (`backup/cyd-fw-3locks-2026-11-16`).
  - Base: `049ff59`. Build the user has: `c1b5659`. v1.1 rewires (not in c1b5659): `ada2253`.
- Locked v6: `LOCK.md` at `a87fe22` on `backup/controlbot-colorui-2026-11-15`, `mocks/controlbot-colorui-1115/v6-cyd-ticker/`.
- Default theme: firmware `lv_theme = 0` → NEON GRAPE (`LV_CB_FW[11] = 0`, still `12_neon_grape@1x.png`).
  sha256 of the locked still (matches `git show a87fe22:.../12_neon_grape@1x.png`):
  `48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779`

## Home spots, from the real home code

`LV_HOME_CHIP[5]` at `c1b5659` line 1729 (rects inclusive):

| i | rect | Name on this mock | Name on c1b5659 home clip | Home tap at c1b5659 | Home tap at ada2253 (v1.1) |
|---|------|-------------------|---------------------------|---------------------|----------------------------|
| 0 | {31, 52, 74, 112} | BOTpheus | MORPHEUS / bot (shades face) | LV_SC_GLOSS (`morpheus-glossary`) | LV_SC_MO BOTpheus EB3 (`botpheus-eb3`) |
| 1 | {78, 30, 160, 150} | GIBSONbot | GIBSONbot (cream face, shades) | LV_SC_RADAR (`gibson-jackin-radar`) | unchanged |
| 2 | {165, 51, 208, 110} | CONTROLbot | CONTROL / bot (theme face) | LV_SC_CONTROLBOT (`ctl`, v6) | unchanged |
| 3 | {33, 156, 101, 255} | HACKbot | SCANbot (metallic gold face) | LV_SC_SCAN (`scan`) | LV_SC_DL HACKbot game (`hackbot-game`); name pixels SCANbot→HACKbot |
| 4 | {139, 156, 207, 255} | TERMINALbot | TERMINALbot (clean silver) | LV_SC_TERM (`term`) | unchanged |

Those five are every home chip. There is no sixth home spot in `LV_HOME_CHIP`.

Swipe order used in the concept overlay and the mp4 is this same array order: BOTpheus → GIBSONbot → CONTROLbot → HACKbot → TERMINALbot (and wrap). Swipe left on the ticker goes to the next bot, swipe right to the previous.

## What was cloned, and what was swapped

Each still starts from the locked v6 NEON GRAPE frame. The grey board, coil, pins, themed ticker (`lv_scene_ticker`), and the CONTROLbot control rows (SCAN / VOL / BRIGHT / CHIME / TIME / BAR / TONE, including the locked "HAZRD" label) are copied byte-for-byte.

Only these regions change:

- Center face: THEME_FACE inclusive (65, 88)–(174, 197), 110×110 — the same slot the locked v6 CONTROLbot face uses.
- Name: LV_GLCD 5×7, ink (231, 239, 247), centred in the black frame pad under the face (y = 200), still inside `LV_CB_BOT` (54, 79)–(185, 210).

The locked v6 still has **no** "CONTROLbot" name plate, so the CONTROLbot mock is pixel-identical to the lock (0 px changed). The other four bots get a name in that pad.

### Faces

- **CONTROLbot:** the locked v6 theme face, untouched.
- **HACKbot:** firmware `e_gold_face` + eyes from `src/ui/botcorps_cyd_edge.inc`, compiled with zig c++ and run at `s = 110`. Metallic gold with the diagonal sheen, never flat yellow. Blink held off.
- **TERMINALbot:** firmware `e_silver_face` + `e_term_marks` (two eyes and `>_`) at `s = 110`, blink off. Clean silver, no extra black face-border. The cloned v6 chip still has its black frame around the 110×110 slot (layout not reinvented). At `s = 110` the prompt uses `u = s/12 = 9`, so the `>_` is very blocky — that is the firmware formula, not a new drawing.
- **BOTpheus:** locked MORPHEUSbot / BOTpheus shades face from `variant_big_morpheus.morpheus_chip` at the designed `glossary_big` size, then nearest-neighbour fitted into the 110×110 slot. Same dark shaded face, round shades and bridge.
- **GIBSONbot:** cream monitor face + shades, cropped from the official fw-3locks-1116 home render `docs/builds/fw-3locks-1116/renders/th0_home_3x.png` (exact 3× nearest-neighbour of 240×320), crop (96, 56)–(144, 104) inclusive, then NN-fitted into 110×110. Side-pin purple and the outer chassis were left out so they would not leak into the face.

Renderer: `sim/bigbot_compose.cpp` (zig c++, `-target x86_64-linux-gnu.2.36`, real `lv_draw.h` / `lv_font.h` from `cyd-portrait-wt` @ `049ff59`). Unchanged pixels are never pushed through RGB565.

## Pixel diff vs locked v6 NEON GRAPE

Every still was diffed against `12_neon_grape@1x.png`. Changed pixels sit only inside `LV_CB_BOT`. 0 px outside that rect. Inclusive bboxes:

| bot | changed px | diff bbox (x0,y0,x1,y1) |
|-----|------------|-------------------------|
| BOTpheus | 12216 | (65, 88, 174, 207) |
| GIBSONbot | 11492 | (65, 88, 174, 206) |
| CONTROLbot | 0 | none (identical) |
| HACKbot | 11440 | (65, 88, 174, 206) |
| TERMINALbot | 11503 | (65, 88, 174, 206) |

`@3x` (720×960) is a pixel-exact 3× nearest-neighbour of `@1x` for every still.

## Theme carry

HACKbot was also composited onto three other locked v6 stills (CIRCUIT, ICE, GOLD RUSH). The gold face stays HACKbot's locked metal; the board, coil, pins, ticker and control rows follow the theme. Diff vs each theme's locked body is the same face+name bbox, 0 px outside `LV_CB_BOT`.

## Per-bot center-tap table (concept)

Center target is the same as CONTROLbot's face tap: `LV_CB_FACE` inclusive (53, 78)–(187, 212), 135×135, already a finger-sized target. This is a mock; nothing is wired in firmware.

| Big-bot screen | Center tap opens | Why this is listed this way |
|----------------|------------------|-----------------------------|
| BOTpheus | Morpheus Dojo / BOTpheus EB3 (`LV_SC_MO`) | Mobandy 4:50 PM ET: "BOTpheus opens the Morpheus Dojo (BOTpheus EB3)" |
| HACKbot | HACKbot game (`LV_SC_DL`) | Mobandy 4:50 PM ET: "HACKbot opens the HACKbot game" |
| GIBSONbot | ask Mobandy | Mobandy said "the scan bot opens the Gibson scan tower." On the locked home board the chip that already opens the Gibson scan tower is GIBSONbot (`LV_HOME_CHIP[1]` → `LV_SC_RADAR`, `gibson-jackin-radar`) at both c1b5659 and ada2253. SCANbot is no longer a home spot (it became HACKbot). The wording was "the scan bot," not "GIBSONbot," so this is not assigned here. |
| CONTROLbot | ask Mobandy | This mock already *is* CONTROLbot's own locked v6 screen. In the firmware a face tap currently cycles the theme (`lv_controlbot_screen.h`). No other destination was stated. |
| TERMINALbot | ask Mobandy | Not mentioned. The home chip currently opens `LV_SC_TERM`, but that was not given as the center-tap rule for this mock. |

## Concept overlays and swipe (not a lock change)

These are labeled concept sketches. They are **not** part of the locked look and were not used in the pixel-diff gate.

- `overlay/nav_strip_overlay_control@1x.png` / `@3x.png` — CONTROLbot still with a small order strip under the ticker, current bot highlighted, swipe arrows on the banner, caption "CONCEPT ONLY".
- `overlay/nav_strip_legend@1x.png` / `@3x.png` — the same strip on its own card.
- `swipe_botpheus_to_gibson@3x.mp4` — 720×960, h264 yuv420p, 30 fps, 25 frames (0.833 s), nearest-neighbour scale. Swipe-left transition at the banner: BOTpheus slides out left, GIBSONbot slides in from the right. Also `swipe_botpheus_to_gibson@3x.gif`.

## Deliverables

- `stills/<bot>@1x.png` and `@3x.png` for botpheus, gibson, control, hackbot, terminal (default theme NEON GRAPE).
- `contact_sheet_5bots@3x.png` — the five stills side by side.
- `themes/hackbot_{01_circuit,15_ice,18_gold_rush}@1x.png` and `@3x.png`, plus `hackbot_3themes@3x.png`.
- Overlay and swipe files above.
- `sim/bigbot_compose.cpp`, `sim/render_bigbot.py`.
- `diff_proof.json`, `SHA256SUMS`.

## What this mock does not do

- No firmware commit to `backup/cyd-fw-3locks-2026-11-16` or any other build branch.
- Nothing flashed, nothing written to an SD card.
- Control rows stay the CONTROLbot set (SCAN/VOL/BRIGHT/…), including "HAZRD", on every bot's screen. They are CONTROLbot-specific; the shapes were kept as cloned.
- CONTROLbot's locked still has no name label; none was invented on that still.
- Theme does not recolor HACKbot gold or TERMINALbot silver (those faces are identity, not the theme preview). CONTROLbot's face remains the theme preview.
