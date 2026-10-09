#pragma once

/*
 * botcorps_rain — Slice 1R2 boot surface.
 *
 * INT: charcoal field, 22px slate bar, white "BOTcorps", grey rule.
 * EXT: bot rain only. No name bar on the rain.
 *
 * The step is 90 ms on a millis() accumulator that keeps the remainder.
 * Trails are a translucent fade of the previous frame, so they need the
 * full-frame sprite. If that alloc fails, a per-cell table supplies
 * computed trail colors. Neither path reads the external panel.
 */

void botcorps_nav_draw(void);
void botcorps_rain_frame(void);
