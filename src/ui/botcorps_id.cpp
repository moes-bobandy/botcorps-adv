#include "ui/botcorps_id.h"

#include "display/botcorps_display.h"
#include "ui/botcorps_sprite.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lgfx/v1/LGFX_Sprite.hpp>
#include <math.h>
#include <stdio.h>
#include <string.h>

static lgfx::LGFX_Sprite *s_id_strip = nullptr;

void botcorps_id_strip_alloc(void)
{
    if (s_id_strip) return;
    auto *sp = new lgfx::LGFX_Sprite(&botcorps_menu());
    sp->setColorDepth(16);
    if (!sp->createSprite(236, 16)) {
        delete sp;
        sp = new lgfx::LGFX_Sprite(&botcorps_menu());
        sp->setColorDepth(16);
        if (!sp->createSprite(236, 8)) {
            delete sp;
            sp = nullptr;
        }
    }
    s_id_strip = sp;
    const uint32_t free_b = (uint32_t)ESP.getFreeHeap();
    const uint32_t largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const uint32_t gate = 48u * 1024u;
    Serial.printf("[botcorps] id strip %s free=%u largest=%u gate=%u %s\n",
                  s_id_strip ? "ok" : "direct",
                  (unsigned)free_b,
                  (unsigned)largest,
                  (unsigned)gate,
                  largest >= gate ? "hold" : "short");
}

/* mod items 12+13 (Mobandy 2026-10-18): the INT BOT ID card is redrawn as THAT BOT'S OWN ESP32 DEV
 * BOARD (CYD-style, matte black, gold edge): its square-chip bot face sits in the WROOM module, the
 * 3 meters stay (bot colour), a GPIO header row shows the pins this bot drives, the badge keeps its
 * lanyard slot + serial/barcode silk. Flip back to the old ibId card with -DBOTCORPS_ID_DEVBOARD=0. */
#ifndef BOTCORPS_ID_DEVBOARD
#define BOTCORPS_ID_DEVBOARD 1
#endif
#if !BOTCORPS_ID_DEVBOARD
/* ID_LAY from the mock. A, C, D, each mirrored by ID_V. Coordinates are the INT 240-wide card. */
struct lay_t {
    int photo[4];
    int chip[4];
    int bars[3][2];
    int bw;
    int name[4];
    int code[4];
    int ant[4];
    int usb[4];
    int oled[4];
    int btn[2][2];
    int slot[4];
    int pat;
};

static const lay_t LAY[3] = {
    { { 8, 32, 44, 52 }, { 60, 38, 34, 34 }, { { 106, 42 }, { 106, 62 }, { 106, 82 } }, 76,
      { 60, 96, 122, 22 }, { 8, 90, 44, 26 }, { 190, 32, 40, 18 }, { 226, 56, 12, 16 },
      { 186, 100, 44, 10 }, { { 188, 114 }, { 202, 114 } }, { 96, 27, 36, 5 }, 0 },
    { { 8, 32, 44, 52 }, { 60, 34, 34, 34 }, { { 8, 104 }, { 84, 104 }, { 160, 104 } }, 70,
      { 102, 34, 128, 20 }, { 102, 72, 70, 14 }, { 178, 72, 44, 14 }, { 226, 72, 12, 14 },
      { 102, 58, 128, 10 }, { { 60, 74 }, { 71, 74 } }, { 150, 27, 36, 5 }, 1 },
    { { 8, 32, 44, 52 }, { 150, 38, 36, 36 }, { { 60, 42 }, { 60, 62 }, { 60, 82 } }, 76,
      { 60, 104, 170, 22 }, { 8, 90, 44, 26 }, { 194, 32, 36, 18 }, { 226, 60, 12, 16 },
      { 192, 80, 40, 10 }, { { 196, 92 }, { 209, 92 } }, { 100, 27, 36, 5 }, 2 },
};

/* ibId traces. A has 9, C has 10, D has 7. Points are x,y packed. */
constexpr uint8_t TR_COUNT[3] = { 9, 10, 7 };
constexpr uint8_t TR_LEN[26] = {
    2, 4, 4, 4, 2, 2, 4, 2, 3,
    2, 2, 4, 2, 2, 3, 3, 3, 2, 2,
    4, 4, 4, 4, 2, 2, 3
};
static const uint8_t TR_XY[] = {
    52,55, 60,55,
    94,46, 98,46, 98,47, 106,47,
    94,55, 101,55, 101,67, 106,67,
    94,64, 104,64, 104,87, 106,87,
    72,72, 72,96,
    82,72, 82,96,
    84,38, 84,33, 210,33, 210,32,
    182,107, 186,105,
    226,64, 216,64, 216,100,
    52,50, 60,50,
    94,44, 102,44,
    94,56, 98,56, 98,63, 102,63,
    88,68, 88,92,
    80,92, 232,92,
    80,92, 80,109, 78,109,
    156,92, 156,109, 154,109,
    232,92, 232,109, 230,109,
    172,79, 178,79,
    222,79, 226,79,
    150,46, 144,46, 144,47, 137,47,
    150,56, 141,56, 141,67, 137,67,
    150,66, 138,66, 138,87, 137,87,
    180,38, 180,34, 212,34, 212,32,
    168,74, 168,104,
    190,85, 192,85,
    226,68, 218,68, 218,80,
};

constexpr int tr_point_n(void)
{
    int n = 0;
    for (int i = 0; i < 26; ++i) n += TR_LEN[i];
    return n;
}
static_assert(TR_COUNT[0] == 9 && TR_COUNT[1] == 10 && TR_COUNT[2] == 7, "A C D trace counts");
static_assert(tr_point_n() * 2 == (int)sizeof(TR_XY), "trace points");

/* ASCII stand-ins for the mock insignia. Font0 cannot draw those glyphs. */
static const char *const INS[12] = {
    "~", "*", "/", "#", "#", "*", "+", "[", "+", "~", "^", "o"
};
/* 0 ready, 1 limited, 2 hold. Fixed per bot. */
static const uint8_t ST[12] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1 };

static const uint16_t C_FACE = 0xAD97; /* #aab2ba */
static const uint16_t C_BADGE = 0x5B0D; /* #59636d */
static const uint16_t C_INNER = 0x10A3; /* #10161b */
static const uint16_t C_PAD = 0x3A29; /* #3a444d */
static const uint16_t C_OFF = 0x1926; /* #1c2733 */
static const uint16_t C_OLED = 0x0021; /* #02060a */
static const uint16_t C_USB = 0x3A2A; /* #3a4650 */
static const uint16_t C_PIN = 0xC65A; /* #c3cad1 */
static const uint16_t C_WHITE = 0xFFFF;

static int mx(bool mirror, int x, int w, int width)
{
    return mirror ? width - x - w : x;
}

static int tr_index(int lay, int i)
{
    int n = 0;
    for (int k = 0; k < lay; ++k) n += TR_COUNT[k];
    return n + i;
}

static const uint8_t *tr_pts(int lay, int i, int *n)
{
    const int idx = tr_index(lay, i);
    int base = 0;
    for (int k = 0; k < idx; ++k) base += TR_LEN[k];
    *n = TR_LEN[idx];
    return TR_XY + base * 2;
}

static int pxx(bool mirror, int width, int x)
{
    return mirror ? width - x : x;
}

static void ib_seg(Gfx g, int x0, int y0, int x1, int y1, uint16_t col)
{
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int adx = dx < 0 ? -dx : dx;
    const int ady = dy < 0 ? -dy : dy;
    int n = adx > ady ? adx : ady;
    if (n < 1) {
        g.fillRect(x0 - 1, y0 - 1, 2, 2, col);
        return;
    }
    for (int k = 0; k <= n; ++k) {
        const int x = (int)lroundf((float)x0 + (float)dx * (float)k / (float)n);
        const int y = (int)lroundf((float)y0 + (float)dy * (float)k / (float)n);
        g.fillRect(x - 1, y - 1, 2, 2, col);
    }
}

static void wire_poly(Gfx g, bool mirror, int width, int lay, int i, uint16_t col)
{
    int n = 0;
    const uint8_t *p = tr_pts(lay, i, &n);
    for (int k = 0; k + 1 < n; ++k) {
        const int x0 = pxx(mirror, width, p[k * 2]);
        const int y0 = p[k * 2 + 1];
        const int x1 = pxx(mirror, width, p[(k + 1) * 2]);
        const int y1 = p[(k + 1) * 2 + 1];
        ib_seg(g, x0, y0, x1, y1, col);
    }
}

static void poly_box(bool mirror, int width, int lay, int i, int *x, int *y, int *w, int *h)
{
    int n = 0;
    const uint8_t *p = tr_pts(lay, i, &n);
    int x0 = 1000;
    int y0 = 1000;
    int x1 = -1;
    int y1 = -1;
    for (int k = 0; k < n; ++k) {
        const int px = pxx(mirror, width, p[k * 2]);
        const int py = p[k * 2 + 1];
        if (px < x0) x0 = px;
        if (py < y0) y0 = py;
        if (px > x1) x1 = px;
        if (py > y1) y1 = py;
    }
    *x = x0 - 2;
    *y = y0 - 2;
    *w = (x1 - x0) + 5;
    *h = (y1 - y0) + 5;
}

static int poly_len(bool mirror, int width, int lay, int i)
{
    int n = 0;
    const uint8_t *p = tr_pts(lay, i, &n);
    int L = 0;
    for (int k = 0; k + 1 < n; ++k) {
        const int x0 = pxx(mirror, width, p[k * 2]);
        const int y0 = p[k * 2 + 1];
        const int x1 = pxx(mirror, width, p[(k + 1) * 2]);
        const int y1 = p[(k + 1) * 2 + 1];
        const int dx = x1 - x0;
        const int dy = y1 - y0;
        L += (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    }
    return L;
}

static void pulse_poly(Gfx g, bool mirror, int width, int lay, int i, int dist, uint16_t col)
{
    const int L = poly_len(mirror, width, lay, i);
    if (L <= 0) return;
    int d = dist % (L + 14);
    if (d > L) return;
    int n = 0;
    const uint8_t *p = tr_pts(lay, i, &n);
    for (int k = 0; k + 1 < n; ++k) {
        const int x0 = pxx(mirror, width, p[k * 2]);
        const int y0 = p[k * 2 + 1];
        const int x1 = pxx(mirror, width, p[(k + 1) * 2]);
        const int y1 = p[(k + 1) * 2 + 1];
        const int dx = x1 > x0 ? x1 - x0 : x0 - x1;
        const int dy = y1 > y0 ? y1 - y0 : y0 - y1;
        const int len = dx + dy;
        if (d <= len) {
            const int sx = (x1 == x0) ? 0 : (x1 > x0 ? 1 : -1);
            const int sy = (y1 == y0) ? 0 : (y1 > y0 ? 1 : -1);
            const int along = d <= dx ? d : dx;
            const int down = d <= dx ? 0 : d - dx;
            g.fillRect(x0 + sx * along - 1, y0 + sy * down - 1, 3, 3, col);
            return;
        }
        d -= len;
    }
}

static void split565(uint16_t p, int *r, int *g, int *b)
{
    const int r5 = (p >> 11) & 31;
    const int g6 = (p >> 5) & 63;
    const int b5 = p & 31;
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

static uint16_t pack565(int r, int g, int b)
{
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t mix_nat(uint16_t a, uint16_t b, int num, int den)
{
    int ar, ag, ab, br, bg, bb;
    const int inv = den - num;
    split565(a, &ar, &ag, &ab);
    split565(b, &br, &bg, &bb);
    return pack565((ar * inv + br * num) / den,
                   (ag * inv + bg * num) / den,
                   (ab * inv + bb * num) / den);
}

struct card_ink {
    uint16_t edge;
    uint16_t tint;
    uint16_t hi;
    uint16_t tr;
};

static const uint16_t MIX_BG = 0x0862;   /* #0a0e12 */
static const uint16_t MIX_EDGE = 0x5B0D; /* #59636d */
static const uint16_t MIX_WIRE = 0x2187; /* #26313a */

static card_ink card_inks(uint8_t col)
{
    const uint16_t b = botcorps_rgb565(col);
    card_ink k;
    k.tint = mix_nat(MIX_BG, b, 12, 100);
    k.edge = mix_nat(MIX_EDGE, b, 35, 100);
    k.hi = mix_nat(k.tint, b, 35, 100);
    k.tr = mix_nat(MIX_WIRE, b, 40, 100);
    return k;
}

/* dfRR from the mock: one pixel row per scan, corners inset by the circle. */
static void df_rr(Gfx g, int x, int y, int w, int h, int r, uint16_t col)
{
    if (w <= 0 || h <= 0) return;
    if (r < 0) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    for (int i = 0; i < h; ++i) {
        int ins = 0;
        const int j = (i < r) ? i : (i >= h - r ? h - 1 - i : -1);
        if (j >= 0) {
            const float inner = (float)r - (float)j - 0.5f;
            const float sq = (float)r * (float)r - inner * inner;
            const int rad = sq > 0.f ? (int)(sqrtf(sq) + 0.5f) : 0;
            ins = r - rad;
            if (ins < 0) ins = 0;
        }
        const int ww = w - 2 * ins;
        if (ww > 0) g.fillRect(x + ins, y + i, ww, 1, col);
    }
}

/* gibsonChip: ivory round body, chin, sunglasses, glint. Not the plain round sprite. */
static void gibson_chip(Gfx g, int cx, int cy, int z)
{
    if (z < 1) z = 1;
    const int ox = cx - (7 * z) / 2;
    const int oy = cy - (7 * z) / 2;
    auto u = [&](int x, int y, int w, int h, uint16_t col) {
        g.fillRect(ox + x * z, oy + y * z, w * z, h * z, col);
    };
    const uint16_t ivory = botcorps_rgb565(PAL_IVORY);
    const uint16_t chin = botcorps_rgb565(PAL_CHIN);
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    const uint16_t steel = botcorps_rgb565(PAL_STEEL);
    const int sx = ox + (z + 1) / 2;
    const int sw = (ox + (13 * z + 1) / 2) - sx;
    g.fillRect(sx, oy + 7 * z, sw > 0 ? sw : z, z > 2 ? 2 : z, black);
    u(3, 0, 1, 1, ivory);
    u(1, 1, 5, 1, ivory);
    u(0, 2, 7, 3, ivory);
    u(0, 5, 7, 1, ivory);
    u(1, 6, 5, 1, ivory);
    u(1, 6, 5, 1, chin);
    u(0, 5, 1, 1, chin);
    u(6, 5, 1, 1, chin);
    u(0, 3, 7, 1, black);
    u(1, 4, 2, 1, black);
    u(4, 4, 2, 1, black);
    const int gl = (int)((millis() / 900u) % 2u);
    u(1 + gl, 3, 1, 1, steel);
    u(4 + gl, 3, 1, 1, steel);
}

static void id_icon(Gfx g, int k, int x, int y, uint16_t col)
{
    auto f = [&](int a, int b, int w, int h) { g.fillRect(x + a, y + b, w, h, col); };
    const uint16_t hole = botcorps_rgb565(PAL_BLACK);
    if (k == 0) { f(2, 0, 4, 3); f(1, 4, 6, 3); }
    else if (k == 1) { f(1, 0, 6, 4); f(2, 4, 4, 2); f(3, 6, 2, 1); }
    else if (k == 2) {
        f(1, 1, 6, 5);
        g.fillRect(x + 3, y + 3, 2, 1, hole);
        f(0, 2, 1, 1); f(7, 2, 1, 1); f(0, 4, 1, 1); f(7, 4, 1, 1);
    } else if (k == 3) {
        f(0, 3, 1, 1); f(1, 2, 1, 3); f(2, 1, 1, 5); f(3, 0, 1, 7);
        f(4, 1, 1, 5); f(5, 2, 1, 3); f(6, 3, 1, 1);
    } else if (k == 4) { f(3, 0, 1, 7); f(1, 3, 5, 1); f(2, 2, 3, 3); }
    else if (k == 5) {
        f(2, 0, 4, 1); f(2, 1, 1, 2); f(5, 1, 1, 2); f(1, 3, 6, 4);
        g.fillRect(x + 3, y + 4, 2, 2, hole);
    } else if (k == 6) {
        f(4, 0, 2, 2); f(3, 2, 2, 2); f(2, 4, 2, 2); f(3, 3, 3, 1); f(4, 5, 1, 2);
    } else {
        f(1, 0, 6, 1); f(1, 6, 6, 1); f(0, 1, 1, 5); f(7, 1, 1, 5);
    }
}

static int icon_kind(int bot, bool gib, int i)
{
    if (gib) return i;
    int v[3] = { bot % 8, (bot * 3 + 1) % 8, (bot * 5 + 2) % 8 };
    for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < k; ++j) {
            if (v[k] == v[j]) v[k] = (v[k] + k + 1) % 8;
        }
    }
    return v[i];
}

static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    if (!s) return h;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        h ^= *p;
        h *= 16777619u;
    }
    return h;
}

static void photo(Gfx g, int x, int y, int w, int h, int bot, bool gib, uint8_t idx, int shape)
{
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    const uint16_t scan = botcorps_rgb565(PAL_BG);
    const uint16_t tick = botcorps_rgb565(PAL_MUTED);
    g.fillRect(x, y, w, h, black);
    for (int yy = y + 1; yy < y + h; yy += 4) g.fillRect(x + 1, yy, w - 2, 1, scan);
    for (int k = 0; k < 6; ++k) {
        g.fillRect(x + k, y, 1, 1, C_FACE);
        g.fillRect(x, y + k, 1, 1, C_FACE);
        g.fillRect(x + w - 1 - k, y, 1, 1, C_FACE);
        g.fillRect(x + w - 1, y + k, 1, 1, C_FACE);
        g.fillRect(x + k, y + h - 1, 1, 1, C_FACE);
        g.fillRect(x, y + h - 1 - k, 1, 1, C_FACE);
        g.fillRect(x + w - 1 - k, y + h - 1, 1, 1, C_FACE);
        g.fillRect(x + w - 1, y + h - 1 - k, 1, 1, C_FACE);
    }
    const int cx = x + w / 2;
    const int cy = y + h / 2 - 2;
    if (gib) {
        gibson_chip(g, cx, cy, 3);
    } else {
        const int bob = ((millis() / 900u + (uint32_t)bot) % 2u) ? 1 : 0;
        botcorps_sprite_draw(*g.raw(), cx - g.ox, cy + bob - g.oy, shape, 4, idx, false);
    }
    g.fillRect(x + w - 12, y + h - 10, 9, 8, C_BADGE);
    for (int k = 0; k < 4; ++k) {
        g.fillRect(x + w - 11 + k * 2, y + h - 9 + (k % 2) * 2, 1, 5, tick);
    }
}

static void chip(Gfx g, int x, int y, int pat, int bot, bool gib, uint8_t idx, int shape)
{
    const uint16_t steel = botcorps_rgb565(PAL_STEEL);
    const uint16_t die = botcorps_rgb565(PAL_DEEP);
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    df_rr(g, x, y, 34, 34, 4, steel);
    df_rr(g, x + 1, y + 1, 32, 32, 3, C_FACE);
    if (pat == 0) {
        g.fillRect(x + 10, y, 1, 34, C_PAD);
        g.fillRect(x + 23, y, 1, 34, C_PAD);
        g.fillRect(x, y + 10, 34, 1, C_PAD);
        g.fillRect(x, y + 23, 34, 1, C_PAD);
    } else if (pat == 1) {
        g.fillRect(x, y + 11, 34, 1, C_PAD);
        g.fillRect(x, y + 22, 34, 1, C_PAD);
        g.fillRect(x + 16, y, 1, 11, C_PAD);
        g.fillRect(x + 16, y + 23, 1, 11, C_PAD);
    } else {
        g.fillRect(x + 16, y, 2, 34, C_PAD);
        g.fillRect(x, y + 16, 34, 2, C_PAD);
    }
    g.fillRect(x + 9, y + 9, 16, 16, C_INNER);
    g.fillRect(x + 10, y + 10, 14, 14, die);
    if (gib) {
        const uint16_t mark = 0x8DBB; /* #8ab4d8, mock idChip */
        g.fillRect(x + 14, y + 14, 6, 6, mark);
        g.fillRect(x + 16, y + 16, 2, 2, C_WHITE);
    } else {
        botcorps_sprite_draw(*g.raw(), x + 17 - g.ox, y + 17 - g.oy, shape, 1, idx, false);
    }
    if (((millis() / 420u) % 2u) == 0u) g.fillRect(x + 28, y + 2, 3, 3, botcorps_rgb565(idx));
    (void)bot;
    (void)black;
}

static int bot_st(int bot, bool gib)
{
    if (gib || bot < 0 || bot > 11) return 0;
    return ST[bot];
}

static void stat_row(int bot, bool gib, int k, int *v100, char *txt, int ntxt)
{
    const uint32_t secs = millis() / 1000u;
    const int m = (int)((secs / 60u) % 100u);
    const int s = (int)(secs % 60u);
    if (gib) {
        if (k == 0) {
            *v100 = 7 * 100 / 12;
            snprintf(txt, (size_t)ntxt, "7/12");
        } else if (k == 1) {
            const float up = 1.f - expf(-(float)secs / 90.f);
            *v100 = (int)(up * 100.f + 0.5f);
            snprintf(txt, (size_t)ntxt, "%02d:%02d", m, s);
        } else {
            float sig = 0.8f + 0.06f * sinf((float)millis() / 700.f);
            if (sig > 0.97f) sig = 0.97f;
            *v100 = (int)(sig * 100.f + 0.5f);
            snprintf(txt, (size_t)ntxt, "%d%%", *v100);
        }
        return;
    }
    if (k == 0) {
        const int hits = ((bot * 37 + 11) % 70) + 20;
        *v100 = hits;
        snprintf(txt, (size_t)ntxt, "%03d", (hits * 60 + 50) / 100);
    } else if (k == 1) {
        const float up = 1.f - expf(-(float)secs / 90.f);
        *v100 = (int)(up * 100.f + 0.5f);
        snprintf(txt, (size_t)ntxt, "%02d:%02d", m, s);
    } else {
        float sig = 0.5f + (float)((bot * 29) % 40) / 100.f + 0.05f * sinf((float)millis() / 700.f + (float)bot);
        if (sig > 0.97f) sig = 0.97f;
        if (sig < 0.f) sig = 0.f;
        *v100 = (int)(sig * 100.f + 0.5f);
        snprintf(txt, (size_t)ntxt, "%d%%", *v100);
    }
}

static void bar_one(Gfx g, int x, int y, int bw, int k, int bot, bool gib, uint16_t col, bool label)
{
    const int N = (bw - 4) / 6;
    int v100 = 0;
    char txt[8];
    txt[0] = 0;
    stat_row(bot, gib, k, &v100, txt, (int)sizeof(txt));
    if (v100 < 0) v100 = 0;
    if (v100 > 100) v100 = 100;
    const int lit = (N * v100 + 50) / 100;
    const uint16_t silk = botcorps_rgb565(PAL_STEEL);
    const uint16_t cap = botcorps_rgb565(PAL_MUTED);
    const bool radio = bot == 0 || bot == 1;
    const char *lab = gib ? ((k == 0) ? "CREW" : (k == 1) ? "UPTIME" : "LINK")
                          : ((k == 0) ? "HITS" : (k == 1) ? "UPTIME" : (radio ? "DUTY" : "SIGNAL"));
    if (label) {
        g.setFont(&fonts::Font0);
        g.setTextSize(1);
        g.setTextColor(mix_nat(silk, col, 45, 100));
        g.setCursor(x, y - 7);
        g.print(lab);
        g.setTextColor(mix_nat(col, C_WHITE, 55, 100));
        const int tw = (int)strlen(txt) * 6;
        g.setCursor(x + bw - tw, y - 7);
        g.print(txt);
    }
    g.fillRect(x, y, bw, 10, botcorps_rgb565(PAL_BLACK));
    g.fillRect(x, y + 2, 2, 6, cap);
    g.fillRect(x + bw - 2, y + 2, 2, 6, cap);
    const bool blink = ((millis() / 260u) % 2u) == 0u;
    for (int n = 0; n < N && n < 16; ++n) {
        uint16_t cc = C_OFF;
        if (n < lit) {
            if (n == lit - 1) cc = blink ? C_WHITE : mix_nat(col, C_WHITE, 45, 100);
            else if (n * 10 > N * 8) cc = mix_nat(col, C_WHITE, 25, 100);
            else cc = col;
        }
        g.fillRect(x + 3 + n * 6, y + 2, 5, 6, cc);
    }
}

static void bars(Gfx g, const lay_t &L, bool mirror, int width, int bot, bool gib, uint16_t col, bool full)
{
    for (int k = 0; k < 3; ++k) {
        if (!full && k != 1) continue;
        const int x = mx(mirror, L.bars[k][0], L.bw, width);
        bar_one(g, x, L.bars[k][1], L.bw, k, bot, gib, col, full);
    }
}

static const lay_t &pick(int bot, bool gib, bool *mirror, int *layi)
{
    const int vi = gib ? 0 : (bot % 6);
    *layi = vi / 2;
    *mirror = false;
    return LAY[*layi];
}

static void chip_at(const lay_t &L, bool mirror, int width, int *x, int *y)
{
    const int S = L.chip[2];
    *x = mx(mirror, L.chip[0], S, width) + ((S - 34) >> 1);
    *y = L.chip[1] + ((L.chip[3] - 34) >> 1);
}

static uint16_t status_col(uint16_t body, int st, bool gib)
{
    if (gib || st == 0) return body;
    if (st == 2) return botcorps_rgb565(PAL_HOLD);
    return mix_nat(body, C_WHITE, 35, 100);
}

static void name_plate(Gfx g, const lay_t &L, bool mirror, int width, int bot, bool gib,
                       uint16_t body, const char *id)
{
    const int x = mx(mirror, L.name[0], L.name[2], width);
    const int y = L.name[1];
    const uint16_t silk = botcorps_rgb565(PAL_STEEL);
    const uint16_t die = botcorps_rgb565(PAL_DEEP);
    g.fillRect(x, y, L.name[2], L.name[3], silk);
    g.fillRect(x + 1, y + 1, L.name[2] - 2, L.name[3] - 2, die);
    for (int k = 0; k < 3; ++k) {
        const int px = x + L.name[2] / 2 - 12 + k * 8;
        const int py = y - 1;
        g.fillRect(px - 1, py - 1, 5, 4, botcorps_rgb565(PAL_BLACK));
        g.fillRect(px, py, 3, 2, C_PIN);
    }
    char plate[24];
    if (gib) snprintf(plate, sizeof(plate), "gibsonbot");
    else snprintf(plate, sizeof(plate), "%s %s", (bot >= 0 && bot < 12) ? INS[bot] : "*", id ? id : "");
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(mix_nat(body, C_WHITE, 40, 100));
    const int tw = (int)strlen(plate) * 6;
    g.setCursor(x + (L.name[2] - tw) / 2, y + 2);
    g.print(plate);
    g.setTextColor(silk);
    g.setCursor(x + (L.name[2] - 5 * 6) / 2, y + L.name[3] - 8);
    g.print("LV 01");
    const int st = bot_st(bot, gib);
    const bool on = gib || st == 2 || ((millis() / 700u) % 4u) != 0u;
    if (on) g.fillRect(x + 4, y + 4, 4, 3, status_col(body, st, gib));
    g.fillRect(x + 2, y + L.name[3] - 2, 3, 1, body);
    g.fillRect(x + L.name[2] - 5, y + L.name[3] - 2, 3, 1, body);
}

void botcorps_id_paint(Gfx g, int width, int bot, bool gib, uint8_t col,
                       int shape, const char *name, const char *target)
{
    bool mirror = false;
    int layi = 0;
    const lay_t &L = pick(bot, gib, &mirror, &layi);
    const uint16_t body = botcorps_rgb565(col);
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    const uint16_t deep = botcorps_rgb565(PAL_DEEP);
    const uint16_t muted = botcorps_rgb565(PAL_MUTED);
    const uint16_t steel = botcorps_rgb565(PAL_STEEL);
    const uint16_t ivory = botcorps_rgb565(PAL_IVORY);
    const card_ink ink = card_inks(col);
    const int pat = gib ? 0 : (bot + (bot > 5 ? 1 : 0)) % 3;
    df_rr(g, 2, 26, width - 4, 107, 8, ink.edge);
    df_rr(g, 3, 27, width - 6, 105, 7, ink.tint);
    g.fillRect(14, 27, width - 28, 1, ink.hi);
    {
        const int x = mx(mirror, L.slot[0], L.slot[2], width);
        const int strap = x + L.slot[2] / 2 - 2;
        if (L.slot[1] > 22) g.fillRect(strap, 22, 4, L.slot[1] - 22, steel);
        df_rr(g, x, L.slot[1], L.slot[2], L.slot[3] + 1, 2, steel);
        df_rr(g, x + 1, L.slot[1] + 1, L.slot[2] - 2, L.slot[3] - 1, 1, black);
    }
    for (int i = 0; i < TR_COUNT[layi]; ++i) wire_poly(g, mirror, width, layi, i, ink.tr);
    {
        const int x = mx(mirror, L.photo[0], L.photo[2], width);
        photo(g, x, L.photo[1], L.photo[2], L.photo[3], bot, gib, col, shape);
    }
    {
        int x = 0;
        int y = 0;
        chip_at(L, mirror, width, &x, &y);
        chip(g, x, y, pat, bot, gib, col, shape);
    }
    bars(g, L, mirror, width, bot, gib, body, true);
    name_plate(g, L, mirror, width, bot, gib, body, name);
    {
        const int x = mx(mirror, L.oled[0], L.oled[2], width);
        g.fillRect(x, L.oled[1], L.oled[2], L.oled[3], black);
        g.fillRect(x + 2, L.oled[1] + 2, L.oled[2] - 4, L.oled[3] - 4, C_OLED);
        const char *msg = target ? target : "";
        g.setTextColor(body);
        const int tw = (int)strlen(msg) * 6;
        g.setCursor(x + (L.oled[2] - tw) / 2, L.oled[1] + 1);
        g.print(msg);
    }
    {
        const int x = mx(mirror, L.ant[0], L.ant[2], width);
        const int rows = L.ant[3] / 4;
        for (int k = 0; k <= rows; ++k) {
            g.fillRect(x, L.ant[1] + k * 4, L.ant[2] - 2, 1, muted);
            if (k < rows) {
                const int side = ((k + (bot > 5 ? 1 : 0)) % 2) ? x : (x + L.ant[2] - 3);
                g.fillRect(side, L.ant[1] + k * 4, 1, 5, muted);
            }
        }
    }
    {
        const int x = mx(mirror, L.usb[0], L.usb[2], width);
        g.fillRect(x, L.usb[1], L.usb[2], L.usb[3], C_FACE);
        const int inset = mirror ? 2 : 4;
        g.fillRect(x + inset, L.usb[1] + 4, L.usb[2] - 6, L.usb[3] - 8, black);
        const int barx = x + (mirror ? 4 : 5);
        const int barw = L.usb[2] - 8;
        if (barw > 0) g.fillRect(barx, L.usb[1] + L.usb[3] / 2 - 1, barw, 2, C_USB);
    }
    for (int i = 0; i < 2; ++i) {
        const int w = (layi == 1) ? 9 : 10;
        const int x = mx(mirror, L.btn[i][0], w, width);
        g.fillRect(x, L.btn[i][1], w, 8, deep);
        g.fillRect(x + 3, L.btn[i][1] + 2, w - 6, 4, steel);
    }
    {
        const int x = mx(mirror, L.code[0], L.code[2], width);
        g.fillRect(x, L.code[1], L.code[2], L.code[3], ivory);
        g.fillRect(x, L.code[1], L.code[2], 11, black);
        for (int i = 0; i < 3; ++i) {
            id_icon(g, icon_kind(bot, gib, i), x + 3 + i * 11, L.code[1] + 2,
                    i == 1 ? body : muted);
        }
        const char *bid = gib ? "gibson" : (name ? name : "");
        int bx = x + 2;
        char key[24];
        int bh = L.code[3] - 15;
        int by = L.code[1] + 13;
        if (bh < 1) {
            bh = 1;
            by = L.code[1] + 12;
        }
        for (int k = 0; bx < x + L.code[2] - 2; ++k) {
            snprintf(key, sizeof(key), "%sbc%d", bid, k);
            int w = 1 + (int)(fnv1a(key) % 3u);
            if (bx + w > x + L.code[2] - 2) w = x + L.code[2] - 2 - bx;
            if (w > 0) g.fillRect(bx, by, w, bh, black);
            snprintf(key, sizeof(key), "%sbd%d", bid, k);
            bx += w + 1 + (int)(fnv1a(key) % 2u);
        }
    }
}

/* Direct path: photo and name plate only. No card tint, no full-card fill. */
static void paint_photo_plate(Gfx g, int width, int bot, bool gib, uint8_t col,
                              int shape, const char *name)
{
    bool mirror = false;
    int layi = 0;
    const lay_t &L = pick(bot, gib, &mirror, &layi);
    const uint16_t body = botcorps_rgb565(col);
    const int x = mx(mirror, L.photo[0], L.photo[2], width);
    photo(g, x, L.photo[1], L.photo[2], L.photo[3], bot, gib, col, shape);
    name_plate(g, L, mirror, width, bot, gib, body, name);
}

void botcorps_id_recompose(lgfx::LovyanGFX &menu, int width, int bot, bool gib,
                           uint8_t col, int shape, const char *name, const char *target)
{
    const int x0 = 2;
    const int y0 = 26;
    const int y1 = 133;
    const int sw = 236;
    const uint32_t t0 = millis();
    if (s_id_strip && s_id_strip->getBuffer()) {
        const int sh = s_id_strip->height();
        if (sh > 0) {
            for (int y = y0; y < y1; y += sh) {
                int h = y1 - y;
                if (h > sh) h = sh;
                s_id_strip->fillRect(0, 0, sw, sh, botcorps_rgb565(PAL_DEEP));
                botcorps_band_set_ox(x0);
                botcorps_band_set_oy(y);
                s_id_strip->setClipRect(0, 0, sw, h);
                botcorps_id_paint(Gfx(*s_id_strip), width, bot, gib, col, shape, name, target);
                s_id_strip->clearClipRect();
                botcorps_band_set_ox(0);
                botcorps_band_set_oy(0);
                menu.startWrite();
                menu.setClipRect(x0, y, sw, h);
                s_id_strip->pushSprite(&menu, x0, y);
                menu.clearClipRect();
                menu.endWrite();
            }
            Serial.printf("[int] recompose STRIP %u\n", (unsigned)(millis() - t0));
            return;
        }
    }
    menu.startWrite();
    paint_photo_plate(Gfx(menu), width, bot, gib, col, shape, name);
    menu.endWrite();
    Serial.println("[int] recompose DIRECT");
    (void)target;
}

void botcorps_id_tick(lgfx::LovyanGFX &menu, int width, int bot, bool gib, uint8_t col)
{
    bool mirror = false;
    int layi = 0;
    const lay_t &L = pick(bot, gib, &mirror, &layi);
    const uint16_t body = botcorps_rgb565(col);
    for (int k = 0; k < 3; ++k) {
        const int x = mx(mirror, L.bars[k][0], L.bw, width);
        const int y = L.bars[k][1];
        menu.startWrite();
        menu.setClipRect(x, y - 8, L.bw, 18);
        Gfx g(menu);
        bar_one(g, x, y, L.bw, k, bot, gib, body, true);
        menu.endWrite();
        menu.clearClipRect();
    }
}

#else  /* BOTCORPS_ID_DEVBOARD */

/* 0 ready, 1 limited, 2 hold. Fixed per bot (same table as the ibId card). */
static const uint8_t ST[12] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1 };
static const char *const INS[12] = { "~", "*", "/", "#", "#", "*", "+", "[", "+", "~", "^", "o" };
static const uint16_t C_WHITE = 0xFFFF;
static const uint16_t C_FACE = 0xAD97;  /* #aab2ba */
static const uint16_t C_OFF = 0x1926;   /* #1c2733 */
static const uint16_t C_OLED = 0x0021;
static const uint16_t C_PCB = 0x0841;   /* matte black board */
static const uint16_t C_TEX = 0x1082;   /* board texture */
static const uint16_t C_MOD = 0x18E3;   /* module PCB */

static void split565(uint16_t p, int *r, int *g, int *b)
{
    const int r5 = (p >> 11) & 31, g6 = (p >> 5) & 63, b5 = p & 31;
    *r = (r5 << 3) | (r5 >> 2); *g = (g6 << 2) | (g6 >> 4); *b = (b5 << 3) | (b5 >> 2);
}
static uint16_t pack565(int r, int g, int b)
{
    if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
static uint16_t mix_nat(uint16_t a, uint16_t b, int num, int den)
{
    int ar, ag, ab, br, bg, bb;
    const int inv = den - num;
    split565(a, &ar, &ag, &ab); split565(b, &br, &bg, &bb);
    return pack565((ar * inv + br * num) / den, (ag * inv + bg * num) / den, (ab * inv + bb * num) / den);
}
static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; ++p) { h ^= *p; h *= 16777619u; }
    return h;
}
/* 3x5 silk font (same glyphs as the EXT board silk) + ':' '/' */
static const uint16_t TINY3[39] = { 0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7292, 0x7BEF, 0x7BCF, 0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A, 0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B73, 0x6BAD, 0x388E, 0x7492, 0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x01C0, 0x0410, 0x12A4 };
static int silk_w(const char *s) { const int n = (int)strlen(s); return n ? n * 4 - 1 : 0; }
static void silk(Gfx g, const char *s, int x0, int y, uint16_t col)
{
    for (int i = 0; s[i]; ++i) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        int k = -1;
        if (c >= '0' && c <= '9') k = c - '0';
        else if (c >= 'A' && c <= 'Z') k = 10 + c - 'A';
        else if (c == '-') k = 36; else if (c == ':') k = 37; else if (c == '/') k = 38;
        if (k < 0) continue;
        const uint16_t m = TINY3[k];
        for (int b = 0; b < 15; ++b)
            if (m & (1u << (14 - b))) g.fillRect(x0 + i * 4 + b % 3, y + b / 3, 1, 1, col);
    }
}
static void ring(Gfx g, int cx, int cy, uint16_t gold, uint16_t hole)
{
    g.fillRect(cx - 1, cy - 3, 3, 7, gold);
    g.fillRect(cx - 3, cy - 1, 7, 3, gold);
    g.fillRect(cx - 2, cy - 2, 5, 5, gold);
    g.fillRect(cx - 1, cy - 1, 3, 3, hole);
}

/* bot meters: HITS / UPTIME / SIGNAL|DUTY (Gibson CREW / UPTIME / LINK), LED-step fill, tip blinks */
static void stat_row(int bot, bool gib, int k, int *v100, char *txt, int ntxt)
{
    const uint32_t secs = millis() / 1000u;
    const int m = (int)((secs / 60u) % 100u), s = (int)(secs % 60u);
    if (k == 1) {
        const float up = 1.f - expf(-(float)secs / 90.f);
        *v100 = (int)(up * 100.f + 0.5f);
        snprintf(txt, (size_t)ntxt, "%02d:%02d", m, s);
        return;
    }
    if (gib) {
        if (k == 0) { *v100 = 7 * 100 / 12; snprintf(txt, (size_t)ntxt, "7/12"); return; }
        float sig = 0.8f + 0.06f * sinf((float)millis() / 700.f);
        if (sig > 0.97f) sig = 0.97f;
        *v100 = (int)(sig * 100.f + 0.5f);
        snprintf(txt, (size_t)ntxt, "%d%%", *v100);
        return;
    }
    if (k == 0) {
        const int hits = ((bot * 37 + 11) % 70) + 20;
        *v100 = hits;
        snprintf(txt, (size_t)ntxt, "%03d", (hits * 60 + 50) / 100);
        return;
    }
    float sig = 0.5f + (float)((bot * 29) % 40) / 100.f + 0.05f * sinf((float)millis() / 700.f + (float)bot);
    if (sig > 0.97f) sig = 0.97f;
    if (sig < 0.f) sig = 0.f;
    *v100 = (int)(sig * 100.f + 0.5f);
    snprintf(txt, (size_t)ntxt, "%d%%", *v100);
}
/* v1019n (#18): all three meters fill in the bot's OWN fixed colour (Gibson ivory). The tricolor review
 * option is removed. */
static uint16_t meter_col(int k, uint16_t body)
{
    (void)k;
    return body;
}
static const int BAR_X = 68, BAR_Y0 = 49, BAR_DY = 17, BAR_W = 104;
static void bar_one(Gfx g, int x, int y, int bw, int k, int bot, bool gib, uint16_t col)
{
    const int N = (bw - 4) / 6;
    int v100 = 0;
    char txt[8];
    txt[0] = 0;
    stat_row(bot, gib, k, &v100, txt, (int)sizeof(txt));
    if (v100 < 0) v100 = 0;
    if (v100 > 100) v100 = 100;
    const int lit = (N * v100 + 50) / 100;
    const uint16_t silkc = botcorps_rgb565(PAL_STEEL);
    const bool radio = bot == 0 || bot == 1;
    const char *lab = gib ? ((k == 0) ? "CREW" : (k == 1) ? "UPTIME" : "LINK")
                          : ((k == 0) ? "HITS" : (k == 1) ? "UPTIME" : (radio ? "DUTY" : "SIGNAL"));
    g.fillRect(x, y - 8, bw, 8, C_PCB);
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(mix_nat(silkc, col, 45, 100));
    g.setCursor(x, y - 8);
    g.print(lab);
    g.setTextColor(mix_nat(col, C_WHITE, 55, 100));
    g.setCursor(x + bw - (int)strlen(txt) * 6, y - 8);
    g.print(txt);
    g.fillRect(x, y, bw, 9, botcorps_rgb565(PAL_BLACK));
    g.fillRect(x, y + 2, 2, 5, botcorps_rgb565(PAL_MUTED));
    g.fillRect(x + bw - 2, y + 2, 2, 5, botcorps_rgb565(PAL_MUTED));
    const bool blink = ((millis() / 260u) % 2u) == 0u;
    for (int n = 0; n < N && n < 16; ++n) {
        uint16_t cc = C_OFF;
        if (n < lit) {
            if (n == lit - 1) cc = blink ? C_WHITE : mix_nat(col, C_WHITE, 45, 100);
            else if (n * 10 > N * 8) cc = mix_nat(col, C_WHITE, 25, 100);
            else cc = col;
        }
        g.fillRect(x + 3 + n * 6, y + 2, 5, 5, cc);
    }
}

/* square-chip bot face (control-panel block): full square in the bot colour, lit top, dark chin,
 * two black eyes; Gibson = ivory with black shades + steel glint */
static void sq_face(Gfx g, int cx, int cy, int s, uint16_t col, bool gib)
{
    const int x = cx - s / 2, y = cy - s / 2, lip = s >= 12 ? 2 : 1;
    g.fillRect(x, y, s, s, col);
    g.fillRect(x, y, s, lip, gib ? C_WHITE : mix_nat(col, C_WHITE, 45, 100));
    g.fillRect(x, y + s - lip, s, lip, gib ? botcorps_rgb565(PAL_CHIN) : mix_nat(col, 0x0000, 45, 100));
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    if (!gib) {
        const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;
        const bool bl = (millis() / 2600u) % 7u == 0u && (millis() % 2600u) < 140u;
        g.fillRect(x + m, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, black);
        g.fillRect(x + m + 2 * c, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, black);
        return;
    }
    const int ew = s / 5, ey = y + s * 28 / 100;
    g.fillRect(x + 1, ey, s - 2, ew > 2 ? ew - 1 : 1, black);                 /* shades bar */
    g.fillRect(x + s / 8, ey, s * 3 / 8, ew + 1, black);
    g.fillRect(x + s / 2 + 1, ey, s * 3 / 8 - 1, ew + 1, black);
    const int gl = (int)((millis() / 900u) % 2u);
    g.fillRect(x + s / 8 + 1 + gl, ey + 1, 2, 1, botcorps_rgb565(PAL_STEEL));
    g.fillRect(x + s / 2 + 2 + gl, ey + 1, 2, 1, botcorps_rgb565(PAL_STEEL));
}

/* GPIO header: the CYD's broken-out pins. Lit pads = pins this bot drives (fixed per bot). */
static const char *const GP_N[14] = { "3V", "GND", "21", "22", "27", "35", "TX", "RX", "4", "16", "17", "25", "26", "32" };
static const uint16_t GP_ACT[13] = {
    /* bit i = GP_N[i] (2..13); 0/1 are power, always shown as power */
    (1 << 2) | (1 << 3) | (1 << 9) | (1 << 10),             /* wifi */
    (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7),              /* bluetooth */
    (1 << 4) | (1 << 11) | (1 << 12),                       /* deauth */
    (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10),  /* hack */
    (1 << 5) | (1 << 11) | (1 << 13),                       /* flock */
    (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12),           /* drone */
    (1 << 2) | (1 << 3) | (1 << 8),                         /* airtag */
    (1 << 4) | (1 << 8) | (1 << 13),                        /* keyfob */
    (1 << 2) | (1 << 3) | (1 << 5),                         /* meta */
    (1 << 5) | (1 << 6) | (1 << 7),                         /* skimmer */
    (1 << 4) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12) | (1 << 13), /* flipper */
    (1 << 2) | (1 << 3) | (1 << 4),                         /* samsungtag */
    0x3FFC,                                                 /* gibson: every pin (crew bus) */
};
static const int GP_X0 = 74, GP_P = 11, GP_Y = 105;
static uint16_t gp_mask(int bot, bool gib) { return gib ? GP_ACT[12] : GP_ACT[(bot < 0 || bot > 11) ? 0 : bot]; }
static void gpio_pad(Gfx g, int i, uint16_t body, bool on, bool tip)
{
    const int x = GP_X0 + i * GP_P, y = GP_Y;
    const uint16_t gold = botcorps_rgb565(177), goldh = botcorps_rgb565(217);
    uint16_t pad = gold;
    if (i == 0) pad = 0xE1A6;                                  /* 3V3: red power pad */
    else if (i == 1) pad = botcorps_rgb565(PAL_STEEL);          /* GND */
    else if (on) pad = tip ? C_WHITE : body;
    g.fillRect(x, y, 7, 7, pad);
    g.fillRect(x, y, 7, 1, (on && i > 1) ? mix_nat(body, C_WHITE, 50, 100) : goldh);
    g.fillRect(x + 2, y + 2, 3, 3, botcorps_rgb565(PAL_BLACK));
    if (on && i > 1) g.fillRect(x + 3, y + 3, 1, 1, tip ? body : C_WHITE);
}

static void board(Gfx g, int width, int bot, bool gib, uint8_t col, const char *name, const char *target)
{
    const uint16_t body = botcorps_rgb565(col);
    const uint16_t black = botcorps_rgb565(PAL_BLACK);
    const uint16_t steel = botcorps_rgb565(PAL_STEEL);
    const uint16_t gold = botcorps_rgb565(177), goldd = botcorps_rgb565(104);
    const uint16_t silkc = mix_nat(steel, C_WHITE, 25, 100);
    const uint16_t trace = mix_nat(0x2187, body, 45, 100);     /* per-bot copper: bot colour over dark */
    const uint16_t rule = mix_nat(C_PCB, body, 40, 100);
    /* PCB: gold edge, matte black, faint texture, bot-colour silk rule */
    g.fillRect(2, 26, width - 4, 107, gold);
    g.fillRect(3, 27, width - 6, 105, C_PCB);
    for (int x = 9; x < width - 4; x += 8) g.fillRect(x, 28, 1, 103, C_TEX);
    g.fillRect(5, 29, width - 10, 1, rule); g.fillRect(5, 129, width - 10, 1, rule);
    g.fillRect(5, 29, 1, 101, rule); g.fillRect(width - 6, 29, 1, 101, rule);
    ring(g, 10, 35, gold, black); ring(g, width - 11, 35, gold, black);
    ring(g, 10, 124, gold, black); ring(g, width - 11, 124, gold, black);
    /* badge lanyard slot + strap (it is still the bot's ID badge) */
    g.fillRect(118, 22, 4, 6, steel);
    g.fillRect(102, 27, 36, 6, steel);
    g.fillRect(103, 28, 34, 4, black);
    /* top silk: insignia + bot name (bot colour), DEV BOARD tag */
    char nm[24];
    if (gib) snprintf(nm, sizeof nm, "gibsonbot");
    else snprintf(nm, sizeof nm, "%s %s", (bot >= 0 && bot < 12) ? INS[bot] : "*", name ? name : "");
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(mix_nat(body, C_WHITE, 35, 100));
    g.setCursor(18, 31);
    g.print(nm);
    silk(g, "ESP32 DEV BOARD", width - 18 - silk_w("ESP32 DEV BOARD"), 32, silkc);
    /* ---- the WROOM module: this bot's own chip ---- */
    const int mx = 12, my = 41, mw = 48, mh = 80;
    g.fillRect(mx, my, mw, mh, C_MOD);
    for (int y = my + 22; y < my + mh - 4; y += 6) {             /* castellated module pads */
        g.fillRect(mx - 2, y, 4, 3, gold); g.fillRect(mx - 2, y, 1, 3, goldd);
        g.fillRect(mx + mw - 2, y, 4, 3, gold); g.fillRect(mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 6) g.fillRect(x, my + mh - 2, 3, 4, gold);
    int ax = mx + 4;                                              /* PCB antenna meander */
    for (int i = 0; i < 6; ++i) {
        g.fillRect(ax, my + 3, 1, 11, gold); g.fillRect(ax, my + 3, 4, 1, gold);
        g.fillRect(ax + 3, my + 3, 1, 11, gold); g.fillRect(ax + 3, my + 13, 4, 1, gold);
        ax += 7;
    }
    const int sx = mx + 3, sy = my + 18, sw = mw - 6, sh = mh - 22;   /* shield can, brushed steel */
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0; if (b < 0) b = 0; if (b > sw) b = sw;
        g.fillRect(sx, sy + r, a, 1, botcorps_rgb565(178));
        g.fillRect(sx + a, sy + r, b - a, 1, botcorps_rgb565(110));
        g.fillRect(sx + b, sy + r, sw - b, 1, botcorps_rgb565(73));
    }
    g.fillRect(sx, sy, sw, 1, botcorps_rgb565(186));
    g.fillRect(sx, sy, 1, sh, botcorps_rgb565(186));
    /* square-chip bot in the can: black die, castellated pins, its face in its fixed colour */
    const int cx = sx + sw / 2, cy = sy + 18, D = 30;
    const uint16_t pin = gib ? gold : botcorps_rgb565(PAL_SOFT);
    for (int k = 0; k < 5; ++k) {                       /* v1019n (#16): exactly 5 pins per side */
        const int i = (D - 4 * (D / 6) - 1) / 2 + (D / 6) * k;
        g.fillRect(cx - D / 2 - 2, cy - D / 2 + i, 2, 1, pin); g.fillRect(cx + D / 2, cy - D / 2 + i, 2, 1, pin);
        g.fillRect(cx - D / 2 + i, cy - D / 2 - 2, 1, 2, pin); g.fillRect(cx - D / 2 + i, cy + D / 2, 1, 2, pin);
    }
    g.fillRect(cx - D / 2, cy - D / 2, D, D, black);
    g.fillRect(cx - D / 2, cy - D / 2, D, 1, steel); g.fillRect(cx - D / 2, cy + D / 2 - 1, D, 1, steel);
    g.fillRect(cx - D / 2, cy - D / 2, 1, D, steel); g.fillRect(cx + D / 2 - 1, cy - D / 2, 1, D, steel);
    const int bob = gib ? 0 : (((millis() / 900u + (uint32_t)bot) % 2u) ? 1 : 0);
    sq_face(g, cx, cy + bob - 1, 22, body, gib);
    char up[16];
    int k = 0;
    for (const char *p = gib ? "gibson" : (name ? name : ""); *p && k < 15; ++p) up[k++] = *p;
    up[k] = 0;
    if (k > 10 && k >= 3 && strcmp(up + k - 3, "bot") == 0) up[k - 3] = 0;   /* fits the 42 px can */
    if (strlen(up) > 10) up[10] = 0;
    silk(g, up, cx - silk_w(up) / 2, sy + 37, botcorps_rgb565(41));
    silk(g, "WROOM-32", cx - silk_w("WROOM-32") / 2, sy + 44, botcorps_rgb565(41));
    silk(g, gib ? "CORE" : "FCC 2AC7Z", cx - silk_w(gib ? "CORE" : "FCC 2AC7Z") / 2, sy + 50, botcorps_rgb565(41));
    /* ---- copper: module -> meters, module -> GPIO bus ---- */
    for (int kk = 0; kk < 3; ++kk) {
        const int y = BAR_Y0 + kk * BAR_DY + 4;
        g.fillRect(mx + mw + 2, y, BAR_X - mx - mw - 3, 1, trace);
    }
    g.fillRect(mx + mw + 2, 99, GP_X0 + 13 * GP_P + 3 - (mx + mw + 2), 1, trace);   /* GPIO bus */
    const uint16_t gm = gp_mask(bot, gib);
    for (int i = 2; i < 14; ++i)
        if (gm & (1u << i)) g.fillRect(GP_X0 + i * GP_P + 3, 100, 1, GP_Y - 100, trace);
    /* ---- meters ---- */
    for (int kk = 0; kk < 3; ++kk) bar_one(g, BAR_X, BAR_Y0 + kk * BAR_DY, BAR_W, kk, bot, gib, meter_col(kk, body));
    /* ---- right column: OLED target readout, BOOT/EN, LEDs, USB-C ---- */
    {
        const int ox = 180, oy = 40, ow = 50, oh = 14;
        g.fillRect(ox, oy, ow, oh, black);
        g.fillRect(ox + 2, oy + 2, ow - 4, oh - 4, C_OLED);
        g.fillRect(ox + 2, oy + 2, ow - 4, 1, mix_nat(C_OLED, body, 55, 100));
        const char *msg = target ? target : "";
        g.setTextColor(body);
        g.setCursor(ox + (ow - (int)strlen(msg) * 6) / 2, oy + 4);
        g.print(msg);
        silk(g, "TARGET", ox, oy + oh + 2, silkc);
    }
    for (int b = 0; b < 2; ++b) {
        const int bx = 182 + b * 22, by = 66;
        g.fillRect(bx, by, 12, 10, botcorps_rgb565(41));
        g.fillRect(bx + 1, by + 1, 10, 8, botcorps_rgb565(PAL_DEEP));
        g.fillRect(bx + 3, by + 3, 6, 4, steel);
        silk(g, b ? "EN" : "BOOT", bx + 6 - silk_w(b ? "EN" : "BOOT") / 2, by + 12, silkc);
    }
    {
        const int st = (gib || bot < 0 || bot > 11) ? 0 : ST[bot];
        const bool on = gib || st == 2 || ((millis() / 700u) % 4u) != 0u;
        g.fillRect(183, 88, 4, 3, (uint16_t)0xF800);                                   /* PWR */
        g.fillRect(184, 88, 2, 1, C_WHITE);
        silk(g, "PWR", 189, 87, silkc);
        const uint16_t sc = (st == 2) ? botcorps_rgb565(PAL_HOLD) : body;     /* status LED = bot colour */
        g.fillRect(205, 88, 4, 3, on ? sc : C_OFF);
        if (on) g.fillRect(206, 88, 2, 1, C_WHITE);
        silk(g, "IO2", 211, 87, silkc);
    }
    g.fillRect(width - 12, 60, 10, 18, C_FACE);                               /* USB-C at the board edge */
    g.fillRect(width - 10, 63, 6, 12, black);
    g.fillRect(width - 8, 66, 2, 6, botcorps_rgb565(110));
    /* ---- GPIO header ---- */
    silk(g, "GPIO", BAR_X, 93, silkc);
    g.fillRect(GP_X0 - 3, GP_Y - 2, 14 * GP_P + 2, 11, black);
    g.fillRect(GP_X0 - 3, GP_Y - 2, 14 * GP_P + 2, 1, botcorps_rgb565(68));
    const int tip = (int)((millis() / 400u) % 14u);
    for (int i = 0; i < 14; ++i) {
        const bool on = i < 2 || (gm & (1u << i));
        gpio_pad(g, i, body, on, i == tip && on && i > 1);
        const char *lb = GP_N[i];
        silk(g, lb, GP_X0 + i * GP_P + 3 - silk_w(lb) / 2, GP_Y + 10, (on && i > 1) ? mix_nat(body, C_WHITE, 30, 100) : silkc);
    }
    /* ---- badge silk: serial + barcode ---- */
    char sn[24];
    snprintf(sn, sizeof sn, "ID %02d LV 01", gib ? 0 : bot + 1);
    silk(g, sn, 18, 124, silkc);
    const char *bid = gib ? "gibson" : (name ? name : "");
    int bx = 62;
    char key[24];
    for (int kk = 0; bx < 100; ++kk) {
        snprintf(key, sizeof(key), "%sbc%d", bid, kk);
        int w = 1 + (int)(fnv1a(key) % 3u);
        if (bx + w > 100) w = 100 - bx;
        if (w > 0) g.fillRect(bx, 123, w, 5, silkc);
        snprintf(key, sizeof(key), "%sbd%d", bid, kk);
        bx += w + 1 + (int)(fnv1a(key) % 2u);
    }
}

void botcorps_id_paint(Gfx g, int width, int bot, bool gib, uint8_t col,
                       int shape, const char *name, const char *target)
{
    (void)shape;
    board(g, width, bot, gib, col, name, target);
}

void botcorps_id_recompose(lgfx::LovyanGFX &menu, int width, int bot, bool gib,
                           uint8_t col, int shape, const char *name, const char *target)
{
    const int x0 = 2, y0 = 22, y1 = 133, sw = 236;
    if (s_id_strip && s_id_strip->getBuffer()) {
        const int sh = s_id_strip->height();
        if (sh > 0) {
            for (int y = y0; y < y1; y += sh) {
                int h = y1 - y;
                if (h > sh) h = sh;
                s_id_strip->fillRect(0, 0, sw, sh, botcorps_rgb565(PAL_DEEP));
                botcorps_band_set_ox(x0);
                botcorps_band_set_oy(y);
                s_id_strip->setClipRect(0, 0, sw, h);
                botcorps_id_paint(Gfx(*s_id_strip), width, bot, gib, col, shape, name, target);
                s_id_strip->clearClipRect();
                botcorps_band_set_ox(0);
                botcorps_band_set_oy(0);
                menu.startWrite();
                menu.setClipRect(x0, y, sw, h);
                s_id_strip->pushSprite(&menu, x0, y);
                menu.clearClipRect();
                menu.endWrite();
            }
            Serial.println("[int] recompose STRIP devboard");
            return;
        }
    }
    menu.startWrite();
    board(Gfx(menu), width, bot, gib, col, name, target);   /* direct fallback: full board */
    menu.endWrite();
    Serial.println("[int] recompose DIRECT devboard");
}

void botcorps_id_tick(lgfx::LovyanGFX &menu, int width, int bot, bool gib, uint8_t col)
{
    (void)width;
    const uint16_t body = botcorps_rgb565(col);
    for (int k = 0; k < 3; ++k) {
        const int y = BAR_Y0 + k * BAR_DY;
        menu.startWrite();
        menu.setClipRect(BAR_X, y - 8, BAR_W, 17);
        Gfx g(menu);
        bar_one(g, BAR_X, y, BAR_W, k, bot, gib, meter_col(k, body));
        menu.endWrite();
        menu.clearClipRect();
    }
}
#endif /* BOTCORPS_ID_DEVBOARD */
