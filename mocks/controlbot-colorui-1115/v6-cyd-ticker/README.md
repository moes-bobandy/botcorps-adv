# CONTROLbot Color UI v6 — our CYD ticker banner (mock only)

Mobandy 2026-11-16 12:38 PM ET: use our ticker banner, with stripes and text following the selected theme and the alert flash staying red. Not SECURITYbot's banners.

Source is our firmware top border-alert banner: `lv_scene_ticker` in `cyd-portrait-wt/src/live/lv_core.h` at commit `049ff59` (lines 880–901). The C++ in `sim/lv_scene_ticker_049ff59.inc` is a byte-identical extract of those lines, compiled against the real `lv_draw.h` / `lv_font.h` with the same zig c++ path as earlier Color UI mocks. Hunt-bar stubs stay off so this mock stays on the scrolling ticker.

Ticker text is the firmware default `lv_tk_msg` at line 234: **`NO THREATS - TAP SCAN`**. No invented copy.

## Tint rule
- **Normal:** the two stripe phases take two contrasting colours from the selected theme's palette (accent / header, with a pull toward the dark accent when those two are too close), text = theme bright (HDR), background = a dark shade of ACCD (40% of ACCD). The 0x200000 drop shadow is kept.
- **Alert:** exactly the firmware red flash, untinted, on every theme (`bg` 0x6a0000 / 0x3a0000, stripes 0xff2020 / 0xe00000 and 0xffb000, white text, 350 ms period).

The banner sits in the CONTROLbot top bar (`LV_R_TICK` = header rows y1..23, x1..238) and replaces the old SECURITYbot EX.DATA bar. 0 px change outside the header (stills and pre-tap video frames). Body frames are the v4 24-theme set; coil/pin mask is untouched. Firmware, locked files, and earlier output folders were not edited.

## Deliverables
- Untinted clone check: verbatim `lv_scene_ticker` vs the tint path given the firmware palette, 5 states including alert on/off. 0 px different. Full-frame proof is composited on the ICE body (`00_clone_fw_untinted@1x.png` / `@3x.png`); `00_clone_diff@3x.png` is black.
- `00_24themes_headers@3x.png` — 6×4 grid of header crops at nearest-neighbor ×3, normal state, still at `now=2000`, `off=220`.
- Full-screen stills in `stills/` (all 24 @1x; CIRCUIT, HI-VIS BLUE, HOLO FOIL, SYNTHWAVE, DIGI CAMO also @3x).
- One alert-state still: `stills/alert_synthwave@1x.png` and `@3x.png` (flash on).
- `tap_cycle_24themes_ticker@3x.mp4` — 720×960, 30 fps, 768 frames (25.6 s), nearest-neighbor scale. Taps through all 24 themes. The ticker scrolls continuously (`lv_now += 30`, `lv_tk_off += 2` per frame, wrap at `(bar width)+12*strlen` like `lv_ticker_tick`). One alert flash on SYNTHWAVE (frames 8..28 of that slot): one 350 ms dark phase then one bright phase, firmware red, untinted.

## Proof (`diff_proof.json`)
- Inc file matches `git show 049ff59:src/live/lv_core.h` lines 880–901.
- Self-test: 256 (now, off, alert, message) frames, tint-path with firmware colours vs the verbatim function, fail=0.
- Clone-check cases all 0 px.
- Stills: 0 px changed outside the header vs the v4 body frames.
- mp4 pre-tap-overlay frames: 0 px changed outside the header.

## Still off
- The firmware default text is wider than the 238 px bar (21 glyphs × 12 = 252 px), so stills end at "TAP SC". That is how the firmware draws it.
- The alert flash uses that same default string (no invented find line). Semantically it is not an alert message.
- The kept 0x200000 shadow reads as a slight red fringe on cool themes (HI-VIS, ICE, TOXIC, and others).
- NEON GRAPE and SYNTHWAVE (and somewhat PERIMETER GRID) have two stripe phases that are close in hue, so the hazard rhythm is softer than the firmware red/amber pair.
- Multi-hue themes (AURORA, PRISM, GLITCH, POLICE SIREN) get a solid palette banner, not the face pattern, so SIREN's bar is red-pink with no police blue, PRISM/AURORA are green, and GLITCH is red with no cyan split.
- On red themes (IR RED-HOT, ALARM STROBE, POLICE SIREN, GLITCH, HOT ROSE) the firmware alert red is close to the theme.
- CIRCUIT, SONAR and DIGI CAMO still have the muddy control box and dim coil/pins noted in v3-newthemes / v4. CAMO's eyes are still hard to read.
- The BAR button still reads "HAZRD" (outside the area this mock is allowed to change).
- With LIME removed, TOXIC is still the closest thing to lime green.
