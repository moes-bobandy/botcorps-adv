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
static const LvRect LV_R_TICK_L = {24, 1, 127, 23};   /* top bar left of the crown (swatches) */
static const LvRect LV_R_TICK_R = {164, 1, 236, 23};  /* right of the crown (counter) */
static const LvRect LV_R_CROWN = {126, 0, 165, 23};
static const LvRect LV_R_BACK = {0, 0, 22, 22};
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
static char lv_tk_msg[96] = "NO THREATS - TAP SCAN";
static int lv_tk_off = 0, lv_tk_item = 0;
static bool lv_tk_alert = false;
static char lv_evq[4][96]; static uint8_t lv_evalert[4]; static int lv_nev = 0;
static void lv_event(bool alert, const char *fmt, ...)
{
    if (lv_nev >= 4) { memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); lv_nev = 3; }
    va_list ap; va_start(ap, fmt); vsnprintf(lv_evq[lv_nev], 96, fmt, ap); va_end(ap);
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
static const char *lv_job_name(int j) { return j == LV_J_WIFI ? "WIFI" : j == LV_J_BLE ? "BLE" : j == LV_J_ALL ? "WIFI+BLE" : j == LV_J_DEAUTH ? "DEAUTH WATCH" : "-"; }
static void lv_hunt_report(void);
static void lv_job_start(int job, int hunt)
{
    if (lv_job != LV_J_NONE) { lv_print("busy: %s scan running (stop)", lv_job_name(lv_job)); return; }
    lv_hunt = hunt;
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
    if (m) lv_event(true, "%s: %d FOUND NEARBY", up, m);
    else lv_event(false, "NO %s FOUND (%d DEVICES SEEN)", up, lv_nf);
}

/* ---------- commands ---------- */
static void lv_go(int s);
static void lv_set_theme(int t)
{
    lv_theme = ((t % LV_NTHEME) + LV_NTHEME) % LV_NTHEME;
    if (lv_radio->save_theme) lv_radio->save_theme(lv_theme);
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = true;
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
    else if (IS("clear") || IS("cls")) { lv_tn = 0; lv_scroll = 0; lv_dirty_term = lv_dirty_tty = true; }
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
    else if (IS("home") || IS("back") || IS("exit") || IS("close") || IS("quit")) lv_go(IS("home") ? LV_SC_HOME : LV_SC_TERM);
    else if (IS("rain") || IS("ping") || IS("track") || IS("load")) lv_print("%s: not in the livescan build", a0);
    else lv_print("unknown: %s (try help)", a0);
#undef IS
}

/* ---------- clip with masks ---------- */
static lk_draw_fn lv_raw_draw;
static LvRect lv_masks[8];
static int lv_nmask = 0;
static void lv_set_masks(void)
{
    lv_nmask = 0;
    if (lv_scr != LV_SC_BOOT && lv_scr != LV_SC_HOME_INTRO && lv_scr != LV_SC_CAL) { lv_masks[lv_nmask++] = LV_R_TICK_L; lv_masks[lv_nmask++] = LV_R_TICK_R; }
    if (lv_scr == LV_SC_SCAN) { lv_masks[lv_nmask++] = LV_R_SBOX; lv_masks[lv_nmask++] = LV_R_STERM; }
    if (lv_scr == LV_SC_TERM) lv_masks[lv_nmask++] = LV_R_TTERM;
    if (lv_scr == LV_SC_CTL) { lv_masks[lv_nmask++] = LV_R_CTHEME; lv_masks[lv_nmask++] = LV_R_CUP; lv_masks[lv_nmask++] = LV_R_CFOCUS; lv_masks[lv_nmask++] = LV_R_CMOTION; }
}
static void lv_clip_draw(int y, int x, int n, const uint8_t *idx)
{
    bool any = false;
    for (int m = 0; m < lv_nmask; ++m) if (y >= lv_masks[m].y0 && y <= lv_masks[m].y1) any = true;
    if (!any) { lv_raw_draw(y, x, n, idx); return; }
    int run = -1;
    for (int i = 0; i <= n; ++i) {
        bool hid = i == n;
        for (int m = 0; !hid && m < lv_nmask; ++m) if (lv_in(lv_masks[m], x + i, y)) hid = true;
        if (!hid && run < 0) run = i;
        else if (hid && run >= 0) { lv_raw_draw(y, x + run, i - run, idx + run); run = -1; }
    }
}

/* ---------- overlay scenes ---------- */
static uint16_t lv_ink(const uint32_t *t) { return lv_hex(t[lv_scr == LV_SC_CTL ? 0 : lv_theme]); }

/* ticker: stripes + scrolling bold text. tx0 = where the text span starts (x of the scroll window's left) */
struct LvTickCtx { int span0, span1; };
static void lv_scene_ticker(LvBand &B, void *vctx)
{
    LvTickCtx *c = (LvTickCtx *)vctx;
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
static void lv_tk_next(void)
{
    lv_tk_off = 0;
    if (lv_job != LV_J_NONE) {
        lv_tk_alert = false;
        if (lv_job == LV_J_DEAUTH) snprintf(lv_tk_msg, sizeof lv_tk_msg, "DEAUTH WATCH CH%d  %lu DEAUTH  %lu MGMT", lv_deauth_ch, (unsigned long)lv_deauth_n, (unsigned long)lv_mgmt_n);
        else snprintf(lv_tk_msg, sizeof lv_tk_msg, "SCANNING %s...%s%s", (lv_job == LV_J_ALL && lv_job_phase) ? "BLE" : (lv_job == LV_J_ALL ? "WIFI" : lv_job_name(lv_job)),
                      lv_hunt >= 0 ? "  HUNT " : "", lv_hunt >= 0 ? LV_HUNT_NAME[lv_hunt] : "");
        for (char *p = lv_tk_msg; *p; ++p) if (*p >= 'a' && *p <= 'z') *p -= 32;
        return;
    }
    if (lv_nev) {
        snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", lv_evq[0]); lv_tk_alert = lv_evalert[0];
        memmove(lv_evq[0], lv_evq[1], sizeof lv_evq[0] * 3); memmove(lv_evalert, lv_evalert + 1, 3); --lv_nev;
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
    const int span0 = tty ? 0 : 24, span1 = tty ? 195 : 236;
    const int w = (int)strlen(lv_tk_msg) * 12;
    lv_tk_off += 2;
    if (lv_tk_off > (span1 - span0 + 1) + w) lv_tk_next();
    LvTickCtx c{span0, span1};
    if (tty) lv_render(lv_blit, LV_R_TTY_TICK, lv_scene_ticker, &c);
    else { lv_render(lv_blit, LV_R_TICK_L, lv_scene_ticker, &c); lv_render(lv_blit, LV_R_TICK_R, lv_scene_ticker, &c); }
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
        const bool sel = has && k == lv_sel;
        const uint16_t bd = !has ? lv_dim(lv_hex(LV_TAN), 1, 2) : sel ? lv_hex(0xffffff) : lv_f[idx[k]].alert ? lv_hex(0xff2020) : lv_hex(LV_TAN);
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
static void lv_scene_tty(LvBand &B, void *)
{
    const uint16_t fr = lv_ink(LV_FRAME), hdr = lv_ink(LV_HDR);
    b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000);
    b_fill(B, 0, 24, 240, 1, lv_ink(LV_ACCD));
    b_fill(B, 0, 25, 240, 10, lv_hex(0x181818));
    b_text35(B, 3, 28, "TERMINALBOT", hdr);
    char t[40]; snprintf(t, sizeof t, "%s  TAP TOP/BOTTOM = SCROLL", lv_scroll ? "SCROLLED" : "LIVE");
    b_text35(B, 237 - lv_w35(t), 28, t, hdr);
    b_frame(B, 0, 36, 240, 126, fr);
    const int first = lv_scroll;
    for (int i = 0; i < LV_TTY_LINES; ++i) {
        const char *s = lv_line_back(first + (LV_TTY_LINES - 1 - i));
        const uint16_t ink = s[0] == '$' ? lv_hex(LV_PROMPT_INK) : s[0] == '!' ? lv_hex(0xff4040) : lv_hex(0xffffff);
        b_text(B, 4, 39 + i * 9, s, ink);
    }
    const int total = lv_tn < LV_TLINES ? lv_tn : LV_TLINES;
    if (total > LV_TTY_LINES) {
        const int h = 118 * LV_TTY_LINES / total, y = 39 + (118 - h) * (total - LV_TTY_LINES - lv_scroll) / (total - LV_TTY_LINES);
        b_fill(B, 236, 39, 2, 118, lv_hex(0x202020)); b_fill(B, 236, y, 2, h, hdr);
    }
    int x = b_text(B, 2, 164, LV_PROMPT " ", lv_hex(LV_PROMPT_INK));
    const char *in = lv_inb; const int maxc = (238 - x - 8) / 6;
    if (lv_inlen > maxc) in += lv_inlen - maxc;
    x = b_text(B, x, 164, in, lv_hex(0xffffff));
    if ((lv_now / 400) & 1) b_fill(B, x + 1, 164, 6, 8, lv_ink(LV_ACC));
}
static void lv_scene_close(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0xffffff));
    b_fill(B, B.x0 + 1, B.y0 + 1, B.w - 2, B.h - 2, lv_ink(LV_ACC));
    b_text(B, B.x0 + (B.w - 29) / 2, B.y0 + 8, "CLOSE", lv_hex(0x000000));
}

/* ---------- flow ---------- */
static int lv_theme_seg(int base) { return base + lv_theme; }
static void lv_go(int s)
{
    lv_scr = s; lv_enter = true;
    lv_key_hl = -1;
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
    lv_dirty_box = lv_dirty_term = lv_dirty_ctl = lv_dirty_tty = lv_dirty_kbd = true;
}
static int lv_period(void) { return lv_scr == LV_SC_CTL ? 90 : 30; }

/* ---------- CALbot: power-on touch calibration screen (CALbot -> BOOTbot -> GIBSONbot home) ----------
 * Four hold-to-capture corner chips in LovyanGFX native order TL, BL, TR, BR (the order the old blocking cal
 * used). Each hold takes 32 raw XPT2046 samples >= 15 ms apart; rejected if the spread is >= 150 raw (old
 * rule); else the mean of the middle 16 (a trimmed median, ~1/2 the noise of the old 16-sample median).
 * The chips sit inset from the glass edge (a finger can hold them), so the four medians are fitted with a
 * least-squares affine screen->raw map and extrapolated to the TRUE corners (0,0) (0,319) (239,0) (239,319).
 * Those 8 raws are exactly what the old cal stored: same c0..c7 + cv NVS keys, same LovyanGFX 8-point matrix. */
#define LV_CAL_IN 24                         /* chip centre inset from the glass edge */
static const int16_t LV_CAL_PT[4][2] = {{LV_CAL_IN, LV_CAL_IN}, {LV_CAL_IN, 319 - LV_CAL_IN},
                                        {239 - LV_CAL_IN, LV_CAL_IN}, {239 - LV_CAL_IN, 319 - LV_CAL_IN}};
enum { LV_CAL_NVS = 0, LV_CAL_RUN = 1, LV_CAL_OK = 2 };
static int lv_cal_phase = LV_CAL_RUN, lv_cal_n = 0, lv_cal_ns = 0, lv_cal_msg = 0;
static bool lv_cal_nvs = false, lv_cal_lift = false, lv_cal_saved = false;
static uint32_t lv_cal_t0, lv_cal_last, lv_cal_up_t, lv_cal_hold0, lv_cal_deadline;
#define LV_CAL_NS 32                         /* samples per hold, >= 15 ms apart (~0.5 s) */
static int lv_cal_sx[LV_CAL_NS], lv_cal_sy[LV_CAL_NS];
static uint16_t lv_cal_med[4][2], lv_cal_c8[8];
static bool lv_cal_dirty_chip[4], lv_cal_dirty_txt, lv_cal_all;
static const char *const LV_CAL_MSG[] = {"", "HOLD STILL - TRY AGAIN", "TIMEOUT - SAVED CAL KEPT", "NVS SAVE FAILED"};

/* 4 medians at the inset chip centres -> raw at the 4 true corners (c0..c7). Pure math, host-testable. */
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
            double v = a + b * tx + c * ty + 0.5;
            if (v < 0) v = 0; if (v > 4095) v = 4095;
            out[i * 2 + ax] = (uint16_t)v;
        }
    }
}
static void lv_cal_start_run(uint32_t now)
{
    lv_cal_phase = LV_CAL_RUN; lv_cal_n = 0; lv_cal_ns = 0; lv_cal_lift = true; lv_cal_msg = 0;
    lv_cal_deadline = now + 15000; lv_cal_all = true;
}
static void lv_cal_finish(uint32_t now)
{
    lv_cal_solve(lv_cal_med, lv_cal_c8);
    const bool ok = lv_radio && lv_radio->cal_save && lv_radio->cal_save(lv_cal_c8);
    if (ok) { lv_cal_phase = LV_CAL_OK; lv_cal_saved = true; lv_cal_t0 = now; lv_cal_msg = 0; lv_cal_all = true; return; }
    lv_cal_msg = 3;
    if (lv_cal_nvs) { lv_cal_phase = LV_CAL_OK; lv_cal_t0 = now; lv_cal_all = true; }  /* old cal still applied */
    else { lv_cal_start_run(now); lv_cal_msg = 3; }
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
    if (lv_cal_lift) { if (!down) lv_cal_lift = false; return; }
    if (!down) {                                      /* resistive drop-outs: > 40 ms up restarts the hold */
        if (lv_cal_ns && (int32_t)(now - lv_cal_up_t) > 40) { lv_cal_ns = 0; lv_cal_dirty_chip[lv_cal_n] = true; }
        return;
    }
    lv_cal_up_t = now;
    if (lv_cal_ns && (int32_t)(now - lv_cal_last) < 15) return;
    lv_cal_last = now;
    lv_cal_sx[lv_cal_ns] = rx; lv_cal_sy[lv_cal_ns] = ry; ++lv_cal_ns;
    lv_cal_dirty_chip[lv_cal_n] = true;
    if (lv_cal_ns < LV_CAL_NS) return;
    int sx[LV_CAL_NS], sy[LV_CAL_NS];
    for (int k = 0; k < LV_CAL_NS; ++k) { sx[k] = lv_cal_sx[k]; sy[k] = lv_cal_sy[k]; }
    for (int a = 0; a < LV_CAL_NS - 1; ++a) for (int b = a + 1; b < LV_CAL_NS; ++b) {
        if (sx[a] > sx[b]) { int t = sx[a]; sx[a] = sx[b]; sx[b] = t; }
        if (sy[a] > sy[b]) { int t = sy[a]; sy[a] = sy[b]; sy[b] = t; }
    }
    lv_cal_ns = 0; lv_cal_lift = true; lv_cal_dirty_txt = true;
    if (sx[LV_CAL_NS - 1] - sx[0] >= 150 || sy[LV_CAL_NS - 1] - sy[0] >= 150) { lv_cal_msg = 1; return; }
    lv_cal_msg = 0;
    int mx = 0, my = 0;                               /* mean of the middle half (outliers dropped) */
    for (int k = LV_CAL_NS / 4; k < LV_CAL_NS * 3 / 4; ++k) { mx += sx[k]; my += sy[k]; }
    lv_cal_med[lv_cal_n][0] = (uint16_t)((mx + LV_CAL_NS / 4) / (LV_CAL_NS / 2)); lv_cal_med[lv_cal_n][1] = (uint16_t)((my + LV_CAL_NS / 4) / (LV_CAL_NS / 2));
    lv_cal_dirty_chip[lv_cal_n] = true;
    lv_cal_deadline = now + 15000;
    if (++lv_cal_n < 4) { lv_cal_dirty_chip[lv_cal_n] = true; return; }
    lv_cal_finish(now);
}

/* drawing: GIBSONbot chip style, CALbot's own fixed colours (not themed) */
#define LV_CAL_BODY  0x6b7584
#define LV_CAL_SHADE 0x4a494a
#define LV_CAL_PLATE 0x181818
#define LV_CAL_GOLD  0xffb200
#define LV_CAL_LIME  0x8cf01e
#define LV_CAL_LIMED 0x5aa010
#define LV_CAL_OFF   0x2a2e33
static void lv_cal_wave(LvBand &B, int x0, int x1, int y, int hgt, int th, int per)
{
    const uint16_t g = lv_hex(LV_CAL_GOLD);
    for (int x = x0, k = 0; x + per / 2 <= x1; x += per / 2, ++k) {
        const int yy = (k & 1) ? y + hgt - th : y;
        b_fill(B, x, yy, per / 2 + th, th, g);         /* flat */
        b_fill(B, x, y, th, hgt, g);                    /* edge */
    }
    b_fill(B, x1 - th + 1, y, th, hgt, g);
}
static void lv_cal_chip_body(LvBand &B, int x0, int y0, int w, int h, int pin, int gap, int cut)
{
    b_fill(B, x0, y0, w, h, lv_hex(LV_CAL_BODY));
    for (int k = 0; k < cut; ++k) b_fill(B, x0 + w - cut + k, y0 + h - 1 - k, cut - k, 1, lv_hex(LV_CAL_SHADE));
    const uint16_t g = lv_hex(LV_CAL_GOLD);
    for (int y = y0 + gap; y + pin <= y0 + h - gap / 2; y += pin + gap) {
        b_fill(B, x0 - pin - 2, y, pin, pin, g); b_fill(B, x0 + w + 2, y, pin, pin, g);
    }
    for (int x = x0 + gap; x + pin <= x0 + w - gap / 2; x += pin + gap) b_fill(B, x, y0 + h + 2, pin, pin, g);
}
static void lv_scene_cal(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, 0);
    const uint16_t dim = lv_hex(0x210c31);
    for (int y = 12; y < 320; y += 24) for (int x = 12; x < 240; x += 24) { b_fill(B, x - 2, y, 5, 1, dim); b_fill(B, x, y - 2, 1, 5, dim); }
    /* centre chip: plate, gold wave, body, gold pins, solid black face frame, lime face */
    b_fill(B, 34, 58, 172, 212, lv_hex(LV_CAL_PLATE));
    lv_cal_wave(B, 48, 191, 64, 14, 2, 10);
    lv_cal_chip_body(B, 52, 86, 136, 160, 5, 5, 12);
    b_fill(B, 66, 98, 108, 92, 0);
    b_fill(B, 71, 103, 98, 82, lv_hex(LV_CAL_LIME));
    b_fill(B, 71, 179, 98, 6, lv_hex(LV_CAL_LIMED));
    b_fill(B, 92, 126, 18, 16, 0); b_fill(B, 130, 126, 18, 16, 0);            /* eyes */
    b_fill(B, 106, 160, 28, 4, 0);                                              /* mouth */
    b_text(B, 120 - 36, 198, "CALbot", 0x0000, 2);
    char st[24];
    if (lv_cal_phase == LV_CAL_RUN) snprintf(st, sizeof st, "CAL %d OF 4", lv_cal_n + 1);
    else snprintf(st, sizeof st, "CAL OK");
    b_text(B, 120 - (int)strlen(st) * 6, 220, st, lv_hex(LV_CAL_LIME), 2);
    /* corner chips: same style, plain, unlabeled */
    for (int i = 0; i < 4; ++i) {
        const int cx = LV_CAL_PT[i][0], cy = LV_CAL_PT[i][1];
        if (cx + 21 <= B.x0 || cx - 21 >= B.x0 + B.w || cy + 21 <= B.y0 || cy - 21 >= B.y0 + B.h) continue;
        b_fill(B, cx - 20, cy - 20, 41, 41, lv_hex(LV_CAL_PLATE));
        lv_cal_wave(B, cx - 15, cx + 15, cy - 18, 5, 1, 6);
        lv_cal_chip_body(B, cx - 13, cy - 11, 27, 27, 2, 3, 4);
        b_fill(B, cx - 9, cy - 8, 19, 19, 0);
        const bool done = lv_cal_phase != LV_CAL_RUN || i < lv_cal_n, act = lv_cal_phase == LV_CAL_RUN && i == lv_cal_n;
        uint16_t face = lv_hex(done ? LV_CAL_LIME : LV_CAL_OFF);
        if (act) face = lv_hex(((lv_now / 400) & 1) ? LV_CAL_LIMED : LV_CAL_OFF);
        b_fill(B, cx - 6, cy - 5, 13, 13, face);
        if (act && lv_cal_ns) { const int f = (13 * lv_cal_ns + LV_CAL_NS - 1) / LV_CAL_NS; b_fill(B, cx - 6, cy + 8 - f, 13, f, lv_hex(LV_CAL_LIME)); }
    }
    const char *h1 = lv_cal_phase == LV_CAL_RUN ? "HOLD THE LIT CHIP" : lv_cal_phase == LV_CAL_NVS ? "SAVED CAL LOADED" : "SAVED TO NVS";
    if (lv_cal_phase == LV_CAL_OK && !lv_cal_saved) h1 = "SAVED CAL KEPT";
    b_text(B, 120 - (int)strlen(h1) * 3, 278, h1, lv_hex(0xc8c0a8), 1);
    const char *h2 = lv_cal_msg ? LV_CAL_MSG[lv_cal_msg] : lv_cal_phase == LV_CAL_NVS ? "HOLD 1s TO RECAL" : "";
    b_text(B, 120 - (int)strlen(h2) * 3, 292, h2, lv_hex(lv_cal_msg ? 0xffb200 : 0x8c8c8c), 1);
}
static const LvRect LV_R_CAL_TXT = {54, 196, 186, 238}, LV_R_CAL_HINT = {48, 276, 191, 301};
static LvRect lv_cal_chip_rect(int i) { return {(int16_t)(LV_CAL_PT[i][0] - 20), (int16_t)(LV_CAL_PT[i][1] - 20), (int16_t)(LV_CAL_PT[i][0] + 20), (int16_t)(LV_CAL_PT[i][1] + 20)}; }
static void lv_cal_tick(uint32_t now)
{
    if (lv_cal_phase == LV_CAL_NVS && !lv_cal_hold0 && (int32_t)(now - lv_cal_t0) >= 2000) { lv_go(LV_SC_BOOT); lv_clip_t = now; return; }
    if (lv_cal_phase == LV_CAL_OK && (int32_t)(now - lv_cal_t0) >= 1200) { lv_go(LV_SC_BOOT); lv_clip_t = now; return; }
    if (lv_cal_phase == LV_CAL_RUN && lv_cal_nvs && (int32_t)(now - lv_cal_deadline) >= 0) {
        lv_cal_phase = LV_CAL_OK; lv_cal_saved = false; lv_cal_msg = 2; lv_cal_t0 = now; lv_cal_all = true;   /* old cal untouched */
    }
    if (lv_cal_phase == LV_CAL_RUN && (now / 400) != ((now - 30) / 400)) lv_cal_dirty_chip[lv_cal_n] = true;  /* blink */
    static int ph = -1, n = -1, msg = -1;
    if (ph != lv_cal_phase || n != lv_cal_n || msg != lv_cal_msg) { lv_cal_dirty_txt = true; ph = lv_cal_phase; n = lv_cal_n; msg = lv_cal_msg; }
    if (lv_enter || lv_cal_all) { lv_render(lv_blit, {0, 0, 239, 319}, lv_scene_cal, 0); lv_enter = lv_cal_all = false; lv_cal_dirty_txt = false; for (int i = 0; i < 4; ++i) lv_cal_dirty_chip[i] = false; return; }
    for (int i = 0; i < 4; ++i) if (lv_cal_dirty_chip[i]) { lv_render(lv_blit, lv_cal_chip_rect(i), lv_scene_cal, 0); lv_cal_dirty_chip[i] = false; }
    if (lv_cal_dirty_txt) { lv_render(lv_blit, LV_R_CAL_TXT, lv_scene_cal, 0); lv_render(lv_blit, LV_R_CAL_HINT, lv_scene_cal, 0); lv_cal_dirty_txt = false; }
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
        if (all || lv_dirty_tty) { lv_render(lv_blit, LV_R_TTY_BODY, lv_scene_tty, 0); lv_dirty_tty = false; }
        if (all || lv_dirty_kbd) { lv_render(lv_blit, LV_R_KBD, lv_scene_kbd, 0); lv_dirty_kbd = false; }
        if (all) lv_render(lv_blit, LV_R_TTY_CLOSE, lv_scene_close, 0);
        return;
    }
    if (lv_scr == LV_SC_SCAN) {
        if (all || lv_dirty_box) { lv_render(lv_blit, LV_R_SBOX, lv_scene_box, 0); lv_dirty_box = false; }
        if (all || lv_dirty_term) { lv_render(lv_blit, LV_R_STERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    }
    if (lv_scr == LV_SC_TERM && (all || lv_dirty_term)) { lv_render(lv_blit, LV_R_TTERM, lv_scene_minterm, 0); lv_dirty_term = false; }
    if (lv_scr == LV_SC_CTL && (all || lv_dirty_ctl)) {
        lv_render(lv_blit, LV_R_CTHEME, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CUP, lv_scene_ctl, 0);
        lv_render(lv_blit, LV_R_CFOCUS, lv_scene_ctl, 0); lv_render(lv_blit, LV_R_CMOTION, lv_scene_ctl, 0);
        lv_dirty_ctl = false;
    }
}

/* public API */
/* cal_valid: a cv=2 8-point cal is already in NVS (and was applied at display begin). */
static void lv_begin(const LvRadio *radio, lv_blit_fn blit, lk_draw_fn raw_draw, int theme, uint32_t now, bool cal_valid)
{
    lv_radio = radio; lv_blit = blit; lv_raw_draw = raw_draw;
    lv_theme = (theme >= 0 && theme < LV_NTHEME) ? theme : 0;
    lv_now = lv_boot_ms = now; lv_clip_t = now;
    lv_print("BOTCORPS %s", LV_VERSION);
    lv_print("type 'help'. scans: tap SCAN or 'scan'");
    lv_cal_nvs = cal_valid; lv_cal_t0 = now; lv_cal_hold0 = 0;
    if (cal_valid) lv_cal_phase = LV_CAL_NVS; else lv_cal_start_run(now);
    lv_go(LV_SC_CAL);
}
/* call every ~30 ms. io.draw must be lv_clip_draw. */
static void lv_tick(const LkIo &io, uint32_t now)
{
    lv_now = now;
    if (lv_scr == LV_SC_CAL) { lv_cal_tick(now); return; }
    lv_job_tick();
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
    if (lv_scr == LV_SC_TERM && (now / 500) != ((now - 30) / 500)) lv_dirty_term = true;
    if (lv_scr == LV_SC_TTY && (now / 400) != ((now - 30) / 400)) lv_dirty_tty = true;
    if (lv_key_hl >= 0 && (int32_t)(now - lv_key_hl_t) > 120) { lv_key_hl = -1; lv_dirty_kbd = true; }
    if (lv_scr != LV_SC_BOOT && lv_scr != LV_SC_HOME_INTRO && (lv_scr == LV_SC_TTY || lv_play.shown >= 0)) {
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

/* Tap at portrait (x,y). Returns a short name of what it did (for logs / the sim). */
static const char *lv_tap(int x, int y)
{
    if (lv_scr == LV_SC_CAL) return "cal-raw-only";
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) { lv_go(LV_SC_HOME); if (lv_selftest_pending) { lv_selftest_pending = false; lv_selftest(); } return "skip-intro"; }
    if (lv_scr == LV_SC_TTY) {
        if (lv_in(LV_R_TTY_CLOSE, x, y)) { lv_go(LV_SC_TERM); return "tty-close"; }
        if (lv_in(LV_R_TTY_TICK, x, y)) { lv_go(LV_SC_SCAN); return "ticker->scan"; }
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
    /* top bar: crown = GIBSONbot home (SPEC X1: the crown is the back control), chip = back, ticker = jump to the find */
    if (lv_in(LV_R_CROWN, x, y)) { if (lv_scr != LV_SC_HOME) lv_go(LV_SC_HOME); return "crown-home"; }
    if (lv_scr != LV_SC_HOME && lv_in(LV_R_BACK, x, y)) { lv_go(LV_SC_HOME); return "back-home"; }
    if (y <= 23 && x >= 24) { lv_mode = 0; if (lv_scr != LV_SC_SCAN) lv_go(LV_SC_SCAN); return "ticker->scan"; }
    if (lv_scr == LV_SC_HOME) {
        if (lv_in({18, 22, 50, 62}, x, y)) { lv_selftest_pending = true; lv_go(LV_SC_BOOT); return "boot-selftest"; }
        if (lv_in({68, 20, 158, 130}, x, y)) { lv_job_start(LV_J_ALL, -1); return "gibson-jackin-scan-all"; }
        if (lv_in({170, 20, 205, 75}, x, y)) { lv_go(LV_SC_CTL); return "ctl"; }
        if (lv_in({18, 135, 105, 215}, x, y)) { lv_go(LV_SC_SCAN); return "scan"; }
        if (lv_in({115, 135, 205, 215}, x, y)) { lv_go(LV_SC_TERM); return "term"; }
        return "none";
    }
    if (lv_scr == LV_SC_TERM) {
        static const LvRect HOT[LV_NHUNT] = {{35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
                                             {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250}};
        for (int h = 0; h < LV_NHUNT; ++h)
            if (lv_in(HOT[h], x, y)) { char c[24]; snprintf(c, sizeof c, "hunt %s", LV_HUNT_NAME[h]); lv_open_tty(c); return LV_HUNT_NAME[h]; }
        if (lv_in({183, 272, 204, 285}, x, y)) { lv_open_tty("scan"); return "term-target-scan"; }
        if (lv_in({183, 286, 204, 297}, x, y)) { lv_open_tty("list"); return "term-folder-list"; }
        if (lv_in({183, 298, 204, 311}, x, y)) { lv_open_tty("help"); return "term-menu-help"; }
        if (lv_in({33, 262, 205, 310}, x, y) || lv_in({75, 30, 165, 150}, x, y)) { lv_open_tty(0); return "tty"; }
        return "none";
    }
    if (lv_scr == LV_SC_SCAN) {
        if (lv_in({32, 30, 72, 88}, x, y) || lv_in({78, 155, 116, 182}, x, y)) { lv_mode = 1; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(LV_J_WIFI, -1); return "wifi-scan"; }
        if (lv_in({32, 95, 72, 150}, x, y) || lv_in({122, 155, 160, 182}, x, y)) { lv_mode = 2; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(LV_J_BLE, -1); return "ble-scan"; }
        if (lv_in({34, 155, 72, 182}, x, y)) { lv_mode = 0; lv_page = 0; lv_sel = -1; lv_detail = false; lv_job_start(LV_J_ALL, -1); return "all-scan"; }
        if (lv_in({166, 155, 204, 182}, x, y)) { lv_mode = 3; lv_detail = false; lv_job_start(LV_J_DEAUTH, LV_H_DEAUTH); return "deauth-watch"; }
        if (lv_in({80, 30, 160, 150}, x, y) || lv_in(LV_R_BTN_SCAN, x, y)) {
            if (lv_job != LV_J_NONE) { lv_job_stop(); return "stop"; }
            lv_job_start(lv_mode == 1 ? LV_J_WIFI : lv_mode == 2 ? LV_J_BLE : lv_mode == 3 ? LV_J_DEAUTH : LV_J_ALL, lv_mode == 3 ? LV_H_DEAUTH : -1); return "scan-button";
        }
        if (lv_in({168, 60, 208, 118}, x, y)) { lv_go(LV_SC_HOME); return "gib-home"; }
        if (lv_in(LV_R_BTN_MORE, x, y)) { int idx[LV_MAXF]; const int n = lv_view(idx); lv_page = n > 8 ? (lv_page + 1) % ((n + 7) / 8) : 0; lv_sel = -1; lv_detail = false; lv_dirty_box = true; return "more-page"; }
        if (lv_in(LV_R_SBOX, x, y)) {
            if (lv_detail) { lv_detail = false; lv_dirty_box = true; return "detail-back"; }
            for (int t = 0; t < 8; ++t)
                if (lv_in({(int16_t)(lv_tile_x(t) - 1), (int16_t)(lv_tile_y(t) - 1), (int16_t)(lv_tile_x(t) + 28), (int16_t)(lv_tile_y(t) + 28)}, x, y)) {
                    int idx[LV_MAXF]; const int n = lv_view(idx); const int k = lv_page * 8 + t;
                    if (k >= n) return "empty-tile";
                    if (lv_sel == k) { lv_detail = true; lv_dirty_box = true; return "detail"; }
                    lv_sel = k; lv_dirty_box = true;
                    char l[96]; lv_find_line(lv_f[idx[k]], l, sizeof l); snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", l); lv_tk_alert = lv_f[idx[k]].alert; lv_tk_off = 0;
                    return "select";
                }
            return "none";
        }
        if (lv_in({184, 274, 205, 290}, x, y)) { lv_mode = 0; lv_job_start(LV_J_ALL, -1); return "target-scan-all"; }
        if (lv_in({184, 291, 205, 302}, x, y)) { lv_open_tty("list"); return "folder-list"; }
        if (lv_in({184, 303, 205, 316}, x, y)) { lv_open_tty("help"); return "menu-help"; }
        if (lv_in({30, 262, 183, 312}, x, y)) { lv_open_tty(0); return "tty"; }
        return "none";
    }
    if (lv_scr == LV_SC_CTL) {
        if (lv_in({35, 33, 113, 48}, x, y)) { lv_set_theme(lv_theme + 1); lv_event(false, "THEME %d/6 %s", lv_theme + 1, LV_THEME_NAME[lv_theme]); lv_tk_off = 1 << 14; return "theme"; }
        if (lv_in({35, 49, 113, 63}, x, y)) { char h[48] = ""; lv_radio->sys(0, h, sizeof h); lv_event(false, "UP %lus  %s", (unsigned long)(lv_now / 1000), h); lv_tk_off = 1 << 14; return "uptime"; }
        if (lv_in({35, 64, 113, 79}, x, y)) { lv_focus = (lv_focus + 1) % 4; lv_tk_item = 0; lv_dirty_ctl = true; lv_event(false, "TICKER FOCUS: %s", LV_FOCUS_NAME[lv_focus]); lv_tk_off = 1 << 14; return "focus"; }
        if (lv_in({160, 222, 208, 256}, x, y)) { lv_motion = !lv_motion; lv_dirty_ctl = true; lv_event(false, "BG MOTION %s", lv_motion ? "ON" : "OFF"); lv_tk_off = 1 << 14; return "bg-motion"; }
        if (lv_in({33, 272, 180, 284}, x, y)) { lv_open_tty("go ble"); return "go-ble"; }
        if (lv_in({33, 285, 180, 293}, x, y)) { lv_go(LV_SC_SCAN); return "go-scan"; }
        if (lv_in({33, 262, 205, 310}, x, y)) { lv_open_tty(0); return "tty"; }
        if (lv_in({75, 82, 165, 205}, x, y)) { lv_open_tty("status"); return "ctl-status"; }
        static const LvRect NA[] = {{35, 85, 70, 140}, {35, 145, 70, 190}, {35, 195, 70, 250}, {165, 80, 205, 130}, {170, 131, 205, 170}, {165, 175, 205, 222}, {90, 212, 160, 258}};
        static const char *const NAN_[] = {"MIX", "RAIN COL", "MASHUP", "RAIN", "RAIN MODE", "MULTI", "ORIENT"};
        for (int i = 0; i < 7; ++i) if (lv_in(NA[i], x, y)) { lv_event(false, "%s: NOT IN LIVESCAN BUILD", NAN_[i]); lv_tk_off = 1 << 14; return NAN_[i]; }
        return "none";
    }
    return "none";
}
