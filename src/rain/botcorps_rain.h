#pragma once

/*
 * botcorps_rain — external (or only-panel) bot rain.
 *
 * The step is 90 ms on a millis() accumulator that keeps the remainder.
 * Trails fade in the full-frame sprite. The sprite stores rgb565_2Byte
 * (byte-swapped). Fade and blend swap on the way in and out. If the
 * sprite cannot be kept, an 8×8 cell table paints through the library.
 * Neither path reads the panel, and neither draws the name bar.
 *
 * botcorps_rain_frame() returns true when a step was presented.
 */

#include <stdint.h>
#include <stdbool.h>

#include <lgfx/v1/LGFX_Sprite.hpp>

/* 0 = MULTI (the 14-colour palette). 1..6 = solid inks. */
#define BOTCORPS_RAIN_COLORS 7

bool botcorps_rain_frame(void);
void botcorps_rain_set_parked(bool parked);
bool botcorps_rain_sprite_held(void);
void botcorps_rain_log_heap(const char *tag);
uint16_t botcorps_rain_palette(int index);
void botcorps_rain_set_mode(int mode);
int botcorps_rain_mode(void);
uint16_t botcorps_rain_solid(void);
/* One full push of the current rain sprite. No step. False if parked. */
bool botcorps_rain_present(lgfx::LovyanGFX &dst);
/* Copy a rect of sprite memory into another rgb565_2Byte sprite.
 * Copies stored pixels; it does not write a native tint into the buffer. */
bool botcorps_rain_copy_rect(lgfx::LGFX_Sprite &dst, int x, int y);
