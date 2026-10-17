#pragma once

#include <stdint.h>

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

struct botcorps_heap_t {
    uint32_t free_b;
    uint32_t max_b;
    uint32_t min_b;
};

enum {
    BOTCORPS_ST_M0 = 0,
    BOTCORPS_ST_M5,
    BOTCORPS_ST_E0,
    BOTCORPS_ST_EXT,
    BOTCORPS_ST_T0,
    BOTCORPS_ST_A0,
    BOTCORPS_ST_A1,
    BOTCORPS_ST_SC0,
    BOTCORPS_ST_SC1,
    BOTCORPS_ST_B0,
    BOTCORPS_ST_B1,
    BOTCORPS_ST_F0,
    BOTCORPS_ST_SF,
    BOTCORPS_ST_W0,
    BOTCORPS_ST_W1,
    BOTCORPS_ST_OV0,
    BOTCORPS_ST_OV1,
    BOTCORPS_ST_N2,
    BOTCORPS_ST_N
};

struct botcorps_probe_t {
    botcorps_heap_t st[BOTCORPS_ST_N];
    uint8_t ord[BOTCORPS_ST_N];
    char ext_st[6];
    char nim_a[8];
    char nim_b[8];
    char nim_2[8];
    int nim_a_rc;
    int nim_b_rc;
    int nim_2_rc;
    int wifi_n;
    int ov_ok;
    int sprite_held;
    int sprite_freed;
    uint32_t sprite_bytes;
    uint32_t tier0;
};

void botcorps_heap_sample(botcorps_heap_t *out);
void botcorps_heap_log(const char *tag);

/* Largest free internal block, before M5Cardputer.begin(). */
void botcorps_probe_before_m5(void);
/* Largest free internal block, after M5Cardputer.begin(). */
void botcorps_probe_after_m5(void);
/* Largest free internal block, before EXT begin. */
void botcorps_probe_before_ext(void);
/* After EXT begin attempt. ext_ok is the size-checked result. */
void botcorps_probe_after_ext(bool ext_ok);
/* Tier 0, both NimBLE cases, Wi-Fi scan, overlay sprite. */
void botcorps_probe_run(void);

void botcorps_probe_screen(const char *leave, const char *enter);
void botcorps_probe_paint(lgfx::LovyanGFX &g);

const botcorps_probe_t *botcorps_probe_get(void);
uint32_t botcorps_tier0_bytes(void);
uint32_t botcorps_tier0_part(int which);
