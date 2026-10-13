#pragma once

/*
 * botcorps_rain — cell-grid rain on the external panel.
 *
 * The step is 140 ms. Columns live in the Tier 0 column buffer and
 * the 8×8 cells live in the Tier 0 rain-cell buffer. Each step draws
 * those cells straight to the panel. There is no full-frame sprite.
 * Cells that intersect a cover, and cells inside the radar disc, are
 * left for the UI. The internal panel is never painted.
 */

#include <stdint.h>
#include <stdbool.h>

/* 0 = MULTI (the 14-colour palette). 1..6 = solid inks. */
#define BOTCORPS_RAIN_COLORS 7

/* 0 = bots and code, 1 = bots only, 2 = code only. */
#define BOTCORPS_RAIN_MIX_CREW 0
#define BOTCORPS_RAIN_MIX_BOTS 1
#define BOTCORPS_RAIN_MIX_CODE 2

struct botcorps_cover_t {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

/* Covers are chrome. radar_disc skips the sweep circle as well. */
void botcorps_rain_set_covers(const botcorps_cover_t *boxes, int n, bool radar_disc);
/* False until the UI is up, so the boot probe does not paint. */
void botcorps_rain_set_draw(bool enable);

/* Advance one 140 ms step and paint the open cells. */
bool botcorps_rain_step(void);
/* Paint the current cells into the open area. No step. */
void botcorps_rain_paint(void);

void botcorps_rain_set_parked(bool parked);
bool botcorps_rain_sprite_held(void);
void botcorps_rain_release_sprite(void);
void botcorps_rain_log_heap(const char *tag);
uint16_t botcorps_rain_palette(int index);
void botcorps_rain_set_mode(int mode);
int botcorps_rain_mode(void);
void botcorps_rain_set_mix(int mix);
int botcorps_rain_mix(void);
uint16_t botcorps_rain_solid(void);
