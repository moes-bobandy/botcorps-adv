#include "host_shim.h"

#define PROGMEM
#include "lgfx/Fonts/glcdfont.h"
#include "lgfx/Fonts/Font8x8C64.h"

uint8_t *host_frame = nullptr;
int host_term_lut = 0;
void botcorps_lut_term(bool on) { host_term_lut = on ? 1 : 0; }

const fonts::Font fonts::Font0 = {0};
const fonts::Font fonts::Font8x8C64 = {1};

static int s_font = 0;
static int s_scale = 1;

void host_note_font(int id) { s_font = id == 1 ? 1 : 0; }

static int s_cx = 0;
static int s_cy = 0;
static int s_datum = 0;
static uint8_t s_fg = 255;
static uint8_t s_bg = 0;
static bool s_bg_on = false;
static int s_clip_x = 0, s_clip_y = 0, s_clip_w = 320, s_clip_h = 240;
static bool s_clip = false;

static void put_px(int x, int y, uint8_t c)
{
    if (!host_frame) return;
    if (x < 0 || y < 0 || x >= 320 || y >= 240) return;
    if (s_clip && (x < s_clip_x || y < s_clip_y || x >= s_clip_x + s_clip_w || y >= s_clip_y + s_clip_h))
        return;
    host_frame[y * 320 + x] = c;
}

static void fill_raw(int x, int y, int w, int h, uint8_t c)
{
    if (w < 0) { x += w; w = -w; }
    if (h < 0) { y += h; h = -h; }
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            put_px(xx, yy, c);
}

Gfx::Gfx() : p(nullptr), ox(0), oy(0) {}
Gfx::Gfx(lgfx::v1::LovyanGFX &) : p(nullptr), ox(0), oy(0) {}
void Gfx::fillRect(int x, int y, int w, int h, uint8_t c) { fill_raw(x - ox, y - oy, w, h, c); }
void Gfx::fillRect(int x, int y, int w, int h, uint16_t) { fill_raw(x - ox, y - oy, w, h, s_fg); }
void Gfx::drawRect(int x, int y, int w, int h, uint8_t c)
{
    fill_raw(x - ox, y - oy, w, 1, c);
    fill_raw(x - ox, y - oy + h - 1, w, 1, c);
    fill_raw(x - ox, y - oy, 1, h, c);
    fill_raw(x - ox + w - 1, y - oy, 1, h, c);
}
void Gfx::fillCircle(int x, int y, int r, uint8_t c)
{
    for (int yy = -r; yy <= r; ++yy)
        for (int xx = -r; xx <= r; ++xx)
            if (xx * xx + yy * yy <= r * r) put_px(x - ox + xx, y - oy + yy, c);
}
void Gfx::drawCircle(int x, int y, int r, uint8_t c)
{
    for (int a = 0; a < 64; ++a) {
        const float t = a * 0.098175f;
        put_px(x - ox + (int)(cosf(t) * r), y - oy + (int)(sinf(t) * r), c);
    }
}
void Gfx::drawLine(int x0, int y0, int x1, int y1, uint8_t c)
{
    int dx = x1 - x0, dy = y1 - y0;
    int n = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (n < 1) n = 1;
    for (int i = 0; i <= n; ++i)
        put_px(x0 - ox + dx * i / n, y0 - oy + dy * i / n, c);
}
void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c)
{
    fill_raw(x0 - 1, y0 - 1, 3, 3, c);
    (void)x1; (void)y1; (void)x2; (void)y2;
}
void Gfx::drawFastHLine(int x, int y, int w, uint8_t c) { fill_raw(x - ox, y - oy, w, 1, c); }
void Gfx::drawFastVLine(int x, int y, int h, uint8_t c) { fill_raw(x - ox, y - oy, 1, h, c); }
void Gfx::fillRoundRect(int x, int y, int w, int h, int, uint8_t c) { fill_raw(x - ox, y - oy, w, h, c); }
void Gfx::fillScreen(uint8_t c) { fill_raw(0, 0, 320, 240, c); }
void Gfx::setClipRect(int x, int y, int w, int h)
{
    s_clip = true;
    s_clip_x = x - ox;
    s_clip_y = y - oy;
    s_clip_w = w;
    s_clip_h = h;
}
void Gfx::clearClipRect(void) { s_clip = false; }
void Gfx::setTextWrap(bool, bool) {}
void Gfx::setTextSize(float s) { s_scale = s < 1.f ? 1 : (int)s; }
void Gfx::setTextSize(float sx, float) { setTextSize(sx); }
void Gfx::setTextColor(uint8_t c) { s_fg = c; s_bg_on = false; }
void Gfx::setTextColor(uint16_t) {}
void Gfx::setTextColor(uint8_t fg, uint8_t bg) { s_fg = fg; s_bg = bg; s_bg_on = true; }
void Gfx::setTextDatum(lgfx::textdatum_t d) { s_datum = (int)d; }
void Gfx::setCursor(int x, int y) { s_cx = x - ox; s_cy = y - oy; }
int Gfx::getCursorX(void) { return s_cx + ox; }
void Gfx::startWrite(void) {}
void Gfx::endWrite(void) {}

static int font_w(void) { return (s_font ? 8 : 6) * s_scale; }
static int font_h(void) { return 8 * s_scale; }

/* LovyanGFX GLCD: each byte is a column, bit 0 is the top pixel. */
static void glyph(int x, int y, char ch, uint8_t fg, bool bg, uint8_t bgc)
{
    const int fw = s_font ? 8 : 6;
    const int dw = s_font ? 8 : 5;
    const int sc = s_scale < 1 ? 1 : s_scale;
    if (bg) fill_raw(x, y, fw * sc, 8 * sc, bgc);
    unsigned u = (unsigned char)ch;
    const uint8_t *bits = nullptr;
    if (!s_font) {
        if (u > 255) return;
        bits = font + u * 5;
    } else {
        if (u < 32 || u > 143) return;
        bits = font8x8_c64 + (u - 32) * 8;
    }
    for (int col = 0; col < dw; ++col) {
        const uint8_t line = bits[col];
        for (int row = 0; row < 8; ++row) {
            if ((line >> row) & 1) {
                for (int sy = 0; sy < sc; ++sy)
                    for (int sx = 0; sx < sc; ++sx)
                        put_px(x + col * sc + sx, y + row * sc + sy, fg);
            }
        }
    }
}

void Gfx::drawString(const char *s, int x, int y)
{
    x -= ox;
    y -= oy;
    if (!s) return;
    const int n = (int)strlen(s);
    const int aw = font_w();
    if (s_datum == lgfx::textdatum::top_center || s_datum == lgfx::textdatum::middle_center ||
        s_datum == lgfx::textdatum::bottom_center)
        x -= (n * aw) / 2;
    else if (s_datum == lgfx::textdatum::top_right)
        x -= n * aw;
    if (s_datum == lgfx::textdatum::middle_center) y -= font_h() / 2;
    for (int i = 0; s[i]; ++i) glyph(x + i * aw, y, s[i], s_fg, s_bg_on, s_bg);
}

void Gfx::print(const char *s)
{
    if (!s) return;
    const int aw = font_w();
    for (int i = 0; s[i]; ++i) {
        glyph(s_cx, s_cy, s[i], s_fg, s_bg_on, s_bg);
        s_cx += aw;
    }
}

uint16_t botcorps_rgb565(uint8_t) { return 0; }
void botcorps_sprite_draw(lgfx::v1::LovyanGFX &, int x, int y, int, int scale, uint8_t color, bool blink)
{
    const int s = scale < 1 ? 1 : scale;
    const int ox = x - 3 * s;
    const int oy = y - 3 * s;
    for (int yy = 0; yy < 7 * s; ++yy)
        for (int xx = 0; xx < 7 * s; ++xx) {
            const int cx = xx - 3 * s;
            const int cy = yy - 3 * s;
            if (cx * cx + cy * cy > (3 * s) * (3 * s)) continue;
            put_px(ox + xx, oy + yy, color);
        }
    if (!blink) {
        fill_raw(x - s, y - s, s, s, 0);
        fill_raw(x + s, y - s, s, s, 0);
    }
}
uint8_t botcorps_pal_hub_of(uint8_t idx) { return idx; }
void botcorps_rain_set_covers(const botcorps_cover_t *, int, bool) {}
void botcorps_probe_screen(const char *, const char *) {}
void botcorps_rain_set_mode(int) {}
int botcorps_rain_mode(void) { return 0; }
int botcorps_shape_for(int) { return 0; }
void botcorps_rain_set_mix(int) {}
void botcorps_rain_set_shape(int) {}
uint8_t botcorps_rain_palette(int) { return 123; }
uint8_t botcorps_rain_solid(void) { return 57; }
int botcorps_rain_mix(void) { return 0; }
void botcorps_id_paint(Gfx, int, int, bool, uint8_t, int, const char *, const char *) {}
void botcorps_id_recompose(lgfx::v1::LovyanGFX &, int, int, bool, uint8_t, int, const char *, const char *) {}
void botcorps_id_tick(lgfx::v1::LovyanGFX &, int, int, bool, uint8_t) {}
static lgfx::v1::LGFX_Device s_dummy;
lgfx::v1::LGFX_Device &botcorps_menu(void) { return s_dummy; }
const char *botcorps_band_mode_name(void) { return "bands"; }
uint32_t botcorps_band_largest(void) { return 0; }
lgfx::v1::LovyanGFX &botcorps_content(void) { return s_dummy; }
bool botcorps_dual_ok(void) { return true; }
void botcorps_set_surface(int) {}
void botcorps_band_set_oy(int) {}
void botcorps_band_present(void (*)(Gfx)) {}
bool botcorps_band_direct(void) { return true; }
bool botcorps_band_live(void) { return false; }
void botcorps_band_patch(void (*draw)(Gfx), int, int, int, int) { if (draw) draw(Gfx()); }
void botcorps_band_set_recompose(void (*)(Gfx)) {}
int botcorps_band_oy(void) { return 0; }
void botcorps_rain_set_hub(bool) {}
void botcorps_rain_draw(lgfx::v1::LovyanGFX &, int, int) {}
bool botcorps_rain_step(void) { return false; }
void botcorps_rain_set_draw(bool) {}
void botcorps_rain_log_heap(const char *) {}
static botcorps_probe_t s_pr = {};
const botcorps_probe_t *botcorps_probe_get(void) { return &s_pr; }
void botcorps_glyph_blit(lgfx::v1::LovyanGFX &, int, int, int, uint8_t, uint8_t, uint8_t, bool (*)(int, int)) {}
uint8_t botcorps_pal_fit(uint16_t) { return 0; }
uint16_t botcorps_pal_native(uint8_t) { return 0; }

char botcorps_t0_term_prompt[80];
char botcorps_t0_term_hist[20][26];
char botcorps_t0_term_log[2048];
char botcorps_t0_term_found[12][28];
uint8_t botcorps_t0_rain_cell[3][1200];
