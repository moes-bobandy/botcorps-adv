# MOCK dojo-white-bg (2026-11-21) - HOST SIM ONLY, sim-only, not yet tested on your CYD. No bin.

Compile flag LV_MO_BG (src/live/lv_core.h, default 0 = the locked Dojo, byte-identical):
- 0: current tatami floor + shoji wall
- 1 (A): solid white floor + wall (ceiling included), grid in the existing Dojo brown MO11_SEAM 0x5c4a32
- 2 (B): solid white floor + wall, black grid

Only the floor and wall change. Grid = the same Gibson floor rows + lanes (same perspective and depth fade, on white), horizon rail,
and the wall framing as 1 px lines at the existing places: shoji panel frames (11 px, camera parallax), wall top, head beam, sill,
ceiling rafters and cross beams. Pillars, emblems, panels, text, rain unchanged in code; translucent pillar edges show the new floor.

Gate (gate_px.txt): every non-Dojo clip 0 px vs dc98ba3 for flags 0, 1, 2 at band rows 24 and 12 (2959 frames); flag 0 = 0 px total.
Timing (dev_time_3runs.txt): devrec device timeline, 3 runs each.
