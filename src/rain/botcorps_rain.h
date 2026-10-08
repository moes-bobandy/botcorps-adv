#pragma once

/*
 * botcorps_rain — boot surface for Slice 1R.
 *
 * The internal panel is a plain navigation bar with the product name.
 * The content panel is bot rain, drawn first, then an opaque name bar
 * so the rain never crosses the glyphs. loop() calls botcorps_rain_frame()
 * on every pass.
 *
 * Does not set rotation, color depth, or the panel pins.
 */

/* Internal name chrome. On a dual build this is the ST7789 only.
 * On a single panel the rain frame owns the name bar. */
void botcorps_nav_draw(void);

/* One rain frame on the content surface, then the name bar. */
void botcorps_rain_frame(void);
