/* ============================================================================
 * HOME hub7 (Mobandy 4:57 PM ET Oct 26): ONE board. GIBSON upper centre; BOOT (GIBSON inverted,
 * not tappable), SCAN (gold), CONTROL (theme), TERMINAL (silver) around it, every bot drawn with the
 * shared main-bot chip (drawBotChipK) as big as each slot allows, wired with copper.
 * hub8: TERMINALbot board = TERM + BOOT/SCAN/CTRL only (no GIBSON).
 * Geometry: botcorps_hubbot_layout.inc. s_hub_term = TERMINALbot board.
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

/* TERMINALbot: hub12 clean silver face, eyes + one >_ prompt (e_silver_face, edge.inc) */
static void hb_term_face(Gfx g, int cx, int cy, int s)
{
    e_metal_face(g, cx, cy, s, false);
}

/* hub7: every surrounding bot is the shared main-bot chip at the board's uniform bot scale */
static int hb_mb_of(int i)
{
    static const int M[HB4_N] = { MB_BOOT, MB_SCAN, MB_CTRL, MB_TERM, MB_GIB };
    return M[(i < 0 || i >= HB4_N) ? HB4_GIB : i];
}
static void hb4_chip(Gfx g, int i)
{
    const int x = HB_CHIPR[i][0], y = HB_CHIPR[i][1], w = HB_CHIPR[i][2], h = HB_CHIPR[i][3];
    if (w <= 0 || h <= 0) return;
    drawBotChipK(g, x, y, botcorps_cyd_orient != 0, hb_mb_of(i), HB_KS[i], nullptr, s_hb_sel == i && intro_on());   /* hub12: no persistent focus tint/ring */
}

/* face geo of the centre die (intro power-up + hub module) at the centre chip's scale */
static void hb4_gib_geo(int *cx, int *cy, int *fs)
{
    hb_layout_sync();
    const bool P = botcorps_cyd_orient != 0;
    int dcx, dcy, D, ty; mb_geo(P, &dcx, &dcy, &D, &ty, HB_KMID, true);
    const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
    *cx = HB_MOD[0] + dcx; *cy = HB_MOD[1] + dcy + bob - 1; *fs = D - mb_S(12, HB_KMID);
}
static void hb_module(Gfx g)
{
    drawBotChipK(g, HB_MOD[0], HB_MOD[1], botcorps_cyd_orient != 0, MB_GIB, HB_KMID, "JACK IN", s_hb_sel == HB4_GIB && intro_on());
}

/* TERMINALbot centre: the shared main-bot chip with the silver face (centre slot) */
static void ht_module(Gfx g)
{
    drawBotChipK(g, HB_MOD[0], HB_MOD[1], botcorps_cyd_orient != 0, MB_TERM, HB_KMID, "TTY0");
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
    const int W = EW();
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
    /* hub7: one layout — radial spoke vias around the centre chip */
    {
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

/* hub13 (Mobandy LOCKED 12:54 PM ET Oct 27, ref hub13-term-v3/term_P.png): PORTRAIT TERMINALbot board.
 * TERMINALBOT big at the home GIBSONBOT seat (78,30, k16) and only the 10 device bots (BOTS[2..11])
 * around it, each a full drawBotChipK chip (k7, 37 x 53) with its BOT_PAL face and full name, wired
 * to TERMINALBOT with 2 px copper. No decor beyond board / pins / copper / plate. Landscape unchanged. */
struct Ht3Chip { int8_t bot; int16_t x, y; int8_t n; Pt r[6]; };
enum { HT3_K = 7, HT3_N = 10 };
static const Ht3Chip HT3_P[HT3_N] = {
    { 2, 34, 30,  2, { {72,56}, {77,56} } },                                             /* DEAUTH */
    { 3, 34, 87,  2, { {72,113}, {77,113} } },                                           /* HACK */
    { 4, 34, 144, 4, { {72,170}, {84,170}, {90,164}, {90,152} } },                       /* FLOCK */
    { 5, 34, 201, 6, { {72,213}, {75,213}, {75,190}, {80,185}, {96,185}, {96,152} } },   /* DRONE */
    { 8, 169, 30,  2, { {168,56}, {163,56} } },                                          /* META */
    { 9, 169, 87,  2, { {168,113}, {163,113} } },                                        /* SKIMMER */
    { 10, 169, 144, 4, { {168,170}, {156,170}, {150,164}, {150,152} } },                 /* FLIPPER */
    { 11, 169, 201, 6, { {168,213}, {165,213}, {165,190}, {160,185}, {144,185}, {144,152} } }, /* SAMSUNG */
    { 6, 80, 201,  2, { {110,201}, {110,152} } },                                        /* AIR TAG */
    { 7, 123, 201, 2, { {130,201}, {130,152} } },                                        /* KEYFOB */
};
static bool ht3_on(void) { return s_hub_term && botcorps_cyd_orient != 0; }
static void ht3_paint(Gfx g)
{
    s_cb_col = accent();
    e_board(g);
    for (int i = 0; i < HT3_N; ++i) e_route2(g, HT3_P[i].r, HT3_P[i].n);
    for (int i = 0; i < HT3_N; ++i) {
        const Ht3Chip &c = HT3_P[i];
        drawBotChipK(g, c.x, c.y, true, MB_N, HT3_K, "", false, BOTS[c.bot].target, bot_color(c.bot));
    }
    drawMainBotChip(g, HB_CHIPR[HB4_TERM][0], HB_CHIPR[HB4_TERM][1], true, MB_TERM, "");
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, s_hb_sel == 15);
    e_finish(g);
}
/* device chip under (tx, ty) -> BOTS index (2..11), or -1. Pad = chip + 2 (as hb_hit). */
static int ht3_hit(int tx, int ty, int *cx = nullptr, int *cy = nullptr)
{
    if (!ht3_on()) return -1;
    const int w = mb_w(true, HT3_K), h = mb_h(true, HT3_K);
    for (int i = 0; i < HT3_N; ++i) {
        const Ht3Chip &c = HT3_P[i];
        if (tx >= c.x - 2 && tx < c.x + w + 2 && ty >= c.y - 2 && ty < c.y + h + 2) {
            if (cx) *cx = c.x;
            if (cy) *cy = c.y;
            return c.bot;
        }
    }
    return -1;
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
    if (ht3_on()) { ht3_paint(g); return; }        /* hub13: portrait TERMINALbot board */
    e_headers_hub(g);
    hb_rich(g);
    hb_copper(g);
    if (s_hub_term) {
        ht_module(g);
        for (int i = 0; i < 3; ++i) hb4_chip(g, i);   /* hub8: BOOT/SCAN/CTRL only, no GIBSON */
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
    for (int i = HB4_SCAN; i <= HB4_GIB; ++i) {   /* empty slots (term board GIBSON) skipped */
        if (ht3_on() && i != HB4_TERM) continue;   /* hub13: P term board has no BOOT/SCAN/CTRL chips */
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
