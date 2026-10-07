#pragma once

/*
 * botcorps_bot_window — seat window on the content surface.
 *
 * Fills the content panel (external 320×240 when that panel is up).
 * Millscale drift runs behind an opaque header and footer. Text is
 * drawn only on those bars, after the bars are filled, so drift never
 * crosses a glyph.
 *
 * The panel is not fillScreen'd every frame. A sprite (two, when the
 * largest free block allows) is drawn offscreen and pushed, or flakes
 * are patched in place. Entering or leaving may paint chrome once.
 *
 * Idle on the content surface runs the screensaver there. Backtick
 * leaves the window and the caller redraws the menu.
 */

#include <stdint.h>

enum { BOTCORPS_CONTENT_IDLE_MS = 20000 };

/* Blocks until backtick. Restores the active surface to MENU. */
void botcorps_bot_window_run(int seat);

/* Drift across the content panel until any key. MENU is not touched
 * when the external panel is up. Restores the active surface to MENU. */
void botcorps_content_screensaver(int seat);
