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

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* 0 = MULTI (the 14-colour palette). 1..6 = solid inks. */
#define BOTCORPS_RAIN_COLORS 7

/* Mock BACKGROUND mixes. FIELD is bots, devices, and code.
 * BOTS is bots plus code. DEVICES is devices, code, and empty columns. */
#define BOTCORPS_RAIN_MIX_FIELD 0
#define BOTCORPS_RAIN_MIX_BOTS  1
#define BOTCORPS_RAIN_MIX_DEV   2

/* fade, id, glyph. 8x8 grid: 40*30*3 = 3600. A 6 px cell grid at 3 bytes is 6480. */
struct botcorps_cell_t {
    uint8_t fade;
    uint8_t id;
    uint8_t glyph;
};

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
/* v1019x: current draw-enable state (so a caller can step without the panel paint and restore it). */
bool botcorps_rain_draw_enabled(void);

/* Advance one 140 ms step and paint the open cells. */
bool botcorps_rain_step(void);
/* Paint the current cells into the open area. No step. */
void botcorps_rain_paint(void);
/* Draw the slice [world y0, y1) into dst. dst y is world y - y0. */
void botcorps_rain_draw(lgfx::LovyanGFX &dst, int y0, int y1);
/* v1019x: the same slice draw limited to a world-space window (cells clipped to it, others skipped);
 * ex = up to 4 world rects {x, y, w, h} inside it where no cell is drawn (a cell touching one is skipped). */
void botcorps_rain_draw_win(lgfx::LovyanGFX &dst, int y0, int y1, int wx, int wy, int ww, int wh,
                            const int16_t (*ex)[4], int nex);
/* Gibson hub: scale rain inks by 0.58 toward #030706 through the fade table. */
void botcorps_rain_set_hub(bool on);
/* v1017c: Gibson's screen (hub, Gibson's scan board): bots in the rain are ivory Gibsons and the
 * devices take the Gibson-screen inks. */
void botcorps_rain_set_gibson(bool on);
/* v1019n (#15): Terminal / prompt: OUR DEVICE-ICON RAIN ONLY (the 10 device icons; no bits, no bots, no
 * background; recoloured only by the BOT-RAIN colour toggle). */
void botcorps_rain_set_no_bots(bool on);
/* Device icons at about a quarter strength (dce1c87 canon faint). */
void botcorps_rain_set_faint(bool on);
/* Faint mix. Terminal stays 1/4. Defend uses 2/5 (~40%). */
void botcorps_rain_set_faint_scale(int num, int den);
/* Defend only: 0 = all 10 device icons, 1 = k0, 2 = k0 or k1. Kinds are rain DEV index. */
void botcorps_rain_set_devsel(int n, int k0, int k1);
/* Gibson's framed panel: the full glossary device set, each in its fixed colour. */
void botcorps_rain_set_devall(bool on);
/* v1019: faint ghost bits of the live rain over a chrome rect (world coords, dst rows from oy). */
void botcorps_rain_ghost(lgfx::LovyanGFX &dst, int ox, int oy, int rx, int ry, int rw, int rh, uint8_t ink);
void botcorps_rain_ghost_ex(lgfx::LovyanGFX &dst, int ox, int oy, int rx, int ry, int rw, int rh, uint8_t ink,
                            bool top_whole);
int botcorps_rain_cell_bytes(void);

void botcorps_rain_set_parked(bool parked);
bool botcorps_rain_sprite_held(void);
void botcorps_rain_release_sprite(void);
void botcorps_rain_log_heap(const char *tag);
uint8_t botcorps_rain_palette(int index);
void botcorps_rain_set_mode(int mode);
int botcorps_rain_mode(void);
void botcorps_rain_set_mix(int mix);
/* 0 = mixed shapes. 1..4 = round, classic, tall, mini. */
void botcorps_rain_set_shape(int shape);
int botcorps_rain_mix(void);
/* v1019n (#6): BACKGROUND = the former screen-saver animations, run live INSTEAD of the rain:
 * 0 RAIN (our BOTcorps rain, default), 1 BOTS, 2 GIBSON, 3 FLYBY, 4 BOUNCE. The Terminal ignores it. */
void botcorps_rain_set_bg(int style);
int botcorps_rain_bg(void);
uint8_t botcorps_rain_solid(void);
