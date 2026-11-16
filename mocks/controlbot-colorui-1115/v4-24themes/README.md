# CONTROLbot Color UI v4 — 24-theme list (mock only)

Mobandy 2026-11-16 11:26-11:27 AM ET: replace LIME, CYD BLUE and HOT PINK with new themes, and add the 4th new theme as an extra, for 24 in total.
Slot assignment is our pick and he may change it: CIRCUIT takes LIME's slot, HOLO FOIL takes HOT PINK's slot, SONAR takes CYD BLUE's slot, and DIGI CAMO is appended as #24.

**SECURITYbot's own lock is untouched.** SEC 01 LIME GREEN is a locked SECURITYbot Color UI v2 theme (2b79d83). Removing it applies only to our CYD CONTROLbot theme list. SECURITYbot's 11-theme lock, its files, and the CYD theme table (the copy of `lv_core.h` @049ff59) are unchanged. The v2, v3-newthemes and v3-alertbars folders are unchanged.

## Theme order
01 CIRCUIT (NEW, replaces SEC LIME) · 02 HI-VIS BLUE · 03 NV GREEN BLOOM · 04 NV WHITE PHOSPHOR · 05 IR IRONBOW · 06 IR RAINBOW HEAT · 07 IR WHITE-HOT · 08 IR RED-HOT · 09 ALARM STROBE · 10 POLICE SIREN · 11 PERIMETER GRID (02-11 SECURITYbot v2) ·
12 NEON GRAPE · 13 HOLO FOIL (NEW, replaces HOT PINK) · 14 TOXIC · 15 ICE · 16 SONAR (NEW, replaces CYD BLUE) · 17 BLAZE ORANGE · 18 GOLD RUSH · 19 SYNTHWAVE · 20 AURORA · 21 PRISM · 22 GLITCH · 23 HOT ROSE (CYD) · 24 DIGI CAMO (NEW extra)

## Pipeline
- **Body:** `sim/render_controlbot_colorui.py` + `controlbot_colorui_reink.cpp` + `new_themes_v3.h`, the same as v3-newthemes with only the theme loop reordered (ORDER). Every body frame is pixel-identical to its v2 or v3-newthemes render.
- **Alert bar:** the locked SECURITYbot v2b BAR cycle (05d2a48, verbatim renderer copies), tinted with `tint()` from `render_alertbars_v3.py`, which is unchanged. Alert red stays red; that question is still open. `bar_theme_ink.cpp` now also covers the 4 new themes.
- **Grid:** `00_24themes_grid@3x.png`. Every tile shows the CURTN bar at its locked still frame.
- **mp4:** `tap_cycle_24themes_alertbars@3x.mp4`, 720x960, 30 fps, 768 frames (25.6 s). It taps through all 24 themes, the BAR steps one bar per theme (all 11 shown), and each bar animates on its own locked clock, tinted live.

## Proof (`diff_proof.json`, `diff_proof_body.json`)
- Body: 0 px changed outside the 4,789 px coil/pin mask for all 24 themes. The grey board is identical across all 24.
- Grid stills and every mp4 frame (before the tap overlay): 0 px changed outside header rows y1..23, x1..238 against the body frame.
- Clone check for the 10 remaining SECURITYbot themes: the pipeline without coil/pin recoloring equals the locked v2 PNG for all 10. The final frames have 0 px different from locked v2 outside the header and the coil/pin mask.
- Bar clone: all 11 bars rendered untinted match the locked v2b stills with 0 px different.

## Still off
- The v3-alertbars meaning issues are unchanged: LITEB, ARM, BADGE, IRON and ZONE lose their red/blue or status color coding under the tint, and ALARM barely follows the theme.
- With LIME removed, no theme has the original lime-green identity. TOXIC is now the closest.
- CIRCUIT, SONAR and DIGI CAMO still have the muddy control box and dim coil/pins noted in v3-newthemes. CAMO's eyes are still hard to read.
- The BAR button in the control box still reads "HAZRD" while other bars show in the video.
- The tap glow colour for every theme is the theme's bar ink at mid-bar (not ACC), so it is a little paler than v2's tap glow on solid themes.
- The header text is still "SECURITYbot EX.DATA" (open question).
- Re-running needs the securitybot worktree at /tmp/sbwt (removed after rendering; recreate it with `git worktree add --detach /tmp/sbwt origin/backup/securitybot/gibson-tower-home-mocks`).
