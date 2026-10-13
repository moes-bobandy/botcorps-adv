/*
 * EXT is the mock's screen: icon buttons, terminal, rain behind chrome.
 * INT is the selected bot's ID card. No text menu and no hint line.
 * A screen change, and an overlay opening or closing, fills the panel
 * before the new chrome is drawn.
 */
#include "ui/botcorps_ui.h"

#include "display/botcorps_display.h"
#include "probe/botcorps_probe.h"
#include "rain/botcorps_rain.h"

#include <Preferences.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern char botcorps_t0_term_prompt[80];
extern char botcorps_t0_term_hist[20][26];
extern char botcorps_t0_term_log[2048];
extern char botcorps_t0_term_found[12][28];

static const uint16_t COL_BG    = 0x0862;
static const uint16_t COL_FIELD = 0x0041;
static const uint16_t COL_BAR   = 0x10C4;
static const uint16_t COL_LINE  = 0x29C8;
static const uint16_t COL_INK   = 0xE77E;
static const uint16_t COL_MUTED = 0x8CD4;
static const uint16_t COL_AMBER = 0xF5A9;
static const uint16_t COL_SEL   = 0x1105;
static const uint16_t COL_WHITE = 0xFFFF;
static const uint16_t COL_DEEP  = 0x0841;
static const uint16_t COL_IVORY = 0xF79C;
static const uint16_t COL_OK    = 0x8FF3;
static const uint16_t COL_HOLD  = 0xE38E;

static const uint16_t ACCENT_C[3] = { 0x4E7B, 0xF5A9, 0x5FF8 };
static const char *const ACCENT_N[3] = { "CYAN", "AMBER", "MINT" };
static const char *const MIX_N[3] = { "BOTS", "CREW", "DEVICES" };
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
    uint16_t c;
    uint8_t radio; /* 0 none, 1 wifi, 2 ble */
    uint8_t hold;
};

static const bot_t BOTS[12] = {
    { "wifibot", "wifi", "wi-fi", 0x5D3F, 1, 0 },
    { "bluetoothbot", "bluetooth", "ble", 0x4E7F, 2, 0 },
    { "deauthbot", "deauth", "deauth", 0x5D3F, 0, 0 },
    { "hackbot", "hack", "hack", 0x7711, 0, 0 },
    { "flockbot", "flock", "flock", 0x9F0B, 0, 0 },
    { "dronebot", "drone", "drone", 0xFB72, 0, 0 },
    { "airtagbot", "airtag", "air tag", 0xAC7F, 0, 0 },
    { "keyfobbot", "keyfob", "keyfob", 0x36B8, 0, 0 },
    { "metabot", "meta", "meta", 0x9F0B, 0, 0 },
    { "skimmerbot", "skimmer", "skimmer", 0x36B8, 0, 1 },
    { "flipperbot", "flipper", "flipper", 0xFC6B, 0, 0 },
    { "samsungtagbot", "samsung", "samsung", 0xBD3F, 0, 0 },
};

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
static const char *const CMD_ABOUT[12] = {
    "sweep", "named sweep", "command list", "board status", "colour", "which heard",
    "wipe log", "found device", "device detail", "pin device", "heard list", "this sheet"
};

static const int BAR_H = 22;
static const int LOG_ROWS = 24;
static const int LOG_W = 80;

static int s_screen = SCR_RAIN;
static int s_cursor = 0;
static int s_bot = 2;
static int s_term = 2;
static int s_color = 0;
static int s_mix = 1;
static int s_saver = 0;
static int s_saver_on = 1;
static int s_idle = 1;
static int s_ret = -1;
static bool s_dirty = true;
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

static const char *const NVS_NS = "botcorps";
static const char *const NVS_KEY = "rain_col";

static uint16_t accent(void)
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
    return s_screen != SCR_WIFI_LIST && s_screen != SCR_BT_LIST && s_screen != SCR_DIAG;
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

static void txt(lgfx::LovyanGFX &g, const char *s, int x, int y, uint16_t col,
                lgfx::textdatum_t dat)
{
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(col);
    g.setTextDatum(dat);
    g.drawString(s ? s : "", x, y);
    g.setTextDatum(lgfx::textdatum::top_left);
}

static void plate(lgfx::LovyanGFX &g, int x, int y, int w, int h,
                  uint16_t fill, uint16_t edge)
{
    g.fillRect(x, y, w, h, fill);
    g.drawRect(x, y, w, h, edge);
}

static void draw_blob(lgfx::LovyanGFX &g, int x, int y, int s, uint16_t c)
{
    g.fillCircle(x, y, s / 2, c);
    g.fillRect(x - 2, y - 1, 2, 2, COL_DEEP);
    g.fillRect(x + 1, y - 1, 2, 2, COL_DEEP);
}

static void draw_icon(lgfx::LovyanGFX &g, int x, int y, int id, uint16_t col)
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

static void draw_btn(lgfx::LovyanGFX &g, int x, int y, int id, bool sel)
{
    const uint16_t edge = sel ? accent() : COL_LINE;
    const uint16_t fill = sel ? COL_SEL : COL_DEEP;
    plate(g, x, y, BS, BS, fill, edge);
    if (sel) g.drawRect(x + 1, y + 1, BS - 2, BS - 2, accent());
    draw_icon(g, x, y, id, sel ? accent() : COL_MUTED);
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
        cover_add(146, 206, BS, BS);
        break;
    case SCR_GIBSON:
        cover_add(108, 28, 104, 120);
        cover_add(92, 206, BS, BS);
        cover_add(128, 206, BS, BS);
        cover_add(164, 206, BS, BS);
        cover_add(200, 206, BS, BS);
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
        cover_add(12, 133, BS, BS);
        cover_add(44, 133, BS, BS);
        cover_add(248, 133, BS, BS);
        cover_add(280, 133, BS, BS);
        break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
        cover_reset(true);
        cover_add(14, 72, BS, BS);
        cover_add(14, 106, BS, BS);
        cover_add(14, 140, BS, BS);
        cover_add(70, 210, 180, 18);
        break;
    case SCR_BOT_RADAR:
    case SCR_SCAN:
        cover_reset(true);
        cover_add(14, 90, BS, BS);
        cover_add(14, 124, BS, BS);
        cover_add(70, 210, 180, 18);
        break;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        break;
    case SCR_CONTROL:
        for (int i = 0; i < 7; ++i) cover_add(8, 2 + i * 22, 304, 20);
        cover_add(14, 200, BS, BS);
        cover_add(48, 200, BS, BS);
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
    case SCR_GIBSON: return 4;
    case SCR_WIFI:
    case SCR_BT:
    case SCR_BOT: return 4;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: return 3;
    case SCR_BOT_RADAR:
    case SCR_SCAN: return 2;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST: return 4;
    case SCR_CONTROL: return 9;
    case SCR_DIAG: return 1;
    default: return 0;
    }
}

static void enter_screen(int id, int ret)
{
    if (id < 0) return;
    const char *from = screen_name(s_screen);
    s_ret = ret;
    s_screen = id;
    s_cursor = 0;
    s_sheet = false;
    s_focus_cmds = false;
    s_dirty = true;
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
    char echo[80];
    snprintf(echo, sizeof(echo), "> %s%s%s", cmd, arg ? " " : "", arg ? arg : "");
    log_add(echo);

    if (strcmp(cmd, "help") == 0) {
        log_add("scan hunt help bots rain filter");
        log_add("clear ping info track sniff go");
        log_add("colours green cyan blue purple amber white");
    } else if (strcmp(cmd, "bots") == 0) {
        log_add("12 boards");
        for (int i = 0; i < 12; ++i) {
            char row[80];
            snprintf(row, sizeof(row), "  %s %s", BOTS[i].short_name, BOTS[i].hold ? "hold" : "ok");
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
            s_dirty = true;
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
        s_focus_cmds = true;
    } else {
        log_add("command not found");
    }
    s_dirty = true;
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

static void paint_boards(lgfx::LovyanGFX &g)
{
    for (int i = 0; i < 12; ++i) {
        int x, y;
        term_xy(i, &x, &y);
        const bool sel = (i == s_term) && !s_focus_cmds && !s_sheet;
        const bool light = BOTS[i].radio != 0;
        const uint16_t base = light ? (sel ? 0xF79C : 0xD69A) : (sel ? 0x18C3 : 0x1082);
        const uint16_t edge = sel ? accent() : COL_LINE;
        plate(g, x, y, 72, 46, base, edge);
        if (sel) g.drawRect(x - 2, y - 2, 76, 50, accent());
        g.fillRect(x + 23, y + 12, 26, 16, light ? 0x1082 : 0x0841);
        draw_blob(g, x + 36, y + 20, 10, BOTS[i].c);
        g.fillCircle(x + 8, y + 6, 2, BOTS[i].hold ? COL_HOLD : COL_OK);
        const uint16_t ink = light ? 0x1082 : COL_INK;
        txt(g, BOTS[i].short_name, x + 36, y + 34, ink, lgfx::textdatum::top_center);
    }
}

static void paint_palette(lgfx::LovyanGFX &g)
{
    g.fillRect(6, 168, 308, 40, COL_DEEP);
    for (int n = 0; n < 12; ++n) {
        const int x = 8 + (n % 6) * 51;
        const int y = 170 + (n / 6) * 18;
        const bool sel = n == s_cmd_sel;
        plate(g, x, y, 49, 16, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt(g, CMDS[n], x + 24, y + 4, sel ? COL_WHITE : COL_MUTED, lgfx::textdatum::top_center);
    }
}

static void paint_sheet(lgfx::LovyanGFX &g)
{
    plate(g, 8, 21, 304, 142, COL_BG, accent());
    const int vis = 8;
    int top = s_sheet_i - 3;
    if (top < 0) top = 0;
    if (top > 12 - vis) top = 12 - vis;
    for (int r = 0; r < vis; ++r) {
        const int i = top + r;
        const int y = 28 + r * 16;
        const bool sel = i == s_sheet_i;
        if (sel) g.fillRect(14, y, 292, 15, COL_SEL);
        txt(g, CMDS[i], 18, y + 3, sel ? accent() : COL_INK, lgfx::textdatum::top_left);
        txt(g, CMD_ABOUT[i], 92, y + 3, COL_MUTED, lgfx::textdatum::top_left);
    }
}

static void paint_prompt(lgfx::LovyanGFX &g)
{
    plate(g, 4, 167, 312, 72, COL_DEEP, accent());
    const int n = s_log_n < 5 ? s_log_n : 5;
    const int start = s_log_n - n;
    for (int k = 0; k < n; ++k) {
        txt(g, log_line(start + k), 10, 171 + k * 9, COL_INK, lgfx::textdatum::top_left);
    }
    const int y = 171 + 5 * 9;
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(accent());
    g.setCursor(10, y);
    g.print("user@botcorps:~$ ");
    g.setTextColor(COL_WHITE);
    g.print(botcorps_t0_term_prompt);
    const int cx = g.getCursorX() + 2;
    if ((millis() / 530u) % 2u == 0) g.fillRect(cx, y, 5, 8, accent());
    if (s_focus_cmds && !s_sheet) paint_palette(g);
}

static void paint_terminal(lgfx::LovyanGFX &g)
{
    g.fillRect(0, 0, 320, 16, COL_DEEP);
    g.drawFastHLine(0, 16, 320, accent());
    txt(g, "terminal", 8, 4, accent(), lgfx::textdatum::top_left);
    txt(g, "12", 76, 4, COL_MUTED, lgfx::textdatum::top_left);
    if (s_track_name[0]) txt(g, s_track_name, 120, 4, accent(), lgfx::textdatum::top_left);
    paint_boards(g);
    paint_prompt(g);
    if (s_sheet) paint_sheet(g);
}

static void paint_home(lgfx::LovyanGFX &g, int bot_index)
{
    const bot_t &b = BOTS[bot_index];
    const int phase = (int)((millis() / 40u) % 14u);
    g.fillCircle(160, 74, 48, COL_DEEP);
    g.drawCircle(160, 74, 46, accent());
    g.drawCircle(160, 74, 53, COL_LINE);
    g.drawCircle(160, 74, 47 + phase, accent());
    draw_blob(g, 160, 74, 28, b.c);
    const int ids[4] = { ICO_RADAR, ICO_TERM, ICO_CTRL, ICO_BACK };
    const int xs[4] = { 12, 44, 248, 280 };
    for (int i = 0; i < 4; ++i) draw_btn(g, xs[i], 133, ids[i], s_cursor == i);
    if (s_screen == SCR_BOT) {
        g.fillTriangle(8, 74, 20, 66, 20, 82, COL_MUTED);
        g.fillTriangle(312, 74, 300, 66, 300, 82, COL_MUTED);
    }
}

static void paint_radar(lgfx::LovyanGFX &g, bool with_list)
{
    const int cx = 160;
    const int cy = 108;
    const int r = 88;
    g.fillCircle(cx, cy, r + 2, COL_BG);
    g.fillCircle(cx, cy, r, 0x1105);
    g.drawCircle(cx, cy, r, accent());
    g.drawCircle(cx, cy, r / 2, COL_LINE);
    g.drawFastHLine(cx - r, cy, r * 2, COL_LINE);
    g.drawFastVLine(cx, cy - r, r * 2, COL_LINE);
    const float ang = (float)(millis() % 1700u) / 1700.0f * 6.283185f;
    for (int k = 0; k < 6; ++k) {
        const float a = ang - (float)k * 0.18f;
        const int x1 = cx + (int)(cosf(a) * (float)(r - 4));
        const int y1 = cy + (int)(sinf(a) * (float)(r - 4));
        g.drawLine(cx, cy, x1, y1, accent());
    }
    plate(g, 70, 210, 180, 18, COL_BG, COL_LINE);
    if (with_list) {
        draw_btn(g, 14, 72, ICO_TRACK, s_cursor == 0);
        draw_btn(g, 14, 106, ICO_LIST, s_cursor == 1);
        draw_btn(g, 14, 140, ICO_BACK, s_cursor == 2);
    } else {
        draw_btn(g, 14, 90, ICO_TRACK, s_cursor == 0);
        draw_btn(g, 14, 124, ICO_BACK, s_cursor == 1);
    }
    if (s_track && s_cursor == 0) g.fillCircle(28, with_list ? 72 : 90, 3, accent());
}

static void paint_list(lgfx::LovyanGFX &g, bool wifi)
{
    g.fillScreen(COL_BG);
    const char *const *names = wifi ? NEAR_WIFI : NEAR_BLE;
    const char *const *rssi = wifi ? NEAR_WIFI_R : NEAR_BLE_R;
    const uint8_t *bars = wifi ? NEAR_WIFI_B : NEAR_BLE_B;
    plate(g, 8, 8, 304, 22, COL_BG, COL_LINE);
    txt(g, wifi ? "WI-FI" : "BLE", 16, 14, accent(), lgfx::textdatum::top_left);
    txt(g, "3", 300, 14, COL_MUTED, lgfx::textdatum::top_right);
    for (int i = 0; i < 3; ++i) {
        const int y = 40 + i * 50;
        const bool sel = s_cursor == i;
        plate(g, 12, y, 296, 44, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt(g, names[i], 24, y + 8, COL_INK, lgfx::textdatum::top_left);
        txt(g, rssi[i], 280, y + 8, COL_AMBER, lgfx::textdatum::top_right);
        for (int k = 0; k < 4; ++k) {
            const int bh = (k + 1) * 4;
            g.fillRect(250 + k * 8, y + 36 - bh, 5, bh, k < bars[i] ? accent() : COL_LINE);
        }
    }
    draw_btn(g, 146, 206, ICO_BACK, s_cursor == 3);
}

static void paint_cp_value(lgfx::LovyanGFX &g, int index, int y)
{
    char value[16];
    value[0] = 0;
    if (index == 0) snprintf(value, sizeof(value), "%s", ACCENT_N[s_color]);
    else if (index == 1) snprintf(value, sizeof(value), "%s", MIX_N[s_mix]);
    else if (index == 2) snprintf(value, sizeof(value), "%s", botcorps_rain_mode() == 0 ? "MIXED" : "SOLID");
    else if (index == 3) snprintf(value, sizeof(value), "%s", RAIN_WORD[botcorps_rain_mode()]);
    else if (index == 4) snprintf(value, sizeof(value), "%s", SAVER_N[s_saver]);
    else if (index == 5) snprintf(value, sizeof(value), "%s", s_saver_on ? "ON" : "OFF");
    else snprintf(value, sizeof(value), "%d S", IDLE_V[s_idle]);
    txt(g, value, 300, y + 4, COL_INK, lgfx::textdatum::top_right);
    if (index == 3) {
        if (botcorps_rain_mode() <= 0) {
            for (int k = 0; k < 4; ++k) g.fillRect(16 + k * 3, y + 12, 3, 6, botcorps_rain_palette(k));
        } else {
            g.fillRect(16, y + 12, 12, 6, botcorps_rain_solid());
        }
    }
}

static void paint_control(lgfx::LovyanGFX &g)
{
    const char *labels[7] = {
        "UI COLOR", "BACKGROUND", "RAIN MODE", "RAIN COLOUR",
        "SCREEN SAVER", "SAVER", "IDLE TIME"
    };
    for (int i = 0; i < 7; ++i) {
        const int y = 2 + i * 22;
        const bool on = s_cursor == i;
        plate(g, 8, y, 304, 20, on ? COL_SEL : COL_DEEP, on ? accent() : COL_LINE);
        txt(g, labels[i], 14, y + 4, on ? accent() : COL_MUTED, lgfx::textdatum::top_left);
        paint_cp_value(g, i, y);
    }
    draw_btn(g, 14, 200, ICO_TERM, s_cursor == 7);
    draw_btn(g, 48, 200, ICO_BACK, s_cursor == 8);
}

static void paint_gibson(lgfx::LovyanGFX &g)
{
    g.fillCircle(160, 88, 36, COL_DEEP);
    g.drawCircle(160, 88, 36, accent());
    g.drawCircle(160, 88, 42, COL_LINE);
    g.fillRoundRect(148, 36, 24, 10, 2, COL_IVORY);
    draw_blob(g, 160, 92, 26, COL_IVORY);
    g.fillRect(150, 88, 20, 4, COL_DEEP);
    const int ids[4] = { ICO_TERM, ICO_CTRL, ICO_RADAR, ICO_DIAG };
    const int xs[4] = { 92, 128, 164, 200 };
    for (int i = 0; i < 4; ++i) draw_btn(g, xs[i], 206, ids[i], s_cursor == i);
}

static void paint_rain_btn(lgfx::LovyanGFX &g)
{
    draw_btn(g, 146, 206, ICO_PLAY, true);
}

static void paint_ext(lgfx::LovyanGFX &g)
{
    g.clearClipRect();
    g.setTextWrap(false, false);
    if (s_screen == SCR_DIAG) {
        botcorps_probe_paint(g);
        draw_btn(g, 146, 206, ICO_BACK, true);
        return;
    }
    if (!wants_rain()) g.fillScreen(COL_BG);
    else g.fillScreen(COL_FIELD);
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

static void tick_radar(lgfx::LovyanGFX &g)
{
    const int cx = 160;
    const int cy = 108;
    const int r = 88;
    g.setClipRect(70, 18, 180, 180);
    g.fillCircle(cx, cy, r, 0x1105);
    g.drawCircle(cx, cy, r, accent());
    g.drawCircle(cx, cy, r / 2, COL_LINE);
    g.drawFastHLine(cx - r, cy, r * 2, COL_LINE);
    g.drawFastVLine(cx, cy - r, r * 2, COL_LINE);
    const float ang = (float)(millis() % 1700u) / 1700.0f * 6.283185f;
    for (int k = 0; k < 6; ++k) {
        const float a = ang - (float)k * 0.18f;
        const int x1 = cx + (int)(cosf(a) * (float)(r - 4));
        const int y1 = cy + (int)(sinf(a) * (float)(r - 4));
        g.drawLine(cx, cy, x1, y1, accent());
    }
    g.clearClipRect();
}

static void tick_home(lgfx::LovyanGFX &g, int bot_index)
{
    g.setClipRect(110, 24, 100, 100);
    g.fillCircle(160, 74, 64, COL_FIELD);
    g.fillCircle(160, 74, 48, COL_DEEP);
    g.drawCircle(160, 74, 46, accent());
    const int phase = (int)((millis() / 40u) % 14u);
    g.drawCircle(160, 74, 47 + phase, accent());
    draw_blob(g, 160, 74, 28, BOTS[bot_index].c);
    g.clearClipRect();
}

static void tick_gibson(lgfx::LovyanGFX &g)
{
    const int bob = (int)((millis() / 700u) % 3u) - 1;
    g.setClipRect(120, 40, 80, 90);
    g.fillCircle(160, 88, 36, COL_DEEP);
    g.drawCircle(160, 88, 36, accent());
    draw_blob(g, 160, 92 + bob, 26, COL_IVORY);
    g.fillRect(150, 88 + bob, 20, 4, COL_DEEP);
    g.clearClipRect();
}

static void tick_ext(lgfx::LovyanGFX &g)
{
    if (s_screen == SCR_GIBSON) tick_gibson(g);
    else if (s_screen == SCR_WIFI) tick_home(g, 0);
    else if (s_screen == SCR_BT) tick_home(g, 1);
    else if (s_screen == SCR_BOT) tick_home(g, s_bot);
    else if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR ||
             s_screen == SCR_BOT_RADAR || s_screen == SCR_SCAN) {
        tick_radar(g);
    } else if (s_screen == SCR_TERMINAL && !s_sheet) {
        const int phase = (int)((millis() / 530u) % 2u);
        if (phase != s_prompt_phase) {
            s_prompt_phase = phase;
            g.setClipRect(4, 167, 312, 72);
            paint_prompt(g);
            g.clearClipRect();
        }
    }
}

static void paint_bars(lgfx::LovyanGFX &g, int x, int y, int lit, int n, uint16_t col)
{
    for (int i = 0; i < n; ++i) {
        g.fillRect(x + i * 6, y, 5, 6, i < lit ? col : COL_LINE);
    }
}

static void paint_int(lgfx::LGFX_Device &menu)
{
    const int width = menu.width();
    const int height = menu.height();
    fmt_times();
    menu.fillScreen(0x0841);
    menu.fillRect(0, 0, width, BAR_H, COL_BAR);
    menu.drawFastHLine(0, BAR_H, width, COL_LINE);
    menu.setFont(&fonts::Font0);
    menu.setTextSize(2, 2);
    menu.setTextColor(COL_WHITE);
    menu.setCursor(6, 3);
    menu.print("BOTcorps");
    txt(menu, s_clk, width - 4, 4, COL_INK, lgfx::textdatum::top_right);

    const botcorps_probe_t *pr = botcorps_probe_get();
    if (pr && strcmp(pr->ext_st, "fail") == 0) {
        menu.setTextSize(2, 2);
        menu.setTextColor(COL_WHITE, 0x0000);
        menu.setCursor(8, 40);
        menu.print("EXT fail");
        return;
    }

    const int bi = card_bot();
    const bool gib = bi < 0;
    const uint16_t col = gib ? COL_IVORY : BOTS[bi].c;
    const char *name = gib ? "gibsonbot" : BOTS[bi].id;
    const char *target = gib ? "ALL BOTS" : BOTS[bi].target;
    plate(menu, 4, 28, width - 8, height - 34, 0x1082, COL_LINE);
    menu.fillRect(12, 36, 52, 52, COL_DEEP);
    if (gib) {
        draw_blob(menu, 38, 62, 22, COL_IVORY);
        menu.fillRect(28, 58, 20, 3, COL_DEEP);
    } else {
        draw_blob(menu, 38, 62, 22, col);
    }
    txt(menu, name, 72, 36, col, lgfx::textdatum::top_left);
    txt(menu, "LV 01", 72, 48, COL_MUTED, lgfx::textdatum::top_left);
    txt(menu, target, 72, 64, COL_INK, lgfx::textdatum::top_left);
    const int idx = gib ? 5 : bi;
    const int hits = 8 + (idx * 3) % 10;
    const int sig = 40 + (idx * 7) % 50;
    const uint32_t sec = (millis() - s_boot_ms) / 1000u;
    const int up_lit = sec > 90 ? 16 : (int)(sec * 16 / 90);
    txt(menu, "HITS", 12, 96, COL_MUTED, lgfx::textdatum::top_left);
    paint_bars(menu, 52, 96, hits, 16, col);
    txt(menu, "UP", 12, 108, COL_MUTED, lgfx::textdatum::top_left);
    paint_bars(menu, 52, 108, up_lit, 16, col);
    txt(menu, s_up, width - 10, 108, COL_INK, lgfx::textdatum::top_right);
    txt(menu, gib ? "SIG" : "DUTY", 12, 120, COL_MUTED, lgfx::textdatum::top_left);
    paint_bars(menu, 52, 120, sig / 6, 16, accent());
}

static void paint_full(void)
{
    layout_covers();
    if (botcorps_dual_ok()) {
        botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
        auto &ext = botcorps_content();
        ext.clearClipRect();
        paint_ext(ext);
        ext.endWrite();
        if (wants_rain()) botcorps_rain_paint();
        tick_ext(ext);
        ext.endWrite();
        ext.clearClipRect();
    }
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &menu = botcorps_menu();
    paint_int(menu);
    menu.endWrite();
    menu.clearClipRect();
}

static void nudge_row(int d)
{
    if (s_cursor == 0) s_color = (s_color + d + 3) % 3;
    else if (s_cursor == 1) {
        s_mix = (s_mix + d + 3) % 3;
        const int mix = s_mix == 0 ? BOTCORPS_RAIN_MIX_BOTS
            : s_mix == 1 ? BOTCORPS_RAIN_MIX_CREW : BOTCORPS_RAIN_MIX_CODE;
        botcorps_rain_set_mix(mix);
    } else if (s_cursor == 2) {
        if (botcorps_rain_mode() == 0) botcorps_rain_set_mode(1);
        else botcorps_rain_set_mode(0);
        save_rain_color();
    } else if (s_cursor == 3) {
        int m = botcorps_rain_mode();
        if (m <= 0) m = 1;
        else m = m + d;
        if (m < 1) m = 6;
        if (m > 6) m = 1;
        botcorps_rain_set_mode(m);
        save_rain_color();
    } else if (s_cursor == 4) {
        s_saver = (s_saver + d + 4) % 4;
    } else if (s_cursor == 5) {
        s_saver_on = s_saver_on ? 0 : 1;
    } else if (s_cursor == 6) {
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
        else if (s_cursor == 2) enter_screen(SCR_SCAN, -1);
        else enter_screen(SCR_DIAG, -1);
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
        if (s_cursor == 0) s_track = !s_track;
        else if (s_cursor == 1) enter_screen(s_screen == SCR_WIFI_RADAR ? SCR_WIFI_LIST : SCR_BT_LIST, -1);
        else go_back();
        if (s_cursor == 0) s_dirty = true;
        break;
    case SCR_BOT_RADAR:
    case SCR_SCAN:
        if (s_cursor == 0) {
            s_track = !s_track;
            s_dirty = true;
        } else {
            go_back();
        }
        break;
    case SCR_WIFI_LIST:
    case SCR_BT_LIST:
        if (s_cursor == 3) go_back();
        break;
    case SCR_CONTROL:
        if (s_cursor == 7) {
            const int back_to = s_ret >= 0 ? s_ret : SCR_GIBSON;
            enter_screen(SCR_TERMINAL, back_to);
            boot_log();
        } else if (s_cursor == 8) {
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

static void move_cursor(int d)
{
    const int n = button_count();
    if (n <= 0) return;
    s_cursor = (s_cursor + d + n) % n;
    s_dirty = true;
}

static void on_arrows(int key)
{
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet) {
            if (key == 1) s_sheet_i = (s_sheet_i + 11) % 12;
            if (key == 2) s_sheet_i = (s_sheet_i + 1) % 12;
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
            s_dirty = true;
            return;
        }
        if (s_focus_cmds) {
            const int d = key == 3 ? -1 : key == 4 ? 1 : key == 1 ? -6 : 6;
            s_cmd_sel = (s_cmd_sel + d + 12) % 12;
            s_dirty = true;
            return;
        }
        int c = s_term % 4;
        int r = s_term / 4;
        if (key == 3) c = (c + 3) % 4;
        if (key == 4) c = (c + 1) % 4;
        if (key == 1) r = (r + 2) % 3;
        if (key == 2) r = (r + 1) % 3;
        s_term = r * 4 + c;
        s_dirty = true;
        return;
    }
    if (s_screen == SCR_CONTROL) {
        if (key == 1) move_cursor(-1);
        if (key == 2) move_cursor(1);
        if (key == 3 || key == 4) {
            if (s_cursor >= 7) move_cursor(key == 4 ? 1 : -1);
            else {
                nudge_row(key == 4 ? 1 : -1);
                s_dirty = true;
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
        s_bot = i;
        s_dirty = true;
        return;
    }
    if (key == 1 || key == 3) move_cursor(-1);
    if (key == 2 || key == 4) move_cursor(1);
}

static void on_ok(void)
{
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet) {
            char line[32];
            snprintf(line, sizeof(line), "%s", CMDS[s_sheet_i]);
            s_sheet = false;
            if (strcmp(line, "cmds") != 0) run_cmd(line);
            else s_dirty = true;
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
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet) {
            s_sheet = false;
            s_dirty = true;
            return;
        }
        if (botcorps_t0_term_prompt[0]) {
            botcorps_t0_term_prompt[0] = 0;
            s_dirty = true;
            return;
        }
    }
    go_back();
}

static void on_tab(void)
{
    if (s_screen != SCR_TERMINAL || s_sheet) return;
    if (!botcorps_t0_term_prompt[0]) {
        s_focus_cmds = !s_focus_cmds;
        s_dirty = true;
        return;
    }
    const char *hit = 0;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(CMDS[i], botcorps_t0_term_prompt, strlen(botcorps_t0_term_prompt)) == 0) {
            hit = CMDS[i];
            break;
        }
    }
    if (!hit) return;
    snprintf(botcorps_t0_term_prompt, 60, "%s ", hit);
    s_dirty = true;
}

static void on_char(char c)
{
    if (s_screen != SCR_TERMINAL) return;
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n + 1 >= 60) return;
    botcorps_t0_term_prompt[n] = c;
    botcorps_t0_term_prompt[n + 1] = 0;
    s_dirty = true;
}

static void on_bs(void)
{
    if (s_screen != SCR_TERMINAL) {
        on_esc();
        return;
    }
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n == 0) return;
    botcorps_t0_term_prompt[n - 1] = 0;
    s_dirty = true;
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
}

void botcorps_ui_begin(void)
{
    s_boot_ms = millis();
    s_screen = SCR_RAIN;
    s_cursor = 0;
    s_dirty = true;
    s_ret = -1;
    s_track_name[0] = 0;
    botcorps_t0_term_prompt[0] = 0;
    load_rain_color();
    botcorps_rain_set_draw(true);
    botcorps_rain_log_heap("boot");
}

void botcorps_ui_input(void)
{
    char typed = 0;
    const int key = read_key(&typed);
    if (key == K_NONE) return;
    if (key == K_UP || key == K_DN || key == K_LT || key == K_RT) on_arrows(key);
    else if (key == K_OK) on_ok();
    else if (key == K_ESC) on_esc();
    else if (key == K_TAB) on_tab();
    else if (key == K_BS) on_bs();
    else if (key == K_CHAR) on_char(typed);
}

void botcorps_ui_draw(void)
{
    layout_covers();
    if (s_dirty) {
        s_dirty = false;
        paint_full();
        return;
    }
    if (botcorps_dual_ok() && wants_rain()) botcorps_rain_step();
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
        botcorps_set_surface(BOTCORPS_SURFACE_MENU);
        paint_int(botcorps_menu());
        botcorps_menu().endWrite();
    }
}
