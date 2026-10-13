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
        const uint16_t v = pack332((uint8_t)i);
        t[i] = v;
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

void Gfx::fillRect(int x, int y, int w, int h, uint16_t c) { p->fillRect(x, y - oy, w, h, c); }
void Gfx::drawRect(int x, int y, int w, int h, uint16_t c) { p->drawRect(x, y - oy, w, h, c); }
void Gfx::fillCircle(int x, int y, int r, uint16_t c) { p->fillCircle(x, y - oy, r, c); }
void Gfx::drawCircle(int x, int y, int r, uint16_t c) { p->drawCircle(x, y - oy, r, c); }
void Gfx::drawLine(int x0, int y0, int x1, int y1, uint16_t c) { p->drawLine(x0, y0 - oy, x1, y1 - oy, c); }
void Gfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c)
{
    p->fillTriangle(x0, y0 - oy, x1, y1 - oy, x2, y2 - oy, c);
}
void Gfx::drawFastHLine(int x, int y, int w, uint16_t c) { p->drawFastHLine(x, y - oy, w, c); }
void Gfx::drawFastVLine(int x, int y, int h, uint16_t c) { p->drawFastVLine(x, y - oy, h, c); }
void Gfx::fillRoundRect(int x, int y, int w, int h, int r, uint16_t c)
{
    p->fillRoundRect(x, y - oy, w, h, r, c);
}
void Gfx::fillScreen(uint16_t c) { p->fillScreen(c); }
void Gfx::setClipRect(int x, int y, int w, int h) { p->setClipRect(x, y - oy, w, h); }
void Gfx::clearClipRect(void) { p->clearClipRect(); }
void Gfx::setTextWrap(bool x, bool y) { p->setTextWrap(x, y); }
void Gfx::setTextSize(float s) { p->setTextSize(s); }
void Gfx::setTextSize(float sx, float sy) { p->setTextSize(sx, sy); }
void Gfx::setTextColor(uint16_t c) { p->setTextColor(c); }
void Gfx::setTextColor(uint16_t fg, uint16_t bg) { p->setTextColor(fg, bg); }
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
