#pragma once
#include <stdint.h>

/* Exact sheet colours. Not part of the shared 256-word lookup.
 * push_slice installs these words on the EXT panel only, and only while
 * a full Terminal sheet is up. INT keeps the shared lookup. */
static const uint8_t botcorps_term_ov_i[] = {
    229, 230, 231, 232, 234, 235, 236, 237, 239, 242, 243, 246, 247, 248
};
static const uint16_t botcorps_term_ov_c[] = {
    0xC67B, /* 229 #c5d0da unselected command name */
    0xF95A, 0x48A9, 0x3887, 0x3086, 0x58CA, 0x2064, 0x079F,
    0x026A, 0x01E8, 0x01A7, 0x02EC, 0x0105, 0xE6FA
};
static const int botcorps_term_ov_n =
    (int)(sizeof(botcorps_term_ov_i) / sizeof(botcorps_term_ov_i[0]));
static_assert(sizeof(botcorps_term_ov_i) / sizeof(botcorps_term_ov_i[0]) ==
                  sizeof(botcorps_term_ov_c) / sizeof(botcorps_term_ov_c[0]),
              "term override table");

/* Native RGB565 for a sheet code, or 0 when the swap is off or the index
 * is not a sheet code. push_slice byte-swaps this word onto the EXT panel. */
static inline uint16_t botcorps_term_ov_native(uint8_t idx, int on)
{
    if (!on) return 0;
    for (int n = 0; n < botcorps_term_ov_n; ++n) {
        if (botcorps_term_ov_i[n] == idx) return botcorps_term_ov_c[n];
    }
    return 0;
}
