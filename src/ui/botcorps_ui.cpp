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

/* v1019 (Dirt, LOCKED #3): five control-panel UI themes REPLACE the old CYAN / AMBER / MINT
 * accents. Each theme owns a clearly distinct palette (accent, second accent, selection,
 * wire 0.35, wash 0.22, light ink, purpose text, scan-box tones). Bot colours never come
 * from here: they stay the fixed BOT_PAL table on every screen.
 * Slots are RGB332 natives (or existing reserved exacts), never the 226..248 sheet codes. */
struct ui_theme_t {
    const char *name;     /* full name (INT card / cp_text) */
    const char *sname;    /* <= 6 silk chars on the COLOR chip */
    uint8_t ac;           /* primary accent stroke + text */
    uint8_t ac2;          /* second accent (duotone / gradient end / circuit nodes) */
    uint8_t sel;          /* selection outline (Ultraviolet: pink on selected) */
    uint8_t wire;         /* accent at ~0.35 over the field */
    uint8_t wash;         /* accent at ~0.22 over the field */
    uint8_t ink;          /* light tint of the accent (sheet selected name) */
    uint8_t pur;          /* command purpose text */
    uint8_t tone[5];      /* sb_tone: 0 ac.55/black, 1 texture, 2 grid wash, 3 ac.35, 4 ac.8 */
    uint8_t sbink;        /* mock UI.ink on the scan box */
    uint8_t grad;         /* 0 solid, 1 duotone (split), 2 two-colour fade (ac2 -> ac), 3 three-stop fade
                           * (ac2 -> ac -> ac3), 4 PRISM spectrum texture, 5 GLITCH RGB-split texture */
    uint8_t ac3;          /* v1019n-mod: third fade stop (grad 3); 0 elsewhere */
};
#if defined(BOTCORPS_HOST_RENDER) && defined(BOTCORPS_LEGACY_ACCENT)
/* Host-only regression twin: the pre-v1019 CYAN / AMBER / MINT accents, so the gate can prove
 * the theme refactor is pixel-identical to the locked renders. Never built for the device. */
static const int THEME_N = 3;
static const ui_theme_t THEMES[3] = {
    { "CYAN", "CYAN", PAL_CYAN, PAL_CYAN, PAL_CYAN, PAL_WIRE_C, PAL_WASH_C, PAL_TERM_INK_C, PAL_TERM_C,
      { PAL_PCB, 45, PAL_WASH_C, PAL_WIRE_C, 86 }, 127, 0 },
    { "AMBER", "AMBER", PAL_AMBER, PAL_AMBER, PAL_AMBER, PAL_WIRE_A, PAL_WASH_A, PAL_TERM_INK_A, PAL_AMBER,
      { 104, 108, PAL_WASH_A, PAL_WIRE_A, 177 }, 250, 0 },
    { "MINT", "MINT", PAL_MINT, PAL_MINT, PAL_MINT, PAL_WIRE_M, PAL_WASH_M, PAL_TERM_INK_M, PAL_MINT,
      { 13, 45, PAL_WASH_M, PAL_WIRE_M, 90 }, 191, 0 },
};
#else
/* v1019n-mod (#5): GRAPE / PINK / DUO / ULTRA unchanged; SYNTH is now a real two-colour fade (hot magenta
 * strokes fading from a sunset orange, was orange-yellow strokes); four DRAFT multi-colour / texture themes
 * follow (Mobandy to keep / rename / cut): VAPOR, AURORA, PRISM, GLITCH. */
static const int THEME_N = 9;
static const ui_theme_t THEMES[9] = {
    /* Neon Grape: electric grape violet, lime-leaf second accent (preview eyes), grape wires */
    { "NEON GRAPE", "GRAPE", 167, 156, 167, 69, 33, 211, 156, { 134, 5, 33, 69, 134 }, 211, 0 },
    /* Hot Pink Circuit: hot pink strokes on deep-pink circuit traces, cyan nodes */
    { "HOT PINK", "PINK", 194, 31, 194, 97, 33, 210, 210, { 161, 40, 33, 97, 194 }, 210, 0 },
    /* ---- v1019r (#7): themes 2-8 reworked so all nine read as different colours (no repeated hue, no
     * near-white PRISM, multi themes visibly multi on the hub / Terminal / scan / games). 0 and 1 are
     * byte-identical (gate theme RGB). Solids 0-4, multi 5-8. Derived slots = nearest natives (no sheet
     * codes): wire ~0.40, wash ~0.20, ink = 45 % toward white, tone {0.62, texture, wash, wire, 0.85}. ---- */
    /* Toxic: acid lime strokes, purple second accent */
    { "TOXIC", "TOXIC", 156, 195, 156, 76, 44, 189, 189, { 112, 5, 44, 76, 120 }, 189, 0 },
    /* Gold Rush: gold strokes, electric-blue second accent */
    { "GOLD RUSH", "GOLD", 248, 47, 248, 72, 44, 253, 253, { 144, 5, 44, 72, 212 }, 253, 0 },
    /* Ice: ice cyan strokes, orchid second accent */
    { "ICE", "ICE", 31, 207, 31, 14, 1, 127, 127, { 18, 5, 1, 14, 58 }, 127, 0 },
    /* Synthwave: hot magenta (top) -> sunset orange fade, orange strokes */
    { "SYNTHWAVE", "SYNTH", 240, 231, 240, 104, 44, 250, 231, { 136, 5, 44, 104, 204 }, 250, 2 },
    /* Aurora: three-stop fade, ice sky -> green -> violet, green strokes */
    { "AURORA", "AURORA", 60, 27, 60, 12, 36, 157, 27, { 48, 5, 36, 12, 56 }, 157, 3, 163 },
    /* Prism: spectrum texture (magenta / orange / gold / green / cyan / blue), electric-blue strokes (was pearl) */
    { "PRISM", "PRISM", 47, 252, 47, 9, 37, 151, 252, { 42, 5, 37, 9, 43 }, 151, 4 },
    /* Glitch: RGB-split texture, red strokes with a cyan split */
    { "GLITCH", "GLITCH", 224, 31, 224, 96, 40, 205, 31, { 128, 5, 40, 96, 192 }, 205, 5 },
};
#endif
static int theme_ix(int c) { return (c < 0 || c >= THEME_N) ? 0 : c; }
static const char *const MIX_N[3] = { "BOT + DEVICE", "BOTS", "DEVICES" };
/* v1019n (#6): the former SCREEN SAVERS are now BACKGROUNDS (botcorps_rain_set_bg): RAIN = our one BOTcorps
 * rain (default); BOTS / GIBSON / FLYBY / BOUNCE = that saver animation runs live INSTEAD of the rain. */
static const int BGS_NUM = 5;
static const char *const BGS_N[BGS_NUM] = { "RAIN", "BOTS", "GIBSON", "FLYBY", "BOUNCE" };
/* v1019n-mod (#7): BG COLOR (the old IDLE-time chip): the multi-colour bots + four solid colours,
 * as rain modes (0 MULTI, 1 GREEN, 2 CYAN, 4 PURPLE, 5 AMBER). BLUE / WHITE stay reachable on the CP part. */
static const int BGC_N = 5;
static const int8_t BGC_MODE[BGC_N] = { 0, 1, 2, 4, 5 };
static const char *const BGC_S[BGC_N] = { "MULTI", "GREEN", "CYAN", "PURPLE", "AMBER" };

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

static const uint8_t BOT_PAL[12] = {   /* v1019r-pending (#8): 12 unique fixed bot colours (min dE 36.8) */
    87 /*1BDD wifi*/, 59 /*269C bt*/, 201 /*DA4A deauth*/, 122 /*1E88 hack*/, 185 /*B6CA flock*/, 252 /*FFE0 drone*/,
    245 /*FDC9 airtag*/, 94 /*5FF8 keyfob*/, 131 /*901F meta*/, 234 /*FA55 skimmer*/, 241 /*F2E3 flipper*/, 207 /*E2FF samsung*/,
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
    /* v1019q (#9a): the bot commands first, the shared ones (help clear cmds) last; cmds stays chip 11 */
    "scan", "hunt", "bots", "ping", "info", "track",
    "sniff", "filter", "rain", "help", "clear", "cmds"
};
/* v1019x V1: build version string, set per build by tools/pio_version.py (host: tools/host/build.sh) */
#ifndef BOTCORPS_BUILD_ID
#define BOTCORPS_BUILD_ID "v1019x nogit"
#endif
/* v3.2 command menu. 42 names + purposes, const in flash. defend stays in CORE.
 * v1019q (#9): CORE + COMMS (the bots' own commands) stay on top, go / load (screen routing) moved down
 * to SYS; two-word forms added only where clearer ("scan all", "hunt run"); every single word kept. The
 * first 8 CORE rows are unchanged (the Tab sheet window, G7-frozen terminal-tab). */
static const int CMD_N = 42;
static const int G_AT[5] = { 0, 16, 24, 33, 42 };
static const char *const G_NAME[4] = { "CORE", "COMMS", "GAME", "SYS" };
static const char *const CMDN[CMD_N] = {
    "scan", "hunt", "help", "bots", "rain", "filter", "ping", "info", "track", "sniff",
    "defend", "scan all", "sweep", "roster", "pulse", "dig",
    "hail", "say", "ask", "rollcall", "whisper", "broadcast", "link", "mute",
    "train", "hunt-run", "hunt run", "score", "rank", "loadout", "lobby", "duel", "stash",
    "go", "load", "clear", "history", "theme", "about", "cmds", "version", "uptime"
};
static const char *const CMDP[CMD_N] = {
    "sweep \x7f wifi|ble|all", "drone|flock|skimmer|deauth", "list the commands", "board status",
    "rain \x7f <colour>|mixed", "wifi|ble|threats|strong|off", "ping a found device", "device details",
    "pin a device \x7f off", "list heard (filtered)",
    "defend game \x7f <bot>", "crew sweep, 12 boards", "alias \x7f scan", "alias \x7f bots", "alias \x7f ping", "alias \x7f info",
    "greet a bot", "send a line to a bot", "query bot status", "crew check-in",
    "quiet line to a bot", "speak to the crew", "open a bot channel", "hush a bot",
    "play this bot's game", "gibson hunt game", "gibson hunt game", "best runs", "games by best run",
    "this bot's 3 games", "all 12 games", "sealed", "sealed",
    "jump to a screen", "load a screen",
    "wipe the log", "recent lines", "next ui colour", "build blurb", "this menu", "term build", "session clock"
};
static const uint8_t CMD_ARG[CMD_N] = {
    1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0
};
static const int PCMD_N = CMD_N;
static const char *const *const PCMD = CMDN;
/* Gibson chat. Own set. Flash only.
 * v1019q (#9): the Defend board's own GAME commands lead, then the bot commands (ASK), then hub / route /
 * system. Two-word forms only where clearer ("jack in", "go hub"); every single word kept; "play" (wired in
 * v1019p, never listed) is now listed. */
static const int GC_N = 25;
static const int GC_GAT[6] = { 0, 9, 13, 17, 22, 25 };
static const char *const GC_GN[5] = { "GAME", "ASK", "HUB", "ROUTE", "SYS" };
static const char *const GC_NM[GC_N] = {
    "jackin", "jack in", "play", "lobby", "score", "defend", "night", "sweep", "dex",
    "ask", "hail", "whisper", "why",
    "crew", "scan", "status", "mood",
    "terminal", "go", "go hub", "load", "control",
    "clear", "history", "about"
};
static const char *const GC_PP[GC_N] = {
    "play this bot's game", "same as jackin", "<1-12> any game", "list all 12 games", "best runs", "<bot>|beginner|veteran",
    "gibson night shift", "gibson sweep", "device dex",
    "ask a bot", "call a bot", "quiet line", "gibson explains",
    "roll the crew", "bus sweep", "core link status", "gibson mood",
    "open terminal", "terminal|scan|control|hub", "back to the hub", "screen|game", "control panel",
    "wipe the chat", "recent lines", "gibson build"
};
static const uint8_t GC_ARG[GC_N] = {
    0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0
};
/* Index into CMD_ICO. Flash only. */
static const uint8_t GC_II[GC_N] = {
    10, 10, 24, 19, 23, 22, 2, 15, 17, 2, 3, 0, 7, 2, 34, 33, 33, 34, 4, 32, 35, 37
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
    { 0x7e, 0xff, 0xff, 0x7e, 0x7e, 0x3c, 0x18, 0x00 },
    { 0x38, 0x44, 0x82, 0x82, 0x44, 0x38, 0x04, 0x02 },
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
    { 0x18, 0x18, 0x24, 0xe7, 0xe7, 0x24, 0x18, 0x18 },
    { 0x3c, 0x42, 0xa5, 0x81, 0x99, 0x42, 0x3c, 0x00 },
    { 0x18, 0x3c, 0x7e, 0xff, 0x18, 0x18, 0x18, 0x00 },
    { 0xff, 0x81, 0xbd, 0xa5, 0xbd, 0x81, 0xff, 0x00 },
    { 0x3c, 0x42, 0x81, 0x99, 0x81, 0x42, 0x3c, 0x00 },
    { 0x81, 0xc3, 0x66, 0x3c, 0x3c, 0x66, 0xc3, 0x81 },
    { 0x3c, 0x42, 0x81, 0xbd, 0xbd, 0x81, 0x42, 0x3c },
    { 0x00, 0x08, 0x04, 0xfe, 0x04, 0x08, 0x00, 0x00 },
    { 0x00, 0x08, 0x04, 0xfe, 0x04, 0x08, 0x00, 0x00 },
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
/* v1019j: Defend mini-game running on the game screen (botcorps_defend.inc); -1 = none */
static int s_dg = -1;
static bool dg_on(void) { return s_dg >= 0 && s_screen == SCR_GCHAT; }
static void dg_start(int bot);
static void dg_start_own(int bot);          /* v1019r (#5): the bot's OWN game (jackin / Enter on an empty prompt) */
static void dg_start_game(int bot, int game);
static void paint_dg_demo(Gfx g, int bot);  /* v1019r (#5/#1): idle Defend board = a still of the bot's own game */
static void dex_open(void);
static void paint_dg_demo_hint(Gfx g, int bot);
static void paint_dex(Gfx g);
static void paint_dg_board(Gfx g);
static void paint_dg_hud_ext(Gfx g);   /* MOD item 14: game HUD in the Defend prompt panel */
static bool dg_zap_full(void);         /* v1019v: Gibson zap Defend (v1019x: inside the scan-shell grid) */
static bool zap_try_cmd(const char *line);
static bool zap_sb_on(void);           /* v1019x: Gibson Defend = restored scan shell + zap in the grid */
static void zap_paint_grid(Gfx g);
static void zap_strip(Gfx g, int dx, int th, int ty, int dw);
static void zap_rain_rect(Gfx g);
static void zap_tick_patch(void);
static void dg_paint_hud(Gfx g, int width, bool full);
static int s_cursor = 0;
static int s_bot = 2;
static int s_term = 2;
static int s_color = 0;
static int s_mix = 0;
static int s_bg = 0;          /* v1019n: BACKGROUND 0 RAIN, 1..4 the saver animations (was s_saver) */
static int s_bg_motion = 1;   /* v1019n-mod: PWR LED = background motion on/off (was saver on/off) */
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
static const int SB_N = 35;   /* mod item 9: device CAPACITY per band (grid extends by rows past 28) */
static int s_sb_n = 12;       /* devices actually in the sample list (12 stock; host can raise it for overflow) */
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
/* v1019u: found-alert is DEAD until a live radio scan exists. The Adv scan lists are canned
 * (cafe-guest etc.), so an alert raised from them is null and void. BOTCORPS_LIVE_SCAN defaults
 * to 0: fa_live() is then a compile-time false on the device, so the takeover can never paint and
 * never eats Up/Down/OK/Esc, whatever s_fa_on holds. The host renderer keeps its forced preview
 * shot of the art (botcorps_host_found_alert*, BOTCORPS_HOST_RENDER only); no scan path sets it. */
#ifndef BOTCORPS_LIVE_SCAN
#define BOTCORPS_LIVE_SCAN 0
#endif
#if BOTCORPS_LIVE_SCAN || defined(BOTCORPS_HOST_RENDER)
static const bool FA_ENABLED = true;
#else
static const bool FA_ENABLED = false;
#endif
static inline bool fa_live(void) { return FA_ENABLED && s_fa_on; }
/* Nav focus. Drawn in place. Never promotes a full-panel clear. */
static int s_focus_old = -1;
static int s_focus_new = -1;
static bool s_focus_pend = false;
static bool s_track = false;
static bool s_focus_cmds = false;
/* v1017a: the commands (≡) button is a cursor stop: Down from the bottom board row; Enter opens the
 * commands menu (the 6x2 chip palette, s_focus_cmds). */
static bool s_cmd_btn = false;
/* #10 (Dirt 9:04): the Terminal's attack-prompt button (under the commands button, in the companion
 * box) and the overlay it pulls up: 0 none, 1 device glossary, 2 per-bot/device attack prompts. */
static bool s_atk_btn = false;
static int s_term_ovl = 0;
/* v1017c (#3): the prompt line is a cursor stop too, like the hub: Up/Down walk
 * boards -> prompt -> commands (≡) button; Left/Right go back to the boards. */
static bool s_term_prompt = false;
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
static int s_gc_caret = -1, s_gc_hist_i = -1;   /* MOD item 2: Defend prompt caret (-1 = end) + history walk */
static char s_gp_log[2][28];
static int s_gp_n = 0;
static bool s_gp_focus = false;
/* #9/#11 (Dirt 2026-10-17 9:04-9:08): dynamic "<Name>> Defend" game title + a viewable device
 * glossary + a per-bot/device attack-prompt menu. The 4 Defend CATEGORIES (A trackers->Triangulate,
 * B watchers/cameras->Sightlines, C network/Meta-glasses->Firewall, D access-thieves->Rolling Code)
 * stay MOCK-FIRST per Dirt: the title plumbing, the glossary screen and the attack menu are built as
 * structural hooks wired to the dynamic title; the category gameplay itself is left as a stub. */
/* v1019 (Dirt #6): the scan box terminal strip (core + radio + every sub-scan) takes typing. */
static char s_sp_buf[14];        /* typed line after user@botcorps:~$ */
/* v1019r (#4): opening wifibot / bluetoothbot's scan (Terminal Select, hub / Terminal wifi|ble routes)
 * pre-types its scan command on the scan prompt, then runs it ~650 ms later (0 = none pending). Esc while
 * it is pending cancels it and still goes straight back (one Esc, OPEN gate); any other key takes over. */
static uint32_t s_sp_auto_t = 0;
static void sp_auto_arm(void);
static char s_sp_l1[32], s_sp_l2[32];   /* last command echo + reply (replace the boot lines) */
static bool s_sp_has = false;    /* a command ran: the strip shows s_sp_l1/l2 */
static int s_sp_cmd = -1;        /* SCR_SCAN arrows: index into SP_CMD (≡ chip lit when >= 0) */
static int s_defend_bot = 12;    /* which bot/device the active Defend game is for (12 = Gibson) */
static int s_gc_panel = 0;       /* game-board overlay: 0 none, 1 device glossary, 2 attack prompts */
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

static const ui_theme_t &theme(void) { return THEMES[theme_ix(s_color)]; }
static uint8_t accent(void) { return theme().ac; }
static uint8_t accent2(void) { return theme().ac2; }
static uint8_t th_wire(void) { return theme().wire; }
static uint8_t th_wash(void) { return theme().wash; }
static uint8_t th_sel(void) { return theme().sel; }
/* v1019: selection ink for dash/segment k on side (0 top, 1 right, 2 bottom, 3 left).
 * Solid themes: theme().sel (= the accent on the legacy table, so legacy renders are unchanged).
 * Duotone: dashes alternate the two tones. Gradient: top/left the gradient start, bottom/right end. */
static uint8_t sel_ink(int k, int side)
{
    const ui_theme_t &t = theme();
    if (t.grad == 1) return ((k >> 1) & 1) ? t.ac2 : t.ac;
    if (t.grad == 2) return (side == 0 || side == 3) ? t.ac2 : t.ac;
    if (t.grad == 3) return (side == 0 || side == 3) ? t.ac2 : ((k & 1) ? t.ac3 : t.ac);
    if (t.grad == 4) { static const uint8_t H[6] = { 231, 240, 248, 60, 31, 47 }; return H[(k + side) % 6]; }
    if (t.grad == 5) return (k % 3 == 0) ? t.ac2 : t.ac;
    return t.sel;
}
static uint16_t mix16(uint16_t a, uint16_t b, int num, int den);
/* v1019n-mod (#5): the theme's ink at position pos of len along a stroke (selection outline, preview rows,
 * feed trace, accent rule). Solid themes: the selection colour (identical pixels to before). */
static uint8_t th_ink_at(int pos, int len)
{
    const ui_theme_t &t = theme();
    if (len < 2) len = 2;
    if (pos < 0) pos = 0;
    if (pos > len - 1) pos = len - 1;
    switch (t.grad) {
    case 1: return pos < len / 2 ? t.ac : t.ac2;
    case 2: return botcorps_pal_fit(mix16(botcorps_rgb565(t.ac2), botcorps_rgb565(t.ac), pos, len - 1));
    case 3: {
        const int h = (len - 1) / 2;
        if (pos <= h) return botcorps_pal_fit(mix16(botcorps_rgb565(t.ac2), botcorps_rgb565(t.ac), pos, h > 0 ? h : 1));
        return botcorps_pal_fit(mix16(botcorps_rgb565(t.ac), botcorps_rgb565(t.ac3), pos - h, (len - 1 - h) > 0 ? len - 1 - h : 1));
    }
    case 4: { static const uint8_t H[6] = { 231, 240, 248, 60, 31, 47 }; return H[(pos * 6 / len) % 6]; }
    case 5: return ((pos / 3) % 4 == 1) ? t.ac2 : t.ac;
    default: return t.sel;
    }
}
/* the same ink dimmed to wire strength (~0.35 over the black board) */
static uint8_t th_wire_at(int pos, int len);
/* v1019r (#7): multi themes visibly multi - per-item ink along the theme (hub tiles / rails, Terminal
 * board rims); solid themes = the plain accent. Theme 0 (every gate render but hub-amber /
 * settings-amber) is untouched by all of these. */
static uint8_t th_item(int i, int n) { return theme().grad ? th_ink_at(i, n) : accent(); }
/* hub core rings: cyan on NEON GRAPE (unchanged), the theme's second accent on solids, the theme's
 * fade / spectrum across the rings on multi themes */
static uint8_t th_ring(int k)
{
    if (theme_ix(s_color) == 0) return PAL_CYAN;
    return theme().grad ? th_ink_at(k, 5) : theme().ac2;
}
/* the Defend games' accent: cyan on NEON GRAPE (the frozen defend renders), the theme accent otherwise */
static uint8_t dg_ac(void) { return theme_ix(s_color) == 0 ? (uint8_t)PAL_CYAN : accent(); }
static uint8_t dg_wire(void) { return theme_ix(s_color) == 0 ? (uint8_t)PAL_WIRE_C : th_wire(); }
static uint8_t th_wire_at(int pos, int len)
{
    if (theme().grad == 0) return theme().wire;
    return botcorps_pal_fit(mix16(botcorps_rgb565(th_ink_at(pos, len)), botcorps_rgb565(PAL_BLACK), 62, 100));
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
static bool s_gib_hunt = false;      /* #5 mock bus.gHunt: Gibson's board plays the hunt (else orbit-leader) */
static bool s_gib_hunt_keep = false; /* #5 mock nav.scanKeep: hunt mode survives only the entry that started it */
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
/* v1019 (Dirt #5/#6): the Terminal and the Gibson game step their rain live again (it was a
 * frozen static layer since v1016k). Both use covers so the direct path never paints rain over
 * their panels; on the band path the chrome is opaque over the rain. */
static bool wants_rain(void)
{
    return !sbox_on()
        && s_screen != SCR_DIAG
        && !dg_on()                   /* v1019j: Defend game = static rain around the opaque board */
        && !zap_sb_on();              /* v1019x: Gibson Defend steps its own rain, left chip rect only */
}

/* Panel screens: rain painted once per full paint (no stepping, no covers needed). */
static bool rain_static(void)
{
    return sbox_on() || s_screen == SCR_DIAG
        || dg_on() || zap_sb_on();
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
static void chip_mini(Gfx g, int cx, int cy, int T, int bot, bool blink, int face_dy);
static uint16_t mix16(uint16_t a, uint16_t b, int num, int den);

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
    const uint8_t glow = th_wash();
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
/* v1019: Terminal / game covers exist only for the direct (unbuffered) panel path. The host renderer
 * paints whole frames like the device's composed bands, so it shows the device look (no covers). */
static bool rain_direct_covers(void)
{
#ifdef BOTCORPS_HOST_RENDER
    return false;
#else
    return botcorps_band_direct();
#endif
}

/* v1019q (item 7): Gibson's Defend-screen chip size. 44 px (was 36) on his Defend board; the in-game board
 * keeps 36 because the Gibson game renders (defend-maze / -signaltrace / -jamdodge) are G7-frozen and this
 * build may only re-lock L1 hashes. Other bots' Defend chips are unchanged. */
static int gib_def_chip(void)
{
    const bool gib = !(s_defend_bot >= 0 && s_defend_bot < 12);
    /* v1019r (#9): 52 px (was 44) idle and in his own NIGHT SHIFT / SWEEP (ids 12/13, not G7 renders); the
     * frozen Gibson game renders (maze / trace / jam) keep 36. Cover follows (layout_covers). */
    return (gib && (!dg_on() || s_dg >= 12)) ? 52 : 36;
}
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
        /* v1019: on the composed band path the chrome is repainted over the rain every frame, so
         * the rain keeps the full-screen layer it had as a static layer (no covers). Covers only
         * matter when the rain steps straight onto the panel (direct fallback). */
        if (!rain_direct_covers()) break;
        cover_add(0, 0, 320, 17);
        for (int i = 0; i < 12; ++i) {
            int x, y;
            term_xy(i, &x, &y);
            cover_add(x - 2, y - 2, 76, 50);
        }
        cover_add(4, 167, 312, 72);
        if (s_sheet) cover_add(8, 21, 304, 142);
        break;
    case SCR_GCHAT:                        /* v1019: live rain frames the game panels, never over them */
        if (!rain_direct_covers()) break;  /* composed path: chrome over rain, as the static layer */
        cover_add(0, 0, 320, 18);          /* top bar */
        if (dg_zap_full()) {
            cover_add(0, 17, 320, 131);    /* v1019v: full-width zap scan chrome */
            cover_add(2, 146, 316, 94);    /* prompt */
        } else {
            cover_add(2, 17, 106, 30);     /* "<Name>> Defend" title plate */
            {                              /* Gibson chip (v1019q item 7: follows its size) */
                const int cs = gib_def_chip();
                cover_add(56 - cs / 2 - 6, 78 - cs / 2 - 6, cs + 12, cs + 12);
            }
            cover_add(104, 18, 26, 130);   /* rail buttons */
            cover_add(126, 16, 194, 132);  /* game board */
            cover_add(2, 146, 316, 94);    /* chat panel */
        }
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
    if (id == SCR_SCAN) {   /* #5 mock: entering the scan board clears Gibson hunt mode unless gibHuntStart opened it */
        if (!(s_gib_hunt_keep && s_scan_bot == 12)) s_gib_hunt = false;
        s_gib_hunt_keep = false;
    }
    const char *from = screen_name(s_screen);
    const int before = card_bot();
    if (s_dg >= 0) { s_dg = -1; s_int_dirty = true; }   /* v1019j: leaving / re-entering ends a Defend game */
    s_ret = ret;
    s_screen = id;
    s_cursor = 0;
    s_hub_zone = 0;
    s_sheet = false;
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0; s_term_prompt = false;
    s_term_crown = false;
    scan_all_set(false);
    botcorps_lut_term(false);
    s_bus_t0 = millis();
    s_act = -1;
    s_act_dev = -1;
    s_info_dev = -1;
    s_gloss = -1;
    s_sp_buf[0] = 0; s_sp_has = false; s_sp_cmd = -1;
    s_sp_auto_t = 0;
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

static void enter_defend(int bot, int ret);
static int gc_bot_of(const char *arg);
static void dg_lobby(void);
static void dg_scores(void);
static bool dg_play_n(int n);
static void dg_scores_bot(int bot);
static void dg_loadout(int bot);
static void dg_rank(void);
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
    /* v1019q (#9b): two-word form "hunt run" = hunt-run */
    if (strcmp(cmd, "hunt") == 0 && arg && strcmp(arg, "run") == 0) { cmd = (char *)"hunt-run"; arg = 0; }
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
        /* v1019r (#7): the four group rows (CORE / COMMS / GAME / SYS) ran up to 68 chars into a 50-char
         * log and clipped at the right edge. They now open as a wrapped, colour-coded commands panel over
         * the boards (every word shown; Esc or any key closes); the log keeps one short line. */
        log_add("[ ok ] commands - esc closes");
        s_term_ovl = 3;
        s_dirty = true;
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
        else if (strcmp(arg, "threats") == 0) s_filter = 3;   /* v1019q (#9c): listed, now wired */
        else if (strcmp(arg, "strong") == 0) s_filter = 4;
        else {
            log_add("unknown filter");
            partial_prompt_panel();
            return;
        }
        static const char *const FW[5] = { "filter off", "filter wifi", "filter ble", "filter threats", "filter strong" };
        log_add(FW[s_filter & 7]);
    } else if (strcmp(cmd, "ping") == 0 || strcmp(cmd, "info") == 0 ||
               strcmp(cmd, "track") == 0 || strcmp(cmd, "sniff") == 0) {
        if (strcmp(cmd, "track") == 0 && arg && strcmp(arg, "off") == 0) {
            s_track_name[0] = 0;
            log_add("track off");
        } else if (s_found_n == 0) {
            log_add("scan first");
        } else if (strcmp(cmd, "sniff") == 0) {
            /* v1019q (#9c): "list heard (filtered)" applies the filter (it used to ignore it). wifi / ble = that
             * radio; strong = -60 dBm or better; threats = open / guest access points (no password). */
            int shown = 0;
            for (int i = 0; i < s_found_n; ++i) {
                const char *nm = botcorps_t0_term_found[i];
                int band = 0, dbm = -99;
                for (int k = 0; k < 3; ++k) {
                    if (strcmp(nm, NEAR_WIFI[k]) == 0) { band = 1; dbm = atoi(NEAR_WIFI_R[k]); }
                    if (strcmp(nm, NEAR_BLE[k]) == 0) { band = 2; dbm = atoi(NEAR_BLE_R[k]); }
                }
                const int f = s_filter & 7;
                if ((f == 1 && band != 1) || (f == 2 && band != 2) || (f == 4 && dbm < -60)) continue;
                if (f == 3 && !(band == 1 && (strstr(nm, "guest") || strstr(nm, "lobby")))) continue;
                char row[80];
                snprintf(row, sizeof(row), "  %s", nm);
                log_add(row);
                ++shown;
            }
            if (!shown) log_add("none heard for this filter");
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
        if (!arg) log_add("rain gibson terminal bots scan wifi ble wifilist blelist control diag");
        else if (strcmp(arg, "rain") == 0) id = SCR_RAIN;
        else if (strcmp(arg, "gibson") == 0) id = SCR_GIBSON;
        else if (strcmp(arg, "terminal") == 0) id = SCR_TERMINAL;
        else if (strcmp(arg, "wifi") == 0) { s_scan_bot = 0; id = SCR_SCAN; }   /* v1016n: wifibot's scan screen (live box) */
        else if (strcmp(arg, "ble") == 0) { s_scan_bot = 1; id = SCR_SCAN; }    /* v1016n: bluetoothbot's scan screen */
        else if (strcmp(arg, "control") == 0) id = SCR_CONTROL;
        else if (strcmp(arg, "diag") == 0) id = SCR_DIAG;
        else if (strcmp(arg, "bots") == 0) id = SCR_TERMINAL;                   /* v1016n: the 12 ESP32 chip tiles */
        /* v1019q (#9c): the listed go / load options that used to answer "no such screen" */
        else if (strcmp(arg, "scan") == 0) { s_scan_bot = 12; id = SCR_SCAN; }  /* Gibson's scan board */
        else if (strcmp(arg, "wifilist") == 0) id = SCR_WIFI_LIST;
        else if (strcmp(arg, "blelist") == 0) id = SCR_BT_LIST;
        else log_add("no such screen");
        if (id >= 0) {
            /* v1016n: "go bots" from the terminal is the terminal itself; never make it its own back target */
            enter_screen(id, id == SCR_TERMINAL ? SCR_GIBSON : SCR_TERMINAL);
            sp_auto_arm();
            return;
        }
    } else if (strcmp(cmd, "cmds") == 0) {
        s_sheet = true;
        s_sheet_i = 0;
        s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
        s_dirty = true;
        return;
    } else if (strcmp(cmd, "hail") == 0 || strcmp(cmd, "ask") == 0 || strcmp(cmd, "link") == 0 ||
               strcmp(cmd, "mute") == 0 || strcmp(cmd, "say") == 0 || strcmp(cmd, "whisper") == 0) {
        int bot = -1;
        /* v1019 prompt audit: the bot is the FIRST word ("say flock hi" used to be "no such bot") */
        const size_t aw = arg ? strcspn(arg, " ") : 0;
        if (!arg || !arg[0]) bot = -1;
        else if ((aw == 6 && strncmp(arg, "gibson", 6) == 0) || (aw == 3 && strncmp(arg, "gib", 3) == 0)) bot = 12;
        else {
            bot = -2;
            for (int i = 0; i < 12; ++i) {
                if (strncmp(BOTS[i].short_name, arg, aw) == 0) {
                    bot = i;
                    break;
                }
            }
        }
        if (bot == -1) {
            char row[40];
            snprintf(row, sizeof(row), "%s <bot>: name a bot", cmd);   /* v1019: was the bare command name */
            log_add(row);
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
        /* v1019q (#9c): "next ui colour" - steps the UI colour (same 9 themes as the control panel) */
        s_color = (s_color + 1) % THEME_N;
        char row[40];
        snprintf(row, sizeof(row), "theme %s", THEMES[theme_ix(s_color)].name);
        log_add(row);
        s_dirty = true;
        s_int_dirty = true;
    } else if (strcmp(cmd, "about") == 0) {
        log_add("BOTcorps term 3.2");
    } else if (strcmp(cmd, "version") == 0) {
        log_add("term 3.2 " BOTCORPS_BUILD_ID);   /* v1019x V1: old token kept + real build id */
    } else if (strcmp(cmd, "uptime") == 0) {
        char row[32];
        const uint32_t sec = (millis() - s_boot_ms) / 1000u;
        snprintf(row, sizeof(row), "uptime %u m %u s", (unsigned)(sec / 60u), (unsigned)(sec % 60u));
        log_add(row);
    } else if (strcmp(cmd, "train") == 0) {
        /* v1019q (#9c): the GAME group does what it lists (was "sealed" for all eight) */
        enter_defend(s_term, SCR_TERMINAL);      /* play this bot's game */
        return;
    } else if (strcmp(cmd, "hunt-run") == 0) {
        s_scan_bot = 12;                         /* Gibson's hunt game (his scan board, hunt mode on) */
        s_gib_hunt = true;
        s_gib_hunt_keep = true;
        enter_screen(SCR_SCAN, SCR_TERMINAL);
        return;
    } else if (strcmp(cmd, "score") == 0) {
        dg_scores_bot(s_term);
    } else if (strcmp(cmd, "rank") == 0) {
        dg_rank();
    } else if (strcmp(cmd, "loadout") == 0) {
        dg_loadout(s_term);
    } else if (strcmp(cmd, "lobby") == 0) {
        dg_lobby();
    } else if (strcmp(cmd, "duel") == 0 || strcmp(cmd, "stash") == 0) {
        log_add("sealed");                       /* no duel / stash feature exists yet (listed as sealed) */
    } else if (strcmp(cmd, "defend") == 0) {
        /* v1019 (Dirt #6/#7): CMDP says defend is the typing game; it enters it (was "sealed"). */
        const int b = gc_bot_of(arg && *arg ? arg : nullptr);
        enter_defend(b < 0 ? 12 : b, SCR_TERMINAL);
        return;
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
    sp_auto_arm();   /* v1019r (#4): wifibot / bluetoothbot: "scan wifi" / "scan ble" typed, then run */
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


/* v1019d (Dirt #4): brighter ghost-rain ink for the Terminal (was index 12, (0,89,16), which left the
 * panel lost in black). Still glyph pixels only, under faces / labels / prompt text, never over. */
/* v1019n (#15/#19): the Terminal / prompt rain is OUR DEVICE-ICON RAIN only (botcorps_rain_set_no_bots);
 * it and its ghosts never follow the UI colour. */
static uint8_t term_ghost_ink(void)
{
    return botcorps_pal_fit(0x2CAA);   /* ~(40,150,80); the device-only ghost draws each icon in its own ink */
}
static uint8_t term_prompt_ghost_ink(void)
{
    return botcorps_pal_fit(0x0366);   /* ~(0,108,48) */
}
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
        const uint8_t wash = th_wash();
        const uint8_t wire = th_wire();
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
            g.fillRect(x - 2 + d, y - 2, dw, 1, sel_ink(k, 0));
            const int bx = x + w + 1 - d - n + (d + n > w + 2 ? d + n - (w + 2) : 0);
            g.fillRect(bx, y + h + 1, dw, 1, sel_ink(k, 2));
        }
        for (int d = 0, k = 0; d < h + 4; d += n, ++k) {
            if (((k + ph) % 2) != 0) continue;
            int dh = n;
            if (d + dh > h + 2) dh = h + 2 - d;
            if (dh <= 0) continue;
            g.fillRect(x + w + 1, y - 2 + d, 1, dh, sel_ink(k, 1));
            const int by = y + h + 1 - d - n + (d + n > h + 2 ? d + n - (h + 2) : 0);
            g.fillRect(x - 2, by, 1, dh, sel_ink(k, 3));
        }
    }
    /* v1019s treatment D: darker matte panel (black) so circuit/wave icons read; chip untouched. */
    (void)wh;
    const uint8_t base = (uint8_t)PAL_BLACK;
    /* v1019s D: 1-2 px rim + brackets + label bar in this bot's fixed colour. */
    const uint8_t rim = bot_color(i);
    /* v1019s D: raised contrast on traces/antennas against the black panel. */
    const uint8_t tr = (uint8_t)PAL_STEEL;
    const uint8_t ant = (uint8_t)PAL_MUTED;
    const uint8_t pad = (uint8_t)PAL_SOFT;
    const uint8_t fp = sel ? (uint8_t)PAL_BLACK : (uint8_t)PAL_FIELD;
    const uint8_t fpp = (uint8_t)PAL_SOFT;
    const uint8_t silk = sel ? (uint8_t)PAL_WHITE : (uint8_t)PAL_MUTED;
    const uint8_t led = (uint8_t)PAL_BOARD_D;
    const uint8_t hole = (uint8_t)PAL_FIELD;
    g.fillRect(x + 1, y, w - 2, h, base);
    g.fillRect(x, y + 1, w, h - 2, base);
    /* v1019t D finish: darker horizontal label band behind the bot name (concept D label box, the
     * PAL_BOARD_D plate) across the inner width, rows y+36..y+43. Drawn before the socket, so the
     * selected outline at y+36 and the label still sit on top; rain ghost stops at y+35. */
    g.fillRect(x + 2, y + 36, w - 4, 8, (uint8_t)PAL_BOARD_D);
    /* Traces on focused tile only (same as before); D raises their contrast via STEEL/MUTED + black panel. */
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
    /* v1019 (Dirt #5): the live Terminal rain shows faintly through the board tile (glyph pixels
     * only, dim ink). Drawn before the bot face and the label, so neither is ever crossed. */
#ifdef BOTCORPS_HOST_RENDER
    if (g.raw() && !host_gate_norain)
#else
    if (g.raw())
#endif
        botcorps_rain_ghost(*g.raw(), g.ox, g.oy, x + 1, y + 1, w - 2, 35, term_ghost_ink());
    const int bob = sel ? -((int)((t / 450u) % 2u)) : 0;
    /* v1017b (Dirt 8:20): every terminal tile is the SQUARE chip-form block (the
     * same block figure Gibson's control-panel / scan-board chip uses), in this
     * bot's fixed colour on black. Never the round sprite. Terminal tiles only. */
    /* v1019e (Dirt): full matched chip (silk outline, black window, silver pins) in the 26x22 socket:
     * 20 px box at x+26..45, y+15..34, face 14 px. The selected bob lifts the whole chip 1 px
     * (y+14..33), still inside the socket and clear of the select outline at y+13. */
    chip_mini(g, x + 36, y + 25 + bob, 20, i, sel && sprite_blink(i), 0);
    /* v1019f (Dirt standing rule): the live rain shows faintly THROUGH the chip face too, in the UI
     * theme's own colour (accent, so it follows the theme), blended 65/35 with the bot's colour so it
     * stays a faint overlay and the face still reads as that bot. Glyph pixels only, in the face rows
     * below the control-panel eyes (eye row + 1 px clear, whole glyph tops only) down to the chin: nothing in the eye band, so
     * no rain fragment ever sits on or beside an eye. The silk frame, pins and label are outside the
     * face, so the rain never reaches them. Drawn after the chip, before the label. */
#ifdef BOTCORPS_HOST_RENDER
    if (g.raw() && !host_gate_norain)
#else
    if (g.raw())
#endif
    {
        const int fs = 20 - 6, fx = x + 36 - fs / 2, fy = y + 25 + bob - fs / 2;
        const int ec = fs / 5 > 2 ? fs / 5 : 2;          /* control-panel eye cell (face_block) */
        const int gy0 = ec + ec + 1;                     /* first row clear of the eyes */
        const uint16_t face = botcorps_rgb565(bot_color(i));
        auto far = [face](uint8_t ix) {                  /* the overlay must still read on the face */
            const uint16_t c = botcorps_rgb565(ix);
            const int dr = (int)((c >> 11) & 31) - (int)((face >> 11) & 31);
            const int dg = ((int)((c >> 5) & 63) - (int)((face >> 5) & 63)) / 2;
            const int db = (int)(c & 31) - (int)(face & 31);
            return (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db) >= 16;
        };
        /* v1019n (#19): UI COLOR never recolours rain, so the face ghost no longer mixes the theme accent: a
         * lifted tint of the face, else a darkened one, so the device-icon rain still reads on every bot */
        uint8_t gi = botcorps_pal_fit(mix16(face, 0xFFFF, 45, 100));
        if (!far(gi)) gi = botcorps_pal_fit(mix16(face, 0x0000, 45, 100));
        botcorps_rain_ghost_ex(*g.raw(), g.ox, g.oy, fx, fy + gy0, fs, fs - gy0, gi, true);
    }
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
        txt(g, lbl, x + 36, y + 37, silk, lgfx::textdatum::top_center);
        s_px = keep;
    }
    /* v1019s D chrome last so rain/ghost never washes it: 2 px rim, L-brackets, label underline */
    g.fillRect(x + 1, y, w - 2, 2, rim);
    g.fillRect(x + 1, y + h - 2, w - 2, 2, rim);
    g.fillRect(x, y + 1, 2, h - 2, rim);
    g.fillRect(x + w - 2, y + 1, 2, h - 2, rim);
    const int bl = 10;
    g.fillRect(x, y, bl, 2, rim);
    g.fillRect(x, y, 2, bl, rim);
    g.fillRect(x + w - bl, y, bl, 2, rim);
    g.fillRect(x + w - 2, y, 2, bl, rim);
    g.fillRect(x, y + h - 2, bl, 2, rim);
    g.fillRect(x, y + h - bl, 2, bl, rim);
    g.fillRect(x + w - bl, y + h - 2, bl, 2, rim);
    g.fillRect(x + w - 2, y + h - bl, 2, bl, rim);
    /* v1019t D finish: a separate thick underline bar under the bot-name label (concept draw_underline:
     * 2 px, half-width min(33, len*3+2), bot colour), stacked under the 2 px bottom rim so the label
     * reads on a 4 px bar while the rest of the rim stays 2 px. Inside the tile's own patch/cover rect
     * (x-2, y-2, 76, 50). */
    {
        int ln = 0;
        while (lbl[ln]) ++ln;
        const int hw = ln * 3 + 2 < 33 ? ln * 3 + 2 : 33;
        g.fillRect(x + 36 - hw, y + h, hw * 2 + 1, 2, rim);
    }
    /* Faint Wi-Fi arcs on the right of the WIFI tile. Raised contrast (steel), always when radio. */
    if (BOTS[i].radio == 1 && !dim) {
        const uint8_t m = (uint8_t)PAL_STEEL;
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
        const bool sel = (i == s_term) && !s_focus_cmds && !s_cmd_btn && !s_atk_btn && !s_term_prompt;
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
static const int TERM_CMD_X = 292, TERM_ATK_X = 268, TERM_BTN_Y = 223;   /* v1019 prompt-row buttons */
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
        return th_wash();
    }
    return PAL_TERM_M10;
}

/* Selected sheet name and purpose. A light tint of the live accent. */
static uint8_t term_ink(void)
{
    return theme().ink;
}

/* Sheet-open steps are the 13 exact codes. Otherwise the reserved UI slots. */
static uint8_t glow_of(int step)
{
    if (!sheet_exact()) {
        if (step == STEP_SHADOW) return PAL_BG;
        if (step == STEP_HAIR || step == STEP_GLOW) return th_wire();
        if (step == STEP_AC) return accent();
        return th_wash();
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
    const uint8_t low = th_wash();
    const uint8_t mid = th_wire();
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
        const uint8_t mid = th_wire();
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
        const uint8_t sh = th_wire();
        txt(g, "terminal", 9, 5, sh, lgfx::textdatum::top_left);
        txt(g, "terminal", 8, 4, accent(), lgfx::textdatum::top_left);
        s_px = keep;
    }
    const int keep = s_px;
    s_px = 6;
    const uint8_t boards_wash = th_wash();
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
                             : theme().pur;
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

/* v1019r (#7): the scrollback plate rim - magenta on NEON GRAPE (frozen), the theme elsewhere (multi:
 * start / end of its fade) */
static uint8_t term_rim(int end) { return theme_ix(s_color) == 0 ? (uint8_t)PAL_TERM_M : th_ink_at(end ? 11 : 0, 12); }
static void paint_prompt(Gfx g)
{
    const int keep = s_px;
    s_px = 6;
    /* Tiles end at y=165. Deep black plate. Enter pulses the hairline cyan. */
    g.fillRect(4, 168, 312, 68, PAL_BLACK);
    /* v1019d (Dirt #4): the live rain shows faintly through the empty part of the prompt plate so the
     * panel no longer sinks into black. Only the rows below the last log line and above the prompt
     * row (y < 216), drawn before the sweep, text, buttons and candidates, so it never crosses text. */
    {
        int nl = 0;
        for (int i = s_log_n - 1; i >= 0 && nl < 6; --i) {
            const char *l = log_line(i);
            if (l && strncmp(l, "gibson>", 7) == 0) continue;
            ++nl;
        }
        /* whole 8 px rain cells only (glyph rows y+1..y+5), so no clipped half-glyphs at either edge */
        const int gy0 = ((171 + nl * 8 + 1) + 7) / 8 * 8, gy1 = 216;
#ifdef BOTCORPS_HOST_RENDER
        if (g.raw() && !host_gate_norain && gy1 > gy0)
#else
        if (g.raw() && gy1 > gy0)
#endif
            botcorps_rain_ghost(*g.raw(), g.ox, g.oy, 6, gy0, 308, gy1 - gy0, term_prompt_ghost_ink());
    }
    /* Plate chrome is the same with the sheet open or closed. The scan-all
     * frame, if any, is painted later and only inside the sheet rect. */
    {
        /* v1016f-2 #4: the scrollback box border is magenta, like the mock. */
        const bool pulse = s_mood_until != 0 && (int32_t)(millis() - s_mood_until) < 0;
        g.fillRect(4, 167, 312, 1, pulse ? (uint8_t)PAL_WHITE : term_rim(0));
    }
    {
        const int span = 46;
        int sy = 171 + (int)((millis() / 28u) % (uint32_t)span);
        if (sy > 223) sy = 223;
        g.fillRect(8, sy, 248, 1, th_wire());
    }
    g.fillRect(4, 235, 312, 1, term_rim(1));
    g.fillRect(4, 168, 1, 68, term_rim(0));
    g.fillRect(315, 168, 1, 68, term_rim(1));
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
    /* v1017c (#2): the prompt line shows exactly the typed characters, in order, and nothing else.
     * The muted inline completion that used to follow the text (cand_ghost) sat under the caret
     * (caret at text end + 2 px), so a sliver of the next ghost letter read as a stray glyph
     * ("sca" + half an 'n'). Completions stay in the candidate list beside the prompt. The caret
     * x comes from the typed length, not the panel cursor. */
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(COL_WHITE);
    g.setCursor(10 + 16 * 6, y);
    g.print(botcorps_t0_term_prompt);
    const int cx = 10 + 16 * 6 + (int)strlen(botcorps_t0_term_prompt) * 6 + 2;
    /* v1017c (#3): prompt focus = accent frame over the prompt row (the plate's bottom border closes
     * it) and a steady caret while there is text, as on the hub prompt strip. */
    const bool pf = s_term_prompt && !s_sheet && !s_focus_cmds && !s_cmd_btn;
    if (pf) {
        g.fillRect(6, 224, 242, 1, accent());
        g.fillRect(6, 224, 1, 11, accent());
        g.fillRect(247, 224, 1, 11, accent());
    }
    const bool caret = pf && botcorps_t0_term_prompt[0] ? true : (millis() / 530u) % 2u == 0;
    if (caret) g.fillRect(cx, y, 6, 8, accent());
    paint_cands(g);
    /* v1019 (Dirt #5): the empty companion block (no bot in it since v1017c) is gone, and the
     * commands pull-down (≡) sits at the FAR RIGHT end of the prompt row. The attack-prompt
     * button keeps its nav stop (one Down past ≡) and sits just left of it on the same row. */
    const bool po = (s_focus_cmds || s_cmd_btn) && !s_sheet;
    plate(g, TERM_CMD_X, TERM_BTN_Y, 20, 12, po ? COL_SEL : COL_DEEP, po ? accent() : COL_LINE);
    for (int k = 0; k < 3; ++k) g.fillRect(TERM_CMD_X + 5, TERM_BTN_Y + 3 + k * 3, 10, 1, po ? accent() : COL_MUTED);
    if (s_focus_cmds && !s_sheet) paint_palette(g);
    {   /* #10: attack-prompt button (target reticle), one stop below the commands button */
        const bool ao = (s_atk_btn || s_term_ovl == 1 || s_term_ovl == 2) && !s_sheet;
        const int ax = TERM_ATK_X, ay = TERM_BTN_Y;
        plate(g, ax, ay, 20, 12, ao ? COL_SEL : COL_DEEP, ao ? accent() : COL_LINE);
        const uint8_t ic = ao ? accent() : (uint8_t)COL_MUTED;
        g.drawCircle(ax + 10, ay + 6, 3, ic);
        g.fillRect(ax + 10, ay + 1, 1, 3, ic);
        g.fillRect(ax + 10, ay + 9, 1, 2, ic);
        g.fillRect(ax + 5, ay + 6, 3, 1, ic);
        g.fillRect(ax + 13, ay + 6, 3, 1, ic);
        g.fillRect(ax + 10, ay + 6, 1, 1, ao ? (uint8_t)PAL_WHITE : ic);
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

static void paint_gc_gloss(Gfx g);
static void paint_gc_attack(Gfx g);
/* v1019r (#7): Terminal `help` panel - the four command groups, every command, word-wrapped inside the
 * panel (nothing clipped), group label in the theme accent, commands pale, aliases muted. */
static void paint_term_help(Gfx g)
{
    const int x = 6, y = 19, w = 308, h = 146;
    GATE_BOX(g, "OCCLUDE", "help", x, y, w, h, "-");
    g.fillRect(x, y, w, h, PAL_BLACK);
    g.drawRect(x, y, w, h, accent());
    g.drawRect(x + 1, y + 1, w - 2, h - 2, th_wire());
    const int keep = s_px; s_px = 6;
    txt(g, "commands:", x + 6, y + 5, th_item(0, 4), lgfx::textdatum::top_left);
    txt(g, "esc closes", x + w - 6, y + 5, PAL_MUTED, lgfx::textdatum::top_right);
    const int cx0 = x + 6 + 6 * 6, cx1 = x + w - 6;      /* commands column: after a 6-char label */
    int ly = y + 18;
    const uint8_t pale = botcorps_pal_fit(0xBFBF);
    for (int gi = 0; gi < 4; ++gi) {
        txt(g, G_NAME[gi], x + 6, ly, th_item(gi, 4), lgfx::textdatum::top_left);
        int cx = cx0;
        for (int i = G_AT[gi]; i < G_AT[gi + 1]; ++i) {
            const int wpx = (int)strlen(CMDN[i]) * 6;
            if (cx > cx0 && cx + wpx > cx1) { cx = cx0; ly += 10; }   /* wrap at a word */
            const bool alias = strncmp(CMDP[i], "alias", 5) == 0 || strcmp(CMDN[i], "hunt run") == 0;
            txt(g, CMDN[i], cx, ly, alias ? (uint8_t)PAL_MUTED : pale, lgfx::textdatum::top_left);
            cx += wpx + 6;
        }
        ly += 13;
    }
    s_px = keep;
}
static void paint_terminal(Gfx g)
{
    paint_term_head(g);
    paint_boards(g);
    paint_prompt(g);
    if (s_sheet) paint_sheet(g);
    else if (s_term_ovl == 1) paint_gc_gloss(g);     /* #10: pulled up from the attack-prompt button */
    else if (s_term_ovl == 2) paint_gc_attack(g);
    else if (s_term_ovl == 3) paint_term_help(g);
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
 * band with two steel eyes. (v1019m: the round dome figure, hub_gibson, is gone; it was dead code.) */
/* v1019: the pre-v1019 block figure, kept ONLY for the centre radar die (scan_die), which Dirt
 * locked as-is. Every other chip uses face_block below. */
static void gib_block_radar(Gfx g, int cx, int cy, int s)
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

static uint16_t mix16(uint16_t a, uint16_t b, int num, int den);
/* v1019n (#16, Mobandy: "dont change anything but the pin amount"): EXACTLY 5 castellated pins per side on
 * every Gibson / bot chip. A chip size whose old pitch already gave 5 keeps its old pin positions to the
 * pixel (56 / 54 px blocks, the 20 px terminal tile); every other size spaces its 5 pins evenly, centred.
 * chip_pin(S, k) = offset of pin k (0..4) along a side for the 2 px pins of the chip_block family. */
static int chip_pin(int S, int k)
{
    const int n_old = S >= 18 ? (S - 18) / 8 + 1 : 0;   /* old loop: i = 8; i <= S - 10; i += 8 */
    if (n_old == 5) return 8 * (k + 1);
    int p = S / 7; if (p < 3) p = 3;
    return (S - 4 * p - 2) / 2 + p * k;
}
/* the same for the 1 px pins of chip_mini (die D): old loop i = 3; i <= D - 3; i += 3 */
static int chip_pin1(int D, int k)
{
    const int n_old = D >= 6 ? (D - 6) / 3 + 1 : 0;
    if (n_old == 5) return 3 * (k + 1);
    int p = D / 6; if (p < 2) p = 2;
    return (D - 4 * p - 1) / 2 + p * k;
}
/* v1019 (Dirt LOCKED #1/#2): THE chip face. Square face (Gibson ivory, every other bot its own
 * fixed BOT_PAL colour, never the theme), lit top row, dark chin.
 * v1019d (Dirt #1): every non-Gibson bot wears the plain control-panel eyes: exactly the eyes of the
 * control-panel COLOR-tile bot (botcorps_sprite SPR_MINI, 5x5 grid: two 1-cell PAL_EYE squares on
 * row 1, columns 1 and 3), scaled to the face. No glasses.
 * v1019d (Dirt #2): Gibson keeps square steel eyes raised to ~22% under his ORIGINAL slim shades:
 * two distinct, separated lenses with an open ivory gap between them (not one solid glass bar). */
static const uint8_t PAL_GOLD_PIN = 177;   /* sb_board gold board edge (182,146,85) */
/* v1019d: Gibson shades variant. 1 = A (original example: thin top rim across, two dark lenses
 * hanging from it, open gap between), 2 = B (open wire frames: 1 px rims, eyes visible inside).
 * Host renders may pick with GIB_GLASS=1|2 to compare; firmware uses the default. */
static int gib_glass_style(void)
{
#ifdef BOTCORPS_HOST_RENDER
    if (const char *e = getenv("GIB_GLASS")) { const int v = atoi(e); if (v == 1 || v == 2) return v; }
#endif
    return 1;
}
static void face_block(Gfx g, int cx, int cy, int s, uint8_t col, bool gib, bool blink)
{
    const int x = cx - s / 2, y = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    const uint8_t top = gib ? (uint8_t)PAL_WHITE
                            : botcorps_pal_fit(mix16(botcorps_rgb565(col), 0xFFFF, 45, 100));
    const uint8_t chin = gib ? (uint8_t)PAL_CHIN
                             : botcorps_pal_fit(mix16(botcorps_rgb565(col), 0x0000, 45, 100));
    g.fillRect(x, y, s, s, col);                      /* full square face */
    g.fillRect(x, y, s, lip, top);                    /* lit top row */
    g.fillRect(x, y + s - lip, s, lip, chin);         /* dark chin */
    if (!gib) {
        /* control-panel eyes: SPR_MINI grid (cell = s/5): margin, eye, gap, eye, margin; row 1 */
        const int c = s / 5 > 2 ? s / 5 : 2;
        const int m = (s - 3 * c) / 2;
        int ey = y + c;
        if (ey < y + lip) ey = y + lip;
        const int eh = blink ? 1 : c, eyy = blink ? ey + c - 1 : ey;
        g.fillRect(x + m, eyy, c, eh, PAL_EYE);
        g.fillRect(x + m + 2 * c, eyy, c, eh, PAL_EYE);
        return;
    }
    const int ew = s / 5 > 2 ? s / 5 : 2;
    const int gap = s / 4 > ew ? s / 4 : ew;
    const int eyc0 = y + (s * 22) / 100;              /* eyes raised into the upper third */
    const int eyc = eyc0 > y + lip + 1 ? eyc0 : y + lip + 1;
    const int lx = x + gap, rx = x + s - gap - ew;
    const int eh = blink ? 1 : ew, ey = blink ? eyc + ew - 1 : eyc;
    g.fillRect(lx, ey, ew, eh, PAL_STEEL);            /* steel eyes under the shades */
    g.fillRect(rx, ey, ew, eh, PAL_STEEL);
    /* lenses grow outward only, so the ivory gap between them (the nose) stays open */
    const int pad = s >= 24 ? 2 : 1;
    const int lw = ew + pad;
    const int l0 = lx - pad, r0 = rx;
    const int tl = s >= 24 ? 3 : 1;                   /* short temple arms */
    if (gib_glass_style() == 2) {
        /* B: open wire frames, 1 px black rims, the raised steel eyes show inside each lens */
        const int lh = ew + 2;
        const int ly = eyc - 1;
        g.drawRect(l0 - 1, ly, lw + 2, lh, PAL_BLACK);
        g.drawRect(r0 - 1, ly, lw + 2, lh, PAL_BLACK);
        g.fillRect(l0 - 1, ly + lh - 1, 1, 1, col);   /* rounded lower outer corners */
        g.fillRect(r0 + lw, ly + lh - 1, 1, 1, col);
        if (r0 - 1 > l0 + lw + 1) g.fillRect(l0 + lw + 1, ly + 1, (r0 - 1) - (l0 + lw + 1), 1, PAL_BLACK); /* bridge */
        if (l0 - 1 - tl >= x) g.fillRect(l0 - 1 - tl, ly, tl, 1, PAL_BLACK);
        if (r0 + lw + 1 + tl <= x + s) g.fillRect(r0 + lw + 1, ly, tl, 1, PAL_BLACK);
        return;
    }
    /* A: the original example-gibson-chip-glasses look: one thin top rim across the brow (bridge +
     * arms), two dark lenses hanging from it, open ivory notch between them, steel glint per lens */
    const int lh = ew * 3 / 4 + pad > 2 ? ew * 3 / 4 + pad : 2;
    const int ly = eyc + (ew - lh) / 2;
    const int rl = l0 - tl >= x ? l0 - tl : x, rr = r0 + lw + tl <= x + s ? r0 + lw + tl : x + s;
    g.fillRect(rl, ly - 1, rr - rl, 1, PAL_BLACK);    /* top rim, bridge and arms in one line */
    g.fillRect(l0, ly, lw, lh, PAL_BLACK);
    g.fillRect(r0, ly, lw, lh, PAL_BLACK);
    if (lh >= 4) {                                    /* rounded lower corners, all four */
        g.fillRect(l0, ly + lh - 1, 1, 1, col);
        g.fillRect(l0 + lw - 1, ly + lh - 1, 1, 1, col);
        g.fillRect(r0, ly + lh - 1, 1, 1, col);
        g.fillRect(r0 + lw - 1, ly + lh - 1, 1, 1, col);
    }
    if (lw >= 6) {                                    /* steel glint on each lens */
        g.fillRect(l0 + 1, ly + 1, 2, 1, PAL_STEEL);
        g.fillRect(r0 + 1, ly + 1, 2, 1, PAL_STEEL);
    }
}

static void gib_block(Gfx g, int cx, int cy, int s) { face_block(g, cx, cy, s, PAL_IVORY, true, false); }

/* v1019m: the square block figure everywhere (the intro ID board's dome path was dead code, removed). */
static void chip_block_ex(Gfx g, int cx, int cy, int S, int bot);
static void chip_block(Gfx g, int cx, int cy, int S, int bot) { chip_block_ex(g, cx, cy, S, bot); }
/* v1019: pre-v1019 chip, centre radar die only (Dirt: leave as-is). */
static void chip_block_radar(Gfx g, int cx, int cy, int S, int bot)
{
    const int x = cx - S / 2, y = cy - S / 2;
    GATE_BOX(g, "CHIP", bot >= 12 ? "gibson" : "bot", x, y, S, S, "-");
    g.fillRect(x + 3, y + 3, S - 6, S - 6, PAL_BG);     /* black face */
    g.drawRect(x - 1, y - 1, S + 2, S + 2, PAL_STEEL);   /* v1016f-4: silk outline reads on black PCB */
    g.drawRect(x, y, S, S, PAL_BLACK);                   /* black outer border */
    g.drawRect(x + 2, y + 2, S - 4, S - 4, PAL_LINE);
    g.drawRect(x + 6, y + 6, S - 12, S - 12, PAL_DIAG);
    for (int k = 0; k < 5; ++k) {                        /* steel pins, 5 per side (v1019n #16) */
        const int i = chip_pin(S, k);
        g.fillRect(x - 1, y + i, 5, 2, PAL_STEEL);
        g.fillRect(x + S - 4, y + i, 5, 2, PAL_STEEL);
        g.fillRect(x + i, y - 1, 2, 5, PAL_STEEL);
        g.fillRect(x + i, y + S - 4, 2, 5, PAL_STEEL);
    }
    g.fillRect(x + 7, y + 7, 2, 2, PAL_IVORY);
    if (bot >= 12) {
        const int gs = S - 12 > 8 ? S - 12 : 8;   /* square Gibson fills the chip face (v1016k: >=60% of the square) */
        GATE_BOX(g, "SPR", "gibson_block", cx - gs / 2, cy - gs / 2, gs, gs, "-");
        gib_block_radar(g, cx, cy, gs);
        return;
    }
    const int shape = botcorps_shape_for(bot);
    static const int bw[4] = { 7, 9, 7, 5 }, bh[4] = { 7, 6, 9, 5 };
    const int m = S - 14 > 6 ? S - 14 : 6;
    int sc = 1;
    while ((sc + 1) * bw[shape] <= m && (sc + 1) * bh[shape] <= m) ++sc;
    draw_blob(g, cx, cy, sc, bot_color(bot), shape, false);
}

/* v1019c: when non-zero, the Gibson-form chip face is drawn in this colour (game-board defender chip
 * recolour only: same frame, pins and shades). 0 = the normal ivory Gibson face. */
static uint8_t s_gib_face = 0;
static void chip_block_ex(Gfx g, int cx, int cy, int S, int bot)
{
    const int x = cx - S / 2, y = cy - S / 2;
    GATE_BOX(g, "CHIP", bot >= 12 ? "gibson" : "bot", x, y, S, S, "-");
    g.fillRect(x + 3, y + 3, S - 6, S - 6, PAL_BG);     /* black face */
    g.drawRect(x - 1, y - 1, S + 2, S + 2, PAL_STEEL);   /* v1016f-4: silk outline reads on black PCB */
    g.drawRect(x, y, S, S, PAL_BLACK);                   /* black outer border */
    g.drawRect(x + 2, y + 2, S - 4, S - 4, PAL_LINE);
    g.drawRect(x + 6, y + 6, S - 12, S - 12, PAL_DIAG);
    /* v1019 (Dirt LOCKED): GOLD pins on Gibson's chip only; every other bot SILVER. */
    const uint8_t pin = bot >= 12 ? PAL_GOLD_PIN : (uint8_t)PAL_SOFT;
    for (int k = 0; k < 5; ++k) {                        /* castellated pins, 5 per side (v1019n #16) */
        const int i = chip_pin(S, k);
        g.fillRect(x - 1, y + i, 5, 2, pin);
        g.fillRect(x + S - 4, y + i, 5, 2, pin);
        g.fillRect(x + i, y - 1, 2, 5, pin);
        g.fillRect(x + i, y + S - 4, 2, 5, pin);
    }
    g.fillRect(x + 7, y + 7, 2, 2, PAL_IVORY);
    /* v1019 (Dirt LOCKED S-18): slight even black window between the face and the square
     * (6 px / 56), face 38 px in the 56 px chip (S-18), scaled for smaller chips. */
    int wi = (S * 6 + 28) / 56;  if (wi < 3) wi = 3;
    int fi = (S * 9 + 28) / 56;  if (fi < wi + 1) fi = wi + 1;
    g.fillRect(x + wi, y + wi, S - 2 * wi, S - 2 * wi, PAL_BLACK);
    const int gs = S - 2 * fi > 8 ? S - 2 * fi : 8;
    if (bot >= 12) {
        GATE_BOX(g, "SPR", "gibson_block", cx - gs / 2, cy - gs / 2, gs, gs, "-");
        /* v1019d (Dirt #1): a recoloured (non-Gibson) defender chip wears the control-panel eyes */
        face_block(g, cx, cy, gs, s_gib_face ? s_gib_face : (uint8_t)PAL_IVORY, s_gib_face == 0, false);
        return;
    }
    GATE_BOX(g, "SPR", "bot_face", cx - gs / 2, cy - gs / 2, gs, gs, "-");
    face_block(g, cx, cy, gs, bot_color(bot), false, false);
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
    chip_block_radar(g, cx, cy, 56, bot);   /* v1019: centre radar die stays exactly as-is */
}

static uint16_t mix16(uint16_t a, uint16_t b, int num, int den);

/* v1017b (Dirt 8:20): the square chip-form block figure (gib_block geometry: full
 * square face, lit top row, dark bottom row, black visor band, two steel eyes)
 * drawn in a bot's fixed colour on a black square. Used by the terminal tiles. */
static void bot_block(Gfx g, int cx, int cy, int s, uint8_t col, bool blink)
{
    const int x = cx - s / 2, y = cy - s / 2;
    g.fillRect(x - 1, y - 1, s + 2, s + 2, PAL_BLACK);  /* black behind the block */
    GATE_BOX(g, "SPR", "bot_block", x, y, s, s, "-");
    /* v1019 (Dirt LOCKED #2): same face treatment as Gibson's chip (raised eyes, Morpheus A
     * gold-rim glasses), the bot's own fixed colour. */
    face_block(g, cx, cy, s, col, false, blink);
}

/* v1019e (Dirt): the matched chip look scaled down for the hub and Terminal bot tiles. Same parts as the
 * scan-board chip (chip_block_ex): steel silk outline, black die, square face in its own locked colour
 * (~68% of the die, as S-18 is 38/56), castellated pins SILVER (bots) or GOLD (Gibson). T = full box
 * including the 1 px pin overhang, so the chip never leaves the T x T square it is given. */
static void chip_mini(Gfx g, int cx, int cy, int T, int bot, bool blink, int face_dy)
{
    const int x = cx - T / 2, y = cy - T / 2;
    const int D = T - 2, dx = x + 1, dy = y + 1;          /* die inside the pin overhang */
    const bool gib = bot >= 12;
    const uint8_t pin = gib ? PAL_GOLD_PIN : (uint8_t)PAL_SOFT;
    /* logged as TCHIP (tile chip): H1's "centre is the rings core, not a CHIP" stays about the hub centre */
    GATE_BOX(g, "TCHIP", gib ? "gibson" : "bot", dx, dy, D, D, "-");
    g.fillRect(dx, dy, D, D, PAL_BLACK);                   /* black die / window */
    g.drawRect(dx, dy, D, D, PAL_STEEL);                   /* silk outline */
    for (int k = 0; k < 5; ++k) {                          /* castellated pins, 1 px, 5 per side (v1019n #16) */
        const int i = chip_pin1(D, k);
        g.fillRect(x, dy + i, 2, 1, pin);
        g.fillRect(x + T - 2, dy + i, 2, 1, pin);
        g.fillRect(dx + i, y, 1, 2, pin);
        g.fillRect(dx + i, y + T - 2, 1, 2, pin);
    }
    const int fs = D - 4;                                  /* 1 px outline + 1 px black window each side */
    GATE_BOX(g, "SPR", gib ? "gibson_block" : "bot_face", cx - fs / 2, cy - fs / 2 + face_dy, fs, fs, "-");
    face_block(g, cx, cy + face_dy, fs, gib ? (uint8_t)PAL_IVORY : bot_color(bot), gib, blink);
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
    const uint8_t spoke = th_wire();
    const uint8_t spoke_glow = th_wash();
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


static int list_band(void);
static void paint_sbox(Gfx g, bool wifi);

static void paint_radar(Gfx g, bool with_list)
{
    /* Device path for SCR_SCAN. */
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
static int s_gl_clip_y = -32768; /* #5 hunt: rows above this y are clipped (mock display clip) */
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
            if (y + r < s_gl_clip_y) continue;
            sb_rect(g, x + c, y + r, 1, 1, k);
        }
    }
}

#define LIST_N (s_sb_n)
/* v1016 1604 mock devices, strongest first. Cells, ids and MACs come from FNV-1a of the name, as in the mock. */
/* mod item 9: entries 12..34 are extra idle samples so the overflow (grid extends past 7x4) can be shown;
 * the stock list is still the first 12 (s_sb_n = 12). */
static const char *const LIST_WIFI_N[SB_N] = {
    "cafe-guest", "home-5g", "tp-link-mesh", "iphone-hotspot", "esp32-marauder", "ring-doorbell",
    "samsung-tv", "printer-2f", "macbook-air", "lobby-ap", "hp-laptop", "xfinitywifi",
    "netgear-24", "pixel-hotspot", "roku-ultra", "nest-cam-2", "epson-wf", "guest-iot", "linksys-ext",
    "galaxy-s24", "ipad-pro", "office-5g", "arlo-cam", "fire-tv", "dell-xps", "eero-mesh", "tesla-m3",
    "brother-ln", "chromecast", "starbucks", "wyze-cam", "surface-7", "lg-webos", "att-5g-x", "sonos-arc"
};
static const int8_t LIST_WIFI_DB[SB_N] = { -42, -49, -55, -57, -58, -63, -66, -67, -70, -71, -74, -78,
    -79, -79, -80, -80, -81, -81, -82, -82, -83, -83, -84, -84, -85, -85, -86, -86, -87, -87, -88, -88, -89, -89, -90 };
static const uint8_t LIST_WIFI_TY[SB_N] = {
    GL_AP, GL_AP, GL_AP, GL_PHONE, GL_DEVBOARD, GL_CAMERA, GL_TV, GL_PRINTER, GL_LAPTOP, GL_AP, GL_LAPTOP, GL_AP,
    GL_AP, GL_PHONE, GL_TV, GL_CAMERA, GL_PRINTER, GL_AP, GL_AP, GL_PHONE, GL_LAPTOP, GL_AP, GL_CAMERA, GL_TV,
    GL_LAPTOP, GL_AP, GL_CAR, GL_PRINTER, GL_TV, GL_AP, GL_CAMERA, GL_LAPTOP, GL_TV, GL_AP, GL_SPEAKER
};
static const uint8_t LIST_WIFI_DT[SB_N] = {
    DEV_AP, DEV_UNK, DEV_AP, DEV_AP, DEV_DEAUTH, DEV_CAM, DEV_TV, DEV_AP, DEV_UNK, DEV_AP, DEV_AP, DEV_AP,
    DEV_AP, DEV_AP, DEV_TV, DEV_CAM, DEV_AP, DEV_AP, DEV_AP, DEV_AP, DEV_UNK, DEV_AP, DEV_CAM, DEV_TV,
    DEV_UNK, DEV_AP, DEV_UNK, DEV_AP, DEV_TV, DEV_AP, DEV_CAM, DEV_UNK, DEV_TV, DEV_AP, DEV_UNK
};
static const char *const LIST_BLE_N[SB_N] = {
    "earbuds-58", "dji-mini", "jbl-flip", "watch-6", "hc-05-atm", "pixel-8",
    "meta-glasses", "tile-mate", "flock-raven", "uconnect-car", "speaker-7", "fitbit-ch",
    "airpods-p2", "galaxy-bud", "garmin-945", "bose-qc45", "airtag-3f", "oura-ring", "beats-flex",
    "mavic-3", "smarttag-2", "iphone-15", "kia-ble", "polar-h10", "echo-buds", "tile-pro",
    "pixel-buds", "whoop-4", "jabra-75", "nrf-beacon", "ford-sync", "sony-xm5", "esp32-ble", "chipolo", "mi-band-8"
};
static const int8_t LIST_BLE_DB[SB_N] = { -58, -60, -61, -63, -66, -68, -69, -71, -72, -73, -74, -76,
    -77, -77, -78, -78, -79, -79, -80, -80, -81, -81, -82, -82, -83, -83, -84, -84, -85, -85, -86, -86, -87, -87, -88 };
static const uint8_t LIST_BLE_TY[SB_N] = {
    GL_SPEAKER, GL_DRONE, GL_SPEAKER, GL_WATCH, GL_DEVBOARD, GL_PHONE, GL_CAMERA, GL_TRACKER, GL_CAMERA, GL_CAR, GL_SPEAKER, GL_WATCH,
    GL_SPEAKER, GL_SPEAKER, GL_WATCH, GL_SPEAKER, GL_TRACKER, GL_WATCH, GL_SPEAKER, GL_DRONE, GL_TRACKER, GL_PHONE, GL_CAR, GL_WATCH,
    GL_SPEAKER, GL_TRACKER, GL_SPEAKER, GL_WATCH, GL_SPEAKER, GL_DEVBOARD, GL_CAR, GL_SPEAKER, GL_DEVBOARD, GL_TRACKER, GL_WATCH
};
static const uint8_t LIST_BLE_DT[SB_N] = {
    DEV_BT, DEV_DRONE, DEV_BT, DEV_BT, DEV_SKIM, DEV_BT, DEV_CAM, DEV_TAG, DEV_FLOCK, DEV_BT, DEV_BT, DEV_BT,
    DEV_BT, DEV_BT, DEV_BT, DEV_BT, DEV_TAG, DEV_BT, DEV_BT, DEV_DRONE, DEV_TAG, DEV_BT, DEV_BT, DEV_BT,
    DEV_BT, DEV_TAG, DEV_BT, DEV_BT, DEV_BT, DEV_BT, DEV_BT, DEV_BT, DEV_SKIM, DEV_TAG, DEV_BT
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
    if (which < 0 || which > 4) which = 0;
    return theme().tone[which];
}

/* mock UI.ink: the accent mixed 45 % toward white (#6eeef6 on cyan) */
static uint8_t sb_ink(void)
{
    return theme().sbink;
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

/* v1019 (Dirt #4): Gibson voice / log ticker in the dead space under his chip on his scan board.
 * Three lines, newest at the bottom typing in; one new line every GT_STEP ms. */
static const int GT_X = 24, GT_Y = 154, GT_W = 71, GT_H = 33, GT_STEP = 1600;
static const char *const GIB_VOICE[] = {
    "BUS CLEAN", "12 BOARDS UP", "SCANNING AIR", "SHIELDS UP", "EYES ON LANES",
    "CORE TEMP OK", "DEFEND READY", "TYPE HELP"
};
static const int GIB_VOICE_N = (int)(sizeof(GIB_VOICE) / sizeof(GIB_VOICE[0]));
static uint32_t gib_ticker_key(uint32_t t)
{
    const uint32_t step = t / (uint32_t)GT_STEP, ph = t % (uint32_t)GT_STEP;
    const uint32_t rev = ph / 60u > 16u ? 16u : ph / 60u;
    return step * 64u + rev * 2u + (t / 530u) % 2u;
}
static void gib_ticker(Gfx g)
{
    const uint8_t ac = accent();
    sb_rect(g, GT_X, GT_Y, GT_W, GT_H, PAL_BLACK);
    sb_frame(g, GT_X, GT_Y, GT_W, GT_H, 41);
    GATE_BOX(g, "TICKER", "gibson", GT_X, GT_Y, GT_W, GT_H, "-");
    const uint32_t t = millis();
    const int step = (int)(t / (uint32_t)GT_STEP);
    const int rev = (int)((t % (uint32_t)GT_STEP) / 60u);
    for (int r = 0; r < 3; ++r) {
        const int idx = ((step - 2 + r) % GIB_VOICE_N + GIB_VOICE_N) % GIB_VOICE_N;
        const char *m = GIB_VOICE[idx];
        const int y = GT_Y + 5 + r * 8;
        const bool nw = r == 2;
        const uint8_t col = nw ? (uint8_t)PAL_IVORY : (uint8_t)41;
        /* '>' chevron (the etch font has none) */
        sb_rect(g, GT_X + 3, y, 1, 1, nw ? ac : (uint8_t)41);
        sb_rect(g, GT_X + 4, y + 1, 1, 1, nw ? ac : (uint8_t)41);
        sb_rect(g, GT_X + 5, y + 2, 1, 1, nw ? ac : (uint8_t)41);
        sb_rect(g, GT_X + 4, y + 3, 1, 1, nw ? ac : (uint8_t)41);
        sb_rect(g, GT_X + 3, y + 4, 1, 1, nw ? ac : (uint8_t)41);
        char b[18];
        snprintf(b, sizeof(b), "%s", m);
        int n = (int)strlen(b);
        if (nw && rev < n) { b[rev] = 0; n = rev; }
        if (n) tiny_text(g, b, GT_X + 8, y, col);
        if (nw && (t / 530u) % 2u == 0u) sb_rect(g, GT_X + 8 + n * 4, y, 3, 5, ac);   /* caret */
    }
}

static void sb_upper(char *dst, int n, const char *src, int maxc)
{
    int k = 0;
    for (const char *p = src; p && *p && k < n - 1 && k < maxc; ++p)
        dst[k++] = (*p >= 'a' && *p <= 'z') ? (char)(*p - 32) : *p;
    dst[k] = 0;
}

/* mod item 9 (Mobandy 2026-10-18): devices sit ON the grid in reading order (strongest first,
 * 7 per row), one device per cell, so arrows walk the grid predictably and every item is selectable.
 * (Was: FNV hash % 28 scatter, where the list-order cursor hopped around the board.) More devices than
 * the 7x4 window = the grid EXTENDS by rows and scrolls to keep the cursor row visible. */
static const int SG_COLS = 7, SG_VROWS = 4;
static int s_sb_row0 = 0;   /* first visible grid row */
static int sb_rows(int n) { return n <= 0 ? 1 : (n + SG_COLS - 1) / SG_COLS; }
static void sb_scroll_fix(int n)
{
    const int rows = sb_rows(n);
    const int cur = (s_cursor >= 0 && s_cursor < n) ? s_cursor / SG_COLS : -1;
    if (cur >= 0) {
        if (cur < s_sb_row0) s_sb_row0 = cur;
        if (cur >= s_sb_row0 + SG_VROWS) s_sb_row0 = cur - SG_VROWS + 1;
    } else if (s_cursor >= LIST_N) {
        s_sb_row0 = 0;      /* the > glossary prompt sits above the grid: show the top */
    }
    if (s_sb_row0 > rows - SG_VROWS) s_sb_row0 = rows - SG_VROWS;
    if (s_sb_row0 < 0) s_sb_row0 = 0;
}
static int sb_cell_id(int band, int d) { (void)band; return d; }
/* Cell centre of device d in the current scroll window. False = scrolled out of the window. */
static bool sb_cell(int band, int d, int *cx, int *cy)
{
    const int c = sb_cell_id(band, d);
    const int r = c / SG_COLS - s_sb_row0;
    *cx = SG_X + 11 + (c % SG_COLS) * SG_C;
    *cy = SG_Y + 11 + r * SG_C;
    return r >= 0 && r < SG_VROWS;
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
    const bool zsb = zap_sb_on();                 /* v1019x: Gibson Defend = this shell */
    const bool core = s_screen == SCR_SCAN || zsb;
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
    const int bot = zsb ? 12 : core ? s_scan_bot : wifi ? 0 : 1;
    if (core) {
        /* v1016f-4 (Dirt 8:19): drop the silver module frame; Gibson/bot is the
         * clean control-panel block with uncut labels. */
        const int cxm = mx + mw / 2, ccy = my + 42;
        const bool gib = bot >= 12;
        if (zsb) zap_rain_rect(g);   /* v1019x: rain behind the chip, this rect only */
        chip_block(g, cxm, ccy, 54, gib ? 12 : bot);
        char nmb[16];
        const char *nm;
        if (gib) nm = "GIBSON";
        else { sb_upper(nmb, 16, BOTS[bot].short_name, 12); if (strlen(nmb) <= 9) strcat(nmb, "BOT"); nm = nmb; }
        const char *l2c = "ESP32-WROOM-32";
        const char *l3c = (gib && !zsb) ? "CORE + PRIME" : "RADIO CORE";   /* v1019x: Defend = AIRTAGBOT-style silk */
        tiny_text(g, nm, cxm - tiny_w(nm) / 2, ccy + 32, 41);
        tiny_text(g, l2c, cxm - tiny_w(l2c) / 2, ccy + 42, 41);
        tiny_text(g, l3c, cxm - tiny_w(l3c) / 2, ccy + 50, 41);
        if (gib && !zsb) gib_ticker(g);
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
    /* mock: nShow = 1 + floor(u * 1.6 * 12). mod item 9: found devices STAY on the grid (the reveal
     * runs once from the scan start and holds; it no longer loops every 3.4 s and empties the board). */
    const uint32_t e = millis() - s_bus_t0;
    const float u = e > 3400u * 4u ? 4.f : (float)e / 3400.f;
    int n = 1 + (int)(u * 1.6f * 12.f);
    return n > LIST_N ? LIST_N : n;
}

/* mod item 9: the selection HALO. Tile 18x18 in a 22 px cell; the halo is a solid 2 px accent ring one
 * pixel off the tile, white L-brackets on its corners, a white tile rim and a lifted tile face. It stays
 * inside its own cell (1 px clear of the neighbours) so it never reads as a neighbour's. */
static void sb_halo(Gfx g, int x, int y, int w)
{
    const uint8_t ac = accent();
    sb_frame(g, x - 3, y - 3, w + 6, w + 6, sel_ink(0, 0));
    sb_frame(g, x - 2, y - 2, w + 4, w + 4, sel_ink(1, 2));
    const int L = 5, x0 = x - 4, y0 = y - 4, x1 = x + w + 3, y1 = y + w + 3;
    sb_rect(g, x0, y0, L, 1, PAL_WHITE); sb_rect(g, x0, y0, 1, L, PAL_WHITE);
    sb_rect(g, x1 - L + 1, y0, L, 1, PAL_WHITE); sb_rect(g, x1, y0, 1, L, PAL_WHITE);
    sb_rect(g, x0, y1, L, 1, PAL_WHITE); sb_rect(g, x0, y1 - L + 1, 1, L, PAL_WHITE);
    sb_rect(g, x1 - L + 1, y1, L, 1, PAL_WHITE); sb_rect(g, x1, y1 - L + 1, 1, L, PAL_WHITE);
    (void)ac;
}

static void sb_grid(Gfx g, int band, bool tiles)
{
    const uint8_t ac = accent();
    const int nall = LIST_N;
    sb_scroll_fix(nall);
    const int rows = sb_rows(nall);
    for (int c = 0; c <= 7; ++c) sb_rect(g, SG_X + c * SG_C, SG_Y, 1, 4 * SG_C + 1, 1);
    for (int r = 0; r <= 4; ++r) sb_rect(g, SG_X, SG_Y + r * SG_C, 7 * SG_C + 1, 1, 1);
    for (int c = 0; c <= 7; ++c)
        for (int r = 0; r <= 4; ++r) sb_rect(g, SG_X + c * SG_C - 1, SG_Y + r * SG_C - 1, 3, 3, 65);
    sb_rect(g, SG_X + 7 * SG_C + 1, SG_Y, 1, 4 * SG_C, sb_tone(3));
    /* extended grid: scroll rail right of the grid (thumb = visible rows, arrows = rows beyond the window) */
    if (rows > SG_VROWS) {
        const int rx = SG_X + 7 * SG_C + 5, ry = SG_Y + 7, rh = 4 * SG_C - 14;
        GATE_BOX(g, "RAIL", "scroll", rx - 1, SG_Y, 7, 4 * SG_C + 1, "MINI");
        sb_rect(g, rx + 1, ry, 3, rh, sb_tone(2));
        const int th = rh * SG_VROWS / rows, ty = ry + (rh - th) * s_sb_row0 / (rows - SG_VROWS);
        sb_rect(g, rx, ty, 5, th, ac);
        sb_rect(g, rx + 1, ty + 1, 3, 1, PAL_WHITE);
        const bool up = s_sb_row0 > 0, dn = s_sb_row0 + SG_VROWS < rows;
        for (int k = 0; k < 3; ++k) {
            sb_rect(g, rx + 2 - k, SG_Y + 1 + k, 1 + 2 * k, 1, up ? (uint8_t)PAL_WHITE : sb_tone(3));
            sb_rect(g, rx + 2 - k, SG_Y + 4 * SG_C - 1 - k, 1 + 2 * k, 1, dn ? (uint8_t)PAL_WHITE : sb_tone(3));
        }
        /* rows beyond the window: a dashed ghost edge on the grid where it continues */
        if (dn) for (int x = SG_X + 2; x < SG_X + 7 * SG_C; x += 4) sb_rect(g, x, SG_Y + 4 * SG_C, 2, 1, ac);
        if (up) for (int x = SG_X + 2; x < SG_X + 7 * SG_C; x += 4) sb_rect(g, x, SG_Y, 2, 1, ac);
    }
    if (!tiles) return;
    const int n = sb_shown(), k0 = n - 1;
    const uint32_t t = millis();
    int hx0 = -1, hy0 = -1;
    for (int d = 0; d < n; ++d) {
        if (s_dev_ign[band][d]) continue;
        int cx = 0, cy = 0;
        if (!sb_cell(band, d, &cx, &cy)) continue;
        const int x = cx - 9, y = cy - 9;
        const bool sel = d == s_cursor && s_act < 0;
        GATE_BOX(g, "TILE", "dev", x, y, 18, 18, "MINI");
        sb_rect(g, x, y, 18, 18, sel ? (uint8_t)PAL_WHITE : (uint8_t)149);
        sb_rect(g, x + 1, y + 1, 16, 16, sel ? sb_tone(3) : (uint8_t)5);
        const bool fresh = d == k0 && n < nall && (t / 110u) % 2u;
        s_gl_mono = fresh;
        gl_icon(g, sb_ty(band, d), x + 3, y + 3, false);
        s_gl_mono = false;
        if (s_dev_tag[band][d] > 0) sb_rect(g, x + 13, y + 1, 4, 4, TAG_COL[s_dev_tag[band][d] - 1]);
        if (sel) { hx0 = x; hy0 = y; }
    }
    if (hx0 >= 0) sb_halo(g, hx0, hy0, 18);   /* last, so no neighbour tile paints over it */
    /* scout: walks from the bottom-left node to the newest find while the sweep is still revealing;
     * once every device is on the grid it steps aside (it no longer sits on the tiles). */
    int nx = 0, ny = 0;
    if (n >= nall || !sb_cell(band, k0, &nx, &ny)) return;
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
    chip_mini(g, hx, hy - 1, 11, bot, false, 0);   /* v1019l: scan screen = hardware side, square chip bot */
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

/* ===== #5 Per-bot hunt mini game (Dirt locked 2026-10-16, EXACT port of mock cand-1538 botHunt;
 * re-confirmed 2026-10-17). On each non-scan bot's chip board only THAT bot's own device icons
 * fall down the code strands, and the bot (in its fixed colour) chases and catches the nearest;
 * catches show as corner pips, no text. v1018b: values ported 1:1 from the mock (same 320x240
 * canvas): non-live grid 9x5 cells of 19 px at (dx+2, dy+2); drops spawn at dt*1.2/s (max 4) just
 * above the grid and fall 14..24 px/s; the chaser walks 46 px/s; catch at < 6 px; the strand trail
 * fades (alpha .23/.18/.13/.08); sparkles 420 ms; pips start at 0. Gibson's board runs the hunt only
 * in hunt mode (Gibson prompt `hunt`, mock gibHuntStart); otherwise ivory Gibson leads the orbiting
 * crew through the rain (mock v16 orbit-leader). The mock integrates with Math.random()+real dt;
 * here the same mechanics run on a deterministic LCG that is idempotent per-millis, so a partial
 * push matches a full paint pixel-for-pixel (flicker gate S1/S2/S4) and the host renders repeat.
 * Icon geometry: the mock's 8 px rain icons are our 12 px glossary icons, so icon centre = +6. */
#define BH_MAX 4
/* device icon kind(s) each bot hunts, mock BOT_DEVS mapped to our glossary icons; -1 = unused. */
static const int8_t BH_DEV[13][2] = {
    { GL_AP, -1 },      /* 0 wifi   (never core) */
    { GL_SPEAKER, -1 }, /* 1 ble    (never core) */
    { GL_AP, -1 },      /* 2 deauth */
    { GL_LAPTOP, -1 },  /* 3 hack */
    { GL_CAMERA, -1 },  /* 4 flock */
    { GL_UNKNOWN, -1 }, /* 5 drone */
    { GL_TRACKER, -1 }, /* 6 airtag */
    { GL_CAR, -1 },     /* 7 keyfob */
    { GL_WATCH, -1 },   /* 8 meta */
    { GL_UNKNOWN, -1 }, /* 9 skimmer */
    { GL_TV, GL_CAR },  /* 10 flipper */
    { GL_TRACKER, -1 }, /* 11 samsungtag */
    { -2, -1 },         /* 12 gibson = mock RAIN_DEV (Dirt's ten) */
};
static const int8_t BH_GIB[10] = { GL_AP, GL_PHONE, GL_LAPTOP, GL_TV, GL_SPEAKER, GL_TRACKER, GL_WATCH, GL_CAMERA, GL_CAR, GL_UNKNOWN };
/* mock non-live grid: C = 19, GC = 9, GR = 5, gx = dx + 2, gy = dy + 2 (display dx 106, dy 42). */
static const int BH_C = 19, BH_GC = 9, BH_GR = 5;
static const int BH_X = 108, BH_Y = 44, BH_W = BH_GC * BH_C, BH_H = BH_GR * BH_C, BH_M = 8;
struct BHDrop { float x, y, v; int8_t k; uint8_t live; };
static int s_bh_who = -2;
static uint32_t s_bh_t = 0, s_bh_rng = 0;
static float s_bh_x = 0.f, s_bh_y = 0.f;
static int s_bh_score = 0;
static BHDrop s_bh_d[BH_MAX];
static float s_bh_fxx[4], s_bh_fxy[4];
static uint32_t s_bh_fxt[4];
static int s_bh_fxn = 0;

static int bh_kind(int bot, uint32_t r)
{
    if (bot == 12) return BH_GIB[r % 10u];
    const int8_t a = BH_DEV[bot][0], b = BH_DEV[bot][1];
    if (b >= 0) return (r & 1u) ? b : a;
    return a >= 0 ? a : GL_UNKNOWN;
}
static uint32_t bh_rng(void) { s_bh_rng = s_bh_rng * 1664525u + 1013904223u; return s_bh_rng; }
static float bh_rngf(void) { return (float)(bh_rng() >> 8) / 16777216.f; }
static bool bh_orbit(void) { return s_scan_bot == 12 && !s_gib_hunt; }

static void bh_spawn(BHDrop *d, int bot)
{
    d->live = 1;
    d->x = (float)(BH_X + BH_M + (int)(bh_rngf() * (float)(BH_W - 2 * BH_M - 12)));  /* mock floor(rand*(gw-2M-w)) */
    d->y = (float)(BH_Y - 12);                                                         /* mock gy - 10, above the grid */
    d->v = 14.f + bh_rngf() * 10.f;                                                    /* mock 14 + rand*10 px/s */
    d->k = (int8_t)bh_kind(bot, bh_rng());
}

static void bot_hunt_step(uint32_t t)
{
    const int bot = s_scan_bot;
    if (s_bh_who != bot) {               /* mock: hunt.who !== id -> empty board, score 0, chaser centred */
        s_bh_who = bot;
        s_bh_rng = 2166136261u ^ (uint32_t)(bot * 2654435761u);
        s_bh_t = t;
        s_bh_score = 0;
        s_bh_fxn = 0;
        s_bh_x = (float)BH_X + (float)BH_W / 2.f;
        s_bh_y = (float)BH_Y + (float)BH_H / 2.f;
        for (int i = 0; i < BH_MAX; ++i) s_bh_d[i].live = 0;
        return;
    }
    if (t == s_bh_t) return;                       /* idempotent within a frame */
    float dt = (float)(t - s_bh_t) / 1000.f;
    s_bh_t = t;
    if (dt > 0.1f) dt = 0.1f;                      /* mock Math.min(.1, dt) */
    if (bh_orbit()) return;                        /* mock: Gibson's board hunts only in hunt mode */
    int nlive = 0;
    for (int i = 0; i < BH_MAX; ++i) if (s_bh_d[i].live) ++nlive;
    if (nlive < BH_MAX && bh_rngf() < dt * 1.2f)
        for (int i = 0; i < BH_MAX; ++i) if (!s_bh_d[i].live) { bh_spawn(&s_bh_d[i], bot); break; }
    for (int i = 0; i < BH_MAX; ++i) {
        if (!s_bh_d[i].live) continue;
        s_bh_d[i].y += s_bh_d[i].v * dt;
        if (s_bh_d[i].y >= (float)(BH_Y + BH_H - BH_M - 10)) s_bh_d[i].live = 0;  /* mock: leaves before the labels */
    }
    int tg = -1;
    float bd = 1e9f;
    for (int i = 0; i < BH_MAX; ++i) {
        if (!s_bh_d[i].live) continue;
        const float ddx = s_bh_d[i].x + 6.f - s_bh_x, ddy = s_bh_d[i].y + 6.f - s_bh_y;
        const float q = sqrtf(ddx * ddx + ddy * ddy);
        if (q < bd) { bd = q; tg = i; }
    }
    if (tg >= 0) {
        const float ddx = s_bh_d[tg].x + 6.f - s_bh_x, ddy = s_bh_d[tg].y + 6.f - s_bh_y;
        const float h = sqrtf(ddx * ddx + ddy * ddy);
        const float L = h < 1.f ? 1.f : h;
        const float sp = 46.f * dt;                /* mock chaser 46 px/s */
        const float mv = sp < L ? sp : L;
        s_bh_x += ddx / L * mv;
        s_bh_y += ddy / L * mv;
        if (L < 6.f) {                             /* mock catch radius */
            if (s_bh_score < 9999) ++s_bh_score;
            if (s_bh_fxn < 4) { s_bh_fxx[s_bh_fxn] = s_bh_d[tg].x + 6.f; s_bh_fxy[s_bh_fxn] = s_bh_d[tg].y + 6.f; s_bh_fxt[s_bh_fxn] = t; ++s_bh_fxn; }
            s_bh_d[tg].live = 0;
        }
    }
    if (s_bh_x < (float)(BH_X + BH_M)) s_bh_x = (float)(BH_X + BH_M);
    if (s_bh_x > (float)(BH_X + BH_W - BH_M)) s_bh_x = (float)(BH_X + BH_W - BH_M);
    if (s_bh_y < (float)(BH_Y + BH_M)) s_bh_y = (float)(BH_Y + BH_M);
    if (s_bh_y > (float)(BH_Y + BH_H - BH_M)) s_bh_y = (float)(BH_Y + BH_H - BH_M);
    int w = 0;                                     /* retire expired catch sparkles (mock 420 ms) */
    for (int i = 0; i < s_bh_fxn; ++i)
        if ((uint32_t)(t - s_bh_fxt[i]) < 420u) { s_bh_fxx[w] = s_bh_fxx[i]; s_bh_fxy[w] = s_bh_fxy[i]; s_bh_fxt[w] = s_bh_fxt[i]; ++w; }
    s_bh_fxn = w;
}

/* colour at mock globalAlpha a (percent) over the display glass (index 8) */
static uint8_t bh_alpha(uint8_t col, int a_pct)
{
    return sb_mix(col, botcorps_rgb565(8), 100 - a_pct);
}

/* mock non-live grid: 9 x 5 cells of 19 px, teal lines + node dots, sweep bar on the right edge */
static void bh_grid(Gfx g)
{
    for (int c = 0; c <= BH_GC; ++c) sb_rect(g, BH_X + c * BH_C, BH_Y, 1, BH_H + 1, 1);
    for (int r = 0; r <= BH_GR; ++r) sb_rect(g, BH_X, BH_Y + r * BH_C, BH_W + 1, 1, 1);
    for (int c = 0; c <= BH_GC; ++c)
        for (int r = 0; r <= BH_GR; ++r) sb_rect(g, BH_X + c * BH_C - 1, BH_Y + r * BH_C - 1, 3, 3, 65);
    sb_rect(g, BH_X + BH_W + 1, BH_Y, 1, BH_H, sb_tone(3));
}

static void bh_sprite(Gfx g, int cx, int cy, int shape, int scale, uint8_t col)
{
    const int h = (9 * scale + 1) / 2;   /* half the largest sprite; the draw is centred (mock spriteC) */
    int x = cx, y = cy;      /* keep the figure inside the playfield (the mock clips at the display) */
    if (x - h < BH_X + 1) x = BH_X + 1 + h;
    if (x + h > BH_X + BH_W) x = BH_X + BH_W - h;
    if (y - h < BH_Y + 1) y = BH_Y + 1 + h;
    if (y + h > BH_Y + BH_H) y = BH_Y + BH_H - h;
#ifdef BOTCORPS_HOST_RENDER
    host_gate_walk = 1;
#endif
    botcorps_sprite_draw(*g.raw(), x - g.ox, y - g.oy, shape, scale, col, false);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_walk = 0;
#endif
}

/* mock v16 (Gibson board, hunt mode off): primary ivory Gibson leads through the rain; the first ten
 * colour bots trail his path (lag (i+1)*520 ms) and orbit it (o = t/700 + i*.63, r = 6+(i%3)*3). */
static void bh_orbit_paint(Gfx g, uint32_t t)
{
    const float cx = (float)BH_X + (float)BH_W / 2.f, cy = (float)BH_Y + (float)BH_H / 2.f;
    const float ax = (float)BH_W / 2.f - 16.f, ay = (float)BH_H / 2.f - 14.f;
    for (int i = 0; i < 10; ++i) {
        const float tt = (float)t - (float)((i + 1) * 520);
        const float px = cx + sinf((tt + 5200.f) / 2300.f) * ax, py = cy + sinf((tt + 5200.f) / 1700.f) * ay;
        const float o = (float)t / 700.f + (float)i * .63f, r = (float)(6 + (i % 3) * 3);
        const int a = 42 - (i * 5 + 1) / 2;       /* mock alpha .42 - i*.025 */
        bh_sprite(g, (int)lroundf(px + cosf(o) * r), (int)lroundf(py + sinf(o) * r), botcorps_shape_for(i), 1,
                  bh_alpha(bot_color(i), a));
    }
    const float gx0 = cx + sinf(((float)t + 5200.f) / 2300.f) * ax, gy0 = cy + sinf(((float)t + 5200.f) / 1700.f) * ay;
    bh_sprite(g, (int)lroundf(gx0), (int)lroundf(gy0), 0, 2, bh_alpha(PAL_IVORY, 62));   /* mock alpha .62 */
}

/* ===== mod item 10 (Mobandy 2026-10-18) DRAFT: GIBSON CREW DROP-DOWN =====
 * Gibson's scan board is not a plain list or the generic grid: Gibson leads the whole crew, so his
 * board is a stack of crew LANES (one per bot, its square chip in its fixed colour + name + finds),
 * and the lane under the cursor DROPS DOWN an ivory-rimmed panel holding that bot's finds as grid
 * tiles (same 18 px tiles + selection halo as the Wi-Fi/BLE grid). Up/Down walk lanes (the drop-down
 * follows), Right/OK drops into the tiles, Left/Up-from-top-row climbs back to the lane. Lane list and
 * tile panel both scroll (rail on the right). Hunt mode (prompt `hunt`) still runs the hunt game.
 * Flip back to the v16 orbit-leader board with -DBOTCORPS_GIB_DROPDOWN=0. */
#ifndef BOTCORPS_GIB_DROPDOWN
#define BOTCORPS_GIB_DROPDOWN 1
#endif
static int s_gd_lane = 0, s_gd_dev = -1, s_gd_top = 0, s_gd_row0 = 0;
static const int GD_X = 108, GD_Y = 44, GD_W = 154, GD_LH = 12, GD_LY = 57, GD_BOT = 142, GD_PR = 2;
static const uint8_t GD_CNT[12] = { 12, 12, 4, 3, 6, 2, 5, 3, 2, 1, 9, 4 };
static int gd_cnt(int b) { return (b >= 0 && b < 12) ? GD_CNT[b] : 0; }
static int gd_kind(int b, int i)
{
    if (b < 2) return sb_ty(b, i % 12);
    const int8_t a = BH_DEV[b][0], c = BH_DEV[b][1];
    return (c >= 0 && (i & 1)) ? c : (a >= 0 ? a : GL_UNKNOWN);
}
static int gd_prows(int b) { const int r = (gd_cnt(b) + 6) / 7; return r < 1 ? 1 : (r > GD_PR ? GD_PR : r); }
static int gd_panel_h(int b) { return gd_prows(b) * SG_C + 3; }
static void gd_fix(void)
{
    if (s_gd_lane < 0) s_gd_lane = 0;
    if (s_gd_lane > 11) s_gd_lane = 11;
    if (s_gd_lane < s_gd_top) s_gd_top = s_gd_lane;
    for (;;) {   /* the open lane + its panel must fit under the header */
        const int need = (s_gd_lane - s_gd_top) * (GD_LH + 1) + GD_LH + gd_panel_h(s_gd_lane);
        if (GD_LY + need <= GD_BOT || s_gd_top >= s_gd_lane) break;
        ++s_gd_top;
    }
    const int rows = (gd_cnt(s_gd_lane) + 6) / 7;
    if (s_gd_dev >= gd_cnt(s_gd_lane)) s_gd_dev = gd_cnt(s_gd_lane) - 1;
    if (s_gd_dev >= 0) {
        const int r = s_gd_dev / 7;
        if (r < s_gd_row0) s_gd_row0 = r;
        if (r >= s_gd_row0 + GD_PR) s_gd_row0 = r - GD_PR + 1;
    }
    if (s_gd_row0 > rows - GD_PR) s_gd_row0 = rows - GD_PR;
    if (s_gd_row0 < 0) s_gd_row0 = 0;
}
static int gd_total(void) { int n = 0; for (int b = 0; b < 12; ++b) n += gd_cnt(b); return n; }
/* lane-header selection: same halo language as the device tiles (2 px accent ring + white brackets) */
static void gd_halo_rect(Gfx g, int x, int y, int w, int h)
{
    sb_frame(g, x - 2, y - 2, w + 4, h + 4, sel_ink(0, 0));
    sb_frame(g, x - 1, y - 1, w + 2, h + 2, sel_ink(1, 2));
    const int L = 5, x0 = x - 3, y0 = y - 3, x1 = x + w + 2, y1 = y + h + 2;
    sb_rect(g, x0, y0, L, 1, PAL_WHITE); sb_rect(g, x0, y0, 1, L, PAL_WHITE);
    sb_rect(g, x1 - L + 1, y0, L, 1, PAL_WHITE); sb_rect(g, x1, y0, 1, L, PAL_WHITE);
    sb_rect(g, x0, y1, L, 1, PAL_WHITE); sb_rect(g, x0, y1 - L + 1, 1, L, PAL_WHITE);
    sb_rect(g, x1 - L + 1, y1, L, 1, PAL_WHITE); sb_rect(g, x1, y1 - L + 1, 1, L, PAL_WHITE);
}
static void gd_chev(Gfx g, int x, int y, bool open, uint8_t c)
{
    if (open) { sb_rect(g, x, y, 7, 1, c); sb_rect(g, x + 1, y + 1, 5, 1, c); sb_rect(g, x + 2, y + 2, 3, 1, c); sb_rect(g, x + 3, y + 3, 1, 1, c); }
    else { sb_rect(g, x + 1, y - 2, 1, 7, c); sb_rect(g, x + 2, y - 1, 1, 5, c); sb_rect(g, x + 3, y, 1, 3, c); sb_rect(g, x + 4, y + 1, 1, 1, c); }
}
static void gd_rail(Gfx g, int x, int y, int h, int first, int vis, int total)
{
    if (total <= vis) return;
    const uint8_t ac = accent();
    sb_rect(g, x + 1, y + 5, 3, h - 10, sb_tone(2));
    const int th = (h - 10) * vis / total, ty = y + 5 + (h - 10 - th) * first / (total - vis);
    sb_rect(g, x, ty, 5, th < 3 ? 3 : th, ac);
    const bool up = first > 0, dn = first + vis < total;
    for (int k = 0; k < 3; ++k) {
        sb_rect(g, x + 2 - k, y + k, 1 + 2 * k, 1, up ? (uint8_t)PAL_WHITE : sb_tone(3));
        sb_rect(g, x + 2 - k, y + h - 1 - k, 1 + 2 * k, 1, dn ? (uint8_t)PAL_WHITE : sb_tone(3));
    }
}
/* lane-size square-chip bot: black die, castellated pins, the bot's square face (fixed colour) */
static void gd_chip(Gfx g, int cx, int cy, int b)
{
    const bool gib = b >= 12;
    const int D = 11, x = cx - D / 2, y = cy - D / 2;
    const uint8_t pin = gib ? PAL_GOLD_PIN : (uint8_t)PAL_SOFT;
    for (int i = 1; i < D - 1; i += 2) {                   /* 1,3,5,7,9: 5 pins per side (v1019n #16) */
        sb_rect(g, x - 1, y + i, 1, 1, pin); sb_rect(g, x + D, y + i, 1, 1, pin);
        sb_rect(g, x + i, y - 1, 1, 1, pin); sb_rect(g, x + i, y + D, 1, 1, pin);
    }
    sb_rect(g, x, y, D, D, PAL_BLACK);
    face_block(g, cx, cy, 9, gib ? (uint8_t)PAL_IVORY : bot_color(b), gib, false);
}
static void gib_dropdown(Gfx g)
{
    gd_fix();
    sb_rect(g, GD_X - 1, GD_Y - 1, GD_W + 14, GD_BOT - GD_Y + 2, 8);   /* covers the hunt grid: own layout */
    const uint8_t ivory = PAL_IVORY;
    GATE_BOX(g, "GDROP", "gibson", GD_X, GD_Y, GD_W + 12, GD_BOT - GD_Y, "MINI");
    /* header: Gibson's own chip + crew total; ivory underline = Gibson's colour, not the theme */
    gd_chip(g, 133, 49, 12);
    tiny_text(g, "GIBSON", 142, 44, ivory);
    tiny_text(g, "CREW DROP-DOWN", 142, 50, COL_MUTED);
    char tot[16];
    snprintf(tot, sizeof tot, "%d FINDS", gd_total());
    tiny_text(g, tot, GD_X + GD_W - tiny_w(tot), 47, ivory);
    sb_rect(g, 126, 55, GD_X + GD_W - 126, 1, sb_mix(ivory, 0x0000, 55));
    int y = GD_LY, shown = 0;
    for (int b = s_gd_top; b < 12; ++b) {
        const bool open = b == s_gd_lane;
        const int need = GD_LH + (open ? gd_panel_h(b) : 0);
        if (y + need > GD_BOT + 1) break;
        const uint8_t bc = bot_color(b);
        /* lane header */
        sb_rect(g, GD_X, y, GD_W, GD_LH - 1, open ? sb_mix(bc, 0x0000, 72) : (uint8_t)5);
        sb_rect(g, GD_X, y, 3, GD_LH - 1, bc);
        gd_chip(g, GD_X + 10, y + 5, b);
        char nm[16];
        sb_upper(nm, 16, BOTS[b].short_name, 12);
        tiny_text(g, nm, GD_X + 18, y + 3, open ? (uint8_t)PAL_WHITE : (uint8_t)COL_INK);
        const int n = gd_cnt(b);
        if (!open) for (int k = 0; k < n && k < 9; ++k) sb_rect(g, GD_X + 78 + k * 5, y + 4, 3, 3, sb_mix(bc, 0x0000, 25));
        char cn[6];
        snprintf(cn, sizeof cn, "%d", n);
        tiny_text(g, cn, GD_X + GD_W - 16 - tiny_w(cn), y + 3, open ? (uint8_t)PAL_WHITE : bc);
        gd_chev(g, GD_X + GD_W - 10, y + 4, open, open ? ivory : (uint8_t)COL_MUTED);
        if (open && s_gd_dev < 0) gd_halo_rect(g, GD_X, y, GD_W, GD_LH - 1);
        y += GD_LH;
        if (open) {
            /* the drop-down: ivory rim hanging off the open lane, grid cells inside, finds as tiles */
            const int ph = gd_panel_h(b), pr = gd_prows(b);
            sb_rect(g, GD_X, y - 1, GD_W, ph, 8);
            sb_rect(g, GD_X, y - 1, 1, ph, ivory);
            sb_rect(g, GD_X + GD_W - 1, y - 1, 1, ph, ivory);
            sb_rect(g, GD_X, y + ph - 2, GD_W, 1, ivory);
            sb_rect(g, GD_X + 1, y - 1, 2, ph - 1, bc);   /* lane colour carries down the panel edge */
            for (int r = 0; r <= pr; ++r) sb_rect(g, GD_X + 3, y + r * SG_C, GD_W - 4, 1, 1);
            for (int c = 1; c < 7; ++c) sb_rect(g, GD_X + c * SG_C, y, 1, pr * SG_C, 1);
            int hx = -1, hy = -1;
            for (int i = s_gd_row0 * 7; i < n && i < (s_gd_row0 + pr) * 7; ++i) {
                const int cx = GD_X + 11 + (i % 7) * SG_C, cy = y + 11 + (i / 7 - s_gd_row0) * SG_C;
                const int tx = cx - 9, ty = cy - 9;
                const bool sel = i == s_gd_dev;
                GATE_BOX(g, "TILE", "dev", tx, ty, 18, 18, "MINI");
                sb_rect(g, tx, ty, 18, 18, sel ? (uint8_t)PAL_WHITE : sb_mix(bc, 0x0000, 35));
                sb_rect(g, tx + 1, ty + 1, 16, 16, sel ? sb_tone(3) : (uint8_t)5);
                gl_icon(g, gd_kind(b, i), tx + 3, ty + 3, false);
                if (sel) { hx = tx; hy = ty; }
            }
            if (hx >= 0) sb_halo(g, hx, hy, 18);
            const int rows = (n + 6) / 7;
            if (rows > pr) gd_rail(g, GD_X + GD_W - 7, y + 1, pr * SG_C - 2, s_gd_row0, pr, rows);
            y += ph;
        }
        y += 1;
        ++shown;
    }
    gd_rail(g, GD_X + GD_W + 4, GD_LY, GD_BOT - GD_LY, s_gd_top, shown, 12);
}
/* arrows on Gibson's board while the drop-down is up. True = handled. */
static bool gd_key(int key)
{
#if BOTCORPS_GIB_DROPDOWN
    if (!(s_screen == SCR_SCAN && s_scan_bot == 12 && !s_gib_hunt) || s_sp_buf[0]) return false;
    const int n = gd_cnt(s_gd_lane);
    if (s_gd_dev < 0) {
        if (key == 1) { s_gd_lane = (s_gd_lane + 11) % 12; s_gd_row0 = 0; }
        else if (key == 2) { s_gd_lane = (s_gd_lane + 1) % 12; s_gd_row0 = 0; }
        else if (key == 4 || key == 5) s_gd_dev = 0;
        else return key == 3;   /* Left on a lane: stays put */
    } else {
        int d = s_gd_dev;
        if (key == 3) d = d % 7 == 0 ? -1 : d - 1;
        else if (key == 4) d = d + 1 < n ? d + 1 : d;
        else if (key == 1) d = d - 7;
        else if (key == 2) d = d + 7 < n ? d + 7 : ((d / 7 < (n - 1) / 7) ? n - 1 : d);
        else return false;
        s_gd_dev = d < 0 ? -1 : d;
    }
    gd_fix();
    return true;
#else
    (void)key;
    return false;
#endif
}

static void bot_hunt(Gfx g)
{
    const uint32_t t = millis();
    bot_hunt_step(t);
#if BOTCORPS_GIB_DROPDOWN
    if (bh_orbit()) { gib_dropdown(g); return; }   /* mod item 10 draft */
#endif
    if (bh_orbit()) { bh_orbit_paint(g, t); return; }
    const int bot = s_scan_bot;
    const uint8_t col = bot == 12 ? (uint8_t)PAL_IVORY : bot_color(bot);
    uint8_t trail[5];
    for (int k = 1; k < 5; ++k) trail[k] = bh_alpha(col, 28 - k * 5);   /* mock fading strand: .28 - k*.05 */
    /* falling device icons + their fading strand trail (only this bot's own kinds) */
    s_gl_clip_y = BH_Y;
    for (int i = 0; i < BH_MAX; ++i) {
        if (!s_bh_d[i].live) continue;
        const int dx = (int)lroundf(s_bh_d[i].x), dy = (int)lroundf(s_bh_d[i].y);
        for (int k = 1; k < 5; ++k) {
            const int ty = dy - k * 4;
            if (ty >= BH_Y) sb_rect(g, dx + 5, ty, 2, 2, trail[k]);
        }
        gl_icon(g, s_bh_d[i].k, dx, dy, false);
    }
    s_gl_clip_y = -32768;
    /* catch sparkles (mock: radius 2..10, fading over 420 ms) */
    for (int i = 0; i < s_bh_fxn; ++i) {
        const float q = (float)(t - s_bh_fxt[i]) / 420.f;
        const int r = (int)lroundf(2.f + q * 8.f);
        const uint8_t fc = bh_alpha(col, (int)(100.f * (1.f - q)));
        const int fx = (int)lroundf(s_bh_fxx[i]), fy = (int)lroundf(s_bh_fxy[i]);
        const int off[8][2] = { { r, 0 }, { -r, 0 }, { 0, r }, { 0, -r }, { r, r }, { -r, -r }, { r, -r }, { -r, r } };
        for (int o = 0; o < 8; ++o) {
            const int px = fx + (int)lroundf(off[o][0] * 0.8f), py = fy + (int)lroundf(off[o][1] * 0.8f);
            if (px >= BH_X && px < BH_X + BH_W && py >= BH_Y && py < BH_Y + BH_H) sb_rect(g, px, py, 2, 2, fc);
        }
    }
    /* the chasing bot (walk-exempt sprite, like the scouts) */
    bh_sprite(g, (int)lroundf(s_bh_x), (int)lroundf(s_bh_y), bot == 12 ? 0 : botcorps_shape_for(bot), 2, col);
    /* catches as corner pips (top-right), no text; mock min(12, score) */
    const int np = s_bh_score < 12 ? s_bh_score : 12;
    for (int k = 0; k < np; ++k) sb_rect(g, BH_X + BH_W - 4 - k * 4, BH_Y + 3, 2, 2, col);
}

/* Faux transparency: the tiles under a panel show through as dim outlines. */
static void sb_ghost(Gfx g, int band, int px, int py, int pw, int ph)
{
    const int n = sb_shown();
    for (int d = 0; d < n; ++d) {
        if (s_dev_ign[band][d]) continue;
        int cx = 0, cy = 0;
        if (!sb_cell(band, d, &cx, &cy)) continue;
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
    chip_mini(g, 14, 12, 14, 12, false, 0);   /* v1019l (Dirt concept): outside games Gibson is a square chip */
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
        if (tally[i] >= 10) tiny_text(g, num, x + 13, 222, COL_INK);   /* mod item 9: 2-digit counts fit the 20 px slot */
        else sb_text(g, num, x + 13, 221, tally[i] ? (uint8_t)COL_INK : (uint8_t)73, PAL_BLACK, false, 1);
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
    const bool zsb = zap_sb_on();                 /* v1019x: Gibson Defend = this shell, zap in the grid */
    const bool core = s_screen == SCR_SCAN || zsb;
    if (core) {
        if (sb_vis(dx, dy, dw, 108)) {
            /* #5 (Dirt locked): the grid runs this bot's own hunt mini game (only its device icons
             * fall; the bot in its fixed colour chases/catches them, catches as pips). */
            if (zsb) zap_paint_grid(g);   /* v1019x: Gibson's zap Defend, inside the grid only */
            else { bh_grid(g); bot_hunt(g); }
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
    if (zsb) {
        zap_strip(g, dx, th, ty, dw);   /* v1019x: shell strip, zap log + typed zap command */
    } else if (s_sp_has || s_sp_buf[0]) {
        /* v1019: the strip is a live prompt once the user types or runs a command here */
        if (s_sp_has) {
            sb_tt(g, s_sp_l1, dx + 3, th, sb_ink());
            sb_tt(g, s_sp_l2, dx + 3, th + 11, PAL_OK);
        } else {
            int x = sb_tt(g, "[ ok ] ", dx + 3, th, PAL_OK);
            sb_tt(g, "type help", x, th, sb_ink());
        }
        int x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
        x = sb_tt(g, s_sp_buf, x, ty, COL_INK);
        if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
        if (core) {
            const bool lit = s_sp_cmd >= 0;
            sb_rect(g, dx + dw - 22, ty - 2, 19, 12, lit ? sb_tone(3) : (uint8_t)8);
            sb_frame(g, dx + dw - 22, ty - 2, 19, 12, lit ? ac : sb_tone(4));
            for (int k = 0; k < 3; ++k) sb_rect(g, dx + dw - 18, ty + 1 + k * 3, 11, 1, lit ? ac : COL_MUTED);
        }
    } else if (core) {
        char who[12];
        snprintf(who, sizeof(who), "%s", s_scan_bot >= 12 ? "gibson" : BOTS[s_scan_bot].short_name);
        int x = sb_tt(g, "[ ok ] ", dx + 3, th, PAL_OK);
        snprintf(l2, sizeof(l2), "%s core linked", who);
        sb_tt(g, l2, x, th + 1, sb_tone(3));
        sb_tt(g, l2, x, th, sb_ink());
        x = sb_tt(g, "[ ok ] ", dx + 3, th + 11, PAL_OK);
        snprintf(l2, sizeof(l2), "%s bus idle", who);
#if BOTCORPS_GIB_DROPDOWN
        if (bh_orbit()) {   /* mod item 10: the strip names the open lane */
            char ln[16];
            snprintf(ln, sizeof ln, "%s", BOTS[s_gd_lane].short_name);
            snprintf(l2, sizeof(l2), "%d finds \xfa %s open", gd_cnt(s_gd_lane), ln);
        }
#endif
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
    if (sb_rows(LIST_N) > SG_VROWS)   /* mod item 9: extended grid shows which rows are in view */
        snprintf(l2, sizeof(l2), "%s %d/%d rows %d-%d/%d", wifi ? "wifi" : "ble", sb_shown(), LIST_N,
                 s_sb_row0 + 1, s_sb_row0 + SG_VROWS, sb_rows(LIST_N));
    else
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
    if (!fa_live()) return;
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
    if (index == 0) snprintf(value, (size_t)n, "%s", theme().name);
    else if (index == 1) snprintf(value, (size_t)n, "%s", MIX_N[s_mix]);
    else if (index == 2) snprintf(value, (size_t)n, "%s", SHAPE_N[s_shape]);
    else if (index == 3) snprintf(value, (size_t)n, "%s", botcorps_rain_mode() == 0 ? "MIXED" : "SOLID");
    else if (index == 4) snprintf(value, (size_t)n, "%s", RAIN_WORD[botcorps_rain_mode()]);
    else if (index == 5) snprintf(value, (size_t)n, "%s", BGS_N[s_bg % BGS_NUM]);
    else if (index == 6) snprintf(value, (size_t)n, "%s", s_bg_motion ? "ON" : "OFF");
    else snprintf(value, (size_t)n, "%s", RAIN_WORD[botcorps_rain_mode()]);
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

/* v1019n (#6): BACKGROUND preview icon on U4 (RAIN bits / the four saver looks) */
static void bg_preview(Gfx g, int px, int cy)
{
    const int x0 = px - 6;
    const int y0 = cy - 6;
    const int mode = botcorps_rain_mode();
    const uint8_t ic = mode > 0 ? botcorps_rain_solid() : botcorps_rain_palette(0);
    if (s_bg == 0) {                 /* RAIN: three falling bit columns, bright leads */
        for (int k = 0; k < 3; ++k) {
            const int cx = x0 + 1 + k * 5, h0 = (k * 4) % 7;
            g.fillRect(cx, y0 + h0, 2, 6, botcorps_pal_fit((uint16_t)0x7E6C));   /* rain code green */
            g.fillRect(cx, y0 + h0 + 6, 2, 2, PAL_WHITE);
        }
    } else if (s_bg == 1) {          /* BOTS: falling bots + the ivory Gibson chip */
        static const int dot[5][2] = { { 1, 1 }, { 9, 2 }, { 3, 8 }, { 7, 0 }, { 10, 8 } };
        for (int k = 0; k < 5; ++k) g.fillRect(x0 + dot[k][0], y0 + dot[k][1], 2, 3, ic);
        g.fillRect(x0 + 5, y0 + 4, 4, 4, PAL_IVORY);
    } else if (s_bg == 2) {          /* GIBSON: the board grid + chips */
        for (int k = 0; k <= 12; k += 4) {
            g.fillRect(x0 + k, y0, 1, 13, PAL_DEEP);
            g.fillRect(x0, y0 + k, 13, 1, PAL_DEEP);
        }
        g.fillRect(x0 + 2, y0 + 2, 4, 4, PAL_IVORY);
        g.fillRect(x0 + 8, y0 + 8, 4, 4, ic);
    } else if (s_bg == 3) {          /* FLYBY: bots growing at the viewer */
        g.fillRect(x0 + 5, y0 + 5, 1, 1, PAL_LINE);
        g.fillRect(x0 + 1, y0 + 1, 2, 2, ic);
        g.fillRect(x0 + 4, y0 + 4, 3, 3, ic);
        g.fillRect(x0 + 8, y0 + 8, 5, 5, ic);
    } else {                         /* BOUNCE: frame + one bot */
        g.fillRect(x0, y0, 13, 1, PAL_LINE);
        g.fillRect(x0, y0 + 12, 13, 1, PAL_LINE);
        g.fillRect(x0, y0, 1, 13, PAL_LINE);
        g.fillRect(x0 + 12, y0, 1, 13, PAL_LINE);
        g.fillRect(x0 + 2, y0 + 7, 3, 3, ic);
        g.fillRect(x0 + 6, y0 + 3, 4, 4, ic);
    }
}

/* v1019n-mod (#7): BG COLOR index (0..4) of the live rain mode; -1 when the CP part picked BLUE / WHITE */
static int bgc_ix(void)
{
    const int m = botcorps_rain_mode();
    for (int i = 0; i < BGC_N; ++i) if (BGC_MODE[i] == m) return i;
    return -1;
}

/* v1016k (SPEC-LOCKED S-*): settings board = the master mock extControl, ported 1:1.
 * Board is a permanent MATTE-BLACK ESP32 shield (black PCB, white silk, gold header pins)
 * that never follows the UI colour; only the selection outline + feed trace use the accent.
 * v1019n-mod: five square ESP32 packages U1..U5 carry UI COLOR / BG MIX / BOT SHAPE / BACKGROUND
 * (the former screen savers) / BG COLOR (the former IDLE-time chip); the switch bank in the lower bay
 * carries the rest on ONE row, one centre line (y 163), labels on one baseline (y 178):
 *   U7 qfn "CP" = RAIN COLOUR (idx4), J1 jumper = RAIN MODE (idx3), PWR LED = BG MOTION on/off (idx6).
 * All eight settings keep their index, nudge_row edits and NVS values (keyboard only). */
static uint8_t wash22(uint8_t accent_idx);
static const int SB_BX = 8, SB_BY = 20, SB_BW = 304, SB_BH = 216; /* mock (3,18,314,220), inset 5 px so rain frames it */
static const int CP_BAY_CY = 163;   /* switch-bank centre line */
static const int CP_BAY_LY = 178;   /* switch-bank silk baseline */
static bool cp_part_rect(int index, int *x, int *y, int *w, int *h)
{
    switch (index) {
    case 0: *x = 38;  *y = 44;  *w = 40; *h = 72; return true; /* U1 UI COLOR    */
    case 1: *x = 82;  *y = 44;  *w = 40; *h = 72; return true; /* U2 BG MIX      */
    case 2: *x = 126; *y = 44;  *w = 40; *h = 72; return true; /* U3 BOT SHAPE   */
    case 5: *x = 170; *y = 44;  *w = 40; *h = 72; return true; /* U4 BACKGROUND  */
    case 7: *x = 214; *y = 44;  *w = 40; *h = 72; return true; /* U5 BG COLOR    */
    case 4: *x = 147; *y = CP_BAY_CY - 11; *w = 22; *h = 22; return true; /* U7 qfn CP = RAIN COLOUR */
    case 3: *x = 180; *y = CP_BAY_CY - 5;  *w = 20; *h = 10; return true; /* J1 jumper = RAIN MODE   */
    case 6: *x = 214; *y = CP_BAY_CY - 4;  *w = 10; *h = 8;  return true; /* PWR LED   = BG MOTION   */
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
    static const char *BGSS[5] = { "RAIN", "BOTS", "GIBSON", "FLYBY", "BOUNCE" };
    if (index == 0) snprintf(v, (size_t)n, "%s", theme().sname);
    else if (index == 1) snprintf(v, (size_t)n, "%s", MIXS[s_mix % 3]);
    else if (index == 2) snprintf(v, (size_t)n, "%s", SHPS[s_shape % 5]);
    else if (index == 5) snprintf(v, (size_t)n, "%s", BGSS[s_bg % BGS_NUM]);
    else if (index == 7) { const int i = bgc_ix(); snprintf(v, (size_t)n, "%s", i >= 0 ? BGC_S[i] : RAIN_WORD[botcorps_rain_mode()]); }
    else cp_text(index, v, n);
}

/* route from the USB-C feed (296,187) along the board bus (y 128), mock sbRoute. v1019n-mod: every part sits
 * inside the dev-kit, so every feed is the same shape: up the x 286 riser, along the bus, straight down / up. */
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
    px[n] = 286; py[n++] = 128;
    px[n] = cx;  py[n++] = 128;
    px[n] = cx;  py[n++] = cy;
    return n;
}

/* v1019: control-panel selection outline in the theme's selection style (solid sel colour; duotone
 * halves; gradient start on top/left). Same pixels as drawRect(ac) on a solid accent theme.
 * v1019n-mod: fades (grad 2/3) run top -> bottom through th_ink_at; textures (grad 4/5) walk the
 * perimeter so the bands / stutter dashes go round the part. */
static void sel_rect(Gfx g, int x, int y, int w, int h)
{
    const ui_theme_t &t = theme();
    if (t.grad == 0) { g.drawRect(x, y, w, h, t.sel); return; }
    if (t.grad == 1) {                       /* duotone: left half tone A, right half tone B */
        const int hw = w / 2;
        g.fillRect(x, y, hw, 1, t.ac);          g.fillRect(x + hw, y, w - hw, 1, t.ac2);
        g.fillRect(x, y + h - 1, hw, 1, t.ac);  g.fillRect(x + hw, y + h - 1, w - hw, 1, t.ac2);
        g.fillRect(x, y, 1, h, t.ac);           g.fillRect(x + w - 1, y, 1, h, t.ac2);
        return;
    }
    if (t.grad == 2 || t.grad == 3) {        /* fade: top row = start, bottom row = end */
        g.fillRect(x, y, w, 1, th_ink_at(0, h));
        g.fillRect(x, y + h - 1, w, 1, th_ink_at(h - 1, h));
        for (int r = 1; r < h - 1; ++r) {
            const uint8_t c = th_ink_at(r, h);
            g.fillRect(x, y + r, 1, 1, c);
            g.fillRect(x + w - 1, y + r, 1, 1, c);
        }
        return;
    }
    const int per = 2 * (w + h);             /* texture: walk the perimeter clockwise from top-left */
    for (int i = 0; i < w; ++i) g.fillRect(x + i, y, 1, 1, th_ink_at(i, per));
    for (int i = 0; i < h; ++i) g.fillRect(x + w - 1, y + i, 1, 1, th_ink_at(w + i, per));
    for (int i = 0; i < w; ++i) g.fillRect(x + w - 1 - i, y + h - 1, 1, 1, th_ink_at(w + h + i, per));
    for (int i = 0; i < h; ++i) g.fillRect(x, y + h - 1 - i, 1, 1, th_ink_at(2 * w + h + i, per));
    if (t.grad == 5) {                       /* glitch: a 1 px magenta split offset on the top + left edge */
        for (int i = 2; i < w - 2; i += 5) g.fillRect(x + i, y + 1, 2, 1, t.ac2);
        for (int i = 2; i < h - 2; i += 5) g.fillRect(x + 1, y + i, 1, 2, t.ac2);
    }
}

static void cp_seg(Gfx g, int x0, int y0, int x1, int y1, int wdt, uint8_t c, bool log_it)
{
    const int x = x0 < x1 ? x0 : x1, y = y0 < y1 ? y0 : y1;
    const int w = (x0 < x1 ? x1 - x0 : x0 - x1) + wdt, h = (y0 < y1 ? y1 - y0 : y0 - y1) + wdt;
    g.fillRect(x, y, w, h, c);
    if (log_it) GATE_BOX(g, "TRACE", "sel", x, y, w, h, "-");
}

/* v1019n-mod (#5): the UI COLOR preview bot in the theme's own style (solid / duo halves / fade rows /
 * prism bands / glitch split), filling the chip face. Shared by the chip and the colour swatch strip. */
static void ui_color_bot(Gfx g, int mx, int my, int fx, int fy, int fw, int fh, int scale)
{
    const ui_theme_t &th = theme();
    const uint8_t ac = th.ac;
    if (th.grad == 5) {                      /* glitch: magenta copy 1 px left, cyan on top, torn bands */
        botcorps_sprite_draw(*g.raw(), mx - 1 - g.ox, my - g.oy, 3, scale, th.ac2, false);
        for (int r = 0; r < fh; r += 3) {
            if ((r / 3) % 3 == 1) continue;  /* a torn band shows only the split copy */
            g.setClipRect(fx, fy + r, fw, 3);
            botcorps_sprite_draw(*g.raw(), mx + ((r / 3) % 4 == 2 ? 1 : 0) - g.ox, my - g.oy, 3, scale, ac, false);
        }
        g.setClipRect(fx, fy, fw, fh);
        return;
    }
    botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, scale, ac, false);
    if (th.grad == 1) {                      /* v1019 duotone: right half of the bot in the second tone */
        g.setClipRect(mx, fy, fx + fw - mx, fh);
        botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, scale, th.ac2, false);
        g.setClipRect(fx, fy, fw, fh);
    } else if (th.grad >= 2) {               /* fades + prism: per-2-row ink through th_ink_at */
        for (int r = 0; r < fh; r += 2) {
            g.setClipRect(fx, fy + r, fw, 2);
            botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, scale, th_ink_at(r, fh), false);
        }
        g.setClipRect(fx, fy, fw, fh);
    }
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
        sel_rect(g, x - 3, y - 3, w + 6, h + 6);
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
        /* UI colour: a SQUARE bot in the selected theme filling the face (mock botSquare) */
        ui_color_bot(g, mx, my, fx, fy, fw, fh, 4);
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
        bg_preview(g, mx, my);
    } else if (index == 7) {
        /* v1019n-mod (#7) BG COLOR: MULTI = three bots in their own colours; a solid = one bot in that ink */
        if (mode <= 0) {
            static const int BX[3] = { -10, 0, 10 };
            for (int k = 0; k < 3; ++k)
                botcorps_sprite_draw(*g.raw(), mx + BX[k] - g.ox, my + (k == 1 ? -5 : 5) - g.oy, 3, 2,
                                     bot_color(k * 4 + 1), false);
        } else {
            botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, 3, botcorps_rain_solid(), false);
        }
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
    const uint8_t metal = sh_rgb(0xb9, 0xbf, 0xcc), silk = sh_rgb(0xe9, 0xe9, 0xee), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    GATE_BOX(g, "PART", index == 3 ? "s3" : index == 4 ? "s4" : "s6", x, y, w, h, "BOARD");
    if (on) {
        sel_rect(g, x - 3, y - 3, w + 6, h + 6);
        GATE_BOX(g, "SEL", "outline", x - 3, y - 3, w + 6, h + 6, "-");
    }
    const char *lab = "PWR";
    if (index == 3) {                              /* J1 = RAIN MODE: 3-pin header + yellow jumper */
        const bool solid = botcorps_rain_mode() > 0;
        g.fillRect(x, y + 2, w, 6, rail);
        for (int k = 0; k < 3; ++k) g.fillRect(x + 3 + k * 6, y + 4, 2, 2, gold);
        const int jx = solid ? x + 9 : x + 1;
        g.fillRect(jx, y, 11, 10, sh_rgb(0xff, 0xd4, 0x00));
        g.fillRect(jx + 2, y + 2, 7, 3, sh_rgb(0xb8, 0x94, 0x00));
        lab = "JUMP";
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
        lab = "RAIN";
    } else if (index == 6) {                       /* PWR LED = BG MOTION on/off */
        const bool pon = s_bg_motion != 0;
        g.fillRect(x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
        g.fillRect(x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
    }
    /* one silk baseline under the whole switch bank, centred on each part */
    txt(g, lab, x + w / 2, CP_BAY_LY, on ? (uint8_t)PAL_WHITE : silk, lgfx::textdatum::top_center);
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
    if (theme().grad == 0) g.fillRect(0, 16, 320, 1, ac);
    else for (int x = 0; x < 320; x += 4) g.fillRect(x, 16, 4, 1, th_ink_at(x, 320));   /* v1019n-mod: themed rule */
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
    /* dev-kit outline + pin dot rows */
    g.drawRect(34, 40, 227, 165, sh_rgb(0x2b, 0x2b, 0x33));
    for (int k = 0; k < 19; ++k) {
        for (int r = 0; r < 2; ++r) {
            const int y0 = r ? 198 : 134;
            g.fillRect(40 + k * 11, y0, 3, 3, sh_rgb(0x5a, 0x5a, 0x66));
            g.fillRect(41 + k * 11, y0 + 1, 1, 1, PAL_BLACK);
        }
    }
    /* v1019n-mod (#8) lower bay, ONE row on the y 163 centre line, silk on the y 178 baseline:
     *   C1 tantalum | R1..R6 SMD array | [switch bank: CP  JUMP  PWR] | BOOT / EN tact column (labels right) */
    const int cy = CP_BAY_CY;
    g.fillRect(44, cy - 6, 22, 12, sh_rgb(0xe0, 0xa2, 0x1c)); g.fillRect(44, cy - 6, 5, 12, sh_rgb(0x7a, 0x52, 0x08));
    g.fillRect(51, cy - 3, 12, 6, sh_rgb(0x5a, 0x3a, 0x00));
    txt(g, "C1", 55, CP_BAY_LY, silk, lgfx::textdatum::top_center);
    for (int k = 0; k < 6; ++k) { g.fillRect(74 + k * 11, cy - 1, 6, 3, sh_rgb(0xb8, 0xb8, 0xc0)); g.fillRect(76 + k * 11, cy - 1, 2, 3, sh_rgb(0x20, 0x20, 0x24)); }
    txt(g, "R1-6", 104, CP_BAY_LY, silk2, lgfx::textdatum::top_center);
    {   /* switch-bank silk frame: one box around CP / JUMP / PWR so the three read as one bank */
        const uint8_t fr = sh_rgb(0x2b, 0x2b, 0x33);
        g.drawRect(140, cy - 17, 92, 44, fr);
    }
    g.fillRect(238, 156, 12, 22, metal); g.fillRect(241, 159, 6, 16, sh_rgb(0x1a, 0x1a, 0x20));   /* data micro-USB */
    for (int b = 0; b < 2; ++b) {                  /* BOOT / EN: one column, both labels on the right */
        const int x = 236, y = b ? 182 : 136, w = 16, h = 14;
        g.fillRect(x, y, w, h, metal); g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xc3, 0xc9, 0xd6));
        g.fillCircle(x + w / 2, y + h / 2, 4, sh_rgb(0x22, 0x23, 0x2c));
        g.fillRect(x - 2, y + 1, 2, 3, gold); g.fillRect(x + w, y + 1, 2, 3, gold);
        g.fillRect(x - 2, y + h - 4, 2, 3, gold); g.fillRect(x + w, y + h - 4, 2, 3, gold);
        txt(g, b ? "EN" : "BOOT", x + w + 4, y + 4, silk, lgfx::textdatum::top_left);
    }
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
    {   /* USB-C (mock 298,172 17x30; kept inside the board edge), silk centred under it */
        const int x = 293, y = 172, w = 17, h = 30;
        g.fillRect(x, y, w, h, sh_rgb(0x9a, 0xa1, 0xb2)); g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xd0, 0xd5, 0xdf));
        g.fillRect(x + 4, y + 4, w - 8, h - 8, sh_rgb(0x0a, 0x0a, 0x10)); g.fillRect(x + 7, y + 8, 2, h - 16, gold);
        txt(g, "USB5V", 312, 208, silk, lgfx::textdatum::top_right);
    }
    /* dark feed traces for every part, then the selected feed in the theme with moving pulses */
    int px[6], py[6];
    for (int i = 0; i < CP_N; ++i) {
        const int n = cp_route(i, px, py);
        for (int q = 0; q + 1 < n; ++q) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 1, trace, false);
    }
    if (s_cursor >= 0 && s_cursor < CP_N) {
        const int n = cp_route(s_cursor, px, py);
        int len = 0;
        for (int q = 0; q + 1 < n; ++q) len += abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
        int run = 0;
        for (int q = 0; q + 1 < n; ++q) {
            const int l2 = abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
            if (theme().grad == 0) {
                cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 2, th_wire(), true);
            } else {                               /* v1019n-mod: the feed fades / bands along its length */
                GATE_BOX(g, "TRACE", "sel", px[q] < px[q + 1] ? px[q] : px[q + 1], py[q] < py[q + 1] ? py[q] : py[q + 1],
                         abs(px[q] - px[q + 1]) + 2, abs(py[q] - py[q + 1]) + 2, "-");
                for (int d = 0; d < l2; d += 2) {
                    const int qx = px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0);
                    const int qy = py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0);
                    const int sw = (px[q] != px[q + 1]) ? 2 : 2;
                    g.fillRect(qx, qy, sw, 2, th_wire_at(run + d, len));
                }
                g.fillRect(px[q + 1], py[q + 1], 2, 2, th_wire_at(run + l2, len));
            }
            run += l2;
        }
        for (int k = 0; k < 3 && len > 0; ++k) {
            int d = (int)(((millis() * 120u) / 1000u + (uint32_t)(k * len / 3)) % (uint32_t)len);
            const int dd = d;
            for (int q = 0; q + 1 < n; ++q) {
                const int l2 = abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
                if (d <= l2) {
                    const int qx = px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0);
                    const int qy = py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0);
                    g.fillRect(qx - 1, qy - 1, 4, 4, theme().grad == 0 ? ac : th_ink_at(dd, len));
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

/* v1019g (Dirt: sharper hub/intro core): only the hub + intro ring disc use px_ring / core_bezel.
 * Solid ring = clean 1 px midpoint circle; dashed rings keep the same dash count / phase walk but
 * round to the nearest pixel (the old truncation skewed every ring half a pixel up-left = fuzzy). */
static void px_ring(Gfx g, int cx, int cy, int r, uint8_t col, int on, int off, int phase)
{
    (void)off;
    if (on <= 0) { g.drawCircle(cx, cy, r, col); return; }
    const int n = (int)((float)r * 6.3f);
    if (n < 1) return;
    for (int k = 0; k < n; ++k) {
        if (((k / on + phase) % 2) != 0) continue;
        const float a = (float)k / (float)r;
        const int x = cx + (int)lroundf(cosf(a) * (float)r);
        const int y = cy + (int)lroundf(sinf(a) * (float)r);
        g.fillRect(x, y, 1, 1, col);
    }
}

/* Crisp core bezel: black keyline just outside the solid rim so the core reads as one defined circle. */
static void core_bezel(Gfx g, int cx, int cy, int r)
{
    g.drawCircle(cx, cy, r + 1, PAL_BLACK);
    px_ring(g, cx, cy, r, PAL_CYAN, 0, 0, 0);
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
    return th_wire();
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
    (void)accent_idx;   /* v1019: the wash is the active theme's own slot */
    return th_wash();
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

/* v1019q (#9): the hub prompt's command list - Gibson's own bot commands first, then screen routing and
 * the shared ones; two-word forms only where clearer ("scan wifi" / "scan ble"); "chat" (wired, never
 * listed) is now listed. */
static const char *const GP_CMD[] = {
    "hunt", "defend", "scan", "scan wifi", "scan ble", "wifi", "ble", "bots", "chat",
    "ping", "info", "track", "sniff", "filter", "rain",
    "terminal", "control", "go", "load", "help", "clear"
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

/* v1019i (Dirt 2026-10-17, EXPLORATION ONLY): two candidate Gibson cores for the hub centre and the
 * intro rain ring disc, selectable at build time. BOTCORPS_GIB_CORE 0 (default) = the locked v1019g
 * core exactly as before (nothing below is compiled in). 1 = Option A: higher-resolution ROUND Gibson
 * (pixel-level dome/jaw, rounded lenses, 1 px keyline) inside clean continuous-arc rings. 2 = Option B:
 * Gibson's SQUARE chip (black window, gold pins) sitting inside the same round ring. 3 = Option C: the
 * current blocky Gibson with a half-round top (square base + semicircular crown).
 * v1019k (Dirt 2026-10-18, LOCKED): Option E is now the DEFAULT. Flag 0 (no flag) renders exactly what
 * BOTCORPS_GIB_CORE=5 rendered: Gibson's square chip (chip_block_ex) in the centre with the pulsing
 * px_ring lines walked on square paths (px_sq_ring), hub + intro ring disc. Flags 1-5 unchanged.
 * v1019m (cleanup): flags 1-3 (A round-dome HD face, B chip in round arcs, C half-round dome) were
 * compiled out of every device build and are removed with the round dome figure; 0/4/5 remain. */
#ifndef BOTCORPS_GIB_CORE
#define BOTCORPS_GIB_CORE 0
#endif
#if BOTCORPS_GIB_CORE >= 1 && BOTCORPS_GIB_CORE <= 3
#error "BOTCORPS_GIB_CORE 1-3 (round-dome Options A/C, chip-in-round-ring B) were removed in v1019m; use 0 (= E), 4 (D) or 5 (E)"
#endif
#define GIB_CORE_IS_E (BOTCORPS_GIB_CORE == 0 || BOTCORPS_GIB_CORE == 5)
/* v1019i Option D (BOTCORPS_GIB_CORE=4, Dirt 2026-10-18 strict one-change): the default v1019g core
 * with ONLY the centre figure swapped for Gibson's existing square scan-board chip (chip_block_ex,
 * bot 12: black window, gold pins, square frame, Gibson's face). Die, bezel, rings, sparks, floor
 * line, sizes, colours and motion are the flag-0 code paths, untouched. Option E (=5) is D plus the
 * px_ring dotted rings walked on SQUARE paths (same half-size r, same dash/phase/colour/motion). */
static void gib_core_chip_d(Gfx g, int cx, int cy)
{
    chip_block_ex(g, cx, cy, 44, 12);           /* 44 px: the whole square, pins + silk, inside the r34 rim */
}
#if GIB_CORE_IS_E
/* px_ring on a square: same contract (on<=0 = solid 1 px outline; else 1 px dots, `on` px on / on
 * px off along the path, shifted by `phase`), walked clockwise from (r,0) round the 8r px square. */
static void px_sq_ring(Gfx g, int cx, int cy, int r, uint8_t col, int on, int off, int phase)
{
    (void)off;
    if (r < 1) return;
    if (on <= 0) { g.drawRect(cx - r, cy - r, 2 * r + 1, 2 * r + 1, col); return; }
    const int n = 8 * r;
    for (int k = 0; k < n; ++k) {
        if (((k / on + phase) % 2) != 0) continue;
        int x, y;
        if (k < r)              { x = r;             y = k; }
        else if (k < 3 * r)     { x = r - (k - r);   y = r; }
        else if (k < 5 * r)     { x = -r;            y = r - (k - 3 * r); }
        else if (k < 7 * r)     { x = -r + (k - 5 * r); y = -r; }
        else                    { x = r;             y = -r + (k - 7 * r); }
        g.fillRect(cx + x, cy + y, 1, 1, col);
    }
}
#define GIB_CORE_RING px_sq_ring
#else
#define GIB_CORE_RING px_ring
#endif

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
            g.fillRect(x1, ys, len, 1, th_item(i, 12));
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
        chip_plate(g, sx, sy, 28, 20, PAL_DEEP, th_item(i, 12));
        g.drawRect(sx - 1, sy - 1, 30, 22, th_item(i, 12));   /* v1019r (#7): multi themes run along the tiles */
        /* v1019 (Dirt LOCKED #2): home tiles carry the same square chip face as Gibson's chip
         * (black window, raised eyes, Morpheus A gold-rim glasses), each bot in its OWN fixed
         * BOT_PAL colour - never the theme. */
        /* v1019e (Dirt): full matched chip (silk outline, black window, silver pins) fitted to the
         * 28x20 tile: 18 px box at x+2..19, y+1..18, face 12 px; status dot keeps x+22. */
        chip_mini(g, sx + 11, sy + 10, 18, i, false, 0);
        g.fillRect(sx + 22, sy + 3, 4, 4, dot_col(i, t, mood, asleep));
    }
    /* v1016k (Dirt 9:31): hub MAIN screen is the animated moving rings core,
     * not a chip. Square chip_block applies only to Gibson inside boards. */
    GATE_BOX(g, "HUBCORE", "rings", cx - 54, cy - 54, 108, 108, "-");
    hub_die(g, cx, cy, 34);
    core_bezel(g, cx, cy, 34);
    const int spin = mood == MOOD_HUNTING ? (int)(t / 30u) : mood == MOOD_EXCITED ? (int)(t / 50u) : (int)(t / 90u);
    GIB_CORE_RING(g, cx, cy, 37, th_ring(0), 5, 5, spin);
    if (mood == MOOD_HUNTING) {
        for (int k = 0; k < 14; ++k) {
            const float ang = (float)t / 260.f - (float)k * 0.07f;
            const uint8_t ink = k < 3 ? (uint8_t)PAL_WHITE : ac;
            g.fillRect(cx + (int)(cosf(ang) * 40.f), cy + (int)(sinf(ang) * 40.f), 1, 1, ink);
        }
    }
    GIB_CORE_RING(g, cx, cy, 42, th_ring(1), 2, 4, (int)(t / 140u));
    GIB_CORE_RING(g, cx, cy, 48, th_ring(2), 1, 5, (int)(t / 80u));
    GIB_CORE_RING(g, cx, cy, 54, th_ring(4), 2, 6, (int)(t / 200u));
    const int ph = (int)((t % 2400u) * 5u / 2400u);
    GIB_CORE_RING(g, cx, cy, 44 + ph, th_ring(3), 3, 3, 0);
    hub_sparks(g, cx, cy, t);
    g.fillRect(cx - 14, cy + 25, 28, 1, wire);
    gib_core_chip_d(g, cx, cy);   /* D/E: Gibson's square chip replaces the z7 figure */
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
    core_bezel(g, cx, cy, 34);
    GIB_CORE_RING(g, cx, cy, 37, PAL_CYAN, 5, 5, (int)(t / 90u));
    GIB_CORE_RING(g, cx, cy, 42, PAL_CYAN, 2, 4, (int)(t / 140u));
    GIB_CORE_RING(g, cx, cy, 48, PAL_CYAN, 1, 5, (int)(t / 80u));
    GIB_CORE_RING(g, cx, cy, 54, PAL_CYAN, 2, 6, (int)(t / 200u));
    g.fillRect(cx - 14, cy + 25, 28, 1, wire_ink());
    gib_core_chip_d(g, cx, cy);   /* D/E: Gibson's square chip replaces the z9 figure */
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
    const size_t aw = strcspn(arg, " ");   /* v1019: first word names the bot */
    if (aw == 0) return -1;
    if ((aw == 6 && strncmp(arg, "gibson", 6) == 0) || (aw == 3 && strncmp(arg, "gib", 3) == 0)) return 12;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(BOTS[i].short_name, arg, aw) == 0) return i;
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
    /* v1019q (#9b): two-word form "jack in" = jackin */
    if (strcmp(cmd, "jack") == 0 && arg && strcmp(arg, "in") == 0) { cmd = (char *)"jackin"; arg = 0; }
    /* #9/#10: the Defend board's viewable device glossary and the per-bot attack-prompt menu. */
    if (strcmp(cmd, "gloss") == 0 || strcmp(cmd, "glossary") == 0) { s_gc_panel = 1; gc_push(12, "device glossary"); return; }
    if (strcmp(cmd, "attack") == 0 || strcmp(cmd, "prompts") == 0) { s_gc_panel = 2; gc_push(12, "attack prompts"); return; }
    if (strcmp(cmd, "defend") == 0 && arg && arg[0]) {
        const int b = gc_bot_of(arg);
        if (b >= -1) { s_defend_bot = b < 0 ? 12 : b; s_gc_panel = 0; gc_push(12, "defend set"); return; }
    }
    bool start_def = false;
    if (strcmp(cmd, "load") == 0 && arg && strcmp(arg, "game") == 0) start_def = true;   /* v1019: CMD hint "screen|game" */
    if (strcmp(cmd, "defend") == 0) {
        /* v1019 (Dirt #6): "defend" / "defend beginner|veteran" starts the game (was "jackin to enter") */
        if (arg && arg[0] && strcmp(arg, "beginner") != 0 && strcmp(arg, "veteran") != 0) {
            gc_push(12, "defend <bot>|beginner|veteran");
            return;
        }
        start_def = true;
    }
    s_gc_panel = 0;
    char echo[40];
    snprintf(echo, sizeof(echo), "gibson> %.16s%s%.10s", cmd, (arg && arg[0]) ? " " : "", arg ? arg : "");
    log_add(echo);
    /* v1019p (field fix, items 1/14): the GAME commands all reach a real game. jackin plays (it used to only
     * print "jacking you in" and leave the board idle); lobby lists all 12 games; play <1-12> starts any of
     * them on this board; score shows the best run of each game of this bot's category. */
    if (strcmp(cmd, "jackin") == 0) start_def = true;
    if (strcmp(cmd, "lobby") == 0) { dg_lobby(); return; }
    /* v1019r (#10): Gibson's own games + the shared Device Dex */
    if (strcmp(cmd, "night") == 0 || strcmp(cmd, "nightshift") == 0 || (strcmp(cmd, "night") == 0 && arg)) {
        gc_push(12, "night shift - clock in"); s_defend_bot = 12; dg_start_game(12, 12); return;
    }
    if (strcmp(cmd, "sweep") == 0) { gc_push(12, "sweep - eat the bits"); s_defend_bot = 12; dg_start_game(12, 13); return; }
    if (strcmp(cmd, "zap") == 0) { gc_push(12, "hackers zap - board armed"); s_defend_bot = 12; dg_start_game(12, 14); return; }  /* DG_ZAP */
    if (strcmp(cmd, "dex") == 0) { dex_open(); return; }
    if (strcmp(cmd, "score") == 0) { dg_scores(); return; }
    if (strcmp(cmd, "play") == 0) {
        if (!arg || !arg[0] || !dg_play_n(atoi(arg))) gc_push(12, "play <1-12> - see lobby");
        return;
    }
    if (strcmp(cmd, "jackin") == 0 || start_def) {
        gc_push(12, "jacking you in");
        gc_push(3, "shields up");
        gc_push(5, "eyes on the lanes");
        s_act = 2;
        s_mood = MOOD_ALERT;
        s_mood_until = millis() + 2400u;
        s_bus_t0 = millis();
        /* v1019j: defend / load game start the category game; v1019r (#5): jackin = the bot's OWN game */
        if (start_def) {
            if (strcmp(cmd, "jackin") == 0) dg_start_own(s_defend_bot);
            else if (s_defend_bot >= 12) dg_start_game(12, 14); /* v1019v DG_ZAP */
            else dg_start(s_defend_bot);
        }
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
            const char *st = bot == 12 ? "crown lit" : BOTS[bot].st == 2 ? "hold" : BOTS[bot].st == 1 ? "limited" : "ready";
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

__attribute__((unused)) static void paint_gc_route(Gfx g, uint8_t ac)
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
    /* v1019c (Dirt): the centre defender chip keeps its exact shape (frame, pins, shades); only its
     * face takes the active bot's own colour. Gibson's own game is unchanged (ivory). */
    if (s_defend_bot >= 0 && s_defend_bot < 12) s_gib_face = bot_color(s_defend_bot);
    chip_block(g, x + 92, y + 54, 28, 12); /* v1016f-4 (Dirt 8:31): Gibson CPU = square block */
    s_gib_face = 0;
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

/* #9/#11: build this game's dynamic title, "<Name>> Defend" (Gibson's is "Gibson> Defend"). */
static void defend_title(char *out, int cap, int bot)
{
    char nm[16];
    const char *src = (bot >= 12 || bot < 0) ? "gibson" : BOTS[bot].id;
    snprintf(nm, sizeof(nm), "%s", src);
    if (nm[0] >= 'a' && nm[0] <= 'z') nm[0] = (char)(nm[0] - 'a' + 'A');
    snprintf(out, cap, "%s> Defend", nm);
}

/* #9: viewable device glossary - every device icon matched to its label. Reuses the scan-box
 * glossary set (GL_LABEL / gl_icon). Drawn as an opaque overlay on the Defend board. */
static void paint_gc_gloss(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    const int bx = 24, by = 24, bw = 272, bh = 196;
    GATE_BOX(g, "OCCLUDE", "gcgloss", bx, by, bw, bh, "-");
    g.fillRect(bx, by, bw, bh, PAL_BLACK);
    g.drawRect(bx, by, bw, bh, ac);
    g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, PAL_WIRE_C);
    g.fillRect(bx + 2, by + 2, bw - 4, 12, PAL_DEEP);
    const int keep = s_px; s_px = 6;
    txt(g, "DEVICE GLOSSARY", bx + 8, by + 4, ac, lgfx::textdatum::top_left);
    char cnt[6]; snprintf(cnt, sizeof(cnt), "%d", GL_N);
    txt(g, cnt, bx + bw - 24, by + 4, PAL_WHITE, lgfx::textdatum::top_left);
    const int cw = (bw - 8) / 2;
    for (int i = 0; i < GL_N; ++i) {
        const int cx = bx + 4 + (i / 7) * cw, cy = by + 18 + (i % 7) * 25;
        g.fillRect(cx, cy, 20, 20, PAL_BG);
        g.drawRect(cx, cy, 20, 20, PAL_WIRE_C);
        gl_icon(g, i, cx + 4, cy + 4, false);
        txt(g, GL_LABEL[i], cx + 26, cy + 7, PAL_IVORY, lgfx::textdatum::top_left);
    }
    s_px = keep;
}

/* #10: per-bot/device attack-prompt menu (pulled up from the Terminal command-prompt button). Each
 * row names a bot, its device target and the Defend category it feeds. The 4 categories are
 * MOCK-FIRST stubs (Dirt), wired to the dynamic "<Name>> Defend" title; gameplay is not built yet. */
static const uint8_t ATK_CAT[12] = { 2, 2, 2, 2, 1, 1, 0, 3, 2, 3, 3, 0 };
static const char *const ATK_CAT_NM[4] = { "TRIANGULATE", "SIGHTLINES", "FIREWALL", "ROLLING CODE" };
static void paint_gc_attack(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    const int bx = 10, by = 20, bw = 300, bh = 208;
    GATE_BOX(g, "OCCLUDE", "gcattack", bx, by, bw, bh, "-");
    g.fillRect(bx, by, bw, bh, PAL_BLACK);
    g.drawRect(bx, by, bw, bh, ac);
    g.fillRect(bx + 2, by + 2, bw - 4, 12, PAL_DEEP);
    const int keep = s_px; s_px = 6;
    txt(g, "ATTACK PROMPTS", bx + 8, by + 4, ac, lgfx::textdatum::top_left);
    txt(g, "mock-first", bx + bw - 64, by + 4, PAL_MUTED, lgfx::textdatum::top_left);
    for (int i = 0; i < 12; ++i) {
        const int col = i / 6, row = i % 6;
        const int cx = bx + 6 + col * 148, cy = by + 18 + row * 31;
        g.fillRect(cx, cy + 2, 10, 10, bot_color(i));
        char nm[16]; snprintf(nm, sizeof(nm), "%s", BOTS[i].id);
        if (nm[0] >= 'a' && nm[0] <= 'z') nm[0] = (char)(nm[0] - 'a' + 'A');
        txt(g, nm, cx + 16, cy, PAL_IVORY, lgfx::textdatum::top_left);
        char tg[18]; snprintf(tg, sizeof(tg), "%s", BOTS[i].target);
        txt(g, tg, cx + 16, cy + 10, PAL_MUTED, lgfx::textdatum::top_left);
        txt(g, ATK_CAT_NM[ATK_CAT[i]], cx + 16, cy + 20, ac, lgfx::textdatum::top_left);
    }
    s_px = keep;
}

static void paint_gchat(Gfx g)
{
    /* v1019x (Mobandy photo order): Gibson's Defend is the restored pre-mod per-bot scan screen
     * (paint_sbox, as on 5608764); the zap plays only inside its right-hand grid. */
    if (dg_zap_full()) {
        const int keep = s_px;
        paint_sbox(g, true);
        if (s_sheet) paint_gc_sheet(g);
        if (s_gc_panel == 1) paint_gc_gloss(g);
        else if (s_gc_panel == 2) paint_gc_attack(g);
        else if (s_gc_panel == 3) paint_dex(g);
        s_px = keep;
        return;
    }
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
    /* MOD item 1: the Defend game no longer takes over the screen. The Defend screen stays (title plate,
     * the bot's square chip, rail, prompt panel); the game plays inside its grid board and its UI sits in
     * the prompt panel. */
    {   /* #9/#11: dynamic "<Name>> Defend" title for the active Defend game.
         * v1018b (Dirt-approved): solid matte plate behind the title so it never reads over the
         * rain glyphs. Sized to the title; a name too long for the 4..105 slot left of the rail
         * (x 108) wraps at the space onto a second plate line ("Samsungtagbot>" / "Defend"). */
        const int kt = s_px; s_px = 6;
        char dtitle[32];
        defend_title(dtitle, sizeof(dtitle), s_defend_bot);
        const int len = (int)strlen(dtitle);
        const int maxc = 16;                       /* 16 x 6 px + padding stays left of the rail */
        const char *sp = strrchr(dtitle, ' ');
        const bool wrap = len > maxc && sp != nullptr;
        char l1[32];
        const int n1 = wrap ? (int)(sp - dtitle) : len;
        snprintf(l1, sizeof(l1), "%.*s", n1, dtitle);
        const char *l2 = wrap ? sp + 1 : "";
        const int n2 = (int)strlen(l2);
        const int cols = n1 > n2 ? n1 : n2;
        const int px = 4, py = 19;
        const int pw = cols * 6 + 6;
        const int ph = wrap ? 25 : 15;
        plate(g, px, py, pw, ph, PAL_DEEP, PAL_WIRE_C);
        txt(g, l1, px + 3, 22, PAL_CYAN, lgfx::textdatum::top_left);
        if (wrap) txt(g, l2, px + 3, 32, PAL_CYAN, lgfx::textdatum::top_left);
        g.fillRect(px + 3, py + ph - 4, (wrap ? n2 : n1) * 6 - 1, 1, PAL_WIRE_C);
        s_px = kt;
    }
    /* v1019b (Dirt): each bot's Defend game shows THAT bot's chip (own colour, silver pins, Morpheus A)
     * in Gibson's spot; Gibson's own game keeps his gold-pin chip. The chat-corner speaker chip and the
     * route board CPU stay Gibson. */
    /* v1019q (item 7): Gibson's chip a modest step bigger in his space (36 -> 44 px, same centre); the
     * device-icon rain stays visible all round it (cover matches, layout_covers). Other bots' chips and the
     * in-game board keep 36 (see gib_def_chip). */
    chip_block(g, 56, 78, gib_def_chip(), (s_defend_bot >= 0 && s_defend_bot < 12) ? s_defend_bot : 12);
    paint_gc_rail(g, ac);
    if (dg_on()) paint_dg_board(g);
    else paint_dg_demo(g, (s_defend_bot >= 0 && s_defend_bot < 12) ? s_defend_bot : 12);   /* v1019r (#5): own game */
    g.fillRect(4, 148, 312, 88, PAL_BLACK);
    g.drawRect(4, 148, 312, 88, ac);
    g.drawRect(5, 149, 310, 86, PAL_WIRE_C);
    g.fillRect(6, 150, 250, 1, (uint32_t)(t - s_bus_t0) < 160u ? ac : (uint8_t)PAL_WIRE_C);
    chip_block(g, 294, 214, 28, 12);
    if (dg_on()) {                /* MOD item 14: the game's HUD in the prompt panel */
        paint_dg_hud_ext(g);
        s_px = keep;
        return;
    }
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
    if (!s_gp_buf[0]) paint_dg_demo_hint(g, (s_defend_bot >= 0 && s_defend_bot < 12) ? s_defend_bot : 12);   /* v1019r (#5) */
    const int gl = (int)strlen(s_gp_buf);
    const int ci = (s_gc_caret < 0 || s_gc_caret > gl) ? gl : s_gc_caret;   /* MOD item 2: caret can sit mid-line */
    const int cx = 10 + (9 + ci) * 6;
    if ((t / 530u) % 2u == 0u) {
        g.fillRect(cx, py, 5, 8, ac);
        if (ci < gl) { const char one[2] = { s_gp_buf[ci], 0 }; txt(g, one, cx, py, PAL_BLACK, lgfx::textdatum::top_left); }
    }
    if (s_sheet) paint_gc_sheet(g);
    if (s_gc_panel == 1) paint_gc_gloss(g);
    else if (s_gc_panel == 2) paint_gc_attack(g);
    else if (s_gc_panel == 3) paint_dex(g);
    s_px = keep;
}

/* v1017c (#4/#6): rain glyph flags per screen. Gibson's screens (hub, Gibson's scan board) mix in
 * ivory Gibsons; the Terminal's rain has no bot emblems. */
static void rain_flags(void)
{
    /* v1019x: Gibson's Defend (scan shell) rain = BOTcorps canon on Gibson's screen: bits, ivory
     * Gibsons and devices (not the prompt screen's device-only rain) */
    const bool zsb = zap_sb_on();
    botcorps_rain_set_gibson(s_screen == SCR_GIBSON || (s_screen == SCR_SCAN && s_scan_bot == 12) || zsb);
    /* v1019n (#15): Terminal + the Gibson prompt screen = our fixed device-icon rain only */
    botcorps_rain_set_no_bots(s_screen == SCR_TERMINAL || (s_screen == SCR_GCHAT && !zsb));
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
    rain_flags();
    /* v1016k (SPEC R1): rain behind EVERY screen. Live (stepped) where it always was;
     * panel screens (terminal, scan box, game, diag) get the same rain painted
     * as a static layer on each full paint, framed by their opaque panels. */
    /* v1019x: Gibson's Defend draws its rain only inside the left chip rect (zap_rain_rect), never as
     * the full-screen layer */
    const bool rain_bg = (wants_rain() || rain_static()) && !zap_sb_on();
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
    if (fa_live()) paint_found_alert(g);
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
#ifdef BOTCORPS_HOST_RENDER
    { extern int host_int_clears; ++host_int_clears; }   /* v1019n-mod #11: count full INT clears */
#endif
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

    if (dg_on()) {                /* v1019j: INT = Defend score / lives / timer as text */
        dg_paint_hud(Gfx(menu), width, true);
        s_int_bot = -1;
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
/* v1019n-mod (#8): the switch bank moved into the lower bay (CP x158, J1 x190, PWR x219): Up from a bank
 * part = the chip above it, Down = the top bar; Down from a chip = the bank part under it. */
static const int8_t CP_UP[CP_N + 2]   = { CP_N + 1, CP_N + 1, CP_N + 1, 5, 2, CP_N, 7, CP_N, 3, 4 };
static const int8_t CP_DN[CP_N + 2]   = { 4, 4, 4, CP_N, CP_N + 1, 3, CP_N, 6, 5, 2 };
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
    if (s_cursor == 0) s_color = (s_color + d + THEME_N) % THEME_N;
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
    } else if (s_cursor == 5) {                /* v1019n-mod (#6): BACKGROUND style (the former savers) */
        s_bg = (s_bg + d + BGS_NUM) % BGS_NUM;
        botcorps_rain_set_bg(s_bg);
    } else if (s_cursor == 6) {                /* v1019n-mod: PWR LED = background motion on/off */
        s_bg_motion = s_bg_motion ? 0 : 1;
    } else if (s_cursor == 7) {                /* v1019n-mod (#7): BG COLOR = MULTI + four solids */
        const int i = bgc_ix();
        const int nx = i < 0 ? 0 : (i + d + BGC_N) % BGC_N;
        botcorps_rain_set_mode(BGC_MODE[nx]);
        save_rain_color();
    }
}

static void gp_run(const char *line);
static void activate(void)
{
    switch (s_screen) {
    case SCR_RAIN:
        enter_screen(SCR_GIBSON, -1); /* mock: rain + Enter = enterHub() */
        break;
    case SCR_GIBSON:
        if (s_hub_zone == 1) break;      /* prompt: Enter runs a typed line (ui_dispatch) */
        if (s_hub_zone == 2) {           /* command menu: Enter runs the picked command (v1019) */
            const int n = (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0]));
            if (!s_gp_buf[0]) {          /* nothing picked yet: Enter picks the first / next one */
                s_hub_cmd = (s_hub_cmd + 1) % n;
                snprintf(s_gp_buf, sizeof(s_gp_buf), "%s", GP_CMD[s_hub_cmd]);
                partial_gp_strip();
                break;
            }
            char line[28];
            snprintf(line, sizeof(line), "%s", s_gp_buf);
            s_gp_buf[0] = 0;
            gp_run(line);
            break;
        }
        if (s_hub_zone == 3) {           /* Gibson: Enter enters the Gibson game */
            s_defend_bot = 12;           /* v1019b: Gibson's own game */
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
            /* v1019n-mod (#11) INT flicker fix: a UI-colour change used to set s_int_dirty here, which runs
             * paint_int(): menu.fillScreen(PAL_DEEP) on the bare INT panel (no sprite / no band buffer), then
             * the bar + ID card are redrawn over it - the deep-fill frame is the visible flash. Nothing on INT
             * reads the theme (THEMES / theme() / accent() are file-static here; paint_int, botcorps_id_paint
             * and dg_paint_hud only use COL_* / PAL_* and the bot colour), so the repaint changed no pixel:
             * it was pure flash. The colour change now repaints EXT only (band present, S1 no-flash). */
#if defined(BOTCORPS_HOST_RENDER) && defined(BOTCORPS_REPRO_INT_FLASH)
            if (s_cursor == 0) s_int_dirty = true;   /* host-only repro of the v1019m behaviour */
#endif
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
        /* v1017c (#5, defensive): the control panel's crown / TERM live in paint_control's top bar.
         * This used to draw the OLD side-button plates (draw_btn "terminal" / "back" with labels) at the
         * pre-v1017a spots (284,8 / 250,8) - the only code that can put old buttons on this screen. Focus
         * here is a full present, as it is for every other control-panel stop. */
        s_dirty = true;
        return;
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
static void sb_patch_one(int x, int y, int w, int h)
{
    s_pc_x = x; s_pc_y = y; s_pc_w = w; s_pc_h = h;
    botcorps_band_patch_rect(draw_scan_part, x, y, w, h);
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
}

/* v1017c (#1): scan parts are drawn IN PLACE - each tick paints only its own dirty rect and pushes
 * only those pixels (botcorps_band_patch_rect). Before, every tick went through band_patch, which
 * rebuilt the whole non-resident 120-row slice first (sb_tick alternates slices, so nearly every
 * tick did): a constant half-screen redraw. paint_sbox is opaque over each part rect (proved on the
 * host band sim by poisoning the rect before the draw: gate S1/S4). */
static void sb_patch(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0 || !botcorps_dual_ok()) return;
    if (botcorps_band_direct() || !botcorps_band_live()) {
        s_pc_x = x; s_pc_y = y; s_pc_w = w; s_pc_h = h;
        patch_world(draw_scan_part, x, y, w, h);
        s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
        return;
    }
    if (y < 120 && y + h > 120) {
        sb_patch_one(x, y, w, 120 - y);
        sb_patch_one(x, 120, w, y + h - 120);
    } else {
        sb_patch_one(x, y, w, h);
    }
}

/* v1019x: the scan shell's live bus keys (top-bar bot squares, top rule sweep, under-grid sweep), shared
 * by sb_tick and the Gibson zap Defend tick so both shells animate identically. */
static void sb_bus_keys(uint32_t t, uint32_t *top, uint32_t *rule, uint32_t *sr)
{
    const uint32_t e = (t - s_bus_t0) % 3400u;
    const float u = (float)e / 3400.f;
    int outn = 0;
    for (int i = 0; i < 12; ++i) outn += (u > (float)i * .02f && u < .7f) ? 1 : 0;
    *top = (uint32_t)outn * 4u + (t / 400u) % 3u;
    *rule = (uint32_t)(320.f * u) / 8u;
    *sr = (uint32_t)(176.f * u) / 4u;
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
    uint32_t top, rule, sr;
    sb_bus_keys(t, &top, &rule, &sr);
    if (top != k_top) { if (k_top != ~0u) sb_patch(0, 0, 320, 24); k_top = top; }
    if (rule != k_rule) { if (k_rule != ~0u) sb_patch(0, 24, 320, 1); k_rule = rule; }
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
    if (s_screen == SCR_SCAN && s_scan_bot >= 12) {             /* v1019: Gibson voice ticker */
        static uint32_t k_tick = ~0u;
        const uint32_t tk = gib_ticker_key(t);
        if (tk != k_tick) { if (k_tick != ~0u) sb_patch(GT_X, GT_Y, GT_W, GT_H); k_tick = tk; }
    }
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
    if (s_screen == SCR_SCAN) {                                   /* #5 hunt grid: this bot's icons fall continuously */
        bot_hunt_step(t);
        const uint32_t hk = t / 33u;
        if (hk != k_grid) { if (k_grid != ~0u) sb_patch(BH_X - 1, BH_Y - 1, BH_W + 3, BH_H + 3); k_grid = hk; }
        return;
    }
    if ((uint32_t)n != k_grid) {
        if (k_grid != ~0u) sb_patch(106, 52, 176, 92);
        k_grid = (uint32_t)n;
        k_fresh = k_scout = ~0u;
        return;
    }
    int nx = 0, ny = 0;
    if (n >= LIST_N || !sb_cell(band, n - 1, &nx, &ny)) return;   /* mod item 9: no fresh blink / scout after the sweep */
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
    /* v1017c: same sel / dim as paint_boards. The partial used to pass dim=false, so a board that lost
     * focus by a partial (focus to the prompt / commands button, or arrows to another board) came back
     * full-bright instead of dimmed until the next full paint (S1 terminal-typing caught it). */
    const bool sel = (s_part_i == s_term) && !s_focus_cmds && !s_cmd_btn && !s_atk_btn && !s_term_prompt;
    paint_one_board(g, s_part_i, sel, !sel);
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

/* v1019 (Dirt #6): scan box terminal. Commands are coherent with the box: help, scan (restart the
 * sweep), hunt / info (the device under the cursor, radio boxes; Gibson: his hunt game), gloss,
 * clear, back, terminal, defend, wifi / ble. */
/* v1019q (#9a): the bot's own scan commands first, routing + help last */
static const char *const SP_CMD[] = { "scan", "hunt", "info", "gloss", "defend", "terminal", "back", "help" };
static const int SP_CMD_N = (int)(sizeof(SP_CMD) / sizeof(SP_CMD[0]));
static void sp_strip(void)
{
    sb_patch(106, 152, 176, 38);
}
static void sp_say(const char *l1, const char *l2)
{
    snprintf(s_sp_l1, sizeof(s_sp_l1), "%s", l1);
    snprintf(s_sp_l2, sizeof(s_sp_l2), "%s", l2);
    s_sp_has = true;
}
static void enter_defend(int bot, int ret);
static int sp_dev(void)
{
    const int band = list_band();
    if (s_cursor >= 0 && s_cursor < LIST_N && !s_dev_ign[band][s_cursor]) return s_cursor;
    for (int i = 0; i < LIST_N; ++i) if (!s_dev_ign[band][i]) return i;
    return -1;
}
static void sp_run(const char *line)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", line);
    char *cmd = buf;
    while (*cmd == ' ') ++cmd;
    char *sp = strchr(cmd, ' ');
    char *arg = 0;
    if (sp) { *sp = 0; arg = sp + 1; while (*arg == ' ') ++arg; }
    char echo[32];
    snprintf(echo, sizeof(echo), "$ %s%s%s", cmd, (arg && arg[0]) ? " " : "", arg ? arg : "");
    const bool core = s_screen == SCR_SCAN;
    const bool gib = core && s_scan_bot >= 12;
    s_sp_cmd = -1;
    if (!cmd[0]) return;
    if (strcmp(cmd, "help") == 0) { sp_say("scan hunt info gloss defend", "terminal back clear help"); sp_strip(); return; }
    if (strcmp(cmd, "clear") == 0) { s_sp_has = false; sp_strip(); return; }
    if (strcmp(cmd, "back") == 0 || strcmp(cmd, "exit") == 0) {
        if (s_hunt_dev >= 0 || s_info_dev >= 0 || s_gloss >= 0 || s_act >= 0) {
            s_hunt_dev = -1; s_hunt_len = 0; s_info_dev = -1; s_gloss = -1; s_act = -1;
            sp_say(echo, "[ ok ] box");
            s_dirty = true;
            return;
        }
        go_back();
        return;
    }
    if (strcmp(cmd, "terminal") == 0 || strcmp(cmd, "term") == 0) { enter_screen(SCR_TERMINAL, SCR_GIBSON); boot_log(); return; }
    if (strcmp(cmd, "defend") == 0) { enter_defend(core ? (gib ? 12 : s_scan_bot) : 12, s_screen); return; }
    if (strcmp(cmd, "wifi") == 0 || (strcmp(cmd, "scan") == 0 && arg && strcmp(arg, "wifi") == 0)) {
        if (s_screen != SCR_WIFI_RADAR) { enter_screen(SCR_WIFI_RADAR, SCR_GIBSON); return; }
    }
    if (strcmp(cmd, "ble") == 0 || (strcmp(cmd, "scan") == 0 && arg && (strcmp(arg, "ble") == 0 || strcmp(arg, "bluetooth") == 0))) {
        if (s_screen != SCR_BT_RADAR) { enter_screen(SCR_BT_RADAR, SCR_GIBSON); return; }
    }
    if (strcmp(cmd, "scan") == 0 || strcmp(cmd, "wifi") == 0 || strcmp(cmd, "ble") == 0) {
        s_bus_t0 = millis();
        sp_say(echo, "[ ok ] scan restarted");   /* v1019r: wifi + ble share this line (was "sweep restarted") */
        s_dirty = true;
        return;
    }
    if (strcmp(cmd, "hunt") == 0) {
        if (core) {
            if (gib) { s_gib_hunt = !s_gib_hunt; sp_say(echo, s_gib_hunt ? "[ ok ] hunt game on" : "[ ok ] hunt game off"); }
            else sp_say(echo, "[ ok ] hunting its lane");
            s_dirty = true;
            return;
        }
        const int d = sp_dev();
        if (d < 0) { sp_say(echo, "no device to hunt"); sp_strip(); return; }
        s_info_dev = -1; s_gloss = -1; s_act = -1;
        s_hunt_dev = d; s_hunt_band = list_band(); s_hunt_len = 0; s_hunt_tS = 0; s_hunt_force = 0;
        s_cursor = d;
        sp_say(echo, "[ ok ] hunting - esc stops");
        s_dirty = true;
        return;
    }
    if (strcmp(cmd, "info") == 0) {
        if (core) {
            char r[32];
            snprintf(r, sizeof(r), "[ ok ] %s core bus idle", gib ? "gibson" : BOTS[s_scan_bot].short_name);
            sp_say(echo, r);
            sp_strip();
            return;
        }
        const int d = sp_dev();
        if (d < 0) { sp_say(echo, "no device"); sp_strip(); return; }
        s_hunt_dev = -1; s_gloss = -1; s_act = -1;
        s_info_dev = d; s_cursor = d;
        sp_say(echo, "[ ok ] info - esc closes");
        s_dirty = true;
        return;
    }
    if (strcmp(cmd, "gloss") == 0 || strcmp(cmd, "glossary") == 0) {
        if (core) {                              /* v1019q (#9c): it used to only answer "gloss lives in wifi / ble" */
            enter_screen(SCR_WIFI_RADAR, s_screen);   /* open the device glossary in the Wi-Fi scan box */
            s_gloss = 0;
            sp_say(echo, "[ ok ] glossary - esc closes");
            s_dirty = true;
            return;
        }
        s_hunt_dev = -1; s_info_dev = -1; s_act = -1;
        s_gloss = 0;
        sp_say(echo, "[ ok ] glossary - esc closes");
        s_dirty = true;
        return;
    }
    /* v1019r (#3): every other Terminal command (bots, ping, rain, filter, track, sniff, roster, theme ...)
     * runs from the scan prompt too (it used to answer "not found"): the Terminal's own run_cmd, its first
     * reply line shown in the strip (the full reply is in the Terminal log). */
    for (int i = 0; i < CMD_N; ++i) {
        if (strcmp(cmd, CMDN[i]) != 0) continue;
        const int scr0 = s_screen, next0 = s_log_next;
        run_cmd(line);
        if (s_screen != scr0) return;               /* go / load / theme moved on */
        const int added = (s_log_next - next0 + LOG_ROWS) % LOG_ROWS;
        char r[40];
        if (added >= 2) {
            const char *ln = log_line(s_log_n - added + 1);
            if (ln[0] == '\x01') ln += 2;
            while (*ln == ' ') ++ln;
            if (added > 2) snprintf(r, sizeof r, "[ ok ] %.18s +%d", ln, added - 2);
            else snprintf(r, sizeof r, "[ ok ] %.24s", ln);
        } else snprintf(r, sizeof r, "[ ok ] %s", cmd);
        sp_say(echo, r);
        s_dirty = true;
        return;
    }
    sp_say(echo, "not found - type help");
    sp_strip();
}

static void sp_auto_arm(void)
{
    if (s_screen != SCR_WIFI_RADAR && s_screen != SCR_BT_RADAR) return;
    snprintf(s_sp_buf, sizeof(s_sp_buf), "%s", s_screen == SCR_WIFI_RADAR ? "scan wifi" : "scan ble");
    s_sp_cmd = -1;
    s_sp_auto_t = millis() + 650u;
    if (!s_sp_auto_t) s_sp_auto_t = 1;
    s_dirty = true;
}
static void sp_auto_poll(void)
{
    if (!s_sp_auto_t || (int32_t)(millis() - s_sp_auto_t) < 0) return;
    s_sp_auto_t = 0;
    if (!sbox_on() || !s_sp_buf[0]) return;
    char line[16];
    snprintf(line, sizeof(line), "%s", s_sp_buf);
    s_sp_buf[0] = 0;
    sp_run(line);
    s_dirty = true;
}

static void on_arrows(int key)
{
    if (s_screen == SCR_GCHAT) {
        /* MOD item 2 (Mobandy): in the Defend-screen prompt the arrows drive ONLY the prompt. Left / Right
         * move the caret, Up / Down walk the prompt history. They never open or walk the command sheet
         * (the Gibson bot command glossary: Tab opens it, Tab steps it, Enter picks, Esc closes) and never
         * reach the rail or the board. */
        const int n = (int)strlen(s_gp_buf);
        int c = (s_gc_caret < 0 || s_gc_caret > n) ? n : s_gc_caret;
        if (key == 3 && c > 0) --c;
        if (key == 4 && c < n) ++c;
        if ((key == 1 || key == 2) && s_hist_n > 0) {
            if (key == 1) s_gc_hist_i = s_gc_hist_i < 0 ? s_hist_n - 1 : (s_gc_hist_i > 0 ? s_gc_hist_i - 1 : 0);
            else s_gc_hist_i = s_gc_hist_i < 0 ? -1 : (s_gc_hist_i + 1 < s_hist_n ? s_gc_hist_i + 1 : -1);
            snprintf(s_gp_buf, sizeof(s_gp_buf), "%s", s_gc_hist_i >= 0 ? botcorps_t0_term_hist[s_gc_hist_i] : "");
            c = (int)strlen(s_gp_buf);
        }
        s_gc_caret = c >= (int)strlen(s_gp_buf) ? -1 : c;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GIBSON) {
        /* v1017b (Dirt 8:27): Left/Right only cycle the icon row; Up/Down walk
         * icons -> prompt -> command menu -> Gibson (Down), reverse with Up. */
        if ((key == 3 || key == 4) && s_hub_zone == 2) {
            /* v1019 (Dirt #6): Left/Right cycle the command menu into the prompt; Enter runs it */
            const int n = (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0]));
            s_hub_cmd = s_hub_cmd < 0 ? (key == 4 ? 0 : n - 1) : (s_hub_cmd + (key == 4 ? 1 : n - 1)) % n;
            snprintf(s_gp_buf, sizeof(s_gp_buf), "%s", GP_CMD[s_hub_cmd]);
            partial_gp_strip();
            return;
        }
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
        /* v1017c (#3): arrows always move focus in the Terminal (they used to be swallowed as
         * history / typed ; . , / once the prompt held text, so the commands menu was unreachable).
         * Same model as the hub: Up/Down walk boards -> prompt -> commands (≡) button, Up reverses;
         * Left/Right from the prompt or the button go back to the boards. Arrow history recall
         * is gone: it is what swallowed Up/Down whenever the prompt held text. */
        if (s_term_ovl == 3) { s_term_ovl = 0; s_dirty = true; return; }   /* v1019r: any arrow closes help */
        if (s_term_ovl) {                            /* #10: Left/Right flip attack prompts <-> glossary */
            if (key == 3 || key == 4) { s_term_ovl = s_term_ovl == 2 ? 1 : 2; s_dirty = true; }
            return;
        }
        if (s_atk_btn) {
            s_atk_btn = false;
            if (key == 1) s_cmd_btn = true;          /* Up: attack button -> commands button */
            else if (key == 2) s_atk_btn = true;     /* Down: bottom of the walk, stay */
            else partial_tile(s_term);               /* Left / Right: back to the boards */
            partial_term_lower();
            return;
        }
        if (s_term_prompt) {
            s_term_prompt = false;
            if (key == 2) s_cmd_btn = true;          /* Down: prompt -> commands button */
            else partial_tile(s_term);               /* Up / Left / Right: back to the boards */
            partial_term_lower();
            return;
        }
        if (s_cmd_btn) {
            s_cmd_btn = false;
            if (key == 1) s_term_prompt = true;      /* Up: button -> prompt */
            else if (key == 2) s_atk_btn = true;     /* #10 Down: commands button -> attack-prompt button */
            else partial_tile(s_term);               /* Left / Right: back to the boards */
            partial_term_lower();
            return;
        }
        const int old = s_term;
        int c = s_term % 4;
        int r = s_term / 4;
        if (key == 2 && r == 2) {
            /* v1017c: Down from the bottom board row stops on the prompt (then the commands button). */
            s_term_prompt = true;
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
    if (fa_live()) {
        if (key == 1 || key == 3) s_fa_sel = (s_fa_sel + FA_N - 1) % FA_N;
        if (key == 2 || key == 4) s_fa_sel = (s_fa_sel + 1) % FA_N;
        s_dirty = true;
        return;
    }
    if (gd_key(key)) { s_dirty = true; return; }   /* mod item 10: Gibson drop-down owns the arrows */
    if (s_screen == SCR_SCAN) {
        /* v1019 (Dirt #6): arrows drive the ≡ command menu: each press steps a command into the
         * prompt (the ≡ chip lights); Enter runs it, Esc clears it. */
        const int d = (key == 1 || key == 3) ? -1 : 1;
        s_sp_cmd = s_sp_cmd < 0 ? (d > 0 ? 0 : SP_CMD_N - 1) : (s_sp_cmd + d + SP_CMD_N) % SP_CMD_N;
        snprintf(s_sp_buf, sizeof(s_sp_buf), "%s", SP_CMD[s_sp_cmd]);
        sp_strip();
        return;
    }
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
        /* positions: 0..LIST_N-1 devices, LIST_N = the > GLOSSARY prompt.
         * mod item 9: Left/Right step one cell, Up/Down step one grid ROW (7) so the halo moves the way
         * the arrow points; off the top = the > prompt, off the end wraps to the prompt. */
        const int step = (key == 1) ? -SG_COLS : (key == 2) ? SG_COLS : dir;
        int i = s_cursor;
        if (i < 0 || i >= LIST_N) i = dir > 0 ? 0 : LIST_N - 1;   /* from the > prompt into the grid */
        else {
            int j = i + step;
            if (key == 2 && j >= LIST_N && i / SG_COLS < (LIST_N - 1) / SG_COLS) j = LIST_N - 1;   /* short last row */
            i = (j < 0 || j >= LIST_N) ? LIST_N : j;
        }
        for (int k = 0; k <= LIST_N && i < LIST_N && s_dev_ign[band][i]; ++k) {
            i += dir;
            if (i < 0 || i >= LIST_N) i = LIST_N;
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
    if (fa_live()) {
        fa_run(s_fa_sel);
        return;
    }
    if (sbox_on() && s_sp_buf[0]) {          /* v1019: a typed line runs on every scan box / sub-scan */
        char line[16];
        snprintf(line, sizeof(line), "%s", s_sp_buf);
        s_sp_buf[0] = 0;
        sp_run(line);
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
        if (s_term_prompt) return;   /* v1017c: Enter on an empty prompt does nothing (hub: same) */
        if (s_term_ovl) return;      /* #10: the attack/glossary overlay is a viewer; Esc closes it */
        if (s_atk_btn) {
            /* #10: Enter on the attack-prompt button pulls up the per-bot/device attack prompts. */
            s_atk_btn = false;
            s_term_ovl = 2;
            s_dirty = true;
            lut_for_term();
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
    if (fa_live()) {
        fa_run(3);
        return;
    }
    if (sbox_on() && (s_sp_buf[0] || s_sp_cmd >= 0)) {   /* v1019: Esc clears the typed line first */
        s_sp_buf[0] = 0;
        s_sp_cmd = -1;
        sp_strip();
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
        if (s_gc_panel == 3) { s_gc_panel = 0; s_dirty = true; return; }   /* v1019r: Esc closes the Device Dex */
        if (s_sheet) {
            s_sheet = false;
            s_dirty = true;
            return;
        }
        if (s_gp_buf[0]) {
            s_gp_buf[0] = 0;
            s_gc_caret = -1; s_gc_hist_i = -1;
            s_dirty = true;
            return;
        }
        enter_screen(SCR_GIBSON, -1);
        return;
    }
    if (s_screen == SCR_TERMINAL && s_term_ovl == 3 && !s_sheet) {   /* v1019r: Esc closes help */
        s_term_ovl = 0;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_TERMINAL && s_term_ovl && !s_sheet) {
        s_term_ovl = 0;
        s_atk_btn = true;        /* #10: Esc closes the attack/glossary overlay back onto its button */
        s_dirty = true;
        lut_for_term();
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
        s_defend_bot = 12;               /* v1019b: Gibson's own game */
        enter_screen(SCR_GCHAT, SCR_GIBSON);
        s_sheet = true;
        s_sheet_i = 0;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_GCHAT) {
        /* MOD item 2: Tab opens the command sheet and then steps it (arrows no longer do); Esc closes */
        if (!s_sheet) { s_sheet = true; s_sheet_i = 0; }
        else s_sheet_i = (s_sheet_i + 1) % GC_N;
        s_dirty = true;
        return;
    }
    if (s_screen != SCR_TERMINAL || s_sheet) return;
    if (!botcorps_t0_term_prompt[0]) {
        s_sheet = !s_sheet;
        s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
            const size_t at = (s_gc_caret < 0 || (size_t)s_gc_caret > n) ? n : (size_t)s_gc_caret;
            memmove(s_gp_buf + at + 1, s_gp_buf + at, n - at + 1);   /* MOD item 2: insert at the caret */
            s_gp_buf[at] = c;
            if (s_gc_caret >= 0) ++s_gc_caret;
        }
        s_gc_hist_i = -1;
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
    if (sbox_on()) {                         /* v1019 (Dirt #6): typing on the scan / sub-scan terminals */
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        const size_t n = strlen(s_sp_buf);
        if (n + 1 < sizeof(s_sp_buf) && c >= 32 && c < 127) {
            s_sp_buf[n] = c;
            s_sp_buf[n + 1] = 0;
        }
        s_sp_cmd = -1;
        sp_strip();
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
    s_cmd_btn = false;
    s_atk_btn = false;
    if (s_term_ovl) { s_term_ovl = 0; s_dirty = true; }
    if (!s_term_prompt) {
        s_term_prompt = true;    /* v1017c: typing puts focus on the prompt (hub: same) */
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
        const size_t at = (s_gc_caret < 0 || (size_t)s_gc_caret > n) ? n : (size_t)s_gc_caret;
        if (at > 0) {                        /* MOD item 2: delete before the caret */
            memmove(s_gp_buf + at - 1, s_gp_buf + at, n - at + 1);
            if (s_gc_caret > 0) --s_gc_caret;
        }
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
    if (sbox_on() && s_sp_buf[0]) {
        s_sp_buf[strlen(s_sp_buf) - 1] = 0;
        s_sp_cmd = -1;
        sp_strip();
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

#include "ui/botcorps_defend.inc"

/* v1017c (#2/#3) keyboard root cause. The old reader fired whenever the NUMBER of held keys changed
 * (Keyboard.isChange() compares only keyList().size()) and then answered from keysState(), which
 * describes ALL keys still held, not the one just pressed:
 *   - rollover typing ("h" still down when "e" lands) re-emitted the first held char: "he" -> "hh";
 *   - releasing one key of a chord re-emitted the key still held (a second letter / a second arrow);
 *   - a held Backspace / Tab / Enter won over a new letter (hid_keys was scanned first);
 *   - a press and a release in the same scan kept the count equal and was dropped;
 *   - with text in the prompt, plain ; . , / were typed as characters, and an extra release-emitted
 *     arrow skipped focus stops, so the commands button could not be reached by arrows.
 * Now: edge-detect presses against the previous keyList (the TCA8418 reader keeps press order), act
 * only on the newest newly pressed non-modifier key, and map it from the keymap with the live
 * Shift / Ctrl / caps state. Releases never produce a key. */
static Point2D_t s_kprev[16];
/* v1019r (#6 keyboard play): held-arrow auto-repeat while a Defend movement game is live (input only:
 * 260 ms first repeat, then every 130 ms, the same key path as a press). */
static int s_rep_key = 0;
static Point2D_t s_rep_pos;
static uint32_t s_rep_next = 0;
static int s_kprev_n = 0;
static int read_key(char *typed)
{
    *typed = 0;
    const auto &kl = M5Cardputer.Keyboard.keyList();
    int fresh = -1;
    bool fn = false, shift = false, ctrl = false;
    for (int i = 0; i < (int)kl.size(); ++i) {
        const uint8_t v = (uint8_t)M5Cardputer.Keyboard.getKeyValue(kl[i]).value_first;
        if (v == KEY_FN) { fn = true; continue; }
        if (v == KEY_LEFT_SHIFT) { shift = true; continue; }
        if (v == KEY_LEFT_CTRL) { ctrl = true; continue; }
        if (v == KEY_OPT || v == KEY_LEFT_ALT) continue;
        bool held = false;
        for (int j = 0; j < s_kprev_n; ++j) if (s_kprev[j] == kl[i]) { held = true; break; }
        if (!held) fresh = i;                    /* newest new press wins */
    }
    s_kprev_n = 0;
    for (const auto &p : kl) if (s_kprev_n < 16) s_kprev[s_kprev_n++] = p;
    if (fresh < 0) return K_NONE;
    s_rep_key = 0;
    s_rep_pos = kl[fresh];
    const KeyValue_t kv = M5Cardputer.Keyboard.getKeyValue(kl[fresh]);
    const uint8_t code = (uint8_t)kv.value_first;
    s_key_fn = false;
    if (code == KEY_ENTER) return K_OK;
    if (code == KEY_BACKSPACE) return K_BS;
    if (code == KEY_TAB) return K_TAB;
    if (fn) {
        /* Fn chords are never typing: Fn+; . , / are arrows and Fn+` is Esc on every screen. */
        s_key_fn = true;
        if (code == ';') return K_UP;
        if (code == '.') return K_DN;
        if (code == ',') return K_LT;
        if (code == '/') return K_RT;
        if (code == '`') return K_ESC;
        s_key_fn = false;
    }
    const char c = (shift || ctrl || M5Cardputer.Keyboard.capslocked()) ? kv.value_second : kv.value_first;
    /* Text-entry screens own plain ; . , / once something is typed (hub prompt / Gibson chat). The
     * Terminal is driven like the hub's walk, so its arrows are always arrows. */
    /* v1019r (#3): the scan boxes too (Gibson's scan board incl.): once a line is open, plain ; . , / type
     * into it instead of becoming arrows that replaced the typed line with a command-rail entry. */
    const bool prompt_owns = (s_screen == SCR_GCHAT && !dg_on()) || (s_screen == SCR_GIBSON && s_gp_buf[0])
                          || (sbox_on() && s_sp_buf[0]);
    if (!prompt_owns) {
        if (c == ';' || c == ':') return K_UP;
        if (c == '.' || c == '>') return K_DN;
        if (c == ',' || c == '<') return K_LT;
        if (c == '/' || c == '?') return K_RT;
    }
    if (c == '`' || c == '~') return K_ESC;
    if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
        *typed = c;
        return K_CHAR;
    }
    return K_NONE;
}
#ifdef BOTCORPS_HOST_RENDER
/* gate K1: drive the real read_key + dispatcher with a scripted key matrix (host keyboard shim). */
int botcorps_host_read_key(char *typed) { return read_key(typed); }
#endif

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

/* v1019 (Dirt #6): every Defend path lands in the Gibson game, jacked in, for that bot. */
static void enter_defend(int bot, int ret)
{
    s_defend_bot = (bot < 0 || bot > 12) ? 12 : bot;
    enter_screen(SCR_GCHAT, ret);
    gc_run("jackin");
    /* v1019v: Gibson Defend = HACKERS ZAP (full-width). Other bots keep category game. */
    if (s_defend_bot >= 12) dg_start_game(12, DG_ZAP);
    else dg_start(s_defend_bot);   /* v1019j: device-category game */
    s_dirty = true;
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
        /* v1019 prompt audit: the hub's own commands (it used to echo the Terminal's SYS help row) */
        gp_push("hunt defend scan wifi ble");   /* v1019q (#9a): bot commands first */
        gp_push("chat terminal control go");
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
        sp_auto_arm();
        return;
    }
    if (strcmp(cmd, "chat") == 0) {
        gp_open(SCR_GCHAT);
        return;
    }
    if (strcmp(cmd, "hunt") == 0 && (!arg || !arg[0])) {
        /* #5 mock gibHuntStart: keyboard path to Gibson's hunt game (his scan board, hunt mode on) */
        s_scan_bot = 12;
        s_gib_hunt = true;
        s_gib_hunt_keep = true;
        gp_open(SCR_SCAN);
        return;
    }
    if (strcmp(cmd, "defend") == 0) {
        /* v1019 (Dirt #6): Defend enters the Gibson game (it used to open the Terminal). */
        const int b = gc_bot_of(arg);
        if (b == -2) { gp_push("no such bot"); partial_gp_strip(); return; }
        enter_defend(b < 0 ? 12 : b, SCR_GIBSON);
        return;
    }
    if (strcmp(cmd, "scan") == 0 && (!arg || !arg[0] || strcmp(arg, "radar") == 0)) {
        s_scan_bot = 12;
        gp_open(SCR_SCAN);
        return;
    }
    if (strcmp(cmd, "scan") == 0 && arg && strcmp(arg, "wifi") == 0) {
        gp_open(SCR_WIFI_RADAR);
        sp_auto_arm();
        return;
    }
    if (strcmp(cmd, "scan") == 0 && arg && (strcmp(arg, "ble") == 0 || strcmp(arg, "bluetooth") == 0)) {
        gp_open(SCR_BT_RADAR);
        sp_auto_arm();
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
static bool dg_repeat_ok(void);
void botcorps_ui_input(void)
{
    char typed = 0;
    const int key = read_key(&typed);
    if (key == K_NONE) {
        if (!s_rep_key) return;
        bool held = false;
        for (const auto &p : M5Cardputer.Keyboard.keyList()) if (p == s_rep_pos) { held = true; break; }
        if (!held || !dg_on() || !dg_repeat_ok() || s_gp_buf[0]) { s_rep_key = 0; return; }
        if ((int32_t)(millis() - s_rep_next) < 0) return;
        s_rep_next = millis() + 130u;
        ui_dispatch(s_rep_key, 0);
        return;
    }
    if (key == K_UP || key == K_DN || key == K_LT || key == K_RT) { s_rep_key = key; s_rep_next = millis() + 260u; }
    ui_dispatch(key, typed);
}

/* v1019q (item 8) root cause: while a Defend game is live, ui_dispatch handed EVERY key to dg_key (the
 * game), which has no use for letters (dropped) and takes Enter as a game key, and the HUD's prompt row
 * painted the fixed control hint instead of s_gp_buf. Every Defend entry (hub / Terminal / scan "defend",
 * jackin, Enter on the empty prompt) starts a game at once, so Gibson's Defend prompt never took a keystroke.
 * Now the Defend prompt shares the keyboard with the game: a letter starts a typed line (the games use only
 * ; . , / arrows, Enter, Space, digits, Tab, backtick); once a line is open every printable key types,
 * Backspace edits, Esc clears the line, and Enter runs it through the board's own gc_run (the game round
 * stops so the reply shows in the log, exactly like the idle Defend prompt; game commands start a game).
 * With the line empty, every key still drives the game exactly as before. */
static bool dg_prompt_key(int key, char typed)
{
    const bool open = s_gp_buf[0] != 0;
    if (key == K_CHAR) {
        const bool letter = (typed >= 'a' && typed <= 'z') || (typed >= 'A' && typed <= 'Z');
        if (!open && !letter) return false;
        on_char(typed);
        return true;
    }
    if (!open) return false;
    if (key == K_BS) { on_bs(); return true; }
    if (key == K_ESC) { s_gp_buf[0] = 0; s_gc_caret = -1; s_gc_hist_i = -1; s_dirty = true; return true; }
    if (key == K_OK) {
        char line[28];
        snprintf(line, sizeof(line), "%s", s_gp_buf);
        s_gp_buf[0] = 0;
        s_gc_caret = -1; s_gc_hist_i = -1;
        if (s_dg == DG_ZAP) {
            /* v1019v: typing a device name zaps without ending the round */
            if (zap_try_cmd(line)) { s_dirty = true; s_int_dirty = true; return true; }
            /* meta commands still leave the round */
            if (strcmp(line, "quit") == 0 || strcmp(line, "back") == 0 || strcmp(line, "night") == 0
                || strcmp(line, "sweep") == 0 || strcmp(line, "lobby") == 0 || strcmp(line, "jackin") == 0
                || strncmp(line, "play", 4) == 0 || strcmp(line, "defend") == 0 || strcmp(line, "dex") == 0) {
                dg_stop();
                gc_run(line);
            } else {
                char b[36];
                snprintf(b, sizeof b, "[ ?? ] try wifi/cam/bt/drone/flock/tower");
                gc_push(12, b);
            }
            s_dirty = true;
            return true;
        }
        dg_stop();
        gc_run(line);
        s_dirty = true;
        return true;
    }
    return false;                                  /* arrows / Tab keep driving the game */
}

static void ui_dispatch(int key, char typed)
{
    s_last_key = millis();
    if (s_sp_auto_t) {                             /* v1019r (#4): a key during the pre-typed scan */
        s_sp_auto_t = 0;
        if (key == K_ESC) { s_sp_buf[0] = 0; s_sp_cmd = -1; }   /* cancel; this Esc still goes back */
    }
    if (dg_on()) {                                 /* v1019j: the Defend game owns the keys ... */
        if (!dg_prompt_key(key, typed)) dg_key(key, typed);   /* v1019q (item 8): ... except a typed line */
        return;
    }
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
            s_gc_caret = -1; s_gc_hist_i = -1;
            gc_run(line);
            s_dirty = true;
            return;
        }
        /* v1019p (field fix, item 1): Enter on the empty Defend prompt PLAYS this bot's game on the board
         * (it used to open the command sheet, so Enter never reached a game; Tab still opens the sheet) */
        gc_run("jackin");
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
    sp_auto_poll();
    botcorps_rain_set_hub(s_screen == SCR_GIBSON);
    rain_flags();
    layout_covers();
    bool ext_anim = false;
    if (botcorps_dual_ok() && !wants_rain() && ext_animates() && anim_step_due())
        ext_anim = true;
    if (s_focus_pend && !s_dirty) partial_cursor(s_focus_old, s_focus_new);
    s_focus_pend = false;
    const bool dirty = s_dirty;
    const bool int_dirty = s_int_dirty;
    bool stepped = false;
    /* v1019n-mod: PWR LED off = the background holds still (no rain steps; full paints still draw it) */
    if (botcorps_dual_ok() && wants_rain() && s_bg_motion) stepped = botcorps_rain_step();
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
            if (dg_on()) dg_snap();
        } else if (dg_on()) {
            dg_tick();
        } else if (zap_sb_on()) {
            zap_tick_patch();   /* v1019x: idle Gibson Defend: rain rect + caret */
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
            if (dg_on()) dg_int_tick(botcorps_menu());
            else if (pulse) {
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
        if (dg_on()) dg_snap();
    }
    if (int_dirty) {
        s_int_dirty = false;
        paint_int_now();
        return;
    }
    if (dirty) return;
    if (dg_on()) dg_tick();
    else if (zap_sb_on()) zap_tick_patch();   /* v1019x */
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
    if (dg_on()) dg_int_tick(botcorps_menu());
    else if (pulse) {
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
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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

/* MOD review render (host only): the Defend screen itself and its prompt under the REAL key path.
 * mode 0: the Defend board idle (no game), prompt typed. mode 1: the same after fn-arrows (Up = history,
 * Left x3 = caret) - proves arrows moved only the prompt: sheet closed, screen / panel / rail unchanged.
 * Writes "DGP <mode> sheet=<0|1> screen=<id> panel=<n> buf=<prompt> caret=<n>" to stdout. */
void botcorps_host_defend_prompt(uint8_t *buf, int mode)
{
    host_frame = buf;
    s_fa_on = false; s_act = -1; s_color = 0; s_sheet = false; s_gc_panel = 0;
    botcorps_lut_term(false);
    enter_screen(SCR_GCHAT, SCR_GIBSON);
    s_defend_bot = 12;
    gc_set_count(0);
    s_gp_buf[0] = 0; s_gc_caret = -1; s_gc_hist_i = -1;
    log_add("gibson> status");
    gc_push(12, "link up");
    log_add("gibson> crew");
    gc_push(12, "12 boards linked");
    hist_push("defend wifi");
    hist_push("scan all");
    const char *ty = "help";
    for (const char *c = ty; *c; ++c) ui_dispatch(K_CHAR, *c);
    const int scr0 = s_screen, pan0 = s_gc_panel;
    if (mode == 1) {
        ui_dispatch(K_UP, 0); ui_dispatch(K_UP, 0);      /* history: "scan all" then "defend wifi" */
        ui_dispatch(K_LT, 0); ui_dispatch(K_LT, 0); ui_dispatch(K_LT, 0);   /* caret 3 left */
        ui_dispatch(K_DN, 0); ui_dispatch(K_UP, 0);      /* down = newer, up again = back to "defend wifi" */
        ui_dispatch(K_LT, 0); ui_dispatch(K_LT, 0); ui_dispatch(K_LT, 0); ui_dispatch(K_RT, 0);
    }
    printf("DGP %d sheet=%d screen=%d(%d) panel=%d(%d) buf=%s caret=%d dg=%d\n", mode, (int)s_sheet, s_screen, scr0,
           s_gc_panel, pan0, s_gp_buf, s_gc_caret, s_dg);
    s_bus_t0 = 0;
    host_set_millis(10000u);   /* caret on phase */
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}

void botcorps_host_gc_gloss(uint8_t *buf, int bot)
{
    botcorps_host_gchat(buf);
    s_fa_on = false;
    s_defend_bot = bot;
    s_gc_panel = 1;
    paint_ext(Gfx());
    s_gc_panel = 0;
    s_defend_bot = 12;
}
void botcorps_host_gc_attack(uint8_t *buf, int bot)
{
    botcorps_host_gchat(buf);
    s_fa_on = false;
    s_defend_bot = bot;
    s_gc_panel = 2;
    paint_ext(Gfx());
    s_gc_panel = 0;
    s_defend_bot = 12;
}
void botcorps_host_term_ovl(uint8_t *buf, int mode)
{
    botcorps_host_enter(buf, SCR_TERMINAL);
    s_atk_btn = mode == 0;
    s_term_ovl = mode;
    paint_ext(Gfx());
    s_atk_btn = false;
    s_term_ovl = 0;
}
void botcorps_host_gc_title(uint8_t *buf, int bot)
{
    botcorps_host_gchat(buf);
    s_fa_on = false;
    s_defend_bot = bot;
    paint_ext(Gfx());
    s_defend_bot = 12;
}
void botcorps_host_gmenu(uint8_t *buf)
{
    botcorps_host_gchat(buf);
    s_sheet = true;
    s_sheet_i = 0;      /* v1019q (#9a): "jackin" (was row 8) now leads the sheet */
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

/* v1019j: Defend game host hooks. The game is entered through the real Defend path (enter_defend)
 * warm_ms before the frame clock, then played by the host autopilot (same keys) at the 33 ms tick. */
static void dg_host_play(int bot, int warm_ms, bool ai, int slot = 0)
{
    const uint32_t t1 = millis();
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0;
    botcorps_lut_term(false);
    host_set_millis(t1 - (uint32_t)warm_ms);
    gc_set_count(0);
    enter_defend(bot, SCR_GIBSON);
    /* v1019v: enter_defend(12) starts ZAP; gate's defend-maze / slot games still need the rogue set */
    if (bot == 12 && slot == 0 && s_dg == DG_ZAP) {
        /* default host_defend(12) used to be maze — keep maze for that render via force below only when
         * caller asks for category slot 0 through defend_g after we detect maze-named path.
         * botcorps_host_defend(12) = zap; botcorps_host_defend_g(12,0) forces maze. */
    }
    for (int k = 0; k < slot; ++k) dg_key(K_TAB, 0);   /* v1019m: the real Tab key picks the category's game */
    for (uint32_t t = t1 - (uint32_t)warm_ms + 33u; t < t1; t += 33u) {
        host_set_millis(t);
        if (ai) dg_ai();
        dg_step(t);
    }
    host_set_millis(t1);
    dg_step(t1);
}
static void dg_host_play_game(int bot, int game, int warm_ms, bool ai)
{
    const uint32_t t1 = millis();
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0;
    botcorps_lut_term(false);
    host_set_millis(t1 - (uint32_t)warm_ms);
    gc_set_count(0);
    s_defend_bot = (bot < 0 || bot > 12) ? 12 : bot;
    enter_screen(SCR_GCHAT, SCR_GIBSON);
    dg_start_game(s_defend_bot, game);
    for (uint32_t t = t1 - (uint32_t)warm_ms + 33u; t < t1; t += 33u) {
        host_set_millis(t);
        if (ai) dg_ai();
        dg_step(t);
    }
    host_set_millis(t1);
    dg_step(t1);
}
void botcorps_host_defend(uint8_t *buf, int bot, int warm_ms)
{
    host_frame = buf;
    /* v1019v: Gibson default Defend host render is ZAP; maze uses defend_g slot 0 force */
    if (bot == 12) dg_host_play_game(12, DG_ZAP, warm_ms, true);
    else dg_host_play(bot, warm_ms, true);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* v1019m: the same, for game slot 0-2 of the bot's category (Tab pressed slot times after entry) */
void botcorps_host_defend_g(uint8_t *buf, int bot, int slot, int warm_ms)
{
    host_frame = buf;
    if (bot == 12 && slot == 0) dg_host_play_game(12, DG_MAZE, warm_ms, true);          /* defend-maze */
    else if (bot == 12 && slot == 1) dg_host_play_game(12, DG_TRACE, warm_ms, true);     /* signaltrace */
    else if (bot == 12 && slot == 2) dg_host_play_game(12, DG_JAM, warm_ms, true);       /* jamdodge */
    else dg_host_play(bot, warm_ms, true, slot);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
void botcorps_host_defend_zap(uint8_t *buf, int warm_ms)
{
    /* v1019v/x: scripted mid-game still (mid-zap NULL + docked TIME), not a post-WIN frame. */
    (void)warm_ms;
    host_frame = buf;
    const uint32_t t1 = millis();
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0;
    botcorps_lut_term(false);
    host_set_millis(t1);
    gc_set_count(0);
    s_defend_bot = 12;
    enter_screen(SCR_GCHAT, SCR_GIBSON);
    dg_start_game(12, DG_ZAP);
    /* v1019x: seed a mid-game still on the in-grid geometry: camera mid-zap (bolt + NULL), flock docked
     * (TIME 02), wifi / bt / drone / tower riding in. */
    for (int i = 0; i < ZAP_MAX; ++i) s_zap[i].live = 0;
    s_zap[0].live = 1; s_zap[0].tr = 1; s_zap[0].dist = 30.f; s_zap[0].dock = 0;
    s_zap[1].live = 1; s_zap[1].tr = 4; s_zap[1].dist = zap_len(4) - 2.f; s_zap[1].dock = 1;
    s_zap_dock_seen[1] = 1; s_zap_dock_t0[1] = s_dg_ms - 900u;
    s_zap[2].live = 1; s_zap[2].tr = 0; s_zap[2].dist = 40.f; s_zap[2].dock = 0;
    s_zap[3].live = 1; s_zap[3].tr = 2; s_zap[3].dist = 34.f; s_zap[3].dock = 0;
    s_zap[4].live = 1; s_zap[4].tr = 3; s_zap[4].dist = 36.f; s_zap[4].dock = 0;
    s_zap[5].live = 1; s_zap[5].tr = 5; s_zap[5].dist = 44.f; s_zap[5].dock = 0;
    s_zap_fx_tr = 1; s_zap_fx_from = 4.f; s_zap_fx_to = 24.f;
    s_zap_fx_until = s_dg_ms + 60000u;
    s_zap_null_tr = 1; s_zap_null_until = s_dg_ms + 60000u;
    s_dg_score = 3; s_dg_miss = 0; s_dg_state = DGS_PLAY;
    s_zap_log_n = 0;
    zap_log("[ ** ] BOARD ARMED. TRACES LIVE");
    zap_log("[ !! ] NULL - CAMERA 03/12");
    zap_log("[ !! ] FLOCK DOCKED - TIME");
    s_gp_buf[0] = 0;
    s_cursor = 0;
    s_bus_t0 = millis();
    s_gc_caret = -1;
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* review only (not a gate item): the INT HUD painter drawn into the top-left 240x135 of buf; the bar
 * is a host stand-in (fill + label), the HUD under it is the real dg_paint_hud. */
/* v1019x review (not a gate item): advance the shared rain one step on Gibson's Defend (game state held)
 * and repaint the full frame; used for the rain-f*.png sequence + the outside-rect mask check. */
void botcorps_host_zap_rain_step(uint8_t *buf)
{
    host_frame = buf;
    host_set_millis(millis() + 140u);
    const bool was = botcorps_rain_draw_enabled();
    botcorps_rain_set_draw(false);
    botcorps_rain_step();
    botcorps_rain_set_draw(was);
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* v1019x QA proof (not a gate item): a live zap round, stepped 33 ms at a time, the host autopilot
 * zapping once every ai_every steps (0 = never: devices dock and time out). */
void botcorps_host_zap_sim_start(uint8_t *buf, uint32_t seed_ms)
{
    host_frame = buf;
    host_set_millis(9000u + seed_ms);
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0;
    botcorps_lut_term(false);
    gc_set_count(0);
    s_defend_bot = 12;
    enter_screen(SCR_GCHAT, SCR_GIBSON);
    dg_start_game(12, DG_ZAP);
    for (uint32_t k = 0; k < seed_ms % 97u; ++k) (void)dg_rng();
}
int botcorps_host_zap_sim_frame(uint8_t *buf, int steps, int ai_every)
{
    static int n = 0;
    for (int k = 0; k < steps; ++k) {
        const uint32_t t = millis() + 33u;
        host_set_millis(t);
        if (ai_every > 0 && (++n % ai_every) == 0) dg_ai();
        dg_step(t);
    }
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
    return s_dg_state == DGS_PLAY;
}
/* v1019x V1 proof (not a gate item): run one Terminal command line, print every log row it added */
void botcorps_host_term_cmd(const char *line)
{
    const int n0 = s_log_n, next0 = s_log_next;
    run_cmd(line);
    int added = (s_log_next - next0 + LOG_ROWS) % LOG_ROWS;
    if (added == 0 && s_log_n != n0) added = s_log_n - n0;
    for (int k = 0; k < added; ++k) printf("%s\n", log_slot((next0 + k) % LOG_ROWS));
}
void botcorps_host_repaint(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
void botcorps_host_defend_hud(uint8_t *buf, int bot, int warm_ms, int slot)
{
    host_frame = buf;
    dg_host_play(bot, warm_ms, true, slot);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    Gfx g = Gfx();
    g.fillRect(0, 0, 240, 135, PAL_DEEP);
    g.fillRect(0, 0, 240, BAR_H, COL_BAR);
    g.fillRect(0, BAR_H, 240, 1, COL_LINE);
    const int keep = s_px; s_px = 8;
    txt(g, "BOTcorps", 6, 7, COL_WHITE, lgfx::textdatum::top_left);
    s_px = keep;
    dg_paint_hud(g, 240, true);
}
/* logic test: play from a fresh start for up to max_ms; returns the end state */
int botcorps_host_defend_run(int bot, int slot, bool ai, bool noghost, int max_ms, int *game, int *score, int *miss, int *junk, int *ms)
{
    static uint8_t fb[320 * 240];
    host_frame = fb;
    host_set_millis(100000u);
    s_dg_noghost = noghost;
    /* v1019v: Gibson Defend entry is ZAP; maze/trace/jam logic tests still force the rogue set by slot. */
    if (bot == 12) {
        static const int ZG[3] = { DG_MAZE, DG_TRACE, DG_JAM };
        dg_host_play_game(12, ZG[slot < 0 ? 0 : (slot > 2 ? 2 : slot)], 33, false);
    } else {
        dg_host_play(bot, 33, false, slot);
    }
    uint32_t t = millis();
    const uint32_t end = t + (uint32_t)max_ms;
    while (t < end && s_dg_state != DGS_WIN && s_dg_state != DGS_LOST) {
        t += 33u;
        host_set_millis(t);
        if (ai) dg_ai();
        else if (s_dg == DG_WALL && s_fw_serve) dg_key(K_OK, 0);   /* idle wall: launch only, never move */
        dg_step(t);
    }
    s_dg_noghost = false;
    *game = s_dg; *score = s_dg_score; *miss = s_dg_miss; *junk = s_dg_junk; *ms = (int)s_dg_ms;
    const int st = s_dg_state;
    dg_stop();
    return st;
}
/* v1019r (#10) host proof: Gibson's NIGHT SHIFT (12) / SWEEP (13) started by the real Defend prompt command
 * ("night" / "sweep" typed through ui_dispatch), played by the host autopilot (same keys) for warm_ms, then
 * painted; panel 3 = open the Device Dex afterwards. Also a logic run: plays `rounds` rounds (Enter on each
 * WIN / LOST) and prints every round's end state. Host only, not a gate item. */
void botcorps_host_gib(uint8_t *buf, int game, int warm_ms, int panel, int rounds)
{
    host_frame = buf;
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0; s_gc_panel = 0;
    memset(s_dex, 0, sizeof s_dex);
    botcorps_lut_term(false);
    uint32_t t = 300000u;
    host_set_millis(t);
    gc_set_count(0);
    s_defend_bot = 12;
    enter_screen(SCR_GCHAT, SCR_GIBSON);
    const char *cmd = game == 13 ? "sweep" : "night";
    for (const char *c = cmd; *c; ++c) ui_dispatch(K_CHAR, *c);
    ui_dispatch(K_OK, 0);
    printf("GIB start dg=%d bot=%d\n", s_dg, s_dg_bot);
    for (int r = 0; r < rounds; ++r) {
        uint32_t end = t + 400000u;
        while (t < end && s_dg_state != DGS_WIN && s_dg_state != DGS_LOST) {
            t += 33u; host_set_millis(t); if (!getenv("GIB_NOAI")) dg_ai(); dg_step(t); dg_note();
        }
        printf("GIB round %d game=%s state=%s score=%d miss=%d ms=%u shift=%d lv=%d n=%d alert=%d left=%d pts=%d dex=%d\n", r + 1,
               DG_NAME[s_dg], s_dg_state == DGS_WIN ? "WIN" : s_dg_state == DGS_LOST ? "LOST" : "PLAY", s_dg_score, s_dg_miss,
               (unsigned)s_dg_ms, s_ns_shift, s_sw_lv, s_ns_n, (int)s_ns_alert, s_sw_left, s_sw_pts, dex_have());
        if (r + 1 < rounds) ui_dispatch(K_OK, 0);
    }
    if (rounds > 0) { /* fresh round for the picture */
        ui_dispatch(K_OK, 0);
    }
    const uint32_t stop = t + (uint32_t)warm_ms;
    while (t < stop) {
        t += 33u; host_set_millis(t);
        if (s_dg_state == DGS_PLAY) dg_ai();
        dg_step(t); dg_note();
    }
    if (panel == 3) { const char *d = "dex"; for (const char *c = d; *c; ++c) ui_dispatch(K_CHAR, *c); ui_dispatch(K_OK, 0); }
    printf("GIB shot dg=%s state=%d ms=%u score=%d alert=%d left=%d dex=%d panel=%d\n", s_dg >= 0 ? DG_NAME[s_dg] : "-",
           s_dg_state, (unsigned)s_dg_ms, s_dg_score, (int)s_ns_alert, s_sw_left, dex_have(), s_gc_panel);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* v1019m cycle test: from a Defend entry for this bot, Tab x3 walks the category's three games and wraps;
 * on a LOST stop `/` steps next and `,` steps back. Writes the 6 game ids seen; returns the category. */
int botcorps_host_defend_cycle(int bot, int *ids)
{
    static uint8_t fb[320 * 240];
    host_frame = fb;
    host_set_millis(200000u);
    s_term = 0; botcorps_t0_term_prompt[0] = 0; s_gp_buf[0] = 0; s_fa_on = false; s_act = -1;
    enter_screen(SCR_GIBSON, -1);
    if (bot == 12) {
        /* Category Tab-walk still starts on Gibson Maze; enter_defend itself launches ZAP. */
        s_defend_bot = 12;
        enter_screen(SCR_GCHAT, SCR_GIBSON);
        dg_start_game(12, DG_MAZE);
    } else {
        enter_defend(bot, SCR_GIBSON);
    }
    ids[0] = s_dg;
    for (int k = 1; k <= 3; ++k) { ui_dispatch(K_TAB, 0); ids[k] = dg_on() ? s_dg : -1; }
    s_dg_state = DGS_LOST;
    ui_dispatch(K_RT, 0); ids[4] = dg_on() ? s_dg : -1;
    s_dg_state = DGS_LOST;
    ui_dispatch(K_LT, 0); ids[5] = dg_on() ? s_dg : -1;
    dg_stop();
    return DG_CAT[(bot < 0 || bot > 12) ? 12 : bot];
}
/* route test: which Defend path launches which game; Esc quits the game to the Defend board */
int botcorps_host_defend_route(int which, int *screen, int *esc1, int *esc1_game, int *esc2)
{
    static uint8_t fb[320 * 240];
    host_frame = fb;
    s_term = 0; botcorps_t0_term_prompt[0] = 0; s_gp_buf[0] = 0; s_fa_on = false; s_act = -1;
    switch (which) {
    case 0: enter_screen(SCR_GIBSON, -1); gp_run("defend"); break;                 /* hub: Gibson */
    case 1: enter_screen(SCR_GIBSON, -1); gp_run("defend wifi"); break;            /* hub: Wi-Fi bot */
    case 2: enter_screen(SCR_TERMINAL, SCR_GIBSON); run_cmd("defend airtag"); break; /* Terminal: BLE bot */
    case 3: enter_screen(SCR_GIBSON, -1); gp_run("defend flock"); break;           /* hub: IoT bot */
    case 4: s_scan_bot = 10; enter_screen(SCR_SCAN, SCR_GIBSON); sp_run("defend"); break;  /* scan board: flipper */
    case 5: s_scan_bot = 12; enter_screen(SCR_SCAN, SCR_GIBSON); sp_run("defend"); break;  /* scan board: Gibson */
    default: enter_screen(SCR_GCHAT, SCR_GIBSON); gc_run("defend"); break;         /* game chat prompt */
    }
    *screen = s_screen;
    const int g = dg_on() ? s_dg : -1;
    ui_dispatch(K_ESC, 0);
    *esc1 = s_screen; *esc1_game = dg_on() ? s_dg : -1;
    ui_dispatch(K_ESC, 0);
    *esc2 = s_screen;
    return g;
}

void botcorps_host_scanall(uint8_t *buf)
{
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    s_screen = SCR_TERMINAL;
    s_sheet = false;
    botcorps_lut_term(false);
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
    s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false; s_term_ovl = 0;
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
/* mod pass (items 9/10): scan-box review render. n devices (12 stock, up to SB_N), cursor, UI colour,
 * millis since the scan started. keys (optional) are fed through the real key handler first. */
void botcorps_host_mod_sbox(uint8_t *buf, int band, int n, int cursor, int color, uint32_t since, const char *keys)
{
    botcorps_host_sbox(buf, band, 0, 0);
    s_sb_n = n < 1 ? 1 : (n > SB_N ? SB_N : n);
    s_color = color;
    s_sb_row0 = 0;
    s_cursor = cursor;
    host_set_millis(1000u + since);
    if (keys) for (const char *k = keys; *k; ++k) ui_dispatch((int)(unsigned char)*k, 0);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
    s_sb_n = 12;
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
            else if (c == '\t') { s_key_fn = false; ui_dispatch(K_TAB, 0); }   /* v1019m: Tab (Defend game switch) */
            else if (c == 0x1b) { s_key_fn = false; ui_dispatch(K_ESC, 0); }
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
/* v1019 prompt audit (host only, not a gate item): run every Terminal / game / hub / scan command
 * through its real parser and print where it lands and what it answers. */
static const char *aud_ln(void)
{
    if (!s_log_n) return "(no log)";
    const char *l = log_line(s_log_n - 1);
    return l[0] == '\x01' ? l + 2 : l;
}
void botcorps_host_prompt_audit(void)
{
    static uint8_t fb[320 * 240];
    host_frame = fb;
    /* v1019q (#9c): pass 1 = every Terminal glossary entry bare, pass 2 = with a sample argument (by name). */
    static const char *const TA[][2] = { { "scan", "wifi" }, { "hunt", "drone" }, { "rain", "green" }, { "filter", "wifi" },
        { "ping", "cafe" }, { "info", "cafe" }, { "track", "off" }, { "defend", "flock" }, { "sweep", "ble" }, { "pulse", "cafe" },
        { "dig", "cafe" }, { "hail", "flock" }, { "say", "flock hi" }, { "ask", "drone" }, { "whisper", "hack yo" }, { "link", "wifi" },
        { "mute", "meta" }, { "go", "control" }, { "load", "terminal" } };
    for (int pass = 0; pass < 2; ++pass)
    for (int i = 0; i < CMD_N; ++i) {
        enter_screen(SCR_TERMINAL, SCR_GIBSON);
        s_log_n = 0;
        const char *ta = 0;
        for (unsigned k = 0; k < sizeof(TA) / sizeof(TA[0]); ++k) if (strcmp(TA[k][0], CMDN[i]) == 0) ta = TA[k][1];
        if (pass && !ta) continue;
        if (strcmp(CMDN[i], "ping") == 0 || strcmp(CMDN[i], "info") == 0 || strcmp(CMDN[i], "track") == 0
            || strcmp(CMDN[i], "sniff") == 0 || strcmp(CMDN[i], "pulse") == 0 || strcmp(CMDN[i], "dig") == 0) run_cmd("scan all");
        s_log_n = 0;
        char line[48];
        snprintf(line, sizeof(line), "%s%s%s", CMDN[i], pass ? " " : "", pass ? ta : "");
        run_cmd(line);
        printf("TERM  %-22s -> %-10s dg=%d | %s\n", line, screen_name(s_screen), dg_on() ? s_dg : -1, aud_ln());
        dg_stop();
    }
    /* v1019q (#9c): the listed argument forms that used to fail, plus their effect */
    static const char *const T2[] = { "filter threats", "sniff", "filter strong", "sniff", "filter ble", "sniff", "filter off",
        "sniff", "go scan", "load wifilist", "load blelist", "go bots", "theme", "theme" };
    enter_screen(SCR_TERMINAL, SCR_GIBSON);
    run_cmd("scan all");
    for (unsigned i = 0; i < sizeof(T2) / sizeof(T2[0]); ++i) {
        if (s_screen != SCR_TERMINAL) enter_screen(SCR_TERMINAL, SCR_GIBSON);
        s_log_n = 0;
        const int n0 = 0;
        const int c0 = s_color;
        run_cmd(T2[i]);
        printf("TERM2 %-22s -> %-10s scan_bot=%d color=%d->%d |", T2[i], screen_name(s_screen), s_scan_bot, c0, s_color);
        for (int k = n0 + 1; k < s_log_n; ++k) { const char *l = log_line(k); printf(" %s;", l[0] == '\x01' ? l + 2 : l); }
        printf("\n");
    }
    s_color = 0;
    static const char *const GA[][2] = { { "scan", "wifi" }, { "go", "scan" }, { "load", "game" }, { "defend", "veteran" },
        { "play", "7" }, { "ask", "drone" }, { "hail", "flock" }, { "whisper", "hack" } };
    for (int pass = 0; pass < 2; ++pass)
    for (int i = 0; i < GC_N; ++i) {
        enter_screen(SCR_GCHAT, SCR_GIBSON);
        s_dg = -1;
        s_log_n = 0;
        const char *ga = 0;
        for (unsigned k = 0; k < sizeof(GA) / sizeof(GA[0]); ++k) if (strcmp(GA[k][0], GC_NM[i]) == 0) ga = GA[k][1];
        if (pass && !ga) continue;
        char line[48];
        snprintf(line, sizeof(line), "%s%s%s", GC_NM[i], pass ? " " : "", pass ? ga : "");
        gc_run(line);
        printf("GAME  %-22s -> %-10s act=%d dg=%d panel=%d | %s\n", line, screen_name(s_screen), s_act, dg_on() ? s_dg : -1,
               s_gc_panel, aud_ln());
        dg_stop();
    }
    for (unsigned i = 0; i < sizeof(GP_CMD) / sizeof(GP_CMD[0]); ++i) {
        enter_screen(SCR_GIBSON, -1);
        s_gp_n = 0; s_log_n = 0;
        gp_run(GP_CMD[i]);
        printf("HUB   %-22s -> %-10s dg=%d | %s | %s\n", GP_CMD[i], screen_name(s_screen), dg_on() ? s_dg : -1,
               s_gp_n ? s_gp_log[s_gp_n - 1] : "-", aud_ln());
        dg_stop();
    }
    static const char *const SPL[] = { "scan", "hunt", "info", "gloss", "defend", "terminal", "back", "help", "clear", "wifi", "ble", "bogus" };
    const int scr[3] = { SCR_SCAN, SCR_WIFI_RADAR, SCR_BT_RADAR };
    for (int k = 0; k < 3 + 12; ++k)          /* v1019q: + every bot's own scan screen (per-bot prompt) */
    for (unsigned i = 0; i < sizeof(SPL) / sizeof(SPL[0]); ++i) {
        s_scan_bot = k < 3 ? 12 : k - 3;
        enter_screen(k < 3 ? scr[k] : SCR_SCAN, SCR_GIBSON);
        sp_run(SPL[i]);
        printf("SCAN  %-9s %-10s -> %-10s hunt=%d info=%d gloss=%d | %s | %s\n", k < 3 ? screen_name(scr[k]) : BOTS[k - 3].short_name, SPL[i], screen_name(s_screen),
               s_hunt_dev, s_info_dev, s_gloss, s_sp_has ? s_sp_l1 : "-", s_sp_has ? s_sp_l2 : "-");
    }
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
/* v1019p (field fix) host proof that the games PLAY from the device key path: the idle Defend board (as
 * reached from the hub), then a script typed on the scripted key MATRIX through the REAL botcorps_ui_input
 * -> read_key -> ui_dispatch (plain ; . , / keys, no Fn, exactly like the Cardputer), with play time between
 * keys at the 33 ms tick. Script: printable chars = that key, '\n' Enter, '\t' Tab, '\b' Backspace, '`' Esc,
 * '^x' = Fn+x, '{N}' = play N ms. Prints "PLAY ..." state lines (game, state, score, controls). */
static void host_kb_tap(int x, int y, bool fn)
{
    if (fn) M5Cardputer.Keyboard.press(0, 2);
    M5Cardputer.Keyboard.press(x, y);
    botcorps_ui_input();
    M5Cardputer.Keyboard.release(x, y);
    if (fn) M5Cardputer.Keyboard.release(0, 2);
    botcorps_ui_input();
}
static bool host_kb_find(char c, int *x, int *y, bool *shift)
{
    for (int r = 0; r < 4; ++r)
        for (int k = 0; k < 14; ++k) {
            if (_key_value_map[r][k].value_first == c) { *x = k; *y = r; *shift = false; return true; }
            if (_key_value_map[r][k].value_second == c) { *x = k; *y = r; *shift = true; return true; }
        }
    return false;
}
static void host_play_state(const char *tag)
{
    if (s_dg < 0) {
        printf("PLAY %s t=%u screen=%d game=none prompt='%s' sheet=%d log='%s'\n", tag, (unsigned)millis(), s_screen, s_gp_buf,
               (int)s_sheet, s_log_n ? (log_line(s_log_n - 1)[0] == '\x01' ? log_line(s_log_n - 1) + 2 : log_line(s_log_n - 1)) : "");
        return;
    }
    printf("PLAY %s prompt='%s'\n", tag, s_gp_buf);   /* v1019q: the Defend prompt line while a game runs */
    DGUi u; dg_ui(&u);
    static const char *const ST[4] = { "PLAY", "WIN", "LOST", "PAUSE" };
    printf("PLAY %s t=%u game=%s slot=%d bot=%d state=%s score=%d/%d lost=%d/%d ms=%u stat='%s' ctl[rc=%d fw=%d mz=%d tw=%d ch=%d pin='%s' pr=%d bn=%d st=%d jd=%d]\n",
           tag, (unsigned)millis(), DG_NAME[s_dg], s_dg_slot, s_dg_bot, ST[s_dg_state & 3], u.score, u.target,
           u.lives_lost, u.lives_n, (unsigned)s_dg_ms, u.stat, s_rc_lane, (int)s_fw_px, (int)s_mz_p.want,
           (int)s_tw_side, s_ch_lane, s_pl_in, s_pr_cur, s_bn_at, s_st_cur, s_jd_lane);
}
void botcorps_host_play(uint8_t *buf, int bot, const char *script)
{
    host_frame = buf;
    s_color = 0; s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0; s_gc_panel = 0;
    botcorps_lut_term(false);
    gc_set_count(0);
    s_dg = -1;
    enter_screen(SCR_GCHAT, SCR_GIBSON);     /* the idle Defend board, as the hub's Gibson Enter opens it */
    s_defend_bot = (bot < 0 || bot > 12) ? 12 : bot;
    uint32_t t = millis();
    host_play_state("idle");
    for (const char *p = script; *p; ++p) {
        if (*p == '{') {
            const int ms = atoi(p + 1);
            while (*p && *p != '}') ++p;
            for (int k = 0; k < ms; k += 33) { t += 33u; host_set_millis(t); dg_step(t); dg_note(); }
            host_play_state("after");
            if (!*p) break;
            continue;
        }
        bool fn = false;
        if (*p == '^' && p[1]) { fn = true; ++p; }
        int x = 0, y = 0; bool sh = false;
        if (*p == '\n') { x = 13; y = 2; }
        else if (*p == '\t') { x = 0; y = 1; }
        else if (*p == '\b') { x = 13; y = 0; }
        else if (!host_kb_find(*p, &x, &y, &sh)) continue;
        if (sh) M5Cardputer.Keyboard.press(1, 2);
        host_kb_tap(x, y, fn);
        if (sh) M5Cardputer.Keyboard.release(1, 2);
        t += 33u; host_set_millis(t); dg_step(t); dg_note();
    }
    host_play_state("end");
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* v1019r: host proof driver (review renders, not a gate item). Enter screen `scr` (SCR_SCAN: scan bot `sb`,
 * hunt = Gibson hunt on), then type `script` on the scripted key MATRIX through the REAL botcorps_ui_input ->
 * read_key -> ui_dispatch, 33 ms of real draw-loop time per key ('{N}' = N ms of live loop). Prints state. */
void botcorps_host_kb(uint8_t *buf, int scr, int sb, int hunt, const char *script)
{
    host_frame = buf;
    s_sheet = false; s_fa_on = false; s_act = -1; s_gp_buf[0] = 0; s_gc_panel = 0;
    s_term = 0; botcorps_t0_term_prompt[0] = 0;
    botcorps_lut_term(false);
    if (scr == SCR_SCAN) { s_scan_bot = sb; s_gib_hunt = hunt != 0; s_gib_hunt_keep = hunt != 0; }
    if (scr == SCR_GCHAT) { s_dg = -1; s_defend_bot = (sb < 0 || sb > 12) ? 12 : sb; }
    enter_screen(scr, SCR_GIBSON);
    if (scr == SCR_GCHAT) s_defend_bot = (sb < 0 || sb > 12) ? 12 : sb;
    uint32_t t = millis();
    for (const char *p = script; *p; ++p) {
        if (*p == '{') {
            const int ms = atoi(p + 1);
            while (*p && *p != '}') ++p;
            for (int k = 0; k < ms; k += 33) { t += 33u; host_set_millis(t); sp_auto_poll(); if (dg_on()) { dg_step(t); dg_note(); } else if (s_screen == SCR_SCAN) bot_hunt_step(t); }
            if (!*p) break;
            continue;
        }
        if (*p == '%' && p[1]) {                  /* '%c' = hold key c for 1000 ms (auto-repeat proof) */
            ++p;
            int hx = 0, hy = 0; bool hs = false;
            if (!host_kb_find(*p, &hx, &hy, &hs)) continue;
            M5Cardputer.Keyboard.press(hx, hy);
            int n = 0;
            for (int k = 0; k < 1000; k += 10) {
                t += 10u; host_set_millis(t);
                const int before = s_ns_gx * 100 + s_ns_gy + s_pr_cur * 7 + s_rc_lane * 3;
                botcorps_ui_input();
                const int after = s_ns_gx * 100 + s_ns_gy + s_pr_cur * 7 + s_rc_lane * 3;
                n += before != after;
                if (dg_on() && k % 30 == 0) { dg_step(t); dg_note(); }
            }
            M5Cardputer.Keyboard.release(hx, hy);
            botcorps_ui_input();
            printf("KB hold '%c' 1000ms -> %d moves (night guard %d,%d)\n", *p, n, s_ns_gx, s_ns_gy);
            continue;
        }
        bool fn = false;
        if (*p == '^' && p[1]) { fn = true; ++p; }
        int x = 0, y = 0; bool sh = false;
        if (*p == '\n') { x = 13; y = 2; }
        else if (*p == '\t') { x = 0; y = 1; }
        else if (*p == '\b') { x = 13; y = 0; }
        else if (!host_kb_find(*p, &x, &y, &sh)) continue;
        if (sh) M5Cardputer.Keyboard.press(1, 2);
        host_kb_tap(x, y, fn);
        if (sh) M5Cardputer.Keyboard.release(1, 2);
        t += 33u; host_set_millis(t); sp_auto_poll(); if (dg_on()) { dg_step(t); dg_note(); }
    }
    printf("KB screen=%d scan_bot=%d hunt=%d sp_buf='%s' sp_has=%d l1='%s' l2='%s' gp='%s' term='%s' dg=%d\n", s_screen, s_scan_bot,
           (int)s_gib_hunt, s_sp_buf, (int)s_sp_has, s_sp_l1, s_sp_l2, s_gp_buf, botcorps_t0_term_prompt, s_dg);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
/* gate.sh: scan board for one bot type (12 = Gibson); settings with a given cursor */
/* #5 host renders: the device runs the hunt continuously, so a one-shot host frame shows the board
 * after 6 s of live play from a fresh (mock: empty, 0 pips) start, stepped at the 33 ms patch rate. */
static void bh_host_warm(int bot)
{
    const uint32_t t1 = millis();
    s_scan_bot = bot;
    s_bh_who = -2;
    bot_hunt_step(t1 - 6000u);
    for (uint32_t t = t1 - 6000u + 33u; t < t1; t += 33u) bot_hunt_step(t);
}
void botcorps_host_scan_n(uint8_t *buf, int bot)
{
    s_gib_hunt = false;
    bh_host_warm(bot);
    botcorps_host_enter(buf, SCR_SCAN);
}
/* mod item 10: Gibson drop-down review render (lane, device or -1, UI colour) */
void botcorps_host_mod_gibdrop(uint8_t *buf, int lane, int dev, int color)
{
    s_gib_hunt = false;
    s_color = color;
    s_gd_lane = lane; s_gd_dev = dev; s_gd_top = 0; s_gd_row0 = 0;
    bh_host_warm(12);
    botcorps_host_enter(buf, SCR_SCAN);
}
/* #5: Gibson's board with hunt mode on (mock gibHuntStart) */
void botcorps_host_scan_gibhunt(uint8_t *buf)
{
    s_gib_hunt = true;
    bh_host_warm(12);
    s_gib_hunt_keep = true;
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
/* v1019n-mod #11: full INT clears (fillScreen + whole-card redraw) while cycling UI COLOR through every
 * theme with Enter on the real draw loop. 0 = no INT flash on a colour change. */
int host_int_clears = 0;
int botcorps_host_int_flash(uint8_t *buf, int presses, int *ext_bad)
{
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = 0;
    host_set_millis(30000u);
    botcorps_ui_draw();                         /* settle: the screen-entry paint is not counted */
    botcorps_ui_draw();
    const int n0 = host_int_clears;
    static char keys[64];
    int k = 0;
    for (; k < presses && k < 63; ++k) keys[k] = '\n';
    keys[k] = 0;
    long pr = 0, pu = 0, rc = 0, wo = 0;
    *ext_bad = botcorps_host_sim(presses * 300 + 600, keys, &pr, &pu, &rc, &wo);
    const int n = host_int_clears - n0;
    s_color = 0;
    return n;
}
static int s_cp_cycle = 0;
int botcorps_host_setfn(uint8_t *buf, int cursor, char *before, char *after, int *nvs)
{
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = cursor;
    const int sv[7] = { s_color, s_mix, s_shape, botcorps_rain_mode(), s_bg, s_bg_motion, 0 };
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
    s_bg = sv[4]; s_bg_motion = sv[5]; (void)sv[6]; botcorps_rain_set_bg(s_bg);
    botcorps_rain_set_mix(s_mix); botcorps_rain_set_shape(s_shape);
    return ok;
}
int botcorps_host_cp_cycle(void) { return s_cp_cycle; }
/* v1017a gate: terminal commands button. v1017c (#3): from board 1 Down x3 must land on the prompt
 * (and only there), one more Down on the button; Enter must open the commands menu, Esc returns to
 * the button, Up goes button -> prompt -> boards (board 9, above the button). */
int botcorps_host_cmd_btn(uint8_t *buf, int *reach, int *opened, int *esc_back, int *up_leaves, int *menu_runs)
{
    botcorps_host_enter(buf, SCR_TERMINAL);
    s_term = 1;
    botcorps_host_seq(buf, "\x02\x02\x02");
    const bool at_prompt = s_term_prompt && !s_cmd_btn && !s_focus_cmds && s_screen == SCR_TERMINAL;
    botcorps_host_seq(buf, "\x02");
    *reach = at_prompt && s_cmd_btn && !s_term_prompt && !s_focus_cmds && s_screen == SCR_TERMINAL;
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
    const bool up_prompt = s_term_prompt && !s_cmd_btn && !s_focus_cmds;
    botcorps_host_seq(buf, "\x01");
    *up_leaves = up_prompt && !s_term_prompt && !s_cmd_btn && !s_focus_cmds && s_term == 9;
    /* menu really runs a command: open, pick chip 'help' (index 2), Enter -> log grows */
    botcorps_host_seq(buf, "\x02\x02\n\x04\x04");
    const int n0 = s_log_next;
    botcorps_host_seq(buf, "\n");
    *menu_runs = s_log_next != n0;
    return s_screen;
}
/* v1017c gate K1 (#2/#3): real read_key on a scripted key matrix. Rollover typing (each key pressed
 * before the previous is released), a press + release landing in one scan, Backspace, and an Fn
 * arrow chord released Fn-first. The prompt must read exactly what was typed, and each arrow chord
 * must move focus exactly one stop. Returns 1 on pass; out = prompt text. */
static void k1_pos(char c, int *x, int *y)
{
    for (int r = 0; r < 4; ++r) for (int k = 0; k < 14; ++k)
        if (_key_value_map[r][k].value_first == c) { *x = k; *y = r; return; }
    *x = 0; *y = 0;
}
static void k1_ev(char c, bool down, bool scan = true)
{
    int x, y;
    k1_pos(c, &x, &y);
    if (down) M5Cardputer.Keyboard.press(x, y); else M5Cardputer.Keyboard.release(x, y);
    if (scan) botcorps_ui_input();
}
int botcorps_host_k1(uint8_t *buf, char *out, int n, int *focus_ok)
{
    botcorps_host_enter(buf, SCR_TERMINAL);
    s_term = 9;
    botcorps_t0_term_prompt[0] = 0;
    M5Cardputer.Keyboard.list.clear();
    s_kprev_n = 0;
    /* "scan wifi" with overlapping presses */
    const char *w = "scan wifi";
    for (int i = 0; w[i]; ++i) {
        if (i == 7) {                /* 'i' released and 'f' pressed in the same scan (count equal) */
            k1_ev(w[6], false, false);
            k1_ev(w[7], true);
            continue;
        }
        k1_ev(w[i], true);
        if (i > 0 && w[i - 1] != w[i]) k1_ev(w[i - 1], false);
    }
    k1_ev('i', false);
    /* a stray x then Backspace while x is still held */
    k1_ev('x', true);
    k1_ev((char)KEY_BACKSPACE, true);
    k1_ev('x', false);
    k1_ev((char)KEY_BACKSPACE, false);
    const bool f0 = s_term_prompt && !s_cmd_btn;
    /* Fn + . (Down) released Fn first: exactly one stop (prompt -> button) */
    k1_ev((char)KEY_FN, true);
    k1_ev('.', true);
    k1_ev((char)KEY_FN, false);
    k1_ev('.', false);
    const bool f1 = s_cmd_btn && !s_term_prompt && !s_focus_cmds;
    /* plain ; (Up) with text in the prompt is an arrow, not a typed ';' */
    k1_ev(';', true);
    k1_ev(';', false);
    const bool f2 = s_term_prompt && !s_cmd_btn;
    snprintf(out, n, "%s", botcorps_t0_term_prompt);
    *focus_ok = f0 && f1 && f2;
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
    return strcmp(botcorps_t0_term_prompt, "scan wifi") == 0 && *focus_ok;
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
void botcorps_host_set_color(int c){ s_color = (c<0||c>=THEME_N)?0:c; }
/* v1019n-mod review renders: BACKGROUND style + motion (PWR LED) before host_rain_init warms the rain */
void botcorps_host_set_bg(int style, int motion) { s_bg = ((style % BGS_NUM) + BGS_NUM) % BGS_NUM; s_bg_motion = motion ? 1 : 0; botcorps_rain_set_bg(s_bg); }
int botcorps_host_theme_n(void) { return THEME_N; }
void botcorps_host_key(int key, char c) { s_key_fn = (c == 1); ui_dispatch(key, c == 1 ? 0 : c); }
void botcorps_host_paint(uint8_t *buf) { host_frame = buf; paint_ext(Gfx()); }
/* v1017c S3: the same frame painted the way the device BANDS present does it (two 120-row slices,
 * Gfx oy = 0 / 120, each clipped to the 320x120 sprite) vs one full paint. Returns differing px. */
int botcorps_host_slice_diff(void)
{
    extern int host_slice_oy, host_slice_h;
    static uint8_t full[320 * 240], tmp[320 * 240], sl[320 * 240];
    uint8_t *saved = host_frame;
    host_frame = full;
    memset(full, PAL_BLACK, sizeof full);
    paint_ext(Gfx());
    for (int s = 0; s < 2; ++s) {
        host_slice_oy = s * 120;
        host_slice_h = 120;
        host_frame = tmp;
        memset(tmp, PAL_BLACK, sizeof tmp);
        paint_ext(Gfx());
        memcpy(sl + s * 120 * 320, tmp, 120 * 320);
    }
    host_slice_oy = 0;
    host_slice_h = 0;
    host_frame = saved;
    int bad = 0;
    for (int i = 0; i < 320 * 240; ++i) bad += full[i] != sl[i];
    if (bad && getenv("SLICE_DUMP")) {   /* first failing frame only: full | sliced */
        FILE *f = fopen(getenv("SLICE_DUMP"), "wb");
        if (f) { fwrite(full, 1, sizeof full, f); fwrite(sl, 1, sizeof sl, f); fclose(f); }
        unsetenv("SLICE_DUMP");
    }
    return bad;
}
#endif
