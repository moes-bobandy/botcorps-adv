# CONTROLbot Color UI — v2 fixes (mock only)

Mobandy 2026-11-15 9:45 PM ET: "Fix the flaws you listed first, then show me." v1 is unchanged in the parent folder.
Rendered with the same cloned SECURITYbot Color UI v2 pipeline (commit 2b79d83), real C++ re-ink via zig. No firmware, no flash, no locked files edited.

## Fixes
1. **Solid CYD themes** (GRAPE, MAGENTA, TOXIC, ICE, BLUE, ORANGE, GOLD, ROSE): coil and pins carry a stepped shimmer made only from each theme's own real inks
   from `lv_core.h` (049ff59): darker ends (LV_ACCD mixed toward LV_ACC), LV_ACC, then an LV_HDR highlight in the middle of each strip. Only LIME and HI-VIS stay solid. Faces and control box are unchanged.
2. **Dim ends**: every coil/pin sample is lifted toward white until luminance is at least 0.20 before the original gold shading is applied.
3. **Flecks**: each pad (8-connected component) takes one sample at its centroid. The coil takes one sample per tooth (12 steps). Every pad reads as one clean step.
4. **Labels**: each pin label takes its own pad's colour, lifted to luminance at least 0.40 so it stays readable against the board.
5. **Bottom-left gold corner ring**: removed from the mask, so it stays exactly as v2.
6. **mp4** (`controlbot_colorui_tapcycle_v2.mp4`, 15 fps, 32.2 s, 720x960): for each of the 23 themes it holds on the theme, shows a touch ring growing from the face centre, a theme-accent glow around the face and a press flash on the face, then switches with a fading glow.

## Proof (`diff_mask_proof.json`)
For all 23 themes, 0 px changed outside the coil and pin mask. On the 11 SEC themes, the no-coil pipeline matches the locked v2 PNGs pixel for pixel. The board outside the mask is identical across all themes.

## Still off
- The tap flash tints the face's black eyes too, so they turn brownish for 3 frames. The glow is a mock overlay on the frames, not firmware touch code.
- Per-pad steps for vertical-scanline themes (NVG) mostly read as a dark/light alternation, not scanlines. That is expected after this fix.
- The NVG and ROSE labels are readable but still the softest in the set.
- Header still says "SECURITYbot EX.DATA". This is an open question.
- Applying CYD themes to this layout is my mapping, not a locked design.
