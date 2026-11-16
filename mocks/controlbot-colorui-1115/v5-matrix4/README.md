# CONTROLbot v5 — 4 Matrix banners tinted to the selected theme (mock only)

Mobandy 2026-11-16 12:17 PM ET: "Just 4 Matrix-style banners that tint to the selected theme." This replaces the 11-bar cycle on our CYD CONTROLbot.
The theme list is the v4 24-theme list. No securitybot file and no prior folder was edited. SECURITYbot's own locks are untouched.

## Banners (cloned verbatim, from securitybot branch backup/securitybot/gibson-tower-home-mocks, read through a temporary worktree)
| BAR id | source | resolved still frame |
|---|---|---|
| CURTN | `render_topbar_v2b.py` CURTN, mocks/topbar-v2b-colors (locked BAR cycle 05d2a48) | 100 |
| TERM | `render_topbar_v2b.py` TERM, same lock | 96 |
| RAIN | `render_topbar_v1.py` M1 matrix rain, mocks/topbar-v1-matrix-security (a05184a) | 37 |
| DECOD | `render_topbar_v1.py` M2 matrix decode, same | 104 |

`sim/render_topbar_v1.py` is byte-identical to the a05184a version. Layouts, glyphs, fonts, animation and 30 fps / 105-frame timing all come from the cloned code.

## Tint (`sim/render_v5_matrix4.py`)
Black stays black. White highlights stay white (each pixel's share of white is kept), and brightness (max channel) is kept. All other ink takes the selected theme's ink at that pixel (`sim/bar_theme_ink.cpp`, same as v4).
There is no alert-red exception: 0 red-ink px were found in any of the 4 banners.

## Deliverables
- `00_matrix4_x_24themes_headers@3x.png`: 24 themes x 4 banners, smooth @3x header crops at each banner's resolved frame.
- `stills/`:
  - full-screen @3x stills: 01 CIRCUIT + CURTN, 06 IR RAINBOW HEAT + RAIN, 16 SONAR + TERM, 19 SYNTHWAVE + DECOD
  - untinted clone header crops
- `tap_cycle_24themes_matrix4@3x.mp4`: 720x960, 30 fps, 768 frames (25.6 s). It taps through all 24 themes, and the banner steps CURTN, TERM, RAIN, DECOD with each theme. Each theme plays the last 29 frames of its banner's animation up to the resolved still, so the bar is fully resolved when the face is tapped.
- `diff_proof.json`, `diff_proof_body.json`

## Proof
- Untinted clone check: 4/4 banners on the locked v9c base match the source @1x stills with 0 px different.
- Body: 0 px changed outside the coil/pin mask for all 24 themes. For the 10 SECURITYbot themes, the pipeline without coil/pin recoloring equals the locked v2 PNGs.
- With banners, across 96 grid stills and every mp4 frame (before the tap overlay): 0 px changed outside header rows y1..23, x1..238. For the SEC themes, 0 px differ from locked v2 outside the header and the coil/pin mask.

## BAR button label
The control box's BAR button (currently "HAZRD") is outside header rows y1..23, so this mock may not change it. It still reads HAZRD.
The intended labels would be CURTN / TERM / RAIN / DECOD (each 5 chars or fewer). This is open: allowing that one button's label region needs Mobandy's OK.

## Still off
- CURTN: the locked white-top / lime-bottom title gradient means the title is mostly white, so only its lower half and the rule follow the theme.
- RAIN: the side glyph columns are dim and thin, and on light themes they read greyish.
- On patterned themes the theme pattern cuts through the letters:
  - CIRCUIT: copper and green streaks inside the title.
  - POLICE SIREN: title split half red, half blue.
  - GLITCH: red/cyan bands through the text.
  - PRISM: rainbow stripes.
  This is honest to the tint rule, but busy.
- The mp4 shows only the tail of each animation (the CURTN wipe and TERM typing are mostly already done), because each banner gets only about 1 s per theme.
- The header text is still "SECURITYbot EX.DATA" (an open question).
- Re-running needs the securitybot worktree at /tmp/sbwt (removed after rendering): `git worktree add --detach /tmp/sbwt origin/backup/securitybot/gibson-tower-home-mocks`.
