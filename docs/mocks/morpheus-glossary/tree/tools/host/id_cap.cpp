/* Host capture of the device INT card. Links the real botcorps_id_paint. */
#include "lgfx/v1/LGFX_Sprite.hpp"
#include "Arduino.h"
#define PROGMEM
#include "ui/botcorps_band.h"
#include "ui/botcorps_id.h"
#include "ui/botcorps_pal_tables.inc"
#include "lgfx/Fonts/glcdfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void botcorps_band_set_ox(int) {}
void botcorps_band_set_oy(int) {}

const fonts::Font fonts::Font0 = {0};
static lgfx::LGFX_Device s_menu;
lgfx::LGFX_Device &botcorps_menu(void) { return s_menu; }

static uint32_t s_ms = 200;
uint32_t millis(void) { return s_ms; }

static uint8_t s_rgb[240 * 135 * 3];
static int s_fw = 240;
static int s_fh = 135;
static uint16_t s_fg = 0xFFFF;
static int s_cx = 0;
static int s_cy = 0;

static uint16_t pack332(uint8_t i)
{
    const int r3 = (i >> 5) & 7;
    const int g3 = (i >> 2) & 7;
    const int b2 = i & 3;
    const int r8 = (((r3 << 3) + r3) << 2) + (r3 >> 1);
    const int g8 = (((g3 << 3) + g3) << 2) + (g3 >> 1);
    const int b8 = b2 * 0x55;
    return (uint16_t)(((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3));
}

uint16_t botcorps_rgb565(uint8_t idx)
{
    if (k_is_res[idx]) return k_exact_nat[idx];
    if (idx == 255) return 0xFFFF;
    return pack332(idx);
}

static void put565(int x, int y, uint16_t c)
{
    if (x < 0 || y < 0 || x >= s_fw || y >= s_fh) return;
    const int r5 = (c >> 11) & 31;
    const int g6 = (c >> 5) & 63;
    const int b5 = c & 31;
    uint8_t *p = s_rgb + (y * s_fw + x) * 3;
    p[0] = (uint8_t)((r5 << 3) | (r5 >> 2));
    p[1] = (uint8_t)((g6 << 2) | (g6 >> 4));
    p[2] = (uint8_t)((b5 << 3) | (b5 >> 2));
}

Gfx::Gfx(lgfx::LovyanGFX &d) : p(&d), ox(0), oy(0) {}
void Gfx::fillRect(int x, int y, int w, int h, uint8_t c) { fillRect(x, y, w, h, botcorps_rgb565(c)); }
void Gfx::fillRect(int x, int y, int w, int h, uint16_t c)
{
    x -= ox;
    y -= oy;
    if (w < 0) { x += w; w = -w; }
    if (h < 0) { y += h; h = -h; }
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) put565(x + xx, y + yy, c);
}
void Gfx::drawRect(int, int, int, int, uint8_t) {}
void Gfx::fillCircle(int, int, int, uint8_t) {}
void Gfx::drawCircle(int, int, int, uint8_t) {}
void Gfx::drawLine(int, int, int, int, uint8_t) {}
void Gfx::fillTriangle(int, int, int, int, int, int, uint8_t) {}
void Gfx::drawFastHLine(int, int, int, uint8_t) {}
void Gfx::drawFastVLine(int, int, int, uint8_t) {}
void Gfx::fillRoundRect(int, int, int, int, int, uint8_t) {}
void Gfx::fillScreen(uint8_t c) { fillRect(0, 0, s_fw, s_fh, c); }
void Gfx::setClipRect(int, int, int, int) {}
void Gfx::clearClipRect(void) {}
void Gfx::setTextWrap(bool, bool) {}
void Gfx::setTextSize(float) {}
void Gfx::setTextSize(float, float) {}
void Gfx::setTextColor(uint8_t c) { s_fg = botcorps_rgb565(c); }
void Gfx::setTextColor(uint16_t c) { s_fg = c; }
void Gfx::setTextColor(uint8_t fg, uint8_t) { s_fg = botcorps_rgb565(fg); }
void Gfx::setTextDatum(int) {}
void Gfx::drawString(const char *, int, int) {}
void Gfx::setCursor(int x, int y) { s_cx = x - ox; s_cy = y - oy; }
int Gfx::getCursorX(void) { return s_cx + ox; }
void Gfx::startWrite(void) {}
void Gfx::endWrite(void) {}
void Gfx::print(const char *s)
{
    if (!s) return;
    for (const char *p = s; *p; ++p) {
        const unsigned u = (unsigned char)*p;
        const uint8_t *bits = font + u * 5;
        for (int col = 0; col < 5; ++col) {
            const uint8_t line = bits[col];
            for (int row = 0; row < 8; ++row)
                if ((line >> row) & 1) put565(s_cx + col, s_cy + row, s_fg);
        }
        s_cx += 6;
    }
}

static const char *const SPR_ROW[4][9] = {
    { "...1...", ".11111.", "1111111", "1221221", "1221221", "1111111", ".11111." },
    { "....1....", ".1111111.", "112211221", "112211221", "111111111", ".1111111." },
    { "...1...", "1111111", "1221221", "1221221", "1111111", ".11111.", ".11111.", ".11.11.", ".11.11." },
    { "11111", "12121", "11111", "11111", "11111" },
};
static const int SPR_W[4] = { 7, 9, 7, 5 };
static const int SPR_H[4] = { 7, 6, 9, 5 };

void botcorps_sprite_draw(lgfx::LovyanGFX &, int x, int y, int shape, int scale, uint8_t color, bool)
{
    if (shape < 0 || shape > 3) shape = 0;
    const int s = scale < 1 ? 1 : scale;
    const int w = SPR_W[shape];
    const int h = SPR_H[shape];
    const int ox = x - (w * s) / 2;
    const int oy = y - (h * s) / 2;
    const uint16_t body = botcorps_rgb565(color);
    const uint16_t eye = botcorps_rgb565(36);
    for (int row = 0; row < h; ++row) {
        const char *line = SPR_ROW[shape][row];
        for (int col = 0; col < w; ++col) {
            const char v = line[col];
            if (v != '1' && v != '2') continue;
            const uint16_t ink = (v == '2') ? eye : body;
            for (int sy = 0; sy < s; ++sy)
                for (int sx = 0; sx < s; ++sx)
                    put565(ox + col * s + sx, oy + row * s + sy, ink);
        }
    }
}

static void bar_title(void)
{
    const uint16_t bar = botcorps_rgb565(5);
    const uint16_t white = 0xFFFF;
    for (int y = 0; y < 22; ++y)
        for (int x = 0; x < 240; ++x) put565(x, y, y == 21 ? botcorps_rgb565(41) : bar);
    s_fg = white;
    s_cx = 6;
    s_cy = 6;
    Gfx g(s_menu);
    g.print("BOTcorps");
}

int main(int argc, char **argv)
{
    const int bot = argc > 1 ? atoi(argv[1]) : 7;   /* mod pass: two-digit bots (10, 11) */
    const bool gib = argc > 2 && argv[2][0] == 'g';
    const char *name = gib ? "gibson" : (argc > 3 ? argv[3] : "keyfobbot");
    const char *target = gib ? "ALL BOTS" : (argc > 4 ? argv[4] : "KEYFOB");
    const uint8_t col = (uint8_t)(argc > 5 ? atoi(argv[5]) : 58);
    const int shape = bot % 4;
    const char *path = argc > 6 ? argv[6] : "/tmp/int-card.rgb";
    if (argc > 7) s_ms = (uint32_t)atol(argv[7]);   /* mod pass: clock for the uptime meter */
    memset(s_rgb, 0, sizeof(s_rgb));
    const uint16_t deep = botcorps_rgb565(64);
    for (int i = 0; i < 240 * 135; ++i) {
        put565(i % 240, i / 240, deep);
    }
    bar_title();
    Gfx g(s_menu);
    botcorps_id_paint(g, 240, gib ? 0 : bot, gib, col, gib ? 0 : shape, name, target);
    FILE *f = fopen(path, "wb");
    if (!f) return 1;
    fwrite(s_rgb, 1, sizeof(s_rgb), f);
    fclose(f);
    /* Badge pixels: the photo badge is a grey block. Count non-deep pixels. */
    int ink = 0;
    for (int i = 0; i < 240 * 135 * 3; i += 3)
        if (s_rgb[i] | s_rgb[i + 1] | s_rgb[i + 2]) ink++;
    printf("int ink %d bot %d col %u\n", ink, bot, col);
    return 0;
}
