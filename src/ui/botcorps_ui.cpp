/*
 * EXT is the mock's screen: icon buttons, terminal, rain behind chrome.
 * INT is the selected bot's ID card. No text menu and no hint line.
 * A screen change, and an overlay opening or closing, fills the panel
 * before the new chrome is drawn.
 */
#ifdef BOTCORPS_HOST_RENDER
#include "host_shim.h"
#else
#include "ui/botcorps_ui.h"
#include "ui/botcorps_band.h"
#include "ui/botcorps_sprite.h"
#include "ui/botcorps_glyph.h"
#include "ui/botcorps_id.h"

#include "display/botcorps_display.h"
#include "probe/botcorps_icons.h"
#include "probe/botcorps_probe.h"
#include "rain/botcorps_rain.h"

#include <Preferences.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#endif

extern char botcorps_t0_term_prompt[80];
extern char botcorps_t0_term_hist[20][26];
extern char botcorps_t0_term_log[2048];
extern char botcorps_t0_term_found[12][28];

static const uint8_t COL_BG    = PAL_BG;
static const uint8_t COL_FIELD = PAL_FIELD;
static const uint8_t COL_BAR   = PAL_BAR;
static const uint8_t COL_LINE  = PAL_LINE;
static const uint8_t COL_INK   = PAL_INK;
static const uint8_t COL_MUTED = PAL_MUTED;
static const uint8_t COL_AMBER = PAL_AMBER;
static const uint8_t COL_SEL   = PAL_SEL;
static const uint8_t COL_WHITE = PAL_WHITE;
static const uint8_t COL_DEEP  = PAL_DEEP;
static const uint8_t COL_IVORY = PAL_IVORY;
static const uint8_t COL_OK    = PAL_OK;
static const uint8_t COL_HOLD  = PAL_HOLD;

static const uint8_t ACCENT_C[3] = { PAL_CYAN, PAL_AMBER, PAL_MINT };
static const char *const ACCENT_N[3] = { "CYAN", "AMBER", "MINT" };
static const char *const MIX_N[3] = { "BOT + DEVICE", "BOTS", "DEVICES" };
static const char *const SAVER_N[4] = { "BOTS", "GIBSON", "FLYBY", "BOUNCE" };
static const int IDLE_V[3] = { 15, 25, 45 };

enum {
    ICO_PLAY = 0, ICO_TERM, ICO_CTRL, ICO_RADAR, ICO_BACK, ICO_TRACK, ICO_LIST, ICO_DIAG
};
static const int BS = 28;

enum {
    SCR_RAIN = 0,
    SCR_GIBSON,
    SCR_TERMINAL,
    SCR_WIFI,
    SCR_WIFI_RADAR,
    SCR_WIFI_LIST,
    SCR_BT,
    SCR_BT_RADAR,
    SCR_BT_LIST,
    SCR_BOT,
    SCR_BOT_RADAR,
    SCR_SCAN,
    SCR_CONTROL,
    SCR_DIAG
};

struct bot_t {
    const char *id;
    const char *short_name;
    const char *target;
    const char *ins;
    uint8_t radio; /* 0 none, 1 wifi, 2 ble */
    uint8_t st; /* 0 ready, 1 limited, 2 hold */
};

static const bot_t BOTS[12] = {
    { "wifibot", "wifi", "WI-FI", "~", 1, 1 },
    { "bluetoothbot", "bluetooth", "BLE", "*", 2, 1 },
    { "deauthbot", "deauth", "DEAUTH", "/", 0, 0 },
    { "hackbot", "hack", "HACK", "#", 0, 0 },
    { "flockbot", "flock", "FLOCK", "#", 0, 0 },
    { "dronebot", "drone", "DRONE", "*", 0, 0 },
    { "airtagbot", "airtag", "AIR TAG", "+", 0, 0 },
    { "keyfobbot", "keyfob", "KEYFOB", "[", 0, 0 },
    { "metabot", "meta", "META", "+", 0, 0 },
    { "skimmerbot", "skimmer", "SKIMMER", "~", 0, 2 },
    { "flipperbot", "flipper", "FLIPPER", "^", 0, 1 },
    { "samsungtagbot", "samsung", "SAMSUNG", "o", 0, 1 },
};

static const uint8_t BOT_PAL[12] = {
    PAL_B5D3F, PAL_B4E7F, PAL_B5D3F, PAL_B7711, PAL_B9F0B, PAL_BFB72,
    PAL_BAC7F, PAL_B36B8, PAL_B9F0B, PAL_B36B8, PAL_BFC6B, PAL_BBD3F,
};

static uint8_t bot_color(int i)
{
    if (i < 0 || i > 11) return PAL_IVORY;
    return BOT_PAL[i];
}

/* INT is 16-bit. The value stays uint16_t so M5GFX reads RGB565, not RGB888. */
static uint16_t panel565(uint8_t idx)
{
    return botcorps_rgb565(idx);
}

static bool sprite_blink(int i)
{
    return ((millis() + (uint32_t)i * 740u) % 3300u) < 140u;
}

static const char *const NEAR_WIFI[3] = { "cafe-guest", "printer-2f", "lobby-ap" };
static const char *const NEAR_WIFI_R[3] = { "-42", "-67", "-71" };
static const uint8_t NEAR_WIFI_B[3] = { 4, 2, 1 };
static const char *const NEAR_BLE[3] = { "earbuds-58", "watch-6", "speaker-7" };
static const char *const NEAR_BLE_R[3] = { "-58", "-63", "-74" };
static const uint8_t NEAR_BLE_B[3] = { 3, 2, 1 };

static const char *const CMDS[12] = {
    "scan", "hunt", "help", "bots", "rain", "filter",
    "clear", "ping", "info", "track", "sniff", "cmds"
};
/* v3.2 command menu. 40 names + purposes, const in flash. defend stays in CORE. */
static const int CMD_N = 40;
static const int G_AT[5] = { 0, 17, 25, 33, 40 };
static const char *const G_NAME[4] = { "CORE", "COMMS", "GAME", "SYS" };
static const char *const CMDN[CMD_N] = {
    "scan", "hunt", "help", "bots", "rain", "filter", "ping", "info", "track", "sniff",
    "go", "load", "defend", "sweep", "roster", "pulse", "dig",
    "hail", "say", "ask", "rollcall", "whisper", "broadcast", "link", "mute",
    "train", "hunt-run", "score", "rank", "loadout", "lobby", "duel", "stash",
    "clear", "history", "theme", "about", "cmds", "version", "uptime"
};
static const char *const CMDP[CMD_N] = {
    "sweep \x7f wifi|ble|all", "drone|flock|skimmer|deauth", "list the commands", "board status",
    "rain \x7f <colour>|mixed", "wifi|ble|threats|strong|off", "ping a found device", "device details",
    "pin a device \x7f off", "list heard (filtered)", "jump to a screen", "load a screen",
    "typing game \x7f beginner|veteran", "alias \x7f scan", "alias \x7f bots", "alias \x7f ping", "alias \x7f info",
    "greet a bot", "send a line to a bot", "query bot status", "crew check-in",
    "quiet line to a bot", "speak to the crew", "open a bot channel", "hush a bot",
    "sealed", "sealed", "sealed", "sealed", "sealed", "sealed", "sealed", "sealed",
    "wipe the log", "recent lines", "neon look", "build blurb", "this menu", "term build", "session clock"
};
static const uint8_t CMD_ARG[CMD_N] = {
    1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
};
static const int PCMD_N = CMD_N;
static const char *const *const PCMD = CMDN;
static const char *const HAIL[13] = {
    "reading you", "channel clear", "link warm", "quiet", "keys ready", "eyes open",
    "airborne", "ping soft", "click armed", "mesh synced", "on hold", "wave ready", "nearby"
};
static const char *const CHAT3[3] = { "copy", "on it", "noted" };
/* Hunt dropdown on the Gibson scan. Order is the hunt-actions menu. */
static const int ACT_N = 6;
static const char *const ACT_NAME[ACT_N] = { "hunt", "follow", "tag", "info", "ignore", "back" };
static const uint8_t ACT_ICO[ACT_N][8] = {
    { 0x3c, 0x42, 0x99, 0xbd, 0xbd, 0x99, 0x42, 0x3c },
    { 0x40, 0x7c, 0x7e, 0x7c, 0x40, 0x40, 0x40, 0x40 },
    { 0x00, 0xfe, 0x83, 0xb1, 0xb3, 0xfe, 0x00, 0x00 },
    { 0x3c, 0x5a, 0x81, 0x99, 0x99, 0x99, 0x42, 0x3c },
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 },
    { 0x00, 0x10, 0x30, 0x7f, 0x30, 0x10, 0x00, 0x00 },
};
static const uint8_t ARROW_UP[8] = { 0x18, 0x3c, 0x7e, 0xff, 0x3c, 0x3c, 0x3c, 0x00 };
static const uint8_t ARROW_DN[8] = { 0x00, 0x3c, 0x3c, 0x3c, 0xff, 0x7e, 0x3c, 0x18 };
/* Existing slots only. One code per glow step, per UI accent (cyan, amber, mint). */
enum {
    STEP_SHADOW = 0, /* 45% black */
    STEP_HAIR,       /* ac at 0.28 */
    STEP_RULE,       /* ac at 0.22 */
    STEP_SEL,        /* ac at 0.18 */
    STEP_HUNT,       /* hunt row ac at 0.22 */
    STEP_GLOW,       /* neon offset, dimmed accent */
    STEP_AC,         /* solid accent stroke and text */
    STEP_N
};
/* TERM_PAIR. UI cyan and mint share magenta. UI amber uses #00f0ff. Explicit slots only. */
static const uint8_t TERM_MAG[6] = {
    PAL_TERM_M, PAL_TERM_M28, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M35, PAL_TERM_M10
};
static const uint8_t TERM_CYN[6] = {
    PAL_TERM_C, PAL_TERM_C28, PAL_TERM_C22, PAL_TERM_C18, PAL_TERM_C35, PAL_TERM_C10
};
static const uint8_t CMD_ICO[CMD_N][8] = {
    { 0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 0x04, 0x02 },
    { 0x18, 0x18, 0x24, 0xe7, 0xe7, 0x24, 0x18, 0x18 },
    { 0x3c, 0x42, 0x02, 0x0c, 0x10, 0x10, 0x00, 0x10 },
    { 0x7e, 0x81, 0xa5, 0x81, 0xbd, 0x7e, 0x24, 0x66 },
    { 0x48, 0x49, 0x11, 0x94, 0x94, 0x12, 0x42, 0x40 },
    { 0xff, 0x7e, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x00 },
    { 0x08, 0x04, 0x22, 0x72, 0x22, 0x04, 0x08, 0x00 },
    { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00 },
    { 0x3c, 0x42, 0x99, 0xbd, 0xbd, 0x99, 0x42, 0x3c },
    { 0x00, 0x42, 0x5a, 0xdb, 0xdb, 0x5a, 0x42, 0x00 },
    { 0x00, 0x08, 0x04, 0xfe, 0x04, 0x08, 0x00, 0x00 },
    { 0x00, 0x08, 0x04, 0xfe, 0x04, 0x08, 0x00, 0x00 },
    { 0x7e, 0xff, 0xff, 0x7e, 0x7e, 0x3c, 0x18, 0x00 },
    { 0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 0x04, 0x02 },
    { 0x7e, 0x81, 0xa5, 0x81, 0xbd, 0x7e, 0x24, 0x66 },
    { 0x08, 0x04, 0x22, 0x72, 0x22, 0x04, 0x08, 0x00 },
    { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00 },
    { 0x18, 0x3c, 0x5a, 0x99, 0x18, 0x18, 0x18, 0x00 },
    { 0x00, 0x7e, 0x81, 0x99, 0x81, 0x7e, 0x02, 0x01 },
    { 0x3c, 0x42, 0x02, 0x0c, 0x10, 0x00, 0x10, 0x00 },
    { 0x55, 0x00, 0xaa, 0x00, 0x55, 0x00, 0xaa, 0x00 },
    { 0x00, 0x3c, 0x42, 0x98, 0x80, 0x42, 0x3c, 0x00 },
    { 0x10, 0x38, 0x54, 0x92, 0x10, 0x10, 0x10, 0x00 },
    { 0x00, 0x66, 0x99, 0x81, 0x42, 0x3c, 0x00, 0x00 },
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 },
    { 0x00, 0x7e, 0x81, 0xbd, 0xa5, 0xbd, 0x81, 0x7e },
    { 0x18, 0x18, 0x24, 0xe7, 0xe7, 0x24, 0x18, 0x18 },
    { 0x3c, 0x42, 0xa5, 0x81, 0x99, 0x42, 0x3c, 0x00 },
    { 0x18, 0x3c, 0x7e, 0xff, 0x18, 0x18, 0x18, 0x00 },
    { 0xff, 0x81, 0xbd, 0xa5, 0xbd, 0x81, 0xff, 0x00 },
    { 0x3c, 0x42, 0x81, 0x99, 0x81, 0x42, 0x3c, 0x00 },
    { 0x81, 0xc3, 0x66, 0x3c, 0x3c, 0x66, 0xc3, 0x81 },
    { 0x3c, 0x42, 0x81, 0xbd, 0xbd, 0x81, 0x42, 0x3c },
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 },
    { 0x00, 0xff, 0x81, 0xbd, 0xa1, 0xbd, 0x81, 0xff },
    { 0x24, 0x7e, 0xa5, 0x7e, 0x24, 0x00, 0x00, 0x00 },
    { 0x3c, 0x42, 0x99, 0x81, 0x99, 0x99, 0x42, 0x3c },
    { 0xbc, 0x00, 0xbe, 0x00, 0xb8, 0x00, 0xbc, 0x00 },
    { 0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 0x7e, 0x00 },
    { 0x3c, 0x42, 0x99, 0xa5, 0x99, 0x42, 0x3c, 0x00 },
};
static const uint8_t TAG_COL[4] = { PAL_AMBER, PAL_MINT, PAL_CYAN, PAL_IVORY };

static const int BAR_H = 22;
static const int LOG_ROWS = 24;
static const int LOG_W = 80;

static int s_screen = SCR_RAIN;
static int s_cursor = 0;
static int s_bot = 2;
static int s_term = 2;
static int s_color = 0;
static int s_mix = 0;
static int s_saver = 0;
static int s_saver_on = 1;
static int s_idle = 1;
static int s_ret = -1;
static bool s_dirty = true;
/* Full INT repaint. Screen changes and boot only. -1 forces the next partial. */
static bool s_int_dirty = true;
static int s_int_bot = -1;
static uint32_t s_bus_t0 = 0;
/* Scan-list hunt menu. -1 is closed. Tags and ignores are per band, three devices. */
static int s_act = -1;
static int s_act_dev = -1;
static int s_info_dev = -1;
static int s_hunt_dev = -1;
static int s_hunt_band = 0;
static uint8_t s_dev_tag[2][3];
static uint8_t s_dev_ign[2][3];
static uint8_t s_dev_fol[2][3];
/* Nav focus. Drawn in place. Never promotes a full-panel clear. */
static int s_focus_old = -1;
static int s_focus_new = -1;
static bool s_focus_pend = false;
static bool s_track = false;
static bool s_focus_cmds = false;
static bool s_sheet = false;
static int s_sheet_i = 0;
static int s_cmd_sel = 0;
static int s_hist_n = 0;
static int s_hist_i = -1;
static int s_found_n = 0;
static int s_log_n = 0;
static int s_log_next = 0;
static int s_filter = 0; /* 0 off, 1 wifi, 2 ble */
static char s_track_name[28];
static uint32_t s_boot_ms = 0;
static char s_clk[8];
static char s_up[8];
static int s_prompt_phase = -1;
static int s_tab_i = -1;
static int s_shape = 0;
enum { MOOD_NEUTRAL = 0, MOOD_HAPPY, MOOD_EXCITED, MOOD_HUNTING, MOOD_SLEEPY, MOOD_SAD, MOOD_ALERT };
static int s_mood = MOOD_NEUTRAL;
static uint32_t s_mood_until = 0;
static uint32_t s_last_key = 0;
static char s_bub[8];
static char s_gp_buf[20];
static char s_gp_log[2][28];
static int s_gp_n = 0;
static bool s_gp_focus = false;
static const int CP_N = 8;
extern uint8_t botcorps_t0_rain_cell[3][1200];
static_assert(1120 + 60 <= 1200, "tab scratch fits inside the rain cell plane");
static char *tab_base(void) { return (char *)&botcorps_t0_rain_cell[1][1120]; }
static char *tab_last(void) { return (char *)&botcorps_t0_rain_cell[2][1120]; }
static const char *const NVS_NS = "botcorps";
static const char *const NVS_KEY = "rain_col";

static uint8_t accent(void)
{
    if (s_color < 0 || s_color > 2) return ACCENT_C[0];
    return ACCENT_C[s_color];
}

static const char *screen_name(int id)
{
    switch (id) {
    case SCR_RAIN: return "rain";
    case SCR_GIBSON: return "gibson";
    case SCR_TERMINAL: return "terminal";
    case SCR_WIFI: return "wifi";
    case SCR_WIFI_RADAR: return "wifi-radar";
    case SCR_WIFI_LIST: return "wifi-list";
    case SCR_BT: return "bluetooth";
    case SCR_BT_RADAR: return "bt-radar";
    case SCR_BT_LIST: return "bt-list";
    case SCR_BOT: return "bot";
    case SCR_BOT_RADAR: return "bot-radar";
    case SCR_SCAN: return "scan";
    case SCR_CONTROL: return "control";
    case SCR_DIAG: return "diag";
    default: return "screen";
    }
}

static int default_back(int id)
{
    switch (id) {
    case SCR_GIBSON: return SCR_RAIN;
    case SCR_TERMINAL: return SCR_GIBSON;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT: return SCR_TERMINAL;
    case SCR_WIFI_RADAR: return SCR_WIFI;
    case SCR_BT_RADAR: return SCR_BT;
    case SCR_BOT_RADAR: return SCR_BOT;
    case SCR_WIFI_LIST: return SCR_WIFI_RADAR;
    case SCR_BT_LIST: return SCR_BT_RADAR;
    case SCR_SCAN:
    case SCR_CONTROL:
    case SCR_DIAG: return SCR_GIBSON;
    default: return -1;
    }
}

static bool wants_rain(void)
{
    return s_screen != SCR_WIFI_LIST && s_screen != SCR_BT_LIST
        && s_screen != SCR_DIAG && s_screen != SCR_TERMINAL;
}

/* EXT chrome that moves on its own: sweep, pulse, bob, or the prompt caret. */
static bool ext_animates(void)
{
    switch (s_screen) {
    case SCR_GIBSON:
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT:
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_BOT_RADAR:
    case SCR_SCAN:
    case SCR_TERMINAL:
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        return true;
    default:
        return false;
    }
}

/* Fixed 140 ms step. A screen with no rain uses this instead of the loop rate. */
static bool due_140(uint32_t *last)
{
    const uint32_t now = millis();
    if (*last != 0 && (uint32_t)(now - *last) < 140u) return false;
    *last = now;
    return true;
}

static bool anim_step_due(void)
{
    static uint32_t s_anim_ms = 0;
    return due_140(&s_anim_ms);
}

bool botcorps_ui_wants_ext_rain(void)
{
    return wants_rain();
}

static int card_bot(void)
{
    switch (s_screen) {
    case SCR_WIFI:
    case SCR_WIFI_RADAR:
    case SCR_WIFI_LIST: return 0;
    case SCR_BT:
    case SCR_BT_RADAR:
    case SCR_BT_LIST: return 1;
    case SCR_BOT:
    case SCR_BOT_RADAR: return s_bot;
    case SCR_TERMINAL: return s_term;
    default: return -1;
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
}

/* 6 = Font0. 8 = Font8x8C64, the largest built-in that fits a 12px row. */
static int s_px = 6;

static void use_px(Gfx g, int px)
{
    s_px = px;
    if (px >= 8) g.setFont(&fonts::Font8x8C64);
    else g.setFont(&fonts::Font0);
    g.setTextSize(1);
}

static void txt(Gfx g, const char *s, int x, int y, uint8_t col,
                lgfx::textdatum_t dat)
{
    use_px(g, s_px);
    g.setTextColor(col);
    g.setTextDatum(dat);
    g.drawString(s ? s : "", x, y);
    g.setTextDatum(lgfx::textdatum::top_left);
}

/* Opaque glyphs. The background is part of the character, not a fillRect under it. */
static void txt_bg(Gfx g, const char *s, int x, int y, uint8_t fg, uint8_t bg,
                   lgfx::textdatum_t dat)
{
    use_px(g, s_px);
    g.setTextColor(fg, bg);
    g.setTextDatum(dat);
    g.drawString(s ? s : "", x, y);
    g.setTextDatum(lgfx::textdatum::top_left);
}

static void plate(Gfx g, int x, int y, int w, int h,
                  uint8_t fill, uint8_t edge)
{
    g.fillRect(x, y, w, h, fill);
    g.drawRect(x, y, w, h, edge);
}

static void draw_blob(Gfx g, int x, int y, int scale, uint8_t c, int shape, bool blink)
{
    botcorps_sprite_draw(*g.raw(), x - g.ox, y - g.oy, shape, scale, c, blink);
}

static void draw_icon(Gfx g, int x, int y, int id, uint8_t col)
{
    if (id == ICO_TERM) {
        g.drawRect(x + 4, y + 6, 20, 16, col);
        g.fillRect(x + 5, y + 7, 18, 3, col);
        g.drawLine(x + 8, y + 16, x + 12, y + 16, col);
        g.drawLine(x + 12, y + 16, x + 14, y + 14, col);
        g.drawLine(x + 16, y + 14, x + 20, y + 14, col);
    } else if (id == ICO_BACK) {
        g.fillTriangle(x + 8, y + 14, x + 16, y + 8, x + 16, y + 20, col);
        g.fillRect(x + 16, y + 12, 8, 4, col);
    } else if (id == ICO_PLAY) {
        g.fillTriangle(x + 9, y + 8, x + 9, y + 20, x + 20, y + 14, col);
    } else if (id == ICO_LIST) {
        for (int r = 0; r < 3; ++r) {
            g.fillRect(x + 6, y + 8 + r * 5, 3, 2, col);
            g.fillRect(x + 11, y + 8 + r * 5, 11, 2, col);
        }
    } else if (id == ICO_TRACK) {
        g.drawCircle(x + 14, y + 14, 7, col);
        g.fillRect(x + 13, y + 4, 2, 6, col);
        g.fillRect(x + 13, y + 18, 2, 6, col);
        g.fillRect(x + 4, y + 13, 6, 2, col);
        g.fillRect(x + 18, y + 13, 6, 2, col);
        g.fillRect(x + 13, y + 13, 2, 2, col);
    } else if (id == ICO_RADAR) {
        g.drawCircle(x + 14, y + 16, 8, col);
        g.drawCircle(x + 14, y + 16, 4, col);
        g.drawLine(x + 14, y + 16, x + 20, y + 10, col);
        g.fillRect(x + 13, y + 15, 2, 2, col);
    } else if (id == ICO_CTRL) {
        for (int k = 0; k < 3; ++k) {
            g.fillRect(x + 7 + k * 7, y + 6, 2, 16, COL_LINE);
            g.fillRect(x + 5 + k * 7, y + (k == 1 ? 14 : 9), 6, 4, col);
        }
    } else {
        for (int k = 0; k < 3; ++k) {
            for (int c = 0; c < 3; ++c) {
                g.fillRect(x + 7 + c * 5, y + 7 + k * 5, 3, 3, col);
            }
        }
    }
}

static const char *btn_name(int id)
{
    if (id == ICO_PLAY) return "play";
    if (id == ICO_TERM) return "terminal";
    if (id == ICO_CTRL) return "control";
    if (id == ICO_RADAR) return "radar";
    if (id == ICO_BACK) return "back";
    if (id == ICO_TRACK) return "track";
    if (id == ICO_LIST) return "list";
    return "";
}

static bool btn_locked(int id)
{
    if (id == ICO_PLAY && s_screen == SCR_RAIN) return true;
    if (id == ICO_BACK && s_screen == SCR_DIAG) return true;
    return false;
}

static uint8_t label_bg(void)
{
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) return COL_DEEP;
    if (s_screen == SCR_GIBSON) return botcorps_pal_hub_of(COL_FIELD);
    if (!wants_rain()) return COL_BG;
    return COL_FIELD;
}

static void draw_btn(Gfx g, int x, int y, int id, bool sel)
{
    const uint8_t edge = sel ? accent() : COL_LINE;
    const uint8_t fill = sel ? COL_SEL : COL_DEEP;
    plate(g, x, y, BS, BS, fill, edge);
    if (sel) g.drawRect(x + 1, y + 1, BS - 2, BS - 2, accent());
    draw_icon(g, x, y, id, sel ? accent() : COL_MUTED);
    const bool icon_only = id == ICO_BACK &&
                           (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST);
    const char *nm = btn_name(id);
    const bool show = !icon_only && (sel || btn_locked(id));
    if (show && nm[0]) {
        const int tw = (int)strlen(nm) * 6;
        const int lx = x + (BS - tw) / 2;
        g.fillRect(lx, y + BS, tw, 8, label_bg());
        txt_bg(g, nm, lx, y + BS, bot_color(card_bot()), label_bg(), lgfx::textdatum::top_left);
    }
}

static botcorps_cover_t s_box[24];
static int s_box_n = 0;
static bool s_box_radar = false;

static void cover_reset(bool radar)
{
    s_box_n = 0;
    s_box_radar = radar;
}

static void cover_add(int x, int y, int w, int h)
{
    if (s_box_n >= 24 || w <= 0 || h <= 0) return;
    s_box[s_box_n++] = { (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h };
}

/* Icon plus the Font0 name that sits on the row under the box. */
static void cover_btn(int x, int y)
{
    int bx = x - 12;
    int bw = BS + 24;
    if (bx < 0) {
        bw += bx;
        bx = 0;
    }
    if (bx + bw > 320) bw = 320 - bx;
    cover_add(bx, y, bw, BS + 8);
}

static void term_xy(int n, int *x, int *y)
{
    *x = 7 + (n % 4) * 78;
    *y = 19 + (n / 4) * 50;
}

static void layout_covers(void)
{
    cover_reset(false);
    switch (s_screen) {
    case SCR_RAIN:
        cover_btn(146, 206);
        break;
    case SCR_GIBSON:
        cover_add(4, 18, 36, 170);
        cover_add(280, 18, 36, 170);
        cover_add(110, 20, 100, 140);
        cover_add(38, 164, 244, 38);
        cover_add(90, 152, 140, 14);
        cover_btn(110, 206);
        cover_btn(146, 206);
        cover_btn(182, 206);
        break;
    case SCR_TERMINAL:
        cover_add(0, 0, 320, 17);
        for (int i = 0; i < 12; ++i) {
            int x, y;
            term_xy(i, &x, &y);
            cover_add(x - 2, y - 2, 76, 50);
        }
        cover_add(4, 167, 312, 72);
        if (s_sheet) cover_add(8, 21, 304, 142);
        break;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT:
        cover_add(96, 14, 128, 120);
        cover_btn(12, 133);
        cover_btn(44, 133);
        cover_btn(248, 133);
        cover_btn(280, 133);
        break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
        cover_reset(false);
        cover_btn(14, 72);
        cover_btn(14, 106);
        cover_btn(14, 140);
        cover_add(50, 26, 220, 182);
        cover_add(40, 210, 240, 22);
        break;
    case SCR_BOT_RADAR:
    case SCR_SCAN:
        cover_reset(false);
        cover_btn(14, 90);
        cover_btn(14, 124);
        cover_add(50, 26, 220, 182);
        cover_add(40, 210, 240, 22);
        break;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        cover_add(0, 0, 320, 24);
        cover_add(8, 26, 304, 182);
        cover_add(0, 209, 320, 31);
        cover_btn(286, 206);
        break;
    case SCR_CONTROL:
        for (int i = 0; i < CP_N; ++i) cover_add(8, 4 + i * 20, 304, 18);
        cover_add(14, 192, 200, 14);
        cover_btn(14, 204);
        cover_btn(48, 204);
        break;
    default:
        break;
    }
    botcorps_rain_set_covers(s_box, s_box_n, s_box_radar);
}

static int button_count(void)
{
    switch (s_screen) {
    case SCR_RAIN: return 1;
    case SCR_GIBSON: return 3;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT: return 4;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: return 3;
    case SCR_BOT_RADAR:
    case SCR_SCAN: return 2;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST: return 4;
    case SCR_CONTROL: return CP_N + 2;
    case SCR_DIAG: return 1;
    default: return 0;
    }
}

static void partial_int(void);

static void enter_screen(int id, int ret)
{
    if (id < 0) return;
    const char *from = screen_name(s_screen);
    const int before = card_bot();
    s_ret = ret;
    s_screen = id;
    s_cursor = 0;
    s_sheet = false;
    s_focus_cmds = false;
    botcorps_lut_term(false);
    s_bus_t0 = millis();
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    if (id == SCR_WIFI_LIST || id == SCR_BT_LIST) {
        s_hunt_dev = -1;
        memset(s_dev_tag, 0, sizeof(s_dev_tag));
        memset(s_dev_ign, 0, sizeof(s_dev_ign));
        memset(s_dev_fol, 0, sizeof(s_dev_fol));
    }
    s_dirty = true;
    /* Same bot: partial_int returns. A real card change redraws in strips. */
    if (card_bot() != before) {
        s_int_dirty = false;
        partial_int();
    }
    Serial.printf("[int] enter_screen %s\n", screen_name(id));
    botcorps_probe_screen(from, screen_name(id));
}

static void go_back(void)
{
    const int b = s_ret >= 0 ? s_ret : default_back(s_screen);
    s_ret = -1;
    if (b >= 0) enter_screen(b, -1);
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

static char *log_slot(int i)
{
    return botcorps_t0_term_log + (i % LOG_ROWS) * LOG_W;
}

static void log_add(const char *s)
{
    snprintf(log_slot(s_log_next), LOG_W, "%s", s ? s : "");
    s_log_next = (s_log_next + 1) % LOG_ROWS;
    if (s_log_n < LOG_ROWS) ++s_log_n;
}

/* First two bytes mark a bot-coloured reply. 12 is Gibson. */
static void log_add_bot(int bot, const char *s)
{
    char *d = log_slot(s_log_next);
    d[0] = '\x01';
    d[1] = (char)(bot < 0 ? 12 : bot);
    snprintf(d + 2, (size_t)LOG_W - 2, "%s", s ? s : "");
    s_log_next = (s_log_next + 1) % LOG_ROWS;
    if (s_log_n < LOG_ROWS) ++s_log_n;
}

static const char *log_line(int logical)
{
    const int slot = (s_log_next - s_log_n + logical + LOG_ROWS * 2) % LOG_ROWS;
    return log_slot(slot);
}

static void log_clear(void)
{
    s_log_n = 0;
    s_log_next = 0;
    memset(botcorps_t0_term_log, 0, 2048);
}

static void found_add(const char *name)
{
    if (!name || !name[0]) return;
    for (int i = 0; i < s_found_n; ++i) {
        if (strcmp(botcorps_t0_term_found[i], name) == 0) return;
    }
    if (s_found_n >= 12) {
        memmove(botcorps_t0_term_found[0], botcorps_t0_term_found[1], 11 * 28);
        s_found_n = 11;
    }
    snprintf(botcorps_t0_term_found[s_found_n], 28, "%s", name);
    ++s_found_n;
}

static void hist_push(const char *line)
{
    if (!line || !line[0]) return;
    if (s_hist_n > 0 && strncmp(botcorps_t0_term_hist[s_hist_n - 1], line, 25) == 0) {
        s_hist_i = -1;
        return;
    }
    if (s_hist_n >= 20) {
        memmove(botcorps_t0_term_hist[0], botcorps_t0_term_hist[1], 19 * 26);
        s_hist_n = 19;
    }
    snprintf(botcorps_t0_term_hist[s_hist_n], 26, "%s", line);
    ++s_hist_n;
    s_hist_i = -1;
}

static const char *RAIN_WORD[7] = {
    "MULTI", "GREEN", "CYAN", "BLUE", "PURPLE", "AMBER", "WHITE"
};

static void partial_prompt_panel(void);
static void partial_box(int index, bool sel);
static void partial_tile(int index);
static void partial_int(void);
static void partial_sheet(void);
static void restore_under_sheet(void);

static void run_cmd(const char *line)
{
    char buf[80];
    snprintf(buf, sizeof(buf), "%s", line ? line : "");
    char *cmd = buf;
    while (*cmd == ' ') ++cmd;
    if (!cmd[0]) return;
    hist_push(cmd);
    char *sp = strchr(cmd, ' ');
    char *arg = 0;
    if (sp) {
        *sp = 0;
        arg = sp + 1;
        while (*arg == ' ') ++arg;
    }
    if (strcmp(cmd, "sweep") == 0) cmd = (char *)"scan";
    else if (strcmp(cmd, "roster") == 0) cmd = (char *)"bots";
    else if (strcmp(cmd, "pulse") == 0) cmd = (char *)"ping";
    else if (strcmp(cmd, "dig") == 0) cmd = (char *)"info";
    char echo[80];
    snprintf(echo, sizeof(echo), "> %s%s%s", cmd, arg ? " " : "", arg ? arg : "");
    log_add(echo);

    if (strcmp(cmd, "help") == 0) {
        for (int g = 0; g < 4; ++g) {
            char row[80];
            int n = snprintf(row, sizeof(row), "%s", G_NAME[g]);
            for (int i = G_AT[g]; i < G_AT[g + 1] && n < 68; ++i)
                n += snprintf(row + n, sizeof(row) - (size_t)n, " %s", CMDN[i]);
            log_add(row);
        }
    } else if (strcmp(cmd, "bots") == 0) {
        log_add("12 boards");
        for (int i = 0; i < 12; ++i) {
            char row[80];
            const char *stn = BOTS[i].st == 2 ? "hold" : BOTS[i].st == 1 ? "limited" : "ready";
            snprintf(row, sizeof(row), "  %s %s", BOTS[i].short_name, stn);
            log_add(row);
        }
    } else if (strcmp(cmd, "clear") == 0) {
        log_clear();
    } else if (strcmp(cmd, "scan") == 0 || strcmp(cmd, "hunt") == 0) {
        int who = s_term;
        if (arg && arg[0]) {
            who = -1;
            for (int i = 0; i < 12; ++i) {
                if (strncmp(BOTS[i].short_name, arg, strlen(arg)) == 0) {
                    who = i;
                    break;
                }
            }
            if (strcmp(arg, "wifi") == 0) who = 0;
            if (strcmp(arg, "ble") == 0 || strcmp(arg, "bluetooth") == 0) who = 1;
        }
        if (who < 0) {
            log_add("no such bot");
        } else {
            const int prev = s_term;
            s_term = who;
            char row[80];
            snprintf(row, sizeof(row), "%s %s", strcmp(cmd, "hunt") == 0 ? "hunting" : "scanning",
                     BOTS[who].short_name);
            log_add(row);
            if (BOTS[who].radio == 1) {
                for (int i = 0; i < 3; ++i) {
                    snprintf(row, sizeof(row), "  %s %s", NEAR_WIFI[i], NEAR_WIFI_R[i]);
                    log_add(row);
                    found_add(NEAR_WIFI[i]);
                }
            } else if (BOTS[who].radio == 2) {
                for (int i = 0; i < 3; ++i) {
                    snprintf(row, sizeof(row), "  %s %s", NEAR_BLE[i], NEAR_BLE_R[i]);
                    log_add(row);
                    found_add(NEAR_BLE[i]);
                }
            } else {
                snprintf(row, sizeof(row), "  frames %d", 240 + who * 17);
                log_add(row);
            }
            log_add("[ok] sweep done");
            partial_prompt_panel();
            if (prev != s_term) {
                partial_tile(prev);
                partial_tile(s_term);
                partial_int();
            }
            return;
        }
    } else if (strcmp(cmd, "rain") == 0) {
        if (!arg || !arg[0]) {
            char row[80];
            snprintf(row, sizeof(row), "rain %s", RAIN_WORD[botcorps_rain_mode()]);
            log_add(row);
        } else if (strcmp(arg, "multi") == 0 || strcmp(arg, "mixed") == 0) {
            botcorps_rain_set_mode(0);
            save_rain_color();
            log_add("rain MULTI");
        } else {
            int mode = -1;
            for (int i = 1; i < 7; ++i) {
                const char *w = RAIN_WORD[i];
                const char *a = arg;
                bool same = true;
                while (*w && *a) {
                    char ca = *a;
                    if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
                    if (ca != *w) same = false;
                    ++w;
                    ++a;
                }
                if (same && *w == 0 && *a == 0) mode = i;
            }
            if (mode < 0) log_add("unknown colour");
            else {
                botcorps_rain_set_mode(mode);
                save_rain_color();
                char row[40];
                snprintf(row, sizeof(row), "rain %s", RAIN_WORD[mode]);
                log_add(row);
            }
        }
    } else if (strcmp(cmd, "filter") == 0) {
        if (!arg || strcmp(arg, "off") == 0) s_filter = 0;
        else if (strcmp(arg, "wifi") == 0) s_filter = 1;
        else if (strcmp(arg, "ble") == 0) s_filter = 2;
        else {
            log_add("unknown filter");
            partial_prompt_panel();
            return;
        }
        log_add(s_filter == 1 ? "filter wifi" : s_filter == 2 ? "filter ble" : "filter off");
    } else if (strcmp(cmd, "ping") == 0 || strcmp(cmd, "info") == 0 ||
               strcmp(cmd, "track") == 0 || strcmp(cmd, "sniff") == 0) {
        if (strcmp(cmd, "track") == 0 && arg && strcmp(arg, "off") == 0) {
            s_track_name[0] = 0;
            log_add("track off");
        } else if (s_found_n == 0) {
            log_add("scan first");
        } else if (strcmp(cmd, "sniff") == 0) {
            for (int i = 0; i < s_found_n; ++i) {
                char row[80];
                snprintf(row, sizeof(row), "  %s", botcorps_t0_term_found[i]);
                log_add(row);
            }
        } else {
            const char *name = botcorps_t0_term_found[s_found_n - 1];
            if (arg && arg[0]) {
                name = 0;
                for (int i = 0; i < s_found_n; ++i) {
                    if (strncmp(botcorps_t0_term_found[i], arg, strlen(arg)) == 0) {
                        name = botcorps_t0_term_found[i];
                        break;
                    }
                }
            }
            if (!name) log_add("no such device");
            else if (strcmp(cmd, "ping") == 0) {
                char row[80];
                snprintf(row, sizeof(row), "pong %s", name);
                log_add(row);
            } else if (strcmp(cmd, "info") == 0) {
                char row[80];
                snprintf(row, sizeof(row), "%s heard", name);
                log_add(row);
            } else {
                snprintf(s_track_name, sizeof(s_track_name), "%s", name);
                char row[80];
                snprintf(row, sizeof(row), "tracking %s", name);
                log_add(row);
            }
        }
    } else if (strcmp(cmd, "go") == 0 || strcmp(cmd, "load") == 0) {
        int id = -1;
        if (!arg) log_add("rain gibson terminal wifi ble control diag");
        else if (strcmp(arg, "rain") == 0) id = SCR_RAIN;
        else if (strcmp(arg, "gibson") == 0) id = SCR_GIBSON;
        else if (strcmp(arg, "terminal") == 0) id = SCR_TERMINAL;
        else if (strcmp(arg, "wifi") == 0) id = SCR_WIFI;
        else if (strcmp(arg, "ble") == 0) id = SCR_BT;
        else if (strcmp(arg, "control") == 0) id = SCR_CONTROL;
        else if (strcmp(arg, "diag") == 0) id = SCR_DIAG;
        else if (strcmp(arg, "bots") == 0) id = SCR_BOT;
        else log_add("no such screen");
        if (id >= 0) {
            enter_screen(id, SCR_TERMINAL);
            return;
        }
    } else if (strcmp(cmd, "cmds") == 0) {
        s_sheet = true;
        s_sheet_i = 0;
        s_focus_cmds = false;
        s_dirty = true;
        return;
    } else if (strcmp(cmd, "hail") == 0 || strcmp(cmd, "ask") == 0 || strcmp(cmd, "link") == 0 ||
               strcmp(cmd, "mute") == 0 || strcmp(cmd, "say") == 0 || strcmp(cmd, "whisper") == 0) {
        int bot = -1;
        if (!arg || !arg[0]) bot = -1;
        else if (strcmp(arg, "gibson") == 0 || strcmp(arg, "gib") == 0) bot = 12;
        else {
            bot = -2;
            for (int i = 0; i < 12; ++i) {
                if (strncmp(BOTS[i].short_name, arg, strlen(arg)) == 0) {
                    bot = i;
                    break;
                }
            }
        }
        if (bot == -1) {
            log_add(cmd);
        } else if (bot < 0) {
            log_add("no such bot");
        } else {
            const char *who = bot == 12 ? "gibson" : BOTS[bot].short_name;
            char row[80];
            if (strcmp(cmd, "mute") == 0) snprintf(row, sizeof(row), "%s hushed", who);
            else if (strcmp(cmd, "link") == 0) snprintf(row, sizeof(row), "%s channel open", who);
            else if (strcmp(cmd, "ask") == 0) {
                const char *st = bot == 12 ? "crown lit" : BOTS[bot].st == 2 ? "hold" : BOTS[bot].st == 1 ? "limited" : "ready";
                snprintf(row, sizeof(row), "%s %s", who, st);
            } else if (strcmp(cmd, "say") == 0 || strcmp(cmd, "whisper") == 0) {
                snprintf(row, sizeof(row), "%s %s", who, CHAT3[(int)strlen(arg) % 3]);
            } else {
                snprintf(row, sizeof(row), "%s %s", who, HAIL[bot == 12 ? 0 : bot + 1]);
            }
            if (strcmp(cmd, "whisper") == 0 || strcmp(cmd, "mute") == 0) log_add(row);
            else log_add_bot(bot, row);
        }
    } else if (strcmp(cmd, "rollcall") == 0) {
        log_add_bot(12, "gibson reading you");
        for (int i = 0; i < 12; ++i) {
            char row[40];
            snprintf(row, sizeof(row), "%s %s", BOTS[i].short_name, HAIL[i + 1]);
            log_add_bot(i, row);
        }
    } else if (strcmp(cmd, "broadcast") == 0) {
        log_add_bot(12, "gibson reading you");
        log_add_bot(3, "hack copy");
        log_add_bot(5, "drone on it");
        log_add_bot(4, "flock noted");
        log_add_bot(0, "wifi copy");
    } else if (strcmp(cmd, "history") == 0) {
        if (s_hist_n <= 0) log_add("history empty");
        else {
            const int n = s_hist_n < 4 ? s_hist_n : 4;
            for (int i = s_hist_n - n; i < s_hist_n; ++i) log_add(botcorps_t0_term_hist[i]);
        }
    } else if (strcmp(cmd, "theme") == 0) {
        log_add("theme neon");
    } else if (strcmp(cmd, "about") == 0) {
        log_add("BOTcorps term 3.2");
    } else if (strcmp(cmd, "version") == 0) {
        log_add("term 3.2");
    } else if (strcmp(cmd, "uptime") == 0) {
        char row[32];
        const uint32_t sec = (millis() - s_boot_ms) / 1000u;
        snprintf(row, sizeof(row), "uptime %u m %u s", (unsigned)(sec / 60u), (unsigned)(sec % 60u));
        log_add(row);
    } else if (strcmp(cmd, "train") == 0 || strcmp(cmd, "hunt-run") == 0 || strcmp(cmd, "score") == 0 ||
               strcmp(cmd, "rank") == 0 || strcmp(cmd, "loadout") == 0 || strcmp(cmd, "lobby") == 0 ||
               strcmp(cmd, "duel") == 0 || strcmp(cmd, "stash") == 0 || strcmp(cmd, "defend") == 0) {
        log_add("sealed");
    } else {
        log_add("command not found");
    }
    partial_prompt_panel();
}

static void open_bot(int index)
{
    if (index < 0 || index > 11) return;
    s_term = index;
    if (BOTS[index].radio == 1) enter_screen(SCR_WIFI, SCR_TERMINAL);
    else if (BOTS[index].radio == 2) enter_screen(SCR_BT, SCR_TERMINAL);
    else {
        s_bot = index;
        enter_screen(SCR_BOT, SCR_TERMINAL);
    }
}

static void boot_log(void)
{
    if (s_log_n > 0) return;
    log_add("BOTcorps term");
    log_add("[ok] 12 boards");
}

static uint8_t term_accent(void);
static uint8_t term_low(void);
static uint8_t glow_of(int step);

static void paint_one_board(Gfx g, int i, bool sel, bool dim)
{
    int x, y;
    term_xy(i, &x, &y);
    const bool wh = BOTS[i].radio != 0;
    const uint8_t ac = accent();
    const uint32_t t = millis();
    const int w = 72;
    const int h = 46;
    if (sel && !dim) {
        const uint8_t wash = (s_color == 1) ? (uint8_t)PAL_WASH_A
                             : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
        const uint8_t wire = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                             : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        g.drawRect(x - 4, y - 4, w + 8, h + 8, wash);
        g.drawRect(x - 3, y - 3, w + 6, h + 6, wire);
        const int ph = (int)((t / 70u) % 2u);
        const int n = 4;
        for (int d = 0, k = 0; d < w + 4; d += n, ++k) {
            if (((k + ph) % 2) != 0) continue;
            int dw = n;
            if (d + dw > w + 2) dw = w + 2 - d;
            if (dw <= 0) continue;
            g.fillRect(x - 2 + d, y - 2, dw, 1, ac);
            const int bx = x + w + 1 - d - n + (d + n > w + 2 ? d + n - (w + 2) : 0);
            g.fillRect(bx, y + h + 1, dw, 1, ac);
        }
        for (int d = 0, k = 0; d < h + 4; d += n, ++k) {
            if (((k + ph) % 2) != 0) continue;
            int dh = n;
            if (d + dh > h + 2) dh = h + 2 - d;
            if (dh <= 0) continue;
            g.fillRect(x + w + 1, y - 2 + d, 1, dh, ac);
            const int by = y + h + 1 - d - n + (d + n > h + 2 ? d + n - (h + 2) : 0);
            g.fillRect(x - 2, by, 1, dh, ac);
        }
    }
    const uint8_t base = dim ? (wh ? (uint8_t)PAL_LINE : (uint8_t)PAL_FIELD)
                             : (wh ? (sel ? (uint8_t)PAL_IVORY : (uint8_t)PAL_BOARD_L)
                                   : (uint8_t)PAL_BOARD_D);
    const uint8_t edge = sel ? ac : (wh ? (uint8_t)150 : (uint8_t)PAL_LINE);
    const uint8_t tr = wh ? (uint8_t)PAL_CHIN : (sel ? (uint8_t)PAL_STEEL : (uint8_t)PAL_LINE);
    const uint8_t ant = wh ? (uint8_t)PAL_HUB_HEAD : (uint8_t)PAL_STEEL;
    const uint8_t pad = (uint8_t)PAL_SOFT;
    const uint8_t fp = sel ? (uint8_t)PAL_BLACK : (uint8_t)PAL_FIELD;
    const uint8_t fpp = wh ? (uint8_t)PAL_ICON_C3 : (uint8_t)PAL_SOFT;
    const uint8_t silk = wh ? (uint8_t)PAL_LINE : (sel ? (uint8_t)PAL_STEEL : (uint8_t)PAL_MUTED);
    const uint8_t led = wh ? (uint8_t)PAL_CHIN : (uint8_t)PAL_BOARD_D;
    const uint8_t hole = (uint8_t)PAL_FIELD;
    g.fillRect(x + 1, y, w - 2, h, base);
    g.fillRect(x, y + 1, w, h - 2, base);
    g.fillRect(x + 1, y, w - 2, 1, edge);
    g.fillRect(x + 1, y + h - 1, w - 2, 1, edge);
    g.fillRect(x, y + 1, 1, h - 2, edge);
    g.fillRect(x + w - 1, y + 1, 1, h - 2, edge);
    /* Sheet-covered rows keep their face. The trace stubs read as stray dashes. */
    const bool traces = !dim && !(s_sheet && i >= 4);
    const int longtr[4] = { 1, 3, 5, 6 };
    if (traces) {
        for (int n = 0; n < 4; ++n) {
            const int k = longtr[n];
            g.fillRect(x + 4, y + 6 + k * 4, 19, 1, tr);
            g.fillRect(x + 49, y + 6 + k * 4, 19, 1, tr);
        }
        const int shorttr[4] = { 0, 2, 4, 7 };
        for (int n = 0; n < 4; ++n) {
            const int k = shorttr[n];
            g.fillRect(x + 4, y + 6 + k * 4, 5, 1, tr);
            g.fillRect(x + 63, y + 6 + k * 4, 5, 1, tr);
        }
        g.fillRect(x + 26, y + 9, 1, 5, tr);
    }
    for (int k = 0; k < 6; ++k) {
        g.fillRect(x + 26 + k * 4, y + 3, 1, 7, ant);
        if (traces && k < 5) g.fillRect(x + 26 + k * 4, (k % 2) ? y + 9 : y + 3, 5, 1, ant);
    }
    const int run = (int)((t / 90u) % 10u);
    for (int k = 0; k < 8; ++k) {
        const bool lit_l = sel && (k == run || (k == run - 1 && k >= 0));
        const bool lit_r = sel && ((7 - k) == run || (7 - k) == run - 1);
        g.fillRect(x + 2, y + 5 + k * 4, 2, 2, lit_l ? ac : pad);
        g.fillRect(x + 68, y + 5 + k * 4, 2, 2, lit_r ? ac : pad);
    }
    g.fillRect(x + 62, y + 2, 3, 3, pad);
    g.fillRect(x + 63, y + 3, 1, 1, hole);
    g.fillRect(x + 23, y + 14, 26, 22, fp);
    for (int k = 0; k < 8; ++k) {
        g.fillRect(x + 25 + k * 3, y + 14, 1, 1, fpp);
        g.fillRect(x + 25 + k * 3, y + 35, 1, 1, fpp);
    }
    if (sel) {
        g.fillRect(x + 22, y + 13, 28, 1, ac);
        g.fillRect(x + 22, y + 36, 28, 1, ac);
        g.fillRect(x + 22, y + 13, 1, 24, ac);
        g.fillRect(x + 49, y + 13, 1, 24, ac);
    }
    if (BOTS[i].radio == 1 && !dim) {
        const uint8_t m = sel ? ac : (uint8_t)PAL_STEEL;
        g.fillRect(x + 8, y + 6, 1, 1, m);
        g.fillRect(x + 6, y + 4, 1, 1, m);
        g.fillRect(x + 10, y + 4, 1, 1, m);
        g.fillRect(x + 5, y + 3, 1, 1, m);
        g.fillRect(x + 11, y + 3, 1, 1, m);
        g.fillRect(x + 4, y + 2, 1, 1, m);
        g.fillRect(x + 12, y + 2, 1, 1, m);
    }
    const int bob = sel ? -((int)((t / 450u) % 2u)) : 0;
    draw_blob(g, x + 36, y + 25 + bob, 2, bot_color(i), botcorps_shape_for(i), sel && sprite_blink(i));
    const int st = BOTS[i].st;
    const bool on = st != 0 || ((t / 700u + (uint32_t)i) % 4u) != 0u;
    uint8_t ledc = led;
    if (on) ledc = st == 2 ? (uint8_t)PAL_HOLD : st == 1 ? (uint8_t)PAL_AMBER : (uint8_t)PAL_OK;
    g.fillRect(x + 6, y + 2, 2, 2, ledc);
    char up[16];
    const char *src = BOTS[i].short_name;
    int n = 0;
    for (; src[n] && n < 15; ++n) {
        char c = src[n];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        up[n] = c;
    }
    up[n] = 0;
    if (!dim && !(s_sheet && i >= 4)) {
        const int keep = s_px;
        s_px = 6;
        txt(g, up, x + 36, y + 38, silk, lgfx::textdatum::top_center);
        s_px = keep;
    }
}

static void paint_boards(Gfx g)
{
    for (int i = 0; i < 12; ++i) {
        paint_one_board(g, i, (i == s_term) && !s_focus_cmds, false);
    }
}

static void paint_palette(Gfx g)
{
    /* Mock tmChip: 6 x 2 chips over the scrollback. */
    g.fillRect(5, 169, 268, 38, COL_DEEP);
    for (int n = 0; n < 12; ++n) {
        const int x = 8 + (n % 6) * 44;
        const int y = 171 + (n / 6) * 18;
        const bool sel = n == s_cmd_sel;
        plate(g, x, y, 41, 15, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt_bg(g, CMDS[n], x + 20, y + 4, sel ? COL_WHITE : COL_MUTED, sel ? COL_SEL : COL_BG,
               lgfx::textdatum::top_center);
    }
}

/* Painted row: header or command. 4 headers + CMD_N commands. */
static int menu_rows(void)
{
    return CMD_N + 4;
}

static int menu_sel_row(int cmd)
{
    int row = 0;
    for (int g = 0; g < 4; ++g) {
        ++row;
        if (cmd < G_AT[g + 1]) return row + (cmd - G_AT[g]);
        row += G_AT[g + 1] - G_AT[g];
    }
    return 0;
}

static void menu_row(int row, int *hdr, int *cmd)
{
    int acc = 0;
    for (int g = 0; g < 4; ++g) {
        if (row == acc) {
            *hdr = g;
            *cmd = -1;
            return;
        }
        ++acc;
        const int n = G_AT[g + 1] - G_AT[g];
        if (row < acc + n) {
            *hdr = -1;
            *cmd = G_AT[g] + (row - acc);
            return;
        }
        acc += n;
    }
    *hdr = -1;
    *cmd = 0;
}

static const int PROMPT_Y = 227;
static void blit_ico(Gfx g, const uint8_t *bits, int x, int y, uint8_t col);

static bool sheet_exact(void)
{
    return s_screen == SCR_TERMINAL && s_sheet;
}

static uint8_t term_accent(void)
{
    /* Sheet chrome is magenta. Everything else follows the UI accent,
     * so an amber theme never falls through to PAL_CYAN or code 237. */
    if (!sheet_exact()) return accent();
    return PAL_TERM_M;
}

static uint8_t term_low(void)
{
    if (!sheet_exact()) {
        if (s_color == 1) return PAL_WASH_A;
        if (s_color == 2) return PAL_WASH_M;
        return PAL_WASH_C;
    }
    return PAL_TERM_M10;
}

/* Selected sheet name and purpose. A light tint of the live accent. */
static uint8_t term_ink(void)
{
    if (s_color == 1) return PAL_TERM_INK_A;
    if (s_color == 2) return PAL_TERM_INK_M;
    return PAL_TERM_INK_C;
}

/* Sheet-open steps are the 13 exact codes. Otherwise the reserved UI slots. */
static uint8_t glow_of(int step)
{
    if (!sheet_exact()) {
        if (step == STEP_SHADOW) return PAL_BG;
        if (s_color == 1) {
            if (step == STEP_HAIR || step == STEP_GLOW) return PAL_WIRE_A;
            if (step == STEP_AC) return PAL_AMBER;
            return PAL_WASH_A;
        }
        if (s_color == 2) {
            if (step == STEP_HAIR || step == STEP_GLOW) return PAL_WIRE_M;
            if (step == STEP_AC) return PAL_MINT;
            return PAL_WASH_M;
        }
        if (step == STEP_HAIR || step == STEP_GLOW) return PAL_WIRE_C;
        if (step == STEP_AC) return PAL_CYAN;
        return PAL_WASH_C;
    }
    const uint8_t *row = TERM_MAG;
    (void)TERM_CYN;
    switch (step) {
    case STEP_SHADOW: return PAL_BG;
    case STEP_HAIR: return row[1];
    case STEP_RULE: return row[2];
    case STEP_SEL: return row[3];
    case STEP_HUNT: return row[2];
    case STEP_GLOW: return row[4];
    default: return row[0];
    }
}

/* 3px halo: .10, then .18, then a 2px .35 copy, then the mark. */
static void glow_hline(Gfx g, int x, int y, int w, int step)
{
    if (w >= 6) {
        g.fillRect(x + 3, y + 3, w, 1, term_low());
        g.fillRect(x + 2, y + 2, w, 1, glow_of(STEP_SEL));
        g.fillRect(x + 1, y + 1, w, 2, glow_of(STEP_GLOW));
    }
    g.fillRect(x, y, w, 1, glow_of(step));
}

static void glow_rect(Gfx g, int x, int y, int w, int h)
{
    g.drawRect(x + 3, y + 3, w, h, term_low());
    g.drawRect(x + 2, y + 2, w, h, glow_of(STEP_SEL));
    g.drawRect(x + 1, y + 1, w, h, glow_of(STEP_GLOW));
    g.drawRect(x + 2, y + 2, w - 2, h - 2, glow_of(STEP_GLOW));
    g.drawRect(x, y, w, h, glow_of(STEP_AC));
}

static void glow_txt(Gfx g, const char *s, int x, int y, uint8_t bg, int px)
{
    const int keep = s_px;
    s_px = px;
    const uint8_t ring[4] = { term_low(), glow_of(STEP_HAIR), glow_of(STEP_GLOW), glow_of(STEP_AC) };
    for (int r = 4; r >= 1; --r) {
        const uint8_t c = ring[4 - r];
        const int inner = (r - 1) * (r - 1);
        const int outer = r * r;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                const int d2 = dx * dx + dy * dy;
                if (d2 > outer || d2 <= inner) continue;
                txt_bg(g, s, x + dx, y + dy, c, bg, lgfx::textdatum::top_left);
            }
        }
    }
    txt_bg(g, s, x + 1, y, glow_of(STEP_AC), bg, lgfx::textdatum::top_left);
    txt_bg(g, s, x, y, glow_of(STEP_AC), bg, lgfx::textdatum::top_left);
    s_px = keep;
}

/* Screen frame follows the UI accent. Halo is wash, then wire, then the accent. */
static void frame_rect(Gfx g, int x, int y, int w, int h)
{
    const uint8_t low = (s_color == 1) ? (uint8_t)PAL_WASH_A
                        : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
    const uint8_t mid = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                        : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
    g.drawRect(x - 3, y - 3, w + 6, h + 6, low);
    g.drawRect(x - 2, y - 2, w + 4, h + 4, mid);
    g.drawRect(x - 1, y - 1, w + 2, h + 2, accent());
    g.drawRect(x, y, w, h, accent());
}

/* r=3 rounded stroke. Corners step in so the sheet matches the mock. */
static void stroke_round(Gfx g, int x, int y, int w, int h, uint8_t c)
{
    g.fillRect(x + 3, y, w - 6, 1, c);
    g.fillRect(x + 3, y + h - 1, w - 6, 1, c);
    g.fillRect(x, y + 3, 1, h - 6, c);
    g.fillRect(x + w - 1, y + 3, 1, h - 6, c);
    g.fillRect(x + 2, y + 1, 1, 1, c);
    g.fillRect(x + 1, y + 2, 1, 1, c);
    g.fillRect(x + w - 3, y + 1, 1, 1, c);
    g.fillRect(x + w - 2, y + 2, 1, 1, c);
    g.fillRect(x + 2, y + h - 2, 1, 1, c);
    g.fillRect(x + 1, y + h - 3, 1, 1, c);
    g.fillRect(x + w - 3, y + h - 2, 1, 1, c);
    g.fillRect(x + w - 2, y + h - 3, 1, 1, c);
}

/* Tab sheet frame is magenta from the EXT-only table, in every UI accent. */
static void frame_mag(Gfx g, int x, int y, int w, int h)
{
    stroke_round(g, x - 4, y - 4, w + 8, h + 8, PAL_TERM_M10);
    stroke_round(g, x - 3, y - 3, w + 6, h + 6, PAL_TERM_M10);
    stroke_round(g, x - 2, y - 2, w + 4, h + 4, PAL_TERM_M22);
    stroke_round(g, x - 1, y - 1, w + 2, h + 2, PAL_TERM_M35);
    stroke_round(g, x, y, w, h, PAL_TERM_M);
}

static void paint_term_icons(Gfx g)
{
    const uint8_t ac = term_accent();
    const uint8_t off = PAL_LINE;
    g.fillRect(205, 1, 20, 14, PAL_BG);
    g.drawRect(205, 1, 20, 14, glow_of(STEP_HAIR));
    static const char *crown[7] = {
        "#.....#.....#", "##...###...##", "###.#####.###", "#############",
        "#############", ".###########.", ".###########."
    };
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (crown[r][c] != '#') continue;
            g.fillRect(208 + c, 4 + r, 1, 1, r >= 5 ? (uint8_t)PAL_CHIN : (uint8_t)PAL_IVORY);
        }
    }
    const int sg = 3 + (int)((millis() / 2500u) % 2u);
    for (int k = 0; k < 4; ++k) {
        const int h = (k + 1) * 2;
        g.fillRect(236 + k * 4, 12 - h, 3, h, k < sg ? ac : off);
    }
    g.fillRect(262, 4, 17, 9, PAL_MUTED);
    g.fillRect(263, 5, 15, 7, PAL_BG);
    g.fillRect(279, 6, 2, 5, PAL_MUTED);
    const bool blink = (millis() / 500u) % 2u == 0u;
    for (int k = 0; k < 5; ++k) {
        uint8_t cell = PAL_BG;
        if (k < 4 || blink) cell = k < 2 ? (uint8_t)PAL_AMBER : (uint8_t)PAL_OK;
        g.fillRect(264 + k * 3, 6, 2, 5, cell);
    }
    const bool rx = (millis() / 700u) % 5u == 0u;
    g.fillRect(290, 6, 4, 4, rx ? (uint8_t)PAL_R_E2FF : off);
    const int keep = s_px;
    s_px = 8;
    txt(g, "RX", 298, 4, rx ? ac : (uint8_t)COL_MUTED, lgfx::textdatum::top_left);
    s_px = keep;
}

static void paint_term_head(Gfx g)
{
    g.fillRect(0, 0, 320, 16, PAL_BG);
    {
        const uint8_t low = (s_color == 1) ? (uint8_t)PAL_WASH_A
                            : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
        const uint8_t mid = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                            : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        g.fillRect(0, 16, 320, 2, accent());
        g.fillRect(0, 18, 320, 1, mid);
    }
    glow_txt(g, "terminal", 8, 4, PAL_BG, 8);
    const int keep = s_px;
    s_px = 6;
    const uint8_t boards_wash = (s_color == 1) ? (uint8_t)PAL_WASH_A
                                 : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
    txt(g, "12 boards", 79, 5, boards_wash, lgfx::textdatum::top_left);
    txt(g, "12 boards", 78, 4, accent(), lgfx::textdatum::top_left);
    if (s_track_name[0]) txt(g, s_track_name, 156, 4, term_accent(), lgfx::textdatum::top_left);
    s_px = keep;
    paint_term_icons(g);
}

static int purpose_px(const char *s)
{
    int n = 0;
    for (const char *p = s; *p; ++p) n += (*p == '\x7f') ? 4 : s_px;
    return n;
}

static void draw_purpose(Gfx g, const char *s, int x_right, int y, uint8_t col, uint8_t bg)
{
    int x = x_right - purpose_px(s);
    char one[2] = { 0, 0 };
    for (const char *p = s; *p; ++p) {
        if (*p == '\x7f') {
            g.fillRect(x + 1, y + 3, 1, 1, col);
            x += 4;
            continue;
        }
        one[0] = *p;
        txt_bg(g, one, x, y, col, bg, lgfx::textdatum::top_left);
        x += 6;
    }
}

static void sheet_box(int *by, int *h, int *top_out, int *nshow_out)
{
    const int vis = 9;
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    const int sel_row = menu_sel_row(sel);
    int top = sel_row - 3;
    if (top < 0) top = 0;
    if (top > rows - vis) top = rows - vis;
    if (top < 0) top = 0;
    int nshow = rows - top;
    if (nshow > vis) nshow = vis;
    const int hh = nshow * 12 + 10;
    int y = PROMPT_Y - 5 - hh;
    if (y < 18) y = 18;
    *by = y;
    *h = hh;
    *top_out = top;
    *nshow_out = nshow;
}

/* Prompt-anchored Tab dropdown. Grouped CORE/COMMS/GAME/SYS. */
static void paint_sheet(Gfx g)
{
    const int keep = s_px;
    s_px = 6;
    const int rh = 12;
    const int w = 292;
    const int bx = 14;
    int by = 0;
    int h = 0;
    int top = 0;
    int nshow = 0;
    sheet_box(&by, &h, &top, &nshow);
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    /* Darken the boards under the sheet, then a dither so they stay visible. */
    for (int i = 0; i < 12; ++i) {
        int tx = 0;
        int ty = 0;
        term_xy(i, &tx, &ty);
        if (tx + 72 <= bx || ty + 46 <= by || tx >= bx + w || ty >= by + h) continue;
        paint_one_board(g, i, (i == s_term) && !s_focus_cmds, true);
    }
    if (by + h > 168) {
        const int y0 = by > 168 ? by : 168;
        g.fillRect(bx + 1, y0, w - 2, by + h - y0, PAL_DEEP);
    }
    frame_mag(g, bx, by, w, h);
    glow_hline(g, bx + 6, by + h - 3, w - 12, STEP_HAIR);
    if (top > 0) blit_ico(g, ARROW_UP, bx + w - 12, by + 2, COL_MUTED);
    if (top + nshow < rows) blit_ico(g, ARROW_DN, bx + w - 12, by + h - 11, COL_MUTED);
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        const int y = by + 5 + r * rh;
        if (hdr >= 0) {
            glow_txt(g, G_NAME[hdr], bx + 8, y + 1, PAL_DEEP, 6);
            glow_hline(g, bx + 48, y + 5, w - 60, STEP_RULE);
            continue;
        }
        const bool on = cmd == sel;
        const uint8_t bg = on ? glow_of(STEP_SEL) : (uint8_t)PAL_DEEP;
        g.fillRect(bx + 3, y - 1, w - 6, rh - 1, bg);
        if (on) g.fillRect(bx + 3, y - 1, 2, rh - 1, glow_of(STEP_AC));
        const uint8_t ac = accent();
        blit_ico(g, CMD_ICO[cmd], bx + 10, y + 1, on ? ac : (uint8_t)PAL_MUTED);
        const uint8_t ink = term_ink();
        const uint8_t name_c = on ? ink : (uint8_t)PAL_TERM_INK;
        if (on) txt_bg(g, CMDN[cmd], bx + 25, y + 1, name_c, bg, lgfx::textdatum::top_left);
        txt_bg(g, CMDN[cmd], bx + 24, y + 1, name_c, bg, lgfx::textdatum::top_left);
        draw_purpose(g, CMDP[cmd], bx + w - 8, y + 1, on ? ink : ac, bg);
    }
    s_px = keep;
}

static const char *cand_ghost(void)
{
    const char *buf = botcorps_t0_term_prompt;
    if (!buf[0] || strchr(buf, ' ')) return 0;
    const size_t n = strlen(buf);
    for (int i = 0; i < PCMD_N; ++i) {
        if (strncmp(PCMD[i], buf, n) == 0 && PCMD[i][n]) return PCMD[i] + n;
    }
    return 0;
}

static int opt_list(const char *cmd, const char *arg, const char **out, int maxn)
{
    const char *rain_o[] = { "green", "cyan", "blue", "purple", "amber", "white", "mixed" };
    const char *scan_o[] = { "wifi", "ble", "all" };
    const char *hunt_o[] = { "drone", "flock", "skimmer", "deauth" };
    const char *filt_o[] = { "wifi", "ble", "threats", "strong", "off" };
    const char *go_o[] = { "terminal", "control", "bots", "wifi", "ble", "scan" };
    const char *load_o[] = { "wifilist", "blelist", "gibson" };
    const char **src = 0;
    int nsrc = 0;
    if (strcmp(cmd, "rain") == 0) { src = rain_o; nsrc = 7; }
    else if (strcmp(cmd, "scan") == 0) { src = scan_o; nsrc = 3; }
    else if (strcmp(cmd, "hunt") == 0) { src = hunt_o; nsrc = 4; }
    else if (strcmp(cmd, "filter") == 0) { src = filt_o; nsrc = 5; }
    else if (strcmp(cmd, "go") == 0) { src = go_o; nsrc = 6; }
    else if (strcmp(cmd, "load") == 0) { src = load_o; nsrc = 3; }
    int n = 0;
    const size_t an = strlen(arg);
    for (int i = 0; src && i < nsrc && n < maxn; ++i) {
        if (strncmp(src[i], arg, an) == 0) out[n++] = src[i];
    }
    return n;
}

/* Mock tmSubDraw: command and option list beside the prompt. Strings stay in flash. */
static void paint_cands(Gfx g)
{
    const char *buf = botcorps_t0_term_prompt;
    if (!buf[0] || s_sheet || s_focus_cmds) return;
    const char *sp = strchr(buf, ' ');
    char cmd[16];
    const char *arg = "";
    cmd[0] = 0;
    if (sp) {
        int n = (int)(sp - buf);
        if (n > 15) n = 15;
        memcpy(cmd, buf, (size_t)n);
        cmd[n] = 0;
        arg = sp + 1;
    }
    const char *rows[8];
    int idx[8];
    int n = 0;
    if (!sp) {
        const size_t bn = strlen(buf);
        for (int i = 0; i < PCMD_N && n < 8; ++i) {
            if (strncmp(PCMD[i], buf, bn) == 0) {
                rows[n] = PCMD[i];
                idx[n] = i;
                ++n;
            }
        }
    } else {
        n = opt_list(cmd, arg, rows, 8);
        for (int i = 0; i < n; ++i) idx[i] = -1;
    }
    if (n <= 0) return;
    const int vis = n > 6 ? 6 : n;
    int sel = 0;
    if (!sp && s_tab_i >= 0 && s_tab_i < n) sel = s_tab_i;
    int start = sel - 2;
    if (start < 0) start = 0;
    if (start > n - vis) start = n - vis;
    int w = 40;
    for (int i = 0; i < vis; ++i) {
        int ww = (int)strlen(rows[start + i]) * 6 + 14;
        if (ww > w) w = ww;
    }
    if (sp) {
        const int hw = (int)strlen(cmd) * 6 + 22;
        if (hw > w) w = hw;
    }
    w += 6;
    if (w > 220) w = 220;
    const int rh = 12;
    const int head = sp ? 10 : 0;
    const int h = vis * rh + 6 + head;
    const int py = PROMPT_Y;
    int bx = 10 + (18 + (int)strlen(buf)) * 6 + 14;
    if (bx + w > 314) bx = 314 - w;
    if (bx < 6) bx = 6;
    int by = py - 4 - h;
    if (by < 122) by = 122;
    plate(g, bx, by, w, h, COL_BG, accent());
    int y = by + 4;
    if (sp) {
        txt_bg(g, cmd, bx + 6, y - 1, accent(), COL_BG, lgfx::textdatum::top_left);
        txt_bg(g, ">", bx + 8 + (int)strlen(cmd) * 6, y - 1, COL_MUTED, COL_BG, lgfx::textdatum::top_left);
        y += 10;
    }
    for (int k = 0; k < vis; ++k) {
        const int on = (start + k) == sel;
        const uint8_t bg = on ? (uint8_t)COL_SEL : (uint8_t)COL_BG;
        if (on) {
            g.fillRect(bx + 2, y - 1, w - 4, rh - 1, COL_SEL);
            g.fillRect(bx + 2, y - 1, 2, rh - 1, accent());
        }
        txt_bg(g, rows[start + k], bx + 6, y, on ? COL_WHITE : COL_INK, bg, lgfx::textdatum::top_left);
        y += rh;
    }
}

static void paint_prompt(Gfx g)
{
    const int keep = s_px;
    s_px = 8;
    /* Tiles end at y=165. The plate starts below them. */
    g.fillRect(4, 168, 312, 68, COL_DEEP);
    g.fillRect(4, 168, 312, 1, accent());
    g.fillRect(4, 235, 312, 1, COL_LINE);
    g.fillRect(4, 168, 1, 68, COL_LINE);
    g.fillRect(315, 168, 1, 68, COL_LINE);
    const int n = s_log_n < 6 ? s_log_n : 6;
    const int start = s_log_n - n;
    for (int k = 0; k < n; ++k) {
        const char *ln = log_line(start + k);
        uint8_t ink = COL_INK;
        const char *show = ln;
        if (ln && ln[0] == '\x01') {
            const int b = (unsigned char)ln[1];
            ink = b >= 12 ? (uint8_t)COL_IVORY : bot_color(b);
            show = ln + 2;
        }
        txt_bg(g, show, 10, 171 + k * 9, ink, PAL_DEEP, lgfx::textdatum::top_left);
    }
    const int y = PROMPT_Y;
    glow_txt(g, "user@botcorps", 10, y, COL_DEEP, 6);
    s_px = 6;
    txt(g, ":~$", 10 + 13 * 6, y, accent(), lgfx::textdatum::top_left);
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(COL_WHITE);
    g.setCursor(10 + 16 * 6, y);
    g.print(botcorps_t0_term_prompt);
    const int cx = g.getCursorX() + 2;
    const char *ghost = cand_ghost();
    if (ghost) {
        g.setTextColor(COL_MUTED);
        g.print(ghost);
    }
    if ((millis() / 530u) % 2u == 0) g.fillRect(cx, y, 6, 8, term_accent());
    paint_cands(g);
    const bool po = s_focus_cmds && !s_sheet;
    plate(g, 250, 214, 20, 12, po ? COL_SEL : COL_DEEP, po ? accent() : COL_LINE);
    for (int k = 0; k < 3; ++k) g.fillRect(255, 217 + k * 3, 10, 1, po ? term_accent() : COL_MUTED);
    if (po) paint_palette(g);
    s_px = keep;
}

static void paint_terminal(Gfx g)
{
    paint_term_head(g);
    paint_boards(g);
    paint_prompt(g);
    if (s_sheet) paint_sheet(g);
}

static void paint_home(Gfx g, int bot_index)
{
    const bot_t &b = BOTS[bot_index];
    const int phase = (int)((millis() / 40u) % 14u);
    g.fillCircle(160, 74, 48, COL_DEEP);
    g.drawCircle(160, 74, 46, accent());
    g.drawCircle(160, 74, 53, COL_LINE);
    g.drawCircle(160, 74, 47 + phase, accent());
    draw_blob(g, 160, 74, 4, bot_color(bot_index), botcorps_shape_for(bot_index), sprite_blink(bot_index));
    const int ids[4] = { ICO_RADAR, ICO_TERM, ICO_CTRL, ICO_BACK };
    const int xs[4] = { 12, 44, 248, 280 };
    for (int i = 0; i < 4; ++i) draw_btn(g, xs[i], 133, ids[i], s_cursor == i);
    if (s_screen == SCR_BOT) {
        g.fillTriangle(8, 74, 20, 66, 20, 82, COL_MUTED);
        g.fillTriangle(312, 74, 300, 66, 300, 82, COL_MUTED);
    }
}

static void hub_gibson(Gfx g, int cx, int cy, int z);
static void chip_plate(Gfx g, int x, int y, int w, int h, uint8_t fill, uint8_t line);

/* Mock busLayout. kind 1 = radar (lanes stop at cx+/-108). kind 0 = full scan. */
struct bus_pt { int16_t x, y; };
static const bus_pt BUS_R[12][4] = {
    { { 129, 101 }, { 125, 101 }, { 91, 67 }, { 52, 67 } },
    { { 129, 117 }, { 52, 117 }, { 0, 0 }, { 0, 0 } },
    { { 129, 133 }, { 125, 133 }, { 91, 167 }, { 52, 167 } },
    { { 191, 101 }, { 195, 101 }, { 229, 67 }, { 268, 67 } },
    { { 191, 117 }, { 268, 117 }, { 0, 0 }, { 0, 0 } },
    { { 191, 133 }, { 195, 133 }, { 229, 167 }, { 268, 167 } },
    { { 140, 86 }, { 140, 32 }, { 0, 0 }, { 0, 0 } },
    { { 160, 86 }, { 160, 32 }, { 0, 0 }, { 0, 0 } },
    { { 180, 86 }, { 180, 32 }, { 0, 0 }, { 0, 0 } },
    { { 140, 148 }, { 140, 202 }, { 0, 0 }, { 0, 0 } },
    { { 160, 148 }, { 160, 202 }, { 0, 0 }, { 0, 0 } },
    { { 180, 148 }, { 180, 202 }, { 0, 0 }, { 0, 0 } },
};
static const bus_pt BUS_F[12][4] = {
    { { 129, 101 }, { 125, 101 }, { 91, 67 }, { 14, 67 } },
    { { 129, 117 }, { 14, 117 }, { 0, 0 }, { 0, 0 } },
    { { 129, 133 }, { 125, 133 }, { 91, 167 }, { 14, 167 } },
    { { 191, 101 }, { 195, 101 }, { 229, 67 }, { 306, 67 } },
    { { 191, 117 }, { 306, 117 }, { 0, 0 }, { 0, 0 } },
    { { 191, 133 }, { 195, 133 }, { 229, 167 }, { 306, 167 } },
    { { 140, 86 }, { 140, 32 }, { 0, 0 }, { 0, 0 } },
    { { 160, 86 }, { 160, 32 }, { 0, 0 }, { 0, 0 } },
    { { 180, 86 }, { 180, 32 }, { 0, 0 }, { 0, 0 } },
    { { 140, 148 }, { 140, 202 }, { 0, 0 }, { 0, 0 } },
    { { 160, 148 }, { 160, 202 }, { 0, 0 }, { 0, 0 } },
    { { 180, 148 }, { 180, 202 }, { 0, 0 }, { 0, 0 } },
};
static const int8_t BUS_N[12] = { 4, 2, 4, 4, 2, 4, 2, 2, 2, 2, 2, 2 };
static const int8_t BUS_DEV_LANE[3] = { 1, 6, 9 };

static int bus_len(const bus_pt *p, int n)
{
    int L = 0;
    for (int i = 1; i < n; ++i) {
        const int dx = p[i].x - p[i - 1].x;
        const int dy = p[i].y - p[i - 1].y;
        const int adx = dx < 0 ? -dx : dx;
        const int ady = dy < 0 ? -dy : dy;
        L += adx > ady ? adx : ady;
    }
    return L;
}

static void bus_at(const bus_pt *p, int n, int dist, int *x, int *y)
{
    int left = dist;
    if (left < 0) left = 0;
    for (int i = 1; i < n; ++i) {
        const int dx = p[i].x - p[i - 1].x;
        const int dy = p[i].y - p[i - 1].y;
        const int adx = dx < 0 ? -dx : dx;
        const int ady = dy < 0 ? -dy : dy;
        int len = adx > ady ? adx : ady;
        if (len < 1) len = 1;
        if (left <= len || i == n - 1) {
            int k = left;
            if (k > len) k = len;
            *x = p[i - 1].x + dx * k / len;
            *y = p[i - 1].y + dy * k / len;
            return;
        }
        left -= len;
    }
    *x = p[0].x;
    *y = p[0].y;
}

static void bus_trace(Gfx g, const bus_pt *p, int n, uint8_t col, uint8_t glow)
{
    const int L = bus_len(p, n);
    const int step = glow ? 1 : 2;
    for (int s = 0; s <= L; s += step) {
        int x = 0;
        int y = 0;
        bus_at(p, n, s, &x, &y);
        if (glow) g.fillRect(x + 1, y + 1, 1, 1, glow);
        g.fillRect(x, y, 1, 1, col);
    }
}

static int bus_dev_s(int L)
{
    int s = 28 + (L > 42 ? (L - 42) / 2 : 0);
    if (s < 27) s = 27;
    if (s > L - 11) s = L - 11;
    if (s < 8) s = 8;
    return s;
}

static int bus_scout_s(int L, uint32_t t, int lane, int period)
{
    if (period < 2) period = 2;
    const uint32_t phase = (t / 16u + (uint32_t)lane * 37u) % (uint32_t)period;
    const int half = period / 2;
    const int u = (int)phase < half ? (int)phase : period - (int)phase;
    int span = L - 16;
    if (span < 1) span = 1;
    return 8 + span * u / half;
}

/* Ten device icons from the v3.2 set. Colours are existing slots. */
enum {
    DEV_SKIM = 0, DEV_FLOCK, DEV_DRONE, DEV_TAG, DEV_CAM, DEV_DEAUTH,
    DEV_AP, DEV_BT, DEV_TV, DEV_UNK, DEV_N
};
static const char *const DEV_ROW[DEV_N][12] = {
    { "............", ".aaaaaaaaaa.", "aaaaaaaaaaaa", "bbbbbbbbbbbb", "bbbbbbbbbbbb", "aaaaaaaaaaaa", "acccaaaaaaaa", "acccaabbbbaa", "acccaaaaaaaa", "aaaaaaaaaaaa", ".aaaaaaaaaa.", "............" },
    { "aa...aa.....", ".aa.aa......", "..aca.......", "............", ".....aa...aa", "......aa.aa.", ".......aca..", "............", "aa...aa.....", ".aa.aa......", "..aca.......", "............" },
    { "ccc......ccc", "ccc......ccc", ".a........a.", "..a.aaaa.a..", "...aabbaa...", "...abccba...", "...aabbaa...", "..a.aaaa.a..", ".a........a.", "ccc......ccc", "ccc......ccc", "............" },
    { "............", "...aaaaaa...", "..aaaaaaaa..", ".aaabbbbaaa.", ".aabbccbbaa.", ".aabcbbcbaa.", ".aabcbbcbaa.", ".aabbccbbaa.", ".aaabbbbaaa.", "..aaaaaaaa..", "...aaaaaa...", "............" },
    { "............", "...aaaa.....", "aaaaaaaaaaaa", "abbbbbbbbbca", "abbbcccbbbba", "abbcbbbcbbba", "abbcbbbcbbba", "abbbcccbbbba", "abbbbbbbbbba", "aaaaaaaaaaaa", "............", "............" },
    { "...aaaaaa...", ".aa......aa.", "a..aaaaaa..a", "..aa....aa..", "...a.aa.a...", ".....aa.....", ".....aa.....", "........c..c", ".........cc.", ".........cc.", "........c..c", "............" },
    { "...cccccc...", ".cc......cc.", "c..cccccc..c", "..cc....cc..", "...c.cc.c...", ".....cc.....", "......c.....", "..aaaaaaaa..", ".abbbbbbbba.", ".abcbbbbcba.", ".aaaaaaaaaa.", "............" },
    { "...bbbbbb...", "..bbbaabbb..", ".bbbbababbb.", ".bbababbabb.", ".bbbaababbb.", ".bbbbcabbbb.", ".bbbbcabbbb.", ".bbbaababbb.", ".bbababbabb.", ".bbbbababbb.", "..bbbaabbb..", "...bbbbbb..." },
    { "............", "aaaaaaaaaaaa", "abbbbbbbbbba", "abbbbbbbbbba", "abbcccccbbba", "abbbbbbbcbba", "abcccbbbcbba", "abbbbcbbcbba", "abc.bcbbcbba", "aaaaaaaaaaaa", "....aaaa....", "...aaaaaa..." },
    { "............", "...cccccc...", "..cc....cc..", "..cc....cc..", "......ccc...", ".....cc.....", ".....cc.....", "............", ".....cc.....", ".....cc.....", "............", "............" },
};
static const uint8_t DEV_COL[DEV_N][3] = {
    { PAL_BFC6B, PAL_WASH_A, PAL_IVORY },
    { PAL_B9F0B, PAL_BOARD_D, PAL_HEAD },
    { PAL_BFB72, PAL_WASH_A, PAL_IVORY },
    { PAL_IVORY, PAL_DIAG, PAL_R_FDC9 },
    { PAL_R_E2FF, 33, PAL_INK },
    { PAL_BAC7F, 33, PAL_INK },
    { PAL_B36B8, PAL_WASH_M, PAL_INK },
    { PAL_B4E7F, PAL_SEL, PAL_INK },
    { PAL_B5D3F, PAL_SEL, PAL_INK },
    { 150, PAL_SEL, PAL_WHITE },
};
static const uint8_t NEAR_WIFI_T[3] = { DEV_AP, DEV_AP, DEV_AP };
static const uint8_t NEAR_BLE_T[3] = { DEV_BT, DEV_BT, DEV_BT };
/* Twelve nodes, one per angled spoke. Existing palette slots only. */
static const uint8_t SCAN_TYPE[12] = {
    DEV_AP, DEV_BT, DEV_TV, DEV_CAM, DEV_DEAUTH, DEV_TAG,
    DEV_DRONE, DEV_FLOCK, DEV_SKIM, DEV_BT, DEV_CAM, DEV_TAG
};
static const int SCAN_N = 12;

static void dev_icon(Gfx g, int type, int x, int y, bool dim)
{
    if (type < 0 || type >= DEV_N) type = DEV_UNK;
    const uint8_t *col = DEV_COL[type];
    for (int r = 0; r < 12; ++r) {
        const char *row = DEV_ROW[type][r];
        for (int c = 0; c < 12; ++c) {
            const char ch = row[c];
            if (ch != 'a' && ch != 'b' && ch != 'c') continue;
            uint8_t ink = col[ch - 'a'];
            if (dim) ink = PAL_LINE;
            g.fillRect(x + c, y + r, 1, 1, ink);
        }
    }
}

/* Non-circular bus. radar=false is the Gibson scan (extBus). 0 B heap. */
static void bus_paint(Gfx g, bool radar, bool wifi, int shown, int sel)
{
    const bus_pt (*tab)[4] = radar ? BUS_R : BUS_F;
    const int period = radar ? 3600 : 7000;
    const uint32_t t = millis();
    const int bx = radar ? 50 : 8;
    const int by = 26;
    const int bw = radar ? 220 : 304;
    const int bh = 182;
    g.fillRect(bx, by, bw, bh, PAL_DEEP);
    g.drawRect(bx - 1, by - 1, bw + 2, bh + 2, COL_LINE);
    const uint8_t ac = accent();
    /* Scan spokes stay dim (PAL_LINE 41) with a wash glow (PAL_WASH_C 1). */
    const uint8_t spoke = radar ? (uint8_t)COL_LINE : (uint8_t)PAL_LINE;
    const uint8_t spoke_glow = radar ? (uint8_t)0 : (uint8_t)PAL_WASH_C;
    for (int i = 0; i < 12; ++i) bus_trace(g, tab[i], BUS_N[i], spoke, spoke_glow);
    if (radar) {
        const int nnode = shown < 3 ? shown : 3;
        for (int d = 0; d < nnode; ++d) {
            const int lane = BUS_DEV_LANE[d];
            const int L = bus_len(tab[lane], BUS_N[lane]);
            int x = 0;
            int y = 0;
            const int typ = wifi ? NEAR_WIFI_T[d] : NEAR_BLE_T[d];
            bus_at(tab[lane], BUS_N[lane], bus_dev_s(L), &x, &y);
            dev_icon(g, typ, x - 6, y - 6, false);
            if (d == sel) g.drawRect(x - 7, y - 7, 14, 14, ac);
        }
    } else {
        for (int d = 0; d < SCAN_N; ++d) {
            const int lane = d;
            const int L = bus_len(tab[lane], BUS_N[lane]);
            int at = L - 10;
            if (at < 8) at = 8;
            int x = 0;
            int y = 0;
            bus_at(tab[lane], BUS_N[lane], at, &x, &y);
            const uint8_t node = DEV_COL[SCAN_TYPE[d]][0];
            g.drawCircle(x, y, 10, PAL_WASH_C);
            g.drawCircle(x, y, 8, PAL_WIRE_C);
            g.drawCircle(x, y, 6, node);
            dev_icon(g, SCAN_TYPE[d], x - 6, y - 6, false);
        }
    }
    chip_plate(g, 160 - 28, 117 - 28, 56, 56, PAL_BG, COL_LINE);
    hub_gibson(g, 160, 117, 3);
    if ((t - s_bus_t0) % (uint32_t)period < 400u) {
        g.drawRect(160 - 28, 117 - 28, 56, 56, ac);
    }
    for (int i = 0; i < 12; ++i) {
        const int L = bus_len(tab[i], BUS_N[i]);
        int x = 0;
        int y = 0;
        bus_at(tab[i], BUS_N[i], bus_scout_s(L, t, i, period), &x, &y);
        draw_blob(g, x, y, 1, bot_color(i), botcorps_shape_for(i), false);
    }
}

static void bus_label(Gfx g, bool wifi, int shown, int sel)
{
    const char *const *names = wifi ? NEAR_WIFI : NEAR_BLE;
    const char *const *rssi = wifi ? NEAR_WIFI_R : NEAR_BLE_R;
    char lab[28];
    if (shown <= 0 || sel < 0 || sel >= shown) snprintf(lab, sizeof(lab), "NO DEVICE YET");
    else snprintf(lab, sizeof(lab), "%s %s dBm", names[sel], rssi[sel]);
    const int tw = (int)strlen(lab) * 6 + 16;
    int x = 160 - tw / 2;
    if (x < 8) x = 8;
    plate(g, x, 216, tw, 16, COL_BG, shown > 0 ? COL_AMBER : COL_LINE);
    txt_bg(g, lab, x + tw / 2, 220, shown > 0 ? COL_AMBER : COL_MUTED, COL_BG,
           lgfx::textdatum::top_center);
}

static void paint_radar(Gfx g, bool with_list)
{
    const bool wifi = s_screen != SCR_BT_RADAR;
    const uint32_t t = millis();
    int shown = 0;
    if (t > s_bus_t0 + 1700u) shown = 1 + (int)((t - s_bus_t0 - 1700u) / 1100u);
    if (shown > 3) shown = 3;
    bus_paint(g, true, wifi, shown, shown > 0 ? shown - 1 : -1);
    bus_label(g, wifi, shown, shown > 0 ? shown - 1 : -1);
    if (with_list) {
        draw_btn(g, 14, 72, ICO_TRACK, s_cursor == 0);
        draw_btn(g, 14, 106, ICO_LIST, s_cursor == 1);
        draw_btn(g, 14, 140, ICO_BACK, s_cursor == 2);
    } else {
        draw_btn(g, 14, 90, ICO_TRACK, s_cursor == 0);
        draw_btn(g, 14, 124, ICO_BACK, s_cursor == 1);
    }
    if (s_track && shown > 0) {
        const bus_pt (*tab)[4] = BUS_R;
        const int lane = BUS_DEV_LANE[shown - 1];
        int x = 0;
        int y = 0;
        bus_at(tab[lane], BUS_N[lane], bus_dev_s(bus_len(tab[lane], BUS_N[lane])), &x, &y);
        g.drawRect(x - 10, y - 10, 20, 20, COL_AMBER);
    }
}

static void paint_scan_hud(Gfx g, bool wifi, int sel)
{
    const char *const *names = wifi ? NEAR_WIFI : NEAR_BLE;
    const char *const *rssi = wifi ? NEAR_WIFI_R : NEAR_BLE_R;
    const uint8_t *bars = wifi ? NEAR_WIFI_B : NEAR_BLE_B;
    const uint8_t ac = accent();
    const uint32_t u = (millis() - s_bus_t0) % 7000u;
    g.fillRect(0, 0, 320, 24, PAL_BG);
    g.fillRect(0, 24, 320, 1, COL_LINE);
    g.fillRect(0, 24, (int)(320u * u / 7000u), 1, ac);
    hub_gibson(g, 14, 12, 1);
    const int keep = s_px;
    s_px = 8;
    txt(g, "GIBSON", 28, 8, ac, lgfx::textdatum::top_left);
    s_px = keep;
    g.drawCircle(214, 12, 5, COL_MUTED);
    g.fillCircle(214, 12, 2, COL_MUTED);
    g.setFont(&fonts::Font8x8C64);
    g.setTextSize(2);
    g.setTextColor(COL_INK);
    g.setCursor(226, 2);
    g.print("12");
    g.setTextSize(1);
    g.drawCircle(268, 12, 5, COL_MUTED);
    g.drawLine(268, 8, 268, 16, COL_MUTED);
    g.drawLine(264, 12, 272, 12, COL_MUTED);
    g.fillRect(0, 209, 320, 31, PAL_BG);
    g.fillRect(0, 209, 320, 1, COL_LINE);
    int tally[DEV_N];
    for (int i = 0; i < DEV_N; ++i) tally[i] = 0;
    for (int d = 0; d < SCAN_N; ++d) tally[SCAN_TYPE[d]]++;
    const uint8_t *types = wifi ? NEAR_WIFI_T : NEAR_BLE_T;
    if (sel >= 0 && sel < 3) dev_icon(g, types[sel], 8, 216, false);
    for (int k = 0; k < 4; ++k) {
        const int bh = (k + 1) * 3;
        const int lit = (sel >= 0 && sel < 3) ? bars[sel] : 0;
        g.fillRect(26 + k * 5, 227 - bh, 3, bh, k < lit ? ac : COL_LINE);
    }
    s_px = 6;
    for (int i = 0; i < DEV_N; ++i) {
        const int x = 52 + i * 26;
        dev_icon(g, i, x, 214, tally[i] == 0);
        char num[4];
        snprintf(num, sizeof(num), "%d", tally[i]);
        txt(g, num, x + 13, 218, tally[i] ? (uint8_t)COL_INK : (uint8_t)COL_MUTED,
            lgfx::textdatum::top_left);
    }
    s_px = keep;
    (void)names;
    (void)rssi;
}

static void blit_ico(Gfx g, const uint8_t *bits, int x, int y, uint8_t col)
{
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (bits[r] & (uint8_t)(1u << (7 - c))) g.fillRect(x + c, y + r, 1, 1, col);
        }
    }
}

static int list_band(void)
{
    return s_screen == SCR_BT_LIST ? 1 : 0;
}

static void act_origin(int dev, int *mx, int *my)
{
    const int rh = 17;
    const int y0 = 32;
    if (dev < 0) dev = 0;
    const int ry = y0 + dev * rh;
    const int mh = ACT_N * 15 + 6;
    int y = ry + rh;
    if (y + mh > 204) y = ry - mh - 1;
    if (y < 28) y = 28;
    *mx = 214;
    *my = y;
}

static void paint_act_menu(Gfx g)
{
    if (s_act < 0 || s_act_dev < 0) return;
    const int keep = s_px;
    s_px = 8;
    int mx = 0;
    int my = 0;
    act_origin(s_act_dev, &mx, &my);
    const int mh = ACT_N * 15 + 6;
    g.fillRect(mx - 2, my - 2, 96, mh + 4, glow_of(STEP_SHADOW));
    plate(g, mx, my, 92, mh, COL_BG, glow_of(STEP_AC));
    const int band = list_band();
    for (int j = 0; j < ACT_N; ++j) {
        const int yy = my + 3 + j * 15;
        const bool on = j == s_act;
        const uint8_t bg = on ? glow_of(STEP_HUNT) : (uint8_t)COL_BG;
        if (on) {
            g.fillRect(mx + 2, yy, 88, 14, bg);
            g.fillRect(mx + 2, yy, 2, 14, glow_of(STEP_AC));
        }
        blit_ico(g, ACT_ICO[j], mx + 8, yy + 3, on ? COL_WHITE : accent());
        const char *lab = ACT_NAME[j];
        if (j == 1 && s_act_dev >= 0 && s_dev_fol[band][s_act_dev]) lab = "unfollow";
        txt_bg(g, lab, mx + 21, yy + 2, on ? COL_WHITE : COL_INK, bg,
               lgfx::textdatum::top_left);
        if (j == 2 && s_act_dev >= 0 && s_dev_tag[band][s_act_dev] > 0) {
            g.fillRect(mx + 80, yy + 4, 6, 6, TAG_COL[s_dev_tag[band][s_act_dev] - 1]);
        }
        if (j == ACT_N - 1) g.fillRect(mx + 6, yy - 1, 80, 1, COL_LINE);
    }
    s_px = keep;
}

static void paint_info_card(Gfx g, bool wifi)
{
    if (s_info_dev < 0 || s_info_dev > 2) return;
    const char *const *names = wifi ? NEAR_WIFI : NEAR_BLE;
    const char *const *rssi = wifi ? NEAR_WIFI_R : NEAR_BLE_R;
    const uint8_t *bars = wifi ? NEAR_WIFI_B : NEAR_BLE_B;
    const int d = s_info_dev;
    g.fillRect(0, 0, 320, 240, PAL_BLACK);
    plate(g, 24, 40, 272, 138, COL_BG, accent());
    g.fillRect(34, 52, 44, 44, PAL_DEEP);
    g.fillRect(40, 64, 12, 12, accent());
    txt_bg(g, names[d], 92, 52, COL_WHITE, COL_BG, lgfx::textdatum::top_left);
    txt_bg(g, rssi[d], 92, 106, COL_INK, COL_BG, lgfx::textdatum::top_left);
    txt_bg(g, "dBm", 92 + (int)strlen(rssi[d]) * 6 + 4, 112, COL_MUTED, COL_BG,
           lgfx::textdatum::top_left);
    for (int k = 0; k < 4; ++k) {
        const int bh = (k + 1) * 4;
        g.fillRect(250 + k * 8, 120 - bh, 5, bh, k < bars[d] ? accent() : COL_LINE);
    }
    const int band = wifi ? 0 : 1;
    if (s_dev_tag[band][d] > 0) g.fillRect(262, 54, 8, 8, TAG_COL[s_dev_tag[band][d] - 1]);
}

static const int LIST_N = 8;
static const char *const LIST_WIFI_N[LIST_N] = {
    "cafe-guest", "printer-2f", "lobby-ap", "tp-link-mesh",
    "roku-ultra", "ring-doorbell", "esp32-marauder", "hp-laptop"
};
static const char *const LIST_WIFI_M[LIST_N] = {
    "a4:c1:38:01:4e:20", "b2:11:90:4c:aa:17", "c8:3a:35:71:0d:42", "e8:48:b8:22:6f:09",
    "d0:3f:27:61:b4:88", "b0:c5:54:63:1a:7e", "24:6f:28:58:c3:11", "3c:22:fb:74:90:5d"
};
static const char *const LIST_WIFI_DB[LIST_N] = {
    "-42", "-67", "-71", "-55", "-61", "-63", "-58", "-74"
};
static const int LIST_WIFI_CH[LIST_N] = { 6, 11, 1, 6, 44, 1, 6, 11 };
static const uint8_t LIST_WIFI_BAR[LIST_N] = { 4, 2, 1, 3, 2, 2, 3, 1 };
static const uint8_t LIST_WIFI_TY[LIST_N] = {
    DEV_AP, DEV_AP, DEV_AP, DEV_AP, DEV_TV, DEV_CAM, DEV_DEAUTH, DEV_AP
};
static const char *const LIST_BLE_N[LIST_N] = {
    "earbuds-58", "watch-6", "speaker-7", "airtag-3f",
    "hc-05-atm", "meta-glasses", "dji-mini", "flock-raven"
};
static const char *const LIST_BLE_M[LIST_N] = {
    "41:20:88:58:10:3a", "c2:44:10:06:9b:21", "7a:19:b2:07:44:80", "d4:8a:3f:3f:12:6c",
    "00:18:e4:05:77:01", "ac:de:48:69:3e:55", "60:60:1f:60:ab:08", "f0:72:ea:72:14:9d"
};
static const char *const LIST_BLE_DB[LIST_N] = {
    "-58", "-63", "-74", "-52", "-66", "-69", "-60", "-72"
};
static const int LIST_BLE_CH[LIST_N] = { 37, 38, 39, 37, 38, 39, 37, 38 };
static const uint8_t LIST_BLE_BAR[LIST_N] = { 3, 2, 1, 4, 2, 2, 3, 1 };
static const uint8_t LIST_BLE_TY[LIST_N] = {
    DEV_BT, DEV_BT, DEV_BT, DEV_TAG, DEV_SKIM, DEV_CAM, DEV_DRONE, DEV_FLOCK
};

static void paint_dev_list(Gfx g, bool wifi)
{
    const char *const *names = wifi ? LIST_WIFI_N : LIST_BLE_N;
    const char *const *macs = wifi ? LIST_WIFI_M : LIST_BLE_M;
    const char *const *rssi = wifi ? LIST_WIFI_DB : LIST_BLE_DB;
    const int *ch = wifi ? LIST_WIFI_CH : LIST_BLE_CH;
    const uint8_t *bars = wifi ? LIST_WIFI_BAR : LIST_BLE_BAR;
    const uint8_t *types = wifi ? LIST_WIFI_TY : LIST_BLE_TY;
    const int sel = (s_cursor >= 0 && s_cursor < LIST_N) ? s_cursor : 0;
    const uint8_t ac = accent();
    const int y0 = 32;
    const int rh = 17;
    g.fillRect(8, 8, 304, 19, PAL_BG);
    g.fillRect(8, 8, 304, 1, ac);
    g.fillRect(8, 26, 304, 1, COL_LINE);
    dev_icon(g, wifi ? DEV_AP : DEV_BT, 13, 11, false);
    const int keep = s_px;
    s_px = 8;
    txt(g, wifi ? "WI-FI" : "BLUETOOTH", 32, 12, COL_INK, lgfx::textdatum::top_left);
    s_px = 6;
    char cnt[8];
    snprintf(cnt, sizeof(cnt), "%d/%d", LIST_N, LIST_N);
    txt(g, cnt, 268, 13, COL_MUTED, lgfx::textdatum::top_left);
    for (int d = 0; d < LIST_N; ++d) {
        const int y = y0 + d * rh;
        const bool on = d == sel;
        const uint8_t dimbar = (s_color == 1) ? (uint8_t)PAL_WASH_A
                               : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
        g.fillRect(8, y, 304, rh - 1, on ? dimbar : (uint8_t)((d & 1) ? PAL_BG : PAL_FIELD));
        dev_icon(g, types[d], 13, y + 2, false);
        txt(g, names[d], 28, y + 4, on ? (uint8_t)COL_WHITE : (uint8_t)COL_INK, lgfx::textdatum::top_left);
        txt(g, macs[d], 116, y + 4, COL_MUTED, lgfx::textdatum::top_left);
        txt(g, rssi[d], 222, y + 4, ac, lgfx::textdatum::top_left);
        char chs[8];
        snprintf(chs, sizeof(chs), "ch%02d", ch[d]);
        txt(g, chs, 246, y + 4, COL_MUTED, lgfx::textdatum::top_left);
        for (int k = 0; k < 4; ++k) {
            const int bh = (k + 1) * 2;
            g.fillRect(286 + k * 4, y + 12 - bh, 3, bh, k < bars[d] ? ac : COL_LINE);
        }
    }
    const int cy = y0 + LIST_N * rh;
    txt(g, ">", 14, cy + 4, ac, lgfx::textdatum::top_left);
    if ((millis() / 420u) % 2u == 0u) g.fillRect(24, cy + 3, 5, 8, ac);
    s_px = keep;
}

static void __attribute__((unused)) paint_bus_scan(Gfx g, bool wifi)
{
    bus_paint(g, false, wifi, 12, -1);
    paint_scan_hud(g, wifi, -1);
}

static void paint_list(Gfx g, bool wifi)
{
    paint_dev_list(g, wifi);
    const int bx = (320 - BS) / 2;
    plate(g, bx, 206, BS, BS, COL_DEEP, accent());
    const uint8_t acb = accent();
    const int ax = bx + 8;
    const int ay = 216;
    g.fillRect(ax + 6, ay + 3, 10, 2, acb);
    g.fillRect(ax + 2, ay + 3, 2, 2, acb);
    g.fillRect(ax + 3, ay + 1, 2, 2, acb);
    g.fillRect(ax + 3, ay + 5, 2, 2, acb);
    g.fillRect(ax + 4, ay, 2, 2, acb);
    g.fillRect(ax + 4, ay + 6, 2, 2, acb);
    paint_act_menu(g);
    paint_info_card(g, wifi);
}

static const char *const SHAPE_N[5] = { "MIXED", "ROUND", "CLASSIC", "TALL", "MINI" };

static void cp_text(int index, char *value, int n)
{
    if (index == 0) snprintf(value, (size_t)n, "%s", ACCENT_N[s_color]);
    else if (index == 1) snprintf(value, (size_t)n, "%s", MIX_N[s_mix]);
    else if (index == 2) snprintf(value, (size_t)n, "%s", SHAPE_N[s_shape]);
    else if (index == 3) snprintf(value, (size_t)n, "%s", botcorps_rain_mode() == 0 ? "MIXED" : "SOLID");
    else if (index == 4) snprintf(value, (size_t)n, "%s", RAIN_WORD[botcorps_rain_mode()]);
    else if (index == 5) snprintf(value, (size_t)n, "%s", SAVER_N[s_saver]);
    else if (index == 6) snprintf(value, (size_t)n, "%s", s_saver_on ? "ON" : "OFF");
    else snprintf(value, (size_t)n, "%d SEC", IDLE_V[s_idle]);
}

static const int SPR_W[4] = { 7, 9, 7, 5 };
static const int SPR_H[4] = { 7, 6, 9, 5 };

static void shape_preview(Gfx g, int px, int cy)
{
    const int mode = botcorps_rain_mode();
    const uint8_t live = mode > 0 ? botcorps_rain_solid() : botcorps_rain_palette(0);
    if (s_shape <= 0) {
        int sx = px - 19;
        for (int id = 0; id < 4; ++id) {
            botcorps_sprite_draw(*g.raw(), sx + SPR_W[id] / 2 - g.ox, cy - g.oy, id, 1, live, false);
            sx += SPR_W[id] + 3;
        }
        return;
    }
    const int id = s_shape - 1;
    botcorps_sprite_draw(*g.raw(), px - g.ox, cy - g.oy, id, 2, live, false);
}

static void saver_preview(Gfx g, int px, int cy)
{
    const int x0 = px - 6;
    const int y0 = cy - 6;
    const int mode = botcorps_rain_mode();
    const uint8_t ic = mode > 0 ? botcorps_rain_solid() : botcorps_rain_palette(0);
    if (s_saver == 0) {
        static const int dot[6][2] = { { 1, 1 }, { 5, 5 }, { 9, 2 }, { 3, 8 }, { 7, 0 }, { 10, 8 } };
        for (int k = 0; k < 6; ++k) g.fillRect(x0 + dot[k][0], y0 + dot[k][1], 2, 3, ic);
    } else if (s_saver == 1) {
        for (int k = 0; k <= 12; k += 4) {
            g.fillRect(x0 + k, y0, 1, 13, PAL_DEEP);
            g.fillRect(x0, y0 + k, 13, 1, PAL_DEEP);
        }
        g.fillRect(x0 + 2, y0 + 2, 4, 4, ic);
        g.fillRect(x0 + 8, y0 + 8, 4, 4, botcorps_rain_palette(5));
    } else if (s_saver == 2) {
        g.fillRect(x0 + 5, y0 + 5, 1, 1, PAL_LINE);
        g.fillRect(x0 + 1, y0 + 1, 2, 2, ic);
        g.fillRect(x0 + 4, y0 + 4, 3, 3, ic);
        g.fillRect(x0 + 8, y0 + 8, 5, 5, ic);
    } else {
        g.fillRect(x0, y0, 13, 1, PAL_LINE);
        g.fillRect(x0, y0 + 12, 13, 1, PAL_LINE);
        g.fillRect(x0, y0, 1, 13, PAL_LINE);
        g.fillRect(x0 + 12, y0, 1, 13, PAL_LINE);
        g.fillRect(x0 + 2, y0 + 7, 3, 3, ic);
        g.fillRect(x0 + 6, y0 + 3, 4, 4, ic);
    }
}

static void paint_cp_row(Gfx g, int index, bool on)
{
    const char *labels[8] = {
        "UI COLOR", "BACKGROUND", "BOT SHAPE", "RAIN MODE",
        "RAIN COLOUR", "SCREEN SAVER", "SAVER", "IDLE TIME"
    };
    const int y = 4 + index * 20;
    const bool preview = index == 0 || index == 2 || index == 3 || index == 4 || index == 5;
    const uint8_t row = on ? (uint8_t)COL_SEL : (uint8_t)COL_DEEP;
    plate(g, 8, y, 304, 18, row, on ? accent() : COL_LINE);
    txt_bg(g, labels[index], 16, y + 5, on ? accent() : COL_MUTED, row, lgfx::textdatum::top_left);
    char value[16];
    cp_text(index, value, (int)sizeof(value));
    const int vr = preview ? 250 : 298;
    txt_bg(g, value, vr, y + 5, on ? COL_INK : PAL_SOFT, row, lgfx::textdatum::top_right);
    if (on) {
        txt_bg(g, "<", vr - (int)strlen(value) * 6 - 8, y + 5, accent(), row, lgfx::textdatum::top_right);
        txt_bg(g, ">", vr + 8, y + 5, accent(), row, lgfx::textdatum::top_left);
    }
    const int px = 286;
    const int cy = y + 9;
    if (index == 0) g.fillCircle(px, cy, 5, accent());
    else if (index == 2) shape_preview(g, px, cy);
    else if (index == 3) {
        if (botcorps_rain_mode() <= 0) {
            static const int sw[4] = { 0, 2, 3, 1 };
            for (int k = 0; k < 4; ++k) {
                g.fillRect(px - 6 + (k % 2) * 6, cy - 6 + (k / 2) * 6, 6, 6, botcorps_rain_palette(sw[k]));
            }
        } else {
            g.fillRect(px - 6, cy - 6, 12, 12, botcorps_rain_solid());
        }
    } else if (index == 4) {
        const int mode = botcorps_rain_mode();
        g.fillRect(px - 6, cy - 6, 12, 12, mode <= 0 ? botcorps_rain_palette(0) : botcorps_rain_solid());
    } else if (index == 5) saver_preview(g, px, cy);
}

static void paint_control(Gfx g)
{
    for (int i = 0; i < CP_N; ++i) paint_cp_row(g, i, s_cursor == i);
    char tag[40];
    const int mode = botcorps_rain_mode();
    const bool solid = mode > 0;
    if (solid) snprintf(tag, sizeof(tag), "LIVE RAIN  SOLID %s  %s", RAIN_WORD[mode], SHAPE_N[s_shape]);
    else snprintf(tag, sizeof(tag), "LIVE RAIN  MIXED  %s", SHAPE_N[s_shape]);
    const int tw = (int)strlen(tag) * 6 + 10;
    const int wy = 188;
    plate(g, 14, wy + 4, tw, 13, COL_DEEP, COL_LINE);
    txt(g, tag, 19, wy + 7, solid ? botcorps_rain_solid() : COL_INK, lgfx::textdatum::top_left);
    draw_btn(g, 14, 204, ICO_TERM, s_cursor == CP_N);
    draw_btn(g, 48, 204, ICO_BACK, s_cursor == CP_N + 1);
}

static const char *GIB_CROWN[7] = {
    "#.....#.....#", "##...###...##", "###.#####.###", "#############",
    "#############", ".###########.", ".###########."
};

static void px_ring(Gfx g, int cx, int cy, int r, uint8_t col, int on, int off, int phase)
{
    const int n = (int)((float)r * 6.3f);
    if (n < 1) return;
    for (int k = 0; k < n; ++k) {
        if (on > 0 && (((k / on + phase) % 2) != 0)) continue;
        (void)off;
        const float a = (float)k / (float)r;
        const int x = cx + (int)(cosf(a) * (float)r);
        const int y = cy + (int)(sinf(a) * (float)r);
        g.fillRect(x, y, 1, 1, col);
    }
}

static void gib_crown(Gfx g, int cx, int y)
{
    const int x0 = cx - 13;
    g.fillRect(x0 - 4, y - 3, 13 * 2 + 8, 7 * 2 + 5, PAL_DEEP);
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (GIB_CROWN[r][c] != '#') continue;
            g.fillRect(x0 + c * 2, y + r * 2, 2, 2, r >= 5 ? PAL_MUTED : PAL_IVORY);
        }
    }
}

static uint8_t wire_ink(void)
{
    if (s_color == 1) return PAL_WIRE_A;
    if (s_color == 2) return PAL_WIRE_M;
    return PAL_WIRE_C;
}

static void chip_plate(Gfx g, int x, int y, int w, int h, uint8_t fill, uint8_t line)
{
    if (w < 3 || h < 3) return;
    g.fillRect(x + 1, y, w - 2, h, fill);
    g.fillRect(x, y + 1, w, h - 2, fill);
    g.fillRect(x + 1, y, w - 2, 1, line);
    g.fillRect(x + 1, y + h - 1, w - 2, 1, line);
    g.fillRect(x, y + 1, 1, h - 2, line);
    g.fillRect(x + w - 1, y + 1, 1, h - 2, line);
}

static void via_node(Gfx g, int x, int y, uint8_t col)
{
    g.fillRect(x, y, 3, 3, col);
    g.fillRect(x + 1, y + 1, 1, 1, PAL_FIELD);
}

/* Inner disc under the hub rings. Integer traces and vias, no buffer. */
static void hub_die(Gfx g, int cx, int cy, int r)
{
    static const int8_t via[8][2] = {
        { -16, -6 }, { 16, -6 }, { -16, 6 }, { 16, 6 },
        { -6, -16 }, { 6, -16 }, { -6, 16 }, { 6, 16 }
    };
    g.fillCircle(cx, cy, r, PAL_DEEP);
    for (int k = 10; k < r; k += 8) g.drawCircle(cx, cy, k, (k & 8) ? (uint8_t)PAL_LINE : wire_ink());
    g.fillRect(cx - r + 8, cy, (r - 8) * 2, 1, PAL_LINE);
    g.fillRect(cx, cy - r + 8, 1, (r - 8) * 2, PAL_LINE);
    for (int i = 0; i < 8; ++i) {
        const int x = cx + via[i][0];
        const int y = cy + via[i][1];
        g.fillRect(x - 1, y - 1, 3, 3, PAL_STEEL);
        g.fillRect(x, y, 1, 1, PAL_IVORY);
    }
}

static void hub_gibson(Gfx g, int cx, int cy, int z)
{
    const int ox = cx - (7 * z) / 2;
    const int oy = cy - (7 * z) / 2;
    auto u = [&](int x, int y, int w, int h, uint8_t col) {
        g.fillRect(ox + x * z, oy + y * z, w * z, h * z, col);
    };
    g.fillRect(ox + z / 2, oy + 7 * z, 6 * z, z > 2 ? 2 : z, PAL_BLACK);
    u(3, 0, 1, 1, PAL_IVORY);
    u(1, 1, 5, 1, PAL_IVORY);
    u(0, 2, 7, 3, PAL_IVORY);
    u(0, 5, 7, 1, PAL_IVORY);
    u(1, 6, 5, 1, PAL_CHIN);
    u(0, 5, 1, 1, PAL_CHIN);
    u(6, 5, 1, 1, PAL_CHIN);
    u(0, 3, 7, 1, PAL_BLACK);
    u(1, 3, 2, 2, PAL_BLACK);
    u(4, 3, 2, 2, PAL_BLACK);
    const int gl = (int)((millis() / 900u) % 2u);
    u(1 + gl, 3, 1, 1, PAL_STEEL);
    u(4 + gl, 3, 1, 1, PAL_STEEL);
}

static int hub_mood(uint32_t t, bool *asleep)
{
    if (s_last_key == 0) s_last_key = t;
    *asleep = (uint32_t)(t - s_last_key) > 20000u;
    if (*asleep) return MOOD_SLEEPY;
    if (s_mood_until != 0 && (int32_t)(t - s_mood_until) < 0) return s_mood;
    return MOOD_NEUTRAL;
}

static uint8_t dot_col(int i, uint32_t t, int mood, bool asleep)
{
    const int st = BOTS[i].st;
    const bool blink = ((t / 700u + (uint32_t)i) % 4u) != 0u;
    uint8_t base = PAL_DEEP;
    if (st == 2 || blink) base = st == 2 ? PAL_HOLD : st == 1 ? PAL_AMBER : PAL_OK;
    if (asleep || mood == MOOD_SLEEPY) {
        return ((t / 1400u + (uint32_t)i) % 3u) == 0u ? PAL_OK : PAL_DEEP;
    }
    if (mood == MOOD_ALERT) return ((t / 160u) % 2u) ? PAL_AMBER : PAL_HOLD;
    if (mood == MOOD_HAPPY) return accent();
    if (mood == MOOD_EXCITED) return ((t / 110u + (uint32_t)i) % 2u) ? PAL_WHITE : accent();
    if (mood == MOOD_HUNTING) return ((t / 110u) % 12u) == (uint32_t)i ? PAL_WHITE : base;
    if (mood == MOOD_SAD) return st == 2 ? PAL_HOLD : PAL_DEEP;
    return base;
}

static void hub_sparks(Gfx g, int cx, int cy, uint32_t t)
{
    const uint8_t ac = accent();
    for (int i = 0; i < 8; ++i) {
        const uint32_t period = 900u + (uint32_t)i * 70u;
        const uint32_t age = (t + (uint32_t)i * 130u) % period;
        const float life = (float)age / (float)period;
        if (life > 0.85f) continue;
        const float a = (float)i * 0.8f + (float)t / 400.f;
        const float rad = 22.f + life * 18.f;
        const int x = cx + (int)(cosf(a) * rad);
        const int y = cy + (int)(sinf(a) * rad * 0.9f) - (int)(life * 16.f);
        const uint8_t ink = (i & 1) ? (uint8_t)PAL_IVORY : ac;
        g.fillRect(x, y, 1 + (i & 1), 1, ink);
    }
}

static uint16_t mix16(uint16_t a, uint16_t b, int num, int den)
{
    const int inv = den - num;
    auto ch = [](uint16_t p, int *r, int *g, int *bl) {
        const int r5 = (p >> 11) & 31;
        const int g6 = (p >> 5) & 63;
        const int b5 = p & 31;
        *r = (r5 << 3) | (r5 >> 2);
        *g = (g6 << 2) | (g6 >> 4);
        *bl = (b5 << 3) | (b5 >> 2);
    };
    int ar, ag, ab, br, bg, bb;
    ch(a, &ar, &ag, &ab);
    ch(b, &br, &bg, &bb);
    const int r = (ar * inv + br * num + den / 2) / den;
    const int g = (ag * inv + bg * num + den / 2) / den;
    const int bl = (ab * inv + bb * num + den / 2) / den;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((bl & 0xFF) >> 3));
}

/* Exact 0.22 wash over the field. One slot per hub accent. */
static uint8_t wash22(uint8_t accent_idx)
{
    if (accent_idx == PAL_AMBER) return PAL_WASH_A;
    if (accent_idx == PAL_MINT) return PAL_WASH_M;
    return PAL_WASH_C;
}

static uint8_t half_over_deep(uint8_t fg)
{
    return botcorps_pal_fit(mix16(botcorps_rgb565(PAL_DEEP), botcorps_rgb565(fg), 50, 100));
}

static void draw_heart(Gfx g, int x, int y, uint8_t col)
{
    g.fillRect(x + 1, y, 2, 1, col);
    g.fillRect(x + 4, y, 2, 1, col);
    g.fillRect(x, y + 1, 6, 2, col);
    g.fillRect(x + 1, y + 3, 4, 1, col);
    g.fillRect(x + 2, y + 4, 2, 1, col);
    g.fillRect(x + 3, y + 5, 1, 1, col);
}

static void draw_filled_pip(Gfx g, int x, int y, uint8_t col)
{
    g.fillCircle(x + 3, y + 3, 3, col);
}

static void draw_fisheye(Gfx g, int x, int y, uint8_t col)
{
    g.drawCircle(x + 3, y + 3, 3, col);
    g.fillCircle(x + 3, y + 3, 1, col);
}

static const char *pip_glyph(int mood)
{
    if (mood == MOOD_SLEEPY) return "Z";
    if (mood == MOOD_SAD) return "?";
    if (mood == MOOD_ALERT) return "!";
    return "*";
}

static const char *const GP_CMD[] = {
    "terminal", "scan", "wifi", "ble", "control", "bots", "go", "load",
    "hunt", "help", "ping", "info", "track", "sniff", "filter", "rain", "clear"
};

static const char *gp_ghost(void)
{
    const int n = (int)strlen(s_gp_buf);
    if (n <= 0) return nullptr;
    for (int i = 0; i < (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0])); ++i) {
        if (strncmp(GP_CMD[i], s_gp_buf, (size_t)n) == 0 && GP_CMD[i][n] != 0)
            return GP_CMD[i] + n;
    }
    return nullptr;
}

static void gp_push(const char *s)
{
    if (s_gp_n >= 2) {
        memcpy(s_gp_log[0], s_gp_log[1], sizeof(s_gp_log[0]));
        --s_gp_n;
    }
    snprintf(s_gp_log[s_gp_n], sizeof(s_gp_log[0]), "%s", s ? s : "");
    ++s_gp_n;
}

static void gp_strip(Gfx g, uint32_t t, uint8_t ac)
{
    const uint8_t edge = s_gp_focus ? ac : (uint8_t)COL_LINE;
    chip_plate(g, 38, 164, 244, 38, PAL_DEEP, edge);
    if (s_gp_focus) g.drawRect(39, 165, 242, 36, ac);
    const uint8_t hair = botcorps_pal_fit(mix16(botcorps_rgb565(PAL_DEEP), botcorps_rgb565(ac),
                                                s_gp_focus ? 60 : 25, 100));
    g.fillRect(41, 165, 238, 1, hair);
    for (int k = 0; k < s_gp_n; ++k) {
        const int y = 164 + 5 + (2 - s_gp_n + k) * 11;
        const bool dim = s_gp_n == 2 && k == 0;
        txt(g, s_gp_log[k], 45, y, dim ? half_over_deep(COL_INK) : (uint8_t)COL_INK,
            lgfx::textdatum::top_left);
    }
    txt(g, "gibson", 45, 191, ac, lgfx::textdatum::top_left);
    txt(g, ">", 45 + 6 * 6, 191, COL_INK, lgfx::textdatum::top_left);
    const int bx = 45 + 8 * 6;
    char gp_box[25];
    snprintf(gp_box, sizeof(gp_box), "%-24.24s", s_gp_buf);
    txt_bg(g, gp_box, bx, 191, COL_INK, PAL_DEEP, lgfx::textdatum::top_left);
    const int after = bx + (int)strlen(s_gp_buf) * 6;
    const char *ghost = gp_ghost();
    if (ghost && s_gp_focus) {
        txt(g, ghost, after + 6, 191, botcorps_pal_fit(0x4AAC), lgfx::textdatum::top_left);
    }
    const bool caret = s_gp_focus ? (s_gp_buf[0] || (t / 530u) % 2u == 0u)
                                  : (t / 700u) % 2u == 0u;
    if (caret) g.fillRect(after + 1, 191, 5, 8, s_gp_focus ? ac : hair);
}

/* chip_plate leaves these four pixels empty. The partial clip would keep them stale. */
static void gp_strip_corners(Gfx g)
{
    g.fillRect(38, 164, 1, 1, PAL_DEEP);
    g.fillRect(281, 164, 1, 1, PAL_DEEP);
    g.fillRect(38, 201, 1, 1, PAL_DEEP);
    g.fillRect(281, 201, 1, 1, PAL_DEEP);
}

static void partial_gp_strip(void)
{
    if (s_screen != SCR_GIBSON || !botcorps_dual_ok()) return;
    if (!botcorps_band_direct()) {
        botcorps_band_patch([](Gfx g) {
            gp_strip(g, millis(), accent());
            gp_strip_corners(g);
        }, 38, 164, 244, 38);
        return;
    }
    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    auto &p = botcorps_content();
    p.startWrite();
    p.setClipRect(38, 164, 244, 38);
    gp_strip(Gfx(p), millis(), accent());
    gp_strip_corners(Gfx(p));
    p.endWrite();
    p.clearClipRect();
}

static void paint_gibson(Gfx g)
{
    /* extGibsonHub. Rain is dimmed by the fade table before this chrome. No read-back. */
    const int cx = 160;
    const int cy = 100;
    const int rr = 44;
    const uint32_t t = millis();
    const uint8_t ac = accent();
    const uint8_t wire = wire_ink();
    bool asleep = false;
    const int mood = hub_mood(t, &asleep);
    static const int stars[10][2] = {
        { 60, 40 }, { 88, 64 }, { 116, 34 }, { 214, 36 }, { 240, 62 },
        { 262, 40 }, { 74, 178 }, { 250, 176 }, { 130, 176 }, { 196, 178 }
    };
    for (int k = 0; k < 10; ++k) {
        const int on = (int)((t / 420u + (uint32_t)k * 3u) % 5u);
        const uint8_t sc = on == 0 ? PAL_WHITE : on == 1 ? PAL_LINE : PAL_IVORY;
        g.fillRect(stars[k][0], stars[k][1], 1, 1, sc);
    }
    for (int i = 0; i < 12; ++i) {
        const bool left = i < 6;
        const int sx = left ? 6 : 286;
        const int sy = 22 + (i % 6) * 27;
        const int ys = sy + 10;
        const int ddy = ys - cy;
        const bool out = ddy > rr - 6 || ddy < -(rr - 6);
        int ex = 26;
        if (!out) ex = (int)sqrtf((float)(rr * rr - ddy * ddy));
        const int x1 = left ? sx + 28 : cx + ex + 1;
        const int x2 = left ? cx - ex : sx;
        const int len = x2 - x1;
        int ry = cy;
        if (ex < rr) {
            const int drop = (int)sqrtf((float)(rr * rr - ex * ex));
            ry = cy + (ddy < 0 ? -drop : drop);
        }
        if (len > 0) g.fillRect(x1, ys, len, 1, wire);
        if (out) {
            const int y0 = ys < ry ? ys : ry;
            g.fillRect(left ? x2 : x1 - 1, y0, 1, (ys > ry ? ys - ry : ry - ys) + 1, wire);
        }
        const int travel = len > 0 ? len : 1;
        const int d = (int)((t / 33u + (uint32_t)i * 11u) % (uint32_t)(travel + 10));
        if (d < travel) g.fillRect((left ? x1 + d : x2 - d) - 1, ys - 1, 2, 3, ac);
        via_node(g, left ? x2 - 1 : x1 - 2, (out ? ry : ys) - 1, ac);
        chip_plate(g, sx, sy, 28, 20, PAL_DEEP, COL_LINE);
        draw_blob(g, sx + 11, sy + 10, 1, bot_color(i), botcorps_shape_for(i), false);
        g.fillRect(sx + 21, sy + 7, 4, 4, dot_col(i, t, mood, asleep));
    }
    hub_die(g, cx, cy, 34);
    const uint8_t rc1 = mood == MOOD_ALERT ? (((t / 160u) % 2u) ? PAL_AMBER : PAL_HOLD)
                        : mood == MOOD_SAD ? PAL_LINE : ac;
    px_ring(g, cx, cy, 34, rc1, 0, 0, 0);
    const int spin = mood == MOOD_HUNTING ? (int)(t / 30u) : mood == MOOD_EXCITED ? (int)(t / 50u) : (int)(t / 90u);
    px_ring(g, cx, cy, 37, mood == MOOD_ALERT ? PAL_AMBER : mood == MOOD_SAD ? PAL_LINE : ac, 5, 5, spin);
    if (mood == MOOD_HUNTING) {
        for (int k = 0; k < 14; ++k) {
            const float ang = (float)t / 260.f - (float)k * 0.07f;
            const uint8_t ink = k < 3 ? (uint8_t)PAL_WHITE : ac;
            g.fillRect(cx + (int)(cosf(ang) * 40.f), cy + (int)(sinf(ang) * 40.f), 1, 1, ink);
        }
    }
    px_ring(g, cx, cy, 42, COL_LINE, 2, 4, (int)(t / 140u));
    const int ph = (int)((t % 2400u) * 5u / 2400u);
    px_ring(g, cx, cy, 44 + ph, wire, 3, 3, 0);
    hub_sparks(g, cx, cy, t);
    g.fillRect(cx - 14, cy + 25, 28, 1, wire);
    hub_gibson(g, cx, cy, 9);
    if (asleep) {
        for (int k = 0; k < 3; ++k) {
            const int q = (int)((t / 1500u + (uint32_t)k) % 3u);
            txt(g, "z", cx + 26 + k * 5, cy - 22 - q * 6, COL_IVORY, lgfx::textdatum::top_left);
        }
    }
    if (s_bub[0] && s_mood_until != 0 && (int32_t)(t - s_mood_until) < 0) {
        const int bw = (int)strlen(s_bub) * 6 + 10;
        const int bx = cx + 34;
        chip_plate(g, bx, 24, bw, 11, PAL_DEEP, ac);
        txt(g, s_bub, bx + 5, 25, COL_INK, lgfx::textdatum::top_left);
        g.fillRect(bx + 6, 35, 3, 2, ac);
    }
    gib_crown(g, cx, 22);
    chip_plate(g, cx - 15, cy + 40, 30, 12, PAL_DEEP, ac);
    g.fillRect(cx - 6, cy + 43, 12, 6, ac);
    g.fillRect(cx - 4, cy + 44, 8, 4, PAL_BG);
    for (int k = 0; k < 4; ++k) {
        g.fillRect(cx - 5 + k * 4, cy + 42, 1, 1, ac);
        g.fillRect(cx - 5 + k * 4, cy + 49, 1, 1, ac);
    }
    const float pu = (sinf((float)t / 380.f) + 1.f) * 0.5f;
    const uint8_t led = pu > 0.5f ? (uint8_t)PAL_WHITE : ac;
    g.fillRect(cx + 8, cy + 43, 2, 2, led);
    static const int pip_m[7] = {
        MOOD_NEUTRAL, MOOD_HAPPY, MOOD_EXCITED, MOOD_HUNTING, MOOD_SLEEPY, MOOD_SAD, MOOD_ALERT
    };
    int hx = (320 - (7 * 17 - 6)) / 2;
    const int shown = asleep ? MOOD_SLEEPY : mood;
    const uint8_t wash = wash22(ac);
    for (int i = 0; i < 7; ++i) {
        const bool on = pip_m[i] == shown;
        if (on) {
            g.fillRect(hx - 3, 154, 11, 11, wash);
            g.fillRect(hx - 3, 164, 11, 1, ac);
        }
        const uint8_t pc = on ? (uint8_t)PAL_WHITE : (uint8_t)PAL_LINE;
        if (pip_m[i] == MOOD_HAPPY) draw_heart(g, hx, 156, pc);
        else if (pip_m[i] == MOOD_NEUTRAL) draw_filled_pip(g, hx, 156, pc);
        else if (pip_m[i] == MOOD_HUNTING) draw_fisheye(g, hx, 156, pc);
        else txt(g, pip_m[i] == MOOD_EXCITED ? "*" : pip_glyph(pip_m[i]), hx, 156, pc,
                 lgfx::textdatum::top_left);
        hx += 17;
    }
    gp_strip(g, t, ac);
    const int ids[3] = { ICO_TERM, ICO_CTRL, ICO_RADAR };
    const int xs[3] = { 110, 146, 182 };
    for (int i = 0; i < 3; ++i) draw_btn(g, xs[i], 206, ids[i], s_cursor == i);
}

static void paint_rain_btn(Gfx g)
{
    draw_btn(g, 146, 206, ICO_PLAY, true);
}

static void paint_diag(Gfx g)
{
    const botcorps_probe_t *p = botcorps_probe_get();
    struct spec {
        int id;
        const char *name;
        int kind;
    };
    static const spec rows[] = {
        { BOTCORPS_ST_M0, "M0", 0 },
        { BOTCORPS_ST_M5, "M5", 0 },
        { BOTCORPS_ST_E0, "E0", 0 },
        { BOTCORPS_ST_EXT, "EXT", 1 },
        { BOTCORPS_ST_T0, "T0", 2 },
        { BOTCORPS_ST_A0, "A0", 0 },
        { BOTCORPS_ST_A1, "A1", 3 },
        { BOTCORPS_ST_SC0, "S0", 0 },
        { BOTCORPS_ST_SC1, "S1", 4 },
        { BOTCORPS_ST_B0, "B0", 0 },
        { BOTCORPS_ST_B1, "B1", 5 },
        { BOTCORPS_ST_F0, "F0", 0 },
        { BOTCORPS_ST_SF, "SF", 6 },
        { BOTCORPS_ST_W0, "W0", 0 },
        { BOTCORPS_ST_W1, "W1", 7 },
        { BOTCORPS_ST_OV0, "O0", 0 },
        { BOTCORPS_ST_OV1, "O1", 8 },
        { BOTCORPS_ST_N2, "N2", 9 },
    };
    g.fillScreen(PAL_BLACK);
    g.setTextWrap(false, false);
    g.setFont(&fonts::Font0);
    g.setTextSize(1, 1);
    g.setTextColor(PAL_WHITE);
    if (p) {
        for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); ++i) {
            char tail[16];
            tail[0] = 0;
            switch (rows[i].kind) {
            case 1: snprintf(tail, sizeof(tail), "%s", p->ext_st); break;
            case 2: snprintf(tail, sizeof(tail), "%lu", (unsigned long)p->tier0); break;
            case 3: snprintf(tail, sizeof(tail), "%s %d", p->nim_a, p->nim_a_rc); break;
            case 4: snprintf(tail, sizeof(tail), "spr%d", p->sprite_held); break;
            case 5: snprintf(tail, sizeof(tail), "%s %d", p->nim_b, p->nim_b_rc); break;
            case 6: snprintf(tail, sizeof(tail), "free%d", p->sprite_freed); break;
            case 7: snprintf(tail, sizeof(tail), "n%d", p->wifi_n); break;
            case 8: snprintf(tail, sizeof(tail), "%d", p->ov_ok); break;
            case 9: snprintf(tail, sizeof(tail), "%s %d", p->nim_2, p->nim_2_rc); break;
            default: break;
            }
            char line[56];
            snprintf(line, sizeof(line), "%02d %s %lu L%lu %lu %s",
                     p->ord[rows[i].id],
                     rows[i].name,
                     (unsigned long)p->st[rows[i].id].free_b,
                     (unsigned long)p->st[rows[i].id].max_b,
                     (unsigned long)p->st[rows[i].id].min_b,
                     tail);
            g.setCursor(2, i * 8);
            g.print(line);
        }
    }
    if (g.raw()) {
        for (int i = 0; i < BOTCORPS_ICON_N; ++i) {
            botcorps_glyph_blit(*g.raw(), 4 + i * 16 - g.ox, 150 - g.oy, i,
                                PAL_DIAG, PAL_MUTED, PAL_WHITE, nullptr);
        }
    }
    char flag[28];
    snprintf(flag, sizeof(flag), "%s %s L%lu",
             botcorps_band_mode_name(),
             botcorps_band_direct() ? "direct" : "ok",
             (unsigned long)botcorps_band_largest());
    txt(g, flag, 4, 166, PAL_WHITE, lgfx::textdatum::top_left);
    draw_btn(g, 146, 206, ICO_BACK, true);
}

static void paint_ext(Gfx g)
{
    g.clearClipRect();
    g.setTextWrap(false, false);
    if (s_screen == SCR_DIAG) {
        paint_diag(g);
        return;
    }
    const bool hub = s_screen == SCR_GIBSON;
    botcorps_rain_set_hub(hub);
    if (!wants_rain()) g.fillScreen(COL_BG);
    else if (hub) g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
    else g.fillScreen(COL_FIELD);
    if (wants_rain() && g.raw()) {
        const int y0 = botcorps_band_oy();
        botcorps_rain_draw(*g.raw(), y0, y0 + g.raw()->height());
    }
    switch (s_screen) {
    case SCR_RAIN: paint_rain_btn(g); break;
    case SCR_GIBSON: paint_gibson(g); break;
    case SCR_TERMINAL: paint_terminal(g); break;
    case SCR_WIFI: paint_home(g, 0); break;
    case SCR_BT: paint_home(g, 1); break;
    case SCR_BOT: paint_home(g, s_bot); break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: paint_radar(g, true); break;
    case SCR_BOT_RADAR:
    case SCR_SCAN: paint_radar(g, false); break;
    case SCR_WIFI_LIST: paint_list(g, true); break;
    case SCR_BT_LIST: paint_list(g, false); break;
    case SCR_CONTROL: paint_control(g); break;
    default: break;
    }
}

static void tick_radar(Gfx g)
{
    /* Scouts move inside paint_radar. A sweep line would cover the bus. */
    (void)g;
}

static void tick_home(Gfx g, int bot_index)
{
    g.startWrite();
    g.setClipRect(110, 24, 100, 100);
    g.fillCircle(160, 74, 64, COL_FIELD);
    g.fillCircle(160, 74, 48, COL_DEEP);
    g.drawCircle(160, 74, 46, accent());
    const int phase = (int)((millis() / 40u) % 14u);
    g.drawCircle(160, 74, 47 + phase, accent());
    draw_blob(g, 160, 74, 4, bot_color(bot_index), botcorps_shape_for(bot_index), sprite_blink(bot_index));
    g.endWrite();
    g.clearClipRect();
}

static void tick_gibson(Gfx g)
{
    /* The hub Gibson is painted with the rings. A second sprite here covered him. */
    (void)g;
}

static void tick_ext(Gfx g)
{
    if (s_screen == SCR_GIBSON) tick_gibson(g);
    else if (s_screen == SCR_WIFI) tick_home(g, 0);
    else if (s_screen == SCR_BT) tick_home(g, 1);
    else if (s_screen == SCR_BOT) tick_home(g, s_bot);
    else if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR ||
             s_screen == SCR_BOT_RADAR || s_screen == SCR_SCAN) {
        tick_radar(g);
    } else if ((s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) && botcorps_band_direct()) {
        const bool wifi = s_screen == SCR_WIFI_LIST;
        g.startWrite();
        g.setClipRect(0, 0, 320, 120);
        paint_list(g, wifi);
        g.endWrite();
        g.clearClipRect();
        g.startWrite();
        g.setClipRect(0, 120, 320, 120);
        paint_list(g, wifi);
        g.endWrite();
        g.clearClipRect();
    } else if (s_screen == SCR_TERMINAL && !s_sheet) {
        const int phase = (int)((millis() / 530u) % 2u);
        if (phase != s_prompt_phase) {
            s_prompt_phase = phase;
            const int y = PROMPT_Y;
            g.startWrite();
            g.setClipRect(8, y, 300, 10);
            g.fillRect(8, y, 300, 10, COL_DEEP);
            g.setFont(&fonts::Font0);
            g.setTextSize(1);
            g.setTextColor(accent());
            g.setCursor(10, y);
            g.print("user@botcorps:~$ ");
            g.setTextColor(COL_WHITE);
            g.print(botcorps_t0_term_prompt);
            if (phase == 0) g.fillRect(g.getCursorX() + 2, y, 5, 8, accent());
            g.endWrite();
            g.clearClipRect();
        }
    }
}

static void paint_bars(Gfx g, int x, int y, int lit, int n, uint8_t col)
{
    for (int i = 0; i < n; ++i) {
        g.fillRect(x + i * 6, y, 5, 6, i < lit ? col : COL_LINE);
    }
}

static void paint_int(lgfx::LGFX_Device &menu)
{
    const int width = menu.width();
    fmt_times();
    menu.fillScreen(panel565(PAL_DEEP));
    s_int_bot = -1;
    Serial.println("[int] FULL fillScreen");
    menu.fillRect(0, 0, width, BAR_H, panel565(COL_BAR));
    menu.drawFastHLine(0, BAR_H, width, panel565(COL_LINE));
    menu.setFont(&fonts::Font0);
    menu.setTextSize(2, 2);
    menu.setTextColor(panel565(COL_WHITE));
    menu.setCursor(6, 3);
    menu.print("BOTcorps");
    txt(menu, s_clk, width - 4, 4, COL_INK, lgfx::textdatum::top_right);

    const botcorps_probe_t *pr = botcorps_probe_get();
    if (pr && strcmp(pr->ext_st, "fail") == 0) {
        menu.setTextSize(2, 2);
        menu.setTextColor(panel565(COL_WHITE), panel565(PAL_BLACK));
        menu.setCursor(8, 40);
        menu.print("EXT fail");
        return;
    }

    const int bi = card_bot();
    const bool gib = bi < 0;
    const uint8_t col = gib ? COL_IVORY : bot_color(bi);
    const char *name = gib ? "gibson" : BOTS[bi].id;
    const char *target = gib ? "ALL BOTS" : BOTS[bi].target;
    botcorps_id_paint(Gfx(menu), width, gib ? 0 : bi, gib, col,
                      gib ? BOTCORPS_SHAPE_ROUND : botcorps_shape_for(bi), name, target);
    s_int_bot = bi;
}

static void tick_int_live(lgfx::LGFX_Device &menu)
{
    const int width = menu.width();
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    /* No fill under the clock. The glyphs carry the bar colour. */
    menu.startWrite();
    menu.setFont(&fonts::Font0);
    menu.setTextSize(1);
    menu.setTextColor(panel565(COL_INK), panel565(COL_BAR));
    menu.setTextDatum(lgfx::textdatum::top_right);
    menu.drawString(s_clk, width - 4, 4);
    menu.setTextDatum(lgfx::textdatum::top_left);
    menu.endWrite();
}

static void paint_ext_full(void)
{
    layout_covers();
    if (!botcorps_dual_ok()) return;
    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    auto &ext = botcorps_content();
    ext.clearClipRect();
    paint_ext(ext);
    ext.endWrite();
    tick_ext(ext);
    ext.endWrite();
    ext.clearClipRect();
}

static void paint_int_now(void)
{
    Serial.println("[int] paint_int_now");
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &menu = botcorps_menu();
    menu.startWrite();
    paint_int(menu);
    menu.endWrite();
    menu.clearClipRect();
}

static void nudge_row(int d)
{
    if (s_cursor == 0) s_color = (s_color + d + 3) % 3;
    else if (s_cursor == 1) {
        s_mix = (s_mix + d + 3) % 3;
        botcorps_rain_set_mix(s_mix);
    } else if (s_cursor == 2) {
        s_shape = (s_shape + d + 5) % 5;
        botcorps_rain_set_shape(s_shape);
    } else if (s_cursor == 3) {
        if (botcorps_rain_mode() == 0) botcorps_rain_set_mode(1);
        else botcorps_rain_set_mode(0);
        save_rain_color();
    } else if (s_cursor == 4) {
        int m = botcorps_rain_mode();
        if (m <= 0) m = 1;
        else m = m + d;
        if (m < 1) m = 6;
        if (m > 6) m = 1;
        botcorps_rain_set_mode(m);
        save_rain_color();
    } else if (s_cursor == 5) {
        s_saver = (s_saver + d + 4) % 4;
    } else if (s_cursor == 6) {
        s_saver_on = s_saver_on ? 0 : 1;
    } else if (s_cursor == 7) {
        s_idle = (s_idle + d + 3) % 3;
    }
}

static void activate(void)
{
    switch (s_screen) {
    case SCR_RAIN:
        enter_screen(SCR_GIBSON, -1);
        break;
    case SCR_GIBSON:
        if (s_cursor == 0) enter_screen(SCR_TERMINAL, SCR_GIBSON);
        else if (s_cursor == 1) enter_screen(SCR_CONTROL, SCR_GIBSON);
        else enter_screen(SCR_SCAN, -1);
        if (s_screen == SCR_TERMINAL) boot_log();
        break;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT:
        if (s_cursor == 0) {
            enter_screen(s_screen == SCR_WIFI ? SCR_WIFI_RADAR
                         : s_screen == SCR_BT ? SCR_BT_RADAR : SCR_BOT_RADAR, -1);
        } else if (s_cursor == 1) {
            if (s_screen == SCR_WIFI) s_term = 0;
            else if (s_screen == SCR_BT) s_term = 1;
            else s_term = s_bot;
            enter_screen(SCR_TERMINAL, s_screen);
            boot_log();
        } else if (s_cursor == 2) {
            enter_screen(SCR_CONTROL, s_screen);
        } else {
            go_back();
        }
        break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
        if (s_cursor == 0) {
            s_track = !s_track;
            partial_box(0, true);
        } else if (s_cursor == 1) {
            enter_screen(s_screen == SCR_WIFI_RADAR ? SCR_WIFI_LIST : SCR_BT_LIST, -1);
        } else {
            go_back();
        }
        break;
    case SCR_BOT_RADAR:
    case SCR_SCAN:
        if (s_cursor == 0) {
            s_track = !s_track;
            partial_box(0, true);
        } else {
            go_back();
        }
        break;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        if (s_cursor == 3) go_back();
        break;
    case SCR_CONTROL:
        if (s_cursor == CP_N) {
            const int back_to = s_ret >= 0 ? s_ret : SCR_GIBSON;
            enter_screen(SCR_TERMINAL, back_to);
            boot_log();
        } else if (s_cursor == CP_N + 1) {
            go_back();
        }
        break;
    case SCR_DIAG:
        go_back();
        break;
    default:
        break;
    }
}

static void part_begin(Gfx g, int x, int y, int w, int h)
{
    g.startWrite();
    g.setClipRect(x, y, w, h);
}

static void part_end(Gfx g)
{
    g.endWrite();
    g.clearClipRect();
}

static bool cursor_box(int index, int *x, int *y, int *w, int *h)
{
    const int xs_g[3] = { 110, 146, 182 };
    const int xs_h[4] = { 12, 44, 248, 280 };
    *w = BS;
    *h = BS;
    switch (s_screen) {
    case SCR_GIBSON:
        if (index < 0 || index > 2) return false;
        *x = xs_g[index];
        *y = 206;
        return true;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT:
        if (index < 0 || index > 3) return false;
        *x = xs_h[index];
        *y = 133;
        return true;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
        if (index < 0 || index > 2) return false;
        *x = 14;
        *y = 72 + index * 34;
        return true;
    case SCR_BOT_RADAR:
    case SCR_SCAN:
        if (index < 0 || index > 1) return false;
        *x = 14;
        *y = 90 + index * 34;
        return true;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        if (index >= 0 && index < 3) {
            *x = 12;
            *y = 40 + index * 50;
            *w = 296;
            *h = 44;
            return true;
        }
        if (index == 3) {
            *x = 286;
            *y = 206;
            return true;
        }
        return false;
    case SCR_CONTROL:
        if (index >= 0 && index < CP_N) {
            *x = 8;
            *y = 4 + index * 20;
            *w = 304;
            *h = 18;
            return true;
        }
        if (index == CP_N || index == CP_N + 1) {
            *x = (index == CP_N) ? 14 : 48;
            *y = 204;
            return true;
        }
        return false;
    case SCR_DIAG:
        if (index != 0) return false;
        *x = 146;
        *y = 206;
        return true;
    default:
        return false;
    }
}

static void paint_cursor_item(Gfx g, int index, bool sel)
{
    int x, y, w, h;
    if (!cursor_box(index, &x, &y, &w, &h)) return;
    if (s_screen == SCR_CONTROL && index < CP_N) {
        paint_cp_row(g, index, sel);
        (void)x; (void)y; (void)w; (void)h;
        return;
    }
    if ((s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) && index < 3) {
        /* Device pads are part of the bus. Focus repaints both slices. */
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)sel;
        paint_list(g, s_screen == SCR_WIFI_LIST);
        return;
    }
    int id = ICO_BACK;
    if (s_screen == SCR_GIBSON) {
        const int ids[3] = { ICO_TERM, ICO_CTRL, ICO_RADAR };
        id = ids[index];
    } else if (s_screen == SCR_WIFI || s_screen == SCR_BT || s_screen == SCR_BOT) {
        const int ids[4] = { ICO_RADAR, ICO_TERM, ICO_CTRL, ICO_BACK };
        id = ids[index];
    } else if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR) {
        const int ids[3] = { ICO_TRACK, ICO_LIST, ICO_BACK };
        id = ids[index];
    } else if (s_screen == SCR_BOT_RADAR || s_screen == SCR_SCAN) {
        id = index == 0 ? ICO_TRACK : ICO_BACK;
    } else if (s_screen == SCR_CONTROL) {
        id = index == 7 ? ICO_TERM : ICO_BACK;
    }
    draw_btn(g, x, y, id, sel);
    if (id == ICO_TRACK && sel && s_track) g.fillCircle(x + 14, y, 3, accent());
}

static int s_part_i = 0;
static bool s_part_sel = false;
static int s_part_top = 0;

static void draw_focus_part(Gfx g)
{
    paint_cursor_item(g, s_part_i, s_part_sel);
}

static void draw_scan_part(Gfx g)
{
    paint_list(g, s_screen == SCR_WIFI_LIST);
}

static void patch_world(void (*fn)(Gfx), int x, int y, int w, int h)
{
    if (!fn || w <= 0 || h <= 0 || !botcorps_dual_ok()) return;
    if (!botcorps_band_direct() && botcorps_band_live()) {
        if (y < 120 && y + h > 120) {
            botcorps_band_patch(fn, x, y, w, 120 - y);
            botcorps_band_patch(fn, x, 120, w, y + h - 120);
        } else {
            botcorps_band_patch(fn, x, y, w, h);
        }
        return;
    }
    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    Gfx g(botcorps_content());
    part_begin(g, x, y, w, h);
    fn(g);
    part_end(g);
}

static int cursor_icon(int index)
{
    if (s_screen == SCR_GIBSON) {
        const int ids[3] = { ICO_TERM, ICO_CTRL, ICO_RADAR };
        return ids[index];
    }
    if (s_screen == SCR_WIFI || s_screen == SCR_BT || s_screen == SCR_BOT) {
        const int ids[4] = { ICO_RADAR, ICO_TERM, ICO_CTRL, ICO_BACK };
        return ids[index];
    }
    if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR) {
        const int ids[3] = { ICO_TRACK, ICO_LIST, ICO_BACK };
        return ids[index];
    }
    if (s_screen == SCR_BOT_RADAR || s_screen == SCR_SCAN) return index == 0 ? ICO_TRACK : ICO_BACK;
    if (s_screen == SCR_CONTROL) return index == CP_N ? ICO_TERM : ICO_BACK;
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) return ICO_BACK;
    if (s_screen == SCR_DIAG) return ICO_BACK;
    if (s_screen == SCR_RAIN) return ICO_PLAY;
    return ICO_BACK;
}

static void partial_box(int index, bool sel)
{
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) {
        s_part_i = index;
        s_part_sel = sel;
        patch_world(draw_scan_part, 0, 0, 320, 120);
        patch_world(draw_scan_part, 0, 120, 320, 120);
        return;
    }
    int x, y, w, h;
    if (!cursor_box(index, &x, &y, &w, &h)) return;
    if (w == BS && h == BS) {
        const int tw = (int)strlen(btn_name(cursor_icon(index))) * 6 + 4;
        if (tw > w) {
            const int grow = tw - w;
            x -= grow / 2;
            w = tw;
        }
        h += 8;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (x + w > 320) w = 320 - x;
        if (y + h > 240) h = 240 - y;
    }
    s_part_i = index;
    s_part_sel = sel;
    patch_world(draw_focus_part, x, y, w, h);
}

static void partial_cursor(int old_i, int new_i)
{
    if (old_i == new_i) {
        partial_box(new_i, true);
        return;
    }
    partial_box(old_i, false);
    partial_box(new_i, true);
}

/* Bot change. Recompose the card in strips. Does not fill the panel. */
static void partial_int(void)
{
    const botcorps_probe_t *pr = botcorps_probe_get();
    if (pr && strcmp(pr->ext_st, "fail") == 0) return;
    const int bi = card_bot();
    if (bi == s_int_bot) return;
    s_int_bot = bi;
    Serial.printf("[int] partial_int bot=%d\n", bi);
    auto &menu = botcorps_menu();
    const bool gib = bi < 0;
    const uint8_t col = gib ? (uint8_t)COL_IVORY : bot_color(bi);
    const char *name = gib ? "gibson" : BOTS[bi].id;
    const char *target = gib ? "ALL BOTS" : BOTS[bi].target;
    botcorps_id_recompose(menu, menu.width(), gib ? 0 : bi, gib, col,
                          gib ? BOTCORPS_SHAPE_ROUND : botcorps_shape_for(bi),
                          name, target);
}

static void draw_tile_part(Gfx g)
{
    paint_one_board(g, s_part_i, (s_part_i == s_term) && !s_focus_cmds, false);
}

static void partial_tile(int index)
{
    if (s_screen != SCR_TERMINAL) return;
    int x, y;
    term_xy(index, &x, &y);
    s_part_i = index;
    patch_world(draw_tile_part, x - 2, y - 2, 76, 50);
}

static void paint_prompt_line(Gfx g)
{
    const int y = PROMPT_Y;
    const int n = (int)strlen(botcorps_t0_term_prompt);
    char typed[33];
    snprintf(typed, sizeof(typed), "%-32.32s", botcorps_t0_term_prompt);
    txt_bg(g, "user@botcorps:~$ ", 10, y, accent(), PAL_DEEP, lgfx::textdatum::top_left);
    txt_bg(g, typed, 10 + 18 * 6, y, COL_WHITE, PAL_DEEP, lgfx::textdatum::top_left);
    const char *ghost = cand_ghost();
    if (ghost && n < 32) {
        txt_bg(g, ghost, 10 + (18 + n) * 6, y, COL_MUTED, PAL_DEEP, lgfx::textdatum::top_left);
    }
    if ((millis() / 530u) % 2u == 0 && n < 32) {
        const int cx = 10 + (18 + n) * 6;
        g.fillRect(cx + 2, y, 5, 8, accent());
    }
}

static void draw_term_lower(Gfx g)
{
    paint_boards(g);
    paint_prompt(g);
}

static void draw_prompt_line_part(Gfx g)
{
    paint_prompt_line(g);
}

static void partial_prompt_line(void)
{
    const int y = PROMPT_Y;
    patch_world(draw_prompt_line_part, 8, y, 300, 10);
}

static void partial_term_lower(void)
{
    if (s_screen != SCR_TERMINAL || s_sheet) {
        partial_prompt_line();
        return;
    }
    patch_world(draw_term_lower, 0, 120, 320, 120);
}

static void draw_prompt_head_part(Gfx g)
{
    paint_term_head(g);
}

static void draw_prompt_body_part(Gfx g)
{
    paint_prompt(g);
}

static void partial_prompt_panel(void)
{
    if (s_screen != SCR_TERMINAL) return;
    patch_world(draw_prompt_head_part, 0, 0, 320, 18);
    patch_world(draw_prompt_body_part, 4, 167, 312, 73);
}

static void draw_chip_part(Gfx g)
{
    const int x = 8 + (s_part_i % 6) * 51;
    const int y = 170 + (s_part_i / 6) * 18;
    plate(g, x, y, 49, 16, s_part_sel ? COL_SEL : COL_BG, s_part_sel ? accent() : COL_LINE);
    txt_bg(g, CMDS[s_part_i], x + 24, y + 4, s_part_sel ? COL_WHITE : COL_MUTED,
           s_part_sel ? COL_SEL : COL_BG, lgfx::textdatum::top_center);
}

static void partial_chip(int index)
{
    const int x = 8 + (index % 6) * 51;
    const int y = 170 + (index / 6) * 18;
    s_part_i = index;
    s_part_sel = index == s_cmd_sel;
    patch_world(draw_chip_part, x, y, 49, 16);
}

static int sheet_row_y(int index, int top)
{
    return 30 + (index - top) * 15;
}

static void draw_sheet_part(Gfx g)
{
    paint_sheet(g);
}

static void partial_sheet(void)
{
    int by = 0;
    int h = 0;
    int top = 0;
    int nshow = 0;
    sheet_box(&by, &h, &top, &nshow);
    const int x = 12;
    const int y = by - 1;
    const int w = 298;
    const int hh = h + 6;
    if (y < 120 && y + hh > 120) {
        patch_world(draw_sheet_part, x, y, w, 120 - y);
        patch_world(draw_sheet_part, x, 120, w, y + hh - 120);
    } else {
        patch_world(draw_sheet_part, x, y < 0 ? 0 : y, w, hh);
    }
}

static void draw_under_sheet_part(Gfx g)
{
    g.fillRect(8, 21, 304, 142, COL_BG);
    paint_boards(g);
}

static void restore_under_sheet(void)
{
    patch_world(draw_under_sheet_part, 8, 21, 304, 142);
}

static void draw_sheet_row_part(Gfx g)
{
    const int y = sheet_row_y(s_part_i, s_part_top);
    const uint8_t bg = s_part_sel ? (uint8_t)COL_SEL : (uint8_t)COL_BG;
    g.fillRect(14, y - 1, 292, 15, bg);
    if (s_part_sel) g.fillRect(14, y - 1, 2, 14, accent());
    if (s_part_i >= 0 && s_part_i < CMD_N) {
        const uint8_t ink = term_ink();
        txt_bg(g, CMDN[s_part_i], 42, y + 2,
               s_part_sel ? ink : (uint8_t)PAL_TERM_INK, bg,
               lgfx::textdatum::top_left);
        txt_bg(g, CMDP[s_part_i], 100, y + 3,
               s_part_sel ? ink : accent(), bg,
               lgfx::textdatum::top_left);
    }
}

static void partial_sheet_row(int index, int top)
{
    if (index < top || index >= top + 8) return;
    const int y = sheet_row_y(index, top);
    s_part_i = index;
    s_part_top = top;
    s_part_sel = index == s_sheet_i;
    patch_world(draw_sheet_row_part, 14, y - 1, 292, 15);
}

static void draw_portrait_part(Gfx g)
{
    g.fillCircle(160, 74, 64, COL_FIELD);
    g.fillCircle(160, 74, 48, COL_DEEP);
    g.drawCircle(160, 74, 46, accent());
    const int phase = (int)((millis() / 40u) % 14u);
    g.drawCircle(160, 74, 47 + phase, accent());
    draw_blob(g, 160, 74, 4, bot_color(s_part_i), botcorps_shape_for(s_part_i), sprite_blink(s_part_i));
}

static void partial_portrait(int bot_index)
{
    s_part_i = bot_index;
    patch_world(draw_portrait_part, 110, 24, 100, 100);
    partial_int();
}

static void queue_focus(int old_i, int new_i)
{
    s_focus_old = old_i;
    s_focus_new = new_i;
    s_focus_pend = true;
}

static void move_cursor(int d)
{
    const int n = button_count();
    if (n <= 0) return;
    const int old = s_cursor;
    s_cursor = (s_cursor + d + n) % n;
    if (s_cursor != old) queue_focus(old, s_cursor);
}

static int sheet_top_for(int index)
{
    (void)index;
    return 0;
}

static void on_arrows(int key)
{
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet && !botcorps_t0_term_prompt[0]) {
            if (key == 1 || key == 3) s_sheet_i = (s_sheet_i + CMD_N - 1) % CMD_N;
            if (key == 2 || key == 4) s_sheet_i = (s_sheet_i + 1) % CMD_N;
            s_dirty = true;
            return;
        }
        if (botcorps_t0_term_prompt[0] && (key == 1 || key == 2)) {
            if (s_hist_n <= 0) return;
            if (s_hist_i < 0) s_hist_i = s_hist_n;
            s_hist_i += (key == 1) ? -1 : 1;
            if (s_hist_i < 0) s_hist_i = 0;
            if (s_hist_i >= s_hist_n) {
                s_hist_i = -1;
                botcorps_t0_term_prompt[0] = 0;
            } else {
                snprintf(botcorps_t0_term_prompt, 80, "%s", botcorps_t0_term_hist[s_hist_i]);
            }
            partial_term_lower();
            return;
        }
        if (s_focus_cmds) {
            const int old = s_cmd_sel;
            const int d = key == 3 ? -1 : key == 4 ? 1 : key == 1 ? -6 : 6;
            s_cmd_sel = (s_cmd_sel + d + 12) % 12;
            if (s_cmd_sel != old) {
                partial_chip(old);
                partial_chip(s_cmd_sel);
            }
            return;
        }
        const int old = s_term;
        int c = s_term % 4;
        int r = s_term / 4;
        if (key == 3) c = (c + 3) % 4;
        if (key == 4) c = (c + 1) % 4;
        if (key == 1) r = (r + 2) % 3;
        if (key == 2) r = (r + 1) % 3;
        s_term = r * 4 + c;
        if (s_term != old) {
            partial_tile(old);
            partial_tile(s_term);
            partial_int();
        }
        return;
    }
    if (s_screen == SCR_CONTROL) {
        if (key == 1) move_cursor(-1);
        if (key == 2) move_cursor(1);
        if (key == 3 || key == 4) {
            if (s_cursor >= CP_N) move_cursor(key == 4 ? 1 : -1);
            else {
                nudge_row(key == 4 ? 1 : -1);
                queue_focus(s_cursor, s_cursor);
            }
        }
        return;
    }
    if (s_screen == SCR_BOT && (key == 3 || key == 4)) {
        int i = s_bot;
        if (i < 2) i = 2;
        i += (key == 4) ? 1 : -1;
        if (i < 2) i = 11;
        if (i > 11) i = 2;
        if (i != s_bot) {
            s_bot = i;
            partial_portrait(s_bot);
        }
        return;
    }
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) {
        if (s_info_dev >= 0) return;
        if (s_act >= 0) {
            if (key == 1 || key == 3) s_act = (s_act + ACT_N - 1) % ACT_N;
            if (key == 2 || key == 4) s_act = (s_act + 1) % ACT_N;
            partial_box(s_cursor, true);
            return;
        }
        const int band = list_band();
        const int dir = (key == 1 || key == 3) ? -1 : (key == 2 || key == 4) ? 1 : 0;
        if (dir == 0) return;
        int i = s_cursor;
        for (int k = 0; k < 4; ++k) {
            i = (i + dir + 4) % 4;
            if (i == 3 || !s_dev_ign[band][i]) break;
        }
        if (i != s_cursor) {
            const int old = s_cursor;
            s_cursor = i;
            queue_focus(old, s_cursor);
        }
        return;
    }
    if (key == 1 || key == 3) move_cursor(-1);
    if (key == 2 || key == 4) move_cursor(1);
}

static void act_pick(void)
{
    const int band = list_band();
    const int dev = s_act_dev;
    const int which = s_act;
    s_act = -1;
    if (dev < 0 || dev > 2) return;
    if (which == 0) {
        s_hunt_dev = dev;
        s_hunt_band = band;
    } else if (which == 1) {
        s_dev_fol[band][dev] = s_dev_fol[band][dev] ? 0 : 1;
    } else if (which == 2) {
        s_dev_tag[band][dev] = s_dev_tag[band][dev] >= 4 ? 0 : (uint8_t)(s_dev_tag[band][dev] + 1);
    } else if (which == 3) {
        s_info_dev = dev;
    } else if (which == 4) {
        s_dev_ign[band][dev] = 1;
        if (s_cursor == dev) {
            s_cursor = 3;
            for (int i = 0; i < 3; ++i) {
                if (!s_dev_ign[band][i]) {
                    s_cursor = i;
                    break;
                }
            }
        }
    }
}

static void on_ok(void)
{
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) {
        if (s_info_dev >= 0) {
            s_info_dev = -1;
            partial_box(s_cursor, true);
            return;
        }
        if (s_act >= 0) {
            act_pick();
            partial_box(s_cursor, true);
            return;
        }
        if (s_cursor >= 0 && s_cursor < 3 && !s_dev_ign[list_band()][s_cursor]) {
            s_act = 0;
            s_act_dev = s_cursor;
            partial_box(s_cursor, true);
            return;
        }
    }
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet) {
            if (botcorps_t0_term_prompt[0]) {
                run_cmd(botcorps_t0_term_prompt);
                botcorps_t0_term_prompt[0] = 0;
                s_dirty = true;
                return;
            }
            if (s_sheet_i < 0) s_sheet_i = 0;
            if (s_sheet_i >= CMD_N) s_sheet_i = CMD_N - 1;
            if (strcmp(CMDN[s_sheet_i], "cmds") == 0) {
                s_sheet_i = 0;
                s_dirty = true;
                return;
            }
            snprintf(botcorps_t0_term_prompt, 60, "%s%s", CMDN[s_sheet_i],
                     CMD_ARG[s_sheet_i] ? " " : "");
            s_sheet = false;
            s_dirty = true;
            botcorps_lut_term(false);
            return;
        }
        if (botcorps_t0_term_prompt[0]) {
            run_cmd(botcorps_t0_term_prompt);
            botcorps_t0_term_prompt[0] = 0;
            return;
        }
        if (s_focus_cmds) {
            if (strcmp(CMDS[s_cmd_sel], "cmds") == 0) {
                s_sheet = true;
                s_sheet_i = 0;
                s_dirty = true;
                /* Same gate as the full present, so this path installs the sheet lut. */
                botcorps_lut_term(s_screen == SCR_TERMINAL && s_sheet);
                partial_sheet();
            } else if (strcmp(CMDS[s_cmd_sel], "rain") == 0) {
                int m = botcorps_rain_mode() + 1;
                if (m > 6) m = 0;
                char line[24];
                snprintf(line, sizeof(line), "rain %s", m == 0 ? "multi" : RAIN_WORD[m]);
                run_cmd(line);
            } else {
                run_cmd(CMDS[s_cmd_sel]);
            }
            return;
        }
        open_bot(s_term);
        return;
    }
    activate();
}

static void on_esc(void)
{
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) {
        if (s_info_dev >= 0) {
            s_info_dev = -1;
            partial_box(s_cursor, true);
            return;
        }
        if (s_act >= 0) {
            s_act = -1;
            partial_box(s_cursor, true);
            return;
        }
    }
    if (s_screen == SCR_TERMINAL && (s_sheet || s_focus_cmds)) {
        s_sheet = false;
        s_focus_cmds = false;
        s_dirty = true;
        botcorps_lut_term(false);
        return;
    }
    go_back();
}

static void on_tab(void)
{
    if (s_screen != SCR_TERMINAL || s_sheet) return;
    if (!botcorps_t0_term_prompt[0]) {
        s_sheet = !s_sheet;
        s_focus_cmds = false;
        if (s_sheet) s_sheet_i = 0;
        s_tab_i = -1;
        s_dirty = true;
        botcorps_lut_term(s_screen == SCR_TERMINAL && s_sheet);
        return;
    }
    const bool cont = s_tab_i >= 0 && strcmp(botcorps_t0_term_prompt, tab_last()) == 0;
    if (!cont) {
        snprintf(tab_base(), 60, "%s", botcorps_t0_term_prompt);
        s_tab_i = -1;
    }
    int match[PCMD_N];
    int n = 0;
    const size_t bn = strlen(tab_base());
    for (int i = 0; i < PCMD_N; ++i) {
        if (strncmp(PCMD[i], tab_base(), bn) == 0) match[n++] = i;
    }
    if (n == 0) return;
    s_tab_i = (s_tab_i + 1) % n;
    snprintf(botcorps_t0_term_prompt, 60, "%s ", PCMD[match[s_tab_i]]);
    snprintf(tab_last(), 60, "%s", botcorps_t0_term_prompt);
    partial_term_lower();
}

static void on_char(char c)
{
    if (s_screen == SCR_GIBSON) {
        s_gp_focus = true;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        const size_t n = strlen(s_gp_buf);
        if (n + 1 < sizeof(s_gp_buf) && c >= 32 && c < 127) {
            s_gp_buf[n] = c;
            s_gp_buf[n + 1] = 0;
        }
        partial_gp_strip();
        return;
    }
    if (s_screen != SCR_TERMINAL) return;
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n + 1 >= 60) return;
    botcorps_t0_term_prompt[n] = c;
    botcorps_t0_term_prompt[n + 1] = 0;
    s_tab_i = -1;
    partial_term_lower();
}

static void on_bs(void)
{
    if (s_screen == SCR_GIBSON && s_gp_buf[0]) {
        s_gp_focus = true;
        const size_t n = strlen(s_gp_buf);
        s_gp_buf[n - 1] = 0;
        partial_gp_strip();
        return;
    }
    if (s_screen != SCR_TERMINAL) {
        on_esc();
        return;
    }
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n == 0) return;
    botcorps_t0_term_prompt[n - 1] = 0;
    s_tab_i = -1;
    partial_term_lower();
}

enum { K_NONE = 0, K_UP = 1, K_DN = 2, K_LT = 3, K_RT = 4, K_OK = 5, K_ESC = 6, K_TAB = 7, K_BS = 8, K_CHAR = 9 };

static int read_key(char *typed)
{
    *typed = 0;
    if (!M5Cardputer.Keyboard.isChange()) return K_NONE;
    if (!M5Cardputer.Keyboard.isPressed()) return K_NONE;
    const auto &st = M5Cardputer.Keyboard.keysState();
    if (st.enter) return K_OK;
    for (uint8_t h : st.hid_keys) {
        if (h == 0x2A) return K_BS;
        if (h == 0x2B) return K_TAB;
        if (h == 0x29) return K_ESC;
        if (h == 0x52) return K_UP;
        if (h == 0x51) return K_DN;
        if (h == 0x50) return K_LT;
        if (h == 0x4F) return K_RT;
    }
    for (char c : st.word) {
        if (c == ';' || c == ':') return K_UP;
        if (c == '.' || c == '>') return K_DN;
        if (c == ',' || c == '<') return K_LT;
        if (c == '/' || c == '?') return K_RT;
        if (c == '`' || c == '~' || c == 0x1B) return K_ESC;
        if (c == '\t') return K_TAB;
        if (c == '\b' || c == 127) return K_BS;
        if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
            *typed = c;
            return K_CHAR;
        }
    }
    return K_NONE;
}

void botcorps_ui_mark_dirty(void)
{
    s_dirty = true;
    s_int_dirty = true;
}

static void recompose_ext(Gfx g)
{
    paint_ext(g);
    tick_ext(g);
}

void botcorps_ui_begin(void)
{
    botcorps_band_set_recompose(recompose_ext);
    s_boot_ms = millis();
    s_last_key = s_boot_ms;
    s_bub[0] = 0;
    s_screen = SCR_RAIN;
    s_cursor = 0;
    s_dirty = true;
    s_int_dirty = true;
    s_ret = -1;
    s_track_name[0] = 0;
    botcorps_t0_term_prompt[0] = 0;
    load_rain_color();
    botcorps_rain_set_draw(true);
    botcorps_rain_log_heap("boot");
}

static int gp_screen(const char *name)
{
    if (!name || !name[0]) return -1;
    if (strcmp(name, "terminal") == 0) return SCR_TERMINAL;
    if (strcmp(name, "control") == 0) return SCR_CONTROL;
    if (strcmp(name, "bots") == 0 || strcmp(name, "bot") == 0) return SCR_BOT;
    if (strcmp(name, "wifi") == 0) return SCR_WIFI;
    if (strcmp(name, "ble") == 0 || strcmp(name, "bluetooth") == 0) return SCR_BT;
    if (strcmp(name, "scan") == 0 || strcmp(name, "radar") == 0) return SCR_SCAN;
    if (strcmp(name, "wifiradar") == 0) return SCR_WIFI_RADAR;
    if (strcmp(name, "bleradar") == 0) return SCR_BT_RADAR;
    if (strcmp(name, "rain") == 0) return SCR_RAIN;
    if (strcmp(name, "gibson") == 0) return SCR_GIBSON;
    if (strcmp(name, "diag") == 0) return SCR_DIAG;
    return -1;
}

static void gp_open(int id)
{
    enter_screen(id, SCR_GIBSON);
    if (id == SCR_TERMINAL) boot_log();
}

static void gp_run(const char *line)
{
    char buf[40];
    snprintf(buf, sizeof(buf), "%s", line ? line : "");
    char *cmd = buf;
    while (*cmd == ' ') ++cmd;
    char *sp = strchr(cmd, ' ');
    char *arg = 0;
    if (sp) {
        *sp = 0;
        arg = sp + 1;
        while (*arg == ' ') ++arg;
    }
    char echo[28];
    snprintf(echo, sizeof(echo), "> %s", cmd);
    if (arg && arg[0]) {
        const size_t n = strlen(echo);
        snprintf(echo + n, sizeof(echo) - n, " %s", arg);
    }
    gp_push(echo);
    if (strcmp(cmd, "clear") == 0) {
        s_gp_n = 0;
        partial_gp_strip();
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        run_cmd("help");
        if (s_log_n > 0) gp_push(log_line(s_log_n - 1));
        partial_gp_strip();
        return;
    }
    if (strcmp(cmd, "go") == 0 || strcmp(cmd, "load") == 0) {
        if (!arg || !arg[0]) {
            snprintf(s_gp_buf, sizeof(s_gp_buf), "%s ", cmd);
            gp_push("need a screen");
            partial_gp_strip();
            return;
        }
        const int id = gp_screen(arg);
        if (id < 0) {
            gp_push("no such screen");
            partial_gp_strip();
            return;
        }
        gp_open(id);
        return;
    }
    if (strcmp(cmd, "terminal") == 0 || strcmp(cmd, "control") == 0 || strcmp(cmd, "bots") == 0
        || strcmp(cmd, "wifi") == 0 || strcmp(cmd, "ble") == 0) {
        gp_open(gp_screen(cmd));
        return;
    }
    if (strcmp(cmd, "scan") == 0 && (!arg || !arg[0] || strcmp(arg, "radar") == 0)) {
        gp_open(SCR_SCAN);
        return;
    }
    if (strcmp(cmd, "scan") == 0 && arg && strcmp(arg, "wifi") == 0) {
        gp_open(SCR_WIFI_RADAR);
        return;
    }
    if (strcmp(cmd, "scan") == 0 && arg && (strcmp(arg, "ble") == 0 || strcmp(arg, "bluetooth") == 0)) {
        gp_open(SCR_BT_RADAR);
        return;
    }
    if (strcmp(cmd, "rain") == 0 || strcmp(cmd, "filter") == 0 || strcmp(cmd, "ping") == 0
        || strcmp(cmd, "info") == 0 || strcmp(cmd, "track") == 0 || strcmp(cmd, "sniff") == 0
        || strcmp(cmd, "hunt") == 0 || strcmp(cmd, "scan") == 0) {
        if (strcmp(cmd, "hunt") == 0 || strcmp(cmd, "scan") == 0) gp_open(SCR_TERMINAL);
        run_cmd(line);
        if (s_screen == SCR_GIBSON && s_log_n > 0) gp_push(log_line(s_log_n - 1));
        if (s_screen == SCR_GIBSON) partial_gp_strip();
        return;
    }
    gp_push("no such command");
    s_mood = MOOD_SAD;
    s_mood_until = millis() + 1400u;
    partial_gp_strip();
}

void botcorps_ui_input(void)
{
    char typed = 0;
    const int key = read_key(&typed);
    if (key == K_NONE) return;
    s_last_key = millis();
    if (s_screen == SCR_GIBSON && (key == K_CHAR || key == K_OK)) {
        s_mood = MOOD_HAPPY;
        s_mood_until = s_last_key + 1500u;
        s_bub[0] = (key == K_CHAR && typed) ? typed : '*';
        s_bub[1] = 0;
    }
    if (key == K_OK && s_screen == SCR_GIBSON && s_gp_buf[0]) {
        s_gp_focus = true;
        char line[28];
        snprintf(line, sizeof(line), "%s", s_gp_buf);
        s_gp_buf[0] = 0;
        gp_run(line);
        return;
    }
    if (key == K_UP || key == K_DN || key == K_LT || key == K_RT) on_arrows(key);
    else if (key == K_OK) on_ok();
    else if (key == K_ESC) on_esc();
    else if (key == K_TAB) on_tab();
    else if (key == K_BS) on_bs();
    else if (key == K_CHAR) on_char(typed);
}

void botcorps_ui_draw(void)
{
    botcorps_rain_set_hub(s_screen == SCR_GIBSON);
    layout_covers();
    bool ext_anim = false;
    if (botcorps_dual_ok() && !wants_rain() && ext_animates() && anim_step_due())
        ext_anim = true;
    if (s_focus_pend && !s_dirty) partial_cursor(s_focus_old, s_focus_new);
    s_focus_pend = false;
    const bool dirty = s_dirty;
    const bool int_dirty = s_int_dirty;
    bool stepped = false;
    if (botcorps_dual_ok() && wants_rain()) stepped = botcorps_rain_step();
    static uint32_t s_pulse_ms = 0;
    const bool pulse = due_140(&s_pulse_ms);
    if (botcorps_dual_ok() && !botcorps_band_direct()) {
        /* A full present fillScreens. Nav must not get here. Rain frames and
         * screen changes do. An anim tick on its own stays in place. */
        if (dirty || stepped) {
            if (dirty) s_dirty = false;
            botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
            /* One swap for the whole frame, before either band is pushed. */
            botcorps_lut_term(s_screen == SCR_TERMINAL && s_sheet);
            botcorps_band_present([](Gfx g) {
                paint_ext(g);
                tick_ext(g);
            });
            botcorps_band_set_oy(0);
        } else if (ext_anim && s_screen == SCR_TERMINAL) {
            partial_prompt_line();
        } else if (ext_anim && (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST)) {
            patch_world(draw_scan_part, 0, 0, 320, 120);
            patch_world(draw_scan_part, 0, 120, 320, 120);
        }
        if (int_dirty) {
            s_int_dirty = false;
            paint_int_now();
        } else {
            fmt_times();
            static char drawn_b[8];
            if (strcmp(drawn_b, s_up) != 0) {
                snprintf(drawn_b, sizeof(drawn_b), "%s", s_up);
                tick_int_live(botcorps_menu());
            }
            if (pulse) {
                const int bi = card_bot();
                const bool gib = bi < 0;
                const botcorps_probe_t *pr = botcorps_probe_get();
                if (!(pr && strcmp(pr->ext_st, "fail") == 0)) {
                    botcorps_id_tick(botcorps_menu(), botcorps_menu().width(),
                                     gib ? 0 : bi, gib, gib ? COL_IVORY : bot_color(bi));
                }
            }
        }
        return;
    }
    if (dirty) {
        s_dirty = false;
        botcorps_lut_term(s_screen == SCR_TERMINAL && s_sheet);
        paint_ext_full();
    }
    if (int_dirty) {
        s_int_dirty = false;
        paint_int_now();
        return;
    }
    if (dirty) return;
    if (botcorps_dual_ok()) {
        botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
        tick_ext(botcorps_content());
        botcorps_content().endWrite();
        botcorps_content().clearClipRect();
    }
    fmt_times();
    static char drawn[8];
    if (strcmp(drawn, s_up) != 0) {
        snprintf(drawn, sizeof(drawn), "%s", s_up);
        tick_int_live(botcorps_menu());
    }
    if (pulse) {
        const int bi = card_bot();
        const bool gib = bi < 0;
        const botcorps_probe_t *pr = botcorps_probe_get();
        if (!(pr && strcmp(pr->ext_st, "fail") == 0)) {
            botcorps_id_tick(botcorps_menu(), botcorps_menu().width(),
                             gib ? 0 : bi, gib, gib ? COL_IVORY : bot_color(bi));
        }
    }
}

#ifdef BOTCORPS_HOST_RENDER
void botcorps_host_terminal(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = true;
    botcorps_lut_term(true);
    s_sheet_i = 2;
    s_focus_cmds = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    log_add("BOTcorps term");
    log_add_bot(0, "wifi channel clear");
    log_add_bot(12, "gibson reading you");
    paint_terminal(Gfx());
}

void botcorps_host_idle(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = false;
    botcorps_lut_term(false);
    s_sheet_i = 0;
    s_focus_cmds = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    log_add("BOTcorps term");
    log_add("[ok] 12 boards");
    paint_terminal(Gfx());
}

void botcorps_host_hunt(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_WIFI_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_cursor = 2;
    s_color = 0;
    s_act = 0;
    s_act_dev = 2;
    s_info_dev = -1;
    s_bus_t0 = 1000;
    paint_list(Gfx(), true);
}

void botcorps_host_bus(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_WIFI_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_cursor = 0;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_bus_t0 = 1000;
    paint_bus_scan(Gfx(), true);
}

void botcorps_host_amber(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = true;
    botcorps_lut_term(true);
    s_sheet_i = 2;
    s_focus_cmds = false;
    s_term = 2;
    s_color = 1;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    log_add("BOTcorps term");
    log_add("[ok] 12 boards");
    paint_terminal(Gfx());
}

int botcorps_host_cmds_sheet(void)
{
    host_term_lut = 0;
    s_screen = SCR_TERMINAL;
    s_focus_cmds = true;
    s_sheet = false;
    s_dirty = false;
    botcorps_t0_term_prompt[0] = 0;
    s_cmd_sel = 11;
    on_ok();
    return (host_term_lut && s_dirty && s_sheet) ? 1 : 0;
}

int botcorps_host_int_span(void)
{
    uint8_t *saved = host_frame;
    static uint8_t scratch[320 * 240];
    host_frame = scratch;
    host_rgb_ov = 0;
    host_rgb_span = 0;
    host_int_full = 0;
    host_int_strip = 0;
    host_int_part = 0;
    s_sheet = true;
    s_term = 2;
    s_bot = 2;
    host_int_mask = 1;
    const int ids[] = {
        SCR_TERMINAL, SCR_GIBSON, SCR_WIFI, SCR_BT, SCR_BOT,
        SCR_WIFI_LIST, SCR_BT_LIST, SCR_CONTROL, SCR_RAIN, SCR_DIAG
    };
    for (int id : ids) {
        s_screen = id;
        s_sheet = true;
        paint_int(botcorps_menu());
    }
    host_int_mask = 4;
    s_screen = SCR_TERMINAL;
    s_term = 2;
    s_int_bot = 99;
    partial_int();
    host_int_mask = 0;
    host_frame = saved;
    return host_rgb_ov;
}
#endif
