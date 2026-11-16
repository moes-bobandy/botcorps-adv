# CONTROLbot Color UI — v3 NEW theme candidates (mock only)

Mobandy 2026-11-16 11:17 AM ET: "make 4 more ui themed color scheme to replace 4 colors i will choose once we decide".
Same pipeline as v2: cloned locked SECURITYbot Color UI v2 renderer, real C++ re-ink via zig, grey board, no rain, coil and pins carry the theme pattern through the same 4,789 px mask.
The new themes are appended in `sim/new_themes_v3.h` (indices 23..26). No existing theme, v2 output, or locked file was changed.

## The 4 new themes
| # | Name (5-char) | Palette / pattern |
|---|---|---|
| NEW 1 | CIRCUIT (CIRC) | Green solder mask (#0f7a45 to #0a4a2c) with a copper (#e0913c) trace lattice and gold (#ffd27a) vias at the joins |
| NEW 2 | SONAR (SONAR) | Deep-teal scope (#00a6a0 to #00343c) with cyan (#3dffcf) range rings and a pale (#9dffe8) sweep wedge |
| NEW 3 | HOLO FOIL (HOLO) | Iridescent pastel foil: cyan, lavender, pink, cream and mint blending on a wavy diagonal |
| NEW 4 | DIGI CAMO (CAMO) | Digital woodland camo: 10 px blocks of #2c3a26 / #55703a / #9aa36a / #cbbb8a, clumped |

Distinct from all 23 existing themes: no other theme uses green-and-copper, radial rings with a sweep, a pastel smooth sheen, or block camo.

## Current solid-face themes (candidates to replace) — see `00_solid_vs_new_comparison.png`
- SECURITYbot v2: 01 LIME GREEN, 02 HI-VIS BLUE
- CYD: 12 NEON GRAPE, 13 HOT PINK, 14 TOXIC, 15 ICE, 16 BLUE, 17 BLAZE ORANGE, 18 GOLD RUSH, 23 HOT ROSE

Since v2, the 8 CYD solids already have a shimmer on the coil and pins, but their faces are flat.
Near-duplicates you could drop first: 14 TOXIC is close to 01 LIME, 16 BLUE is close to 02 HI-VIS BLUE, and 23 HOT ROSE is close to 13 HOT PINK.

## Proof
`diff_mask_proof.json`: 0 px changed outside the coil and pin mask for all 4 new themes. The grey board outside the mask is identical to v2 LIME.

## Still off
- CIRCUIT, SONAR and CAMO: the control box (SCAN/VOL/BRIGHT and the buttons) re-inks to a dull grey-green or teal. These palettes are dark and low in saturation, so the box looks muddy next to the v2 themes.
- CIRCUIT: the coil and pins sample mostly copper, so they read orange rather than green PCB.
- CAMO: the dark blocks around the eyes make the eyes hard to pick out, and the coil and pins look dim olive.
- SONAR: the sweep wedge is a static still. In firmware it would want to rotate, which is not done here.
- HOLO: the pastels sit close to white, so the labels and box text have the lowest contrast of the four.
- The header still says "SECURITYbot EX.DATA" (an open question, unchanged).
