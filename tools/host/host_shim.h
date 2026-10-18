#pragma once
/* Host-only stand-ins so the firmware paint file can draw into an 8-bit buffer.
 * The firmware build does not include this header. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ui/botcorps_pal.h"
#include "probe/botcorps_probe.h"
#include "probe/botcorps_icons.h"

namespace lgfx {
inline namespace v1 {
namespace textdatum {
enum textdatum_t { top_left = 0, top_center = 1, top_right = 2, middle_center = 3, bottom_center = 4 };
}
using textdatum_t = textdatum::textdatum_t;
class LovyanGFX {
public:
    template <class F> void setFont(const F *) {}
    void startWrite() {}
    void endWrite() {}
    void setClipRect(int, int, int, int);
    void clearClipRect();
    int width() const { return 320; }
    int height() const { return 240; }
    void fillScreen(uint8_t) {}
    void fillScreen(uint16_t) {}
    void fillRect(int, int, int, int, uint8_t);
    void fillRect(int, int, int, int, uint16_t) {}
    void drawFastHLine(int, int, int, uint8_t) {}
    void drawFastHLine(int, int, int, uint16_t) {}
    void setTextSize(float) {}
    void setTextSize(float, float) {}
    void setTextWrap(bool, bool) {}
    void setTextColor(uint8_t);
    void setTextColor(uint16_t) {}
    void setTextColor(uint8_t, uint8_t) {}
    void setTextColor(uint16_t, uint16_t) {}
    void setCursor(int, int);
    void print(const char *);
    void setTextDatum(textdatum_t) {}
    void drawString(const char *, int, int) {}
};
class LGFX_Device : public LovyanGFX {};
}
}

namespace fonts {
struct Font { int id; };
extern const Font Font0;
extern const Font Font8x8C64;
}
void host_note_font(int id);
lgfx::v1::LovyanGFX *host_rain_panel(void);
void host_rain_init(int mode);

struct Gfx {
    lgfx::v1::LovyanGFX *p;
    int ox;
    int oy;
    Gfx();
    Gfx(lgfx::v1::LovyanGFX &d);
    lgfx::v1::LovyanGFX *raw() const { return p; }
    void fillRect(int x, int y, int w, int h, uint8_t c);
    void fillRect(int x, int y, int w, int h, uint16_t c);
    void drawRect(int x, int y, int w, int h, uint8_t c);
    void fillCircle(int x, int y, int r, uint8_t c);
    void drawCircle(int x, int y, int r, uint8_t c);
    void drawLine(int x0, int y0, int x1, int y1, uint8_t c);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c);
    void drawFastHLine(int x, int y, int w, uint8_t c);
    void drawFastVLine(int x, int y, int h, uint8_t c);
    void fillRoundRect(int x, int y, int w, int h, int r, uint8_t c);
    void fillScreen(uint8_t c);
    void setClipRect(int x, int y, int w, int h);
    void clearClipRect(void);
    void setTextWrap(bool x, bool y);
    template <class F>
    void setFont(const F *f) { host_note_font(f ? f->id : 0); }
    void setTextSize(float s);
    void setTextSize(float sx, float sy);
    void setTextColor(uint8_t c);
    void setTextColor(uint16_t c);
    void setTextColor(uint8_t fg, uint8_t bg);
    void setTextDatum(lgfx::textdatum_t dat);
    void drawString(const char *s, int x, int y);
    void setCursor(int x, int y);
    void print(const char *s);
    int getCursorX(void);
    void startWrite(void);
    void endWrite(void);
};

/* gate.sh hooks: element/text bbox log per paint_ext */
void host_gate_reset(void);
void host_gate_box(int ox, int oy, const char *kind, const char *name, int x, int y, int w, int h, const char *parent);
extern int host_gate_norain;
extern int host_gate_mute;
extern int host_gate_walk;

struct botcorps_cover_t {
    int16_t x, y, w, h;
};

extern int host_nvs_puts;
class Preferences {
public:
    bool begin(const char *, bool ro) { return !ro; }
    void end() {}
    unsigned getUChar(const char *, unsigned d = 0) { return d; }
    void putUChar(const char *, uint8_t) { ++host_nvs_puts; }
};

struct HostPrint {
    void printf(const char *, ...) {}
    void println(const char *) {}
};
static HostPrint Serial;
uint32_t host_millis(void);
void host_set_millis(uint32_t t);
static inline uint32_t millis(void) { return host_millis(); }

/* v1017c: scriptable copy of the M5Cardputer keyboard API that read_key uses (keyList in press order,
 * getKeyValue from the library keymap, keysState modifiers, capslocked). Gate K1 drives it. */
#include <vector>
#define KEY_LEFT_CTRL  0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT   0x82
#define KEY_FN  0xff
#define KEY_OPT 0x00
#define KEY_BACKSPACE 0x2a
#define KEY_TAB       0x2b
#define KEY_ENTER     0x28
struct Point2D_t {
    int x = 0;
    int y = 0;
    bool operator==(const Point2D_t &o) const { return x == o.x && y == o.y; }
};
struct KeyValue_t {
    const char value_first;
    const char value_second;
};
static const KeyValue_t _key_value_map[4][14] = {{{'`', '~'},
                                           {'1', '!'},
                                           {'2', '@'},
                                           {'3', '#'},
                                           {'4', '$'},
                                           {'5', '%'},
                                           {'6', '^'},
                                           {'7', '&'},
                                           {'8', '*'},
                                           {'9', '('},
                                           {'0', ')'},
                                           {'-', '_'},
                                           {'=', '+'},
                                           {(char)KEY_BACKSPACE, (char)KEY_BACKSPACE}},
                                          {{(char)KEY_TAB, (char)KEY_TAB},
                                           {'q', 'Q'},
                                           {'w', 'W'},
                                           {'e', 'E'},
                                           {'r', 'R'},
                                           {'t', 'T'},
                                           {'y', 'Y'},
                                           {'u', 'U'},
                                           {'i', 'I'},
                                           {'o', 'O'},
                                           {'p', 'P'},
                                           {'[', '{'},
                                           {']', '}'},
                                           {'\\', '|'}},
                                          {{(char)KEY_FN, (char)KEY_FN},
                                           {(char)KEY_LEFT_SHIFT, (char)KEY_LEFT_SHIFT},
                                           {'a', 'A'},
                                           {'s', 'S'},
                                           {'d', 'D'},
                                           {'f', 'F'},
                                           {'g', 'G'},
                                           {'h', 'H'},
                                           {'j', 'J'},
                                           {'k', 'K'},
                                           {'l', 'L'},
                                           {';', ':'},
                                           {'\'', '\"'},
                                           {(char)KEY_ENTER, (char)KEY_ENTER}},
                                          {{(char)KEY_LEFT_CTRL, (char)KEY_LEFT_CTRL},
                                           {(char)KEY_OPT, (char)KEY_OPT},
                                           {(char)KEY_LEFT_ALT, (char)KEY_LEFT_ALT},
                                           {'z', 'Z'},
                                           {'x', 'X'},
                                           {'c', 'C'},
                                           {'v', 'V'},
                                           {'b', 'B'},
                                           {'n', 'N'},
                                           {'m', 'M'},
                                           {',', '<'},
                                           {'.', '>'},
                                           {'/', '?'},
                                           {' ', ' '}}};
struct HostKeys {
    bool fn = false, shift = false, ctrl = false, enter = false;
    std::vector<char> word;          /* library updateKeysState: chars of ALL held keys */
    std::vector<uint8_t> hid_keys;   /* (TAB/BS/ENTER codes; printable HID codes not needed here) */
};
struct HostKeyboard {
    std::vector<Point2D_t> list;
    bool caps = false;
    const std::vector<Point2D_t> &keyList() { return list; }
    KeyValue_t getKeyValue(const Point2D_t &p) { return _key_value_map[p.y][p.x]; }
    bool capslocked() { return caps; }
    HostKeys keysState()
    {
        HostKeys k;
        for (const auto &p : list) {
            const uint8_t v = (uint8_t)getKeyValue(p).value_first;
            k.fn |= v == KEY_FN; k.shift |= v == KEY_LEFT_SHIFT; k.ctrl |= v == KEY_LEFT_CTRL;
        }
        for (const auto &p : list) {
            const KeyValue_t kv = getKeyValue(p);
            const uint8_t v = (uint8_t)kv.value_first;
            if (v == KEY_FN || v == KEY_OPT || v == KEY_LEFT_CTRL || v == KEY_LEFT_SHIFT || v == KEY_LEFT_ALT) continue;
            if (v == KEY_TAB || v == KEY_BACKSPACE || v == KEY_ENTER) { k.enter |= v == KEY_ENTER; k.hid_keys.push_back(v); continue; }
            k.word.push_back((k.ctrl || k.shift || caps) ? kv.value_second : kv.value_first);
        }
        return k;
    }
    /* library isChange / isPressed: key COUNT only */
    size_t last_n = 0;
    bool isChange() { if (list.size() != last_n) { last_n = list.size(); return true; } return false; }
    bool isPressed() { return !list.empty(); }
    void press(int x, int y) { Point2D_t p; p.x = x; p.y = y; list.push_back(p); }
    void release(int x, int y)
    {
        for (size_t i = 0; i < list.size(); ++i) if (list[i].x == x && list[i].y == y) { list.erase(list.begin() + i); return; }
    }
};
struct HostM5 { HostKeyboard Keyboard; };
static HostM5 M5Cardputer;

#define BOTCORPS_SHAPE_ROUND 0
#define BOTCORPS_RAIN_COLORS 7

extern uint8_t *host_frame;
extern int host_term_lut;
extern int host_fit_calls;
extern int host_rgb_ov;
extern int host_rgb_span;
extern int host_int_mask;
extern int host_int_full;
extern int host_int_strip;
extern int host_int_part;
void botcorps_lut_term(bool on);
int botcorps_host_cmds_sheet(void);
int botcorps_host_int_span(void);

uint16_t botcorps_rgb565(uint8_t);
void botcorps_sprite_draw(lgfx::v1::LovyanGFX &, int, int, int, int, uint8_t, bool);
uint8_t botcorps_pal_hub_of(uint8_t idx);
void botcorps_rain_set_covers(const botcorps_cover_t *, int, bool);
void botcorps_probe_screen(const char *, const char *);
void botcorps_rain_set_mode(int);
int botcorps_rain_mode(void);
int botcorps_shape_for(int);
void botcorps_rain_set_mix(int);
void botcorps_rain_set_shape(int);
uint8_t botcorps_rain_palette(int);
uint8_t botcorps_rain_solid(void);
int botcorps_rain_mix(void);
void botcorps_id_paint(Gfx, int, int, bool, uint8_t, int, const char *, const char *);
void botcorps_id_recompose(lgfx::v1::LovyanGFX &, int, int, bool, uint8_t, int, const char *, const char *);
void botcorps_id_tick(lgfx::v1::LovyanGFX &, int, int, bool, uint8_t);
lgfx::v1::LGFX_Device &botcorps_menu(void);
const char *botcorps_band_mode_name(void);
uint32_t botcorps_band_largest(void);
lgfx::v1::LovyanGFX &botcorps_content(void);
bool botcorps_dual_ok(void);
void botcorps_set_surface(int);
void botcorps_band_set_oy(int);
void botcorps_band_present(void (*)(Gfx));
bool botcorps_band_direct(void);
bool botcorps_band_live(void);
void botcorps_band_patch(void (*)(Gfx), int, int, int, int);
void botcorps_band_patch_rect(void (*)(Gfx), int, int, int, int);
void botcorps_band_set_recompose(void (*)(Gfx));
int botcorps_band_oy(void);
void botcorps_rain_set_hub(bool);
void botcorps_rain_set_gibson(bool);
void botcorps_rain_set_no_bots(bool);
void botcorps_rain_draw(lgfx::v1::LovyanGFX &, int, int);
bool botcorps_rain_step(void);
void botcorps_rain_ghost(lgfx::v1::LovyanGFX &, int, int, int, int, int, int, uint8_t);
void botcorps_rain_ghost_ex(lgfx::v1::LovyanGFX &, int, int, int, int, int, int, uint8_t, bool);
void botcorps_rain_set_draw(bool);
void botcorps_rain_log_heap(const char *);
const botcorps_probe_t *botcorps_probe_get(void);
void botcorps_glyph_blit(lgfx::v1::LovyanGFX &, int, int, int, uint8_t, uint8_t, uint8_t, bool (*)(int, int));
uint8_t botcorps_pal_fit(uint16_t);
uint16_t botcorps_pal_native(uint8_t);

enum { BOTCORPS_SURFACE_CONTENT = 1, BOTCORPS_SURFACE_MENU = 0 };

void botcorps_host_terminal(uint8_t *buf);
void botcorps_host_idle(uint8_t *buf);
void botcorps_host_hunt(uint8_t *buf);
void botcorps_host_hunt_view(uint8_t *buf, int force_dbm);
void botcorps_host_ble_list(uint8_t *buf);
void botcorps_host_found_alert(uint8_t *buf);
void botcorps_host_scan_tint(uint8_t *buf, int color);
void botcorps_host_sbox(uint8_t *buf, int band, int mode, int force_dbm);
void botcorps_host_found_alert_b(uint8_t *buf, int band, int dev);
void botcorps_host_list(uint8_t *buf);
void botcorps_host_bus(uint8_t *buf);
void botcorps_host_bus_ble(uint8_t *buf);
void botcorps_host_amber(uint8_t *buf);
void botcorps_host_botscan(uint8_t *buf);
void botcorps_host_botscan_n(uint8_t *buf, int bot);
void botcorps_host_sheet(uint8_t *buf, int sel);
void botcorps_host_sheet_key(uint8_t *buf, int key);
void botcorps_host_sig(int s);
void botcorps_host_gchat(uint8_t *buf);
void botcorps_host_gmenu(uint8_t *buf);
void botcorps_host_jackin(uint8_t *buf);
void botcorps_host_scanall(uint8_t *buf);
void botcorps_host_scanall_sheet(uint8_t *buf);
void botcorps_host_stale_sweep(uint8_t *buf);
void botcorps_host_keyfob(uint8_t *buf);
void botcorps_host_hub(uint8_t *buf);
void botcorps_host_enter(uint8_t *buf, int id);
int botcorps_host_screen_now(void);
void botcorps_host_paint(uint8_t *buf);
void botcorps_host_key(int key, char c);
void botcorps_host_seq(uint8_t *buf, const char *keys);
void botcorps_host_set_color(int c);
void botcorps_host_intro(uint8_t *buf);
