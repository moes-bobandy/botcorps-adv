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
#include <math.h>
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
    void (*set_flip)(bool flip);                 /* CONTROLbot ORIENT: false = USB at the bottom (rot 2), true = rot 0; persist */
};

enum LvScr { LV_SC_BOOT, LV_SC_HOME_INTRO, LV_SC_HOME, LV_SC_TERM, LV_SC_SCAN, LV_SC_CTL, LV_SC_TTY, LV_SC_CAL };
#define LV_NTHEME 6
static const char *const LV_THEME_NAME[LV_NTHEME] = {"GRAPE", "MAGENTA", "TOXIC", "ICE", "BLUE", "ORANGE"};
/* Theme inks sampled from the LOCKED SCANbot/TERMINALbot clips (frame 0 of each theme loop). */
static const uint32_t LV_ACC[LV_NTHEME] = {0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00};
static const uint32_t LV_ACCD[LV_NTHEME] = {0x53205d, 0x68004f, 0x4a6010, 0x00636d, 0x0e5166, 0x61480f};
static const uint32_t LV_HDR[LV_NTHEME] = {0xb36afe, 0xdc4aac, 0xb9fb4f, 0x4afffc, 0x6891ff, 0xf4b24c};
static const uint32_t LV_FRAME[LV_NTHEME] = {0xff25ae, 0xdf00ae, 0x92fd00, 0x00feff, 0xfd22ff, 0xfd6b00};
#define LV_PROMPT_INK 0xdd00a7
#define LV_TAN 0xb39152
#define LV_PROMPT "user@botcorps:~$"

/* ---------- geometry (native portrait 240x320, measured on the LOCKED clips) ---------- */
/* hit list item 3: the crown and the GIBSON chip icon are gone, the ticker owns the whole top bar inside the frame */
static const LvRect LV_R_TICK = {3, 1, 236, 23};
/* hit list item 4: BACK lives in the bottom-left corner strip under the frame's GND pin (x3..28, y306..316). It is
 * empty black on every locked clip (SCANbot's bottom-left text ends at x28, BOOTbot's board label starts at x16,
 * so BOOTbot gets the arrow-only button). Thumb-sized hit area {0,290,29,319}, no other target is there. */
static const LvRect LV_R_BACK = {3, 306, 28, 316}, LV_R_BACK_S = {3, 306, 14, 316}, LV_R_BACK_HIT = {0, 290, 29, 319};
static const LvRect LV_R_SCANLBL = {86, 133, 152, 143}; /* SCANbot centre plate label ("ALL RADIO") */
static const LvRect LV_R_CLABEL = {96, 189, 143, 199};  /* CONTROLbot chip label ("GRAPE") */
/* CONTROLbot value overlays (only painted while the value differs from the locked art) */
static const LvRect LV_R_CMIX = {34, 125, 72, 134}, LV_R_CRCOL = {36, 180, 68, 186}, LV_R_CMASH = {33, 238, 76, 246};
static const LvRect LV_R_CRAIN = {167, 118, 206, 126}, LV_R_CRMODE = {170, 156, 207, 162}, LV_R_CMULTI = {167, 215, 209, 223};
static const LvRect LV_R_CORIENT = {112, 219, 148, 225};
static const LvRect LV_R_BANNER = {0, 36, 239, 57};    /* TTY hunt banner (option A) */
static const LvRect LV_R_SBOX = {36, 188, 203, 253};  /* SCANbot results box inner */
static const LvRect LV_R_STERM = {32, 275, 182, 308}; /* SCANbot terminal box inner */
static const LvRect LV_R_TTERM = {34, 272, 181, 305}; /* TERMINALbot terminal box inner */
static const LvRect LV_R_CTHEME = {82, 36, 112, 44}, LV_R_CUP = {76, 51, 112, 59}, LV_R_CFOCUS = {82, 66, 112, 74};
static const LvRect LV_R_CMOTION = {178, 246, 198, 252};
static const LvRect LV_R_TTY_TICK = {0, 0, 195, 23}, LV_R_TTY_CLOSE = {196, 0, 239, 23};
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
static int lv_prev = LV_SC_TERM;   /* screen the TTY was opened from (BACK returns there) */
static bool lv_flip = false;       /* CONTROLbot ORIENT: true = 180 deg from the default USB-at-bottom */
static bool lv_rot180 = true;      /* panel rotation is 2 (USB at the bottom): CALbot maps corners through 180 deg */
/* CONTROLbot settings (0 = the value the locked art shows) */
static int lv_mix = 0;    /* MIX: what a plain scan runs: 0 WIFI+BLE 1 WIFI 2 BLE */
static int lv_rcol = 0;   /* RAIN COL: ticker colours 0 MULTI 1 THEME 2 MONO */
static int lv_rmode = 0;  /* RAIN MODE: ticker stripes 0 MIXED (scroll) 1 STILL 2 PLAIN */
static int lv_rain = 1;   /* RAIN: ticker speed 0 SLOW 1 NORM 2 FAST (art shows no value: NORM is default) */
static int lv_sortk = 0;  /* MULTI: result sort 0 SIGNAL 1 NAME 2 TYPE */
static int lv_mash = 0;   /* MASHUP: auto rescan 0 OFF 1 30 s 2 60 s 3 120 s */
static const char *const LV_MIX_NAME[3] = {"ALL", "WIFI", "BLE"};
static const char *const LV_RCOL_NAME[3] = {"MULTI", "THEME", "MONO"};
static const char *const LV_RMODE_NAME[3] = {"MIXED", "STILL", "PLAIN"};
static const char *const LV_RAIN_NAME[3] = {"SLOW", "NORM", "FAST"};
static const char *const LV_SORT_NAME[3] = {"SIGNAL", "NAME", "TYPE"};
static const uint16_t LV_MASH_S[4] = {0, 30, 60, 120};
static uint32_t lv_last_done = 0;
static int lv_mk_hunt = -1; static uint32_t lv_mk_t0 = 0; /* sticky hunt for the A banner / D ticker */
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
        for (int o = 0; o < len; o += LV_TTY_COLS) { char t[LV_TTY_COLS + 1]; strncpy(t, s + o, LV_TTY_COLS); t[LV_TTY_COLS] = 0; lv_tline(t); }
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
#define LV_TKN 200
static char lv_tk_msg[LV_TKN] = "NO THREATS - TAP SCAN";
static int lv_tk_off = 0, lv_tk_item = 0;
static bool lv_tk_alert = false;
static char lv_evq[4][LV_TKN]; static uint8_t lv_evalert[4]; static int lv_nev = 0;
static void lv_event(bool alert, const char *fmt, ...)
{
    if (lv_nev >= 4) { memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); lv_nev = 3; }
    va_list ap; va_start(ap, fmt); vsnprintf(lv_evq[lv_nev], LV_TKN, fmt, ap); va_end(ap);
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

/* hunt A banner / D ticker data (ported from the hunt-target mocks, option A + D) */
static const int LV_MK_ICON[LV_NHUNT] = {LV_IC_AP, LV_IC_GLASSES, LV_IC_LAPTOP, LV_IC_SKIMMER, LV_IC_CCTV, LV_IC_FLIPPER, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GALAXYTAG};
static const char *const LV_MK_BOT[LV_NHUNT] = {"DEAUTHbot", "METAbot", "HACKbot", "SKIMMERbot", "FLOCKbot", "FLIPPERbot", "DRONEbot", "AIRTAGbot", "KEYFOBbot", "SAMSUNGbot"};
static const char *const LV_MK_WHAT[LV_NHUNT] = {"DEAUTH FRAMES", "META GLASSES", "OPEN / WEP WIFI", "SERIAL SKIMMERS", "FLOCK CAMERAS", "FLIPPER ZERO", "DRONES", "FIND MY TAGS", "TILE TAGS", "SMARTTAGS"};
static void lv_upper(char *p) { for (; *p; ++p) if (*p >= 'a' && *p <= 'z') *p -= 32; }
/* D: one find's full info, only fields LvFind really carries. k/m = match k of m, seen = finds stored. */
static void lv_mk_full(const LvFind &f, int k, int m, int seen, char *o, int n)
{
    char l[34], mac[18]; lv_label(f, l, sizeof l); lv_macstr(f.mac, mac);
    char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[lv_mk_hunt < 0 ? 0 : lv_mk_hunt]); lv_upper(up);
    if (f.kind == LV_K_WIFI)
        snprintf(o, n, "!! WIFI: %s  CH%d  %ddBm  %s  BSSID %s  TYPE %s  %s MATCH %d/%d  %d SEEN !!", l, f.ch, f.rssi, lv_auth_name(f.auth), mac, LV_ICON_NAME[f.icon], up, k, m, seen);
    else {
        const char *mn = lv_mfg_name(f.mfg); char mf[24];
        if (f.mfg == 0xFFFF) snprintf(mf, sizeof mf, "NO MFG DATA"); else snprintf(mf, sizeof mf, "%s %04X", mn ? mn : "MFG", f.mfg);
        char sv[16] = ""; if (f.svc) snprintf(sv, sizeof sv, "  SVC %04X", f.svc);
        snprintf(o, n, "!! BLE: %s  %ddBm  %s%s%s  MAC %s  TYPE %s  %s MATCH %d/%d  %d SEEN !!", l, f.rssi, mf, (f.mfg == 0x004C && f.apple == 0x12) ? " FINDMY" : "", sv, mac, LV_ICON_NAME[f.icon], up, k, m, seen);
    }
}

/* ---------- store results ---------- */
/* true if a sorts after b: hunt matches first, then the CONTROLbot MULTI key (signal / name / type), then signal */
static bool lv_after(const LvFind &a, const LvFind &b)
{
    if (a.alert != b.alert) return a.alert < b.alert;
    if (lv_sortk == 1) {
        if (!a.name[0] != !b.name[0]) return !a.name[0];
        const int c = strcmp(a.name, b.name);
        if (c) return c > 0;
    }
    if (lv_sortk == 2 && a.icon != b.icon) return a.icon > b.icon;
    return a.rssi < b.rssi;
}
static void lv_sort(void)
{
    for (int i = 1; i < lv_nf; ++i) {
        LvFind t = lv_f[i]; int j = i - 1;
        while (j >= 0 && lv_after(lv_f[j], t)) { lv_f[j + 1] = lv_f[j]; --j; }
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
    lv_dirty_tty = true;
}
static int lv_mk_matches(int *idx = 0)
{
    int m = 0;
    for (int i = 0; i < lv_nf; ++i) if (lv_f[i].alert) { if (idx) idx[m] = i; ++m; }
    return m;
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
static const char *lv_job_name(int j) { return j == LV_J_WIFI ? "WIFI" : j == LV_J_BLE ? "BLE" : j == LV_J_ALL ? "WIFI+BLE" : j == LV_J_DEAUTH ? "DEAUTH WATCH" : "-"; }
static void lv_hunt_report(void);
static void lv_job_start(int job, int hunt)
{
    if (lv_job != LV_J_NONE) { lv_print("busy: %s scan running (stop)", lv_job_name(lv_job)); return; }
    lv_hunt = hunt;
    lv_mk_hunt = hunt; lv_mk_t0 = lv_now; lv_tk_item = 0;
    lv_job = job; lv_job_phase = 0; lv_job_t0 = lv_now;
    int r = 0;
    if (job == LV_J_WIFI || job == LV_J_ALL) { lv_print("> scan wifi: radio on, scanning"); r = lv_radio->wifi_start(); }
    else if (job == LV_J_BLE) { lv_print("> scan ble: NimBLE on, 5 s active scan"); r = lv_radio->ble_start(5000); }
    else if (job == LV_J_DEAUTH) { lv_print("> deauth watch: passive, 10 s, ch hop"); r = lv_radio->deauth_start(10000); lv_deauth_have = false; }
    if (r != 0) { lv_print("radio start failed (%d)", r); lv_job = LV_J_NONE; lv_event(true, "RADIO START FAILED"); }
    lv_tk_off = 1 << 14; /* show SCANNING now */
    lv_dirty_box = true;
}
static void lv_job_stop(void)
{
    if (lv_job == LV_J_NONE) return;
    lv_radio->stop();
    lv_print("stopped %s, radios off", lv_job_name(lv_job));
    lv_job = LV_J_NONE; lv_hunt = -1; lv_dirty_box = true; lv_dirty_tty = true; lv_last_done = lv_now;
}
static void lv_job_tick(void)
{
    if (lv_job == LV_J_NONE) return;
    if ((int32_t)(lv_now - lv_job_t0) > 20000) {   /* a radio that never reports done: stop it, radios off */
        lv_radio->stop();
        lv_print("%s scan timed out, radios off", lv_job_name(lv_job));
        lv_event(true, "%s SCAN TIMED OUT - RADIOS OFF", lv_job_name(lv_job));
        lv_job = LV_J_NONE; lv_hunt = -1; lv_dirty_box = lv_dirty_tty = true; lv_last_done = lv_now;
        return;
    }
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
            lv_hunt = -1; lv_dirty_box = true; lv_dirty_tty = true; lv_last_done = lv_now;
        }
        return;
    }
    const bool wifi_phase = (lv_job == LV_J_WIFI) || (lv_job == LV_J_ALL && lv_job_phase == 0);
    const int r = wifi_phase ? lv_radio->wifi_poll(lv_tmp, LV_MAXF) : lv_radio->ble_poll(lv_tmp, LV_MAXF);
    if (r < 0) return;
    lv_store(wifi_phase ? LV_K_WIFI : LV_K_BLE, lv_tmp, r);
    lv_print("%s: %d found (%lu ms), radio off", wifi_phase ? "wifi" : "ble", r, (unsigned long)(lv_now - lv_job_t0));
    if (lv_job == LV_J_ALL && lv_job_phase == 0) {
        lv_job_phase = 1; lv_job_t0 = lv_now;
        lv_print("> scan ble: NimBLE on, 5 s active scan");
        if (lv_radio->ble_start(5000) != 0) { lv_print("ble start failed"); lv_job = LV_J_NONE; }
        lv_dirty_box = true;
        return;
    }
    lv_job = LV_J_NONE;
    int nw = 0, nb = 0;
    for (int i = 0; i < lv_nf; ++i) (lv_f[i].kind == LV_K_WIFI ? nw : nb)++;
    if (lv_hunt >= 0) lv_hunt_report();
    else lv_event(false, "SCAN DONE: %d WIFI  %d BLE", nw, nb);
    lv_hunt = -1; lv_last_done = lv_now;
    lv_dirty_box = true; lv_dirty_tty = true;
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
    if (m) lv_event(true, "%s: %d FOUND NEARBY", up, m);
    else lv_event(false, "NO %s FOUND (%d DEVICES SEEN)", up, lv_nf);
    lv_tk_item = 0; /* then the ticker rotates every match's full line (option D) */
}

/* ---------- commands ---------- */
static void lv_go(int s);
static void lv_back(void);
static void lv_set_theme(int t)
{
    lv_theme = ((t % LV_NTHEME) + LV_NTHEME) % LV_NTHEME;
    if (lv_radio->save_theme) lv_radio->save_theme(lv_theme);
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = true;
    if (lv_scr == LV_SC_HOME || lv_scr == LV_SC_TERM || lv_scr == LV_SC_SCAN || lv_scr == LV_SC_CTL) lv_go(lv_scr); /* repaint in the new theme */
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
    "hunt <dev>   deauth|sniff   stop",
    "list [wifi|ble]   info <n>",
    "filter <all|wifi|ble>",
    "go <home|scan|term|ctl|boot|wifi|ble>",
    "theme [next|name|1-6]  history  clear",
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
    else if (IS("hunt")) {
        int h = -1;
        for (int k = 0; a1 && k < LV_NHUNT; ++k) if (!strcmp(a1, LV_HUNT_NAME[k]) || (!strcmp(a1, "air") && k == LV_H_AIRTAG)) h = k;
        if (h < 0) { lv_print("hunt what? airtag samsung flipper flock"); lv_print("  drone meta skimmer hack keyfob deauth"); }
        else { lv_print("hunt %s: %s", LV_HUNT_NAME[h], LV_HUNT_DESC[h]); lv_job_start(LV_HUNT_JOB[h], h); }
    }
    else if (IS("list") || IS("ls")) lv_list(!a1 ? 0 : !strcmp(a1, "wifi") ? 1 : 2);
    else if (IS("info")) { if (a1) lv_info(atoi(a1)); else lv_sysinfo(); }
    else if (IS("filter")) { lv_mode = !a1 ? 0 : !strcmp(a1, "wifi") ? 1 : (!strcmp(a1, "ble") || !strcmp(a1, "bt")) ? 2 : 0; lv_page = 0; lv_sel = -1; lv_dirty_box = true; lv_print("SCANbot view: %s", lv_mode == 1 ? "WIFI" : lv_mode == 2 ? "BT" : "ALL"); }
    else if (IS("clear") || IS("cls")) { lv_mk_hunt = -1; lv_tn = 0; lv_scroll = 0; lv_dirty_term = lv_dirty_tty = true; }
    else if (IS("history")) { for (int i = 0; i < lv_nhist; ++i) lv_print("%d %s", i + 1, lv_hist[i]); }
    else if (IS("theme")) {
        int t = -1;
        for (int k = 0; a1 && k < LV_NTHEME; ++k) { char lo[12]; snprintf(lo, sizeof lo, "%s", LV_THEME_NAME[k]); for (char *c = lo; *c; ++c) *c += 32; if (!strcmp(a1, lo)) t = k; }
        if (a1 && t < 0 && a1[0] >= '1' && a1[0] <= '6') t = a1[0] - '1';
        lv_set_theme(t >= 0 ? t : lv_theme + 1);
        lv_print("theme %d/6 %s", lv_theme + 1, LV_THEME_NAME[lv_theme]);
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
        else if (!strcmp(a1, "boot")) { lv_selftest_pending = true; lv_go(LV_SC_BOOT); }
        else if (!strcmp(a1, "wifi")) lv_job_start(LV_J_WIFI, -1);
        else if (!strcmp(a1, "ble") || !strcmp(a1, "bt")) lv_job_start(LV_J_BLE, -1);
        else lv_print("go: unknown '%s'", a1);
    }
    else if (IS("home")) lv_go(LV_SC_HOME);
    else if (IS("back") || IS("exit") || IS("close") || IS("quit")) lv_back();
    else if (IS("rain") || IS("ping") || IS("track") || IS("load")) lv_print("%s: not in the livescan build", a0);
    else lv_print("unknown: %s (try help)", a0);
#undef IS
}

/* ---------- clip: masks, FX (lit chips / tabs / CONTROLbot theme), press flash, region repaint ---------- */
static lk_draw_fn lv_raw_draw;
static lv_blit_fn lv_hw_blit;
static const LkIo *lv_io = 0;
static LvRect lv_masks[20];
static int lv_nmask = 0;
static bool lv_dirty_back = true, lv_dirty_dyn = true, lv_dirty_banner = false;
/* value overlays drawn only while their setting is not the default (default = untouched locked art) */
enum { LV_D_SCANLBL, LV_D_CLABEL, LV_D_CMIX, LV_D_CRCOL, LV_D_CMASH, LV_D_CRAIN, LV_D_CRMODE, LV_D_CMULTI, LV_D_CORIENT, LV_ND };
static const LvRect *const LV_DYN_R[LV_ND] = {&LV_R_SCANLBL, &LV_R_CLABEL, &LV_R_CMIX, &LV_R_CRCOL, &LV_R_CMASH, &LV_R_CRAIN, &LV_R_CRMODE, &LV_R_CMULTI, &LV_R_CORIENT};
static uint32_t lv_dyn = 0;
static uint32_t lv_dyn_want(void)
{
    uint32_t d = 0;
    if (lv_scr == LV_SC_SCAN && (lv_job != LV_J_NONE || lv_mode != 0)) d |= 1u << LV_D_SCANLBL;
    if (lv_scr == LV_SC_CTL) {
        if (lv_theme) d |= 1u << LV_D_CLABEL;
        if (lv_mix) d |= 1u << LV_D_CMIX;
        if (lv_rcol) d |= 1u << LV_D_CRCOL;
        if (lv_mash) d |= 1u << LV_D_CMASH;
        if (lv_rain != 1) d |= 1u << LV_D_CRAIN;
        if (lv_rmode) d |= 1u << LV_D_CRMODE;
        if (lv_sortk) d |= 1u << LV_D_CMULTI;
        if (lv_flip) d |= 1u << LV_D_CORIENT;
    }
    return d;
}
/* FX slots on SCANbot: the two radio chips and the four view tabs */
enum { LV_FX_NONE, LV_FX_LIT, LV_FX_GLOW, LV_FX_TABON, LV_FX_TABOFF };
#define LV_NFX 6
static const LvRect LV_FX_R[LV_NFX] = {{32, 37, 72, 88}, {32, 100, 72, 150}, {34, 155, 71, 182}, {78, 155, 115, 182}, {122, 155, 159, 182}, {166, 155, 203, 182}};
static uint8_t lv_fxk[LV_NFX];
static bool lv_wifi_busy(void) { return lv_job == LV_J_WIFI || (lv_job == LV_J_ALL && lv_job_phase == 0) || lv_job == LV_J_DEAUTH; }
static bool lv_ble_busy(void) { return lv_job == LV_J_BLE || (lv_job == LV_J_ALL && lv_job_phase == 1); }
static void lv_fx_want(uint8_t *k)
{
    memset(k, 0, LV_NFX);
    if (lv_scr != LV_SC_SCAN) return;
    bool hw = lv_deauth_have, hb = false;
    for (int i = 0; i < lv_nf; ++i) (lv_f[i].kind == LV_K_WIFI ? hw : hb) = true;
    const bool pulse = (lv_now / 350) & 1;
    k[0] = lv_wifi_busy() ? (pulse ? LV_FX_LIT : LV_FX_GLOW) : hw ? LV_FX_LIT : LV_FX_NONE;
    k[1] = lv_ble_busy() ? (pulse ? LV_FX_LIT : LV_FX_GLOW) : hb ? LV_FX_LIT : LV_FX_NONE;
    if (lv_mode) { k[2] = LV_FX_TABOFF; k[2 + lv_mode] = LV_FX_TABON; }
}
static LvRect lv_fl = {0, 0, -1, -1};
static bool lv_fl_on = false;
static uint32_t lv_fl_t = 0;
static LvRect lv_rp; static bool lv_rp_on = false;
static uint16_t lv_cpal[256]; static int lv_cpal_key = -1;

static inline void lv_un(uint16_t c, int &r, int &g, int &b) { r = (c >> 11) * 255 / 31; g = ((c >> 5) & 63) * 255 / 63; b = (c & 31) * 255 / 31; }
static inline uint16_t lv_flash_px(uint16_t c) { int r, g, b; lv_un(c, r, g, b); return lv_rgb(r + (255 - r) * 3 / 8, g + (255 - g) * 3 / 8, b + (255 - b) * 3 / 8); }
static void lv_hsv(int r, int g, int b, float &h, float &s, float &v)
{
    const int mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    v = mx / 255.f; s = mx ? (mx - mn) / (float)mx : 0; const float d = (float)(mx - mn);
    if (mx == mn) h = 0; else if (mx == r) h = 60.f * ((g - b) / d); else if (mx == g) h = 60.f * ((b - r) / d) + 120; else h = 60.f * ((r - g) / d) + 240;
    if (h < 0) h += 360;
}
static uint16_t lv_from_hsv(float h, float s, float v)
{
    const float c = v * s, hh = h / 60.f, x = c * (1 - fabsf(fmodf(hh, 2.f) - 1)), m = v - c;
    float r = 0, g = 0, b = 0; const int i = (int)hh % 6;
    if (i == 0) { r = c; g = x; } else if (i == 1) { r = x; g = c; } else if (i == 2) { g = c; b = x; } else if (i == 3) { g = x; b = c; } else if (i == 4) { r = x; b = c; } else { r = c; b = x; }
    return lv_rgb((uint8_t)((r + m) * 255), (uint8_t)((g + m) * 255), (uint8_t)((b + m) * 255));
}
static float lv_hue_of(uint32_t hex) { float h, s, v; lv_hsv(hex >> 16 & 255, hex >> 8 & 255, hex & 255, h, s, v); return h; }
/* CONTROLbot theme: the GRAPE-baked purples take the theme accent hue, the pinks the theme frame hue */
static uint16_t lv_theme_px(uint16_t c)
{
    if (!lv_theme) return c;
    int r, g, b; lv_un(c, r, g, b); float h, s, v; lv_hsv(r, g, b, h, s, v);
    if (s < 0.35f || v < 0.15f) return c;
    if (h >= 250 && h < 305) return lv_from_hsv(lv_hue_of(LV_ACC[lv_theme]), s, v);
    if (h >= 305 && h < 345) return lv_from_hsv(lv_hue_of(LV_FRAME[lv_theme]), s, v);
    return c;
}
static void lv_cpal_sync(void)
{
    const int c = lk_clip_of_seg(lv_play.seg), key = c * 8 + lv_theme;
    if (key == lv_cpal_key) return;
    lv_cpal_key = key;
    for (int i = 0; i < LK_CLIP[c].npal && i < 256; ++i) lv_cpal[i] = lv_theme_px(LK_CLIP[c].pal[i]);
}
static uint16_t lv_fx_px(int slot, int x, int y, uint16_t c)
{
    const LvRect &R = LV_FX_R[slot];
    int r, g, b; lv_un(c, r, g, b);
    const int mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b), L = (r * 3 + g * 6 + b) / 10;
    const bool edge = x == R.x0 || x == R.x1 || y == R.y0 || y == R.y1;
    const uint32_t a = LV_ACC[lv_theme];
    switch (lv_fxk[slot]) {
    case LV_FX_LIT: case LV_FX_GLOW: {
        if (mx - mn > 40 || L < 45) return c;               /* gold pins, coloured art and black stay */
        int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255;
        int nr = ar * L / 150, ng = ag * L / 150, nb = ab * L / 150;
        if (L > 190) { const int t = (L - 190) * 255 / 65; nr += (255 - nr) * t / 255; ng += (255 - ng) * t / 255; nb += (255 - nb) * t / 255; }
        nr = nr > 255 ? 255 : nr; ng = ng > 255 ? 255 : ng; nb = nb > 255 ? 255 : nb;
        if (lv_fxk[slot] == LV_FX_GLOW) { nr = (nr + r) / 2; ng = (ng + g) / 2; nb = (nb + b) / 2; }
        return lv_rgb(nr, ng, nb);
    }
    case LV_FX_TABON: return edge ? lv_hex(0xffffff) : L > 150 ? 0x0000 : lv_hex(a);
    case LV_FX_TABOFF: return edge ? lv_hex(0x293442) : L < 60 ? lv_hex(0xffffff) : 0x0000;
    }
    return c;
}
static void lv_set_masks(void)
{
    lv_nmask = 0;
    const bool bar = lv_scr != LV_SC_BOOT && lv_scr != LV_SC_HOME_INTRO && lv_scr != LV_SC_CAL && lv_scr != LV_SC_TTY;
    if (bar) lv_masks[lv_nmask++] = LV_R_TICK;
    if (lv_scr >= LV_SC_HOME && lv_scr <= LV_SC_CTL) lv_masks[lv_nmask++] = LV_R_BACK;  /* item 4: back only on the 4 navigable screens, never on BOOT/INTRO/CAL/TTY */
    if (lv_scr == LV_SC_SCAN) { lv_masks[lv_nmask++] = LV_R_SBOX; lv_masks[lv_nmask++] = LV_R_STERM; }
    if (lv_scr == LV_SC_TERM) lv_masks[lv_nmask++] = LV_R_TTERM;
    if (lv_scr == LV_SC_CTL) { lv_masks[lv_nmask++] = LV_R_CTHEME; lv_masks[lv_nmask++] = LV_R_CUP; lv_masks[lv_nmask++] = LV_R_CFOCUS; lv_masks[lv_nmask++] = LV_R_CMOTION; }
    for (int d = 0; d < LV_ND; ++d) if (lv_dyn >> d & 1) lv_masks[lv_nmask++] = *LV_DYN_R[d];
}
static void lv_clip_draw(int y, int x, int n, const uint8_t *idx)
{
    if (lv_rp_on && (y < lv_rp.y0 || y > lv_rp.y1 || x > lv_rp.x1 || x + n - 1 < lv_rp.x0)) return;
    const bool remap = lv_scr == LV_SC_CTL && lv_theme != 0;
    bool row = remap || lv_rp_on || (lv_fl_on && y >= lv_fl.y0 && y <= lv_fl.y1);
    for (int m = 0; !row && m < lv_nmask; ++m) if (y >= lv_masks[m].y0 && y <= lv_masks[m].y1) row = true;
    for (int f = 0; !row && f < LV_NFX; ++f) if (lv_fxk[f] && y >= LV_FX_R[f].y0 && y <= LV_FX_R[f].y1) row = true;
    if (!row) { lv_raw_draw(y, x, n, idx); return; }
    if (remap) lv_cpal_sync();
    const uint16_t *pal = remap ? lv_cpal : LK_CLIP[lk_clip_of_seg(lv_play.seg)].pal;
    static uint16_t px[240];
    int run = 0, rk = 0;                      /* rk: 0 hidden, 1 raw, 2 transformed colour */
    for (int i = 0; i <= n; ++i) {
        int k = 0;
        if (i < n) {
            const int xx = x + i;
            bool hid = lv_rp_on && !lv_in(lv_rp, xx, y);
            for (int m = 0; !hid && m < lv_nmask; ++m) if (lv_in(lv_masks[m], xx, y)) hid = true;
            if (!hid) {
                int f = -1;
                for (int q = 0; q < LV_NFX; ++q) if (lv_fxk[q] && lv_in(LV_FX_R[q], xx, y)) f = q;
                const bool fl = lv_fl_on && lv_in(lv_fl, xx, y);
                if (f >= 0 || fl || remap) {
                    uint16_t c = pal[idx[i]];
                    if (f >= 0) c = lv_fx_px(f, xx, y, c);
                    if (fl) c = lv_flash_px(c);
                    px[i] = c; k = 2;
                } else k = 1;
            }
        }
        if (i == n || k != rk) {
            if (rk == 1) lv_raw_draw(y, x + run, i - run, idx + run);
            else if (rk == 2) lv_hw_blit(x + run, y, i - run, 1, px + run);
            run = i; rk = k;
        }
    }
}
/* overlays: everything drawn with lv_blit brightens inside the press-flash rect */
static void lv_fx_blit(int x, int y, int w, int h, const uint16_t *px)
{
    if (lv_fl_on && !(x > lv_fl.x1 || x + w - 1 < lv_fl.x0 || y > lv_fl.y1 || y + h - 1 < lv_fl.y0)) {
        uint16_t *p = (uint16_t *)px;
        for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) if (lv_in(lv_fl, x + c, y + r)) p[r * w + c] = lv_flash_px(p[r * w + c]);
    }
    lv_hw_blit(x, y, w, h, px);
}
/* redraw region R of the clip as it is now (replays the shown frame clipped to R), then its overlays */
static void lv_repaint(const LvRect &R)
{
    if (lv_io && lv_play.seg >= 0 && lv_play.shown >= 0) {
        const LkSeg &S = LK_SEG[lv_play.seg];
        lv_rp = R; lv_rp_on = true;
        for (int k = 0; k <= lv_play.shown; ++k) lk_run_op(*lv_io, S.op0 + k);
        lv_rp_on = false;
    }
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = lv_dirty_back = lv_dirty_dyn = lv_dirty_banner = true;
}

/* ---------- overlay scenes ---------- */
static uint16_t lv_ink(const uint32_t *t) { return lv_hex(t[lv_theme]); } /* item 5: CONTROLbot follows the theme too */

/* ticker: stripes + scrolling bold text over [span0, span1]. CONTROLbot RAIN COL / RAIN MODE / RAIN restyle it. */
struct LvTickCtx { int span0, span1; };
static void lv_scene_ticker(LvBand &B, void *vctx)
{
    LvTickCtx *c = (LvTickCtx *)vctx;
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    uint16_t bg = lv_tk_alert ? (flash ? lv_hex(0x6a0000) : lv_hex(0x3a0000)) : lv_hex(0x2a0400);
    uint16_t s1 = lv_hex(flash ? 0xff2020 : 0xe00000), s2 = lv_hex(0xffb000);
    uint16_t ink = lv_tk_alert ? lv_hex(0xffffff) : lv_hex(0xffc830), sh = lv_hex(0x200000);
    if (lv_rcol == 1 && !lv_tk_alert) { bg = lv_ink(LV_ACCD) ; bg = lv_dim(bg, 1, 2); s1 = lv_ink(LV_ACC); s2 = lv_ink(LV_ACCD); ink = lv_ink(LV_HDR); sh = 0; }
    if (lv_rcol == 2) { bg = lv_tk_alert && flash ? lv_hex(0x303030) : lv_hex(0x101010); s1 = lv_hex(0xffffff); s2 = lv_hex(0x404040); ink = lv_hex(0xffffff); sh = 0; }
    b_fill(B, B.x0, B.y0, B.w, B.h, bg);
    const int ph = lv_rmode == 0 ? (int)(lv_now / 40) : 0;
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) {
            const bool red = lv_rmode == 2 ? (y == 2 || y == 22) : (((x + (y <= 3 ? y : -y) + ph) / 5) & 1);
            b_pix(B, x, y, red ? s1 : (lv_rmode == 2 ? bg : s2));
        }
    }
    const int tx = c->span1 + 1 - lv_tk_off;
    b_text(B, tx + 1, 6, lv_tk_msg, sh, 2);
    b_text(B, tx, 5, lv_tk_msg, ink, 2);
}
static int lv_tk_span0(void) { return lv_scr == LV_SC_TTY ? 0 : LV_R_TICK.x0; }
static int lv_tk_span1(void) { return lv_scr == LV_SC_TTY ? 195 : LV_R_TICK.x1; }
static void lv_tk_next(void)
{
    lv_tk_off = 0;
    int mi[LV_MAXF]; const int m = lv_mk_matches(mi);
    char up[16] = ""; if (lv_mk_hunt >= 0) { snprintf(up, sizeof up, "%s", LV_MK_BOT[lv_mk_hunt]); lv_upper(up); }
    if (lv_job != LV_J_NONE) {
        lv_tk_alert = false;
        if (lv_job == LV_J_DEAUTH) snprintf(lv_tk_msg, sizeof lv_tk_msg, "DEAUTH WATCH CH%d  %lu DEAUTH  %lu MGMT", lv_deauth_ch, (unsigned long)lv_deauth_n, (unsigned long)lv_mgmt_n);
        else if (lv_hunt >= 0) {        /* D: hunt status line, then each match found so far in full */
            const int slot = lv_tk_item++ % (1 + m);
            if (slot) { lv_mk_full(lv_f[mi[slot - 1]], slot, m, lv_nf, lv_tk_msg, sizeof lv_tk_msg); lv_tk_alert = true; return; }
            char d[48]; snprintf(d, sizeof d, "%s", LV_HUNT_DESC[lv_hunt]); lv_upper(d);
            snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s: %s  SCANNING %s...  %s", up, LV_MK_WHAT[lv_hunt], (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)), d);
            lv_tk_off = lv_tk_span1() + 1 - (lv_tk_span0() + 2); /* status starts at the left edge */
            return;
        }
        else snprintf(lv_tk_msg, sizeof lv_tk_msg, "SCANNING %s...", (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)));
        lv_upper(lv_tk_msg);
        return;
    }
    if (lv_nev) {
        snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", lv_evq[0]); lv_tk_alert = lv_evalert[0];
        memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); --lv_nev;
        return;
    }
    if (lv_mk_hunt >= 0 && lv_mk_hunt != LV_H_DEAUTH && m) {  /* D after the hunt: rotate the matches' full lines */
        const int k = lv_tk_item++ % m;
        lv_mk_full(lv_f[mi[k]], k + 1, m, lv_nf, lv_tk_msg, sizeof lv_tk_msg); lv_tk_alert = true;
        return;
    }
    int idx[LV_MAXF], n = 0;
    for (int i = 0; i < lv_nf; ++i) {
        const LvFind &f = lv_f[i];
        if (lv_focus == 3 ? f.alert : lv_in_mode(f, lv_focus)) idx[n++] = i;
    }
    if (!n) { lv_tk_alert = false; snprintf(lv_tk_msg, sizeof lv_tk_msg, lv_nf ? "NO %s FINDS" : "NO THREATS - TAP SCAN", LV_FOCUS_NAME[lv_focus]); return; }
    if (lv_tk_item >= n) lv_tk_item = 0;
    const LvFind &f = lv_f[idx[lv_tk_item]];
    lv_find_line(f, lv_tk_msg, sizeof lv_tk_msg);
    lv_tk_alert = f.alert;
    ++lv_tk_item;
}
static void lv_ticker_tick(void)
{
    const bool tty = lv_scr == LV_SC_TTY;
    const int span0 = lv_tk_span0(), span1 = lv_tk_span1();
    const int w = (int)strlen(lv_tk_msg) * 12;
    lv_tk_off += lv_rain == 0 ? 1 : lv_rain == 1 ? 2 : 4;
    if (lv_tk_off > (span1 - span0 + 1) + w) lv_tk_next();
    LvTickCtx c{span0, span1};
    lv_render(lv_blit, tty ? LV_R_TTY_TICK : LV_R_TICK, lv_scene_ticker, &c);
}

/* SCANbot results box */
static int lv_tile_x(int i) { return 42 + 31 * (i & 3); }
static int lv_tile_y(int i) { return 193 + 31 * (i >> 2); }
static const LvRect LV_R_BTN_SCAN = {176, 199, 203, 224}, LV_R_BTN_MORE = {176, 228, 203, 251};
static void lv_button(LvBand &B, const LvRect &r, const char *t, bool on)
{
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    b_fill(B, r.x0, r.y0, w, h, lv_hex(0xffffff));
    b_fill(B, r.x0 + 1, r.y0 + 1, w - 2, h - 2, on ? lv_ink(LV_ACC) : lv_hex(0x303030));
    b_text35(B, r.x0 + (w - lv_w35(t)) / 2, r.y0 + (h - 5) / 2, t, on ? lv_hex(0x000000) : lv_hex(0x808080));
}
static void lv_scene_box(LvBand &B, void *)
{
    const uint16_t hdr = lv_ink(LV_HDR);
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_fill(B, 36, 188, 168, 5, lv_hex(0x181818));
    b_fill(B, 163, 193, 41, 3, lv_hex(0x181818));
    int idx[LV_MAXF]; const int n = lv_view(idx);
    const int pages = n ? (n + 7) / 8 : 1;
    if (lv_page >= pages) lv_page = 0;
    char l[40], r[24];
    if (lv_job != LV_J_NONE) snprintf(l, sizeof l, "SCANNING %s", (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)));
    else if (lv_mode == 3) snprintf(l, sizeof l, "DEAUTH WATCH");
    else snprintf(l, sizeof l, "%s P%d/%d", lv_mode == 1 ? "WIFI" : lv_mode == 2 ? "BT" : "ALL", lv_page + 1, pages);
    snprintf(r, sizeof r, "FINDS %d", n);
    b_text35(B, 38, 188, l, hdr);
    b_text35(B, 203 - lv_w35(r), 188, r, hdr);
    const bool busy = lv_job != LV_J_NONE;
    lv_button(B, LV_R_BTN_SCAN, busy ? "STOP" : "SCAN", true);
    lv_button(B, LV_R_BTN_MORE, "MORE", n > 8);
    if (lv_mode == 3 && !lv_detail) {
        char t[40];
        b_text(B, 40, 199, "PASSIVE DEAUTH WATCH", lv_hex(0xffffff));
        snprintf(t, sizeof t, "CH %d  %s", lv_deauth_ch, lv_job == LV_J_DEAUTH ? "LISTENING" : (lv_deauth_have ? "DONE" : "TAP SCAN"));
        b_text(B, 40, 210, t, lv_hex(LV_TAN));
        snprintf(t, sizeof t, "DEAUTH %lu", (unsigned long)lv_deauth_n);
        b_text(B, 40, 221, t, lv_deauth_n ? lv_hex(0xff3030) : lv_hex(0xffffff));
        snprintf(t, sizeof t, "MGMT %lu", (unsigned long)lv_mgmt_n);
        b_text(B, 40, 232, t, lv_hex(0xffffff));
        b_text35(B, 40, 244, "RX ONLY. NO TRANSMIT.", hdr);
        return;
    }
    if (lv_detail && lv_sel >= 0 && lv_sel < n) {
        const LvFind &f = lv_f[idx[lv_sel]];
        char t[48], m[18]; lv_macstr(f.mac, m);
        b_frame(B, 38, 197, 20, 20, f.alert ? lv_hex(0xff2020) : lv_hex(0xffffff));
        b_icon(B, 40, 199, f.icon);
        lv_label(f, t, 19); b_text(B, 61, 198, t, lv_hex(0xffffff));
        snprintf(t, sizeof t, "%s %d DBM", LV_ICON_NAME[f.icon], f.rssi); b_text35(B, 61, 209, t, lv_hex(LV_TAN));
        b_text(B, 40, 220, m, lv_hex(0xffffff));
        if (f.kind == LV_K_WIFI) snprintf(t, sizeof t, "WIFI CH%d %s", f.ch, lv_auth_name(f.auth));
        else { const char *mn = lv_mfg_name(f.mfg); if (f.mfg == 0xFFFF) snprintf(t, sizeof t, "BLE NO MFG DATA"); else snprintf(t, sizeof t, "BLE %s %04X%s", mn ? mn : "MFG", f.mfg, f.apple == 0x12 ? " FINDMY" : ""); }
        b_text(B, 40, 230, t, lv_hex(LV_TAN));
        b_text35(B, 40, 244, f.alert ? "HUNT MATCH  TAP=BACK" : "TAP=BACK", hdr);
        return;
    }
    for (int t = 0; t < 8; ++t) {
        const int x = lv_tile_x(t), y = lv_tile_y(t), k = lv_page * 8 + t;
        const bool has = k < n;
        const uint16_t bd = !has ? lv_dim(lv_hex(LV_TAN), 1, 2) : lv_f[idx[k]].alert ? lv_hex(0xff2020) : lv_hex(LV_TAN); /* item 2: no persistent selection border */
        b_frame(B, x, y, 28, 28, bd);
        b_fill(B, x + 1, y + 1, 26, 26, lv_hex(0x060a0c));
        if (!has) continue;
        const LvFind &f = lv_f[idx[k]];
        b_icon(B, x + 6, y + 3, f.icon);
        char rs[8]; snprintf(rs, sizeof rs, "%d", f.rssi);
        b_text35(B, x + 2, y + 21, rs, lv_hex(0xffffff));
        b_pix(B, x + 25, y + 23, f.kind == LV_K_WIFI ? lv_hex(0x00f7ff) : lv_hex(0x3a7bff));
        b_pix(B, x + 24, y + 23, f.kind == LV_K_WIFI ? lv_hex(0x00f7ff) : lv_hex(0x3a7bff));
    }
}
/* the small terminal boxes on SCANbot / TERMINALbot */
static void lv_scene_minterm(LvBand &B, void *)
{
    const bool scan = lv_scr == LV_SC_SCAN;
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    const int x0 = scan ? 35 : 37, ry = scan ? 279 : 290, py = scan ? 299 : 296;
    b_fill(B, x0, ry, scan ? 143 : 139, 1, lv_ink(LV_ACCD));
    const int l1 = scan ? 281 : 273, l2 = scan ? 290 : 281;
    char a[25], b2[25];
    snprintf(a, sizeof a, "%s", lv_line_back(1)); snprintf(b2, sizeof b2, "%s", lv_line_back(0));
    b_text(B, x0, l1, a, lv_hex(0xffffff));
    b_text(B, x0, l2, b2, lv_hex(0xffffff));
    int x = b_text(B, x0, py, LV_PROMPT, lv_hex(LV_PROMPT_INK));
    const int maxc = (scan ? 182 : 181) - x - 8;
    const char *in = lv_inb; int len = lv_inlen;
    if (len * 6 > maxc) in += len - maxc / 6;
    x = b_text(B, x, py, in, lv_hex(0xffffff));
    if ((lv_now / 500) & 1 || lv_scr == LV_SC_SCAN) b_fill(B, x + 1, py, 7, 7, lv_ink(LV_ACC));
}
static void lv_scene_ctl(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x080808));
    const uint16_t ink = lv_hex(0xdd92fe);
    char t[16];
    if (lv_in(LV_R_CTHEME, B.x0, B.y0)) { snprintf(t, sizeof t, "%d/%d", lv_theme + 1, LV_NTHEME); b_text(B, 112 - (int)strlen(t) * 6 + 1, 37, t, ink); }
    else if (lv_in(LV_R_CUP, B.x0, B.y0)) { const uint32_t s = lv_now / 1000; if (s < 6000) snprintf(t, sizeof t, "%02lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60)); else snprintf(t, sizeof t, "%luh", (unsigned long)(s / 3600)); b_text(B, 112 - (int)strlen(t) * 6 + 1, 52, t, ink); }
    else if (lv_in(LV_R_CFOCUS, B.x0, B.y0)) { snprintf(t, sizeof t, "%d/4", lv_focus + 1); b_text(B, 112 - (int)strlen(t) * 6 + 1, 67, t, ink); }
    else { b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x000707)); b_text35(B, 182, 247, lv_motion ? "ON" : "OFF", lv_hex(0xffffff)); }
}

/* full-screen terminal + keyboard */
static const char *const LV_KROW[4] = {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm._/"};
static const char *const LV_KROW_S[4] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL:", "ZXCVBNM,?="};
enum { LV_KEY_SHIFT = 40, LV_KEY_SPACE, LV_KEY_DEL, LV_KEY_ENTER, LV_NKEY };
static LvRect lv_key_rect(int k)
{
    if (k < 40) { const int r = k / 10, c = k % 10; const int x = c * LV_KBD_PITCH_X + 1, y = LV_KBD_Y + r * LV_KBD_PITCH_Y + 1; return {(int16_t)x, (int16_t)y, (int16_t)(x + LV_KEY_W - 1), (int16_t)(y + LV_KEY_H - 1)}; }
    const int y = LV_KBD_Y + 4 * LV_KBD_PITCH_Y + 1;
    static const int X0[4] = {0, 48, 144, 192}, X1[4] = {47, 143, 191, 239};
    const int i = k - 40;
    return {(int16_t)(X0[i] + 1), (int16_t)y, (int16_t)(X1[i] - 1), (int16_t)(y + LV_KEY_H - 1)};
}
static int lv_key_at(int x, int y)
{
    if (y < LV_KBD_Y) return -1;
    const int r = (y - LV_KBD_Y) / LV_KBD_PITCH_Y;
    if (r < 4) { int c = x / LV_KBD_PITCH_X; if (c > 9) c = 9; return r * 10 + c; }
    if (x < 48) return LV_KEY_SHIFT;
    if (x < 144) return LV_KEY_SPACE;
    if (x < 192) return LV_KEY_DEL;
    return LV_KEY_ENTER;
}
static void lv_scene_kbd(LvBand &B, void *)
{
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
        else snprintf(t, sizeof t, "%s", k == LV_KEY_SHIFT ? (lv_shift ? "ABC" : "SHIFT") : k == LV_KEY_SPACE ? "SPACE" : k == LV_KEY_DEL ? "DEL" : "ENTER");
        const uint16_t ink = hl || special ? lv_hex(0x000000) : lv_hex(0xffffff);
        if (k < 40) b_text(B, r.x0 + (w - 10) / 2, r.y0 + (h - 14) / 2, t, ink, 2);
        else b_text(B, r.x0 + (w - (int)strlen(t) * 6 + 1) / 2, r.y0 + (h - 7) / 2, t, ink);
    }
}
#define LV_MK_GOLD 0xb39152   /* SCANbot gold (LV_TAN) */
/* A: the hunt target banner (22 px under the TERMINALBOT header) */
static void lv_mk_banner(LvBand &B, int y0, int h)
{
    const int hu = lv_mk_hunt < 0 ? LV_H_HACK : lv_mk_hunt;
    const uint16_t fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR), gold = lv_hex(LV_MK_GOLD);
    b_frame(B, 0, y0, 240, h, fr);
    b_fill(B, 1, y0 + 1, 238, h - 2, lv_hex(0x0a0a0a));
    b_frame(B, 3, y0 + 2, 18, 18, gold);
    b_fill(B, 4, y0 + 3, 16, 16, lv_hex(0x060a0c));
    b_icon(B, 4, y0 + 3, LV_MK_ICON[hu]);
    b_text(B, 25, y0 + 3, LV_MK_BOT[hu], lv_hex(0xffffff));
    char t[40]; snprintf(t, sizeof t, "HUNTS %s", LV_MK_WHAT[hu]);
    b_text35(B, 25, y0 + 14, t, gold);
    const int bx = 182, bw = 54, by = y0 + 14;
    if (lv_job != LV_J_NONE) {
        b_text35(B, bx + bw - lv_w35("SCANNING"), y0 + 4, "SCANNING", hdr);
        const int dots = (lv_now / 300) % 4; for (int d = 0; d < dots; ++d) b_fill(B, bx + bw - lv_w35("SCANNING") - 14 + d * 4, y0 + 8, 2, 1, hdr);
        b_frame(B, bx, by - 1, bw, 7, lv_dim(gold, 1, 2));
        const int seg = 12, span = bw - 2 - seg, p = (int)(lv_now / 30) % (2 * span), px = p < span ? p : 2 * span - p;
        for (int i = 0; i < seg; ++i) b_fill(B, bx + 1 + px + i, by, 1, 5, lv_dim(lv_ink(LV_ACC), i + 4, seg + 3));
    } else {
        char c[16], s2[24];
        int n = lv_mk_matches(), seen = lv_nf;
        if (hu == LV_H_DEAUTH) { n = (int)lv_deauth_n; snprintf(s2, sizeof s2, "IN %lu MGMT", (unsigned long)lv_mgmt_n); }
        else snprintf(s2, sizeof s2, "OF %d SEEN", seen);
        snprintf(c, sizeof c, "%d FOUND", n);
        b_text(B, bx + bw - ((int)strlen(c) * 6 - 1), y0 + 3, c, n ? lv_hex(0xff4040) : lv_hex(0xffffff));
        b_text35(B, bx + bw - lv_w35(s2), by, s2, gold);
    }
}
static const LvRect LV_R_TTY_BANNER = {0, 36, 239, 57};
static int lv_tty_nl(void) { return lv_mk_hunt >= 0 ? 11 : LV_TTY_LINES; }
static void lv_scene_banner(LvBand &B, void *) { lv_mk_banner(B, 36, 22); }
static void lv_scene_tty(LvBand &B, void *)
{
    const uint16_t fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR);
    const bool hunt = lv_mk_hunt >= 0;
    const int nl = lv_tty_nl(), ty = hunt ? 61 : 39, fy = hunt ? 58 : 36, fh = hunt ? 104 : 126, sh = nl * 9 + 1;
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_fill(B, 0, 24, 240, 1, lv_ink(LV_ACCD));
    b_fill(B, 0, 25, 240, 10, lv_hex(0x181818));
    b_text35(B, 3, 28, "TERMINALBOT", hdr);
    char t[40]; snprintf(t, sizeof t, "%s  TAP TOP/BOTTOM = SCROLL", lv_scroll ? "SCROLLED" : "LIVE");
    b_text35(B, 237 - lv_w35(t), 28, t, hdr);
    if (hunt) lv_mk_banner(B, 36, 22);
    b_frame(B, 0, fy, 240, fh, fr);
    const int first = lv_scroll;
    for (int i = 0; i < nl; ++i) {
        const char *s = lv_line_back(first + (nl - 1 - i));
        const uint16_t ink = s[0] == '$' ? lv_hex(LV_PROMPT_INK) : s[0] == '!' ? lv_hex(0xff4040) : lv_hex(0xffffff);
        b_text(B, 4, ty + i * 9, s, ink);
    }
    const int total = lv_tn < LV_TLINES ? lv_tn : LV_TLINES;
    if (total > nl) {
        const int h = sh * nl / total, y = ty + (sh - h) * (total - nl - lv_scroll) / (total - nl);
        b_fill(B, 236, ty, 2, sh, lv_hex(0x202020)); b_fill(B, 236, y, 2, h, hdr);
    }
    int x = b_text(B, 2, 164, LV_PROMPT " ", lv_hex(LV_PROMPT_INK));
    const char *in = lv_inb; const int maxc = (238 - x - 8) / 6;
    if (lv_inlen > maxc) in += lv_inlen - maxc;
    x = b_text(B, x, 164, in, lv_hex(0xffffff));
    if ((lv_now / 400) & 1) b_fill(B, x + 1, 164, 6, 8, lv_ink(LV_ACC));
}
/* TTY top-right slot (was CLOSE): back to the screen that opened the terminal */
static void lv_scene_close(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0xffffff));
    b_fill(B, B.x0 + 1, B.y0 + 1, B.w - 2, B.h - 2, lv_ink(LV_ACC));
    b_text(B, B.x0 + (B.w - 29) / 2, B.y0 + 8, "<BACK", lv_hex(0x000000));
}
/* item 4: BACK button, bottom-left corner below the left pin column (no UI there on any screen) */
static void lv_scene_back(LvBand &B, void *)
{
    const bool home = lv_scr == LV_SC_HOME;         /* HOME shows it dimmed (home is already the root) */
    const LvRect r = LV_R_BACK;
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    const uint16_t gold = home ? lv_hex(0x4a3c22) : lv_hex(LV_TAN), ink = home ? lv_hex(0x505050) : lv_hex(0xffffff);
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_frame(B, r.x0, r.y0, w, h, gold);
    b_fill(B, r.x0 + 1, r.y0 + 1, w - 2, h - 2, lv_hex(0x0a0a0a));
    const int ax = r.x0 + 3, ay = r.y0 + 3;
    static const int8_t CH[5][2] = {{2, 0}, {1, 1}, {0, 2}, {1, 3}, {2, 4}};
    for (int i = 0; i < 5; ++i) { b_pix(B, ax + CH[i][0], ay + CH[i][1], ink); b_pix(B, ax + CH[i][0] + 1, ay + CH[i][1], ink); }
    b_text35(B, r.x0 + 8, r.y0 + 3, "BACK", ink);
}
/* value overlays (only while the setting is not its default) */
static void lv_scene_dyn(LvBand &B, void *vctx)
{
    const int d = *(const int *)vctx;
    const LvRect &R = *LV_DYN_R[d];
    const int cx = (R.x0 + R.x1 + 1) / 2;
    const uint16_t W = lv_hex(0xffffff);
    char t[24] = "";
    if (d == LV_D_SCANLBL) {
        b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x6b7584));
        if (lv_job == LV_J_DEAUTH) snprintf(t, sizeof t, "WATCH CH%d", lv_deauth_ch);
        else if (lv_job != LV_J_NONE) snprintf(t, sizeof t, "SCAN %s", lv_wifi_busy() ? "WIFI" : "BLE");
        else snprintf(t, sizeof t, "%s", lv_mode == 1 ? "WIFI RADIO" : lv_mode == 2 ? "BT RADIO" : lv_mode == 3 ? "DEAUTH" : "ALL RADIO");
        const uint16_t ink = lv_job != LV_J_NONE ? W : lv_theme ? lv_dim(lv_ink(LV_ACC), 2, 3) : lv_hex(0x7324a5);
        b_text(B, 119 - ((int)strlen(t) * 6 - 1) / 2, 135, t, ink);
        return;
    }
    if (d == LV_D_CLABEL) {
        b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x6b7584));
        snprintf(t, sizeof t, "%s", LV_THEME_NAME[lv_theme]);
        b_text(B, 120 - ((int)strlen(t) * 6 - 1) / 2, 191, t, lv_theme_px(lv_hex(0x7324a5)));
        return;
    }
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    bool glcd = true; int y = R.y0 + 1;
    switch (d) {
    case LV_D_CMIX: snprintf(t, sizeof t, "%s", LV_MIX_NAME[lv_mix]); y = 127; break;
    case LV_D_CRCOL: snprintf(t, sizeof t, "%s", LV_RCOL_NAME[lv_rcol]); glcd = false; y = 181; break;
    case LV_D_CMASH: if (LV_MASH_S[lv_mash] >= 120) snprintf(t, sizeof t, "%d MIN", LV_MASH_S[lv_mash] / 60); else snprintf(t, sizeof t, "%d SEC", LV_MASH_S[lv_mash]); y = 239; break;
    case LV_D_CRAIN: snprintf(t, sizeof t, "%s", LV_RAIN_NAME[lv_rain]); y = 119; break;
    case LV_D_CRMODE: snprintf(t, sizeof t, "%s", LV_RMODE_NAME[lv_rmode]); glcd = false; y = 157; break;
    case LV_D_CMULTI: snprintf(t, sizeof t, "%s", LV_SORT_NAME[lv_sortk]); y = 216; break;
    case LV_D_CORIENT: snprintf(t, sizeof t, "FLIP 180"); glcd = false; y = 220; break;
    }
    if (glcd) b_text(B, cx - ((int)strlen(t) * 6 - 1) / 2, y, t, W);
    else b_text35(B, cx - lv_w35(t) / 2, y, t, W);
}

/* ---------- flow ---------- */
static int lv_theme_seg(int base) { return base + lv_theme; }
static void lv_go(int s)
{
    if (s == LV_SC_TTY && lv_scr != LV_SC_TTY) lv_prev = (lv_scr == LV_SC_TERM || lv_scr == LV_SC_SCAN || lv_scr == LV_SC_CTL || lv_scr == LV_SC_HOME) ? lv_scr : LV_SC_TERM;
    lv_scr = s; lv_enter = true;
    lv_key_hl = -1; lv_fl_on = false;
    lv_fx_want(lv_fxk); lv_dyn = lv_dyn_want();
    switch (s) {
    case LV_SC_BOOT: lv_play.start(LK_S_BOOT); break;
    case LV_SC_HOME_INTRO: lv_play.start(LK_S_HOME_INTRO); break;
    case LV_SC_HOME: lv_play.start(lv_theme_seg(LK_S_HOME_T0)); break;
    case LV_SC_TERM: lv_play.start(lv_theme_seg(LK_S_TERM_T0)); break;
    case LV_SC_SCAN: lv_play.start(lv_theme_seg(LK_S_SCAN_T0)); break;
    case LV_SC_CTL: lv_play.start(LK_S_CTL); break;
    case LV_SC_TTY: lv_play.seg = -1; break;
    case LV_SC_CAL: lv_play.seg = -1; break;
    }
    lv_set_masks();
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = lv_dirty_back = lv_dirty_dyn = true;
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
static const int16_t LV_CAL_PT[4][2] = {{52, 55}, {187, 55}, {187, 226}, {52, 226}};   /* chip centres TL TR BR BL */
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
static void lv_cal_solve(const uint16_t med[4][2], uint16_t out[8])
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
            const int tx = (i & 2) ? 239 : 0, ty = (i & 1) ? 319 : 0;   /* LovyanGFX corner order */
            const int sx = lv_rot180 ? 239 - tx : tx, sy = lv_rot180 ? 319 - ty : ty;   /* item 6: native corner -> where it sits on the 180-rotated screen */
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
    lv_cal_solve(lv_cal_med, lv_cal_c8);
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

static void lv_paint_overlays(bool all)
{
    if (lv_scr == LV_SC_TTY) {
        if (all || lv_dirty_tty) { lv_render(lv_blit, LV_R_TTY_BODY, lv_scene_tty, 0); lv_dirty_tty = lv_dirty_banner = false; }
        else if (lv_dirty_banner && lv_mk_hunt >= 0) { lv_render(lv_blit, LV_R_TTY_BANNER, lv_scene_banner, 0); lv_dirty_banner = false; }
        if (all || lv_dirty_kbd) { lv_render(lv_blit, LV_R_KBD, lv_scene_kbd, 0); lv_dirty_kbd = false; }
        if (all || lv_dirty_back) { lv_render(lv_blit, LV_R_TTY_CLOSE, lv_scene_close, 0); lv_dirty_back = false; }
        return;
    }
    if (lv_scr >= LV_SC_HOME && lv_scr <= LV_SC_CTL && (all || lv_dirty_back)) { lv_render(lv_blit, LV_R_BACK, lv_scene_back, 0); lv_dirty_back = false; }
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) return;
    if (lv_scr == LV_SC_SCAN) {
        if (all || lv_dirty_box) { lv_render(lv_blit, LV_R_SBOX, lv_scene_box, 0); lv_dirty_box = false; }
        if (all || lv_dirty_term) { lv_render(lv_blit, LV_R_STERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    }
    if (lv_scr == LV_SC_TERM && (all || lv_dirty_term)) { lv_render(lv_blit, LV_R_TTERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    if (lv_scr == LV_SC_CTL && (all || lv_dirty_ctl)) {
        lv_render(lv_blit, LV_R_CTHEME, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CUP, lv_scene_ctl, 0);
        lv_render(lv_blit, LV_R_CFOCUS, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CMOTION, lv_scene_ctl, 0);
        lv_dirty_ctl = false; lv_dirty_dyn = true;
    }
    if (all || lv_dirty_dyn) {
        for (int d = 0; d < LV_ND; ++d) if (lv_dyn >> d & 1) lv_render(lv_blit, *LV_DYN_R[d], lv_scene_dyn, &d);
        lv_dirty_dyn = false;
    }
}

/* public API */
/* cal_valid: a cv=2 8-point cal is already in NVS (and was applied at display begin). */
static void lv_begin(const LvRadio *radio, lv_blit_fn blit, lk_draw_fn raw_draw, int theme, uint32_t now, bool cal_valid)
{
    lv_radio = radio; lv_hw_blit = blit; lv_blit = lv_fx_blit; lv_raw_draw = raw_draw;
    lv_theme = (theme >= 0 && theme < LV_NTHEME) ? theme : 0;
    lv_now = lv_boot_ms = now; lv_clip_t = now;
    lv_print("BOTCORPS %s", LV_VERSION);
    lv_print("type 'help'. scans: tap SCAN or 'scan'");
    lv_cal_nvs = cal_valid; lv_cal_t0 = lv_cal_open = now; lv_cal_hold0 = 0;
    if (cal_valid) lv_cal_phase = LV_CAL_NVS; else lv_cal_start_run(now);
    lv_go(LV_SC_CAL);
}
static const int LV_MIX_JOB[3] = {LV_J_ALL, LV_J_WIFI, LV_J_BLE};
static uint32_t lv_mash_t = 0;
/* call every ~30 ms. io.draw must be lv_clip_draw. */
static void lv_tick(const LkIo &io, uint32_t now)
{
    lv_now = now; lv_io = &io;
    if (lv_scr == LV_SC_CAL) { lv_cal_tick(now); return; }
    lv_job_tick();
    if (lv_mash && lv_job == LV_J_NONE) {           /* MASHUP: auto re-scan (MIX radios) every N s */
        const uint32_t ref = (int32_t)(lv_last_done - lv_mash_t) > 0 ? lv_last_done : lv_mash_t;
        if ((int32_t)(now - ref) >= LV_MASH_S[lv_mash] * 1000) { lv_print("mashup: auto re-scan"); lv_job_start(LV_MIX_JOB[lv_mix], -1); }
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
    const bool shown = lv_scr == LV_SC_TTY || lv_play.shown >= 0;
    const bool all = lv_enter && shown;
    if (all) lv_enter = false;
    if (shown && !all) {
        if (lv_fl_on && (int32_t)(now - lv_fl_t) > 150) { lv_fl_on = false; lv_repaint(lv_fl); }   /* press flash ends */
        uint8_t k[LV_NFX]; lv_fx_want(k);                                                          /* chips / tabs */
        for (int f = 0; f < LV_NFX; ++f) if (k[f] != lv_fxk[f]) { lv_fxk[f] = k[f]; lv_repaint(LV_FX_R[f]); }
        const uint32_t d = lv_dyn_want();                                                          /* value overlays */
        if (d != lv_dyn) {
            const uint32_t off = lv_dyn & ~d; lv_dyn = d; lv_set_masks();
            for (int q = 0; q < LV_ND; ++q) if (off >> q & 1) lv_repaint(*LV_DYN_R[q]);
            lv_dirty_dyn = true;
        }
        if (lv_scr == LV_SC_SCAN && (lv_dyn >> LV_D_SCANLBL & 1) && (now / 500) != ((now - 30) / 500)) lv_dirty_dyn = true;
    }
    if (lv_scr == LV_SC_TERM && (now / 500) != ((now - 30) / 500)) lv_dirty_term = true;
    if (lv_scr == LV_SC_TTY && (now / 400) != ((now - 30) / 400)) lv_dirty_tty = true;
    if (lv_scr == LV_SC_TTY && lv_mk_hunt >= 0 && lv_job != LV_J_NONE && (now / 60) != ((now - 30) / 60)) lv_dirty_banner = true;
    if (lv_key_hl >= 0 && (int32_t)(now - lv_key_hl_t) > 120) { lv_key_hl = -1; lv_dirty_kbd = true; }
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) { if (lv_play.shown >= 0) lv_paint_overlays(all); return; }
    if (shown) {
        lv_paint_overlays(all);
        lv_ticker_tick();
    }
}

static void lv_open_tty(const char *cmd)
{
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
    else if (k == LV_KEY_ENTER) { char c[64]; snprintf(c, sizeof c, "%s", lv_inb); lv_inlen = 0; lv_inb[0] = 0; if (lv_shift) lv_shift = false; if (c[0]) lv_cmd(c); }
    lv_dirty_tty = true;
}

/* item 4: back one level (TTY -> the screen that opened it, SCAN detail -> list, others -> GIBSONbot home) */
static void lv_back(void)
{
    if (lv_scr == LV_SC_TTY) { lv_go(lv_prev); return; }
    if (lv_scr == LV_SC_SCAN && lv_detail) { lv_detail = false; lv_dirty_box = true; return; }
    if (lv_scr == LV_SC_HOME) { lv_event(false, "GIBSONBOT HOME IS THE TOP SCREEN"); lv_tk_off = 1 << 14; return; }
    if (lv_scr == LV_SC_CAL) return;
    lv_go(LV_SC_HOME);
    if (lv_selftest_pending) { lv_selftest_pending = false; lv_selftest(); }
}

/* ---------- item 9: one hit table per screen (taps, press flash and the sim audit all use it) ---------- */
enum {
    LV_A_SKIP, LV_A_BACK, LV_A_TICK, LV_A_SELF, LV_A_JACK, LV_A_GO, LV_A_HUNT, LV_A_CROSS, LV_A_LIST, LV_A_MENU, LV_A_TTY,
    LV_A_WCHIP, LV_A_BCHIP, LV_A_TAB, LV_A_SCANBTN, LV_A_MORE, LV_A_TILE, LV_A_DETAIL, LV_A_THEME, LV_A_UP, LV_A_FOCUS,
    LV_A_MIX, LV_A_RCOL, LV_A_MASH, LV_A_RAIN, LV_A_RMODE, LV_A_MULTI, LV_A_MOTION, LV_A_ORIENT, LV_A_STATUS, LV_A_GOBLE, LV_A_KEY, LV_A_SCROLL
};
struct LvHit { uint8_t act; int8_t arg; LvRect r; const char *name; };
#define LV_MAXHIT 72
static void lv_t(LvHit *o, int &n, int act, int arg, int x0, int y0, int x1, int y1, const char *nm)
{
    if (n < LV_MAXHIT) o[n++] = LvHit{(uint8_t)act, (int8_t)arg, {(int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1}, nm};
}
static int lv_targets(LvHit *o)
{
    int n = 0;
    const int s = lv_scr;
    if (s == LV_SC_CAL) return 0;   /* CALbot reads raw touch only */
    if (s == LV_SC_TTY) {
        lv_t(o, n, LV_A_BACK, 0, LV_R_TTY_CLOSE.x0, LV_R_TTY_CLOSE.y0, LV_R_TTY_CLOSE.x1, LV_R_TTY_CLOSE.y1, "tty-back");
        lv_t(o, n, LV_A_TICK, 0, LV_R_TTY_TICK.x0, LV_R_TTY_TICK.y0, LV_R_TTY_TICK.x1, LV_R_TTY_TICK.y1, "ticker->scan");
        lv_t(o, n, LV_A_KEY, -1, 0, LV_KBD_Y, 239, 319, "keyboard(44 keys)");
        lv_t(o, n, LV_A_SCROLL, 1, 1, 37, 238, 97, "scroll-up");
        lv_t(o, n, LV_A_SCROLL, -1, 1, 98, 238, 160, "scroll-down");
        return n;
    }
    if (s == LV_SC_BOOT || s == LV_SC_HOME_INTRO) { lv_t(o, n, LV_A_SKIP, 0, 0, 0, 239, 319, "skip-intro"); return n; }
    const LvRect &bk = LV_R_BACK_HIT;   /* item 4: back target only on the 4 navigable screens, matching where it renders */
    lv_t(o, n, LV_A_BACK, 0, bk.x0, bk.y0, bk.x1, bk.y1, s == LV_SC_HOME ? "back(home=top)" : "back");
    lv_t(o, n, LV_A_TICK, 0, LV_R_TICK.x0, 0, LV_R_TICK.x1, 23, "ticker->scan");
    if (s == LV_SC_HOME) {
        lv_t(o, n, LV_A_SELF, 0, 18, 22, 50, 62, "boot-selftest");
        lv_t(o, n, LV_A_JACK, 0, 68, 20, 158, 130, "gibson-jackin-scan(mix)");
        lv_t(o, n, LV_A_GO, LV_SC_CTL, 170, 20, 205, 75, "ctl");
        lv_t(o, n, LV_A_GO, LV_SC_SCAN, 18, 135, 105, 215, "scan");
        lv_t(o, n, LV_A_GO, LV_SC_TERM, 115, 135, 205, 215, "term");
        return n;
    }
    if (s == LV_SC_TERM) {
        static const LvRect HOT[LV_NHUNT] = {{35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
                                             {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250}};
        for (int h = 0; h < LV_NHUNT; ++h) lv_t(o, n, LV_A_HUNT, h, HOT[h].x0, HOT[h].y0, HOT[h].x1, HOT[h].y1, LV_HUNT_NAME[h]);
        lv_t(o, n, LV_A_CROSS, 0, 180, 270, 205, 283, "corner-crosshair");
        lv_t(o, n, LV_A_LIST, 0, 180, 284, 205, 296, "corner-list");
        lv_t(o, n, LV_A_MENU, 0, 180, 297, 205, 311, "corner-menu");
        lv_t(o, n, LV_A_TTY, 0, 33, 262, 179, 310, "tty");
        lv_t(o, n, LV_A_TTY, 0, 75, 30, 165, 150, "tty(face)");
        return n;
    }
    if (s == LV_SC_SCAN) {
        lv_t(o, n, LV_A_WCHIP, 0, 32, 30, 72, 88, "wifi-chip-scan");
        lv_t(o, n, LV_A_BCHIP, 0, 32, 95, 72, 150, "ble-chip-scan");
        static const char *const TN[4] = {"tab-all", "tab-wifi", "tab-bt", "tab-deauth"};
        for (int t = 0; t < 4; ++t) lv_t(o, n, LV_A_TAB, t, LV_FX_R[2 + t].x0, LV_FX_R[2 + t].y0, LV_FX_R[2 + t].x1, LV_FX_R[2 + t].y1, TN[t]);
        lv_t(o, n, LV_A_SCANBTN, 0, 80, 30, 160, 150, "centre-scan/stop");
        lv_t(o, n, LV_A_SCANBTN, 0, LV_R_BTN_SCAN.x0, LV_R_BTN_SCAN.y0, LV_R_BTN_SCAN.x1, LV_R_BTN_SCAN.y1, "scan/stop-button");
        lv_t(o, n, LV_A_GO, LV_SC_HOME, 168, 60, 208, 118, "gib-home");
        int idx[LV_MAXF]; const int nv = lv_view(idx);
        if (nv > 8) lv_t(o, n, LV_A_MORE, 0, LV_R_BTN_MORE.x0, LV_R_BTN_MORE.y0, LV_R_BTN_MORE.x1, LV_R_BTN_MORE.y1, "more-page");
        if (lv_detail) lv_t(o, n, LV_A_DETAIL, 0, LV_R_SBOX.x0, LV_R_SBOX.y0, 172, LV_R_SBOX.y1, "detail-back");
        else if (lv_mode != 3)
            for (int t = 0; t < 8; ++t) if (lv_page * 8 + t < nv) lv_t(o, n, LV_A_TILE, t, lv_tile_x(t) - 1, lv_tile_y(t) - 1, lv_tile_x(t) + 28, lv_tile_y(t) + 28, "tile-detail");
        lv_t(o, n, LV_A_CROSS, 0, 183, 274, 205, 287, "corner-crosshair");
        lv_t(o, n, LV_A_LIST, 0, 183, 288, 205, 299, "corner-list");
        lv_t(o, n, LV_A_MENU, 0, 183, 300, 205, 313, "corner-menu");
        lv_t(o, n, LV_A_TTY, 0, 30, 262, 182, 312, "tty");
        return n;
    }
    if (s == LV_SC_CTL) {
        lv_t(o, n, LV_A_THEME, 0, 35, 33, 113, 48, "theme");
        lv_t(o, n, LV_A_UP, 0, 35, 49, 113, 63, "uptime");
        lv_t(o, n, LV_A_FOCUS, 0, 35, 64, 113, 79, "ticker-focus");
        lv_t(o, n, LV_A_MIX, 0, 35, 85, 70, 140, "mix(radios)");
        lv_t(o, n, LV_A_RCOL, 0, 35, 145, 70, 190, "rain-col");
        lv_t(o, n, LV_A_MASH, 0, 35, 195, 76, 258, "mashup(auto-rescan)");
        lv_t(o, n, LV_A_RAIN, 0, 165, 80, 205, 130, "rain(speed)");
        lv_t(o, n, LV_A_RMODE, 0, 165, 131, 205, 170, "rain-mode");
        lv_t(o, n, LV_A_MULTI, 0, 165, 175, 205, 224, "multi(sort)");
        lv_t(o, n, LV_A_MOTION, 0, 160, 225, 208, 256, "bg-motion");
        lv_t(o, n, LV_A_ORIENT, 0, 85, 206, 158, 258, "orient(flip180)");
        lv_t(o, n, LV_A_STATUS, 0, 75, 82, 165, 205, "ctl-status");
        lv_t(o, n, LV_A_CROSS, 0, 180, 274, 205, 287, "corner-crosshair");
        lv_t(o, n, LV_A_LIST, 0, 180, 288, 205, 299, "corner-list");
        lv_t(o, n, LV_A_MENU, 0, 180, 300, 205, 312, "corner-menu");
        lv_t(o, n, LV_A_GOBLE, 0, 33, 272, 179, 284, "go-ble");
        lv_t(o, n, LV_A_GO, LV_SC_SCAN, 33, 285, 179, 293, "go-scan");
        lv_t(o, n, LV_A_TTY, 0, 33, 262, 179, 310, "tty");
        return n;
    }
    return n;
}
static bool lv_hit(int x, int y, LvHit &h)
{
    LvHit o[LV_MAXHIT]; const int n = lv_targets(o);
    for (int i = 0; i < n; ++i) if (lv_in(o[i].r, x, y)) { h = o[i]; return true; }
    return false;
}
/* item 2: touch-down feedback. The pressed target brightens ~150 ms (keys use their own white flash). */
static void lv_press(int x, int y)
{
    LvHit h;
    if (!lv_hit(x, y, h)) return;
    if (h.act == LV_A_KEY) { lv_key_hl = lv_key_at(x, y); lv_key_hl_t = lv_now; lv_dirty_kbd = true; }
    else {
        if (lv_fl_on) { lv_fl_on = false; lv_repaint(lv_fl); }
        lv_fl = h.r; lv_fl_on = true; lv_fl_t = lv_now;
        lv_repaint(h.r);
    }
    if (lv_scr == LV_SC_TTY || lv_play.shown >= 0) lv_paint_overlays(false);
}
static void lv_note(const char *fmt, ...)
{
    char b[LV_TKN]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    lv_event(false, "%s", b); lv_tk_off = 1 << 14; lv_dirty_ctl = lv_dirty_dyn = true;
}
static const char *lv_do(const LvHit &h, int x, int y)
{
    switch (h.act) {
    case LV_A_SKIP: lv_go(LV_SC_HOME); if (lv_selftest_pending) { lv_selftest_pending = false; lv_selftest(); } return "skip-intro";
    case LV_A_BACK: lv_back(); return h.name;
    case LV_A_TICK: lv_mode = 0; if (lv_scr != LV_SC_SCAN) lv_go(LV_SC_SCAN); return "ticker->scan";
    case LV_A_SELF: lv_selftest_pending = true; lv_go(LV_SC_BOOT); return "boot-selftest";
    case LV_A_JACK: lv_job_start(LV_MIX_JOB[lv_mix], -1); return "gibson-jackin-scan";
    case LV_A_GO: lv_go(h.arg); return h.name;
    case LV_A_HUNT: { char c[24]; snprintf(c, sizeof c, "hunt %s", LV_HUNT_NAME[h.arg]); lv_open_tty(c); return LV_HUNT_NAME[h.arg]; }
    case LV_A_CROSS:      /* item 8: start / stop the scan (or the last hunt) without leaving the screen */
        if (lv_job != LV_J_NONE) { lv_job_stop(); return "corner-crosshair:stop"; }
        if (lv_scr == LV_SC_TERM && lv_mk_hunt >= 0) { char c[24]; snprintf(c, sizeof c, "hunt %s", LV_HUNT_NAME[lv_mk_hunt]); lv_cmd(c); return "corner-crosshair:rehunt"; }
        if (lv_scr == LV_SC_SCAN) lv_job_start(lv_mode == 1 ? LV_J_WIFI : lv_mode == 2 ? LV_J_BLE : lv_mode == 3 ? LV_J_DEAUTH : LV_J_ALL, lv_mode == 3 ? LV_H_DEAUTH : -1);
        else lv_job_start(LV_MIX_JOB[lv_mix], -1);
        return "corner-crosshair:scan";
    case LV_A_LIST: lv_open_tty("list"); return "corner-list";
    case LV_A_MENU: lv_open_tty("help"); return "corner-menu";
    case LV_A_TTY: lv_open_tty(0); return "tty";
    case LV_A_WCHIP: lv_mode = 1; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(LV_J_WIFI, -1); return "wifi-chip-scan";
    case LV_A_BCHIP: lv_mode = 2; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(LV_J_BLE, -1); return "ble-chip-scan";
    case LV_A_TAB: lv_mode = h.arg; lv_page = 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; return h.name;
    case LV_A_SCANBTN:
        if (lv_job != LV_J_NONE) { lv_job_stop(); return "stop"; }
        lv_job_start(lv_mode == 1 ? LV_J_WIFI : lv_mode == 2 ? LV_J_BLE : lv_mode == 3 ? LV_J_DEAUTH : LV_J_ALL, lv_mode == 3 ? LV_H_DEAUTH : -1); return "scan-button";
    case LV_A_MORE: { int idx[LV_MAXF]; const int n = lv_view(idx); lv_page = n > 8 ? (lv_page + 1) % ((n + 7) / 8) : 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; return "more-page"; }
    case LV_A_DETAIL: lv_detail = false; lv_dirty_box = true; return "detail-back";
    case LV_A_TILE: {
        int idx[LV_MAXF]; const int n = lv_view(idx); const int k = lv_page * 8 + h.arg;
        if (k >= n) return "empty-tile";
        lv_sel = k; lv_detail = true; lv_dirty_box = true;
        lv_find_line(lv_f[idx[k]], lv_tk_msg, sizeof lv_tk_msg); lv_tk_alert = lv_f[idx[k]].alert; lv_tk_off = 0;
        return "tile-detail";
    }
    case LV_A_THEME: lv_set_theme(lv_theme + 1); lv_note("THEME %d/6 %s", lv_theme + 1, LV_THEME_NAME[lv_theme]); return "theme";
    case LV_A_UP: { char hh[48] = ""; lv_radio->sys(0, hh, sizeof hh); lv_note("UP %lus  %s", (unsigned long)(lv_now / 1000), hh); return "uptime"; }
    case LV_A_FOCUS: lv_focus = (lv_focus + 1) % 4; lv_tk_item = 0; lv_note("TICKER FOCUS: %s", LV_FOCUS_NAME[lv_focus]); return "focus";
    case LV_A_MIX: lv_mix = (lv_mix + 1) % 3; lv_note("MIX: %s - SCANS USE %s", LV_MIX_NAME[lv_mix], lv_mix == 0 ? "WIFI+BLE" : LV_MIX_NAME[lv_mix]); return "mix";
    case LV_A_RCOL: lv_rcol = (lv_rcol + 1) % 3; lv_note("RAIN COL: %s TICKER", LV_RCOL_NAME[lv_rcol]); return "rain-col";
    case LV_A_MASH: lv_mash = (lv_mash + 1) % 4; lv_mash_t = lv_now; if (lv_mash) lv_note("MASHUP: AUTO RE-SCAN EVERY %dS", LV_MASH_S[lv_mash]); else lv_note("MASHUP: AUTO RE-SCAN OFF"); return "mashup";
    case LV_A_RAIN: lv_rain = (lv_rain + 1) % 3; lv_note("RAIN: %s TICKER SCROLL", LV_RAIN_NAME[lv_rain]); return "rain";
    case LV_A_RMODE: lv_rmode = (lv_rmode + 1) % 3; lv_note("RAIN MODE: %s STRIPES", LV_RMODE_NAME[lv_rmode]); return "rain-mode";
    case LV_A_MULTI: lv_sortk = (lv_sortk + 1) % 3; lv_sort(); lv_dirty_box = true; lv_note("MULTI: SORT FINDS BY %s", LV_SORT_NAME[lv_sortk]); return "multi-sort";
    case LV_A_MOTION: lv_motion = !lv_motion; lv_note("BG MOTION %s", lv_motion ? "ON" : "OFF"); return "bg-motion";
    case LV_A_ORIENT:
        lv_flip = !lv_flip; lv_rot180 = !lv_flip;
        if (lv_radio->set_flip) lv_radio->set_flip(lv_flip);
        lv_go(lv_scr); lv_note("ORIENT: %s", lv_flip ? "FLIPPED 180 (USB TOP)" : "NORMAL (USB BOTTOM)"); return "orient";
    case LV_A_STATUS: lv_open_tty("status"); return "ctl-status";
    case LV_A_GOBLE: lv_open_tty("go ble"); return "go-ble";
    case LV_A_KEY: lv_key(lv_key_at(x, y)); return "key";
    case LV_A_SCROLL: {
        const int total = lv_tn < LV_TLINES ? lv_tn : LV_TLINES, nl = lv_tty_nl(), mx = total > nl ? total - nl : 0;
        lv_scroll += h.arg > 0 ? 6 : -6;
        if (lv_scroll > mx) lv_scroll = mx;
        if (lv_scroll < 0) lv_scroll = 0;
        lv_dirty_tty = true; return h.name;
    }
    }
    (void)x; (void)y;
    return "none";
}
/* Tap (release) at portrait (x,y). Returns a short name of what it did (for logs / the sim). */
static const char *lv_tap(int x, int y)
{
    if (lv_scr == LV_SC_CAL) return "cal-raw-only";
    LvHit h;
    if (!lv_hit(x, y, h)) return "none";
    return lv_do(h, x, y);
}
