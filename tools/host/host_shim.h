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
    void setClipRect(int, int, int, int) {}
    void clearClipRect() {}
    int width() const { return 240; }
    int height() const { return 135; }
    void fillScreen(uint8_t) {}
    void fillScreen(uint16_t) {}
    void fillRect(int, int, int, int, uint8_t) {}
    void fillRect(int, int, int, int, uint16_t) {}
    void drawFastHLine(int, int, int, uint8_t) {}
    void drawFastHLine(int, int, int, uint16_t) {}
    void setTextSize(float) {}
    void setTextSize(float, float) {}
    void setTextColor(uint8_t) {}
    void setTextColor(uint16_t) {}
    void setTextColor(uint8_t, uint8_t) {}
    void setTextColor(uint16_t, uint16_t) {}
    void setCursor(int, int) {}
    void print(const char *) {}
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

struct botcorps_cover_t {
    int16_t x, y, w, h;
};

class Preferences {
public:
    bool begin(const char *, bool) { return false; }
    void end() {}
    unsigned getUChar(const char *, unsigned d = 0) { return d; }
    void putUChar(const char *, uint8_t) {}
};

struct HostPrint {
    void printf(const char *, ...) {}
    void println(const char *) {}
};
static HostPrint Serial;
uint32_t host_millis(void);
void host_set_millis(uint32_t t);
static inline uint32_t millis(void) { return host_millis(); }

struct HostKeys {
    bool enter = false;
    bool del = false;
    bool tab = false;
    char word[8] = {0};
    struct Empty {
        const uint8_t *begin() const { return nullptr; }
        const uint8_t *end() const { return nullptr; }
    } hid_keys;
};
struct HostKeyboard {
    bool isChange() { return false; }
    bool isPressed() { return false; }
    HostKeys keysState() { return {}; }
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
void botcorps_band_set_recompose(void (*)(Gfx));
int botcorps_band_oy(void);
void botcorps_rain_set_hub(bool);
void botcorps_rain_draw(lgfx::v1::LovyanGFX &, int, int);
bool botcorps_rain_step(void);
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
void botcorps_host_keyfob(uint8_t *buf);
void botcorps_host_hub(uint8_t *buf);
void botcorps_host_intro(uint8_t *buf);
