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

/* v1.2.14 (Mobandy 11:34 AM ET "we need an up and down on the dpad"): the v1.2.11 hackbot_dpad_MOCK cross in the same
 * octagon housing on BOTH decks. Finger zones: the old rocker box x6..94 y268..316 split on its diagonals around the
 * centre (50,292): up / down arms 89 px wide at the outer edge x 24 px deep, left / right 44 px wide x 49 px tall. */
static int lv_pad_dpad_at(int x, int y)   /* 0 none, 1 up, 2 down, 3 left, 4 right */
{
    if (x < PAD_HIT_L.x0 || x > PAD_HIT_R.x1 || y < PAD_HIT_L.y0 || y > PAD_HIT_L.y1) return 0;
    const int dx = x - PAD_CX, dy = y - PAD_CY;
    if (abs(dy) * 45 > abs(dx) * 24) return dy < 0 ? 1 : 2;
    return dx < 0 ? 3 : 4;
}
static int lv_rd_dpad_pick(int cur, int dir);   /* lv_radar.h: 0 L, 1 R (within the row), 2 UP (farther row), 3 DOWN (nearer row) */
static int lv_rd_fsel = 0;   /* v1.2.14: the d-pad's folder cursor (0 HUNT, 1 IGNORE, 2 WATCH) */
static const char *lv_radar_tap(int x, int y);
static int lv_rd_pad_hit_at(int x, int y)
{
    if (y < PAD_Y0 || y > PAD_Y1 || x < PAD_X0 || x > PAD_X1) return 0;
    { const int d = lv_pad_dpad_at(x, y); if (d) return d == 1 ? 11 : d == 2 ? 12 : d == 3 ? 1 : 2; }   /* v1.2.14: 4-way d-pad (11 up, 12 down) */
    if (lv_in(PAD_HIT_B, x, y)) return 6;
    if (lv_in(PAD_HIT_A, x, y)) return 5;
#if LV_RD_FOLD
    if (lv_scr == LV_SC_RADAR && x >= PAD_HIT_HNT.x0 && x <= PAD_HIT_HNT.x1 && y >= PAD_HIT_HNT.y0 && y <= PAD_HIT_WAT.y1) return 10;   /* SPEED: the whole old stack, 46 x 49 */
#endif
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
    if (id == 11 || id == 12) {   /* v1.2.14 UP / DOWN: open tower = folder cursor; else jump a depth row */
        if (lv_rd_lock) {
#if LV_RD_FOLD
            if (lv_rd_fold_mode == 1) { lv_rd_fsel = (lv_rd_fsel + (id == 12 ? 1 : 2)) % 3; return id == 11 ? "pad-up-folder" : "pad-down-folder"; }
#endif
            return "pad-updn-locked";
        }
        const int nx = lv_rd_dpad_pick(lv_rd_sel, id == 11 ? 2 : 3);
        if (nx < 0) return "pad-empty";
        lv_rd_select(nx); lv_rd_glow = true; lv_rd_glow_t = lv_now; return id == 11 ? "pad-up-row" : "pad-down-row";
    }
    if ((id == 1 || id == 2) && !lv_rd_lock) {   /* v1.2.14: L / R move within the selected tower's depth row */
        const int nx = lv_rd_dpad_pick(lv_rd_sel, id == 2 ? 1 : 0);
        if (nx < 0) return "pad-empty";
        lv_rd_select(nx); lv_rd_glow = true; lv_rd_glow_t = lv_now; return id == 2 ? "pad-right" : "pad-left";
    }
    if (id == 1 || id == 2) {
        const int nx = (id == 2) ? lv_rd_cycle_next() : lv_rd_cycle_prev();
        if (nx < 0) return "pad-empty";
        lv_rd_select(nx); lv_rd_glow = true; lv_rd_glow_t = lv_now; return id == 2 ? "pad-right" : "pad-left";
    }
    if (id == 5) {
        if (lv_rd_lock) {
#if LV_RD_FOLD
            if (lv_rd_fold_mode == 1) { const LvRect &r = LV_RD_FTAP[lv_rd_fsel]; return lv_radar_tap((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2); }   /* A = a tap on the selected folder (same action + press flash) */
#endif
            return "pad-a-locked";
        }
        int i = lv_rd_sel;
        if (i < 0 || i >= lv_nf) { i = lv_rd_cycle_next(); if (i < 0) return "pad-a-empty"; lv_rd_select(i); }
        lv_rd_lock_on(i); return "pad-a-lock";
    }
    if (id == 6) {
        if (lv_rd_lock) { lv_rd_release(); lv_rd_log("RELEASED"); return "pad-b-unlock"; }
        lv_go(lv_rd_from == LV_SC_RADAR || lv_rd_from == LV_SC_BOOT || lv_rd_from == LV_SC_HOME_INTRO || lv_rd_from == LV_SC_CAL ? LV_SC_HOME : lv_rd_from);
        return "pad-b-back";
    }
#if LV_RD_FOLD
    if (id == 10) { lv_rd_speed = lv_rd_speed % 3 + 1; lv_rd_log("SPEED %dX", lv_rd_speed); static const char *const S[3] = {"pad-speed-1x", "pad-speed-2x", "pad-speed-3x"}; return S[lv_rd_speed - 1]; }
#endif
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
    for (int dy = b_ylo(B, cy - r) - cy; dy <= b_yhi(B, cy + r) - cy; ++dy) for (int dx = -r; dx <= r; ++dx) {   /* v1.2.11: band rows only */
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
    for (int dy = b_ylo(B, cy - r - 1) - cy; dy <= b_yhi(B, cy + r + 1) - cy; ++dy) for (int dx = -r - 1; dx <= r + 1; ++dx) {   /* v1.2.11: band rows only */
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
    for (int dy = b_ylo(B, cy - ry) - cy; dy <= b_yhi(B, cy + ry) - cy; ++dy) for (int dx = -rx; dx <= rx; ++dx) {   /* v1.2.11: band rows only */
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
    for (int dy = b_ylo(B, cy - ry - 1) - cy; dy <= b_yhi(B, cy + ry + 1) - cy; ++dy) for (int dx = -rx - 1; dx <= rx + 1; ++dx) {   /* v1.2.11: band rows only */
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

/* v1.2.14: the 4-way cross, cloned verbatim from the v1.2.11 hackbot_dpad_MOCK (3 x 3 cells of 15 px); H 1 up 2 down 3 left
 * 4 right (held arm pressed + white arrow). ctr = the centre pip ink (mock: ink; GIBSON keeps its magenta-when-locked pip). */
static void pad_cross(LvBand &B, int cx, int cy, int H, uint16_t ink, uint16_t ctr)
{
    const int a = 15;
    pad_bevel_rect(B, cx - a / 2 - a, cy - a / 2, 3 * a, a, lv_hex(0x1a1018), ink, 0);          /* horizontal bar */
    pad_bevel_rect(B, cx - a / 2, cy - a / 2 - a + 3, a, 3 * a - 6, lv_hex(0x1a1018), ink, 0);  /* vertical bar */
    if (H >= 1 && H <= 4) { const int ox = H == 3 ? -a : H == 4 ? a : 0, oy = H == 1 ? -a + 3 : H == 2 ? a - 3 : 0;
        pad_bevel_rect(B, cx - a / 2 + ox, cy - a / 2 + oy, a, a, lv_mix(lv_hex(0x1a1018), ink, 5, 8), ink, 1); }
    b_fill(B, cx - 4, cy - 4, 9, 9, lv_hex(0x1a1018));
    pad_disc(B, cx, cy, 3, ctr, ink, 0);
    pad_arrow(B, cx - a - 1, cy, 1, 6, H == 3 ? lv_hex(0xffffff) : ink);
    pad_arrow(B, cx + a + 1, cy, 2, 6, H == 4 ? lv_hex(0xffffff) : ink);
    for (int r = 0; r < 4; ++r) { b_fill(B, cx - r, cy - a - 4 + r + 3, 2 * r + 1, 1, H == 1 ? lv_hex(0xffffff) : ink); b_fill(B, cx - r, cy + a + 4 - r - 3, 2 * r + 1, 1, H == 2 ? lv_hex(0xffffff) : ink); }
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
    { const int h = lv_rd_pad_held; pad_cross(B, PAD_CX, PAD_CY, h == 11 ? 1 : h == 12 ? 2 : h == 1 ? 3 : h == 2 ? 4 : 0, ink, lv_rd_lock ? ac : ink); }   /* v1.2.14: was pad_rocker (kept) */
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

#if LV_RD_FOLD
static void pad_draw_speed(LvBand &B)   /* v1.2.11: one SPEED button in the old centre stack's frame (x106 w38, y270..314) */
{
    const int x = PAD_MID_X, w = PAD_MID_W, y = PAD_MID_YH, h = PAD_MID_YW + PAD_MID_H - PAD_MID_YH;
    const bool held = lv_rd_pad_held == 10; const uint16_t acc = lv_hex(0xffb000);
    pad_bevel_rect(B, x, y, w, h, held ? lv_mix(lv_hex(0x100818), acc, 5, 8) : lv_hex(0x100818), acc, held);
    const uint16_t lab = held ? lv_hex(0xffffff) : acc;
    b_text35(B, x + (w - lv_w35("SPEED")) / 2, y + 5 + (held ? 1 : 0), "SPEED", lab);
    char s[4]; snprintf(s, sizeof s, "%dX", lv_rd_speed); b_text(B, x + (w - 2 * 11 + 1) / 2, y + 15 + (held ? 1 : 0), s, lab, 2);
    for (int k = 0; k < 3; ++k) b_fill(B, x + 8 + k * 9, y + h - 7, 6, 3, k < lv_rd_speed ? acc : lv_dim(acc, 2, 8));   /* 1..3 pips */
}
#endif
static void lv_rd_pad_draw(LvBand &B, uint16_t hdr)
{
    (void)hdr;
    if (!b_yhit(B, PAD_Y0, PAD_Y1)) return;   /* v1.2.11 speed: the deck is drawn only into the bands it covers (draws nothing elsewhere) */
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD);
    b_fill(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, lv_hex(0x000000));
    b_frame(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, ink);
    pad_vias_frame(B, ink);
    pad_plaque(B, ink, dim);
    pad_status(B, 10, 253, ink, dim);
    pad_draw_joey(B, ink, dim);
#if LV_RD_FOLD
    if (lv_scr == LV_SC_RADAR) pad_draw_speed(B); else pad_draw_mid3(B, ink, dim);   /* HACKbot DOWNLOAD's deck sliver stays as locked */
#else
    pad_draw_mid3(B, ink, dim);
#endif
}


/* v1.2.14 REAL FIRMWARE (Mobandy 11:34 AM ET "hackerbot has no control ui"), cloned from the v1.2.11 MOCK (Mobandy 2026-11-20 7:07 "HACKbot needs the d pad and needs to be more game-control oriented"). The MOCK's
 * code, now built into the firmware (was host-sim only behind LV_MOCK_HC_DPAD). HACKbot DOWNLOAD deck = the GIBSON deck's own primitives
 * (frame, vias, plaque, bevel, arcade discs) in the theme ink: a 4-way cross d-pad + B BACK / A OPEN. Finger zones:
 * the d-pad box x6..95 y268..316 split on its diagonals (each arm >= 44 px wide), A / B 40 x 40 as GIBSON. */
static int lv_hc_dpad_held = 0;   /* 1 up 2 down 3 left 4 right 5 A 6 B */
static void pad_hc_deck(LvBand &B, const char *st, const char *st2)
{
    if (!b_yhit(B, PAD_Y0, PAD_Y1)) return;
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD), ice = lv_hex(0x30e8ff), ac = lv_hex(0xff2ad4);
    b_fill(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, lv_hex(0x000000));
    b_frame(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, ink);
    pad_vias_frame(B, ink); pad_plaque(B, ink, dim);
    b_text35(B, 10, 253, st, ink); b_text35(B, 232 - lv_w35(st2), 253, st2, dim);
    const int cx = 50, cy = 292, a = 15;   /* cross: 3 x 3 cells of 15 px */
    pad_octagon(B, cx, cy, 42, 25, lv_hex(0x06040a), dim, 0);
    pad_glow_oct(B, cx, cy, 42, 25, ink, 2);
    pad_octagon(B, cx, cy, 36, 20, lv_hex(0x100818), ink, 0);
    const int H = lv_hc_dpad_held;
    pad_cross(B, cx, cy, H, ink, ink);
    b_text35(B, 104, 272, "UP/DN", dim); b_text35(B, 104, 280, "FOLDER", ink); b_text35(B, 104, 292, "L/R", dim); b_text35(B, 104, 300, "TOWER", ink);
    auto arcade = [&](int x, int y, uint16_t col, const char *lab, const char *sub, int held) {
        pad_disc(B, x, y, 20, lv_hex(0x08040c), col, 0); pad_glow(B, x, y, 20, held ? lv_hi(col) : col, held ? 6 : 3);
        pad_disc(B, x, y, 15, held ? lv_mix(lv_hex(0x100818), col, 5, 8) : lv_hex(0x120818), col, held);
        b_text35(B, x - 1, y - 6, lab, held ? lv_hex(0xffffff) : col); b_text35(B, x - lv_w35(sub) / 2, y + 3, sub, held ? lv_hex(0xffffff) : lv_dim(col, 6, 8)); };
    arcade(172, cy, ice, "B", "BACK", H == 6);
    arcade(216, cy, ac, "A", "OPEN", H == 5);
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
