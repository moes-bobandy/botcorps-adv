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
/* index -> rect, portrait 240x320 */
static bool ctlbot_rect(int index, int *x, int *y, int *w, int *h)
{
    if (index < 0 || index > 8) return false;
    const CbR &r = CB_P_RECT[index];
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
/* botcorps_id.cpp:1027-1078 WROOM module + die + face, scaled to the boss size
 * (portrait 84x120, die 60, face 48; landscape 74x104, die 50, face 38), centred on the board's x axis */
static void cb_module(Gfx g, int mx, int my, int mw, int mh)
{
    const uint8_t gold = 177, goldd = 104, steel = PAL_STEEL;
    cbf(g, mx, my, mw, mh, cbp(0x18E3));
    for (int y = my + 22; y < my + mh - 4; y += 6) {
        cbf(g, mx - 2, y, 4, 3, gold); cbf(g, mx - 2, y, 1, 3, goldd);
        cbf(g, mx + mw - 2, y, 4, 3, gold); cbf(g, mx + mw + 1, y, 1, 3, goldd);
    }
    for (int x = mx + 7; x < mx + mw - 6; x += 6) cbf(g, x, my + mh - 2, 3, 4, gold);
    int ax = mx + (mw - 48) / 2;
    for (int i = 0; i < 7; ++i) {
        cbf(g, ax, my + 3, 1, 11, gold); cbf(g, ax, my + 3, 4, 1, gold);
        cbf(g, ax + 3, my + 3, 1, 11, gold); cbf(g, ax + 3, my + 13, 4, 1, gold);
        ax += 7;
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
    cb_face(g, cx, cy + bob - 1, D - 12);
    const int ty = cy + D / 2 + 5;
    tiny_text(g, "CONTROL", cx - tiny_w("CONTROL") / 2, ty, 41);
    tiny_text(g, "WROOM-32", cx - tiny_w("WROOM-32") / 2, ty + 7, 41);
    tiny_text(g, "FCC 2AC7Z", cx - tiny_w("FCC 2AC7Z") / 2, ty + 14, 41);
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
    txt(g, up, x + w / 2, y + h - 9, on ? (uint8_t)PAL_WHITE : silk2, lgfx::textdatum::top_center);
}

/* ---------- small parts (live cp_part art, labels in 3x5 silk) ---------- */
static uint8_t cb_silk(void) { return cbmix(botcorps_rgb565(PAL_STEEL), 0xFFFF, 25); }
static void cb_lab(Gfx g, const char *s, int cx, int y, bool on)
{
    const int w = tiny_w(s);
    cbf(g, cx - w / 2 - 1, y - 1, w + 2, 7, PAL_BLACK);
    tiny_text(g, s, cx - w / 2, y, on ? (uint8_t)PAL_WHITE : cb_silk());
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
        cbf(g, x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
        cbf(g, x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
        lab = pon ? "ON" : "OFF";
    }
    /* two-line silk: function name, then live value */
    static const char *const FN[9] = { "", "", "", "RAIN MODE", "RAIN COL", "", "BG MOTION", "", "" };
    const int ly = y + h + 4;
    cb_lab(g, FN[i], x + w / 2, ly, on);
    cb_lab(g, lab, x + w / 2, ly + 7, on);
}
/* NEW: ORIENT slide switch (LAND <-> PORT), gpio_pad contacts at each throw */
static void cb_orient(Gfx g)
{
    int x, y, w, h;
    ctlbot_rect(8, &x, &y, &w, &h);
    const bool on = s_cursor == 8, port = botcorps_cyd_orient != 0;
    const uint16_t body = botcorps_rgb565(s_cb_col);
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
    if (!port) tiny_text(g, "LAND", x + 14 - tiny_w("LAND") / 2, y + h + 3, PAL_WHITE);
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
    const int n = CB_P_RN[i];
    for (int q = 0; q < n; ++q) { px[q] = CB_P_RT[i][q][0]; py[q] = CB_P_RT[i][q][1]; }
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
    for (int i = 0; i < 9; ++i) { const int n = cb_route(i, px, py); cb_poly(g, px, py, n, trace, via); }
    for (int d = 0; d < CB_P_DN; ++d) {
        const int n = CB_P_DL[d];
        for (int q = 0; q < n; ++q) { px[q] = CB_P_DT[d][q][0]; py[q] = CB_P_DT[d][q][1]; }
        cb_poly(g, px, py, n, trace, via);
    }
}

static void cb_topbar(Gfx g, int W)
{
    const uint8_t ac = accent();
    cbf(g, 0, 0, W, 16, PAL_BLACK);
    if (theme().grad == 0) cbf(g, 0, 16, W, 1, ac);
    else for (int x = 0; x < W; x += 4) cbf(g, x, 16, 4, 1, th_ink_at(x, W));
    for (int k = 0; k < CP_N; ++k) cbf(g, 6 + k * 6, 7, 4, 3, k == s_cursor ? ac : sh_rgb(0x2a, 0x2a, 0x34));
    const bool bsel = s_cursor == CP_N + 1;
    gib_crown(g, W / 2, 3);
    if (bsel) g.drawRect(W / 2 - 18, 0, 36, 16, PAL_WHITE);
    const bool tsel = s_cursor == CP_N;
    const uint8_t tc = tsel ? (uint8_t)PAL_WHITE : ac;
    const int tx = W - 34;
    g.drawRect(tx, 3, 18, 11, tc); cbf(g, tx, 3, 18, 2, tc);
    g.drawLine(tx + 3, 7, tx + 6, 9, tc); g.drawLine(tx + 6, 9, tx + 3, 11, tc);
    cbf(g, tx + 8, 11, 5, 1, tc);
    if (tsel) g.drawRect(tx - 2, 1, 22, 14, PAL_WHITE);
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

static void paint_ctlbot(Gfx g)
{
    const bool P = true;   /* portrait only (landscape Control keeps paint_control) */
    const int W = 240, H = 320;
    s_cb_col = accent();   /* controlbot IS the UI COLOR: face, body, meters, lit pins follow the theme accent */
    const uint16_t body = botcorps_rgb565(s_cb_col);
    const uint8_t gold = 177, trace = cb_trace(), rule = cbmix(0x0841, body, 40), silkc = cb_silk();
    cb_topbar(g, W);
    /* board outline only (rain shows through the PCB), bot-colour silk rule, corner rings */
    g.drawRect(2, 18, W - 4, H - 19, gold);
    g.drawRect(4, 20, W - 8, H - 23, rule);
    cb_ring(g, 9, 25); cb_ring(g, W - 10, 25); cb_ring(g, 9, H - 9); cb_ring(g, W - 10, H - 9);
    const int tip = cb_tip();
    const uint8_t via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    cb_copper(g, trace, via);   /* scattered parts: one organic 45-degree trace per part, vias at every bend */
    cb_hilite(g);
    if (P) {
        /* ---- headers: black strips + gpio_pad, labels 3x5 tight to each pad ---- */
        cbf(g, 4, 34, 11, 12 * 23 - 12, PAL_BLACK); cbf(g, 4, 34, 11, 1, 68);
        cbf(g, 225, 34, 11, 12 * 23 - 12, PAL_BLACK); cbf(g, 225, 34, 11, 1, 68);
        for (int k = 0; k < 12; ++k) {
            const int y = 36 + k * 23;
            const bool lr = cb_lit_r(k);
            cb_pad(g, 6, y, k == 0 ? 0 : k == 1 ? 1 : 2, false, false);
            cb_pad(g, 227, y, 2, lr, lr && (k == tip || k + 6 == tip));
            const int lw = tiny_w(CB_LP[k]), rw = tiny_w(CB_RP[k]);
            cbf(g, 15, y, lw + 3, 7, PAL_BLACK);
            tiny_text(g, CB_LP[k], 17, y + 1, silkc);
            cbf(g, 222 - rw - 1, y, rw + 3, 7, PAL_BLACK);
            tiny_text(g, CB_RP[k], 223 - rw, y + 1, lr ? cbmix(body, 0xFFFF, 30) : silkc);
        }
        /* ---- name row ---- */
        cbf(g, 31, 21, 62, 9, PAL_BLACK);
        txt(g, "controlbot", 32, 22, cbmix(body, 0xFFFF, 35), lgfx::textdatum::top_left);
        cbf(g, 208 - tiny_w("ESP32 DEV BOARD") - 1, 23, tiny_w("ESP32 DEV BOARD") + 2, 7, PAL_BLACK);
        tiny_text(g, "ESP32 DEV BOARD", 208 - tiny_w("ESP32 DEV BOARD"), 24, silkc);
        /* ---- dead-centre bot, meters tucked top-left, ORIENT + BOOT/EN + power scattered ---- */
        cb_module(g, CB_P_MOD[0], CB_P_MOD[1], CB_P_MOD[2], CB_P_MOD[3]);
        cb_meters(g, CB_P_MET);
        cb_orient(g);
        cb_power(g, CB_P_PWR, CB_P_TACT, CB_P_SER);
    }
    /* function parts on top: five live chips + switch bank */
    static const int CH[5] = { 0, 1, 2, 5, 7 };
    for (int k = 0; k < 5; ++k) {
        int x, y, w, h;
        ctlbot_rect(CH[k], &x, &y, &w, &h);
        (void)x; (void)y; (void)w; (void)h;
        cb_chip(g, CH[k], s_cursor == CH[k]);
    }
    cb_part(g, 3); cb_part(g, 4); cb_part(g, 6);
}


/* Rain covers: the 17 px top bar plus the board art (generated: CB_P_COVER, every rain cell that the board
 * paints into, merged into <= 23 boxes), so stepped rain on the direct portrait path never overdraws it. */
static void layout_covers_ctlbot(void)
{
    cover_add(0, 0, 240, 17);
    for (int i = 0; i < CB_P_COVER_N; ++i) cover_add(CB_P_COVER[i][0], CB_P_COVER[i][1], CB_P_COVER[i][2], CB_P_COVER[i][3]);
}
