# LOCKED: CYD CONTROLbot Color UI v6 (Mobandy 2026-11-16 12:59 PM ET, "Lock it in")

Locked as-is from commit 585810cebd297a41b16a2373b636468fe0d5d824 on this branch.

- 24 themes: the v4 list (CIRCUIT, HOLO FOIL and SONAR take the LIME, HOT PINK and CYD BLUE slots; DIGI CAMO is #24). The grey board stays, and the coil and pins carry each theme's pattern (4,789 px mask).
- Top banner: OUR firmware ticker, lv_scene_ticker from src/live/lv_core.h @ 049ff59 lines 880-901, copied verbatim. Stripes, text and background follow the selected theme. The alert flash stays the firmware red/white on every theme.
- The theme picked on CONTROLbot carries through every CYD screen (Mobandy 2026-11-15 9:46 PM ET).
- Reference renders: 00_24themes_headers@3x.png (5ab4800c), tap_cycle_24themes_ticker@3x.mp4 (5a17b62c), stills/. Full hashes are in SHA256SUMS.
- Known items carried into the lock: the BAR button still reads HAZRD, the alert still reuses the default ticker text, and the shadow shows a slight red fringe on cool themes.
- Mock lock only. No firmware has been changed, built, or flashed. A firmware port needs Mobandy's go.
- Never change this lock; only add.
