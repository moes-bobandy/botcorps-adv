/*
 * Rain look from the approved desktop mock (dense, bots + code, 90 ms).
 *
 * Dense pitch in that script is gap = 6 (ceil(320/6) = 54, plus the
 * script's extra column). The 8px pitch is the non-dense setting.
 * Mix is bots and code only: a bot every third column, code on the
 * rest, then seven anchored bots. No device glyphs.
 *
 * Fade is rgba(3,12,6,0.18) over the previous frame. The external
 * panel cannot be read back, so the fade runs in the sprite buffer.
 */
#include "rain/botcorps_rain.h"

#include "display/botcorps_display.h"

#include <lgfx/v1/LGFX_Sprite.hpp>
#include <string.h>

static const uint16_t COL_INT_FIELD = 0x1082; /* 16,16,16 */
static const uint16_t COL_INT_BAR   = 0x10C4; /* 22,26,34 */
static const uint16_t COL_INT_RULE  = 0x8C71; /* 140,140,140 */
static const uint16_t COL_NAME      = 0xFFFF;

static const uint16_t COL_FIELD = 0x0041; /* #070a08 */
static const uint16_t COL_EYE   = 0x0080; /* #061006 */
static const uint16_t COL_CODE  = 0x7E6C; /* #7dce62 */
static const uint16_t COL_HEAD  = 0xEFF9; /* #e9ffc8 */
static const int FADE_R = 3, FADE_G = 12, FADE_B = 6;
static const int FADE_A = 46;   /* 0.18 * 256 */
static const int BACK_A = 141;  /* 0.55 * 256 */

static const uint16_t PALETTE[14] = {
    0x9F0B, 0xFDC9, 0x36B8, 0xAC7F, 0xFB72, 0x5D3F, 0xF689,
    0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8,
};

static const char CODE_GLYPHS[] = "01<>[]{}:/\\|+-=*#";
static const int CODE_N = 18;
static const int BAR_H = 22;
static const int GAP = 6;
static const int STEP_MS = 90;
static const int RAIN_COLS = 56;
static const int CELL = 8;
static const int CELL_COLS = 40;
static const int CELL_ROWS = 30;

enum { KIND_CODE = 0, KIND_BOT = 1 };

struct col_t {
    int16_t y;
    uint8_t kind;
    uint8_t hat;
    uint8_t trail;
    uint8_t color;
    uint8_t jitter; /* 0..35, percent of steps held */
};

static col_t s_cols[RAIN_COLS];
static int s_ncols = 0;
static int s_w = 0;
static int s_h = 0;
static uint32_t s_rng = 0xA5C3E1u;
static uint32_t s_last_ms = 0;
static uint32_t s_acc_ms = 0;
static bool s_started = false;

static lgfx::LGFX_Sprite *s_sprite = nullptr;
static bool s_use_sprite = false;
static bool s_path_logged = false;

/* Fallback memory: one brightness and one color id per 8x8 cell. */
static uint8_t s_cell_fade[CELL_COLS * CELL_ROWS];
static uint8_t s_cell_id[CELL_COLS * CELL_ROWS];
static uint8_t s_cell_glyph[CELL_COLS * CELL_ROWS];

static uint32_t rnd_u(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return s_rng;
}

static int rnd_n(int n)
{
    if (n <= 1) return 0;
    return (int)(rnd_u() % (uint32_t)n);
}

static int js_round_pos(float x)
{
    return (int)(x + 0.5f);
}

static void unpack565(uint16_t p, int *r, int *g, int *b)
{
    int r5 = (p >> 11) & 31;
    int g6 = (p >> 5) & 63;
    int b5 = p & 31;
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

static uint16_t blend565(uint16_t dst, int sr, int sg, int sb, int a)
{
    int r, g, b;
    int inv = 256 - a;
    unpack565(dst, &r, &g, &b);
    r = (sr * a + r * inv) >> 8;
    g = (sg * a + g * inv) >> 8;
    b = (sb * a + b * inv) >> 8;
    return pack565(r, g, b);
}

static int pick_bot_color(int self)
{
    bool used[14] = {};
    int free_n = 0;
    for (int i = 0; i < s_ncols; ++i) {
        if (i == self || s_cols[i].kind != KIND_BOT) continue;
        if (s_cols[i].color < 14) used[s_cols[i].color] = true;
    }
    for (int i = 0; i < 14; ++i) {
        if (!used[i]) ++free_n;
    }
    if (free_n == 0) return rnd_n(14);
    int pick = rnd_n(free_n);
    for (int i = 0; i < 14; ++i) {
        if (used[i]) continue;
        if (pick == 0) return i;
        --pick;
    }
    return 0;
}

static void build_columns(int width, int height)
{
    s_ncols = (width + GAP - 1) / GAP + 1;
    if (s_ncols > RAIN_COLS) s_ncols = RAIN_COLS;
    for (int i = 0; i < s_ncols; ++i) {
        s_cols[i].y = (int16_t)rnd_n(height);
        s_cols[i].kind = (i % 3 == 0) ? KIND_BOT : KIND_CODE;
        s_cols[i].hat = (uint8_t)(i % 2 == 0);
        s_cols[i].trail = (uint8_t)(rnd_n(100) < 15);
        s_cols[i].color = (uint8_t)rnd_n(14);
        s_cols[i].jitter = (uint8_t)rnd_n(36);
    }
    uint8_t deal[14];
    for (int i = 0; i < 14; ++i) deal[i] = (uint8_t)i;
    for (int i = 13; i > 0; --i) {
        int j = rnd_n(i + 1);
        uint8_t tmp = deal[i];
        deal[i] = deal[j];
        deal[j] = tmp;
    }
    bool taken[RAIN_COLS] = {};
    const int anchors = 7;
    for (int k = 0; k < anchors; ++k) {
        int idx = js_round_pos((k + 0.5f) * (float)(s_ncols - 1) / (float)anchors);
        if (idx < 0) idx = 0;
        if (idx >= s_ncols) idx = s_ncols - 1;
        int guard = 0;
        while (taken[idx] && guard < s_ncols) {
            idx = (idx + 1) % s_ncols;
            ++guard;
        }
        taken[idx] = true;
        s_cols[idx].kind = KIND_BOT;
        s_cols[idx].y = (int16_t)(((k + 0.2f) / (float)anchors) * (float)height);
        s_cols[idx].hat = (uint8_t)(k % 2 == 0);
        s_cols[idx].color = deal[k];
    }
    s_w = width;
    s_h = height;
}

static void draw_bot(lgfx::LovyanGFX &g, int x, int y, uint16_t color, bool hat)
{
    const int top = hat ? 1 : 0;
    if (hat) g.fillRect(x, y, 7, 1, color);
    g.fillRect(x, y + top, 7, 6, color);
    const int ey = y + top + 2;
    g.fillRect(x + 1, ey, 2, 2, COL_EYE);
    g.fillRect(x + 4, ey, 2, 2, COL_EYE);
}

static void sprite_blend_rect(lgfx::LGFX_Sprite &spr, int x, int y, int w, int h, int a)
{
    uint16_t *base = (uint16_t *)spr.getBuffer();
    if (!base) return;
    const int sw = spr.width();
    const int sh = spr.height();
    const size_t stride = (sh > 0) ? (spr.bufferLength() / (size_t)sh) / 2u : (size_t)sw;
    for (int row = 0; row < h; ++row) {
        const int py = y + row;
        if (py < 0 || py >= sh) continue;
        uint16_t *line = base + (size_t)py * stride;
        for (int col = 0; col < w; ++col) {
            const int px = x + col;
            if (px < 0 || px >= sw) continue;
            line[px] = blend565(line[px], FADE_R, FADE_G, FADE_B, a);
        }
    }
}

static void sprite_fade_all(lgfx::LGFX_Sprite &spr)
{
    uint16_t *base = (uint16_t *)spr.getBuffer();
    if (!base) return;
    const int sw = spr.width();
    const int sh = spr.height();
    const size_t stride = (sh > 0) ? (spr.bufferLength() / (size_t)sh) / 2u : (size_t)sw;
    for (int y = 0; y < sh; ++y) {
        uint16_t *line = base + (size_t)y * stride;
        for (int x = 0; x < sw; ++x) {
            line[x] = blend565(line[x], FADE_R, FADE_G, FADE_B, FADE_A);
        }
    }
}

static void draw_code(lgfx::LovyanGFX &g, int x, int y, bool head)
{
    const char ch = CODE_GLYPHS[rnd_n(CODE_N)];
    g.setTextWrap(false, false);
    g.setTextDatum(lgfx::textdatum::top_left);
    g.setTextSize(1);
    g.setTextColor(head ? COL_HEAD : COL_CODE);
    g.setCursor(x, y);
    g.print(ch);
}

static bool code_is_head(int index, int y)
{
    int q;
    if (y >= 0) q = y / GAP;
    else q = -(((-y) + GAP - 1) / GAP);
    int slot = index + q;
    int m = slot % 6;
    if (m < 0) m += 6;
    return m == 0;
}

static void draw_column(lgfx::LovyanGFX &g, int index, bool blend_back)
{
    col_t &c = s_cols[index];
    const int x = index * GAP;
    if (c.kind == KIND_BOT) {
        if (!c.trail && blend_back && s_sprite) {
            sprite_blend_rect(*s_sprite, x - 1, c.y - 18, 12, 28, BACK_A);
        }
        draw_bot(g, x, c.y, PALETTE[c.color], c.hat != 0);
    } else {
        draw_code(g, x, c.y, code_is_head(index, c.y));
    }
}

static void step_columns(lgfx::LovyanGFX &g, bool blend_back)
{
    for (int i = 0; i < s_ncols; ++i) {
        col_t &c = s_cols[i];
        if (rnd_n(100) < c.jitter) continue;
        draw_column(g, i, blend_back);
        c.y = (int16_t)(c.y + GAP);
        if (c.y > s_h + GAP) {
            c.y = (int16_t)(-GAP - rnd_n(4) * GAP);
            if (c.kind == KIND_BOT) c.color = (uint8_t)pick_bot_color(i);
        }
    }
}

static int cell_index(int cx, int cy)
{
    if (cx < 0 || cy < 0 || cx >= CELL_COLS || cy >= CELL_ROWS) return -1;
    return cy * CELL_COLS + cx;
}

static void cell_paint(lgfx::LGFX_Device &panel, int cx, int cy)
{
    const int i = cell_index(cx, cy);
    if (i < 0) return;
    const int x = cx * CELL;
    const int y = cy * CELL;
    const uint8_t fade = s_cell_fade[i];
    uint16_t color = COL_FIELD;
    if (fade > 0 && s_cell_id[i] != 0) {
        uint16_t base = COL_CODE;
        if (s_cell_id[i] == 16) base = COL_HEAD;
        else if (s_cell_id[i] == 17) base = blend565(COL_FIELD, FADE_R, FADE_G, FADE_B, BACK_A);
        else if (s_cell_id[i] <= 14) base = PALETTE[s_cell_id[i] - 1];
        const int a = 256 - (fade * 16);
        color = blend565(base, FADE_R, FADE_G, FADE_B, a > 255 ? 255 : a);
    }
    panel.fillRect(x, y, CELL, CELL, color);
    if (fade > 10 && s_cell_id[i] >= 1 && s_cell_id[i] <= 14) {
        panel.fillRect(x + 1, y + 2, 2, 2, COL_EYE);
        panel.fillRect(x + 4, y + 2, 2, 2, COL_EYE);
    } else if (fade > 8 && (s_cell_id[i] == 15 || s_cell_id[i] == 16)) {
        const char ch = CODE_GLYPHS[s_cell_glyph[i] % CODE_N];
        panel.setTextSize(1);
        panel.setTextColor(color);
        panel.setCursor(x, y);
        panel.print(ch);
    }
}

static void cell_stamp(lgfx::LGFX_Device &panel, int x, int y, int w, int h,
                       uint8_t id, uint8_t glyph)
{
    if (w <= 0 || h <= 0) return;
    const int x0 = x < 0 ? 0 : x;
    const int y0 = y < 0 ? 0 : y;
    const int x1 = x + w;
    const int y1 = y + h;
    for (int cy = y0 / CELL; cy <= (y1 - 1) / CELL; ++cy) {
        for (int cx = x0 / CELL; cx <= (x1 - 1) / CELL; ++cx) {
            const int i = cell_index(cx, cy);
            if (i < 0) continue;
            s_cell_fade[i] = 15;
            s_cell_id[i] = id;
            s_cell_glyph[i] = glyph;
            cell_paint(panel, cx, cy);
        }
    }
}

static void fallback_fade_and_draw(lgfx::LGFX_Device &panel)
{
    for (int i = 0; i < CELL_COLS * CELL_ROWS; ++i) {
        if (s_cell_fade[i] == 0) continue;
        const uint8_t next = (uint8_t)((s_cell_fade[i] * 210) >> 8);
        if (next == s_cell_fade[i]) continue;
        s_cell_fade[i] = next;
        if (s_cell_fade[i] < 2) {
            s_cell_fade[i] = 0;
            s_cell_id[i] = 0;
        }
        cell_paint(panel, i % CELL_COLS, i / CELL_COLS);
    }
    for (int i = 0; i < s_ncols; ++i) {
        col_t &c = s_cols[i];
        if (rnd_n(100) < c.jitter) continue;
        const int x = i * GAP;
        if (c.kind == KIND_BOT) {
            if (!c.trail) {
                cell_stamp(panel, x - 1, c.y - 18, 12, 28, 17, 0);
            }
            cell_stamp(panel, x, c.y, 7, c.hat ? 7 : 6, (uint8_t)(c.color + 1), 0);
        } else {
            const bool head = code_is_head(i, c.y);
            const uint8_t glyph = (uint8_t)rnd_n(CODE_N);
            cell_stamp(panel, x, c.y, 6, 8, head ? 16 : 15, glyph);
        }
        c.y = (int16_t)(c.y + GAP);
        if (c.y > s_h + GAP) {
            c.y = (int16_t)(-GAP - rnd_n(4) * GAP);
            if (c.kind == KIND_BOT) c.color = (uint8_t)pick_bot_color(i);
        }
    }
}

static bool ensure_sprite(lgfx::LGFX_Device &panel, int width, int height)
{
    if (s_path_logged && !s_use_sprite) return false;
    if (s_sprite && s_sprite->width() == width && s_sprite->height() == height) {
        return true;
    }
    delete s_sprite;
    s_sprite = nullptr;
    const size_t bytes = (size_t)width * (size_t)height * 2u;
    const size_t max_alloc = ESP.getMaxAllocHeap();
    if (max_alloc < bytes + 8192u) {
        s_use_sprite = false;
        if (!s_path_logged) {
            Serial.printf("[botcorps] rain path=fallback cells=%dx%d max_alloc=%u need=%u\n",
                          CELL_COLS, CELL_ROWS, (unsigned)max_alloc,
                          (unsigned)(bytes + 8192u));
            s_path_logged = true;
        }
        return false;
    }
    s_sprite = new lgfx::LGFX_Sprite(&panel);
    if (!s_sprite->createSprite(width, height)) {
        delete s_sprite;
        s_sprite = nullptr;
        s_use_sprite = false;
        if (!s_path_logged) {
            Serial.printf("[botcorps] rain path=fallback cells=%dx%d create failed\n",
                          CELL_COLS, CELL_ROWS);
            s_path_logged = true;
        }
        return false;
    }
    s_sprite->fillScreen(COL_FIELD);
    s_use_sprite = true;
    if (!s_path_logged) {
        Serial.printf("[botcorps] rain path=sprite %dx%d max_alloc=%u\n",
                      width, height, (unsigned)max_alloc);
        s_path_logged = true;
    }
    return true;
}

static void paint_name_bar(lgfx::LGFX_Device &d, int width)
{
    d.fillRect(0, 0, width, BAR_H, COL_INT_BAR);
    d.setTextWrap(false, false);
    d.setTextDatum(lgfx::textdatum::top_left);
    d.setTextSize(2);
    d.setTextColor(COL_NAME, COL_INT_BAR);
    d.setCursor(8, 3);
    d.print("BOTcorps");
    d.drawFastHLine(0, BAR_H, width, COL_INT_RULE);
}

void botcorps_nav_draw(void)
{
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &menu = botcorps_menu();
    menu.clearClipRect();
    menu.fillScreen(COL_INT_FIELD);
    paint_name_bar(menu, menu.width());
}

static void present_step(lgfx::LGFX_Device &panel, int width, int height)
{
    if (ensure_sprite(panel, width, height)) {
        sprite_fade_all(*s_sprite);
        step_columns(*s_sprite, true);
        s_sprite->pushSprite(0, 0);
        return;
    }
    fallback_fade_and_draw(panel);
}

void botcorps_rain_frame(void)
{
    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    auto &panel = botcorps_content();
    const int width = panel.width();
    const int height = panel.height();
    if (width < 32 || height < 32) return;
    if (s_w != width || s_h != height) {
        build_columns(width, height);
        delete s_sprite;
        s_sprite = nullptr;
        s_use_sprite = false;
        s_path_logged = false;
        s_started = false;
        memset(s_cell_fade, 0, sizeof(s_cell_fade));
    }

    const uint32_t now = millis();
    bool stepped = false;
    if (!s_started) {
        s_started = true;
        s_last_ms = now;
        s_acc_ms = 0;
        if (ensure_sprite(panel, width, height)) {
            s_sprite->fillScreen(COL_FIELD);
        } else {
            panel.fillRect(0, 0, width, height, COL_FIELD);
        }
        present_step(panel, width, height);
        stepped = true;
    } else {
        s_acc_ms += now - s_last_ms;
        s_last_ms = now;
        int guard = 0;
        while (s_acc_ms >= (uint32_t)STEP_MS && guard < 2) {
            s_acc_ms -= (uint32_t)STEP_MS;
            present_step(panel, width, height);
            stepped = true;
            ++guard;
        }
        if (s_acc_ms > (uint32_t)STEP_MS * 4u) s_acc_ms = (uint32_t)STEP_MS;
    }

    /* One panel: put the name bar back. Dual EXT stays rain only. */
    if (stepped && !botcorps_dual_ok()) {
        paint_name_bar(panel, width);
    }
}
