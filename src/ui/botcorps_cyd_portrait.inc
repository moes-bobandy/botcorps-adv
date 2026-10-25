/* BOTCORPS_CYD screens (portrait 240x320 AND landscape 320x240): every screen is painted on the shared
 * edge board (botcorps_cyd_edge.inc). Included from botcorps_ui.cpp under #ifdef BOTCORPS_CYD only;
 * the Adv build never sees this file. */
#include "ui/botcorps_cyd_ctlbot.inc"

/* Control part rects (index map matches landscape CP_N): the CONTROLbot layout of the current orientation. */
static bool cp_part_rect_p(int index, int *x, int *y, int *w, int *h) { return ctlbot_rect(index, x, y, w, h); }
static bool cp_ctlbot_on(void) { return true; }

/* terminal plate state (the plate rect is set by e_plate each paint; 0 width = no plate on screen) */
static uint8_t s_pt_mode = 0; /* 0 log, 1 keys, 2 list */
static bool s_pt_sym = false;
static int s_pt_top = 0;
static int s_pt_sel = 0;
static int s_pt_bx = 0, s_pt_by = 0, s_pt_bw = 0, s_pt_bh = 0;

static char *pt_buf(int *cap)
{
    if (s_screen == SCR_TERMINAL) {
        if (cap) *cap = 60;
        return botcorps_t0_term_prompt;
    }
    if (sbox_on()) {
        if (cap) *cap = 14;
        return s_sp_buf;
    }
    if (cap) *cap = GP_BUF_N;
    return s_gp_buf;
}

static const char *pt_pfx(void)
{
    if (s_screen == SCR_GIBSON) return "gibson> ";
    /* SCANbot boards use the short, non-clipping scanbot prompt (cp1 fix 2: the long
     * user@botcorps prompt + a typed command overran the small plate and clipped on the left). */
    if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR || s_screen == SCR_SCAN) return "scanbot> ";
    return "user@botcorps:~$ ";
}

static const char *const *pt_list(int *n)
{
    if (s_screen == SCR_TERMINAL) {
        if (n) *n = CMD_N;
        return CMDN;
    }
    if (sbox_on()) {
        static const char *const SP_P[] = { "scan", "hunt", "info", "gloss", "defend", "terminal", "back", "help" };
        if (n) *n = 8;
        return SP_P;
    }
    if (s_screen == SCR_GCHAT) {
        if (n) *n = GC_N;
        return GC_NM;
    }
    if (n) *n = (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0]));
    return GP_CMD;
}

/* TERMINALbot home routes, typed or picked on any plate (field fix 2026-10-25):
 * home / hub -> TERMINALbot (the hub), scanbot / scan -> SCANbot (Gibson scan), control -> CONTROLbot.
 * On SCANbot itself a bare "scan" stays its own re-scan. The line is echoed into the plate log. */
static bool cyd_route_line(void)
{
    int cap = 0;
    char *b = pt_buf(&cap);
    if (!b || !b[0]) return false;
    char w[16];
    int o = 0;
    for (const char *c = b; *c && o < 15; ++c) { if (*c == ' ' && o == 0) continue; w[o++] = (*c >= 'A' && *c <= 'Z') ? (char)(*c + 32) : *c; }
    while (o > 0 && w[o - 1] == ' ') --o;
    w[o] = 0;
    int to = -1;
    if (!strcmp(w, "home") || !strcmp(w, "hub") || !strcmp(w, "go hub") || !strcmp(w, "go home") || !strcmp(w, "terminalbot")) to = SCR_GIBSON;
    else if (!strcmp(w, "scanbot") || (!strcmp(w, "scan") && s_screen != SCR_SCAN)) to = SCR_SCAN;
    else if (!strcmp(w, "control") || !strcmp(w, "controlbot")) to = SCR_CONTROL;
    if (to < 0) return false;
    char row[40];
    snprintf(row, sizeof row, "gibson> %s", w);
    log_add(row);
    b[0] = 0;
    s_pt_mode = 0;
    if (to == SCR_SCAN) { s_scan_bot = 12; s_gib_hunt = false; }
    if (s_screen == SCR_GCHAT && dl_live()) dl_end();
    if (to != s_screen || to == SCR_SCAN) enter_screen(to, SCR_GIBSON);
    s_dirty = true;
    return true;
}

static void pt_draw_cmdline(Gfx g, int x, int y, int w)
{
    int cap = 0;
    char *buf = pt_buf(&cap);
    const char *pfx = pt_pfx();
    char line[96];
    snprintf(line, sizeof(line), "%s%s", pfx, buf ? buf : "");
    /* Keep prompt + caret readable: scroll the typed tail, never clip the caret off. */
    const int maxc = (w - 8) / 6;
    const char *show = line;
    int len = (int)strlen(line);
    if (len > maxc) show = line + (len - maxc);
    g.fillRect(x, y, w, 12, PAL_BLACK);
    g.drawRect(x, y, w, 12, PAL_LINE);
    txt(g, show, x + 2, y + 2, PAL_WHITE, lgfx::textdatum::top_left);
    if (((millis() / 400u) & 1u) == 0) {
        const int cx = x + 2 + (int)strlen(show) * 6;
        if (cx + 4 <= x + w - 2) g.fillRect(cx, y + 2, 4, 8, accent());
    }
}

static void pt_draw_btn(Gfx g, int x, int y, int w, int h, const char *lab, bool lit)
{
    const uint8_t ac = accent();
    g.fillRect(x, y, w, h, lit ? (uint8_t)PAL_SEL : (uint8_t)PAL_DEEP);
    g.drawRect(x, y, w, h, lit ? ac : (uint8_t)PAL_LINE);
    if (lab && lab[0] == '=' && lab[1] == 0) {
        for (int k = 0; k < 3; ++k)
            g.fillRect(x + 8, y + 7 + k * 5, w - 16, 2, lit ? ac : (uint8_t)COL_MUTED);
    } else {
        txt(g, lab, x + w / 2, y + h / 2 - 4, lit ? ac : (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
    }
}

static void pt_draw_keys(Gfx g, int x, int y, int w, int h)
{
    const int kp = (w - 4) / 10;
    const int rp = (h - 2) / 5;
    const int n = (int)(sizeof(CYD_OSK_SPEC) / sizeof(CYD_OSK_SPEC[0]));
    for (int i = 0; i < n; ++i) {
        const CydOskSpec &sp = CYD_OSK_SPEC[i];
        const int kx = x + 2 + sp.col * kp;
        const int ky = y + 1 + sp.row * rp;
        const int kw = kp * sp.span - 1;
        const int kh = rp - 1;
        g.fillRect(kx, ky, kw, kh, PAL_DEEP);
        g.drawRect(kx, ky, kw, kh, PAL_LINE);
        char lab[8] = {0};
        if (sp.kind == CYD_OSK_CHAR) {
            lab[0] = s_pt_sym ? sp.symbol : sp.alpha;
        } else if (sp.kind == CYD_OSK_SPACE) snprintf(lab, sizeof(lab), "SPC");
        else if (sp.kind == CYD_OSK_BKSP) snprintf(lab, sizeof(lab), "<");
        else if (sp.kind == CYD_OSK_ENTER) snprintf(lab, sizeof(lab), "OK");
        else if (sp.kind == CYD_OSK_TAB) snprintf(lab, sizeof(lab), "TAB");
        else if (sp.kind == CYD_OSK_SYM) snprintf(lab, sizeof(lab), s_pt_sym ? "abc" : "SYM");
        else if (sp.kind == CYD_OSK_HIDE) snprintf(lab, sizeof(lab), "BACK"); /* never HID */
        txt(g, lab, kx + kw / 2, ky + kh / 2 - 4, PAL_WHITE, lgfx::textdatum::top_center);
    }
}

/* field fix: list row pitch grows with the panel so rows are ~28 px in the expanded overlay */
static int pt_list_rh(int h) { const int r = (h - 26) / 6; return r < 9 ? 9 : r > 30 ? 30 : r; }
static void pt_draw_list(Gfx g, int x, int y, int w, int h)
{
    int n = 0;
    const char *const *cmds = pt_list(&n);
    const int back_h = h > 90 ? 26 : 12;
    const int rh = pt_list_rh(h);
    const int rows = (h - back_h) / rh;
    if (s_pt_sel < s_pt_top) s_pt_top = s_pt_sel;
    if (s_pt_sel >= s_pt_top + rows) s_pt_top = s_pt_sel - rows + 1;
    if (s_pt_top < 0) s_pt_top = 0;
    for (int r = 0; r < rows; ++r) {
        const int i = s_pt_top + r;
        if (i >= n) break;
        const bool on = i == s_pt_sel;
        const int ry = y + r * rh;
        g.fillRect(x, ry, w, rh - 1, on ? accent() : (uint8_t)PAL_DEEP);
        g.drawRect(x, ry, w, rh - 1, on ? (uint8_t)PAL_WHITE : (uint8_t)PAL_LINE);
        txt(g, cmds[i], x + 4, ry + (rh - 1) / 2 - 4, on ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE,
            lgfx::textdatum::top_left);
    }
    /* Dedicated UP / DOWN / BACK under the list (big targets in the expanded overlay). */
    const int bw3 = w / 3 - 2;
    pt_draw_btn(g, x, y + h - back_h, bw3, back_h - 1, "UP", false);
    pt_draw_btn(g, x + w / 3 + 1, y + h - back_h, bw3, back_h - 1, "DOWN", false);
    pt_draw_btn(g, x + 2 * (w / 3) + 2, y + h - back_h, w - 2 * (w / 3) - 2, back_h - 1, "BACK", false);
}

static void pt_draw_log_line(Gfx g, const char *ln, int lx, int ly)
{
    if (!ln || !ln[0]) return;
    const uint8_t pale = botcorps_pal_fit(0xBFBF);
    if (ln[0] == '\x01') {
        const int b = (unsigned char)ln[1];
        const uint8_t ink = b >= 12 ? (uint8_t)COL_IVORY : bot_color(b);
        txt(g, ln + 2, lx, ly, ink, lgfx::textdatum::top_left);
        return;
    }
    if (ln[0] == '[') {
        const char *rb = strchr(ln, ']');
        if (rb) {
            char tag[12];
            int tn = (int)(rb - ln) + 1;
            if (tn > 11) tn = 11;
            for (int i = 0; i < tn; ++i) tag[i] = ln[i];
            tag[tn] = 0;
            txt(g, tag, lx, ly, (uint8_t)PAL_OK, lgfx::textdatum::top_left);
            if (rb[1]) txt(g, rb + 1, lx + tn * 6, ly, pale, lgfx::textdatum::top_left);
            return;
        }
    }
    txt(g, ln, lx, ly, pale, lgfx::textdatum::top_left);
}


#include "ui/botcorps_cyd_edge.inc"

/* ---- scan helpers (mock-B parts) ---- */
/* ---- Mock B scan helpers: device panel + traced chip cluster + logs ---- */
static void port_seed_scan_logs(bool wifi, bool ble, bool gib, int bot)
{
    if (s_log_n > 0) return;   /* seed an empty log only: never wipe real output */
    char a[40], b[40];
    if (wifi) {
        log_add("[ ok ] wifi radio up - ch sweep");
        log_add("[ ok ] wifi sweep 17/17");
    } else if (ble) {
        log_add("[ ok ] bluetooth radio up");
        log_add("[ ok ] ble sweep 12/12");
    } else if (gib) {
        log_add("[ ok ] gibson core linked");
        log_add("[ ok ] 12 finds - wifi open");
    } else {
        const char *who = (bot >= 0 && bot < 12) ? BOTS[bot].short_name : "bot";
        snprintf(a, sizeof a, "[ ok ] %s core linked", who);
        snprintf(b, sizeof b, "[ ok ] %s bus idle", who);
        log_add(a);
        log_add(b);
    }
}

/* Upper panel: 1 px grape rim + green grid (5 rows, open bottom) at the landscape pitch. */
static void paint_scanb_panel_p(Gfx g, bool grid)
{
    g.drawRect(29, 41, 181, 119, (uint8_t)69);
    if (!grid) return;
    g.fillRect(207, 45, 1, 110, (uint8_t)69);
    for (int c = 0; c < 8; ++c) g.fillRect(33 + c * 22, 46, 1, 109, (uint8_t)1);
    for (int r = 0; r < 5; ++r) g.fillRect(33, 46 + r * 22, 174, 1, (uint8_t)1);
    for (int c = 0; c < 8; ++c)
        for (int r = 0; r < 5; ++r)
            g.fillRect(32 + c * 22, 45 + r * 22, 3, 3, (uint8_t)65);
}
static void scanb_diag_p(Gfx g, int x, int y, int dx, int dy, int n, uint8_t c)
{
    for (int i = 0; i < n; ++i) g.fillRect(x + i * dx, y + i * dy, 1, 1, c);
}
/* MCU package: steel ring + silver pins + bot face with two eyes (mock B 56 px / 36 px face). */
static void paint_scanb_chip_p(Gfx g, uint8_t face, uint8_t hi, uint8_t lo, uint8_t tr, bool eyes)
{
    const uint8_t ring = (uint8_t)110, pin = (uint8_t)186;
    g.fillRect(92, 180, 56, 56, PAL_BLACK);
    g.drawRect(92, 180, 56, 56, ring);
    g.fillRect(95, 183, 51, 1, ring);
    g.fillRect(95, 183, 1, 53, ring);
    g.fillRect(94, 191, 1, 42, (uint8_t)9);
    g.fillRect(144, 184, 1, 48, (uint8_t)41);
    g.fillRect(94, 232, 50, 1, (uint8_t)41);
    g.fillRect(94, 233, 6, 1, (uint8_t)9);
    static const int PX[5] = { 101, 109, 117, 125, 133 };
    for (int i = 0; i < 5; ++i) {
        g.fillRect(PX[i], 180, 2, 5, pin);
        g.fillRect(PX[i], 231, 2, 5, pin);
        g.fillRect(92, 189 + i * 8, 5, 2, pin);
        g.fillRect(143, 189 + i * 8, 5, 2, pin);
        g.fillRect(148, 190 + i * 8 - (i > 1 ? 1 : 0), 2, 1, tr);
    }
    g.fillRect(102, 190, 36, 36, face);
    g.fillRect(102, 190, 36, 2, hi);
    g.fillRect(102, 224, 36, 2, lo);
    if (eyes) {
        g.fillRect(109, 197, 7, 7, PAL_BLACK);
        g.fillRect(123, 197, 7, 7, PAL_BLACK);
    }
}
/* Peri die: 1 px device-ink rim, silver/steel castellations, device glyph, fan-in traces. */
static void paint_scanb_peri_p(Gfx g, int kind, uint8_t rim, uint8_t tooth, uint8_t tr)
{
    const int x = 158, y = 193, T = 26;
    g.fillRect(x, y, T, T, PAL_BLACK);
    g.drawRect(x, y, T, T, rim);
    for (int k = 0; k < 5; ++k) {
        const int i = 1 + k * 5;
        g.fillRect(x + i, y - 2, 4, 2, tooth); g.fillRect(x + i + 1, y - 2, 2, 2, (uint8_t)186);
        g.fillRect(x + i, y + T, 4, 2, tooth); g.fillRect(x + i + 1, y + T, 2, 2, (uint8_t)186);
        g.fillRect(x - 2, y + i, 2, 4, tooth); g.fillRect(x - 2, y + i + 1, 2, 2, (uint8_t)186);
        g.fillRect(x + T, y + i, 2, 4, tooth); g.fillRect(x + T, y + i + 1, 2, 2, (uint8_t)186);
    }
    gl_icon(g, kind, x + 7, y + 7, false);
}
/* Mock B tiles: 18x18 rim 149 / face 5; selected = white rim, grape face, white corner ticks. */
static void paint_scanb_tiles_at_p(Gfx g, int y0, int n, bool wifi)
{
    for (int k = 0; k < n && k < 40; ++k) {
        const int c = k % 8, r = k / 8;
        const int x = 35 + c * 22, y = y0 + r * 22;
        const bool sel = (k == 0);
        g.fillRect(x, y, 18, 18, sel ? (uint8_t)PAL_WHITE : (uint8_t)149);
        g.fillRect(x + 1, y + 1, 16, 16, sel ? (uint8_t)69 : (uint8_t)5);
        if (wifi) v2_icon_at(g, k, x + 3, y + 3);
        else gl_icon(g, (int)LIST_BLE_TY[k % SB_N], x + 3, y + 3, false);
        if (sel) {
            g.fillRect(x + 21, y - 3, 1, 4, PAL_WHITE);
            g.fillRect(x + 21, y + 17, 1, 2, PAL_WHITE);
            g.fillRect(x - 1, y + 19, 1, 3, PAL_WHITE);
            g.fillRect(x - 3, y + 21, 3, 1, PAL_WHITE);
        }
    }
}
/* Gibson crew lane die: 13 px, steel side pins, body with a lit top row, two eyes, dark foot. */
static void gib_lane_die_p(Gfx g, int y0, uint8_t body, uint8_t hi, uint8_t lo)
{
    g.fillRect(39, y0 + 1, 11, 9, PAL_BLACK);
    g.fillRect(40, y0 + 1, 9, 9, body);
    g.fillRect(40, y0 + 1, 9, 1, hi);
    g.fillRect(40, y0 + 9, 9, 1, lo);
    g.fillRect(41, y0 + 3, 2, 1, PAL_BLACK);
    g.fillRect(45, y0 + 3, 2, 1, PAL_BLACK);
    for (int r = 1; r < 10; r += 2) {
        g.fillRect(38, y0 + r, 1, 1, (uint8_t)186);
        g.fillRect(50, y0 + r, 1, 1, (uint8_t)186);
    }
}
/* Collapsed crew lane (BT / DEAUTH): slate body split by the trace channel, meter pips, count, caret. */
static void gib_lane_closed_p(Gfx g, int y0, const char *nm, const char *cnt, uint8_t acc,
                              uint8_t hi, uint8_t lo, uint8_t pip, int pips)
{
    g.fillRect(34, y0, 3, 11, acc);
    g.fillRect(37, y0, 89, 11, (uint8_t)5);
    g.fillRect(142, y0, 46, 11, (uint8_t)5);
    gib_lane_die_p(g, y0, acc, hi, lo);
    tiny_text(g, nm, 52, y0 + 3, (uint8_t)223);
    for (int k = 0; k < pips; ++k) {
        const int x = k < 3 ? 112 + k * 5 : 142 + (k - 3) * 5;
        g.fillRect(x, y0 + 5, 3, 3, pip);
    }
    tiny_text(g, cnt, 172 - tiny_w(cnt), y0 + 3, acc);
    tiny_text(g, ">", 179, y0 + 2, (uint8_t)146);
    g.fillRect(205, y0, 1, 11, (uint8_t)1);
    g.fillRect(206, y0, 1, 11, (uint8_t)69);
    g.fillRect(193, y0, 3, 11, (uint8_t)33);
}

/* Gibson crew drop-down vs mock B: header, open WIFI lane with 17 finds, BT / DEAUTH collapsed. */
static void paint_gibson_accordion_p(Gfx g)
{
    paint_scanb_panel_p(g, false);
    g.fillRect(30, 42, 179, 117, PAL_FIELD);
    /* Header: violet bracket, Gibson chip, name, finds. */
    const uint8_t vio = (uint8_t)134;
    g.fillRect(34, 44, 15, 1, vio);
    g.fillRect(34, 45, 1, 8, vio);
    g.fillRect(48, 45, 1, 8, vio);
    g.fillRect(34, 53, 2, 1, vio);
    g.fillRect(42, 53, 7, 1, vio);
    g.fillRect(54, 45, 11, 11, PAL_BLACK);
    for (int k = 0; k < 5; ++k) {
        g.fillRect(55 + k * 2, 44, 1, 1, (uint8_t)177);
        g.fillRect(53, 46 + k * 2, 1, 1, (uint8_t)177);
        g.fillRect(65, 46 + k * 2, 1, 1, (uint8_t)177);
    }
    g.fillRect(55, 46, 9, 1, (uint8_t)251);
    g.fillRect(55, 48, 1, 2, (uint8_t)251);
    g.fillRect(59, 48, 1, 2, (uint8_t)251);
    g.fillRect(63, 48, 1, 2, (uint8_t)251);
    g.fillRect(55, 50, 9, 4, (uint8_t)251);
    g.fillRect(55, 54, 9, 1, (uint8_t)214);
    tiny_text(g, "GIBSON", 68, 45, (uint8_t)251);
    tiny_text(g, "CREW DROP-DOWN", 68, 51, (uint8_t)146);
    tiny_text(g, "63 FINDS", 157, 48, (uint8_t)251);
    g.fillRect(205, 44, 1, 23, (uint8_t)1);
    g.fillRect(206, 44, 1, 23, (uint8_t)69);
    /* Open WIFI lane. */
    g.fillRect(34, 58, 3, 10, (uint8_t)87);
    g.fillRect(37, 58, 151, 10, (uint8_t)1);
    gib_lane_die_p(g, 57, (uint8_t)87, (uint8_t)151, (uint8_t)10);
    tiny_text(g, "WIFI", 52, 61, PAL_WHITE);
    tiny_text(g, "12", 165, 61, PAL_WHITE);
    for (int r = 0; r < 4; ++r) g.fillRect(178 + r, 62 + r, 7 - 2 * r, 1, (uint8_t)251);
    g.fillRect(186, 55, 5, 1, PAL_WHITE);
    g.fillRect(190, 56, 1, 4, PAL_WHITE);
    for (int r = 0; r < 3; ++r) g.fillRect(194 - r, 58 + r, 1 + 2 * r, 1, (uint8_t)69);
    /* Finds grid: 2 full rows + Bluetooth start, then empty slots. */
    for (int c = 0; c < 8; ++c) g.fillRect(33 + c * 22, c ? 68 : 93, 1, c ? 66 : 41, (uint8_t)1);
    for (int c = 0; c < 8; ++c) {
        if (c >= 2) g.fillRect(32 + c * 22, 68, 3, 3, (uint8_t)65);
        g.fillRect(32 + c * 22, 90, 3, 3, (uint8_t)65);
    }
    g.fillRect(32, 112, 3, 3, (uint8_t)65);
    g.fillRect(35, 113, 19, 1, (uint8_t)1);
    for (int c = 1; c < 8; ++c) {
        g.fillRect(32 + c * 22, 132, 19, 1, (uint8_t)1);
        g.fillRect(51 + c * 22, 113, 1, 20, (uint8_t)251);
    }
    paint_scanb_tiles_at_p(g, 71, 17, true);
    gib_lane_closed_p(g, 135, "BLUETOOTH", "12", (uint8_t)59, (uint8_t)155, (uint8_t)14, (uint8_t)50, 6);
    gib_lane_closed_p(g, 147, "DEAUTH", "4", (uint8_t)201, (uint8_t)210, (uint8_t)100, (uint8_t)165, 3);
    g.fillRect(192, 156, 5, 1, PAL_WHITE);
}
/* Tracker ring (mock B AirTag find / die glyph): 10x10, ink ring, white pip, smoke core. */
static void scanb_ring_p(Gfx g, int x, int y, uint8_t ink)
{
    static const char *const R[10] = {
        "..######..", ".########.", "###....###", "##..ww..##", "##.w..w.##",
        "##.w..w.##", "##..ww..##", "###....###", ".########.", "..######..",
    };
    for (int r = 0; r < 10; ++r)
        for (int c = 0; c < 10; ++c) {
            const char ch = R[r][c];
            if (ch == '.') continue;
            g.fillRect(x + c, y + r, 1, 1, ch == '#' ? ink : (uint8_t)PAL_WHITE);
        }
    g.fillRect(x + 3, y + 2, 4, 1, (uint8_t)68);
    g.fillRect(x + 2, y + 3, 2, 4, (uint8_t)68);
    g.fillRect(x + 6, y + 3, 2, 4, (uint8_t)68);
    g.fillRect(x + 4, y + 3, 2, 1, (uint8_t)68);
    g.fillRect(x + 4, y + 6, 2, 1, (uint8_t)68);
    g.fillRect(x + 3, y + 7, 4, 1, (uint8_t)68);
    g.fillRect(x + 4, y + 4, 2, 2, (uint8_t)68);
}
/* Gibson crew die (mock B): 24 px steel rim, slate core, silver castellations, one crew glyph. */
static void scanb_crew_die_p(Gfx g, int x, int y, bool bt)
{
    const uint8_t rim = (uint8_t)110, core = (uint8_t)41, pin = (uint8_t)186;
    g.fillRect(x, y, 24, 24, PAL_BLACK);
    g.drawRect(x, y, 24, 24, rim);
    g.drawRect(x + 3, y + 3, 18, 18, core);
    for (int k = 0; k < 5; ++k) {
        const int o = 5 + k * 3;
        g.fillRect(x + o, y, 2, 4, pin);
        g.fillRect(x + o, y + 20, 2, 4, pin);
        g.fillRect(x, y + o, 4, 2, pin);
        g.fillRect(x + 20, y + o, 4, 2, pin);
    }
    static const char *const WIFI_G[12] = {
        "....########....", "...###.....###..", "..##........###.", "..##........###.",
        "......#####.....", "....##.....#....", ".......###......", ".......###......",
        ".......###......", "................", ".......###......", ".......###......",
    };
    static const char *const BT_G[14] = {
        "......##......", "......##......", "......####....", "....###.###...",
        "...###.###....", "...###.###....", "....####......", "......####....",
        "...###.###....", "...###.###....", "....###.###...", "......####....",
        "......##......", "......##......",
    };
    const char *const *gl = bt ? BT_G : WIFI_G;
    const int rows = bt ? 14 : 12, gy = bt ? y + 4 : y + 6;
    const uint8_t ink = bt ? (uint8_t)59 : (uint8_t)87;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; gl[r][c]; ++c)
            if (gl[r][c] == '#') g.fillRect(x + 4 + c, gy + r, 1, 1, ink);
}

/* ============================================================================
 * Edge-board screens: Wi-Fi / BT radar, bot + Gibson SCAN, Rain, Defend chat, Diag.
 * Each one: e_board (side headers) -> parts -> silk -> castellated plate wired by traces -> e_finish
 * (top bar + gold edge). The scan parts are drawn in the mock-B portrait geometry and placed per
 * orientation through EShift.
 * ==========================================================================*/
struct EScanGeo { int pdx, pdy, cdx, cdy, fdx, fdy, scx, sy; bool fan; int plate[4]; };
static const EScanGeo &e_scan_geo(void)
{
    static const EScanGeo P = { 0, -11, 0, -11, 0, -11, 60, 183, true, { 36, 230, 168, 80 } };
    static const EScanGeo L = { 0, -11, 132, -150, 83, -93, 253, 134, false, { 30, 160, 184, 72 } };
    return botcorps_cyd_orient ? P : L;
}
static void e_silk_c(Gfx g, const char *t, int cx, int y, uint8_t c)
{
    if (!t || !*t) return;
    const int w = tiny_w(t);
    cbf(g, cx - w / 2 - 1, y - 1, w + 2, 7, PAL_BLACK);
    tiny_text(g, t, cx - w / 2, y, c);
}
/* silk block beside / under the MCU: name, module, role, device label (device ink) */
static void e_scan_silk(Gfx g, const char *nm, const char *l3, const char *lab, uint8_t ink)
{
    const EScanGeo &G = e_scan_geo();
    const uint8_t s = cb_silk();
    e_silk_c(g, nm, G.scx, G.sy, s);
    e_silk_c(g, "ESP32-WROOM-32", G.scx, G.sy + 7, s);
    e_silk_c(g, l3, G.scx, G.sy + 14, s);
    if (lab && *lab) { char u[24]; sb_upper(u, sizeof u, lab, 20); e_silk_c(g, u, G.scx, G.sy + 22, ink); }
}
/* MCU right pins -> peri left teeth (portrait only: the peri sits beside the chip) */
static void e_peri_fan(Gfx g, uint8_t tr)
{
    scanb_diag_p(g, 150, 191, 1, 1, 6, tr);
    scanb_diag_p(g, 150, 199, 1, 1, 2, tr);
    g.fillRect(152, 201, 4, 1, tr);
    g.fillRect(148, 205, 8, 1, tr);
    g.fillRect(152, 210, 4, 1, tr);
    scanb_diag_p(g, 151, 211, -1, 1, 2, tr);
    scanb_diag_p(g, 155, 215, -1, 1, 6, tr);
}
/* gold pins, shades and (portrait) the two crew dies + glyph: the Gibson MCU (mock-B geometry) */
static void e_gib_chip_extras(Gfx g, uint8_t tr, bool dies)
{
    static const int PX[5] = { 101, 109, 117, 125, 133 };
    for (int i = 0; i < 5; ++i) {
        g.fillRect(PX[i], 180, 2, 5, (uint8_t)177);
        g.fillRect(PX[i], 231, 2, 5, (uint8_t)177);
        g.fillRect(92, 189 + i * 8, 5, 2, (uint8_t)177);
        g.fillRect(143, 189 + i * 8, 5, 2, (uint8_t)177);
        g.fillRect(148, 190 + i * 8 - (i > 1 ? 1 : 0), 2, 1, PAL_BLACK);
    }
    g.fillRect(106, 196, 28, 1, PAL_BLACK);
    g.fillRect(109, 197, 9, 6, PAL_BLACK);
    g.fillRect(122, 197, 9, 6, PAL_BLACK);
    g.fillRect(110, 203, 7, 1, PAL_BLACK);
    g.fillRect(123, 203, 7, 1, PAL_BLACK);
    g.fillRect(111, 198, 2, 1, (uint8_t)110);
    g.fillRect(124, 198, 2, 1, (uint8_t)110);
    if (!dies) return;
    g.fillRect(148, 190, 8, 1, tr);
    g.fillRect(148, 198, 4, 1, tr); g.fillRect(152, 197, 1, 1, tr); g.fillRect(153, 196, 3, 1, tr);
    g.fillRect(148, 213, 4, 1, tr); g.fillRect(152, 214, 1, 1, tr); g.fillRect(153, 215, 3, 1, tr);
    g.fillRect(148, 221, 8, 1, tr);
    scanb_crew_die_p(g, 156, 178, false);
    scanb_crew_die_p(g, 156, 210, true);
    static const char *const RG[8] = {
        "..####..", ".##..##.", "##.ff.##", "#.f..f.#", "#.f..f.#", "##.ff.##", ".##..##.", "..####..",
    };
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            const char ch = RG[r][c];
            if (ch != '.') g.fillRect(190 + c, 176 + r, 1, 1, ch == '#' ? (uint8_t)109 : (uint8_t)41);
        }
}
/* bot SCAN panel (AirTag pattern): field, radar grid, tag with find trail and two tracker rings */
static void e_bot_panel(Gfx g, int bot, uint8_t ink)
{
    const uint8_t bc = bot_color(bot);
    paint_scanb_panel_p(g, false);
    g.fillRect(30, 42, 179, 117, PAL_FIELD);
    g.fillRect(204, 53, 1, 95, (uint8_t)69);
    for (int c = 0; c < 9; ++c) g.fillRect(34 + c * 19, 53, 1, 95, (uint8_t)1);
    for (int r = 0; r < 5; ++r) g.fillRect(34, 53 + r * 19, 170, 1, (uint8_t)1);
    for (int c = 0; c < 9; ++c)
        for (int r = 0; r < 6; ++r) {
            const int x0 = c ? 33 + c * 19 : 34, y0 = 52 + r * 19;
            g.fillRect(x0, y0, c ? 3 : 2, r < 5 ? 3 : 1, (uint8_t)65);
        }
    g.fillRect(143, 68, 1, 2, bc);
    g.fillRect(143, 70, 7, 2, bc);
    g.fillRect(143, 72, 1, 4, bc);
    g.fillRect(144, 72, 4, 4, (uint8_t)PAL_BLACK);
    g.fillRect(148, 72, 2, 4, bc);
    g.fillRect(143, 76, 7, 2, bc);
    g.fillRect(143, 78, 5, 4, bc);
    g.fillRect(144, 82, 4, 4, bc);
    g.fillRect(164, 60, 2, 2, (uint8_t)109); g.fillRect(164, 64, 2, 2, (uint8_t)109);
    g.fillRect(157, 69, 2, 2, (uint8_t)109); g.fillRect(157, 73, 2, 2, (uint8_t)109);
    scanb_ring_p(g, 160, 73, ink);
    scanb_ring_p(g, 153, 82, ink);
}

/* ============================================================================
 * SCANbot (checkpoint 1, 2026-10-25, Dirt): ONE unified scan board for every scan
 * enum (SCR_WIFI_RADAR / SCR_BT_RADAR / SCR_SCAN), portrait AND landscape.
 * Three selector chips (WIFIbot / BLUETOOTHbot / GIBSON) wired by traces, the active
 * one lit and the others dimmed; one shared tile box with >= 28 px hit cells and
 * readable device icons; Gibson-style category tabs fed ONLY by the real device
 * fields (sb_ty -> LIST_*_TY); a small silk 'SCANBOT' terminal plate at the foot
 * inside the board. Edge-to-edge board, full-length pins (e_board), rain stays off the
 * interior (e_covers). BOTCORPS_CYD only.
 * HONEST NOTE (per brief "band/security ... otherwise say so"): the firmware carries
 * NO per-device Wi-Fi security or radio-band field for the 35-entry scan list -- only
 * name, dBm, device type and a detect class. So the category tabs are driven by device
 * TYPE (sb_ty) for BOTH bands. There is no security/band field to tab on.
 * ==========================================================================*/
enum { SB_WIFI = 0, SB_BLE = 1, SB_GIB = 2 };
static int scanbot_active(void)
{
    return s_screen == SCR_WIFI_RADAR ? SB_WIFI : s_screen == SCR_BT_RADAR ? SB_BLE : SB_GIB;
}
static int scanbot_band(void) { return scanbot_active() == SB_BLE ? 1 : 0; }  /* gibson leads Wi-Fi band 0 */

struct SBGeo {
    int cx[3], cy, cw, ch;          /* selector chips */
    int tx0, ty, th;                /* category tab row (width = bw, split by ntab) */
    int bx, by, bw, bh;             /* shared scan box */
    int t0x, t0y, pitch, cols, rows, tile;   /* tile grid */
    int pl[4];                      /* SCANBOT plate: x y w h */
};
static const SBGeo &scanbot_geo(void)
{
    static SBGeo P, L;
    static bool init = false;
    if (!init) {
        init = true;
        /* portrait 240 x 320 */
        P.cx[0] = 34; P.cx[1] = 93; P.cx[2] = 152; P.cy = 30; P.cw = 54; P.ch = 28;
        P.tx0 = 34; P.ty = 62; P.th = 18;
        P.bx = 34; P.by = 84; P.bw = 172; P.bh = 172;
        P.t0x = 37; P.t0y = 88; P.pitch = 34; P.cols = 5; P.rows = 5; P.tile = 28;
        P.pl[0] = 34; P.pl[1] = 260; P.pl[2] = 172; P.pl[3] = 54;
        /* landscape 320 x 240 */
        L.cx[0] = 34; L.cx[1] = 120; L.cx[2] = 206; L.cy = 30; L.cw = 80; L.ch = 26;
        L.tx0 = 34; L.ty = 58; L.th = 18;
        L.bx = 34; L.by = 80; L.bw = 252; L.bh = 102;
        L.t0x = 37; L.t0y = 84; L.pitch = 31; L.cols = 8; L.rows = 3; L.tile = 28;
        L.pl[0] = 34; L.pl[1] = 186; L.pl[2] = 252; L.pl[3] = 46;
    }
    return botcorps_cyd_orient ? P : L;
}

/* GIBSON face (ivory + dark Agent Smith glasses) lives in hubbot.inc, included later. */
static void hb_gib_face(Gfx g, int cx, int cy, int s);

/* ---- SCANbot category + device model (cp1 fixes, 2026-10-25, Dirt) ----------
 * Cap at 4 tabs so they stay even and uncrowded (fix 5). WIFI/BLE bots tab by real
 * device TYPE (sb_ty). GIBSON is the combined all-radio view -- matching the old
 * b98559e Gibson accordion whose lanes were WIFI / BLUETOOTH / DEAUTH -- so it tabs
 * by BAND + the real DEAUTH detect class and its tiles span BOTH bands (fix 4).
 * Category match + device enumeration are shared by the painter and the touch
 * handler so the two never drift. */
enum { SBC_ALL = 0, SBC_TYPE, SBC_BAND, SBC_DEAUTH };
struct SBCat { uint8_t kind; int16_t arg; const char *lab; };

/* real detect class per device (DEV_*), from the static DT tables */
static int sb_dt(int band, int d) { return (band ? LIST_BLE_DT : LIST_WIFI_DT)[d]; }

/* short, centre-able tab label for a GL device type */
static const char *sb_type_lab(int t)
{
    switch (t) {
    case GL_AP: return "AP";
    case GL_PHONE: return "PHONE";
    case GL_LAPTOP: return "LAPTOP";
    case GL_TV: return "TV";
    case GL_SPEAKER: return "SPKR";
    case GL_TRACKER: return "TAG";
    case GL_WATCH: return "WATCH";
    case GL_CAMERA: return "CAM";
    case GL_CAR: return "CAR";
    case GL_DRONE: return "DRONE";
    case GL_DEVBOARD: return "DEV";
    case GL_PRINTER: return "PRINT";
    default: return "UNK";
    }
}

/* virtual device space for the active bot: gibson spans both bands, others one */
static int sbv_total(int act) { return act == SB_GIB ? SB_N * 2 : SB_N; }
static void sbv_resolve(int act, int v, int *band, int *d)
{
    if (act == SB_GIB) { if (v < SB_N) { *band = 0; *d = v; } else { *band = 1; *d = v - SB_N; } }
    else { *band = act == SB_BLE ? 1 : 0; *d = v; }
}

/* does device (band,d) fall under category c? */
static bool sb_cat_hit(const SBCat &c, int band, int d)
{
    switch (c.kind) {
    case SBC_TYPE: return sb_ty(band, d) == c.arg;
    case SBC_BAND: return band == c.arg;
    case SBC_DEAUTH: return sb_dt(band, d) == DEV_DEAUTH;
    default: return true;   /* SBC_ALL */
    }
}

/* fill cat[] (<= 4) for the active bot, return the tab count */
static int scanbot_cats_e(int act, SBCat *cat)
{
    cat[0].kind = SBC_ALL; cat[0].arg = 0; cat[0].lab = "ALL";
    if (act == SB_GIB) {
        cat[1].kind = SBC_BAND;   cat[1].arg = 0; cat[1].lab = "WIFI";
        cat[2].kind = SBC_BAND;   cat[2].arg = 1; cat[2].lab = "BT";
        cat[3].kind = SBC_DEAUTH; cat[3].arg = 0; cat[3].lab = "DEAUTH";
        return 4;
    }
    const int band = act == SB_BLE ? 1 : 0;
    int cnt[GL_N];
    for (int t = 0; t < GL_N; ++t) cnt[t] = 0;
    for (int i = 0; i < SB_N; ++i) { const int t = sb_ty(band, i); if (t >= 0 && t < GL_N) cnt[t]++; }
    int n = 1;
    for (int pick = 0; pick < 3; ++pick) {
        int best = -1, bc = 0;
        for (int t = 0; t < GL_N; ++t) {
            if (cnt[t] <= 0) continue;
            bool used = false;
            for (int k = 1; k < n; ++k) if (cat[k].arg == t) used = true;
            if (used) continue;
            if (cnt[t] > bc) { bc = cnt[t]; best = t; }
        }
        if (best < 0) break;
        cat[n].kind = SBC_TYPE; cat[n].arg = (int16_t)best; cat[n].lab = sb_type_lab(best);
        ++n;
    }
    return n;
}

/* one selector SCANBOT (cp1 fix 1): a little bot MCU chip -- body, castellation
 * teeth, side pins and a face -- with its full silk name UNDER it. Lit + powered
 * when active (bright rail, gold pins, white rim), dimmed otherwise.
 * kind 0 = WIFIbot, 1 = BLUETOOTHbot (themed faces); 2 = GIBSON (ivory, dark glasses,
 * never theme-coloured). */
static void sb_chip(Gfx g, int x, int y, int w, int h, int kind, const char *silk, uint8_t col, bool lit)
{
    (void)h;
    const uint8_t edge = lit ? (uint8_t)PAL_WHITE : (uint8_t)PAL_LINE;
    const uint8_t pin  = lit ? (uint8_t)177 : (uint8_t)COL_MUTED;   /* gold when powered, muted when dim */
    const int bw_ = w >= 70 ? 32 : 26;
    const int bh_ = 18;
    const int bx_ = x + (w - bw_) / 2;
    const int by_ = y;
    cbf(g, bx_, by_, bw_, bh_, (uint8_t)PAL_BLACK);
    g.drawRect(bx_, by_, bw_, bh_, edge);
    if (lit) cbf(g, bx_ + 1, by_ + 1, bw_ - 2, 1, col);            /* lit top rail = powered */
    for (int px = bx_ + 3; px + 3 <= bx_ + bw_ - 2; px += 6) { cbf(g, px, by_ - 2, 3, 2, pin); cbf(g, px, by_ + bh_, 3, 2, pin); }
    for (int py = by_ + 3; py + 2 <= by_ + bh_ - 2; py += 5) { cbf(g, bx_ - 2, py, 2, 2, pin); cbf(g, bx_ + bw_, py, 2, 2, pin); }
    const int fs = bh_ - 4;
    const int fcx = bx_ + bw_ / 2, fcy = by_ + bh_ / 2;
    if (kind == 2) {
        hb_gib_face(g, fcx, fcy, fs);                             /* always ivory + dark glasses */
    } else {
        const int fx = fcx - fs / 2, fy = fcy - fs / 2;
        cbf(g, fx, fy, fs, fs, lit ? col : (uint8_t)PAL_DEEP);
        g.drawRect(fx, fy, fs, fs, lit ? (uint8_t)PAL_WHITE : (uint8_t)PAL_STEEL);
        const int ew_ = fs / 4 > 2 ? fs / 4 : 2;
        const int ey = fy + fs / 3;
        const uint8_t eye = lit ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE;
        cbf(g, fx + fs / 4 - ew_ / 2, ey, ew_, ew_, eye);
        cbf(g, fx + 3 * fs / 4 - ew_ / 2, ey, ew_, ew_, eye);
        cbf(g, fx + fs / 4, fy + 2 * fs / 3, fs / 2, 1, eye);     /* mouth */
    }
    char nm[16];
    snprintf(nm, sizeof nm, "%.15s", silk);
    const int tw = tiny_w(nm);
    const int sx = (w - tw) / 2 > 0 ? x + (w - tw) / 2 : x;
    tiny_text(g, nm, sx, by_ + bh_ + 3, lit ? (uint8_t)PAL_WHITE : cb_silk(), w - 2);
}

/* one category tab */
static void sb_tab(Gfx g, int x, int y, int w, int h, const char *lab, uint8_t col, bool sel)
{
    cbf(g, x, y, w, h, sel ? col : (uint8_t)PAL_DEEP);
    g.drawRect(x, y, w, h, sel ? (uint8_t)PAL_WHITE : (uint8_t)PAL_LINE);
    char b[10];
    snprintf(b, sizeof b, "%.8s", lab);
    const int tw = tiny_w(b), tx = (w - tw) / 2 > 1 ? x + (w - tw) / 2 : x + 1;
    tiny_text(g, b, tx, y + (h - 5) / 2, sel ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE, w - 2);
}

/* Unified SCANbot board: every scan enum renders this, active bot from s_screen. */
static void paint_scan_e(Gfx g)
{
    const SBGeo &G = scanbot_geo();
    const int act = scanbot_active();          /* SB_WIFI / SB_BLE / SB_GIB */
    const uint8_t COL[3] = { (uint8_t)87, (uint8_t)59, (uint8_t)251 };   /* wifi / ble / gibson accents */
    const char *const NAME[3] = { "WIFIBOT", "BLUETOOTHBOT", "GIBSON" };

    e_board(g);
    cbf(g, 34, 27, EW() - 68, EH() - 32, (uint8_t)PAL_FIELD);   /* clean board under scanbots/tabs */

    /* ---- selector SCANBOTS, wired by a trace from the powered one into the box (fix 1) ---- */
    for (int i = 0; i < 3; ++i) sb_chip(g, G.cx[i], G.cy, G.cw, G.ch, i, NAME[i], COL[i], i == act);
    {
        const int ax = G.cx[act] + G.cw / 2;
        const Pt tp[3] = { { ax, G.cy + G.ch }, { ax, G.by - 6 }, { G.bx + G.bw / 2, G.by - 2 } };
        e_trace(g, tp, 3);
    }

    /* ---- category tabs: even widths, centred short labels (fix 5) ---- */
    SBCat cat[5];
    const int ntab = scanbot_cats_e(act, cat);
    if (s_scan_cat < 0 || s_scan_cat >= ntab) s_scan_cat = 0;
    for (int i = 0; i < ntab; ++i) {
        const int tx = G.tx0 + (G.bw * i) / ntab;
        const int tw = G.tx0 + (G.bw * (i + 1)) / ntab - tx;
        sb_tab(g, tx, G.ty, tw, G.th, cat[i].lab, COL[act], i == s_scan_cat);
    }

    /* ---- shared scan box ---- */
    cbf(g, G.bx, G.by, G.bw, G.bh, (uint8_t)PAL_BLACK);
    g.drawRect(G.bx, G.by, G.bw, G.bh, COL[act]);
    g.drawRect(G.bx + 1, G.by + 1, G.bw - 2, G.bh - 2, (uint8_t)PAL_LINE);

    /* tiles: the active bot's devices (gibson = BOTH bands) filtered by the selected category (fix 4) */
    const SBCat &fc = cat[s_scan_cat];
    const int total = sbv_total(act), cap = G.cols * G.rows;
    int slot = 0;
    for (int v = 0; v < total && slot < cap; ++v) {
        int band, d;
        sbv_resolve(act, v, &band, &d);
        if (!sb_cat_hit(fc, band, d)) continue;
        const int cxi = slot % G.cols, ryi = slot / G.cols;
        const int tx = G.t0x + cxi * G.pitch, tyy = G.t0y + ryi * G.pitch;
        const bool seld = (d == s_cursor && band == s_scan_selband);
        cbf(g, tx, tyy, G.tile, G.tile, seld ? (uint8_t)PAL_SEL : (uint8_t)PAL_DEEP);
        g.drawRect(tx, tyy, G.tile, G.tile, seld ? (uint8_t)PAL_WHITE : COL[act]);
        gl_icon_sz(g, sb_ty(band, d), tx + (G.tile - 18) / 2, tyy + 2, false, 18);
        char db[8];
        snprintf(db, sizeof db, "%d", sb_db(band, d));
        tiny_text(g, db, tx + 2, tyy + G.tile - 7, (uint8_t)PAL_WHITE, G.tile - 4);
        ++slot;
    }

    /* ---- SCANBOT silk terminal plate at the foot; buttons on the plate's right edge,
     * log/cmdline width capped so text never runs under them (fix 3) ---- */
    {
        const int *PL = G.pl;
        e_plate(g, PL[0], PL[1], PL[2], PL[3], "SCANBOT", 2, false, false, true);  /* 2 log lines: room in the small plate */
        const int mx = G.bx + G.bw / 2;
        const Pt fp[2] = { { mx, G.by + G.bh }, { mx, PL[1] } };
        e_trace(g, fp, 2);
    }
    /* (no bot-name silk inside the grid: silk lives only under the chips + plate title) */
    e_finish(g);
}

/* Rain: the Gibson ring disc in the middle of the board, silk under it */
static void e_rain_c(int *cx, int *cy) { *cx = botcorps_cyd_orient ? 120 : 160; *cy = botcorps_cyd_orient ? 160 : 128; }
static void paint_rain_e(Gfx g)
{
    int cx, cy; e_rain_c(&cx, &cy);
    e_board(g);
    { EShift s(g, cx - 160, cy - 100); paint_rain_btn(s.h); }
    e_silk_c(g, "GIBSON CORE", cx, cy + 62, cb_silk());
    e_silk_c(g, "TAP TO JACK IN", cx, cy + 70, cb_silk());
    e_finish(g);
}

/* Defend chat: the defending bot's MCU + silk title, chat output in the plate */
static void e_gchat_geo(int *pl, int *cdx, int *cdy)
{
    if (botcorps_cyd_orient) { pl[0] = 36; pl[1] = 124; pl[2] = 168; pl[3] = 186; *cdx = 0; *cdy = -140; }
    else { pl[0] = 108; pl[1] = 34; pl[2] = 180; pl[3] = 198; *cdx = -50; *cdy = -148; }
}
static void paint_gchat_e(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    const int b = (s_defend_bot >= 0 && s_defend_bot < 12) ? s_defend_bot : 12;
    int PL[4], cdx, cdy; e_gchat_geo(PL, &cdx, &cdy);
    e_board(g);
    const uint8_t tr = cb_trace();
    {
        EShift s(g, cdx, cdy);
        if (b >= 12) { paint_scanb_chip_p(s.h, (uint8_t)251, (uint8_t)255, (uint8_t)214, tr, false); e_gib_chip_extras(s.h, tr, false); }
        else paint_scanb_chip_p(s.h, bot_color(b), (uint8_t)250, (uint8_t)140, tr, true);
    }
    char nm[16];
    if (b >= 12) snprintf(nm, sizeof nm, "GIBSON");
    else sb_upper(nm, 16, BOTS[b].short_name, 12);
    const int scx = P ? 60 : 70, sy = P ? 52 : 96;
    e_silk_c(g, nm, scx, sy, cb_silk());
    e_silk_c(g, "DEFEND", scx, sy + 7, cb_silk());
    e_silk_c(g, "CHAT U1", scx, sy + 14, cb_silk());
    e_plate(g, PL[0], PL[1], PL[2], PL[3], "DEFEND CHAT", (PL[3] - 13 - 34) / 9, false);
    if (P) { g.fillRect(101, 96, 1, 26, tr); g.fillRect(125, 96, 1, 26, tr); }
    else { g.fillRect(98, 49, 10, 1, tr); g.fillRect(98, 65, 10, 1, tr); }
    e_finish(g);
}

/* Diag: the probe readout in a silk plate, wired to a probe chip */
static void e_diag_geo(int *pl)
{
    if (botcorps_cyd_orient) { pl[0] = 30; pl[1] = 34; pl[2] = 180; pl[3] = 200; }
    else { pl[0] = 30; pl[1] = 32; pl[2] = 190; pl[3] = 198; }
}
static void e_probe_chip(Gfx g, int x, int y)
{
    const uint8_t ring = (uint8_t)110, pin = (uint8_t)186;
    g.fillRect(x, y, 40, 40, PAL_BLACK);
    g.drawRect(x, y, 40, 40, ring);
    for (int k = 0; k < 4; ++k) {
        g.fillRect(x + 7 + k * 8, y - 3, 2, 3, pin); g.fillRect(x + 7 + k * 8, y + 40, 2, 3, pin);
        g.fillRect(x - 3, y + 7 + k * 8, 3, 2, pin); g.fillRect(x + 40, y + 7 + k * 8, 3, 2, pin);
    }
    g.fillRect(x + 8, y + 8, 24, 24, (uint8_t)PAL_DIAG);
    tiny_text(g, "PRB", x + 14, y + 17, PAL_BLACK);
}
static void paint_diag_e(Gfx g)
{
    const bool P = botcorps_cyd_orient != 0;
    int PL[4]; e_diag_geo(PL);
    e_board(g);
    int sx, sy, sw, sh;
    e_plate_shell(g, PL[0], PL[1], PL[2], PL[3], "DIAG", true, &sx, &sy, &sw, &sh);
    const botcorps_probe_t *p = botcorps_probe_get();
    struct spec { int id; const char *name; int kind; };
    static const spec rows[] = {
        { BOTCORPS_ST_M0, "M0", 0 }, { BOTCORPS_ST_M5, "M5", 0 }, { BOTCORPS_ST_E0, "E0", 0 },
        { BOTCORPS_ST_EXT, "EXT", 1 }, { BOTCORPS_ST_T0, "T0", 2 }, { BOTCORPS_ST_A0, "A0", 0 },
        { BOTCORPS_ST_A1, "A1", 3 }, { BOTCORPS_ST_SC0, "S0", 0 }, { BOTCORPS_ST_SC1, "S1", 4 },
        { BOTCORPS_ST_B0, "B0", 0 }, { BOTCORPS_ST_B1, "B1", 5 }, { BOTCORPS_ST_F0, "F0", 0 },
        { BOTCORPS_ST_SF, "SF", 6 }, { BOTCORPS_ST_W0, "W0", 0 }, { BOTCORPS_ST_W1, "W1", 7 },
        { BOTCORPS_ST_OV0, "O0", 0 }, { BOTCORPS_ST_OV1, "O1", 8 }, { BOTCORPS_ST_N2, "N2", 9 },
    };
    const int maxc = (sw - 6) / 4;
    char line[64], up[64];
    if (p) {
        for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); ++i) {
            char tail[16];
            tail[0] = 0;
            switch (rows[i].kind) {
            case 1: snprintf(tail, sizeof(tail), "%s", p->ext_st); break;
            case 2: snprintf(tail, sizeof(tail), "%lu", (unsigned long)p->tier0); break;
            case 3: snprintf(tail, sizeof(tail), "%s %d", p->nim_a, p->nim_a_rc); break;
            case 4: snprintf(tail, sizeof(tail), "spr%d", p->sprite_held); break;
            case 5: snprintf(tail, sizeof(tail), "%s %d", p->nim_b, p->nim_b_rc); break;
            case 6: snprintf(tail, sizeof(tail), "free%d", p->sprite_freed); break;
            case 7: snprintf(tail, sizeof(tail), "n%d", p->wifi_n); break;
            case 8: snprintf(tail, sizeof(tail), "%d", p->ov_ok); break;
            case 9: snprintf(tail, sizeof(tail), "%s %d", p->nim_2, p->nim_2_rc); break;
            default: break;
            }
            snprintf(line, sizeof(line), "%02d %s %lu L%lu %lu %s", p->ord[rows[i].id], rows[i].name,
                     (unsigned long)p->st[rows[i].id].free_b, (unsigned long)p->st[rows[i].id].max_b,
                     (unsigned long)p->st[rows[i].id].min_b, tail);
            sb_upper(up, sizeof up, line, maxc);
            tiny_text(g, up, sx + 3, sy + 3 + i * 7, PAL_WHITE);
        }
    }
    snprintf(line, sizeof(line), "%s %s L%lu", botcorps_band_mode_name(), botcorps_band_direct() ? "direct" : "ok",
             (unsigned long)botcorps_band_largest());
    sb_upper(up, sizeof up, line, maxc);
    tiny_text(g, up, sx + 3, sy + 3 + 18 * 7 + 2, accent());
    if (g.raw()) {
        for (int i = 0; i < BOTCORPS_ICON_N && sx + 4 + i * 16 + 14 <= sx + sw; ++i)
            botcorps_glyph_blit(*g.raw(), sx + 4 + i * 16 - g.ox, sy + 3 + 18 * 7 + 12 - g.oy, i, PAL_DIAG, PAL_MUTED, PAL_WHITE, nullptr);
    }
    const uint8_t tr = cb_trace();
    if (P) {
        e_probe_chip(g, 100, 252);
        g.fillRect(118, PL[1] + PL[3] + 2, 1, 252 - 3 - (PL[1] + PL[3] + 2), tr);
        e_silk_c(g, "PROBE U9", 120, 300, cb_silk());
    } else {
        e_probe_chip(g, 236, 60);
        g.fillRect(PL[0] + PL[2], 79, 236 - 3 - (PL[0] + PL[2]), 1, tr);
        e_silk_c(g, "PROBE U9", 256, 108, cb_silk());
    }
    e_finish(g);
}

/* rain covers for the live-rain edge screens (hub / Control have their own) */
static void e_covers(void)
{
    const int W = EW(), H = EH();
    const bool P = botcorps_cyd_orient != 0;
    cover_add(0, 0, W, 27);
    cover_add(0, 27, 33, H - 27);
    cover_add(W - 33, 27, 33, H - 27);
    cover_add(0, H - 4, W, 4);
    if (s_screen == SCR_RAIN) {
        int cx, cy; e_rain_c(&cx, &cy);
        cover_add(cx - 57, cy - 57, 114, 114);
        cover_add(cx - 32, cy + 60, 64, 17);
    } else if (s_screen == SCR_GCHAT) {
        if (P) { cover_add(28, 34, 184, 64); cover_add(96, 96, 36, 28); cover_add(34, 120, 172, 193); }
        else { cover_add(28, 28, 82, 84); cover_add(96, 40, 14, 30); cover_add(106, 30, 184, 206); }
    } else if (s_screen == SCR_DIAG) {
        int PL[4]; e_diag_geo(PL);
        cover_add(PL[0] - 2, PL[1] - 2, PL[2] + 4, PL[3] + 4);
        if (P) cover_add(94, 230, 52, 76); else cover_add(218, 54, 72, 62);
    } else {
        cover_add(28, 28, W - 56, H - 32);   /* scan boards: the whole interior is parts */
    }
}


#include "ui/botcorps_cyd_hubbot.inc"   /* Gibson Hub v4 (both orientations) */
#include "ui/botcorps_cyd_intro.inc"    /* BOOTbot intro from setup(), before the hub (PICK.md) */

static void paint_ext_portrait(Gfx g)
{
    g.clearClipRect();
    g.setTextWrap(false, false);
    s_pt_bw = 0;                         /* no plate until a painter draws one */
    if (intro_on()) {                    /* BOOTbot intro: static rain under its covers, then the frame */
        IntroSt is; intro_state(is);
        botcorps_rain_set_hub(is.scr != 0);
        g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
        if (is.rain && g.raw()) { const int y0 = botcorps_band_oy(); botcorps_rain_draw(*g.raw(), y0, y0 + g.raw()->height()); }
        paint_intro_e(g);
        return;
    }
    const bool hub = s_screen == SCR_GIBSON;
    const bool ctl = s_screen == SCR_CONTROL;
    botcorps_rain_set_hub(hub || ctl);
    rain_flags();
    const bool rain_bg = (wants_rain() || rain_static()) && !zap_sb_on();
    if (hub || ctl) g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
    else g.fillScreen(COL_FIELD);
    if (rain_bg && g.raw()) {
        const int y0 = botcorps_band_oy();
        botcorps_rain_draw(*g.raw(), y0, y0 + g.raw()->height());
    }
    switch (s_screen) {
    case SCR_RAIN: paint_rain_e(g); break;
    case SCR_GCHAT: paint_gchat_e(g); break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR:
    case SCR_SCAN: paint_scan_e(g); break;
    case SCR_CONTROL: paint_ctlbot(g); break;
    case SCR_DIAG: paint_diag_e(g); break;
    default: paint_hubbot(g); break;   /* SCR_GIBSON (and anything unmapped lands on the hub) */
    }
    if (fa_live()) paint_found_alert(g);
}
