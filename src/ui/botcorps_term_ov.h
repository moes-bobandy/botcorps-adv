#pragma once
#include <stdint.h>

/* Exact sheet colours. Not part of the shared 256-word lookup.
 * push_slice installs these words on the EXT panel only, and only while
 * a full Terminal sheet is up. INT keeps the shared lookup.
 * Slot order matches botcorps_term_map (value = slot + 1). */
static constexpr uint8_t botcorps_term_ov_i[] = {
    226, 227, 228, 229, 230, 231, 232, 234, 235, 236, 237, 239, 242, 243, 246, 247, 248
};
static constexpr uint16_t botcorps_term_ov_c[] = {
    0xD7BF, /* 226 #D6F7FF light cyan UI.ink */
    0xFF7A, /* 227 #FFEFD6 light amber UI.ink */
    0xDFFE, /* 228 #DEFFF7 light mint UI.ink */
    0xC67B, /* 229 #c5d0da unselected command name */
    0xF95A, 0x48A9, 0x3887, 0x3086, 0x58CA, 0x2064, 0x079F,
    0x026A, 0x01E8, 0x01A7, 0x02EC, 0x0105, 0xE6FA
};
static constexpr int botcorps_term_ov_n =
    (int)(sizeof(botcorps_term_ov_i) / sizeof(botcorps_term_ov_i[0]));
static_assert(sizeof(botcorps_term_ov_i) / sizeof(botcorps_term_ov_i[0]) ==
                  sizeof(botcorps_term_ov_c) / sizeof(botcorps_term_ov_c[0]),
              "term override table");

/* Direct remap. 0 means "not a sheet code". Any other value is one past
 * the slot in botcorps_term_ov_c. Compile-time constant, flash only. */
static constexpr uint8_t botcorps_term_map[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 8, 9, 10, 11, 0, 12,
    0, 0, 13, 14, 0, 0, 15, 16, 17, 0, 0, 0, 0, 0, 0, 0
};
static_assert(botcorps_term_map[226] == 1 && botcorps_term_map[229] == 4, "term map");
static_assert(botcorps_term_map[248] == 17 && botcorps_term_map[233] == 0, "term map gaps");

/* Native RGB565 for a sheet code, or 0 when the swap is off or the index
 * is not a sheet code. push_slice byte-swaps this word onto the EXT panel. */
static inline uint16_t botcorps_term_ov_native(uint8_t idx, int on)
{
    if (!on) return 0;
    const uint8_t slot = botcorps_term_map[idx];
    if (!slot) return 0;
    return botcorps_term_ov_c[slot - 1];
}
