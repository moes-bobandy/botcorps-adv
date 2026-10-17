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

/* v1017c S3: device BANDS paints one 120-row slice at a time with oy = 0 / 120 into a 320x120
 * sprite. host_slice_h = 120 + host_slice_oy emulate that exactly (Gfx oy, sprite height clip). */
int host_slice_oy = 0;
int host_slice_h = 0;
static void put_px(int x, int y, uint8_t c)
{
    if (!host_frame) return;
    if (x < 0 || y < 0 || x >= 320 || y >= 240) return;
    if (host_slice_h && y >= host_slice_h) return;
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

Gfx::Gfx() : p(host_rain_panel()), ox(0), oy(host_slice_oy) {}
Gfx::Gfx(lgfx::v1::LovyanGFX &r) : p(&r), ox(0), oy(host_slice_oy) {}
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
    /* midpoint circle, same pixels as LovyanGFX drawCircle (was 64 sparse points: host-only dotting) */
    int f = 1 - r, ddx = 1, ddy = -2 * r, xx = 0, yy = r;
    const int cx = x - ox, cy = y - oy;
    put_px(cx, cy + r, c); put_px(cx, cy - r, c); put_px(cx + r, cy, c); put_px(cx - r, cy, c);
    while (xx < yy) {
        if (f >= 0) { yy--; ddy += 2; f += ddy; }
        xx++; ddx += 2; f += ddx;
        put_px(cx + xx, cy + yy, c); put_px(cx - xx, cy + yy, c); put_px(cx + xx, cy - yy, c); put_px(cx - xx, cy - yy, c);
        put_px(cx + yy, cy + xx, c); put_px(cx - yy, cy + xx, c); put_px(cx + yy, cy - xx, c); put_px(cx - yy, cy - xx, c);
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
void Gfx::fillRoundRect(int x, int y, int w, int h, int r, uint8_t c)
{
    /* v1016m: real rounded corners (LovyanGFX fillRoundRect semantics) - was a plain rect */
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 1) { fill_raw(x - ox, y - oy, w, h, c); return; }
    fill_raw(x - ox, y - oy + r, w, h - 2 * r, c);
    for (int dy = 0; dy < r; ++dy) {
        const float yy = (float)(r - dy) - 0.5f;
        const int dx = (int)((float)r - sqrtf((float)(r * r) - yy * yy) + 0.5f);
        fill_raw(x - ox + dx, y - oy + dy, w - 2 * dx, 1, c);
        fill_raw(x - ox + dx, y - oy + h - 1 - dy, w - 2 * dx, 1, c);
    }
}
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

extern bool host_rb_clip_ok(int x, int y);
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
                    if (host_rb_clip_ok(ox + col * s + sx, oy + row * s + sy))
                        put_px(ox + col * s + sx, oy + row * s + sy, ink);
        }
    }
}
/* v1017c: device-faithful inks variant (rain's ivory Gibsons). Same as src/ui/botcorps_sprite.cpp:
 * runs go through botcorps_pal_rect on the rain panel, so the hub fade and the cell clip apply. */
void botcorps_pal_rect(lgfx::v1::LovyanGFX &, int, int, int, int, uint8_t);
void botcorps_sprite_draw_inks(lgfx::v1::LovyanGFX &d, int x, int y, int shape, int scale, uint8_t body, uint8_t eye, bool blink)
{
    if (shape < 0 || shape > 3) shape = 0;
    const int s = scale < 1 ? 1 : scale;
    const int w = SPR_W[shape], h = SPR_H[shape];
    const int ox = x - (w * s) / 2, oy = y - (h * s) / 2;
    host_gate_box(0, 0, "SPR", "sprite", ox, oy, w * s, h * s, "-");
    for (int row = 0; row < h; ++row) {
        const char *line = SPR_ROW[shape][row];
        for (int col = 0; col < w;) {
            const char v = line[col];
            int run = 1;
            while (col + run < w && line[col + run] == v) ++run;
            if (v == '1' || (v == '2' && !blink))
                botcorps_pal_rect(d, ox + col * s, oy + row * s, run * s, s, v == '2' ? eye : body);
            col += run;
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
/* ---- v1017a band simulator (gate F*: flicker) --------------------------------------------
 * host_band_sim=1 emulates botcorps_band.cpp: one 120-row slice resident in the sprite, a patch in
 * the other slice first recomposes that slice (s_recompose), then draws clipped, then the slice's
 * changed rows are pushed to the panel. Every push is checked against a full reference paint at the
 * same millis(): pixels the panel newly shows that differ from the reference are a visible glitch. */
int host_band_sim = 0;
uint8_t host_sim_panel[320 * 240];
/* v1017c: the sim is now the device band exactly: ONE 320x120 sprite (rows 0..119 of s_spr), painted
 * per slice with Gfx oy = 0 / 120 (host_slice_oy) and clipped to the sprite height (host_slice_h),
 * never cleared between slices/patches - what botcorps_band.cpp does on the ESP32. */
static uint8_t s_spr[320 * 240];
static uint8_t s_sim_ref[320 * 240];
static int s_sim_res = -1;
static void (*s_sim_recomp)(Gfx) = nullptr;
long host_sim_presents = 0, host_sim_pushes = 0, host_sim_recomposes = 0, host_sim_bad = 0, host_sim_worst = 0;
long host_sim_rects = 0;
extern void botcorps_host_ref_paint(uint8_t *buf);
static void sim_ref(void)
{
    uint8_t *saved = host_frame;
    const int so = host_slice_oy, sh = host_slice_h;
    const bool sc = s_clip;
    host_slice_oy = 0; host_slice_h = 0; s_clip = false;
    botcorps_host_ref_paint(s_sim_ref);
    host_slice_oy = so; host_slice_h = sh; s_clip = sc;
    host_frame = saved;
}
/* push rows [r0, r1) of the sprite (columns [x0, x1)) to panel rows y0 + r */
static void sim_push_rect(int y0, int r0, int r1, int x0, int x1)
{
    sim_ref();
    long bad = 0;
    for (int r = r0; r < r1; ++r) {
        const int y = y0 + r;
        const uint8_t *w = s_spr + r * 320, *pn = host_sim_panel + y * 320, *rf = s_sim_ref + y * 320;
        if (!memcmp(w + x0, pn + x0, (size_t)(x1 - x0))) continue;   /* row hash unchanged: not pushed */
        for (int x = x0; x < x1; ++x) bad += (w[x] != pn[x] && w[x] != rf[x]);
        memcpy(host_sim_panel + y * 320 + x0, w + x0, (size_t)(x1 - x0));
    }
    ++host_sim_pushes;
    if (bad > host_sim_worst) host_sim_worst = bad;
    if (bad > 600) ++host_sim_bad;
}
static void sim_push(int y0) { sim_push_rect(y0, 0, 120, 0, 320); }
void host_sim_reset(void)
{
    s_sim_res = -1;
    host_sim_presents = host_sim_pushes = host_sim_recomposes = host_sim_bad = host_sim_worst = 0;
    host_sim_rects = 0;
}
static void sim_paint_slice(void (*draw)(Gfx), int slice)
{
    uint8_t *saved = host_frame;
    host_frame = s_spr;
    host_slice_oy = slice * 120;
    host_slice_h = 120;
    s_clip = false;
    draw(Gfx());
    s_clip = false;
    host_slice_oy = 0;
    host_slice_h = 0;
    host_frame = saved;
}
void botcorps_band_present(void (*draw)(Gfx))
{
    if (!host_band_sim || !draw) return;
    ++host_sim_presents;
    for (int sl = 0; sl < 2; ++sl) {
        sim_paint_slice(draw, sl);
        sim_push(sl * 120);
        s_sim_res = sl;
    }
}
bool botcorps_band_direct(void) { return !host_band_sim; }
bool botcorps_band_live(void) { return host_band_sim != 0; }
void botcorps_band_patch(void (*draw)(Gfx), int x, int y, int w, int h)
{
    if (!draw) return;
    if (!host_band_sim) { draw(Gfx()); return; }
    if (y < 120 && y + h > 120) return;                  /* device refuses a patch across y=120 */
    const int slice = y / 120, y0 = slice * 120;
    if (s_sim_res != slice) {
        if (!s_sim_recomp) return;
        sim_paint_slice(s_sim_recomp, slice);            /* recompose_slice: full slice paint + push */
        sim_push(y0);
        ++host_sim_recomposes;
        s_sim_res = slice;
    }
    uint8_t *saved = host_frame;
    host_frame = s_spr;
    host_slice_oy = y0;
    host_slice_h = 120;
    s_clip = true; s_clip_x = x; s_clip_y = y - y0; s_clip_w = w; s_clip_h = h;
    draw(Gfx());
    s_clip = false;
    host_slice_oy = 0;
    host_slice_h = 0;
    host_frame = saved;
    sim_push(y0);
    s_sim_res = slice;
}
/* v1017c: device botcorps_band_patch_rect. When the slice is not the one the sprite holds, the rect
 * still contains the OTHER slice's pixels on the device; the sim poisons it (0xE3) first, so a painter
 * that is not opaque over its rect shows up as glitch pixels in the S1 compare. Only the rect is pushed. */
void botcorps_band_patch_rect(void (*draw)(Gfx), int x, int y, int w, int h)
{
    if (!draw || w <= 0 || h <= 0) return;
    if (!host_band_sim) { draw(Gfx()); return; }
    if (x < 0) { w += x; x = 0; }
    if (x + w > 320) w = 320 - x;
    if (w <= 0) return;
    if (y < 120 && y + h > 120) return;
    const int slice = y / 120, y0 = slice * 120;
    const bool held = s_sim_res == slice;
    if (!held)
        for (int r = y - y0; r < y - y0 + h && r < 120; ++r) memset(s_spr + r * 320 + x, 0xE3, (size_t)w);
    uint8_t *saved = host_frame;
    host_frame = s_spr;
    host_slice_oy = y0;
    host_slice_h = 120;
    s_clip = true; s_clip_x = x; s_clip_y = y - y0; s_clip_w = w; s_clip_h = h;
    draw(Gfx());
    s_clip = false;
    host_slice_oy = 0;
    host_slice_h = 0;
    host_frame = saved;
    sim_push_rect(y0, y - y0, (y - y0 + h) < 120 ? (y - y0 + h) : 120, x, x + w);
    ++host_sim_rects;
    if (!held) s_sim_res = -1;
}
void botcorps_band_set_recompose(void (*f)(Gfx)) { s_sim_recomp = f; }
int botcorps_band_oy(void) { return host_slice_oy; }
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
