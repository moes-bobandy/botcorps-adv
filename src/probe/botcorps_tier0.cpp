/*
 * Tier 0 BSS at the planned final sizes. Reserved, not used as features.
 * The linker keeps them because botcorps_tier0_bytes() reads a byte.
 */
#include "probe/botcorps_probe.h"

#include <stdint.h>

static uint8_t s_rain_cell[3][1200];
static uint8_t s_rain_col[56][8];

static char s_term_prompt[80];
static char s_term_hist[20][26];
static char s_term_log[2048];
static char s_term_found[12][28];

static uint8_t s_bot_ui[2560];

static uint8_t s_wifi_q[24][32];
static uint8_t s_ble_q[24][32];

uint32_t botcorps_tier0_bytes(void)
{
    s_rain_cell[0][0] ^= 0;
    s_rain_col[0][0] ^= 0;
    s_term_prompt[0] ^= 0;
    s_term_hist[0][0] ^= 0;
    s_term_log[0] ^= 0;
    s_term_found[0][0] ^= 0;
    s_bot_ui[0] ^= 0;
    s_wifi_q[0][0] ^= 0;
    s_ble_q[0][0] ^= 0;
    return (uint32_t)sizeof(s_rain_cell) + (uint32_t)sizeof(s_rain_col)
         + (uint32_t)sizeof(s_term_prompt) + (uint32_t)sizeof(s_term_hist)
         + (uint32_t)sizeof(s_term_log) + (uint32_t)sizeof(s_term_found)
         + (uint32_t)sizeof(s_bot_ui)
         + (uint32_t)sizeof(s_wifi_q) + (uint32_t)sizeof(s_ble_q);
}

uint32_t botcorps_tier0_part(int which)
{
    switch (which) {
    case 0: return (uint32_t)sizeof(s_rain_cell);
    case 1: return (uint32_t)sizeof(s_rain_col);
    case 2: return (uint32_t)(sizeof(s_term_prompt) + sizeof(s_term_hist)
                              + sizeof(s_term_log) + sizeof(s_term_found));
    case 3: return (uint32_t)sizeof(s_bot_ui);
    default: return (uint32_t)(sizeof(s_wifi_q) + sizeof(s_ble_q));
    }
}
