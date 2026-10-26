/* ============================================================================
 * HOME hub4 (SCR_GIBSON) — Mobandy 3:25–3:29 PM ET Oct 26. One edge-to-edge CONTROLbot-style PCB:
 * GIBSONbot big in the centre (ivory face, dark glasses, never theme-coloured) and four corner bots
 * wired to it: TL BOOTbot (black face, glossy white sunglasses; drawn, NOT tappable), TR SCANbot (gold
 * face), BL CONTROLbot (face = live UI theme), BR TERMINALbot (>_ face). Full-length side headers,
 * no rain on the pins, the shared 3-line Adv TERMINALBOT plate at the bottom.
 * Geometry: botcorps_hubbot_layout.inc (P + L). Included from botcorps_cyd_portrait.inc (CYD only).
 * ==========================================================================*/
#include "ui/botcorps_hubbot_layout.inc"

/* Highlight: HB4_* chip, 15 plate, -1 none (from the real hub state each paint). */
static int s_hb_sel = -1;
static int hb_sel_now(void)
{
    if (s_pt_mode != 0) return 15;
    if (s_hub_term) return -1;
    if (s_hub_zone == 3) return HB4_GIB;
    if (s_hub_zone == 0 && s_cursor == 0) return HB4_TERM;   /* keyboard cursor 0 = Terminal */
    if (s_hub_zone == 0 && s_cursor == 1) return HB4_CTRL;   /* keyboard cursor 1 = Control  */
    return -1;
}
static void hb_covers(void)
{
    const bool P = botcorps_cyd_orient != 0;
    const botcorps_cover_t *c = s_hub_term ? (P ? HT_COVER_P : HT_COVER_L) : (P ? HB_COVER_P : HB_COVER_L);
    const int n = s_hub_term ? 9 : 10;
    for (int i = 0; i < n; ++i) cover_add(c[i].x, c[i].y, c[i].w, c[i].h);
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


/* BOOTbot face: black, glossy white wrap-around sunglasses (matches the intro BOOTbot), never theme */
static void hb_boot_face(Gfx g, int cx, int cy, int s)
{
    const int x = cx - s / 2, y = cy - s / 2;
    cbf(g, x, y, s, s, PAL_BLACK);
    g.drawRect(x, y, s, s, sh_rgb(0x48, 0x48, 0x48));
    const uint8_t lens = sh_rgb(0xe4, 0xe7, 0xee), shade = sh_rgb(0xa9, 0xaf, 0xbb);
    const int lx = x + s / 6, lw = s - 2 * (s / 6), ly = y + s * 32 / 100, lh = s / 4 > 3 ? s / 4 : 3;
    cbf(g, x + 2, ly - 1, s - 4, 1, PAL_WHITE);                 /* rim */
    cbf(g, lx, ly, lw / 2 - 1, lh, lens);
    cbf(g, lx + lw / 2 + 1, ly, lw / 2 - 1, lh, lens);
    cbf(g, lx, ly + lh - 1, lw, 1, shade);
    cbf(g, lx + 2, ly + 1, 2, 1, PAL_WHITE);                    /* gloss glints */
    cbf(g, lx + lw / 2 + 3, ly + 1, 2, 1, PAL_WHITE);
    cbf(g, cx - s / 6, y + s * 72 / 100, s / 3, 1, PAL_STEEL);  /* mouth */
}
/* TERMINALbot face: black screen, >_ prompt in the theme ink */
static void hb_term_face(Gfx g, int cx, int cy, int s)
{
    const int x = cx - s / 2, y = cy - s / 2;
    cbf(g, x, y, s, s, PAL_BLACK);
    g.drawRect(x, y, s, s, PAL_STEEL);
    const uint8_t ac = accent();
    const int u = s / 8 > 1 ? s / 8 : 1, ox = x + s / 5, oy = cy - 2 * u;
    for (int k = 0; k < 2; ++k) { cbf(g, ox + k * u, oy + k * u, u, u, ac); cbf(g, ox + k * u, oy + (3 - k) * u, u, u, ac); }
    cbf(g, ox + 2 * u, oy + 2 * u - u / 2, u, u, ac);
    if ((millis() / 500u) % 2u == 0u) cbf(g, ox + 4 * u, oy + 3 * u, 2 * u, u, PAL_WHITE);
}

/* chip body: black die, gold castellations, steel rim; face; two-line silk inside the body */
static void hb4_chip(Gfx g, int i)
{
    const int x = HB_CHIPR[i][0], y = HB_CHIPR[i][1], w = HB_CHIPR[i][2], h = HB_CHIPR[i][3];
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
    const int fs = (w - 10 < h - 26 ? w - 10 : h - 26);
    const int cx = x + w / 2, cy = y + 5 + fs / 2;
    if (i == HB4_BOOT) hb_boot_face(g, cx, cy, fs);
    else if (i == HB4_SCAN) face_block(g, cx, cy, fs, sh_rgb(0xd4, 0xa9, 0x3c), false, false);  /* gold, never theme */
    else if (i == HB4_CTRL) face_block(g, cx, cy, fs, accent(), false, false);                   /* live theme */
    else hb_term_face(g, cx, cy, fs);
    const char *a = HB_SILK1[i];
    tiny_text(g, a, cx - tiny_w(a) / 2, y + h - 17, silk);
    tiny_text(g, "BOT", cx - tiny_w("BOT") / 2, y + h - 10, silk);
}

/* face geometry of the big centre GIBSON die (shared with the intro power-up) */
static void hb4_gib_geo(int *cx, int *cy, int *fs)
{
    const int x = HB_MOD[0], y = HB_MOD[1], w = HB_MOD[2], h = HB_MOD[3];
    int D = w - 14; if (D > h - 44) D = h - 44;
    *cx = x + w / 2; *cy = y + 16 + D / 2; *fs = D - 8;
}
static void hb_module(Gfx g)
{
    const int mx = HB_MOD[0], my = HB_MOD[1], mw = HB_MOD[2], mh = HB_MOD[3];
    const uint8_t gold = 177, goldd = 104;
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    for (int y = my + 20; y < my + mh - 6; y += 8) {
        cbf(g, mx - 2, y, 4, 3, gold); cbf(g, mx - 2, y, 1, 3, goldd);
        cbf(g, mx + mw - 2, y, 4, 3, gold); cbf(g, mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 8) cbf(g, x, my + mh - 2, 3, 4, gold);
    int ax = mx + (mw - 28) / 2;                                  /* antenna meander */
    for (int i = 0; i < 4; ++i) {
        cbf(g, ax, my + 3, 1, 9, gold); cbf(g, ax, my + 3, 4, 1, gold);
        cbf(g, ax + 3, my + 3, 1, 9, gold); cbf(g, ax + 3, my + 11, 4, 1, gold);
        ax += 7;
    }
    g.drawRect(mx, my, mw, mh, PAL_STEEL);
    int cx, cy, fs; hb4_gib_geo(&cx, &cy, &fs);
    const int D = fs + 8;
    cbf(g, cx - D / 2, cy - D / 2, D, D, PAL_BLACK);
    g.drawRect(cx - D / 2, cy - D / 2, D, D, PAL_STEEL);
    const int bob = ((millis() / 900u) % 2u) ? 1 : 0;
    hb_gib_face(g, cx, cy + bob - 1, fs);
    if (s_hb_sel == HB4_GIB) { g.drawRect(mx - 3, my - 3, mw + 6, mh + 6, PAL_WHITE); g.drawRect(mx - 2, my - 2, mw + 4, mh + 4, accent()); }
    static const char *const SILK[3] = { "GIBSONBOT", "WROOM-32", "HUB-LL19" };
    const int ty = cy + D / 2 + 5;
    for (int j = 0; j < 3; ++j)
        if (ty + 7 * j + 6 < my + mh - 3) tiny_text(g, SILK[j], cx - tiny_w(SILK[j]) / 2, ty + 7 * j, j ? (uint8_t)41 : (uint8_t)PAL_WHITE);
}

/* TERMINALbot centre module: same module art as GIBSON, >_ face in the theme ink */
static void ht_module(Gfx g)
{
    const int mx = HB_CHIPR[HB4_TERM][0], my = HB_CHIPR[HB4_TERM][1], mw = HB_CHIPR[HB4_TERM][2], mh = HB_CHIPR[HB4_TERM][3];
    const uint8_t gold = 177, goldd = 104;
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    for (int y = my + 20; y < my + mh - 6; y += 8) {
        cbf(g, mx - 2, y, 4, 3, gold); cbf(g, mx - 2, y, 1, 3, goldd);
        cbf(g, mx + mw - 2, y, 4, 3, gold); cbf(g, mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 8) cbf(g, x, my + mh - 2, 3, 4, gold);
    int ax = mx + (mw - 28) / 2;
    for (int i = 0; i < 4; ++i) {
        cbf(g, ax, my + 3, 1, 9, gold); cbf(g, ax, my + 3, 4, 1, gold);
        cbf(g, ax + 3, my + 3, 1, 9, gold); cbf(g, ax + 3, my + 11, 4, 1, gold);
        ax += 7;
    }
    g.drawRect(mx, my, mw, mh, PAL_STEEL);
    int D = mw - 14; if (D > mh - 38) D = mh - 38;
    const int cx = mx + mw / 2, cy = my + 16 + D / 2;
    hb_term_face(g, cx, cy, D);
    const int ty = cy + D / 2 + 4;
    tiny_text(g, "TERMINALBOT", cx - tiny_w("TERMINALBOT") / 2, ty, PAL_WHITE);
    if (ty + 13 < my + mh - 3) tiny_text(g, "WROOM-32", cx - tiny_w("WROOM-32") / 2, ty + 7, 41);
}
static void hb_copper(Gfx g)
{
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    const bool P = botcorps_cyd_orient != 0;
    const int nr = s_hub_term ? 3 : 4;
    const int8_t *RN = s_hub_term ? HT_RN : HB_RN;
    const int16_t (*RT)[4][2] = s_hub_term ? (P ? HT_RT_P : HT_RT_L) : HB_RT;
    for (int i = 0; i < nr; ++i) {
        const int n = RN[i];
        for (int q = 0; q + 1 < n; ++q) {
            cb_line(g, RT[i][q][0], RT[i][q][1], RT[i][q + 1][0], RT[i][q + 1][1], trace);
            cb_line(g, RT[i][q][0] + 1, RT[i][q][1] + 1, RT[i][q + 1][0] + 1, RT[i][q + 1][1] + 1, trace);
        }
        for (int q = 1; q + 1 < n; ++q) cb_via(g, RT[i][q][0], RT[i][q][1], via);
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
    hb_copper(g);
    if (s_hub_term) {                       /* TERMINALbot board: no GIBSON */
        ht_module(g);
        for (int i = 0; i < 3; ++i) hb4_chip(g, i);
    } else {
        hb_module(g);
        for (int i = 0; i < 4; ++i) hb4_chip(g, i);
    }
    hb_terminalbot(g);
    e_finish(g);
}

/* Touch: HB4_SCAN / HB4_CTRL / HB4_TERM / HB4_GIB, or -1. BOOTbot is deliberately NOT a target
 * (Mobandy 3:27 PM ET: "leave it inaccessible to click on the main screen"). Pad = chip body + 2. */
static int hb_hit(int tx, int ty)
{
    hb_layout_sync();
    const int last = s_hub_term ? HB4_TERM : HB4_GIB;
    for (int i = HB4_SCAN; i <= last; ++i) {
        const int x = HB_CHIPR[i][0] - 2, y = HB_CHIPR[i][1] - 2, w = HB_CHIPR[i][2] + 4, h = HB_CHIPR[i][3] + 4;
        if (tx >= x && tx < x + w && ty >= y && ty < y + h) return i;
    }
    return -1;
}
static bool hb_hit_box(int i, int *x, int *y, int *w, int *h)
{
    hb_layout_sync();
    if (i < 0 || i >= HB4_N || HB_CHIPR[i][2] == 0) return false;
    *x = HB_CHIPR[i][0] - 2; *y = HB_CHIPR[i][1] - 2; *w = HB_CHIPR[i][2] + 4; *h = HB_CHIPR[i][3] + 4;
    return true;
}

/* Adv plate idle hits: cmdline -> OSK, ≡ -> palette, reticle -> attack, sheet/cands. */
static int hb_pt_hit0(int tx, int ty)
{
    const int r = cyd_adv_hit_idle(tx, ty);
    return r < 0 ? 0 : r;
}
