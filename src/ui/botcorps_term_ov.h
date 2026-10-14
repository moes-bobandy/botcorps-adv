#pragma once
#include <stdint.h>

/* Exact sheet colours. Not part of the shared 256-word lookup.
 * Installed into the live lut only for a full Terminal-sheet present. */
static const uint8_t botcorps_term_ov_i[13] = {
    230, 231, 232, 234, 235, 236, 237, 239, 242, 243, 246, 247, 248
};
static const uint16_t botcorps_term_ov_c[13] = {
    0xF95A, 0x48A9, 0x3887, 0x3086, 0x58CA, 0x2064, 0x079F,
    0x026A, 0x01E8, 0x01A7, 0x02EC, 0x0105, 0xE6FA
};
static const int botcorps_term_ov_n = 13;
