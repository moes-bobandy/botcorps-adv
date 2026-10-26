/* BOTCORPS_CYD portrait Control = "controlbot": the Control panel as a chip-bot ID board (v4 scatter mock,
 * /workspace/out/cyd-control-bot-mock). Included from botcorps_cyd_portrait.inc (BOTCORPS_CYD only).
 * Art ported 1:1 from Adv botcorps_id.cpp @ f1c08aa (board/gpio_pad/sq_face/bar_one/module teeth),
 * function parts reuse the live Control painters (cp_chip U1..U5) and cp_part art (CP / J1 / PWR). */

static inline void cbf(Gfx g, int x, int y, int w, int h, int c) { g.fillRect(x, y, w, h, (uint8_t)c); }
static uint8_t s_cb_col = 43;               /* controlbot body colour: set to the UI COLOR accent each paint (43 = idle default) */
static uint8_t cbp(uint16_t c565) { return botcorps_pal_fit(c565); }
static uint8_t cbmix(uint16_t a, uint16_t b, int num) { return cbp(mix16(a, b, num, 100)); }

/* ---------- geometry ---------- */
struct CbR { int x, y, w, h; };
/* v4 "scatter" layout: every coordinate comes from harness/layout.py, which checks overlaps,
 * trace clearance, 45-degree routing and staggered chip columns before emitting this table */
#include "ui/botcorps_cyd_ctlbot_layout.inc"
/* index -> rect, per orientation (CB_P portrait 240x320, CB_L landscape 320x240) */
static bool ctlbot_rect(int index, int *x, int *y, int *w, int *h)
{
    if (index < 0 || index > 8) return false;
    /* index 0 = centre CONTROLbot module (theme button); RECT[0] mirrors MOD */
    if (index == 0) {
        const int *M = botcorps_cyd_orient ? CB_P_MOD : CB_L_MOD;
        *x = M[0]; *y = M[1]; *w = M[2]; *h = M[3];
        return true;
    }
    const CbR &r = botcorps_cyd_orient ? CB_P_RECT[index] : CB_L_RECT[index];
    *x = r.x; *y = r.y; *w = r.w; *h = r.h;
    return true;
}

/* ---------- ported Adv ID art (palette path) ---------- */
static void cb_ring(Gfx g, int cx, int cy)
{
    const uint8_t gold = 177;
    cbf(g, cx - 1, cy - 3, 3, 7, gold);
    cbf(g, cx - 3, cy - 1, 7, 3, gold);
    cbf(g, cx - 2, cy - 2, 5, 5, gold);
    cbf(g, cx - 1, cy - 1, 3, 3, PAL_BLACK);
}
/* botcorps_id.cpp:990 gpio_pad, position-free */
static void cb_pad(Gfx g, int x, int y, int kind, bool on, bool tip)
{
    const uint16_t body = botcorps_rgb565(s_cb_col);
    uint8_t pad = 177;
    if (kind == 0) pad = cbp(0xE1A6);
    else if (kind == 1) pad = PAL_STEEL;
    else if (on) pad = tip ? (uint8_t)PAL_WHITE : s_cb_col;
    cbf(g, x, y, 7, 7, pad);
    cbf(g, x, y, 7, 1, (on && kind > 1) ? cbmix(body, 0xFFFF, 50) : (uint8_t)217);
    cbf(g, x + 2, y + 2, 3, 3, PAL_BLACK);
    if (on && kind > 1) cbf(g, x + 3, y + 3, 1, 1, tip ? s_cb_col : (uint8_t)PAL_WHITE);
}
/* botcorps_id.cpp:947 sq_face (non-Gibson branch) */
static void cb_face(Gfx g, int cx, int cy, int s)
{
    const uint16_t col = botcorps_rgb565(s_cb_col);
    const int x = cx - s / 2, y = cy - s / 2, lip = s >= 12 ? 2 : 1;
    cbf(g, x, y, s, s, s_cb_col);
    cbf(g, x, y, s, lip, cbmix(col, 0xFFFF, 45));
    cbf(g, x, y + s - lip, s, lip, cbmix(col, 0x0000, 45));
    const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;
    const bool bl = (millis() / 2600u) % 7u == 0u && (millis() % 2600u) < 140u;
    cbf(g, x + m, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, PAL_BLACK);
    cbf(g, x + m + 2 * c, y + c + (bl ? c - 1 : 0), c, bl ? 1 : c, PAL_BLACK);
}
/* ---- hub6 (Mobandy 4:30 PM ET Oct 26): ONE main-bot chip for every main board ----
 * CONTROLbot's centre module (frame, side/bottom pins, power-bar header, shaded face window,
 * die, name/info label) at EXACTLY CONTROLbot's size per orientation. Only the face inside the
 * die window and the two label lines switch per bot; every other pixel is identical. */
enum { MB_GIB = 0, MB_TERM = 1, MB_SCAN = 2, MB_CTRL = 3, MB_N = 4 };
static const int MB_W_L = 74, MB_H_L = 96, MB_W_P = 84, MB_H_P = 120;
static int mb_w(bool P) { return P ? MB_W_P : MB_W_L; }
static int mb_h(bool P) { return P ? MB_H_P : MB_H_L; }
static void e_metal_face(Gfx g, int cx, int cy, int s, bool gold);   /* edge.inc */
static void hb_gib_face(Gfx g, int cx, int cy, int s);               /* hubbot.inc */
static void hb_term_face(Gfx g, int cx, int cy, int s);              /* hubbot.inc */
/* die geometry relative to the chip origin: centre, die side D, label top */
static void mb_geo(bool P, int *dcx, int *dcy, int *D, int *ty)
{
    const int mw = mb_w(P), mh = mb_h(P), sw = mw - 6;
    int d = sw - 18;
    if (!P) { const int dmax = mh - 18 - 24; if (d > dmax) d = dmax; }
    if (d < 20) d = 20;
    *dcx = 3 + sw / 2; *dcy = 18 + d / 2 + 3; *D = d;
    int t = *dcy + d / 2 + 5;
    if (!P && t + 18 > mh - 4) t = mh - 22;
    *ty = t;
}
/* face rect (die window incl. its pin ticks) and label rect, chip-relative — the only areas that differ */
static void mb_face_rect(bool P, int *x, int *y, int *w, int *h)
{ int cx, cy, D, ty; mb_geo(P, &cx, &cy, &D, &ty); *x = cx - D / 2 - 2; *y = cy - D / 2 - 2; *w = D + 4; *h = D + 4; }
static void mb_label_rect(bool P, int *x, int *y, int *w, int *h)
{ int cx, cy, D, ty; mb_geo(P, &cx, &cy, &D, &ty); *x = 3; *y = ty - 1; *w = mb_w(P) - 6; *h = P ? 21 : 19; }
static const char *mb_name(int bot)
{
    static const char *const N[MB_N] = { "GIBSONBOT", "TERMINALBOT", "SCANBOT", "CONTROLBOT" };
    return N[(bot < 0 || bot >= MB_N) ? MB_CTRL : bot];
}
static void drawMainBotChip(Gfx g, int mx, int my, bool P, int bot, const char *info = nullptr)
{
    const uint8_t gold = 177, goldd = 104, steel = PAL_STEEL;
    const int mw = mb_w(P), mh = mb_h(P);
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    for (int y = my + 22; y < my + mh - 4; y += 6) {
        cbf(g, mx - 2, y, 4, 3, gold); cbf(g, mx - 2, y, 1, 3, goldd);
        cbf(g, mx + mw - 2, y, 4, 3, gold); cbf(g, mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 6) cbf(g, x, my + mh - 2, 3, 4, gold);
    /* power-bar header, stretched across the whole chip top (Mobandy 4:27): evenly spaced meanders */
    {
        const int span = mw - 8, n = (span - 4) / 7, used = n * 7 - 3;
        int ax = mx + (mw - used) / 2;
        for (int i = 0; i < n; ++i) {
            cbf(g, ax, my + 3, 1, 11, gold); cbf(g, ax, my + 3, 4, 1, gold);
            cbf(g, ax + 3, my + 3, 1, 11, gold); cbf(g, ax + 3, my + 13, 4, 1, gold);
            ax += 7;
        }
    }
    const int sx = mx + 3, sy = my + 18, sw = mw - 6, sh = mh - 22;
    for (int r = 0; r < sh; ++r) {
        int a = sw / 3 - r / 3, b = sw + sw / 3 - r / 3;
        if (a < 0) a = 0;
        if (b < 0) b = 0;
        if (b > sw) b = sw;
        cbf(g, sx, sy + r, a, 1, 178);
        cbf(g, sx + a, sy + r, b - a, 1, 110);
        cbf(g, sx + b, sy + r, sw - b, 1, 73);
    }
    cbf(g, sx, sy, sw, 1, 186);
    cbf(g, sx, sy, 1, sh, 186);
    int dcx, dcy, D, rty; mb_geo(P, &dcx, &dcy, &D, &rty);
    const int cx = mx + dcx, cy = my + dcy;
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
    const int fs = D - 12;
    if (bot == MB_GIB) hb_gib_face(g, cx, cy + bob - 1, fs);              /* ivory + dark glasses */
    else if (bot == MB_SCAN) e_metal_face(g, cx, cy + bob - 1, fs, true);  /* metallic gold */
    else if (bot == MB_TERM) hb_term_face(g, cx, cy + bob - 1, fs);        /* metallic silver */
    else cb_face(g, cx, cy + bob - 1, fs);                                 /* live theme */
    const char *nm = mb_name(bot);
    const char *in = info ? info : (bot == MB_CTRL ? theme().sname : "");
    const int ty = my + rty;
    if (!P) {
        const int keep = s_px; use_px(g, 6);
        txt(g, nm, cx, ty, 41, lgfx::textdatum::top_center);
        if (in[0]) txt(g, in, cx, ty + 9, accent(), lgfx::textdatum::top_center);
        s_px = keep;
    } else {
        tiny_text(g, nm, cx - tiny_w(nm) / 2, ty, 41);
        tiny_text(g, "WROOM-32", cx - tiny_w("WROOM-32") / 2, ty + 7, 41);
        if (in[0]) tiny_text(g, in, cx - tiny_w(in) / 2, ty + 14, accent());
    }
    OVL_BOX("CHIP", nm, mx, my, mw, mh);
}
/* hub6 (Mobandy 4:27): CONTROLbot board power-bar strip stretched across the whole top area
 * between the side pin columns, just under the top bar; header centred on a black window. */
static uint8_t cb_silk(void);
static void cb_power_strip(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    const int x0 = P ? 30 : 36, x1 = P ? 210 : 284, y = 27, h = P ? 8 : 10;
    const uint8_t gold = 177;
    const char *nm = "CONTROLBOT";
    const int tw = P ? tiny_w(nm) : (int)strlen(nm) * 6, tx0 = (x0 + x1) / 2 - tw / 2 - 4, tx1 = (x0 + x1) / 2 + tw / 2 + 4;
    cbf(g, x0, y, x1 - x0, h, PAL_BLACK);
    /* evenly spaced meanders (period 7) on each side of the header window, mirrored */
    const int side = tx0 - x0, n = (side - 2) / 7, used = n * 7 - 3, off = (side - used) / 2;
    for (int sd = 0; sd < 2; ++sd)
        for (int i = 0; i < n; ++i) {
            const int ax = sd == 0 ? x0 + off + i * 7 : x1 - off - used + i * 7;
            cbf(g, ax, y + 1, 1, h - 2, gold); cbf(g, ax, y + 1, 4, 1, gold);
            cbf(g, ax + 3, y + 1, 1, h - 2, gold); cbf(g, ax + 3, y + h - 2, 4, 1, gold);
        }
    if (P) tiny_text(g, nm, (x0 + x1) / 2 - tw / 2, y + 2, cb_silk());
    else { const int keep = s_px; use_px(g, 6); txt(g, nm, (x0 + x1) / 2, y + 1, cb_silk(), lgfx::textdatum::top_center); s_px = keep; }
    (void)tx1;
    OVL_BOX("CHIP", "PWRSTRIP", x0, y, x1 - x0, h);
}
static void cb_module(Gfx g, int mx, int my, int mw, int mh)
{
    (void)mw; (void)mh;
    drawMainBotChip(g, mx, my, botcorps_cyd_orient != 0, MB_CTRL);
}
/* botcorps_id.cpp:908 bar_one, palette path; label / value given */
static void cb_bar(Gfx g, int x, int y, int bw, const char *lab, const char *val, int v100)
{
    const uint16_t col = botcorps_rgb565(s_cb_col), silkc = botcorps_rgb565(PAL_STEEL);
    const int N = (bw - 4) / 6;
    if (v100 < 0) v100 = 0;
    if (v100 > 100) v100 = 100;
    const int lit = (N * v100 + 50) / 100;
    cbf(g, x, y - 8, bw, 8, cbp(0x0841));
    txt(g, lab, x, y - 8, cbp(mix16(silkc, col, 45, 100)), lgfx::textdatum::top_left);
    txt(g, val, x + bw - (int)strlen(val) * 6, y - 8, cbmix(col, 0xFFFF, 55), lgfx::textdatum::top_left);
    cbf(g, x, y, bw, 7, PAL_BLACK);
    cbf(g, x, y + 1, 2, 5, PAL_MUTED);
    cbf(g, x + bw - 2, y + 1, 2, 5, PAL_MUTED);
    const bool blink = ((millis() / 260u) % 2u) == 0u;
    for (int n = 0; n < N && n < 16; ++n) {
        uint8_t cc = cbmix(col, 0x0841, 72);                 /* unlit = dark UI COLOR, so the whole bar retints */
        if (n < lit) {
            if (n == lit - 1) cc = blink ? cbmix(col, 0xFFFF, 45) : s_cb_col;
            else if (n * 10 > N * 8) cc = cbmix(col, 0xFFFF, 25);
            else cc = s_cb_col;
        }
        cbf(g, x + 3 + n * 6, y + 1, 5, 5, cc);
    }
}

/* live cp_chip (botcorps_ui.cpp:6080) at S 28 so the centre module reads as the boss chip */
static void cb_chip(Gfx g, int index, bool on)
{
    int x, y, w, h;
    if (!ctlbot_rect(index, &x, &y, &w, &h)) return;
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t ac = accent(), dark = sh_rgb(0x2a, 0x2a, 0x32);
    cbf(g, x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
    if (on) { cbf(g, x - 3, y - 3, w + 6, h + 6, wash22(ac)); sel_rect(g, x - 3, y - 3, w + 6, h + 6); cbf(g, x, y, w, h, PAL_BLACK); }
    OVL_BOX("CHIP", "CB", x, y, w, h);
    const int S = 28, sx = x + (w - S) / 2, sy = y + 6;
    const int lit = (int)((millis() / 90u) % 6u);
    for (int k = 0; k < 6; ++k) {
        const int o = 3 + k * 4;
        const uint8_t pc = (on && lit == k) ? ac : gold;
        cbf(g, sx + o, sy - 3, 2, 3, pc); cbf(g, sx + o, sy + S, 2, 3, pc);
        cbf(g, sx - 3, sy + o, 3, 2, pc); cbf(g, sx + S, sy + o, 3, 2, pc);
    }
    cbf(g, sx, sy, S, S, on ? ac : dark);
    cbf(g, sx + 1, sy + 1, S - 2, S - 2, PAL_BLACK);
    const int fx = sx + 1, fy = sy + 1, fw = S - 2, fh = S - 2, mx = sx + S / 2, my = sy + S / 2;
    g.setClipRect(fx, fy, fw, fh);
    const int mode = botcorps_rain_mode();
    const uint8_t live = mode > 0 ? botcorps_rain_solid() : botcorps_rain_palette(0);
    if (index == 0) ui_color_bot(g, mx, my, fx, fy, fw, fh, 3);
    else if (index == 1) {
        if (s_mix == 1) draw_blob(g, mx, my + 1, 2, live, 0, false);
        else if (s_mix == 2) gl_icon(g, GL_AP, mx - 6, my - 6, false);
        else { draw_blob(g, mx - 5, my - 3, 2, live, 0, false); cbf(g, mx + 3, my + 2, 7, 7, PAL_DEV); cbf(g, mx + 5, my + 4, 3, 3, PAL_BLACK); }
    } else if (index == 2) shape_preview(g, mx, my);
    else if (index == 5) bg_preview(g, mx, my);
    else if (index == 7) {
        if (mode <= 0) {
            static const int BX[3] = { -8, 0, 8 };
            for (int k = 0; k < 3; ++k)
                botcorps_sprite_draw(*g.raw(), mx + BX[k] - g.ox, my + (k == 1 ? -4 : 4) - g.oy, 3, 2, bot_color(k * 4 + 1), false);
        } else botcorps_sprite_draw(*g.raw(), mx - g.ox, my - g.oy, 3, 3, botcorps_rain_solid(), false);
    }
    g.clearClipRect();
    char up[16];
    cp_short(index, up, (int)sizeof(up));
    { const int keep = s_px; use_px(g, 6);
      txt(g, up, x + w / 2, y + h - 9, on ? (uint8_t)PAL_WHITE : silk2, lgfx::textdatum::top_center);
      s_px = keep; }
}

/* ---------- small parts (live cp_part art, labels in 3x5 silk) ---------- */
static uint8_t cb_silk(void) { return cbmix(botcorps_rgb565(PAL_STEEL), 0xFFFF, 25); }
static void cb_lab(Gfx g, const char *s, int cx, int y, bool on)
{
    if (!botcorps_cyd_orient) {
        const int keep = s_px; use_px(g, 6);
        const int tw = (int)strlen(s) * 6;
        cbf(g, cx - tw / 2 - 1, y - 1, tw + 2, 9, PAL_BLACK);
        txt(g, s, cx, y, on ? (uint8_t)PAL_WHITE : cb_silk(), lgfx::textdatum::top_center);
        s_px = keep;
    } else {
        const int w = tiny_w(s);
        cbf(g, cx - w / 2 - 1, y - 1, w + 2, 7, PAL_BLACK);
        tiny_text(g, s, cx - w / 2, y, on ? (uint8_t)PAL_WHITE : cb_silk());
    }
}
static void cb_part(Gfx g, int i)
{
    int x, y, w, h;
    if (!ctlbot_rect(i, &x, &y, &w, &h)) return;
    const bool on = s_cursor == i;
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), rail = sh_rgb(0x02, 0x02, 0x03), metal = sh_rgb(0xb9, 0xbf, 0xcc);
    cbf(g, x - 3, y - 3, w + 6, h + 6, PAL_BLACK);
    if (on) { cbf(g, x - 3, y - 3, w + 6, h + 6, wash22(accent())); sel_rect(g, x - 4, y - 4, w + 8, h + 8); }
    const char *lab = "";
    if (i == 3) {
        const bool solid = botcorps_rain_mode() > 0;
        cbf(g, x, y + 2, w, 6, rail);
        for (int k = 0; k < 3; ++k) cbf(g, x + 3 + k * 6, y + 4, 2, 2, gold);
        const int jx = solid ? x + 9 : x + 1;
        cbf(g, jx, y, 11, 10, sh_rgb(0xff, 0xd4, 0x00));
        cbf(g, jx + 2, y + 2, 7, 3, sh_rgb(0xb8, 0x94, 0x00));
        lab = solid ? "SOLID" : "MIXED";
    } else if (i == 4) {
        cbf(g, x, y, w, h, sh_rgb(0x16, 0x16, 0x1b));
        for (int k = 0; k < 5; ++k) {
            cbf(g, x + 3 + k * 4, y - 2, 2, 2, metal);
            cbf(g, x + 3 + k * 4, y + h, 2, 2, metal);
            cbf(g, x - 2, y + 3 + k * 4, 2, 2, metal);
            cbf(g, x + w, y + 3 + k * 4, 2, 2, metal);
        }
        tiny_text(g, "CP", x + w / 2 - 3, y + 3, sh_rgb(0x9a, 0x9a, 0xa6));
        const int m = botcorps_rain_mode();
        cbf(g, x + 4, y + 13, w - 8, 5, m <= 0 ? botcorps_rain_palette(0) : botcorps_rain_solid());
        cbf(g, x + 2, y + 2, 2, 2, sh_rgb(0x5d, 0x5d, 0x6a));
        lab = RAIN_WORD[botcorps_rain_mode() < 0 ? 0 : botcorps_rain_mode()];
    } else if (i == 6) {
        const bool pon = s_bg_motion != 0;
        lab = pon ? "ON" : "OFF";
        if (!botcorps_cyd_orient) {
            /* hub6 L: wide toggle mirroring ORIENT; name above, live value inside (no stray square) */
            cbf(g, x - 3, y - 10, w + 6, h + 11, PAL_BLACK);
            if (on) { cbf(g, x - 3, y - 10, w + 6, h + 11, wash22(accent())); sel_rect(g, x - 4, y - 11, w + 8, h + 13); }
            cb_lab(g, "BG MOTION", x + w / 2, y - 9, on);
            cbf(g, x, y, w, h, 68);
            cbf(g, x + 1, y + 1, w - 2, h - 2, cbp(0x18E3));
            const int kw = 22, kx = pon ? x + w - 2 - kw : x + 2;
            cbf(g, kx, y + 2, kw, h - 4, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
            cbf(g, kx, y + 2, kw, 1, sh_rgb(0xd9, 0xd9, 0xdf));
            const int keep = s_px; use_px(g, 6);
            txt(g, lab, pon ? x + 2 + (w - 4 - kw) / 2 : x + 2 + kw + (w - 4 - kw) / 2, y + (h - 8) / 2 + 1,
                on ? (uint8_t)PAL_WHITE : cb_silk(), lgfx::textdatum::top_center);
            s_px = keep;
            OVL_BOX("CHIP", "BG MOTION", x, y, w, h);
            return;
        }
        cbf(g, x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
        cbf(g, x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
    }
    /* two-line silk: function name, then live value */
    static const char *const FN[9] = { "", "", "", "RAIN MODE", "RAIN COL", "", "BG MOTION", "", "" };
    const int ly = y + h + 4;
    OVL_BOX("CHIP", FN[i], x, y, w, h);
    cb_lab(g, FN[i], x + w / 2, ly, on);
    cb_lab(g, lab, x + w / 2, ly + (botcorps_cyd_orient ? 7 : 9), on);   /* hub6 L: Font0 rows don't touch */
}
/* NEW: ORIENT slide switch (LAND <-> PORT), gpio_pad contacts at each throw */
static void cb_orient(Gfx g)
{
    int x, y, w, h;
    ctlbot_rect(8, &x, &y, &w, &h);
    const bool on = s_cursor == 8, port = botcorps_cyd_orient != 0;
    const uint16_t body = botcorps_rgb565(s_cb_col);
    OVL_BOX("CHIP", "ORIENT", x, y, w, h);
    if (!port) {
        /* hub6 L: wide 72x16 toggle; ORIENT above, LAND on the knob-free side, nothing below */
        cbf(g, x - 3, y - 10, w + 6, h + 11, PAL_BLACK);
        if (on) { cbf(g, x - 3, y - 10, w + 6, h + 11, wash22(accent())); sel_rect(g, x - 4, y - 11, w + 8, h + 13); }
        cb_lab(g, "ORIENT", x + w / 2, y - 9, on);
        cbf(g, x, y, w, h, 68);
        cbf(g, x + 1, y + 1, w - 2, h - 2, cbp(0x18E3));
        const int kw = 22, kx = x + 2;                                     /* knob on the LAND end */
        cbf(g, kx, y + 2, kw, h - 4, sh_rgb(0x00, 0x7a, 0x9a));
        cbf(g, kx, y + 2, kw, h - 6, sh_rgb(0x00, 0xc8, 0xff));
        for (int k = 0; k < 3; ++k) cbf(g, kx + 7 + k * 3, y + 5, 1, h - 10, sh_rgb(0x00, 0x7a, 0x9a));
        const int keep = s_px; use_px(g, 6);
        txt(g, "LAND", x + 2 + kw + (w - 4 - kw) / 2, y + (h - 8) / 2 + 1, on ? (uint8_t)PAL_WHITE : cb_silk(), lgfx::textdatum::top_center);
        s_px = keep;
        (void)body;
        return;
    }
    cbf(g, x - 3, y - 10, w + 6, h + 20, PAL_BLACK);
    if (on) { cbf(g, x - 3, y - 10, w + 6, h + 20, wash22(accent())); sel_rect(g, x - 4, y - 11, w + 8, h + 22); }
    cb_lab(g, "ORIENT", x + w / 2, y - 8, on);
    cbf(g, x, y, w, h, 68);
    cbf(g, x + 1, y + 1, w - 2, h - 2, cbp(0x18E3));
    cb_pad(g, x + 3, y + 5, 2, !port, !port && on);
    cb_pad(g, x + w - 10, y + 5, 2, port, port && on);
    cbf(g, x + 12, y + 7, w - 24, 3, PAL_BLACK);                     /* slot */
    const int kx = port ? x + w - 12 - 26 : x + 12, ky = y + 3;           /* knob 26x11 */
    cbf(g, kx, ky, 26, 12, sh_rgb(0x00, 0x7a, 0x9a));
    cbf(g, kx, ky, 26, 10, sh_rgb(0x00, 0xc8, 0xff));
    cbf(g, kx, ky, 26, 1, sh_rgb(0x9a, 0xee, 0xff));
    for (int k = 0; k < 3; ++k) cbf(g, kx + 9 + k * 3, ky + 3, 1, 5, sh_rgb(0x00, 0x7a, 0x9a));
    /* tiny screen glyphs on the knob-free side */
    const uint8_t ic = cbmix(body, 0xFFFF, 40);
    if (port) g.drawRect(x + 13, y + 5, 11, 7, ic);       /* landscape glyph sits on the free LAND side */
    else      g.drawRect(x + w - 24, y + 4, 7, 10, ic);   /* portrait glyph on the free PORT side */
    cb_lab(g, "LAND", x + 14, y + h + 3, on && !port);
    cb_lab(g, "PORT", x + w - 14, y + h + 3, on && port);
    if (!botcorps_cyd_orient) {
        const int keep = s_px; use_px(g, 6);
        txt(g, port ? "PORT" : "LAND", x + w / 2, y + h - 8, PAL_WHITE, lgfx::textdatum::top_center);
        s_px = keep;
    } else if (!port) tiny_text(g, "LAND", x + 14 - tiny_w("LAND") / 2, y + h + 3, PAL_WHITE);
    else tiny_text(g, "PORT", x + w - 14 - tiny_w("PORT") / 2, y + h + 3, PAL_WHITE);
}
static void cb_tact(Gfx g, int bx, int by, const char *lab)
{
    cbf(g, bx - 2, by - 2, 16, 21, PAL_BLACK);
    cbf(g, bx, by, 12, 10, 41);
    cbf(g, bx + 1, by + 1, 10, 8, PAL_DEEP);
    cbf(g, bx + 3, by + 3, 6, 4, PAL_STEEL);
    tiny_text(g, lab, bx + 6 - tiny_w(lab) / 2, by + 12, cb_silk());
}
/* power bay: DC6.5 / 1117C / USB5V / C1 / R1-6, same parts as live Control at smaller pitch */
static void cb_dc(Gfx g, int x, int y)
{
    cbf(g, x - 1, y - 1, 24, 29, PAL_BLACK);
    cbf(g, x, y, 22, 20, sh_rgb(0x15, 0x15, 0x18));
    cbf(g, x + 3, y + 3, 16, 14, sh_rgb(0x05, 0x05, 0x06));
    g.fillCircle(x + 11, y + 10, 4, sh_rgb(0x2a, 0x2a, 0x30));
    tiny_text(g, "DC6.5", x + 11 - tiny_w("DC6.5") / 2 + 1, y + 22, cb_silk());
    cbf(g, x + 11 - tiny_w("DC6.5") / 2 + 1 + 3 * 4 + 1, y + 26, 1, 1, cb_silk());   /* TINY3 has no '.' glyph */
}
static void cb_reg(Gfx g, int x, int y)
{
    cbf(g, x - 1, y - 1, 24, 29, PAL_BLACK);
    cbf(g, x + 3, y, 16, 4, sh_rgb(0xa9, 0xaf, 0xbb));
    cbf(g, x, y + 4, 22, 11, sh_rgb(0x1a, 0x1a, 0x1f));
    for (int k = 0; k < 3; ++k) cbf(g, x + 3 + k * 7, y + 15, 3, 5, sh_rgb(0xa9, 0xaf, 0xbb));
    tiny_text(g, "1117C", x + 11 - tiny_w("1117C") / 2, y + 22, cb_silk());
}
static void cb_usb(Gfx g, int x, int y)
{
    cbf(g, x - 4, y - 1, 21, 29, PAL_BLACK);
    cbf(g, x, y, 13, 20, sh_rgb(0x9a, 0xa1, 0xb2));
    cbf(g, x + 1, y + 1, 11, 18, sh_rgb(0xd0, 0xd5, 0xdf));
    cbf(g, x + 3, y + 3, 7, 14, sh_rgb(0x0a, 0x0a, 0x10));
    cbf(g, x + 6, y + 6, 1, 8, sh_rgb(0xd4, 0xa9, 0x3c));
    tiny_text(g, "USB5V", x + 7 - tiny_w("USB5V") / 2, y + 22, cb_silk());
}
static void cb_c1(Gfx g, int x, int y)
{
    cbf(g, x - 1, y - 1, 18, 20, PAL_BLACK);
    cbf(g, x, y, 16, 10, sh_rgb(0xe0, 0xa2, 0x1c));
    cbf(g, x, y, 4, 10, sh_rgb(0x7a, 0x52, 0x08));
    cbf(g, x + 6, y + 3, 8, 4, sh_rgb(0x5a, 0x3a, 0x00));
    tiny_text(g, "C1", x + 8 - 3, y + 13, cb_silk());
}
static void cb_r16(Gfx g, int x, int y)
{
    cbf(g, x - 1, y - 1, 36, 16, PAL_BLACK);
    for (int k = 0; k < 6; ++k) {
        cbf(g, x + k * 6, y + 2, 5, 3, sh_rgb(0xb8, 0xb8, 0xc0));
        cbf(g, x + 2 + k * 6, y + 2, 1, 3, sh_rgb(0x20, 0x20, 0x24));
    }
    tiny_text(g, "R1-6", x + 17 - tiny_w("R1-6") / 2, y + 8, cb_silk());
}

/* ---------- header pins ---------- */
static const char *const CB_LP[12] = { "3V", "GND", "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16" };
static const char *const CB_RP[12] = { "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12" };
/* lit = pins controlbot "drives": XPT2046 touch (25/32/33) + TFT bus (12/14) */
static bool cb_lit_r(int k) { return k == 5 || k == 6 || k == 7 || k == 10 || k == 11; }
static int cb_tip(void) { return (int)((millis() / 400u) % 12u); }

/* ---------- copper ---------- */
static uint8_t cb_trace(void) { return cbmix(0x2187, botcorps_rgb565(s_cb_col), 45); }
static void cb_line(Gfx g, int x0, int y0, int x1, int y1, uint8_t c)
{
    if (x0 == x1) cbf(g, x0, y0 < y1 ? y0 : y1, 1, abs(y1 - y0) + 1, c);
    else if (y0 == y1) cbf(g, x0 < x1 ? x0 : x1, y0, abs(x1 - x0) + 1, 1, c);
    else g.drawLine(x0, y0, x1, y1, c);                   /* 45-degree run */
}
/* via: 3x3 plated ring, drilled centre */
static void cb_via(Gfx g, int x, int y, uint8_t c)
{
    cbf(g, x - 1, y - 1, 3, 3, c);
    cbf(g, x, y, 1, 1, PAL_BLACK);
}
/* individual organic route from part i to its module pad (from layout table) */
static int cb_route(int i, int *px, int *py)
{
    if (i < 0 || i > 8) return 0;
    const bool P = botcorps_cyd_orient != 0;
    const int n = P ? CB_P_RN[i] : CB_L_RN[i];
    for (int q = 0; q < n; ++q) { px[q] = P ? CB_P_RT[i][q][0] : CB_L_RT[i][q][0]; py[q] = P ? CB_P_RT[i][q][1] : CB_L_RT[i][q][1]; }
    return n;
}
static void cb_poly(Gfx g, const int *px, const int *py, int n, uint8_t c, uint8_t via)
{
    for (int q = 0; q + 1 < n; ++q) cb_line(g, px[q], py[q], px[q + 1], py[q + 1], c);
    for (int q = 1; q + 1 < n; ++q) cb_via(g, px[q], py[q], via);
}
static void cb_copper(Gfx g, uint8_t trace, uint8_t via)
{
    int px[6], py[6];
    for (int i = 1; i < 9; ++i) { const int n = cb_route(i, px, py); cb_poly(g, px, py, n, trace, via); }
    const bool P = botcorps_cyd_orient != 0;
    for (int d = 0; d < (P ? CB_P_DN : CB_L_DN); ++d) {
        const int n = P ? CB_P_DL[d] : CB_L_DL[d];
        for (int q = 0; q < n; ++q) { px[q] = P ? CB_P_DT[d][q][0] : CB_L_DT[d][q][0]; py[q] = P ? CB_P_DT[d][q][1] : CB_L_DT[d][q][1]; }
        cb_poly(g, px, py, n, trace, via);
    }
}

static void cb_serial(Gfx g, int x, int y)
{
    cbf(g, x - 1, y - 2, 87, 7, PAL_BLACK);
    tiny_text(g, "ID 13 LV 01", x, y, cb_silk());
    int bx = x + 46;
    char key[24];
    for (int kk = 0; bx < x + 84; ++kk) {
        snprintf(key, sizeof key, "controlbotbc%d", kk);
        uint32_t hsh = 2166136261u; for (const char *p = key; *p; ++p) { hsh ^= (uint8_t)*p; hsh *= 16777619u; }
        int w = 1 + (int)(hsh % 3u);
        if (bx + w > x + 84) w = x + 84 - bx;
        if (w > 0) cbf(g, bx, y - 1, w, 5, cb_silk());
        bx += w + 1 + (int)((hsh >> 8) % 2u);
    }
}

static void cb_hilite(Gfx g)
{
    int px[6], py[6];
    /* selected feed in the theme ink, 2 px wide, + travelling pulses (live Control behaviour) */
    if (s_cursor < 0 || s_cursor > 8) return;
    if (s_cursor == 0) {
        const int *M = botcorps_cyd_orient ? CB_P_MOD : CB_L_MOD;
        sel_rect(g, M[0] - 3, M[1] - 3, M[2] + 6, M[3] + 6);
        return;
    }
    const int n = cb_route(s_cursor, px, py);
    const uint8_t wc = th_wire();
    int len = 0;
    for (int q = 0; q + 1 < n; ++q) {
        const int dx = abs(px[q] - px[q + 1]), dy = abs(py[q] - py[q + 1]);
        len += dx > dy ? dx : dy;
        cb_line(g, px[q], py[q], px[q + 1], py[q + 1], wc);
        if (dy == 0) cb_line(g, px[q], py[q] + 1, px[q + 1], py[q + 1] + 1, wc);
        else cb_line(g, px[q] + 1, py[q], px[q + 1] + 1, py[q + 1], wc);
    }
    for (int q = 1; q + 1 < n; ++q) cb_via(g, px[q], py[q], cbmix(botcorps_rgb565(wc), 0xFFFF, 40));
    for (int k = 0; k < 3 && len > 0; ++k) {
        int d = (int)(((millis() * 120u) / 1000u + (uint32_t)(k * len / 3)) % (uint32_t)len);
        for (int q = 0; q + 1 < n; ++q) {
            const int dx = abs(px[q] - px[q + 1]), dy = abs(py[q] - py[q + 1]), l2 = dx > dy ? dx : dy;
            if (d <= l2) {
                cbf(g, px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0) - 1, py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0) - 1, 4, 4, accent());
                break;
            }
            d -= l2;
        }
    }
}

static void cb_meters(Gfx g, const int *M)
{
    if (!M || M[2] <= 0) return;           /* L hub5: meters omitted when MET w=0 */
    OVL_BOX("CHIP", "METERS", M[0], M[1] - 8, M[2], 45);
    char v[12];
    snprintf(v, sizeof v, "%d/%d", theme_ix(s_color) + 1, THEME_N);
    cb_bar(g, M[0], M[1], M[2], "THEME", v, (theme_ix(s_color) + 1) * 100 / THEME_N);
    { const uint32_t s = millis() / 1000u; snprintf(v, sizeof v, "%02d:%02d", (int)((s / 60u) % 100u), (int)(s % 60u));
      cb_bar(g, M[0], M[1] + 15, M[2], "UPTIME", v, (int)(100.f * (1.f - expf(-(float)s / 90.f)) + .5f)); }
    { const int f = (s_cursor >= 0 && s_cursor < CP_N + 2) ? s_cursor + 1 : 0; snprintf(v, sizeof v, "%d/%d", f, CP_N + 2);
      cb_bar(g, M[0], M[1] + 30, M[2], "FOCUS", v, f * 100 / (CP_N + 2)); }
}
static void cb_power(Gfx g, const int (*PW)[3], const int (*T)[2], const int *SER)
{
    for (int k = 0; k < 5; ++k) {
        const int x = PW[k][1], y = PW[k][2];
        switch (PW[k][0]) { case 0: cb_dc(g, x, y); break; case 1: cb_reg(g, x, y); break; case 2: cb_usb(g, x, y); break; case 3: cb_c1(g, x, y); break; default: cb_r16(g, x, y); }
    }
    cb_tact(g, T[0][0], T[0][1], "BOOT"); cb_tact(g, T[1][0], T[1][1], "EN");
    cb_serial(g, SER[0], SER[1]);
}

/* paint_ctlbot is defined in botcorps_cyd_edge.inc: it paints on the shared edge board
 * (board outline + full-length 20-pad headers + top bar), in both orientations. */
