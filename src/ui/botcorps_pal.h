#pragma once

#include <stdint.h>

/*
 * Named palette indices. The band stores the index byte. pushImage reads
 * lut565 as swap565, so every lut entry is stored pre-swapped. Direct draws
 * swap that word back to normal-order RGB565.
 *
 * Reserved colours are not a block at 1..126. Each exact colour and each ramp
 * step sits on the RGB332 code nearest its real colour, and only when that
 * code already looked almost the same. Pass-bar colours (field, bot backing,
 * solids, device ink, icon second colour, fixed bots) always keep an exact
 * slot on their nearest code. Index 0 stays black. Index 255 stays RGB332
 * white. Four ramp steps are reused across a 20-stamp trail.
 *
 * Some names share an index when a second colour is nearer to that exact slot
 * than to any leftover code. The lut still stores one native per index.
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
