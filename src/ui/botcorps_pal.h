#pragma once

#include <stdint.h>

/*
 * Named palette indices. The band stores the index byte. pushImage reads
 * lut565 as swap565, so every lut entry is stored pre-swapped. Direct draws
 * swap that word back to normal-order RGB565.
 *
 * Indices 216..222 and 224..254 are exact RGB565. Index 223 stays RGB332.
 * Index 255 stays RGB332 white (0xFF): white draws and any 0xFF key use that code.
 * PAL_BLACK is the free code 0.
 */
enum : uint8_t {
    PAL_BLACK = 0,
    PAL_R_FDC9 = 216, /* MULTI rain ink 0xFDC9 */
    PAL_R_F689,       /* 0xF689 */
    PAL_R_FA69,       /* 0xFA69 */
    PAL_R_E2FF,       /* 0xE2FF */
    PAL_R_CF89,       /* 0xCF89 */
    PAL_R_FC43,       /* 0xFC43 */
    PAL_R_6BBF,       /* 0x6BBF */
    PAL_LO_FIRST = PAL_R_FDC9,
    PAL_LO_HI = PAL_R_6BBF + 1, /* 223, first free code after the low block */
    PAL_FIELD = 224, /* #070a08  0x0041 */
    PAL_BG,          /* 0x0862 */
    PAL_BAR,         /* 0x10C4 */
    PAL_LINE,        /* 0x29C8 */
    PAL_INK,         /* 0xE77E */
    PAL_MUTED,       /* 0x8CD4 */
    PAL_AMBER,       /* 0xF5A9 */
    PAL_SEL,         /* 0x1105 */
    PAL_WHITE,       /* 0xFFFF */
    PAL_DEEP,        /* 0x0841 */
    PAL_IVORY,       /* 0xF79C */
    PAL_OK,          /* 0x8FF3 */
    PAL_HOLD,        /* 0xE38E */
    PAL_CYAN,        /* 0x4E7B accent */
    PAL_MINT,        /* 0x5FF8 accent */
    PAL_PLATE,       /* 0x1082 */
    PAL_BOARD_L,     /* 0xD69A */
    PAL_BOARD_D,     /* 0x18C3 */
    PAL_EYE,         /* 0x0080 */
    PAL_CODE,        /* 0x7E6C */
    PAL_HEAD,        /* 0xEFF9 */
    PAL_B5D3F,       /* 0x5D3F bot */
    PAL_B4E7F,       /* 0x4E7F bot */
    PAL_B7711,       /* 0x7711 bot */
    PAL_B9F0B,       /* 0x9F0B bot */
    PAL_BFB72,       /* 0xFB72 bot */
    PAL_BAC7F,       /* 0xAC7F bot */
    PAL_B36B8,       /* 0x36B8 bot */
    PAL_BFC6B,       /* 0xFC6B bot */
    PAL_BBD3F,       /* 0xBD3F bot */
    PAL_DIAG,        /* 0x4A69 diag icon */
    PAL_RES_LO = PAL_FIELD,
    PAL_RES_HI = PAL_DIAG + 1
};

static_assert(PAL_LO_FIRST == 216, "low exact block starts at 216");
static_assert(PAL_R_6BBF == 222, "seven MULTI colours occupy 216..222");
static_assert(PAL_LO_HI == 223, "223 stays a free RGB332 code");
static_assert(PAL_FIELD == 224, "upper exact block still starts at 224");
static_assert(PAL_DIAG == 254, "254 stays the last exact slot");
static_assert(PAL_RES_HI == 255, "255 stays RGB332 white");
