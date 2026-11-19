/* MOCK 2026-11-19: Nintendo-style D-pad + A/B in the locked GIBSON footer terminal.
 * Same frame (1,287)-(218,319), same theme-ink border, 3x5 pixel font, chip/pin vias.
 * Three skins (lv_rd_pad_var 0..2). Nothing above y=286 is restyled. BACKbot stays outside.
 * The D4 charge current / HOLD TO LOCK bar is disabled for this mock (A locks). */
#pragma once

#ifndef LV_RD_PAD
#define LV_RD_PAD 1
#endif

static int lv_rd_pad_var  = 0;   /* 0 plus-well, 1 key-cluster, 2 rail-deck */
static int lv_rd_pad_held = 0;   /* 0 none, 1 L, 2 R, 3 U, 4 D, 5 A, 6 B */
static int lv_rd_pad_line = 0;   /* up/down cycles the pad's own 3 status lines */

static int  lv_rd_cycle_next(void);
static int  lv_rd_cycle_prev(void);
static void lv_rd_select(int i);
static void lv_rd_lock_on(int i);
static void lv_rd_release(void);
#if LV_RD_INFO
static void lv_rd_info_open(int i);
static void lv_rd_info_close(void);
#endif

#define PAD_X0 1
#define PAD_Y0 287
#define PAD_X1 218
#define PAD_Y1 319
#define PAD_CX 19
#define PAD_CY 303

static const LvRect PAD_HIT_L = {  3, 296, 13, 311 };
static const LvRect PAD_HIT_R = { 25, 296, 35, 311 };
static const LvRect PAD_HIT_U = { 11, 288, 27, 297 };
static const LvRect PAD_HIT_D = { 11, 309, 27, 318 };
static const LvRect PAD_HIT_B = {152, 289, 180, 317 };
static const LvRect PAD_HIT_A = {184, 289, 216, 317 };

static int lv_rd_pad_hit_at(int x, int y)
{
    if (y < PAD_Y0 || y > PAD_Y1 || x < PAD_X0 || x > PAD_X1) return 0;
    if (lv_in(PAD_HIT_L, x, y)) return 1;
    if (lv_in(PAD_HIT_R, x, y)) return 2;
    if (lv_in(PAD_HIT_U, x, y)) return 3;
    if (lv_in(PAD_HIT_D, x, y)) return 4;
    if (lv_in(PAD_HIT_B, x, y)) return 6;
    if (lv_in(PAD_HIT_A, x, y)) return 5;
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
    if (id == 3) { lv_rd_pad_line = (lv_rd_pad_line + 2) % 3; return "pad-up"; }
    if (id == 4) { lv_rd_pad_line = (lv_rd_pad_line + 1) % 3; return "pad-down"; }
    if (id == 5) {
#if LV_RD_INFO
        if (lv_rd_lock) { const int i = lv_rd_find(lv_rd_lockmac); if (i >= 0) lv_rd_info_open(i); return "pad-a-folder"; }
#endif
        int i = lv_rd_sel;
        if (i < 0 || i >= lv_nf) { i = lv_rd_cycle_next(); if (i < 0) return "pad-a-empty"; lv_rd_select(i); }
        lv_rd_lock_on(i); return "pad-a-lock";
    }
    if (id == 6) {
#if LV_RD_INFO
        if (lv_rd_info) { lv_rd_info_close(); return "pad-b-info-back"; }
#endif
        if (lv_rd_lock) { lv_rd_release(); lv_rd_log("RELEASED"); return "pad-b-unlock"; }
        lv_go(lv_rd_from == LV_SC_RADAR || lv_rd_from == LV_SC_BOOT || lv_rd_from == LV_SC_HOME_INTRO || lv_rd_from == LV_SC_CAL ? LV_SC_HOME : lv_rd_from);
        return "pad-b-back";
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
    for (int i = 0; i < 6; ++i) pad_via(B, 40 + i * 16, 288, ink);
    pad_via(B, 3, 288, ink); pad_via(B, 215, 288, ink);
    pad_via(B, 3, 316, ink); pad_via(B, 215, 316, ink);
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

static void pad_plaque(LvBand &B, uint16_t ink, uint16_t dim)
{
    /* recessed status well between the D-pad and A/B — same chip language as the frame */
    const int x = 37, y = 290, w = 112, h = 27;
    b_fill(B, x, y, w, h, lv_hex(0x08060a));
    b_fill(B, x, y, w, 1, lv_dim(dim, 2, 8));
    b_fill(B, x, y, 1, h, lv_dim(dim, 2, 8));
    b_fill(B, x, y + h - 1, w, 1, lv_mix(ink, lv_hex(0x000000), 2, 8));
    b_fill(B, x + w - 1, y, 1, h, lv_mix(ink, lv_hex(0x000000), 2, 8));
    b_frame(B, x, y, w, h, dim);
    pad_via(B, x + 2, y + 2, ink);
    pad_via(B, x + w - 4, y + 2, ink);
    pad_via(B, x + 2, y + h - 4, ink);
    pad_via(B, x + w - 4, y + h - 4, ink);
}

static void pad_status(LvBand &B, int x, int y, uint16_t ink, uint16_t dim)
{
    char a[28], b[28];
    int n = 0; for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) ++n;
    int k = 0; if (lv_rd_sel >= 0) { for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) { ++k; if (i == lv_rd_sel) break; } }
    if (lv_rd_sel < 0 || lv_rd_sel >= lv_nf) {
        b_text35(B, x, y, "L/R CYCLE", ink);
        b_text35(B, x, y + 7, "A LOCK  B BACK", dim);
        return;
    }
    const LvFind &f = lv_f[lv_rd_sel];
    char lab[20]; lv_label(f, lab, 16);
    if (lv_rd_pad_line == 0) snprintf(a, sizeof a, "%s", lab);
    else if (lv_rd_pad_line == 1) snprintf(a, sizeof a, "%d DBM  EX.DATA", f.rssi);
    else { char m[18]; lv_macstr(f.mac, m); snprintf(a, sizeof a, "%s", m); }
    snprintf(b, sizeof b, "%s  %d/%d", lv_rd_lock ? "LOCK" : "SEL", k, n);
    while (lv_w35(a) > 108 && a[0]) a[strlen(a) - 1] = 0;
    b_text35(B, x, y, a, lv_hex(0xffffff));
    b_text35(B, x, y + 7, b, lv_rd_lock ? lv_hex(0xff2ad4) : ink);
    for (int i = 0; i < 3; ++i) {
        const int px = x + i * 6, py = y + 16;
        b_fill(B, px, py, 4, 3, i == lv_rd_pad_line ? ink : lv_dim(ink, 3, 8));
    }
}

/* 0: circular well, joined plus, two round neon buttons (NES-like, BOTcorps chrome). */
static void pad_draw_pluswell(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t well = lv_hex(0x08040c), plastic = lv_hex(0x141018);
    const uint16_t ac = lv_hex(0xff2ad4);
    pad_disc(B, PAD_CX, PAD_CY, 15, well, dim, 0);
    const int aw = 9, al = 25, ax = PAD_CX - aw / 2, ay = PAD_CY - al / 2;
    pad_bevel_rect(B, ax, ay, aw, al, plastic, ink, 0);
    pad_bevel_rect(B, PAD_CX - al / 2, PAD_CY - aw / 2, al, aw, plastic, ink, 0);
    if (lv_rd_pad_held == 3) pad_bevel_rect(B, ax, ay, aw, 11, plastic, ink, 1);
    if (lv_rd_pad_held == 4) pad_bevel_rect(B, ax, PAD_CY + 1, aw, 11, plastic, ink, 1);
    if (lv_rd_pad_held == 1) pad_bevel_rect(B, PAD_CX - al / 2, PAD_CY - aw / 2, 11, aw, plastic, ink, 1);
    if (lv_rd_pad_held == 2) pad_bevel_rect(B, PAD_CX + 1, PAD_CY - aw / 2, 11, aw, plastic, ink, 1);
    pad_disc(B, PAD_CX, PAD_CY, 3, lv_rd_pad_held && lv_rd_pad_held <= 4 ? lv_hi(ink) : dim, ink, 0);
    pad_disc(B, 166, PAD_CY, 12, lv_hex(0x100818), ink, lv_rd_pad_held == 6);
    pad_disc(B, 200, PAD_CY, 12, lv_rd_pad_held == 5 ? lv_hex(0x140010) : lv_mix(lv_hex(0x140010), ac, 3, 8), ac, lv_rd_pad_held == 5);
    b_text35(B, 164, 301, "B", lv_rd_pad_held == 6 ? lv_hex(0xffffff) : ink);
    b_text35(B, 198, 301, "A", lv_rd_pad_held == 5 ? lv_hex(0xffffff) : ac);
    pad_plaque(B, ink, dim);
    pad_status(B, 41, 294, ink, dim);
}

/* 1: four separate chip keys + rectangular A/B keycaps with corner vias. */
static void pad_chip_key(LvBand &B, int x, int y, int w, int h, const char *g, uint16_t ink, int pressed)
{
    const uint16_t fill = pressed ? lv_mix(lv_hex(0x100818), ink, 4, 8) : lv_hex(0x100818);
    pad_bevel_rect(B, x, y, w, h, fill, ink, pressed);
    pad_via(B, x + 1, y + 1, ink);
    pad_via(B, x + w - 3, y + 1, ink);
    pad_via(B, x + 1, y + h - 3, ink);
    pad_via(B, x + w - 3, y + h - 3, ink);
    if (g && g[0]) {
        const int tx = x + (w - lv_w35(g)) / 2;
        const int ty = y + (h - 5) / 2 + (pressed ? 1 : 0);
        b_text35(B, tx, ty, g, pressed ? lv_hex(0xffffff) : ink);
    }
}

static void pad_mini_arrow(LvBand &B, int cx, int cy, int dir, uint16_t c)
{
    /* dir: 1 L, 2 R, 3 U, 4 D — 3-px chevron inside a 9x9 key */
    if (dir == 3) { b_pix(B, cx, cy - 1, c); b_pix(B, cx - 1, cy, c); b_pix(B, cx, cy, c); b_pix(B, cx + 1, cy, c); }
    else if (dir == 4) { b_pix(B, cx, cy + 1, c); b_pix(B, cx - 1, cy, c); b_pix(B, cx, cy, c); b_pix(B, cx + 1, cy, c); }
    else if (dir == 1) { b_pix(B, cx - 1, cy, c); b_pix(B, cx, cy - 1, c); b_pix(B, cx, cy, c); b_pix(B, cx, cy + 1, c); }
    else { b_pix(B, cx + 1, cy, c); b_pix(B, cx, cy - 1, c); b_pix(B, cx, cy, c); b_pix(B, cx, cy + 1, c); }
}

static void pad_draw_keys(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4);
    const int s = 9, g = 1, cx = PAD_CX, cy = PAD_CY;
    /* recessed well behind the four keys */
    pad_bevel_rect(B, 4, 288, 31, 31, lv_hex(0x08040c), dim, 1);
    const int ux = cx - s / 2, uy = cy - s - g - s / 2;
    const int dx = cx - s / 2, dy = cy + s / 2 + g;
    const int lx = cx - s - g - s / 2, ly = cy - s / 2;
    const int rx = cx + s / 2 + g, ry = cy - s / 2;
    pad_bevel_rect(B, ux, uy, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 3);
    pad_bevel_rect(B, dx, dy, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 4);
    pad_bevel_rect(B, lx, ly, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 1);
    pad_bevel_rect(B, rx, ry, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 2);
    pad_mini_arrow(B, ux + s / 2, uy + s / 2, 3, lv_rd_pad_held == 3 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, dx + s / 2, dy + s / 2, 4, lv_rd_pad_held == 4 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, lx + s / 2, ly + s / 2, 1, lv_rd_pad_held == 1 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, rx + s / 2, ry + s / 2, 2, lv_rd_pad_held == 2 ? lv_hex(0xffffff) : ink);
    pad_chip_key(B, 154, 291, 26, 26, "B", ink, lv_rd_pad_held == 6);
    pad_chip_key(B, 186, 291, 28, 26, "A", ac,  lv_rd_pad_held == 5);
    pad_plaque(B, ink, dim);
    pad_status(B, 41, 294, ink, dim);
}

/* 2: SNES-like disc pad, diagonal A/B, pin rail, faint CRT scanlines. */
static void pad_draw_rail(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), bc = lv_hex(0x30e8ff);
    /* CRT scanlines inside the locked frame only */
    for (int y = 288; y <= 318; y += 2) b_fill(B, 2, y, 216, 1, lv_dim(ink, 1, 10));
    /* pin rail under the top frame */
    for (int x = 6; x <= 212; x += 4) { b_pix(B, x, 289, dim); b_pix(B, x, 290, ink); }
    pad_disc(B, PAD_CX, PAD_CY, 15, lv_hex(0x0a0810), dim, 0);
    pad_disc(B, PAD_CX, PAD_CY, 12, lv_hex(0x141018), ink, 0);
    /* plus cut inside the disc */
    const int aw = 7, al = 19, ax = PAD_CX - aw / 2, ay = PAD_CY - al / 2;
    pad_bevel_rect(B, ax, ay, aw, al, lv_hex(0x18141c), ink, 0);
    pad_bevel_rect(B, PAD_CX - al / 2, PAD_CY - aw / 2, al, aw, lv_hex(0x18141c), ink, 0);
    if (lv_rd_pad_held == 3) pad_bevel_rect(B, ax, ay, aw, 9, lv_hex(0x18141c), ink, 1);
    if (lv_rd_pad_held == 4) pad_bevel_rect(B, ax, PAD_CY + 1, aw, 9, lv_hex(0x18141c), ink, 1);
    if (lv_rd_pad_held == 1) pad_bevel_rect(B, PAD_CX - al / 2, PAD_CY - aw / 2, 9, aw, lv_hex(0x18141c), ink, 1);
    if (lv_rd_pad_held == 2) pad_bevel_rect(B, PAD_CX + 1, PAD_CY - aw / 2, 9, aw, lv_hex(0x18141c), ink, 1);
    pad_disc(B, PAD_CX, PAD_CY, 2, dim, ink, 0);
    /* diagonal NES cluster: B lower-left, A upper-right, cyan / magenta */
    pad_disc(B, 164, 308, 10, lv_hex(0x041018), bc, lv_rd_pad_held == 6);
    pad_disc(B, 200, 297, 11, lv_hex(0x140010), ac, lv_rd_pad_held == 5);
    b_text35(B, 162, 306, "B", lv_rd_pad_held == 6 ? lv_hex(0xffffff) : bc);
    b_text35(B, 198, 295, "A", lv_rd_pad_held == 5 ? lv_hex(0xffffff) : ac);
    pad_plaque(B, ink, dim);
    pad_status(B, 41, 294, ink, dim);
}

static void lv_rd_pad_draw(LvBand &B, uint16_t hdr)
{
    (void)hdr;
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD);
    b_fill(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, lv_hex(0x000000));
    b_frame(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, ink);
    pad_vias_frame(B, ink);
    if (lv_rd_pad_var == 1) pad_draw_keys(B, ink, dim);
    else if (lv_rd_pad_var == 2) pad_draw_rail(B, ink, dim);
    else pad_draw_pluswell(B, ink, dim);
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
