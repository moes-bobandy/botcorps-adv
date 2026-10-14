#pragma once

#include "ui/botcorps_band.h"

/* Full ibId card. Call when the screen or the selected bot changes. */
void botcorps_id_paint(Gfx g, int width, int bot, bool gib, uint8_t col,
                       int shape, const char *name, const char *target);
/* Wiring pulses and the LED bars only. Balanced startWrite/endWrite pairs. */
void botcorps_id_tick(lgfx::LovyanGFX &menu, int width, int bot, bool gib, uint8_t col);
