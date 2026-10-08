/*
 * Falling bot marks on a charcoal field. The name sits on an opaque
 * bar drawn after the marks. No test-plate fill, no size readout.
 */
#include "rain/botcorps_rain.h"

#include "display/botcorps_display.h"

#include <lgfx/v1/LGFX_Sprite.hpp>

static const uint16_t COL_FIELD = 0x1082; /* 16,16,16 charcoal */
static const uint16_t COL_BAR   = 0x10C4; /* 22,26,34 slate */
static const uint16_t COL_NAME  = 0xFFFF;
static const uint16_t COL_HEAD  = 0xDEFB;
static const uint16_t COL_MID   = 0x8C71;
static const uint16_t COL_DIM   = 0x4A69;

static const int BAR_H = 22;
static const int RAIN_MAX = 28;
static const char NAME[] = "BOTcorps";

struct drop_t {
    int x;
    int y;
    int speed;
    int tail;
};

static drop_t s_drops[RAIN_MAX];
static int s_count = 0;
static int s_field_w = 0;
static int s_field_h = 0;
static lgfx::LGFX_Sprite *s_sprite = nullptr;
static bool s_sprite_failed = false;

static void bot_mark(lgfx::LovyanGFX &g, int x, int y, uint16_t color)
{
    g.drawPixel(x + 1, y, color);
    g.drawFastHLine(x, y + 1, 3, color);
    g.drawPixel(x, y + 2, color);
    g.drawPixel(x + 2, y + 2, color);
    g.drawFastVLine(x + 1, y + 3, 2, color);
}

static void name_bar(lgfx::LovyanGFX &g, int width)
{
    g.fillRect(0, 0, width, BAR_H, COL_BAR);
    g.setTextWrap(false, false);
    g.setTextDatum(lgfx::textdatum::top_left);
    g.setTextSize(2);
    g.setTextColor(COL_NAME, COL_BAR);
    g.setCursor(8, 3);
    g.print(NAME);
}

static void init_drops(int width, int height)
{
    uint32_t s = 0xC0FFEEu;
    s_count = width / 14;
    if (s_count < 8) s_count = 8;
    if (s_count > RAIN_MAX) s_count = RAIN_MAX;
    for (int i = 0; i < s_count; ++i) {
        s = s * 1664525u + 1013904223u;
        const int span = width / s_count;
        s_drops[i].x = i * span + (int)(s % (uint32_t)(span > 4 ? span - 3 : 1));
        s_drops[i].y = (int)((s >> 4) % (uint32_t)height);
        s_drops[i].speed = 2 + (int)((s >> 12) % 3u);
        s_drops[i].tail = 4 + (int)((s >> 16) % 6u);
    }
    s_field_w = width;
    s_field_h = height;
}

static void step_drops(void)
{
    for (int i = 0; i < s_count; ++i) {
        s_drops[i].y += s_drops[i].speed;
        if (s_drops[i].y - s_drops[i].tail * 5 > s_field_h) {
            s_drops[i].y = -6;
        }
    }
}

static void paint_rain(lgfx::LovyanGFX &g, int width, int height)
{
    g.fillRect(0, 0, width, height, COL_FIELD);
    for (int i = 0; i < s_count; ++i) {
        const drop_t &drop = s_drops[i];
        for (int t = drop.tail; t >= 1; --t) {
            const uint16_t tone = (t > drop.tail / 2) ? COL_DIM : COL_MID;
            bot_mark(g, drop.x, drop.y - t * 5, tone);
        }
        bot_mark(g, drop.x, drop.y, COL_HEAD);
    }
    name_bar(g, width);
}

static bool ensure_sprite(lgfx::LGFX_Device &panel, int width, int height)
{
    if (s_sprite_failed) return false;
    if (s_sprite && s_sprite->width() == width && s_sprite->height() == height) {
        return true;
    }
    delete s_sprite;
    s_sprite = nullptr;
    const size_t bytes = (size_t)width * (size_t)height * 2u;
    const size_t max_alloc = ESP.getMaxAllocHeap();
    if (max_alloc < bytes + 4096u) {
        s_sprite_failed = true;
        Serial.printf("[botcorps] rain sprite skipped max_alloc=%u need=%u\n",
                      (unsigned)max_alloc, (unsigned)(bytes + 4096u));
        return false;
    }
    s_sprite = new lgfx::LGFX_Sprite(&panel);
    if (!s_sprite->createSprite(width, height)) {
        delete s_sprite;
        s_sprite = nullptr;
        s_sprite_failed = true;
        return false;
    }
    Serial.printf("[botcorps] rain sprite %dx%d max_alloc=%u\n",
                  width, height, (unsigned)max_alloc);
    return true;
}

void botcorps_nav_draw(void)
{
    if (!botcorps_dual_ok()) return;
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &menu = botcorps_menu();
    const int width = menu.width();
    menu.clearClipRect();
    menu.fillScreen(COL_FIELD);
    name_bar(menu, width);
    menu.drawFastHLine(0, BAR_H, width, COL_MID);
}

void botcorps_rain_frame(void)
{
    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    auto &panel = botcorps_content();
    const int width = panel.width();
    const int height = panel.height();
    if (width < 16 || height < BAR_H + 8) return;
    if (s_field_w != width || s_field_h != height) {
        init_drops(width, height);
    }
    step_drops();

    if (ensure_sprite(panel, width, height)) {
        paint_rain(*s_sprite, width, height);
        s_sprite->pushSprite(0, 0);
        return;
    }

    /* First frame lays the field. Later frames patch each column. */
    static bool primed = false;
    static int primed_w = 0;
    if (!primed || primed_w != width) {
        panel.fillRect(0, 0, width, height, COL_FIELD);
        primed = true;
        primed_w = width;
    } else {
        for (int i = 0; i < s_count; ++i) {
            panel.fillRect(s_drops[i].x, BAR_H, 3, height - BAR_H, COL_FIELD);
        }
    }
    for (int i = 0; i < s_count; ++i) {
        const drop_t &drop = s_drops[i];
        for (int t = drop.tail; t >= 1; --t) {
            const uint16_t tone = (t > drop.tail / 2) ? COL_DIM : COL_MID;
            bot_mark(panel, drop.x, drop.y - t * 5, tone);
        }
        bot_mark(panel, drop.x, drop.y, COL_HEAD);
    }
    name_bar(panel, width);
}
