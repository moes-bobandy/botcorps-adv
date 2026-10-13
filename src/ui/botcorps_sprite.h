#pragma once

#include <stdint.h>
#include <stdbool.h>

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* Mock SPRITE_SRC order. MIXED cycles these by index. Bitmaps stay in flash. */
enum {
    BOTCORPS_SHAPE_ROUND = 0,
    BOTCORPS_SHAPE_CLASSIC,
    BOTCORPS_SHAPE_TALL,
    BOTCORPS_SHAPE_MINI,
    BOTCORPS_SHAPE_N
};

int botcorps_shape_for(int index);
void botcorps_sprite_draw(lgfx::LovyanGFX &dst, int x, int y, int shape, int scale,
                          uint8_t color, bool blink);
