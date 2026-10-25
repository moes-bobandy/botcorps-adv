/* ============================================================================
 * CYD edge board (PICK.md idea A-fixed, out/cyd-bootbot-intro-mock): the shared painter for every CYD
 * screen, portrait 240x320 and landscape 320x240. The screen perimeter IS the dev-board's gold edge,
 * the top bar sits ON the board, full-length pin headers run down both sides (20 a side in portrait,
 * pitch 14 from y 32; landscape fits 14), and all text output lives in castellated, silk-labelled
 * plates wired into the copper. Ported from tools-bc/intro3_harness.cpp (e_frame / e_headers /
 * e_hub_plate), with the 240/320 constants generalised to the current orientation.
 * Included from botcorps_cyd_portrait.inc (BOTCORPS_CYD only).
 * ==========================================================================*/
struct Pt { int x, y; };
static const uint8_t GOLD = 177;
static int EW(void) { return botcorps_cyd_orient ? 240 : 320; }
static int EH(void) { return botcorps_cyd_orient ? 320 : 240; }
static uint8_t e_rule(void) { return cbmix(0x0841, botcorps_rgb565(accent()), 40); }
static int ri(float v) { return (int)(v + 0.5f); }

/* edge-to-edge: the perimeter is the board's gold edge; inner rule 2 px in; mounting holes at the bottom corners */
static void e_frame(Gfx g)
{
    const int W = EW(), H = EH();
    const uint8_t rule = e_rule();
    g.drawRect(0, 0, W, H, GOLD);
    cbf(g, 2, 1, 1, H - 3, rule); cbf(g, W - 3, 1, 1, H - 3, rule); cbf(g, 2, H - 3, W - 4, 1, rule);
    cbf(g, 2, 26, 1, 1, rule);
    cb_ring(g, 8, H - 9); cb_ring(g, W - 9, H - 9);
}

/* full-length headers (ESP32 DevKitC 38-pin style), pitch 14 from y 32: 20 a side in portrait, 14 in landscape */
static const int EPY0 = 32, EPP = 14;
static const char *const E_LP[20] = { "3V", "GND", "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16",
                                      "D4", "D0", "D2", "D15", "SD1", "SD0", "CLK", "GND" };
static const char *const E_RP[20] = { "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12",
                                      "GND", "D13", "SD2", "SD3", "CMD", "3V3", "GND", "5V" };
static int e_pn(void) { const int n = (EH() - 54) / EPP + 1; return n > 20 ? 20 : n; }
static int e_py(int k) { return EPY0 + k * EPP; }
static int e_kind(const char *n) { if (!strcmp(n, "GND")) return 1; if (n[0] == '3' || n[0] == '5') return 0; return 2; }
static void e_headers(Gfx g, const bool *litL, const bool *litR, const bool *tipL, const bool *tipR)
{
    const int W = EW(), N = e_pn();
    const uint8_t silkc = cb_silk();
    cbf(g, 4, EPY0 - 2, 11, (N - 1) * EPP + 11, PAL_BLACK);
    cbf(g, W - 15, EPY0 - 2, 11, (N - 1) * EPP + 11, PAL_BLACK);
    for (int k = 0; k < N; ++k) {
        const int y = e_py(k);
        cb_pad(g, 6, y, e_kind(E_LP[k]), litL[k], tipL[k]);
        cb_pad(g, W - 13, y, e_kind(E_RP[k]), litR[k], tipR[k]);
        const int lw = tiny_w(E_LP[k]), rw = tiny_w(E_RP[k]);
        cbf(g, 15, y, lw + 3, 7, PAL_BLACK); tiny_text(g, E_LP[k], 17, y + 1, litL[k] ? (uint8_t)PAL_WHITE : silkc);
        cbf(g, W - 18 - rw - 1, y, rw + 3, 7, PAL_BLACK); tiny_text(g, E_RP[k], W - 17 - rw, y + 1, litR[k] ? (uint8_t)PAL_WHITE : silkc);
    }
}
/* powered-board semantics (hub): right D32 D33 D25 D14 D12 lit + 400 ms white tip */
static void e_headers_hub(Gfx g)
{
    bool L[20] = {}, R[20] = {}, TL[20] = {}, TR[20] = {};
    const int tip = (int)((millis() / 400u) % 12u);
    for (int k = 0; k < 20; ++k) { const bool lr = (k == 5 || k == 6 || k == 7 || k == 10 || k == 11); R[k] = lr; TR[k] = lr && k == tip; }
    e_headers(g, L, R, TL, TR);
}

/* the shared top bar: Gibson status chip, 12 bot swatches, crown (= BACK) at W-94, link + counters.
 * No terminal glyph: there is no standalone Terminal screen on the CYD (the hub plate is the terminal). */
static int e_crown_x(void) { return EW() - 94; }
static void e_bar(Gfx g)
{
    const int W = EW();
    const uint8_t ac = accent();
    g.fillRect(0, 0, W, 26, PAL_BLACK);
    chip_mini(g, 14, 12, 14, 12, false, 0);
    static const uint8_t DIM[12] = { 9, 9, 9, 12, 76, 100, 33, 13, 76, 13, 100, 69 };
    for (int i = 0; i < 12; ++i) g.fillRect(30 + i * 8, 9, 5, 5, DIM[i]);
    gib_crown(g, e_crown_x(), 3);
    g.drawCircle(W - 40, 12, 3, ac);
    g.fillRect(W - 41, 11, 3, 3, ac);
    txt(g, "12", W - 34, 6, PAL_WHITE, lgfx::textdatum::top_left);
    txt(g, "0", W - 12, 9, ac, lgfx::textdatum::top_left);
    g.fillRect(0, 24, W, 1, sb_tone(3));
}
static void e_silk(Gfx g, const char *t, int x, int y) { cbf(g, x - 1, y - 1, tiny_w(t) + 2, 7, PAL_BLACK); tiny_text(g, t, x, y, cb_silk()); }

/* board base for a screen: headers + bottom silk; e_finish() draws the top bar and the edge last */
static void e_board(Gfx g)
{
    s_cb_col = accent();
    const int W = EW(), H = EH();
    /* header strips are solid board (no rain between the pin labels) */
    const uint8_t bg = (uint8_t)PAL_FIELD;
    g.fillRect(0, 26, 33, H - 26, bg);
    g.fillRect(W - 33, 26, 33, H - 26, bg);
    g.fillRect(0, H - 4, W, 4, bg);
    e_headers_hub(g);
}
static void e_finish(Gfx g)
{
    e_bar(g);
    e_frame(g);
}

/* 2 px power copper + animated head (intro) */
static int seg_len(Pt a, Pt b) { const int dx = abs(a.x - b.x), dy = abs(a.y - b.y); return dx > dy ? dx : dy; }
static void line2(Gfx g, Pt a, Pt b, uint8_t c)
{
    cb_line(g, a.x, a.y, b.x, b.y, c);
    if (a.y == b.y) cb_line(g, a.x, a.y + 1, b.x, b.y + 1, c);
    else cb_line(g, a.x + 1, a.y, b.x + 1, b.y, c);
}
static void path2(Gfx g, const Pt *p, int n, float prog, uint8_t base, uint8_t lit, bool head)
{
    int L = 0; for (int q = 0; q + 1 < n; ++q) L += seg_len(p[q], p[q + 1]);
    for (int q = 0; q + 1 < n; ++q) line2(g, p[q], p[q + 1], base);
    if (prog <= 0) return;
    int rem = ri(prog * L); Pt h = p[0];
    for (int q = 0; q + 1 < n && rem > 0; ++q) {
        const int sl = seg_len(p[q], p[q + 1]), k = rem < sl ? rem : sl;
        const Pt e = { p[q].x + (p[q + 1].x > p[q].x ? k : p[q + 1].x < p[q].x ? -k : 0), p[q].y + (p[q + 1].y > p[q].y ? k : p[q + 1].y < p[q].y ? -k : 0) };
        line2(g, p[q], e, lit); h = e; rem -= k;
    }
    if (head && prog < 1) cbf(g, h.x - (h.x >= EW() / 2 ? 2 : 1), h.y - 1, 4, 4, PAL_WHITE);
}
/* 1 px signal trace with vias at the bends (plate feeds on non-hub screens) */
static void e_trace(Gfx g, const Pt *p, int n)
{
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    for (int q = 0; q + 1 < n; ++q) cb_line(g, p[q].x, p[q].y, p[q + 1].x, p[q + 1].y, trace);
    for (int q = 1; q + 1 < n; ++q) cb_via(g, p[q].x, p[q].y, via);
}

static void hb_kbd_emblem(Gfx g, int x, int y, int w, int h, bool lit)
{
    const uint8_t ac = accent();
    g.fillRect(x, y, w, h, lit ? (uint8_t)PAL_SEL : (uint8_t)PAL_DEEP);
    g.drawRect(x, y, w, h, lit ? ac : (uint8_t)PAL_LINE);
    const uint8_t ink = lit ? ac : (uint8_t)PAL_WHITE;
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 4; ++c)
            g.fillRect(x + 5 + c * 5, y + 6 + r * 5, 3, 3, ink);
    g.fillRect(x + 7, y + h - 6, w - 14, 3, ink);
}

/* ---- plates: the terminal language of the board (hb_terminalbot geometry per PICK.md §9) ---- */
/* shell only: substrate, gold edge + rule, castellations x = bx+3+12k top and bottom, silk legend, pads, black screen.
 * Returns the screen rect. Pads: RX TX IO0 EN at sx+52+26k when the plate is wide enough, else RX TX at the right. */
static void e_plate_shell(Gfx g, int bx, int by, int bw, int bh, const char *silk, bool pads_on,
                          int *osx, int *osy, int *osw, int *osh)
{
    const uint8_t ac = accent(), silkc = cb_silk();
    cbf(g, bx, by, bw, bh, cbp(0x18E3));
    g.drawRect(bx, by, bw, bh, GOLD);
    g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, e_rule());
    for (int x = bx + 3; x < bx + bw - 6; x += 12) { cbf(g, x, by - 2, 3, 3, GOLD); cbf(g, x, by + bh - 1, 3, 3, GOLD); }
    const int sx = bx + 6, sw = bw - 12, sy = by + 10, sh = bh - 13;
    tiny_text(g, silk, sx + 1, by + 3, cbmix(botcorps_rgb565(ac), 0xFFFF, 35));
    const int tip = (int)((millis() / 400u) % 4u);
    if (sw >= 52 + 26 * 3 + 20 && tiny_w(silk) < 48) {
        static const char *const TP[4] = { "RX", "TX", "IO0", "EN" };
        for (int k = 0; k < 4; ++k) { const int px = sx + 52 + k * 26; cb_pad(g, px, by + 2, 2, pads_on, pads_on && k == tip); tiny_text(g, TP[k], px + 9, by + 3, silkc); }
    } else {
        static const char *const TP[2] = { "RX", "TX" };
        for (int q = 0; q < 2; ++q) { const int px = sx + sw - 38 + q * 20; cb_pad(g, px, by + 2, 2, pads_on, pads_on && q == (tip & 1)); tiny_text(g, TP[q], px + 9, by + 3, silkc); }
    }
    g.fillRect(sx, sy, sw, sh, PAL_BLACK);
    g.drawRect(sx, sy, sw, sh, PAL_LINE);
    *osx = sx; *osy = sy; *osw = sw; *osh = sh;
}
/* one log line, cut to the plate width (no clip rect: the intro wipe owns the clip) */
static void e_log_line(Gfx g, const char *ln, int lx, int ly, int w)
{
    if (!ln || !ln[0]) return;
    const int maxc = w / 6;
    char b[64];
    int n = 0;
    if (ln[0] == '\x01' && ln[1]) { b[0] = ln[0]; b[1] = ln[1]; n = 2; }
    const int lim = n + maxc < (int)sizeof b - 1 ? n + maxc : (int)sizeof b - 1;
    for (int i = n; ln[i] && i < lim; ++i) b[n++] = ln[i];   /* n tracks i */
    b[n] = 0;
    pt_draw_log_line(g, b, lx, ly);
}
/* the live terminal plate: mode 0 = command line + nlog newest log lines + keyboard emblem + "=" list button,
 * mode 1 keys, mode 2 command list (s_pt_mode). mode_dark: shell + black screen only (intro staging). */
static void e_plate(Gfx g, int bx, int by, int bw, int bh, const char *silk, int nlog, bool sel, bool dark = false)
{
    int sx, sy, sw, sh;
    e_plate_shell(g, bx, by, bw, bh, silk, true, &sx, &sy, &sw, &sh);
    if (!dark) {
        if (s_pt_mode == 0) {
            pt_draw_cmdline(g, sx + 2, sy + 2, sw - 4);
            const int bwid = 34, bht = 16, byb = sy + sh - bht - 2;
            for (int i = 0; i < nlog; ++i) {
                const int idx = s_log_n - nlog + i;
                if (idx < 0) continue;
                const int ly = sy + 16 + i * 9;
                const bool under = ly + 8 > byb;                  /* rows beside the buttons stop short of them */
                e_log_line(g, log_line(idx), sx + 3, ly, under ? sw - bwid * 2 - 12 : sw - 6);
            }
            hb_kbd_emblem(g, sx + sw - bwid * 2 - 6, byb, bwid, bht, false);
            pt_draw_btn(g, sx + sw - bwid - 2, byb, bwid, bht, "=", false);
        } else {
            pt_draw_cmdline(g, sx + 2, sy + 2, sw - 4);
            const int by2 = sy + 15, bh2 = sh - 17;
            if (s_pt_mode == 1) pt_draw_keys(g, sx + 2, by2, sw - 4, bh2);
            else pt_draw_list(g, sx + 2, by2, sw - 4, bh2);
        }
        s_pt_bx = sx; s_pt_by = sy; s_pt_bw = sw; s_pt_bh = sh;
    }
    if (sel) g.drawRect(bx - 2, by - 2, bw + 4, bh + 4, PAL_WHITE);
}
/* plate rows per screen: portrait plates are 80 tall at the board foot, landscape plates 72 */
static int e_plate_h(void) { return botcorps_cyd_orient ? 80 : 72; }

/* draw in a shifted frame: everything painted through .h lands (dx, dy) away (Gfx fill ops use the
 * global band x origin + the member y origin, raw blits use .ox/.oy) */
struct EShift {
    int sox;
    Gfx h;
    EShift(Gfx g, int dx, int dy) : sox(botcorps_band_ox()), h(g)
    {
        botcorps_band_set_ox(sox - dx);
        h.oy -= dy;
        h.ox -= dx;
    }
    ~EShift() { botcorps_band_set_ox(sox); }
};

/* CONTROLbot on the edge board: the scattered parts (ctlbot_layout per orientation) over the shared
 * board, headers and top bar; no own frame, no own pin rows. */
static void paint_ctlbot(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    e_board(g);
    const uint8_t trace = cb_trace(), via = cbmix(botcorps_rgb565(trace), 0xFFFF, 35);
    cb_copper(g, trace, via);
    cb_hilite(g);
    const int *M = P ? CB_P_MOD : CB_L_MOD;
    cb_module(g, M[0], M[1], M[2], M[3]);
    cb_meters(g, P ? CB_P_MET : CB_L_MET);
    cb_orient(g);
    cb_power(g, P ? CB_P_PWR : CB_L_PWR, P ? CB_P_TACT : CB_L_TACT, P ? CB_P_SER : CB_L_SER);
    static const int CH[5] = { 0, 1, 2, 5, 7 };
    for (int k = 0; k < 5; ++k) cb_chip(g, CH[k], s_cursor == CH[k]);
    cb_part(g, 3); cb_part(g, 4); cb_part(g, 6);
    const char *nm = "CONTROLBOT";
    if (P) e_silk(g, nm, 205 - tiny_w(nm), 311);
    else e_silk(g, nm, 160 - tiny_w(nm) / 2, 37);
    e_finish(g);
}
/* the whole Control board is copper: rain is painted once under it and never stepped over the parts */
static void layout_covers_ctlbot(void)
{
    cover_add(0, 0, EW(), EH());
}
