# Firmware features 2026-10-31: CMDS cheat sheet, hunt bar, background engine

Sim evidence, rendered with tools/livescan/sim_live.cpp from the committed source (3x nearest-neighbour). Every image here was looked at.
Nothing was flashed or merged. The CTL chip redesign is NOT in firmware, and no shape button was removed.

## 1. TTY CMDS cheat sheet (as mocked)
- cmds_cheatsheet_3x.png: closed, then p1, NEXT p2, NEXT p3, tap hunt -> "hunt ", type 3, CMDS again, BACK (no insert).
- cs_deadtaps.txt: sheet bbox (0,176)-(239,319), 2160 points sampled every 4 px, 0 dead taps. The firmware renders are byte-identical to the mock renders.

## 2. Hunt bar in the GIBSON radar top banner (as mocked)
- hunt_bar_3x.png: before, 1ST READ, STEADY, STEADY, WARMER +19DB, COLDER -12DB, NO SIG, ended.
- hunt_pixdiff.txt: locked radar body/rim y24-289 has **0 changed px in every frame**. Only the banner and the mini-terminal lines change.

## 3. Background engine (BG STYLE x BG COLOR, saved in NVS)
- Styles: RAIN, BOTS, GIBSON, FLYBY, BOUNCE, OFF. Colours: GREEN, CYAN, THEME, MULTI, PURPLE, AMBER. THEME is kept because it already existed.
- NVS namespace botcorps: lkbgs (0-4 = style, 5 = OFF) and lkbgc (0-5). With no keys stored, the default is RAIN + GREEN, which is pixel-identical to the previous firmware.
- Selection uses the existing CTL chip taps, and the chip look is unchanged:
  - The RAIN chip steps the style.
  - The RAIN COL chip steps the colour.
  - Only the badge value text changes, and the default badge still reads "ON".
- bg_styles_3x.png, bg_colors_3x.png, bg_engine_radar.mp4 (h264 yuv420p faststart): the radar field in each style and colour.
- ctl_badges_3x.png, ctl_badges_closeup_5x.png: the CTL badge text for each state. All text fits.
- ctl_pixdiff.txt: default CTL is 0 px changed. After one style tap, only the RAIN badge bbox (166,125,204,138) and the ticker change.
- bg_nvs_log.txt: every tap writes NVS, and a sim reboot (bgboot) reloads it.

### What visibly changes, and what does not
- CHANGES: the GIBSON radar screen's procedural background field, which is the only free background area.
- DOES NOT CHANGE: HOME, TERM, SCAN, CTL (except the badge text) and TTY. These are locked full-frame clips or functional screens with no background area. Proof:
  - bg_isolation.txt: 0 px changed on those screens in every BG state tested.
  - screens_pixdiff.txt: 0 px changed versus fa33cf7.
