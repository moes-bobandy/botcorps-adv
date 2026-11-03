# home-tap-pulse — MOCK ONLY (no firmware change, no bin, no flash)
Request (Mobandy, 2026-11-02): make the bots around the rain chip pulse brighter or glow so it reads that they can be tapped.
Board: ESP32-2432S028R, 240x320 portrait ILI9341, shown at 3x nearest-neighbour (720x960), green theme.
Base: the locked main-bot rain home board (LOCKED/ui_main_bot_rain: g200 green home frame + GIBSONbot gaps3 rain, 140 ms/row),
built by importing gaps3.py unchanged. Rain chip on the home board: GIBSONbot (centre). SCANbot/TERMINALbot rain lives on their own
screens, so on HOME they are ordinary chips.
Chips that animate (none moved, nothing else changed): BOOTbot (top-left), CONTROLbot (top-right), SCANbot (bottom-left), TERMINALbot (bottom-right).

A PULSE: every pixel of each chip (bezel, waveform, body, face, label, pins) x k, k = 1.00 -> 1.40 -> 1.00, raised cosine,
  period 1.4 s, all four chips IN SYNC (one shared phase; reads as one "these are buttons" heartbeat). Clamped at 255; black stays black.
B GLOW: soft halo outside each chip edge in the chip's own colour (BOOT/CONTROL lime 147,254,0; SCAN gold 245,180,73; TERMINAL silver
  197,205,212), width 7 px (1x), alpha = 0.50 * (1-(d-1)/7)^2 * s, same 1.4 s in-sync breath; drawn only on dark PCB pixels, fades out
  3 px before the GIBSONbot chip box; no solid outline, no persistent border (alpha 0 at the trough).
Loop: 168 frames = 5.6 s = 4 breaths = 40 rain steps (rain loops every 10) -> seamless. Peak stills = frame 21 (s = 1).

Firmware notes (ESP32, rough):
- A: per chip keep the draw colours as a small RGB565 palette (or redraw via the existing drawBotChip with a brightness arg); scale with
  a 32/64-entry per-channel LUT per breath level. Quantise breath to ~16 levels; redraw a chip only when its level changes.
  Pixels pushed per update: BOOT/CONTROL 45x61 = 2.7k each, SCAN/TERM 74x99 = 7.3k each -> ~20k px = 40 KB over SPI @40 MHz ~ 8-10 ms.
  At ~16 levels per 1.4 s that is ~23 updates/s worst case; throttle to ~12 Hz (~10% of the SPI budget) and it still looks smooth.
- B: precompute one glow ring per chip at boot (pixel run list + 3-bit alpha level; ~945/875/2020/1810 px, ~5.7k total, ~12 KB as
  run-length strips). Per breath level, blend accent over the PCB background colour with an 8-entry LUT and push only ring pixels as
  short horizontal strips: ~5.7k px = 11 KB/update ~ 3 ms SPI. Chips themselves are never redrawn. Cheapest of the two.
- Both leave the GIBSONbot rain task (140 ms row step) untouched; the rects do not overlap the rain chip.

Files: tap_pulse.py (generator), home_tap_pulse_A_3x.mp4, home_tap_glow_B_3x.mp4, home_tap_sbs_A_B_3x.mp4 (A left, B right, 36 px caption
strip), home_tap_pulse_A_peak_3x.png, home_tap_glow_B_peak_3x.png. MP4s: H.264 Main, yuv420p, 30 fps, 168 frames, +faststart, crf 12.
Verified: GIBSONbot rain box (x76-164, y40-155) pixel-identical to locked gaps3 on all 168 frames for A and B; frame 0 3x crop ==
locked still PNG; nothing outside the four chip rects (A) / glow rings (B) changes. Note: the locked 3-up crop (x74-167) includes
BOOTbot's right pin column (x74-75) and CONTROLbot's left pin column (x165-167); those neighbour pixels do pulse/glow by design.

## Variant C — FACE GLOW (Mobandy, 2026-11-02: "Are you able to just have the bot face glow a bit stronger")
B's outer chip halo removed. Only the face square of BOOTbot, CONTROLbot, SCANbot, TERMINALbot glows; same 1.4 s in-sync breath.
- Face squares (1x, incl.): BOOT 40,65-66,91 (white-framed screen), CONTROL 178,65-196,83, SCAN 48,178-86,216, TERMINAL 154,178-192,216.
- Glow: radiates out from the face edge over the black face panel and grey bezel, inside the chip only, R = 7 px,
  alpha = AMAX * ((8-d)/7)^2 * s; AMAX 0.80 (TERMINAL 0.95, colour 226,236,250 from his face highlight so he reads as clearly
  as the others). Colours: BOOT/CONTROL lime 147,254,0; SCAN gold 245,180,73. Never on lime pin/decor dots, labels,
  header waveform or outline. Fades to 0 at the trough, so there is no persistent border.
- Face: mild midtone lift x -> 255*(x/255)^(1/(1+0.15 s)): black and white fixed, so SCAN keeps its bronze-to-pale gold ramp and
  TERMINAL keeps his shaded silver (no new near-white pixels: 84 -> 84 px >= 240 on all channels).
- Verified: 0/168 frames change any pixel outside the face-glow regions (7257 px total), so chip bodies, pins, labels, board
  and the GIBSONbot rain chip match the locked base exactly.
- Firmware: precompute per face a ring list (~570/461/1047/1047 px, 3-bit alpha) plus a 16-level gamma LUT for the face colours.
  Per update push face + ring as strips: ~7.3k px = 14.5 KB, ~3-4 ms SPI at 40 MHz; throttled to ~12 Hz that is ~4-5% of the
  bus budget. Cheaper than A, a bit more than B.
Files: tap_faceglow_C.py, home_tap_faceglow_C_3x.mp4, home_tap_faceglow_C_peak_3x.png (frame 21), home_tap_sbs_B_C_3x.mp4 (B left, C right).

## Variant E: locked MORPHEUSbot chip + inner face glow only (Mobandy, 2026-11-03)
Briefs: "Morpheus needs to be the new Morpheus ui we locked in" (D), then "The boarder around the chip is glowing. Make only the
bot color right beyond the glasses pulse" -> "Not pulse but glow" (E). D on its own was not rendered; E includes the D chip swap.
- Top-left chip (old BOOTbot look; MORPHEUSbot's slot, 1x x31..75 y51..111, same place and size) is redrawn in the locked
  MORPHEUSbot look (LOCKED/ui_glossary_morpheus_rain): black body with a 1px quiet grey outline (84,88,98), a grey ring round the
  face window, a black shaded face square, small round black shades with a bridge (morpheus_chip() art, LENS_SMALL), and toned-down
  firmware device rain (DEV_ART/DEV_NEON at half scale, 4x4 icons, lead 80 %, trails 65..45 %, 30 % behind the label,
  whole icons only, 133 ms/step, seamless over the 5.6 s loop). Pins, header wave and top/bottom lime dashes are kept; the
  side dashes inside the body are removed so the rain runs down both sides as in the locked chip.
- Label: the locked chip says "MORPHEUS". "MORPHEUSbot" in the home 3x5 silk font (TERMINALbot's) is 43 px wide; the body
  is 37 px, so it does not fit. "MORPHEUS" (31 px) is drawn in that font in the locked label ink (232,236,249) with a shadow.
- No outward glow anywhere. Only the face-fill pixels glow, steadily (E_steady) or breathing gently between 75 and 100 %
  (E_breathe, 1.4 s cycle): brighter toward the eyes, colour-true. CONTROL: screen blend with a lime light. SCAN: hue-locked
  gain, max channel <= 250. TERMINAL: hue-locked gain, max channel <= 232 (no new near-white). MORPHEUS: a faint slate light
  (96,100,110) on his black face round the shades, kept dark so it still reads as a black face. Eyes, glasses, glints and white highlights are unchanged.
- Verified: D base vs locked board differs only inside the top-left chip footprint (0/168 frames outside it); E vs D base
  differs only on the face-fill pixels (0/336 frames outside them).
Files: tap_faceglow_E.py, home_tap_faceglow_E_steady_3x.{mp4,png}, home_tap_faceglow_E_breathe_3x.{mp4,png} (PNG = frame 21),
home_tap_faceglow_E_faces_4x.png (top: no glow, bottom: E steady), home_tap_faceglow_E_morpheus_chip_4x.png,
home_tap_faceglow_E_morpheus_vs_locked.png.

## Variant F: GIBSONbot small pins + MORPHEUS face-only swap (Mobandy, 2026-11-03)
Briefs: "Make the pin ui for Gibson look like the rest on the page", then "I only wanted you to change the face ui not the board
to have the bot rain."
- GIBSONbot pins: the old pins were double size (sides 5 deep x 4 tall on a 6 px pitch, 16 per side; bottom 4 wide x 5 tall,
  pitch 6, 12 pins). They are erased to the sampled local background (dark strip / black) and redrawn in the SCANbot/TERMINALbot
  style, stamped from SCANbot's own pixels: sides 2 lime + 1 dark shade, 2 tall, pitch 5 (18 per side, 1x rows 55..141);
  bottom 2 wide x 3 tall, pitch 5, no shade (14 pins, x86..152). Lime ends 2 px short of the body outline, as on SCANbot.
  All four traces still land on pins (left/right at rows 95-96, bottom at x112-113 and x126-127). There were no outer top
  pins (the header wave sits there); the header wave and the small lime dashes inside the outline are kept.
- MORPHEUS chip: E's black rain body is removed. The base chip body is restored pixel-for-pixel (grey body, header wave,
  lime border and side pins, white bezel, label position). Only the face window changes: the locked black face
  square with diagonal shading and small round black shades (lit rim + glint), and the label reads MORPHEUS in the
  base label ink. E's face-fill glow stays on all four faces (face colour only, never borders).
- Verified: 0/168 frames differ from E outside the MORPHEUS chip box (1x x31..75 y51..111) and the GIBSON pin bands. Changed
  bbox outside the chip box is x76..163 y52..151, all in the pin bands. 0/168 frames differ from the locked base board
  outside chip + pin bands + face fills.
Files: tap_faceglow_F.py, tap_faceglow_F_zooms.py, home_tap_faceglow_F_steady_3x.{mp4,png} (PNG = frame 21),
home_tap_faceglow_F_gibson_pins_4x.png (E old vs F new vs SCANbot), home_tap_faceglow_F_morpheus_vs_control_4x.png.

## Variant G: MORPHEUS bezel dark grey (Mobandy, 2026-11-03)
Brief: "Make that white boarder around him dark grey."
- The white bezel ring around MORPHEUS's face window (1x x40..66 y65..91 minus the screen/chin x42..64 y66..88; 200 px) is
  recoloured to the board's existing dark grey SEP (84,88,98), the quiet grey already used for the MORPHEUS outline and lens
  drop shade (most common dark grey on the board). The face, glints, body, label, pins, glow and the rest of the board are unchanged.
- Verified against F: 0/168 frames differ outside those 200 bezel pixels, exactly 200 px change in every frame (bbox x40..66
  y65..91), and the white-bezel pixel set is identical in all 168 frames.
Files: tap_faceglow_G.py, home_tap_faceglow_G_steady_3x.{mp4,png} (PNG = frame 21),
home_tap_faceglow_G_morpheus_F_vs_G_vs_control_4x.png (F, G, CONTROLbot).

## Variant H: MORPHEUS chip in CONTROLbot's style (Mobandy, 2026-11-03)
Brief: "make the rest of Morpheus hold the CONTROLbot look for the board like you did for Gibson."
- The two chips are the same size, so CONTROL's own pixels (1x x165..208, y51..111) are copied onto MORPHEUS (x31..74,
  dx = -134): neutral grey body with its left highlight and lower-right diagonal, header wave, lime border and inner dashes,
  'dL'/'Ld' side pins on a 3 px pitch, and bottom pins. Column x75 is kept from G. The right trace (rows 95-96) meets a
  CONTROL-style pin and the bottom trace (x52-53) meets the bottom pins.
- Face: G's locked MORPHEUS screen (23x21 with its glow) is moved up 1 px into CONTROL's face-window rows (64..86), with a
  1 px dark grey (84,88,98) bezel ring at x41..65, y64..86 (his bezel instead of CONTROL's black window).
- Label: CONTROL's ink is erased to the body. MORPHEUS is drawn in TINY3 (CONTROL's font, 4 px pitch) in CONTROL's ink
  (23,23,25) on CONTROL's name row (y95..99), at x38..68 centred like CONTROL's label. It ends 2 px before the diagonal.
- Verified against G: 0/168 frames change any pixel outside the MORPHEUS chip box (x31..75, y51..111). In frame 21 all
  1734 body pixels outside the face window and label rows are identical to CONTROL's.
Files: tap_faceglow_H.py, tap_faceglow_H_zoom.py, home_tap_faceglow_H_steady_3x.{mp4,png} (PNG = frame 21),
home_tap_faceglow_H_morpheus_G_vs_H_vs_control_4x.png (G, H, CONTROLbot).
