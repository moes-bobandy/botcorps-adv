#pragma once

#include <stdint.h>

/*
 * Named palette indices. The band stores the index byte. pushImage reads
 * lut565 as swap565, so every lut entry is stored pre-swapped. Direct draws
 * swap that word back to normal-order RGB565.
 *
 * Reserved colours are not a block at 1..126. Each named colour stores its
 * exact RGB565 in its own slot. Index 0 stays black and index 255 stays
 * RGB332 white, so a dark colour whose nearest code is 0 is placed on the
 * nearest code that is still free. INT draws those exact values as uint16_t.
 * The band looks them up through a 4096-entry table (4 bits per channel).
 * Four ramp steps cover a 20-stamp trail.
 */
enum : uint8_t {
#include "botcorps_pal_enum.inc"
};

#include "botcorps_pal_assert.inc"

static_assert(botcorps_pal_ids_ok(), "0 and 255 stay black and RGB332 white");
static_assert(PAL_RAMP_N == 4, "each ink reuses 4 steps");
static_assert(PAL_WHITE == 255, "255 stays RGB332 white");
static_assert(PAL_BLACK == 0, "0 stays black");
static_assert(PAL_FIELD != PAL_WHITE && PAL_BOT_BACK != PAL_WHITE, "white is not a stolen dark slot");
static_assert(PAL_SOLID_G != PAL_SOLID_C && PAL_SOLID_C != PAL_SOLID_B, "solid inks keep distinct slots");
static_assert(PAL_DEV != PAL_DEV2 && PAL_DEV2 != PAL_ICON_C3, "icon colours keep distinct slots");
