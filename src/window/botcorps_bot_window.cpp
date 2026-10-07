/*
 * Bot window. Drift is horizontal millscale in the seat color, not
 * phosphor rain. Chrome is an opaque plate drawn after the drift.
 *
 * Sprite setColorDepth runs on the offscreen sprite only. This file
 * does not call setColorDepth, setRotation, or the SD quiesce path
 * on either panel.
 */
#include "window/botcorps_bot_window.h"

#include "display/botcorps_display.h"
#include "ready/botcorps_ready.h"

#include <lgfx/v1/LGFX_Sprite.hpp>
#include <stdio.h>

static const uint16_t COL_PLATE   = 0x31C4;
static const uint16_t COL_SEAM_HI = 0x73E9;
static const uint16_t COL_STENCIL = 0xD673;
static const uint16_t COL_DIM     = 0x8C2C;

enum { FLAKE_N = 28 };

struct flake_t {
    int x;
    int y;
    int len;
    int vx;
    uint16_t tone;
};

struct sprites_t {
    lgfx::LGFX_Sprite *a;
    lgfx::LGFX_Sprite *b;
    int w;
    int h;
    int count;
    bool full;
};

static flake_t s_flakes[FLAKE_N];
static int s_flake_seat = -1;
static int s_field_w = 0;
static int s_field_h = 0;

static void begin_text(lgfx::LovyanGFX &d)
{
    d.setTextWrap(false, false);
    d.setTextDatum(lgfx::textdatum::top_left);
    d.setTextSize(1);
}

static void init_flakes(int seat, int w, int h)
{
    uint32_t s = 0xB07Cu + (uint32_t)seat * 131u;
    const uint16_t flake = botcorps_seat_flake(seat);
    const uint16_t ink = botcorps_seat_ink(seat);
    for (int i = 0; i < FLAKE_N; ++i) {
        s = s * 1664525u + 1013904223u;
        s_flakes[i].len = 4 + (int)((s >> 8) & 15u);
        s_flakes[i].x = (int)(s % (uint32_t)(w + s_flakes[i].len));
        s_flakes[i].y = (int)((s >> 4) % (uint32_t)h);
        s_flakes[i].vx = 1 + (int)((s >> 16) & 2u);
        s_flakes[i].tone = (s & 1u) ? flake : ink;
    }
    s_flake_seat = seat;
    s_field_w = w;
    s_field_h = h;
}

static void step_flakes(void)
{
    for (int i = 0; i < FLAKE_N; ++i) {
        s_flakes[i].x += s_flakes[i].vx;
        if (s_flakes[i].x > s_field_w) s_flakes[i].x = -s_flakes[i].len;
    }
}

/* Offscreen or one band. y is the row inside this target. */
static void paint_drift(lgfx::LovyanGFX &dst, int seat, int w, int h,
                        int world_y)
{
    dst.fillRect(0, 0, w, h, botcorps_seat_body(seat));
    for (int i = 0; i < FLAKE_N; ++i) {
        const int y = s_flakes[i].y;
        if (y < world_y || y >= world_y + h) continue;
        dst.drawFastHLine(s_flakes[i].x, y - world_y, s_flakes[i].len,
                          s_flakes[i].tone);
    }
}

static void paint_saver_ring(lgfx::LovyanGFX &dst, int w, int local_h,
                             int world_y, int panel_h)
{
    const int r = panel_h / 2 - 8;
    if (r < 8) return;
    const int cy = panel_h / 2 - world_y;
    dst.drawCircle(w / 2, cy, r, COL_SEAM_HI);
    dst.drawCircle(w / 2, cy, r - 8, COL_DIM);
}

static void draw_chrome(lgfx::LovyanGFX &d, int seat, int w, int h,
                        int head, int foot)
{
    const uint16_t body = botcorps_seat_body(seat);
    begin_text(d);

    d.fillRect(0, 0, w, head, COL_PLATE);
    d.drawFastHLine(0, head - 1, w, COL_SEAM_HI);
    const int box = head - 10;
    if (box >= 10) {
        d.fillRect(6, 5, box, box, body);
        botcorps_seat_draw_icon(d, seat, 6 + box / 2, 5 + box / 2,
                                box / 2 - 2);
        d.setTextSize(w >= 300 ? 2 : 1);
        d.setTextColor(COL_STENCIL, COL_PLATE);
        d.setCursor(box + 14, 6);
        d.print(botcorps_seat_name(seat));
    } else {
        d.setTextSize(1);
        d.setTextColor(COL_STENCIL, COL_PLATE);
        d.setCursor(8, 4);
        d.print(botcorps_seat_name(seat));
    }

    d.fillRect(0, h - foot, w, foot, COL_PLATE);
    d.drawFastHLine(0, h - foot, w, COL_SEAM_HI);
    d.setTextSize(1);
    d.setTextColor(COL_STENCIL, COL_PLATE);
    char state[28];
    snprintf(state, sizeof(state), "RADIO OFF  %s", botcorps_seat_band(seat));
    if (w >= 300) {
        const int ty = h - foot + (foot - 8) / 2;
        d.setCursor(8, ty);
        d.print(state);
        const int bw = d.textWidth("` BACK");
        d.setTextColor(COL_DIM, COL_PLATE);
        d.setCursor(w - 8 - bw, ty);
        d.print("` BACK");
    } else {
        d.setCursor(8, h - foot + 2);
        d.print(state);
        d.setTextColor(COL_DIM, COL_PLATE);
        d.setCursor(8, h - 10);
        d.print("` BACK");
    }
}

static void free_sprites(sprites_t *s)
{
    delete s->a;
    delete s->b;
    s->a = nullptr;
    s->b = nullptr;
    s->count = 0;
}

/* Sprite depth is the sprite's own buffer. The check is against the
 * largest free block so a failed alloc does not get attempted. */
static lgfx::LGFX_Sprite *make_sprite(lgfx::LGFX_Device &d, int w, int h)
{
    const size_t bytes = (size_t)w * (size_t)h * 2u;
    if (ESP.getMaxAllocHeap() < bytes + 4096u) return nullptr;
    lgfx::LGFX_Sprite *s = new lgfx::LGFX_Sprite(&d);
    if (!s->createSprite(w, h)) {
        delete s;
        return nullptr;
    }
    return s;
}

static bool alloc_sprites(lgfx::LGFX_Device &d, int panel_w, int panel_h,
                          int band_h, sprites_t *out)
{
    const int candidates[6] = { panel_h, band_h, 120, 80, 60, 48 };
    *out = sprites_t{};
    for (int i = 0; i < 6; ++i) {
        const int h = candidates[i];
        if (h < 16 || h > panel_h) continue;
        bool dup = false;
        for (int j = 0; j < i; ++j) {
            if (candidates[j] == h) dup = true;
        }
        if (dup) continue;
        lgfx::LGFX_Sprite *a = make_sprite(d, panel_w, h);
        if (!a) continue;
        out->a = a;
        out->b = make_sprite(d, panel_w, h);
        out->w = panel_w;
        out->h = h;
        out->count = out->b ? 2 : 1;
        out->full = (h == panel_h);
        return true;
    }
    return false;
}

static lgfx::LGFX_Sprite *back_buffer(sprites_t *s, uint32_t frame)
{
    if (s->count == 2 && (frame & 1u)) return s->b;
    return s->a;
}

static void push_band(lgfx::LGFX_Device &panel, lgfx::LGFX_Sprite &spr,
                      int x, int y, int view_h)
{
    if (view_h < spr.height()) {
        panel.setClipRect(x, y, spr.width(), view_h);
        spr.pushSprite(x, y);
        panel.clearClipRect();
    } else {
        spr.pushSprite(x, y);
    }
}

static bool poll_key(bool *back)
{
    *back = false;
    M5Cardputer.update();
    auto &kb = M5Cardputer.Keyboard;
    if (!kb.isChange() || !kb.isPressed()) return false;
    if (kb.isKeyPressed('`') || kb.isKeyPressed((char)0x1B)) *back = true;
    return true;
}

static void wait_keys_up(void)
{
    while (true) {
        M5Cardputer.update();
        if (!M5Cardputer.Keyboard.isPressed()) return;
        delay(10);
    }
}

/* Dirty-rect fallback. Erases each flake's previous span and draws
 * the new one. The panel is not cleared. */
static void dirty_step(lgfx::LGFX_Device &panel, int seat, int w,
                       int top, int height)
{
    const uint16_t body = botcorps_seat_body(seat);
    for (int i = 0; i < FLAKE_N; ++i) {
        flake_t &f = s_flakes[i];
        if (f.y >= top && f.y < top + height) {
            int ex = f.x;
            if (ex < 0) ex = 0;
            int ew = f.len;
            if (ex + ew > w) ew = w - ex;
            if (ew > 0) panel.fillRect(ex, f.y, ew, 1, body);
        }
        f.x += f.vx;
        if (f.x > w) f.x = -f.len;
        if (f.y >= top && f.y < top + height) {
            panel.drawFastHLine(f.x, f.y, f.len, f.tone);
        }
    }
}

static void log_heap(const char *where, const sprites_t *spr)
{
    Serial.printf("[botcorps] %s heap free=%u max_alloc=%u\n",
                  where,
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getMaxAllocHeap());
    if (spr && spr->count) {
        Serial.printf("[botcorps] %s sprite %dx%d x%d %s\n",
                      where, spr->w, spr->h, spr->count,
                      spr->full ? "full" : "band");
    } else {
        Serial.printf("[botcorps] %s sprite none — dirty rect\n", where);
    }
}

static void run_sprite_loop(lgfx::LGFX_Device &panel, sprites_t *spr, int seat,
                            int panel_w, int panel_h, int head, int foot,
                            bool window_mode)
{
    const int band_y = head;
    const int band_h = panel_h - head - foot;
    uint32_t frame = 0;
    uint32_t last_input = millis();
    bool saver = !window_mode;
    bool chrome_on_panel = false;

    if (!spr->full && window_mode) {
        draw_chrome(panel, seat, panel_w, panel_h, head, foot);
        chrome_on_panel = true;
    }

    wait_keys_up();
    while (true) {
        bool back = false;
        const bool key = poll_key(&back);
        if (key) {
            last_input = millis();
            if (window_mode && back) return;
            if (!window_mode) return;
            if (saver) {
                saver = false;
                chrome_on_panel = false;
            }
        } else if (window_mode && !saver &&
                   (uint32_t)(millis() - last_input) > BOTCORPS_CONTENT_IDLE_MS) {
            saver = true;
            Serial.println("[botcorps] screensaver");
        }

        step_flakes();
        lgfx::LGFX_Sprite *buf = back_buffer(spr, frame);
        if (spr->full) {
            paint_drift(*buf, seat, panel_w, panel_h, 0);
            if (saver || !window_mode) {
                paint_saver_ring(*buf, panel_w, panel_h, 0, panel_h);
            } else {
                draw_chrome(*buf, seat, panel_w, panel_h, head, foot);
            }
            buf->pushSprite(0, 0);
        } else if (saver || !window_mode) {
            for (int y = 0; y < panel_h; y += spr->h) {
                const int rows = (y + spr->h > panel_h) ? (panel_h - y) : spr->h;
                paint_drift(*buf, seat, panel_w, spr->h, y);
                paint_saver_ring(*buf, panel_w, spr->h, y, panel_h);
                push_band(panel, *buf, 0, y, rows);
            }
            chrome_on_panel = false;
        } else {
            if (!chrome_on_panel) {
                draw_chrome(panel, seat, panel_w, panel_h, head, foot);
                chrome_on_panel = true;
            }
            for (int y = band_y; y < band_y + band_h; ) {
                int rows = band_y + band_h - y;
                if (rows > spr->h) rows = spr->h;
                paint_drift(*buf, seat, panel_w, spr->h, y);
                push_band(panel, *buf, 0, y, rows);
                y += rows;
            }
        }
        ++frame;
        delay(50);
    }
}

static void run_dirty_loop(lgfx::LGFX_Device &panel, int seat,
                           int panel_w, int panel_h, int head, int foot,
                           bool window_mode)
{
    const int band_y = head;
    const int band_h = panel_h - head - foot;
    uint32_t last_input = millis();
    bool saver = !window_mode;
    bool primed = false;

    wait_keys_up();
    while (true) {
        bool back = false;
        const bool key = poll_key(&back);
        if (key) {
            last_input = millis();
            if (window_mode && back) return;
            if (!window_mode) return;
            saver = false;
            primed = false;
        } else if (window_mode && !saver &&
                   (uint32_t)(millis() - last_input) > BOTCORPS_CONTENT_IDLE_MS) {
            saver = true;
            primed = false;
            Serial.println("[botcorps] screensaver");
        }

        if (!primed) {
            /* One transition paint, not a per-frame clear. */
            if (saver || !window_mode) {
                panel.fillRect(0, 0, panel_w, panel_h, botcorps_seat_body(seat));
                paint_saver_ring(panel, panel_w, panel_h, 0, panel_h);
            } else {
                panel.fillRect(0, band_y, panel_w, band_h, botcorps_seat_body(seat));
                draw_chrome(panel, seat, panel_w, panel_h, head, foot);
            }
            primed = true;
        }

        if (saver || !window_mode) {
            dirty_step(panel, seat, panel_w, 0, panel_h);
        } else {
            dirty_step(panel, seat, panel_w, band_y, band_h);
        }
        delay(50);
    }
}

static void prepare_field(int seat, int w, int h)
{
    if (s_flake_seat != seat || s_field_w != w || s_field_h != h) {
        init_flakes(seat, w, h);
    }
}

static void run_on_content(int seat, bool window_mode)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return;

    botcorps_set_surface(BOTCORPS_SURFACE_CONTENT);
    auto &panel = botcorps_content();
    const int w = panel.width();
    const int h = panel.height();
    int head = (h >= 180) ? 40 : 24;
    int foot = (h >= 180) ? 28 : 20;
    if (head + foot > h - 24) {
        head = 20;
        foot = 16;
    }

    prepare_field(seat, w, h);

    sprites_t spr = {};
    const size_t max_alloc = ESP.getMaxAllocHeap();
    Serial.printf("[botcorps] %s open %s free=%u max_alloc=%u panel=%dx%d\n",
                  window_mode ? "window" : "screensaver",
                  botcorps_seat_name(seat),
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)max_alloc, w, h);

    const bool got = alloc_sprites(panel, w, h, h - head - foot, &spr);
    log_heap(window_mode ? "window" : "screensaver", got ? &spr : nullptr);

    if (got) {
        run_sprite_loop(panel, &spr, seat, w, h, head, foot, window_mode);
        free_sprites(&spr);
    } else {
        run_dirty_loop(panel, seat, w, h, head, foot, window_mode);
    }

    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
}

void botcorps_bot_window_run(int seat)
{
    Serial.printf("[botcorps] window %s\n", botcorps_seat_name(seat));
    run_on_content(seat, true);
    Serial.println("[botcorps] window closed");
}

void botcorps_content_screensaver(int seat)
{
    if (!botcorps_dual_ok()) return;
    Serial.println("[botcorps] screensaver");
    run_on_content(seat, false);
}
