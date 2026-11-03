#pragma once
/*
 * Landscape plate for the CYD livescan UI (portrait 240x320 shadow).
 * Chip slots are copied 1:1 — same pixel size as the CYD portrait canon —
 * then placed on the 320x240 external glass. Included only by the Adv
 * firmware TU and the host renderer, after lv_core.h.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LV_LAND_W 320
#define LV_LAND_H 240
#define LV_PORT_W 240
#define LV_PORT_H 320

struct LvSlot {
    int sx, sy, sw, sh, dx, dy;
    const char *name;
};

enum {
    LV_NAV_LEFT = 1,
    LV_NAV_RIGHT,
    LV_NAV_UP,
    LV_NAV_DOWN,
    LV_NAV_OK,
    LV_NAV_BACK,
    LV_NAV_TAB,
    LV_NAV_CHAR
};

static uint16_t *lv_port_px;
static uint16_t *lv_land_px;
static int lv_focus_i;
static int lv_focus_scr = -1;

static int lv_land_init(void *(*alloc_fn)(size_t))
{
    if (!alloc_fn) alloc_fn = malloc;
    lv_port_px = (uint16_t *)alloc_fn((size_t)LV_PORT_W * LV_PORT_H * sizeof(uint16_t));
    lv_land_px = (uint16_t *)alloc_fn((size_t)LV_LAND_W * LV_LAND_H * sizeof(uint16_t));
    if (!lv_port_px || !lv_land_px) return 0;
    memset(lv_port_px, 0, (size_t)LV_PORT_W * LV_PORT_H * sizeof(uint16_t));
    memset(lv_land_px, 0, (size_t)LV_LAND_W * LV_LAND_H * sizeof(uint16_t));
    return 1;
}

static void lv_land_plot(int x, int y, uint16_t c)
{
    if ((unsigned)x >= LV_PORT_W || (unsigned)y >= LV_PORT_H || !lv_port_px) return;
    lv_port_px[y * LV_PORT_W + x] = c;
}

static void lv_land_blit_port(int x, int y, int w, int h, const uint16_t *px)
{
    if (!px) return;
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) lv_land_plot(x + c, y + r, px[r * w + c]);
}

static void lv_slot_add(LvSlot *o, int *n, int cap, int sx, int sy, int sw, int sh, int dx, int dy, const char *name)
{
    if (*n >= cap || sw <= 0 || sh <= 0) return;
    o[*n] = {sx, sy, sw, sh, dx, dy, name};
    *n += 1;
}

/* Inclusive portrait rect -> slot at a landscape origin. */
static void lv_slot_rect(LvSlot *o, int *n, int cap, int x0, int y0, int x1, int y1, int dx, int dy, const char *name)
{
    lv_slot_add(o, n, cap, x0, y0, x1 - x0 + 1, y1 - y0 + 1, dx, dy, name);
}

/*
 * Placement keeps every copied rectangle at its CYD pixel size.
 * Home is the portrait board reflowed into two rows around GIBSON,
 * with the terminal strip along the bottom. Term spreads the ten
 * hunt chips into a 5x2 grid beside the centre bot. Scan puts the
 * results box to the right of the bot column. Control keeps the
 * centre face and stacks the same chips down the two margins.
 */
static int lv_land_slots(LvSlot *o, int cap)
{
    int n = 0;
    const int s = lv_scr;
    if (s == LV_SC_BOOT) {
        lv_slot_rect(o, &n, cap, 0, 40, 239, 239, 40, 20, "boot-window");
        return n;
    }
    if (s == LV_SC_HOME_INTRO) {
        lv_slot_rect(o, &n, cap, 0, 20, 239, 239, 40, 10, "intro-window");
        return n;
    }
    if (s == LV_SC_TTY) {
        lv_slot_rect(o, &n, cap, 0, 0, 239, 23, 40, 0, "tick");
        lv_slot_rect(o, &n, cap, 0, 24, 239, 173, 40, 28, "tty-body");
        return n;
    }
    if (s == LV_SC_RADAR) {
        /* Top 240 rows of the portrait radar, centred. The bottom 80 rows
         * of the 320-tall city do not fit the glass. */
        lv_slot_rect(o, &n, cap, 0, 0, 239, 239, 40, 0, "radar-window");
        return n;
    }
    if (s == LV_SC_HOME) {
        lv_slot_rect(o, &n, cap, 0, 0, 239, 23, 40, 0, "tick");
        lv_slot_rect(o, &n, cap, 0, 24, 27, 203, 2, 28, "rail-l");
        lv_slot_rect(o, &n, cap, 212, 24, 239, 203, 292, 28, "rail-r");
        lv_slot_rect(o, &n, cap, 31, 52, 74, 112, 53, 28, "boot");       /* 44x61 */
        lv_slot_rect(o, &n, cap, 78, 30, 160, 150, 118, 51, "gibson");   /* 83x121 */
        lv_slot_rect(o, &n, cap, 165, 51, 208, 110, 221, 28, "control"); /* 44x60 */
        lv_slot_rect(o, &n, cap, 33, 156, 101, 255, 41, 96, "scan");     /* 69x100 */
        lv_slot_rect(o, &n, cap, 139, 156, 207, 255, 209, 96, "terminal"); /* 69x100 */
        lv_slot_rect(o, &n, cap, 34, 272, 204, 305, 74, 202, "tbox");    /* 171x34 */
        return n;
    }
    if (s == LV_SC_TERM) {
        lv_slot_rect(o, &n, cap, 0, 0, 239, 23, 40, 0, "tick");
        lv_slot_rect(o, &n, cap, 75, 30, 165, 150, 6, 30, "terminalbot"); /* 91x121 */
        static const int HOT[10][4] = {
            {35, 30, 70, 80}, {170, 30, 205, 80}, {35, 87, 70, 137}, {170, 87, 205, 137}, {35, 145, 70, 195},
            {170, 145, 205, 195}, {35, 200, 70, 250}, {80, 200, 115, 250}, {124, 200, 160, 250}, {170, 200, 205, 250}};
        static const char *const HN[10] = {"deauth", "meta", "hack", "skimmer", "flock", "flipper", "drone", "airtag", "keyfob", "samsung"};
        for (int i = 0; i < 10; ++i) {
            const int col = i % 5, row = i / 5;
            lv_slot_rect(o, &n, cap, HOT[i][0], HOT[i][1], HOT[i][2], HOT[i][3], 108 + col * 42, 32 + row * 54, HN[i]);
        }
        lv_slot_rect(o, &n, cap, 34, 272, 204, 305, 6, 168, "tbox");
        lv_slot_rect(o, &n, cap, 221, 301, 239, 319, 299, 219, "back");
        return n;
    }
    if (s == LV_SC_SCAN) {
        lv_slot_rect(o, &n, cap, 0, 0, 239, 23, 40, 0, "tick");
        lv_slot_rect(o, &n, cap, 32, 30, 74, 92, 4, 28, "wifi");     /* 43x63 */
        lv_slot_rect(o, &n, cap, 32, 93, 74, 152, 4, 96, "ble");     /* 43x60 */
        lv_slot_rect(o, &n, cap, 78, 30, 165, 150, 52, 28, "scanbot"); /* 88x121 */
        lv_slot_rect(o, &n, cap, 166, 58, 208, 122, 146, 28, "gib"); /* 43x65 */
        lv_slot_rect(o, &n, cap, 33, 153, 206, 256, 146, 100, "sbox"); /* 174x104 */
        lv_slot_rect(o, &n, cap, 32, 275, 206, 308, 4, 206, "tbox"); /* 175x34 */
        lv_slot_rect(o, &n, cap, 221, 301, 239, 319, 299, 206, "back");
        return n;
    }
    if (s == LV_SC_CTL) {
        lv_slot_rect(o, &n, cap, 0, 0, 239, 23, 40, 0, "tick");
        lv_slot_rect(o, &n, cap, 35, 33, 113, 48, 4, 26, "theme");
        lv_slot_rect(o, &n, cap, 35, 49, 113, 63, 4, 42, "uptime");
        lv_slot_rect(o, &n, cap, 35, 64, 113, 79, 4, 57, "focus");
        lv_slot_rect(o, &n, cap, 35, 85, 70, 140, 4, 76, "mix");
        lv_slot_rect(o, &n, cap, 35, 145, 70, 190, 4, 134, "raincol");
        lv_slot_rect(o, &n, cap, 35, 195, 70, 250, 4, 182, "mashup");
        lv_slot_rect(o, &n, cap, 75, 82, 164, 205, 96, 28, "face");
        lv_slot_rect(o, &n, cap, 90, 212, 159, 258, 106, 156, "orient");
        lv_slot_rect(o, &n, cap, 165, 80, 205, 130, 236, 28, "rain");
        lv_slot_rect(o, &n, cap, 170, 131, 205, 170, 236, 84, "rainmode");
        lv_slot_rect(o, &n, cap, 165, 175, 207, 237, 236, 128, "multi");
        lv_slot_rect(o, &n, cap, 160, 238, 208, 258, 236, 194, "motion");
        lv_slot_rect(o, &n, cap, 34, 272, 204, 305, 48, 206, "tbox");
        lv_slot_rect(o, &n, cap, 221, 301, 239, 319, 299, 206, "back");
        return n;
    }
    return n;
}

static void lv_land_pxput(int x, int y, uint16_t c)
{
    if ((unsigned)x >= LV_LAND_W || (unsigned)y >= LV_LAND_H || !lv_land_px) return;
    lv_land_px[y * LV_LAND_W + x] = c;
}

static void lv_land_orth(int x0, int y0, int x1, int y1, uint16_t c)
{
    int x = x0, y = y0;
    const int sx = x1 >= x0 ? 1 : -1, sy = y1 >= y0 ? 1 : -1;
    while (x != x1) { lv_land_pxput(x, y, c); x += sx; }
    while (y != y1) { lv_land_pxput(x, y, c); y += sy; }
    lv_land_pxput(x1, y1, c);
}

static int lv_land_center(const LvSlot *o, int n, const char *name, int *x, int *y)
{
    for (int i = 0; i < n; ++i) if (o[i].name && !strcmp(o[i].name, name)) {
        *x = o[i].dx + o[i].sw / 2;
        *y = o[i].dy + o[i].sh / 2;
        return 1;
    }
    return 0;
}

static void lv_land_trace(const LvSlot *o, int n, const char *a, const char *b, uint16_t c)
{
    int x0, y0, x1, y1;
    if (!lv_land_center(o, n, a, &x0, &y0) || !lv_land_center(o, n, b, &x1, &y1)) return;
    lv_land_orth(x0, y0, x1, y1, c);
}

static void lv_land_copy_slot(const LvSlot &s)
{
    for (int y = 0; y < s.sh; ++y)
        for (int x = 0; x < s.sw; ++x) {
            const int sx = s.sx + x, sy = s.sy + y, dx = s.dx + x, dy = s.dy + y;
            if ((unsigned)sx >= LV_PORT_W || (unsigned)sy >= LV_PORT_H) continue;
            if ((unsigned)dx >= LV_LAND_W || (unsigned)dy >= LV_LAND_H) continue;
            lv_land_px[dy * LV_LAND_W + dx] = lv_port_px[sy * LV_PORT_W + sx];
        }
}

static void lv_land_extend_tick(const LvSlot *o, int n)
{
    /* The portrait ticker is 240 wide. Side margins keep the hazard stripes
     * (top and bottom 4 rows) and a solid bar fill. They do not repeat the
     * scrolling glyphs. */
    const LvSlot *t = 0;
    for (int i = 0; i < n; ++i) if (o[i].name && !strcmp(o[i].name, "tick")) t = &o[i];
    if (!t || t->sh < 8) return;
    const uint16_t bg = lv_hex(0x2a0400);
    for (int y = 0; y < t->sh && t->dy + y < LV_LAND_H; ++y) {
        const bool stripe = y <= 3 || y >= t->sh - 4;
        uint16_t *row = lv_land_px + (t->dy + y) * LV_LAND_W;
        for (int x = 0; x < t->dx; ++x)
            row[x] = stripe ? row[t->dx + (x & 7)] : bg;
        for (int x = t->dx + t->sw; x < LV_LAND_W; ++x)
            row[x] = stripe ? row[t->dx + ((x - t->dx) & 7)] : bg;
    }
}

static int lv_adv_fill_targets(LvRect *tg, int cap);

static void lv_land_ring(const LvSlot *o, int n)
{
    if (lv_scr == LV_SC_TTY || lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) return;
    LvRect tg[48];
    const int nt = lv_adv_fill_targets(tg, 48);
    if (nt <= 0) return;
    if (lv_focus_scr != lv_scr) { lv_focus_scr = lv_scr; lv_focus_i = 0; }
    if (lv_focus_i >= nt) lv_focus_i = 0;
    const LvRect &r = tg[lv_focus_i];
    const int cx = (r.x0 + r.x1) / 2, cy = (r.y0 + r.y1) / 2;
    const LvSlot *host = 0;
    for (int i = 0; i < n; ++i) {
        if (cx < o[i].sx || cy < o[i].sy || cx >= o[i].sx + o[i].sw || cy >= o[i].sy + o[i].sh) continue;
        host = &o[i];
    }
    if (!host) return;
    const int x0 = host->dx + (r.x0 - host->sx) - 1;
    const int y0 = host->dy + (r.y0 - host->sy) - 1;
    const int x1 = host->dx + (r.x1 - host->sx) + 1;
    const int y1 = host->dy + (r.y1 - host->sy) + 1;
    const uint16_t c = lv_hex(LV_FRAME[lv_scr == LV_SC_CTL ? 0 : lv_theme]);
    for (int x = x0; x <= x1; ++x) { lv_land_pxput(x, y0, c); lv_land_pxput(x, y1, c); }
    for (int y = y0; y <= y1; ++y) { lv_land_pxput(x0, y, c); lv_land_pxput(x1, y, c); }
}

static void lv_land_compose(int ring)
{
    if (!lv_port_px || !lv_land_px) return;
    memset(lv_land_px, 0, (size_t)LV_LAND_W * LV_LAND_H * sizeof(uint16_t));
    LvSlot slots[24];
    const int n = lv_land_slots(slots, 24);
    const uint16_t trace = lv_dim(lv_hex(LV_ACC[lv_scr == LV_SC_CTL ? 0 : lv_theme]), 2, 3);
    if (lv_scr == LV_SC_HOME) {
        lv_land_trace(slots, n, "boot", "gibson", trace);
        lv_land_trace(slots, n, "gibson", "control", trace);
        lv_land_trace(slots, n, "boot", "scan", trace);
        lv_land_trace(slots, n, "control", "terminal", trace);
        lv_land_trace(slots, n, "scan", "tbox", trace);
        lv_land_trace(slots, n, "terminal", "tbox", trace);
        lv_land_trace(slots, n, "gibson", "tbox", trace);
    } else if (lv_scr == LV_SC_SCAN) {
        lv_land_trace(slots, n, "wifi", "scanbot", trace);
        lv_land_trace(slots, n, "ble", "scanbot", trace);
        lv_land_trace(slots, n, "scanbot", "gib", trace);
        lv_land_trace(slots, n, "scanbot", "sbox", trace);
        lv_land_trace(slots, n, "sbox", "tbox", trace);
    } else if (lv_scr == LV_SC_TERM) {
        lv_land_trace(slots, n, "terminalbot", "deauth", trace);
        lv_land_trace(slots, n, "terminalbot", "samsung", trace);
        lv_land_trace(slots, n, "terminalbot", "tbox", trace);
    } else if (lv_scr == LV_SC_CTL) {
        lv_land_trace(slots, n, "face", "mix", trace);
        lv_land_trace(slots, n, "face", "rain", trace);
        lv_land_trace(slots, n, "face", "tbox", trace);
    }
    for (int i = 0; i < n; ++i) lv_land_copy_slot(slots[i]);
    lv_land_extend_tick(slots, n);
    if (ring) lv_land_ring(slots, n);
}

static int lv_adv_push_target(LvRect *tg, int *n, int cap, LvRect r)
{
    if (*n >= cap) return 0;
    tg[(*n)++] = r;
    return 1;
}

static int lv_adv_fill_targets(LvRect *tg, int cap)
{
    int n = 0;
    if (lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) {
        lv_adv_push_target(tg, &n, cap, {80, 60, 160, 180});
        return n;
    }
    if (lv_scr == LV_SC_HOME) {
        for (const LvRect &r : LV_PAD_HOME) lv_adv_push_target(tg, &n, cap, r);
        return n;
    }
    if (lv_scr == LV_SC_TERM) {
        for (const LvRect &r : LV_PAD_TERM) lv_adv_push_target(tg, &n, cap, r);
        lv_adv_push_target(tg, &n, cap, LV_R_BACKBOT);
        return n;
    }
    if (lv_scr == LV_SC_CTL) {
        for (const LvRect &r : LV_PAD_CTL) lv_adv_push_target(tg, &n, cap, r);
        lv_adv_push_target(tg, &n, cap, LV_R_BACKBOT);
        return n;
    }
    if (lv_scr == LV_SC_SCAN) {
        static const LvRect CHIP[] = {{32, 30, 74, 92}, {32, 93, 74, 152}, {78, 30, 165, 150}, {166, 58, 208, 122},
                                      {174, 165, 203, 206}, {174, 210, 203, 252}};
        for (const LvRect &r : CHIP) lv_adv_push_target(tg, &n, cap, r);
        if (lv_cat < 0) {
            for (int t = 0; t < 12; ++t)
                lv_adv_push_target(tg, &n, cap, {(int16_t)(lv_tile_x(t) - 1), (int16_t)(lv_tile_y(t) - 1),
                                                (int16_t)(lv_tile_x(t) + 31), (int16_t)(lv_tile_y(t) + 28)});
        }
        lv_adv_push_target(tg, &n, cap, {28, 265, 90, 298});
        lv_adv_push_target(tg, &n, cap, {91, 265, 148, 298});
        lv_adv_push_target(tg, &n, cap, {149, 265, 208, 298});
        lv_adv_push_target(tg, &n, cap, {28, 299, 208, 316});
        lv_adv_push_target(tg, &n, cap, LV_R_BACKBOT);
        return n;
    }
    if (lv_scr == LV_SC_RADAR) {
        lv_adv_push_target(tg, &n, cap, {0, 0, 239, 23});
        lv_adv_push_target(tg, &n, cap, LV_R_BACKBOT);
        return n;
    }
    return n;
}

static void lv_adv_scroll(int dir)
{
    const int total = lv_tn < LV_TLINES ? lv_tn : LV_TLINES;
    const int mx = total > LV_TTY_LINES ? total - LV_TTY_LINES : 0;
    lv_scroll += dir > 0 ? 6 : -6;
    if (lv_scroll > mx) lv_scroll = mx;
    if (lv_scroll < 0) lv_scroll = 0;
    lv_dirty_tty = true;
}

static void lv_adv_submit(void)
{
    char c[64];
    snprintf(c, sizeof c, "%s", lv_inb);
    lv_inlen = 0;
    lv_inb[0] = 0;
    if (c[0]) lv_cmd(c);
    lv_dirty_tty = true;
}

static void lv_adv_type_char(int ch)
{
    if (lv_scr != LV_SC_TTY) lv_open_tty(0);
    if (ch == 8 || ch == 127) {
        if (lv_inlen) lv_inb[--lv_inlen] = 0;
    } else if (ch == '\n' || ch == '\r') {
        lv_adv_submit();
        return;
    } else if (ch >= 32 && ch < 127 && lv_inlen < 62) {
        lv_inb[lv_inlen++] = (char)ch;
        lv_inb[lv_inlen] = 0;
    }
    lv_dirty_tty = true;
}

static const char *lv_land_nav(int op, int ch)
{
    if (lv_focus_scr != lv_scr) { lv_focus_scr = lv_scr; lv_focus_i = 0; }
    if (op == LV_NAV_CHAR) { lv_adv_type_char(ch); return "type"; }
    if (op == LV_NAV_TAB) { lv_open_tty("help"); return "help"; }
    if (lv_scr == LV_SC_TTY) {
        if (op == LV_NAV_UP) { lv_adv_scroll(1); return "scroll-up"; }
        if (op == LV_NAV_DOWN) { lv_adv_scroll(-1); return "scroll-down"; }
        if (op == LV_NAV_OK) { lv_adv_submit(); return "enter"; }
        if (op == LV_NAV_BACK) {
            if (lv_inlen) { lv_inb[--lv_inlen] = 0; lv_dirty_tty = true; return "bs"; }
            return lv_back();
        }
        return "tty-none";
    }
    if (op == LV_NAV_BACK) return lv_back();
    LvRect tg[48];
    const int n = lv_adv_fill_targets(tg, 48);
    if (n <= 0) return "no-target";
    if (lv_focus_i >= n) lv_focus_i = 0;
    if (op == LV_NAV_OK || lv_scr == LV_SC_BOOT || lv_scr == LV_SC_HOME_INTRO) {
        const LvRect &r = tg[lv_focus_i];
        return lv_tap((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2);
    }
    int dx = 0, dy = 0;
    if (op == LV_NAV_LEFT) dx = -1;
    else if (op == LV_NAV_RIGHT) dx = 1;
    else if (op == LV_NAV_UP) dy = -1;
    else if (op == LV_NAV_DOWN) dy = 1;
    else return "nav-none";
    const LvRect &c = tg[lv_focus_i];
    const int cx = (c.x0 + c.x1) / 2, cy = (c.y0 + c.y1) / 2;
    int best = -1;
    long bd = 0;
    for (int i = 0; i < n; ++i) {
        if (i == lv_focus_i) continue;
        const int x = (tg[i].x0 + tg[i].x1) / 2, y = (tg[i].y0 + tg[i].y1) / 2;
        const int vx = x - cx, vy = y - cy;
        if (dx && vx * dx <= 0) continue;
        if (dy && vy * dy <= 0) continue;
        if (dx && abs(vy) > abs(vx) * 2 + 8) continue;
        if (dy && abs(vx) > abs(vy) * 2 + 8) continue;
        const long d = (long)vx * vx + (long)vy * vy;
        if (best < 0 || d < bd) { best = i; bd = d; }
    }
    if (best >= 0) lv_focus_i = best;
    return best >= 0 ? "focus" : "focus-stay";
}

static int lv_land_focus(void) { return lv_focus_i; }
