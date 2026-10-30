#pragma once
/*
 * lv_draw - band renderer for the cyd_livescan overlays (portable: firmware + host sim).
 * An overlay region is drawn as horizontal bands into one RGB565 buffer (native
 * endian) and handed to blit(); nothing outside the region is touched, so the
 * LOCKED clip around it is never redrawn and a region repaint does not flicker.
 */
#include <stdint.h>
#include <string.h>
#include "lv_font.h"
#include "lv_icons.h"

typedef void (*lv_blit_fn)(int x, int y, int w, int h, const uint16_t *px);

#define LV_BAND_PX (240 * 24)
struct LvBand { uint16_t *px; int x0, y0, w, h; };

static inline uint16_t lv_rgb(uint8_t r, uint8_t g, uint8_t b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }
static inline uint16_t lv_hex(uint32_t v) { return lv_rgb((uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v); }
static inline uint16_t lv_dim(uint16_t c, int num, int den)
{
    int r = (c >> 11) * num / den, g = ((c >> 5) & 63) * num / den, b = (c & 31) * num / den;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static inline void b_fill(LvBand &B, int x, int y, int w, int h, uint16_t c)
{
    int x0 = x > B.x0 ? x : B.x0, y0 = y > B.y0 ? y : B.y0;
    int x1 = x + w < B.x0 + B.w ? x + w : B.x0 + B.w, y1 = y + h < B.y0 + B.h ? y + h : B.y0 + B.h;
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t *p = B.px + (yy - B.y0) * B.w + (x0 - B.x0);
        for (int xx = x0; xx < x1; ++xx) *p++ = c;
    }
}
static inline void b_pix(LvBand &B, int x, int y, uint16_t c)
{
    if (x >= B.x0 && x < B.x0 + B.w && y >= B.y0 && y < B.y0 + B.h) B.px[(y - B.y0) * B.w + (x - B.x0)] = c;
}
static inline void b_frame(LvBand &B, int x, int y, int w, int h, uint16_t c)
{
    b_fill(B, x, y, w, 1, c); b_fill(B, x, y + h - 1, w, 1, c); b_fill(B, x, y, 1, h, c); b_fill(B, x + w - 1, y, 1, h, c);
}
/* glcd 5x7, advance 6*s. Returns end x. */
static inline int b_text(LvBand &B, int x, int y, const char *s, uint16_t c, int sc = 1)
{
    for (; *s; ++s, x += 6 * sc) {
        int ch = (uint8_t)*s; if (ch < 0x20 || ch > 0x7e) ch = '?';
        if (x >= B.x0 + B.w || x + 5 * sc <= B.x0 || y >= B.y0 + B.h || y + 8 * sc <= B.y0) continue;
        const uint8_t *g = LV_GLCD + (ch - 0x20) * 5;
        for (int cx = 0; cx < 5; ++cx)
            for (int cy = 0; cy < 8; ++cy)
                if (g[cx] >> cy & 1) { if (sc == 1) b_pix(B, x + cx, y + cy, c); else b_fill(B, x + cx * sc, y + cy * sc, sc, sc, c); }
    }
    return x;
}
/* 3x5 caps, advance 4. */
static inline int b_text35(LvBand &B, int x, int y, const char *s, uint16_t c)
{
    for (; *s; ++s, x += 4) {
        int ch = (uint8_t)*s; if (ch < 0x20 || ch > 0x7e) ch = '?';
        const uint16_t g = LV_F35[ch - 0x20];
        for (int k = 0; k < 15; ++k) if (g >> (14 - k) & 1) b_pix(B, x + k % 3, y + k / 3, c);
    }
    return x;
}
static inline int lv_w35(const char *s) { return (int)strlen(s) * 4 - 1; }
static inline void b_icon(LvBand &B, int x, int y, int id, int num = 1, int den = 1)
{
    if (id < 0 || id >= LV_IC_N) return;
    if (y >= B.y0 + B.h || y + 16 <= B.y0) return;
    for (int r = 0; r < 16; ++r)
        for (int k = 0; k < 16; ++k) {
            const char ch = LV_ICON_ART[id][r][k];
            if (ch == '.') continue;
            uint16_t c = lv_icon_ink(id, ch);
            if (num != den) c = lv_dim(c, num, den);
            b_pix(B, x + k, y + r, c);
        }
}

struct LvRect { int16_t x0, y0, x1, y1; };  /* inclusive */
static inline bool lv_in(const LvRect &r, int x, int y) { return x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1; }

/* Render region r band by band through scene(B, ctx). */
typedef void (*lv_scene_fn)(LvBand &B, void *ctx);
static uint16_t lv_bandbuf[LV_BAND_PX];
static inline void lv_render(lv_blit_fn blit, const LvRect &r, lv_scene_fn scene, void *ctx)
{
    const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    if (w <= 0 || h <= 0) return;
    int rows = LV_BAND_PX / w; if (rows > h) rows = h;
    for (int y = r.y0; y <= r.y1; y += rows) {
        const int bh = (y + rows - 1 <= r.y1) ? rows : r.y1 - y + 1;
        LvBand B{lv_bandbuf, r.x0, y, w, bh};
        scene(B, ctx);
        blit(r.x0, y, w, bh, lv_bandbuf);
    }
}
