/*
 * Tier 0 BSS at the planned final sizes. The boot touch writes and
 * reads every byte through a volatile pointer so the linker cannot
 * drop the arrays. After that touch, rain uses the cell and column
 * buffers and the terminal uses the prompt, log, history, and found
 * buffers.
 */
#include "probe/botcorps_probe.h"

#include <stddef.h>
#include <stdint.h>

uint8_t botcorps_t0_rain_cell[3][1200];
uint8_t botcorps_t0_rain_col[56][8];

char botcorps_t0_term_prompt[80];
char botcorps_t0_term_hist[20][26];
char botcorps_t0_term_log[2048];
char botcorps_t0_term_found[12][28];

alignas(8) uint8_t botcorps_t0_bot_ui[2560];

uint8_t botcorps_t0_wifi_q[24][32];
uint8_t botcorps_t0_ble_q[24][32];

static uint8_t touch_span(void *base, size_t n)
{
    volatile uint8_t *p = (volatile uint8_t *)base;
    uint8_t acc = 0;
    for (size_t i = 0; i < n; ++i) {
        const uint8_t v = p[i];
        p[i] = (uint8_t)(v ^ (uint8_t)i);
        acc = (uint8_t)(acc ^ p[i]);
        p[i] = v;
    }
    return acc;
}

static volatile uint8_t s_tier0_sink;

uint32_t botcorps_tier0_bytes(void)
{
    uint8_t acc = 0;
    acc ^= touch_span(botcorps_t0_rain_cell, sizeof(botcorps_t0_rain_cell));
    acc ^= touch_span(botcorps_t0_rain_col, sizeof(botcorps_t0_rain_col));
    acc ^= touch_span(botcorps_t0_term_prompt, sizeof(botcorps_t0_term_prompt));
    acc ^= touch_span(botcorps_t0_term_hist, sizeof(botcorps_t0_term_hist));
    acc ^= touch_span(botcorps_t0_term_log, sizeof(botcorps_t0_term_log));
    acc ^= touch_span(botcorps_t0_term_found, sizeof(botcorps_t0_term_found));
    acc ^= touch_span(botcorps_t0_bot_ui, sizeof(botcorps_t0_bot_ui));
    acc ^= touch_span(botcorps_t0_wifi_q, sizeof(botcorps_t0_wifi_q));
    acc ^= touch_span(botcorps_t0_ble_q, sizeof(botcorps_t0_ble_q));
    s_tier0_sink = acc;
    return (uint32_t)sizeof(botcorps_t0_rain_cell)
         + (uint32_t)sizeof(botcorps_t0_rain_col)
         + (uint32_t)sizeof(botcorps_t0_term_prompt)
         + (uint32_t)sizeof(botcorps_t0_term_hist)
         + (uint32_t)sizeof(botcorps_t0_term_log)
         + (uint32_t)sizeof(botcorps_t0_term_found)
         + (uint32_t)sizeof(botcorps_t0_bot_ui)
         + (uint32_t)sizeof(botcorps_t0_wifi_q)
         + (uint32_t)sizeof(botcorps_t0_ble_q);
}

uint32_t botcorps_tier0_part(int which)
{
    switch (which) {
    case 0: return (uint32_t)sizeof(botcorps_t0_rain_cell);
    case 1: return (uint32_t)sizeof(botcorps_t0_rain_col);
    case 2: return (uint32_t)(sizeof(botcorps_t0_term_prompt)
                              + sizeof(botcorps_t0_term_hist)
                              + sizeof(botcorps_t0_term_log)
                              + sizeof(botcorps_t0_term_found));
    case 3: return (uint32_t)sizeof(botcorps_t0_bot_ui);
    default: return (uint32_t)(sizeof(botcorps_t0_wifi_q)
                               + sizeof(botcorps_t0_ble_q));
    }
}
