/*
 * EXT is the mock's screen: icon buttons, terminal, rain behind chrome.
 * INT is the selected bot's ID card. No text menu and no hint line.
 * A screen change, and an overlay opening or closing, fills the panel
 * before the new chrome is drawn.
 */
#include "ui/botcorps_frame_cfg.h"
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

#ifdef BOTCORPS_HOST_RENDER
#define GATE_BOX(g, k, n, x, y, w, h, p) host_gate_box((g).ox, (g).oy, k, n, x, y, w, h, p)
#else
#define GATE_BOX(g, k, n, x, y, w, h, p) ((void)0)
#endif

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
    /* v1016n (HIT LIST #1, Oct 15, ac9c53b): no per-bot home screens and no bot radar.
     * Only Gibson has a main screen; every other bot lives on its scan screen as a chip. */
    SCR_WIFI_RADAR,
    SCR_WIFI_LIST,
    SCR_BT_RADAR,
    SCR_BT_LIST,
    SCR_SCAN,
    SCR_CONTROL,
    SCR_DIAG,
    SCR_GCHAT
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
/* Gibson chat. Own set. jackin is the only game entry. Flash only. */
static const int GC_N = 19;
static const int GC_GAT[6] = { 0, 4, 8, 12, 16, 19 };
static const char *const GC_GN[5] = { "HUB", "ROUTE", "GAME", "ASK", "SYS" };
static const char *const GC_NM[GC_N] = {
    "crew", "scan", "status", "mood",
    "terminal", "go", "load", "control",
    "jackin", "defend", "lobby", "score",
    "ask", "hail", "whisper", "why",
    "clear", "history", "about"
};
static const char *const GC_PP[GC_N] = {
    "roll the crew", "bus sweep", "core link heat", "gibson mood",
    "open terminal", "terminal|scan|control|hub", "screen|game", "control panel",
    "enter defend", "beginner|veteran", "game lobby", "best run",
    "ask a bot", "call a bot", "quiet line", "gibson explains",
    "wipe the chat", "recent lines", "gibson build"
};
static const uint8_t GC_ARG[GC_N] = {
    0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0
};
/* Index into CMD_ICO. Flash only. */
static const uint8_t GC_II[GC_N] = {
    3, 0, 7, 2, 11, 10, 11, 4, 12, 23, 20, 24, 2, 16, 18, 2, 32, 33, 35
};
static const char *const HAIL[13] = {
    "reading you", "channel clear", "link warm", "quiet", "keys ready", "eyes open",
    "airborne", "ping soft", "click armed", "mesh synced", "on hold", "wave ready", "nearby"
};
static const char *const CHAT3[3] = { "copy", "on it", "noted" };
/* Hunt dropdown on the Gibson scan. Order is the hunt-actions menu. */
static const int ACT_N = 5;
static const char *const ACT_NAME[ACT_N] = { "hunt", "info", "tag", "ignore", "back" };
static const uint8_t ACT_ICO[ACT_N][8] = {
    { 0x3c, 0x42, 0x99, 0xbd, 0xbd, 0x99, 0x42, 0x3c },
    { 0x3c, 0x5a, 0x81, 0x99, 0x99, 0x99, 0x42, 0x3c },
    { 0x00, 0xfe, 0x83, 0xb1, 0xb3, 0xfe, 0x00, 0x00 },
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
static const int HUNT_N = 24;
static const int HUNT_NEAR = -55;
static const int HUNT_LOST = -90;
static int8_t s_hunt_buf[HUNT_N];
static uint8_t s_hunt_len = 0;
static uint32_t s_hunt_tS = 0;
static int s_hunt_force = 0; /* 0 = live mock; nonzero = forced dBm for host paints */
static const int SB_N = 12;   /* devices per band in the scan box */
static uint8_t s_dev_tag[2][SB_N];
static uint8_t s_dev_ign[2][SB_N];
static int s_gloss = -1;      /* glossary row, -1 closed */
/* Found-alert takeover. Dismiss only via Hunt/Info/Ignore/Back. */
static const int FA_N = 4;
static const char *const FA_NAME[FA_N] = { "hunt", "info", "ignore", "back" };
static bool s_fa_on = false;
static int s_fa_sel = 0;
static int s_fa_band = 0;
static int s_fa_dev = 0;
/* Nav focus. Drawn in place. Never promotes a full-panel clear. */
static int s_focus_old = -1;
static int s_focus_new = -1;
static bool s_focus_pend = false;
static bool s_track = false;
static bool s_focus_cmds = false;
/* v1017a: the commands (≡) button is a cursor stop: Down from the bottom board row; Enter opens the
 * commands menu (the 6x2 chip palette, s_focus_cmds). */
static bool s_cmd_btn = false;
static bool s_sheet = false;
static int s_sheet_i = 0;
static int s_cmd_sel = 0;
static int s_hist_n = 0;
static int s_hist_i = -1;
static int s_found_n = 0;
static int s_log_n = 0;
static int s_log_next = 0;
static int s_filter = 0; /* low bits: 0 off, 1 wifi, 2 ble. Bit 8: scan-all mode. */
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
/* v1017b (Dirt 8:26-8:28): hub focus zone. 0 = icon row (Left/Right cycle the icons),
 * 1 = prompt, 2 = command menu, 3 = Gibson (Enter = Gibson game). Up/Down walk the zones. */
static int s_hub_zone = 0;
static int s_hub_cmd = -1;
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
    case SCR_WIFI_RADAR: return "wifi-radar";
    case SCR_WIFI_LIST: return "wifi-list";
    case SCR_BT_RADAR: return "bt-radar";
    case SCR_BT_LIST: return "bt-list";
    case SCR_SCAN: return "scan";
    case SCR_CONTROL: return "control";
    case SCR_DIAG: return "diag";
    case SCR_GCHAT: return "gchat";
    default: return "screen";
    }
}

static int default_back(int id)
{
    switch (id) {
    case SCR_GIBSON: return SCR_RAIN;
    case SCR_TERMINAL: return SCR_GIBSON;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: return SCR_TERMINAL;   /* v1016n: live scan boxes go back to the 12 chip tiles */
    case SCR_WIFI_LIST: return SCR_WIFI_RADAR;
    case SCR_BT_LIST: return SCR_BT_RADAR;
    case SCR_SCAN:
    case SCR_CONTROL:
    case SCR_DIAG: return SCR_GIBSON;
    case SCR_GCHAT: return SCR_GIBSON;
    default: return -1;
    }
}

/* v1016f: which board the bus scan shows. 0 wifi, 1 ble (live), 2..11 a bot, 12 the Gibson core. */
static int s_scan_bot = 12;
/* v1016f: Terminal focus on the crown (Up from the top board row); Fn chord flag from read_key. */
static bool s_term_crown = false;
static bool s_key_fn = false;

static bool sbox_id(int id)
{
    return id == SCR_SCAN || id == SCR_WIFI_RADAR || id == SCR_BT_RADAR || id == SCR_WIFI_LIST || id == SCR_BT_LIST;
}

static bool sbox_on(void)
{
    return sbox_id(s_screen);
}

static bool rain_static(void);
static bool wants_rain(void)
{
    return !sbox_on()
        && s_screen != SCR_DIAG && s_screen != SCR_TERMINAL
        && s_screen != SCR_GCHAT;
}

/* Panel screens: rain painted once per full paint (no stepping, no covers needed). */
static bool rain_static(void)
{
    return sbox_on() || s_screen == SCR_TERMINAL || s_screen == SCR_GCHAT || s_screen == SCR_DIAG
        || (s_screen == SCR_RAIN && s_act == 4);
}

/* EXT chrome that moves on its own: sweep, pulse, bob, or the prompt caret. */
static bool ext_animates(void)
{
    switch (s_screen) {
    case SCR_GIBSON:
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_SCAN:
    case SCR_TERMINAL:
    case SCR_GCHAT:
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
    case SCR_WIFI_RADAR:
    case SCR_WIFI_LIST: return 0;
    case SCR_BT_RADAR:
    case SCR_BT_LIST: return 1;
    case SCR_TERMINAL: return s_term;
    case SCR_GCHAT: return -1;
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
static void bot_block(Gfx g, int cx, int cy, int s, uint8_t col, bool blink);

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
    if (sbox_on()) return COL_DEEP;
    if (s_screen == SCR_GIBSON) return botcorps_pal_hub_of(COL_FIELD);
    if (!wants_rain()) return COL_BG;
    return COL_FIELD;
}

static void draw_btn(Gfx g, int x, int y, int id, bool sel)
{
    /* v1016f-2 #1: hub buttons are glowing filled plates like the mock, not outlines. */
    const bool hub = s_screen == SCR_GIBSON;
    const uint8_t glow = (s_color == 1) ? (uint8_t)PAL_WASH_A
                        : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
    const uint8_t edge = sel ? accent() : hub ? accent() : COL_LINE;
    const uint8_t fill = hub ? COL_SEL : (sel ? COL_SEL : COL_DEEP); /* v1016j: lit cyan plates */
    if (hub) {
        /* v1016g: brighter lit plate like the mock (halo + filled accent face) */
        g.fillRect(x - 3, y - 3, BS + 6, BS + 6, glow);
        g.fillRect(x - 1, y - 1, BS + 2, BS + 2, glow);
    }
    GATE_BOX(g, "BTN", hub ? "hubnav" : "btn", x, y, BS, BS, "-");
    plate(g, x, y, BS, BS, fill, edge);
    if (hub) {
        g.drawRect(x, y, BS, BS, accent());
        g.drawRect(x + 1, y + 1, BS - 2, BS - 2, accent());
    }
    if (sel) g.drawRect(x + 1, y + 1, BS - 2, BS - 2, accent());
    if (hub && sel) g.drawRect(x - 1, y - 1, BS + 2, BS + 2, PAL_WHITE);
    draw_icon(g, x, y, id, (sel || hub) ? (uint8_t)PAL_WHITE : COL_MUTED);
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

static bool rain_static(void);
static void layout_covers(void)
{
    cover_reset(false);
    if (rain_static()) {           /* v1016k: static rain under opaque panels, never stepped */
        botcorps_rain_set_covers(s_box, 0, false);
        return;
    }
    switch (s_screen) {
    case SCR_RAIN:
        cover_add(104, 44, 113, 113);
        break;
    case SCR_GIBSON:
        cover_add(4, 18, 36, 170);
        cover_add(280, 18, 36, 170);
        cover_add(110, 20, 100, 140);
        cover_add(38, 164, 244, 38);
        cover_add(90, 152, 140, 14);
        cover_add(98, 200, 124, 40);   /* lit nav plates + glow halo */
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
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_SCAN:
        break;                 /* v1016k: static rain layer, no stepping on the scan box */
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        cover_add(0, 0, 320, 24);
        cover_add(8, 26, 304, 182);
        cover_add(0, 209, 320, 31);
        cover_btn(286, 206);
        break;
    case SCR_CONTROL:
        cover_add(0, 0, 320, 17);          /* top chrome */
        cover_add(8, 20, 304, 216);        /* matte-black board (SB_BX/BY/BW/BH): rain frames it, never paints over it */
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
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: return 0;
    case SCR_SCAN: return 2;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST: return 4;
    case SCR_CONTROL: return CP_N + 2;
    case SCR_DIAG: return 1;
    default: return 0;
    }
}

static void partial_int(void);
static void scan_all_set(bool on);

static void enter_screen(int id, int ret)
{
    if (id < 0) return;
    /* v1016: the Wi-Fi / BLE lists live inside the scan box. No full-screen switch. */
    if (id == SCR_WIFI_LIST) id = SCR_WIFI_RADAR;
    if (id == SCR_BT_LIST) id = SCR_BT_RADAR;
    /* v1016f root fix: SCR_SCAN is the scan box too. Radio boards run live. */
    if (id == SCR_SCAN && s_scan_bot == 0) id = SCR_WIFI_RADAR;
    if (id == SCR_SCAN && s_scan_bot == 1) id = SCR_BT_RADAR;
    const char *from = screen_name(s_screen);
    const int before = card_bot();
    s_ret = ret;
    s_screen = id;
    s_cursor = 0;
    s_hub_zone = 0;
    s_sheet = false;
    s_focus_cmds = false; s_cmd_btn = false;
    s_term_crown = false;
    scan_all_set(false);
    botcorps_lut_term(false);
    s_bus_t0 = millis();
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_gloss = -1;
    if (sbox_id(id)) {
        s_hunt_dev = -1;
        s_hunt_len = 0;
        s_hunt_tS = 0;
        s_hunt_force = 0;
        memset(s_dev_tag, 0, sizeof(s_dev_tag));
        memset(s_dev_ign, 0, sizeof(s_dev_ign));
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

/* Bit 8 lives in s_filter so the mode flag does not grow .bss. */
static bool scan_all_on(void)
{
    return (s_filter & 0x100) != 0;
}

static void scan_all_set(bool on)
{
    if (on) s_filter |= 0x100;
    else s_filter &= ~0x100;
}

/* Sheet colours, or the scan-all frame. Not called from inside a paint. */
static void lut_for_term(void)
{
    botcorps_lut_term(s_screen == SCR_TERMINAL && (s_sheet || scan_all_on()));
}

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
    /* A new command leaves scan-all. The scan-all branch turns it back on.
     * The term lut is not touched here. The full repaint installs it. */
    scan_all_set(false);

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
            log_add_bot(i, row);
        }
    } else if (strcmp(cmd, "clear") == 0) {
        log_clear();
    } else if (strcmp(cmd, "scan") == 0 || strcmp(cmd, "hunt") == 0) {
        if (strcmp(cmd, "scan") == 0 && arg && strcmp(arg, "all") == 0) {
            /* Crew sweep stays on the terminal. It does not open a bot screen.
             * Mode only. The next full repaint installs the term lut. */
            scan_all_set(true);
            s_dirty = true;
            s_mood_until = millis() + 160u;
            log_add_bot(12, "crew sweep - 12 boards");
            log_add("scan ################ 100%");
            log_add_bot(0, "cafe-guest   -42 dBm");
            log_add_bot(0, "printer-2f   -67 dBm");
            log_add_bot(1, "earbuds-58   -58 dBm");
            log_add("[ ok ] all 12 boards swept - 6 new");
            found_add("cafe-guest");
            found_add("printer-2f");
            found_add("earbuds-58");
            found_add("watch-6");
            found_add("lobby-ap");
            found_add("speaker-7");
            return;
        }
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
            s_mood_until = millis() + 160u;
            char row[80];
            snprintf(row, sizeof(row), "%s %s", strcmp(cmd, "hunt") == 0 ? "hunting" : "scanning",
                     BOTS[who].short_name);
            log_add_bot(who, row);
            if (BOTS[who].radio == 1) {
                for (int i = 0; i < 3; ++i) {
                    snprintf(row, sizeof(row), "  %s %s", NEAR_WIFI[i], NEAR_WIFI_R[i]);
                    log_add_bot(who, row);
                    found_add(NEAR_WIFI[i]);
                }
            } else if (BOTS[who].radio == 2) {
                for (int i = 0; i < 3; ++i) {
                    snprintf(row, sizeof(row), "  %s %s", NEAR_BLE[i], NEAR_BLE_R[i]);
                    log_add_bot(who, row);
                    found_add(NEAR_BLE[i]);
                }
            } else {
                snprintf(row, sizeof(row), "  frames %d", 240 + who * 17);
                log_add_bot(who, row);
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
        else if (strcmp(arg, "wifi") == 0) { s_scan_bot = 0; id = SCR_SCAN; }   /* v1016n: wifibot's scan screen (live box) */
        else if (strcmp(arg, "ble") == 0) { s_scan_bot = 1; id = SCR_SCAN; }    /* v1016n: bluetoothbot's scan screen */
        else if (strcmp(arg, "control") == 0) id = SCR_CONTROL;
        else if (strcmp(arg, "diag") == 0) id = SCR_DIAG;
        else if (strcmp(arg, "bots") == 0) id = SCR_TERMINAL;                   /* v1016n: the 12 ESP32 chip tiles */
        else log_add("no such screen");
        if (id >= 0) {
            /* v1016n: "go bots" from the terminal is the terminal itself; never make it its own back target */
            enter_screen(id, id == SCR_TERMINAL ? SCR_GIBSON : SCR_TERMINAL);
            return;
        }
    } else if (strcmp(cmd, "cmds") == 0) {
        s_sheet = true;
        s_sheet_i = 0;
        s_focus_cmds = false; s_cmd_btn = false;
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
    /* Select stays on the terminal and scans that board in place. */
    if (index < 0 || index > 11) return;
    /* mock termOpen: a Terminal board opens THAT bot's own scan screen */
    s_scan_bot = index;
    enter_screen(SCR_SCAN, SCR_TERMINAL);
}

static void boot_log(void)
{
    if (s_log_n > 0) return;
    /* Idle scrollback matches the v3.2 boot, then the deauth scan it starts. */
    log_add("BOTcorps term 0.9  12 boards linked");
    log_add("[ ok ] rf core calibrated");
    log_add("[ ok ] bot roster loaded");
    log_add("scan deauth");
    log_add("scanning deauth ...");
    log_add("frames 362  hits 8");
}

static uint8_t term_accent(void);
static uint8_t term_low(void);
static uint8_t glow_of(int step);

static void hub_gibson(Gfx g, int cx, int cy, int z);

static void paint_one_board(Gfx g, int i, bool sel, bool dim)
{
    int x, y;
    term_xy(i, &x, &y);
    /* v3.2 boards are the dark die. Radio no longer lifts a cream tile. */
    const bool wh = false;
    const uint8_t ac = accent();
    const uint32_t t = millis();
    const int w = 72;
    const int h = 46;
    if (sel && !dim) {
        const uint8_t wash = (s_color == 1) ? (uint8_t)PAL_WASH_A
                             : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
        const uint8_t wire = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                             : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        /* Cyan dashes are the select mark. The rim stays wash and wire. */
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
    const uint8_t silk = wh ? (sel ? (uint8_t)PAL_BLACK : (uint8_t)PAL_LINE)
                            : (sel ? (uint8_t)PAL_WHITE : (uint8_t)PAL_MUTED);
    const uint8_t led = wh ? (uint8_t)PAL_CHIN : (uint8_t)PAL_BOARD_D;
    const uint8_t hole = (uint8_t)PAL_FIELD;
    g.fillRect(x + 1, y, w - 2, h, base);
    g.fillRect(x, y + 1, w, h - 2, base);
    g.fillRect(x + 1, y, w - 2, 1, edge);
    g.fillRect(x + 1, y + h - 1, w - 2, 1, edge);
    g.fillRect(x, y + 1, 1, h - 2, edge);
    g.fillRect(x + w - 1, y + 1, 1, h - 2, edge);
    const bool traces = !dim;
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
    const int bob = sel ? -((int)((t / 450u) % 2u)) : 0;
    /* v1017b (Dirt 8:20): every terminal tile is the SQUARE chip-form block (the
     * same block figure Gibson's control-panel / scan-board chip uses), in this
     * bot's fixed colour on black. Never the round sprite. Terminal tiles only. */
    bot_block(g, x + 36, y + 25 + bob, 16, bot_color(i), sel && sprite_blink(i));
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
    /* v1016f-2 #5: the 12th board reads SAMSUNGTAG, abbreviated to fit the tile. */
    const char *lbl = (i == 11) ? "SAMSUNGTAG" : up;
    {
        const int keep = s_px;
        s_px = 6;
        const uint8_t lab = dim ? (uint8_t)PAL_STEEL : silk;
        txt(g, lbl, x + 36, y + 37, lab, lgfx::textdatum::top_center);
        s_px = keep;
    }
    /* Faint Wi-Fi arcs on the right of the WIFI tile. Wire slot, not the accent. */
    if (BOTS[i].radio == 1 && !dim) {
        const uint8_t m = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                          : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        const int ax = x + 54;
        const int ay = y + 26;
        g.fillRect(ax + 1, ay + 2, 2, 2, m);
        g.fillRect(ax, ay - 1, 1, 1, m);
        g.fillRect(ax + 2, ay - 2, 2, 1, m);
        g.fillRect(ax + 4, ay - 1, 1, 1, m);
        g.fillRect(ax + 5, ay + 1, 1, 2, m);
        g.fillRect(ax - 1, ay - 4, 1, 1, m);
        g.fillRect(ax + 1, ay - 5, 3, 1, m);
        g.fillRect(ax + 4, ay - 4, 1, 1, m);
        g.fillRect(ax + 6, ay - 2, 1, 2, m);
        g.fillRect(ax + 7, ay, 1, 2, m);
    }
}

static void paint_boards(Gfx g)
{
    /* v1016f-2 #2: only the focused board is full-bright; the rest are dimmed, like the mock. */
    for (int i = 0; i < 12; ++i) {
        const bool sel = (i == s_term) && !s_focus_cmds && !s_cmd_btn;
        paint_one_board(g, i, sel, !sel);
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
/* v1017a: the prompt-line patch rect (inside the plate side borders, row 226..234) */
static const int PL_X = 5, PL_Y = PROMPT_Y - 1, PL_W = 310, PL_H = 9;
static void draw_prompt_line_part(Gfx g);
static void blit_ico(Gfx g, const uint8_t *bits, int x, int y, uint8_t col);

static bool sheet_exact(void)
{
    return s_screen == SCR_TERMINAL && s_sheet;
}

static uint8_t term_accent(void)
{
    /* Shared slot only. Header and the prompt plate call this outside
     * the sheet, so it must never be an override code. */
    return accent();
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

/* 6x6 corner. (sx, sy) point inward from the outer pixel. */
static void sheet_corner(Gfx g, int x, int y, int sx, int sy)
{
    const uint8_t corn[6][6] = {
        { PAL_TERM_M35, PAL_TERM_M, PAL_TERM_M, PAL_TERM_M, PAL_TERM_M, PAL_TERM_M },
        { PAL_TERM_M, PAL_TERM_M10, PAL_TERM_M10, PAL_TERM_M35, PAL_TERM_M35, PAL_TERM_M35 },
        { PAL_TERM_M, PAL_TERM_M10, PAL_TERM_M10, PAL_TERM_M22, PAL_TERM_M22, PAL_TERM_M22 },
        { PAL_TERM_M, PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M18, PAL_TERM_M18 },
        { PAL_TERM_M, PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M10, PAL_TERM_M10 },
        { PAL_TERM_M, PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M10, PAL_TERM_M10 },
    };
    for (int dy = 0; dy < 6; ++dy) {
        for (int dx = 0; dx < 6; ++dx)
            g.fillRect(x + sx * dx, y + sy * dy, 1, 1, corn[dy][dx]);
    }
}

/* Ramp A, edge inward: 230, 235, 232, 234, 236, 236.
 * Top rows and the side columns. The 6x6 corner table replaces the
 * square corner. The bottom edge is painted later, after the hairline. */
static void sheet_ramp_top(Gfx g, int x, int y, int w, int h)
{
    const uint8_t ramp[6] = {
        PAL_TERM_M, PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M10, PAL_TERM_M10
    };
    const int x1 = x + w;
    const int y1 = y + h;
    for (int i = 0; i < 6; ++i) g.fillRect(x, y + i, w + 1, 1, ramp[i]);
    const int tall = (y1 - 6) - y + 1;
    for (int i = 0; i < 6; ++i) {
        g.fillRect(x + i, y, 1, tall, ramp[i]);
        g.fillRect(x1 - i, y, 1, tall, ramp[i]);
    }
    sheet_corner(g, x, y, 1, 1);
    sheet_corner(g, x1, y, -1, 1);
    sheet_corner(g, x, y1, 1, -1);
    sheet_corner(g, x1, y1, -1, -1);
}

static void paint_term_icons(Gfx g)
{
    const uint8_t off = PAL_LINE;
    /* Crown sits in the header, outside the sheet. Shared slots only. */
    GATE_BOX(g, "CROWN", "back", 203, 0, 24, 16, "-");
    g.drawRect(203, 0, 24, 16, PAL_WASH_M);
    g.drawRect(204, 0, 22, 16, PAL_WIRE_M);
    g.fillRect(205, 1, 20, 14, PAL_BG);
    g.drawRect(205, 1, 20, 14, PAL_R_E2FF);
    g.drawRect(206, 2, 18, 12, PAL_WIRE_M);
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
    if (s_term_crown) {
        g.drawRect(201, 0, 28, 17, PAL_WHITE);
        g.drawRect(202, 0, 26, 16, accent());
    }
    const int sg = 3 + (int)((millis() / 2500u) % 2u);
    for (int k = 0; k < 4; ++k) {
        const int h = (k + 1) * 2;
        g.fillRect(236 + k * 4, 12 - h, 3, h, k < sg ? accent() : off);
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
    const bool rx = ((millis() / 700u) % 5u) != 4u;
    g.fillRect(290, 6, 4, 4, rx ? (uint8_t)PAL_R_E2FF : off);
    const int keep = s_px;
    s_px = 8;
    txt(g, "RX", 298, 4, rx ? accent() : (uint8_t)COL_MUTED, lgfx::textdatum::top_left);
    s_px = keep;
}

static void paint_term_head(Gfx g)
{
    g.fillRect(0, 0, 320, 16, PAL_BLACK);
    {
        const uint8_t mid = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                            : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        g.fillRect(0, 16, 320, 2, accent());
        g.fillRect(0, 18, 320, 1, mid);
        /* Rolling scanline on the accent bar. Shared slots only. */
        const int sx = (int)((millis() / 18u) % 300u);
        g.fillRect(sx, 16, 28, 2, PAL_WHITE);
    }
    /* Header sits outside the sheet, so this glow stays on reserved slots. */
    {
        const int keep = s_px;
        s_px = 8;
        const uint8_t sh = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                           : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        txt(g, "terminal", 9, 5, sh, lgfx::textdatum::top_left);
        txt(g, "terminal", 8, 4, accent(), lgfx::textdatum::top_left);
        s_px = keep;
    }
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
    /* a. Under the sheet is shared slot 8, then each row's own background. */
    GATE_BOX(g, "OCCLUDE", "sheet", bx, by, w + 1, h + 1, "-");
    g.fillRect(bx + 1, by + 1, w - 1, h - 1, PAL_BG);
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + r * rh;
        g.fillRect(bx + 4, y, w - 8, rh - 1, PAL_BG);
    }
    /* b. Top rows and side columns, then the 6-step corners. */
    const int x1 = bx + w;
    const int y1 = by + h;
    sheet_ramp_top(g, bx, by, w, h);
    /* c. CORE glow after the ramp, so it keeps a full 5 px inside x17..49. */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr < 0) continue;
        const int y = by + 5 + r * rh;
        if (hdr == 0) {
            const uint8_t ringc[5] = {
                PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M10, PAL_TERM_M10
            };
            const int gx = bx + 8;
            const int gy = y + 1;
            const int keepf = s_px;
            s_px = 6;
            for (int d = 5; d >= 1; --d) {
                const uint8_t c = ringc[d - 1];
                for (int dy = -d; dy <= d; ++dy) {
                    for (int dx = -d; dx <= d; ++dx) {
                        const int ax = dx < 0 ? -dx : dx;
                        const int ay = dy < 0 ? -dy : dy;
                        if ((ax > ay ? ax : ay) != d) continue;
                        txt(g, G_NAME[hdr], gx + dx, gy + dy, c, lgfx::textdatum::top_left);
                    }
                }
            }
            txt(g, G_NAME[hdr], gx, gy, PAL_TERM_M, lgfx::textdatum::top_left);
            s_px = keepf;
            /* The glow copy reaches x17. The top-left grid keeps x14..19. */
            sheet_corner(g, bx, by, 1, 1);
        } else {
            glow_txt(g, G_NAME[hdr], bx + 8, y + 1, PAL_BG, 6);
        }
    }
    /* d. CORE rule: one pixel of 232. Other groups keep their rule. */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr < 0) continue;
        const int y = by + 5 + r * rh;
        if (hdr == 0) g.fillRect(bx + 48, y + 5, w - 60, 1, PAL_TERM_M22);
        else glow_hline(g, bx + 48, y + 5, w - 60, STEP_RULE);
    }
    /* e. Hairline, one pixel of 231. */
    g.fillRect(bx + 6, y1 - 4, w - 12, 1, PAL_TERM_M28);
    /* f. Bottom ramp. The hairline keeps x20..299 at y218. */
    g.fillRect(bx + 1, y1, w - 1, 1, PAL_TERM_M);
    g.fillRect(bx + 3, y1 - 1, w - 5, 1, PAL_TERM_M35);
    g.fillRect(bx + 3, y1 - 2, w - 5, 1, PAL_TERM_M22);
    g.fillRect(bx + 3, y1 - 3, w - 5, 1, PAL_TERM_M18);
    g.fillRect(x1 - 6, y1 - 4, 1, 1, PAL_TERM_M10);
    g.fillRect(bx + 5, y1 - 5, w - 9, 1, PAL_TERM_M10);
    /* Inner halo on pixels the ramp does not own. Corners, the 6px
     * ramp, the rules, and the CORE column stay as painted above. */
    {
        int hdr0 = -1;
        int cmd0 = 0;
        menu_row(top, &hdr0, &cmd0);
        if (hdr0 >= 0) {
            g.fillRect(bx + 40, by + 6, w - 70, 1, PAL_TERM_M22);
            g.fillRect(bx + 40, by + 7, w - 70, 1, PAL_TERM_M18);
            g.fillRect(bx + 40, by + 8, w - 70, 1, PAL_TERM_M10);
        }
        for (int r = 0; r < nshow; ++r) {
            int hdr = -1;
            int cmd = 0;
            menu_row(top + r, &hdr, &cmd);
            if (hdr >= 0) continue;
            const int y = by + 5 + r * rh;
            /* The 6-row ramp owns by..by+5. A scrolled command on the
             * first row would otherwise paint y109. Start at by+6. */
            int ys = y;
            int hs = rh - 1;
            if (ys < by + 6) {
                const int cut = (by + 6) - ys;
                ys += cut;
                hs -= cut;
            }
            if (hs <= 0) continue;
            g.fillRect(bx + 6, ys, 1, hs, PAL_TERM_M22);
            g.fillRect(bx + 7, ys, 1, hs, PAL_TERM_M18);
            g.fillRect(bx + 8, ys, 1, hs, PAL_TERM_M10);
            g.fillRect(x1 - 7, ys, 1, hs, PAL_TERM_M18);
            g.fillRect(x1 - 6, ys, 1, hs, PAL_TERM_M22);
        }
        g.fillRect(bx + 8, y1 - 8, w - 28, 1, PAL_TERM_M10);
        g.fillRect(bx + 8, y1 - 7, w - 28, 1, PAL_TERM_M18);
        g.fillRect(bx + 8, y1 - 6, w - 16, 1, PAL_TERM_M22);
    }
    /* g. Selected row clips the ramp on that row. The bar stays whole. */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0 || cmd != sel) continue;
        const int y = by + 5 + r * rh;
        g.fillRect(bx + 3, y - 1, w - 6, rh - 1, PAL_TERM_M18);
        g.fillRect(bx + 3, y - 1, 2, rh - 1, PAL_TERM_M);
    }
    /* h. Text and arrows. Selected command text is shared slot 127. */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1;
        int cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + r * rh;
        const bool on = cmd == sel;
        const uint8_t bg = on ? (uint8_t)PAL_TERM_M18 : (uint8_t)PAL_BG;
        const uint8_t ac = accent();
        blit_ico(g, CMD_ICO[cmd], bx + 10, y + 1, on ? (uint8_t)PAL_TERM_M : ac);
        const uint8_t name_c = on ? (uint8_t)127 : (uint8_t)PAL_TERM_INK;
        if (on) {
            txt(g, CMDN[cmd], bx + 25, y + 2, PAL_TERM_C, lgfx::textdatum::top_left);
            txt(g, CMDN[cmd], bx + 23, y, PAL_CYAN, lgfx::textdatum::top_left);
        }
        txt_bg(g, CMDN[cmd], bx + 24, y + 1, name_c, bg, lgfx::textdatum::top_left);
        const uint8_t pur = on ? (uint8_t)127
                             : (s_color == 1) ? (uint8_t)PAL_AMBER
                             : (s_color == 2) ? (uint8_t)PAL_MINT : (uint8_t)PAL_TERM_C;
        draw_purpose(g, CMDP[cmd], bx + w - 8, y + 1, pur, bg);
    }
    /* 8px icons. x293..300 stays off the x301 ramp. y is 110..117 and 208..215. */
    const int ax = x1 - 13;
    if (top > 0) blit_ico(g, ARROW_UP, ax, by + 6, COL_MUTED);
    if (top + nshow < rows) blit_ico(g, ARROW_DN, ax, y1 - 14, COL_MUTED);
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

/* "scan #### 100%" becomes the v3.2 16-block bar. Label and percent stay text. */
static bool paint_hash_bar(Gfx g, const char *ln, int x, int y, uint8_t ink)
{
    const char *hash = ln ? strchr(ln, '#') : 0;
    if (!hash) return false;
    int n = (int)(hash - ln);
    while (n > 0 && ln[n - 1] == ' ') --n;
    if (n <= 0 || n > 12) return false;
    char label[13];
    for (int i = 0; i < n; ++i) label[i] = ln[i];
    label[n] = 0;
    const char *pct = strrchr(ln, '%');
    int pr = 100;
    if (pct) {
        const char *p = pct;
        while (p > ln && p[-1] >= '0' && p[-1] <= '9') --p;
        if (p < pct) {
            pr = 0;
            while (p < pct) { pr = pr * 10 + (*p - '0'); ++p; }
        }
    }
    if (pr < 0) pr = 0;
    if (pr > 100) pr = 100;
    txt_bg(g, label, x, y, ink, PAL_BLACK, lgfx::textdatum::top_left);
    const int bx = x + n * 6 + 4;
    const int by = y + 1;
    const int lit = (pr * 16 + 50) / 100;
    /* TERM magenta only while scan-all mode is up, and the blocks sit
     * inside the sheet rect. Unlit is a base-lookup dark, not an override. */
    const uint8_t on = scan_all_on() ? (uint8_t)PAL_TERM_M : accent();
    const uint8_t off = (uint8_t)PAL_LINE;
    for (int i = 0; i < 16; ++i)
        g.fillRect(bx + i * 5, by, 4, 6, i < lit ? on : off);
    char tail[8];
    snprintf(tail, sizeof(tail), "%d%%", pr);
    txt_bg(g, tail, bx + 16 * 5 + 4, y, ink, PAL_BLACK, lgfx::textdatum::top_left);
    return true;
}

static void paint_prompt(Gfx g)
{
    const int keep = s_px;
    s_px = 6;
    /* Tiles end at y=165. Deep black plate. Enter pulses the hairline cyan. */
    g.fillRect(4, 168, 312, 68, PAL_BLACK);
    /* Plate chrome is the same with the sheet open or closed. The scan-all
     * frame, if any, is painted later and only inside the sheet rect. */
    {
        /* v1016f-2 #4: the scrollback box border is magenta, like the mock. */
        const bool pulse = s_mood_until != 0 && (int32_t)(millis() - s_mood_until) < 0;
        g.fillRect(4, 167, 312, 1, pulse ? (uint8_t)PAL_WHITE : (uint8_t)PAL_TERM_M);
    }
    {
        const int span = 46;
        int sy = 171 + (int)((millis() / 28u) % (uint32_t)span);
        if (sy > 223) sy = 223;
        g.fillRect(8, sy, 248, 1, (s_color == 1) ? (uint8_t)PAL_WIRE_A
                                       : (s_color == 2) ? (uint8_t)PAL_WIRE_M
                                                        : (uint8_t)PAL_WIRE_C);
    }
    g.fillRect(4, 235, 312, 1, PAL_TERM_M);
    g.fillRect(4, 168, 1, 68, PAL_TERM_M);
    g.fillRect(315, 168, 1, 68, PAL_TERM_M);
    /* v1016f #9: gchat echoes ("gibson> ...") share the log but never show in the Terminal. */
    int pick[6];
    int n = 0;
    for (int i = s_log_n - 1; i >= 0 && n < 6; --i) {
        const char *l = log_line(i);
        if (l && strncmp(l, "gibson>", 7) == 0) continue;
        pick[n++] = i;
    }
    for (int a = 0, b = n - 1; a < b; ++a, --b) { const int t = pick[a]; pick[a] = pick[b]; pick[b] = t; }
    const int lx = scan_all_on() ? 20 : 10;
    for (int k = 0; k < n; ++k) {
        const char *ln = log_line(pick[k]);
        const int ly = 171 + k * 8;
        const uint8_t pale = botcorps_pal_fit(0xBFBF);   /* v1016k (SPEC T1): pale cyan log (#bff6ff) */
        uint8_t ink = pale;
        const char *show = ln;
        if (ln && ln[0] == '\x01') {
            const int b = (unsigned char)ln[1];
            ink = b >= 12 ? (uint8_t)COL_IVORY : bot_color(b);
            show = ln + 2;
        } else if (ln && ln[0] == '[') {
            const char *rb = strchr(ln, ']');
            if (rb) {
                char tag[12];
                int tn = (int)(rb - ln) + 1;
                if (tn > 11) tn = 11;
                for (int i = 0; i < tn; ++i) tag[i] = ln[i];
                tag[tn] = 0;
                txt_bg(g, tag, lx, ly, (uint8_t)PAL_OK, PAL_BLACK, lgfx::textdatum::top_left);
                if (rb[1]) txt_bg(g, rb + 1, lx + tn * 6, ly, pale, PAL_BLACK,
                                  lgfx::textdatum::top_left);
                continue;
            }
        }
        if (paint_hash_bar(g, show, lx, ly, ink)) continue;
        txt_bg(g, show, lx, ly, ink, PAL_BLACK, lgfx::textdatum::top_left);
    }
    const int y = PROMPT_Y;
    s_px = 6;
    {
        txt(g, "user@botcorps", 8, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", 10, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", 9, y + 1, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", 9, y, (uint8_t)194, lgfx::textdatum::top_left);
    }
    txt(g, ":~$", 10 + 13 * 6 + 1, y, accent(), lgfx::textdatum::top_left);
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
    if ((millis() / 530u) % 2u == 0) g.fillRect(cx, y, 6, 8, accent());
    paint_cands(g);
    const bool po = (s_focus_cmds || s_cmd_btn) && !s_sheet;
    plate(g, 250, 214, 20, 12, po ? COL_SEL : COL_DEEP, po ? accent() : COL_LINE);
    for (int k = 0; k < 3; ++k) g.fillRect(255, 217 + k * 3, 10, 1, po ? accent() : COL_MUTED);
    if (s_focus_cmds && !s_sheet) paint_palette(g);
    /* Companion box, bottom right. Same place with the sheet open or closed. */
    {
        const int bx = 274;
        const int by = 174;
        const uint8_t edge = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                             : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
        g.fillRect(bx, by, 38, 60, PAL_BG);
        g.fillRect(bx, by, 38, 1, edge);
        g.fillRect(bx, by + 59, 38, 1, COL_LINE);
        g.fillRect(bx, by, 1, 60, COL_LINE);
        g.fillRect(bx + 37, by, 1, 60, COL_LINE);
        const int bot = s_term >= 0 && s_term <= 11 ? s_term : 0;
        draw_blob(g, bx + 19, by + 30, 2, bot_color(bot), 0, false);
        g.fillRect(bx + 9, by + 48, 20, 1, edge);
    }
    if (scan_all_on() && !s_sheet) {
        /* TERM override codes, inside x14..306, y167..222 only. Prompt is y=227.
         * No lut call here. The full repaint already installed the table. */
        g.drawRect(14, 167, 293, 56, PAL_TERM_M);
        g.drawRect(15, 168, 291, 54, PAL_TERM_M35);
        g.drawRect(16, 169, 289, 52, PAL_TERM_M22);
    }
    s_px = keep;
}

static void paint_terminal(Gfx g)
{
    paint_term_head(g);
    paint_boards(g);
    paint_prompt(g);
    if (s_sheet) paint_sheet(g);
}

static void gib_crown(Gfx g, int cx, int y);
/* v1016k (SPEC X1): the crown is the ONLY back control. Screens that had a back icon
 * keep the same cursor slot, now drawn as the crown at the top centre. */
static void crown_btn(Gfx g, bool sel)
{
    g.fillRect(140, 0, 40, 20, PAL_BLACK);
    gib_crown(g, 160, 3);
    if (sel) {
        g.drawRect(141, 0, 38, 20, PAL_WHITE);
        g.drawRect(142, 1, 36, 18, accent());
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

#ifdef BOTCORPS_HOST_RENDER
/* Host proofs rotate the existing bar triple. Firmware leaves this at 0. */
static int s_host_sig = 0;
#endif

/* Top row level at y=55 (dock 31). Bottom row level at y=180 (dock 32).
 * Side icons sit on the lane by the existing bar count:
 * s = clamp(28 + (1 - bars/4) * (L-42), 27, L-11). */
static int bus_dock(const bus_pt *p, int n, int L, int lane, bool wifi)
{
    (void)p;
    (void)n;
    if (lane >= 6 && lane <= 8) return 31;
    if (lane >= 9 && lane <= 11) return 32;
    int bi = lane % 3;
#ifdef BOTCORPS_HOST_RENDER
    bi = (lane + s_host_sig) % 3;
#endif
    const uint8_t *barsrc = wifi ? NEAR_WIFI_B : NEAR_BLE_B;
    int bars = barsrc[bi];
    if (bars > 4) bars = 4;
    if (bars < 0) bars = 0;
    int span = L - 42;
    if (span < 0) span = 0;
    int s = 28 + ((4 - bars) * span) / 4;
    if (s < 27) s = 27;
    if (s > L - 11) s = L - 11;
    return s;
}

/* Scan: 3400 ms. Out to 11 px short of the chip, wait, back to the hub.
 * Radar keeps the older t/16 sweep (period 3600). */
static int bus_scout_s(int L, uint32_t t, int lane, int period, int dock, int dmax)
{
    if (period == 3400) {
        int tgt = dock - 11;
        if (tgt < 8) tgt = 8;
        if (dmax < 1) dmax = 1;
        int l = (int)(t % 3400u) - lane * 68;
        int to = 204 + (1020 * (tgt - 8)) / dmax;
        if (to < 1) to = 1;
        const int dw = 170;
        int s = 8;
        if (l >= 0 && l < to) s = 8 + (tgt - 8) * l / to;
        else if (l >= to && l < to + dw) s = tgt;
        else if (l >= to + dw && l < to + dw + to)
            s = tgt - (tgt - 8) * (l - to - dw) / to;
        if (s > tgt) s = tgt;
        if (s < 8) s = 8;
        return s;
    }
    if (period < 2) period = 2;
    const uint32_t phase = (t / 16u + (uint32_t)lane * 37u) % (uint32_t)period;
    const int half = period / 2;
    const int u = (int)phase < half ? (int)phase : period - (int)phase;
    int span = L - 16;
    if (span < 1) span = 1;
    return 8 + span * u / half;
}

#if BOTCORPS_FRAME_MODE == FRAME
static bool box_hit(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void scout_box(int cx, int cy, int *x, int *y, int *w, int *h)
{
    *x = cx - 4;
    *y = cy - 4;
    *w = 9;
    *h = 9;
}
#endif

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

/* 56px die. Scale 6 of the 7px sprite is 42px, a 7px margin.
 * 90b1602 drew the hub portrait at z=9 (63px). That scale does not fit
 * between these traces, and that commit's bug was the INT strip blanking
 * the card, not this sprite. */
/* v1016f-3 (Dirt 8:02/8:06): THE one shared ESP32 chip block. Every bot/Gibson
 * chip on every screen draws through this: a square chip window with a BLACK
 * face and black control-panel border + steel castellated pins, the bot (or
 * Gibson) filling the square and keeping its fixed colour. bot 12 = Gibson. */
/* v1016k (Dirt 9:32): Gibson as a SQUARE BLOCK figure for chips inside boards
 * (scan/settings/game). Full square face, light top, dark bottom, black visor
 * band with two steel eyes. Round dome (hub_gibson) stays only in the hub rings. */
static void gib_block(Gfx g, int cx, int cy, int s)
{
    const int x = cx - s / 2, y = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    g.fillRect(x, y, s, s, PAL_IVORY);               /* full square face */
    g.fillRect(x, y, s, lip, PAL_WHITE);             /* lit top row */
    g.fillRect(x, y + s - lip, s, lip, PAL_CHIN);    /* dark bottom row */
    const int vh = (s * 26) / 100 > 2 ? (s * 26) / 100 : 2;
    const int vy = y + (s * 34) / 100;
    g.fillRect(x, vy, s, vh, PAL_BLACK);             /* visor band */
    const int ew = s / 5 > 2 ? s / 5 : 2;
    const int gap = s / 4 > ew ? s / 4 : ew;
    const int gl = (int)((millis() / 900u) % 2u);
    const int eyc = vy + (vh - ew) / 2;
    g.fillRect(x + gap + gl, eyc, ew, ew, PAL_STEEL);
    g.fillRect(x + s - gap - ew + gl, eyc, ew, ew, PAL_STEEL);
}

static void hub_gibson(Gfx g, int cx, int cy, int z);
/* dome=true keeps the intro ID board's approved dome Gibson (mock intro v11); the square
 * block figure is only for Gibson's scan board + game boards (SPEC C-items). */
static void chip_block_ex(Gfx g, int cx, int cy, int S, int bot, bool dome);
static void chip_block(Gfx g, int cx, int cy, int S, int bot) { chip_block_ex(g, cx, cy, S, bot, false); }
static void chip_block_ex(Gfx g, int cx, int cy, int S, int bot, bool dome)
{
    const int x = cx - S / 2, y = cy - S / 2;
    GATE_BOX(g, "CHIP", bot >= 12 ? "gibson" : "bot", x, y, S, S, "-");
    g.fillRect(x + 3, y + 3, S - 6, S - 6, PAL_BG);     /* black face */
    g.drawRect(x - 1, y - 1, S + 2, S + 2, PAL_STEEL);   /* v1016f-4: silk outline reads on black PCB */
    g.drawRect(x, y, S, S, PAL_BLACK);                   /* black outer border */
    g.drawRect(x + 2, y + 2, S - 4, S - 4, PAL_LINE);
    g.drawRect(x + 6, y + 6, S - 12, S - 12, PAL_DIAG);
    for (int i = 8; i <= S - 10; i += 8) {               /* steel pins */
        g.fillRect(x - 1, y + i, 5, 2, PAL_STEEL);
        g.fillRect(x + S - 4, y + i, 5, 2, PAL_STEEL);
        g.fillRect(x + i, y - 1, 2, 5, PAL_STEEL);
        g.fillRect(x + i, y + S - 4, 2, 5, PAL_STEEL);
    }
    g.fillRect(x + 7, y + 7, 2, 2, PAL_IVORY);
    if (bot >= 12 && dome) {
        const int z = S >= 50 ? 5 : S >= 36 ? 4 : S >= 24 ? 3 : 2;
        hub_gibson(g, cx, cy, z);
        return;
    }
    if (bot >= 12) {
        const int gs = S - 12 > 8 ? S - 12 : 8;   /* square Gibson fills the chip face (v1016k: >=60% of the square) */
        GATE_BOX(g, "SPR", "gibson_block", cx - gs / 2, cy - gs / 2, gs, gs, "-");
        gib_block(g, cx, cy, gs);
        return;
    }
    const int shape = botcorps_shape_for(bot);
    static const int bw[4] = { 7, 9, 7, 5 }, bh[4] = { 7, 6, 9, 5 };
    const int m = S - 14 > 6 ? S - 14 : 6;
    int sc = 1;
    while ((sc + 1) * bw[shape] <= m && (sc + 1) * bh[shape] <= m) ++sc;
    draw_blob(g, cx, cy, sc, bot_color(bot), shape, false);
}

static void scan_die(Gfx g, int cx, int cy, bool gibson)
{
    int bot = 12;
    if (!gibson) {
        bot = s_bot;
        if (s_screen == SCR_BT_RADAR) bot = 1;
        else if (s_screen == SCR_WIFI_RADAR) bot = 0;
        if (bot < 0) bot = 0;
        if (bot > 11) bot = 11;
    }
    chip_block(g, cx, cy, 56, bot);
}

static uint16_t mix16(uint16_t a, uint16_t b, int num, int den);

/* v1017b (Dirt 8:20): the square chip-form block figure (gib_block geometry: full
 * square face, lit top row, dark bottom row, black visor band, two steel eyes)
 * drawn in a bot's fixed colour on a black square. Used by the terminal tiles. */
static void bot_block(Gfx g, int cx, int cy, int s, uint8_t col, bool blink)
{
    const int x = cx - s / 2, y = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    g.fillRect(x - 1, y - 1, s + 2, s + 2, PAL_BLACK);  /* black behind the block */
    GATE_BOX(g, "SPR", "bot_block", x, y, s, s, "-");
    g.fillRect(x, y, s, s, col);                                                         /* face */
    g.fillRect(x, y, s, lip, botcorps_pal_fit(mix16(botcorps_rgb565(col), 0xFFFF, 45, 100)));      /* lit top */
    g.fillRect(x, y + s - lip, s, lip, botcorps_pal_fit(mix16(botcorps_rgb565(col), 0x0000, 45, 100))); /* dark bottom */
    const int vh = (s * 26) / 100 > 2 ? (s * 26) / 100 : 2;
    const int vy = y + (s * 34) / 100;
    g.fillRect(x, vy, s, vh, PAL_BLACK);             /* visor band */
    const int ew = s / 5 > 2 ? s / 5 : 2;
    const int gap = s / 4 > ew ? s / 4 : ew;
    const int gl = (int)((millis() / 900u) % 2u);
    const int eyc = vy + (vh - ew) / 2;
    const int eh = blink ? 1 : ew, ey = blink ? eyc + ew - 1 : eyc;
    g.fillRect(x + gap + gl, ey, ew, eh, PAL_STEEL);
    g.fillRect(x + s - gap - ew + gl, ey, ew, eh, PAL_STEEL);
}

/* Non-circular bus. radar=false is the Gibson scan (extBus). 0 B heap. */
static void bus_paint(Gfx g, bool radar, bool wifi, int shown, int sel)
{
    const bus_pt (*tab)[4] = radar ? BUS_R : BUS_F;
    const int period = radar ? 3600 : 3400;
    const uint32_t t = millis();
    const int bx = radar ? 50 : 8;
    const int by = 26;
    const int bw = radar ? 220 : 304;
    const int bh = 182;
    const uint8_t ac = accent();
    /* v1016k (SPEC B1): scan boards are permanent matte-black PCB, never the UI colour. */
    const uint8_t pcb = PAL_BLACK;
    const uint8_t pcb_tex = PAL_DEEP;
    const uint8_t pcb_edge = 177;   /* gold board edge, as sb_board */
    g.fillRect(bx, by, bw, bh, pcb);
    for (int tx = bx + 6; tx < bx + bw - 4; tx += 8)
        g.fillRect(tx, by + 2, 1, bh - 4, pcb_tex);
    g.drawRect(bx - 1, by - 1, bw + 2, bh + 2, pcb_edge);
    /* Chrome follows the accent. Reserved slots only, never the sheet codes. */
    const uint8_t spoke = (s_color == 1) ? (uint8_t)PAL_WIRE_A
                          : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
    const uint8_t spoke_glow = (s_color == 1) ? (uint8_t)PAL_WASH_A
                               : (s_color == 2) ? (uint8_t)PAL_WASH_M : (uint8_t)PAL_WASH_C;
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
            const int pulse = 12 + (int)((t / 180u + (uint32_t)d) % 3u);
            g.drawCircle(x, y, pulse, ac);
            g.drawCircle(x, y, pulse + 1, ac);
            g.drawCircle(x, y, 8, ac);
            if (d == sel) g.drawRect(x - 9, y - 9, 18, 18, ac);
        }
    } else {
        for (int d = 0; d < SCAN_N; ++d) {
            const int lane = d;
            const int L = bus_len(tab[lane], BUS_N[lane]);
            int x = 0;
            int y = 0;
            bus_at(tab[lane], BUS_N[lane], bus_dock(tab[lane], BUS_N[lane], L, lane, wifi), &x, &y);
            g.fillRect(x - 7, y - 7, 14, 14, PAL_BG);
            g.drawRect(x - 7, y - 7, 14, 14, COL_LINE);
            dev_icon(g, SCAN_TYPE[d], x - 6, y - 6, false);
        }
    }
    /* Gibson only on the Gibson scan. A bot scan keeps that bot's sprite. */
    scan_die(g, 160, 117, s_screen == SCR_SCAN);
    int docks[12];
    int dmax = 1;
    for (int i = 0; i < 12; ++i) {
        const int L = bus_len(tab[i], BUS_N[i]);
        docks[i] = bus_dock(tab[i], BUS_N[i], L, i, wifi);
        int tgt = docks[i] - 11;
        if (tgt < 8) tgt = 8;
        int span = tgt - 8;
        if (span > dmax) dmax = span;
    }
#ifdef BOTCORPS_HOST_RENDER
    if (!radar) printf("scan-dmax %d wifi %d\n", dmax, wifi ? 1 : 0);
#endif
    for (int i = 0; i < 12; ++i) {
        const int L = bus_len(tab[i], BUS_N[i]);
        int x = 0;
        int y = 0;
        const int s = bus_scout_s(L, t, i, period, docks[i], dmax);
        bus_at(tab[i], BUS_N[i], s, &x, &y);
#if BOTCORPS_FRAME_MODE == FRAME
        if (!radar) {
            const int sp = bus_scout_s(L, t >= 16u ? t - 16u : 0u, i, period, docks[i], dmax);
            int px = 0;
            int py = 0;
            bus_at(tab[i], BUS_N[i], sp, &px, &py);
            int ex = 0;
            int ey = 0;
            int ew = 0;
            int eh = 0;
            scout_box(px, py, &ex, &ey, &ew, &eh);
            g.fillRect(ex, ey, ew, eh, pcb);
            for (int u = 0; u <= L; ++u) {
                int tx = 0;
                int ty = 0;
                bus_at(tab[i], BUS_N[i], u, &tx, &ty);
                if (tx < ex || tx >= ex + ew || ty < ey || ty >= ey + eh) continue;
                g.fillRect(tx + 1, ty + 1, 1, 1, spoke_glow);
                g.fillRect(tx, ty, 1, 1, spoke);
            }
            if (box_hit(ex, ey, ew, eh, 132, 89, 56, 56)) scan_die(g, 160, 117, true);
            for (int d = 0; d < SCAN_N; ++d) {
                int cx = 0;
                int cy = 0;
                bus_at(tab[d], BUS_N[d], docks[d], &cx, &cy);
                const int co = (d == sel) ? 8 : 7;
                const int cw = (d == sel) ? 16 : 14;
                if (!box_hit(ex, ey, ew, eh, cx - co, cy - co, cw, cw)) continue;
                g.fillRect(cx - 7, cy - 7, 14, 14, PAL_BG);
                g.drawRect(cx - 7, cy - 7, 14, 14, COL_LINE);
                dev_icon(g, SCAN_TYPE[d], cx - 6, cy - 6, false);
                if (d == sel) g.drawRect(cx - 8, cy - 8, 16, 16, ac);
            }
        }
#endif
        draw_blob(g, x, y, 1, bot_color(i), botcorps_shape_for(i), false);
    }
    if (!radar && sel >= 0 && sel < SCAN_N) {
        const int lane = sel;
        const int L = bus_len(tab[lane], BUS_N[lane]);
        int x = 0;
        int y = 0;
        bus_at(tab[lane], BUS_N[lane], bus_dock(tab[lane], BUS_N[lane], L, lane, wifi), &x, &y);
        g.drawRect(x - 8, y - 8, 16, 16, ac);
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
    /* Above the icon row (y>=209). The banner must not sit on those glyphs. */
    plate(g, x, 186, tw, 16, PAL_BLACK, shown > 0 ? COL_AMBER : COL_LINE);
    txt_bg(g, lab, x + tw / 2, 190, shown > 0 ? COL_AMBER : COL_MUTED, PAL_BLACK,
           lgfx::textdatum::top_center);
}

static void paint_bus_scan(Gfx g, bool wifi);

static int list_band(void);
static void paint_sbox(Gfx g, bool wifi);

static void paint_radar(Gfx g, bool with_list)
{
    /* Device path for SCR_SCAN. The full extBus layout lives in paint_bus_scan. */
    if (sbox_on()) {
        paint_sbox(g, list_band() == 0);
        return;
    }
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
        crown_btn(g, s_cursor == 2);
    } else {
        draw_btn(g, 14, 90, ICO_TRACK, s_cursor == 0);
        crown_btn(g, s_cursor == 1);
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
    const uint32_t u = (millis() - s_bus_t0) % 3400u;
    g.fillRect(0, 0, 320, 24, PAL_BLACK);
    g.fillRect(0, 24, 320, 1, COL_LINE);
    g.fillRect(0, 24, (int)(320u * u / 3400u), 1, ac);
    hub_gibson(g, 14, 12, 1);
    const int keep = s_px;
    s_px = 8;
    txt(g, "GIBSON", 29, 8, ac, lgfx::textdatum::top_left);
    txt(g, "GIBSON", 28, 8, ac, lgfx::textdatum::top_left);
    s_px = keep;
    g.drawCircle(232, 12, 5, COL_MUTED);
    g.fillCircle(232, 12, 2, COL_MUTED);
    g.setFont(&fonts::Font8x8C64);
    g.setTextSize(2);
    g.setTextColor(COL_INK);
    g.setCursor(248, 2);
    g.print("12");
    g.setTextSize(1);
    g.drawCircle(296, 12, 5, COL_MUTED);
    g.drawLine(296, 8, 296, 16, COL_MUTED);
    g.drawLine(292, 12, 300, 12, COL_MUTED);
    /* Name banner sits above the icon strip (y>=209). */
    s_px = 6;
    if (sel >= 0 && sel < 3) {
        char lab[28];
        snprintf(lab, sizeof(lab), "%s %s dBm", names[sel], rssi[sel]);
        const int tw = (int)strlen(lab) * 6 + 16;
        int bx = 160 - tw / 2;
        if (bx < 8) bx = 8;
        plate(g, bx, 186, tw, 16, PAL_BLACK, ac);
        txt_bg(g, lab, bx + tw / 2, 190, ac, PAL_BLACK, lgfx::textdatum::top_center);
    }
    g.fillRect(0, 209, 320, 31, PAL_BLACK);
    g.fillRect(0, 209, 320, 1, COL_LINE);
    int tally[DEV_N];
    for (int i = 0; i < DEV_N; ++i) tally[i] = 0;
    for (int d = 0; d < SCAN_N; ++d) tally[SCAN_TYPE[d]]++;
    const uint8_t *types = wifi ? NEAR_WIFI_T : NEAR_BLE_T;
    if (sel >= 0 && sel < 3) dev_icon(g, types[sel], 12, 218, false);
    for (int k = 0; k < 4; ++k) {
        const int bh = (k + 1) * 3;
        const int lit = (sel >= 0 && sel < 3) ? bars[sel] : 0;
        g.fillRect(30 + k * 5, 227 - bh, 3, bh, k < lit ? ac : COL_LINE);
    }
    s_px = 6;
    for (int i = 0; i < DEV_N; ++i) {
        const int x = 62 + i * 25;
        dev_icon(g, i, x, 214, tally[i] == 0);
        char num[4];
        snprintf(num, sizeof(num), "%d", tally[i]);
        txt(g, num, x + 13, 218, tally[i] ? (uint8_t)COL_INK : (uint8_t)COL_MUTED,
            lgfx::textdatum::top_left);
    }
    s_px = keep;
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
    return (s_screen == SCR_BT_LIST || s_screen == SCR_BT_RADAR) ? 1 : 0;
}

static inline void sb_rect(Gfx g, int x, int y, int w, int h, uint8_t c) { g.fillRect(x, y, w, h, c); }
static inline void sb_circ(Gfx g, int x, int y, int r, uint8_t c) { g.fillCircle(x, y, r, c); }

/* v1016 scan box. Thirteen glossary types, one 12x12 icon each, three inks.
 * Six rows reuse the v3.2 set above; phone, laptop, speaker, watch, car,
 * dev board and printer are new. Inks are fixed deep saturated indexes. */
enum {
    GL_AP = 0, GL_PHONE, GL_LAPTOP, GL_TV, GL_SPEAKER, GL_TRACKER, GL_WATCH,
    GL_CAMERA, GL_CAR, GL_UNKNOWN, GL_DRONE, GL_DEVBOARD, GL_PRINTER, GL_N
};
static const char *const GL_LABEL[GL_N] = {
    "WI-FI AP", "PHONE", "LAPTOP", "TV", "SPEAKER", "TRACKER", "WATCH",
    "CAMERA", "CAR", "UNKNOWN", "DRONE", "DEV BOARD", "PRINTER"
};
static const char *const GL_NEW[7][12] = {
    { "...aaaaaa...", "..abbbbbba..", "..abccccba..", "..abccccba..", "..abccccba..", "..abccccba..", "..abccccba..", "..abccccba..", "..abbbbbba..", "..abbaabba..", "...aaaaaa...", "............" },
    { "............", "..aaaaaaaa..", "..abbbbbba..", "..abccccba..", "..abccccba..", "..abccccba..", "..abbbbbba..", "..aaaaaaaa..", ".aaaaaaaaaa.", "aaabbbbbbaaa", "aaaaaaaaaaaa", "............" },
    { "...aaaaaa...", "..abbbbbba..", "..abbccbba..", "..abbbbbba..", "..abaaaaba..", "..aabbbbaa..", "..abbccbba..", "..abcbbcba..", "..abbccbba..", "..aabbbbaa..", "...aaaaaa...", "............" },
    { "....aaaa....", "....abba....", "...aaaaaa...", "..abbbbbba..", "..abbcbbba..", "..abbcccba..", "..abbbbbba..", "..abbbbbba..", "...aaaaaa...", "....abba....", "....aaaa....", "............" },
    { "............", "............", "...aaaaaa...", "..abbbbbba..", ".aabbbbbbaa.", "aaaaaaaaaaaa", "acccaaaaccca", "aaaaaaaaaaaa", ".bbb....bbb.", ".bcb....bcb.", "............", "............" },
    { ".c.c.c.c.c.c", "aaaaaaaaaaaa", "abbbbbaaaaaa", "abbbbbaaccaa", "abbbbbaaccaa", "abbbbbaaaaaa", "aaaaaaaaaaaa", "aacaaaaaacaa", "aaaaaaaaaaaa", ".c.c.c.c.c.c", "............", "............" },
    { "...cccccc...", "...cccccc...", ".aaaaaaaaaa.", "aaaaaaaaaaaa", "aaaaaaaaabaa", "aaaaaaaaaaaa", "aabbbbbbbbaa", "aabccccccbaa", "..cccccccc..", "..cbbbbbbc..", "..cccccccc..", "............" },
};
/* a = neon body, b = deep shade, c = highlight. RGB332 / reserved slots nearest the mock neon. */
static const uint8_t GL_INK[GL_N][3] = {
    { 31, 1, 255 },    /* ap #00F0FF */
    { 92, 36, 255 },   /* phone #39FF14 */
    { 188, 44, 255 },  /* laptop #C8FF00 */
    { 30, 1, 255 },    /* tv #00FFA8 */
    { 240, 44, 255 },  /* speaker #FFA000 */
    { 252, 68, 255 },  /* tracker #FFF000 */
    { 231, 64, 255 },  /* watch #FF2BD6 */
    { 167, 64, 255 },  /* camera #B026FF */
    { 111, 37, 255 },  /* car #6E74FF */
    { 243, 64, 255 },  /* unknown #FF8AE8 */
    { 224, 64, 255 },  /* drone #FF2020 */
    { 51, 37, 252 },   /* dev board #2EA0FF, gold pins */
    { 234, 64, 255 },  /* printer #FF4F9A */
};

static bool s_gl_mono = false; /* newest scan tile blinks white */
static void gl_icon(Gfx g, int type, int x, int y, bool dim)
{
    if (type < 0 || type >= GL_N) type = GL_UNKNOWN;
    const char *const *rows = nullptr;
    switch (type) {
    case GL_AP: rows = DEV_ROW[DEV_AP]; break;
    case GL_TV: rows = DEV_ROW[DEV_TV]; break;
    case GL_TRACKER: rows = DEV_ROW[DEV_TAG]; break;
    case GL_CAMERA: rows = DEV_ROW[DEV_CAM]; break;
    case GL_UNKNOWN: rows = DEV_ROW[DEV_UNK]; break;
    case GL_DRONE: rows = DEV_ROW[DEV_DRONE]; break;
    case GL_PHONE: rows = GL_NEW[0]; break;
    case GL_LAPTOP: rows = GL_NEW[1]; break;
    case GL_SPEAKER: rows = GL_NEW[2]; break;
    case GL_WATCH: rows = GL_NEW[3]; break;
    case GL_CAR: rows = GL_NEW[4]; break;
    case GL_DEVBOARD: rows = GL_NEW[5]; break;
    default: rows = GL_NEW[6]; break;
    }
    const uint8_t *ink = GL_INK[type];
    for (int r = 0; r < 12; ++r) {
        for (int c = 0; c < 12; ++c) {
            const char ch = rows[r][c];
            if (ch != 'a' && ch != 'b' && ch != 'c') continue;
            uint8_t k = ink[ch - 'a'];
            if (type == GL_AP || type == GL_TV || type == GL_CAMERA || type == GL_UNKNOWN || type == GL_TRACKER || type == GL_DRONE) {
                /* v3.2 rows: 'c' is the outline on AP/drone/unknown; keep the body neon there. */
                if (ch == 'c' && (type == GL_AP || type == GL_DRONE || type == GL_UNKNOWN)) k = ink[0];
            }
            if (dim) k = (ch == 'b') ? (uint8_t)PAL_DEEP : (uint8_t)PAL_LINE;
            if (s_gl_mono) k = PAL_WHITE;
            sb_rect(g, x + c, y + r, 1, 1, k);
        }
    }
}

static const int LIST_N = SB_N;
/* v1016 1604 mock devices, strongest first. Cells, ids and MACs come from FNV-1a of the name, as in the mock. */
static const char *const LIST_WIFI_N[LIST_N] = {
    "cafe-guest", "home-5g", "tp-link-mesh", "iphone-hotspot", "esp32-marauder", "ring-doorbell",
    "samsung-tv", "printer-2f", "macbook-air", "lobby-ap", "hp-laptop", "xfinitywifi"
};
static const int8_t LIST_WIFI_DB[LIST_N] = { -42, -49, -55, -57, -58, -63, -66, -67, -70, -71, -74, -78 };
static const uint8_t LIST_WIFI_TY[LIST_N] = {
    GL_AP, GL_AP, GL_AP, GL_PHONE, GL_DEVBOARD, GL_CAMERA, GL_TV, GL_PRINTER, GL_LAPTOP, GL_AP, GL_LAPTOP, GL_AP
};
static const uint8_t LIST_WIFI_DT[LIST_N] = {
    DEV_AP, DEV_UNK, DEV_AP, DEV_AP, DEV_DEAUTH, DEV_CAM, DEV_TV, DEV_AP, DEV_UNK, DEV_AP, DEV_AP, DEV_AP
};
static const char *const LIST_BLE_N[LIST_N] = {
    "earbuds-58", "dji-mini", "jbl-flip", "watch-6", "hc-05-atm", "pixel-8",
    "meta-glasses", "tile-mate", "flock-raven", "uconnect-car", "speaker-7", "fitbit-ch"
};
static const int8_t LIST_BLE_DB[LIST_N] = { -58, -60, -61, -63, -66, -68, -69, -71, -72, -73, -74, -76 };
static const uint8_t LIST_BLE_TY[LIST_N] = {
    GL_SPEAKER, GL_DRONE, GL_SPEAKER, GL_WATCH, GL_DEVBOARD, GL_PHONE, GL_CAMERA, GL_TRACKER, GL_CAMERA, GL_CAR, GL_SPEAKER, GL_WATCH
};
static const uint8_t LIST_BLE_DT[LIST_N] = {
    DEV_BT, DEV_DRONE, DEV_BT, DEV_BT, DEV_SKIM, DEV_BT, DEV_CAM, DEV_TAG, DEV_FLOCK, DEV_BT, DEV_BT, DEV_BT
};

static const char *sb_name(int band, int d) { return (band ? LIST_BLE_N : LIST_WIFI_N)[d]; }
static int sb_db(int band, int d) { return (band ? LIST_BLE_DB : LIST_WIFI_DB)[d]; }
static int sb_ty(int band, int d) { return (band ? LIST_BLE_TY : LIST_WIFI_TY)[d]; }

static uint32_t sb_hash(const char *s)
{
    uint32_t h = 2166136261u;
    for (; s && *s; ++s) { h ^= (uint8_t)*s; h *= 16777619u; }
    return h;
}

static void paint_bus_scan(Gfx g, bool wifi)
{
    bus_paint(g, false, wifi, 12, 0);
    paint_scan_hud(g, wifi, 0);
}

static int hunt_rssi_now(void)
{
    if (s_hunt_force != 0) return s_hunt_force;
    const uint32_t t = millis();
    const int ph = (s_hunt_dev >= 0 ? s_hunt_dev : 0) * 37;
    int v = -72 + (int)(22.f * sinf((float)(t + (uint32_t)ph) / 3100.f))
            + (int)(6.f * sinf((float)(t + (uint32_t)ph) / 700.f))
            + (int)((t / 250u) % 5u) - 2;
    if (v < -88) v = -99;
    return v;
}

static void hunt_sample(void)
{
    const uint32_t t = millis();
    if (s_hunt_tS != 0 && (uint32_t)(t - s_hunt_tS) < 250u) return;
    s_hunt_tS = t;
    const int r = hunt_rssi_now();
    if (s_hunt_len < HUNT_N) {
        s_hunt_buf[s_hunt_len++] = (int8_t)r;
    } else {
        for (int i = 1; i < HUNT_N; ++i) s_hunt_buf[i - 1] = s_hunt_buf[i];
        s_hunt_buf[HUNT_N - 1] = (int8_t)r;
    }
}

/* ===== v1016 scan box: Wi-Fi / BLE, drawn to the 1604 mock. ===== */
static const int SB_X = 108, SB_Y = 54, SB_W = 172, SB_H = 96; /* overlay panel, mock dx+2,dy+12 */
static const int SG_X = 108, SG_Y = 54, SG_C = 22;             /* 7x4 grid */

/* Patch region of the current scan-box paint. Blocks outside it are skipped. */
static int s_pc_x = 0, s_pc_y = 0, s_pc_w = 320, s_pc_h = 240;
static bool sb_vis(int x, int y, int w, int h)
{
    return x < s_pc_x + s_pc_w && x + w > s_pc_x && y < s_pc_y + s_pc_h && y + h > s_pc_y;
}

static uint8_t sb_tone(int which)
{
    /* 0 board, 1 board texture, 2 grid wash (accent .22), 3 accent .35, 4 accent .8 */
    static const uint8_t T[3][5] = { { PAL_PCB, 45, PAL_WASH_C, PAL_WIRE_C, 86 },
                                     { 104, 108, PAL_WASH_A, PAL_WIRE_A, 177 },
                                     { 13, 45, PAL_WASH_M, PAL_WIRE_M, 90 } };
    const int c = (s_color < 0 || s_color > 2) ? 0 : s_color;
    return T[c][which];
}

/* mock UI.ink: the accent mixed 45 % toward white (#6eeef6 on cyan) */
static uint8_t sb_ink(void)
{
    static const uint8_t K[3] = { 127, 250, 191 };
    return K[(s_color < 0 || s_color > 2) ? 0 : s_color];
}

static void sb_frame(Gfx g, int x, int y, int w, int h, uint8_t c)
{
    sb_rect(g, x, y, w, 1, c);
    sb_rect(g, x, y + h - 1, w, 1, c);
    sb_rect(g, x, y, 1, h, c);
    sb_rect(g, x + w - 1, y, 1, h, c);
}

static void sb_text(Gfx g, const char *s, int x, int y, uint8_t fg, uint8_t bg, bool big8, int size)
{
    g.setFont(big8 ? &fonts::Font8x8C64 : &fonts::Font0);
    g.setTextSize(size);
    g.setTextColor(fg, bg);
    g.setTextDatum(lgfx::textdatum::top_left);
    g.drawString(s ? s : "", x, y);
    g.setTextSize(1);
}

/* Terminal text: Font0 glyphs at a 5.5 px pitch, the mock's 9 px mono. Transparent. */
static int sb_tt(Gfx g, const char *s, int x, int y, uint8_t fg)
{
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(fg);
    g.setTextDatum(lgfx::textdatum::top_left);
    char one[2] = { 0, 0 };
    for (int i = 0; s && s[i] && x <= 277; ++i) {
        one[0] = s[i];
        if (s[i] != ' ') g.drawString(one, x, y);
        x += (i & 1) ? 5 : 6;
    }
    return x;
}

/* Transparent text with a one-pixel halo in the glow ink: the mock's canvas glow, cheaply. */
static void sb_glow(Gfx g, const char *s, int x, int y, uint8_t fg, uint8_t glow, int size)
{
    g.setFont(&fonts::Font0);
    g.setTextSize(size);
    g.setTextDatum(lgfx::textdatum::top_left);
    g.setTextColor(glow);
    g.drawString(s, x - 1, y);
    g.drawString(s, x + 1, y);
    g.drawString(s, x, y - 1);
    g.drawString(s, x, y + 1);
    g.setTextColor(fg);
    g.drawString(s, x, y);
    g.setTextSize(1);
}

/* 3x5 etch font for the module silk: 0-9, A-Z, '-'. */
static const uint16_t TINY3[37] = { 0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7292, 0x7BEF, 0x7BCF, 0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A, 0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B73, 0x6BAD, 0x388E, 0x7492, 0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x01C0 };
static int tiny_w(const char *s) { const int n = (int)strlen(s); return n ? n * 4 - 1 : 0; }
static void tiny_text(Gfx g, const char *s, int x0, int y, uint8_t col, int maxw = 0)
{
    const int n = (int)strlen(s);
    GATE_BOX(g, "SILK", s, x0, y, (maxw > 0 && n * 4 - 1 > maxw) ? maxw : n * 4 - 1, 5, "-");
    for (int i = 0; i < n; ++i) {
        /* past maxw the glyphs squeeze, like the mock's maxWidth silk */
        const int x = (maxw > 0 && n * 4 - 1 > maxw) ? x0 + i * (maxw - 2) / (n - 1) : x0 + i * 4;
        const char *sp = s + i;
        const char *s = sp;
        int k = -1;
        if (*s >= '0' && *s <= '9') k = *s - '0';
        else if (*s >= 'A' && *s <= 'Z') k = 10 + *s - 'A';
        else if (*s == '-') k = 36;
        if (k < 0) continue;
        const uint16_t m = TINY3[k];
        for (int b = 0; b < 15; ++b)
            if (m & (1u << (14 - b))) sb_rect(g, x + b % 3, y + b / 3, 1, 1, col);
    }
}

static void sb_upper(char *dst, int n, const char *src, int maxc)
{
    int k = 0;
    for (const char *p = src; p && *p && k < n - 1 && k < maxc; ++p)
        dst[k++] = (*p >= 'a' && *p <= 'z') ? (char)(*p - 32) : *p;
    dst[k] = 0;
}

/* Mock cell: hash % 28, linear probe in device order. */
static int8_t s_cell_c[2][SB_N] = { { -1 }, { -1 } };
static int sb_cell_id(int band, int d)
{
    if (s_cell_c[band][0] >= 0) return s_cell_c[band][d];
    bool used[28] = {};
    for (int k = 0; k < LIST_N; ++k) {
        int c = (int)(sb_hash(sb_name(band, k)) % 28u);
        while (used[c]) c = (c + 1) % 28;
        used[c] = true;
        s_cell_c[band][k] = (int8_t)c;
    }
    return s_cell_c[band][d];
}

static void sb_cell(int band, int d, int *cx, int *cy)
{
    const int c = sb_cell_id(band, d);
    *cx = SG_X + 11 + (c % 7) * SG_C;
    *cy = SG_Y + 11 + (c / 7) * SG_C;
}

static void sb_mac(char *out, int n, int band, int d, int bytes)
{
    const uint32_t h = sb_hash(sb_name(band, d));
    if (bytes == 3) snprintf(out, (size_t)n, "%02X:%02X:%02X", (unsigned)(h & 255u), (unsigned)((h >> 8) & 255u), (unsigned)((h >> 16) & 255u));
    else snprintf(out, (size_t)n, "%02X:%02X:%02X:%02X", (unsigned)(h & 255u), (unsigned)((h >> 8) & 255u), (unsigned)((h >> 16) & 255u), (unsigned)(h >> 24));
}

static int sb_lit(int dbm)
{
    /* mock str = (dbm + 92) / 50; bar q lit when str > (q + .5) / 4 * .9 + .05 */
    int lit = 0;
    for (int q = 0; q < 4; ++q)
        if ((float)(dbm + 92) / 50.f > ((float)q + .5f) / 4.f * .9f + .05f) lit = q + 1;
    return lit;
}

static void sb_bars(Gfx g, int x, int y, int dbm, uint8_t on)
{
    const int lit = sb_lit(dbm);
    for (int q = 0; q < 4; ++q)
        sb_rect(g, x + q * 4, y + 12 - (q + 1) * 3, 3, (q + 1) * 3, q < lit ? on : (uint8_t)PAL_LINE);
}

static uint8_t sb_mix(uint8_t a, uint16_t b565, int pct)
{
    return botcorps_pal_fit(mix16(botcorps_rgb565(a), b565, pct, 100));
}

/* mock botSquare: face in the bot colour, light top row, dark bottom row, two eyes */
static void sb_bot_square(Gfx g, int x, int y, int s, uint8_t col)
{
    sb_rect(g, x, y, s, s, col);
    sb_rect(g, x, y, s, 1, sb_mix(col, 0xFFFF, 45));
    sb_rect(g, x, y + s - 1, s, 1, sb_mix(col, 0x0000, 45));
    const int e = s / 6 > 2 ? (s + 3) / 6 : 2, ey = y + (s * 38 + 50) / 100, o = (s * 26 + 50) / 100;
    const bool bl = (millis() / 2600u) % 7u == 0u && (millis() % 2600u) < 140u;
    sb_rect(g, x + o, bl ? ey + e - 1 : ey, e, bl ? 1 : e, PAL_BLACK);
    sb_rect(g, x + s - o - e, bl ? ey + e - 1 : ey, e, bl ? 1 : e, PAL_BLACK);
}

/* v1016f-3: every bot chip is the shared control-panel block (square, black). */
static void sb_bot_chip(Gfx g, int cx, int cy, int S, int bot)
{
    chip_block(g, cx, cy, S, bot);
}

/* ESP32 dev board around the box (mock cydBoard + module). */
static void sb_board(Gfx g, bool wifi)
{
    const uint8_t ac = accent();
    const uint8_t pcb = PAL_BLACK, tex = PAL_DEEP; /* v1016f-4 (Dirt 8:24): fixed matte-black PCB */
    const uint8_t gold = 177, goldh = 217, goldd = 104;
    const bool core = s_screen == SCR_SCAN;
    /* soft accent glow ring outside the board edge */
    sb_frame(g, 14, 26, 292, 183, PAL_LINE);
    sb_frame(g, 15, 27, 290, 181, PAL_LINE);
    sb_frame(g, 16, 28, 288, 179, PAL_DEEP);
    sb_frame(g, 17, 29, 286, 177, PAL_DEEP);
    GATE_BOX(g, "BOARD", "scan", 14, 26, 292, 183, "-");
    sb_rect(g, 18, 30, 284, 175, pcb);
    for (int x = 24; x < 300; x += 8) sb_rect(g, x, 31, 1, 173, tex);
    sb_frame(g, 18, 30, 284, 175, gold); /* gold board edge, fixed */
    /* 2.54 mm headers, top and bottom */
    for (int row = 0; row < 2; ++row) {
        const int y = row ? 195 : 33;
        sb_rect(g, 36, y - 1, 246, 8, 64);
        sb_rect(g, 36, y - 1, 246, 1, 68);
        for (int x = 38; x <= 276; x += 8) {
            sb_rect(g, x, y + 1, 4, 4, x == 38 ? (uint8_t)209 : gold);
            sb_rect(g, x, y + 1, 4, 1, goldh);
            sb_rect(g, x + 1, y + 2, 2, 2, x == 38 ? (uint8_t)goldd : (uint8_t)108);
        }
    }
    const int hx[4] = { 26, 294, 26, 294 }, hy[4] = { 38, 38, 197, 197 };
    for (int k = 0; k < 4; ++k) {
        sb_circ(g, hx[k], hy[k], 4, gold);
        sb_circ(g, hx[k], hy[k], 2, PAL_BLACK);
    }
    /* BOOT / EN buttons and the USB port on the right edge */
    for (int k = 0; k < 2; ++k) {
        const int y = k ? 140 : 84;
        sb_rect(g, 292, y, 8, 12, 41);
        sb_circ(g, 296, y + 6, 2, 40);
    }
    sb_rect(g, 295, 106, 10, 24, 37);
    sb_frame(g, 295, 106, 10, 24, sb_tone(3));
    sb_rect(g, 298, 110, 4, 16, 4);
    /* WROOM module (radio bots) or the clean Gibson/bot core block */
    const int mx = 34, my = 52, mw = 54, mh = 128;
    const int bot = core ? s_scan_bot : wifi ? 0 : 1;
    if (core) {
        /* v1016f-4 (Dirt 8:19): drop the silver module frame; Gibson/bot is the
         * clean control-panel block with uncut labels. */
        const int cxm = mx + mw / 2, ccy = my + 42;
        const bool gib = bot >= 12;
        chip_block(g, cxm, ccy, 54, gib ? 12 : bot);
        char nmb[16];
        const char *nm;
        if (gib) nm = "GIBSON";
        else { sb_upper(nmb, 16, BOTS[bot].short_name, 12); if (strlen(nmb) <= 9) strcat(nmb, "BOT"); nm = nmb; }
        const char *l2c = "ESP32-WROOM-32";
        const char *l3c = gib ? "CORE + PRIME" : "RADIO CORE";
        tiny_text(g, nm, cxm - tiny_w(nm) / 2, ccy + 32, 41);
        tiny_text(g, l2c, cxm - tiny_w(l2c) / 2, ccy + 42, 41);
        tiny_text(g, l3c, cxm - tiny_w(l3c) / 2, ccy + 50, 41);
    } else {
    sb_rect(g, mx, my, mw, mh, 5);
    sb_frame(g, mx, my, mw, mh, 41);
    int ax = mx + 5;
    for (int i = 0; i < 6; ++i) {
        sb_rect(g, ax, my + 4, 1, 15, 176);
        sb_rect(g, ax, my + 4, 4, 1, 176);
        sb_rect(g, ax + 3, my + 4, 1, 15, 176);
        sb_rect(g, ax + 3, my + 18, 4, 1, 176);
        ax += 7;
    }
    sb_rect(g, mx + 4, my + 21, 3, 3, 176);
    sb_rect(g, mx + 9, my + 21, 2, 3, goldd);
    /* shield can: cheap three-band diagonal brushed metal */
    const int sx = mx + 4, sy = my + 25, sw = 46, sh = 95;
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0;
        if (b < 0) b = 0;
        if (b > sw) b = sw;
        sb_rect(g, sx, sy + r, a, 1, 178);
        sb_rect(g, sx + a, sy + r, b - a, 1, 110);
        sb_rect(g, sx + b, sy + r, sw - b, 1, 73);
    }
    sb_rect(g, sx, sy, sw, 1, 186);
    sb_rect(g, sx, sy, 1, sh, 186);
    sb_rect(g, sx, sy + sh - 1, sw, 1, 41);
    sb_rect(g, sx + sw - 1, sy, 1, sh, 41);
    sb_rect(g, sx - 1, sy + 6, 2, 4, 73);
    sb_rect(g, sx + sw - 1, sy + 6, 2, 4, 73);
    sb_rect(g, sx - 1, sy + sh - 10, 2, 4, 73);
    sb_rect(g, sx + sw - 1, sy + sh - 10, 2, 4, 73);
    const int wx = sx + 5, wy = sy + 8, ww = 36, wh = 42;
    sb_rect(g, wx, wy, ww, wh, 8);
    char nmb[16];
    const char *nm = wifi ? "WIFIBOT" : "BLE-BOT"; /* BLUETOOTHBOT is 47 px, wider than the can */
    sb_bot_chip(g, wx + ww / 2, wy + 21, 40, bot);
    const int cxm = sx + sw / 2;
    /* plain 4 px pitch so every glyph reads; long lines are abbreviated, never squeezed */
    tiny_text(g, nm, cxm - tiny_w(nm) / 2, wy + wh + 4, 41);
    tiny_text(g, "WROOM-32", cxm - tiny_w("WROOM-32") / 2, wy + wh + 14, 41);
    tiny_text(g, "FCC 2AC7Z", cxm - tiny_w("FCC 2AC7Z") / 2, wy + wh + 22, 41);
    static const char *const PIN_L[12] = { "3V3", "EN", "VP", "VN", "34", "35", "32", "33", "25", "26", "27", "14" };
    int i = 0;
    for (int y = my + 30; y < my + mh - 8; y += 8, ++i) {
        sb_rect(g, mx - 2, y, 4, 4, gold);
        sb_rect(g, mx - 2, y, 1, 4, 213);
        sb_rect(g, mx - 2, y + 1, 1, 2, 4);
        sb_rect(g, mx + mw - 2, y, 4, 4, gold);
        sb_rect(g, mx + mw + 1, y + 1, 1, 2, 4);
        if (i < 12) tiny_text(g, PIN_L[i], mx - 4 - tiny_w(PIN_L[i]), y - 1, 223);
    }
    for (int x = mx + 8; x < mx + mw - 8; x += 7) {
        sb_rect(g, x, my + mh - 2, 4, 4, gold);
        sb_rect(g, x, my + mh - 2, 4, 1, 213);
    }
    sb_rect(g, mx + 2, my + 27, 1, 2, 223);
    /* traces to the display FPC, LEDs */
    const uint32_t t = millis();
    const int span = 106 - 11 - mx - mw;
    for (int k = 0; k < 5; ++k) {
        const int y = my + 46 + k * 9;
        sb_rect(g, mx + mw + 2, y, 106 - 9 - mx - mw - 2, 1, sb_tone(3));
        const int f = (int)((t / 40u + (uint32_t)k * 7u) % (uint32_t)(span > 1 ? span : 1));
        sb_rect(g, mx + mw + 2 + f, y, 2, 1, ac);
    }
    const uint8_t led[5] = { ((t / 500u) % 2u) ? ac : pcb, 212, 51, 224, 57 };
    const int lx[5] = { 94, 94, 94, 93, 93 }, ly[5] = { 54, 63, 72, 160, 170 };
    for (int k = 0; k < 5; ++k) {
        sb_rect(g, lx[k] - 1, ly[k] + 1, 1, 1, goldd);
        sb_rect(g, lx[k] + 4, ly[k] + 1, 1, 1, goldd);
        sb_rect(g, lx[k], ly[k], 4, 3, led[k]);
        if (led[k] != pcb) sb_rect(g, lx[k] + 1, ly[k], 2, 1, PAL_WHITE);
    }
    tiny_text(g, "ON", 93, 152, 223);
    }
}

static int sb_shown(void)
{
    if (s_screen == SCR_SCAN) return LIST_N; /* v1016f-3 #7: core scan shows the full dense device cloud */
    /* mock: nShow = 1 + floor(u * 1.6 * 12) */
    const uint32_t e = (millis() - s_bus_t0) % 3400u; /* mock: the sweep loops every 3.4 s */
    int n = 1 + (int)((float)e / 3400.f * 1.6f * 12.f);
    return n > LIST_N ? LIST_N : n;
}

static void sb_grid(Gfx g, int band, bool tiles)
{
    const uint8_t ac = accent();
    for (int c = 0; c <= 7; ++c) sb_rect(g, SG_X + c * SG_C, SG_Y, 1, 4 * SG_C + 1, 1);
    for (int r = 0; r <= 4; ++r) sb_rect(g, SG_X, SG_Y + r * SG_C, 7 * SG_C + 1, 1, 1);
    for (int c = 0; c <= 7; ++c)
        for (int r = 0; r <= 4; ++r) sb_rect(g, SG_X + c * SG_C - 1, SG_Y + r * SG_C - 1, 3, 3, 65);
    sb_rect(g, SG_X + 7 * SG_C + 1, SG_Y, 1, 4 * SG_C, sb_tone(3));
    if (!tiles) return;
    const int n = sb_shown(), k0 = n - 1;
    const uint32_t t = millis();
    for (int d = 0; d < n; ++d) {
        if (s_dev_ign[band][d]) continue;
        int cx = 0, cy = 0;
        sb_cell(band, d, &cx, &cy);
        const int x = cx - 10, y = cy - 10;
        GATE_BOX(g, "TILE", "dev", x, y, 20, 20, "MINI");
        sb_rect(g, x, y, 20, 20, 149);
        sb_rect(g, x + 1, y + 1, 18, 18, 5);
        for (int q = 0; q < 4; ++q) {
            sb_rect(g, x - 1, y + 3 + q * 4, 1, 2, 149);
            sb_rect(g, x + 20, y + 3 + q * 4, 1, 2, 149);
        }
        const bool fresh = d == k0 && (t / 110u) % 2u;
        s_gl_mono = fresh;
        gl_icon(g, sb_ty(band, d), x + 4, y + 4, false);
        s_gl_mono = false;
        if (s_dev_tag[band][d] > 0) sb_rect(g, x + 15, y + 1, 4, 4, TAG_COL[s_dev_tag[band][d] - 1]);
        if (d == s_cursor && s_act < 0) sb_frame(g, x - 2, y - 2, 24, 24, ac);
    }
    /* scout: walks from the bottom-left cell to the newest find, horizontal then vertical */
    int nx = 0, ny = 0;
    sb_cell(band, k0, &nx, &ny);
    const int bot = k0 % 12;
    const uint8_t bc = bot_color(bot), tr = bc;
    const int sx = SG_X + 11, sy = SG_Y + 3 * SG_C + 11;
    float ph = (float)(t % 900u) / 900.f * 1.4f;
    if (ph > 1.f) ph = 1.f;
    const float h1 = ph * 2.f > 1.f ? 1.f : ph * 2.f;
    const int hx = sx + (int)((float)(nx - sx) * h1);
    const int hy = ph < .5f ? sy : sy + (int)((float)(ny - sy) * (ph - .5f) * 2.f);
    sb_rect(g, hx < sx ? hx : sx, sy, (hx > sx ? hx - sx : sx - hx) + 1, 1, tr);
    if (ph > .5f) sb_rect(g, hx, hy < sy ? hy : sy, 1, (hy > sy ? hy - sy : sy - hy) + 1, tr);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_walk = 1;
#endif
    botcorps_sprite_draw(*g.raw(), hx - g.ox, hy - 1 - g.oy, botcorps_shape_for(bot), 1, bc, false);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_walk = 0;
#endif
}

/* Core bus scan (Gibson or a non-radio bot): the crew patrols the grid lines, no device tiles. */
static void sb_scouts(Gfx g)
{
    const uint32_t t = millis();
    const int span = 7 * SG_C;
    for (int k = 0; k < 6; ++k) {
        const int bot = (k * 2 + (s_scan_bot < 12 ? s_scan_bot : 0)) % 12;
        const int row = k % 4;
        int p = (int)((t / 45u + (uint32_t)k * 37u) % (uint32_t)(span * 2));
        if (p > span) p = span * 2 - p;
        const int x = SG_X + p, y = SG_Y + row * SG_C + (k >= 4 ? SG_C : 0);
#ifdef BOTCORPS_HOST_RENDER
        host_gate_walk = 1;
#endif
        botcorps_sprite_draw(*g.raw(), x - 3 - g.ox, y - 4 - g.oy, botcorps_shape_for(bot), 1, bot_color(bot), false);
#ifdef BOTCORPS_HOST_RENDER
        host_gate_walk = 0;
#endif
    }
}

/* Faux transparency: the tiles under a panel show through as dim outlines. */
static void sb_ghost(Gfx g, int band, int px, int py, int pw, int ph)
{
    const int n = sb_shown();
    for (int d = 0; d < n; ++d) {
        if (s_dev_ign[band][d]) continue;
        int cx = 0, cy = 0;
        sb_cell(band, d, &cx, &cy);
        if (cx - 10 < px + 1 || cy - 10 < py + 1 || cx + 10 > px + pw - 1 || cy + 10 > py + ph - 1) continue;
        sb_frame(g, cx - 10, cy - 10, 20, 20, 37);
        sb_rect(g, cx - 2, cy - 2, 4, 4, 37);
    }
}

static void sb_panel(Gfx g, int band)
{
    const int d = (s_act_dev >= 0 && s_act_dev < LIST_N) ? s_act_dev : 0;
    const uint8_t ac = accent(), sel = sb_tone(3);
    const int x0 = SB_X, y0 = SB_Y, w = SB_W, h = SB_H;
    GATE_BOX(g, "OCCLUDE", "sbpanel", x0, y0, w, h, "-");
    sb_rect(g, x0, y0, w, h, 4);
    sb_ghost(g, band, x0, y0 + 21, w, h - 21);
    sb_frame(g, x0, y0, w, h, ac);
    sb_frame(g, x0 + 4, y0 + 3, 16, 16, 149);
    gl_icon(g, sb_ty(band, d), x0 + 6, y0 + 5, false);
    char nm[16];
    sb_upper(nm, sizeof(nm), sb_name(band, d), 15);
    sb_text(g, nm, x0 + 25, y0 + 5, PAL_WHITE, 4, false, 1);
    sb_text(g, GL_LABEL[sb_ty(band, d)], x0 + 25, y0 + 13, ac, 4, false, 1);
    sb_bars(g, x0 + w - 22, y0 + 3, sb_db(band, d), ac);
    sb_rect(g, x0 + 2, y0 + 20, w - 4, 1, sel);
    for (int j = 0; j < ACT_N; ++j) {
        const int yy = y0 + 22 + j * 14;
        const bool on = j == s_act;
        const uint8_t bg = on ? sb_tone(2) : (uint8_t)4;
        if (on) {
            sb_rect(g, x0 + 2, yy, w - 4, 13, bg);
            sb_rect(g, x0 + 2, yy, 2, 13, ac);
        }
        blit_ico(g, ACT_ICO[j], x0 + 9, yy + 3, on ? (uint8_t)PAL_WHITE : ac);
        sb_text(g, ACT_NAME[j], x0 + 22, yy + 3, on ? (uint8_t)PAL_WHITE : (uint8_t)COL_INK, bg, false, 1);
        if (on) sb_text(g, ACT_NAME[j], x0 + 23, yy + 3, PAL_WHITE, bg, false, 1);
        if (j == 2 && s_dev_tag[band][d] > 0) sb_rect(g, x0 + w - 14, yy + 4, 6, 6, TAG_COL[s_dev_tag[band][d] - 1]);
    }
}

static void sb_info(Gfx g, int band)
{
    const int d = (s_info_dev >= 0 && s_info_dev < LIST_N) ? s_info_dev : 0;
    const uint8_t ac = accent(), gold = 177;
    const uint32_t hs = sb_hash(sb_name(band, d));
    const int bx = 112, by = 57, bw = 62, bh = 82;
    GATE_BOX(g, "OCCLUDE", "info", SB_X, SB_Y, SB_W, SB_H, "-");
    sb_rect(g, bx, by, bw, bh, 5);
    sb_frame(g, bx, by, bw, bh, 65);
    for (int k = 0; k < 6; ++k) {
        const int yy = by + 6 + k * 7, xx = bx + 4 + (int)((hs >> k) & 7u);
        sb_rect(g, xx, yy, bw - 10 - (int)((hs >> (k + 2)) & 7u), 1, 17);
        sb_rect(g, xx, yy - 1, 2, 3, 108);
    }
    sb_rect(g, bx + 22, by + 13, 18, 18, 40);
    for (int k = 0; k < 4; ++k) {
        sb_rect(g, bx + 20, by + 15 + k * 4, 2, 1, gold);
        sb_rect(g, bx + 40, by + 15 + k * 4, 2, 1, gold);
    }
    gl_icon(g, sb_ty(band, d), bx + 25, by + 16, false);
    const int scy = by + (int)((millis() / 18u) % (uint32_t)bh);
    sb_rect(g, bx + 1, scy > by + 6 ? scy - 6 : by + 1, bw - 2, scy > by + 6 ? 6 : scy - by, sb_tone(2));
    sb_rect(g, bx + 1, scy, bw - 2, 1, ac);
    char nm[14];
    sb_upper(nm, sizeof(nm), sb_name(band, d), 13);
    sb_text(g, nm, 182, 61, PAL_WHITE, PAL_BG, false, 1);
    char id[8];
    snprintf(id, sizeof(id), "%04X", (unsigned)(hs % 65536u));
    sb_text(g, id, 182, 75, ac, PAL_BG, false, 1);
    const int lit = sb_lit(sb_db(band, d));
    for (int b = 0; b < 4; ++b) sb_rect(g, 182 + b * 5, 101 - (b + 1) * 3, 3, (b + 1) * 3, b < lit ? ac : (uint8_t)41);
    char db[8];
    snprintf(db, sizeof(db), "%d", sb_db(band, d));
    sb_text(g, db, 206, 93, ac, PAL_BG, false, 1);
}

static void sb_gloss(Gfx g)
{
    const uint8_t ac = accent(), sel = sb_tone(3), hd = sb_tone(2);
    const int x0 = SB_X, y0 = SB_Y, w = SB_W, h = SB_H;
    GATE_BOX(g, "OCCLUDE", "sbpanel", x0, y0, w, h, "-");
    sb_rect(g, x0, y0, w, h, 4);
    sb_ghost(g, list_band(), x0, y0 + 11, w, h - 11);
    sb_frame(g, x0, y0, w, h, ac);
    sb_rect(g, x0 + 1, y0 + 1, w - 2, 10, hd);
    sb_text(g, ">", x0 + 4, y0 + 3, ac, hd, false, 1);
    sb_text(g, "GLOSSARY", x0 + 12, y0 + 3, PAL_WHITE, hd, false, 1);
    if ((millis() / 530u) % 2u) sb_rect(g, x0 + 62, y0 + 3, 4, 6, ac);
    sb_text(g, "13", x0 + w - 17, y0 + 3, ac, hd, false, 1);
    const int cw = (w - 6) / 2;
    for (int i = 0; i < GL_N; ++i) {
        const int cx = x0 + 3 + (i / 7) * cw, cy = y0 + 12 + (i % 7) * 12;
        const bool on = i == s_gloss;
        if (on) {
            sb_rect(g, cx, cy, cw - 2, 12, sel);
            sb_rect(g, cx, cy, 1, 12, PAL_WHITE);
        }
        gl_icon(g, i, cx + 3, cy, false);
        sb_text(g, GL_LABEL[i], cx + 19, cy + 3, on ? (uint8_t)PAL_WHITE : (uint8_t)COL_INK, on ? sel : (uint8_t)4, false, 1);
    }
}

static void sb_hunt(Gfx g, int band)
{
    hunt_sample();
    const int d = s_hunt_dev;
    const uint8_t ac = accent();
    const int r = s_hunt_len ? (int)s_hunt_buf[s_hunt_len - 1] : hunt_rssi_now();
    const bool lost = r <= HUNT_LOST;
    const bool near = !lost && r >= HUNT_NEAR;
    const bool fl = ((millis() / 250u) % 2u) == 1u;
    const int x0 = 106, y0 = 42, w = 176, h = 102;
    GATE_BOX(g, "HUNT", "panel", x0, y0, w, h, "MINI");
    sb_rect(g, x0 + 2, y0 + 2, w - 4, h - 4, 4);
    /* faint board traces behind the panel (the mock shows bot rain at .35 here) */
    for (int k = 0; k < 7; ++k) {
        const int yy = y0 + 10 + k * 13, xx = x0 + 6 + (k * 23) % 40;
        sb_rect(g, xx, yy, x0 + w - 8 - xx - (k * 17) % 50, 1, 37);
        sb_rect(g, xx, yy - 1, 2, 3, 41);
    }
    const uint8_t bc = near ? (fl ? (uint8_t)228 : ac) : lost ? (uint8_t)73 : ac;
    sb_frame(g, x0 + 2, y0 + 2, w - 4, h - 4, bc);
    if (near) sb_frame(g, x0 + 3, y0 + 3, w - 6, h - 6, bc);
    gl_icon(g, sb_ty(band, d), x0 + 7, y0 + 7, lost);
    char nm[17];
    sb_upper(nm, sizeof(nm), sb_name(band, d), 16);
    sb_text(g, nm, x0 + 22, y0 + 6, PAL_WHITE, 4, false, 1);
    char mac[16];
    sb_mac(mac, sizeof(mac), band, d, 3);
    sb_text(g, mac, x0 + 22, y0 + 15, ac, 4, false, 1);
    if (near) {
        const uint8_t cb = fl ? (uint8_t)228 : sb_tone(3);
        sb_rect(g, x0 + w - 44, y0 + 6, 38, 11, cb);
        sb_text(g, "CLOSE", x0 + w - 40, y0 + 8, PAL_WHITE, cb, false, 1);
    }
    if (lost) sb_text(g, "LOST", x0 + w - 32, y0 + 8, fl ? (uint8_t)241 : ac, 4, false, 1);
    char db[8];
    if (lost) snprintf(db, sizeof(db), "--");
    else snprintf(db, sizeof(db), "%d", r);
    sb_text(g, db, x0 + 8, y0 + 30, lost ? (uint8_t)73 : near ? (uint8_t)PAL_WHITE : ac, 4, false, 3);
    sb_text(g, "DBM", x0 + 8 + (int)strlen(db) * 18 + 4, y0 + 42, ac, 4, false, 1);
    /* cold-to-hot meter, ten steps */
    static const uint8_t HOT[10] = { 43, 47, 23, 26, 89, 152, 248, 240, 232, 228 };
    const int seg = 10, mx = x0 + 8, my = y0 + 58, sw = (w - 20) / seg;
    int q = 0;
    if (!lost) {
        q = (int)lroundf((float)(r + 90) / 35.f * (float)seg);
        if (q < 1) q = 1;
        if (q > seg) q = seg;
    }
    for (int k = 0; k < seg; ++k) sb_rect(g, mx + k * sw, my, sw - 2, 7, k < q ? HOT[k] : (uint8_t)41);
    /* 24-sample trend: 2 px bars at a 3 px pitch, 20 tall */
    const int tx = x0 + w - 8 - HUNT_N * 3, ty = y0 + 30, th = 20;
    sb_rect(g, tx - 2, ty - 2, HUNT_N * 3 + 3, th + 4, 8);
    for (int k = 0; k < (int)s_hunt_len; ++k) {
        const int v = (int)s_hunt_buf[k];
        int hh = 1;
        if (v > HUNT_LOST) {
            hh = (int)lroundf((float)(v + 90) / 45.f * (float)th);
            if (hh < 1) hh = 1;
            if (hh > th) hh = th;
        }
        const uint8_t ink = v <= HUNT_LOST ? (uint8_t)73 : v >= HUNT_NEAR ? (uint8_t)224 : ac;
        sb_rect(g, tx + k * 3, ty + th - hh, 2, hh, ink);
    }
}

/* mock extBus top bar: Gibson chip, 12 bot squares, crown plate, >_ chip, count. */
static void sb_top(Gfx g)
{
    const uint8_t ac = accent();
    const uint32_t t = millis();
    const float u = (float)((t - s_bus_t0) % 3400u) / 3400.f;
    sb_rect(g, 0, 0, 320, 24, 8);
    sb_rect(g, 0, 24, 320, 1, sb_tone(3));
    sb_rect(g, 0, 24, (int)(320.f * u), 1, ac);
    hub_gibson(g, 14, 12, 1);
    for (int i = 0; i < 12; ++i) {
        const bool out = u > (float)i * .02f && u < .7f;
        const uint8_t c = bot_color(i);
        /* dimmed bots stay visible: 38 % of the bot hue over the bar, nearest of all 256 shown colours */
        static const uint8_t DIM[12] = { 9, 9, 9, 12, 76, 100, 33, 13, 76, 13, 100, 69 };
        sb_rect(g, 30 + i * 8, 9, 5, 5, out ? c : DIM[i]);
    }
    /* >_ quick terminal chip */
    sb_rect(g, 190, 2, 16, 13, 8);
    sb_frame(g, 190, 2, 16, 13, ac);
    sb_text(g, ">", 193, 5, ac, 8, false, 1);
    sb_rect(g, 199, 11, 4, 1, ac);
    g.drawCircle(235, 12, 3, ac);
    sb_rect(g, 234, 11, 3, 3, ac);
    sb_text(g, "12", 241, 6, PAL_WHITE, 8, false, 2);
    sb_text(g, "0", 296, 9, ac, 8, false, 1); /* the mock's pixel font draws its target glyph as 0 */
    /* crown plate, mock gibCrown(160, 1, 2) */
    static const char *const CR[7] = { "#.....#.....#", "##...###...##", "###.#####.###", "#############",
                                       "#############", ".###########.", ".###########." };
    const int x0 = 147, y = 1;
    GATE_BOX(g, "CROWN", "back", x0 - 4, 0, 34, 16, "-");
    sb_rect(g, x0 - 4, 0, 34, 16, 8);
    sb_rect(g, x0 - 4, 16, 34, 1, sb_tone(4));
    for (int r = 0; r < 7; ++r)
        for (int c = 0; c < 13; ++c) {
            if (CR[r][c] != '#') continue;
            sb_rect(g, x0 + c * 2, y + r * 2, 2, 2, r >= 5 ? (uint8_t)214 : (uint8_t)251);
        }
    const int jw = (int)((t / 400u) % 3u);
    for (int j = 0; j < 3; ++j) sb_rect(g, x0 + (3 + j * 3) * 2, y + 8, 2, 2, j == jw ? (uint8_t)PAL_WHITE : ac);
}

static void sb_hud(Gfx g, int band)
{
    if (sb_vis(0, 0, 320, 25)) sb_top(g);
    if (!sb_vis(0, 209, 320, 31)) return;
    sb_rect(g, 0, 209, 320, 31, PAL_BLACK);
    sb_rect(g, 0, 209, 320, 1, COL_LINE);
    const uint8_t ac = accent();
    int tally[GL_N];
    for (int i = 0; i < GL_N; ++i) tally[i] = 0;
    for (int d = 0; d < LIST_N; ++d)
        if (!s_dev_ign[band][d]) tally[sb_ty(band, d)]++;
    const int cur = (s_cursor >= 0 && s_cursor < LIST_N) ? s_cursor : 0;
    gl_icon(g, sb_ty(band, cur), 12, 218, false);
    const int lit = sb_lit(sb_db(band, cur));
    for (int k = 0; k < 4; ++k) {
        const int bh = (k + 1) * 3;
        sb_rect(g, 30 + k * 5, 227 - bh, 3, bh, k < lit ? ac : (uint8_t)41);
    }
    for (int i = 0; i < GL_N; ++i) {
        const int x = 58 + i * 20;
        gl_icon(g, i, x, 218, tally[i] == 0);
        char num[4];
        snprintf(num, sizeof(num), "%d", tally[i]);
        sb_text(g, num, x + 13, 221, tally[i] ? (uint8_t)COL_INK : (uint8_t)73, PAL_BLACK, false, 1);
    }
}

static void paint_sbox(Gfx g, bool wifi)
{
    const int band = wifi ? 0 : 1;
    const uint8_t ac = accent();
    /* the display: bezel, dark glass, prompt chip, grid, sweep rule, terminal */
    const int dx = 106, dy = 42, dw = 176, dh = 148;
    const bool in_disp = s_pc_x >= dx && s_pc_y >= dy && s_pc_x + s_pc_w <= dx + dw && s_pc_y + s_pc_h <= dy + dh;
    if (!in_disp) {
        /* v1016k: chrome bars + board only; the side margins keep the rain layer */
        sb_rect(g, 0, 0, 320, 26, PAL_BLACK);
        sb_rect(g, 0, 209, 320, 31, PAL_BLACK);
        if (sb_vis(14, 26, 292, 183)) sb_board(g, wifi);
        sb_rect(g, dx - 9, dy + 44, 5, 44, 41);
        sb_rect(g, dx - 4, dy - 4, dw + 8, dh + 8, 37);
        sb_rect(g, dx - 3, dy - 3, dw + 6, dh + 6, 40);
    }
    GATE_BOX(g, "MINI", "screen", dx, dy, dw, dh, "BOARD");
    sb_rect(g, dx, dy, dw, dh, 8);
    const uint32_t u = (millis() - s_bus_t0) % 3400u;
    sb_rect(g, dx, 144, dw, 1, sb_tone(3));
    sb_rect(g, dx, 144, (int)((uint32_t)dw * u / 3400u), 1, ac);
    const bool core = s_screen == SCR_SCAN;
    if (core) {
        if (sb_vis(dx, dy, dw, 108)) {
            /* v1016f-2 #7 / v1016f-4: dense device cloud + bots walking the strands. */
            sb_grid(g, band, true);
            sb_scouts(g);
            GATE_BOX(g, "PROMPT", "index", dx + 2, dy + 1, 15, 10, "MINI");
            sb_rect(g, dx + 2, dy + 1, 15, 10, 8);
            sb_frame(g, dx + 2, dy + 1, 15, 10, sb_tone(4));
            sb_text(g, ">", dx + 4, dy + 3, ac, 8, false, 1);
        }
    } else if (s_hunt_dev >= 0) {
        sb_grid(g, band, false);
        sb_hunt(g, band);
    } else if (sb_vis(dx, dy, dw, 108)) {
        sb_grid(g, band, s_info_dev < 0);
        const bool pf = s_cursor == LIST_N && s_gloss < 0;
        GATE_BOX(g, "PROMPT", "index", dx + 2, dy + 1, 15, 10, "MINI");
        sb_rect(g, dx + 2, dy + 1, 15, 10, pf ? sb_tone(3) : (uint8_t)8);
        sb_frame(g, dx + 2, dy + 1, 15, 10, pf ? (uint8_t)PAL_WHITE : sb_tone(4));
        sb_text(g, ">", dx + 4, dy + 3, pf ? (uint8_t)PAL_WHITE : ac, pf ? sb_tone(3) : (uint8_t)8, false, 1);
        if (pf || (millis() / 530u) % 2u) sb_rect(g, dx + 10, dy + 8, 4, 1, pf ? (uint8_t)PAL_WHITE : ac);
        if (s_gloss >= 0) sb_gloss(g);
        else if (s_info_dev >= 0) sb_info(g, band);
        else if (s_act >= 0) sb_panel(g, band);
    }
    /* terminal strip */
    const int ty = dy + dh - 13, th = ty - 22;
    if (sb_vis(dx, th - 3, dw, dy + dh - th + 3)) {
    sb_rect(g, dx, th - 3, dw, dy + dh - th + 3, 4);
    sb_rect(g, dx, th - 3, dw, 1, sb_tone(3));
    const char *bn = wifi ? "wifi" : "bluetooth";
    char l2[40];
    if (core) {
        char who[12];
        snprintf(who, sizeof(who), "%s", s_scan_bot >= 12 ? "gibson" : BOTS[s_scan_bot].short_name);
        int x = sb_tt(g, "[ ok ] ", dx + 3, th, PAL_OK);
        snprintf(l2, sizeof(l2), "%s core linked", who);
        sb_tt(g, l2, x, th + 1, sb_tone(3));
        sb_tt(g, l2, x, th, sb_ink());
        x = sb_tt(g, "[ ok ] ", dx + 3, th + 11, PAL_OK);
        snprintf(l2, sizeof(l2), "%s bus idle", who);
        sb_tt(g, l2, x, th + 12, sb_tone(3));
        sb_tt(g, l2, x, th + 11, sb_ink());
        x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
        if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
        /* mock busscan menu chip, bottom right of the display */
        sb_rect(g, dx + dw - 22, ty - 2, 19, 12, 8);
        sb_frame(g, dx + dw - 22, ty - 2, 19, 12, sb_tone(4));
        for (int k = 0; k < 3; ++k) sb_rect(g, dx + dw - 18, ty + 1 + k * 3, 11, 1, COL_MUTED);
    } else {
    int x = sb_tt(g, "[ ok ] ", dx + 3, th, PAL_OK);
    snprintf(l2, sizeof(l2), "%s radio up \xfa ch sweep", bn);
    sb_tt(g, l2, x, th + 1, sb_tone(3));
    sb_tt(g, l2, x, th, sb_ink());
    x = sb_tt(g, "[ ok ] ", dx + 3, th + 11, PAL_OK);
    snprintf(l2, sizeof(l2), "%s sweep %d/%d", bn, sb_shown(), LIST_N);
    sb_tt(g, l2, x, th + 12, sb_tone(3));
    sb_tt(g, l2, x, th + 11, sb_ink());
    x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
    /* typed command: one character per 90 ms of the first sweep */
    snprintf(l2, sizeof(l2), "scan %s", wifi ? "wifi" : "ble");
    const uint32_t e = (millis() - s_bus_t0) % 3400u; /* retyped every sweep, as the mock */
    const size_t nt = (size_t)(e / 90u);
    if (nt < strlen(l2)) l2[nt] = 0;
    x = sb_tt(g, l2, x, ty, COL_INK);
    if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
    }
    }
    if (sb_vis(0, 0, 320, 25) || sb_vis(0, 209, 320, 31)) sb_hud(g, band);
}

static void paint_found_alert(Gfx g)
{
    if (!s_fa_on) return;
    const uint8_t ac = accent();
    const uint8_t red = 228;                 /* mock #ff2a2a */
    const uint8_t hi = sb_tone(0);           /* #00666c on cyan, hexMix(ac,.55,0) */
    const bool wifi = s_fa_band == 0;
    const int band = wifi ? 0 : 1;
    const int d = (s_fa_dev >= 0 && s_fa_dev < LIST_N) ? s_fa_dev : 0;
    GATE_BOX(g, "OCCLUDE", "alert", 0, 0, 320, 240, "-");
    sb_rect(g, 0, 0, 320, 240, 4);
    for (int k = 0; k < 3; ++k) sb_frame(g, k, k, 320 - 2 * k, 240 - 2 * k, red);
    sb_frame(g, 3, 3, 314, 234, 132);   /* soft red glow off the border */
    sb_frame(g, 4, 4, 312, 232, 100);
    sb_rect(g, 8, 8, 304, 22, red);
    sb_text(g, "DEVICE FOUND", 16, 12, PAL_WHITE, red, false, 2);
    sb_text(g, wifi ? "WI-FI" : "BLE", 268, 15, PAL_WHITE, red, false, 1);
    sb_rect(g, 14, 40, 40, 40, 37);
    sb_frame(g, 14, 40, 40, 40, ac);
    dev_icon(g, (wifi ? LIST_WIFI_DT : LIST_BLE_DT)[d], 28, 54, false);
    char who[17];
    sb_upper(who, sizeof(who), sb_name(band, d), 15);
    sb_text(g, who, 64, 42, PAL_WHITE, 4, false, 2);
    char mac[24];
    sb_mac(mac, sizeof(mac), band, d, 4);
    if (wifi) {
        const size_t n = strlen(mac);
        snprintf(mac + n, sizeof(mac) - n, "  CH%u", (unsigned)(1u + sb_hash(sb_name(band, d)) % 11u));
    }
    sb_text(g, mac, 64, 62, 146, 4, false, 1);
    char db[16];
    snprintf(db, sizeof(db), "%d DBM", sb_db(band, d));
    sb_text(g, db, 64, 72, ac, 4, false, 1);
    static const int FA_ICO_I[FA_N] = { 0, 1, 3, 4 };
    for (int j = 0; j < FA_N; ++j) {
        const int y = 96 + j * 30;
        const bool on = j == s_fa_sel;
        const uint8_t row = on ? hi : (uint8_t)5;
        if (on) {
            /* mock canvas glow on the selected row: two soft accent rings */
            sb_frame(g, 12, y - 2, 296, 28, sb_tone(2));
            sb_frame(g, 13, y - 1, 294, 26, sb_tone(3));
        }
        sb_rect(g, 14, y, 292, 24, row);
        sb_rect(g, 14, y, on ? 3 : 1, 24, ac);
        blit_ico(g, ACT_ICO[FA_ICO_I[j]], 26, y + 8, on ? (uint8_t)PAL_WHITE : ac);
        char lab[12];
        sb_upper(lab, sizeof(lab), FA_NAME[j], 11);
        if (on) {
            sb_glow(g, lab, 42, y + 8, PAL_WHITE, sb_tone(4), 1);
            g.setTextColor(PAL_WHITE);
            g.drawString(lab, 43, y + 8); /* bold */
        } else {
            sb_glow(g, lab, 42, y + 8, COL_INK, sb_tone(3), 1);
        }
    }
}

static void paint_list(Gfx g, bool wifi)
{
    paint_sbox(g, wifi);
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
        int sx = px - 15;   /* mock: 4 shapes side by side, 1 px apart (31 px), inside the 32 px face */
        for (int id = 0; id < 4; ++id) {
            botcorps_sprite_draw(*g.raw(), sx + SPR_W[id] / 2 - g.ox, cy - g.oy, id, 1, live, false);
            sx += SPR_W[id] + 1;
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

/* v1016f-3 (Dirt 7:52): the mock's horizontal ESP32 shield settings board.
 * Five WROOM chip modules across the top carry UI COLOR / BACKGROUND / BOT SHAPE /
 * SCREEN SAVER / IDLE; the three board parts carry the remaining settings:
 *   JUMP = RAIN MODE (idx3), CP = RAIN COLOUR (idx4), PWR LED = SAVER on/off (idx6).
 * All eight settings keep their existing index, nudge_row edits and NVS values, and
 * stay keyboard-reachable (up/down through 0..7, left/right edits). */
/* v1016k (SPEC-LOCKED S-*): settings board = the master mock extControl, ported 1:1.
 * Board is a permanent MATTE-BLACK ESP32 shield (black PCB, white silk, gold header pins)
 * that never follows the UI colour; only the selection outline + feed trace use the accent.
 * Five square ESP32 packages U1..U5 (mock SB_PARTS) carry UI COLOR / BACKGROUND / BOT SHAPE /
 * SCREEN SAVER / IDLE; three small mock parts carry the rest:
 *   J1 jumper (268,150) = RAIN MODE (idx3), U7 qfn "CP" (196,150) = RAIN COLOUR (idx4),
 *   PWR LED (266,194) = SAVER on/off (idx6).
 * All eight settings keep their index, nudge_row edits and NVS values (keyboard only). */
static uint8_t wash22(uint8_t accent_idx);
static const int SB_BX = 8, SB_BY = 20, SB_BW = 304, SB_BH = 216; /* mock (3,18,314,220), inset 5 px so rain frames it */
static bool cp_part_rect(int index, int *x, int *y, int *w, int *h)
{
    switch (index) {
    case 0: *x = 38;  *y = 44;  *w = 40; *h = 72; return true; /* U1 UI COLOR    */
    case 1: *x = 82;  *y = 44;  *w = 40; *h = 72; return true; /* U2 BACKGROUND  */
    case 2: *x = 126; *y = 44;  *w = 40; *h = 72; return true; /* U3 BOT SHAPE   */
    case 5: *x = 170; *y = 44;  *w = 40; *h = 72; return true; /* U4 SCREEN SAVER*/
    case 7: *x = 214; *y = 44;  *w = 40; *h = 72; return true; /* U5 IDLE        */
    case 3: *x = 268; *y = 150; *w = 20; *h = 10; return true; /* J1 jumper  = RAIN MODE   */
    case 4: *x = 196; *y = 150; *w = 22; *h = 22; return true; /* U7 qfn CP  = RAIN COLOUR */
    case 6: *x = 266; *y = 194; *w = 10; *h = 8;  return true; /* PWR LED    = SAVER on/off */
    default: return false;
    }
}

static uint8_t sh_rgb(int r, int gg, int b)
{
    return botcorps_pal_fit((uint16_t)(((r >> 3) << 11) | ((gg >> 2) << 5) | (b >> 3)));
}

static void cp_short(int index, char *v, int n)
{
    /* mock sbShort6: <= 6 silk chars so nothing overruns a chip */
    static const char *MIXS[3] = { "MASHUP", "BOTS", "DEVS" };
    static const char *SHPS[5] = { "MIX", "ROUND", "CLSC", "TALL", "MINI" };
    static const char *SAVS[4] = { "BOTS", "GIB", "FLYBY", "BNCE" };
    if (index == 0) snprintf(v, (size_t)n, "%s", ACCENT_N[s_color]);
    else if (index == 1) snprintf(v, (size_t)n, "%s", MIXS[s_mix % 3]);
    else if (index == 2) snprintf(v, (size_t)n, "%s", SHPS[s_shape % 5]);
    else if (index == 5) snprintf(v, (size_t)n, "%s", SAVS[s_saver % 4]);
    else if (index == 7) snprintf(v, (size_t)n, "%dS", IDLE_V[s_idle]);
    else cp_text(index, v, n);
}

/* route from the USB-C feed (296,187) along the board bus (y 128), mock sbRoute */
static int cp_route(int index, int *px, int *py)
{
    int x, y, w, h;
    if (!cp_part_rect(index, &x, &y, &w, &h)) return 0;
    const bool chip = index == 0 || index == 1 || index == 2 || index == 5 || index == 7;
    const int cx = x + w / 2;
    const int cy = chip ? y + h : (y > 128 ? y : y + h);
    int n = 0;
    px[n] = 296; py[n++] = 187;
    px[n] = 286; py[n++] = 187;
    if (x >= 262) {
        const int ty = cy > 187 ? y : cy;
        px[n] = 286; py[n++] = ty;
        px[n] = cx;  py[n++] = ty;
    } else {
        px[n] = 286; py[n++] = 128;
        px[n] = cx;  py[n++] = 128;
        px[n] = cx;  py[n++] = cy;
    }
    return n;
}

static void cp_seg(Gfx g, int x0, int y0, int x1, int y1, int wdt, uint8_t c, bool log_it)
{
    const int x = x0 < x1 ? x0 : x1, y = y0 < y1 ? y0 : y1;
    const int w = (x0 < x1 ? x1 - x0 : x0 - x1) + wdt, h = (y0 < y1 ? y1 - y0 : y0 - y1) + wdt;
    g.fillRect(x, y, w, h, c);
    if (log_it) GATE_BOX(g, "TRACE", "sel", x, y, w, h, "-");
}

static void cp_chip(Gfx g, int index, bool on)
{
    int x, y, w, h;
    if (!cp_part_rect(index, &x, &y, &w, &h)) return;
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t ac = accent(), dark = sh_rgb(0x2a, 0x2a, 0x32);
    GATE_BOX(g, "PART", index == 0 ? "s0" : index == 1 ? "s1" : index == 2 ? "s2" : index == 5 ? "s5" : "s7", x, y, w, h, "BOARD");
    if (on) {
        g.fillRect(x - 3, y - 3, w + 6, h + 6, wash22(ac));
        g.drawRect(x - 3, y - 3, w + 6, h + 6, ac);
        GATE_BOX(g, "SEL", "outline", x - 3, y - 3, w + 6, h + 6, "-");
        g.fillRect(x, y, w, h, PAL_BLACK);
    }
    const int S = 34, sx = x + (w - S) / 2, sy = y + 12;
    const int lit = (int)((millis() / 90u) % 7u);
    for (int k = 0; k < 7; ++k) {                       /* gold pins on all four edges (mock sbChipH) */
        const int o = (3 * 2 + k * 9) / 2;
        const uint8_t pc = (on && lit == k) ? ac : gold;
        g.fillRect(sx + o, sy - 3, 2, 3, pc);
        g.fillRect(sx + o, sy + S, 2, 3, pc);
        g.fillRect(sx - 3, sy + o, 3, 2, pc);
        g.fillRect(sx + S, sy + o, 3, 2, pc);
    }
    g.fillRect(sx, sy, S, S, on ? ac : dark);           /* package edge */
    g.fillRect(sx + 1, sy + 1, S - 2, S - 2, PAL_BLACK); /* black face (never the UI colour) */
    const int fx = sx + 1, fy = sy + 1, fw = S - 2, fh = S - 2, mx = sx + S / 2, my = sy + S / 2;
    GATE_BOX(g, "FACE", "chip", fx, fy, fw, fh, "-");
    g.setClipRect(fx, fy, fw, fh);
    const int mode = botcorps_rain_mode();
    const uint8_t live = mode > 0 ? botcorps_rain_solid() : botcorps_rain_palette(0);
    if (index == 0) {
        /* UI colour: a SQUARE bot in the selected accent filling the face (mock botSquare) */
        botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, 4, ac, false);
    } else if (index == 1) {
        if (s_mix == 1) draw_blob(g, mx, my + 1, 2, live, 0, false);           /* crew */
        else if (s_mix == 2) gl_icon(g, GL_AP, mx - 6, my - 6, false);          /* devices */
        else {                                                                 /* mashup: bot + device */
            draw_blob(g, mx - 5, my - 3, 2, live, 0, false);
            g.fillRect(mx + 4, my + 2, 7, 7, PAL_DEV);
            g.fillRect(mx + 6, my + 4, 3, 3, PAL_BLACK);
        }
    } else if (index == 2) {
        shape_preview(g, mx, my);
    } else if (index == 5) {
        saver_preview(g, mx, my);
    } else if (index == 7) {
        const bool onS = s_saver_on != 0;
        g.fillRect(fx + 2, fy + 2, 3, fh - 4, onS ? ac : sh_rgb(0x3a, 0x0d, 0x22));
        char num[8];
        snprintf(num, sizeof(num), "%d", IDLE_V[s_idle]);
        txt(g, num, mx + 2, my - 7, onS ? (uint8_t)PAL_WHITE : silk2, lgfx::textdatum::top_center);
        txt(g, "s", mx + 2, my + 3, silk2, lgfx::textdatum::top_center);
    }
    g.clearClipRect();
    char up[16];
    cp_short(index, up, (int)sizeof(up));
    txt(g, up, x + w / 2, y + h - 12, on ? (uint8_t)PAL_WHITE : silk2, lgfx::textdatum::top_center);
}

static void cp_part(Gfx g, int index, bool on)
{
    int x, y, w, h;
    if (!cp_part_rect(index, &x, &y, &w, &h)) return;
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), rail = sh_rgb(0x02, 0x02, 0x03);
    const uint8_t metal = sh_rgb(0xb9, 0xbf, 0xcc), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t ac = accent();
    GATE_BOX(g, "PART", index == 3 ? "s3" : index == 4 ? "s4" : "s6", x, y, w, h, "BOARD");
    if (on) {
        g.drawRect(x - 3, y - 3, w + 6, h + 6, ac);
        GATE_BOX(g, "SEL", "outline", x - 3, y - 3, w + 6, h + 6, "-");
    }
    if (index == 3) {                              /* J1 = RAIN MODE: 3-pin header + yellow jumper */
        const bool solid = botcorps_rain_mode() > 0;
        g.fillRect(x, y + 2, w, 6, rail);
        for (int k = 0; k < 3; ++k) g.fillRect(x + 3 + k * 6, y + 4, 2, 2, gold);
        const int jx = solid ? x + 9 : x + 1;
        g.fillRect(jx, y, 11, 10, sh_rgb(0xff, 0xd4, 0x00));
        g.fillRect(jx + 2, y + 2, 7, 3, sh_rgb(0xb8, 0x94, 0x00));
    } else if (index == 4) {                       /* U7 qfn "CP" = RAIN COLOUR swatch */
        g.fillRect(x, y, w, h, sh_rgb(0x16, 0x16, 0x1b));
        for (int k = 0; k < 5; ++k) {
            g.fillRect(x + 3 + k * 4, y - 2, 2, 2, metal);
            g.fillRect(x + 3 + k * 4, y + h, 2, 2, metal);
            g.fillRect(x - 2, y + 3 + k * 4, 2, 2, metal);
            g.fillRect(x + w, y + 3 + k * 4, 2, 2, metal);
        }
        tiny_text(g, "CP", x + w / 2 - 3, y + 3, silk2);
        const int m = botcorps_rain_mode();
        g.fillRect(x + 4, y + 13, w - 8, 5, m <= 0 ? botcorps_rain_palette(0) : botcorps_rain_solid());
        g.fillRect(x + 2, y + 2, 2, 2, sh_rgb(0x5d, 0x5d, 0x6a));
    } else if (index == 6) {                       /* PWR LED = SAVER on/off */
        const bool pon = s_saver_on != 0;
        g.fillRect(x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
        g.fillRect(x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
    }
}
int botcorps_cp_overlap_count(void)
{
    int rx[8], ry[8], rw[8], rh[8], n = 0;
    for (int i = 0; i < CP_N && n < 8; ++i) {
        int x, y, w, h;
        if (cp_part_rect(i, &x, &y, &w, &h)) { rx[n]=x; ry[n]=y; rw[n]=w; rh[n]=h; ++n; }
    }
    int bad = 0;
    for (int a = 0; a < n; ++a)
        for (int b = a + 1; b < n; ++b)
            if (rx[a] < rx[b]+rw[b] && rx[b] < rx[a]+rw[a] &&
                ry[a] < ry[b]+rh[b] && ry[b] < ry[a]+rh[a]) ++bad;
    return bad;
}

int botcorps_cp_edge_overflow_count(void)
{
    int bad = 0;
    for (int i = 0; i < CP_N; ++i) {
        int x, y, w, h;
        if (!cp_part_rect(i, &x, &y, &w, &h)) continue;
        if (x < SB_BX || y < SB_BY || x + w > SB_BX + SB_BW || y + h > SB_BY + SB_BH) ++bad;
    }
    return bad;
}

int botcorps_cp_sprite_inside(const uint8_t *buf)
{
    (void)buf;
    return 0;   /* superseded by tools/gate (G2 sprite-in-chip from the element log) */
}

static void paint_cp_row(Gfx g, int index, bool on)
{
    if (index == 0 || index == 1 || index == 2 || index == 5 || index == 7) cp_chip(g, index, on);
    else cp_part(g, index, on);
}

static void gib_crown(Gfx g, int cx, int y);
static void paint_control(Gfx g)
{
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = sh_rgb(0xe9, 0xe9, 0xee), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t rail = sh_rgb(0x02, 0x02, 0x03), trace = sh_rgb(0x1c, 0x1c, 0x24), metal = sh_rgb(0x8f, 0x95, 0xa3);
    const uint8_t ac = accent();
    /* top chrome bar: part dots, crown (BACK), terminal (TERM) */
    g.fillRect(0, 0, 320, 16, PAL_BLACK);
    g.fillRect(0, 16, 320, 1, ac);
    for (int k = 0; k < CP_N; ++k) g.fillRect(6 + k * 6, 7, 4, 3, k == s_cursor ? ac : sh_rgb(0x2a, 0x2a, 0x34));
    {
        const bool bsel = s_cursor == CP_N + 1;
        gib_crown(g, 160, 3);
        if (bsel) g.drawRect(160 - 18, 0, 36, 16, PAL_WHITE);
        const bool tsel = s_cursor == CP_N;
        const uint8_t tc = tsel ? (uint8_t)PAL_WHITE : ac;
        const int tx = 196;
        GATE_BOX(g, "BTN", "term", tx - 2, 1, 22, 14, "-");
        g.drawRect(tx, 3, 18, 11, tc); g.fillRect(tx, 3, 18, 2, tc);
        g.drawLine(tx + 3, 7, tx + 6, 9, tc); g.drawLine(tx + 6, 9, tx + 3, 11, tc);
        g.fillRect(tx + 8, 11, 5, 1, tc);
        if (tsel) g.drawRect(tx - 2, 1, 22, 14, PAL_WHITE);
    }
    /* matte-black PCB (fixed), silk mounting holes */
    GATE_BOX(g, "BOARD", "settings", SB_BX, SB_BY, SB_BW, SB_BH, "-");
    g.fillRect(SB_BX, SB_BY, SB_BW, SB_BH, sh_rgb(0x0a, 0x0a, 0x0c));
    const int hx[4] = { 14, 306, 14, 306 }, hy[4] = { 26, 26, 230, 230 };
    for (int k = 0; k < 4; ++k) { g.fillCircle(hx[k], hy[k], 4, silk); g.fillCircle(hx[k], hy[k], 2, PAL_BLACK); }
    /* header rails top + bottom, labels upright: top labels sit UNDER the top rail, ABOVE the chips */
    static const char *const TP[10] = { "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16" };
    static const char *const BP[10] = { "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27" };
    g.fillRect(36, 22, 224, 8, rail);
    g.fillRect(36, 218, 224, 8, rail);
    for (int k = 0; k < 10; ++k) {
        const int x0 = 40 + k * 22;
        for (int q = 0; q < 3; ++q) { g.fillRect(x0 + 6 + q * 4, 25, 2, 2, gold); g.fillRect(x0 + 6 + q * 4, 221, 2, 2, gold); }
        txt(g, TP[k], x0 + 11, 31, silk, lgfx::textdatum::top_center);
        txt(g, BP[k], x0 + 11, 209, silk, lgfx::textdatum::top_center);
    }
    /* dev-kit outline + pin dot rows + SMD resistor row + data micro-USB */
    g.drawRect(34, 40, 227, 165, sh_rgb(0x2b, 0x2b, 0x33));
    for (int k = 0; k < 19; ++k) {
        for (int r = 0; r < 2; ++r) {
            const int y0 = r ? 198 : 134;
            g.fillRect(40 + k * 11, y0, 3, 3, sh_rgb(0x5a, 0x5a, 0x66));
            g.fillRect(41 + k * 11, y0 + 1, 1, 1, PAL_BLACK);
        }
    }
    for (int k = 0; k < 6; ++k) { g.fillRect(90 + k * 14, 160, 6, 3, sh_rgb(0xb8, 0xb8, 0xc0)); g.fillRect(92 + k * 14, 160, 2, 3, sh_rgb(0x20, 0x20, 0x24)); }
    g.fillRect(238, 156, 12, 22, metal); g.fillRect(241, 159, 6, 16, sh_rgb(0x1a, 0x1a, 0x20));
    /* power section at the right end: DC jack, power micro-USB, AMS1117, USB-C */
    g.fillRect(270, 36, 40, 36, sh_rgb(0x15, 0x15, 0x18)); g.fillRect(275, 41, 30, 26, sh_rgb(0x05, 0x05, 0x06));
    g.fillCircle(290, 54, 6, sh_rgb(0x2a, 0x2a, 0x30));
    txt(g, "DC6.5", 290, 75, silk, lgfx::textdatum::top_center);
    g.fillRect(296, 128, 13, 22, metal); g.fillRect(299, 131, 7, 16, sh_rgb(0x1a, 0x1a, 0x20));
    {   /* U6 AMS1117 regulator (mock 268,92 30x30) */
        const int x = 268, y = 92;
        g.fillRect(x + 4, y, 22, 6, sh_rgb(0xa9, 0xaf, 0xbb));
        g.fillRect(x, y + 6, 30, 16, sh_rgb(0x1a, 0x1a, 0x1f));
        for (int k = 0; k < 3; ++k) g.fillRect(x + 5 + k * 9, y + 22, 3, 6, sh_rgb(0xa9, 0xaf, 0xbb));
        txt(g, "1117C", x + 15, y + 8, silk2, lgfx::textdatum::top_center);
        for (int k = 0; k < 3; ++k) g.fillRect(x + 6 + k * 7, y + 16, 5, 3, sh_rgb(0x2a, 0x2a, 0x34));
    }
    {   /* USB-C (mock 298,172 17x30; kept inside the board edge) */
        const int x = 293, y = 172, w = 17, h = 30;
        g.fillRect(x, y, w, h, sh_rgb(0x9a, 0xa1, 0xb2)); g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xd0, 0xd5, 0xdf));
        g.fillRect(x + 4, y + 4, w - 8, h - 8, sh_rgb(0x0a, 0x0a, 0x10)); g.fillRect(x + 7, y + 8, 2, h - 16, gold);
    }
    txt(g, "JUMP", 270, 163, silk, lgfx::textdatum::top_center);
    txt(g, "USB5V", 312, 208, silk, lgfx::textdatum::top_right);
    txt(g, "PWR", 264, 205, silk2, lgfx::textdatum::top_left);
    /* C1 tantalum + BOOT / EN buttons (decorative, mock coords) */
    g.fillRect(52, 156, 22, 12, sh_rgb(0xe0, 0xa2, 0x1c)); g.fillRect(52, 156, 5, 12, sh_rgb(0x7a, 0x52, 0x08));
    g.fillRect(59, 159, 12, 6, sh_rgb(0x5a, 0x3a, 0x00)); txt(g, "C1", 52, 147, silk, lgfx::textdatum::top_left);
    for (int b = 0; b < 2; ++b) {
        const int x = 236, y = b ? 182 : 136, w = 16, h = 14;
        g.fillRect(x, y, w, h, metal); g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xc3, 0xc9, 0xd6));
        g.fillCircle(x + w / 2, y + h / 2, 4, sh_rgb(0x22, 0x23, 0x2c));
        g.fillRect(x - 2, y + 1, 2, 3, gold); g.fillRect(x + w, y + 1, 2, 3, gold);
        g.fillRect(x - 2, y + h - 4, 2, 3, gold); g.fillRect(x + w, y + h - 4, 2, 3, gold);
        if (b) txt(g, "EN", x + w + 4, y + 4, silk, lgfx::textdatum::top_left);
        else txt(g, "BOOT", x - 4, y + 4, silk, lgfx::textdatum::top_right);
    }
    /* dark feed traces for every part, then the selected feed in the accent with moving pulses */
    int px[6], py[6];
    for (int i = 0; i < CP_N; ++i) {
        const int n = cp_route(i, px, py);
        for (int q = 0; q + 1 < n; ++q) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 1, trace, false);
    }
    if (s_cursor >= 0 && s_cursor < CP_N) {
        const int n = cp_route(s_cursor, px, py);
        int len = 0;
        for (int q = 0; q + 1 < n; ++q) {
            const uint8_t wire = (s_color == 1) ? (uint8_t)PAL_WIRE_A : (s_color == 2) ? (uint8_t)PAL_WIRE_M : (uint8_t)PAL_WIRE_C;
            cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 2, wire, true);
            len += abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
        }
        for (int k = 0; k < 3 && len > 0; ++k) {
            int d = (int)(((millis() * 120u) / 1000u + (uint32_t)(k * len / 3)) % (uint32_t)len);
            for (int q = 0; q + 1 < n; ++q) {
                const int l2 = abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
                if (d <= l2) {
                    const int qx = px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0);
                    const int qy = py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0);
                    g.fillRect(qx - 1, qy - 1, 4, 4, ac);
                    break;
                }
                d -= l2;
            }
        }
    }
    for (int i = 0; i < CP_N; ++i) paint_cp_row(g, i, s_cursor == i);
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
    GATE_BOX(g, "CROWN", "back", x0 - 4, y - 3, 13 * 2 + 8, 7 * 2 + 5, "-");
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
    /* v1016f-3 (Dirt 7:57): reverted the teal fill; background stays as before */
    g.fillCircle(cx, cy, r - 5, PAL_DEEP);
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
    /* v1017b: hub command-menu stop (Up/Down zone 2), right end of the prompt strip. */
    const bool cm = s_hub_zone == 2;
    const int mx = 258, my = 187;
    g.fillRect(mx, my, 18, 12, PAL_DEEP);
    g.drawRect(mx, my, 18, 12, cm ? ac : (uint8_t)COL_LINE);
    if (cm) g.drawRect(mx - 1, my - 1, 20, 14, PAL_WHITE);
    for (int k = 0; k < 3; ++k) g.fillRect(mx + 4, my + 3 + k * 3, 10, 1, cm ? (uint8_t)PAL_WHITE : (uint8_t)COL_MUTED);
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
    /* Sparse code field so the hub matches the v1 rain ground on a host with no rain sprite. */
    for (int col = 0; col < 18; ++col) {
        const int x = 8 + col * 18;
        const int n = 4 + (col % 5);
        const int y0 = (int)((t / 40u + (uint32_t)col * 37u) % 200u);
        for (int k = 0; k < n; ++k) {
            const int y = (y0 + k * 10) % 200;
            if (y < 18 || y > 200) continue;
            g.fillRect(x, y, 1, 6, (k == 0) ? (uint8_t)PAL_CODE : (uint8_t)PAL_LINE);
        }
    }
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
        if (len > 0) {
            g.fillRect(x1, ys - 1, len, 1, PAL_LINE);
            g.fillRect(x1, ys, len, 1, ac);
            g.fillRect(x1 + len / 2, ys - 2, 3, 3, bot_color(i));
        }
        if (out) {
            const int y0 = ys < ry ? ys : ry;
            g.fillRect(left ? x2 : x1 - 1, y0, 1, (ys > ry ? ys - ry : ry - ys) + 1, ac);
        }
        const int travel = len > 0 ? len : 1;
        const int d = (int)((t / 33u + (uint32_t)i * 11u) % (uint32_t)(travel + 10));
        if (d < travel) g.fillRect((left ? x1 + d : x2 - d) - 1, ys - 1, 2, 3, ac);
        via_node(g, left ? x2 - 1 : x1 - 2, (out ? ry : ys) - 1, ac);
        /* v1016f-2 #2: hub tiles are dim like the mock; the bot face keeps its fixed colour. */
        g.fillRect(sx - 2, sy - 2, 32, 24, wire); /* v1016g: mock-lit glow halo */
        GATE_BOX(g, "TILE", "hubbot", sx, sy, 28, 20, "-");
        chip_plate(g, sx, sy, 28, 20, PAL_DEEP, accent());
        g.drawRect(sx - 1, sy - 1, 30, 22, accent());
        draw_blob(g, sx + 11, sy + 10, 2, bot_color(i), botcorps_shape_for(i), false);
        g.fillRect(sx + 22, sy + 3, 4, 4, dot_col(i, t, mood, asleep));
    }
    /* v1016k (Dirt 9:31): hub MAIN screen is the animated moving rings core,
     * not a chip. Square chip_block applies only to Gibson inside boards. */
    GATE_BOX(g, "HUBCORE", "rings", cx - 54, cy - 54, 108, 108, "-");
    hub_die(g, cx, cy, 34);
    px_ring(g, cx, cy, 34, PAL_CYAN, 0, 0, 0);
    const int spin = mood == MOOD_HUNTING ? (int)(t / 30u) : mood == MOOD_EXCITED ? (int)(t / 50u) : (int)(t / 90u);
    px_ring(g, cx, cy, 37, PAL_CYAN, 5, 5, spin);
    if (mood == MOOD_HUNTING) {
        for (int k = 0; k < 14; ++k) {
            const float ang = (float)t / 260.f - (float)k * 0.07f;
            const uint8_t ink = k < 3 ? (uint8_t)PAL_WHITE : ac;
            g.fillRect(cx + (int)(cosf(ang) * 40.f), cy + (int)(sinf(ang) * 40.f), 1, 1, ink);
        }
    }
    px_ring(g, cx, cy, 42, PAL_CYAN, 2, 4, (int)(t / 140u));
    px_ring(g, cx, cy, 48, PAL_CYAN, 1, 5, (int)(t / 80u));
    px_ring(g, cx, cy, 54, PAL_CYAN, 2, 6, (int)(t / 200u));
    const int ph = (int)((t % 2400u) * 5u / 2400u);
    px_ring(g, cx, cy, 44 + ph, PAL_CYAN, 3, 3, 0);
    hub_sparks(g, cx, cy, t);
    g.fillRect(cx - 14, cy + 25, 28, 1, wire);
    hub_gibson(g, cx, cy, 7);
    if (s_hub_zone == 3) {
        /* v1017b: Gibson focused (Enter = Gibson game). */
        g.drawCircle(cx, cy, 31, PAL_WHITE);
        g.drawCircle(cx, cy, 30, ac);
    }
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
    /* v1016k (SPEC H2): lit nav plates like the mock - dark accent face, bright 2 px accent
     * rim with cut corners, accent glow halo (stronger on the selected plate). */
    static const int xs[3] = { 110, 146, 182 };
    const uint16_t a565 = botcorps_rgb565(ac);
    auto tint = [&](int pct) -> uint8_t {
        const int r = ((a565 >> 11) & 31) * pct / 100, gg = ((a565 >> 5) & 63) * pct / 100, b = (a565 & 31) * pct / 100;
        return botcorps_pal_fit((uint16_t)((r << 11) | (gg << 5) | b));
    };
    const uint8_t face = tint(22), glow1 = tint(55), glow2 = tint(30);
    const uint8_t gold = (uint8_t)PAL_AMBER;
    for (int i = 0; i < 3; ++i) {
        const int x = xs[i], y = 206;
        const bool sel = s_hub_zone == 0 && s_cursor == i;
        GATE_BOX(g, "BTN", "hubnav", x, y, BS, BS, "-");
        /* glow halo */
        g.fillRect(x - 3, y - 2, BS + 6, BS + 4, glow2);
        g.fillRect(x - 2, y - 3, BS + 4, BS + 6, glow2);
        if (sel) {
            /* v1017b (Dirt 8:26): selected hub button must read at a glance - full accent
             * halo, white outer outline, accent inner ring. */
            g.fillRect(x - 3, y - 3, BS + 6, BS + 6, ac);
            g.drawRect(x - 4, y - 4, BS + 8, BS + 8, PAL_WHITE);
            g.drawRect(x - 2, y - 2, BS + 4, BS + 4, glow1);
        }
        /* rim (2 px, corners cut) + face */
        g.fillRect(x + 1, y, BS - 2, BS, ac);
        g.fillRect(x, y + 1, BS, BS - 2, ac);
        g.fillRect(x + 2, y + 2, BS - 4, BS - 4, face);
        if (sel) {
            g.fillRect(x + 2, y, BS - 4, 1, PAL_WHITE);
            g.fillRect(x + 2, y + BS - 1, BS - 4, 1, PAL_WHITE);
            g.fillRect(x, y + 2, 1, BS - 4, PAL_WHITE);
            g.fillRect(x + BS - 1, y + 2, 1, BS - 4, PAL_WHITE);
        }
        if (i == 0) {            /* terminal: black screen, white bezel, >_ */
            for (int k = 0; k < 5; ++k) g.fillRect(x + 6 + k * 4, y + 4, 2, 2, gold);
            g.fillRect(x + 4, y + 8, 20, 15, PAL_WHITE);
            g.fillRect(x + 5, y + 9, 18, 13, PAL_BLACK);
            g.drawLine(x + 7, y + 12, x + 10, y + 15, PAL_WHITE);
            g.drawLine(x + 10, y + 15, x + 7, y + 18, PAL_WHITE);
            g.fillRect(x + 12, y + 18, 6, 1, PAL_WHITE);
        } else if (i == 1) {     /* scan: square chip with gold pins + accent grid + sweep */
            for (int k = 0; k < 4; ++k) {
                const int o = 8 + k * 4;
                g.fillRect(x + o, y + 3, 2, 2, gold);
                g.fillRect(x + o, y + 23, 2, 2, gold);
                g.fillRect(x + 3, y + o, 2, 2, gold);
                g.fillRect(x + 23, y + o, 2, 2, gold);
            }
            g.drawRect(x + 6, y + 6, 16, 16, ac);
            g.fillRect(x + 7, y + 7, 14, 14, PAL_BLACK);
            for (int k = 1; k < 3; ++k) {
                g.fillRect(x + 7 + k * 5, y + 7, 1, 14, ac);
                g.fillRect(x + 7, y + 7 + k * 5, 14, 1, ac);
            }
            g.fillRect(x + 8, y + 14, 12, 1, PAL_WHITE);
        } else {                 /* control: black board, accent chip, gold pin rows, jumper */
            g.fillRect(x + 4, y + 6, 20, 16, PAL_BLACK);
            g.drawRect(x + 4, y + 6, 20, 16, ac);
            for (int k = 0; k < 5; ++k) {
                g.fillRect(x + 6 + k * 4, y + 8, 2, 1, gold);
                g.fillRect(x + 6 + k * 4, y + 19, 2, 1, gold);
            }
            g.drawRect(x + 7, y + 10, 8, 8, ac);
            g.fillRect(x + 18, y + 11, 3, 6, gold);
            g.fillRect(x + 18, y + 13, 3, 2, ac);
        }
    }
}


static void px_ring(Gfx g, int cx, int cy, int r, uint8_t col, int on, int off, int phase);
static void hub_die(Gfx g, int cx, int cy, int r);
/* v1016f #1: the intro rain carries the Gibson in its ring disc (mock 'rain' screen). No button. */
static void paint_rain_btn(Gfx g)
{
    const int cx = 160, cy = 100;
    const uint32_t t = millis();
    /* v1016k (Dirt 9:31): intro/rain screen is the ring disc, not a chip. */
    g.fillCircle(cx, cy, 56, PAL_BLACK);
    hub_die(g, cx, cy, 34);
    px_ring(g, cx, cy, 34, PAL_CYAN, 0, 0, 0);
    px_ring(g, cx, cy, 37, PAL_CYAN, 5, 5, (int)(t / 90u));
    px_ring(g, cx, cy, 42, PAL_CYAN, 2, 4, (int)(t / 140u));
    px_ring(g, cx, cy, 48, PAL_CYAN, 1, 5, (int)(t / 80u));
    px_ring(g, cx, cy, 54, PAL_CYAN, 2, 6, (int)(t / 200u));
    g.fillRect(cx - 14, cy + 25, 28, 1, wire_ink());
    hub_gibson(g, cx, cy, 9);
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
    g.fillRect(0, 0, 320, 184, PAL_BLACK);   /* v1016k: rain shows below the readout */
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
    crown_btn(g, true);
}

/* Chat shares the terminal log so replies keep each bot's colour. No new buffer. */
static void gc_set_count(int n)
{
    if (n <= 0) log_clear();
}

static void gc_push(int bot, const char *s)
{
    log_add_bot(bot, s);
}

static int gc_bot_of(const char *arg)
{
    if (!arg || !arg[0]) return -1;
    if (strcmp(arg, "gibson") == 0 || strcmp(arg, "gib") == 0) return 12;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(BOTS[i].short_name, arg, strlen(arg)) == 0) return i;
    }
    return -2;
}

static void gc_run(const char *line)
{
    char buf[40];
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
    char echo[40];
    snprintf(echo, sizeof(echo), "gibson> %.16s%s%.10s", cmd, (arg && arg[0]) ? " " : "", arg ? arg : "");
    log_add(echo);
    if (strcmp(cmd, "jackin") == 0) {
        gc_push(12, "jacking you in");
        gc_push(3, "shields up");
        gc_push(5, "eyes on the lanes");
        s_act = 2;
        s_mood = MOOD_ALERT;
        s_mood_until = millis() + 2400u;
        s_bus_t0 = millis();
        return;
    }
    if (strcmp(cmd, "defend") == 0 || strcmp(cmd, "lobby") == 0 || strcmp(cmd, "score") == 0
        || (strcmp(cmd, "load") == 0 && arg && strcmp(arg, "game") == 0)) {
        gc_push(12, "jackin to enter");
        return;
    }
    if (strcmp(cmd, "clear") == 0) {
        gc_set_count(0);
        return;
    }
    if (strcmp(cmd, "crew") == 0) {
        gc_push(12, "12 boards linked");
        return;
    }
    if (strcmp(cmd, "status") == 0) {
        gc_push(12, "link up");
        return;
    }
    if (strcmp(cmd, "mood") == 0) {
        gc_push(12, "crown lit");
        return;
    }
    if (strcmp(cmd, "why") == 0 || strcmp(cmd, "about") == 0) {
        gc_push(12, strcmp(cmd, "why") == 0 ? "i route the crew" : "gibson chat");
        return;
    }
    if (strcmp(cmd, "history") == 0) {
        if (s_hist_n <= 0) gc_push(-1, "history empty");
        else gc_push(-1, botcorps_t0_term_hist[s_hist_n - 1]);
        return;
    }
    if (strcmp(cmd, "terminal") == 0) {
        enter_screen(SCR_TERMINAL, SCR_GCHAT);
        boot_log();
        return;
    }
    if (strcmp(cmd, "control") == 0) {
        enter_screen(SCR_CONTROL, SCR_GCHAT);
        return;
    }
    if (strcmp(cmd, "scan") == 0) {
        s_act = 1;
        s_bus_t0 = millis();
        if (arg && strcmp(arg, "wifi") == 0) {
            gc_push(0, "cafe-guest -42");
            gc_push(-1, "[ ok ] wifi sweep");
        } else if (arg && (strcmp(arg, "ble") == 0 || strcmp(arg, "bluetooth") == 0)) {
            gc_push(1, "earbuds-58 -58");
            gc_push(-1, "[ ok ] ble sweep");
        } else {
            gc_push(12, "bus sweep - 12 scouts");
            gc_push(0, "cafe-guest -42");
            gc_push(-1, "[ ok ] crew sweep");
        }
        return;
    }
    if (strcmp(cmd, "go") == 0 || strcmp(cmd, "load") == 0) {
        if (!arg || !arg[0]) {
            gc_push(12, "terminal scan control hub");
            return;
        }
        if (strcmp(arg, "hub") == 0 || strcmp(arg, "gibson") == 0) {
            enter_screen(SCR_GIBSON, SCR_GCHAT);
            return;
        }
        if (strcmp(arg, "terminal") == 0) {
            enter_screen(SCR_TERMINAL, SCR_GCHAT);
            boot_log();
            return;
        }
        if (strcmp(arg, "scan") == 0 || strcmp(arg, "busscan") == 0) {
            s_scan_bot = 12;
            enter_screen(SCR_SCAN, SCR_GCHAT);
            return;
        }
        if (strcmp(arg, "control") == 0) {
            enter_screen(SCR_CONTROL, SCR_GCHAT);
            return;
        }
        gc_push(12, "no such screen");
        return;
    }
    if (strcmp(cmd, "ask") == 0 || strcmp(cmd, "hail") == 0 || strcmp(cmd, "whisper") == 0) {
        const int bot = gc_bot_of(arg);
        if (bot < 0) {
            gc_push(12, bot == -1 ? "name a bot" : "no such bot");
            return;
        }
        const char *who = bot == 12 ? "gibson" : BOTS[bot].short_name;
        char row[30];
        if (strcmp(cmd, "ask") == 0) {
            const char *st = bot == 12 ? "ready" : BOTS[bot].st == 2 ? "hold" : "ready";
            snprintf(row, sizeof(row), "%s %s", who, st);
        } else if (strcmp(cmd, "whisper") == 0) {
            snprintf(row, sizeof(row), "%s quiet", who);
        } else {
            snprintf(row, sizeof(row), "%s %s", who, HAIL[bot == 12 ? 0 : bot + 1]);
        }
        gc_push(bot, row);
        return;
    }
    gc_push(12, "no such command");
}

static void gc_pick(void)
{
    if (s_sheet_i < 0) s_sheet_i = 0;
    if (s_sheet_i >= GC_N) s_sheet_i = GC_N - 1;
    s_sheet = false;
    if (GC_ARG[s_sheet_i]) {
        snprintf(s_gp_buf, sizeof(s_gp_buf), "%s ", GC_NM[s_sheet_i]);
        return;
    }
    s_gp_buf[0] = 0;
    gc_run(GC_NM[s_sheet_i]);
}

static void gc_row_at(int row, int *hdr, int *cmd)
{
    int r = 0;
    for (int g = 0; g < 5; ++g) {
        if (r == row) {
            *hdr = g;
            *cmd = -1;
            return;
        }
        ++r;
        for (int i = GC_GAT[g]; i < GC_GAT[g + 1]; ++i) {
            if (r == row) {
                *hdr = -1;
                *cmd = i;
                return;
            }
            ++r;
        }
    }
    *hdr = -1;
    *cmd = 0;
}

static void paint_gc_sheet(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    const int rh = 12;
    const int vis = 8;
    const int rows = GC_N + 5;
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= GC_N) sel = GC_N - 1;
    int sel_row = 0;
    int seen = 0;
    for (int row = 0; row < rows; ++row) {
        int hdr = -1;
        int cmd = 0;
        gc_row_at(row, &hdr, &cmd);
        if (hdr < 0 && cmd == sel) {
            sel_row = row;
            break;
        }
        if (hdr < 0) ++seen;
        (void)seen;
    }
    int top = sel_row - 3;
    if (top < 0) top = 0;
    if (top > rows - vis) top = rows - vis;
    const int h = vis * rh + 10;
    const int bx = 14;
    const int by = 214 - 5 - h;
    GATE_BOX(g, "OCCLUDE", "gcsheet", bx, by, 292, h, "-");
    g.fillRect(bx, by, 292, h, PAL_BLACK);
    g.drawRect(bx, by, 292, h, ac);
    g.fillRect(bx + 3, by + h - 3, 286, 1, PAL_WIRE_C);
    const int keep = s_px;
    s_px = 6;
    for (int k = 0; k < vis; ++k) {
        int hdr = -1;
        int cmd = 0;
        gc_row_at(top + k, &hdr, &cmd);
        const int y = by + 5 + k * rh;
        if (hdr >= 0) {
            txt(g, GC_GN[hdr], bx + 8, y, ac, lgfx::textdatum::top_left);
            g.fillRect(bx + 48, y + 4, 220, 1, PAL_WIRE_C);
            continue;
        }
        const bool on = cmd == sel;
        const uint8_t bg = on ? ac : (uint8_t)PAL_BLACK;
        const uint8_t fg = on ? (uint8_t)PAL_BLACK : (uint8_t)COL_INK;
        if (on) g.fillRect(bx + 2, y - 1, 288, rh - 1, ac);
        const int ii = GC_II[cmd];
        if (ii >= 0 && ii < CMD_N) blit_ico(g, CMD_ICO[ii], bx + 6, y, fg);
        txt_bg(g, GC_NM[cmd], bx + 18, y, fg, bg, lgfx::textdatum::top_left);
        txt_bg(g, GC_PP[cmd], bx + 84, y, on ? (uint8_t)PAL_BLACK : (uint8_t)COL_MUTED,
               bg, lgfx::textdatum::top_left);
    }
    s_px = keep;
}

static void paint_gc_rail(Gfx g, uint8_t ac)
{
    const int x = 108;
    const int ys[4] = { 24, 48, 72, 96 };
    for (int i = 0; i < 4; ++i) {
        const int y = ys[i];
        const uint8_t edge = (i == 3) ? (uint8_t)PAL_HOLD : ac;
        g.fillRect(x, y, 18, 16, PAL_BLACK);
        g.drawRect(x, y, 18, 16, edge);
        if (i == 0) {
            g.drawRect(x + 4, y + 3, 10, 8, ac);
            g.fillRect(x + 6, y + 5, 6, 2, ac);
        } else if (i == 1) {
            g.drawCircle(x + 8, y + 7, 4, ac);
            g.fillRect(x + 11, y + 10, 4, 1, ac);
        } else if (i == 2) {
            g.fillRect(x + 4, y + 4, 10, 1, ac);
            g.fillRect(x + 4, y + 8, 10, 1, ac);
            g.fillRect(x + 4, y + 12, 10, 1, ac);
            g.fillRect(x + 6, y + 3, 2, 3, PAL_WHITE);
            g.fillRect(x + 10, y + 7, 2, 3, PAL_WHITE);
        } else {
            g.fillRect(x + 4, y + 6, 3, 3, PAL_HOLD);
            g.fillRect(x + 8, y + 4, 3, 3, PAL_IVORY);
            g.fillRect(x + 12, y + 7, 3, 3, ac);
        }
    }
}

static void paint_gc_route(Gfx g, uint8_t ac)
{
    const int x = 132;
    const int y = 20;
    const int w = 180;
    const int h = 126;
    g.fillRect(x, y, w, h, PAL_BLACK);
    g.drawRect(x - 2, y - 2, w + 4, h + 4, PAL_HOLD);
    g.drawRect(x - 1, y - 1, w + 2, h + 2, PAL_BFB72);
    g.drawRect(x, y, w, h, PAL_BFB72);
    g.drawRect(x + 1, y + 1, w - 2, h - 2, PAL_HOLD);
    for (int gy = 16; gy < h - 8; gy += 16) g.fillRect(x + 8, y + gy, w - 16, 1, PAL_LINE);
    for (int gx = 16; gx < w - 8; gx += 18) g.fillRect(x + gx, y + 8, 1, h - 16, PAL_LINE);
    dev_icon(g, DEV_DEAUTH, x + 18, y + 16, false);
    dev_icon(g, DEV_TAG, x + 70, y + 14, false);
    dev_icon(g, DEV_AP, x + 120, y + 18, false);
    dev_icon(g, DEV_CAM, x + 36, y + 52, false);
    dev_icon(g, DEV_BT, x + 96, y + 70, false);
    dev_icon(g, DEV_DRONE, x + 140, y + 48, false);
    chip_block(g, x + 92, y + 54, 28, 12); /* v1016f-4 (Dirt 8:31): Gibson CPU = square block */
    for (int k = 0; k < 5; ++k) {
        g.fillRect(x + 16 + k * 8, y + 96, 3, 2, PAL_OK);
        g.fillRect(x + 20 + k * 10, y + h - 14, 6, 2, PAL_HOLD);
    }
    if (s_act == 1) {
        const uint32_t u = (millis() - s_bus_t0) % 1100u;
        const int px = (int)(u * (uint32_t)(w - 24) / 1100u);
        g.fillRect(x + 10, y + h - 8, px, 2, ac);
    }
}

static void paint_gchat(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    const uint32_t t = millis();
    g.fillRect(0, 0, 320, 16, PAL_BLACK);       /* v1016k: no full-screen fill, rain shows between panels */
    g.fillRect(0, 16, 320, 1, ac);
    GATE_BOX(g, "CROWN", "back", 4, 2, 13, 7, "-");
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 13; ++c) {
            if (GIB_CROWN[r][c] != '#') continue;
            g.fillRect(4 + c, 2 + r, 1, 1, r >= 5 ? (uint8_t)PAL_CHIN : (uint8_t)PAL_IVORY);
        }
    }
    for (int i = 0; i < 12; ++i) {
        g.fillRect(28 + i * 9, 5, 6, 5, bot_color(i));
    }
    const int sg = 3 + (int)((t / 2500u) % 2u);
    for (int k = 0; k < 4; ++k) {
        const int bh = (k + 1) * 2;
        g.fillRect(236 + k * 4, 12 - bh, 3, bh, k < sg ? ac : (uint8_t)PAL_LINE);
    }
    g.fillRect(290, 5, 4, 4, (uint32_t)(t - s_last_key) < 220u ? ac : (uint8_t)PAL_LINE);
    const int keep = s_px;
    s_px = 6;
    txt(g, "RX", 296, 4, (uint32_t)(t - s_last_key) < 220u ? ac : (uint8_t)COL_MUTED,
        lgfx::textdatum::top_left);
    s_px = keep;
    chip_block(g, 56, 78, 36, 12); /* v1016f-4: Gibson block, no disc/ring */
    paint_gc_rail(g, ac);
    paint_gc_route(g, ac);
    g.fillRect(4, 148, 312, 88, PAL_BLACK);
    g.drawRect(4, 148, 312, 88, ac);
    g.drawRect(5, 149, 310, 86, PAL_WIRE_C);
    g.fillRect(6, 150, 250, 1, (uint32_t)(t - s_bus_t0) < 160u ? ac : (uint8_t)PAL_WIRE_C);
    chip_block(g, 294, 214, 28, 12);
    s_px = 6;
    const int n = s_log_n < 6 ? s_log_n : 6;
    const int start = s_log_n - n;
    for (int k = 0; k < n; ++k) {
        const char *ln = log_line(start + k);
        const int y = 156 + k * 9;
        if (ln && ln[0] == '\x01') {
            const int b = (unsigned char)ln[1];
            const char *show = ln + 2;
            if (b >= 0 && b < 12) {
                draw_blob(g, 16, y + 4, 1, bot_color(b), botcorps_shape_for(b), false);
                txt_bg(g, BOTS[b].short_name, 24, y, bot_color(b), PAL_BLACK, lgfx::textdatum::top_left);
                const int nx = 24 + (int)strlen(BOTS[b].short_name) * 6;
                txt_bg(g, " - ", nx, y, bot_color(b), PAL_BLACK, lgfx::textdatum::top_left);
                txt_bg(g, show, nx + 18, y, bot_color(b), PAL_BLACK, lgfx::textdatum::top_left);
            } else {
                txt_bg(g, "gibson", 10, y, ac, PAL_BLACK, lgfx::textdatum::top_left);
                txt_bg(g, " - ", 52, y, PAL_IVORY, PAL_BLACK, lgfx::textdatum::top_left);
                txt_bg(g, show, 76, y, PAL_IVORY, PAL_BLACK, lgfx::textdatum::top_left);
            }
            continue;
        }
        if (ln && strncmp(ln, "gibson>", 7) == 0) {
            txt_bg(g, "gibson", 10, y, ac, PAL_BLACK, lgfx::textdatum::top_left);
            txt_bg(g, ln + 6, 52, y, COL_WHITE, PAL_BLACK, lgfx::textdatum::top_left);
            continue;
        }
        txt_bg(g, ln, 10, y, COL_INK, PAL_BLACK, lgfx::textdatum::top_left);
    }
    const int py = 216;
    txt(g, "gibson", 10, py, ac, lgfx::textdatum::top_left);
    txt(g, ">", 10 + 7 * 6, py, COL_INK, lgfx::textdatum::top_left);
    txt(g, s_gp_buf, 10 + 9 * 6, py, COL_WHITE, lgfx::textdatum::top_left);
    const int cx = 10 + (9 + (int)strlen(s_gp_buf)) * 6;
    if ((t / 530u) % 2u == 0u) g.fillRect(cx, py, 5, 8, ac);
    if (s_sheet) paint_gc_sheet(g);
    s_px = keep;
}

static void paint_intro(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    g.fillRect(4, 4, 312, 232, PAL_BLACK);      /* v1016k: 4 px rain gutter outside the ID board */
    g.drawRect(4, 4, 312, 232, PAL_LINE);
    const int keep = s_px;
    s_px = 6;
    txt(g, "BOTcorps", 14, 6, PAL_MUTED, lgfx::textdatum::top_left);
    txt(g, "S3", 150, 6, PAL_LINE, lgfx::textdatum::top_left);
    txt(g, "REV A", 250, 6, PAL_LINE, lgfx::textdatum::top_left);
    g.drawRect(10, 22, 116, 78, ac);
    g.drawRect(12, 24, 112, 74, PAL_WIRE_C);
    for (int d = 0; d < 110; d += 6) {
        g.fillRect(14 + d, 20, 3, 1, ac);
        g.fillRect(14 + d, 100, 3, 1, ac);
    }
    chip_block_ex(g, 68, 58, 46, 12, true);
    txt(g, "GIBSON", 48, 86, ac, lgfx::textdatum::top_left);
    g.fillRect(14, 104, 92, 18, PAL_BG);
    g.drawRect(14, 104, 92, 18, PAL_WIRE_C);
    txt(g, "U14 FLASH", 22, 108, PAL_MUTED, lgfx::textdatum::top_left);
    for (int i = 0; i < 12; ++i) {
        const int col = i % 4;
        const int row = i / 4;
        const int x = 140 + col * 44;
        const int y = 22 + row * 30;
        g.fillRect(x, y, 34, 22, PAL_BG);
        g.drawRect(x, y, 34, 22, bot_color(i));
        draw_blob(g, x + 17, y + 10, 1, bot_color(i), 0, false);
    }
    g.fillRect(304, 108, 10, 14, PAL_STEEL);
    g.fillRect(306, 111, 8, 8, PAL_BLACK);
    txt(g, "USB", 286, 112, PAL_MUTED, lgfx::textdatum::top_left);
    g.drawRect(8, 132, 304, 96, ac);
    g.fillRect(10, 134, 300, 92, PAL_BLACK);
    g.fillRect(12, 148, 2, 60, ac);
    g.fillRect(298, 148, 2, 60, ac);
    chip_block_ex(g, 36, 176, 30, 12, true);
    txt(g, "BOTS", 70, 150, PAL_MUTED, lgfx::textdatum::top_left);
    txt(g, "7/12", 200, 150, ac, lgfx::textdatum::top_left);
    txt(g, "UPTIME", 70, 166, PAL_MUTED, lgfx::textdatum::top_left);
    txt(g, "LINK", 70, 182, PAL_MUTED, lgfx::textdatum::top_left);
    for (int k = 0; k < 16; ++k) {
        g.fillRect(120 + k * 8, 160, 6, 4, k < 10 ? ac : (uint8_t)PAL_LINE);
        g.fillRect(120 + k * 8, 176, 6, 4, k < 12 ? ac : (uint8_t)PAL_LINE);
        g.fillRect(120 + k * 8, 192, 6, 4, k < 13 ? ac : (uint8_t)PAL_LINE);
    }
    g.fillRect(70, 204, 90, 10, PAL_BG);
    txt(g, "BOTcorps", 78, 205, ac, lgfx::textdatum::top_left);
    txt(g, "LV 01", 160, 214, PAL_MUTED, lgfx::textdatum::top_left);
    for (int k = 0; k < 28; ++k) g.fillRect(16 + k * 4, 210, k % 3 == 0 ? 1 : 2, 8, PAL_WHITE);
    s_px = keep;
}

static void paint_ext(Gfx g)
{
#ifdef BOTCORPS_HOST_RENDER
    host_gate_reset();
    layout_covers();   /* device paint_ext_full does this first; host must too or rain lands under covers */
#endif
    g.clearClipRect();
    g.setTextWrap(false, false);
    const bool hub = s_screen == SCR_GIBSON;
    botcorps_rain_set_hub(hub);
    /* v1016k (SPEC R1): rain behind EVERY screen. Live (stepped) where it always was;
     * panel screens (terminal, scan box, game, diag, intro) get the same rain painted
     * as a static layer on each full paint, framed by their opaque panels. */
    const bool rain_bg = wants_rain() || rain_static();
    if (hub) g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
    else g.fillScreen(COL_FIELD);
#ifdef BOTCORPS_HOST_RENDER
    if (rain_bg) GATE_BOX(g, "RAIN", "layer", 0, 0, 320, 240, "-");
    if (rain_bg && g.raw() && !host_gate_norain) {
#else
    if (rain_bg && g.raw()) {
#endif
        const int y0 = botcorps_band_oy();
#ifdef BOTCORPS_HOST_RENDER
        host_gate_mute = 1;
#endif
        botcorps_rain_draw(*g.raw(), y0, y0 + g.raw()->height());
#ifdef BOTCORPS_HOST_RENDER
        host_gate_mute = 0;
#endif
    }
    if (s_screen == SCR_RAIN && s_act == 4) {
        paint_intro(g);
        return;
    }
    if (s_screen == SCR_DIAG) {
        paint_diag(g);
        return;
    }
    switch (s_screen) {
    case SCR_RAIN: paint_rain_btn(g); break;
    case SCR_GIBSON: paint_gibson(g); break;
    case SCR_GCHAT: paint_gchat(g); break;
    case SCR_TERMINAL: paint_terminal(g); break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: paint_radar(g, true); break;
    case SCR_SCAN: paint_radar(g, false); break;
    case SCR_WIFI_LIST: paint_list(g, true); break;
    case SCR_BT_LIST: paint_list(g, false); break;
    case SCR_CONTROL: paint_control(g); break;
    default: break;
    }
    if (s_fa_on) paint_found_alert(g);
}

static void tick_gibson(Gfx g)
{
    /* The hub Gibson is painted with the rings. A second sprite here covered him. */
    (void)g;
}

static void tick_ext(Gfx g)
{
    if (s_screen == SCR_GIBSON) tick_gibson(g);
    else if (sbox_on() && botcorps_band_direct()) {
        const bool wifi = list_band() == 0;
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
        /* Caret only. A full-line fill here was the grey box on Enter. */
        const int phase = (int)((millis() / 530u) % 2u);
        if (phase != s_prompt_phase) {
            s_prompt_phase = phase;
            /* v1017a: same painter as the full paint, clipped to the line (the old black erase
             * blanked the first ghost character on the off phase). */
            g.startWrite();
            g.setClipRect(PL_X, PL_Y, PL_W, PL_H);
            draw_prompt_line_part(g);
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

/* v1017a control-panel focus map over the EXISTING layout (no re-layout):
 *   top bar:  crown(9) x160   TERM(8) x196
 *   chips:    U1 0 x38  U2 1 x82  U3 2 x126  U4 5 x170 (SCREEN SAVER)  U5 7 x214
 *   lower:    CP qfn 4 x196,150   J1 jumper 3 x268,150   PWR LED 6 x266,194 (SAVER on/off)
 * Left/Right walk one ring through every stop in reading order, so every control is reachable from
 * anywhere; Up/Down go to the visually nearest stop above/below (wrapping top<->bottom). */
static const int8_t CP_RING[CP_N + 2] = { CP_N + 1, CP_N, 0, 1, 2, 5, 7, 4, 3, 6 };
static const int8_t CP_UP[CP_N + 2]   = { CP_N + 1, CP_N + 1, CP_N + 1, 7, 5, CP_N, 3, CP_N, 6, 6 };
static const int8_t CP_DN[CP_N + 2]   = { 4, 4, 4, 6, 6, 4, CP_N + 1, 3, 5, 2 };
static int cp_nav(int cur, int key)
{
    if (cur < 0 || cur >= CP_N + 2) return 0;
    if (key == 1) return CP_UP[cur];
    if (key == 2) return CP_DN[cur];
    int at = 0;
    for (int i = 0; i < CP_N + 2; ++i) if (CP_RING[i] == cur) at = i;
    if (key == 3) return CP_RING[(at + CP_N + 1) % (CP_N + 2)];
    if (key == 4) return CP_RING[(at + 1) % (CP_N + 2)];
    return cur;
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
        enter_screen(SCR_GIBSON, -1); /* mock: rain + Enter = enterHub() */
        break;
    case SCR_GIBSON:
        if (s_hub_zone == 1) break;      /* prompt: Enter runs a typed line (ui_dispatch) */
        if (s_hub_zone == 2) {           /* command menu: Enter steps to the next command */
            const int n = (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0]));
            s_hub_cmd = (s_hub_cmd + 1) % n;
            snprintf(s_gp_buf, sizeof(s_gp_buf), "%s", GP_CMD[s_hub_cmd]);
            partial_gp_strip();
            break;
        }
        if (s_hub_zone == 3) {           /* Gibson: Enter enters the Gibson game */
            enter_screen(SCR_GCHAT, SCR_GIBSON);
            s_dirty = true;
            break;
        }
        if (s_cursor == 0) {
            enter_screen(SCR_TERMINAL, SCR_GIBSON);
            boot_log();
        } else if (s_cursor == 1) {
            s_scan_bot = 12;
            enter_screen(SCR_SCAN, SCR_GIBSON);
        } else {
            enter_screen(SCR_CONTROL, SCR_GIBSON);
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
        if (s_cursor >= 0 && s_cursor < CP_N) {
            /* v1017a: Enter cycles the focused setting forward (rain mode / colour still save NVS). */
            nudge_row(1);
            if (s_cursor == 0) s_int_dirty = true;   /* UI colour recolours the INT name bar too */
            s_dirty = true;
        } else if (s_cursor == CP_N) {
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
    *w = BS;
    *h = BS;
    switch (s_screen) {
    case SCR_GIBSON:
        if (index < 0 || index > 2) return false;
        *x = xs_g[index];
        *y = 206;
        return true;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_SCAN:
        return false; /* scan box: no side buttons */
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
        if (index >= 0 && index < CP_N) return cp_part_rect(index, x, y, w, h);
        if (index == CP_N || index == CP_N + 1) {
            *x = (index == CP_N) ? 284 : 250;
            *y = 8;
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
        /* Chip buttons live in paint_gibson; force a full EXT refresh for focus. */
        s_dirty = true;
        return;
    } else if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR) {
        const int ids[3] = { ICO_TRACK, ICO_LIST, ICO_BACK };
        id = ids[index];
    } else if (s_screen == SCR_SCAN) {
        id = index == 0 ? ICO_TRACK : ICO_BACK;
    } else if (s_screen == SCR_CONTROL) {
        id = index == CP_N ? ICO_TERM : ICO_BACK;
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
    paint_list(g, list_band() == 0);
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

/* Scan box dirty rects: each moving part is patched only when its own key changes. */
static void sb_patch(int x, int y, int w, int h)
{
    s_pc_x = x; s_pc_y = y; s_pc_w = w; s_pc_h = h;
    patch_world(draw_scan_part, x, y, w, h);
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
}

static void sb_tick(void)
{
    static uint32_t k_top = ~0u, k_rule = ~0u, k_srule = ~0u, k_line = ~0u, k_prompt = ~0u;
    static uint32_t k_grid = ~0u, k_fresh = ~0u, k_scout = ~0u, k_chip = ~0u, k_box = ~0u, k_mod = ~0u, k_led = ~0u;
    static int s_seen = -1;
    const int scr = (int)s_screen * 64 + (s_hunt_dev >= 0) * 32 + (s_info_dev >= 0) * 16 + (s_gloss >= 0) * 8 + (s_act >= 0) * 4;
    if (scr != s_seen) { /* entered fresh: everything was just presented in full */
        s_seen = scr;
        k_top = k_rule = k_srule = k_line = k_prompt = k_grid = k_fresh = k_scout = k_chip = k_box = k_mod = k_led = ~0u;
    }
    const int band = list_band();
    const uint32_t t = millis(), e = (t - s_bus_t0) % 3400u;
    const float u = (float)e / 3400.f;
    int outn = 0;
    for (int i = 0; i < 12; ++i) outn += (u > (float)i * .02f && u < .7f) ? 1 : 0;
    const uint32_t top = (uint32_t)outn * 4u + (t / 400u) % 3u;
    if (top != k_top) { if (k_top != ~0u) sb_patch(0, 0, 320, 24); k_top = top; }
    const uint32_t rule = (uint32_t)(320.f * u) / 8u;
    if (rule != k_rule) { if (k_rule != ~0u) sb_patch(0, 24, 320, 1); k_rule = rule; }
    const uint32_t sr = (uint32_t)(176.f * u) / 4u;
    if (sr != k_srule) { if (k_srule != ~0u) sb_patch(106, 144, 176, 1); k_srule = sr; }
    const int n = sb_shown();
    if ((uint32_t)n != k_line) { if (k_line != ~0u) sb_patch(106, 163, 176, 11); k_line = (uint32_t)n; }
    const uint32_t pr = (e / 90u > 9u ? 9u : e / 90u) * 2u + ((t / 530u) % 2u);
    if (pr != k_prompt) { if (k_prompt != ~0u) sb_patch(106, 175, 176, 13); k_prompt = pr; }
    /* module: trace pulses and the blinking LED */
    const uint32_t md = t / 120u;
    if (md != k_mod) { if (k_mod != ~0u) sb_patch(88, 97, 9, 38); k_mod = md; }
    const uint32_t ld = (t / 500u) % 2u;
    if (ld != k_led) { if (k_led != ~0u) sb_patch(93, 54, 5, 3); k_led = ld; }
    if (s_hunt_dev >= 0) {
        hunt_sample();
        const int r = s_hunt_len ? (int)s_hunt_buf[s_hunt_len - 1] : 0;
        const uint32_t hk = s_hunt_tS * 2u + ((t / 250u) % 2u) + (uint32_t)(r & 0xff);
        if (hk != k_box) { if (k_box != ~0u) sb_patch(108, 44, 172, 98); k_box = hk; }
        return;
    }
    if (s_info_dev >= 0) { sb_patch(113, 58, 60, 80); return; }   /* moving scan line */
    const uint32_t ck = (t / 530u) % 2u;
    if (ck != k_chip) { if (k_chip != ~0u) sb_patch(114, 49, 8, 2); k_chip = ck; }
    if (s_gloss >= 0) {
        if (ck != k_box) { if (k_box != ~0u) sb_patch(170, 56, 6, 8); k_box = ck; }
        return;
    }
    if (s_act >= 0) return;                                       /* menu panel is static */
    if ((uint32_t)n != k_grid) {
        if (k_grid != ~0u) sb_patch(106, 52, 176, 92);
        k_grid = (uint32_t)n;
        k_fresh = k_scout = ~0u;
        return;
    }
    int nx = 0, ny = 0;
    sb_cell(band, n - 1, &nx, &ny);
    const uint32_t fk = (t / 110u) % 2u;
    if (fk != k_fresh) { if (k_fresh != ~0u) sb_patch(nx - 12, ny - 12, 25, 25); k_fresh = fk; }
    float ph = (float)(t % 900u) / 900.f * 1.4f;
    if (ph > 1.f) ph = 1.f;
    const uint32_t sk = (uint32_t)(ph * 24.f);
    if (sk != k_scout) {
        if (k_scout != ~0u) {
            const int sx = SG_X + 11, sy = SG_Y + 3 * SG_C + 11;
            const int x0 = sx < nx ? sx : nx, x1 = sx < nx ? nx : sx;
            sb_patch(x0 - 6, sy - 6, x1 - x0 + 13, 13);
            if (ny < sy - 6) sb_patch(nx - 6, ny - 6, 13, sy - ny);
        }
        k_scout = sk;
    }
}

static int cursor_icon(int index)
{
    if (s_screen == SCR_GIBSON) {
        const int ids[3] = { ICO_TERM, ICO_RADAR, ICO_CTRL };
        return ids[index];
    }
    if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR) {
        const int ids[3] = { ICO_TRACK, ICO_LIST, ICO_BACK };
        return ids[index];
    }
    if (s_screen == SCR_SCAN) return index == 0 ? ICO_TRACK : ICO_BACK;
    if (s_screen == SCR_CONTROL) return index == CP_N ? ICO_TERM : ICO_BACK;
    if (s_screen == SCR_WIFI_LIST || s_screen == SCR_BT_LIST) return ICO_BACK;
    if (s_screen == SCR_DIAG) return ICO_BACK;
    if (s_screen == SCR_RAIN) return ICO_PLAY;
    return ICO_BACK;
}

static void partial_box(int index, bool sel)
{
    if (sbox_on()) {
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
    paint_one_board(g, s_part_i, (s_part_i == s_term) && !s_focus_cmds && !s_cmd_btn, false);
}

static void partial_tile(int index)
{
    if (s_screen != SCR_TERMINAL) return;
    int x, y;
    term_xy(index, &x, &y);
    s_part_i = index;
    patch_world(draw_tile_part, x - 2, y - 2, 76, 50);
}

static void draw_term_lower(Gfx g)
{
    paint_boards(g);
    paint_prompt(g);
}

/* v1017a terminal fix: the prompt-line partial used to call paint_prompt_line, a second layout of the
 * same line (prompt at x=10 on a PAL_DEEP box, typed text at x=118, no glow, 300 px wide so it also
 * cut through the companion box), while the full paint draws it via paint_prompt (glow prompt, typed
 * text at x=106 on the black plate). Every anim tick swapped one layout for the other: the typed line
 * jumped 12 px and changed background while typing. The partial now runs the full painter's own code
 * clipped to the line, so a partial is pixel-identical to a full paint at the same millis. */
static void draw_prompt_line_part(Gfx g)
{
    paint_prompt(g);
    if (s_sheet) paint_sheet(g);
}

static void partial_prompt_line(void)
{
    if (s_screen != SCR_TERMINAL) return;   /* v1017a: never paint the terminal prompt onto another screen */
    patch_world(draw_prompt_line_part, PL_X, PL_Y, PL_W, PL_H);
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
    if (s_screen == SCR_GCHAT) {
        if (!s_sheet) return;
        if (key == 1 || key == 3) s_sheet_i = (s_sheet_i + GC_N - 1) % GC_N;
        if (key == 2 || key == 4) s_sheet_i = (s_sheet_i + 1) % GC_N;
        if (s_sheet_i < 0) s_sheet_i = 0;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GIBSON) {
        /* v1017b (Dirt 8:27): Left/Right only cycle the icon row; Up/Down walk
         * icons -> prompt -> command menu -> Gibson (Down), reverse with Up. */
        if (key == 3 || key == 4) {
            const int n = button_count();
            if (s_hub_zone == 0 && n > 0) s_cursor = (s_cursor + (key == 4 ? 1 : n - 1)) % n;
            s_hub_zone = 0;
        } else {
            s_hub_zone = (s_hub_zone + (key == 2 ? 1 : 3)) % 4;
        }
        s_gp_focus = s_hub_zone == 1;
        s_dirty = true;   /* full present (band diff-push), like the control panel */
        return;
    }
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet && !botcorps_t0_term_prompt[0]) {
            if (key == 1 || key == 3) s_sheet_i = (s_sheet_i + CMD_N - 1) % CMD_N;
            if (key == 2 || key == 4) s_sheet_i = (s_sheet_i + 1) % CMD_N;
            s_dirty = true;
            return;
        }
        if (s_term_crown) {
            if (key == 2) {
                s_term_crown = false;
                partial_prompt_panel();
                partial_tile(s_term);
            }
            return;
        }
        if (botcorps_t0_term_prompt[0] && !s_key_fn) {
            /* Prompt owns the plain keys (history). Fn+arrows always leave the prompt. */
            if (key == 1 || key == 2) {
                if (s_hist_n <= 0) return;
                if (s_hist_i < 0) s_hist_i = s_hist_n;
                s_hist_i += (key == 1) ? -1 : 1;
                if (s_hist_i < 0) s_hist_i = 0;
                if (s_hist_i >= s_hist_n) s_hist_i = s_hist_n - 1;
                snprintf(botcorps_t0_term_prompt, 80, "%s", botcorps_t0_term_hist[s_hist_i]);
                partial_term_lower();
            }
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
        if (s_cmd_btn) {
            /* v1017a: on the commands button only Up leaves (back to the board above it). */
            if (key == 1) {
                s_cmd_btn = false;
                partial_tile(s_term);
                partial_term_lower();
            }
            return;
        }
        const int old = s_term;
        int c = s_term % 4;
        int r = s_term / 4;
        if (key == 2 && r == 2) {
            /* v1017a: Down from the bottom board row stops on the commands button. */
            s_cmd_btn = true;
            partial_tile(old);
            partial_term_lower();
            return;
        }
        if (key == 1 && r == 0) {
            /* top row + Up reaches the crown, the Terminal's only back button */
            s_term_crown = true;
            partial_tile(old);
            partial_prompt_panel();
            return;
        }
        if (key == 3) c = (c + 3) % 4;
        if (key == 4) c = (c + 1) % 4;
        if (key == 1) r = r - 1;
        if (key == 2) r = r < 2 ? r + 1 : 2;
        s_term = r * 4 + c;
        if (s_term != old) {
            partial_tile(old);
            partial_tile(s_term);
            partial_int();
        }
        return;
    }
    if (s_screen == SCR_CONTROL) {
        /* v1017a: arrows only MOVE focus; Enter changes the focused setting (activate). */
        const int n = cp_nav(s_cursor, key);
        if (n != s_cursor) {
            s_cursor = n;
            /* Focus shows in the top dots, the part, and its accent feed trace, so a focus move is a
             * full present (band diff-push: only changed rows reach the panel, no clear). The old
             * partial_box path drew the crown / TERM highlight at stale coordinates (250,8 / 284,8)
             * and left the dots + feed on the old part. */
            s_dirty = true;
        }
        return;
    }
    if (s_fa_on) {
        if (key == 1 || key == 3) s_fa_sel = (s_fa_sel + FA_N - 1) % FA_N;
        if (key == 2 || key == 4) s_fa_sel = (s_fa_sel + 1) % FA_N;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_SCAN) return;
    if (sbox_on()) {
        /* Everything stays in the scan box. Left / Right never switch screens here. */
        if (s_hunt_dev >= 0 || s_info_dev >= 0) return;
        if (s_gloss >= 0) {
            if (key == 1) s_gloss = (s_gloss + GL_N - 1) % GL_N;
            if (key == 2) s_gloss = (s_gloss + 1) % GL_N;
            if (key == 3 || key == 4) s_gloss = s_gloss < 7 ? (s_gloss + 7 < GL_N ? s_gloss + 7 : GL_N - 1) : s_gloss - 7;
            partial_box(s_cursor, true);
            return;
        }
        if (s_act >= 0) {
            if (key == 1 || key == 3) s_act = (s_act + ACT_N - 1) % ACT_N;
            if (key == 2 || key == 4) s_act = (s_act + 1) % ACT_N;
            partial_box(s_cursor, true);
            return;
        }
        const int band = list_band();
        const int dir = (key == 1 || key == 3) ? -1 : (key == 2 || key == 4) ? 1 : 0;
        if (dir == 0) return;
        /* positions: 0..LIST_N-1 devices, LIST_N = the > GLOSSARY prompt */
        int i = s_cursor;
        for (int k = 0; k <= LIST_N; ++k) {
            i = (i + dir + LIST_N + 1) % (LIST_N + 1);
            if (i == LIST_N || !s_dev_ign[band][i]) break;
        }
        s_cursor = i;
        partial_box(s_cursor, true);
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
    if (dev < 0 || dev >= LIST_N) return;
    if (which == 0) {
        s_hunt_dev = dev;
        s_hunt_band = band;
        s_hunt_len = 0;
        s_hunt_tS = 0;
        s_hunt_force = 0;
    } else if (which == 1) {
        s_info_dev = dev;
    } else if (which == 2) {
        s_dev_tag[band][dev] = s_dev_tag[band][dev] >= 4 ? 0 : (uint8_t)(s_dev_tag[band][dev] + 1);
    } else if (which == 3) {
        s_dev_ign[band][dev] = 1;
        if (s_cursor == dev) {
            s_cursor = LIST_N;
            for (int i = 0; i < LIST_N; ++i) {
                if (!s_dev_ign[band][i]) {
                    s_cursor = i;
                    break;
                }
            }
        }
    }
}

static void fa_run(int which);

static void on_ok(void)
{
    if (s_fa_on) {
        fa_run(s_fa_sel);
        return;
    }
    if (s_screen == SCR_SCAN) return;
    if (sbox_on()) {
        if (s_hunt_dev >= 0 || s_gloss >= 0) return; /* Esc steps back */
        if (s_info_dev >= 0) s_info_dev = -1;
        else if (s_act >= 0) act_pick();
        else if (s_cursor == LIST_N) s_gloss = 0;
        else if (s_cursor >= 0 && s_cursor < LIST_N && !s_dev_ign[list_band()][s_cursor]) {
            s_act = 0;
            s_act_dev = s_cursor;
        }
        partial_box(s_cursor, true);
        return;
    }
    if (s_screen == SCR_TERMINAL && s_term_crown && !s_sheet) {
        s_term_crown = false;
        go_back();
        return;
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
            lut_for_term();
            return;
        }
        if (botcorps_t0_term_prompt[0]) {
            run_cmd(botcorps_t0_term_prompt);
            botcorps_t0_term_prompt[0] = 0;
            return;
        }
        if (s_cmd_btn) {
            /* v1017a: Enter on the button opens the commands menu (chip palette). */
            s_cmd_btn = false;
            s_focus_cmds = true;
            s_dirty = true;
            lut_for_term();
            return;
        }
        if (s_focus_cmds) {
            if (strcmp(CMDS[s_cmd_sel], "cmds") == 0) {
                s_sheet = true;
                s_sheet_i = 0;
                s_dirty = true;
                /* Same gate as the full present, so this path installs the sheet lut. */
                lut_for_term();
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

static void fa_run(int which)
{
    const int band = s_fa_band;
    const int dev = s_fa_dev;
    s_fa_on = false;
    if (which == 0) { /* hunt */
        enter_screen(band == 0 ? SCR_WIFI_RADAR : SCR_BT_RADAR, -1);
        s_hunt_dev = dev;
        s_hunt_band = band;
        s_hunt_len = 0;
        s_hunt_tS = 0;
        s_hunt_force = 0;
        s_cursor = dev;
        s_dirty = true;
    } else if (which == 1) { /* info */
        enter_screen(band == 0 ? SCR_WIFI_RADAR : SCR_BT_RADAR, -1);
        s_info_dev = (dev < LIST_N) ? dev : 0;
        s_cursor = s_info_dev;
        s_dirty = true;
    } else if (which == 2) { /* ignore */
        if (dev >= 0 && dev < LIST_N) s_dev_ign[band][dev] = 1;
        s_dirty = true;
    }
    /* back / ignore: dismiss only */
}

static void on_esc(void)
{
    if (s_fa_on) {
        fa_run(3);
        return;
    }
    if (sbox_on()) {
        bool stepped = true;
        if (s_hunt_dev >= 0) {
            s_hunt_dev = -1;
            s_hunt_len = 0;
        } else if (s_gloss >= 0) s_gloss = -1;
        else if (s_info_dev >= 0) s_info_dev = -1;
        else if (s_act >= 0) s_act = -1;
        else stepped = false;
        if (stepped) {
            partial_box(s_cursor, true);
            return;
        }
    }
    if (s_screen == SCR_GCHAT) {
        if (s_sheet) {
            s_sheet = false;
            s_dirty = true;
            return;
        }
        if (s_gp_buf[0]) {
            s_gp_buf[0] = 0;
            s_dirty = true;
            return;
        }
        enter_screen(SCR_GIBSON, -1);
        return;
    }
    if (s_screen == SCR_TERMINAL && (s_sheet || s_focus_cmds)) {
        const bool from_menu = s_focus_cmds && !s_sheet;
        s_sheet = false;
        s_focus_cmds = false;
        s_cmd_btn = from_menu;   /* v1017a: Esc closes the commands menu back onto its button */
        s_dirty = true;
        lut_for_term();
        return;
    }
    if (s_screen == SCR_TERMINAL && botcorps_t0_term_prompt[0]) {
        /* mock: Esc clears the buffer first, then goes back */
        botcorps_t0_term_prompt[0] = 0;
        partial_term_lower();
        return;
    }
    s_term_crown = false;
    go_back();
}

static void on_tab(void)
{
    if (s_screen == SCR_GIBSON) {
        enter_screen(SCR_GCHAT, SCR_GIBSON);
        s_sheet = true;
        s_sheet_i = 0;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GCHAT) {
        s_sheet = !s_sheet;
        if (s_sheet) s_sheet_i = 0;
        s_dirty = true;
        return;
    }
    if (s_screen != SCR_TERMINAL || s_sheet) return;
    if (!botcorps_t0_term_prompt[0]) {
        s_sheet = !s_sheet;
        s_focus_cmds = false; s_cmd_btn = false;
        if (s_sheet) s_sheet_i = 0;
        s_tab_i = -1;
        s_dirty = true;
        lut_for_term();
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
    if (s_screen == SCR_GCHAT) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        const size_t n = strlen(s_gp_buf);
        if (n + 1 < sizeof(s_gp_buf) && c >= 32 && c < 127) {
            s_gp_buf[n] = c;
            s_gp_buf[n + 1] = 0;
        }
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GIBSON) {
        s_gp_focus = true;
        if (s_hub_zone != 1) { s_hub_zone = 1; s_dirty = true; }
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
    if (s_term_crown) {
        s_term_crown = false;
        partial_prompt_panel();
    }
    if (s_cmd_btn) {
        s_cmd_btn = false;       /* typing returns focus to the prompt / boards */
        partial_tile(s_term);
    }
    botcorps_t0_term_prompt[n] = c;
    botcorps_t0_term_prompt[n + 1] = 0;
    s_tab_i = -1;
    partial_term_lower();
}

static void on_bs(void)
{
    if (s_screen == SCR_GCHAT && s_gp_buf[0]) {
        const size_t n = strlen(s_gp_buf);
        s_gp_buf[n - 1] = 0;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GIBSON && s_gp_buf[0]) {
        s_gp_focus = true;
        if (s_hub_zone != 1) { s_hub_zone = 1; s_dirty = true; }
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
    s_key_fn = false;
    if (st.enter) return K_OK;
    if (st.fn) {
        /* Fn chords are never typing: Fn+; . , / are arrows and Fn+` is Esc on every screen. */
        for (char c : st.word) {
            s_key_fn = true;
            if (c == ';' || c == ':') return K_UP;
            if (c == '.' || c == '>') return K_DN;
            if (c == ',' || c == '<') return K_LT;
            if (c == '/' || c == '?') return K_RT;
            if (c == '`' || c == '~') return K_ESC;
            s_key_fn = false;
        }
    }
    for (uint8_t h : st.hid_keys) {
        if (h == 0x2A) return K_BS;
        if (h == 0x2B) return K_TAB;
        if (h == 0x29) return K_ESC;
        if (h == 0x52) return K_UP;
        if (h == 0x51) return K_DN;
        if (h == 0x50) return K_LT;
        if (h == 0x4F) return K_RT;
    }
    const bool prompt_owns =
        (s_screen == SCR_TERMINAL && botcorps_t0_term_prompt[0]) ||
        s_screen == SCR_GCHAT ||
        (s_screen == SCR_GIBSON && s_gp_buf[0]);
    for (char c : st.word) {
        if (!prompt_owns) {
            if (c == ';' || c == ':') return K_UP;
            if (c == '.' || c == '>') return K_DN;
            if (c == ',' || c == '<') return K_LT;
            if (c == '/' || c == '?') return K_RT;
        }
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

/* v1017a flicker fix: the band calls this to rebuild a whole 120-row slice when a patch lands in the
 * slice that is not resident in the sprite. It used to run with sb_patch's s_pc_* still set to the
 * small patch rect, so paint_sbox culled the matte board + chrome for the whole slice and push_slice
 * sent field-only rows to the panel (half-screen flash until the next full present). A recompose is a
 * full paint: widen s_pc_* to the screen for its duration, then restore the caller's patch rect. */
static void recompose_ext(Gfx g)
{
    const int px = s_pc_x, py = s_pc_y, pw = s_pc_w, ph = s_pc_h;
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
    paint_ext(g);
    tick_ext(g);
    s_pc_x = px; s_pc_y = py; s_pc_w = pw; s_pc_h = ph;
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
    if (strcmp(name, "bots") == 0 || strcmp(name, "bot") == 0) return SCR_TERMINAL;
    if (strcmp(name, "wifi") == 0) return SCR_WIFI_RADAR;
    if (strcmp(name, "ble") == 0 || strcmp(name, "bluetooth") == 0) return SCR_BT_RADAR;
    if (strcmp(name, "chat") == 0 || strcmp(name, "gchat") == 0) return SCR_GCHAT;
    if (strcmp(name, "scan") == 0 || strcmp(name, "radar") == 0) { s_scan_bot = 12; return SCR_SCAN; }
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
    if (strcmp(cmd, "chat") == 0) {
        gp_open(SCR_GCHAT);
        return;
    }
    if (strcmp(cmd, "defend") == 0) {
        /* GAP: the mock's defend typing game is not in this firmware; open its home, the Terminal. */
        gp_open(SCR_TERMINAL);
        log_add("defend: game not in this build");
        return;
    }
    if (strcmp(cmd, "scan") == 0 && (!arg || !arg[0] || strcmp(arg, "radar") == 0)) {
        s_scan_bot = 12;
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

static void ui_dispatch(int key, char typed);
void botcorps_ui_input(void)
{
    char typed = 0;
    const int key = read_key(&typed);
    if (key == K_NONE) return;
    ui_dispatch(key, typed);
}

static void ui_dispatch(int key, char typed)
{
    s_last_key = millis();
    if (s_screen == SCR_GIBSON && (key == K_CHAR || key == K_OK)) {
        s_mood = MOOD_HAPPY;
        s_mood_until = s_last_key + 1500u;
        s_bub[0] = (key == K_CHAR && typed) ? typed : '*';
        s_bub[1] = 0;
    }
    if (s_screen == SCR_GCHAT && key == K_OK) {
        if (s_sheet) {
            gc_pick();
            s_dirty = true;
            return;
        }
        if (s_gp_buf[0]) {
            char line[28];
            snprintf(line, sizeof(line), "%s", s_gp_buf);
            s_gp_buf[0] = 0;
            gc_run(line);
            s_dirty = true;
            return;
        }
        s_sheet = true;
        s_sheet_i = 0;
        s_dirty = true;
        return;
    }
    if (key == K_OK && s_screen == SCR_GIBSON && s_gp_buf[0] && s_hub_zone <= 1) {
        s_gp_focus = true;
        s_hub_zone = 1;
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
            lut_for_term();
            botcorps_band_present([](Gfx g) {
                paint_ext(g);
                tick_ext(g);
            });
            botcorps_band_set_oy(0);
        } else if (ext_anim && s_screen == SCR_TERMINAL) {
            partial_prompt_line();
        } else if (ext_anim && sbox_on()) {
            /* Hunt meter lives in the box; otherwise only the sweep rule and caret move. */
            sb_tick();
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
        lut_for_term();
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
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_track_name[0] = 0;
    boot_log();
    paint_ext(Gfx());
}

void botcorps_host_idle(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = false;
    botcorps_lut_term(false);
    s_sheet_i = 0;
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_track_name[0] = 0;
    scan_all_set(false);
    s_mood_until = 0;
    boot_log();
    paint_ext(Gfx());
}

void botcorps_host_hunt(uint8_t *buf)
{
    /* Action-list popup on the Wi-Fi list (legacy host name). */
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_WIFI_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_hunt_dev = -1;
    s_cursor = 2;
    s_color = 0;
    s_act = 0;
    s_act_dev = 2;
    s_info_dev = -1;
    s_bus_t0 = 1000;
    paint_ext(Gfx());
}

void botcorps_host_hunt_view(uint8_t *buf, int force_dbm)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_WIFI_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_act = -1;
    s_info_dev = -1;
    s_color = 0;
    s_hunt_band = 0;
    s_hunt_dev = 3; /* tp-link-mesh */
    s_hunt_len = 0;
    s_hunt_tS = 0;
    s_hunt_force = force_dbm;
    host_set_millis(5000);
    for (int k = 0; k < HUNT_N; ++k) {
        s_hunt_tS = 0;
        host_set_millis(5000u + (uint32_t)k * 260u);
        hunt_sample();
    }
    paint_ext(Gfx());
}

void botcorps_host_ble_list(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_BT_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_hunt_dev = -1;
    s_cursor = 0;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_bus_t0 = 1000;
    paint_ext(Gfx());
}

void botcorps_host_found_alert(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_TERMINAL;
    botcorps_lut_term(false);
    s_sheet = false;
    s_color = 0;
    s_fa_on = true;
    s_fa_sel = 0;
    s_fa_band = 0;
    s_fa_dev = 3;
    paint_ext(Gfx());
}

void botcorps_host_scan_tint(uint8_t *buf, int color)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_SCAN;
    s_hunt_band = 0;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_cursor = 0;
    s_color = color;
    s_act = -1;
    s_bus_t0 = millis() - 4000u;
    paint_ext(Gfx());
}

void botcorps_host_list(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_WIFI_LIST;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_hunt_dev = -1;
    s_cursor = 2;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_bus_t0 = 1000;
    paint_ext(Gfx());
}

void botcorps_host_bus(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_SCAN;
    s_hunt_band = 0;
    botcorps_lut_term(false);
    s_sheet = false;
    s_cursor = 0;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_bus_t0 = millis() - 4000u;
    paint_ext(Gfx());
}

void botcorps_host_bus_ble(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_SCAN;
    s_hunt_band = 1;
    botcorps_lut_term(false);
    s_sheet = false;
    s_cursor = 0;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_bus_t0 = millis() - 4000u;
    paint_ext(Gfx());
    s_hunt_band = 0;
}

void botcorps_host_botscan_n(uint8_t *buf, int bot)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_SCAN;          /* v1016n: a bot's own scan screen (bot radar removed) */
    s_scan_bot = bot;
    s_bot = bot;
    s_color = 0;
    s_sheet = false;
    botcorps_lut_term(false);
    s_cursor = 0;
    s_bus_t0 = millis() - 4000u;
    paint_ext(Gfx());
}

void botcorps_host_botscan(uint8_t *buf)
{
    botcorps_host_botscan_n(buf, 2);
}

void botcorps_host_amber(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = true;
    botcorps_lut_term(true);
    s_sheet_i = 2;
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 1;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_track_name[0] = 0;
    boot_log();
    paint_ext(Gfx());
}

int botcorps_host_cmds_sheet(void)
{
    uint8_t *saved = host_frame;
    static uint8_t scratch[320 * 240];
    host_frame = scratch;
    host_term_lut = 0;
    s_screen = SCR_TERMINAL;
    s_focus_cmds = true;
    s_sheet = false;
    s_dirty = false;
    botcorps_t0_term_prompt[0] = 0;
    s_cmd_sel = 11;
    on_ok();
    const int r = (host_term_lut && s_dirty && s_sheet) ? 1 : 0;
    host_frame = saved;
    return r;
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
        SCR_TERMINAL, SCR_GIBSON, SCR_WIFI_RADAR, SCR_BT_RADAR, SCR_SCAN,
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

void botcorps_host_sig(int s)
{
    s_host_sig = s;
}

static void host_sheet_setup(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = true;
    botcorps_lut_term(true);
    s_sheet_i = 0;
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_track_name[0] = 0;
    boot_log();
}

void botcorps_host_sheet(uint8_t *buf, int sel)
{
    host_sheet_setup(buf);
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    s_sheet_i = sel;
    paint_ext(Gfx());
}

/* Arrow the open sheet, then the same paint_ext a clean frame uses. */
void botcorps_host_sheet_key(uint8_t *buf, int key)
{
    host_frame = buf;
    on_arrows(key);
    paint_ext(Gfx());
}

void botcorps_host_gchat(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_GCHAT;
    s_sheet = false;
    botcorps_lut_term(false);
    s_color = 0;
    s_act = -1;
    s_gp_buf[0] = 0;
    gc_set_count(0);
    log_add("gibson> ask drone");
    gc_push(12, "patching drone in");
    gc_push(5, "ready");
    log_add("gibson> scan all");
    gc_push(0, "cafe-guest -42");
    gc_push(1, "earbuds-58 -58");
    s_bus_t0 = 0;
    paint_ext(Gfx());
}

void botcorps_host_intro(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_RAIN;
    s_act = 4;
    s_sheet = false;
    botcorps_lut_term(false);
    paint_ext(Gfx());
    s_act = -1;
}

void botcorps_host_gmenu(uint8_t *buf)
{
    botcorps_host_gchat(buf);
    s_sheet = true;
    s_sheet_i = 8;
    paint_ext(Gfx());
}

void botcorps_host_jackin(uint8_t *buf)
{
    botcorps_host_gchat(buf);
    s_sheet = false;
    gc_set_count(0);
    gc_run("jackin");
    paint_ext(Gfx());
}

void botcorps_host_scanall(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = false;
    botcorps_lut_term(false);
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_bus_t0 = 0;
    run_cmd("scan all");
    lut_for_term();
    paint_ext(Gfx());
}

void botcorps_host_scanall_sheet(uint8_t *buf)
{
    botcorps_host_scanall(buf);
    s_sheet = true;
    s_sheet_i = 0;
    lut_for_term();
    paint_ext(Gfx());
}

/* Log text says crew sweep, but scan-all mode was never set. */
void botcorps_host_stale_sweep(uint8_t *buf)
{
    botcorps_host_idle(buf);
    scan_all_set(false);
    botcorps_lut_term(false);
    s_mood_until = 0;
    log_add("crew sweep - 12 boards");
    paint_ext(Gfx());
}

void botcorps_host_keyfob(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = false;
    botcorps_lut_term(false);
    s_focus_cmds = false; s_cmd_btn = false;
    s_term = 2;
    s_color = 0;
    s_log_n = 0;
    s_log_next = 0;
    botcorps_t0_term_prompt[0] = 0;
    s_bus_t0 = 0;
    run_cmd("scan keyfob");
    paint_ext(Gfx());
}

void botcorps_host_hub(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_FIELD, 320u * 240u);
    s_screen = SCR_GIBSON;
    s_sheet = false;
    botcorps_lut_term(false);
    s_fa_on = false;
    s_color = 0;
    s_cursor = 0;
    s_act = -1;
    s_mood = MOOD_NEUTRAL;
    s_mood_until = 0;
    paint_ext(Gfx());
}
#endif

#ifdef BOTCORPS_HOST_RENDER
/* v1016 scan box host paints. mode: 0 icons, 1 selected+prompt, 2 info, 3 hunt, 4 glossary */
void botcorps_host_sbox(uint8_t *buf, int band, int mode, int force_dbm)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = band ? SCR_BT_RADAR : SCR_WIFI_RADAR;
    botcorps_lut_term(false);
    s_sheet = false;
    s_fa_on = false;
    s_color = 0;
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_gloss = -1;
    s_hunt_dev = -1;
    memset(s_dev_tag, 0, sizeof(s_dev_tag));
    memset(s_dev_ign, 0, sizeof(s_dev_ign));
    s_cursor = 0;
    s_bus_t0 = 1000;
    host_set_millis(mode == 5 ? 1000u + 578u : 10400u);
    if (mode == 1) { s_cursor = 0; s_act = 0; s_act_dev = 0; }
    if (mode == 2) { s_cursor = 0; s_info_dev = 0; }
    if (mode == 4) { s_cursor = LIST_N; s_gloss = 8; }
    if (mode == 3) {
        s_hunt_band = band;
        s_hunt_dev = 0;
        s_hunt_len = 0;
        s_hunt_force = force_dbm;
        for (int k = 0; k < HUNT_N; ++k) {
            s_hunt_tS = 0;
            host_set_millis(5000u + (uint32_t)k * 260u);
            hunt_sample();
        }
    }
    if (mode == 3) host_set_millis(13900u);
    paint_ext(Gfx());
}
void botcorps_host_found_alert_b(uint8_t *buf, int band, int dev)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_TERMINAL;
    botcorps_lut_term(false);
    s_sheet = false;
    s_color = 0;
    s_fa_on = true;
    s_fa_sel = 0;
    s_fa_band = band;
    s_fa_dev = dev;
    paint_ext(Gfx());
}
#endif

#ifdef BOTCORPS_HOST_RENDER
/* v1016f: render a screen through the real device entry (enter_screen + key handlers). */
void botcorps_host_enter(uint8_t *buf, int id)
{
    s_term = 0; botcorps_t0_term_prompt[0] = 0; s_gp_buf[0] = 0;
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    /* s_color kept: harness sets the UI colour before entering */
    s_fa_on = false;
    s_act = -1;
    enter_screen(id, -1);
    paint_ext(Gfx());
}
int botcorps_host_screen_now(void) { return s_screen; }
/* v1017a flicker sim (tools/host band simulator): full reference paint at the current millis */
extern int host_band_sim;
extern long host_sim_presents, host_sim_pushes, host_sim_recomposes, host_sim_bad, host_sim_worst;
extern uint8_t host_sim_panel[320 * 240];
void host_sim_reset(void);
void botcorps_host_ref_paint(uint8_t *buf)
{
    const int px = s_pc_x, py = s_pc_y, pw = s_pc_w, ph = s_pc_h;
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
    s_pc_x = px; s_pc_y = py; s_pc_w = pw; s_pc_h = ph;
}
/* Run the real device draw loop (botcorps_ui_draw) for ms milliseconds in 10 ms steps on the band
 * simulator. keys (optional) are fed one per 300 ms. Returns pushes with a visible glitch. */
int botcorps_host_sim(int ms, const char *keys, long *presents, long *pushes, long *recomp, long *worst)
{
    extern int host_gate_norain;
    host_gate_norain = 1;
    host_band_sim = 1;
    host_sim_reset();
    botcorps_band_set_recompose(recompose_ext);
    memset(host_sim_panel, 0, sizeof host_sim_panel);
    s_dirty = true;
    const uint32_t t0 = millis();
    const char *k = keys;
    for (int i = 0; i * 10 < ms; ++i) {
        host_set_millis(t0 + (uint32_t)i * 10u);
        if (k && *k && i % 30 == 15) {
            const unsigned char c = (unsigned char)*k++;
            if (c >= 1 && c <= 4) { s_key_fn = true; ui_dispatch((int)c, 0); }
            else if (c == '\n') { s_key_fn = false; ui_dispatch(K_OK, 0); }
            else if (c == '\b') { s_key_fn = false; ui_dispatch(K_BS, 0); }
            else { s_key_fn = false; ui_dispatch(K_CHAR, (char)c); }
        }
        botcorps_ui_draw();
    }
    host_band_sim = 0;
    host_gate_norain = 0;
    *presents = host_sim_presents; *pushes = host_sim_pushes; *recomp = host_sim_recomposes; *worst = host_sim_worst;
    return (int)host_sim_bad;
}
/* v1016n gate: terminal Select on board b; returns the screen it opened and the scan bot */
int botcorps_host_open_bot(uint8_t *buf, int b, int *scan_bot)
{
    botcorps_host_enter(buf, SCR_TERMINAL);
    s_term = b;
    open_bot(b);
    *scan_bot = s_scan_bot;
    host_frame = buf;
    paint_ext(Gfx());
    return s_screen;
}
void botcorps_host_seq(uint8_t *buf, const char *keys)
{
    /* Real device path: boot state, then the same dispatcher read_key feeds. */
    for (const char *p = keys; *p; ++p) {
        const unsigned char c = (unsigned char)*p;
        if (c >= 1 && c <= 4) { s_key_fn = true; ui_dispatch((int)c, 0); }
        else if (c == '\n') { s_key_fn = false; ui_dispatch(K_OK, 0); }
        else if (c == 0x1b) { s_key_fn = false; ui_dispatch(K_ESC, 0); }
        else if (c == '\t') { s_key_fn = false; ui_dispatch(K_TAB, 0); }
        else if (c == '\b') { s_key_fn = false; ui_dispatch(K_BS, 0); }
        else { s_key_fn = false; ui_dispatch(K_CHAR, (char)c); }
    }
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* gate.sh: scan board for one bot type (12 = Gibson); settings with a given cursor */
void botcorps_host_scan_n(uint8_t *buf, int bot)
{
    s_scan_bot = bot;
    botcorps_host_enter(buf, SCR_SCAN);
}
void botcorps_host_control(uint8_t *buf, int cursor)
{
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = cursor;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* gate.sh functional checks */
int botcorps_host_esc_test(uint8_t *buf, int id, int *expect)
{
    botcorps_host_enter(buf, id);
    *expect = default_back(s_screen);
    s_key_fn = false;
    ui_dispatch(K_ESC, 0);
    return s_screen;
}
static int s_cp_cycle = 0;
int botcorps_host_setfn(uint8_t *buf, int cursor, char *before, char *after, int *nvs)
{
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = cursor;
    const int sv[7] = { s_color, s_mix, s_shape, botcorps_rain_mode(), s_saver, s_saver_on, s_idle };
    cp_text(cursor, before, 24);
    const int n0 = host_nvs_puts;
    s_key_fn = false;
    ui_dispatch(K_OK, 0);                      /* v1017a: Enter cycles the focused setting */
    cp_text(cursor, after, 24);
    *nvs = host_nvs_puts - n0;
    /* full cycle: presses until the value after the first press comes round again */
    int k = 1;
    char cur[24];
    for (; k < 12; ++k) {
        ui_dispatch(K_OK, 0);
        cp_text(cursor, cur, 24);
        if (!strcmp(cur, after)) break;
    }
    s_cp_cycle = k < 12 ? k : -1;
    const bool ok = strcmp(before, after) != 0 && s_screen == SCR_CONTROL && s_cursor == cursor;
    /* restore the defaults so later checks are not affected */
    s_color = sv[0]; s_mix = sv[1]; s_shape = sv[2]; botcorps_rain_set_mode(sv[3]);
    s_saver = sv[4]; s_saver_on = sv[5]; s_idle = sv[6];
    botcorps_rain_set_mix(s_mix); botcorps_rain_set_shape(s_shape);
    return ok;
}
int botcorps_host_cp_cycle(void) { return s_cp_cycle; }
/* v1017a gate: terminal commands button. From board 0: Down x3 must land on the button (and only
 * there), Enter must open the commands menu, Esc returns to the button, Up leaves to the boards. */
int botcorps_host_cmd_btn(uint8_t *buf, int *reach, int *opened, int *esc_back, int *up_leaves, int *menu_runs)
{
    botcorps_host_enter(buf, SCR_TERMINAL);
    s_term = 1;
    botcorps_host_seq(buf, "\x02\x02\x02");
    *reach = s_cmd_btn && !s_focus_cmds && s_screen == SCR_TERMINAL;
    botcorps_host_seq(buf, "\n");
    *opened = s_focus_cmds && !s_cmd_btn && s_screen == SCR_TERMINAL;
    {   /* the palette is painted (chip 0 plate at 8,171) */
        static uint8_t a[320 * 240];
        memset(a, PAL_BLACK, sizeof a); host_frame = a; paint_ext(Gfx());
        static uint8_t b2[320 * 240];
        s_focus_cmds = false; s_cmd_btn = true;
        memset(b2, PAL_BLACK, sizeof b2); host_frame = b2; paint_ext(Gfx());
        s_focus_cmds = true; s_cmd_btn = false;
        int diff = 0;
        for (int y = 169; y < 207; ++y) for (int x = 5; x < 273; ++x) diff += a[y * 320 + x] != b2[y * 320 + x];
        *opened = *opened && diff > 500;
        host_frame = buf;
    }
    botcorps_host_seq(buf, "\x1b");
    *esc_back = s_cmd_btn && !s_focus_cmds && s_screen == SCR_TERMINAL;
    botcorps_host_seq(buf, "\x01");
    *up_leaves = !s_cmd_btn && !s_focus_cmds && s_term == 9;
    /* menu really runs a command: open, pick chip 'help' (index 2), Enter -> log grows */
    botcorps_host_seq(buf, "\x02\n\x04\x04");
    const int n0 = s_log_next;
    botcorps_host_seq(buf, "\n");
    *menu_runs = s_log_next != n0;
    return s_screen;
}
int botcorps_host_cp_start(void) { uint8_t b[1]; (void)b; static uint8_t buf[320 * 240]; botcorps_host_enter(buf, SCR_CONTROL); return s_cursor; }
/* v1017a gate: arrows only move. From a stop, press dir; returns the new cursor and whether any
 * setting text changed (must be 0). */
int botcorps_host_cp_move(uint8_t *buf, int from, int dir, int *changed)
{
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = from;
    char b[CP_N][24], a[24];
    for (int i = 0; i < CP_N; ++i) cp_text(i, b[i], 24);
    s_key_fn = true;
    ui_dispatch(dir, 0);
    s_key_fn = false;
    *changed = 0;
    for (int i = 0; i < CP_N; ++i) { cp_text(i, a, 24); if (strcmp(a, b[i])) *changed = 1; }
    return s_screen == SCR_CONTROL ? s_cursor : -100 - s_screen;
}
void botcorps_host_set_color(int c){ s_color = (c<0||c>2)?0:c; }
void botcorps_host_key(int key, char c) { s_key_fn = (c == 1); ui_dispatch(key, c == 1 ? 0 : c); }
void botcorps_host_paint(uint8_t *buf) { host_frame = buf; paint_ext(Gfx()); }
#endif
