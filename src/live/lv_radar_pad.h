/* MOCK 2026-11-19: Nintendo-style D-pad + A/B in a wider GIBSON controls deck.
 * Frame (1,247)-(218,319) — same top as the locked mid-panel, BACKbot stays in the corner.
 * lv_rd_pad_var 0..5. Towers / rain / camera clip at y=246; nothing above is restyled.
 * D4 charge beam is off; A locks. L/R cycle towers, U/D cycle status lines, B unlocks or back. */
#pragma once

#ifndef LV_RD_PAD
#define LV_RD_PAD 1
#endif

static int lv_rd_pad_var  = 0;   /* 0 pluswell 1 keycluster 2 raildeck 3 joey 4 famicom 5 griddeck */
static int lv_rd_pad_held = 0;   /* 0 none, 1 L, 2 R, 3 U, 4 D, 5 A, 6 B */
static int lv_rd_pad_line = 0;

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
#define PAD_Y0 247
#define PAD_X1 218
#define PAD_Y1 319
#define PAD_CX 27
#define PAD_CY 292

/* ~40 px D-pad well and A/B, fitted to the wider deck, clear of BACKbot */
static const LvRect PAD_HIT_L = {  5, 276, 20, 308 };
static const LvRect PAD_HIT_R = { 34, 276, 49, 308 };
static const LvRect PAD_HIT_U = { 12, 268, 42, 284 };
static const LvRect PAD_HIT_D = { 12, 300, 42, 316 };
static const LvRect PAD_HIT_B = {132, 272, 171, 311 };
static const LvRect PAD_HIT_A = {176, 272, 215, 311 };
/* Center I/W/H: fill the gap between the D-pad (x<=49) and B (x>=132), band y=268..317.
 * Visual pills sit inside these hits. Sizes: IGNORE 78x16, WATCH 78x16, HUNT 78x17. */
static const LvRect PAD_HIT_IGN = { 52, 268, 129, 283 };
static const LvRect PAD_HIT_WAT = { 52, 284, 129, 299 };
static const LvRect PAD_HIT_HNT = { 52, 300, 129, 316 };
#define PAD_MID_X  58
#define PAD_MID_W  58
#define PAD_MID_H  11
#define PAD_MID_Y0 270
#define PAD_MID_Y1 287
#define PAD_MID_Y2 304

static int lv_rd_pad_hit_at(int x, int y)
{
    if (y < PAD_Y0 || y > PAD_Y1 || x < PAD_X0 || x > PAD_X1) return 0;
    if (lv_in(PAD_HIT_L, x, y)) return 1;
    if (lv_in(PAD_HIT_R, x, y)) return 2;
    if (lv_in(PAD_HIT_U, x, y)) return 3;
    if (lv_in(PAD_HIT_D, x, y)) return 4;
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
    if (id == 7 || id == 8 || id == 9) {
        if (!lv_rd_lock) return "pad-mid-idle";
        /* actions are handled in lv_radar_tap via PAD_HIT_* / LV_RD_BTNT so a lock-row tap
         * still runs IGNORE / WATCH / HUNT. This path is the unlocked no-op plus a fallback. */
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
    for (int i = 0; i < 10; ++i) pad_via(B, 8 + i * 20, 248, ink);
    pad_via(B, 3, 248, ink); pad_via(B, 215, 248, ink);
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

static void pad_glow(LvBand &B, int cx, int cy, int r, uint16_t c, int k)
{
    const int r2 = (r + 1) * (r + 1), r0 = (r - 1) * (r - 1);
    for (int dy = -r - 1; dy <= r + 1; ++dy) for (int dx = -r - 1; dx <= r + 1; ++dx) {
        const int d2 = dx * dx + dy * dy;
        if (d2 > r2 || d2 < r0) continue;
        b_blend(B, cx + dx, cy + dy, c, k);
    }
}

static void pad_octagon(LvBand &B, int cx, int cy, int r, uint16_t fill, uint16_t ink, int pressed)
{
    const uint16_t hi = pressed ? lv_dim(fill, 3, 8) : lv_mix(ink, lv_hex(0xffffff), 6, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 3, 8) : lv_dim(ink, 3, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 5, 8) : fill;
    const int cut = r - r / 3;
    for (int dy = -r; dy <= r; ++dy) for (int dx = -r; dx <= r; ++dx) {
        if (abs(dx) + abs(dy) > r + cut) continue;
        if (abs(dx) > r || abs(dy) > r) continue;
        uint16_t c = body;
        const int edge = (abs(dx) == r || abs(dy) == r || abs(dx) + abs(dy) == r + cut);
        if (edge) c = (dx + dy < 0) ? hi : lo;
        else if (dx * 2 + dy * 3 < -r) c = lv_mix(body, hi, 3, 8);
        if (pressed) c = lv_mix(c, ink, 3, 8);
        b_pix(B, cx + dx, cy + dy, c);
    }
}

static void pad_plaque(LvBand &B, uint16_t ink, uint16_t dim)
{
    const int x = 6, y = 249, w = 206, h = 16;
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
        b_text35(B, x + 120, y, "A LOCK  B BACK", dim);
        return;
    }
    const LvFind &f = lv_f[lv_rd_sel];
    char lab[20]; lv_label(f, lab, 16);
    if (lv_rd_pad_line == 0) snprintf(a, sizeof a, "%s", lab);
    else if (lv_rd_pad_line == 1) snprintf(a, sizeof a, "%d DBM  EX.DATA", f.rssi);
    else { char m[18]; lv_macstr(f.mac, m); snprintf(a, sizeof a, "%s", m); }
    snprintf(b, sizeof b, "%s %d/%d", lv_rd_lock ? "LOCK" : "SEL", k, n);
    while (lv_w35(a) > 140 && a[0]) a[strlen(a) - 1] = 0;
    b_text35(B, x, y, a, lv_hex(0xffffff));
    b_text35(B, x + 148, y, b, lv_rd_lock ? lv_hex(0xff2ad4) : ink);
    for (int i = 0; i < 3; ++i) b_fill(B, x + 200 + i * 5, y + 1, 3, 3, i == lv_rd_pad_line ? ink : lv_dim(ink, 3, 8));
}

static void pad_plus_arms(LvBand &B, int cx, int cy, int aw, int al, uint16_t fill, uint16_t ink)
{
    const int ax = cx - aw / 2, ay = cy - al / 2;
    pad_bevel_rect(B, ax, ay, aw, al, fill, ink, 0);
    pad_bevel_rect(B, cx - al / 2, cy - aw / 2, al, aw, fill, ink, 0);
    if (lv_rd_pad_held == 3) pad_bevel_rect(B, ax, ay, aw, al / 2, fill, ink, 1);
    if (lv_rd_pad_held == 4) pad_bevel_rect(B, ax, cy + 1, aw, al / 2, fill, ink, 1);
    if (lv_rd_pad_held == 1) pad_bevel_rect(B, cx - al / 2, cy - aw / 2, al / 2, aw, fill, ink, 1);
    if (lv_rd_pad_held == 2) pad_bevel_rect(B, cx + 1, cy - aw / 2, al / 2, aw, fill, ink, 1);
}

/* 0: circular well, joined plus, two round neon buttons. */
static void pad_draw_pluswell(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), plastic = lv_hex(0x141018);
    pad_disc(B, PAD_CX, PAD_CY, 22, lv_hex(0x08040c), dim, 0);
    pad_plus_arms(B, PAD_CX, PAD_CY, 13, 36, plastic, ink);
    pad_disc(B, PAD_CX, PAD_CY, 4, lv_rd_pad_held && lv_rd_pad_held <= 4 ? lv_hi(ink) : dim, ink, 0);
    if (lv_rd_sel >= 0 && !lv_rd_lock) pad_glow(B, 196, PAD_CY, 20, ac, 3);
    pad_disc(B, 152, PAD_CY, 18, lv_hex(0x100818), ink, lv_rd_pad_held == 6);
    pad_disc(B, 196, PAD_CY, 18, lv_rd_pad_held == 5 ? lv_hex(0x140010) : lv_mix(lv_hex(0x140010), ac, 3, 8), ac, lv_rd_pad_held == 5);
    b_text35(B, 150, PAD_CY - 2, "B", lv_rd_pad_held == 6 ? lv_hex(0xffffff) : ink);
    b_text35(B, 194, PAD_CY - 2, "A", lv_rd_pad_held == 5 ? lv_hex(0xffffff) : ac);
}

/* 1: four chip keys + rectangular A/B keycaps with corner vias. */
static void pad_mini_arrow(LvBand &B, int cx, int cy, int dir, uint16_t c)
{
    if (dir == 3) { b_pix(B, cx, cy - 2, c); b_fill(B, cx - 2, cy - 1, 5, 1, c); b_fill(B, cx - 1, cy, 3, 1, c); b_pix(B, cx, cy + 1, c); }
    else if (dir == 4) { b_pix(B, cx, cy + 2, c); b_fill(B, cx - 2, cy + 1, 5, 1, c); b_fill(B, cx - 1, cy, 3, 1, c); b_pix(B, cx, cy - 1, c); }
    else if (dir == 1) { b_pix(B, cx - 2, cy, c); b_fill(B, cx - 1, cy - 2, 1, 5, c); b_fill(B, cx, cy - 1, 1, 3, c); b_pix(B, cx + 1, cy, c); }
    else { b_pix(B, cx + 2, cy, c); b_fill(B, cx + 1, cy - 2, 1, 5, c); b_fill(B, cx, cy - 1, 1, 3, c); b_pix(B, cx - 1, cy, c); }
}

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

static void pad_draw_keys(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4);
    const int s = 14, g = 2, cx = PAD_CX, cy = PAD_CY;
    pad_bevel_rect(B, 5, 268, 45, 48, lv_hex(0x08040c), dim, 1);
    const int ux = cx - s / 2, uy = cy - s - g - s / 2;
    const int dx_ = cx - s / 2, dy = cy + s / 2 + g;
    const int lx = cx - s - g - s / 2, ly = cy - s / 2;
    const int rx = cx + s / 2 + g, ry = cy - s / 2;
    pad_bevel_rect(B, ux, uy, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 3);
    pad_bevel_rect(B, dx_, dy, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 4);
    pad_bevel_rect(B, lx, ly, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 1);
    pad_bevel_rect(B, rx, ry, s, s, lv_hex(0x100818), ink, lv_rd_pad_held == 2);
    pad_mini_arrow(B, ux + s / 2, uy + s / 2, 3, lv_rd_pad_held == 3 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, dx_ + s / 2, dy + s / 2, 4, lv_rd_pad_held == 4 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, lx + s / 2, ly + s / 2, 1, lv_rd_pad_held == 1 ? lv_hex(0xffffff) : ink);
    pad_mini_arrow(B, rx + s / 2, ry + s / 2, 2, lv_rd_pad_held == 2 ? lv_hex(0xffffff) : ink);
    if (lv_rd_sel >= 0 && !lv_rd_lock) pad_glow(B, 196, 292, 20, ac, 3);
    pad_chip_key(B, 134, 272, 38, 40, "B", ink, lv_rd_pad_held == 6);
    pad_chip_key(B, 176, 272, 40, 40, "A", ac,  lv_rd_pad_held == 5);
}

/* 2: SNES-like disc pad, diagonal A/B, pin rail, CRT scanlines. */
static void pad_draw_rail(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), bc = lv_hex(0x30e8ff);
    for (int y = 267; y <= 318; y += 2) b_fill(B, 2, y, 216, 1, lv_dim(ink, 1, 12));
    for (int x = 6; x <= 212; x += 4) { b_pix(B, x, 267, dim); b_pix(B, x, 268, ink); }
    pad_disc(B, PAD_CX, PAD_CY, 22, lv_hex(0x0a0810), dim, 0);
    pad_disc(B, PAD_CX, PAD_CY, 18, lv_hex(0x141018), ink, 0);
    pad_plus_arms(B, PAD_CX, PAD_CY, 11, 30, lv_hex(0x18141c), ink);
    pad_disc(B, PAD_CX, PAD_CY, 3, dim, ink, 0);
    if (lv_rd_sel >= 0 && !lv_rd_lock) pad_glow(B, 196, 284, 18, ac, 3);
    pad_disc(B, 150, 302, 16, lv_hex(0x041018), bc, lv_rd_pad_held == 6);
    pad_disc(B, 196, 284, 17, lv_hex(0x140010), ac, lv_rd_pad_held == 5);
    b_text35(B, 148, 300, "B", lv_rd_pad_held == 6 ? lv_hex(0xffffff) : bc);
    b_text35(B, 194, 282, "A", lv_rd_pad_held == 5 ? lv_hex(0xffffff) : ac);
}

/* 3 JOEY CONSOLE: octagonal cyber gate, CRT status already in the plaque, heavy arcade LOCK/BACK. */
static void pad_draw_joey(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), ice = lv_hex(0x30e8ff);
    /* deep octagonal gate */
    pad_octagon(B, PAD_CX, PAD_CY, 23, lv_hex(0x06040a), dim, 0);
    pad_glow(B, PAD_CX, PAD_CY, 23, ink, 2);
    pad_octagon(B, PAD_CX, PAD_CY, 18, lv_hex(0x100818), ink, 0);
    pad_plus_arms(B, PAD_CX, PAD_CY, 12, 28, lv_hex(0x1a1018), ink);
    pad_disc(B, PAD_CX, PAD_CY, 3, lv_rd_lock ? ac : ink, ink, 0);
    /* arcade buttons: thick bevel rings + inner glow */
    auto arcade = [&](int cx, int cy, uint16_t col, const char *lab, const char *sub, int held) {
        pad_disc(B, cx, cy, 20, lv_hex(0x08040c), col, 0);
        pad_glow(B, cx, cy, 20, held ? lv_hi(col) : col, held ? 6 : 3);
        pad_disc(B, cx, cy, 15, held ? lv_mix(lv_hex(0x100818), col, 5, 8) : lv_hex(0x120818), col, held);
        b_text35(B, cx - 1, cy - 6, lab, held ? lv_hex(0xffffff) : col);
        b_text35(B, cx - lv_w35(sub) / 2, cy + 3, sub, held ? lv_hex(0xffffff) : lv_dim(col, 6, 8));
    };
    arcade(152, PAD_CY, ice, "B", "BACK", lv_rd_pad_held == 6);
    arcade(196, PAD_CY, ac,  "A", "LOCK", lv_rd_pad_held == 5);
}

/* 4 FAMICOM BRICK: raised controller body, circular well, classic offset A/B. */
static void pad_draw_famicom(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), cream = lv_hex(0xc8c0b4), body = lv_hex(0x3a3244);
    /* raised controller brick — visible mid-purple plastic, not a black slab */
    pad_bevel_rect(B, 4, 267, 212, 51, lv_hex(0x221c28), dim, 0);
    pad_bevel_rect(B, 6, 269, 208, 47, body, ink, 0);
    b_fill(B, 7, 270, 206, 2, lv_mix(body, lv_hex(0xffffff), 3, 8));
    b_fill(B, 7, 313, 206, 2, lv_dim(body, 3, 8));
    /* screw vias at the four corners of the brick */
    pad_via(B, 8, 271, cream); pad_via(B, 208, 271, cream);
    pad_via(B, 8, 311, cream); pad_via(B, 208, 311, cream);
    /* SEL/STR slots are now IGNORE / WATCH / HUNT, drawn by pad_draw_mid3 after the brick */
    /* deep cream-lipped well, lighter plus so it reads on the brick */
    pad_disc(B, PAD_CX, PAD_CY, 22, lv_hex(0x0c0810), cream, 0);
    pad_disc(B, PAD_CX, PAD_CY, 18, lv_hex(0x201820), ink, 0);
    pad_plus_arms(B, PAD_CX, PAD_CY, 13, 32, lv_hex(0x4a4258), cream);
    pad_disc(B, PAD_CX, PAD_CY, 4, lv_hex(0x2a2434), cream, 0);
    if (lv_rd_sel >= 0 && !lv_rd_lock) pad_glow(B, 198, 282, 18, ac, 4);
    pad_disc(B, 154, 300, 17, lv_hex(0x1a1018), ink, lv_rd_pad_held == 6);
    pad_disc(B, 198, 282, 18, lv_mix(lv_hex(0x140010), ac, 4, 8), ac, lv_rd_pad_held == 5);
    b_text35(B, 152, 298, "B", lv_rd_pad_held == 6 ? lv_hex(0xffffff) : cream);
    b_text35(B, 196, 280, "A", lv_rd_pad_held == 5 ? lv_hex(0xffffff) : ac);
}

/* 5 GIBSON GRID DECK: mini perspective grid, tile plus, trapezoid hacking keys. */
static void pad_trap_key(LvBand &B, int x, int y, int w, int h, uint16_t fill, uint16_t ink, const char *lab, const char *sub, int pressed)
{
    /* trapezoid: top 4 px narrower */
    const uint16_t hi = pressed ? lv_dim(fill, 2, 8) : lv_mix(ink, lv_hex(0xffffff), 6, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 4, 8) : lv_dim(fill, 2, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 5, 8) : fill;
    for (int yy = 0; yy < h; ++yy) {
        const int inset = (h - 1 - yy) * 3 / h;
        const int x0 = x + inset, ww = w - inset * 2;
        uint16_t c = body;
        if (yy == 0) c = hi;
        else if (yy == h - 1) c = lo;
        b_fill(B, x0, y + yy, ww, 1, c);
        b_pix(B, x0, y + yy, yy < h / 2 ? hi : lo);
        b_pix(B, x0 + ww - 1, y + yy, lo);
    }
    b_text35(B, x + (w - lv_w35(lab)) / 2, y + 8 + (pressed ? 1 : 0), lab, pressed ? lv_hex(0xffffff) : ink);
    b_text35(B, x + (w - lv_w35(sub)) / 2, y + h - 10 + (pressed ? 1 : 0), sub, pressed ? lv_hex(0xffffff) : lv_dim(ink, 6, 8));
}

static void pad_draw_griddeck(LvBand &B, uint16_t ink, uint16_t dim)
{
    const uint16_t ac = lv_hex(0xff2ad4), ice = lv_hex(0x30e8ff);
    /* mini floor grid in the control band */
    b_fill(B, 2, 267, 216, 52, lv_hex(0x040208));
    const int hor = 292;
    for (int i = 0; i < 6; ++i) {
        const int y = 268 + i * i / 2 + i * 6;
        if (y > 318) break;
        b_fill(B, 2, y, 216, 1, lv_dim(ink, 2 + i, 16));
    }
    for (int i = -6; i <= 6; ++i) {
        const int x0 = 109 + i * 8, x1 = 109 + i * 18;
        /* vanishing lines toward mid of the deck */
        int x = x0, y = 268;
        const int n = 50;
        for (int s = 0; s < n; ++s) {
            const int xx = x0 + (x1 - x0) * s / n;
            const int yy = 268 + s * 50 / n;
            if (yy > 318 || xx < 2 || xx > 217) continue;
            b_pix(B, xx, yy, lv_dim(ink, 2, 10));
        }
        (void)x; (void)y; (void)hor;
    }
    /* tile plus — four lit blocks + center */
    auto tile = [&](int x, int y, int w, int h, int held) {
        pad_bevel_rect(B, x, y, w, h, held ? lv_mix(lv_hex(0x100818), ink, 5, 8) : lv_hex(0x100818), ink, held);
        if (held) pad_glow(B, x + w / 2, y + h / 2, w / 2 + 2, ink, 4);
    };
    tile(PAD_CX - 6, PAD_CY - 20, 13, 16, lv_rd_pad_held == 3);
    tile(PAD_CX - 6, PAD_CY + 5,  13, 16, lv_rd_pad_held == 4);
    tile(PAD_CX - 20, PAD_CY - 6, 16, 13, lv_rd_pad_held == 1);
    tile(PAD_CX + 5,  PAD_CY - 6, 16, 13, lv_rd_pad_held == 2);
    pad_bevel_rect(B, PAD_CX - 6, PAD_CY - 6, 13, 13, lv_rd_lock ? ac : dim, ink, 0);
    pad_mini_arrow(B, PAD_CX, PAD_CY - 13, 3, ink);
    pad_mini_arrow(B, PAD_CX, PAD_CY + 13, 4, ink);
    pad_mini_arrow(B, PAD_CX - 13, PAD_CY, 1, ink);
    pad_mini_arrow(B, PAD_CX + 13, PAD_CY, 2, ink);
    pad_trap_key(B, 130, 272, 40, 42, lv_hex(0x041018), ice, "B", "BACK", lv_rd_pad_held == 6);
    pad_trap_key(B, 174, 272, 42, 42, lv_hex(0x140010), ac,  "A", "LOCK", lv_rd_pad_held == 5);
    if (lv_rd_sel >= 0 && !lv_rd_lock) pad_glow(B, 195, 293, 22, ac, 3);
}


/* Center START/SELECT/TURBO-style IGNORE / WATCH / HUNT, in each skin's own language.
 * Locked = live colours; idle = dimmed / disabled in the same place; press sinks the pill. */
static void pad_pill_oval(LvBand &B, int x, int y, int w, int h, uint16_t fill, uint16_t ink, int pressed)
{
    const uint16_t hi = pressed ? lv_dim(fill, 2, 8) : lv_mix(ink, lv_hex(0xffffff), 6, 8);
    const uint16_t lo = pressed ? lv_mix(ink, lv_hex(0xffffff), 3, 8) : lv_dim(fill, 2, 8);
    const uint16_t body = pressed ? lv_mix(fill, ink, 4, 8) : fill;
    const int cy = y + h / 2, r = h / 2;
    for (int yy = 0; yy < h; ++yy) {
        const int dy = yy - r;
        int inset = 0;
        for (int s = r; s >= 0; --s) if (s * s + dy * dy <= r * r) { inset = r - s; break; }
        if (inset > w / 3) inset = w / 3;
        uint16_t c = body;
        if (yy == 0) c = hi;
        else if (yy == h - 1) c = lo;
        else if (yy == 1) c = lv_mix(body, hi, 3, 8);
        b_fill(B, x + inset, y + yy, w - 2 * inset, 1, c);
        b_pix(B, x + inset, y + yy, yy < h / 2 ? hi : lo);
        b_pix(B, x + w - inset - 1, y + yy, lo);
    }
}

static void pad_draw_mid3(LvBand &B, uint16_t ink, uint16_t dim)
{
    const char *L[3] = { "IGNORE", "WATCH", "HUNT" };
    const uint16_t live[3] = { lv_hex(0xa0a0a8), lv_hex(0xa0fc00), lv_hex(0xff2ad4) };
    const int ys[3] = { PAD_MID_Y0, PAD_MID_Y1, PAD_MID_Y2 };
    const int x = PAD_MID_X, w = PAD_MID_W, h = PAD_MID_H;
    const bool on = lv_rd_lock;
    const int v = lv_rd_pad_var;
    for (int k = 0; k < 3; ++k) {
        const int y = ys[k];
        const int held = (lv_rd_pad_held == 7 + k);
        const uint16_t acc = on ? live[k] : lv_dim(live[k], 3, 8);
        const uint16_t fill = on
            ? (held ? lv_mix(lv_hex(0x100818), acc, 5, 8) : lv_hex(0x100818))
            : lv_hex(0x0a0810);
        const uint16_t lab = !on ? lv_dim(acc, 4, 8) : (held ? lv_hex(0xffffff) : acc);
        const int ly = y + (h - 5) / 2 + (held ? 1 : 0);
        const int lx = x + (w - lv_w35(L[k])) / 2;
        if (v == 1) {
            /* keycluster: tiny chip keys with corner vias */
            pad_bevel_rect(B, x, y, w, h, fill, acc, held);
            pad_via(B, x + 1, y + 1, acc);
            pad_via(B, x + w - 3, y + 1, acc);
        } else if (v == 2) {
            /* raildeck: slim CRT slots */
            pad_bevel_rect(B, x, y, w, h, fill, acc, held);
            if ((y + 2) % 2 == 0) b_fill(B, x + 2, y + 2, w - 4, 1, lv_dim(acc, 2, 10));
        } else if (v == 3) {
            /* joey: glow-ring arcade micros */
            pad_bevel_rect(B, x - 1, y - 1, w + 2, h + 2, lv_hex(0x08040c), acc, 0);
            if (on) pad_glow(B, x + w / 2, y + h / 2, w / 2, acc, held ? 5 : 2);
            pad_bevel_rect(B, x, y, w, h, fill, acc, held);
        } else if (v == 4) {
            /* famicom: recessed NES SELECT/START/TURBO slots */
            pad_bevel_rect(B, x, y, w, h, lv_hex(0x18141c), on ? acc : dim, 1);
            if (held) b_fill(B, x + 1, y + 1, w - 2, h - 2, lv_mix(lv_hex(0x18141c), acc, 4, 8));
        } else if (v == 5) {
            /* griddeck: lit trapezoid tiles */
            const int inset = 2;
            for (int yy = 0; yy < h; ++yy) {
                const int in = (h - 1 - yy) * inset / h;
                uint16_t c = fill;
                if (yy == 0) c = on ? lv_mix(acc, lv_hex(0xffffff), 4, 8) : dim;
                else if (yy == h - 1) c = lv_dim(fill, 2, 8);
                b_fill(B, x + in, y + yy, w - 2 * in, 1, c);
            }
            b_pix(B, x, y, acc); b_pix(B, x + w - 1, y + h - 1, lv_dim(acc, 3, 8));
        } else {
            /* pluswell: glossy oval START/SELECT/TURBO pills */
            pad_pill_oval(B, x, y, w, h, fill, acc, held);
        }
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
    if (lv_rd_pad_var == 1) pad_draw_keys(B, ink, dim);
    else if (lv_rd_pad_var == 2) pad_draw_rail(B, ink, dim);
    else if (lv_rd_pad_var == 3) pad_draw_joey(B, ink, dim);
    else if (lv_rd_pad_var == 4) pad_draw_famicom(B, ink, dim);
    else if (lv_rd_pad_var == 5) pad_draw_griddeck(B, ink, dim);
    else pad_draw_pluswell(B, ink, dim);
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
