/* ============================================================================
 * HOME hub5 — Mobandy 4:07 PM ET Oct 26. THREE selectable edge-to-edge PCB boards (A/B/C):
 * A radial corners, B backplane bus, C diamond/compass + ring. GIBSONbot square centre ~1.25x
 * the other four (ivory + dark glasses). BOOTbot black/white shades (not tappable). SCANbot
 * metallic gold. TERMINALbot metallic silver. CONTROLbot = live theme. Rich decorative copper.
 * Geometry: botcorps_hubbot_layout.inc. s_hub5_var selects (default A). s_hub_term = TERMINALbot board.
 * Included from botcorps_cyd_portrait.inc (CYD only).
 * ==========================================================================*/
#include "ui/botcorps_hubbot_layout.inc"

static int s_hb_sel = -1;
static int hb_sel_now(void)
{
    if (s_pt_mode != 0) return 15;
    if (s_hub_term) return -1;
    if (s_hub_zone == 3) return HB4_GIB;
    if (s_hub_zone == 0 && s_cursor == 0) return HB4_TERM;
    if (s_hub_zone == 0 && s_cursor == 1) return HB4_CTRL;
    return -1;
}
static void hb_covers(void) { cover_add(0, 0, EW(), EH()); }

/* GIBSON face — always ivory + dark glasses */
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

/* BOOTbot: exact inverse of GIBSONbot face — same outline/glasses shape, colors flipped
 * (black face where GIBSON is ivory; white/glossy glasses where GIBSON's are dark). */
static void hb_boot_face(Gfx g, int cx, int cy, int s)
{
    const int x = cx - s / 2, y = cy - s / 2;
    const int lip = s >= 12 ? 2 : 1;
    cbf(g, x, y, s, s, PAL_BLACK);                                    /* ivory -> black */
    cbf(g, x, y, s, lip, sh_rgb(0x28, 0x28, 0x28));                   /* white lip -> dark */
    cbf(g, x, y + s - lip, s, lip, sh_rgb(0x48, 0x48, 0x48));         /* chin -> mid grey */
    const int ew = s / 5 > 2 ? s / 5 : 2;
    const int gap = s / 4 > ew ? s / 4 : ew;
    const int eyc0 = y + (s * 22) / 100;
    const int eyc = eyc0 > y + lip + 1 ? eyc0 : y + lip + 1;
    const int lx = x + gap, rx = x + s - gap - ew;
    cbf(g, lx, eyc, ew, ew, PAL_STEEL);                               /* same lens wells */
    cbf(g, rx, eyc, ew, ew, PAL_STEEL);
    const int pad = s >= 24 ? 2 : 1;
    const int lw = ew + pad;
    const int l0 = lx - pad, r0 = rx;
    const int tl = s >= 24 ? 3 : 1;
    const int lh = ew * 3 / 4 + pad > 2 ? ew * 3 / 4 + pad : 2;
    const int ly = eyc + (ew - lh) / 2;
    const int rl = l0 - tl >= x ? l0 - tl : x, rr = r0 + lw + tl <= x + s ? r0 + lw + tl : x + s;
    cbf(g, rl, ly - 1, rr - rl, 1, PAL_WHITE);                        /* bridge: dark -> white */
    cbf(g, l0, ly, lw, lh, PAL_WHITE);                                /* glasses: black -> white */
    cbf(g, r0, ly, lw, lh, PAL_WHITE);
    if (lh >= 4) {
        cbf(g, l0, ly + lh - 1, 1, 1, PAL_BLACK);                     /* corner ticks inverted */
        cbf(g, l0 + lw - 1, ly + lh - 1, 1, 1, PAL_BLACK);
        cbf(g, r0, ly + lh - 1, 1, 1, PAL_BLACK);
        cbf(g, r0 + lw - 1, ly + lh - 1, 1, 1, PAL_BLACK);
    }
    if (lw >= 6) {
        cbf(g, l0 + 1, ly + 1, 2, 1, sh_rgb(0xe8, 0xec, 0xf2));       /* gloss glints on white lenses */
        cbf(g, r0 + 1, ly + 1, 2, 1, sh_rgb(0xe8, 0xec, 0xf2));
    }
}

/* TERMINALbot: metallic silver face with dark >_ prompt etched on */
static void hb_term_face(Gfx g, int cx, int cy, int s)
{
    e_metal_face(g, cx, cy, s, false);                 /* silver plate */
    const int x = cx - s / 2, y = cy - s / 2;
    const uint8_t ink = PAL_BLACK;
    /* hub6: on the big main chip the prompt sits under the eyes (small chips unchanged) */
    const int u = s >= 36 ? s / 12 : (s / 8 > 1 ? s / 8 : 1), ox = x + s / 5, oy = s >= 36 ? y + 3 * s / 5 - u : cy - 2 * u;
    for (int k = 0; k < 2; ++k) {
        cbf(g, ox + k * u, oy + k * u, u, u, ink);
        cbf(g, ox + k * u, oy + (3 - k) * u, u, u, ink);
    }
    cbf(g, ox + 2 * u, oy + 2 * u - u / 2, u, u, ink);
    if ((millis() / 500u) % 2u == 0u) cbf(g, ox + 4 * u, oy + 3 * u, 2 * u, u, sh_rgb(0x3a, 0x40, 0x4c));
}

/* chip body + face + two-line silk */
static void hb4_chip(Gfx g, int i)
{
    const int x = HB_CHIPR[i][0], y = HB_CHIPR[i][1], w = HB_CHIPR[i][2], h = HB_CHIPR[i][3];
    if (w <= 0 || h <= 0) return;
    const bool on = s_hb_sel == i;
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = on ? (uint8_t)PAL_WHITE : cb_silk();
    cbf(g, x, y, w, h, cbp(0x18E3));
    for (int k = 0; k < 5; ++k) {
        const int py = y + 6 + k * ((h - 14) / 4);
        cbf(g, x - 2, py, 3, 2, gold); cbf(g, x + w - 1, py, 3, 2, gold);
        const int px = x + 5 + k * ((w - 12) / 4);
        cbf(g, px, y - 2, 2, 3, gold); cbf(g, px, y + h - 1, 2, 3, gold);
    }
    g.drawRect(x, y, w, h, on ? accent() : (uint8_t)PAL_STEEL);
    if (on) sel_rect(g, x - 3, y - 3, w + 6, h + 6);
    OVL_BOX("CHIP", HB_SILK1[i], x, y, w, h);
    const int sm = botcorps_cyd_orient ? 26 : 18;          /* L: one Font0 line */
    const int fs = (w - 10 < h - sm ? w - 10 : h - sm);
    const int cx = x + w / 2, cy = y + 4 + fs / 2;
    if (i == HB4_BOOT) hb_boot_face(g, cx, cy, fs);
    else if (i == HB4_SCAN) e_metal_face(g, cx, cy, fs, true);     /* metallic gold */
    else if (i == HB4_CTRL) face_block(g, cx, cy, fs, accent(), false, false);
    else hb_term_face(g, cx, cy, fs);                               /* metallic silver */
    /* Landscape: Font0 (~6 px) single-line name — tiny 3x5 is unreadable on 320x240.
     * Portrait keeps two-line tiny silk (same as before). Short L names for fit. */
    if (!botcorps_cyd_orient) {
        static const char *const SHORT[5] = { "BOOT", "SCAN", "CTRL", "TERM", "GIBSON" };
        const char *a = SHORT[i];
        const int keep = s_px; use_px(g, 6);
        txt(g, a, cx, y + h - 9, silk, lgfx::textdatum::top_center);
        s_px = keep;
    } else {
        const char *a = HB_SILK1[i];
        tiny_text(g, a, cx - tiny_w(a) / 2, y + h - 17, silk);
        tiny_text(g, "BOT", cx - tiny_w("BOT") / 2, y + h - 10, silk);
    }
}

/* face geo of centre die (intro power-up + hub module): the shared main-bot chip die (hub6) */
static void hb4_gib_geo(int *cx, int *cy, int *fs)
{
    const bool P = botcorps_cyd_orient != 0;
    int dcx, dcy, D, ty; mb_geo(P, &dcx, &dcy, &D, &ty);
    const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
    *cx = HB_MOD[0] + dcx; *cy = HB_MOD[1] + dcy + bob - 1; *fs = D - 12;
}
static void hb_module(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    const int mx = HB_MOD[0], my = HB_MOD[1], mw = HB_MOD[2], mh = HB_MOD[3];
    drawMainBotChip(g, mx, my, P, MB_GIB, "JACK IN");
    if (s_hb_sel == HB4_GIB) {
        g.drawRect(mx - 4, my - 4, mw + 8, mh + 8, PAL_WHITE);
        g.drawRect(mx - 3, my - 3, mw + 6, mh + 6, accent());
    }
}

/* TERMINALbot centre: the shared main-bot chip with the silver face */
static void ht_module(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    drawMainBotChip(g, HB_CHIPR[HB4_TERM][0], HB_CHIPR[HB4_TERM][1], P, MB_TERM, "TTY0");
}

/* route copper between chips */
static void hb_copper(Gfx g)
{
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    const int nr = HB_RNcount;
    const int8_t *RN = HB_ROUTE_N;
    const int16_t (*RT)[4][2] = HB_RT;
    for (int i = 0; i < nr; ++i) {
        const int n = RN[i];
        for (int q = 0; q + 1 < n; ++q) {
            cb_line(g, RT[i][q][0], RT[i][q][1], RT[i][q + 1][0], RT[i][q + 1][1], trace);
            cb_line(g, RT[i][q][0] + 1, RT[i][q][1] + 1, RT[i][q + 1][0] + 1, RT[i][q + 1][1] + 1, trace);
        }
        for (int q = 1; q + 1 < n; ++q) cb_via(g, RT[i][q][0], RT[i][q][1], via);
    }
}

/* Rich decorative PCB: ground pours, vias, SMD, test pads, silk nets — never over chips/pins/plate */
static bool hb_over_chip(int x, int y)
{
    for (int i = 0; i < HB4_N; ++i) {
        const int cx = HB_CHIPR[i][0], cy = HB_CHIPR[i][1], cw = HB_CHIPR[i][2], ch = HB_CHIPR[i][3];
        if (cw <= 0) continue;
        if (x >= cx - 4 && x < cx + cw + 4 && y >= cy - 4 && y < cy + ch + 4) return true;
    }
    if (x >= HB_TBOT[0] - 2 && x < HB_TBOT[0] + HB_TBOT[2] + 2 &&
        y >= HB_TBOT[1] - 2 && y < HB_TBOT[1] + HB_TBOT[3] + 2) return true;
    return false;
}
/* hub6: a silk label is only printed where no chip (+4px keep-out) sits under any of its pixels */
static bool hb_rect_free(int x, int y, int w, int h)
{
    for (int yy = y; yy <= y + h; yy += 2)
        for (int xx = x; xx <= x + w; xx += 2)
            if (hb_over_chip(xx < x + w ? xx : x + w - 1, yy < y + h ? yy : y + h - 1)) return false;
    return true;
}
static void hb_rich(Gfx g)
{
    const int W = EW(), H = EH();
    const uint8_t tr = cb_trace();
    const uint8_t via = cbmix(botcorps_rgb565(tr), 0xFFFF, 40);
    const uint8_t pour = cbmix(botcorps_rgb565(tr), 0x0000, 55);
    const uint8_t silk = cb_silk();
    const uint8_t smd = sh_rgb(0x2a, 0x2e, 0x36);
    const uint8_t pad = sh_rgb(0xb8, 0x9a, 0x3c);
    /* keep clear of side headers (x 0..28 / W-28..W) and top bar */
    const int x0 = 30, x1 = W - 30, y0 = 34, y1 = HB_TBOT[1] - 6;

    /* ground-pour hatch (sparse, not rain) */
    for (int y = y0 + 4; y < y1; y += 14) {
        for (int x = x0 + 4; x < x1; x += 18) {
            if (hb_over_chip(x, y)) continue;
            cbf(g, x, y, 5, 1, pour);
            cbf(g, x + 2, y - 2, 1, 5, pour);
        }
    }
    /* via grid */
    for (int y = y0 + 10; y < y1; y += 22) {
        for (int x = x0 + 10; x < x1; x += 26) {
            if (hb_over_chip(x, y)) continue;
            cb_via(g, x, y, via);
        }
    }
    /* variant-specific bus art */
    if (!s_hub_term && s_hub5_var == HUB5_B) {
        /* horizontal backplane spine */
        const int by = botcorps_cyd_orient ? 140 : 104;   /* hub6 L: between A/B bus routes 96/112 */
        for (int x = x0 + 8; x < x1 - 8; x += 2) {
            if (hb_over_chip(x, by)) continue;
            cbf(g, x, by, 2, 2, tr);
            cbf(g, x, by + 3, 2, 1, pour);
        }
        if (botcorps_cyd_orient) tiny_text(g, "BUS0", x0 + 4, by - 8, silk);
        else { const int keep = s_px; use_px(g, 6); txt(g, "BUS0", x0 + 4, by - 10, silk, lgfx::textdatum::top_left); s_px = keep; }
    } else if (!s_hub_term && s_hub5_var == HUB5_C) {
        /* ring bus around Gibson */
        const int cx = HB_MOD[0] + HB_MOD[2] / 2, cy = HB_MOD[1] + HB_MOD[3] / 2;
        const int rx = botcorps_cyd_orient ? 70 : 78, ry = botcorps_cyd_orient ? 62 : 44;
        static const int8_t OX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
        static const int8_t OY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
        int px = cx, py = cy - ry;
        for (int s = 0; s < 8; ++s) {
            const int nx = cx + OX[s] * rx, ny = cy + OY[s] * ry;
            cb_line(g, px, py, nx, ny, tr);
            cb_via(g, nx, ny, via);
            px = nx; py = ny;
        }
        if (botcorps_cyd_orient) tiny_text(g, "RING", cx + 24, cy - ry - 10, silk);   /* hub6: beside N chip */
        else { const int keep = s_px; use_px(g, 6); txt(g, "RING", cx, 156, silk, lgfx::textdatum::top_center); s_px = keep; }   /* hub6 L: between BOOT|TERM pair */
    } else {
        /* A: radial spoke stubs from centre midpoints */
        const int cx = HB_MOD[0] + HB_MOD[2] / 2, cy = HB_MOD[1] + HB_MOD[3] / 2;
        for (int a = 0; a < 4; ++a) {
            const int dx = (a == 1) ? 28 : (a == 3) ? -28 : 0;
            const int dy = (a == 0) ? -28 : (a == 2) ? 28 : 0;
            if (!hb_over_chip(cx + dx, cy + dy)) cb_via(g, cx + dx, cy + dy, via);
        }
    }
    /* SMD resistors + caps in free pockets */
    static const int16_t SMD[][4] = {
        { 78, 88, 10, 4 }, { 152, 88, 10, 4 }, { 78, 188, 10, 4 }, { 152, 188, 10, 4 },
        { 110, 70, 4, 8 }, { 126, 70, 4, 8 }, { 110, 196, 4, 8 }, { 70, 150, 8, 4 },
        { 160, 150, 8, 4 }, { 100, 150, 12, 3 }
    };
    for (int i = 0; i < 10; ++i) {
        int x = SMD[i][0], y = SMD[i][1], w = SMD[i][2], h = SMD[i][3];
        if (!botcorps_cyd_orient) { x = x * 320 / 240; y = y * 240 / 320; }
        if (x < x0 || x + w > x1 || y < y0 || y + h > y1) continue;
        if (hb_over_chip(x + w / 2, y + h / 2)) continue;
        cbf(g, x, y, w, h, smd);
        cbf(g, x, y, 2, h, pad); cbf(g, x + w - 2, y, 2, h, pad);
    }
    /* test pads */
    static const int16_t TP[][2] = {
        { 88, 100 }, { 148, 100 }, { 88, 176 }, { 148, 176 },
        { 120, 90 }, { 120, 186 }, { 70, 140 }, { 170, 140 }
    };
    for (int i = 0; i < 8; ++i) {
        int x = TP[i][0], y = TP[i][1];
        if (!botcorps_cyd_orient) { x = x * 320 / 240; y = y * 240 / 320; }
        if (x < x0 || x > x1 || y < y0 || y > y1) continue;
        if (hb_over_chip(x, y)) continue;
        cbf(g, x - 2, y - 2, 5, 5, pad);
        cbf(g, x - 1, y - 1, 3, 3, smd);
    }
    /* silk net labels — portrait tiny OK; landscape Font0 or drop (Mobandy 4:13) */
    if (botcorps_cyd_orient) {
        if (hb_rect_free(x0 + 2, y0 + 2, tiny_w("3V3"), 5)) tiny_text(g, "3V3", x0 + 2, y0 + 2, silk);
        if (hb_rect_free(x1 - tiny_w("GND") - 2, y0 + 2, tiny_w("GND"), 5)) tiny_text(g, "GND", x1 - tiny_w("GND") - 2, y0 + 2, silk);
        if (hb_rect_free(x0 + 2, y1 - 10, tiny_w("NET-H"), 5)) tiny_text(g, "NET-H", x0 + 2, y1 - 10, silk);
    } else {
        const int keep = s_px; use_px(g, 6);
        if (hb_rect_free(x0 + 2, y0 + 2, 18, 8)) txt(g, "3V3", x0 + 2, y0 + 2, silk, lgfx::textdatum::top_left);
        if (hb_rect_free(x1 - 20, y0 + 2, 18, 8)) txt(g, "GND", x1 - 2, y0 + 2, silk, lgfx::textdatum::top_right);
        s_px = keep;
    }
}

static void hb_terminalbot(Gfx g)
{
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, s_hb_sel == 15);
}
static void paint_hubbot(Gfx g)
{
    hb_layout_sync();
    s_hb_sel = hb_sel_now();
    s_cb_col = accent();
    e_headers_hub(g);
    hb_rich(g);
    hb_copper(g);
    if (s_hub_term) {
        ht_module(g);
        for (int i = 0; i < 3; ++i) hb4_chip(g, i);
    } else {
        hb_module(g);
        for (int i = 0; i < 4; ++i) hb4_chip(g, i);
    }
    hb_terminalbot(g);
    e_finish(g);
}

/* Touch: SCAN/CTRL/TERM/GIB. BOOT not a target. Pad = chip + 2. */
static int hb_hit(int tx, int ty)
{
    hb_layout_sync();
    const int last = s_hub_term ? HB4_TERM : HB4_GIB;
    for (int i = HB4_SCAN; i <= last; ++i) {
        const int x = HB_CHIPR[i][0] - 2, y = HB_CHIPR[i][1] - 2;
        const int w = HB_CHIPR[i][2] + 4, h = HB_CHIPR[i][3] + 4;
        if (w <= 4 || h <= 4) continue;
        if (tx >= x && tx < x + w && ty >= y && ty < y + h) return i;
    }
    return -1;
}
static bool hb_hit_box(int i, int *x, int *y, int *w, int *h)
{
    hb_layout_sync();
    if (i < 0 || i >= HB4_N || HB_CHIPR[i][2] == 0) return false;
    *x = HB_CHIPR[i][0] - 2; *y = HB_CHIPR[i][1] - 2;
    *w = HB_CHIPR[i][2] + 4; *h = HB_CHIPR[i][3] + 4;
    return true;
}
static int hb_pt_hit0(int tx, int ty)
{
    const int r = cyd_adv_hit_idle(tx, ty);
    return r < 0 ? 0 : r;
}
