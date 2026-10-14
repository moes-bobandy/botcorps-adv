#include "ui/botcorps_band.h"

#include "display/botcorps_display.h"

#include <esp_heap_caps.h>
#include <lgfx/v1/LGFX_Sprite.hpp>
#include <lgfx/v1/misc/colortype.hpp>
#include <stdio.h>

uint32_t botcorps_t0_row_hash[240];
uint16_t botcorps_t0_lut565[256];

static lgfx::LGFX_Sprite *s_sp = nullptr;
static bool s_live = false;
static bool s_direct = false;
static int s_oy = 0;
static uint32_t s_largest = 0;
static uint32_t s_frame = 0;
static volatile uint32_t s_touch_sink;

static const int SLICE_H =
#if BOTCORPS_FRAME_MODE == FRAME
    240;
#else
    120;
#endif
static const int SLICE_N =
#if BOTCORPS_FRAME_MODE == FRAME
    1;
#else
    2;
#endif

int botcorps_band_bytes(void)
{
    return 320 * SLICE_H;
}

const char *botcorps_band_mode_name(void)
{
#if BOTCORPS_FRAME_MODE == FRAME
    return "FRAME";
#else
    return "BANDS";
#endif
}

bool botcorps_band_direct(void)
{
    return s_direct || !s_live;
}

bool botcorps_band_live(void)
{
    return s_live;
}

int botcorps_band_oy(void)
{
    return s_oy;
}

void botcorps_band_set_oy(int y)
{
    s_oy = y;
}

uint32_t botcorps_band_largest(void)
{
    return s_largest;
}

lgfx::LovyanGFX *botcorps_band_sprite(void)
{
    return s_sp;
}

static uint16_t pack332(uint8_t raw)
{
    lgfx::rgb332_t c;
    c.raw = raw;
    const int r5 = c.R8() >> 3;
    const int g6 = c.G8() >> 2;
    const int b5 = c.B8() >> 3;
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

/* Exact MULTI rain colours, native RGB565, indices 216..222. 0xFDC9 is one of these seven. */
static const uint16_t k_lo[] = {
    0xFDC9, /* PAL_R_FDC9 */
    0xF689, /* PAL_R_F689 */
    0xFA69, /* PAL_R_FA69 */
    0xE2FF, /* PAL_R_E2FF */
    0xCF89, /* PAL_R_CF89 */
    0xFC43, /* PAL_R_FC43 */
    0x6BBF, /* PAL_R_6BBF */
};
static_assert(sizeof(k_lo) / sizeof(k_lo[0]) == (PAL_LO_HI - PAL_LO_FIRST),
              "low reserved lut span");

/* Exact mock hexes, native RGB565, in palette-index order from PAL_FIELD. */
static const uint16_t k_res[] = {
    0x0041, /* PAL_FIELD #070a08 */
    0x0862, /* PAL_BG */
    0x10C4, /* PAL_BAR */
    0x29C8, /* PAL_LINE */
    0xE77E, /* PAL_INK */
    0x8CD4, /* PAL_MUTED */
    0xF5A9, /* PAL_AMBER */
    0x1105, /* PAL_SEL */
    0xFFFF, /* PAL_WHITE */
    0x0841, /* PAL_DEEP */
    0xF79C, /* PAL_IVORY */
    0x8FF3, /* PAL_OK */
    0xE38E, /* PAL_HOLD */
    0x4E7B, /* PAL_CYAN */
    0x5FF8, /* PAL_MINT */
    0x1082, /* PAL_PLATE */
    0xD69A, /* PAL_BOARD_L */
    0x18C3, /* PAL_BOARD_D */
    0x0080, /* PAL_EYE */
    0x7E6C, /* PAL_CODE */
    0xEFF9, /* PAL_HEAD */
    0x5D3F, /* PAL_B5D3F */
    0x4E7F, /* PAL_B4E7F */
    0x7711, /* PAL_B7711 */
    0x9F0B, /* PAL_B9F0B */
    0xFB72, /* PAL_BFB72 */
    0xAC7F, /* PAL_BAC7F */
    0x36B8, /* PAL_B36B8 */
    0xFC6B, /* PAL_BFC6B */
    0xBD3F, /* PAL_BBD3F */
    0x4A69, /* PAL_DIAG */
};
static_assert(sizeof(k_res) / sizeof(k_res[0]) == (PAL_RES_HI - PAL_RES_LO),
              "reserved lut span");

/* Head, code, #9dcf72, nine fixed bots, nine other MULTI inks. Six steps each. */
static const uint16_t k_ramp_src[] = {
    0xEFF9, 0x7E6C, 0x9E6E,
    0x5D3F, 0x4E7F, 0x7711, 0x9F0B, 0xFB72, 0xAC7F, 0x36B8, 0xFC6B, 0xBD3F,
    0xFDC9, 0xF689, 0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8,
};
static_assert(sizeof(k_ramp_src) / sizeof(k_ramp_src[0]) ==
                  (PAL_RAMP_END - PAL_RAMP_HEAD) / PAL_RAMP_N,
              "ramp sources match the slot block");
static const int k_ramp_age[PAL_RAMP_N] = { 0, 4, 8, 12, 15, 19 };

static void ch8(uint16_t native, int *r, int *g, int *b)
{
    const int r5 = (native >> 11) & 31;
    const int g6 = (native >> 5) & 63;
    const int b5 = native & 31;
    *r = (r5 << 3) + (r5 >> 2);
    *g = (g6 << 2) + (g6 >> 4);
    *b = (b5 << 3) + (b5 >> 2);
}

static uint16_t fade_once(uint16_t native)
{
    int r, g, b;
    ch8(native, &r, &g, &b);
    r = (r * 210 + 3 * 46) >> 8;
    g = (g * 210 + 12 * 46) >> 8;
    b = (b * 210 + 6 * 46) >> 8;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t ramp_native(int ink, int step)
{
    uint16_t c = k_ramp_src[ink];
    const int age = k_ramp_age[step];
    for (int k = 0; k < age; ++k) c = fade_once(c);
    return c;
}

static uint16_t native_at(int i)
{
    if (i >= PAL_RAMP_HEAD && i < PAL_RAMP_END) {
        const int o = i - PAL_RAMP_HEAD;
        return ramp_native(o / PAL_RAMP_N, o % PAL_RAMP_N);
    }
    if (i == PAL_BOT_BACK) return 0x0061; /* #030c06, the mock bot backing */
    if (i >= PAL_LO_FIRST && i < PAL_LO_HI) return k_lo[i - PAL_LO_FIRST];
    if (i >= PAL_RES_LO && i < PAL_RES_HI) return k_res[i - PAL_RES_LO];
    return pack332((uint8_t)i);
}

static uint8_t quant332(uint16_t native)
{
    int r, g, b;
    ch8(native, &r, &g, &b);
    return (uint8_t)(((((r >> 5) << 3) + (g >> 5)) << 2) + (b >> 6));
}

static int dist2(uint16_t a, uint16_t b)
{
    int ar, ag, ab, br, bg, bb;
    ch8(a, &ar, &ag, &ab);
    ch8(b, &br, &bg, &bb);
    const int dr = ar - br;
    const int dg = ag - bg;
    const int db = ab - bb;
    return dr * dr + dg * dg + db * db;
}

static bool reserved_slot(int i)
{
    if (i >= PAL_RAMP_HEAD && i < PAL_RAMP_END) return true;
    if (i == PAL_BOT_BACK) return true;
    if (i >= PAL_LO_FIRST && i < PAL_LO_HI) return true;
    return i >= PAL_RES_LO && i < PAL_RES_HI;
}

static uint8_t nearest_free(uint16_t native)
{
    int best = 0;
    int best_d = 1 << 30;
    for (int i = 0; i < 256; ++i) {
        if (reserved_slot(i)) continue;
        const int d = dist2(native, pack332((uint8_t)i));
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return (uint8_t)best;
}

uint16_t botcorps_pal_native(uint8_t idx)
{
    return __builtin_bswap16(botcorps_t0_lut565[idx]);
}

uint8_t botcorps_pal_ramp(int ink, int age)
{
    if (ink < 0) ink = 0;
    if (ink >= (int)(sizeof(k_ramp_src) / sizeof(k_ramp_src[0]))) {
        ink = (int)(sizeof(k_ramp_src) / sizeof(k_ramp_src[0])) - 1;
    }
    if (age < 0) age = 0;
    if (age > 19) age = 19;
    const int step = age * (PAL_RAMP_N - 1) / 19;
    return (uint8_t)(PAL_RAMP_HEAD + ink * PAL_RAMP_N + step);
}

uint8_t botcorps_pal_fit(uint16_t native)
{
    for (int i = 0; i < (int)(sizeof(k_lo) / sizeof(k_lo[0])); ++i) {
        if (k_lo[i] == native) return (uint8_t)(PAL_LO_FIRST + i);
    }
    for (int i = 0; i < (int)(sizeof(k_res) / sizeof(k_res[0])); ++i) {
        if (k_res[i] == native) return (uint8_t)(PAL_RES_LO + i);
    }
    const uint8_t q = quant332(native);
    if (reserved_slot(q)) return nearest_free(native);
    return q;
}

static bool to_band(const lgfx::LovyanGFX *p)
{
    return s_sp && p == static_cast<const lgfx::LovyanGFX *>(s_sp);
}

template <class Draw>
static void with_idx(lgfx::LovyanGFX *p, uint8_t idx, Draw draw)
{
    if (to_band(p)) draw(lgfx::rgb332_t(idx));
    else draw(botcorps_pal_native(idx));
}

void botcorps_pal_rect(lgfx::LovyanGFX &dst, int x, int y, int w, int h, uint8_t idx)
{
    with_idx(&dst, idx, [&](auto ink) { dst.fillRect(x, y, w, h, ink); });
}

void botcorps_pal_pixel(lgfx::LovyanGFX &dst, int x, int y, uint8_t idx)
{
    with_idx(&dst, idx, [&](auto ink) { dst.drawPixel(x, y, ink); });
}

void botcorps_pal_text(lgfx::LovyanGFX &dst, uint8_t idx)
{
    /* uint8_t on the band round-trips through RGB888 back to the same index byte. */
    if (to_band(&dst)) dst.setTextColor(idx);
    else dst.setTextColor(botcorps_pal_native(idx));
}

void botcorps_band_touch(void)
{
    uint32_t acc = 0;
    volatile uint32_t *h = botcorps_t0_row_hash;
    for (int i = 0; i < 240; ++i) {
        const uint32_t v = h[i];
        h[i] = v ^ 0u;
        acc ^= h[i];
    }
    volatile uint16_t *t = botcorps_t0_lut565;
    for (int i = 0; i < 256; ++i) {
        const uint16_t native = native_at(i);
        const uint16_t swapped = __builtin_bswap16(native);
        t[i] = swapped;
        acc ^= t[i];
    }
    s_touch_sink = acc;
}

static void close_panel(void)
{
    if (!botcorps_dual_ok()) return;
    auto &p = botcorps_content();
    p.endWrite();
    p.clearClipRect();
}

void botcorps_band_release(void)
{
    s_live = false;
    s_oy = 0;
    if (s_sp) {
        s_sp->endWrite();
        s_sp->clearClipRect();
        delete s_sp;
        s_sp = nullptr;
    }
    close_panel();
}

bool botcorps_band_rebuild(void)
{
    botcorps_band_release();
    s_largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    if (s_direct || !botcorps_dual_ok()) {
        s_live = false;
        Serial.printf("[botcorps] band mode=%s alloc=0 bytes=%d largest=%u direct=1\n",
                      botcorps_band_mode_name(), botcorps_band_bytes(),
                      (unsigned)s_largest);
        return false;
    }
    s_sp = new lgfx::LGFX_Sprite(&botcorps_content());
    s_sp->setColorDepth(8);
    if (!s_sp->createSprite(320, SLICE_H)) {
        delete s_sp;
        s_sp = nullptr;
        s_direct = true;
        s_live = false;
        close_panel();
        Serial.printf("[botcorps] band mode=%s alloc=0 bytes=%d largest=%u direct=1\n",
                      botcorps_band_mode_name(), botcorps_band_bytes(),
                      (unsigned)s_largest);
        return false;
    }
    s_live = true;
    s_largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    Serial.printf("[botcorps] band mode=%s alloc=1 bytes=%d largest=%u direct=0\n",
                  botcorps_band_mode_name(), botcorps_band_bytes(),
                  (unsigned)s_largest);
    return true;
}

Gfx::Gfx(lgfx::LovyanGFX &d) : p(&d), oy(botcorps_band_oy()) {}

void Gfx::fillRect(int x, int y, int w, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillRect(x, y - oy, w, h, ink); });
}
void Gfx::drawRect(int x, int y, int w, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawRect(x, y - oy, w, h, ink); });
}
void Gfx::fillCircle(int x, int y, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillCircle(x, y - oy, r, ink); });
}
void Gfx::drawCircle(int x, int y, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawCircle(x, y - oy, r, ink); });
}
void Gfx::drawLine(int x0, int y0, int x1, int y1, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawLine(x0, y0 - oy, x1, y1 - oy, ink); });
}
void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c)
{
    with_idx(p, c, [&](auto ink) {
        p->fillTriangle(x0, y0 - oy, x1, y1 - oy, x2, y2 - oy, ink);
    });
}
void Gfx::drawFastHLine(int x, int y, int w, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawFastHLine(x, y - oy, w, ink); });
}
void Gfx::drawFastVLine(int x, int y, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawFastVLine(x, y - oy, h, ink); });
}
void Gfx::fillRoundRect(int x, int y, int w, int h, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillRoundRect(x, y - oy, w, h, r, ink); });
}
void Gfx::fillScreen(uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillScreen(ink); });
}
void Gfx::setClipRect(int x, int y, int w, int h) { p->setClipRect(x, y - oy, w, h); }
void Gfx::clearClipRect(void) { p->clearClipRect(); }
void Gfx::setTextWrap(bool x, bool y) { p->setTextWrap(x, y); }
void Gfx::setTextSize(float s) { p->setTextSize(s); }
void Gfx::setTextSize(float sx, float sy) { p->setTextSize(sx, sy); }
void Gfx::setTextColor(uint8_t c)
{
    if (to_band(p)) p->setTextColor(c);
    else p->setTextColor(botcorps_pal_native(c));
}
void Gfx::setTextColor(uint8_t fg, uint8_t bg)
{
    if (to_band(p)) p->setTextColor(fg, bg);
    else p->setTextColor(botcorps_pal_native(fg), botcorps_pal_native(bg));
}
void Gfx::setTextDatum(int dat) { p->setTextDatum((lgfx::textdatum_t)dat); }
void Gfx::drawString(const char *s, int x, int y) { p->drawString(s ? s : "", x, y - oy); }
void Gfx::setCursor(int x, int y) { p->setCursor(x, y - oy); }
void Gfx::print(const char *s) { p->print(s ? s : ""); }
int Gfx::getCursorX(void) { return p->getCursorX(); }
void Gfx::startWrite(void) { p->startWrite(); }
void Gfx::endWrite(void) { p->endWrite(); }

static uint32_t row_hash(const uint8_t *row, int n)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i += 4) {
        uint32_t w = (uint32_t)row[i];
        if (i + 1 < n) w |= (uint32_t)row[i + 1] << 8;
        if (i + 2 < n) w |= (uint32_t)row[i + 2] << 16;
        if (i + 3 < n) w |= (uint32_t)row[i + 3] << 24;
        h ^= w;
        h *= 16777619u;
    }
    return h;
}

static void push_slice(int world_y0)
{
    if (!s_sp || !botcorps_dual_ok()) return;
    const uint8_t *buf = (const uint8_t *)s_sp->getBuffer();
    if (!buf) return;
    const int stride = (int)(s_sp->bufferLength() / (size_t)SLICE_H);
    const bool full = (s_frame % 64u) == 0u;
    auto &panel = botcorps_content();
    uint16_t line[320];
    panel.startWrite();
    for (int r = 0; r < SLICE_H; ++r) {
        const int world = world_y0 + r;
        const uint8_t *row = buf + (size_t)r * (size_t)stride;
        const uint32_t h = row_hash(row, 320);
        const bool dirty = full || botcorps_t0_row_hash[world] != h;
        if (!dirty) continue;
        botcorps_t0_row_hash[world] = h;
        for (int x = 0; x < 320; ++x) line[x] = botcorps_t0_lut565[row[x]];
        panel.pushImage(0, world, 320, 1, line);
    }
    panel.endWrite();
    panel.clearClipRect();
    s_sp->endWrite();
    s_sp->clearClipRect();
}

void botcorps_band_present(void (*draw)(Gfx))
{
    if (!draw || !s_live || !s_sp || s_direct) return;
    ++s_frame;
    for (int s = 0; s < SLICE_N; ++s) {
        const int y0 = s * SLICE_H;
        botcorps_band_set_oy(y0);
        s_sp->clearClipRect();
        s_sp->setTextWrap(false, false);
        draw(Gfx(*s_sp));
        s_sp->endWrite();
        s_sp->clearClipRect();
        push_slice(y0);
    }
    botcorps_band_set_oy(0);
}
