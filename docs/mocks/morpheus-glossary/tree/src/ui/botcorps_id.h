#pragma once

#include "ui/botcorps_band.h"

/* Full ibId card. Call when the screen changes or at boot. */
void botcorps_id_paint(Gfx g, int width, int bot, bool gib, uint8_t col,
                       int shape, const char *name, const char *target);
/* 236x16 RGB565 strip. Once, for the whole run. Null if the alloc failed. */
void botcorps_id_strip_alloc(void);
/* Bot change. Strips (2,26)-(238,133). No whole-card fill of the glass. */
void botcorps_id_recompose(lgfx::LovyanGFX &menu, int width, int bot, bool gib,
                           uint8_t col, int shape, const char *name, const char *target);
/* Wiring pulses and the LED bars only. Balanced startWrite/endWrite pairs. */
void botcorps_id_tick(lgfx::LovyanGFX &menu, int width, int bot, bool gib, uint8_t col);
