/* v1.2.9: v3 Joey Console full-width GIBSON controls deck.
 * Frame (1,247)-(238,319) — left margin 1, matching right margin on a 240-wide screen.
 * No power beam. D-pad is LEFT/RIGHT only (horizontal rocker in the octagon gate).
 * L/R cycles towers, A locks, B unlocks or goes back.
 * Center stack is HUNT, then IGNORE, then WATCH; dimmed unless locked. */
#pragma once

#ifndef LV_RD_PAD
#define LV_RD_PAD 1
#endif

static int lv_rd_pad_held = 0;   /* 0 none, 1 L, 2 R, 5 A, 6 B, 7 IGNORE, 8 WATCH, 9 HUNT */

static int  lv_rd_cycle_next(void);
static int  lv_rd_cycle_prev(void);
static void lv_rd_select(int i);
static void lv_rd_lock_on(int i);
static void lv_rd_release(void);

#define PAD_X0 1
#define PAD_Y0 247
#define PAD_X1 238
#define PAD_Y1 319
#define PAD_CX 50
#define PAD_CY 292

/* v1.2.9: rocker (octagon gate) gets the biggest width share, larger than the center stack.
 * A/B stay about the v1.2.8 size (r=20, hits 40x40). */
static const LvRect PAD_HIT_L = {  6, 268, 49, 316 };   /* 44 x 49 */
static const LvRect PAD_HIT_R = { 51, 268, 94, 316 };   /* 44 x 49 */
static const LvRect PAD_HIT_B = {152, 272, 191, 311 };  /* 40 x 40 */
static const LvRect PAD_HIT_A = {196, 272, 235, 311 };  /* 40 x 40 */
/* HUNT on top, then IGNORE, then WATCH — narrower so the rocker can own the width. */
static const LvRect PAD_HIT_HNT = {102, 268, 147, 283 };   /* 46 x 16 */
static const LvRect PAD_HIT_IGN = {102, 284, 147, 299 };   /* 46 x 16 */
static const LvRect PAD_HIT_WAT = {102, 300, 147, 316 };   /* 46 x 17 */
#define PAD_MID_X  106
#define PAD_MID_W  38
#define PAD_MID_H  11
#define PAD_MID_YH 270
#define PAD_MID_YI 287
#define PAD_MID_YW 304

static int lv_rd_pad_hit_at(int x, int y)
{
    if (y < PAD_Y0 || y > PAD_Y1 || x < PAD_X0 || x > PAD_X1) return 0;
    if (lv_in(PAD_HIT_L, x, y)) return 1;
    if (lv_in(PAD_HIT_R, x, y)) return 2;
    if (lv_in(PAD_HIT_B, x, y)) return 6;
    if (lv_in(PAD_HIT_A, x, y)) return 5;
    if (lv_in(PAD_HIT_IGN, x, y)) return 7;
    if (lv_in(PAD_HIT_WAT, x, y)) return 8;
    if (lv_in(PAD_HIT_HNT, x, y)) return 9;
    return 0;
}

static int lv_rd_cycle_prev(void)
{
    int ord[LV_MAXF], n = 0;
    for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) ord[n++] = i;
    for (int a = 1; a < n; ++a) { const int t = ord[a]; int b = a - 1; while (b >= 0 && lv_f[ord[b]].rssi < lv_f[t].rssi) { ord[b + 1] = ord[b]; --b; } ord[b + 1] = t; }
    if (!n) return -1;
    int p = -1; for (int k = 0; k < n; ++k) if (ord[k] == lv_rd_sel) p = k;
    return ord[(p - 1 + n) % n];
}

static const char *lv_rd_pad_go(int id)
{
    if (id == 1 || id == 2) {
        const int nx = (id == 2) ? lv_rd_cycle_next() : lv_rd_cycle_prev();
        if (nx < 0) return "pad-empty";
        lv_rd_select(nx); lv_rd_glow = true; lv_rd_glow_t = lv_now; return id == 2 ? "pad-right" : "pad-left";
    }
    if (id == 5) {
        if (lv_rd_lock) return "pad-a-locked";
        int i = lv_rd_sel;
        if (i < 0 || i >= lv_nf) { i = lv_rd_cycle_next(); if (i < 0) return "pad-a-empty"; lv_rd_select(i); }
        lv_rd_lock_on(i); return "pad-a-lock";
    }
    if (id == 6) {
        if (lv_rd_lock) { lv_rd_release(); lv_rd_log("RELEASED"); return "pad-b-unlock"; }
        lv_go(lv_rd_from == LV_SC_RADAR || lv_rd_from == LV_SC_BOOT || lv_rd_from == LV_SC_HOME_INTRO || lv_rd_from == LV_SC_CAL ? LV_SC_HOME : lv_rd_from);
        return "pad-b-back";
    }
    if (id == 7 || id == 8 || id == 9) {
        if (!lv_rd_lock) return "pad-mid-idle";
        return id == 7 ? "pad-ignore" : id == 8 ? "pad-watch" : "pad-hunt";
    }
    return "pad-miss";
}

static void pad_via(LvBand &B, int x, int y, uint16_t ink)
{
    const uint16_t d = lv_dim(ink, 5, 8);
    b_pix(B, x, y, d); b_pix(B, x + 1, y, ink);
    b_pix(B, x, y + 1, ink); b_pix(B, x + 1, y + 1, lv_hex(0x000000));
}

static void pad_vias_frame(LvBand &B, uint16_t ink)
{
    for (int i = 0; i < 11; ++i) pad_via(B, 8 + i * 21, 248, ink);
    pad_via(B, 3, 248, ink); pad_via(B, 233, 248, ink);
    pad_via(B, 3, 316, ink); pad_via(B, 233, 316, ink);
}

static void pad_bevel_rect(LvBand &B, int x, int y, int w, int h, uint16_t fill, uint16_t ink, int pressed)
{
    const uint16_t hi = pressed ? lv_dim(fill, 2, 8) : lv_mix(ink, lv_hex(0xffffff), 6, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 4, 8) : lv_dim(fill, 1, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 4, 8) : fill;
    b_fill(B, x, y, w, h, body);
    b_fill(B, x, y, w, 1, hi); b_fill(B, x, y, 1, h, hi);
    b_fill(B, x, y + h - 1, w, 1, lo); b_fill(B, x + w - 1, y, 1, h, lo);
    b_frame(B, x, y, w, h, pressed ? lv_hi(ink) : ink);
}

static void pad_disc(LvBand &B, int cx, int cy, int r, uint16_t fill, uint16_t ink, int pressed)
{
    const uint16_t hi = pressed ? lv_dim(fill, 3, 8) : lv_mix(ink, lv_hex(0xffffff), 7, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 3, 8) : lv_dim(ink, 3, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 5, 8) : fill;
    const int r2 = r * r, rin = (r - 1) * (r - 1);
    for (int dy = -r; dy <= r; ++dy) for (int dx = -r; dx <= r; ++dx) {
        const int d2 = dx * dx + dy * dy; if (d2 > r2) continue;
        uint16_t c = body;
        if (d2 > rin) c = (dx + dy < 0) ? hi : lo;
        else if (dx * 2 + dy * 3 < -r) c = lv_mix(body, hi, 3, 8);
        else if (dx + dy > r / 2) c = lv_mix(body, lo, 2, 8);
        if (pressed && d2 <= rin) c = lv_mix(c, ink, 3, 8);
        b_pix(B, cx + dx, cy + dy, c);
    }
}

static void pad_glow(LvBand &B, int cx, int cy, int r, uint16_t c, int k)
{
    const int r2 = (r + 1) * (r + 1), r0 = (r - 1) * (r - 1);
    for (int dy = -r - 1; dy <= r + 1; ++dy) for (int dx = -r - 1; dx <= r + 1; ++dx) {
        const int d2 = dx * dx + dy * dy;
        if (d2 > r2 || d2 < r0) continue;
        b_blend(B, cx + dx, cy + dy, c, k);
    }
}

static void pad_octagon(LvBand &B, int cx, int cy, int rx, int ry, uint16_t fill, uint16_t ink, int pressed)
{
    const uint16_t hi = pressed ? lv_dim(fill, 3, 8) : lv_mix(ink, lv_hex(0xffffff), 6, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 3, 8) : lv_dim(ink, 3, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 5, 8) : fill;
    const int lim = 5 * rx * ry;   /* |dx|/rx + |dy|/ry <= 5/3  (same corner cut as the old equal-r octagon) */
    for (int dy = -ry; dy <= ry; ++dy) for (int dx = -rx; dx <= rx; ++dx) {
        const int s = 3 * (abs(dx) * ry + abs(dy) * rx);
        if (s > lim) continue;
        uint16_t c = body;
        const int edge = (abs(dx) == rx || abs(dy) == ry || s >= lim - (3 * (ry + rx)));
        if (edge) c = (dx + dy < 0) ? hi : lo;
        else if (dx * 2 + dy * 3 < -ry) c = lv_mix(body, hi, 3, 8);
        if (pressed) c = lv_mix(c, ink, 3, 8);
        b_pix(B, cx + dx, cy + dy, c);
    }
}

/* 1-2 px halo around a wide octagon gate. */
static void pad_glow_oct(LvBand &B, int cx, int cy, int rx, int ry, uint16_t c, int k)
{
    const int lim0 = 5 * rx * ry, lim1 = 5 * (rx + 1) * (ry + 1);
    for (int dy = -ry - 1; dy <= ry + 1; ++dy) for (int dx = -rx - 1; dx <= rx + 1; ++dx) {
        const int adx = abs(dx), ady = abs(dy);
        const int s = 3 * (adx * (ry + 1) + ady * (rx + 1));
        if (adx > rx + 1 || ady > ry + 1 || s > lim1) continue;
        const int inner = adx <= rx && ady <= ry && 3 * (adx * ry + ady * rx) <= lim0;
        if (!inner) b_blend(B, cx + dx, cy + dy, c, k);
    }
}

static void pad_arrow(LvBand &B, int cx, int cy, int dir, int s, uint16_t c)
{
    /* Bigger L/R chevron: height 2*s-1, width s. dir 1 = left, 2 = right. */
    for (int i = 0; i < s; ++i) {
        const int x = (dir == 1) ? (cx - s / 2 + i) : (cx + s / 2 - i);
        b_fill(B, x, cy - i, 1, 2 * i + 1, c);
    }
}

/* Horizontal rocker only, inside the octagon gate. No up/down arms. */
static void pad_rocker(LvBand &B, int cx, int cy, int aw, int al, uint16_t fill, uint16_t ink)
{
    pad_bevel_rect(B, cx - al / 2, cy - aw / 2, al, aw, fill, ink, 0);
    if (lv_rd_pad_held == 1) pad_bevel_rect(B, cx - al / 2, cy - aw / 2, al / 2, aw, fill, ink, 1);
    if (lv_rd_pad_held == 2) pad_bevel_rect(B, cx + 1, cy - aw / 2, al / 2, aw, fill, ink, 1);
    const uint16_t lc = lv_rd_pad_held == 1 ? lv_hex(0xffffff) : ink;
    const uint16_t rc = lv_rd_pad_held == 2 ? lv_hex(0xffffff) : ink;
    pad_arrow(B, cx - al / 2 + 8, cy, 1, 8, lc);
    pad_arrow(B, cx + al / 2 - 8, cy, 2, 8, rc);
}

static void pad_plaque(LvBand &B, uint16_t ink, uint16_t dim)
{
    const int x = 6, y = 249, w = 228, h = 16;   /* 6..233, 5 px inset from the 1..238 frame */
    b_fill(B, x, y, w, h, lv_hex(0x08060a));
    b_fill(B, x, y, w, 1, lv_dim(dim, 2, 8));
    b_fill(B, x, y, 1, h, lv_dim(dim, 2, 8));
    b_fill(B, x, y + h - 1, w, 1, lv_mix(ink, lv_hex(0x000000), 2, 8));
    b_fill(B, x + w - 1, y, 1, h, lv_mix(ink, lv_hex(0x000000), 2, 8));
    b_frame(B, x, y, w, h, dim);
    pad_via(B, x + 2, y + 2, ink);
    pad_via(B, x + w - 4, y + 2, ink);
}

static void pad_status(LvBand &B, int x, int y, uint16_t ink, uint16_t dim)
{
    char a[36], b[28];
    int n = 0; for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) ++n;
    int k = 0; if (lv_rd_sel >= 0) { for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) { ++k; if (i == lv_rd_sel) break; } }
    if (lv_rd_sel < 0 || lv_rd_sel >= lv_nf) {
        b_text35(B, x, y, "L/R CYCLE TOWERS", ink);
        b_text35(B, x + 140, y, "A LOCK  B BACK", dim);
        return;
    }
    const LvFind &f = lv_f[lv_rd_sel];
    char lab[20]; lv_label(f, lab, 16);
    snprintf(a, sizeof a, "%s", lab);
    snprintf(b, sizeof b, "%s %d/%d", lv_rd_lock ? "LOCK" : "SEL", k, n);
    while (lv_w35(a) > 140 && a[0]) a[strlen(a) - 1] = 0;
    b_text35(B, x, y, a, lv_hex(0xffffff));
    b_text35(B, x + 168, y, b, lv_rd_lock ? lv_hex(0xff2ad4) : ink);
}

static void pad_draw_joey(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), ice = lv_hex(0x30e8ff);
    pad_octagon(B, PAD_CX, PAD_CY, 42, 25, lv_hex(0x06040a), dim, 0);
    pad_glow_oct(B, PAD_CX, PAD_CY, 42, 25, ink, 2);
    pad_octagon(B, PAD_CX, PAD_CY, 36, 20, lv_hex(0x100818), ink, 0);
    pad_rocker(B, PAD_CX, PAD_CY, 16, 68, lv_hex(0x1a1018), ink);
    pad_disc(B, PAD_CX, PAD_CY, 3, lv_rd_lock ? ac : ink, ink, 0);
    auto arcade = [&](int cx, int cy, uint16_t col, const char *lab, const char *sub, int held) {
        pad_disc(B, cx, cy, 20, lv_hex(0x08040c), col, 0);
        pad_glow(B, cx, cy, 20, held ? lv_hi(col) : col, held ? 6 : 3);
        pad_disc(B, cx, cy, 15, held ? lv_mix(lv_hex(0x100818), col, 5, 8) : lv_hex(0x120818), col, held);
        b_text35(B, cx - 1, cy - 6, lab, held ? lv_hex(0xffffff) : col);
        b_text35(B, cx - lv_w35(sub) / 2, cy + 3, sub, held ? lv_hex(0xffffff) : lv_dim(col, 6, 8));
    };
    arcade(172, PAD_CY, ice, "B", "BACK", lv_rd_pad_held == 6);
    arcade(216, PAD_CY, ac,  "A", "LOCK", lv_rd_pad_held == 5);
}

static void pad_draw_mid3(LvBand &B, uint16_t ink, uint16_t dim)
{
    (void)ink; (void)dim;
    const char *L[3] = { "HUNT", "IGNORE", "WATCH" };
    const uint16_t live[3] = { lv_hex(0xff2ad4), lv_hex(0xa0a0a8), lv_hex(0xa0fc00) };
    const int ys[3] = { PAD_MID_YH, PAD_MID_YI, PAD_MID_YW };
    const int hid[3] = { 9, 7, 8 };
    const int x = PAD_MID_X, w = PAD_MID_W, h = PAD_MID_H;
    const bool on = lv_rd_lock;
    for (int k = 0; k < 3; ++k) {
        const int y = ys[k];
        const int held = (lv_rd_pad_held == hid[k]);
        const uint16_t acc = on ? live[k] : lv_dim(live[k], 3, 8);
        const uint16_t fill = on
            ? (held ? lv_mix(lv_hex(0x100818), acc, 5, 8) : lv_hex(0x100818))
            : lv_hex(0x0a0810);
        const uint16_t lab = !on ? lv_dim(acc, 4, 8) : (held ? lv_hex(0xffffff) : acc);
        const int ly = y + (h - 5) / 2 + (held ? 1 : 0);
        const int lx = x + (w - lv_w35(L[k])) / 2;
        pad_bevel_rect(B, x, y, w, h, fill, acc, held);
        b_text35(B, lx, ly, L[k], lab);
    }
}

static void lv_rd_pad_draw(LvBand &B, uint16_t hdr)
{
    (void)hdr;
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD);
    b_fill(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, lv_hex(0x000000));
    b_frame(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, ink);
    pad_vias_frame(B, ink);
    pad_plaque(B, ink, dim);
    pad_status(B, 10, 253, ink, dim);
    pad_draw_joey(B, ink, dim);
    pad_draw_mid3(B, ink, dim);
}

static void lv_rd_pad_touch(bool down, int x, int y)
{
    if (!down) { lv_rd_pad_held = 0; return; }
    const int id = lv_rd_pad_hit_at(x, y);
    if (id) lv_rd_pad_held = id;
}

static const char *lv_rd_pad_tap(int x, int y)
{
    const int id = lv_rd_pad_hit_at(x, y);
    if (!id) return "radar-term";
    return lv_rd_pad_go(id);
}
