/*
 * Screens from the approved desktop mock. Keyboard only.
 *
 * INT name bar: 1R repainted "BOTcorps" every rain step inside the
 * EXT sprite (paint_rain at e8fe1e68 src/rain/botcorps_rain.cpp:95).
 * 1R2 left a single nav_draw on the menu (158025bb rain.cpp:425-431)
 * and loop() never touched botcorps_menu() again. Rain still called
 * setTextSize(1) on the content sprite (rain.cpp draw_code). The menu
 * paint was not repeated, so a size-1 boot font or a clip that hid
 * the 22px bar and the rule stayed on the glass as a small top line.
 * This file paints Font0 at setTextSize(2, 2), white on #121821, with
 * the mock rule, on botcorps_menu() every INT frame. Rain never
 * receives the menu device.
 *
 * Wi-Fi and Bluetooth do not start a radio. Lists are the mock's
 * sample rows under a RADIO OFF header. The EXT sprite is released
 * on those screens when the largest free block is under 48 KB.
 */
#include "ui/botcorps_ui.h"

#include "display/botcorps_display.h"
#include "rain/botcorps_rain.h"

#include <lgfx/v1/LGFX_Sprite.hpp>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <Preferences.h>

static const uint16_t COL_BG     = 0x0862; /* #0b0f14 */
static const uint16_t COL_BAR    = 0x10C4; /* #121821 */
static const uint16_t COL_PANEL2 = 0x1906; /* #1a2330 */
static const uint16_t COL_LINE   = 0x29C8; /* #2c3846 */
static const uint16_t COL_INK    = 0xE77E; /* #e6edf3 */
static const uint16_t COL_MUTED  = 0x8CD4; /* #8d99a6 */
static const uint16_t COL_AMBER  = 0xF5A9; /* #f0b44a */
static const uint16_t COL_SEL    = 0x1105; /* #16202b */
static const uint16_t COL_WHITE  = 0xFFFF;
static const uint16_t COL_RAIN   = 0x0041; /* #070a0e */
static const uint16_t COL_EYE    = 0x0080;
static const uint16_t COL_IVORY  = 0xF79C;
static const uint16_t COL_IVORY2 = 0xCE16;
static const uint16_t COL_BLACK  = 0x0020;
static const uint16_t COL_READY  = 0x8FF3;
static const uint16_t COL_LIM    = 0xFE8D;
static const uint16_t COL_HOLD_C = 0xE38E;
static const uint16_t COL_DIMTXT = 0xC67A;
static const uint16_t COL_DIS    = 0x5B0D;
static const uint16_t COL_HINTTX = 0x9D15;
static const uint16_t COL_DEEP   = 0x0883;
static const uint16_t COL_POWER  = 0xDFBF;
static const uint16_t COL_CODE   = 0x7E6C;
static const uint16_t COL_HEAD   = 0xEFF9;

static const uint16_t ACCENT_C[3] = { 0x4E7B, 0xF5A9, 0x5FF8 };
static const char *const ACCENT_N[3] = { "CYAN", "AMBER", "MINT" };

enum { RADIO_NONE = 0, RADIO_WIFI = 1, RADIO_BLE = 2 };
enum { ST_READY = 0, ST_LIMITED = 1, ST_HOLD = 2 };

struct bot_t {
    const char *id;
    uint16_t c;
    uint16_t c2;
    const char *target;
    uint8_t radio;
    uint8_t st;
};

static const bot_t BOTS[] = {
    { "wifibot",       0x5D3F, 0x1B75, "wi-fi",   RADIO_WIFI, ST_LIMITED },
    { "bluetoothbot",  0x4E7F, 0x2B76, "ble",     RADIO_BLE,  ST_LIMITED },
    { "deauthbot",     0x5D3F, 0x3B3A, "deauth",  RADIO_NONE, ST_READY },
    { "hackbot",       0x7711, 0x246E, "hack",    RADIO_NONE, ST_READY },
    { "flockbot",      0x9F0B, 0x4D6E, "flock",   RADIO_NONE, ST_READY },
    { "dronebot",      0xFB72, 0xD1EE, "drone",   RADIO_NONE, ST_READY },
    { "airtagbot",     0xAC7F, 0x6A99, "air tag", RADIO_NONE, ST_READY },
    { "keyfobbot",     0x36B8, 0x1434, "keyfob",  RADIO_NONE, ST_READY },
    { "metabot",       0x9F0B, 0x4D6E, "meta",    RADIO_NONE, ST_READY },
    { "skimmerbot",    0x36B8, 0x1434, "skimmer", RADIO_NONE, ST_HOLD },
    { "flipperbot",    0xFC6B, 0xC2C5, "flipper", RADIO_NONE, ST_LIMITED },
    { "samsungtagbot", 0xBD3F, 0x6AD8, "samsung", RADIO_NONE, ST_LIMITED },
};
static const int BOT_N = 12;
static const int PLAIN[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const int PLAIN_N = 10;

enum {
    SCR_RAIN = 0,
    SCR_INTRO,
    SCR_GIBSON,
    SCR_ROLE,
    SCR_TERMINAL,
    SCR_WIFI,
    SCR_WIFI_RADAR,
    SCR_WIFI_LIST,
    SCR_BT,
    SCR_BT_RADAR,
    SCR_BT_LIST,
    SCR_BOT,
    SCR_BOT_RADAR,
    SCR_CONTROL
};

struct near_t {
    const char *name;
    const char *rssi;
    uint8_t bars;
};

static const near_t NEAR_WIFI[3] = {
    { "cafe-guest", "-42 dBm", 4 },
    { "printer-2f", "-67 dBm", 2 },
    { "lobby-ap", "-71 dBm", 1 },
};
static const near_t NEAR_BLE[3] = {
    { "earbuds-58", "-58 dBm", 3 },
    { "watch-6", "-63 dBm", 2 },
    { "speaker-7", "-74 dBm", 1 },
};

static const int BAR_H = 22;
static const int HINT_H = 14;
static const int INT_GAP = 8;
static const int INT_COLS = 32;
static const char CODE_GLYPHS[] = "01<>[]{}:/\\|+-=*#";
static const uint8_t TRAIL_NUM[4] = { 150, 80, 40, 16 };

struct int_col_t {
    int16_t y;
    uint8_t kind;
    uint8_t hat;
    uint8_t color;
    uint8_t jitter;
};

static int_col_t s_int[INT_COLS];
static int s_int_n = 0;
static uint16_t s_dim_bot[14];
static uint16_t s_dim_code = COL_CODE;
static uint16_t s_dim_head = COL_HEAD;
static uint32_t s_rng = 0x51F00Du;

static int s_screen = SCR_RAIN;
static int s_cursor = 0;
static int s_bot = 2;
static int s_term = 2;
static int s_color = 0;
static int s_bg = 2;
static int s_saver = 0;
static int s_saver_on = 1;
static int s_idle = 1;
static bool s_dirty = true;
static bool s_backdrop_ready = false;
static int s_heap_for = -1;
static uint32_t s_boot_ms = 0;
static uint32_t s_ui_ms = 0;
static uint32_t s_stat_ms = 0;
static uint32_t s_full_n = 0;
static uint32_t s_ext_ticks = 0;
static uint32_t s_int_ticks = 0;
static bool s_name_logged = false;
static float s_sweep = -100.0f;
static char s_clk_drawn[8];
static lgfx::LGFX_Sprite *s_anim = nullptr;
static int s_anim_w = 0;
static int s_anim_h = 0;

static const char *const RAIN_NAME[BOTCORPS_RAIN_COLORS] = {
    "MULTI", "GREEN", "CYAN", "BLUE", "PURPLE", "AMBER", "WHITE"
};
static const char *const NVS_NS = "botcorps";
static const char *const NVS_KEY = "rain_col";

static char s_clk[8];
static char s_up[8];
static char s_bat[8];
static char s_slot[16];
static char s_pair[16];
static char s_sec[8];

static uint32_t rnd_u(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return s_rng;
}

static int rnd_n(int n)
{
    if (n <= 1) return 0;
    return (int)(rnd_u() % (uint32_t)n);
}

static void unpack565(uint16_t p, int *r, int *g, int *b)
{
    const int r5 = (p >> 11) & 31;
    const int g6 = (p >> 5) & 63;
    const int b5 = p & 31;
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

static uint16_t pack565(int r, int g, int b)
{
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t scale565(uint16_t c, int num, int den)
{
    int r, g, b;
    if (den <= 0) return c;
    unpack565(c, &r, &g, &b);
    return pack565(r * num / den, g * num / den, b * num / den);
}

static uint16_t mix565(uint16_t c, uint16_t bg, int num)
{
    int r, g, b, br, bgc, bb;
    const int inv = 256 - num;
    unpack565(c, &r, &g, &b);
    unpack565(bg, &br, &bgc, &bb);
    return pack565((r * num + br * inv) >> 8,
                   (g * num + bgc * inv) >> 8,
                   (b * num + bb * inv) >> 8);
}

static uint16_t accent(void)
{
    if (s_color < 0 || s_color > 2) return ACCENT_C[0];
    return ACCENT_C[s_color];
}

static bool int_rain_on(void)
{
    return s_screen != SCR_TERMINAL && s_screen != SCR_CONTROL;
}

bool botcorps_ui_wants_ext_rain(void)
{
    /* Boot rain only. Other screens keep the last frame and do not
     * push the full sprite on the 90 ms tick. */
    return s_screen == SCR_RAIN;
}

static bool is_radio_screen(int id)
{
    return id == SCR_WIFI || id == SCR_WIFI_RADAR || id == SCR_WIFI_LIST
        || id == SCR_BT || id == SCR_BT_RADAR || id == SCR_BT_LIST;
}

static const char *screen_tag(int id)
{
    switch (id) {
    case SCR_WIFI: return "wifi";
    case SCR_WIFI_RADAR: return "wifi-radar";
    case SCR_WIFI_LIST: return "wifi-list";
    case SCR_BT: return "bluetooth";
    case SCR_BT_RADAR: return "bt-radar";
    case SCR_BT_LIST: return "bt-list";
    default: return "screen";
    }
}

static void note_heap_for(int id)
{
    if (!is_radio_screen(id)) {
        botcorps_rain_set_parked(false);
        return;
    }
    botcorps_rain_log_heap(screen_tag(id));
    if (botcorps_rain_sprite_held() && ESP.getMaxAllocHeap() < 48u * 1024u) {
        botcorps_rain_set_parked(true);
    } else if (botcorps_rain_sprite_held()) {
        Serial.println("[botcorps] heap radio sprite kept max_alloc>=48KB");
    } else {
        Serial.println("[botcorps] heap radio no sprite held");
    }
}

static void anim_release(void);

static void set_screen(int id)
{
    /* Leaving rain (and every other entry) is a full dirty paint.
     * The sprite stays until that paint has copied it. */
    anim_release();
    s_sweep = -100.0f;
    s_backdrop_ready = false;
    s_heap_for = -1;
    s_screen = id;
    s_cursor = 0;
    s_dirty = true;
}

static void build_dim(void)
{
    if (botcorps_rain_mode() == 0) {
        for (int i = 0; i < 14; ++i) {
            s_dim_bot[i] = scale565(botcorps_rain_palette(i), 140, 256);
        }
        s_dim_code = scale565(COL_CODE, 115, 256);
        s_dim_head = scale565(COL_HEAD, 115, 256);
        return;
    }
    const uint16_t ink = botcorps_rain_solid();
    const uint16_t dim = scale565(ink, 140, 256);
    for (int i = 0; i < 14; ++i) s_dim_bot[i] = dim;
    s_dim_code = scale565(ink, 115, 256);
    s_dim_head = scale565(ink, 170, 256);
}

static void load_rain_color(void)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) {
        botcorps_rain_set_mode(0);
        return;
    }
    const unsigned v = prefs.getUChar(NVS_KEY, 0);
    prefs.end();
    botcorps_rain_set_mode(v >= (unsigned)BOTCORPS_RAIN_COLORS ? 0 : (int)v);
}

static void save_rain_color(void)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return;
    prefs.putUChar(NVS_KEY, (uint8_t)botcorps_rain_mode());
    prefs.end();
}

static void anim_release(void)
{
    delete s_anim;
    s_anim = nullptr;
    s_anim_w = 0;
    s_anim_h = 0;
}

static void build_int_rain(void)
{
    s_int_n = (240 + INT_GAP - 1) / INT_GAP + 1;
    if (s_int_n > INT_COLS) s_int_n = INT_COLS;
    for (int i = 0; i < s_int_n; ++i) {
        uint8_t kind = (i % 2 == 0) ? 1 : 2;
        if (rnd_n(100) > 55) kind = 0;
        s_int[i].y = (int16_t)rnd_n(135);
        s_int[i].kind = kind;
        s_int[i].hat = (uint8_t)(i % 2 == 0);
        s_int[i].color = (uint8_t)rnd_n(14);
        s_int[i].jitter = (uint8_t)(20 + rnd_n(36));
    }
}

static void draw_tiny_bot(lgfx::LovyanGFX &g, int x, int y, uint16_t color, bool hat)
{
    const int top = hat ? 1 : 0;
    if (hat) g.fillRect(x, y, 7, 1, color);
    g.fillRect(x, y + top, 7, 6, color);
    const int ey = y + top + 2;
    g.fillRect(x + 1, ey, 2, 2, COL_EYE);
    g.fillRect(x + 4, ey, 2, 2, COL_EYE);
}

static void step_int_rain(lgfx::LovyanGFX &g, int width, int height)
{
    g.fillScreen(COL_RAIN);
    g.setFont(&fonts::Font0);
    g.setTextSize(1, 1);
    g.setTextWrap(false, false);
    g.setTextDatum(lgfx::textdatum::top_left);
    for (int i = 0; i < s_int_n; ++i) {
        int_col_t &c = s_int[i];
        if (c.kind == 0) continue;
        if (rnd_n(100) < c.jitter) continue;
        const int x = i * INT_GAP;
        const uint16_t body = (c.kind == 1) ? s_dim_bot[c.color % 14] : s_dim_code;
        for (int k = 3; k >= 0; --k) {
            const int y = c.y - k * INT_GAP;
            const uint16_t tone = mix565(k == 0 ? body : body, COL_RAIN, TRAIL_NUM[k]);
            if (c.kind == 1) {
                draw_tiny_bot(g, x, y, tone, c.hat != 0);
            } else {
                const bool head = ((i + (y >= 0 ? y / INT_GAP : 0)) % 7) == 0;
                const char ch = CODE_GLYPHS[rnd_n(18)];
                char buf[2] = { ch, 0 };
                g.setTextColor(k == 0 && head ? s_dim_head : tone);
                g.setCursor(x, y);
                g.print(buf);
            }
        }
        c.y = (int16_t)(c.y + INT_GAP);
        if (c.y > height + INT_GAP) {
            c.y = (int16_t)(-INT_GAP - rnd_n(3) * INT_GAP);
            if (c.kind == 1) c.color = (uint8_t)rnd_n(14);
        }
    }
    (void)width;
}

static void txt(lgfx::LovyanGFX &g, const char *s, int x, int y, float sz,
                uint16_t col, lgfx::textdatum_t dat)
{
    g.setFont(&fonts::Font0);
    g.setTextSize(sz, sz);
    g.setTextWrap(false, false);
    g.setTextDatum(dat);
    g.setTextColor(col);
    g.drawString(s, x, y);
}

static void plate(lgfx::LovyanGFX &g, int x, int y, int w, int h,
                  uint16_t fill, uint16_t stroke)
{
    if (w < 2 || h < 2) return;
    g.fillRoundRect(x, y, w, h, 3, fill);
    if (stroke) g.drawRoundRect(x, y, w, h, 3, stroke);
}

static void draw_blob(lgfx::LovyanGFX &g, int x, int y, int s, uint16_t c, uint16_t c2)
{
    if (s < 6) s = 6;
    g.fillEllipse(x, y, (s * 7) / 10, (s * 3) / 5, c2);
    g.fillEllipse(x - s / 10, y - s / 10, (s * 2) / 5, (s * 2) / 5, c);
    const int ew = s / 5;
    const int eh = s / 4;
    if (ew < 2 || eh < 2) return;
    const int ey = y - s / 8;
    g.fillRoundRect(x - (s / 3), ey, ew, eh, 1, COL_EYE);
    g.fillRoundRect(x + (s / 8), ey, ew, eh, 1, COL_EYE);
}

static void draw_gibson(lgfx::LovyanGFX &g, int x, int y, int s)
{
    if (s < 8) s = 8;
    g.fillEllipse(x, y, (s * 18) / 25, (s * 4) / 5, COL_IVORY2);
    g.fillEllipse(x - s / 12, y - s / 10, (s * 3) / 5, (s * 7) / 10, COL_IVORY);
    const int ly = y - s / 8;
    const int lh = s / 5 + 2;
    const int lw = s / 3;
    g.fillRect(x - s / 2, ly, lw, lh, COL_BLACK);
    g.fillRect(x + s / 7, ly, lw, lh, COL_BLACK);
    g.fillRect(x - s / 8, ly + lh / 3, s / 4, 2, COL_BLACK);
}

static const char *st_text(uint8_t st)
{
    if (st == ST_LIMITED) return "look only";
    if (st == ST_HOLD) return "on hold";
    return "ready";
}

static uint16_t st_col(uint8_t st)
{
    if (st == ST_LIMITED) return COL_LIM;
    if (st == ST_HOLD) return COL_HOLD_C;
    return COL_READY;
}

static const char *cp_value(void)
{
    switch (s_cursor) {
    case 0: return ACCENT_N[s_color];
    case 1: return s_bg == 0 ? "BOT+DEVICE" : s_bg == 1 ? "DEVICES" : "BOTS";
    case 2:
        if (s_saver == 1) return "GIBSON";
        if (s_saver == 2) return "FLYBY";
        if (s_saver == 3) return "BOUNCE";
        return "BOTS";
    case 3: return s_saver_on ? "ON" : "OFF";
    case 4: return s_idle == 0 ? "15 SEC" : s_idle == 2 ? "45 SEC" : "25 SEC";
    default: {
        const int m = botcorps_rain_mode();
        if (m < 0 || m >= BOTCORPS_RAIN_COLORS) return RAIN_NAME[0];
        return RAIN_NAME[m];
    }
    }
}

static const char *cp_focus_name(void)
{
    switch (s_cursor) {
    case 0: return "ui color";
    case 1: return "background";
    case 2: return "screen saver";
    case 3: return "saver";
    case 4: return "idle time";
    default: return "rain colour";
    }
}

static void fmt_times(void)
{
    const uint32_t sec = (millis() - s_boot_ms) / 1000u;
    snprintf(s_clk, sizeof(s_clk), "%02u:%02u",
             (unsigned)((sec / 3600u) % 24u),
             (unsigned)((sec / 60u) % 60u));
    snprintf(s_up, sizeof(s_up), "%02u:%02u",
             (unsigned)((sec / 60u) % 100u),
             (unsigned)(sec % 60u));
    const int lvl = M5.Power.getBatteryLevel();
    if (lvl < 0) snprintf(s_bat, sizeof(s_bat), "n/a");
    else snprintf(s_bat, sizeof(s_bat), "%d%%", lvl);
}

struct view_t {
    const char *k[4];
    const char *v[4];
    int nmeta;
    const char *chip;
    uint16_t chip_c;
    const char *focus;
    const char *item[4];
    int nitem;
    int cursor;
    uint8_t dis;
    const char *hints;
};

static const char *home_item(int radio, int i)
{
    if (i == 0) {
        if (radio == RADIO_BLE) return "ble scan";
        if (radio == RADIO_WIFI) return "wi-fi scan";
        return "radar";
    }
    if (i == 1) return "terminal";
    return "control panel";
}

static void fill_home(view_t *vw, int radio, int bot_index)
{
    const bot_t &b = BOTS[bot_index];
    vw->nmeta = 3;
    vw->k[0] = "sig";
    vw->v[0] = radio == RADIO_WIFI ? "-42" : radio == RADIO_BLE ? "-58" : "n/a";
    vw->k[1] = "near";
    vw->v[1] = radio ? "3" : "00";
    vw->k[2] = "next";
    vw->v[2] = home_item(radio, s_cursor);
    vw->chip = st_text(b.st);
    vw->chip_c = st_col(b.st);
    vw->nitem = 3;
    vw->item[0] = home_item(radio, 0);
    vw->item[1] = home_item(radio, 1);
    vw->item[2] = home_item(radio, 2);
    vw->cursor = s_cursor;
    vw->hints = radio ? ";. ENTER ESC" : ";. ENTER ,/ ESC";
}

static void fill_view(view_t *vw)
{
    memset(vw, 0, sizeof(*vw));
    vw->cursor = -1;
    vw->chip_c = accent();
    fmt_times();
    switch (s_screen) {
    case SCR_RAIN:
        vw->nmeta = 3;
        vw->k[0] = "up"; vw->v[0] = s_up;
        vw->k[1] = "mix"; vw->v[1] = "crew";
        vw->k[2] = "ms"; vw->v[2] = "90";
        vw->chip = "BOOT";
        vw->nitem = 1;
        vw->item[0] = "continue";
        vw->cursor = 0;
        vw->hints = "ENTER continue";
        break;
    case SCR_INTRO:
        vw->nmeta = 3;
        vw->k[0] = "clk"; vw->v[0] = s_clk;
        vw->k[1] = "up"; vw->v[1] = s_up;
        vw->k[2] = "bat"; vw->v[2] = s_bat;
        vw->chip = "IDLE";
        vw->nitem = 1;
        vw->item[0] = "power on";
        vw->cursor = 0;
        vw->hints = "ENTER power on";
        break;
    case SCR_GIBSON:
        vw->nmeta = 3;
        vw->k[0] = "clk"; vw->v[0] = s_clk;
        vw->k[1] = "up"; vw->v[1] = s_up;
        vw->k[2] = "next";
        vw->v[2] = s_cursor == 0 ? "terminal" : s_cursor == 1 ? "control panel" : "role card";
        vw->chip = "MENU";
        vw->nitem = 3;
        vw->item[0] = "terminal";
        vw->item[1] = "control panel";
        vw->item[2] = "role card";
        vw->cursor = s_cursor;
        vw->hints = ";. ENTER";
        break;
    case SCR_ROLE:
        vw->nmeta = 3;
        vw->k[0] = "clk"; vw->v[0] = s_clk;
        vw->k[1] = "view"; vw->v[1] = "ext";
        vw->k[2] = "page"; vw->v[2] = "1/1";
        vw->chip = "CARD";
        vw->nitem = 1;
        vw->item[0] = "back";
        vw->cursor = 0;
        vw->hints = "ENTER/ESC back";
        break;
    case SCR_TERMINAL: {
        const bot_t &b = BOTS[s_term];
        snprintf(s_slot, sizeof(s_slot), "%d/%d", s_term + 1, BOT_N);
        vw->nmeta = 3;
        vw->k[0] = "slot"; vw->v[0] = s_slot;
        vw->k[1] = "tgt"; vw->v[1] = b.target;
        vw->k[2] = "up"; vw->v[2] = s_up;
        vw->chip = st_text(b.st);
        vw->chip_c = st_col(b.st);
        vw->focus = b.id;
        vw->hints = ",/;. ENTER ESC";
        break;
    }
    case SCR_WIFI:
        fill_home(vw, RADIO_WIFI, 0);
        break;
    case SCR_BT:
        fill_home(vw, RADIO_BLE, 1);
        break;
    case SCR_BOT:
        fill_home(vw, RADIO_NONE, s_bot);
        break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_BOT_RADAR:
        vw->nmeta = 3;
        vw->k[0] = "sweep"; vw->v[0] = "0";
        vw->k[1] = "hits"; vw->v[1] = "0";
        vw->k[2] = "mode"; vw->v[2] = "idle";
        vw->chip = "IDLE";
        vw->chip_c = COL_AMBER;
        vw->item[0] = "track";
        vw->dis = 1;
        if (s_screen == SCR_BOT_RADAR) {
            vw->nitem = 2;
            vw->item[1] = "back";
        } else {
            vw->nitem = 3;
            vw->item[1] = "list";
            vw->item[2] = "back";
        }
        vw->cursor = s_cursor;
        vw->hints = ";. ENTER ESC";
        break;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST: {
        const near_t *rows = (s_screen == SCR_BT_LIST) ? NEAR_BLE : NEAR_WIFI;
        int idx = s_cursor;
        if (idx < 0) idx = 0;
        if (idx > 2) idx = 2;
        snprintf(s_pair, sizeof(s_pair), "%d/3", idx + 1);
        vw->nmeta = 4;
        vw->k[0] = "found"; vw->v[0] = "3";
        vw->k[1] = "sel"; vw->v[1] = s_pair;
        vw->k[2] = "rssi"; vw->v[2] = rows[idx].rssi;
        vw->k[3] = "bars";
        snprintf(s_sec, sizeof(s_sec), "%u", (unsigned)rows[idx].bars);
        vw->v[3] = s_sec;
        vw->chip = "IDLE";
        vw->chip_c = COL_AMBER;
        vw->nitem = 1;
        vw->item[0] = "back to radar";
        vw->cursor = -1;
        vw->hints = ";. on EXT  ESC";
        break;
    }
    case SCR_CONTROL:
        snprintf(s_sec, sizeof(s_sec), "%d/6", s_cursor + 1);
        vw->nmeta = 3;
        vw->k[0] = "sec"; vw->v[0] = s_sec;
        vw->k[1] = "val"; vw->v[1] = cp_value();
        vw->k[2] = "up"; vw->v[2] = s_up;
        vw->chip = "SET";
        vw->focus = cp_focus_name();
        vw->hints = ";. section ,/ ESC";
        break;
    default:
        break;
    }
}

static void paint_name(lgfx::LGFX_Device &menu, int width)
{
    menu.fillRect(0, 0, width, BAR_H, COL_BAR);
    menu.setFont(&fonts::Font0);
    menu.setTextSize(2, 2);
    menu.setTextWrap(false, false);
    menu.setTextDatum(lgfx::textdatum::top_left);
    menu.setTextColor(COL_WHITE, COL_BAR);
    menu.setCursor(6, 3);
    menu.print("BOTcorps");
    if (!s_name_logged) {
        s_name_logged = true;
        auto &content = botcorps_content();
        Serial.printf("[botcorps] int name menu %dx%d text_size=%d dual=%d menu_is_content=%d\n",
                      (int)menu.width(), (int)menu.height(),
                      (int)(menu.getTextSizeX() + 0.5f),
                      botcorps_dual_ok() ? 1 : 0,
                      (&menu == &content) ? 1 : 0);
    }
    txt(menu, s_clk, width - 6, 7, 1, accent(), lgfx::textdatum::top_right);
    menu.drawFastHLine(0, BAR_H, width, COL_LINE);
}

static void paint_int(lgfx::LGFX_Device &menu)
{
    const int width = menu.width();
    const int height = menu.height();
    menu.clearClipRect();
    menu.setClipRect(0, 0, width, height);
    if (int_rain_on()) step_int_rain(menu, width, height);
    else menu.fillScreen(COL_BG);

    fmt_times();
    paint_name(menu, width);

    view_t vw;
    fill_view(&vw);

    const int meta_y = BAR_H + 1;
    menu.fillRect(0, meta_y, width, 18, COL_BG);
    int mx = 4;
    menu.setFont(&fonts::Font0);
    menu.setTextSize(1, 1);
    for (int i = 0; i < vw.nmeta; ++i) {
        if (i) {
            menu.drawFastVLine(mx, meta_y + 4, 9, COL_LINE);
            mx += 4;
        }
        txt(menu, vw.k[i], mx, meta_y + 5, 1, COL_MUTED, lgfx::textdatum::top_left);
        mx += menu.textWidth(vw.k[i]) + 2;
        txt(menu, vw.v[i], mx, meta_y + 4, 1, COL_INK, lgfx::textdatum::top_left);
        mx += menu.textWidth(vw.v[i]) + 6;
        if (mx > width - 50) break;
    }
    if (vw.chip) {
        menu.setFont(&fonts::Font0);
        menu.setTextSize(1, 1);
        const int cw = menu.textWidth(vw.chip) + 8;
        const int cx = width - 4 - cw;
        plate(menu, cx, meta_y + 3, cw, 12, COL_DEEP, vw.chip_c);
        txt(menu, vw.chip, cx + cw / 2, meta_y + 5, 1, vw.chip_c, lgfx::textdatum::top_center);
    }

    const int body_top = meta_y + 18;
    const int body_h = height - HINT_H - body_top;
    plate(menu, 4, body_top + 2, width - 8, body_h - 4, COL_BG, COL_LINE);

    int y = body_top + 6;
    if (vw.focus) {
        txt(menu, vw.focus, 12, y, 1, COL_INK, lgfx::textdatum::top_left);
        y += 12;
        menu.drawFastHLine(12, y, width - 24, COL_LINE);
        y += 4;
    }
    for (int i = 0; i < vw.nitem; ++i) {
        const bool sel = (vw.cursor == i);
        const bool off = (vw.dis & (1u << i)) != 0;
        const int ry = y + i * 14;
        if (sel) {
            menu.fillRect(8, ry - 1, width - 16, 13, COL_SEL);
            menu.fillRect(8, ry - 1, 3, 13, accent());
        }
        char line[28];
        snprintf(line, sizeof(line), "%s%s", sel ? "> " : "  ", vw.item[i]);
        txt(menu, line, 14, ry, 1, off ? COL_DIS : (sel ? COL_WHITE : COL_DIMTXT),
            lgfx::textdatum::top_left);
    }

    menu.fillRect(0, height - HINT_H, width, HINT_H, COL_BG);
    menu.drawFastHLine(0, height - HINT_H, width, COL_LINE);
    txt(menu, vw.hints ? vw.hints : "", width / 2, height - HINT_H + 3, 1,
        COL_HINTTX, lgfx::textdatum::top_center);
}

static void paint_ext_intro(lgfx::LovyanGFX &g)
{
    g.fillRect(0, 0, 320, 18, COL_BG);
    txt(g, "BOTcorps", 8, 4, 1, COL_INK, lgfx::textdatum::top_left);
    txt(g, "OBSERVE ONLY", 312, 5, 1, COL_MUTED, lgfx::textdatum::top_right);
    const float p = (sinf((float)millis() / 420.0f) + 1.0f) * 0.5f;
    const int halo = 47 + (int)(2.0f * p);
    g.fillCircle(160, 126, halo, scale565(accent(), 50, 256));
    g.fillCircle(160, 126, 40, 0x1926);
    g.drawCircle(160, 126, 40, accent());
    g.drawArc(160, 128, 16, 13, 40, 320, COL_POWER);
    g.drawFastVLine(160, 112, 14, COL_POWER);
}

static void paint_ext_gibson(lgfx::LovyanGFX &g)
{
    plate(g, 136, 6, 48, 16, COL_BG, COL_LINE);
    txt(g, "CORE", 160, 9, 1, accent(), lgfx::textdatum::top_center);
    const int bob = (int)(sinf((float)millis() / 700.0f) * 1.5f);
    draw_gibson(g, 160, 92 + bob, 58);
    plate(g, 70, 158, 180, 48, COL_BG, COL_LINE);
    txt(g, "gibsonbot", 160, 166, 2, COL_INK, lgfx::textdatum::top_center);
    txt(g, "IVORY  SUNGLASSES", 160, 188, 1, accent(), lgfx::textdatum::top_center);
}

static void paint_ext_role(lgfx::LovyanGFX &g)
{
    plate(g, 16, 12, 288, 216, COL_BG, COL_LINE);
    draw_gibson(g, 58, 58, 28);
    txt(g, "ROLE CARD", 96, 28, 1, accent(), lgfx::textdatum::top_left);
    txt(g, "gibsonbot", 96, 42, 2, COL_INK, lgfx::textdatum::top_left);
    txt(g, "core", 96, 64, 1, COL_MUTED, lgfx::textdatum::top_left);
    const char *lab[3] = { "ROLE", "LEVEL", "SCORE" };
    const char *val[3] = { "GPU", "01", "00" };
    for (int i = 0; i < 3; ++i) {
        const int x = 28 + i * 90;
        plate(g, x, 92, 82, 40, COL_DEEP, COL_LINE);
        txt(g, lab[i], x + 6, 98, 1, COL_MUTED, lgfx::textdatum::top_left);
        txt(g, val[i], x + 6, 112, 2, COL_INK, lgfx::textdatum::top_left);
    }
    const char *rk[3] = { "ABOUT", "LOOK", "COLOR" };
    const char *rv[3] = { "gibson processing unit", "ivory, sunglasses", "fixed" };
    for (int i = 0; i < 3; ++i) {
        const int y = 146 + i * 24;
        txt(g, rk[i], 28, y, 1, COL_MUTED, lgfx::textdatum::top_left);
        txt(g, rv[i], 90, y, 1, COL_INK, lgfx::textdatum::top_left);
        if (i < 2) g.drawFastHLine(28, y + 16, 260, COL_LINE);
    }
}

static void paint_ext_home(lgfx::LovyanGFX &g, int bot_index)
{
    const bot_t &b = BOTS[bot_index];
    const float ph = fmodf((float)millis() / 1800.0f, 1.0f);
    g.drawCircle(160, 78, 46 + (int)(ph * 14), accent());
    draw_blob(g, 160, 78, 36, b.c, b.c2);
    plate(g, 40, 128, 240, 96, COL_BG, COL_LINE);
    txt(g, b.id, 160, 136, 2, COL_INK, lgfx::textdatum::top_center);
    txt(g, "LV 01", 160, 158, 1, COL_MUTED, lgfx::textdatum::top_center);
    char chip[28];
    snprintf(chip, sizeof(chip), "WATCHING %s", b.target);
    plate(g, 70, 176, 180, 18, COL_DEEP, accent());
    txt(g, chip, 160, 180, 1, accent(), lgfx::textdatum::top_center);
    txt(g, "RX  OBSERVE ONLY", 160, 202, 1, COL_MUTED, lgfx::textdatum::top_center);
}

static void paint_ext_radar(lgfx::LovyanGFX &g)
{
    const int cx = 160;
    const int cy = 108;
    const int r = 88;
    g.fillCircle(cx, cy, r + 2, COL_BLACK);
    g.fillCircle(cx, cy, r, 0x1105);
    g.drawCircle(cx, cy, r, accent());
    g.drawCircle(cx, cy, r / 2, scale565(accent(), 80, 256));
    g.drawFastHLine(cx - r, cy, r * 2, scale565(accent(), 70, 256));
    g.drawFastVLine(cx, cy - r, r * 2, scale565(accent(), 70, 256));
    const float ang = fmodf((float)millis() / 1700.0f, 1.0f) * 6.283185f;
    for (int k = 0; k < 6; ++k) {
        const float a = ang - (float)k * 0.18f;
        const int x1 = cx + (int)(cosf(a) * (float)(r - 4));
        const int y1 = cy + (int)(sinf(a) * (float)(r - 4));
        g.drawLine(cx, cy, x1, y1, scale565(accent(), 180 - k * 24, 256));
    }
    plate(g, 70, 210, 180, 18, COL_BG, COL_LINE);
    txt(g, "RADIO OFF", 160, 214, 1, COL_MUTED, lgfx::textdatum::top_center);
    s_sweep = fmodf((float)millis() / 1700.0f, 1.0f) * 6.283185f;
}

static void paint_ext_list(lgfx::LovyanGFX &g, const near_t *rows, const char *band)
{
    plate(g, 8, 8, 304, 22, COL_BG, COL_LINE);
    txt(g, "NEARBY", 16, 14, 1, accent(), lgfx::textdatum::top_left);
    txt(g, "RADIO OFF", 304, 14, 1, COL_AMBER, lgfx::textdatum::top_right);
    (void)band;
    for (int i = 0; i < 3; ++i) {
        const int y = 40 + i * 62;
        const bool sel = (s_cursor == i);
        plate(g, 12, y, 296, 54, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt(g, rows[i].name, 24, y + 8, 2, COL_INK, lgfx::textdatum::top_left);
        txt(g, "sample  this sweep", 24, y + 32, 1, COL_MUTED, lgfx::textdatum::top_left);
        txt(g, rows[i].rssi, 292, y + 8, 1, COL_AMBER, lgfx::textdatum::top_right);
        for (int k = 0; k < 4; ++k) {
            const uint16_t col = (k < rows[i].bars) ? accent() : COL_LINE;
            const int bh = (k + 1) * 4;
            g.fillRect(250 + k * 8, y + 40 - bh, 5, bh, col);
        }
    }
}

static void paint_ext_terminal(lgfx::LovyanGFX &g)
{
    g.fillScreen(COL_BG);
    const int tw = 74;
    const int th = 70;
    const int gx = (320 - 4 * tw) / 5;
    const int gy = (240 - 3 * th) / 4;
    for (int i = 0; i < BOT_N; ++i) {
        const int c = i % 4;
        const int r = i / 4;
        const int x = gx + c * (tw + gx);
        const int y = gy + r * (th + gy);
        const bool sel = (i == s_term);
        g.setClipRect(x, y, tw, th);
        plate(g, x, y, tw, th, sel ? COL_PANEL2 : COL_BAR, sel ? accent() : COL_LINE);
        txt(g, BOTS[i].id, x + tw / 2, y + 4, 1, sel ? COL_INK : COL_DIMTXT,
            lgfx::textdatum::top_center);
        draw_blob(g, x + tw / 2, y + 36, 16, BOTS[i].c, BOTS[i].c2);
        g.fillCircle(x + 8, y + th - 10, 2, st_col(BOTS[i].st));
        txt(g, st_text(BOTS[i].st), x + 14, y + th - 14, 1, COL_MUTED,
            lgfx::textdatum::top_left);
    }
    g.clearClipRect();
}

static void paint_rain_swatch(lgfx::LovyanGFX &g, int x, int y)
{
    const int mode = botcorps_rain_mode();
    if (mode <= 0) {
        for (int k = 0; k < 4; ++k) {
            g.fillRect(x + k * 3, y, 3, 10, botcorps_rain_palette(k));
        }
        return;
    }
    g.fillRect(x, y, 12, 10, botcorps_rain_solid());
}

static void paint_cp_row(lgfx::LovyanGFX &g, int index, int y)
{
    const bool on = (s_cursor == index);
    plate(g, 8, y, 304, 36, on ? COL_PANEL2 : COL_BAR, on ? accent() : COL_LINE);
    const char *label = index == 0 ? "UI COLOR"
        : index == 1 ? "BACKGROUND"
        : index == 2 ? "SCREEN SAVER"
        : index == 3 ? "SAVER"
        : index == 4 ? "IDLE TIME" : "RAIN COLOUR";
    txt(g, label, 16, y + 3, 1, on ? accent() : COL_MUTED, lgfx::textdatum::top_left);
    if (index == 5) {
        paint_rain_swatch(g, 16, y + 18);
        txt(g, cp_value(), 34, y + 18, 1, COL_INK, lgfx::textdatum::top_left);
        return;
    }

    const char *opts[4];
    int n = 0;
    int sel = 0;
    uint16_t dots[3] = { 0, 0, 0 };
    if (index == 0) {
        opts[0] = "CYAN"; opts[1] = "AMBER"; opts[2] = "MINT";
        n = 3; sel = s_color;
        dots[0] = ACCENT_C[0]; dots[1] = ACCENT_C[1]; dots[2] = ACCENT_C[2];
    } else if (index == 1) {
        opts[0] = "BOT+DEVICE"; opts[1] = "DEVICES"; opts[2] = "BOTS";
        n = 3; sel = s_bg;
    } else if (index == 2) {
        opts[0] = "BOTS"; opts[1] = "GIBSON"; opts[2] = "FLYBY"; opts[3] = "BOUNCE";
        n = 4; sel = s_saver;
    } else if (index == 3) {
        opts[0] = "ON"; opts[1] = "OFF";
        n = 2; sel = s_saver_on ? 0 : 1;
    } else {
        opts[0] = "15 SEC"; opts[1] = "25 SEC"; opts[2] = "45 SEC";
        n = 3; sel = s_idle;
    }
    g.setFont(&fonts::Font0);
    g.setTextSize(1, 1);
    int x = 16;
    for (int i = 0; i < n; ++i) {
        const int tw = g.textWidth(opts[i]) + (index == 0 ? 18 : 10);
        const bool picked = (i == sel);
        plate(g, x, y + 18, tw, 16, picked ? COL_DEEP : COL_BLACK, picked ? accent() : COL_LINE);
        int tx = x + 4;
        if (index == 0) {
            g.fillCircle(x + 8, y + 26, 3, dots[i]);
            tx = x + 14;
        }
        txt(g, opts[i], tx, y + 22, 1, picked ? COL_INK : COL_MUTED, lgfx::textdatum::top_left);
        x += tw + 4;
    }
}

static void paint_ext_control(lgfx::LovyanGFX &g)
{
    g.fillScreen(COL_BG);
    for (int i = 0; i < 6; ++i) paint_cp_row(g, i, 3 + i * 39);
}

static void paint_ext(lgfx::LGFX_Device &ext)
{
    ext.clearClipRect();
    ext.setTextWrap(false, false);
    ext.setFont(&fonts::Font0);
    switch (s_screen) {
    case SCR_RAIN:
        break;
    case SCR_INTRO:
        paint_ext_intro(ext);
        break;
    case SCR_GIBSON:
        paint_ext_gibson(ext);
        break;
    case SCR_ROLE:
        paint_ext_role(ext);
        break;
    case SCR_TERMINAL:
        paint_ext_terminal(ext);
        break;
    case SCR_WIFI:
        paint_ext_home(ext, 0);
        break;
    case SCR_BT:
        paint_ext_home(ext, 1);
        break;
    case SCR_BOT:
        paint_ext_home(ext, s_bot);
        break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_BOT_RADAR:
        paint_ext_radar(ext);
        break;
    case SCR_WIFI_LIST:
        paint_ext_list(ext, NEAR_WIFI, "wi-fi");
        break;
    case SCR_BT_LIST:
        paint_ext_list(ext, NEAR_BLE, "ble");
        break;
    case SCR_CONTROL:
        paint_ext_control(ext);
        break;
    default:
        break;
    }
}

static void end_panel(lgfx::LGFX_Device &d)
{
    d.endWrite();
    d.clearClipRect();
}

static bool anim_screen(void)
{
    return s_screen == SCR_INTRO || s_screen == SCR_GIBSON
        || s_screen == SCR_WIFI || s_screen == SCR_BT || s_screen == SCR_BOT;
}

static bool radar_screen(void)
{
    return s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR
        || s_screen == SCR_BOT_RADAR;
}

static lgfx::LGFX_Sprite *anim_sprite(lgfx::LGFX_Device &panel, int w, int h)
{
    if (s_anim && s_anim_w == w && s_anim_h == h) return s_anim;
    anim_release();
    if (ESP.getMaxAllocHeap() < (size_t)w * (size_t)h * 2u + 8192u) return nullptr;
    s_anim = new lgfx::LGFX_Sprite(&panel);
    if (!s_anim->createSprite(w, h)) {
        anim_release();
        return nullptr;
    }
    s_anim_w = w;
    s_anim_h = h;
    return s_anim;
}

static void draw_intro_btn(lgfx::LovyanGFX &g, int ox, int oy)
{
    const float p = (sinf((float)millis() / 420.0f) + 1.0f) * 0.5f;
    const int halo = 47 + (int)(2.0f * p);
    const int cx = 160 - ox;
    const int cy = 126 - oy;
    g.fillCircle(cx, cy, halo, scale565(accent(), 50, 256));
    g.fillCircle(cx, cy, 40, 0x1926);
    g.drawCircle(cx, cy, 40, accent());
    g.drawArc(cx, cy + 2, 16, 13, 40, 320, COL_POWER);
    g.drawFastVLine(cx, 112 - oy, 14, COL_POWER);
}

static void draw_gib_bot(lgfx::LovyanGFX &g, int ox, int oy)
{
    const int bob = (int)(sinf((float)millis() / 700.0f) * 1.5f);
    draw_gibson(g, 160 - ox, 92 + bob - oy, 58);
}

static int home_bot_index(void)
{
    if (s_screen == SCR_BT) return 1;
    if (s_screen == SCR_BOT) return s_bot;
    return 0;
}

static void draw_home_live(lgfx::LovyanGFX &g, int ox, int oy)
{
    const bot_t &b = BOTS[home_bot_index()];
    const float ph = fmodf((float)millis() / 1800.0f, 1.0f);
    const int cx = 160 - ox;
    const int cy = 78 - oy;
    g.drawCircle(cx, cy, 46 + (int)(ph * 14), accent());
    draw_blob(g, cx, cy, 36, b.c, b.c2);
}

static void push_anim(lgfx::LGFX_Device &panel, int x, int y, int w, int h,
                      void (*draw)(lgfx::LovyanGFX &, int, int))
{
    lgfx::LGFX_Sprite *spr = anim_sprite(panel, w, h);
    if (!spr) return;
    if (!botcorps_rain_copy_rect(*spr, x, y)) return;
    draw(*spr, x, y);
    panel.setClipRect(x, y, w, h);
    spr->pushSprite(x, y);
    panel.clearClipRect();
}

static void radar_rays(lgfx::LovyanGFX &g, float ang, uint16_t col)
{
    const int cx = 160;
    const int cy = 108;
    const int r = 84;
    for (int k = 0; k < 6; ++k) {
        const float a = ang - (float)k * 0.18f;
        const int x1 = cx + (int)(cosf(a) * (float)(r - 4));
        const int y1 = cy + (int)(sinf(a) * (float)(r - 4));
        g.drawLine(cx, cy, x1, y1, col);
    }
}

static void radar_guides(lgfx::LovyanGFX &g)
{
    g.drawCircle(160, 108, 88, accent());
    g.drawCircle(160, 108, 44, scale565(accent(), 80, 256));
    g.drawFastHLine(72, 108, 176, scale565(accent(), 70, 256));
    g.drawFastVLine(160, 20, 176, scale565(accent(), 70, 256));
}

/* Sweep lines only. No fillScreen and no fillRect. */
static void tick_ext_radar(lgfx::LGFX_Device &g)
{
    g.setClipRect(68, 16, 184, 184);
    if (s_sweep >= 0.0f) radar_rays(g, s_sweep, 0x1105);
    radar_guides(g);
    s_sweep = fmodf((float)millis() / 1700.0f, 1.0f) * 6.283185f;
    radar_rays(g, s_sweep, accent());
    g.clearClipRect();
}

static void tick_ext_anim(lgfx::LGFX_Device &panel)
{
    if (s_screen == SCR_INTRO) {
        push_anim(panel, 104, 70, 112, 112, draw_intro_btn);
    } else if (s_screen == SCR_GIBSON) {
        push_anim(panel, 108, 36, 104, 120, draw_gib_bot);
    } else if (s_screen == SCR_WIFI || s_screen == SCR_BT || s_screen == SCR_BOT) {
        push_anim(panel, 96, 14, 128, 114, draw_home_live);
    }
}

static void int_mark(lgfx::LovyanGFX &g, int index, int y, bool erase)
{
    int_col_t &c = s_int[index];
    const int x = index * INT_GAP;
    if (erase) {
        g.fillRect(x - 1, y - 28, 14, 40, COL_RAIN);
        return;
    }
    const uint16_t body = (c.kind == 1) ? s_dim_bot[c.color % 14] : s_dim_code;
    for (int k = 3; k >= 0; --k) {
        const int ty = y - k * INT_GAP;
        const uint16_t tone = mix565(body, COL_RAIN, TRAIL_NUM[k]);
        if (c.kind == 1) {
            draw_tiny_bot(g, x, ty, tone, c.hat != 0);
        } else if (k == 0) {
            const bool head = ((index + (ty >= 0 ? ty / INT_GAP : 0)) % 7) == 0;
            const char ch = CODE_GLYPHS[rnd_n(18)];
            char buf[2] = { ch, 0 };
            g.setFont(&fonts::Font0);
            g.setTextSize(1, 1);
            g.setTextColor(head ? s_dim_head : tone);
            g.setCursor(x, ty);
            g.print(buf);
        }
    }
}

/* Column gutters only, plus the clock rect when the second changes.
 * The size-2 name drawn on the dirty frame is left in place. */
static void tick_int_ambience(lgfx::LGFX_Device &menu)
{
    const int width = menu.width();
    const int height = menu.height();
    const int y0 = BAR_H + 19;
    const int gh = (height - HINT_H) - y0;
    if (gh > 0 && s_int_n > 0) {
        int16_t prev_y[INT_COLS];
        uint8_t moved[INT_COLS];
        for (int i = 0; i < s_int_n; ++i) {
            int_col_t &c = s_int[i];
            prev_y[i] = c.y;
            moved[i] = 0;
            if (c.kind == 0) continue;
            if (rnd_n(100) < c.jitter) continue;
            moved[i] = 1;
            c.y = (int16_t)(c.y + INT_GAP);
            if (c.y > height + INT_GAP) {
                c.y = (int16_t)(-INT_GAP - rnd_n(3) * INT_GAP);
                if (c.kind == 1) c.color = (uint8_t)rnd_n(14);
            }
        }
        for (int pass = 0; pass < 2; ++pass) {
            if (pass == 0) menu.setClipRect(0, y0, 4, gh);
            else menu.setClipRect(width - 4, y0, 4, gh);
            for (int i = 0; i < s_int_n; ++i) {
                if (!moved[i]) continue;
                int_mark(menu, i, prev_y[i], true);
                int_mark(menu, i, s_int[i].y, false);
            }
        }
        menu.clearClipRect();
    }
    char next[8];
    const uint32_t sec = (millis() - s_boot_ms) / 1000u;
    snprintf(next, sizeof(next), "%02u:%02u",
             (unsigned)((sec / 3600u) % 24u),
             (unsigned)((sec / 60u) % 60u));
    if (strcmp(next, s_clk_drawn) != 0) {
        memcpy(s_clk_drawn, next, sizeof(s_clk_drawn));
        memcpy(s_clk, next, sizeof(s_clk));
        const int cw = 52;
        menu.setClipRect(width - cw, 0, cw, BAR_H + 1);
        menu.fillRect(width - cw, 0, cw, BAR_H, COL_BAR);
        txt(menu, s_clk, width - 6, 7, 1, accent(), lgfx::textdatum::top_right);
        menu.drawFastHLine(width - cw, BAR_H, cw, COL_LINE);
        menu.clearClipRect();
    }
}

static void paint_full(void)
{
    if (botcorps_dual_ok()) {
        botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
        auto &ext = botcorps_content();
        ext.clearClipRect();
        if (s_screen == SCR_TERMINAL || s_screen == SCR_CONTROL) {
            /* fillScreen lives in those painters, dirty path only. */
        } else if (!botcorps_rain_present(ext) && !s_backdrop_ready) {
            ext.fillRect(0, 0, ext.width(), ext.height(), COL_RAIN);
        }
        paint_ext(ext);
        s_backdrop_ready = true;
        end_panel(ext);
    }
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &menu = botcorps_menu();
    paint_int(menu);
    memcpy(s_clk_drawn, s_clk, sizeof(s_clk_drawn));
    end_panel(menu);
    if (s_heap_for != s_screen) {
        s_heap_for = s_screen;
        note_heap_for(s_screen);
    }
    s_full_n++;
    Serial.printf("[botcorps] repaint full screen=%d\n", s_screen);
}

static void note_stats(uint32_t now)
{
    if ((uint32_t)(now - s_stat_ms) < 2000u) return;
    if (s_ext_ticks == 0 && s_int_ticks == 0) {
        s_stat_ms = now;
        return;
    }
    Serial.printf("[botcorps] repaint ext_tick=%u int_tick=%u full=%u\n",
                  (unsigned)s_ext_ticks, (unsigned)s_int_ticks, (unsigned)s_full_n);
    s_ext_ticks = 0;
    s_int_ticks = 0;
    s_stat_ms = now;
}

void botcorps_ui_mark_dirty(void)
{
    s_dirty = true;
}

void botcorps_ui_begin(void)
{
    s_boot_ms = millis();
    s_screen = SCR_RAIN;
    s_cursor = 0;
    s_dirty = true;
    s_backdrop_ready = false;
    load_rain_color();
    build_dim();
    build_int_rain();
    botcorps_rain_log_heap("boot");
}

void botcorps_ui_draw(bool ext_presented)
{
    (void)ext_presented;
    const uint32_t now = millis();
    const bool tick = (uint32_t)(now - s_ui_ms) >= 90u;
    if (s_dirty) {
        if (tick) s_ui_ms = now;
        s_dirty = false;
        paint_full();
        return;
    }
    /* Rain boot: EXT is the sprite path. INT ambience is per column. */
    if (s_screen == SCR_RAIN) {
        if (!tick) return;
        s_ui_ms = now;
        botcorps_set_surface(BOTCORPS_SURFACE_MENU);
        tick_int_ambience(botcorps_menu());
        end_panel(botcorps_menu());
        s_int_ticks++;
        note_stats(now);
        return;
    }
    const bool amb = int_rain_on();
    const bool ext_tick = anim_screen() || radar_screen();
    if (!tick || (!amb && !ext_tick)) return;
    s_ui_ms = now;
    if (botcorps_dual_ok() && ext_tick) {
        botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
        auto &ext = botcorps_content();
        if (radar_screen()) tick_ext_radar(ext);
        else tick_ext_anim(ext);
        end_panel(ext);
        s_ext_ticks++;
    }
    if (amb) {
        botcorps_set_surface(BOTCORPS_SURFACE_MENU);
        tick_int_ambience(botcorps_menu());
        end_panel(botcorps_menu());
        s_int_ticks++;
    }
    note_stats(now);
}

enum { K_NONE = 0, K_UP, K_DN, K_LT, K_RT, K_OK, K_ESC };

static int read_key(void)
{
    if (!M5Cardputer.Keyboard.isChange()) return K_NONE;
    if (!M5Cardputer.Keyboard.isPressed()) return K_NONE;
    const auto &st = M5Cardputer.Keyboard.keysState();
    if (st.enter) return K_OK;
    for (char c : st.word) {
        if (c == ';' || c == ':') return K_UP;
        if (c == '.' || c == '>') return K_DN;
        if (c == ',' || c == '<') return K_LT;
        if (c == '/' || c == '?') return K_RT;
        if (c == '`' || c == '~' || c == 0x1B) return K_ESC;
    }
    for (uint8_t h : st.hid_keys) {
        if (h == 0x52) return K_UP;
        if (h == 0x51) return K_DN;
        if (h == 0x50) return K_LT;
        if (h == 0x4F) return K_RT;
        if (h == 0x29) return K_ESC;
    }
    return K_NONE;
}

static int item_count(void)
{
    switch (s_screen) {
    case SCR_RAIN:
    case SCR_INTRO:
    case SCR_ROLE:
        return 1;
    case SCR_GIBSON:
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT:
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
        return 3;
    case SCR_BOT_RADAR:
        return 2;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        return 3;
    case SCR_CONTROL:
        return 6;
    default:
        return 0;
    }
}

static void run_cursor(void)
{
    const int i = s_cursor;
    switch (s_screen) {
    case SCR_RAIN: set_screen(SCR_INTRO); break;
    case SCR_INTRO: set_screen(SCR_GIBSON); break;
    case SCR_GIBSON:
        if (i == 0) set_screen(SCR_TERMINAL);
        else if (i == 1) set_screen(SCR_CONTROL);
        else set_screen(SCR_ROLE);
        break;
    case SCR_ROLE: set_screen(SCR_GIBSON); break;
    case SCR_WIFI:
        if (i == 0) set_screen(SCR_WIFI_RADAR);
        else if (i == 1) set_screen(SCR_TERMINAL);
        else set_screen(SCR_CONTROL);
        break;
    case SCR_BT:
        if (i == 0) set_screen(SCR_BT_RADAR);
        else if (i == 1) set_screen(SCR_TERMINAL);
        else set_screen(SCR_CONTROL);
        break;
    case SCR_BOT:
        if (i == 0) set_screen(SCR_BOT_RADAR);
        else if (i == 1) set_screen(SCR_TERMINAL);
        else set_screen(SCR_CONTROL);
        break;
    case SCR_WIFI_RADAR:
        if (i == 1) set_screen(SCR_WIFI_LIST);
        else if (i == 2) set_screen(SCR_WIFI);
        break;
    case SCR_BT_RADAR:
        if (i == 1) set_screen(SCR_BT_LIST);
        else if (i == 2) set_screen(SCR_BT);
        break;
    case SCR_BOT_RADAR:
        if (i == 1) set_screen(SCR_BOT);
        break;
    default:
        break;
    }
}

static void go_back(void)
{
    switch (s_screen) {
    case SCR_INTRO: set_screen(SCR_RAIN); break;
    case SCR_ROLE:
    case SCR_TERMINAL:
    case SCR_CONTROL: set_screen(SCR_GIBSON); break;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT: set_screen(SCR_TERMINAL); break;
    case SCR_WIFI_RADAR: set_screen(SCR_WIFI); break;
    case SCR_WIFI_LIST: set_screen(SCR_WIFI_RADAR); break;
    case SCR_BT_RADAR: set_screen(SCR_BT); break;
    case SCR_BT_LIST: set_screen(SCR_BT_RADAR); break;
    case SCR_BOT_RADAR: set_screen(SCR_BOT); break;
    default: break;
    }
}

static void nudge_bot(int d)
{
    int idx = 0;
    for (int i = 0; i < PLAIN_N; ++i) {
        if (PLAIN[i] == s_bot) idx = i;
    }
    idx = (idx + d + PLAIN_N) % PLAIN_N;
    s_bot = PLAIN[idx];
}

static void nudge_cp(int d)
{
    if (s_cursor == 0) s_color = (s_color + d + 3) % 3;
    else if (s_cursor == 1) s_bg = (s_bg + d + 3) % 3;
    else if (s_cursor == 2) s_saver = (s_saver + d + 4) % 4;
    else if (s_cursor == 3) s_saver_on = (s_saver_on + d + 2) % 2;
    else if (s_cursor == 4) s_idle = (s_idle + d + 3) % 3;
    else {
        const int n = BOTCORPS_RAIN_COLORS;
        const int m = (botcorps_rain_mode() + d + n) % n;
        botcorps_rain_set_mode(m);
        save_rain_color();
        build_dim();
    }
}

static void on_key(int k)
{
    if (k == K_NONE) return;
    s_dirty = true;
    if (s_screen == SCR_TERMINAL) {
        const int c = s_term % 4;
        const int r = s_term / 4;
        if (k == K_LT) s_term = r * 4 + (c + 3) % 4;
        if (k == K_RT) s_term = r * 4 + (c + 1) % 4;
        if (k == K_UP) s_term = ((r + 2) % 3) * 4 + c;
        if (k == K_DN) s_term = ((r + 1) % 3) * 4 + c;
        if (k == K_OK) {
            const bot_t &b = BOTS[s_term];
            if (b.radio == RADIO_WIFI) set_screen(SCR_WIFI);
            else if (b.radio == RADIO_BLE) set_screen(SCR_BT);
            else {
                s_bot = s_term;
                set_screen(SCR_BOT);
            }
        }
        if (k == K_ESC) go_back();
        return;
    }
    if (s_screen == SCR_CONTROL) {
        if (k == K_UP) s_cursor = (s_cursor + 5) % 6;
        if (k == K_DN) s_cursor = (s_cursor + 1) % 6;
        if (k == K_LT) nudge_cp(-1);
        if (k == K_RT) nudge_cp(1);
        if (k == K_ESC) go_back();
        return;
    }
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) {
        if (k == K_UP) s_cursor = (s_cursor + 2) % 3;
        if (k == K_DN) s_cursor = (s_cursor + 1) % 3;
        if (k == K_OK || k == K_ESC) go_back();
        return;
    }
    const int n = item_count();
    if (k == K_UP && n) s_cursor = (s_cursor + n - 1) % n;
    if (k == K_DN && n) s_cursor = (s_cursor + 1) % n;
    if ((k == K_LT || k == K_RT) && s_screen == SCR_BOT) nudge_bot(k == K_LT ? -1 : 1);
    if (k == K_OK) run_cursor();
    if (k == K_ESC) go_back();
}

void botcorps_ui_input(void)
{
    on_key(read_key());
}
