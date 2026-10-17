#include "host_shim.h"
#include "ui/botcorps_term_ov.h"

#define PROGMEM
#include "lgfx/Fonts/glcdfont.h"
#include "lgfx/Fonts/Font8x8C64.h"

uint8_t *host_frame = nullptr;
int host_term_lut = 0;
int host_fit_calls = 0;
int host_rgb_ov = 0;
int host_rgb_span = 0;
int host_int_mask = 0;
int host_int_full = 0;
int host_int_strip = 0;
int host_int_part = 0;
void botcorps_lut_term(bool on) { host_term_lut = on ? 1 : 0; }

static int term_code(uint8_t idx)
{
    for (int n = 0; n < botcorps_term_ov_n; ++n)
        if (idx == botcorps_term_ov_i[n]) return 1;
    return 0;
}

static void note_int(uint8_t idx)
{
    if (!host_int_mask || !term_code(idx)) return;
    if (host_int_mask & 1) host_int_full++;
    if (host_int_mask & 2) host_int_strip++;
    if (host_int_mask & 4) host_int_part++;
}

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

Gfx::Gfx() : p(host_rain_panel()), ox(0), oy(0) {}
Gfx::Gfx(lgfx::v1::LovyanGFX &r) : p(&r), ox(0), oy(0) {}
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
void Gfx::setTextColor(uint8_t c)
{
    note_int(c);
    s_fg = c;
    s_bg_on = false;
}
void Gfx::setTextColor(uint16_t) {}
void Gfx::setTextColor(uint8_t fg, uint8_t bg)
{
    note_int(fg);
    note_int(bg);
    s_fg = fg;
    s_bg = bg;
    s_bg_on = true;
}
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

static void gate_txt(int x, int y, int w, int h, const char *s);
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
    if (s_datum == lgfx::textdatum::bottom_center) y -= font_h();
    gate_txt(x, y, n * aw, font_h(), s);
    for (int i = 0; s[i]; ++i) glyph(x + i * aw, y, s[i], s_fg, s_bg_on, s_bg);
}

void Gfx::print(const char *s)
{
    if (!s) return;
    const int aw = font_w();
    gate_txt(s_cx, s_cy, (int)strlen(s) * aw, font_h(), s);
    for (int i = 0; s[i]; ++i) {
        glyph(s_cx, s_cy, s[i], s_fg, s_bg_on, s_bg);
        s_cx += aw;
    }
}

uint16_t botcorps_rgb565(uint8_t idx)
{
    if (idx >= 229 && idx <= 248) host_rgb_span++;
    if (term_code(idx)) host_rgb_ov++;
    note_int(idx);
    extern uint16_t host_rgb565_word(uint8_t);
    return host_rgb565_word(idx);   /* v1016k: was 0, which blacked every mix16() tint on host */
}

static void touch_rgb(uint8_t c) { (void)botcorps_rgb565(c); }

static void touch_card(void (*fn)(uint8_t))
{
    static const uint8_t ids[] = {
        PAL_DEEP, PAL_IVORY, PAL_CHIN, PAL_BLACK, PAL_STEEL, PAL_BG, PAL_MUTED,
        PAL_HOLD, PAL_WHITE, PAL_BAR, PAL_LINE, PAL_INK, PAL_B5D3F, PAL_B4E7F,
        PAL_B7711, PAL_B9F0B, PAL_BFB72, PAL_BAC7F, PAL_B36B8, PAL_BFC6B, PAL_BBD3F
    };
    for (unsigned i = 0; i < sizeof(ids); ++i) fn(ids[i]);
}


static void touch_native(uint8_t c) { (void)botcorps_pal_native(c); }
/* Same bitmaps as src/ui/botcorps_sprite.cpp. 1 body, 2 eye. */
static const int SPR_W[4] = { 7, 9, 7, 5 };
static const int SPR_H[4] = { 7, 6, 9, 5 };
static const char *const SPR_ROW[4][9] = {
    { "...1...", ".11111.", "1111111", "1221221", "1221221", "1111111", ".11111." },
    { "....1....", ".1111111.", "112211221", "112211221", "111111111", ".1111111." },
    { "...1...", "1111111", "1221221", "1221221", "1111111", ".11111.", ".11111.", ".11.11.", ".11.11." },
    { "11111", "12121", "11111", "11111", "11111" },
};

void botcorps_sprite_draw(lgfx::v1::LovyanGFX &, int x, int y, int shape, int scale, uint8_t color, bool blink)
{
    if (shape < 0 || shape > 3) shape = 0;
    const int s = scale < 1 ? 1 : scale;
    const int w = SPR_W[shape];
    const int h = SPR_H[shape];
    const int ox = x - (w * s) / 2;
    const int oy = y - (h * s) / 2;
    host_gate_box(0, 0, host_gate_walk ? "WALK" : "SPR", "sprite", ox, oy, w * s, h * s, "-");
    for (int row = 0; row < h; ++row) {
        const char *line = SPR_ROW[shape][row];
        for (int col = 0; col < w; ++col) {
            const char v = line[col];
            if (v != '1' && !(v == '2' && !blink)) continue;
            const uint8_t ink = (v == '2') ? 36 : color;
            for (int sy = 0; sy < s; ++sy)
                for (int sx = 0; sx < s; ++sx)
                    put_px(ox + col * s + sx, oy + row * s + sy, ink);
        }
    }
}
uint8_t botcorps_pal_hub_of(uint8_t idx) { return idx; }
void botcorps_probe_screen(const char *, const char *) {}
int botcorps_shape_for(int index)
{
    if (index < 0) index = 0;
    return index % 4;
}
void botcorps_id_paint(Gfx, int, int, bool, uint8_t, int, const char *, const char *)
{
    if (host_int_mask & 2) touch_card(touch_native);
    else touch_card(touch_rgb);
}
void botcorps_id_recompose(lgfx::v1::LovyanGFX &, int, int, bool, uint8_t, int, const char *, const char *)
{
    const int saved = host_int_mask;
    host_int_mask |= 2;
    touch_native(PAL_DEEP);
    botcorps_id_paint(Gfx(), 240, 0, false, PAL_IVORY, 0, "", "");
    host_int_mask = saved;
}
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
static uint32_t s_host_ms = 2500;
void host_set_millis(uint32_t t) { s_host_ms = t; }
uint32_t host_millis(void) { return s_host_ms; }
static botcorps_probe_t s_pr = {};
const botcorps_probe_t *botcorps_probe_get(void) { return &s_pr; }
void botcorps_glyph_blit(lgfx::v1::LovyanGFX &, int, int, int, uint8_t, uint8_t, uint8_t, bool (*)(int, int)) {}

char botcorps_t0_term_prompt[80];
char botcorps_t0_term_hist[20][26];
char botcorps_t0_term_log[2048];
char botcorps_t0_term_found[12][28];
uint8_t botcorps_t0_rain_cell[3][1200];

/* ---- gate.sh log ---- */
int host_gate_norain = 0;
int host_gate_mute = 0;
int host_gate_walk = 0;
int host_nvs_puts = 0;
static FILE *s_gate_f = nullptr;
static char s_gate_buf[200000];
static size_t s_gate_n = 0;
void host_gate_reset(void) { s_gate_n = 0; s_gate_buf[0] = 0; }
static void gate_put(const char *line)
{
    const size_t l = strlen(line);
    if (s_gate_n + l + 1 >= sizeof(s_gate_buf)) return;
    memcpy(s_gate_buf + s_gate_n, line, l + 1);
    s_gate_n += l;
}
void host_gate_box(int ox, int oy, const char *kind, const char *name, int x, int y, int w, int h, const char *parent)
{
    if (host_gate_mute) return;
    char b[256];
    snprintf(b, sizeof b, "BOX\t%s\t%s\t%d\t%d\t%d\t%d\t%s\n", kind, name, x - ox, y - oy, w, h, parent ? parent : "-");
    gate_put(b);
}
static void gate_txt(int x, int y, int w, int h, const char *s)
{
    if (host_gate_mute) return;
    char b[256], t[96];
    int j = 0;
    for (int i = 0; s[i] && j < 90; ++i) t[j++] = (s[i] == '\t' || s[i] == '\n' || (unsigned char)s[i] < 32) ? ' ' : s[i];
    t[j] = 0;
    if (s_clip) snprintf(b, sizeof b, "TXT\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", x, y, w, h, s_clip_x, s_clip_y, s_clip_w, s_clip_h, t);
    else snprintf(b, sizeof b, "TXT\t%d\t%d\t%d\t%d\t0\t0\t320\t240\t%s\n", x, y, w, h, t);
    gate_put(b);
}
void host_gate_dump(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    fwrite(s_gate_buf, 1, s_gate_n, f);
    fclose(f);
}
