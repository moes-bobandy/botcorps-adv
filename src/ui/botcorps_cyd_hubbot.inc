/* ============================================================================
 * GIBSON HUB v4 — CYD portrait hub (SCR_GIBSON) drawn as a Control-bot v4 board:
 * GIBSONbot module (ivory face, dark Agent Smith glasses, never theme-coloured),
 * 12 bot chips + TERMINAL/BOT + CONTROL/BOT nav chips, copper traces, full-length side headers,
 * TERMINALBOT plate. PICK.md (out/cyd-bootbot-intro-mock) paint_hub_target: edge-to-edge board,
 * NO BOOTbot on the hub (BOOTbot only lives in the boot intro), no top-bar terminal glyph.
 * Geometry: botcorps_hubbot_layout.inc, portrait + landscape tables (hb_layout_sync).
 * Included from botcorps_cyd_portrait.inc under BOTCORPS_CYD, after botcorps_cyd_edge.inc.
 * ==========================================================================*/
#include "ui/botcorps_hubbot_layout.inc"

/* Highlight: 0..13 chip, 14 GIBSON module, 15 TERMINALBOT plate, -1 none.
 * Derived from the real hub state each paint (hb_sel_now). */
static int s_hb_sel = -1;

static int hb_sel_now(void)
{
    if (s_pt_mode != 0) return 15;                 /* plate open (keys / prompt list) */
    if (s_hub_zone == 3) return 14;                /* Gibson focused */
    if (s_hub_zone == 0 && (s_cursor == 0 || s_cursor == 1)) return 12 + s_cursor;  /* nav chips */
    return -1;
}

/* Rain covers: PICK.md HUB_COVER (portrait) / HB_COVER_L (landscape), all within cover_add's 24. */
static void hb_covers(void)
{
    const botcorps_cover_t *c = botcorps_cyd_orient ? HB_COVER_P : HB_COVER_L;
    const int n = botcorps_cyd_orient ? 22 : 21;
    for (int i = 0; i < n; ++i) cover_add(c[i].x, c[i].y, c[i].w, c[i].h);
}

/* chip silk: one or two lines from HB_SILK1/HB_SILK2 (layout.py is the source) */
static void hb_silk(Gfx g, int i, int cx, int y, uint8_t col, uint8_t bg)
{
    const char *a = HB_SILK1[i], *b = HB_SILK2[i];
    const int wa = tiny_w(a), wb = tiny_w(b), w = wa > wb ? wa : wb;
    const int h = 7 + (b[0] ? HB_SILK_PITCH : 0);
    cbf(g, cx - w / 2 - 1, y - 1, w + 2, h, bg);
    tiny_text(g, a, cx - wa / 2, y, col);
    if (b[0]) tiny_text(g, b, cx - wb / 2, y + HB_SILK_PITCH, col);
}

static void hb_castle(Gfx g, int cx, int cy, int T)
{
    const int x = cx - T / 2, y = cy - T / 2;
    const int D = T - 2, dx = x + 1, dy = y + 1;
    g.fillRect(dx, dy, D, D, PAL_BLACK);
    g.drawRect(dx, dy, D, D, PAL_STEEL);
    for (int k = 0; k < 5; ++k) {
        const int i = chip_pin1(D, k);
        g.fillRect(x, dy + i, 2, 1, PAL_SOFT);
        g.fillRect(x + T - 2, dy + i, 2, 1, PAL_SOFT);
        g.fillRect(dx + i, y, 1, 2, PAL_SOFT);
        g.fillRect(dx + i, y + T - 2, 1, 2, PAL_SOFT);
    }
}



static void hb_chip(Gfx g, int i)
{
    if (i >= 14) return;                           /* no BOOTbot on the hub */
    const int cx = HB_CHIP[i][0], top = HB_CHIP[i][1];
    const int T = 18, cy = top + T / 2;
    const uint8_t ac = accent();
    const bool on = s_hb_sel == i;
    if (on) sel_rect(g, cx - T / 2 - 2, top - 2, T + 4, T + 4);
    if (i < 12) {
        chip_mini(g, cx, cy, T, i, false, 0);
        g.fillRect(cx + T / 2 - 4, top - 1, 4, 4, bot_color(i));
    } else {
        hb_castle(g, cx, cy, T);
        if (i == 12) {
            txt(g, ">", cx - 5, cy - 4, ac, lgfx::textdatum::top_left);
            g.fillRect(cx, cy + 4, 5, 1, ac);
        } else {
            g.drawRect(cx - 5, cy - 5, 10, 10, ac);
            g.fillRect(cx - 2, cy - 2, 4, 4, ac);
        }
    }
    hb_silk(g, i, cx, top + T + 2, on ? (uint8_t)PAL_WHITE : cb_silk(), PAL_BLACK);
}

/* GIBSON face — always ivory + dark glasses (never follows BOOTBOT variant). */
static void hb_gib_face(Gfx g, int cx, int cy, int s)
{
    const int x = cx - s / 2, y = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    cbf(g, x, y, s, s, PAL_IVORY);
    cbf(g, x, y, s, lip, PAL_WHITE);
    cbf(g, x, y + s - lip, s, lip, PAL_CHIN);
    const int ew = s / 5 > 2 ? s / 5 : 2;
    const int gap = s / 4 > ew ? s / 4 : ew;
    const int eyc0 = y + (s * 22) / 100;
    const int eyc = eyc0 > y + lip + 1 ? eyc0 : y + lip + 1;
    const int lx = x + gap, rx = x + s - gap - ew;
    cbf(g, lx, eyc, ew, ew, PAL_STEEL);
    cbf(g, rx, eyc, ew, ew, PAL_STEEL);
    const int pad = s >= 24 ? 2 : 1;
    const int lw = ew + pad;
    const int l0 = lx - pad, r0 = rx;
    const int tl = s >= 24 ? 3 : 1;
    const int lh = ew * 3 / 4 + pad > 2 ? ew * 3 / 4 + pad : 2;
    const int ly = eyc + (ew - lh) / 2;
    const int rl = l0 - tl >= x ? l0 - tl : x, rr = r0 + lw + tl <= x + s ? r0 + lw + tl : x + s;
    cbf(g, rl, ly - 1, rr - rl, 1, PAL_BLACK);
    cbf(g, l0, ly, lw, lh, PAL_BLACK);
    cbf(g, r0, ly, lw, lh, PAL_BLACK);
    if (lh >= 4) {
        cbf(g, l0, ly + lh - 1, 1, 1, PAL_IVORY);
        cbf(g, l0 + lw - 1, ly + lh - 1, 1, 1, PAL_IVORY);
        cbf(g, r0, ly + lh - 1, 1, 1, PAL_IVORY);
        cbf(g, r0 + lw - 1, ly + lh - 1, 1, 1, PAL_IVORY);
    }
    if (lw >= 6) {
        cbf(g, l0 + 1, ly + 1, 2, 1, PAL_STEEL);
        cbf(g, r0 + 1, ly + 1, 2, 1, PAL_STEEL);
    }
}

static void hb_module(Gfx g)
{
    const int mx = HB_MOD[0], my = HB_MOD[1], mw = HB_MOD[2], mh = HB_MOD[3];
    const uint8_t gold = 177, goldd = 104, steel = PAL_STEEL;
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    for (int y = my + 22; y < my + mh - 4; y += 6) {
        cbf(g, mx - 2, y, 4, 3, gold); cbf(g, mx - 2, y, 1, 3, goldd);
        cbf(g, mx + mw - 2, y, 4, 3, gold); cbf(g, mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 6) cbf(g, x, my + mh - 2, 3, 4, gold);
    int ax = mx + (mw - 28) / 2;
    for (int i = 0; i < 4; ++i) {
        cbf(g, ax, my + 3, 1, 9, gold); cbf(g, ax, my + 3, 4, 1, gold);
        cbf(g, ax + 3, my + 3, 1, 9, gold); cbf(g, ax + 3, my + 11, 4, 1, gold);
        ax += 7;
    }
    const int sx = mx + 3, sy = my + 15, sw = mw - 6, sh = mh - 19;
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0;
        if (b < 0) b = 0;
        if (b > sw) b = sw;
        cbf(g, sx, sy + r, a, 1, 178);
        cbf(g, sx + a, sy + r, b - a, 1, 110);
        cbf(g, sx + b, sy + r, sw - b, 1, 73);
    }
    cbf(g, sx, sy, sw, 1, 186); cbf(g, sx, sy, 1, sh, 186);
    const int D = sw - 18, cx = sx + sw / 2, cy = sy + D / 2 + 3;
    const uint8_t pin = PAL_SOFT;
    for (int k = 0; k < 5; ++k) {
        const int i = (D - 4 * (D / 6) - 1) / 2 + (D / 6) * k;
        cbf(g, cx - D / 2 - 2, cy - D / 2 + i, 2, 1, pin); cbf(g, cx + D / 2, cy - D / 2 + i, 2, 1, pin);
        cbf(g, cx - D / 2 + i, cy - D / 2 - 2, 1, 2, pin); cbf(g, cx - D / 2 + i, cy + D / 2, 1, 2, pin);
    }
    cbf(g, cx - D / 2, cy - D / 2, D, D, PAL_BLACK);
    cbf(g, cx - D / 2, cy - D / 2, D, 1, steel); cbf(g, cx - D / 2, cy + D / 2 - 1, D, 1, steel);
    cbf(g, cx - D / 2, cy - D / 2, 1, D, steel); cbf(g, cx + D / 2 - 1, cy - D / 2, 1, D, steel);
    const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
    hb_gib_face(g, cx, cy + bob - 1, D - 8);
    if (s_hb_sel == 14) { g.drawRect(mx - 3, my - 3, mw + 6, mh + 6, PAL_WHITE); g.drawRect(mx - 2, my - 2, mw + 4, mh + 4, accent()); }
    const int ty = cy + D / 2 + 4;
    for (int j = 0; j < 3; ++j)
        tiny_text(g, HB_MOD_SILK[j], cx - tiny_w(HB_MOD_SILK[j]) / 2, ty + 7 * j, 41);
}

static void hb_copper(Gfx g)
{
    const uint8_t trace = cb_trace();
    const uint8_t via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    for (int i = 0; i < 16; ++i) {
        const int n = HB_RN[i];
        if (i == 14) continue;                     /* old BOOTbot route */
        for (int q = 0; q + 1 < n; ++q)
            cb_line(g, HB_RT[i][q][0], HB_RT[i][q][1], HB_RT[i][q + 1][0], HB_RT[i][q + 1][1], trace);
        for (int q = 1; q + 1 < n; ++q) cb_via(g, HB_RT[i][q][0], HB_RT[i][q][1], via);
    }
}

static void hb_terminalbot(Gfx g)
{
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, 1, s_hb_sel == 15);
}

/* PICK.md paint_hub_target order: headers, copper, module, crown, chips, plate, top bar, edge */
static void paint_hubbot(Gfx g)
{
    hb_layout_sync();
    s_hb_sel = hb_sel_now();
    s_cb_col = accent();
    e_headers_hub(g);
    hb_copper(g);
    hb_module(g);
    gib_crown(g, HB_CROWN[0], HB_CROWN[1]);
    for (int i = 0; i < 14; ++i) hb_chip(g, i);
    hb_terminalbot(g);
    e_bar(g);
    e_frame(g);
}

/* ---- Touch. Hit boxes follow the drawn art: chip body + its silk. Returns 0..11 bot chip,
 * 12 TERMINAL/BOT, 13 CONTROL/BOT, 15 GIBSON module (+ its crown), -1 none. (14 = old BOOTbot: gone.) */
static int hb_hit(int tx, int ty)
{
    hb_layout_sync();
    for (int i = 0; i < 14; ++i) {
        const int T = 18;
        const int x = HB_CHIP[i][0] - T / 2, y = HB_CHIP[i][1];
        const int sh = HB_SILK2[i][0] ? 7 + HB_SILK_PITCH : 7;
        if (tx >= x - 1 && tx < x + T + 1 && ty >= y - 2 && ty < y + T + 2 + sh) return i;
    }
    if (tx >= HB_MOD[0] - 2 && tx < HB_MOD[0] + HB_MOD[2] + 2 && ty >= HB_MOD[1] && ty < HB_MOD[1] + HB_MOD[3] + 2)
        return 15;
    if (tx >= HB_CROWN[0] - 19 && tx < HB_CROWN[0] + 19 && ty >= HB_CROWN[1] - 4 && ty < HB_MOD[1]) return 15;
    return -1;
}

/* TERMINALBOT plate, log mode (s_pt_mode 0): command line -> keys, keyboard emblem -> keys,
 * "=" glossary -> prompt list. Same rects hb_terminalbot draws. Keys / list modes use the
 * shared pt_hit math on s_pt_bx/by/bw/bh, which hb_terminalbot sets to the plate screen. */
/* returns 0 (K_NONE; the key enum is declared later in botcorps_ui.cpp) */
static int hb_pt_hit0(int tx, int ty)
{
    const int sx = s_pt_bx, sy = s_pt_by, sw = s_pt_bw, sh = s_pt_bh;
    const int bwid = 34, bht = 16, byb = sy + sh - bht - 2;
    if (ty >= byb && ty < byb + bht) {
        const int kx = sx + sw - bwid * 2 - 6, gx = sx + sw - bwid - 2;
        if (tx >= kx && tx < kx + bwid) { s_pt_mode = 1; s_dirty = true; return 0; }
        if (tx >= gx && tx < gx + bwid) {
            /* prompt list opens on row 0 with it already on the command line (locked frame
             * hub-*-list); tap another row to pick it, tap the picked row again (or OK) to run. */
            s_pt_mode = 2; s_pt_sel = 0; s_pt_top = 0;
            int n = 0, cap = 0;
            const char *const *cmds = pt_list(&n);
            char *b = pt_buf(&cap);
            if (b && cmds && n > 0) snprintf(b, (size_t)cap, "%s", cmds[0]);
            s_dirty = true;
            return 0;
        }
    }
    if (ty >= sy + 2 && ty < sy + 14) { s_pt_mode = 1; s_dirty = true; return 0; }
    return 0;
}
