#pragma once

#include <stdint.h>

/*
 * Named palette indices. The band stores the index byte. pushImage reads
 * lut565 as swap565, so every lut entry is stored pre-swapped. Direct draws
 * swap that word back to normal-order RGB565.
 *
 * Indices 224..254 are the reserved mock-hex block (exact RGB565, not RGB332).
 * Index 255 and 0..223 stay RGB332. PAL_BLACK is the free code 0.
 */
enum : uint8_t {
    PAL_BLACK = 0,
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

static_assert(PAL_RES_HI <= 256, "reserved block stays inside the lut");
static_assert(PAL_RES_HI - PAL_RES_LO <= 32, "reserved block is at most 32");
static_assert(PAL_RES_HI - PAL_RES_LO >= 16, "reserved block covers the mock hexes");
