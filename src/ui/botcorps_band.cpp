#include "ui/botcorps_band.h"

#include "display/botcorps_display.h"

#include <Arduino.h>
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
static int s_ox = 0;
static int s_resident = -1;
static void (*s_recompose)(Gfx) = nullptr;
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

#if BOTCORPS_FRAME_MODE == FRAME
extern "C" const char botcorps_mode_frame[] = "FRAME";
#else
extern "C" const char botcorps_mode_bands[] = "BANDS";
#endif

const char *botcorps_band_mode_name(void)
{
#if BOTCORPS_FRAME_MODE == FRAME
    return botcorps_mode_frame;
#else
    return botcorps_mode_bands;
#endif
}

/* Diagnostic only. The screen that follows is always BANDS.
 * 153600 is the full RGB565 frame. The NimBLE gate is probe.cpp:93. */
void botcorps_frame_diag(void)
{
    const uint32_t bytes = 153600u;
    const uint32_t margin = 4096u;
    const uint32_t gate = 48u * 1024u;
    const uint32_t largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const char *pick = "BANDS";
    if (largest >= bytes + margin) {
        void *buf = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL);
        if (buf) {
            const uint32_t after = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
            if (after >= gate) pick = "FRAME";
            heap_caps_free(buf);
        }
    }
    Serial.printf("[botcorps] frame diag largest=%u need=%u gate=%u pick=%s\n",
                  (unsigned)largest, (unsigned)(bytes + margin), (unsigned)gate, pick);
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

void botcorps_band_set_ox(int x)
{
    s_ox = x;
}

void botcorps_band_set_recompose(void (*draw)(Gfx))
{
    s_recompose = draw;
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

#include "botcorps_pal_tables.inc"

static uint16_t native_at(int i)
{
    if (k_is_res[i]) return k_exact_nat[i];
    return pack332((uint8_t)i);
}

#include "botcorps_pal_fit.inc"
#include "botcorps_pal_hub.inc"

static bool s_hub_dim = false;

void botcorps_pal_hub_set(bool on)
{
    s_hub_dim = on;
}

uint8_t botcorps_pal_hub_of(uint8_t idx)
{
    return k_hub_fade[idx];
}

static uint8_t hub_map(uint8_t idx)
{
    return s_hub_dim ? k_hub_fade[idx] : idx;
}

uint16_t botcorps_pal_native(uint8_t idx)
{
    return __builtin_bswap16(botcorps_t0_lut565[idx]);
}

uint8_t botcorps_pal_ramp(int ink, int age)
{
    if (ink < 0) ink = 0;
    if (ink >= k_ramp_n) ink = k_ramp_n - 1;
    if (age < 0) age = 0;
    if (age > 13) age = 13;
    const int step = age * (PAL_RAMP_N - 1) / 13;
    return k_ramp_idx[ink][step];
}

bool botcorps_pal_ramp_for(uint16_t native, int age, uint8_t *out)
{
    if (!out) return false;
    for (int i = 0; i < k_ramp_n; ++i) {
        if (k_ramp_src[i] != native) continue;
        *out = botcorps_pal_ramp(i, age);
        return true;
    }
    return false;
}

uint16_t botcorps_rgb565(uint8_t idx)
{
    if (k_is_res[idx]) return k_exact_nat[idx];
    return pack332(idx);
}

#include "ui/botcorps_term_ov.h"

/* Sheet colours stay out of botcorps_t0_lut565. The flag only changes
 * the words pushed to the EXT panel. INT and the strip sprite keep reading
 * the shared lookup through botcorps_pal_native. */
static bool s_term_lut = false;

void botcorps_lut_term(bool on)
{
    s_term_lut = on;
}

static uint16_t term_ov_native(uint8_t idx)
{
    return botcorps_term_ov_native(idx, s_term_lut ? 1 : 0);
}

/* Pre-swapped word for pushImage onto EXT. */
static uint16_t ext_push_word(uint8_t idx)
{
    const uint16_t nat = term_ov_native(idx);
    if (nat) return __builtin_bswap16(nat);
    return botcorps_t0_lut565[idx];
}

/* Normal-order word for a direct draw on the EXT panel. */
static uint16_t ext_draw_word(uint8_t idx)
{
    const uint16_t nat = term_ov_native(idx);
    if (nat) return nat;
    return botcorps_pal_native(idx);
}

uint8_t botcorps_pal_fit(uint16_t native)
{
    for (int n = 0; n < k_own_n; ++n) {
        const uint8_t i = k_own_idx[n];
        if (k_exact_nat[i] == native) return i;
    }
    const unsigned r = (native >> 11) & 31;
    const unsigned g = (native >> 5) & 63;
    const unsigned b = native & 31;
    const unsigned key = ((r >> 1) << 8) | ((g >> 2) << 4) | (b >> 1);
    return k_fit4[key];
}

static void check_reserved_fit(void)
{
    for (int n = 0; n < k_own_n; ++n) {
        const uint8_t i = k_own_idx[n];
        const uint8_t got = botcorps_pal_fit(k_exact_nat[i]);
        if (got != i) {
            Serial.printf("[botcorps] pal fit miss idx=%u native=%04x got=%u direct=1\n",
                          (unsigned)i, (unsigned)k_exact_nat[i], (unsigned)got);
            s_direct = true;
            return;
        }
    }
}

static bool to_band(const lgfx::LovyanGFX *p)
{
    return s_sp && p == static_cast<const lgfx::LovyanGFX *>(s_sp);
}

static bool on_menu(const lgfx::LovyanGFX *p)
{
    return p == static_cast<const lgfx::LovyanGFX *>(&botcorps_menu());
}

static bool on_ext_panel(const lgfx::LovyanGFX *p)
{
    return p == static_cast<const lgfx::LovyanGFX *>(&botcorps_content());
}

template <class Draw>
static void with_idx(lgfx::LovyanGFX *p, uint8_t idx, Draw draw)
{
    if (to_band(p)) draw(lgfx::rgb332_t(idx));
    else if (on_menu(p)) draw(botcorps_rgb565(idx));
    else if (on_ext_panel(p)) draw(ext_draw_word(idx));
    else draw(botcorps_pal_native(idx));
}

void botcorps_pal_rect(lgfx::LovyanGFX &dst, int x, int y, int w, int h, uint8_t idx)
{
    idx = hub_map(idx);
    with_idx(&dst, idx, [&](auto ink) { dst.fillRect(x, y, w, h, ink); });
}

void botcorps_pal_pixel(lgfx::LovyanGFX &dst, int x, int y, uint8_t idx)
{
    idx = hub_map(idx);
    with_idx(&dst, idx, [&](auto ink) { dst.drawPixel(x, y, ink); });
}

void botcorps_pal_text(lgfx::LovyanGFX &dst, uint8_t idx)
{
    idx = hub_map(idx);
    /* uint8_t on the band round-trips through RGB888 back to the same index byte. */
    if (to_band(&dst)) dst.setTextColor(idx);
    else if (on_menu(&dst)) dst.setTextColor(botcorps_rgb565(idx));
    else if (on_ext_panel(&dst)) dst.setTextColor(ext_draw_word(idx));
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
    check_reserved_fit();
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
    s_ox = 0;
    s_resident = -1;
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

Gfx::Gfx(lgfx::LovyanGFX &d) : p(&d), ox(s_ox), oy(botcorps_band_oy()) {}

void Gfx::fillRect(int x, int y, int w, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillRect(x - s_ox, y - oy, w, h, ink); });
}
void Gfx::fillRect(int x, int y, int w, int h, uint16_t c)
{
    p->fillRect(x - s_ox, y - oy, w, h, c);
}
void Gfx::drawRect(int x, int y, int w, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawRect(x - s_ox, y - oy, w, h, ink); });
}
void Gfx::fillCircle(int x, int y, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillCircle(x - s_ox, y - oy, r, ink); });
}
void Gfx::drawCircle(int x, int y, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawCircle(x - s_ox, y - oy, r, ink); });
}
void Gfx::drawLine(int x0, int y0, int x1, int y1, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawLine(x0 - s_ox, y0 - oy, x1 - s_ox, y1 - oy, ink); });
}
void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c)
{
    with_idx(p, c, [&](auto ink) {
        p->fillTriangle(x0 - s_ox, y0 - oy, x1 - s_ox, y1 - oy, x2 - s_ox, y2 - oy, ink);
    });
}
void Gfx::drawFastHLine(int x, int y, int w, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawFastHLine(x - s_ox, y - oy, w, ink); });
}
void Gfx::drawFastVLine(int x, int y, int h, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->drawFastVLine(x - s_ox, y - oy, h, ink); });
}
void Gfx::fillRoundRect(int x, int y, int w, int h, int r, uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillRoundRect(x - s_ox, y - oy, w, h, r, ink); });
}
void Gfx::fillScreen(uint8_t c)
{
    with_idx(p, c, [&](auto ink) { p->fillScreen(ink); });
}
void Gfx::setClipRect(int x, int y, int w, int h) { p->setClipRect(x - s_ox, y - oy, w, h); }
void Gfx::clearClipRect(void) { p->clearClipRect(); }
void Gfx::setTextWrap(bool x, bool y) { p->setTextWrap(x, y); }
void Gfx::setTextSize(float s) { p->setTextSize(s); }
void Gfx::setTextSize(float sx, float sy) { p->setTextSize(sx, sy); }
void Gfx::setTextColor(uint8_t c)
{
    if (to_band(p)) p->setTextColor(c);
    else if (on_menu(p)) p->setTextColor(botcorps_rgb565(c));
    else if (on_ext_panel(p)) p->setTextColor(ext_draw_word(c));
    else p->setTextColor(botcorps_pal_native(c));
}
void Gfx::setTextColor(uint16_t c)
{
    p->setTextColor(c);
}
void Gfx::setTextColor(uint8_t fg, uint8_t bg)
{
    if (to_band(p)) p->setTextColor(fg, bg);
    else if (on_menu(p)) p->setTextColor(botcorps_rgb565(fg), botcorps_rgb565(bg));
    else if (on_ext_panel(p)) p->setTextColor(ext_draw_word(fg), ext_draw_word(bg));
    else p->setTextColor(botcorps_pal_native(fg), botcorps_pal_native(bg));
}
void Gfx::setTextDatum(int dat) { p->setTextDatum((lgfx::textdatum_t)dat); }
void Gfx::drawString(const char *s, int x, int y) { p->drawString(s ? s : "", x - s_ox, y - oy); }
void Gfx::setCursor(int x, int y) { p->setCursor(x - s_ox, y - oy); }
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
        for (int x = 0; x < 320; ++x) line[x] = ext_push_word(row[x]);
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
        s_resident = s;
    }
    botcorps_band_set_oy(0);
}

static bool recompose_slice(int slice)
{
    if (!s_recompose || !s_sp || !s_live || s_direct) return false;
    if (slice < 0 || slice >= SLICE_N) return false;
    const int y0 = slice * SLICE_H;
    botcorps_band_set_oy(y0);
    s_sp->clearClipRect();
    s_sp->setTextWrap(false, false);
    s_recompose(Gfx(*s_sp));
    s_sp->endWrite();
    s_sp->clearClipRect();
    push_slice(y0);
    s_resident = slice;
    botcorps_band_set_oy(0);
    return true;
}

void botcorps_band_patch(void (*draw)(Gfx), int x, int y, int w, int h)
{
    if (!draw || !s_live || !s_sp || s_direct || w <= 0 || h <= 0) return;
    /* BANDS keeps one 120-row slice resident. A patch must not cross y=120. */
    if (SLICE_N > 1 && y < SLICE_H && (y + h) > SLICE_H) {
        Serial.printf("[botcorps] band_patch refused, crosses y=%d (y=%d h=%d)\n",
                      SLICE_H, y, h);
        return;
    }
    const int slice = y / SLICE_H;
    if (slice < 0 || slice >= SLICE_N) return;
    /* After present the sprite still holds the last slice. A patch in the
     * other slice has to rebuild that slice before it draws, or push_slice
     * would send the resident slice's pixels to the wrong rows. */
    if (s_resident != slice && !recompose_slice(slice)) {
        Serial.printf("[botcorps] band_patch refused, slice %d not resident\n", slice);
        return;
    }
    const int y0 = slice * SLICE_H;
    botcorps_band_set_oy(y0);
    s_sp->setTextWrap(false, false);
    s_sp->setClipRect(x, y - y0, w, h);
    draw(Gfx(*s_sp));
    s_sp->clearClipRect();
    push_slice(y0);
    s_resident = slice;
    botcorps_band_set_oy(0);
}
