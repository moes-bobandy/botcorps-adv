# CONTROLbot v3 — alert bars that follow the Color UI theme (mock only)

Mobandy 2026-11-16 11:18-11:20 AM ET: the CYD CONTROLbot top bar becomes the alert border, and the Color UI theme selection changes it. "Clone all 11 bars but tint them to follow the selected Color UI theme".

## Source (read only)
- The SECURITYbot LOCKED BAR cycle on `moes-bobandy/securitybot`, branch `backup/securitybot/gibson-tower-home-mocks`: `mocks/topbar-v2b-colors/` (lock 05d2a48, BAR_CYCLE_LOCK.md). Read through a detached git worktree at /tmp/sbwt; no securitybot file was edited.
- `sim/render_topbar_v1.py`, `render_topbar_v2.py`, `render_topbar_v2b.py` and `verify_topbar_v2b.py` are VERBATIM copies. Every bar's layout, glyphs, fonts, animation and 105-frame / 30 fps timing come from that code unchanged.
- Theme inks come from `sim/bar_theme_ink.cpp`, built with zig from the same theme sources as v2: `themes_spec_v2.h` @2b79d83 and the copy of the CYD theme list in `lv_core.h` @049ff59. The face pattern is squeezed into the 23 header rows, with the same minimum-brightness lift as the coil and pins.

## Tint rule (`sim/render_alertbars_v3.py`)
For each header pixel:
- Black stays black.
- White highlights stay white: each pixel's share of white is kept.
- Brightness (max channel) is kept.
- Every other ink takes the selected theme's ink at that pixel.
- **Alert red stays red** (R high, G and B low). My decision: red means "alert" on a security header, so it should not turn into a theme colour. This affects ZONE Z3, the RADAR blip and bearing, ARM's AWAY chip and the red half of its shield, LITEB's red pod, BADGE's red pinstripe and LEDs, all of ALARM, and IRON's hot end. It is open for Mobandy to overrule.

## Deliverables
- `00_clone_untinted_11bars@3x.png`: the untinted clone of all 11 bars, original colours.
- `00_alertbars_11x4_grid@3x.png`: 11 bars x 4 sample themes (SEC HI-VIS BLUE, SEC IR IRONBOW, CYD NEON GRAPE, CYD SYNTHWAVE), as smooth 3x header crops like the locked `_header@3x_smooth` files.
- `stills/`: the 44 full @1x frames behind the grid.
- `alertbars_theme_tap_cycle@3x.mp4`: 720x960, 30 fps, 24.5 s, 736 frames. It cycles all 23 themes with a tap on the face (press glow, then switch), the BAR steps through all 11 bars, and each bar animates on its own locked clock, tinted live.
- `diff_proof.json`

## Proof (`diff_proof.json`)
1. Clone check: all 11 bars, rendered untinted on the locked v9c base, match the locked v2b @1x stills with 0 px different, both in the header crop and in the full frame.
2. Across all 44 tinted stills, 0 px changed outside header rows y1..23, x1..238 compared with our v2 CONTROLbot frame for the same theme. For the SEC themes, 0 px changed against the locked SECURITYbot v2 outside the header and the coil/pin mask.

## Still off
- **Meaning breaks under tint:**
  - LITEB (police red/blue): the blue pod takes the theme ink. It turns amber or gold on IRONBOW and SYNTH, and green or yellow on RAINBOW HEAT, so it no longer reads as police.
  - ARM shield: the half-red / half-blue split becomes red plus theme colour.
  - BADGE: the cruiser blue becomes theme colour.
  - IRON: the heat scale and blob become the theme colour, so it is no longer a heat scale outside IR themes. Only the red hot end survives.
  - ZONE: Z1 OK, Z2 WARN and Z4 INFO all collapse to the same theme colour. Only Z3 ALERT (red) stays distinct, so the status coding is lost.
  - ALARM: almost entirely red/white, so it barely changes with the theme. That is by design under the "red = alert" rule.
- On red themes (IR RED-HOT, ALARM, SIREN red half, GLITCH, HOT ROSE), "alert red" and the theme ink look the same, so alerts lose contrast.
- In the mp4, the control box BAR button still reads "HAZRD" while a different bar shows. The v2 frame's BAR value was not changed, because that area is outside the allowed header region.
- The tap glow and flash are drawn onto the video frames (not firmware). The face's black eyes are now excluded from the flash.
- The header text is still "SECURITYbot EX.DATA" (an open question).
