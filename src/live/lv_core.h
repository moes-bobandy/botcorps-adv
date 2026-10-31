#pragma once
/*
 * lv_core - cyd_livescan UI (portable: firmware src/live/lv_ui.cpp + host sim tools/livescan/sim_live.cpp).
 *
 * The LOCKED non-game clips (BOOTbot, GIBSONbot home, TERMINALbot, SCANbot, CONTROLbot) keep playing
 * from the byte-identical live blob. Live overlays own a few rectangles per screen: the clip player's
 * row runs are cut around them (lv_clip_draw), so the clip never paints there and the overlays never
 * repaint the clip. Overlays: the emergency ALERT TICKER in the top bar (every screen but BOOTbot and
 * the touch-cal routine), the SCANbot results box + its terminal box, the TERMINALbot terminal box, the
 * CONTROLbot value fields, and the full-screen operable terminal with an on-screen keyboard (TTY).
 * Radios go through LvRadio: one radio at a time (WiFi scan, then radio off, then BLE scan, then BLE
 * deinit), never both.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "../lock/lock_core.h"
#include "lv_draw.h"

#ifdef BOTCORPS_BUILD_ID
#define LV_VERSION "cyd-livescan 1.0 " BOTCORPS_BUILD_ID
#else
#define LV_VERSION "cyd-livescan 1.0"
#endif

/* ---------- finds ---------- */
enum { LV_K_WIFI = 0, LV_K_BLE = 1 };
struct LvFind {
    char name[33];
    uint8_t mac[6];
    int8_t rssi;
    uint8_t ch;      /* WiFi channel */
    uint8_t auth;    /* WiFi wifi_auth_mode_t */
    uint8_t kind;    /* LV_K_* */
    uint16_t mfg;    /* BLE company id, 0xFFFF none */
    uint8_t apple;   /* Apple continuity type (mfg data byte 2), 0 none */
    uint16_t svc;    /* first 16-bit service UUID of interest, 0 none */
    uint8_t icon;    /* LvIcon */
    uint8_t alert;   /* matched the last hunt */
};
/* Background engine (BG STYLE / BG COLOR on the RAIN / RAIN COL chips, NVS lkbgs / lkbgc): DISABLED 2026-10-31 12:02 ET
 * (Mobandy: "get rid of the background changing ui for the gibson scan", "no extra background on the scan screen").
 * Kept in the source but compiled out: with 0 the radar, the CTL chips and NVS are exactly as in 7ecb753. */
#ifndef LV_BG_ENGINE
#define LV_BG_ENGINE 0
#endif
/* CONTROLbot value boxes (theme / focus names, BG MOTION and the 7 per-chip value badges): REMOVED 2026-10-31 12:04 ET
 * (Mobandy: "get rid of the blocks of changing text not redesign all the chips"). With 0 those rects are no longer masked
 * or overdrawn, so the locked CTL clip shows its own pixels there; chips, positions and taps are unchanged. */
#ifndef LV_CTL_VALUEBOX
#define LV_CTL_VALUEBOX 0
#endif
/* WiFi / BLE bot identity colours — fixed, not theme-tinted (Mobandy 2026-10-31 12:06 ET:
 * "Make WiFi bot orange and Bluetooth bot blue"). Distinct from SCANbot gold. */
#ifndef LV_WIFI_COL
#define LV_WIFI_COL 0xff7a00   /* ~#FF7A00 orange */
#endif
#ifndef LV_BLE_COL
#define LV_BLE_COL  0x2f6bff   /* ~#2F6BFF blue */
#endif
/* CONTROLbot UI-selection chips redrawn as BACKbot-form overlays (Mobandy 12:05-12:07 ET:
 * "make the chips shaped like the BACKbot", "just the ui selection chips", "keep controlbot the same"). */
#ifndef LV_CTL_CHIPBOT
#define LV_CTL_CHIPBOT 1
#endif
#define LV_MAXF 48

/* ---------- radio + system hooks ---------- */
struct LvRadio {
    int (*wifi_start)(void);                     /* 0 = started */
    int (*wifi_poll)(LvFind *out, int max);      /* -1 running; else count, WiFi radio already OFF */
    int (*ble_start)(uint32_t ms);
    int (*ble_poll)(LvFind *out, int max);       /* -1 running; else count, BLE stack already deinit */
    int (*deauth_start)(uint32_t ms);            /* passive promiscuous monitor, no transmit */
    int (*deauth_poll)(uint32_t *deauth, uint32_t *mgmt, uint8_t *ch); /* -1 running; else done, radio OFF */
    void (*stop)(void);                          /* abort whatever runs, radios OFF */
    void (*sys)(int what, char *out, int n);     /* 0 heap, 1 chip, 2 reset reason, 3 flash, 4 touch cal */
    void (*save_theme)(int t);
    bool (*cal_save)(const uint16_t c8[8]);     /* apply the 8-point matrix + write NVS c0..c7, cv LAST; true = saved */
    void (*set_rot)(int flip);                   /* 0 = normal (setRotation 2, USB at the bottom), 1 = flipped (rot 0); saved */
    int (*mac_load)(const char *key, uint8_t *out, int max);        /* radar IGNORE / WATCH lists (6 bytes per MAC), NVS */
    void (*mac_save)(const char *key, const uint8_t *macs, int n);
#if LV_BG_ENGINE
    void (*bg_save)(int style, int col);         /* background engine: BG STYLE 0..4 (5 = OFF) + BG COLOR 0..5, NVS lkbgs / lkbgc */
#endif
};

enum LvScr { LV_SC_BOOT, LV_SC_HOME_INTRO, LV_SC_HOME, LV_SC_TERM, LV_SC_SCAN, LV_SC_CTL, LV_SC_TTY, LV_SC_CAL, LV_SC_RADAR };
#define LV_NTHEME 12
/* Issue 2: the full original CYD THEMES[11] (src/ui/botcorps_ui.cpp) on the live screens. 0..5 keep their NVS
 * meaning (lktheme) and their LOCKED clips: GRAPE = NEON GRAPE, MAGENTA = HOT PINK, TOXIC = TOXIC, ICE = ICE,
 * ORANGE = BLAZE ORANGE; BLUE is a live-only extra (no original is a solid blue). 6..11 are the originals the
 * live UI never had: they play the ICE clip and re-ink its cyan accent pixels (lv_multi_ink) - gold, silver and
 * pink art is never cyan, so SCANbot / TERMINALbot / BACKbot keep their own colours. LV_THEME_NAME fits the
 * 31 px THEME field; LV_THEME_FULL is the original name (terminal 'theme', CONTROLbot name plate). */
static const char *const LV_THEME_NAME[LV_NTHEME] = {"GRAPE", "MAGENTA", "TOXIC", "ICE", "BLUE", "ORANGE", "GOLD", "SYNTH", "AURA", "PRISM", "GLTCH", "ROSE"};
static const char *const LV_THEME_FULL[LV_NTHEME] = {"NEON GRAPE", "HOT PINK", "TOXIC", "ICE", "BLUE", "BLAZE ORANGE", "GOLD RUSH", "SYNTHWAVE", "AURORA", "PRISM", "GLITCH", "HOT ROSE"};
#define LV_THEME_ORIG 6          /* first re-inked theme */
#define LV_THEME_BASE 3          /* ICE: the clip the re-inked themes play */
/* Theme inks sampled from the LOCKED SCANbot/TERMINALbot clips (frame 0 of each theme loop). 6..11: the
 * originals' accents (botcorps_rgb565 of ac); on the multi themes (7..10) these four values are sentinels
 * that lv_blit_theme re-inks per pixel, so every overlay stroke follows the fade / spectrum / split. */
static const uint32_t LV_ACC[LV_NTHEME] = {0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052};
static const uint32_t LV_ACCD[LV_NTHEME] = {0x53205d, 0x68004f, 0x4a6010, 0x00636d, 0x0e5166, 0x61480f, 0x6a5a00, 0x690d69, 0x0d690d, 0x086908, 0x690405, 0x690022};
static const uint32_t LV_HDR[LV_NTHEME] = {0xb36afe, 0xdc4aac, 0xb9fb4f, 0x4afffc, 0x6891ff, 0xf4b24c, 0xffe86a, 0xff6afd, 0x6aff6b, 0x6afe6c, 0xff6a6b, 0xff6a96};
static const uint32_t LV_FRAME[LV_NTHEME] = {0xff25ae, 0xdf00ae, 0x92fd00, 0x00feff, 0xfd22ff, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052};
static bool lv_multi_theme(int t) { return t >= 7 && t <= 10; }
/* the theme's ink at screen pixel (x, y), 0xRRGGBB (originals: th_ink_at / sel_ink in botcorps_ui.cpp) */
static uint32_t lv_mix3(uint32_t a, uint32_t b, int num, int den)
{
    uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) { const int ca = (a >> sh) & 255, cb = (b >> sh) & 255; o |= (uint32_t)(ca + (cb - ca) * num / den) << sh; }
    return o;
}
static uint32_t lv_theme_at(int t, int x, int y)
{
    switch (t) {
    case 7: return lv_mix3(0xff24ff, 0xff8918, y, 319);                                    /* SYNTHWAVE: magenta top -> sunset orange */
    case 8: {                                                                                 /* AURORA: ice sky -> green -> violet */
        const int k = y < 40 ? 40 : y > 280 ? 280 : y;
        return k < 160 ? lv_mix3(0x00daff, 0x20ff00, k - 40, 120) : lv_mix3(0x20ff00, 0xb400ff, k - 160, 120);
    }
    case 9: { static const uint32_t H[6] = {0xff24ff, 0xff8918, 0xffda00, 0x20ff00, 0x00ffff, 0x3161ff}; return H[((x + y) / 22) % 6]; } /* PRISM */
    case 10: return ((y / 3) % 4 == 1) ? 0x00ffff : 0xff0000;                                /* GLITCH: red, cyan split bands */
    default: return LV_ACC[t];
    }
}
#define LV_PROMPT_INK 0xdd00a7
#define LV_TAN 0xb39152
#define LV_PROMPT "user@botcorps:~$"

/* ---------- geometry (native portrait 240x320, measured on the LOCKED clips) ---------- */
/* ticker = the whole top bar inside the gold rim (item 3: the crown + top-left GIBSON icon are gone; the colour
 * swatches + counter were already hidden under the old two ticker masks) */
static const LvRect LV_R_TICK = {1, 1, 238, 23};
static const LvRect LV_R_SBOX = {33, 153, 206, 256};  /* SCANbot device-emoji grid (old 4 tabs + results box) */
/* Small terminal box (item 11): the whole inner box is now overlay - 1 line row, an enlarged button row right above
 * the prompt, and the prompt. The old 20x12 icon stack hung 5 rows out of the box (spill); those rows are rebuilt from
 * the clip's own art 12 px to the left (pins / rails repeat every 12 px), see LV_T_SPILL. */
static const LvRect LV_R_STERM = {32, 275, 206, 308}; /* SCANbot terminal box inner */
static const LvRect LV_R_TTERM = {34, 272, 204, 305}; /* HOME / TERMINALbot / CONTROLbot terminal box inner */
static const LvRect LV_R_SPILL_S = {172, 309, 203, 313}, LV_R_SPILL_T = {170, 306, 201, 310};
static const LvRect &lv_mt_box(void);
static const LvRect LV_R_CTHEME = {82, 36, 112, 44}, LV_R_CUP = {76, 51, 112, 59}, LV_R_CFOCUS = {82, 66, 112, 74};
static const LvRect LV_R_CMOTION = {178, 246, 198, 252};
static const LvRect LV_R_TTY_TICK = {0, 0, 239, 23}; /* full width; BACKbot paints on top in CLOSE slot */
static const LvRect LV_R_TTY_BODY = {0, 24, 239, 173}, LV_R_TTY_OUT = {1, 37, 238, 160};
static const LvRect LV_R_KBD = {0, 174, 239, 319};
#define LV_TTY_LINES 13
#define LV_TTY_COLS 38
#define LV_KBD_Y 174
#define LV_KBD_PITCH_X 24
#define LV_KBD_PITCH_Y 29
#define LV_KEY_W 22
#define LV_KEY_H 27

/* ---------- state ---------- */
static const LvRadio *lv_radio;
static lv_blit_fn lv_blit;
static LkPlay lv_play;
static int lv_scr = LV_SC_BOOT, lv_theme = 0;
static bool lv_motion = true, lv_enter = true;
static uint32_t lv_now, lv_clip_t, lv_boot_ms;
static bool lv_selftest_pending = false;

static LvFind lv_f[LV_MAXF];
static int lv_nf = 0;
static LvFind lv_tmp[LV_MAXF];

enum { LV_J_NONE, LV_J_WIFI, LV_J_BLE, LV_J_ALL, LV_J_DEAUTH };
static int lv_job = LV_J_NONE, lv_job_phase = 0, lv_hunt = -1;
static uint32_t lv_job_t0;
static uint32_t lv_deauth_n, lv_mgmt_n; static uint8_t lv_deauth_ch; static bool lv_deauth_have = false;

/* scan view */
static int lv_mode = 0; /* 0 ALL 1 WIFI 2 BLE 3 DEAUTH */
static int lv_page = 0, lv_sel = -1;
static bool lv_detail = false;
static bool lv_dirty_box = true, lv_dirty_term = true, lv_dirty_ctl = true, lv_dirty_tty = true, lv_dirty_kbd = true;
static int lv_focus = 0; /* ticker focus: 0 all 1 wifi 2 ble 3 alerts */
static const char *const LV_FOCUS_NAME[4] = {"ALL", "WIFI", "BLE", "ALERTS"};

/* ---------- terminal text ---------- */
#define LV_TLINES 80
static char lv_tl[LV_TLINES][LV_TTY_COLS + 1];
static int lv_tn = 0;      /* total lines ever (ring) */
static uint8_t lv_tcont[LV_TLINES]; /* 1 = this stored line continues the previous print() (wrapped at 38 cols) */
static int lv_scroll = 0;  /* lines up from the bottom in TTY */
static char lv_inb[64];
static int lv_inlen = 0;
static bool lv_shift = false;
static char lv_hist[8][64];
static int lv_nhist = 0;
static int lv_key_hl = -1; static uint32_t lv_key_hl_t;

static void lv_tline(const char *s)
{
    char *d = lv_tl[lv_tn % LV_TLINES];
    lv_tcont[lv_tn % LV_TLINES] = 0;
    strncpy(d, s, LV_TTY_COLS); d[LV_TTY_COLS] = 0;
    ++lv_tn; lv_scroll = 0;
    lv_dirty_term = lv_dirty_tty = true;
}
static void lv_print(const char *fmt, ...)
{
    char buf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    char *s = buf;
    while (1) {
        char *nl = strchr(s, '\n');
        if (nl) *nl = 0;
        int len = (int)strlen(s);
        if (len == 0 && nl) lv_tline("");
        for (int o = 0; o < len; o += LV_TTY_COLS) { char t[LV_TTY_COLS + 1]; strncpy(t, s + o, LV_TTY_COLS); t[LV_TTY_COLS] = 0; lv_tline(t); if (o) lv_tcont[(lv_tn - 1) % LV_TLINES] = 1; }
        if (!nl) break;
        s = nl + 1;
        if (!*s) break;
    }
}
static const char *lv_line_back(int k) /* k = 0 newest */
{
    if (k >= lv_tn || k >= LV_TLINES) return "";
    return lv_tl[(lv_tn - 1 - k) % LV_TLINES];
}

/* ---------- ticker ---------- */
#define LV_TKLEN 200
static char lv_tk_msg[LV_TKLEN] = "NO THREATS - TAP SCAN";
static int lv_tk_off = 0, lv_tk_item = 0;
static bool lv_tk_alert = false;
static char lv_evq[4][LV_TKLEN]; static uint8_t lv_evalert[4]; static int lv_nev = 0;
static void lv_event(bool alert, const char *fmt, ...)
{
    if (lv_nev >= 4) { memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); lv_nev = 3; }
    va_list ap; va_start(ap, fmt); vsnprintf(lv_evq[lv_nev], LV_TKLEN, fmt, ap); va_end(ap);
    lv_evalert[lv_nev++] = alert;
    if (!lv_tk_alert || alert) lv_tk_off = 1 << 14; /* cut over now unless an alert is mid-scroll */
}

/* ---------- classification (heuristic, from what the radio really reports) ---------- */
static bool lv_has(const char *h, const char *n)
{
    for (; *h; ++h) {
        const char *a = h, *b = n;
        while (*a && *b && ((*a | 32) == (*b | 32))) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}
static const char *lv_auth_name(int a)
{
    static const char *const N[] = {"OPEN", "WEP", "WPA", "WPA2", "WPA/2", "ENT", "WPA3", "WPA2/3", "WAPI", "OWE", "ENT3", "ENT3", "ENT"};
    return a >= 0 && a < (int)(sizeof N / sizeof N[0]) ? N[a] : "?";
}
static const char *lv_mfg_name(uint16_t m)
{
    switch (m) {
    case 0x004C: return "APPLE"; case 0x0075: return "SAMSUNG"; case 0x0006: return "MICROSOFT"; case 0x00E0: return "GOOGLE";
    case 0x01AB: return "META"; case 0x058E: return "META"; case 0x0087: return "GARMIN"; case 0x0157: return "HUAWEI";
    case 0x038F: return "XIAOMI"; case 0x02E5: return "ESPRESSIF"; case 0x0059: return "NORDIC"; case 0x000D: return "TI";
    case 0x0171: return "AMAZON"; case 0x00D2: return "DIALOG"; case 0x0499: return "RUUVI"; case 0x0822: return "ADAFRUIT";
    default: return 0;
    }
}
static void lv_classify(LvFind &f)
{
    const char *n = f.name;
    if (f.kind == LV_K_WIFI) {
        if (lv_has(n, "DJI") || lv_has(n, "TELLO") || lv_has(n, "SKYDIO") || lv_has(n, "ANAFI") || lv_has(n, "MAVIC")) f.icon = LV_IC_DRONE;
        else if (lv_has(n, "FLOCK")) f.icon = LV_IC_CCTV;
        else if (lv_has(n, "DIRECT-") && (lv_has(n, "HP") || lv_has(n, "EPSON") || lv_has(n, "BROTHER") || lv_has(n, "CANON"))) f.icon = LV_IC_PRINTER;
        else if (lv_has(n, "PRINT")) f.icon = LV_IC_PRINTER;
        else if (lv_has(n, "TESLA") || lv_has(n, "MYCAR") || lv_has(n, "UCONNECT") || lv_has(n, "SYNC ")) f.icon = LV_IC_CAR;
        else if (lv_has(n, "IPHONE") || lv_has(n, "GALAXY") || lv_has(n, "PIXEL") || lv_has(n, "ANDROID")) f.icon = LV_IC_PHONE;
        else if (lv_has(n, "TV") || lv_has(n, "ROKU") || lv_has(n, "BRAVIA")) f.icon = LV_IC_TV;
        else f.icon = LV_IC_AP;
        return;
    }
    if (f.svc == 0xFFFA) f.icon = LV_IC_DRONE;                                      /* ASTM Remote ID */
    else if (lv_has(n, "FLIPPER")) f.icon = LV_IC_FLIPPER;
    else if (lv_has(n, "PENGUIN") || lv_has(n, "FS EXT BATTERY") || lv_has(n, "FLOCK")) f.icon = LV_IC_CCTV;
    else if (lv_has(n, "HC-05") || lv_has(n, "HC-06") || lv_has(n, "HC-08") || lv_has(n, "RNBT")) f.icon = LV_IC_SKIMMER;
    else if (f.mfg == 0x01AB || f.mfg == 0x058E || lv_has(n, "RAY-BAN")) f.icon = LV_IC_GLASSES;
    else if (f.mfg == 0x004C && f.apple == 0x12) f.icon = LV_IC_AIRTAG;            /* Find My (offline finding) */
    else if (f.svc == 0xFD5A || f.svc == 0xFD59 || lv_has(n, "SMART TAG") || lv_has(n, "SMARTTAG")) f.icon = LV_IC_GALAXYTAG;
    else if (f.svc == 0xFEED || f.svc == 0xFEEC || lv_has(n, "TILE")) f.icon = LV_IC_KEYFOB;
    else if (lv_has(n, "WATCH") || lv_has(n, "BAND") || lv_has(n, "FITBIT") || lv_has(n, "GARMIN")) f.icon = LV_IC_WATCH;
    else if (lv_has(n, "BUDS") || lv_has(n, "AIRPODS") || lv_has(n, "JBL") || lv_has(n, "BOSE") || lv_has(n, "SPEAKER") || lv_has(n, "ECHO") || lv_has(n, "SONOS")) f.icon = LV_IC_SPEAKER;
    else if (lv_has(n, "[TV]") || lv_has(n, " TV") || lv_has(n, "ROKU") || lv_has(n, "BRAVIA") || lv_has(n, "FIRE TV")) f.icon = LV_IC_TV;
    else if (lv_has(n, "MACBOOK") || lv_has(n, "LAPTOP") || lv_has(n, "DESKTOP-") || lv_has(n, "THINKPAD")) f.icon = LV_IC_LAPTOP;
    else if (lv_has(n, "TESLA") || lv_has(n, "CAR")) f.icon = LV_IC_CAR;
    else if (lv_has(n, "PRINT")) f.icon = LV_IC_PRINTER;
    else if (f.mfg == 0x004C || f.mfg == 0x0075 || f.mfg == 0x00E0 || lv_has(n, "PHONE") || lv_has(n, "GALAXY") || lv_has(n, "PIXEL")) f.icon = LV_IC_PHONE;
    else if (!n[0] && f.mfg == 0xFFFF) f.icon = LV_IC_UNKNOWN;
    else f.icon = LV_IC_BLE;
}

/* hunts = the TERMINALbot device chips */
enum { LV_H_DEAUTH, LV_H_META, LV_H_HACK, LV_H_SKIMMER, LV_H_FLOCK, LV_H_FLIPPER, LV_H_DRONE, LV_H_AIRTAG, LV_H_KEYFOB, LV_H_SAMSUNG, LV_NHUNT };
static const char *const LV_HUNT_NAME[LV_NHUNT] = {"deauth", "meta", "hack", "skimmer", "flock", "flipper", "drone", "airtag", "keyfob", "samsung"};
static const char *const LV_HUNT_DESC[LV_NHUNT] = {
    "passive deauth/disassoc frame watch",
    "BLE: Meta glasses (company 0x01AB/0x058E)",
    "WiFi: OPEN or WEP access points",
    "BLE: HC-05/06/08, RNBT serial modules",
    "WiFi+BLE: Flock SSIDs, Penguin/FS Ext Battery",
    "BLE: Flipper Zero advert names",
    "WiFi+BLE: DJI/Tello/Skydio/Anafi, Remote ID",
    "BLE: Apple Find My adverts (0x004C type 0x12)",
    "BLE: Tile trackers (FEED/FEEC). No sub-GHz radio",
    "BLE: Samsung SmartTag (FD5A/FD59)"};
static const int LV_HUNT_JOB[LV_NHUNT] = {LV_J_DEAUTH, LV_J_BLE, LV_J_WIFI, LV_J_BLE, LV_J_ALL, LV_J_BLE, LV_J_ALL, LV_J_BLE, LV_J_BLE, LV_J_BLE};
static bool lv_hunt_match(int h, const LvFind &f)
{
    switch (h) {
    case LV_H_META: return f.icon == LV_IC_GLASSES;
    case LV_H_HACK: return f.kind == LV_K_WIFI && f.auth <= 1;
    case LV_H_SKIMMER: return f.icon == LV_IC_SKIMMER;
    case LV_H_FLOCK: return f.icon == LV_IC_CCTV;
    case LV_H_FLIPPER: return f.icon == LV_IC_FLIPPER;
    case LV_H_DRONE: return f.icon == LV_IC_DRONE;
    case LV_H_AIRTAG: return f.icon == LV_IC_AIRTAG;
    case LV_H_KEYFOB: return f.icon == LV_IC_KEYFOB;
    case LV_H_SAMSUNG: return f.icon == LV_IC_GALAXYTAG;
    }
    return false;
}

static void lv_macstr(const uint8_t *m, char *o) { snprintf(o, 18, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]); }
static const char *lv_label(const LvFind &f, char *o, int n)
{
    if (f.name[0]) { snprintf(o, n, "%s", f.name); return o; }
    if (f.kind == LV_K_WIFI) { snprintf(o, n, "<hidden>"); return o; }
    char m[18]; lv_macstr(f.mac, m); snprintf(o, n, "%s", m); return o;
}
static void lv_find_line(const LvFind &f, char *o, int n)
{
    char l[34];
    lv_label(f, l, sizeof l);
    if (f.kind == LV_K_WIFI) snprintf(o, n, "%sWIFI: %s CH%d %ddBm %s", f.alert ? "!! " : "", l, f.ch, f.rssi, lv_auth_name(f.auth));
    else {
        const char *mn = lv_mfg_name(f.mfg);
        snprintf(o, n, "%sBLE: %s %ddBm %s%s", f.alert ? "!! " : "", l, f.rssi, mn ? mn : "", (f.mfg == 0x004C && f.apple == 0x12) ? " FINDMY" : "");
    }
}

/* hunt target (item 0 A+D): the bot that hunts, what it hunts, its emoji; sticky after the job ends */
static const int LV_HUNT_ICON[LV_NHUNT] = {LV_IC_AP, LV_IC_GLASSES, LV_IC_LAPTOP, LV_IC_SKIMMER, LV_IC_CCTV, LV_IC_FLIPPER, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GALAXYTAG};
static const char *const LV_HUNT_BOT[LV_NHUNT] = {"DEAUTHbot", "METAbot", "HACKbot", "SKIMMERbot", "FLOCKbot", "FLIPPERbot", "DRONEbot", "AIRTAGbot", "KEYFOBbot", "SAMSUNGbot"};
static const char *const LV_HUNT_WHAT[LV_NHUNT] = {"DEAUTH FRAMES", "META GLASSES", "OPEN / WEP WIFI", "SERIAL SKIMMERS", "FLOCK CAMERAS", "FLIPPER ZERO", "DRONES", "FIND MY TAGS", "TILE TAGS", "SMARTTAGS"};
static int lv_hunt_last = -1;      /* the last hunt started from TERMINALbot / 'hunt' (cleared by a plain scan or 'clear') */
static void lv_upcase(char *p) { for (; *p; ++p) if (*p >= 'a' && *p <= 'z') *p -= 32; }
/* D: full-info line for one find, built only from fields LvFind really carries. k/m = match k of m (m 0 = not a hunt) */
static void lv_full_line(const LvFind &f, int k, int m, char *o, int n)
{
    char l[34], mac[18], tail[40] = ""; lv_label(f, l, sizeof l); lv_macstr(f.mac, mac);
    if (m && lv_hunt_last >= 0) { char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[lv_hunt_last]); lv_upcase(up); snprintf(tail, sizeof tail, "  %s MATCH %d/%d  %d SEEN", up, k, m, lv_nf); }
    const char *bang = f.alert ? "!! " : "";
    if (f.kind == LV_K_WIFI)
        snprintf(o, n, "%sWIFI: %s  CH%d  %ddBm  %s  BSSID %s  TYPE %s%s%s", bang, l, f.ch, f.rssi, lv_auth_name(f.auth), mac, LV_ICON_NAME[f.icon], tail, f.alert ? " !!" : "");
    else {
        const char *mn = lv_mfg_name(f.mfg); char mf[24];
        if (f.mfg == 0xFFFF) snprintf(mf, sizeof mf, "NO MFG DATA"); else snprintf(mf, sizeof mf, "%s %04X", mn ? mn : "MFG", f.mfg);
        char sv[16] = ""; if (f.svc) snprintf(sv, sizeof sv, "  SVC %04X", f.svc);
        snprintf(o, n, "%sBLE: %s  %ddBm  %s%s%s  MAC %s  TYPE %s%s%s", bang, l, f.rssi, mf, f.apple == 0x12 ? " FINDMY" : "", sv, mac, LV_ICON_NAME[f.icon], tail, f.alert ? " !!" : "");
    }
}

/* ---------- store results ---------- */
static void lv_sort(void)
{
    for (int i = 1; i < lv_nf; ++i) {
        LvFind t = lv_f[i]; int j = i - 1;
        while (j >= 0 && (lv_f[j].alert < t.alert || (lv_f[j].alert == t.alert && lv_f[j].rssi < t.rssi))) { lv_f[j + 1] = lv_f[j]; --j; }
        lv_f[j + 1] = t;
    }
}
static void lv_store(int kind, const LvFind *src, int n)
{
    int k = 0;
    for (int i = 0; i < lv_nf; ++i) if (lv_f[i].kind != kind) lv_f[k++] = lv_f[i];
    lv_nf = k;
    for (int i = 0; i < n && lv_nf < LV_MAXF; ++i) {
        LvFind f = src[i]; f.kind = (uint8_t)kind; f.name[32] = 0;
        lv_classify(f);
        f.alert = (lv_hunt >= 0 && lv_hunt_match(lv_hunt, f)) ? 1 : 0;
        lv_f[lv_nf++] = f;
    }
    lv_sort();
    lv_page = 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; lv_tk_item = 0;
}
static bool lv_in_mode(const LvFind &f, int mode)
{
    return mode == 0 || (mode == 1 && f.kind == LV_K_WIFI) || (mode == 2 && f.kind == LV_K_BLE);
}
static int lv_view(int *idx) /* finds shown in the SCANbot box for lv_mode */
{
    int n = 0;
    for (int i = 0; i < lv_nf; ++i) if (lv_in_mode(lv_f[i], lv_mode == 3 ? 0 : lv_mode)) idx[n++] = i;
    return n;
}

/* ---------- jobs ---------- */
/* GIBSON radar hunt (lv_radar.h, item 8) */
static bool lv_rd_is_trk(void);
static void lv_rd_track_done(void);
static bool lv_rd_hunt_tk(void);
struct LvBand; static bool lv_hunt_bar_on(void); static void lv_hunt_bar(LvBand &B);   /* issue 5 (MOCK): fixed hunt bar in the ticker */
static void lv_rd_hunt_open(int i, int from);
static int lv_rd_unignore(const char *a);
static const char *lv_job_name(int j) { return j == LV_J_WIFI ? "WIFI" : j == LV_J_BLE ? "BLE" : j == LV_J_ALL ? "WIFI+BLE" : j == LV_J_DEAUTH ? "DEAUTH WATCH" : "-"; }
static void lv_hunt_report(void);
static void lv_job_start(int job, int hunt)
{
    if (lv_job != LV_J_NONE) { lv_print("busy: %s scan running (stop)", lv_job_name(lv_job)); return; }
    lv_hunt = hunt; lv_hunt_last = hunt;
    lv_job = job; lv_job_phase = 0; lv_job_t0 = lv_now;
    int r = 0;
    if (job == LV_J_WIFI || job == LV_J_ALL) { lv_print("> scan wifi: radio on, scanning"); r = lv_radio->wifi_start(); }
    else if (job == LV_J_BLE) { lv_print("> scan ble: NimBLE on, 5 s active scan"); r = lv_radio->ble_start(5000); }
    else if (job == LV_J_DEAUTH) { lv_print("> deauth watch: passive, 10 s, ch hop"); r = lv_radio->deauth_start(10000); lv_deauth_have = false; }
    if (r != 0) { char h[64]; lv_radio->sys(0, h, sizeof h); lv_print("radio start failed (%d) %s", r, h); lv_job = LV_J_NONE; lv_event(true, "RADIO START FAILED (%d)", r); }
    lv_tk_off = 1 << 14; /* show SCANNING now */
    lv_dirty_box = true;
}
static void lv_job_stop(void)
{
    if (lv_job == LV_J_NONE) return;
    lv_radio->stop();
    lv_print("stopped %s, radios off", lv_job_name(lv_job));
    lv_job = LV_J_NONE; lv_hunt = -1; lv_dirty_box = true;
}
static void lv_job_tick(void)
{
    if (lv_job == LV_J_NONE) return;
    if (lv_job == LV_J_DEAUTH) {
        uint32_t d = 0, m = 0; uint8_t ch = 0;
        const int r = lv_radio->deauth_poll(&d, &m, &ch);
        if (d != lv_deauth_n || ch != lv_deauth_ch) lv_dirty_box = true;
        lv_deauth_n = d; lv_mgmt_n = m; lv_deauth_ch = ch; lv_deauth_have = true;
        if (r >= 0) {
            lv_job = LV_J_NONE;
            lv_print("deauth watch done: %lu deauth/disassoc in %lu mgmt frames", (unsigned long)d, (unsigned long)m);
            if (d) lv_event(true, "DEAUTH ATTACK? %lu DEAUTH/DISASSOC FRAMES SEEN", (unsigned long)d);
            else lv_event(false, "DEAUTH WATCH: NO DEAUTH FRAMES (%lu MGMT)", (unsigned long)m);
            lv_hunt = -1; lv_dirty_box = true;
        }
        return;
    }
    const bool wifi_phase = (lv_job == LV_J_WIFI) || (lv_job == LV_J_ALL && lv_job_phase == 0);
    const int r = wifi_phase ? lv_radio->wifi_poll(lv_tmp, LV_MAXF) : lv_radio->ble_poll(lv_tmp, LV_MAXF);
    if (r < 0) return;
    lv_store(wifi_phase ? LV_K_WIFI : LV_K_BLE, lv_tmp, r);
    if (lv_rd_is_trk()) { lv_job = LV_J_NONE; lv_rd_track_done(); return; }   /* radar hunt re-scan: no SCAN DONE event */
    lv_print("%s: %d found (%lu ms), radio off", wifi_phase ? "wifi" : "ble", r, (unsigned long)(lv_now - lv_job_t0));
    if (lv_job == LV_J_ALL && lv_job_phase == 0) {
        lv_job_phase = 1; lv_job_t0 = lv_now;
        lv_print("> scan ble: NimBLE on, 5 s active scan");
        if (lv_radio->ble_start(5000) != 0) { char h[64]; lv_radio->sys(0, h, sizeof h); lv_print("ble start failed %s", h); lv_job = LV_J_NONE; lv_event(true, "RADIO START FAILED (BLE)"); }
        lv_dirty_box = true;
        return;
    }
    lv_job = LV_J_NONE;
    int nw = 0, nb = 0;
    for (int i = 0; i < lv_nf; ++i) (lv_f[i].kind == LV_K_WIFI ? nw : nb)++;
    if (lv_hunt >= 0) lv_hunt_report();
    else lv_event(false, "SCAN DONE: %d WIFI  %d BLE", nw, nb);
    lv_hunt = -1;
    lv_dirty_box = true;
}
static void lv_hunt_report(void)
{
    int m = 0;
    for (int i = 0; i < lv_nf; ++i) if (lv_f[i].alert) ++m;
    char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[lv_hunt]);
    for (char *c = up; *c; ++c) if (*c >= 'a' && *c <= 'z') *c -= 32;
    lv_print("hunt %s: %d match / %d seen", LV_HUNT_NAME[lv_hunt], m, lv_nf);
    for (int i = 0, k = 0; i < lv_nf && k < 8; ++i)
        if (lv_f[i].alert) { char l[96]; lv_find_line(lv_f[i], l, sizeof l); lv_print(" %s", l + 3); ++k; }
    if (lv_hunt == LV_H_KEYFOB) lv_print("note: car keyfobs are sub-GHz; CYD has no sub-GHz radio. BLE trackers only.");
    if (m) { int k = 0; for (int i = 0; i < lv_nf && k < 3; ++i) if (lv_f[i].alert) { char t[LV_TKLEN]; lv_full_line(lv_f[i], ++k, m, t, sizeof t); lv_event(true, "%s", t); } }
    else lv_event(false, "NO %s FOUND (%d DEVICES SEEN)", up, lv_nf);
}

/* ---------- commands ---------- */
static void lv_go(int s);
static int lv_tty_from = LV_SC_TERM; /* where TTY returns to (BACKbot / back) */
static void lv_set_theme(int t)
{
    lv_theme = ((t % LV_NTHEME) + LV_NTHEME) % LV_NTHEME;
    if (lv_radio->save_theme) lv_radio->save_theme(lv_theme);
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = true;
}
/* item 4: one theme step per 250 ms (a bouncy double contact on the theme bar / CONTROLbot face can't skip a theme or hammer NVS) */
static uint32_t lv_theme_tap_t = 0; static bool lv_theme_tapped = false;
static bool lv_theme_bounce(void)
{
    if (lv_theme_tapped && (uint32_t)(lv_now - lv_theme_tap_t) < 250) return true;
    lv_theme_tapped = true; lv_theme_tap_t = lv_now; return false;
}
static void lv_sysinfo(void)
{
    char b[64];
    const uint32_t s = lv_now / 1000;
    lv_print("%s  up %02lu:%02lu:%02lu", LV_VERSION, (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60), (unsigned long)(s % 60));
    for (int w = 0; w < 5; ++w) { b[0] = 0; lv_radio->sys(w, b, sizeof b); if (b[0]) lv_print("%s", b); }
}
static void lv_list(int mode)
{
    int n = 0;
    for (int i = 0; i < lv_nf; ++i) {
        if (!lv_in_mode(lv_f[i], mode)) continue;
        char l[34]; lv_label(lv_f[i], l, 15);
        if (lv_f[i].kind == LV_K_WIFI) lv_print("%2d %-14s %4d c%-2d %s", i, l, lv_f[i].rssi, lv_f[i].ch, lv_auth_name(lv_f[i].auth));
        else lv_print("%2d %-14s %4d %s", i, l, lv_f[i].rssi, LV_ICON_NAME[lv_f[i].icon]);
        ++n;
    }
    if (!n) lv_print("no results yet: try 'scan'");
}
static void lv_info(int i)
{
    if (i < 0 || i >= lv_nf) { lv_print("info: no find #%d", i); return; }
    const LvFind &f = lv_f[i]; char m[18], l[34]; lv_macstr(f.mac, m); lv_label(f, l, sizeof l);
    lv_print("#%d %s [%s]", i, l, LV_ICON_NAME[f.icon]);
    if (f.kind == LV_K_WIFI) lv_print(" BSSID %s CH%d %ddBm %s", m, f.ch, f.rssi, lv_auth_name(f.auth));
    else {
        const char *mn = lv_mfg_name(f.mfg);
        lv_print(" ADDR %s %ddBm", m, f.rssi);
        if (f.mfg != 0xFFFF) lv_print(" MFG 0x%04X %s%s", f.mfg, mn ? mn : "", f.apple ? "" : "");
        if (f.mfg == 0x004C && f.apple) lv_print(" APPLE TYPE 0x%02X%s", f.apple, f.apple == 0x12 ? " FIND MY" : "");
        if (f.svc) lv_print(" SVC 0x%04X", f.svc);
    }
}
static const char *const LV_HELP[] = {
    "scan [all|wifi|ble]  wifi  ble  stop",
    "hunt [name|mac|n] radar hunt  stop",
    "sweep <dev>  deauth|sniff",
    "ignored  unignore [mac|all]",
    "list [wifi|ble]   info <n>",
    "filter <all|wifi|ble>",
    "go <home|scan|term|ctl|radar|boot|wifi|ble>",
    "theme [next|name|1-12]  history  clear",
    "status heap uptime version about bots",
    "help cmds  home  back|exit|close",
    "dev: airtag samsung flipper flock",
    "     drone meta skimmer hack keyfob",
};
static void lv_cmd(const char *line)
{
    char buf[64]; snprintf(buf, sizeof buf, "%s", line);
    char *a0 = strtok(buf, " "), *a1 = a0 ? strtok(0, " ") : 0;
    if (!a0) return;
    lv_print("%s %s", "$", line);
    if (lv_nhist < 8) snprintf(lv_hist[lv_nhist++], 64, "%s", line);
    else { memmove(lv_hist[0], lv_hist[1], 64 * 7); snprintf(lv_hist[7], 64, "%s", line); }
    for (char *c = a0; *c; ++c) if (*c >= 'A' && *c <= 'Z') *c += 32;
    if (a1) for (char *c = a1; *c; ++c) if (*c >= 'A' && *c <= 'Z') *c += 32;
#define IS(s) (!strcmp(a0, s))
    if (IS("help") || IS("cmds") || IS("?")) { for (const char *h : LV_HELP) lv_print("%s", h); }
    else if (IS("scan")) lv_job_start(!a1 || !strcmp(a1, "all") ? LV_J_ALL : !strcmp(a1, "wifi") ? LV_J_WIFI : !strcmp(a1, "ble") || !strcmp(a1, "bt") ? LV_J_BLE : LV_J_ALL, -1);
    else if (IS("wifi")) lv_job_start(LV_J_WIFI, -1);
    else if (IS("ble") || IS("bt")) lv_job_start(LV_J_BLE, -1);
    else if (IS("deauth") || IS("sniff")) lv_job_start(LV_J_DEAUTH, LV_H_DEAUTH);
    else if (IS("stop")) lv_job_stop();
    else if (IS("hunt")) {   /* item 10: opens the GIBSON radar hunt (was: category sweep, now 'sweep <dev>') */
        if (!a1) { lv_print("hunt: radar - tap empty = next, hold a tower 1 s = lock"); lv_go(LV_SC_RADAR); }
        else {
            const char *q = line; while (*q == ' ') ++q; while (*q && *q != ' ') ++q; while (*q == ' ') ++q;   /* raw arg, keeps ':' */
            int best = -1;
            const bool num = a1[0] >= '0' && a1[0] <= '9' && !strchr(a1, ':') && strlen(a1) <= 2;
            if (num) { const int n = atoi(a1); if (n >= 0 && n < lv_nf) best = n; }   /* 'list' row number */
            for (int i = 0; best < 0 && i < lv_nf; ++i) {   /* lv_f is strongest-first, so the first match is the best */
                char m[18], m2[13]; lv_macstr(lv_f[i].mac, m);
                snprintf(m2, sizeof m2, "%02x%02x%02x%02x%02x%02x", lv_f[i].mac[0], lv_f[i].mac[1], lv_f[i].mac[2], lv_f[i].mac[3], lv_f[i].mac[4], lv_f[i].mac[5]);
                char l[34]; lv_label(lv_f[i], l, sizeof l);
                if (lv_has(lv_f[i].name, q) || lv_has(l, q) || lv_has(m, q) || lv_has(m2, q) || lv_has(LV_ICON_NAME[lv_f[i].icon], q)) best = i;
            }
            if (best < 0) {
                lv_print("hunt %s: NO MATCH in %d tracked", q, lv_nf);
                lv_go(LV_SC_RADAR);
                lv_event(false, "HUNT %s: NO MATCH (%d TRACKED)", q, lv_nf); lv_tk_off = 1 << 14;
            } else {
                char l[34]; lv_label(lv_f[best], l, sizeof l); lv_print("hunt #%d %s %ddBm", best, l, lv_f[best].rssi);
                lv_rd_hunt_open(best, LV_SC_TTY);
            }
        }
    }
    else if (IS("sweep")) {   /* the old 'hunt <dev>' category sweep (TERMINALbot tiles use it) */
        int h = -1;
        for (int k = 0; a1 && k < LV_NHUNT; ++k) if (!strcmp(a1, LV_HUNT_NAME[k]) || (!strcmp(a1, "air") && k == LV_H_AIRTAG)) h = k;
        if (h < 0) { lv_print("sweep what? airtag samsung flipper flock"); lv_print("  drone meta skimmer hack keyfob deauth"); }
        else { lv_print("sweep %s: %s", LV_HUNT_NAME[h], LV_HUNT_DESC[h]); lv_job_start(LV_HUNT_JOB[h], h); }
    }
    else if (IS("unignore") || IS("ignored")) { const int n = lv_rd_unignore(IS("ignored") ? "?" : a1); if (n >= 0 && !IS("ignored")) lv_print("unignored %d", n); }
    else if (IS("list") || IS("ls")) lv_list(!a1 ? 0 : !strcmp(a1, "wifi") ? 1 : 2);
    else if (IS("info")) { if (a1) lv_info(atoi(a1)); else lv_sysinfo(); }
    else if (IS("filter")) { lv_mode = !a1 ? 0 : !strcmp(a1, "wifi") ? 1 : (!strcmp(a1, "ble") || !strcmp(a1, "bt")) ? 2 : 0; lv_page = 0; lv_sel = -1; lv_dirty_box = true; lv_print("SCANbot view: %s", lv_mode == 1 ? "WIFI" : lv_mode == 2 ? "BT" : "ALL"); }
    else if (IS("clear") || IS("cls")) { lv_hunt_last = -1; lv_tn = 0; lv_scroll = 0; lv_dirty_term = lv_dirty_tty = true; }
    else if (IS("history")) { for (int i = 0; i < lv_nhist; ++i) lv_print("%d %s", i + 1, lv_hist[i]); }
    else if (IS("theme")) {
        int t = -1;
        for (int k = 0; a1 && k < LV_NTHEME; ++k)   /* short name, or the original name without its space (hotpink, goldrush, ...) */
            for (const char *nm : {LV_THEME_NAME[k], LV_THEME_FULL[k]}) {
                char lo[16]; int n = 0; for (const char *c = nm; *c && n < 15; ++c) if (*c != ' ') lo[n++] = (char)(*c >= 'A' && *c <= 'Z' ? *c + 32 : *c); lo[n] = 0;
                if (!strcmp(a1, lo)) t = k;
            }
        if (a1 && t < 0 && atoi(a1) >= 1 && atoi(a1) <= LV_NTHEME) t = atoi(a1) - 1;
        lv_set_theme(t >= 0 ? t : lv_theme + 1);
        lv_print("theme %d/%d %s", lv_theme + 1, LV_NTHEME, LV_THEME_FULL[lv_theme]);
    }
    else if (IS("status") || IS("sys") || IS("heap")) lv_sysinfo();
    else if (IS("uptime")) { const uint32_t s = lv_now / 1000; lv_print("up %02lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60), (unsigned long)(s % 60)); }
    else if (IS("version")) lv_print("%s (env cyd_livescan)", LV_VERSION);
    else if (IS("about")) { lv_print("BOTCORPS CYD live scan. WiFi + BLE"); lv_print("scans are passive/defensive, one radio"); lv_print("at a time. Games removed."); }
    else if (IS("bots")) { lv_print("GIBSONbot home   SCANbot scan"); lv_print("TERMINALbot term CONTROLbot ctl"); lv_print("BOOTbot boot (self test)"); }
    else if (IS("go")) {
        if (!a1) lv_print("go where? home scan term ctl boot wifi ble");
        else if (!strcmp(a1, "home")) lv_go(LV_SC_HOME);
        else if (!strcmp(a1, "scan")) lv_go(LV_SC_SCAN);
        else if (!strcmp(a1, "term")) lv_go(LV_SC_TERM);
        else if (!strcmp(a1, "ctl")) lv_go(LV_SC_CTL);
        else if (!strcmp(a1, "radar") || !strcmp(a1, "gibson")) lv_go(LV_SC_RADAR);
        else if (!strcmp(a1, "boot")) { lv_selftest_pending = true; lv_go(LV_SC_BOOT); }
        else if (!strcmp(a1, "wifi")) lv_job_start(LV_J_WIFI, -1);
        else if (!strcmp(a1, "ble") || !strcmp(a1, "bt")) lv_job_start(LV_J_BLE, -1);
        else lv_print("go: unknown '%s'", a1);
    }
    else if (IS("home") || IS("back") || IS("exit") || IS("close") || IS("quit")) lv_go(IS("home") ? LV_SC_HOME : lv_tty_from);
    else if (IS("rain") || IS("ping") || IS("track") || IS("load")) lv_print("%s: not in the livescan build", a0);
    else lv_print("unknown: %s (try help)", a0);
#undef IS
}

/* ---------- clip with masks + live tints ----------
 * masks: rectangles the clip never paints (overlays own them).
 * tints: rectangles of LOCKED clip art that show live state (item 1 chip colours, item 2 press flash, CTL face hue,
 * BACKbot). The clip's palette indices for every tint pixel are kept in lv_shadow, so a tint can be repainted from
 * the shadow when its state changes, without waiting for the clip. RAW state = byte-identical to the clip. */
static lk_draw_fn lv_raw_draw;
static LvRect lv_masks[16];
static int lv_nmask = 0;
#define LV_SHADOW_CAP 32768
#define LV_MAXT 24
enum { LV_TM_RAW = 0, LV_TM_LIT, LV_TM_COLOR, LV_TM_DIM, LV_TM_HUE, LV_TM_GREY };
/* item 7: BOOTbot is black + grey, never white. Exactly two greys: pure white -> L, other near-white -> M. */
#define LV_BB_GREY_L 0xC618   /* (198,195,198) */
#define LV_BB_GREY_M 0x9CF3   /* (156,158,156) */
struct LvTS { uint8_t mode, amt; uint32_t col; uint8_t deco; };   /* deco != 0: the tint's own painter draws over the shadow */
static bool lv_ts_eq(const LvTS &a, const LvTS &b) { return a.mode == b.mode && a.amt == b.amt && a.col == b.col && a.deco == b.deco; }
struct LvTint { LvRect r; int off; uint8_t id; };
static uint8_t *lv_shadow = nullptr;   /* heap, allocated once on first tint: 32 KB static did not fit dram0_0_seg */
static LvTint lv_tints[LV_MAXT];
static int lv_ntint = 0, lv_shadow_used = 0, lv_shadow_peak = 0;
static LvTS lv_tdrawn[LV_MAXT];
static bool lv_tdirty[LV_MAXT];   /* the clip wrote under a deco tint: repaint it (keeps the art animating under BACKbot) */
static LvTS lv_tint_state(int id);
static void lv_tint_deco(LvBand &B, int id, const LvTS &s);
static const uint16_t *lv_pal(void) { return LK_CLIP[lk_clip_of_seg(lv_play.seg)].pal; }
static void lv_tint_add(int id, LvRect r)
{
    const int n = (r.x1 - r.x0 + 1) * (r.y1 - r.y0 + 1);
    if (!lv_shadow) lv_shadow = (uint8_t *)malloc(LV_SHADOW_CAP);
    if (!lv_shadow || lv_ntint >= LV_MAXT || lv_shadow_used + n > LV_SHADOW_CAP) return;   /* no buffer: art stays plain, nothing breaks */
    lv_tints[lv_ntint] = {r, lv_shadow_used, (uint8_t)id}; lv_tdrawn[lv_ntint].mode = 255; lv_tdirty[lv_ntint] = false;
    ++lv_ntint; lv_shadow_used += n; if (lv_shadow_used > lv_shadow_peak) lv_shadow_peak = lv_shadow_used;
}
static uint16_t lv_tx(const LvTS &s, uint16_t c, int px = 0, int py = 0)
{
    const int R = (c >> 11) << 3, G = ((c >> 5) & 63) << 2, Bl = (c & 31) << 3;
    switch (s.mode) {
    case LV_TM_LIT: return lv_boost(c, 8 + s.amt, 8);
    case LV_TM_DIM: return lv_dim(c, 8 - s.amt, 8);
    case LV_TM_COLOR: {                                  /* colourize toward the radio colour, keep the shading */
        const int L = (R * 3 + G * 6 + Bl) / 10;
        int cr = (int)((s.col >> 16) & 255) * L / 150, cg = (int)((s.col >> 8) & 255) * L / 150, cb = (int)(s.col & 255) * L / 150;
        if (cr > 255) cr = 255; if (cg > 255) cg = 255; if (cb > 255) cb = 255;
        return lv_mix(c, lv_rgb((uint8_t)cr, (uint8_t)cg, (uint8_t)cb), s.amt, 8);
    }
    case LV_TM_GREY:
        if ((c >> 11) >= 28 && ((c >> 5) & 63) >= 56 && (c & 31) >= 28) return c == 0xFFFF ? LV_BB_GREY_L : LV_BB_GREY_M;
        return c;
    case LV_TM_HUE: {                                    /* re-ink the purple (GRAPE) pixels in the theme colour */
        const int mx = R > Bl ? R : Bl;
        if (mx - G < 60 || Bl < 80) return c;
        /* CONTROLbot's face is the theme preview: the fades (7, 8) run their whole range across its 66 px height */
        const int fy = (lv_theme == 7 || lv_theme == 8) ? (py - 95) * 319 / 65 : py;
        const uint32_t col = lv_multi_theme(lv_theme) ? lv_theme_at(lv_theme, px, fy < 0 ? 0 : fy > 319 ? 319 : fy) : s.col;
        int cr = (int)((col >> 16) & 255) * mx / 255, cg = (int)((col >> 8) & 255) * mx / 255, cb = (int)(col & 255) * mx / 255;
        return lv_rgb((uint8_t)cr, (uint8_t)cg, (uint8_t)cb);
    }
    }
    return c;
}
/* Issue 2: themes 6..11 play the ICE clip; its cyan accent pixels (R well under G and B, G ~ B) take the theme ink at
 * their screen position, keeping the clip's shading (max(G,B)) and its white lift (R). Everything else - black,
 * greys, silver TERMINALbot, gold SCANbot, pink BACKbot, the green / red status art - is not cyan and stays. */
static bool lv_reink_on(void) { return lv_theme >= LV_THEME_ORIG && (lv_scr == LV_SC_HOME || lv_scr == LV_SC_TERM || lv_scr == LV_SC_SCAN); }
static uint16_t lv_multi_ink(uint16_t c, int x, int y)
{
    const int R = (c >> 11) << 3, G = ((c >> 5) & 63) << 2, Bl = (c & 31) << 3;
    const int mx = G > Bl ? G : Bl, mn = G < Bl ? G : Bl;
    if (mx < 12 || R * 10 > mn * 7 || Bl * 10 < G * 6 || G * 10 < Bl * 5) return c;
    const uint32_t T = lv_theme_at(lv_theme, x, y);
    const int W = R * 255 / mx;   /* 0 = pure accent, 255 = white highlight */
    int o[3];
    for (int k = 0; k < 3; ++k) { const int tc = (int)((T >> (16 - 8 * k)) & 255); o[k] = (tc * (255 - W) + 255 * W) / 255 * mx / 255; }
    return lv_rgb((uint8_t)o[0], (uint8_t)o[1], (uint8_t)o[2]);
}
static uint16_t lv_pc(const uint16_t *pal, uint8_t i, int x, int y) { return lv_reink_on() ? lv_multi_ink(pal[i], x, y) : pal[i]; }
static void lv_scene_tint(LvBand &B, void *ctx)
{
    const int t = *(const int *)ctx;
    const LvTint &T = lv_tints[t];
    const LvTS s = lv_tint_state(T.id);
    const uint16_t *pal = lv_pal();
    const int w = T.r.x1 - T.r.x0 + 1;
    for (int y = B.y0; y < B.y0 + B.h; ++y)
        for (int x = B.x0; x < B.x0 + B.w; ++x) B.px[(y - B.y0) * B.w + (x - B.x0)] = lv_tx(s, lv_pc(pal, lv_shadow[T.off + (y - T.r.y0) * w + (x - T.r.x0)], x, y), x, y);
    if (s.deco) lv_tint_deco(B, T.id, s);
}
/* Issue 1 (theme-tap flicker): a tint rect may overlap a mask (the CTL FLASH tints contain the value badges, the
 * theme-bar tint contains THEME / UPTIME / FOCUS). lv_clip_draw never writes the shadow under a mask, so painting the
 * whole tint rect put black / stale shadow pixels over the overlay, and the overlay repainted a moment later: a black
 * blink on every badge after each lv_go(). Tints now skip masked pixels, so the overlays are never overdrawn. */
static bool lv_rect_hit(const LvRect &a, const LvRect &b) { return a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1; }
static void lv_tint_paint(int t)
{
    lv_tdrawn[t] = lv_tint_state(lv_tints[t].id); lv_tdirty[t] = false;
    const LvRect r = lv_tints[t].r;
    int ml[16], nm = 0;
    for (int m = 0; m < lv_nmask; ++m) if (lv_rect_hit(lv_masks[m], r)) ml[nm++] = m;
    if (!nm) { lv_render(lv_blit, r, lv_scene_tint, &t); return; }
    const int w = r.x1 - r.x0 + 1;
    int rows = LV_BAND_PX / w; if (rows > r.y1 - r.y0 + 1) rows = r.y1 - r.y0 + 1;
    for (int y = r.y0; y <= r.y1; y += rows) {
        const int bh = (y + rows - 1 <= r.y1) ? rows : r.y1 - y + 1;
        LvBand B{lv_bandbuf, r.x0, y, w, bh};
        lv_scene_tint(B, &t);
        if (lv_flash_r.x1 >= lv_flash_r.x0) lv_flash_band(B);
        for (int yy = y; yy < y + bh; ++yy) {
            auto masked = [&](int x) { for (int k = 0; k < nm; ++k) if (lv_in(lv_masks[ml[k]], x, yy)) return true; return false; };
            for (int x = r.x0; x <= r.x1;) {
                if (masked(x)) { ++x; continue; }
                int e = x; while (e + 1 <= r.x1 && !masked(e + 1)) ++e;
                lv_blit(x, yy, e - x + 1, 1, lv_bandbuf + (yy - y) * w + (x - r.x0));
                x = e + 1;
            }
        }
    }
}
static void lv_clip_draw(int y, int x, int n, const uint8_t *idx)
{
    int ml[16], nm = 0, tl[LV_MAXT], nt = 0;
    for (int m = 0; m < lv_nmask; ++m) if (y >= lv_masks[m].y0 && y <= lv_masks[m].y1 && lv_masks[m].x1 >= x && lv_masks[m].x0 < x + n) ml[nm++] = m;
    for (int t = 0; t < lv_ntint; ++t) if (y >= lv_tints[t].r.y0 && y <= lv_tints[t].r.y1 && lv_tints[t].r.x1 >= x && lv_tints[t].r.x0 < x + n) tl[nt++] = t;
    const uint16_t *pal = lv_pal();
    static uint16_t lb[240];
    const bool ri = lv_reink_on();
    auto raw = [&](int x0, int m, const uint8_t *ix) {   /* re-inked themes push RGB565, the rest the palette indices */
        if (!ri) { lv_raw_draw(y, x0, m, ix); return; }
        for (int k = 0; k < m; ++k) lb[k] = lv_multi_ink(pal[ix[k]], x0 + k, y);
        lv_blit(x0, y, m, 1, lb);
    };
    if (!nm && !nt) { raw(x, n, idx); return; }
    const bool fl = lv_flash_r.x1 >= lv_flash_r.x0;
    auto cls = [&](int i) -> int {                      /* -2 masked, -1 raw clip, else tint index */
        const int px = x + i;
        for (int k = 0; k < nm; ++k) if (px >= lv_masks[ml[k]].x0 && px <= lv_masks[ml[k]].x1) return -2;
        for (int k = 0; k < nt; ++k) if (px >= lv_tints[tl[k]].r.x0 && px <= lv_tints[tl[k]].r.x1) return tl[k];
        return -1;
    };
    for (int i = 0; i < n;) {
        const int c = cls(i); int j = i + 1;
        while (j < n && cls(j) == c) ++j;
        if (c == -1) raw(x + i, j - i, idx + i);
        else if (c >= 0) {
            const LvTint &T = lv_tints[c]; const int w = T.r.x1 - T.r.x0 + 1;
            uint8_t *sh = lv_shadow + T.off + (y - T.r.y0) * w + (x + i - T.r.x0);
            for (int k = i; k < j; ++k) *sh++ = idx[k];
            const LvTS s = lv_tint_state(T.id);
            if (s.deco) lv_tdirty[c] = true;          /* painted by its own scene (shadow + decorator) */
            else {
                for (int k = i; k < j; ++k) { uint16_t v = lv_tx(s, ri ? lv_multi_ink(pal[idx[k]], x + k, y) : pal[idx[k]], x + k, y); if (fl && lv_in(lv_flash_r, x + k, y)) v = lv_hi(v); lb[k - i] = v; }
                lv_blit(x + i, y, j - i, 1, lb);
            }
        }
        i = j;
    }
}

/* ---------- overlay scenes ---------- */
static uint16_t lv_ink(const uint32_t *t) { return lv_hex(t[lv_scr == LV_SC_CTL ? 0 : lv_theme]); }

/* CONTROLbot settings (item 5): every chip on CONTROLbot drives one of these */
static int lv_scanmix = 0;        /* MIX: what SCANbot's chip / crosshair scan: 0 WIFI+BLE, 1 WIFI, 2 BLE */
#if LV_BG_ENGINE
static int lv_rain = 1;       /* RAIN: GIBSON radar background layer on / off */
static int lv_bgstyle = 0;    /* BG STYLE (background engine, picked on the RAIN chip): 0 RAIN, 1 BOTS, 2 GIBSON, 3 FLYBY, 4 BOUNCE */
static int lv_raincol = 0;    /* BG COLOR (RAIN COL chip): 0 GREEN, 1 CYAN, 2 THEME, 3 MULTI, 4 PURPLE, 5 AMBER */
#else
static int lv_rain = 1;       /* RAIN: GIBSON radar code rain on / off */
static int lv_raincol = 0;    /* RAIN COL: 0 GREEN, 1 CYAN, 2 THEME */
#endif
static int lv_rainmode = 0;   /* RAIN MODE: radar camera 0 DRIFT, 1 SWEEP, 2 STILL */
static int lv_speed = 1;      /* MASHUP: ticker speed 0 SLOW, 1 MED, 2 FAST */
static int lv_auto = 0;       /* MULTI: auto re-scan 0 OFF, 1 30S, 2 60S */
static int lv_flip = 0;       /* ORIENT: 0 PORT (USB bottom, setRotation 2), 1 FLIP (rot 0) */
static uint32_t lv_auto_t = 0;
static const char *const LV_MIX_NAME[3] = {"ALL", "WIFI", "BLE"};
static const char *const LV_ONOFF[2] = {"OFF", "ON"};
#if LV_BG_ENGINE
#define LV_NBGCOL 6
#define LV_NBGSTYLE 5
static const char *const LV_RAINCOL_NAME[LV_NBGCOL] = {"GREEN", "CYAN", "THEME", "MULTI", "PURPLE", "AMBER"};
static const char *const LV_BGSTYLE_NAME[LV_NBGSTYLE] = {"RAIN", "BOTS", "GIBSON", "FLYBY", "BOUNCE"};
static const char *lv_bgstyle_name(void) { return lv_rain ? LV_BGSTYLE_NAME[lv_bgstyle] : "OFF"; }
static const char *lv_bgstyle_badge(void) { return lv_rain && lv_bgstyle == 0 ? "ON" : lv_bgstyle_name(); }   /* RAIN badge: the default keeps its old "ON" (0 px change) */
static void lv_bg_save(void) { if (lv_radio && lv_radio->bg_save) lv_radio->bg_save(lv_rain ? lv_bgstyle : LV_NBGSTYLE, lv_raincol); }
#else
static const char *const LV_RAINCOL_NAME[3] = {"GREEN", "CYAN", "THEME"};
#endif
static const char *const LV_RAINMODE_NAME[3] = {"DRIFT", "SWEEP", "STILL"};
static const char *const LV_SPEED_NAME[3] = {"SLOW", "MED", "FAST"};
static const char *const LV_AUTO_NAME[3] = {"OFF", "30S", "60S"};
static const char *const LV_FLIP_NAME[2] = {"PORT", "FLIP"};

/* ticker: stripes + scrolling bold text over the whole top bar */
struct LvTickCtx { int span0, span1; };
static void lv_scene_ticker(LvBand &B, void *vctx)
{
    LvTickCtx *c = (LvTickCtx *)vctx;
    if (lv_hunt_bar_on()) { lv_hunt_bar(B); return; }   /* hunting: the banner stops scrolling and becomes the hunt bar */
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    const uint16_t bg = lv_tk_alert ? (flash ? lv_hex(0x6a0000) : lv_hex(0x3a0000)) : lv_hex(0x2a0400);
    b_fill(B, B.x0, B.y0, B.w, B.h, bg);
    const int ph = (int)(lv_now / 40);
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) {
            const bool red = (((x + (y <= 3 ? y : -y) + ph) / 5) & 1);
            b_pix(B, x, y, red ? lv_hex(flash ? 0xff2020 : 0xe00000) : lv_hex(0xffb000));
        }
    }
    const int tx = c->span1 + 1 - lv_tk_off;
    const uint16_t ink = lv_tk_alert ? lv_hex(0xffffff) : lv_hex(0xffc830);
    b_text(B, tx + 1, 6, lv_tk_msg, lv_hex(0x200000), 2);
    b_text(B, tx, 5, lv_tk_msg, ink, 2);
}
static int lv_matches(void) { int m = 0; for (int i = 0; i < lv_nf; ++i) if (lv_f[i].alert) ++m; return m; }
static void lv_tk_next(void)
{
    lv_tk_off = 0;
    if (lv_rd_hunt_tk()) return;   /* radar HUNT: the target's live line stays on the ticker */
    if (lv_job != LV_J_NONE) {
        lv_tk_alert = false;
        if (lv_hunt >= 0) {       /* D: name the hunter + the target first, then what it is doing */
            char d[64]; snprintf(d, sizeof d, "%s", LV_HUNT_DESC[lv_hunt]); lv_upcase(d);
            snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s HUNTS %s  SCANNING %s...  %s", LV_HUNT_BOT[lv_hunt], LV_HUNT_WHAT[lv_hunt],
                     lv_job == LV_J_DEAUTH ? "CH HOP" : (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)), d);
            if (lv_job == LV_J_DEAUTH) { char t[48]; snprintf(t, sizeof t, "  CH%d %lu DEAUTH %lu MGMT", lv_deauth_ch, (unsigned long)lv_deauth_n, (unsigned long)lv_mgmt_n); strncat(lv_tk_msg, t, sizeof lv_tk_msg - strlen(lv_tk_msg) - 1); }
            lv_tk_off = 196;      /* start near the left edge so the target reads at once */
            return;
        }
        snprintf(lv_tk_msg, sizeof lv_tk_msg, "SCANNING %s...", (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)));
        return;
    }
    if (lv_nev) {
        snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", lv_evq[0]); lv_tk_alert = lv_evalert[0];
        memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); --lv_nev;
        return;
    }
    int idx[LV_MAXF], n = 0;
    const int m = lv_hunt_last >= 0 ? lv_matches() : 0;
    for (int i = 0; i < lv_nf; ++i) {
        const LvFind &f = lv_f[i];
        if (m ? f.alert : (lv_focus == 3 ? f.alert : lv_in_mode(f, lv_focus))) idx[n++] = i;
    }
    if (!n) { lv_tk_alert = false; snprintf(lv_tk_msg, sizeof lv_tk_msg, lv_nf ? "NO %s FINDS" : "NO THREATS - TAP SCAN", LV_FOCUS_NAME[lv_focus]); return; }
    if (lv_tk_item >= n) lv_tk_item = 0;
    const LvFind &f = lv_f[idx[lv_tk_item]];
    lv_full_line(f, lv_tk_item + 1, m, lv_tk_msg, sizeof lv_tk_msg);   /* D: each find's full info */
    lv_tk_alert = f.alert;
    ++lv_tk_item;
}
static void lv_ticker_tick(void)
{
    const bool tty = lv_scr == LV_SC_TTY;
    const LvRect r = tty ? LV_R_TTY_TICK : LV_R_TICK;
    const int w = (int)strlen(lv_tk_msg) * 12;
    if (!lv_hunt_bar_on()) lv_tk_off += lv_speed + 1;
    if (lv_tk_off > (r.x1 - r.x0 + 1) + w) lv_tk_next();
    LvTickCtx c{r.x0, r.x1};
    lv_render(lv_blit, r, lv_scene_ticker, &c);
}

/* ---------- SCANbot: device-emoji category grid + sublists (item 13) ----------
 * The four tabs under SCANbot (ALL / WIFI / BT / DEAUTH) are gone; their room and the old results box are one
 * grid of emoji categories. Tap a category -> its sublist (the Gibson-crew drop-down accordion rebuilt as a
 * paged list); tap a row -> ticker shows its full info; tap it again -> detail. BACKbot steps back out. */
#define LV_CAT_GRID -1
#define LV_CAT_DEAUTH 99
static int lv_cat = LV_CAT_GRID, lv_cpage = 0;
static const LvRect LV_R_BTN_SCAN = {174, 165, 203, 206}, LV_R_BTN_MORE = {174, 210, 203, 252};
static int lv_tile_x(int i) { return 38 + 34 * (i & 3); }
static int lv_tile_y(int i) { return 165 + 30 * (i >> 2); }
struct LvCatInfo { int icon, n, best; bool alert, wifi, ble; };
static int lv_cats(LvCatInfo *c) /* categories in the current view, alerts first, then count, then best signal */
{
    int idx[LV_MAXF]; const int n = lv_view(idx); int k = 0;
    for (int i = 0; i < n; ++i) {
        const LvFind &f = lv_f[idx[i]]; int j = 0;
        while (j < k && c[j].icon != f.icon) ++j;
        if (j == k) { c[k] = {f.icon, 0, -127, false, false, false}; ++k; }
        ++c[j].n; if (f.rssi > c[j].best) c[j].best = f.rssi; if (f.alert) c[j].alert = true;
        if (f.kind == LV_K_WIFI) c[j].wifi = true; else c[j].ble = true;
    }
    for (int a = 1; a < k; ++a) { LvCatInfo t = c[a]; int b = a - 1;
        while (b >= 0 && (c[b].alert < t.alert || (c[b].alert == t.alert && (c[b].n < t.n || (c[b].n == t.n && c[b].best < t.best))))) { c[b + 1] = c[b]; --b; }
        c[b + 1] = t; }
    return k;
}
static int lv_cat_rows(int *idx) /* finds of the open category, view order */
{
    int all[LV_MAXF]; const int n = lv_view(all); int k = 0;
    for (int i = 0; i < n; ++i) if (lv_f[all[i]].icon == lv_cat) idx[k++] = all[i];
    return k;
}
static void lv_button(LvBand &B, const LvRect &r, const char *t, bool on)
{
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    b_fill(B, r.x0, r.y0, w, h, lv_hex(0xffffff));
    b_fill(B, r.x0 + 1, r.y0 + 1, w - 2, h - 2, on ? lv_ink(LV_ACC) : lv_hex(0x303030));
    b_text35(B, r.x0 + (w - lv_w35(t)) / 2, r.y0 + (h - 5) / 2, t, on ? lv_hex(0x000000) : lv_hex(0x808080));
}
static const char *lv_job_now(void) { return (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)); }
static void lv_scene_box(LvBand &B, void *)
{
    const uint16_t hdr = lv_ink(LV_HDR), tan = lv_hex(LV_TAN);
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_frame(B, 34, 154, 172, 102, tan);
    b_fill(B, 35, 155, 170, 9, lv_hex(0x181818));
    LvCatInfo cats[LV_IC_N]; const int nc = lv_cats(cats);
    int idx[LV_MAXF]; const int nv = lv_view(idx);
    const bool busy = lv_job != LV_J_NONE;
    char l[40], r[24];
    /* ---- deauth view (was the DEAUTH tab) ---- */
    if (lv_cat == LV_CAT_DEAUTH) {
        b_text35(B, 38, 157, "DEAUTH WATCH", hdr);
        snprintf(r, sizeof r, "CH %d", lv_deauth_ch); b_text35(B, 203 - lv_w35(r), 157, r, hdr);
        lv_button(B, LV_R_BTN_SCAN, lv_job == LV_J_DEAUTH ? "STOP" : "WATCH", true);
        lv_button(B, LV_R_BTN_MORE, "BACK", true);
        char t[40];
        b_text(B, 39, 168, "PASSIVE DEAUTH WATCH", lv_hex(0xffffff));
        snprintf(t, sizeof t, "%s", lv_job == LV_J_DEAUTH ? "LISTENING, CH HOP" : (lv_deauth_have ? "DONE" : "TAP WATCH"));
        b_text(B, 39, 180, t, tan);
        snprintf(t, sizeof t, "DEAUTH %lu", (unsigned long)lv_deauth_n);
        b_text(B, 39, 194, t, lv_deauth_n ? lv_hex(0xff3030) : lv_hex(0xffffff), 2);
        snprintf(t, sizeof t, "MGMT %lu", (unsigned long)lv_mgmt_n);
        b_text(B, 39, 216, t, lv_hex(0xffffff));
        b_text35(B, 39, 230, "RX ONLY. NO TRANSMIT.", hdr);
        b_text35(B, 39, 240, "BACKBOT = CATEGORIES", lv_dim(hdr, 2, 3));
        return;
    }
    /* ---- sublist of one emoji category ---- */
    if (lv_cat >= 0) {
        int rows[LV_MAXF]; const int nr = lv_cat_rows(rows);
        const int pages = nr ? (nr + 7) / 8 : 1; if (lv_cpage >= pages) lv_cpage = 0;
        int al = 0; for (int i = 0; i < nr; ++i) if (lv_f[rows[i]].alert) ++al;
        b_fill(B, 35, 155, 170, 19, lv_hex(0x181818));
        b_frame(B, 36, 155, 18, 18, al ? lv_hex(0xff2020) : tan);
        b_icon(B, 37, 156, lv_cat);
        snprintf(l, sizeof l, "%s %d", LV_ICON_NAME[lv_cat], nr); lv_upcase(l);
        b_text(B, 57, 156, l, lv_hex(0xffffff));
        if (lv_detail && lv_sel >= 0 && lv_sel < nr) snprintf(r, sizeof r, "DETAIL  BACKBOT=LIST");
        else if (pages > 1) snprintf(r, sizeof r, "P%d/%d  TAP ROW=INFO", lv_cpage + 1, pages);
        else snprintf(r, sizeof r, "TAP ROW=INFO  2X=DETAIL");
        b_text35(B, 57, 166, r, hdr);
        lv_button(B, LV_R_BTN_SCAN, busy ? "STOP" : "SCAN", true);
        if (lv_detail && lv_sel >= 0 && lv_sel < nr) lv_button(B, LV_R_BTN_MORE, "HUNT", true);   /* item 9: detail -> radar hunt */
        else lv_button(B, LV_R_BTN_MORE, "MORE", nr > 8);
        if (lv_detail && lv_sel >= 0 && lv_sel < nr) {
            const LvFind &f = lv_f[rows[lv_sel]];
            char t[48], m[18]; lv_macstr(f.mac, m);
            lv_label(f, t, 22); b_text(B, 38, 177, t, f.alert ? lv_hex(0xff4040) : lv_hex(0xffffff));
            snprintf(t, sizeof t, "%d DBM  %s", f.rssi, f.kind == LV_K_WIFI ? "WIFI" : "BLE"); b_text(B, 38, 188, t, tan);
            b_text(B, 38, 199, m, lv_hex(0xffffff));
            if (f.kind == LV_K_WIFI) snprintf(t, sizeof t, "CH%d %s", f.ch, lv_auth_name(f.auth));
            else { const char *mn = lv_mfg_name(f.mfg); if (f.mfg == 0xFFFF) snprintf(t, sizeof t, "NO MFG DATA"); else snprintf(t, sizeof t, "%s %04X%s", mn ? mn : "MFG", f.mfg, f.apple == 0x12 ? " FMY" : ""); }
            b_text(B, 38, 210, t, tan);
            if (f.kind == LV_K_BLE && f.svc) { snprintf(t, sizeof t, "SVC %04X", f.svc); b_text(B, 38, 221, t, tan); }
            b_text35(B, 38, 236, f.alert ? "HUNT MATCH" : "", lv_hex(0xff4040));
            b_text35(B, 38, 245, "TAP = LIST   BACKBOT = LIST", hdr);
            return;
        }
        for (int i = 0; i < 8; ++i) {
            const int k = lv_cpage * 8 + i; if (k >= nr) break;
            const LvFind &f = lv_f[rows[k]]; const int y = 175 + i * 10;
            const bool sel = k == lv_sel;
            if (sel) b_fill(B, 36, y - 1, 136, 10, lv_ink(LV_ACCD));
            char nm[12], t[40]; lv_label(f, nm, 10);
            if (f.kind == LV_K_WIFI) snprintf(t, sizeof t, "%-9s%4d C%d", nm, f.rssi, f.ch);
            else { const char *mn = lv_mfg_name(f.mfg); char mt[6]; snprintf(mt, sizeof mt, "%02X%02X", f.mac[4], f.mac[5]);
                snprintf(t, sizeof t, "%-9s%4d %.4s", nm, f.rssi, f.mfg != 0xFFFF && mn ? mn : mt); }
            b_text(B, 38, y, t, f.alert ? lv_hex(0xff4040) : sel ? lv_hex(0xffffff) : lv_hex(0xd8d8d8));
            b_fill(B, 168, y + 2, 2, 3, f.kind == LV_K_WIFI ? lv_hex(LV_WIFI_COL) : lv_hex(LV_BLE_COL));
        }
        return;
    }
    /* ---- category grid ---- */
    if (busy) snprintf(l, sizeof l, "SCANNING %s", lv_job_now());
    else snprintf(l, sizeof l, "%s %d FINDS", lv_mode == 1 ? "WIFI" : lv_mode == 2 ? "BLE" : "ALL", nv);
    const int pages = nc > 11 ? (nc + 10) / 11 : 1; if (lv_page >= pages) lv_page = 0;
    if (pages > 1) snprintf(r, sizeof r, "%d TYPES P%d/%d", nc, lv_page + 1, pages); else snprintf(r, sizeof r, "%d TYPES", nc);
    b_text35(B, 38, 157, l, hdr);
    b_text35(B, 203 - lv_w35(r), 157, r, hdr);
    lv_button(B, LV_R_BTN_SCAN, busy ? "STOP" : "SCAN", true);
    lv_button(B, LV_R_BTN_MORE, "MORE", pages > 1);
    for (int t = 0; t < 12; ++t) {
        const int x = lv_tile_x(t), y = lv_tile_y(t);
        if (t == 11) {                                   /* fixed DEAUTH tile (was the DEAUTH tab) */
            const bool hot = lv_deauth_n > 0, on = lv_job == LV_J_DEAUTH;
            b_frame(B, x, y, 31, 28, hot ? lv_hex(0xff2020) : on ? lv_ink(LV_ACC) : tan);
            b_fill(B, x + 1, y + 1, 29, 26, lv_hex(0x060a0c));
            b_icon(B, x + 7, y + 2, LV_HUNT_ICON[LV_H_DEAUTH]);
            char d[12]; snprintf(d, sizeof d, lv_deauth_have || on ? "D%lu" : "DEAU", (unsigned long)lv_deauth_n);
            b_text35(B, x + 2, y + 21, d, hot ? lv_hex(0xff4040) : lv_hex(0xffffff));
            continue;
        }
        const int k = lv_page * 11 + t; const bool has = k < nc;
        b_frame(B, x, y, 31, 28, !has ? lv_dim(tan, 1, 2) : cats[k].alert ? lv_hex(0xff2020) : tan);
        b_fill(B, x + 1, y + 1, 29, 26, lv_hex(0x060a0c));
        if (!has) continue;
        const LvCatInfo &c = cats[k];
        b_icon(B, x + 7, y + 2, c.icon);
        char s[8]; snprintf(s, sizeof s, "X%d", c.n);
        b_text35(B, x + 2, y + 21, s, lv_hex(0xffffff));
        if (c.wifi) b_fill(B, x + 27, y + 22, 2, 2, lv_hex(LV_WIFI_COL));
        if (c.ble) b_fill(B, x + 24, y + 22, 2, 2, lv_hex(LV_BLE_COL));
        if (c.alert) b_text35(B, x + 27, y + 2, "!", lv_hex(0xff4040));
    }
}
/* the small terminal box on HOME / SCANbot / TERMINALbot / CONTROLbot (item 11).
 * Rows from the inner top T: T+2 newest log line (SCAN / TERM) or the old separator (HOME / CTL), T+11..T+23 the
 * button row (same D-grey frame + E-grey icon art as the old 20x12 icons, now 53 / 31 px wide with a themed label),
 * T+25 the prompt. Buttons stay inside the box; the tap rects (lv_mt_btn) cover the whole column above the prompt. */
static const char *const LV_MT_ICON[3][10] = {
    {".....#.....", ".....#.....", "....###....", "...#...#...", "..#.....#..", "###..#..###", "..#.....#..", "...#...#...", "....###....", ".....#....."},
    {"######......", "############", "#..........#", "#.########.#", "#..........#", "#.########.#", "#..........#", "############", 0, 0},
    {"##########", "..........", "..........", "##########", "..........", "..........", "##########", 0, 0, 0}};
#define LV_MT_D 0x29a8   /* (41,52,66) button frame, as the old icons */
#define LV_MT_E 0x8cd4   /* (140,154,165) icon art, as the old icons */
static const LvRect &lv_mt_box(void) { return lv_scr == LV_SC_SCAN ? LV_R_STERM : LV_R_TTERM; }
static int lv_mt_n(void) { return lv_scr == LV_SC_CTL ? 5 : 3; }
/* drawn rect of button i (frame included) */
static LvRect lv_mt_btn(int i)
{
    const LvRect &b = lv_mt_box(); const int y0 = b.y0 + 11;
    if (lv_scr == LV_SC_CTL) { const int x = 36 + i * 34; return {(int16_t)x, (int16_t)y0, (int16_t)(x + 30), (int16_t)(y0 + 12)}; }
    static const int16_t XS[3][2] = {{36, 88}, {93, 145}, {150, 202}}, XSS[3][2] = {{35, 88}, {93, 146}, {151, 204}};
    const int16_t(*X)[2] = lv_scr == LV_SC_SCAN ? XSS : XS;
    return {X[i][0], (int16_t)y0, X[i][1], (int16_t)(y0 + 12)};
}
/* tap rect of button i: from above the box top (title strip) to the row above the prompt; columns split at the gaps */
static LvRect lv_mt_tap(int i)
{
    const LvRect &b = lv_mt_box(); const int n = lv_mt_n();
    const LvRect d = lv_mt_btn(i);
    const int xl = i == 0 ? b.x0 - 4 : (lv_mt_btn(i - 1).x1 + d.x0) / 2 + 1, xr = i == n - 1 ? b.x1 + 2 : (d.x1 + lv_mt_btn(i + 1).x0) / 2;
    return {(int16_t)xl, (int16_t)(b.y0 - 10), (int16_t)xr, (int16_t)(b.y0 + 23)};
}
static LvRect lv_mt_prompt_draw(void) { const LvRect &b = lv_mt_box(); return {b.x0, (int16_t)(b.y0 + 24), b.x1, b.y1}; }
static LvRect lv_mt_prompt_tap(void)
{
    const LvRect &b = lv_mt_box();
    return {(int16_t)(b.x0 - 4), (int16_t)(b.y0 + 24), (int16_t)(b.x1 + 2), (int16_t)(lv_scr == LV_SC_SCAN ? 316 : 312)};
}
static const char *lv_mt_label(int i)
{
    if (lv_scr == LV_SC_CTL) { static const char *const L[2] = {"BLE", "SCAN"}; return i < 2 ? L[i] : 0; }
    static const char *const L[3] = {"SCAN", "LIST", "HELP"}; return L[i];
}
static void lv_mt_icon(LvBand &B, int k, int x, int y)
{
    for (int r = 0; r < 10 && LV_MT_ICON[k][r]; ++r)
        for (int c = 0; LV_MT_ICON[k][r][c]; ++c) if (LV_MT_ICON[k][r][c] == '#') b_pix(B, x + c, y + r, LV_MT_E);
}
static void lv_scene_minterm(LvBand &B, void *)
{
    const LvRect &bx = lv_mt_box();
    const bool log = lv_scr == LV_SC_SCAN || lv_scr == LV_SC_TERM;
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    const int T = bx.y0, x0 = bx.x0 + 3;
    if (log) { char a[28]; snprintf(a, sizeof a, "%s", lv_line_back(0)); b_text(B, x0, T + 2, a, lv_hex(0xffffff)); }
    else b_fill(B, 37, T + 6, 139, 1, lv_ink(LV_ACCD));   /* the clip's separator, kept where it was */
    static const int IW[3] = {11, 12, 10}, IH[3] = {10, 8, 7};
    for (int i = 0; i < lv_mt_n(); ++i) {
        const LvRect r = lv_mt_btn(i); const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
        b_fill(B, r.x0, r.y0, w, h, LV_MT_D); b_fill(B, r.x0 + 1, r.y0 + 1, w - 2, h - 2, 0x0000);
        const char *lab = lv_mt_label(i); const int k = lv_scr == LV_SC_CTL ? i - 2 : i;
        const int iw = k >= 0 ? IW[k] : 0, lw = lab ? (int)strlen(lab) * 6 - 1 : 0, gap = (iw && lw) ? 3 : 0;
        int cx = r.x0 + (w - iw - gap - lw) / 2;
        if (k >= 0) { lv_mt_icon(B, k, cx, r.y0 + 1 + (11 - IH[k]) / 2); cx += iw + gap; }
        if (lab) b_text(B, cx, r.y0 + 3, lab, lv_ink(LV_ACC));
    }
    const int py = T + 25;
    int x = b_text(B, x0, py, LV_PROMPT, lv_hex(LV_PROMPT_INK));
    const int maxc = bx.x1 - 2 - x - 8;
    const char *in = lv_inb; int len = lv_inlen;
    if (len * 6 > maxc) in += len - maxc / 6;
    x = b_text(B, x, py, in, lv_hex(0xffffff));
    if ((lv_now / 500) & 1 || lv_scr == LV_SC_SCAN) b_fill(B, x + 1, py, 7, 7, lv_ink(LV_ACC));
}
/* CONTROLbot overlays: the three value fields (theme / uptime / focus), BG MOTION and one value badge per chip.
 * The selected value is always spelled out in white on the theme accent, so the choice can't be missed. */
struct LvCtlBadge { LvRect r; const char *name; };
static const LvCtlBadge LV_CTL_BADGE[7] = {
    /* Taller value strips (was 7 px — owner: toggles showed no visible change). Still inside each chip's foot. */
    {{36, 134, 69, 147}, "MIX"}, {{36, 180, 69, 193}, "RAIN COL"}, {{36, 244, 69, 257}, "MASHUP"},
    {{166, 125, 204, 138}, "RAIN"}, {{166, 159, 204, 172}, "RAIN MODE"}, {{166, 220, 207, 237}, "MULTI"}, {{90, 242, 160, 256}, "ORIENT"}};
static const char *lv_ctl_val(int i)
{
    switch (i) {
    case 0: return LV_MIX_NAME[lv_scanmix]; case 1: return LV_RAINCOL_NAME[lv_raincol]; case 2: return LV_SPEED_NAME[lv_speed];
#if LV_BG_ENGINE
    case 3: return lv_bgstyle_badge(); case 4: return LV_RAINMODE_NAME[lv_rainmode]; case 5: return LV_AUTO_NAME[lv_auto];
#else
    case 3: return LV_ONOFF[lv_rain]; case 4: return LV_RAINMODE_NAME[lv_rainmode]; case 5: return LV_AUTO_NAME[lv_auto];
#endif
    }
    return LV_FLIP_NAME[lv_flip];
}
static void lv_badge(LvBand &B, const LvRect &r, const char *v, bool on)
{
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    b_fill(B, r.x0, r.y0, w, h, on ? lv_hex(LV_ACC[lv_theme]) : lv_hex(0x202020));
    b_frame(B, r.x0, r.y0, w, h, on ? lv_hex(0xffffff) : lv_hex(0x606060));
    b_text35(B, r.x0 + (w - lv_w35(v)) / 2, r.y0 + (h - 5) / 2, v, on ? lv_hex(0xffffff) : lv_hex(0xb0b0b0));
}
#if !LV_CTL_VALUEBOX
/* The locked CTL clip has stale value text baked in under six of the removed boxes (THEME "1/11", FOCUS "1/11",
 * BG MOTION "ON", RAIN COL "MULTI", RAIN MODE "MIXED", ORIENT "LAND PORT"). Each blank is the tight text bbox, kept
 * INSIDE the old box rect, on a spot where the clip is plain black (0x000000) and static; it is filled with that black.
 * MIX, RAIN, MASHUP and MULTI need no blank: under their old boxes the clip shows only its own chip art. */
static const LvRect LV_CTL_BLANK[6] = {{88, 36, 110, 44}, {88, 66, 110, 74}, {181, 246, 189, 252}, {43, 180, 63, 186}, {178, 159, 198, 162}, {102, 248, 154, 254}};
static void lv_scene_ctl_blank(LvBand &B, void *) { b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x000000)); }
#endif
static void lv_scene_ctl(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x080808));
    const uint16_t ink = lv_hex(0xdd92fe);
    char t[16];
    if (lv_in(LV_R_CTHEME, B.x0, B.y0)) { snprintf(t, sizeof t, "%s", LV_THEME_NAME[lv_theme]); b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(LV_ACC[lv_theme])); b_text(B, 112 - (int)strlen(t) * 6 + 1, 37, t, lv_hex(0xffffff)); }
    else if (lv_in(LV_R_CUP, B.x0, B.y0)) { const uint32_t s = lv_now / 1000; if (s < 6000) snprintf(t, sizeof t, "%02lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60)); else snprintf(t, sizeof t, "%luh", (unsigned long)(s / 3600)); b_text(B, 112 - (int)strlen(t) * 6 + 1, 52, t, ink); }
    else if (lv_in(LV_R_CFOCUS, B.x0, B.y0)) { snprintf(t, sizeof t, "%s", LV_FOCUS_NAME[lv_focus]); b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(LV_ACC[0])); b_text(B, 112 - (int)strlen(t) * 6 + 1, 67, t, lv_hex(0xffffff)); }
    else if (lv_in(LV_R_CMOTION, B.x0, B.y0)) { b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x000707)); lv_badge(B, {178, 246, 198, 252}, lv_motion ? "ON" : "OFF", lv_motion); }
    else for (int i = 0; i < 7; ++i) if (lv_in(LV_CTL_BADGE[i].r, B.x0, B.y0)) {
        if (i == 5) {   /* MULTI: status LED + value strip spanning the badge rect */
            b_fill(B, B.x0, B.y0, B.w, B.h, 0);
            b_fill(B, 168, 222, 12, 12, lv_auto ? lv_hex(0x20ff40) : lv_hex(0xff3048));
            b_frame(B, 168, 222, 12, 12, lv_auto ? lv_hex(0xb0ffb8) : lv_hex(0xff9aa6));
            lv_badge(B, {182, 222, 207, 235}, lv_ctl_val(5), lv_auto != 0);
        } else if (i == 6) {   /* ORIENT: both choices, the active one lit (replaces the old LAND / PORT legend) */
            const LvRect &r = LV_CTL_BADGE[6].r; const int h = r.x0 + (r.x1 - r.x0 + 1) / 2;
            lv_badge(B, {r.x0, r.y0, (int16_t)(h - 1), r.y1}, "PORT", !lv_flip); lv_badge(B, {(int16_t)(h + 1), r.y0, r.x1, r.y1}, "FLIP", lv_flip);
            b_fill(B, h - 1, r.y0, 2, r.y1 - r.y0 + 1, 0);
        } else lv_badge(B, LV_CTL_BADGE[i].r, lv_ctl_val(i), i == 0 || i == 1 || i == 2 || i == 4 ? true : lv_ctl_val(i)[1] != 'F');
        break;
    }
}

/* full-screen terminal + keyboard */
static const char *const LV_KROW[4] = {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm._/"};
static const char *const LV_KROW_S[4] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL:", "ZXCVBNM,?="};
enum { LV_KEY_SHIFT = 40, LV_KEY_SPACE, LV_KEY_DEL, LV_KEY_ENTER, LV_KEY_CMDS, LV_NKEY };
/* item 4 (mock-pending-approval): CMDS cheat sheet. Every row is a REAL lv_cmd verb (see LV_HELP / lv_cmd);
 * arg = 1: it takes an argument, so the insert ends with a space. */
struct LvCs { const char *cmd, *hint; uint8_t arg; };
static const LvCs LV_CS[] = {
    {"help", "list commands", 0}, {"scan", "[all/wifi/ble]", 1}, {"wifi", "wifi scan", 0}, {"ble", "ble scan", 0},
    {"stop", "stop scan", 0}, {"hunt", "[name/mac/n]", 1}, {"sweep", "<dev>", 1}, {"deauth", "deauth watch", 0},
    {"sniff", "= deauth", 0}, {"list", "[wifi/ble]", 1}, {"info", "[n]", 1}, {"filter", "<all/wifi/ble>", 1},
    {"ignored", "ignore list", 0}, {"unignore", "<mac/all>", 1}, {"go", "<home/scan/..>", 1}, {"theme", "[next/name/n]", 1},
    {"history", "last 8 cmds", 0}, {"clear", "clear log", 0}, {"status", "sys info", 0}, {"heap", "sys info", 0},
    {"uptime", "", 0}, {"version", "", 0}, {"about", "", 0}, {"bots", "bot names", 0},
    {"home", "GIBSONbot", 0}, {"back", "prev screen", 0},
};
#define LV_NCS ((int)(sizeof LV_CS / sizeof LV_CS[0]))
#define LV_CS_ROWS 5
#define LV_CS_HDR 26   /* header band: BACK | PREV | NEXT */
#define LV_CS_RH 24
static bool lv_cs_open = false; static int lv_cs_page = 0, lv_cs_hl = -1;
static LvRect lv_key_rect(int k)
{
    if (k < 40) { const int r = k / 10, c = k % 10; const int x = c * LV_KBD_PITCH_X + 1, y = LV_KBD_Y + r * LV_KBD_PITCH_Y + 1; return {(int16_t)x, (int16_t)y, (int16_t)(x + LV_KEY_W - 1), (int16_t)(y + LV_KEY_H - 1)}; }
    const int y = LV_KBD_Y + 4 * LV_KBD_PITCH_Y + 1;
    static const int X0[5] = {0, 48, 144, 192, 112}, X1[5] = {47, 111, 191, 239, 143};   /* SPACE 48-111, CMDS 112-143 */
    const int i = k - 40;
    return {(int16_t)(X0[i] + 1), (int16_t)y, (int16_t)(X1[i] - 1), (int16_t)(y + LV_KEY_H - 1)};
}
static int lv_key_at(int x, int y)
{
    if (y < LV_KBD_Y) return -1;
    const int r = (y - LV_KBD_Y) / LV_KBD_PITCH_Y;
    if (r < 4) { int c = x / LV_KBD_PITCH_X; if (c > 9) c = 9; return r * 10 + c; }
    if (x < 48) return LV_KEY_SHIFT;
    if (x < 112) return LV_KEY_SPACE;
    if (x < 144) return LV_KEY_CMDS;
    if (x < 192) return LV_KEY_DEL;
    return LV_KEY_ENTER;
}
static int lv_cs_npage(void) { return (LV_NCS + LV_CS_ROWS - 1) / LV_CS_ROWS; }
static int lv_cs_first(void) { const int f = lv_cs_page * LV_CS_ROWS; return f + LV_CS_ROWS > LV_NCS ? LV_NCS - LV_CS_ROWS : f; }   /* last page backs up: always 5 full rows, no empty (dead) slot */
static void lv_cs_tri(LvBand &B, int cx, int cy, bool up, uint16_t c)
{
    for (int i = 0; i < 6; ++i) b_fill(B, cx - i, up ? cy - 3 + i : cy + 2 - i, 2 * i + 1, 1, c);
}
static void lv_scene_cs(LvBand &B)   /* overlay on the keyboard rect; TERMINALbot look: black, theme frame, white text */
{
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x050505));
    const uint16_t acc = lv_ink(LV_ACC), fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR);
    const int np = lv_cs_npage(), y0 = LV_KBD_Y;
    static const int HX0[3] = {0, 80, 160}, HX1[3] = {79, 159, 239};
    for (int i = 0; i < 3; ++i) {
        const int x = HX0[i] + 1, w = HX1[i] - HX0[i] - 1, h = LV_CS_HDR - 2;
        const bool hl = lv_cs_hl == 100 + i, dis = (i == 1 && lv_cs_page == 0) || (i == 2 && lv_cs_page >= np - 1);
        b_fill(B, x, y0 + 1, w, h, hl ? lv_hex(0xffffff) : dis ? lv_hex(0x141414) : acc);
        b_frame(B, x, y0 + 1, w, h, dis ? fr : lv_hex(0xffffff));
        const uint16_t ink = dis ? lv_hex(0x505050) : lv_hex(0x000000);
        if (i == 0) b_text(B, x + (w - 4 * 12 + 2) / 2, y0 + 6, "BACK", ink, 2);
        else {
            char t[8]; snprintf(t, sizeof t, "%d/%d", lv_cs_page + 1, np);
            const int tw = (int)strlen(t) * 6 - 1, gx = x + (w - (12 + 4 + tw)) / 2;
            lv_cs_tri(B, gx + 6, y0 + 13, i == 1, ink);
            if (i == 2) b_text(B, gx + 16, y0 + 10, t, ink); else b_text(B, gx + 16, y0 + 10, "PREV", ink);
        }
    }
    for (int r = 0; r < LV_CS_ROWS; ++r) {
        const int n = lv_cs_first() + r, y = y0 + LV_CS_HDR + r * LV_CS_RH;
        if (n >= LV_NCS) break;
        const bool hl = lv_cs_hl == n;
        b_fill(B, 1, y + 1, 238, LV_CS_RH - 2, hl ? lv_hex(0xffffff) : lv_hex(0x141414));
        b_frame(B, 1, y + 1, 238, LV_CS_RH - 2, fr);
        b_text(B, 7, y + 5, LV_CS[n].cmd, hl ? lv_hex(0x000000) : lv_hex(0xffffff), 2);
        const int hw = (int)strlen(LV_CS[n].hint) * 6 - 1;
        if (hw > 0) b_text(B, 232 - hw, y + 9, LV_CS[n].hint, hl ? lv_hex(0x000000) : LV_CS[n].arg ? hdr : acc);
    }
}
static void lv_scene_kbd(LvBand &B, void *)
{
    if (lv_cs_open) { lv_scene_cs(B); return; }
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x050505));
    const uint16_t acc = lv_ink(LV_ACC), fr = lv_ink(LV_FRAME);
    for (int k = 0; k < LV_NKEY; ++k) {
        const LvRect r = lv_key_rect(k);
        if (r.y1 < B.y0 || r.y0 >= B.y0 + B.h) continue;
        const bool hl = k == lv_key_hl, special = k >= 40;
        const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
        b_fill(B, r.x0, r.y0, w, h, hl ? lv_hex(0xffffff) : special ? acc : lv_hex(0x141414));
        b_frame(B, r.x0, r.y0, w, h, special ? lv_hex(0xffffff) : fr);
        char t[8];
        if (k < 40) { t[0] = (lv_shift ? LV_KROW_S : LV_KROW)[k / 10][k % 10]; t[1] = 0; }
        else snprintf(t, sizeof t, "%s", k == LV_KEY_SHIFT ? (lv_shift ? "ABC" : "SHIFT") : k == LV_KEY_SPACE ? "SPACE" : k == LV_KEY_DEL ? "DEL" : k == LV_KEY_CMDS ? "CMDS" : "ENTER");
        const uint16_t ink = hl || special ? lv_hex(0x000000) : lv_hex(0xffffff);
        if (k < 40) b_text(B, r.x0 + (w - 10) / 2, r.y0 + (h - 14) / 2, t, ink, 2);
        else b_text(B, r.x0 + (w - (int)strlen(t) * 6 + 1) / 2, r.y0 + (h - 7) / 2, t, ink);
    }
}
/* TTY text area: rejoin print() segments that were wrapped at 38 cols (lv_tcont) so a long line never splits a word
 * across the scroll edge, then lay them out bottom-up in nrows rows from ry0. */
static void lv_tty_text(LvBand &B, int fy, int fh, int nrows, int ry0)
{
    const uint16_t fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR);
    b_frame(B, 0, fy, 240, fh, fr);
    static char lg[16][LV_TKLEN]; const char *vs[16]; int vl[16]; char ik[16];
    for (int i = 0; i < nrows; ++i) { vs[i] = ""; vl[i] = 0; ik[i] = ' '; }
    int r = nrows - 1, k = lv_scroll, ln = 0;
    const int avail = lv_tn < LV_TLINES ? lv_tn : LV_TLINES;
    while (r >= 0 && k < avail && ln < 16) {
        int j = k; while (j + 1 < avail && lv_tcont[(lv_tn - 1 - j) % LV_TLINES]) ++j;  /* j = first segment of that print */
        char *T = lg[ln++]; T[0] = 0;
        for (int q = j; q >= k; --q) strncat(T, lv_line_back(q), LV_TKLEN - 1 - strlen(T));
        k = j + 1;
        const int L = (int)strlen(T), n = L ? (L + LV_TTY_COLS - 1) / LV_TTY_COLS : 1;
        for (int q = 0; q < n; ++q) { const int row = r - n + 1 + q; if (row >= 0) { vs[row] = T + q * LV_TTY_COLS; vl[row] = L - q * LV_TTY_COLS > LV_TTY_COLS ? LV_TTY_COLS : L - q * LV_TTY_COLS; ik[row] = T[0]; } }
        r -= n;
    }
    for (int i = 0; i < nrows; ++i) {
        char t[LV_TTY_COLS + 1]; const int n = vl[i] < 0 ? 0 : vl[i]; memcpy(t, vs[i], n); t[n] = 0;
        const uint16_t ink = ik[i] == '$' ? lv_hex(LV_PROMPT_INK) : ik[i] == '!' ? lv_hex(0xff4040) : lv_hex(0xffffff);
        b_text(B, 4, ry0 + i * 9, t, ink);
    }
    const int total = avail;
    if (total > nrows) {
        const int bh = nrows * 9 + 1, h = bh * nrows / total, y = ry0 + (bh - h) * (total - nrows - lv_scroll) / (total - nrows);
        b_fill(B, 236, ry0, 2, bh, lv_hex(0x202020)); b_fill(B, 236, y, 2, h, hdr);
    }
}
/* A: target banner (22 px) between the header and the text while a hunt is running or its result is up */
static void lv_tty_banner(LvBand &B, int y0)
{
    const int hu = lv_hunt >= 0 ? lv_hunt : lv_hunt_last;
    const uint16_t fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR), gold = lv_hex(LV_TAN);
    b_frame(B, 0, y0, 240, 22, fr);
    b_fill(B, 1, y0 + 1, 238, 20, lv_hex(0x0a0a0a));
    b_frame(B, 3, y0 + 2, 18, 18, gold);
    b_fill(B, 4, y0 + 3, 16, 16, lv_hex(0x060a0c));
    b_icon(B, 4, y0 + 3, LV_HUNT_ICON[hu]);
    b_text(B, 25, y0 + 3, LV_HUNT_BOT[hu], lv_hex(0xffffff));
    char t[40]; snprintf(t, sizeof t, "HUNTS %s", LV_HUNT_WHAT[hu]);
    b_text35(B, 25, y0 + 14, t, gold);
    const int bx = 182, bw = 54, by = y0 + 14;
    if (lv_job != LV_J_NONE) {
        b_text35(B, bx + bw - lv_w35("SCANNING"), y0 + 4, "SCANNING", hdr);
        const int dots = (lv_now / 300) % 4; for (int d = 0; d < dots; ++d) b_fill(B, bx + bw - lv_w35("SCANNING") - 14 + d * 4, y0 + 8, 2, 1, hdr);
        b_frame(B, bx, by - 1, bw, 7, lv_dim(gold, 1, 2));
        const int seg = 12, span = bw - 2 - seg, p = (int)(lv_now / 30) % (2 * span), px = p < span ? p : 2 * span - p;
        for (int i = 0; i < seg; ++i) b_fill(B, bx + 1 + px + i, by, 1, 5, lv_dim(lv_ink(LV_ACC), i + 4, seg + 3));
    } else {
        const int m = lv_matches();
        char c[16]; if (hu == LV_H_DEAUTH) snprintf(c, sizeof c, "%lu DEAUTH", (unsigned long)lv_deauth_n); else snprintf(c, sizeof c, "%d FOUND", m);
        b_text(B, bx + bw - ((int)strlen(c) * 6 - 1), y0 + 3, c, (hu == LV_H_DEAUTH ? lv_deauth_n : (uint32_t)m) ? lv_hex(0xff4040) : lv_hex(0xffffff));
        char s[24]; if (hu == LV_H_DEAUTH) snprintf(s, sizeof s, "OF %lu MGMT", (unsigned long)lv_mgmt_n); else snprintf(s, sizeof s, "OF %d SEEN", lv_nf);
        b_text35(B, bx + bw - lv_w35(s), by, s, gold);
    }
}
static void lv_scene_tty(LvBand &B, void *)
{
    const uint16_t hdr = lv_ink(LV_HDR);
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_fill(B, 0, 24, 240, 1, lv_ink(LV_ACCD));
    b_fill(B, 0, 25, 240, 10, lv_hex(0x181818));
    b_text35(B, 3, 28, "TERMINALBOT", hdr);
    char t[40]; snprintf(t, sizeof t, "%s  TAP TOP/BOTTOM = SCROLL", lv_scroll ? "SCROLLED" : "LIVE");
    b_text35(B, 237 - lv_w35(t), 28, t, hdr);
    if (lv_hunt >= 0 || lv_hunt_last >= 0) { lv_tty_banner(B, 36); lv_tty_text(B, 58, 104, LV_TTY_LINES - 2, 61); }
    else lv_tty_text(B, 36, 126, LV_TTY_LINES, 39);
    int x = b_text(B, 2, 164, LV_PROMPT " ", lv_hex(LV_PROMPT_INK));
    const char *in = lv_inb; const int maxc = (238 - x - 8) / 6;
    if (lv_inlen > maxc) in += lv_inlen - maxc;
    x = b_text(B, x, 164, in, lv_hex(0xffffff));
    if ((lv_now / 400) & 1) b_fill(B, x + 1, 164, 6, 8, lv_ink(LV_ACC));
}

/* ---------- BACKbot: the return control (replaces the crown; one level up, else GIBSONbot home) ----------
 * A nameless square bot: a black-framed hot-pink square face with two eyes (the SCANbot/TERMINALbot face style:
 * light top-left bevel, dark bottom-right, a diagonal shine) and one row of gold pins on every side.
 * 19x19 (15x15 face + 2 px pins each side), hugging the bottom-right corner on every non-home screen so the face covers
 * the right mounting-hole ring (228..234, 308..314); the top pins overlap the 5V pin art (allowed); GND, the 5V label
 * and the terminal box (right edge x209) stay clear. TTY keeps it top-right (CLOSE's slot). It is a deco tint: the art
 * around it is the real clip (from the shadow). BACKBOT_FACE is the one face-colour constant. */
#ifndef BACKBOT_FACE_SEL
#define BACKBOT_FACE_SEL 3          /* 0 orange, 1 hot red, 2 neon cyan, 3 hot pink (default) */
#endif
static const uint32_t BACKBOT_FACES[4][4] = {{0x8a3200, 0xe06000, 0xff8c1a, 0xffc878}, {0x780010, 0xd01022, 0xff3040, 0xff9a9a},
                                             {0x005e6c, 0x00b4cc, 0x1ef0ff, 0xb4ffff}, {0x780050, 0xd0208c, 0xff44b4, 0xffa8dc}};
static int lv_backbot_face = BACKBOT_FACE_SEL;
#define BACKBOT_FACE (BACKBOT_FACES[lv_backbot_face])
#define BACKBOT_FS 15                                             /* square face side (incl. black frame) */
#define BACKBOT_W (BACKBOT_FS + 4)                                /* + 2 px pins on each side */
#define BACKBOT_H (BACKBOT_FS + 4)
static const LvRect LV_R_BACKBOT = {240 - BACKBOT_W, 320 - BACKBOT_H, 239, 319}; /* {221,301,239,319}: bottom-right corner, over the hole ring */
static const LvRect LV_R_BACKBOT_HIT = {210, 289, 239, 319}; /* finger target incl. slop (30x31); corner buttons end x205 */
static const LvRect LV_R_BACKBOT_TTY = {204, 0, 239, 23};    /* TTY: the keyboard owns the bottom row, BACKbot takes CLOSE's slot */
static void lv_draw_backbot(LvBand &B, int x0, int y0)
{
    const uint16_t gold = lv_hex(0xc8a24a), gold_hi = lv_hex(0xf0d080);
    const int S = BACKBOT_FS, fx = x0 + 2, fy = y0 + (BACKBOT_H - S) / 2;
    /* one row of pins on each of the four sides: 2x2 gold, light top row, 1 px gaps */
    for (int i = 0; i < 5; ++i) {
        const int py = fy + 1 + i * 3;
        b_fill(B, x0, py, 2, 2, gold); b_fill(B, fx + S, py, 2, 2, gold);
        b_fill(B, x0, py, 2, 1, gold_hi); b_fill(B, fx + S, py, 2, 1, gold_hi);
        const int px = fx + 1 + i * 3;
        b_fill(B, px, y0, 2, 2, gold); b_fill(B, px, fy + S, 2, 2, gold); b_fill(B, px, y0, 2, 1, gold_hi); b_fill(B, px, fy + S, 2, 1, gold_hi);
    }
    /* face: black frame, pink square with bevel + diagonal shine, two black eyes */
    b_fill(B, fx, fy, S, S, 0x0000);
    const uint32_t *F = BACKBOT_FACE;
    const int n = S - 2;
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
        uint32_t c;
        if (y == 0 || x == 0) c = F[3];
        else if (y == n - 1 || x == n - 1) c = F[0];
        else { const int d = x + y; c = (d >= 9 && d <= 10) ? F[3] : (d < 9 ? F[2] : F[1]); }
        b_pix(B, fx + 1 + x, fy + 1 + y, lv_hex(c));
    }
    b_fill(B, fx + 4, fy + 4, 3, 3, 0x0000); b_fill(B, fx + 8, fy + 4, 3, 3, 0x0000);
}

/* ---------- live tints: state per id (item 1 chip colours, CTL face, BACKbot) ---------- */

/* Scaled BACKbot form for CONTROLbot UI-selection chips: same pins / black frame / bevel face / two eyes.
 * S = face side (incl. black frame); outer size is S+4. face = mid identity colour; F[] = dark/mid/lit/hi. */
static void lv_draw_chipbot(LvBand &B, int x0, int y0, int S, const uint32_t F[4])
{
    const uint16_t gold = lv_hex(0xc8a24a), gold_hi = lv_hex(0xf0d080);
    const int fx = x0 + 2, fy = y0 + 2;
    const int n_side = (S - 1) / 3; if (n_side < 1) return;
    for (int i = 0; i < n_side; ++i) {
        const int py = fy + 1 + i * 3;
        if (py + 1 < fy + S) {
            b_fill(B, x0, py, 2, 2, gold); b_fill(B, fx + S, py, 2, 2, gold);
            b_fill(B, x0, py, 2, 1, gold_hi); b_fill(B, fx + S, py, 2, 1, gold_hi);
        }
        const int px = fx + 1 + i * 3;
        if (px + 1 < fx + S) {
            b_fill(B, px, y0, 2, 2, gold); b_fill(B, px, fy + S, 2, 2, gold);
            b_fill(B, px, y0, 2, 1, gold_hi); b_fill(B, px, fy + S, 2, 1, gold_hi);
        }
    }
    b_fill(B, fx, fy, S, S, 0x0000);
    const int n = S - 2;
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
        uint32_t c;
        if (y == 0 || x == 0) c = F[3];
        else if (y == n - 1 || x == n - 1) c = F[0];
        else { const int d = x + y; const int lo = n * 2 / 5, hi = n * 2 / 5 + 1; c = (d >= lo && d <= hi) ? F[3] : (d < lo ? F[2] : F[1]); }
        b_pix(B, fx + 1 + x, fy + 1 + y, lv_hex(c));
    }
    const int es = S >= 28 ? 3 : (S >= 18 ? 2 : 2);
    const int ey = fy + S / 5, ex0 = fx + S / 5, ex1 = fx + S - S / 5 - es;
    b_fill(B, ex0, ey, es, es, 0x0000); b_fill(B, ex1, ey, es, es, 0x0000);
}

#if LV_CTL_CHIPBOT
/* UI-selection chips converted to BACKbot form. Positions = old pin bboxes (labels stay in the clip below).
 * MIX / RAIN COL / MASHUP / RAIN / MULTI. Skipped (not IC-chip forms): RAIN MODE (label+value), BG MOTION (LED),
 * ORIENT (slider), THEME bar, big CONTROLbot, UPTIME, decorations. */
struct LvChipBot { LvRect r; int S; uint32_t F[4]; uint8_t multi; };
/* Face palettes: dark / mid / lit / hi, derived from each chip's identity colour. */
static const LvChipBot LV_CTL_CHIPBOTS[5] = {
    /* MIX      green 0x73db08 */ {{36, 89, 69, 122}, 30, {0x286800, 0x58c000, 0x90ff28, 0xd8ffb0}, 0},
    /* RAIN COL cyan          */ {{39, 146, 64, 171}, 22, {0x005868, 0x00b4cc, 0x40f0ff, 0xc0ffff}, 0},
    /* MASHUP   green         */ {{36, 201, 69, 234}, 30, {0x286800, 0x58c000, 0x90ff28, 0xd8ffb0}, 0},
    /* RAIN     green         */ {{170, 81, 203, 114}, 30, {0x286820, 0x58b038, 0x90ef70, 0xd8ffc0}, 0},
    /* MULTI    three-colour  */ {{170, 178, 203, 211}, 30, {0x401868, 0x8020b0, 0xb040ff, 0xe0a0ff}, 1},
};
static void lv_scene_ctl_chipbot(LvBand &B, void *ctx)
{
    const int i = *(const int *)ctx;
    const LvChipBot &C = LV_CTL_CHIPBOTS[i];
    /* clear to black inside the mask so any gap around the new form matches the surrounding clip */
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x000000));
    if (C.multi) {
        /* MULTI: BACKbot form with a 3-band cyan / yellow / magenta face (the old three-bot icon). */
        static const uint32_t BAND[3][4] = {
            {0x006878, 0x00a8c0, 0x18d3e7, 0xa0ffff},
            {0x686800, 0xb0b000, 0xffff00, 0xffffa0},
            {0x680040, 0xb02070, 0xde49ad, 0xffa0d0},
        };
        const int x0 = C.r.x0, y0 = C.r.y0, S = C.S;
        const uint16_t gold = lv_hex(0xc8a24a), gold_hi = lv_hex(0xf0d080);
        const int fx = x0 + 2, fy = y0 + 2;
        const int n_side = (S - 1) / 3;
        for (int k = 0; k < n_side; ++k) {
            const int py = fy + 1 + k * 3;
            if (py + 1 < fy + S) { b_fill(B, x0, py, 2, 2, gold); b_fill(B, fx + S, py, 2, 2, gold); b_fill(B, x0, py, 2, 1, gold_hi); b_fill(B, fx + S, py, 2, 1, gold_hi); }
            const int px = fx + 1 + k * 3;
            if (px + 1 < fx + S) { b_fill(B, px, y0, 2, 2, gold); b_fill(B, px, fy + S, 2, 2, gold); b_fill(B, px, y0, 2, 1, gold_hi); b_fill(B, px, fy + S, 2, 1, gold_hi); }
        }
        b_fill(B, fx, fy, S, S, 0x0000);
        const int n = S - 2, bh = n / 3;
        for (int y = 0; y < n; ++y) {
            const uint32_t *F = BAND[y < bh ? 0 : y < 2 * bh ? 1 : 2];
            for (int x = 0; x < n; ++x) {
                uint32_t c;
                if (y == 0 || x == 0) c = F[3];
                else if (y == n - 1 || x == n - 1) c = F[0];
                else { const int d = x + (y % bh); const int lo = bh * 2 / 5; c = (d >= lo && d <= lo + 1) ? F[3] : (d < lo ? F[2] : F[1]); }
                b_pix(B, fx + 1 + x, fy + 1 + y, lv_hex(c));
            }
        }
        const int es = 3, ey = fy + S / 5, ex0 = fx + S / 5, ex1 = fx + S - S / 5 - es;
        b_fill(B, ex0, ey, es, es, 0x0000); b_fill(B, ex1, ey, es, es, 0x0000);
    } else {
        lv_draw_chipbot(B, C.r.x0, C.r.y0, C.S, C.F);
    }
}
#endif

enum { LV_T_FLASH = 0, LV_T_SWIFI, LV_T_SBLE, LV_T_SBOT, LV_T_CFACE, LV_T_CLABEL, LV_T_BACK, LV_T_BGREY, LV_T_SPILL, LV_T_HUNT0 };
static bool lv_wifi_busy(void) { return lv_job == LV_J_WIFI || (lv_job == LV_J_ALL && lv_job_phase == 0); }
static bool lv_ble_busy(void) { return lv_job == LV_J_BLE || (lv_job == LV_J_ALL && lv_job_phase == 1); }
static bool lv_have(int kind) { for (int i = 0; i < lv_nf; ++i) if (lv_f[i].kind == kind) return true; return false; }
static LvTS lv_tint_state(int id)
{
    LvTS s = {LV_TM_RAW, 0, 0, 0};
    const bool ph = (lv_now / 400) & 1;
    switch (id) {
    case LV_T_SWIFI: s = {LV_TM_COLOR, (uint8_t)(lv_wifi_busy() ? (ph ? 7 : 6) : 5), LV_WIFI_COL, 0}; break;
    case LV_T_SBLE: s = {LV_TM_COLOR, (uint8_t)(lv_ble_busy() ? (ph ? 7 : 6) : 5), LV_BLE_COL, 0}; break;
    case LV_T_SBOT: if (lv_job != LV_J_NONE) s = {LV_TM_LIT, (uint8_t)(ph ? 3 : 1), 0, 0}; break;
    case LV_T_CFACE: if (lv_theme) s = {LV_TM_HUE, 0, LV_ACC[lv_theme], 0}; break;
    case LV_T_CLABEL: if (lv_theme) s = {LV_TM_RAW, 0, (uint32_t)lv_theme, 1}; break;
    case LV_T_BACK: s = {LV_TM_RAW, 0, (uint32_t)lv_backbot_face, 1}; break;
    case LV_T_BGREY: s = {LV_TM_GREY, 0, 0, 0}; break;
    case LV_T_SPILL: s = {LV_TM_RAW, 0, 0, 1}; break;
    default:
        if (id >= LV_T_HUNT0 && id < LV_T_HUNT0 + LV_NHUNT) {
            const int h = id - LV_T_HUNT0;
            if (lv_hunt == h && lv_job != LV_J_NONE) s = {LV_TM_LIT, (uint8_t)(ph ? 4 : 2), 0, 0};
            else if (lv_hunt_last == h) s = {LV_TM_LIT, 2, 0, 0};
        }
    }
    return s;
}
static const LvRect LV_R_CLABEL = {90, 188, 150, 198};
static void lv_tint_deco(LvBand &B, int id, const LvTS &s)
{
    if (id == LV_T_BACK) { lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0); return; }
    if (id == LV_T_SPILL) {   /* old icon stack below the box: copy the clip's rails / pins from 12 px left (they repeat every 12) */
        for (int y = B.y0; y < B.y0 + B.h; ++y)
            for (int x = B.x0 + 12; x < B.x0 + B.w; ++x) B.px[(y - B.y0) * B.w + (x - B.x0)] = B.px[(y - B.y0) * B.w + (x - 12 - B.x0)];
        return;
    }
    if (id == LV_T_CLABEL) {   /* CONTROLbot's name plate follows the theme (it was baked as GRAPE) */
        const LvRect &r = LV_R_CLABEL;
        for (int y = r.y0; y <= r.y1; ++y) {
            if (y < B.y0 || y >= B.y0 + B.h) continue;
            const uint16_t c = B.px[(y - B.y0) * B.w + (r.x0 - B.x0)];
            for (int x = r.x0 + 1; x < r.x1; ++x) B.px[(y - B.y0) * B.w + (x - B.x0)] = c;
        }
        const char *t = s.col >= LV_THEME_ORIG ? LV_THEME_FULL[s.col] : LV_THEME_NAME[s.col];
        b_text(B, (r.x0 + r.x1 + 1 - ((int)strlen(t) * 6 - 1)) / 2, r.y0 + 2, t, lv_hex(LV_ACC[s.col]));
    }
}
/* BOOTbot footprints (item 7) and the HOME chips as drawn (item 1: tap = drawn rect, padding comes from LV_PAD_HOME) */
static const LvRect LV_R_BB_BOOT = {76, 58, 164, 182}, LV_R_BB_HOME = {30, 45, 75, 113};
static const LvRect LV_HOME_CHIP[5] = {{31, 52, 74, 112}, {78, 30, 160, 150}, {165, 51, 208, 110}, {33, 156, 101, 255}, {139, 156, 207, 255}};
static void lv_tint_setup(void)
{
    lv_ntint = 0; lv_shadow_used = 0;
    const int s = lv_scr;
    if (s == LV_SC_BOOT) lv_tint_add(LV_T_BGREY, LV_R_BB_BOOT);           /* item 7: the big BOOTbot */
    if (s == LV_SC_HOME_INTRO) lv_tint_add(LV_T_BGREY, LV_R_BB_HOME);     /* item 7: the BOOTbot chip */
    if (s == LV_SC_HOME) {
        /* item 1: the drawn chips (measured on the home clip; the old rects were the pre-v2 layout, ~13 px left / 29 px up) */
        lv_tint_add(LV_T_BGREY, LV_R_BB_HOME);   /* BOOTbot chip: grey (item 7) + press flash */
        for (int i = 1; i < 5; ++i) lv_tint_add(LV_T_FLASH, LV_HOME_CHIP[i]);
        lv_tint_add(LV_T_SPILL, LV_R_SPILL_T);
    } else if (s == LV_SC_SCAN) {
        lv_tint_add(LV_T_SWIFI, {32, 30, 74, 92}); lv_tint_add(LV_T_SBLE, {32, 93, 74, 152}); lv_tint_add(LV_T_SBOT, {78, 30, 165, 150});
        lv_tint_add(LV_T_FLASH, {166, 58, 208, 122}); lv_tint_add(LV_T_SPILL, LV_R_SPILL_S);
    } else if (s == LV_SC_TERM) {
        static const LvRect HOT[LV_NHUNT] = {{35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
                                             {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250}};
        for (int h = 0; h < LV_NHUNT; ++h) lv_tint_add(LV_T_HUNT0 + h, HOT[h]);
        lv_tint_add(LV_T_FLASH, {75, 30, 165, 150}); lv_tint_add(LV_T_SPILL, LV_R_SPILL_T);
    } else if (s == LV_SC_CTL) {
        lv_tint_add(LV_T_CFACE, {95, 103, 145, 160}); lv_tint_add(LV_T_CLABEL, LV_R_CLABEL);
        static const LvRect R[] = {{35, 85, 70, 140}, {35, 145, 70, 190}, {35, 195, 70, 250}, {165, 80, 205, 130}, {170, 131, 205, 170},
                                   {165, 175, 207, 237}, {90, 212, 159, 258}, {160, 238, 208, 258}, {35, 33, 113, 79}};
        for (const LvRect &r : R) lv_tint_add(LV_T_FLASH, r);
        lv_tint_add(LV_T_SPILL, LV_R_SPILL_T);
    }
    if (s == LV_SC_SCAN || s == LV_SC_TERM || s == LV_SC_CTL) lv_tint_add(LV_T_BACK, LV_R_BACKBOT);
}
static void lv_set_masks(void)
{
    lv_nmask = 0;
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO || lv_scr == LV_SC_CAL || lv_scr == LV_SC_TTY || lv_scr == LV_SC_RADAR) return;
    lv_masks[lv_nmask++] = LV_R_TICK;
    if (lv_scr == LV_SC_SCAN) { lv_masks[lv_nmask++] = LV_R_SBOX; lv_masks[lv_nmask++] = LV_R_STERM; }
    if (lv_scr == LV_SC_TERM || lv_scr == LV_SC_HOME || lv_scr == LV_SC_CTL) lv_masks[lv_nmask++] = LV_R_TTERM;
    if (lv_scr == LV_SC_CTL) {
#if LV_CTL_VALUEBOX
        lv_masks[lv_nmask++] = LV_R_CTHEME; lv_masks[lv_nmask++] = LV_R_CUP; lv_masks[lv_nmask++] = LV_R_CFOCUS; lv_masks[lv_nmask++] = LV_R_CMOTION;
        for (int i = 0; i < 7; ++i) lv_masks[lv_nmask++] = LV_CTL_BADGE[i].r;
#else
        lv_masks[lv_nmask++] = LV_R_CUP;   /* uptime clock stays (it is not a tap value) */
        for (int i = 0; i < 6; ++i) lv_masks[lv_nmask++] = LV_CTL_BLANK[i];
#if LV_CTL_CHIPBOT
        for (int i = 0; i < 5; ++i) lv_masks[lv_nmask++] = LV_CTL_CHIPBOTS[i].r;
#endif
#endif
    }
}
static void lv_paint_tints(bool all)
{
    for (int t = 0; t < lv_ntint; ++t) {
        const LvTS s = lv_tint_state(lv_tints[t].id);
        if (all || lv_tdirty[t] || !lv_ts_eq(s, lv_tdrawn[t])) lv_tint_paint(t);
    }
}

#include "lv_radar.h"

/* ---------- flow ---------- */
static int lv_theme_seg(int base) { return base + (lv_theme < LV_THEME_ORIG ? lv_theme : LV_THEME_BASE); }
static uint32_t lv_flash_t = 0;
static int lv_soon_kind = 0, lv_soon_scr = 0; static char lv_soon_cmd[32]; static uint32_t lv_soon_t = 0;
static void lv_go(int s)
{
    const int prev = lv_scr;
    lv_cs_open = false;   /* the CMDS overlay never survives a screen change */
    if (prev == LV_SC_RADAR && s != LV_SC_RADAR) lv_radar_leave();   /* leaving releases a lock and ends a hunt */
    /* item 14: tear the radio down when heading home so no scan/deauth/hunt is left running across screens */
    if (prev != s && lv_job != LV_J_NONE && (s == LV_SC_HOME || s == LV_SC_HOME_INTRO || s == LV_SC_BOOT)) lv_job_stop();
    if (s == LV_SC_RADAR && prev != LV_SC_RADAR) { lv_rd_from = prev; if (prev == LV_SC_SCAN) lv_rd_save_scan(); }
    lv_scr = s; lv_enter = true;
    lv_key_hl = -1; lv_flash_r = {0, 0, -1, -1}; lv_soon_kind = 0;
    switch (s) {
    case LV_SC_BOOT: lv_play.start(LK_S_BOOT); break;
    case LV_SC_HOME_INTRO: lv_play.start(LK_S_HOME_INTRO); break;
    case LV_SC_HOME: lv_play.start(lv_theme_seg(LK_S_HOME_T0)); break;
    case LV_SC_TERM: lv_play.start(lv_theme_seg(LK_S_TERM_T0)); break;
    case LV_SC_SCAN: lv_play.start(lv_theme_seg(LK_S_SCAN_T0)); break;
    case LV_SC_CTL: lv_play.start(LK_S_CTL); break;
    default: lv_play.seg = -1; break;   /* TTY, CAL, RADAR draw themselves */
    }
    if (s == LV_SC_RADAR) lv_radar_enter();
    if (prev == LV_SC_RADAR && s == LV_SC_SCAN) lv_rd_restore_scan();
    lv_set_masks(); lv_tint_setup();
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = true;
}
static int lv_period(void) { return lv_scr == LV_SC_CTL ? 90 : 30; }
/* ---------- CALbot: power-on touch calibration screen (CALbot -> BOOTbot -> GIBSONbot home) ----------
 * The board is the user's approved CALbot design, replayed from src/live/lv_calbot_art.h (generated from the
 * LOCKED refs by tools/livescan/make_calbot_art.py): the CAL OK frame is the base image and every part that moves
 * (centre chip face, the four corner chips' faces / antennas / labels / gold traces, CAL n OF 4 status, rail
 * pins, tabs, cursor) is a variant picked from the cal state and the 100 ms beat since CALbot opened, exactly as
 * the reference flow. The two terminal lines are live glcd text ("cal 1/4 TOP LEFT" / "hold the chip 1.5s").
 * Capture order is clockwise as on the board: 1 TOP LEFT, 2 TOP RIGHT, 3 BOTTOM RIGHT, 4 BOTTOM LEFT.
 * A hold is 1.5 s: 32 raw XPT2046 samples >= 45 ms apart, rejected if the spread is >= 150 raw (old rule), else
 * the mean of the middle 16. The four chip centres are fitted with a least-squares affine screen->raw map and
 * extrapolated to the TRUE corners, so NVS keeps exactly the old 8-point LovyanGFX matrix (botcorps c0..c7, cv). */
#include "lv_calbot_art.h"
static const int16_t LV_CAL_PT[4][2] = {{52, 51}, {187, 51}, {187, 222}, {52, 222}};   /* face centres TL TR BR BL (match LVC_EL face1..4) */
static const char *const LV_CAL_NAME[4] = {"TOP LEFT", "TOP RIGHT", "BOTTOM RIGHT", "BOTTOM LEFT"};
enum { LV_CAL_NVS = 0, LV_CAL_RUN = 1, LV_CAL_OK = 2, LV_CAL_KEPT = 3 };
#define LV_CAL_NS 32                         /* samples per hold, >= 45 ms apart, hold done at 1.5 s */
#define LV_CAL_HOLD_MS 1500
static int lv_cal_phase = LV_CAL_RUN, lv_cal_n = 0, lv_cal_ns = 0, lv_cal_msg = 0;
static bool lv_cal_nvs = false, lv_cal_lift = false, lv_cal_saved = false, lv_cal_done = false;
static uint32_t lv_cal_open, lv_cal_t0, lv_cal_last, lv_cal_up_t, lv_cal_hold0, lv_cal_deadline, lv_cal_msg_t, lv_cal_lock_t[4];
static int lv_cal_sx[LV_CAL_NS], lv_cal_sy[LV_CAL_NS];
static uint16_t lv_cal_med[4][2], lv_cal_c8[8];
/* line-2 notices: 1 spread, 2 released early, 3 nvs save failed (shown for LV_CAL_MSG_MS[msg]) */
static const char *const LV_CAL_MSG[] = {"", "hold still - try again", "released early - hold ", "nvs save failed"};
/* (the reference draws "released early - hold" with a trailing blank: its scan rule starts one cell later) */
static const uint16_t LV_CAL_MSG_MS[] = {0, 700, 500, 1500};

/* 4 medians at the chip centres -> raw at the 4 true corners (c0..c7). Pure math, host-testable. */
/* rot2: the CALbot screen runs at setRotation(2) (portrait, USB at the bottom), which is the native panel turned
 * 180 deg. The c0..c7 LovyanGFX stores are for the NATIVE corners (rotation 0 order), so native corner (tx,ty) is
 * screen point (239-tx, 319-ty) here: evaluate the fit there. rot2 = 0 (ORIENT FLIP, setRotation 0) is identity. */
static void lv_cal_solve(const uint16_t med[4][2], uint16_t out[8], int rot2)
{
    for (int ax = 0; ax < 2; ++ax) {          /* fit raw = a + b*sx + c*sy (least squares over 4 points) */
        double S[3][4] = {{0}};
        for (int i = 0; i < 4; ++i) {
            const double v[3] = {1.0, (double)LV_CAL_PT[i][0], (double)LV_CAL_PT[i][1]};
            for (int r = 0; r < 3; ++r) { for (int c = 0; c < 3; ++c) S[r][c] += v[r] * v[c]; S[r][3] += v[r] * med[i][ax]; }
        }
        for (int p = 0; p < 3; ++p) {         /* Gauss-Jordan, the 3x3 is well conditioned (rectangle) */
            for (int r = 0; r < 3; ++r) if (r != p) {
                const double f = S[r][p] / S[p][p];
                for (int c = p; c < 4; ++c) S[r][c] -= f * S[p][c];
            }
        }
        const double a = S[0][3] / S[0][0], b = S[1][3] / S[1][1], c = S[2][3] / S[2][2];
        for (int i = 0; i < 4; ++i) {
            const int tx = (i & 2) ? 239 : 0, ty = (i & 1) ? 319 : 0;   /* LovyanGFX corner order (native) */
            const int sx = rot2 ? 239 - tx : tx, sy = rot2 ? 319 - ty : ty;
            double v = a + b * sx + c * sy + 0.5;
            if (v < 0) v = 0; if (v > 4095) v = 4095;
            out[i * 2 + ax] = (uint16_t)v;
        }
    }
}
static void lv_cal_start_run(uint32_t now)
{
    lv_cal_phase = LV_CAL_RUN; lv_cal_n = 0; lv_cal_ns = 0; lv_cal_lift = true; lv_cal_msg = 0; lv_cal_done = false;
    lv_cal_deadline = now + 15000;
    for (int i = 0; i < 4; ++i) lv_cal_lock_t[i] = 0;
}
static void lv_cal_note(int m, uint32_t now) { lv_cal_msg = m; lv_cal_msg_t = now; }
static void lv_cal_finish(uint32_t now)
{
    lv_cal_solve(lv_cal_med, lv_cal_c8, lv_flip ? 0 : 1);
    const bool ok = lv_radio && lv_radio->cal_save && lv_radio->cal_save(lv_cal_c8);
    if (ok) { lv_cal_phase = LV_CAL_OK; lv_cal_saved = true; lv_cal_t0 = now; lv_cal_msg = 0; return; }
    if (lv_cal_nvs) { lv_cal_phase = LV_CAL_KEPT; lv_cal_t0 = now; lv_cal_note(3, now); return; }  /* old cal still applied */
    lv_cal_start_run(now); lv_cal_note(3, now);
}
/* the held chip: hold the samples, lock it 100 ms after the hold completes (100% shown for one beat) */
static void lv_cal_lock_due(uint32_t now)
{
    if (lv_cal_phase != LV_CAL_RUN || !lv_cal_done || (int32_t)(now - lv_cal_hold0) < LV_CAL_HOLD_MS + 100) return;
    lv_cal_done = false; lv_cal_ns = 0;
    lv_cal_lock_t[lv_cal_n] = lv_cal_hold0 + LV_CAL_HOLD_MS + 100;
    lv_cal_deadline = now + 15000;
    if (++lv_cal_n == 4) lv_cal_finish(now);
}
/* Firmware feeds the RAW touch every loop while lv_scr == LV_SC_CAL. */
static void lv_cal_raw(bool down, int rx, int ry, uint32_t now)
{
    if (lv_scr != LV_SC_CAL) return;
    if (lv_cal_phase == LV_CAL_NVS) {                 /* saved cal: hold the glass 1 s to redo it */
        if (!down) { lv_cal_hold0 = 0; return; }
        if (!lv_cal_hold0) lv_cal_hold0 = now ? now : 1;
        if ((int32_t)(now - lv_cal_hold0) >= 1000) lv_cal_start_run(now);
        return;
    }
    if (lv_cal_phase != LV_CAL_RUN) return;
    lv_cal_lock_due(now);
    if (lv_cal_phase != LV_CAL_RUN || lv_cal_done) return;
    if (lv_cal_lift) { if (!down) lv_cal_lift = false; return; }
    if (!down) {                                      /* resistive drop-outs: > 40 ms up ends the hold early */
        if (lv_cal_ns && (int32_t)(now - lv_cal_up_t) > 40) { lv_cal_ns = 0; lv_cal_note(2, now); }
        return;
    }
    lv_cal_up_t = now;
    if (!lv_cal_ns) { lv_cal_hold0 = now; if (lv_cal_msg == 2) lv_cal_msg = 0; }
    else if ((int32_t)(now - lv_cal_last) < 45) goto check;
    if (lv_cal_ns < LV_CAL_NS) { lv_cal_last = now; lv_cal_sx[lv_cal_ns] = rx; lv_cal_sy[lv_cal_ns] = ry; ++lv_cal_ns; }
check:
    if (lv_cal_ns < LV_CAL_NS || (int32_t)(now - lv_cal_hold0) < LV_CAL_HOLD_MS) return;
    {
        int sx[LV_CAL_NS], sy[LV_CAL_NS];
        for (int k = 0; k < LV_CAL_NS; ++k) { sx[k] = lv_cal_sx[k]; sy[k] = lv_cal_sy[k]; }
        for (int a = 0; a < LV_CAL_NS - 1; ++a) for (int b = a + 1; b < LV_CAL_NS; ++b) {
            if (sx[a] > sx[b]) { int t = sx[a]; sx[a] = sx[b]; sx[b] = t; }
            if (sy[a] > sy[b]) { int t = sy[a]; sy[a] = sy[b]; sy[b] = t; }
        }
        lv_cal_lift = true;
        if (sx[LV_CAL_NS - 1] - sx[0] >= 150 || sy[LV_CAL_NS - 1] - sy[0] >= 150) { lv_cal_ns = 0; lv_cal_note(1, now); return; }
        int mx = 0, my = 0;                           /* mean of the middle half (outliers dropped) */
        for (int k = LV_CAL_NS / 4; k < LV_CAL_NS * 3 / 4; ++k) { mx += sx[k]; my += sy[k]; }
        lv_cal_med[lv_cal_n][0] = (uint16_t)((mx + LV_CAL_NS / 4) / (LV_CAL_NS / 2)); lv_cal_med[lv_cal_n][1] = (uint16_t)((my + LV_CAL_NS / 4) / (LV_CAL_NS / 2));
        lv_cal_done = true;                           /* 100% + lime antenna for one beat, then locked */
    }
}

/* ---- what the board shows at time now: one variant per art element + the two terminal lines ---- */
struct LvCalView { uint8_t v[LVC_NE]; uint16_t rule; char l1[24], l2[24]; };
static LvCalView lv_cal_view;
static void lv_cal_compute(LvCalView &V, uint32_t now)
{
    const uint32_t ms = now - lv_cal_open, f = ms / 100;
    const int pb = (int)(((f + 2) / 9) & 1);
    const bool run = lv_cal_phase == LV_CAL_RUN;
    const int n = run ? lv_cal_n : (lv_cal_phase == LV_CAL_KEPT ? lv_cal_n : 4);
    const int32_t e = (run && (lv_cal_ns || lv_cal_done)) ? (int32_t)(now - lv_cal_hold0) : -1;
    static const uint8_t LBL_BLINK[7] = {1, 1, 2, 1, 2, 1, 2};
    V.v[LVC_E_CTR] = (uint8_t)pb;
    for (int i = 0; i < 4; ++i) {
        const int b = LVC_E_FACE1 + i * 4;            /* face, ant, lbl, tr */
        V.v[b] = (uint8_t)((i <= n ? 2 : 0) + pb);
        uint8_t a = 0, t;
        if (i < n) { a = 15; t = 17; }
        else if (i > n) t = 0;
        else if (e >= LV_CAL_HOLD_MS) { a = 15; t = 2; }
        else if (e >= 100) { a = (uint8_t)(e / 100); t = (uint8_t)(2 + e / 100); }
        else t = (f % 5) >= 3 ? 2 : 1;
        V.v[b + 1] = a; V.v[b + 3] = t;
        const uint32_t d = (i < n && lv_cal_lock_t[i]) ? (now - lv_cal_lock_t[i]) / 100 : 99;
        V.v[b + 2] = d < 7 ? LBL_BLINK[d] : 0;
    }
    V.v[LVC_E_STATUS] = (uint8_t)n;
    static const uint8_t PIN_OFF[5] = {12, 16, 20, 32, 36};
    for (int j = 0; j < 5; ++j) V.v[LVC_E_PIN1 + j] = (uint8_t)(((f + 48 * 8 - PIN_OFF[j]) % 48) < 4);
    const int g = (int)(f % 16);
    V.v[LVC_E_TAB1] = g >= 8 && g < 12; V.v[LVC_E_TAB2] = g >= 12; V.v[LVC_E_TAB3] = g >= 4; V.v[LVC_E_TAB4] = g >= 4 && g < 8;
    V.v[LVC_E_CURSOR] = ((ms + 920) % 1058) < 545;
    V.rule = LVC_RULE[f % (sizeof LVC_RULE / sizeof LVC_RULE[0])];
    const bool msg = lv_cal_msg && (int32_t)(now - lv_cal_msg_t) < LV_CAL_MSG_MS[lv_cal_msg];
    if (lv_cal_phase == LV_CAL_NVS) { snprintf(V.l1, 24, "saved cal loaded 4/4"); snprintf(V.l2, 24, "hold 1s to recal"); }
    else if (lv_cal_phase == LV_CAL_OK) { snprintf(V.l1, 24, "touch cal ok 4/4 saved"); snprintf(V.l2, 24, "starting BOOTbot"); }
    else if (lv_cal_phase == LV_CAL_KEPT) { snprintf(V.l1, 24, msg ? "nvs save failed" : "cal timeout 15s"); snprintf(V.l2, 24, "saved cal kept"); }
    else {
        snprintf(V.l1, 24, "cal %d/4 %s", n + 1, LV_CAL_NAME[n]);
        if (e >= 100) snprintf(V.l2, 24, "holding %d%%", e >= LV_CAL_HOLD_MS ? 100 : (int)(e / 15));
        else if (n && (int32_t)(now - lv_cal_lock_t[n - 1]) < 700) snprintf(V.l2, 24, "cal %d locked", n);
        else if (msg) snprintf(V.l2, 24, "%s", LV_CAL_MSG[lv_cal_msg]);
        else snprintf(V.l2, 24, "hold the chip 1.5s");
    }
}
/* palette-RLE decode of one element variant into the band (255 = leave the base) */
static void lv_cal_el(LvBand &B, int el, int var)
{
    const LvcElem &E = LVC_EL[el];
    if (E.y1 < B.y0 || E.y0 >= B.y0 + B.h || E.x1 < B.x0 || E.x0 >= B.x0 + B.w) return;
    const int w = E.x1 - E.x0 + 1;
    const uint8_t *p = LVC_VAR + LVC_VOFF[E.v0 + var], *end = LVC_VAR + LVC_VOFF[E.v0 + var + 1];
    for (int k = 0; p < end; p += 2) {
        const int cnt = p[0], c = p[1];
        if (c != 255) for (int j = 0; j < cnt; ++j) b_pix(B, E.x0 + (k + j) % w, E.y0 + (k + j) / w, LVC_PAL[c]);
        k += cnt;
    }
}
static void lv_scene_cal(LvBand &B, void *)
{
    for (int y = B.y0; y < B.y0 + B.h; ++y) {         /* base = the approved CAL OK frame */
        const uint8_t *p = LVC_BASE + LVC_BASE_ROW[y];
        uint16_t *row = B.px + (y - B.y0) * B.w;
        for (int x = 0; x < 240; p += 2) {
            const int x1 = x + p[0];
            const int a = x > B.x0 ? x : B.x0, b = x1 < B.x0 + B.w ? x1 : B.x0 + B.w;
            for (int xx = a; xx < b; ++xx) row[xx - B.x0] = LVC_PAL[p[1]];
            x = x1;
        }
    }
    const LvCalView &V = lv_cal_view;
    for (int el = 0; el < LVC_NE; ++el) lv_cal_el(B, el, V.v[el]);
    if (B.y0 > 290 || B.y0 + B.h <= 275) return;      /* terminal: two glcd lines + the scan rule */
    b_fill(B, 37, 275, 139, 16, 0);
    b_text(B, 39, 275, V.l1, LVC_INK); b_text(B, 39, 283, V.l2, LVC_INK);
    const int r = V.rule, x = 39 + 6 * (int)strlen(r <= 282 ? V.l1 : V.l2);
    b_fill(B, 37, r, 2, 1, LVC_RULE_C);
    if (x <= 175) b_fill(B, x, r, 176 - x, 1, LVC_RULE_C);
}
static const LvRect LV_R_CAL_TERM = {37, 275, 175, 290};
static void lv_cal_tick(uint32_t now)
{
    lv_cal_lock_due(now);
    if (lv_cal_phase == LV_CAL_NVS && !lv_cal_hold0 && (int32_t)(now - lv_cal_t0) >= 2000) { lv_go(LV_SC_BOOT); lv_clip_t = now; return; }
    if ((lv_cal_phase == LV_CAL_OK || lv_cal_phase == LV_CAL_KEPT) && (int32_t)(now - lv_cal_t0) >= 1300) { lv_go(LV_SC_BOOT); lv_clip_t = now; return; }
    if (lv_cal_phase == LV_CAL_RUN && lv_cal_nvs && !lv_cal_ns && !lv_cal_done && (int32_t)(now - lv_cal_deadline) >= 0) {
        lv_cal_phase = LV_CAL_KEPT; lv_cal_saved = false; lv_cal_msg = 0; lv_cal_t0 = now;   /* old cal untouched */
    }
    LvCalView V; memset(&V, 0, sizeof V);
    lv_cal_compute(V, now);
    const LvCalView old = lv_cal_view;
    lv_cal_view = V;
    if (lv_enter) { lv_render(lv_blit, {0, 0, 239, 319}, lv_scene_cal, 0); lv_enter = false; return; }
    for (int el = 0; el < LVC_NE; ++el)
        if (V.v[el] != old.v[el]) { const LvcElem &E = LVC_EL[el]; lv_render(lv_blit, {E.x0, E.y0, E.x1, E.y1}, lv_scene_cal, 0); }
    if (V.rule != old.rule || strcmp(V.l1, old.l1) || strcmp(V.l2, old.l2)) lv_render(lv_blit, LV_R_CAL_TERM, lv_scene_cal, 0);
}

static void lv_selftest(void)
{
    lv_print("BOOTbot self test:");
    lv_sysinfo();
    lv_print("radios: WiFi off, BLE off (idle)");
    lv_event(false, "SELF TEST OK  %s", LV_VERSION);
}

static void lv_scene_backbot_tty(LvBand &B, void *)
{
    /* Band is the 19x19 bot footprint only (see paint). Do not black-fill the old 36x24 hit slot — that was the shade. */
    lv_draw_backbot(B, B.x0, B.y0);
}
static void lv_paint_overlays(bool all)
{
    if (lv_scr == LV_SC_TTY) {
        if (all || lv_dirty_tty) { lv_render(lv_blit, LV_R_TTY_BODY, lv_scene_tty, 0); lv_dirty_tty = false; }
        if (all || lv_dirty_kbd) { lv_render(lv_blit, LV_R_KBD, lv_scene_kbd, 0); lv_dirty_kbd = false; }
        if (all) {
            const int bx = LV_R_BACKBOT_TTY.x1 + 1 - BACKBOT_W, by = LV_R_BACKBOT_TTY.y0 + (24 - BACKBOT_H) / 2;
            lv_render(lv_blit, {(int16_t)bx, (int16_t)by, (int16_t)(bx + BACKBOT_W - 1), (int16_t)(by + BACKBOT_H - 1)}, lv_scene_backbot_tty, 0);
        }
        return;
    }
    if (lv_scr == LV_SC_SCAN) {
        if (all || lv_dirty_box) { lv_render(lv_blit, LV_R_SBOX, lv_scene_box, 0); lv_dirty_box = false; }
        if (all || lv_dirty_term) { lv_render(lv_blit, LV_R_STERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    }
    if ((lv_scr == LV_SC_TERM || lv_scr == LV_SC_HOME || lv_scr == LV_SC_CTL) && (all || lv_dirty_term)) { lv_render(lv_blit, LV_R_TTERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    if (lv_scr == LV_SC_CTL && (all || lv_dirty_ctl)) {
#if LV_CTL_VALUEBOX
        lv_render(lv_blit, LV_R_CTHEME, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CUP, lv_scene_ctl, 0);
        lv_render(lv_blit, LV_R_CFOCUS, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CMOTION, lv_scene_ctl, 0);
        for (int i = 0; i < 7; ++i) lv_render(lv_blit, LV_CTL_BADGE[i].r, lv_scene_ctl, 0);
#else
        lv_render(lv_blit, LV_R_CUP, lv_scene_ctl, 0);
        for (int i = 0; i < 6; ++i) lv_render(lv_blit, LV_CTL_BLANK[i], lv_scene_ctl_blank, 0);
#if LV_CTL_CHIPBOT
        for (int i = 0; i < 5; ++i) { int id = i; lv_render(lv_blit, LV_CTL_CHIPBOTS[i].r, lv_scene_ctl_chipbot, &id); }
#endif
#endif
        lv_dirty_ctl = false;
    }
}
/* item 2: brief brighten flash on whatever was pressed (clip art via its tint, overlays via their scene) */
static void lv_flash_repaint(const LvRect &r)
{
    if (lv_scr == LV_SC_RADAR) return;   /* redrawn every frame */
    for (int t = 0; t < lv_ntint; ++t) if (lv_rect_hit(lv_tints[t].r, r)) lv_tint_paint(t);
    if (lv_scr == LV_SC_TTY) {
        const int bx = LV_R_BACKBOT_TTY.x1 + 1 - BACKBOT_W, by = LV_R_BACKBOT_TTY.y0 + (24 - BACKBOT_H) / 2;
        const LvRect br = {(int16_t)bx, (int16_t)by, (int16_t)(bx + BACKBOT_W - 1), (int16_t)(by + BACKBOT_H - 1)};
        if (lv_rect_hit(br, r)) lv_render(lv_blit, br, lv_scene_backbot_tty, 0);
    }
    if (lv_rect_hit(LV_R_SBOX, r)) lv_dirty_box = true;
    if (lv_rect_hit(LV_R_STERM, r) || lv_rect_hit(LV_R_TTERM, r)) lv_dirty_term = true;
    if (lv_scr == LV_SC_CTL) lv_dirty_ctl = true;
    lv_paint_overlays(false);
}
static void lv_press(LvRect r)
{
    if (lv_flash_r.x1 >= lv_flash_r.x0) { const LvRect o = lv_flash_r; lv_flash_r = {0, 0, -1, -1}; lv_flash_repaint(o); }
    lv_flash_r = r; lv_flash_t = lv_now;
    lv_flash_repaint(r);
}
/* Issue 1: a theme tap on CONTROLbot used to lv_go(LV_SC_CTL) + lv_press(): the CTL clip restarted (full keyframe
 * pushed again, animation jumped to frame 0), every tint/overlay was reset and painted twice, and the press brighten
 * added one more intermediate frame. The CTL clip is theme-agnostic (one segment), so a theme change repaints in place:
 * only tints whose state changed (CFACE hue, CLABEL name) and the CTL overlays, each through the band buffer, once. */
static void lv_ctl_retheme(void)
{
    if (lv_flash_r.x1 >= lv_flash_r.x0) { const LvRect o = lv_flash_r; lv_flash_r = {0, 0, -1, -1}; lv_flash_repaint(o); }
    if (lv_play.shown < 0) return;            /* nothing on screen yet: lv_tick paints everything once */
    lv_paint_tints(false);
    lv_dirty_ctl = lv_dirty_term = true;
    lv_paint_overlays(false);
}
static void lv_go_soon(int scr, const char *tty_cmd)   /* navigate after the flash has been seen (~110 ms) */
{
    lv_soon_kind = tty_cmd ? 2 : 1; lv_soon_scr = scr; lv_soon_t = lv_now;
    snprintf(lv_soon_cmd, sizeof lv_soon_cmd, "%s", tty_cmd ? tty_cmd : "");
}

/* public API */
/* cal_valid: a cv=3 8-point cal (cv 3: targets moved to face centres, forces one recal) is already in NVS (and was applied at display begin). flip: NVS ORIENT setting. */
/* Issue 2: on the multi themes (SYNTHWAVE / AURORA / PRISM / GLITCH) every overlay pixel drawn in the theme's
 * LV_ACC / LV_FRAME / LV_HDR / LV_ACCD sentinel takes the theme ink at its screen position on the way out. The
 * pixels are always lv_bandbuf or a line buffer of ours, consumed by this one blit, so they are re-inked in place. */
static lv_blit_fn lv_blit_hw;
static void lv_blit_theme(int x, int y, int w, int h, const uint16_t *px)
{
    if (lv_multi_theme(lv_theme)) {
        const int t = lv_theme;
        const uint16_t sa = lv_hex(LV_ACC[t]), sf = lv_hex(LV_FRAME[t]), sh = lv_hex(LV_HDR[t]), sd = lv_hex(LV_ACCD[t]);
        uint16_t *p = (uint16_t *)px;
        for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) {
            uint16_t &v = p[r * w + c];
            if (v != sa && v != sf && v != sh && v != sd) continue;
            const uint32_t T = lv_theme_at(t, x + c, y + r);
            v = (v == sh) ? lv_hex(lv_mix3(T, 0xffffff, 2, 5)) : (v == sd) ? lv_hex(lv_mix3(T, 0x000000, 59, 100)) : lv_hex(T);
        }
    }
    lv_blit_hw(x, y, w, h, px);
}
static void lv_begin(const LvRadio *radio, lv_blit_fn blit, lk_draw_fn raw_draw, int theme, uint32_t now, bool cal_valid)
{
    lv_radio = radio; lv_blit_hw = blit; lv_blit = lv_blit_theme; lv_raw_draw = raw_draw;
    lv_theme = (theme >= 0 && theme < LV_NTHEME) ? theme : 0;
    lv_now = lv_boot_ms = now; lv_clip_t = now; lv_auto_t = now;
    lv_print("BOTCORPS %s", LV_VERSION);
    lv_print("type 'help'. scans: tap SCAN or 'scan'");
    lv_cal_nvs = cal_valid; lv_cal_t0 = lv_cal_open = now; lv_cal_hold0 = 0;
    if (cal_valid) lv_cal_phase = LV_CAL_NVS; else lv_cal_start_run(now);
    lv_go(LV_SC_CAL);
}
static void lv_open_tty(const char *cmd);
/* call every ~30 ms. io.draw must be lv_clip_draw. */
static void lv_tick(const LkIo &io, uint32_t now)
{
    lv_now = now;
    if (lv_scr == LV_SC_CAL) { lv_cal_tick(now); return; }
    lv_job_tick();
    /* MULTI: auto re-scan */
    if (lv_job != LV_J_NONE) lv_auto_t = now;
    else if (lv_auto && (int32_t)(now - lv_auto_t) >= (lv_auto == 1 ? 30000 : 60000) && lv_scr != LV_SC_BOOT && lv_scr != LV_SC_HOME_INTRO) {
        lv_auto_t = now; lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1);
    }
    /* press flash end, then any deferred navigation */
    if (lv_flash_r.x1 >= lv_flash_r.x0 && (int32_t)(now - lv_flash_t) >= 130) { const LvRect o = lv_flash_r; lv_flash_r = {0, 0, -1, -1}; lv_flash_repaint(o); }
    if (lv_soon_kind && (int32_t)(now - lv_soon_t) >= 100) {
        const int k = lv_soon_kind; lv_soon_kind = 0;
        if (k == 1) lv_go(lv_soon_scr);
        else lv_open_tty(lv_soon_cmd[0] ? lv_soon_cmd : 0);
    }
    if (lv_scr == LV_SC_RADAR) {
        if ((int32_t)(now - lv_clip_t) >= 30) { lv_clip_t = now; lv_radar_frame(); }   /* every 30 ms loop tick: <= 33 fps, SPI-bound ~28 ms */
        if (lv_enter) { lv_enter = false; LvRect e = {0, 0, 239, 0}; lv_render(lv_blit, e, [](LvBand &B, void *) { b_fill(B, B.x0, B.y0, B.w, B.h, 0); }, 0);
            lv_render(lv_blit, {0, 0, 0, 23}, [](LvBand &B, void *) { b_fill(B, B.x0, B.y0, B.w, B.h, 0); }, 0); lv_render(lv_blit, {239, 0, 239, 23}, [](LvBand &B, void *) { b_fill(B, B.x0, B.y0, B.w, B.h, 0); }, 0); }
        lv_ticker_tick();
        return;
    }
    if (lv_scr != LV_SC_TTY && (int32_t)(now - lv_clip_t) >= lv_period()) {
        lv_clip_t = now;
        const bool first = lv_play.shown < 0;
        if (lv_motion || first || lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) lv_play.step(io);
        if (lv_play.done && lv_scr == LV_SC_BOOT) {
            lv_go(LV_SC_HOME_INTRO);
            if (lv_selftest_pending) { lv_selftest_pending = false; lv_selftest(); }
        }
        else if (lv_play.done && lv_scr == LV_SC_HOME_INTRO) lv_go(LV_SC_HOME);
        if (lv_scr == LV_SC_CTL) lv_dirty_ctl = true; /* uptime */
    }
    const bool all = lv_enter && (lv_scr == LV_SC_TTY || lv_play.shown >= 0);
    if (all) lv_enter = false;
    if ((lv_scr == LV_SC_TERM || lv_scr == LV_SC_HOME || lv_scr == LV_SC_CTL) && (now / 500) != ((now - 30) / 500)) lv_dirty_term = true;
    if (lv_scr == LV_SC_TTY && (now / 400) != ((now - 30) / 400)) lv_dirty_tty = true;
    if (lv_scr == LV_SC_TTY && (lv_hunt >= 0 || lv_hunt_last >= 0) && lv_job != LV_J_NONE && (now / 90) != ((now - 30) / 90)) lv_dirty_tty = true; /* banner sweep */
    if (lv_key_hl >= 0 && (int32_t)(now - lv_key_hl_t) > 120) { lv_key_hl = -1; lv_dirty_kbd = true; }
    if (lv_scr != LV_SC_BOOT && lv_scr != LV_SC_HOME_INTRO && (lv_scr == LV_SC_TTY || lv_play.shown >= 0)) {
        if (lv_scr != LV_SC_TTY) lv_paint_tints(false);
        lv_paint_overlays(all);
        lv_ticker_tick();
    }
}

static void lv_open_tty(const char *cmd)
{
    if (lv_scr != LV_SC_TTY) lv_tty_from = lv_scr;
    lv_go(LV_SC_TTY);
    if (cmd) lv_cmd(cmd);
}
static void lv_key(int k)
{
    lv_key_hl = k; lv_key_hl_t = lv_now; lv_dirty_kbd = true;
    if (k < 40) { if (lv_inlen < 62) { lv_inb[lv_inlen++] = (lv_shift ? LV_KROW_S : LV_KROW)[k / 10][k % 10]; lv_inb[lv_inlen] = 0; } }
    else if (k == LV_KEY_SHIFT) lv_shift = !lv_shift;
    else if (k == LV_KEY_SPACE) { if (lv_inlen < 62) { lv_inb[lv_inlen++] = ' '; lv_inb[lv_inlen] = 0; } }
    else if (k == LV_KEY_DEL) { if (lv_inlen) lv_inb[--lv_inlen] = 0; }
    else if (k == LV_KEY_CMDS) { lv_cs_open = true; lv_cs_page = 0; lv_cs_hl = -1; }
    else if (k == LV_KEY_ENTER) { char c[64]; snprintf(c, sizeof c, "%s", lv_inb); lv_inlen = 0; lv_inb[0] = 0; if (lv_shift) lv_shift = false; if (c[0]) lv_cmd(c); }
    lv_dirty_tty = true;
}
static void lv_scan_mix(void) { lv_mode = lv_scanmix; lv_cat = LV_CAT_GRID; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1); }
static void lv_scan_view(int mode, int job) { lv_mode = mode; lv_cat = LV_CAT_GRID; lv_page = 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; if (job != LV_J_NONE) lv_job_start(job, -1); }
/* one level up: SCAN detail -> sublist -> grid -> home; TTY -> where it was opened; others -> home */
static const char *lv_back(void)
{
    if (lv_scr == LV_SC_TTY) { lv_go_soon(lv_tty_from == LV_SC_TTY ? LV_SC_HOME : lv_tty_from, 0); return "backbot-tty"; }
    if (lv_scr == LV_SC_RADAR) { const int f = lv_rd_from; lv_go_soon(f == LV_SC_RADAR || f == LV_SC_BOOT || f == LV_SC_HOME_INTRO || f == LV_SC_CAL ? LV_SC_HOME : f, 0); return "backbot-radar"; }
    if (lv_scr == LV_SC_SCAN && lv_cat != LV_CAT_GRID) {
        if (lv_detail) { lv_detail = false; lv_dirty_box = true; return "backbot-detail->list"; }
        lv_cat = LV_CAT_GRID; lv_sel = -1; lv_dirty_box = true; return "backbot-list->grid";
    }
    lv_go_soon(LV_SC_HOME, 0); return "backbot-home";
}
#define LV_TAP(R) (lv_in(R, x, y) && (lv_press(R), true))
#define LV_TAPD(R, D) (lv_in(R, x, y) && (lv_press(D), true))   /* tap rect R, highlight only the drawn element D (item 5) */

/* item 11: the small terminal's button row + prompt. Returns the action name, or 0 if (x,y) is not on them. */
static const char *lv_mt_hit(int x, int y)
{
    int b = -1;
    for (int i = 0; i < lv_mt_n(); ++i) if (LV_TAPD(lv_mt_tap(i), lv_mt_btn(i))) b = i;
    if (b < 0) {
        if (!LV_TAPD(lv_mt_prompt_tap(), lv_mt_prompt_draw())) return 0;
        lv_go_soon(0, ""); return lv_scr == LV_SC_HOME ? "home-tty" : "tty";
    }
    switch (lv_scr) {
    case LV_SC_HOME:
        if (b == 0) { lv_scan_mix(); lv_go_soon(LV_SC_SCAN, 0); return "home-crosshair-scan"; }
        lv_go_soon(0, b == 1 ? "list" : "help"); return b == 1 ? "home-list" : "home-menu-help";
    case LV_SC_TERM:
        lv_go_soon(0, b == 0 ? "scan" : b == 1 ? "list" : "help"); return b == 0 ? "term-crosshair-scan" : b == 1 ? "term-list" : "term-menu-help";
    case LV_SC_SCAN:
        if (b == 0) { lv_scan_view(0, LV_J_ALL); return "crosshair-scan-all"; }
        lv_go_soon(0, b == 1 ? "list" : "help"); return b == 1 ? "list" : "menu-help";
    default:   /* CTL */
        if (b == 0) { lv_go_soon(0, "go ble"); return "go-ble"; }
        if (b == 1) { lv_go_soon(LV_SC_SCAN, 0); return "go-scan"; }
        if (b == 2) { lv_scan_mix(); lv_go_soon(LV_SC_SCAN, 0); return "ctl-crosshair-scan"; }
        lv_go_soon(0, b == 3 ? "list" : "help"); return b == 3 ? "ctl-list" : "ctl-menu-help";
    }
}

/* Touch padding (hit-list item 1): a tap that lands in no target but within LV_TAP_PAD px of one or more is
 * moved onto the target whose centre is nearest, so every target grows ~8 px without any two overlapping.
 * Draw rects are untouched; only the tap point moves. */
#define LV_TAP_PAD 8
/* the terminal-box rows below mirror lv_mt_tap / lv_mt_prompt_tap (T = 272; SCAN T = 275) */
static const LvRect LV_PAD_HOME[] = {{31, 52, 74, 112}, {78, 30, 160, 150}, {165, 51, 208, 110}, {33, 156, 101, 255}, {139, 156, 207, 255},
                                     {30, 262, 90, 295}, {91, 262, 147, 295}, {148, 262, 206, 295}, {30, 296, 206, 312}};
static const LvRect LV_PAD_CTL[] = {{35, 33, 113, 48}, {35, 49, 113, 63}, {35, 64, 113, 79}, {165, 175, 207, 237}, {160, 238, 208, 258},
                                    {35, 85, 70, 140}, {35, 145, 70, 190}, {35, 195, 70, 250}, {165, 80, 205, 130}, {170, 131, 205, 170},
                                    {90, 212, 159, 258}, {30, 262, 68, 295}, {69, 262, 102, 295}, {103, 262, 136, 295}, {137, 262, 170, 295},
                                    {171, 262, 206, 295}, {30, 296, 206, 312}, {75, 82, 164, 205}};
static const LvRect LV_PAD_TERM[] = {{35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
                                     {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250},
                                     {30, 262, 90, 295}, {91, 262, 147, 295}, {148, 262, 206, 295}, {30, 296, 206, 312}, {75, 30, 165, 150}};
static const LvRect LV_PAD_SCAN[] = {{32, 30, 74, 92}, {32, 93, 74, 152}, {78, 30, 165, 150}, {166, 58, 208, 122}, {174, 165, 203, 206},
                                     {174, 210, 203, 252}, {33, 153, 206, 256}, {28, 265, 90, 298}, {91, 265, 148, 298}, {149, 265, 208, 298},
                                     {28, 299, 208, 316}};
static void lv_pad_snap(const LvRect *R, int n, int &x, int &y)
{
    int best = -1; long bd = 0;
    for (int i = 0; i < n; ++i) {
        if (lv_in(R[i], x, y)) return;   /* direct hit: never moved */
        const LvRect p = {(int16_t)(R[i].x0 - LV_TAP_PAD), (int16_t)(R[i].y0 - LV_TAP_PAD), (int16_t)(R[i].x1 + LV_TAP_PAD), (int16_t)(R[i].y1 + LV_TAP_PAD)};
        if (!lv_in(p, x, y)) continue;
        const long dx = 2 * x - (R[i].x0 + R[i].x1), dy = 2 * y - (R[i].y0 + R[i].y1), d = dx * dx + dy * dy;
        if (best < 0 || d < bd) { best = i; bd = d; }
    }
    if (best < 0) return;
    const LvRect &r = R[best];
    x = x < r.x0 ? r.x0 : x > r.x1 ? r.x1 : x;
    y = y < r.y0 ? r.y0 : y > r.y1 ? r.y1 : y;
}
#define LV_PAD(T) lv_pad_snap(T, (int)(sizeof T / sizeof T[0]), x, y)

/* Touch state every loop (portrait coords, down = finger on glass). Only the radar uses it (hold to lock on). */
static void lv_touch(bool down, int x, int y) { if (lv_scr == LV_SC_RADAR) lv_radar_touch(down, x, y); }

/* Tap at portrait (x,y). Returns a short name of what it did (for logs / the sim). */
static const char *lv_tap(int x, int y)
{
    if (lv_scr == LV_SC_CAL) return "cal-raw-only";
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) { lv_go(LV_SC_HOME); if (lv_selftest_pending) { lv_selftest_pending = false; lv_selftest(); } return "skip-intro"; }
    if (lv_soon_kind) return "busy-nav";
    if (lv_scr == LV_SC_TTY) {
        if (LV_TAP(LV_R_BACKBOT_TTY)) return lv_back();
        if (lv_in(LV_R_TTY_TICK, x, y)) { lv_go(LV_SC_SCAN); return "ticker->scan"; }
        if (y >= LV_KBD_Y && lv_cs_open) {   /* every pixel of the overlay is a button: header thirds, rows full width */
            lv_dirty_kbd = true;
            if (y < LV_KBD_Y + LV_CS_HDR) {
                if (x < 80) { lv_cs_open = false; return "cmds-back"; }
                const int np = lv_cs_npage();
                if (x < 160) { if (lv_cs_page > 0) --lv_cs_page; return "cmds-prev"; }
                if (lv_cs_page < np - 1) ++lv_cs_page; return "cmds-next";
            }
            const int n = lv_cs_first() + (y - LV_KBD_Y - LV_CS_HDR) / LV_CS_RH;
            if (n >= LV_NCS) return "cmds-empty";
            if (lv_inlen && lv_inb[lv_inlen - 1] != ' ' && lv_inlen < 62) { lv_inb[lv_inlen++] = ' '; lv_inb[lv_inlen] = 0; }
            lv_inlen += snprintf(lv_inb + lv_inlen, 63 - lv_inlen, "%s%s", LV_CS[n].cmd, LV_CS[n].arg ? " " : "");
            if (lv_inlen > 62) lv_inlen = 62;
            lv_cs_open = false; lv_dirty_tty = true; return "cmds-insert";
        }
        if (y >= LV_KBD_Y) { const int k = lv_key_at(x, y); lv_key(k); return "key"; }
        if (lv_in(LV_R_TTY_OUT, x, y)) {
            const int total = lv_tn < LV_TLINES ? lv_tn : LV_TLINES, mx = total > LV_TTY_LINES ? total - LV_TTY_LINES : 0;
            lv_scroll += (y < 98) ? 6 : -6;
            if (lv_scroll > mx) lv_scroll = mx;
            if (lv_scroll < 0) lv_scroll = 0;
            lv_dirty_tty = true; return y < 98 ? "scroll-up" : "scroll-down";
        }
        return "none";
    }
    if (lv_scr != LV_SC_HOME && LV_TAP(LV_R_BACKBOT_HIT)) return lv_back();
    if (lv_scr == LV_SC_RADAR && y > 23) return y >= LV_RD_Y0 ? lv_radar_tap(x, y) : "none";
    /* ticker (full width now: crown + GIBSON icon removed) -> SCANbot, all finds */
    if (y <= 23) { lv_press(LV_R_TICK); lv_mode = 0; lv_cat = LV_CAT_GRID; lv_dirty_box = true; if (lv_scr != LV_SC_SCAN) lv_go_soon(LV_SC_SCAN, 0); return "ticker->scan"; }
    if (lv_scr == LV_SC_HOME) LV_PAD(LV_PAD_HOME);
    else if (lv_scr == LV_SC_CTL) LV_PAD(LV_PAD_CTL);
    else if (lv_scr == LV_SC_TERM) LV_PAD(LV_PAD_TERM);
    else if (lv_scr == LV_SC_SCAN) LV_PAD(LV_PAD_SCAN);
    if (lv_scr == LV_SC_HOME) {
        if (LV_TAP(LV_HOME_CHIP[0])) { lv_selftest_pending = true; lv_go_soon(LV_SC_BOOT, 0); return "boot-selftest"; }
        if (LV_TAP(LV_HOME_CHIP[1])) { lv_go_soon(LV_SC_RADAR, 0); return "gibson-jackin-radar"; }
        if (LV_TAP(LV_HOME_CHIP[2])) { lv_go_soon(LV_SC_CTL, 0); return "ctl"; }
        if (LV_TAP(LV_HOME_CHIP[3])) { lv_go_soon(LV_SC_SCAN, 0); return "scan"; }
        if (LV_TAP(LV_HOME_CHIP[4])) { lv_go_soon(LV_SC_TERM, 0); return "term"; }
        if (const char *a = lv_mt_hit(x, y)) return a;
        return "none";
    }
    if (lv_scr == LV_SC_TERM) {
        static const LvRect HOT[LV_NHUNT] = {{35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
                                             {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250}};
        for (int h = 0; h < LV_NHUNT; ++h)
            if (LV_TAP(HOT[h])) { char c[24]; snprintf(c, sizeof c, "sweep %s", LV_HUNT_NAME[h]); lv_go_soon(0, c); return LV_HUNT_NAME[h]; }
        if (const char *a = lv_mt_hit(x, y)) return a;
        if (LV_TAP(((LvRect){75, 30, 165, 150}))) { lv_go_soon(0, ""); return "tty"; }
        return "none";
    }
    if (lv_scr == LV_SC_SCAN) {
        if (LV_TAP(((LvRect){32, 30, 74, 92}))) { lv_scan_view(1, LV_J_WIFI); return "wifi-chip-scan"; }
        if (LV_TAP(((LvRect){32, 93, 74, 152}))) { lv_scan_view(2, LV_J_BLE); return "ble-chip-scan"; }
        if (LV_TAP(((LvRect){78, 30, 165, 150}))) { if (lv_job != LV_J_NONE) { lv_job_stop(); return "scanbot-stop"; } lv_scan_mix(); return "scanbot-scan-mix"; }
        if (LV_TAP(((LvRect){166, 58, 208, 122}))) { lv_go_soon(LV_SC_HOME, 0); return "gib-home"; }
        if (LV_TAP(LV_R_BTN_SCAN)) {
            if (lv_job != LV_J_NONE) { lv_job_stop(); return "stop"; }
            if (lv_cat == LV_CAT_DEAUTH) { lv_mode = 3; lv_job_start(LV_J_DEAUTH, LV_H_DEAUTH); return "deauth-watch"; }
            lv_job_start(lv_mode == 1 ? LV_J_WIFI : lv_mode == 2 ? LV_J_BLE : LV_J_ALL, -1); return "scan-button";
        }
        if (LV_TAP(LV_R_BTN_MORE)) {
            if (lv_cat == LV_CAT_DEAUTH) { lv_cat = LV_CAT_GRID; lv_dirty_box = true; return "deauth-back"; }
            if (lv_cat >= 0 && lv_detail) { int r[LV_MAXF]; const int n = lv_cat_rows(r); if (lv_sel >= 0 && lv_sel < n) { lv_rd_hunt_open(r[lv_sel], LV_SC_SCAN); return "detail-hunt"; } }
            if (lv_cat >= 0) { int r[LV_MAXF]; const int n = lv_cat_rows(r); lv_cpage = n > 8 ? (lv_cpage + 1) % ((n + 7) / 8) : 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; return "more-rows"; }
            LvCatInfo c[LV_IC_N]; const int n = lv_cats(c); lv_page = n > 11 ? (lv_page + 1) % ((n + 10) / 11) : 0; lv_dirty_box = true; return "more-types";
        }
        if (lv_in(LV_R_SBOX, x, y)) {
            if (lv_cat == LV_CAT_DEAUTH) return "none";
            if (lv_cat >= 0) {
                lv_press({34, 154, 172, 255});
                int r[LV_MAXF]; const int n = lv_cat_rows(r);
                if (lv_detail) { lv_detail = false; lv_dirty_box = true; return "detail->list"; }
                if (y < 174) return "none";
                const int k = lv_cpage * 8 + (y - 174) / 10;
                if ((y - 174) / 10 >= 8 || k >= n) return "empty-row";
                if (lv_sel == k) { lv_detail = true; lv_dirty_box = true; return "detail"; }
                lv_sel = k; lv_dirty_box = true;
                lv_full_line(lv_f[r[k]], 0, 0, lv_tk_msg, sizeof lv_tk_msg); lv_tk_alert = lv_f[r[k]].alert; lv_tk_off = 0;
                return "row-select";
            }
            for (int t = 0; t < 12; ++t) {
                const LvRect tr = {(int16_t)(lv_tile_x(t) - 1), (int16_t)(lv_tile_y(t) - 1), (int16_t)(lv_tile_x(t) + 31), (int16_t)(lv_tile_y(t) + 28)};
                if (!lv_in(tr, x, y)) continue;
                lv_press({(int16_t)lv_tile_x(t), (int16_t)lv_tile_y(t), (int16_t)(lv_tile_x(t) + 30), (int16_t)(lv_tile_y(t) + 27)});
                if (t == 11) { lv_cat = LV_CAT_DEAUTH; lv_mode = 3; lv_dirty_box = true; if (lv_job == LV_J_NONE) lv_job_start(LV_J_DEAUTH, LV_H_DEAUTH); return "deauth-tile"; }
                LvCatInfo c[LV_IC_N]; const int n = lv_cats(c); const int k = lv_page * 11 + t;
                if (k >= n) return "empty-tile";
                lv_cat = c[k].icon; lv_cpage = 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true;
                return "category";
            }
            return "none";
        }
        if (const char *a = lv_mt_hit(x, y)) return a;
        return "none";
    }
    if (lv_scr == LV_SC_CTL) {
        auto ev = [](const char *n, const char *v) { lv_event(false, "%s: %s", n, v); lv_tk_off = 1 << 14; lv_dirty_ctl = true; };
        if (lv_in(((LvRect){35, 33, 113, 48}), x, y)) { if (lv_theme_bounce()) return "theme-debounce"; /* no lv_press: the new colour is the feedback */ lv_set_theme(lv_theme + 1); lv_ctl_retheme(); ev("THEME", LV_THEME_NAME[lv_theme]); return "theme"; }
        if (LV_TAP(((LvRect){35, 49, 113, 63}))) { char h[48] = ""; lv_radio->sys(0, h, sizeof h); lv_event(false, "UP %lus  %s", (unsigned long)(lv_now / 1000), h); lv_tk_off = 1 << 14; return "uptime"; }
        if (LV_TAP(((LvRect){35, 64, 113, 79}))) { lv_focus = (lv_focus + 1) % 4; lv_tk_item = 0; ev("TICKER FOCUS", LV_FOCUS_NAME[lv_focus]); return "focus"; }
        if (LV_TAP(((LvRect){165, 175, 207, 237}))) { lv_auto = (lv_auto + 1) % 3; lv_auto_t = lv_now; ev("MULTI AUTO SCAN", LV_AUTO_NAME[lv_auto]); return "multi"; }
        if (LV_TAP(((LvRect){160, 238, 208, 258}))) { lv_motion = !lv_motion; ev("BG MOTION", lv_motion ? "ON" : "OFF"); return "bg-motion"; }
        if (LV_TAP(((LvRect){35, 85, 70, 140}))) { lv_scanmix = (lv_scanmix + 1) % 3; ev("MIX (SCAN)", LV_MIX_NAME[lv_scanmix]); return "mix"; }
#if LV_BG_ENGINE
        if (LV_TAP(((LvRect){35, 145, 70, 190}))) { lv_raincol = (lv_raincol + 1) % LV_NBGCOL; lv_bg_save(); ev("BG COLOR", LV_RAINCOL_NAME[lv_raincol]); return "rain-col"; }
#else
        if (LV_TAP(((LvRect){35, 145, 70, 190}))) { lv_raincol = (lv_raincol + 1) % 3; ev("RAIN COL", LV_RAINCOL_NAME[lv_raincol]); return "rain-col"; }
#endif
        if (LV_TAP(((LvRect){35, 195, 70, 250}))) { lv_speed = (lv_speed + 1) % 3; ev("MASHUP TICKER", LV_SPEED_NAME[lv_speed]); return "mashup"; }
#if LV_BG_ENGINE
        if (LV_TAP(((LvRect){165, 80, 205, 130}))) {   /* BG STYLE (temporary home on the RAIN chip): RAIN > BOTS > GIBSON > FLYBY > BOUNCE > OFF > RAIN */
            if (!lv_rain) { lv_rain = 1; lv_bgstyle = 0; } else if (lv_bgstyle + 1 < LV_NBGSTYLE) ++lv_bgstyle; else lv_rain = 0;
            lv_bg_save(); ev("BG STYLE", lv_bgstyle_name()); return "rain"; }
#else
        if (LV_TAP(((LvRect){165, 80, 205, 130}))) { lv_rain = !lv_rain; ev("RAIN", LV_ONOFF[lv_rain]); return "rain"; }
#endif
        if (LV_TAP(((LvRect){170, 131, 205, 170}))) { lv_rainmode = (lv_rainmode + 1) % 3; ev("RAIN MODE", LV_RAINMODE_NAME[lv_rainmode]); return "rain-mode"; }
        if (LV_TAP(((LvRect){90, 212, 159, 258}))) {
            lv_flip = !lv_flip; if (lv_radio->set_rot) lv_radio->set_rot(lv_flip);
            lv_go(LV_SC_CTL); lv_event(false, "ORIENT: %s", LV_FLIP_NAME[lv_flip]); lv_tk_off = 1 << 14; return "orient";
        }
        if (const char *a = lv_mt_hit(x, y)) return a;
        if (lv_in(((LvRect){75, 82, 164, 205}), x, y)) { if (lv_theme_bounce()) return "theme-debounce"; lv_set_theme(lv_theme + 1); lv_ctl_retheme(); ev("THEME", LV_THEME_NAME[lv_theme]); return "ctl-face-theme"; }
        return "none";
    }
    return "none";
}
