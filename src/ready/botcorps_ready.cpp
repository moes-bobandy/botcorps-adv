/*
 * Ready Room. Army-green plate, two lit seats. Wi-Fi is brass.
 * Bluetooth is field slate. Both colors are on the glass before any
 * key is pressed. Text prints left to right.
 *
 * This file does not set rotation, color depth, or the SD bus, and
 * it does not start a radio.
 */
#include "ready/botcorps_ready.h"

#include "display/botcorps_display.h"
#include "window/botcorps_bot_window.h"

#include <math.h>
#include <stdio.h>

static const uint16_t COL_PLATE       = 0x31C4;
static const uint16_t COL_SCALE_HI    = 0x5B07;
static const uint16_t COL_SCALE_LO    = 0x1922;
static const uint16_t COL_RECESS      = 0x10A1;
static const uint16_t COL_SEAM_HI     = 0x73E9;
static const uint16_t COL_STENCIL     = 0xD673;
static const uint16_t COL_STENCIL_DIM = 0x8C2C;

static const uint16_t COL_WIFI        = 0xBC68; /* 186,140, 64 brass */
static const uint16_t COL_WIFI_FLAKE  = 0x7AA4;
static const uint16_t COL_WIFI_INK    = 0x18A1;
static const uint16_t COL_BT          = 0x2AED; /* 46, 92,108 slate */
static const uint16_t COL_BT_FLAKE    = 0x19A8;
static const uint16_t COL_BT_INK      = 0xD673;

/* Both radios are off: this firmware does not start Wi-Fi or BLE.
 * ESP32-S3 Wi-Fi is 2.4 GHz. Its Bluetooth is LE, not classic. */
static const char RADIO_STATE[] = "RADIO OFF";

struct seat_def_t {
    const char *name;
    uint16_t body;
    uint16_t ink;
    uint16_t flake;
    const char *band;
};

static const seat_def_t SEATS[BOTCORPS_SEAT_COUNT] = {
    { "WI-FI",     COL_WIFI, COL_WIFI_INK, COL_WIFI_FLAKE, "2.4 GHz" },
    { "BLUETOOTH", COL_BT,   COL_BT_INK,   COL_BT_FLAKE,   "BLE" },
};

const char *botcorps_seat_name(int seat)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return "";
    return SEATS[seat].name;
}

uint16_t botcorps_seat_body(int seat)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return COL_PLATE;
    return SEATS[seat].body;
}

uint16_t botcorps_seat_ink(int seat)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return COL_STENCIL;
    return SEATS[seat].ink;
}

uint16_t botcorps_seat_flake(int seat)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return COL_SCALE_LO;
    return SEATS[seat].flake;
}

const char *botcorps_seat_band(int seat)
{
    if (seat < 0 || seat >= BOTCORPS_SEAT_COUNT) return "";
    return SEATS[seat].band;
}

static void begin_text(lgfx::LovyanGFX &d)
{
    d.setTextWrap(false, false);
    d.setTextDatum(lgfx::textdatum::top_left);
    d.setTextSize(1);
}

static void print_ltr(lgfx::LovyanGFX &d, int x, int y, const char *text,
                      uint16_t fg, uint16_t bg)
{
    d.setTextColor(fg, bg);
    d.setCursor(x, y);
    d.print(text);
}

static void print_centered_ltr(lgfx::LovyanGFX &d, int box_x, int box_w,
                               int y, const char *text,
                               uint16_t fg, uint16_t bg)
{
    const int tw = d.textWidth(text);
    int x = box_x + (box_w - tw) / 2;
    if (x < box_x) x = box_x;
    print_ltr(d, x, y, text, fg, bg);
}

/* 0 degrees is up, clockwise, in screen space (y grows downward). */
static void arc_ltr(lgfx::LovyanGFX &d, int cx, int cy, int r,
                    int deg0, int deg1, uint16_t ink)
{
    if (r < 2 || deg1 <= deg0) return;
    const float k = 0.0174532925f;
    int prev_x = 0;
    int prev_y = 0;
    bool pen = false;
    for (int deg = deg0; deg <= deg1; deg += 10) {
        const float rad = (float)(deg - 90) * k;
        const int x = cx + (int)(cosf(rad) * (float)r);
        const int y = cy + (int)(sinf(rad) * (float)r);
        if (pen) d.drawLine(prev_x, prev_y, x, y, ink);
        prev_x = x;
        prev_y = y;
        pen = true;
    }
}

static void draw_wifi_icon(lgfx::LovyanGFX &d, int cx, int cy, int radius,
                           uint16_t ink)
{
    const int dot_y = cy + radius / 5;
    const int dot_r = radius > 14 ? 3 : 2;
    d.fillCircle(cx, dot_y, dot_r, ink);
    const int rings = radius > 16 ? 3 : 2;
    for (int i = 0; i < rings; ++i) {
        const int r = radius / 3 + i * (radius / 3);
        if (r > radius) break;
        arc_ltr(d, cx, dot_y, r, 300, 420, ink);
    }
}

static void draw_bt_icon(lgfx::LovyanGFX &d, int cx, int cy, int radius,
                         uint16_t ink)
{
    const int top = cy - radius;
    const int bot = cy + radius;
    const int wing = cx + (radius * 2) / 3;
    const int mid = cy;
    const int upper = cy - radius / 3;
    const int lower = cy + radius / 3;
    d.drawLine(cx, top, cx, bot, ink);
    d.drawLine(cx, top, wing, upper, ink);
    d.drawLine(wing, upper, cx, mid, ink);
    d.drawLine(cx, mid, wing, lower, ink);
    d.drawLine(wing, lower, cx, bot, ink);
}

void botcorps_seat_draw_icon(lgfx::LovyanGFX &d, int seat,
                             int cx, int cy, int radius)
{
    const uint16_t ink = botcorps_seat_ink(seat);
    if (seat == BOTCORPS_SEAT_BLUETOOTH) {
        draw_bt_icon(d, cx, cy, radius, ink);
    } else {
        draw_wifi_icon(d, cx, cy, radius, ink);
    }
}

static void header_flakes(lgfx::LGFX_Device &d, int w)
{
    uint32_t s = 0x51A7u;
    for (int y = 4; y < 18; y += 3) {
        int x = 4;
        while (x < w - 4) {
            s = s * 1664525u + 1013904223u;
            const int len = 2 + (int)((s >> 8) & 5u);
            d.drawFastHLine(x, y, len, (s & 8u) ? COL_SCALE_HI : COL_SCALE_LO);
            x += len + 2 + (int)((s >> 16) & 3u);
        }
    }
}

static void draw_grid(int selected)
{
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    auto &d = botcorps_menu();
    const int w = d.width();
    const int h = d.height();
    begin_text(d);
    d.clearClipRect();
    d.fillScreen(COL_PLATE);
    header_flakes(d, w);
    d.drawFastHLine(8, 20, w - 16, COL_RECESS);
    d.drawFastHLine(8, 21, w - 16, COL_SEAM_HI);
    print_ltr(d, 8, 6, "READY", COL_STENCIL, COL_PLATE);
    const int room_w = d.textWidth("ROOM");
    print_ltr(d, w - 8 - room_w, 6, "ROOM", COL_STENCIL_DIM, COL_PLATE);

    const int margin = 8;
    const int gap = 8;
    const int card_w = (w - margin * 2 - gap) / BOTCORPS_SEAT_COUNT;
    const int card_y = 26;
    const int card_h = h - card_y - 16;

    for (int i = 0; i < BOTCORPS_SEAT_COUNT; ++i) {
        const int x = margin + i * (card_w + gap);
        /* Body color is always the seat's own color, selected or not. */
        d.fillRect(x, card_y, card_w, card_h, SEATS[i].body);
        const uint16_t frame = (i == selected) ? COL_SEAM_HI : COL_RECESS;
        const int thick = (i == selected) ? 2 : 1;
        for (int t = 0; t < thick; ++t) {
            d.drawRect(x + t, card_y + t, card_w - 2 * t, card_h - 2 * t, frame);
        }
        const int cx = x + card_w / 2;
        const int cy = card_y + 28;
        if (i == BOTCORPS_SEAT_BLUETOOTH) {
            draw_bt_icon(d, cx, cy, 16, SEATS[i].ink);
        } else {
            draw_wifi_icon(d, cx, cy, 18, SEATS[i].ink);
        }
        d.setTextSize(1);
        print_centered_ltr(d, x + 4, card_w - 8, card_y + card_h - 18,
                           SEATS[i].name, SEATS[i].ink, SEATS[i].body);
    }

    d.setTextSize(1);
    print_ltr(d, 8, h - 12, "` HATCH", COL_STENCIL_DIM, COL_PLATE);
    const int hint_w = d.textWidth(";.,/  ENTER");
    print_ltr(d, w - 8 - hint_w, h - 12, ";.,/  ENTER", COL_STENCIL_DIM, COL_PLATE);
    d.drawRect(0, 0, w, h, COL_RECESS);
}

/* One full-panel card. Called only from the selection-change path. */
static int s_card_seat = 0;
static int s_card_kind = 0; /* 0 grid card, 1 detail card */

static void paint_content_card(void)
{
    auto &d = botcorps_content();
    const int seat = s_card_seat;
    const int w = d.width();
    const int h = d.height();
    begin_text(d);
    d.clearClipRect();
    d.fillScreen(SEATS[seat].body);

    uint32_t s = 0xC0FFu + (uint32_t)seat * 17u;
    for (int y = 2; y < h - 2; y += 4) {
        int x = 2;
        while (x < w - 2) {
            s = s * 1664525u + 1013904223u;
            const int len = 3 + (int)((s >> 8) & 7u);
            const bool margin = (y < 18 || y > h - 28 || x < 14 || x > w - 20);
            if (margin) {
                d.drawFastHLine(x, y, len, SEATS[seat].flake);
            }
            x += len + 3;
        }
    }

    const int cx = w / 2;
    const int cy = h / 2 - 10;
    if (seat == BOTCORPS_SEAT_BLUETOOTH) {
        draw_bt_icon(d, cx, cy, 36, SEATS[seat].ink);
    } else {
        draw_wifi_icon(d, cx, cy, 40, SEATS[seat].ink);
    }

    d.setTextSize(1);
    print_ltr(d, 12, 10, "READY", SEATS[seat].ink, SEATS[seat].body);
    d.setTextSize(3);
    print_centered_ltr(d, 12, w - 24, h - 78, SEATS[seat].name,
                       SEATS[seat].ink, SEATS[seat].body);
    d.setTextSize(1);
    if (s_card_kind == 1) {
        char state[28];
        snprintf(state, sizeof(state), "%s  %s", RADIO_STATE, SEATS[seat].band);
        print_centered_ltr(d, 12, w - 24, h - 28, state,
                           SEATS[seat].ink, SEATS[seat].body);
    } else {
        print_centered_ltr(d, 12, w - 24, h - 28, "SELECTED",
                           SEATS[seat].ink, SEATS[seat].body);
    }
    d.drawRect(0, 0, w, h, SEATS[seat].ink);
    d.drawRect(3, 3, w - 6, h - 6, SEATS[seat].flake);

    Serial.printf("[botcorps] content card %s %dx%d kind=%d\n",
                  SEATS[seat].name, w, h, s_card_kind);
    if (botcorps_dual_ok() && (w != 320 || h != 240)) {
        Serial.printf("[botcorps] content card geometry %dx%d, want 320x240\n",
                      w, h);
    }
}

static int s_shown_seat = -1;
static int s_shown_kind = -1;

static void show_card(int seat, int kind)
{
    if (!botcorps_dual_ok()) return;
    if (seat == s_shown_seat && kind == s_shown_kind) return;
    s_card_seat = seat;
    s_card_kind = kind;
    s_shown_seat = seat;
    s_shown_kind = kind;
    botcorps_while_content(paint_content_card);
}

struct key_edge_t {
    bool enter;
    bool back;
    int move; /* -1 previous, +1 next, 0 none */
};

static key_edge_t poll_keys(void)
{
    key_edge_t edge = { false, false, 0 };
    M5Cardputer.update();
    auto &kb = M5Cardputer.Keyboard;
    if (!kb.isChange()) return edge;
    if (!kb.isPressed()) return edge;

    if (kb.keysState().enter) {
        edge.enter = true;
        return edge;
    }
    if (kb.isKeyPressed('`') || kb.isKeyPressed((char)0x1B)) {
        edge.back = true;
        return edge;
    }
    /* Right-hand cluster stands in for arrows: ; up, . down, , left, / right. */
    if (kb.isKeyPressed(',') || kb.isKeyPressed(';')) edge.move = -1;
    else if (kb.isKeyPressed('/') || kb.isKeyPressed('.')) edge.move = 1;

    const auto &hid = kb.keysState().hid_keys;
    for (size_t i = 0; i < hid.size(); ++i) {
        const uint8_t h = hid[i];
        if (h == 0x50 || h == 0x52) edge.move = -1; /* left, up */
        else if (h == 0x4F || h == 0x51) edge.move = 1; /* right, down */
    }
    return edge;
}

static void wait_until_keys_up(void)
{
    while (true) {
        M5Cardputer.update();
        if (!M5Cardputer.Keyboard.isPressed()) return;
        delay(20);
    }
}

void botcorps_ready_run(void)
{
    int selected = BOTCORPS_SEAT_WIFI;
    bool grid_dirty = true;
    uint32_t last_input = millis();
    s_shown_seat = -1;
    s_shown_kind = -1;

    Serial.println("[botcorps] ready room — WI-FI, BLUETOOTH");
    wait_until_keys_up();

    while (true) {
        if (grid_dirty) {
            draw_grid(selected);
            grid_dirty = false;
            Serial.printf("[botcorps] ready select %s\n", SEATS[selected].name);
        }
        show_card(selected, 0);

        /* EXT screensaver while the grid stays on MENU. One restore paint after. */
        if (botcorps_dual_ok() &&
            (uint32_t)(millis() - last_input) > BOTCORPS_CONTENT_IDLE_MS) {
            botcorps_content_screensaver(selected);
            last_input = millis();
            s_shown_seat = -1;
            s_shown_kind = -1;
            continue;
        }

        const key_edge_t key = poll_keys();
        if (key.back) {
            Serial.println("[botcorps] ready back to hatch");
            return;
        }
        if (key.enter || key.move != 0) last_input = millis();
        if (key.enter) {
            botcorps_bot_window_run(selected);
            grid_dirty = true;
            s_shown_seat = -1;
            s_shown_kind = -1;
            last_input = millis();
            wait_until_keys_up();
            continue;
        }
        if (key.move != 0) {
            int next = selected + key.move;
            if (next < 0) next = BOTCORPS_SEAT_COUNT - 1;
            if (next >= BOTCORPS_SEAT_COUNT) next = 0;
            if (next != selected) {
                selected = next;
                grid_dirty = true;
            }
        }
        delay(20);
    }
}
