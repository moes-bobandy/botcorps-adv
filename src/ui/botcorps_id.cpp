#include "ui/botcorps_id.h"

#include "display/botcorps_display.h"
#include "ui/botcorps_sprite.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

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

struct seg_t { int x0, y0, x1, y1; };
static const seg_t WIRE[3][4] = {
    { { 52, 55, 60, 55 }, { 72, 72, 72, 96 }, { 82, 72, 82, 96 }, { 94, 46, 106, 46 } },
    { { 52, 50, 60, 50 }, { 94, 44, 102, 44 }, { 88, 68, 88, 92 }, { 80, 92, 200, 92 } },
    { { 136, 46, 150, 46 }, { 168, 74, 168, 104 }, { 190, 85, 192, 85 }, { 180, 38, 210, 38 } },
};

static int mx(bool mirror, int x, int w, int width)
{
    return mirror ? width - x - w : x;
}

static void hline(Gfx g, int x0, int x1, int y, uint8_t col)
{
    if (x1 < x0) { const int t = x0; x0 = x1; x1 = t; }
    g.fillRect(x0, y, x1 - x0 + 1, 1, col);
}

static void vline(Gfx g, int x, int y0, int y1, uint8_t col)
{
    if (y1 < y0) { const int t = y0; y0 = y1; y1 = t; }
    g.fillRect(x, y0, 1, y1 - y0 + 1, col);
}

static void wire(Gfx g, bool mirror, int width, const seg_t &s, uint8_t col)
{
    const int x0 = mirror ? width - s.x0 : s.x0;
    const int x1 = mirror ? width - s.x1 : s.x1;
    if (s.y0 == s.y1) hline(g, x0, x1, s.y0, col);
    else if (s.x0 == s.x1) vline(g, x0, s.y0, s.y1, col);
    else {
        hline(g, x0, x1, s.y0, col);
        vline(g, x1, s.y0, s.y1, col);
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
    uint8_t edge;
    uint8_t tint;
    uint8_t hi;
    uint8_t tr;
};

static card_ink card_inks(uint8_t col)
{
    const uint16_t b = botcorps_pal_native(col);
    const uint16_t tint_n = mix_nat(0x0862, b, 12, 100); /* #0a0e12 toward the bot */
    card_ink k;
    k.tint = botcorps_pal_fit(tint_n);
    k.edge = botcorps_pal_fit(mix_nat(0x5B0D, b, 35, 100)); /* #59636d */
    k.hi = botcorps_pal_fit(mix_nat(tint_n, b, 35, 100));
    k.tr = botcorps_pal_fit(mix_nat(0x2187, b, 40, 100)); /* #26313a */
    return k;
}

/* dfRR from the mock: one pixel row per scan, corners inset by the circle. */
static void df_rr(Gfx g, int x, int y, int w, int h, int r, uint8_t col)
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
    auto u = [&](int x, int y, int w, int h, uint8_t col) {
        g.fillRect(ox + x * z, oy + y * z, w * z, h * z, col);
    };
    const int sx = ox + (z + 1) / 2;
    const int sw = (ox + (13 * z + 1) / 2) - sx;
    g.fillRect(sx, oy + 7 * z, sw > 0 ? sw : z, z > 2 ? 2 : z, PAL_BLACK);
    u(3, 0, 1, 1, PAL_IVORY);
    u(1, 1, 5, 1, PAL_IVORY);
    u(0, 2, 7, 3, PAL_IVORY);
    u(0, 5, 7, 1, PAL_IVORY);
    u(1, 6, 5, 1, PAL_IVORY);
    u(1, 6, 5, 1, PAL_CHIN);
    u(0, 5, 1, 1, PAL_CHIN);
    u(6, 5, 1, 1, PAL_CHIN);
    u(0, 3, 7, 1, PAL_BLACK);
    u(1, 4, 2, 1, PAL_BLACK);
    u(4, 4, 2, 1, PAL_BLACK);
    const int gl = (int)((millis() / 900u) % 2u);
    u(1 + gl, 3, 1, 1, PAL_STEEL);
    u(4 + gl, 3, 1, 1, PAL_STEEL);
}

static void id_icon(Gfx g, int k, int x, int y, uint8_t col)
{
    auto f = [&](int a, int b, int w, int h) { g.fillRect(x + a, y + b, w, h, col); };
    if (k == 0) { f(2, 0, 4, 3); f(1, 4, 6, 3); }
    else if (k == 1) { f(1, 0, 6, 4); f(2, 4, 4, 2); f(3, 6, 2, 1); }
    else if (k == 2) {
        f(1, 1, 6, 5);
        g.fillRect(x + 3, y + 3, 2, 1, PAL_BLACK);
        f(0, 2, 1, 1); f(7, 2, 1, 1); f(0, 4, 1, 1); f(7, 4, 1, 1);
    } else if (k == 3) {
        f(0, 3, 1, 1); f(1, 2, 1, 3); f(2, 1, 1, 5); f(3, 0, 1, 7);
        f(4, 1, 1, 5); f(5, 2, 1, 3); f(6, 3, 1, 1);
    } else if (k == 4) { f(3, 0, 1, 7); f(1, 3, 5, 1); f(2, 2, 3, 3); }
    else if (k == 5) {
        f(2, 0, 4, 1); f(2, 1, 1, 2); f(5, 1, 1, 2); f(1, 3, 6, 4);
        g.fillRect(x + 3, y + 4, 2, 2, PAL_BLACK);
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

static void pulse(Gfx g, bool mirror, int width, const seg_t &s, int dist, uint8_t col)
{
    int x0 = mirror ? width - s.x0 : s.x0;
    int y0 = s.y0;
    int x1 = mirror ? width - s.x1 : s.x1;
    int y1 = s.y1;
    const int len = (x1 > x0 ? x1 - x0 : x0 - x1) + (y1 > y0 ? y1 - y0 : y0 - y1);
    if (len <= 0) return;
    int d = dist % (len + 14);
    if (d > len) return;
    const int sx = (x1 == x0) ? 0 : (x1 > x0 ? 1 : -1);
    const int sy = (y1 == y0) ? 0 : (y1 > y0 ? 1 : -1);
    const int hx = (x1 > x0 ? x1 - x0 : x0 - x1);
    int x = x0;
    int y = y0;
    if (d <= hx) x += sx * d;
    else { x = x1; y += sy * (d - hx); }
    g.fillRect(x - 1, y - 1, 3, 3, col);
}

static void photo(Gfx g, int x, int y, int w, int h, int bot, bool gib, uint8_t col, int shape)
{
    g.fillRect(x, y, w, h, PAL_BLACK);
    for (int yy = y + 1; yy < y + h; yy += 4) g.fillRect(x + 1, yy, w - 2, 1, PAL_DEEP);
    for (int k = 0; k < 6; ++k) {
        g.fillRect(x + k, y, 1, 1, PAL_MUTED);
        g.fillRect(x, y + k, 1, 1, PAL_MUTED);
        g.fillRect(x + w - 1 - k, y, 1, 1, PAL_MUTED);
        g.fillRect(x + w - 1, y + k, 1, 1, PAL_MUTED);
        g.fillRect(x + k, y + h - 1, 1, 1, PAL_MUTED);
        g.fillRect(x, y + h - 1 - k, 1, 1, PAL_MUTED);
        g.fillRect(x + w - 1 - k, y + h - 1, 1, 1, PAL_MUTED);
        g.fillRect(x + w - 1, y + h - 1 - k, 1, 1, PAL_MUTED);
    }
    const int cx = x + w / 2;
    const int cy = y + h / 2 - 2;
    if (gib) {
        gibson_chip(g, cx, cy, 4);
    } else {
        botcorps_sprite_draw(*g.raw(), cx, cy - g.oy, shape, 4, col, false);
    }
}

static void chip(Gfx g, int x, int y, int pat, int bot, bool gib, uint8_t col, int shape)
{
    g.fillRect(x, y, 34, 34, PAL_MUTED);
    g.fillRect(x + 1, y + 1, 32, 32, PAL_LINE);
    if (pat == 0) {
        g.fillRect(x + 10, y, 1, 34, PAL_DEEP);
        g.fillRect(x + 23, y, 1, 34, PAL_DEEP);
        g.fillRect(x, y + 10, 34, 1, PAL_DEEP);
        g.fillRect(x, y + 23, 34, 1, PAL_DEEP);
    } else if (pat == 1) {
        g.fillRect(x, y + 11, 34, 1, PAL_DEEP);
        g.fillRect(x, y + 22, 34, 1, PAL_DEEP);
        g.fillRect(x + 16, y, 1, 11, PAL_DEEP);
        g.fillRect(x + 16, y + 23, 1, 11, PAL_DEEP);
    } else {
        g.fillRect(x + 16, y, 2, 34, PAL_DEEP);
        g.fillRect(x, y + 16, 34, 2, PAL_DEEP);
    }
    g.fillRect(x + 9, y + 9, 16, 16, PAL_BLACK);
    if (gib) {
        gibson_chip(g, x + 17, y + 17, 1);
    } else {
        botcorps_sprite_draw(*g.raw(), x + 17, y + 17 - g.oy, shape, 1, col, false);
    }
    if (((millis() / 420u) % 2u) == 0u) g.fillRect(x + 28, y + 2, 3, 3, col);
    (void)bot;
}

static void bars(Gfx g, const lay_t &L, bool mirror, int width, uint8_t col, bool full)
{
    const int nseg = L.bw / 6;
    const uint32_t sec = millis() / 1000u;
    const int lit0 = 4 + (int)(sec % (uint32_t)(nseg > 0 ? nseg : 1));
    for (int k = 0; k < 3; ++k) {
        if (!full && k != 1) continue;
        const int x = mx(mirror, L.bars[k][0], L.bw, width);
        const int y = L.bars[k][1];
        const char *lab = (k == 0) ? "HITS" : (k == 1) ? "UP" : "DUTY";
        if (full) {
            g.setFont(&fonts::Font0);
            g.setTextSize(1);
            g.setTextColor(PAL_MUTED);
            g.setCursor(x, y - 7);
            g.print(lab);
        }
        g.fillRect(x, y, L.bw, 10, PAL_BLACK);
        const int lit = (k == 1) ? lit0 : (3 + k * 2);
        for (int n = 0; n < nseg && n < 12; ++n) {
            const uint8_t cc = (n < lit) ? col : PAL_DEEP;
            g.fillRect(x + 3 + n * 6, y + 2, 5, 6, cc);
        }
    }
}

static const lay_t &pick(int bot, bool gib, bool *mirror, int *layi)
{
    const int vi = gib ? 0 : (bot % 6);
    *layi = vi / 2;
    *mirror = (vi % 2) == 1;
    return LAY[*layi];
}

void botcorps_id_paint(Gfx g, int width, int bot, bool gib, uint8_t col,
                       int shape, const char *name, const char *target)
{
    bool mirror = false;
    int layi = 0;
    const lay_t &L = pick(bot, gib, &mirror, &layi);
    const card_ink ink = card_inks(col);
    const int pat = gib ? 0 : (bot + (bot > 5 ? 1 : 0)) % 3;
    df_rr(g, 2, 26, width - 4, 107, 8, ink.edge);
    df_rr(g, 3, 27, width - 6, 105, 7, ink.tint);
    g.fillRect(14, 27, width - 28, 1, ink.hi);
    {
        const int x = mx(mirror, L.slot[0], L.slot[2], width);
        const int strap = x + L.slot[2] / 2 - 2;
        if (L.slot[1] > 22) g.fillRect(strap, 22, 4, L.slot[1] - 22, PAL_STEEL);
        df_rr(g, x, L.slot[1], L.slot[2], L.slot[3] + 1, 2, PAL_STEEL);
        df_rr(g, x + 1, L.slot[1] + 1, L.slot[2] - 2, L.slot[3] - 1, 1, PAL_BLACK);
    }
    for (int i = 0; i < 4; ++i) wire(g, mirror, width, WIRE[layi][i], ink.tr);
    {
        const int x = mx(mirror, L.photo[0], L.photo[2], width);
        photo(g, x, L.photo[1], L.photo[2], L.photo[3], bot, gib, col, shape);
    }
    {
        const int x = mx(mirror, L.chip[0], 34, width);
        chip(g, x, L.chip[1], pat, bot, gib, col, shape);
    }
    bars(g, L, mirror, width, col, true);
    {
        const int x = mx(mirror, L.name[0], L.name[2], width);
        g.fillRect(x, L.name[1], L.name[2], L.name[3], PAL_MUTED);
        g.fillRect(x + 1, L.name[1] + 1, L.name[2] - 2, L.name[3] - 2, PAL_BLACK);
        g.setFont(&fonts::Font0);
        g.setTextSize(1);
        g.setTextColor(col);
        g.setCursor(x + 4, L.name[1] + 2);
        g.print(name ? name : "");
        g.setTextColor(PAL_MUTED);
        g.setCursor(x + 4, L.name[1] + L.name[3] - 8);
        g.print("LV 01");
        if (((millis() / 700u) % 4u) != 0u) g.fillRect(x + 4, L.name[1] + 12, 4, 3, col);
    }
    {
        const int x = mx(mirror, L.oled[0], L.oled[2], width);
        g.fillRect(x, L.oled[1], L.oled[2], L.oled[3], PAL_BLACK);
        g.setTextColor(col);
        g.setCursor(x + 2, L.oled[1] + 1);
        g.print(target ? target : "");
    }
    {
        const int x = mx(mirror, L.ant[0], L.ant[2], width);
        const int rows = L.ant[3] / 4;
        for (int k = 0; k <= rows; ++k) g.fillRect(x, L.ant[1] + k * 4, L.ant[2] - 2, 1, PAL_MUTED);
    }
    {
        const int x = mx(mirror, L.usb[0], L.usb[2], width);
        g.fillRect(x, L.usb[1], L.usb[2], L.usb[3], PAL_MUTED);
        g.fillRect(x + 3, L.usb[1] + 4, L.usb[2] - 6, L.usb[3] - 8, PAL_BLACK);
    }
    for (int i = 0; i < 2; ++i) {
        const int w = (layi == 1) ? 9 : 10;
        const int x = mx(mirror, L.btn[i][0], w, width);
        g.fillRect(x, L.btn[i][1], w, 8, PAL_DEEP);
        g.fillRect(x + 3, L.btn[i][1] + 2, w - 6, 4, PAL_MUTED);
    }
    {
        const int x = mx(mirror, L.code[0], L.code[2], width);
        g.fillRect(x, L.code[1], L.code[2], L.code[3], PAL_IVORY);
        g.fillRect(x, L.code[1], L.code[2], 11, PAL_BLACK);
        for (int i = 0; i < 3; ++i) {
            id_icon(g, icon_kind(bot, gib, i), x + 3 + i * 11, L.code[1] + 2,
                    i == 1 ? col : PAL_MUTED);
        }
        int bx = x + 2;
        for (int k = 0; bx < x + L.code[2] - 2 && k < 16; ++k) {
            const int w = 1 + ((bot + 3) * (k + 1)) % 3;
            g.fillRect(bx, L.code[1] + 13, w, L.code[3] > 15 ? L.code[3] - 15 : 4, PAL_BLACK);
            bx += w + 1 + (k % 2);
        }
    }
}

void botcorps_id_tick(lgfx::LovyanGFX &menu, int width, int bot, bool gib, uint8_t col)
{
    bool mirror = false;
    int layi = 0;
    const lay_t &L = pick(bot, gib, &mirror, &layi);
    const uint32_t t = millis();
    const card_ink ink = card_inks(col);
    for (int i = 0; i < 4; ++i) {
        const seg_t &s = WIRE[layi][i];
        int x0 = mirror ? width - s.x0 : s.x0;
        int x1 = mirror ? width - s.x1 : s.x1;
        if (x1 < x0) { const int tmp = x0; x0 = x1; x1 = tmp; }
        int y0 = s.y0 < s.y1 ? s.y0 : s.y1;
        int y1 = s.y0 < s.y1 ? s.y1 : s.y0;
        menu.startWrite();
        menu.setClipRect(x0 - 2, y0 - 2, (x1 - x0) + 6, (y1 - y0) + 6);
        Gfx g(menu);
        wire(g, mirror, width, s, ink.tr);
        pulse(g, mirror, width, s, (int)((t / 40u) + (uint32_t)i * 9u), col);
        menu.endWrite();
        menu.clearClipRect();
    }
    {
        const int x = mx(mirror, L.bars[1][0], L.bw, width);
        const int y = L.bars[1][1];
        menu.startWrite();
        menu.setClipRect(x, y, L.bw, 10);
        Gfx g(menu);
        bars(g, L, mirror, width, col, false);
        menu.endWrite();
        menu.clearClipRect();
    }
}
