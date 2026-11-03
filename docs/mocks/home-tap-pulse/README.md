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
