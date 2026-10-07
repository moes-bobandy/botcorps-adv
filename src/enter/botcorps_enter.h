#pragma once

/*
 * botcorps_enter — army-green millscale hatch on the menu surface.
 *
 * Draws only through botcorps_menu() (internal ST7789 240×135, or the
 * one panel on a single-screen build). Text is printed forward,
 * left to right. ENTER leaves the hatch; the caller then runs the
 * Slice 0 menu path.
 *
 * Does not address the external ILI9341, change its rotation, or
 * touch the SD bus. No rotate / corner-flip control.
 */

/* Paint the hatch once. Menu surface only. */
void botcorps_enter_draw(void);

/* Block until Enter is pressed after the hatch is up.
 * A key already held at entry must be released and pressed again. */
void botcorps_enter_run(void);
