/*
 * Hatch plate for BOTcorps. Olive drab ground, flaked millscale in
 * the margin, a clean door, stencil caps. Not phosphor rain, not a
 * cyan terminal, not a magenta HUD.
 *
 * The glass path is botcorps_menu() only. Glyphs use the panel's
 * existing landscape rotation and advance left to right. This file
 * does not call setRotation, setSwapBytes, or setColorDepth.
 */
#include "enter/botcorps_enter.h"

#include "display/botcorps_display.h"

#include <stdio.h>

/* 8-bit sources, stored as RGB565. Boot already set 16-bit color. */
static const uint16_t COL_PLATE       = 0x31C4; /* 48, 58, 34  olive drab */
static const uint16_t COL_SCALE_HI    = 0x5B07; /* 90, 98, 60  raised flake */
static const uint16_t COL_SCALE_LO    = 0x1922; /* 30, 36, 20  recessed flake */
static const uint16_t COL_DOOR        = 0x4285; /* 70, 80, 46  hatch door */
static const uint16_t COL_RECESS      = 0x10A1; /* 18, 22, 12  shadow */
static const uint16_t COL_SEAM_HI     = 0x73E9; /* 118,124, 78  milled lip */
static const uint16_t COL_STENCIL     = 0xD673; /* 214,204,156 khaki stencil */
static const uint16_t COL_STENCIL_DIM = 0x8C2C; /* 140,132, 96 */
static const uint16_t COL_BOLT        = 0x4A05; /* 78, 66, 42 */
static const uint16_t COL_BOLT_HI     = 0x9C6B; /* 156,140, 88 */

static uint32_t flake_mix(uint32_t x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static void draw_bolt(lgfx::LGFX_Device &d, int x, int y)
{
    d.fillRect(x, y, 7, 7, COL_BOLT);
    d.drawRect(x, y, 7, 7, COL_RECESS);
    d.drawFastHLine(x + 1, y + 1, 5, COL_BOLT_HI);
    d.drawPixel(x + 3, y + 3, COL_RECESS);
}

/* Horizontal flakes, scanned left to right. The door is filled
 * afterwards, so the legend stays a solid plate. */
static void draw_millscale(lgfx::LGFX_Device &d, int w, int h)
{
    uint32_t s = 0xB07C011u;
    for (int y = 3; y < h - 3; y += 3) {
        int x = 3;
        while (x < w - 3) {
            s = flake_mix(s + (uint32_t)(y * 131 + x));
            int len = 2 + (int)(s % 8u);
            if (x + len > w - 3) len = (w - 3) - x;
            if (len > 0 && (s & 3u) != 0u) {
                const uint16_t tone = (s & 4u) ? COL_SCALE_HI : COL_SCALE_LO;
                d.drawFastHLine(x, y, len, tone);
            }
            x += len + 1 + (int)((s >> 8) & 3u);
        }
    }
}

/* Print s starting at x and growing to the right. Datum stays top-left. */
static void print_ltr(lgfx::LGFX_Device &d, int x, int y, const char *text,
                      uint16_t fg, uint16_t bg)
{
    d.setTextColor(fg, bg);
    d.setCursor(x, y);
    d.print(text);
}

static void print_centered_ltr(lgfx::LGFX_Device &d, int box_x, int box_w,
                               int y, const char *text,
                               uint16_t fg, uint16_t bg)
{
    const int tw = d.textWidth(text);
    int x = box_x + (box_w - tw) / 2;
    if (x < box_x) x = box_x;
    print_ltr(d, x, y, text, fg, bg);
}

void botcorps_enter_draw(void)
{
    /* Active surface is the menu, but every pixel goes through
     * botcorps_menu() so a stale content surface cannot steal the hatch. */
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);

    auto &d = botcorps_menu();
    const int w = d.width();
    const int h = d.height();
    if (w < 16 || h < 16) return;

    d.clearClipRect();
    d.setTextWrap(false, false);
    d.setTextDatum(lgfx::textdatum::top_left);
    d.setTextSize(1);

    d.fillScreen(COL_PLATE);
    draw_millscale(d, w, h);

    const int door_x = 20;
    const int door_y = 30;
    const int door_w = w - 40;
    const int door_h = h - 54;
    if (door_w > 8 && door_h > 8) {
        d.fillRect(door_x, door_y, door_w, door_h, COL_DOOR);
        d.drawRect(door_x, door_y, door_w, door_h, COL_RECESS);
        d.drawFastHLine(door_x + 1, door_y + 1, door_w - 2, COL_SEAM_HI);
        d.drawFastVLine(door_x + 1, door_y + 1, door_h - 2, COL_SEAM_HI);
        d.drawFastHLine(door_x + 1, door_y + door_h - 2, door_w - 2, COL_SCALE_LO);
        d.drawFastVLine(door_x + door_w - 2, door_y + 1, door_h - 2, COL_SCALE_LO);

        /* Machined ticks down the left lip of the door. */
        for (int i = 0; i < 6; ++i) {
            const int ty = door_y + 8 + i * 8;
            if (ty >= door_y + door_h - 6) break;
            const int len = (i & 1) ? 4 : 7;
            d.drawFastHLine(door_x + 4, ty, len, COL_STENCIL_DIM);
        }

        /* Latch dog on the right of the door. */
        const int lx = door_x + door_w - 16;
        const int ly = door_y + (door_h / 2) - 6;
        d.fillRect(lx, ly, 8, 12, COL_BOLT);
        d.drawRect(lx, ly, 8, 12, COL_RECESS);
        d.fillRect(lx + 2, ly + 4, 4, 4, COL_BOLT_HI);

        d.setTextSize(2);
        print_centered_ltr(d, door_x + 8, door_w - 28, door_y + 16,
                           "PRESS ENTER", COL_STENCIL, COL_DOOR);
        d.setTextSize(1);
        print_centered_ltr(d, door_x + 8, door_w - 28, door_y + 40,
                           "TO OPEN", COL_STENCIL_DIM, COL_DOOR);
    }

    d.drawRect(0, 0, w, h, COL_RECESS);
    d.drawRect(2, 2, w - 4, h - 4, COL_SCALE_LO);
    d.drawFastHLine(8, 24, w - 16, COL_RECESS);
    d.drawFastHLine(8, 25, w - 16, COL_SEAM_HI);

    if (w >= 80 && h >= 40) {
        draw_bolt(d, 5, 5);
        draw_bolt(d, w - 12, 5);
        draw_bolt(d, 5, h - 12);
        draw_bolt(d, w - 12, h - 12);
    }

    d.setTextSize(1);
    print_ltr(d, 16, 10, "BOTCORPS", COL_STENCIL, COL_PLATE);
    const int hatch_w = d.textWidth("HATCH");
    int hatch_x = w - 16 - hatch_w;
    if (hatch_x < 80) hatch_x = 80;
    print_ltr(d, hatch_x, 10, "HATCH", COL_STENCIL_DIM, COL_PLATE);

    print_ltr(d, 16, h - 16, "MENU", COL_STENCIL, COL_PLATE);
    char geom[16];
    snprintf(geom, sizeof(geom), "%dx%d", w, h);
    const int geom_w = d.textWidth(geom);
    int geom_x = w - 16 - geom_w;
    if (geom_x < 70) geom_x = 70;
    print_ltr(d, geom_x, h - 16, geom, COL_STENCIL_DIM, COL_PLATE);
}

static bool enter_down(void)
{
    M5Cardputer.update();
    return M5Cardputer.Keyboard.keysState().enter;
}

void botcorps_enter_run(void)
{
    botcorps_enter_draw();
    Serial.println("[botcorps] hatch on MENU — ENTER to leave");

    /* Require a press that begins after the plate is visible. */
    bool armed = false;
    while (true) {
        const bool down = enter_down();
        if (!down) armed = true;
        if (armed && down) break;
        delay(20);
    }
    Serial.println("[botcorps] hatch ENTER");
}
