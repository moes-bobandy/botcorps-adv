#include "ui/botcorps_sprite.h"

#include "ui/botcorps_band.h"

#include <pgmspace.h>

/* Mock SPRITE_SRC. 0 empty, 1 body, 2 eye. Width is the first byte of each row set. */
static const uint8_t SPR_ROUND[] PROGMEM = {
    0,0,0,1,0,0,0,
    0,1,1,1,1,1,0,
    1,1,1,1,1,1,1,
    1,2,2,1,2,2,1,
    1,2,2,1,2,2,1,
    1,1,1,1,1,1,1,
    0,1,1,1,1,1,0,
};
static const uint8_t SPR_CLASSIC[] PROGMEM = {
    0,0,0,0,1,0,0,0,0,
    0,1,1,1,1,1,1,1,0,
    1,1,2,2,1,2,2,1,1,
    1,1,2,2,1,2,2,1,1,
    1,1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,1,0,
};
static const uint8_t SPR_TALL[] PROGMEM = {
    0,0,0,1,0,0,0,
    1,1,1,1,1,1,1,
    1,2,2,1,2,2,1,
    1,2,2,1,2,2,1,
    1,1,1,1,1,1,1,
    0,1,1,1,1,1,0,
    0,1,1,1,1,1,0,
    0,1,1,0,1,1,0,
    0,1,1,0,1,1,0,
};
static const uint8_t SPR_MINI[] PROGMEM = {
    1,1,1,1,1,
    1,2,1,2,1,
    1,1,1,1,1,
    1,1,1,1,1,
    1,1,1,1,1,
};

static const uint8_t *const SPR_SRC[BOTCORPS_SHAPE_N] = {
    SPR_ROUND, SPR_CLASSIC, SPR_TALL, SPR_MINI
};
static const uint8_t SPR_W[BOTCORPS_SHAPE_N] = { 7, 9, 7, 5 };
static const uint8_t SPR_H[BOTCORPS_SHAPE_N] = { 7, 6, 9, 5 };
static const uint8_t COL_EYE = PAL_EYE;

int botcorps_shape_for(int index)
{
    if (index < 0) index = 0;
    return index % BOTCORPS_SHAPE_N;
}

void botcorps_sprite_draw(lgfx::LovyanGFX &dst, int x, int y, int shape, int scale,
                          uint8_t color, bool blink)
{
    if (shape < 0 || shape >= BOTCORPS_SHAPE_N) shape = 0;
    if (scale < 1) scale = 1;
    const int w = SPR_W[shape];
    const int h = SPR_H[shape];
    const uint8_t *src = SPR_SRC[shape];
    const int ox = x - (w * scale) / 2;
    const int oy = y - (h * scale) / 2;
    for (int row = 0; row < h; ++row) {
        int col = 0;
        while (col < w) {
            const uint8_t v = pgm_read_byte(src + row * w + col);
            int run = 1;
            while (col + run < w && pgm_read_byte(src + row * w + col + run) == v) ++run;
            if (v == 1 || (v == 2 && !blink)) {
                const uint8_t ink = (v == 2) ? COL_EYE : color;
                botcorps_pal_rect(dst, ox + col * scale, oy + row * scale, run * scale, scale, ink);
            }
            col += run;
        }
    }
}
