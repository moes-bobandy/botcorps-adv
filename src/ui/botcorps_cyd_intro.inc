/* ============================================================================
 * BOOTbot boot intro (PICK.md idea A-fixed, out/cyd-bootbot-intro-mock): runs from setup()
 * (botcorps_ui_begin arms it) before the hub. 47 steps, 6300 ms. Ported from
 * tools-bc/intro3_harness.cpp paint_intro / paint_hub_staged / compose_wipe, on the shared edge
 * board (e_headers / e_frame from botcorps_cyd_edge.inc). Portrait is PICK's geometry 1:1;
 * landscape keeps the same parts with the centre column shifted to x 160, the right-hand parts
 * pinned to the right rail, and the rails / J1 / regulator rows fitted into 240 px.
 * The hand-off frame IS paint_hubbot (the real hub), so the intro lands on the hub with no cut.
 * BOOTbot exists only here; the hub never paints him.
 * Included from botcorps_cyd_portrait.inc (BOTCORPS_CYD only), after botcorps_cyd_hubbot.inc.
 * ==========================================================================*/
static const uint32_t INTRO_MS = 6300;

struct IntroSt {
    int scr = 0;          /* 0 BOOTbot board, 1 hub staged power-up, 2 hub (hand-off) */
    float src = 0, feed = 0, rail = 0, drop = 0, surge = 0, hsurge = 0, wp = 0;
    int boot = 0, bhalo = 0, glint = 0, parts = 0, con = 0, login = 0, lk = 0, flash = 0, exitb = 0, pips = 0, rain = 0;
    int hin = 0, term = 0, ctl = 0, halo = -1, plate = 0, gib = 0, gk = 0, crown = 0, seat = 0, title = 0;
};
/* replays the harness push() sequence (PICK.md timeline, 47 steps / 6300 ms) and returns the
 * state live at `e` ms after the intro started; *idx gets the step index */
static void intro_state_at(uint32_t e, IntroSt &out, int *idx)
{
    IntroSt s;
    int i = 0;
    uint32_t acc = 0;
#define IP(ms) do { if (e < acc + (ms)) { out = s; if (idx) *idx = i; return; } acc += (ms); ++i; } while (0)
    IP(450);
    s.src = 0.5f; IP(60);
    s.src = 1; IP(60);
    s.boot = 1; s.bhalo = 1; s.pips = 2; IP(160);
    s.glint = 1; IP(260);
    s.bhalo = 0; s.feed = 0.5f; s.pips = 3; IP(60);
    s.feed = 1; s.pips = 4; IP(60);
    s.rail = 0.34f; s.pips = 6; IP(60);
    s.rail = 0.67f; s.pips = 9; s.parts = 1; s.rain = 1; IP(60);
    s.rail = 1; s.pips = 12; s.drop = 0.5f; IP(60);
    s.drop = 1; s.con = 1; IP(380);
    s.login = 1;
    for (int k = 2; k <= 11; k += 3) { s.lk = k; IP(55); }
    s.lk = 14; IP(180);
    s.login = 2;
    for (int k = 2; k <= 6; k += 2) { s.lk = k; IP(k == 6 ? 180 : 60); }
    s.login = 3;
    for (int k = 2; k <= 6; k += 2) { s.lk = k; IP(k == 6 ? 200 : 65); }
    s.login = 4;
    s.flash = 1; IP(120); s.flash = 2; IP(90); s.flash = 1; IP(90); s.flash = 2; IP(140);
    s.surge = 0.35f; IP(60);
    s.surge = 0.7f; IP(60);
    s.surge = 1; IP(60);
    s.exitb = 1; IP(100);
    s.wp = 0.30f; IP(60); s.wp = 0.60f; IP(60); s.wp = 0.85f; IP(60);
    { IntroSt h; h.scr = 1; h.hin = 1; h.rain = 1; s = h; }
    IP(140);
    s.hin = 0; s.term = 1; s.halo = 12; IP(170);
    s.ctl = 1; s.halo = 13; IP(170);
    s.halo = -1; s.hsurge = 0.5f; IP(60);
    s.hsurge = 1; s.halo = 14; IP(80);
    s.gib = 1; s.gk = 4; s.crown = 1; IP(100);
    s.gk = 2; IP(80);
    s.gk = 0; s.halo = -1; IP(240);
    s.plate = 2;
    for (int k = 4; k <= 12; k += 4) { s.seat = k; IP(70); }
    s.title = 1; IP(100);
    s.title = 2; IP(450);
    { IntroSt h; h.scr = 2; h.rain = 1; s = h; }
    IP(0xFFFFu);
#undef IP
    out = s; if (idx) *idx = i;
}
static bool intro_on(void) { return s_intro_until && (int32_t)(millis() - s_intro_until) < 0; }
static uint32_t intro_ms(void)
{
    const uint32_t t0 = s_intro_until - INTRO_MS;
    const uint32_t e = millis() - t0;
    return e > INTRO_MS ? INTRO_MS : e;
}
static int intro_step(void) { IntroSt s; int k = 0; intro_state_at(intro_ms(), s, &k); return k; }
static void intro_state(IntroSt &s) { intro_state_at(intro_ms(), s, nullptr); }

/* ---- geometry, per orientation (portrait = PICK.md §3-§5 exactly) ---- */
struct IntroGeo {
    int BX, BY, BD, RL, RR, RY0, RY1, FY, USBX, USBY, LEDY, LEDL, LEDR, TCY, TCL, TCR;
    int REGX, REGY, C1X, C1Y, R16X, R16Y, J1X, J1Y, PX, PY, PW, PH, WY;
};
static const IntroGeo IG_P = { 88, 80, 64, 38, 200, 32, 268, 111, 113, 30, 48, 58, 176, 124, 55, 173,
                               52, 228, 168, 233, 102, 236, 97, 302, 44, 176, 152, 38, 194 };
static const IntroGeo IG_L = { 128, 80, 64, 38, 280, 32, 182, 111, 153, 30, 48, 58, 256, 124, 55, 253,
                               50, 150, 250, 155, -1, 0, 137, 224, 84, 172, 152, 38, 190 };
static const IntroGeo &ig(void) { return botcorps_cyd_orient ? IG_P : IG_L; }
static int ig_legy(void) { return ig().J1Y + 2; }

/* mirror about BOOTbot's centre column (x 119.5 in portrait) */
static void ib_mf(Gfx g, int x, int y, int w, int h, uint8_t c)
{
    const IntroGeo &G = ig();
    cbf(g, x, y, w, h, c);
    cbf(g, 2 * G.BX + G.BD - x - w, y, w, h, c);
}
static void ib_bootbot(Gfx g, const IntroSt &s)
{
    const IntroGeo &G = ig();
    const int BX = G.BX, BY = G.BY, BD = G.BD;
    const uint8_t ac = accent();
    const bool on = s.boot != 0;
    const uint8_t gph = sh_rgb(0x3c, 0x3c, 0x3c), gph2 = sh_rgb(0x2c, 0x2c, 0x2c);
    const uint8_t edge = on ? ac : gph, pin = on ? ac : sh_rgb(0x48, 0x48, 0x48);
    const uint8_t root = on ? cbmix(botcorps_rgb565(ac), 0x0000, 45) : gph2;
    if (s.bhalo) { cbf(g, BX - 9, BY - 9, BD + 18, BD + 18, wash22(ac)); sel_rect(g, BX - 9, BY - 9, BD + 18, BD + 18); }
    cbf(g, BX - 5, BY - 5, BD + 10, BD + 10, PAL_BLACK);
    const int np = (BD - 16) / 8 + 1;
    for (int q = 0; q < np; ++q) {
        const int o = 7 + 8 * q;
        cbf(g, BX + o, BY - 4, 2, 4, pin); cbf(g, BX + o, BY + BD, 2, 4, pin);
        cbf(g, BX - 4, BY + o, 4, 2, pin); cbf(g, BX + BD, BY + o, 4, 2, pin);
        cbf(g, BX + o, BY - 1, 2, 1, root); cbf(g, BX + o, BY + BD, 2, 1, root);
        cbf(g, BX - 1, BY + o, 1, 2, root); cbf(g, BX + BD, BY + o, 1, 2, root);
    }
    cbf(g, BX, BY, BD, BD, PAL_BLACK);
    g.drawRect(BX, BY, BD, BD, edge);
    ib_mf(g, BX + 2, BY + 2, BD / 2 - 2, 1, cbmix(0x0000, 0xFFFF, 18));
    ib_mf(g, BX + 2, BY + BD - 4, BD / 2 - 2, 2, cbmix(0x0000, 0x0841, 40));
    const uint8_t lens = on ? sh_rgb(0xe4, 0xe7, 0xee) : sh_rgb(0x9c, 0x9c, 0x9c);
    const uint8_t hi = on ? (uint8_t)PAL_WHITE : sh_rgb(0xc8, 0xc8, 0xc8);
    const uint8_t shade = on ? sh_rgb(0xa9, 0xaf, 0xbb) : sh_rgb(0x74, 0x74, 0x74);
    const int cx = BX + BD / 2;
    const int lx = BX + 8, lw = (cx - 3) - lx, ly = BY + 22, lh = 12;
    const int rx = BX + 6;
    ib_mf(g, rx, ly - 2, cx - rx, 2, hi);
    ib_mf(g, lx, ly, lw, lh, lens);
    ib_mf(g, lx, ly, lw, 1, hi);
    ib_mf(g, lx, ly + lh - 2, lw, 2, shade);
    ib_mf(g, lx + lw, ly, cx - (lx + lw), 2, hi);
    ib_mf(g, lx, ly + lh - 1, 2, 1, PAL_BLACK); ib_mf(g, lx, ly + lh - 2, 1, 1, PAL_BLACK);
    ib_mf(g, lx + lw - 1, ly + lh - 1, 1, 1, PAL_BLACK);
    if (on && s.glint) {
        for (int d = 0; d < 6; ++d) ib_mf(g, lx + 3 + d, ly + lh - 4 - d, 2, 1, PAL_WHITE);
        for (int d = 0; d < 3; ++d) ib_mf(g, lx + 3 + 6 + d, ly + lh - 4 - d, 1, 1, PAL_WHITE);
    }
    ib_mf(g, cx - 8, BY + 45, 8, 1, on ? (uint8_t)PAL_STEEL : sh_rgb(0x30, 0x30, 0x30));
    cbf(g, BX + BD - 6, BY - 1, 6, 6, on ? ac : sh_rgb(0x30, 0x30, 0x30));   /* ONE corner LED */
    use_px(g, 6);
    txt(g, "BOOT", cx, BY + BD + 9, PAL_WHITE, lgfx::textdatum::top_center);
    txt(g, "BOT", cx, BY + BD + 18, PAL_WHITE, lgfx::textdatum::top_center);
}

/* U2 TERMINAL: castellated plate built into the board (PICK.md §5) */
static void ib_plate(Gfx g, bool on, int hot)
{
    const IntroGeo &G = ig();
    const int X = G.PX, Y = G.PY, Wd = G.PW, Hh = G.PH, dx = G.PX - 44;
    const uint8_t ac = accent(), silkc = cb_silk();
    cbf(g, X, Y, Wd, Hh, cbp(0x18E3));
    g.drawRect(X, Y, Wd, Hh, GOLD);
    g.drawRect(X + 1, Y + 1, Wd - 2, Hh - 2, e_rule());
    static const int PCT[6] = { 54, 66, 78, 90, 102, 114 };
    static const int PCB[6] = { 58, 70, 82, 94, 106, 118 };
    const int M = 2 * X + Wd - 4;              /* mirror: a -> M - a (236 - a in portrait) */
    for (int i = 0; i < 6; ++i) {
        cbf(g, PCT[i] + dx, Y - 2, 4, 3, GOLD); cbf(g, M - (PCT[i] + dx), Y - 2, 4, 3, GOLD);
        cbf(g, PCB[i] + dx, Y + Hh - 1, 4, 3, GOLD); cbf(g, M - (PCB[i] + dx), Y + Hh - 1, 4, 3, GOLD);
    }
    const int sx = X + 6, sw = Wd - 12;
    tiny_text(g, "U2 TERMINAL", sx + 1, Y + 3, cbmix(botcorps_rgb565(ac), 0xFFFF, 35));
    static const char *const TP[2] = { "RX", "TX" };
    for (int q = 0; q < 2; ++q) { const int px = sx + sw - 38 + q * 20; cb_pad(g, px, Y + 2, 2, on, false); tiny_text(g, TP[q], px + 9, Y + 3, silkc); }
    const int sy = Y + 10, sh = Hh - 13;
    g.fillRect(sx, sy, sw, sh, PAL_BLACK);
    g.drawRect(sx, sy, sw, sh, PAL_LINE);
    if (hot) g.drawRect(X - 1, Y - 1, Wd + 2, Hh + 2, hot == 2 ? (uint8_t)PAL_WHITE : ac);
}
static void ib_cmd(Gfx g, int x, int y, int w, const char *line, bool caret)
{
    g.fillRect(x, y, w, 12, PAL_BLACK);
    g.drawRect(x, y, w, 12, PAL_LINE);
    txt(g, line, x + 2, y + 2, PAL_WHITE, lgfx::textdatum::top_left);
    if (caret && ((millis() / 400u) % 2u) == 0u) {
        const int cx = x + 2 + (int)strlen(line) * 6;
        if (cx + 4 <= x + w - 2) g.fillRect(cx, y + 2, 4, 8, accent());
    }
}
static void ib_screen(Gfx g, const IntroSt &s)
{
    const IntroGeo &G = ig();
    const int sx = G.PX + 6, sw = G.PW - 12, sy = G.PY + 10, sh = G.PH - 13;
    if (!s.con) return;
    use_px(g, 6);
    const int cx = sx + 2, cy = sy + 2, cw = sw - 4, lx = sx + 3, l0 = sy + 15;
    char buf[48];
    if (s.login == 0) { ib_cmd(g, cx, cy, cw, "", true); pt_draw_log_line(g, "[BOOT] 3V3 rail good", lx, l0); return; }
    if (s.login == 1) { snprintf(buf, sizeof buf, "> %.*s", s.lk, "connect GIBSON"); ib_cmd(g, cx, cy, cw, buf, true); pt_draw_log_line(g, "[BOOT] 3V3 rail good", lx, l0); return; }
    if (s.login == 2) { snprintf(buf, sizeof buf, "login: %.*s", s.lk, "gibson"); ib_cmd(g, cx, cy, cw, buf, true); pt_draw_log_line(g, "[NET] GIBSON carrier ok", lx, l0); return; }
    if (s.login == 3) {
        char st[8] = { 0 };
        for (int i = 0; i < s.lk && i < 6; ++i) st[i] = '*';
        snprintf(buf, sizeof buf, "password: %s", st); ib_cmd(g, cx, cy, cw, buf, true); pt_draw_log_line(g, "login: gibson", lx, l0); return;
    }
    /* ACCESS GRANTED: inside the plate's screen only */
    const uint8_t ac = accent();
    if (s.flash == 1) g.fillRect(sx + 1, sy + 1, sw - 2, sh - 2, ac);
    else { g.fillRect(sx + 1, sy + 1, sw - 2, sh - 2, PAL_BLACK); g.drawRect(sx + 2, sy + 2, sw - 4, sh - 4, ac); }
    use_px(g, 8);
    txt(g, "ACCESS GRANTED", sx + sw / 2, sy + sh / 2 - 4, s.flash == 1 ? (uint8_t)PAL_BLACK : ac, lgfx::textdatum::top_center);
    use_px(g, 6);
}
static void ib_led(Gfx g, int x, int y, bool on, uint8_t col, const char *lab)
{
    cbf(g, x - 1, y - 1, 8, 6, PAL_BLACK);
    cbf(g, x, y, 6, 4, sh_rgb(0xd9, 0xd9, 0xdf));
    cbf(g, x + 1, y + 1, 4, 2, on ? col : sh_rgb(0x3a, 0x0a, 0x12));
    if (on) cbf(g, x + 1, y + 1, 1, 1, PAL_WHITE);
    cb_lab(g, lab, x + 3, y + 7, false);
}
static void ib_topbar(Gfx g, const IntroSt &s)
{
    const int W = EW();
    const uint8_t ac = accent();
    g.fillRect(0, 0, W, 26, PAL_BLACK);
    for (int i = 0; i < 12; ++i) g.fillRect(30 + i * 8, 9, 5, 5, i < s.pips ? ac : sh_rgb(0x2a, 0x2a, 0x34));
    use_px(g, 6);
    txt(g, s.pips >= 12 ? "3V3" : s.boot ? "PWR" : "OFF", W - 34, 6, s.boot ? (uint8_t)PAL_WHITE : cb_silk(), lgfx::textdatum::top_left);
    g.fillRect(0, 24, W, 1, sb_tone(3));
}
static void ib_rail(Gfx g, int x, float r, uint8_t base, uint8_t lit)
{
    const IntroGeo &G = ig();
    cbf(g, x, G.RY0, 2, G.RY1 - G.RY0 + 1, base);
    if (r <= 0) return;
    const int dn = (int)(r * (G.RY1 - G.FY));
    const int up = dn < G.FY - G.RY0 ? dn : G.FY - G.RY0;
    cbf(g, x, G.FY - up, 2, up + dn + 1, lit);
    if (r < 1) {
        cbf(g, x - 1, G.FY + dn - 1, 4, 4, PAL_WHITE);
        if (up < G.FY - G.RY0) cbf(g, x - 1, G.FY - up - 2, 4, 4, PAL_WHITE);
    }
}
static bool ib_reach(float r, int y)
{
    const IntroGeo &G = ig();
    if (r >= 1) return true;
    const int dn = (int)(r * (G.RY1 - G.FY));
    return y >= G.FY - dn && y <= G.FY + dn;
}
static void ib_j1(Gfx g, int lit)
{
    const IntroGeo &G = ig();
    cbf(g, G.J1X - 2, G.J1Y - 1, 50, 9, PAL_BLACK);
    for (int k = 0; k < 8; ++k) {
        const int x = G.J1X + 6 * k;
        cbf(g, x, G.J1Y, 4, 7, lit == 2 ? (uint8_t)PAL_WHITE : lit ? accent() : GOLD);
        cbf(g, x + 1, G.J1Y + 2, 2, 3, PAL_BLACK);
    }
    cb_lab(g, "J1 GIBSON BUS", G.J1X + 23, G.J1Y - 8, lit != 0);
}

static void paint_intro_board(Gfx g, const IntroSt &s)
{
    const IntroGeo &G = ig();
    const int W = EW(), H = EH();
    s_cb_col = accent();
    const uint8_t ac = accent(), trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    const uint8_t hot = cbmix(botcorps_rgb565(ac), 0xFFFF, 55);
    bool L[20], R[20], TL[20] = {}, TR[20] = {};
    for (int k = 0; k < 20; ++k) L[k] = R[k] = ib_reach(s.rail, e_py(k) + 3);
    e_headers(g, L, R, TL, TR);
    ib_rail(g, G.RL, s.rail, trace, ac); ib_rail(g, G.RR, s.rail, trace, ac);
    const int ly = ig_legy(), d = ly - G.RY1;
    const Pt PBL[3] = { { G.RL, G.RY1 }, { G.RL + d, ly }, { G.J1X - 1, ly } };
    const Pt PBR[3] = { { G.RR, G.RY1 }, { G.RR - d, ly }, { G.J1X + 46, ly } };
    path2(g, PBL, 3, s.rail >= 1 ? 1.f : 0.f, trace, ac, false);
    path2(g, PBR, 3, s.rail >= 1 ? 1.f : 0.f, trace, ac, false);
    cb_via(g, G.RL + 1, G.RY0, s.rail >= 1 ? ac : via); cb_via(g, G.RR, G.RY0, s.rail >= 1 ? ac : via);
    const int cx = G.BX + G.BD / 2 - 1;        /* 119 in portrait */
    const Pt PU[2] = { { cx, 58 }, { cx, G.BY - 5 } };
    const Pt PFL[2] = { { G.BX - 5, G.FY }, { G.RL + 2, G.FY } }, PFR[2] = { { G.BX + G.BD + 4, G.FY }, { G.RR - 2, G.FY } };
    const Pt PDL[2] = { { cx - 16, G.BY + 68 }, { cx - 16, G.PY - 2 } }, PDR[2] = { { cx + 16, G.BY + 68 }, { cx + 16, G.PY - 2 } };
    path2(g, PU, 2, s.src, trace, ac, true);
    path2(g, PFL, 2, s.feed, trace, ac, true); path2(g, PFR, 2, s.feed, trace, ac, true);
    path2(g, PDL, 2, s.drop, trace, ac, true); path2(g, PDR, 2, s.drop, trace, ac, true);
    const uint8_t stub = s.parts ? ac : trace;
    cb_line(g, G.RL + 2, G.LEDY + 1, G.LEDL - 2, G.LEDY + 1, stub);   cb_line(g, G.LEDR + 7, G.LEDY + 1, G.RR - 1, G.LEDY + 1, stub);
    cb_line(g, G.RL + 2, G.TCY + 5, G.TCL - 3, G.TCY + 5, stub);      cb_line(g, G.TCR + 14, G.TCY + 5, G.RR - 1, G.TCY + 5, stub);
    cb_line(g, G.RL + 2, G.REGY + 10, G.REGX - 2, G.REGY + 10, stub); cb_line(g, G.C1X + 17, G.REGY + 10, G.RR - 1, G.REGY + 10, stub);
    const uint8_t tw = s.con ? ac : trace;
    cb_line(g, G.RL + 2, G.WY, G.PX - 1, G.WY, tw); cb_line(g, G.PX + G.PW, G.WY, G.RR - 1, G.WY, tw);
    if (G.R16X >= 0) {
        cb_line(g, cx, G.PY + G.PH + 2, cx, G.R16Y - 2, stub); cb_line(g, cx + 1, G.PY + G.PH + 2, cx + 1, G.R16Y - 2, stub);
    }
    const int vy[4] = { G.LEDY + 1, G.TCY + 5, G.REGY + 10, G.WY };
    for (int i = 0; i < 4; ++i) { cb_via(g, G.RL + 3, vy[i], via); cb_via(g, G.RR - 2, vy[i], via); }
    if (s.surge > 0) {
        const Pt PSL[5] = { { G.BX - 5, G.FY }, { G.RL, G.FY }, { G.RL, G.RY1 }, { G.RL + d, ly }, { G.J1X - 1, ly } };
        const Pt PSR[5] = { { G.BX + G.BD + 4, G.FY }, { G.RR, G.FY }, { G.RR, G.RY1 }, { G.RR - d, ly }, { G.J1X + 46, ly } };
        path2(g, PSL, 5, s.surge, ac, hot, true); path2(g, PSR, 5, s.surge, ac, hot, true);
    }
    if (s.exitb) {
        const int y0 = G.J1Y + 7;
        cbf(g, G.J1X + 21, y0, 4, H - y0, PAL_WHITE);
        cbf(g, G.J1X + 19, y0, 1, H - y0, hot); cbf(g, G.J1X + 26, y0, 1, H - y0, hot);
    }
    cb_usb(g, G.USBX, G.USBY);
    ib_led(g, G.LEDL, G.LEDY, s.parts, sh_rgb(0xff, 0x22, 0x44), "PWR");
    ib_led(g, G.LEDR, G.LEDY, s.parts && s.con, ac, "IO2");
    cb_tact(g, G.TCL, G.TCY, "BOOT"); cb_tact(g, G.TCR, G.TCY, "EN");
    cb_reg(g, G.REGX, G.REGY); cb_c1(g, G.C1X, G.C1Y);
    if (G.R16X >= 0) cb_r16(g, G.R16X, G.R16Y);
    ib_j1(g, s.exitb ? 2 : s.surge >= 1 ? 1 : 0);
    ib_plate(g, s.con != 0, s.flash ? 1 : s.surge > 0 ? 2 : 0);
    cbf(g, G.PX - 1, G.WY, 1, 1, tw); cbf(g, G.PX + G.PW, G.WY, 1, 1, tw);
    ib_screen(g, s);
    ib_bootbot(g, s);
    e_silk(g, "ESP32 DEV BOARD", 16, H - 10);
    e_silk(g, "BOOTBOT REV C", W - 17 - tiny_w("BOOTBOT REV C"), H - 10);
    ib_topbar(g, s);
    e_frame(g);
}

/* ---- hub power-up (PICK.md §8 staged order), on the real hub tables (hb_layout_sync) ---- */
static void ib_face_geo(int *cx, int *cy, int *fs) { hb4_gib_geo(cx, cy, fs); }
static void ib_hub_route(Gfx g, int i, float prog)
{
    if (prog <= 0) return;
    const uint8_t ac = accent();
    const int n = HB_RN[i];
    int Lsum = 0;
    for (int q = 0; q + 1 < n; ++q) {
        const int ddx = abs(HB_RT[i][q + 1][0] - HB_RT[i][q][0]), ddy = abs(HB_RT[i][q + 1][1] - HB_RT[i][q][1]);
        Lsum += ddx > ddy ? ddx : ddy;
    }
    int lit = ri(prog * Lsum), hx = HB_RT[i][0][0], hy = HB_RT[i][0][1];
    for (int q = 0; q + 1 < n && lit > 0; ++q) {
        const int x0 = HB_RT[i][q][0], y0 = HB_RT[i][q][1], x1 = HB_RT[i][q + 1][0], y1 = HB_RT[i][q + 1][1];
        const int ddx = abs(x1 - x0), ddy = abs(y1 - y0), seg = ddx > ddy ? ddx : ddy, k = lit < seg ? lit : seg;
        const int ex = x0 + (x1 > x0 ? k : x1 < x0 ? -k : 0), ey = y0 + (y1 > y0 ? k : y1 < y0 ? -k : 0);
        cb_line(g, x0, y0, ex, ey, ac); hx = ex; hy = ey; lit -= k;
    }
    if (prog < 1) cbf(g, hx - 1, hy - 1, 3, 3, PAL_WHITE);
}
static void ib_title(Gfx g, int mode)
{
    const int W = EW();
    g.fillRect(0, 0, W, 26, PAL_BLACK); g.fillRect(0, 24, W, 1, sb_tone(3));
    char b[16]; snprintf(b, sizeof b, "%s", "TERMINALBOT");
    if (mode == 1) {
        static const char SCR[] = "#%&0101@$?*<>";
        for (int i = 0; b[i]; ++i) if (b[i] != ' ' && (i % 3) != 1) b[i] = SCR[(i * 7 + 3) % 13];
    }
    use_px(g, 8);
    txt(g, b, W / 2, 8, mode == 2 ? (uint8_t)PAL_WHITE : accent(), lgfx::textdatum::top_center);
    use_px(g, 6);
    if (mode == 2) g.fillRect(W / 2 - 56, 18, 112, 1, accent());
}
static void paint_intro_hub(Gfx g, const IntroSt &s)
{
    /* hub4 power-up: copper surges in, GIBSON face drops in, corner bots seat, plate lights */
    hb_layout_sync();
    s_cb_col = accent(); s_hb_sel = -1;
    const int W = EW(), H = EH();
    e_headers_hub(g);
    hb_copper(g);
    for (int r = 0; r < 4; ++r) ib_hub_route(g, r, s.hsurge);
    s_hb_sel = s.halo == 14 ? HB4_GIB : -1;
    hb_module(g);
    s_hb_sel = -1;
    int fcx, fcy, fs; ib_face_geo(&fcx, &fcy, &fs);
    const int D = fs + 8;
    if (!s.gib) cbf(g, fcx - D / 2 + 1, fcy - D / 2 + 1, D - 2, D - 2, PAL_BLACK);
    else if (s.gk > 0) {
        const int bob = ((millis() / 900u) % 2u) ? 1 : 0, cy = fcy + bob - 1;
        const int fx = fcx - fs / 2, fy = cy - fs / 2, gk = s.gk;
        g.setClipRect(fx, fy + 2, fs, 8 - gk);  hb_gib_face(g, fcx, cy - gk, fs);
        g.setClipRect(fx, fy + 10 - gk, fs, gk); hb_gib_face(g, fcx, cy - 6, fs);
        g.clearClipRect();
    }
    if (s.seat > 0 || s.term) hb4_chip(g, HB4_BOOT);
    if (s.seat > 0 || s.term) hb4_chip(g, HB4_SCAN);
    if (s.seat > 0 || s.ctl) hb4_chip(g, HB4_CTRL);
    if (s.seat > 0 || s.term) hb4_chip(g, HB4_TERM);
    e_plate(g, HB_TBOT[0], HB_TBOT[1], HB_TBOT[2], HB_TBOT[3], HB_TBOT_SILK, s.hin != 0, s.plate != 2);
    if (s.hin) {
        const uint8_t hot = cbmix(botcorps_rgb565(accent()), 0xFFFF, 55);
        const int bx = HB_TBOT[0] + HB_TBOT[2] / 2 + 1, by = HB_TBOT[1] + HB_TBOT[3] + 2;
        if (by < H) { cbf(g, bx, by, 4, H - by, PAL_WHITE); cbf(g, bx - 1, by, 1, H - by, hot); cbf(g, bx + 4, by, 1, H - by, hot); }
    }
    if (s.title) ib_title(g, s.title);
    else { g.fillRect(0, 0, W, 26, PAL_BLACK); g.fillRect(0, 24, W, 1, sb_tone(3)); }
    e_frame(g);
}

/* rain covers while the intro runs (PICK.md §9; the hub frames use the hub's covers) */
static void intro_covers(void)
{
    IntroSt s; intro_state(s);
    if (!s.rain) { cover_add(0, 0, EW(), EH()); return; }
    if (s.scr != 0 && s.wp <= 0) { hb_layout_sync(); hb_covers(); return; }
    const IntroGeo &G = ig();
    const int W = EW(), H = EH();
    cover_add(0, 0, W, 26); cover_add(0, 26, 46, H - 26); cover_add(W - 46, 26, 46, H - 26);
    cover_add(0, G.J1Y - 12, W, H - (G.J1Y - 12));
    cover_add(G.BX - 10, G.BY - 8, 84, 102); cover_add(G.USBX - 7, 26, 28, 52);
    cover_add(G.PX - 4, G.PY - 4, G.PW + 8, G.PH + 8);
    cover_add(G.LEDL - 6, 44, 20, 20); cover_add(G.LEDR - 8, 44, 20, 20);
    cover_add(G.TCL - 5, 120, 24, 26); cover_add(G.TCR - 7, 120, 24, 26);
    cover_add(G.REGX - 4, G.REGY - 4, 30, 34); cover_add(G.C1X - 4, G.C1Y - 5, 24, 22);
    if (G.R16X >= 0) cover_add(G.R16X - 4, G.R16Y - 4, 44, 22);
    if (s.wp > 0) {   /* the hub below the seam: its plate and side strips are covered already; add the plate row */
        hb_layout_sync();
        cover_add(HB_TBOT[0] - 3, HB_TBOT[1] - 5, HB_TBOT[2] + 6, HB_TBOT[3] + 5);
    }
}

/* one intro frame; the rain (static during the intro) is already under it */
static void paint_intro_e(Gfx g)
{
    IntroSt s; intro_state(s);
    if (s.scr == 2) { paint_hubbot(g); return; }          /* hand-off = the real hub */
    if (s.scr == 1) { paint_intro_hub(g, s); return; }
    paint_intro_board(g, s);
    if (s.wp > 0) {                                        /* bottom-up wipe: hub below the seam */
        const int W = EW(), H = EH();
        const int ys = H - ri(s.wp * H);
        IntroSt h; h.scr = 1; h.hin = 1; h.rain = 1;
        g.setClipRect(0, ys, W, H - ys);
        g.fillRect(0, ys, W, H - ys, botcorps_pal_hub_of(COL_FIELD));
        paint_intro_hub(g, h);
        g.clearClipRect();
        cbf(g, 3, ys - 3, W - 6, 2, accent()); cbf(g, 3, ys - 1, W - 6, 1, PAL_WHITE);
        e_frame(g);
    }
}
/* repaint key: step index + caret phase while a command line is live */
static uint32_t intro_key(void)
{
    const int k = intro_step();
    const uint32_t caret = (k >= 10 && k <= 21) ? (millis() / 400u) % 2u : 0u;
    return (uint32_t)k * 2u + caret;
}
static bool intro_hub_phase(void) { IntroSt s; intro_state(s); return s.scr != 0; }
