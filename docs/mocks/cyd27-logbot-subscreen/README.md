# MOCK 2026-11-28 (CYD-27): LOGbot subscreen, live log + device health strip

**Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no SD upload, no flash, no merge.**
Branch `mock/cyd27-logbot-subscreen-2026-11-28` on `fw/cyd-v1.2.25-2026-11-27` (5c88bba). Flag `LV_CYD27_LOGSUB` (default 0).
With the flag 0 every render is byte-identical to the v1.2.25 base (gate below).

Renderer: the real `src/live/lv_core.h` engine through `tools/livescan/sim_live.cpp` (host sim), flags LV_CYD17_PANEL_TERM=1 LV_CYD19_LOGBOT=1 LV_TPS_G2_ONLY=1 LV_CYD22_G2=1 LV_C22_VAR=6.

What is on the screen: the LOGbot chip face in its locked board with the standard amber hazard banner and Stripe-Slot back, a four-cell health strip
(SD OK + free, TOUCH OK + last tap x/y, HEAP free KB, UPTIME h:m:s) where the coil band was, and a SCAN/SYS log plate whose rows now scroll (new row every 1.8 s, 270 ms slide).

Reused verbatim: board still, LOGbot face + rain (oil-slick finish), log plate frame + SCAN/SYS tabs + EX.DATA tag, log row generator, F2 theme-ink roles, banner back.
Added only: scrolling rows (clipped to the plate), health strip, subscreen mode (tap LOGbot face to open, banner back to return, tap plate flips SCAN/SYS).
Off on the subscreen (not on any sub-screen): pager arrow chips, pips, coil swipe.
Log and strip ink = live theme per pixel (lv_theme_at); no hardcoded colour, no bars, no GPIO/NVS text.

Caveats: SD free (14.2G), heap and the uptime base are SAMPLE values (plate says EX.DATA); TOUCH shows the real last tap the engine received.
Mock RAM note: reuses the 21 KB CYD-17 panel buffer; a firmware port would draw row-wise. The coil band is replaced on this subscreen only.
The v1.2.26 home order (CONTROLbot center) is NOT part of this mock; entry here is from the existing LOGbot pager page.

Gate (gate_script.txt, 9 screens: pagers 0-5 + TERM + SCAN sub-screens):
- OFF build vs v1.2.25 base sim: 0 px diff on all 9 shots.
- ON build vs base: 0 px diff on all except the LOGbot subscreen shot (the intended change).
- mp4: 418 frames, 720x960 h264 yuv420p, 100/3 fps; @3x PNG is an exact 3x nearest-neighbour expansion of the @1x.
