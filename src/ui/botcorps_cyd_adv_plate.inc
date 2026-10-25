/* CYD Adv terminal plate — port of f1c08aa paint_prompt / paint_cands / sheet /
 * palette / attack chrome into the edge-board plate rect. BOTCORPS_CYD only.
 * Replaces the b98559e pt_paint_old0 / old12 / abc+list keyboard look. */

/* Plate-local geometry (set each paint). Art stays Adv-sized (6px Font0, 20x12 btns). */
static int s_adv_sx, s_adv_sy, s_adv_sw, s_adv_sh;
static int s_adv_py;          /* prompt y (screen) */
static int s_adv_cmd_x, s_adv_atk_x, s_adv_btn_y, s_adv_atk_y;
/* OSK-open input row: full screen width, directly above the bottom-anchored OSK. */
static const int CYD_ADV_ROW_H = 15;
static int s_adv_row_y = -1;
static bool s_cyd_adv_focus = false;   /* cmdline / OSK owns typing into term prompt */

static void cyd_adv_geo(int sx, int sy, int sw, int sh)
{
    s_adv_sx = sx; s_adv_sy = sy; s_adv_sw = sw; s_adv_sh = sh;
    s_adv_py = sy + sh - 11;                          /* Adv: PROMPT_Y = plate_bot - 8 */
    if (s_adv_py < sy + 10) s_adv_py = sy + 10;
    /* ≡ and reticle stacked in the plate's top-right corner (owner 5:45 PM): one 28 px
     * pad column [sx+sw-28, sx+sw); ≡ pad rows [sy, sy+28), reticle [sy+28, sy+56).
     * The cmdline below gets the full plate width. Art stays Adv 20x12. */
    s_adv_cmd_x = sx + sw - 24;                       /* art x; pad x = art+10-14 = sx+sw-28 */
    s_adv_atk_x = s_adv_cmd_x;
    s_adv_btn_y = sy + 8;                             /* ≡ art; pad centred = sy .. sy+28 */
    s_adv_atk_y = sy + 36;                            /* reticle art; pad sy+28 .. sy+56 */
}
/* Left edge of the button pad column: log text, scanline and overlays stop 2 px short. */
static int cyd_adv_col_x(void) { return s_adv_cmd_x - 4; }

/* Adv cmdline (glow user@botcorps, accent :~$, white typed, accent caret / focus box).
 * px = prefix x, fx0/fx1 = focus box edges (also the text's right limit). Longer input
 * shows its tail so the caret end stays visible; the caret always has room. Returns the
 * number of typed chars that fit. */
static int cyd_adv_cmdline(Gfx g, int px, int y, int fx0, int fx1, bool pf)
{
    {
        txt(g, "user@botcorps", px - 1, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", px + 1, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", px, y + 1, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", px, y, (uint8_t)194, lgfx::textdatum::top_left);
    }
    txt(g, ":~$", px + 13 * 6 + 1, y, accent(), lgfx::textdatum::top_left);
    txt(g, ":~$", px + 13 * 6, y, accent(), lgfx::textdatum::top_left);

    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(COL_WHITE);
    const char *typed = botcorps_t0_term_prompt;
    const int typed_x = px + 16 * 6;   /* "user@botcorps" + ":~$" = 13+3 chars */
    int typed_max = (fx1 - typed_x - 8) / 6;   /* 2 px gap + 6 px caret inside fx1 */
    if (typed_max < 1) typed_max = 1;
    const int tlen = (int)strlen(typed);
    const char *showt = typed;
    if (tlen > typed_max) showt = typed + (tlen - typed_max);
    g.setCursor(typed_x, y);
    g.print(showt);
    const int cx = typed_x + (int)strlen(showt) * 6 + 2;
    if (pf) {
        const int fy = y - 3;
        g.fillRect(fx0, fy, fx1 - fx0, 1, accent());
        g.fillRect(fx0, fy, 1, 11, accent());
        g.fillRect(fx1, fy, 1, 11, accent());
    }
    const bool caret = pf && typed[0] ? true : (millis() / 530u) % 2u == 0;
    if (caret && cx + 6 <= fx1) g.fillRect(cx, y, 6, 8, accent());
    return typed_max;
}

/* Invisible touch pads >=28x28 around Adv-sized 20x12 art. */
static void cyd_adv_btn_hit(int art_x, int art_y, int *hx, int *hy, int *hw, int *hh)
{
    const int aw = 20, ah = 12;
    int pad = 28;
    *hx = art_x + aw / 2 - pad / 2;
    *hy = art_y + ah / 2 - pad / 2;
    *hw = pad; *hh = pad;
    if (*hx < s_adv_sx) *hx = s_adv_sx;
    if (*hy < s_adv_sy) *hy = s_adv_sy;
    if (*hx + *hw > s_adv_sx + s_adv_sw) *hx = s_adv_sx + s_adv_sw - *hw;
    if (*hy + *hh > s_adv_sy + s_adv_sh) *hy = s_adv_sy + s_adv_sh - *hh;
}

/* ---- paint_cands port. Closed OSK: inside the plate, left of the button column.
 * Open OSK: just above the full-width input row, anchored at the typed end. ---- */
struct CydAdvCands {
    const char *rows[8];
    int n, vis, start, sel, bx, by, w, h, rh, head;
    bool sp;
    char cmd[16];
};
static bool cyd_adv_cands_calc(CydAdvCands *c)
{
    const char *buf = botcorps_t0_term_prompt;
    if (!buf[0] || s_sheet || s_focus_cmds || s_pt_bw <= 0) return false;
    const bool osk = s_pt_mode == 1 && s_adv_row_y >= 0;
    const char *sp = strchr(buf, ' ');
    const char *arg = "";
    c->cmd[0] = 0;
    c->sp = sp != nullptr;
    if (sp) {
        int n = (int)(sp - buf);
        if (n > 15) n = 15;
        memcpy(c->cmd, buf, (size_t)n);
        c->cmd[n] = 0;
        arg = sp + 1;
    }
    int n = 0;
    if (!sp) {
        const size_t bn = strlen(buf);
        for (int i = 0; i < PCMD_N && n < 8; ++i) {
            if (strncmp(PCMD[i], buf, bn) == 0) { c->rows[n] = PCMD[i]; ++n; }
        }
    } else {
        n = opt_list(c->cmd, arg, c->rows, 8);
    }
    if (n <= 0) return false;
    c->n = n;
    c->rh = 12;
    c->head = sp ? 10 : 0;
    /* vertical room: OSK row top (open) or the prompt (closed) down from the top limit */
    const int lim_top = osk ? 26 : s_adv_sy + 2;
    const int lim_bot = osk ? s_adv_row_y - 1 : s_adv_py - 4;
    int room = (lim_bot - lim_top - 6 - c->head) / c->rh;
    if (room < 1) room = 1;
    int vis = n > 6 ? 6 : n;
    if (vis > room) vis = room;
    c->vis = vis;
    int sel = 0;
    if (!sp && s_tab_i >= 0 && s_tab_i < n) sel = s_tab_i;
    c->sel = sel;
    int start = sel - 2;
    if (start < 0) start = 0;
    if (start > n - vis) start = n - vis;
    c->start = start;
    int w = 40;
    for (int i = 0; i < vis; ++i) {
        int ww = (int)strlen(c->rows[start + i]) * 6 + 14;
        if (ww > w) w = ww;
    }
    if (sp) {
        const int hw = (int)strlen(c->cmd) * 6 + 22;
        if (hw > w) w = hw;
    }
    w += 6;
    const int left = osk ? 2 : s_adv_sx + 2;
    const int right = osk ? EW() - 2 : cyd_adv_col_x() - 2;   /* closed: clear of ≡ / reticle */
    if (w > right - left) w = right - left;
    if (w < 40) w = 40;
    c->w = w;
    c->h = vis * c->rh + 6 + c->head;
    int bx;
    if (osk) {
        /* typed end in the row (tail shown when long) */
        const int typed_x = 3 + 16 * 6;
        int typed_max = (EW() - 4 - typed_x - 8) / 6;
        int tl = (int)strlen(buf);
        if (tl > typed_max) tl = typed_max;
        bx = typed_x + tl * 6 + 10;
    } else {
        bx = s_adv_sx + 6 + (18 + (int)strlen(buf)) * 6 + 14;
    }
    if (bx + w > right) bx = right - w;
    if (bx < left) bx = left;
    c->bx = bx;
    int by = lim_bot - c->h;
    if (by < lim_top) by = lim_top;
    c->by = by;
    return true;
}

static void cyd_adv_paint_cands(Gfx g)
{
    CydAdvCands c;
    if (!cyd_adv_cands_calc(&c)) return;
    const int bx = c.bx, by = c.by, w = c.w, h = c.h, rh = c.rh;
    plate(g, bx, by, w, h, COL_BG, accent());
    int y = by + 4;
    if (c.sp) {
        txt_bg(g, c.cmd, bx + 6, y - 1, accent(), COL_BG, lgfx::textdatum::top_left);
        txt_bg(g, ">", bx + 8 + (int)strlen(c.cmd) * 6, y - 1, COL_MUTED, COL_BG, lgfx::textdatum::top_left);
        y += 10;
    }
    for (int k = 0; k < c.vis; ++k) {
        const int on = (c.start + k) == c.sel;
        const uint8_t bg = on ? (uint8_t)COL_SEL : (uint8_t)COL_BG;
        if (on) {
            g.fillRect(bx + 2, y - 1, w - 4, rh - 1, COL_SEL);
            g.fillRect(bx + 2, y - 1, 2, rh - 1, accent());
        }
        txt_bg(g, c.rows[c.start + k], bx + 6, y, on ? COL_WHITE : COL_INK, bg, lgfx::textdatum::top_left);
        y += rh;
    }
}

/* ---- paint_prompt port into plate ---- */
static void cyd_adv_paint_prompt(Gfx g)
{
    const int sx = s_adv_sx, sy = s_adv_sy, sw = s_adv_sw, sh = s_adv_sh;
    const int keep = s_px;
    s_px = 6;
    g.fillRect(sx, sy + 1, sw, sh - 1, PAL_BLACK);
    {
        const bool pulse = s_mood_until != 0 && (int32_t)(millis() - s_mood_until) < 0;
        g.fillRect(sx, sy, sw, 1, pulse ? (uint8_t)PAL_WHITE : term_rim(0));
    }
    {
        const int span = sh > 20 ? sh - 20 : 8;
        int scy = sy + 4 + (int)((millis() / 28u) % (uint32_t)span);
        if (scy > s_adv_py - 4) scy = s_adv_py - 4;
        const int slw = cyd_adv_col_x() - 2 - (sx + 4);   /* stops short of the button column */
        if (slw > 8) g.fillRect(sx + 4, scy, slw, 1, th_wire());
    }
    g.fillRect(sx, sy + sh - 1, sw, 1, term_rim(1));
    g.fillRect(sx, sy + 1, 1, sh - 2, term_rim(0));
    g.fillRect(sx + sw - 1, sy + 1, 1, sh - 2, term_rim(1));

    /* log: last N lines, 8 px pitch, Font0 via txt_bg, pale cyan #bff6ff */
    int pick[8];
    int n = 0;
    const int max_rows = (s_adv_py - (sy + 4)) / 8;
    const int nrows = max_rows > 8 ? 8 : (max_rows < 0 ? 0 : max_rows);
    for (int i = s_log_n - 1; i >= 0 && n < nrows; --i) {
        const char *l = log_line(i);
        if (l && strncmp(l, "gibson>", 7) == 0) continue;
        pick[n++] = i;
    }
    for (int a = 0, b = n - 1; a < b; ++a, --b) { const int t = pick[a]; pick[a] = pick[b]; pick[b] = t; }
    const int lx = sx + 6;
    const int maxc = (cyd_adv_col_x() - 2 - lx) / 6;   /* log rows stop short of ≡ / reticle pads */
    for (int k = 0; k < n; ++k) {
        const char *ln = log_line(pick[k]);
        const int ly = sy + 4 + k * 8;
        const uint8_t pale = botcorps_pal_fit(0xBFBF);
        uint8_t ink = pale;
        const char *show = ln;
        if (ln && ln[0] == '\x01') {
            const int b = (unsigned char)ln[1];
            ink = b >= 12 ? (uint8_t)COL_IVORY : bot_color(b);
            show = ln + 2;
        } else if (ln && ln[0] == '[') {
            const char *rb = strchr(ln, ']');
            if (rb) {
                char tag[12];
                int tn = (int)(rb - ln) + 1;
                if (tn > 11) tn = 11;
                for (int i = 0; i < tn; ++i) tag[i] = ln[i];
                tag[tn] = 0;
                txt_bg(g, tag, lx, ly, (uint8_t)PAL_OK, PAL_BLACK, lgfx::textdatum::top_left);
                if (rb[1]) {
                    char rest[48];
                    int ri = 0;
                    for (const char *p = rb + 1; *p && ri < maxc - tn && ri < 47; ++p) rest[ri++] = *p;
                    rest[ri] = 0;
                    txt_bg(g, rest, lx + tn * 6, ly, pale, PAL_BLACK, lgfx::textdatum::top_left);
                }
                continue;
            }
        }
        if (show) {
            char cut[64];
            int ci = 0;
            for (const char *p = show; *p && ci < maxc && ci < 63; ++p) cut[ci++] = *p;
            cut[ci] = 0;
            if (paint_hash_bar(g, cut, lx, ly, ink)) continue;
            txt_bg(g, cut, lx, ly, ink, PAL_BLACK, lgfx::textdatum::top_left);
        }
    }

    /* cmdline: user@botcorps (idx 194 + glow) :~$ accent, typed white, scroll on overflow.
     * Full plate width now that ≡ / reticle sit in the top-right column. */
    const bool pf = (s_cyd_adv_focus || s_term_prompt || s_pt_mode == 1) && !s_sheet && !s_focus_cmds && !s_cmd_btn;
    cyd_adv_cmdline(g, sx + 5, s_adv_py, sx + 2, sx + sw - 4, pf);

    if (s_pt_mode != 1) cyd_adv_paint_cands(g);   /* OSK open: cands ride above the input row */

    /* ≡ chip palette button */
    {
        const bool po = (s_focus_cmds || s_cmd_btn) && !s_sheet;
        plate(g, s_adv_cmd_x, s_adv_btn_y, 20, 12, po ? COL_SEL : COL_DEEP, po ? accent() : COL_LINE);
        if (po) g.drawRect(s_adv_cmd_x - 1, s_adv_btn_y - 1, 22, 14, PAL_WHITE);
        for (int k = 0; k < 3; ++k)
            g.fillRect(s_adv_cmd_x + 5, s_adv_btn_y + 3 + k * 3, 10, 1, po ? (uint8_t)PAL_WHITE : (uint8_t)COL_MUTED);
    }
    /* reticle attack button */
    {
        const bool ao = (s_atk_btn || s_term_ovl == 1 || s_term_ovl == 2) && !s_sheet;
        const int ax = s_adv_atk_x, ay = s_adv_atk_y;
        plate(g, ax, ay, 20, 12, ao ? COL_SEL : COL_DEEP, ao ? accent() : COL_LINE);
        if (ao) g.drawRect(ax - 1, ay - 1, 22, 14, PAL_WHITE);
        const uint8_t ic = ao ? (uint8_t)PAL_WHITE : (uint8_t)COL_MUTED;
        g.drawCircle(ax + 10, ay + 6, 3, ic);
        g.fillRect(ax + 10, ay + 1, 1, 3, ic);
        g.fillRect(ax + 10, ay + 9, 1, 2, ic);
        g.fillRect(ax + 5, ay + 6, 3, 1, ic);
        g.fillRect(ax + 13, ay + 6, 3, 1, ic);
        g.fillRect(ax + 10, ay + 6, 1, 1, ao ? (uint8_t)PAL_WHITE : ic);
    }
    s_px = keep;
}

/* ---- palette over plate (paint_palette port) ----
 * Sits left of the ≡ / reticle column so ≡ stays visible to close it. 6x2 when a chip
 * is >= 30 px (landscape), else 4x3 (portrait) so the labels fit their chips. */
static void cyd_adv_pal_geo(int *x0, int *y0, int *aw, int *cols, int *cw, int *pitch)
{
    *x0 = s_adv_sx + 1;
    *y0 = s_adv_sy + 2;
    *aw = cyd_adv_col_x() - 2 - *x0;
    *cols = (*aw / 6 - 3 >= 30) ? 6 : 4;
    *cw = *aw / *cols;
    *pitch = *cols == 6 ? 18 : 17;
}
static bool cyd_adv_pal_chip(int n, int *x, int *y, int *ww, int *ch)
{
    int x0, y0, aw, cols, cw, pitch;
    cyd_adv_pal_geo(&x0, &y0, &aw, &cols, &cw, &pitch);
    *x = x0 + 3 + (n % cols) * cw;
    *y = y0 + 2 + (n / cols) * pitch;
    *ww = cw - 3;
    *ch = 15;
    return *ww >= 20;
}
static void cyd_adv_paint_palette(Gfx g)
{
    if (!s_focus_cmds || s_sheet) return;
    int x0, y0, aw, cols, cw, pitch;
    cyd_adv_pal_geo(&x0, &y0, &aw, &cols, &cw, &pitch);
    const int rows = 12 / cols;
    const int bh = rows * pitch + 2;
    g.fillRect(x0, y0, aw, bh > s_adv_sh - 14 ? s_adv_sh - 14 : bh, COL_DEEP);
    for (int n = 0; n < 12; ++n) {
        int x, y, ww, ch;
        if (!cyd_adv_pal_chip(n, &x, &y, &ww, &ch)) continue;
        const bool sel = n == s_cmd_sel;
        plate(g, x, y, ww, ch, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt_bg(g, CMDS[n], x + ww / 2, y + 4, sel ? COL_WHITE : COL_MUTED, sel ? COL_SEL : COL_BG,
               lgfx::textdatum::top_center);
    }
}

/* ---- glossary sheet opens upward from prompt (paint_sheet geometry) ---- */
/* Portrait: cmd rows get a second line for purpose when name+purpose do not
 * fit (dim, indented). Landscape keeps Adv one-line right-aligned purposes.
 * Row pitch is variable; hit tests walk cumulative Y. */
/* Sheet width: full plate width (sw-4) as before. The sheet is modal and covers the
 * top-right ≡ / reticle column while open; Back / tap outside closes it. A narrower
 * sheet dropped purpose words in portrait. */
static int cyd_adv_sheet_w(void)
{
    int w = s_adv_sw - 4;
    if (w > 292) w = 292;
    if (w < 80) w = 80;
    return w;
}
static int cyd_adv_sheet_row_h(int hdr, int cmd, int sheet_w)
{
    if (hdr >= 0) return 12;
    if (botcorps_cyd_orient == 0) return 12; /* landscape: Adv one-line */
    /* portrait: wrap purpose under the name when it would collide */
    int name_w = 0;
    for (const char *p = CMDN[cmd]; *p; ++p) name_w += 6;
    const int name_end = 24 + name_w + 6; /* relative to bx */
    const int pur_right = sheet_w - 8;
    int max_pw = pur_right - name_end;
    if (max_pw < 0) max_pw = 0;
    if (purpose_px(CMDP[cmd]) <= max_pw) return 12;
    return 22; /* name line + purpose line */
}

static void cyd_adv_sheet_box(int *by, int *h, int *top_out, int *nshow_out)
{
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    const int sel_row = menu_sel_row(sel);
    const int sheet_w = cyd_adv_sheet_w();
    const int max_h = (s_adv_py - 5) - 2; /* y min 2 .. above prompt */
    int budget = max_h - 10;
    if (budget < 24) budget = 24;

    /* Prefer centering the selection; grow nshow until height budget fills. */
    int top = sel_row;
    int nshow = 0;
    int used = 0;
    /* seed with the selected row */
    {
        int hdr = -1, cmd = 0;
        menu_row(sel_row, &hdr, &cmd);
        used = cyd_adv_sheet_row_h(hdr, cmd, sheet_w);
        nshow = 1;
        top = sel_row;
    }
    while (true) {
        bool grew = false;
        if (top > 0) {
            int hdr = -1, cmd = 0;
            menu_row(top - 1, &hdr, &cmd);
            const int rh = cyd_adv_sheet_row_h(hdr, cmd, sheet_w);
            if (used + rh <= budget) {
                top -= 1; used += rh; nshow += 1; grew = true;
            }
        }
        if (top + nshow < rows) {
            int hdr = -1, cmd = 0;
            menu_row(top + nshow, &hdr, &cmd);
            const int rh = cyd_adv_sheet_row_h(hdr, cmd, sheet_w);
            if (used + rh <= budget) {
                used += rh; nshow += 1; grew = true;
            }
        }
        if (!grew) break;
    }
    const int hh = used + 10;
    int y = s_adv_py - 5 - hh;
    if (y < 2) y = 2;
    *by = y;
    *h = hh;
    *top_out = top;
    *nshow_out = nshow;
}

/* Cumulative Y of visible row r (0..nshow-1) relative to by+5. */
static int cyd_adv_sheet_row_y(int top, int r, int sheet_w)
{
    int y = 0;
    for (int i = 0; i < r; ++i) {
        int hdr = -1, cmd = 0;
        menu_row(top + i, &hdr, &cmd);
        y += cyd_adv_sheet_row_h(hdr, cmd, sheet_w);
    }
    return y;
}

static void cyd_adv_paint_sheet(Gfx g)
{
    /* Adv paint_sheet onto the CYD plate: icons, CORE glow, single magenta
     * rim (term_ov). Portrait wraps purposes on a second line — no mid-word cuts. */
    if (!s_sheet) return;
    const int keep = s_px;
    s_px = 6;
    int by = 0, h = 0, top = 0, nshow = 0;
    cyd_adv_sheet_box(&by, &h, &top, &nshow);
    const int bx = s_adv_sx + 2;
    const int w = cyd_adv_sheet_w();
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    const bool port = botcorps_cyd_orient != 0;

    GATE_BOX(g, "OCCLUDE", "sheet", bx, by, w + 1, h + 1, "-");
    g.fillRect(bx + 1, by + 1, w - 1, h - 1, PAL_BG);
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
        const int rh = cyd_adv_sheet_row_h(hdr, cmd, w);
        g.fillRect(bx + 4, y, w - 8, rh - 1, PAL_BG);
    }
    const int x1 = bx + w;
    const int y1 = by + h;
    sheet_ramp_top(g, bx, by, w, h);

    /* CORE glow / group headers */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr < 0) continue;
        const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
        if (hdr == 0) {
            const uint8_t ringc[5] = {
                PAL_TERM_M35, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M10, PAL_TERM_M10
            };
            const int gx = bx + 8, gy = y + 1;
            const int keepf = s_px; s_px = 6;
            for (int d = 5; d >= 1; --d) {
                const uint8_t c = ringc[d - 1];
                for (int dy = -d; dy <= d; ++dy) {
                    for (int dx = -d; dx <= d; ++dx) {
                        const int ax = dx < 0 ? -dx : dx;
                        const int ay = dy < 0 ? -dy : dy;
                        if ((ax > ay ? ax : ay) != d) continue;
                        txt(g, G_NAME[hdr], gx + dx, gy + dy, c, lgfx::textdatum::top_left);
                    }
                }
            }
            txt(g, G_NAME[hdr], gx, gy, PAL_TERM_M, lgfx::textdatum::top_left);
            s_px = keepf;
            sheet_corner(g, bx, by, 1, 1);
        } else {
            glow_txt(g, G_NAME[hdr], bx + 8, y + 1, PAL_BG, 6);
        }
    }
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr < 0) continue;
        const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
        if (hdr == 0) g.fillRect(bx + 48, y + 5, w - 60, 1, PAL_TERM_M22);
        else glow_hline(g, bx + 48, y + 5, w - 60, STEP_RULE);
    }
    /* single magenta bottom ramp (Adv) */
    g.fillRect(bx + 6, y1 - 4, w - 12, 1, PAL_TERM_M28);
    g.fillRect(bx + 1, y1, w - 1, 1, PAL_TERM_M);
    g.fillRect(bx + 3, y1 - 1, w - 5, 1, PAL_TERM_M35);
    g.fillRect(bx + 3, y1 - 2, w - 5, 1, PAL_TERM_M22);
    g.fillRect(bx + 3, y1 - 3, w - 5, 1, PAL_TERM_M18);
    g.fillRect(x1 - 6, y1 - 4, 1, 1, PAL_TERM_M10);
    g.fillRect(bx + 5, y1 - 5, w - 9, 1, PAL_TERM_M10);
    {
        int hdr0 = -1, cmd0 = 0;
        menu_row(top, &hdr0, &cmd0);
        if (hdr0 >= 0) {
            g.fillRect(bx + 40, by + 6, w - 70, 1, PAL_TERM_M22);
            g.fillRect(bx + 40, by + 7, w - 70, 1, PAL_TERM_M18);
            g.fillRect(bx + 40, by + 8, w - 70, 1, PAL_TERM_M10);
        }
        for (int r = 0; r < nshow; ++r) {
            int hdr = -1, cmd = 0;
            menu_row(top + r, &hdr, &cmd);
            if (hdr >= 0) continue;
            const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
            const int rh = cyd_adv_sheet_row_h(hdr, cmd, w);
            int ys = y, hs = rh - 1;
            if (ys < by + 6) { const int cut = (by + 6) - ys; ys += cut; hs -= cut; }
            if (hs <= 0) continue;
            g.fillRect(bx + 6, ys, 1, hs, PAL_TERM_M22);
            g.fillRect(bx + 7, ys, 1, hs, PAL_TERM_M18);
            g.fillRect(bx + 8, ys, 1, hs, PAL_TERM_M10);
            g.fillRect(x1 - 7, ys, 1, hs, PAL_TERM_M18);
            g.fillRect(x1 - 6, ys, 1, hs, PAL_TERM_M22);
        }
        g.fillRect(bx + 8, y1 - 8, w - 28, 1, PAL_TERM_M10);
        g.fillRect(bx + 8, y1 - 7, w - 28, 1, PAL_TERM_M18);
        g.fillRect(bx + 8, y1 - 6, w - 16, 1, PAL_TERM_M22);
    }
    /* selected highlight covers the whole (possibly two-line) row */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0 || cmd != sel) continue;
        const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
        const int rh = cyd_adv_sheet_row_h(hdr, cmd, w);
        g.fillRect(bx + 3, y - 1, w - 6, rh - 1, PAL_TERM_M18);
        g.fillRect(bx + 3, y - 1, 2, rh - 1, PAL_TERM_M);
    }
    /* text: icon + name + purpose */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + cyd_adv_sheet_row_y(top, r, w);
        const int rh = cyd_adv_sheet_row_h(hdr, cmd, w);
        const bool on = cmd == sel;
        const uint8_t bg = on ? (uint8_t)PAL_TERM_M18 : (uint8_t)PAL_BG;
        blit_ico(g, CMD_ICO[cmd], bx + 10, y + 1, on ? (uint8_t)PAL_TERM_M : accent());
        const uint8_t name_c = on ? (uint8_t)127 : (uint8_t)PAL_TERM_INK;
        if (on) {
            txt(g, CMDN[cmd], bx + 25, y + 2, PAL_TERM_C, lgfx::textdatum::top_left);
            txt(g, CMDN[cmd], bx + 23, y, PAL_CYAN, lgfx::textdatum::top_left);
        }
        txt_bg(g, CMDN[cmd], bx + 24, y + 1, name_c, bg, lgfx::textdatum::top_left);
        const uint8_t pc = on ? (uint8_t)127 : theme().pur;
        {
            /* Cut purpose only at '|' or word boundary — never mid-word.
             * Portrait two-line: purpose under the name, dim/indented.
             * One-line (L or short P): right-aligned beside the name. */
            const char *pur = CMDP[cmd];
            char pbuf[48];
            const int pur_right = bx + w - 8;
            const int pur_left = (rh > 12 && port) ? (bx + 24) : 0;
            int name_w = 0;
            for (const char *p = CMDN[cmd]; *p; ++p) name_w += 6;
            const int name_end = bx + 24 + name_w + 6;
            int max_pw = (rh > 12 && port) ? (pur_right - pur_left) : (pur_right - name_end);
            if (max_pw < 0) max_pw = 0;
            int best = 0;
            for (int i = 0; pur[i] && i < 47; ++i) {
                pbuf[i] = pur[i];
                pbuf[i + 1] = 0;
                if (purpose_px(pbuf) > max_pw) break;
                if (pur[i + 1] == 0 || pur[i] == '|' || pur[i] == ' ') best = i + 1;
            }
            while (best > 0 && (pur[best - 1] == '|' || pur[best - 1] == ' ')) --best;
            if (best <= 0) continue;
            for (int i = 0; i < best; ++i) pbuf[i] = pur[i];
            pbuf[best] = 0;
            if (rh > 12 && port) {
                int px = pur_left;
                char one[2] = { 0, 0 };
                for (const char *p = pbuf; *p; ++p) {
                    if (*p == '\x7f') {
                        g.fillRect(px + 1, y + 11 + 3, 1, 1, pc);
                        px += 4;
                        continue;
                    }
                    one[0] = *p;
                    txt_bg(g, one, px, y + 11, pc, bg, lgfx::textdatum::top_left);
                    px += 6;
                }
            } else {
                draw_purpose(g, pbuf, pur_right, y + 1, pc, bg);
            }
        }
    }
    const int ax = x1 - 13;
    if (top > 0) blit_ico(g, ARROW_UP, ax, by + 6, COL_MUTED);
    if (top + nshow < rows) blit_ico(g, ARROW_DN, ax, y1 - 14, COL_MUTED);
    s_px = keep;
}

/* ---- OSK opens upward from cmdline, Adv rim/palette, full-cell hits ---- */
static int s_adv_osk_x, s_adv_osk_y, s_adv_osk_w, s_adv_osk_h;
static int s_adv_osk_kp, s_adv_osk_rp;

static void cyd_adv_osk_layout(void)
{
    /* Open upward from the cmdline; keep cmdline visible. Stay between pin columns when
     * possible; portrait may use near-full width so 10 cols can meet the 24 px floor. */
    const int W = EW();
    const bool P = botcorps_cyd_orient != 0;
    const int pin_l = 33, pin_r = W - 33;
    int avail = pin_r - pin_l;
    if (avail < 80) avail = W - 4;
    int kp = avail / 10;
    if (P) {
        /* portrait: require >=24 — if pins are too tight, widen to full width (rain covered). */
        if (kp < 24) {
            avail = W - 4;
            kp = avail / 10;
            if (kp < 24) kp = 24;
            s_adv_osk_x = (W - kp * 10) / 2;
        } else {
            s_adv_osk_x = pin_l + (avail - kp * 10) / 2;
        }
    } else {
        /* landscape: between pins; take largest kp that fits (target 28 when avail allows). */
        if (kp > 28) kp = 28;
        if (kp < 1) kp = 1;
        s_adv_osk_x = pin_l + (avail - kp * 10) / 2;
    }
    s_adv_osk_kp = kp;
    s_adv_osk_w = kp * 10;
    int rp = 28;
    s_adv_osk_rp = rp;
    s_adv_osk_h = rp * 5 + 4;
    /* Bottom-anchored: the outer frame (osk_y-2, height osk_h+4) ends at H-2. The
     * full-width input row sits directly above it; cands above the row, top bar clear. */
    const int H = EH();
    s_adv_osk_y = (H - 2) - 1 - s_adv_osk_h;
    if (s_adv_osk_y - 2 - CYD_ADV_ROW_H < 26 + 20) {
        /* short screen: shrink row pitch (floor 24) so row + one cand row stay below the bar */
        const int max_h = (H - 3) - (26 + 20 + CYD_ADV_ROW_H + 2);
        s_adv_osk_rp = (max_h - 4) / 5;
        if (s_adv_osk_rp < 24) s_adv_osk_rp = 24;
        s_adv_osk_h = s_adv_osk_rp * 5 + 4;
        s_adv_osk_y = (H - 2) - 1 - s_adv_osk_h;
    }
    s_adv_row_y = s_adv_osk_y - 2 - CYD_ADV_ROW_H;
}

static void cyd_adv_osk_key_rect(int i, int *kx, int *ky, int *kw, int *kh,
                                 int *cx, int *cy, int *cw, int *ch)
{
    const CydOskSpec &sp = CYD_OSK_SPEC[i];
    const int kp = s_adv_osk_kp, rp = s_adv_osk_rp;
    *cx = s_adv_osk_x + sp.col * kp;
    *cy = s_adv_osk_y + 2 + sp.row * rp;
    *cw = kp * sp.span;
    *ch = rp;
    *kx = *cx;
    *ky = *cy;
    *kw = *cw - 1;
    *kh = *ch - 1;
}

static int cyd_adv_osk_at(int tx, int ty)
{
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < CYD_OSK_N; ++i) {
        int kx, ky, kw, kh, cx, cy, cw, ch;
        cyd_adv_osk_key_rect(i, &kx, &ky, &kw, &kh, &cx, &cy, &cw, &ch);
        const int dx = tx < cx ? cx - tx : (tx >= cx + cw ? tx - (cx + cw - 1) : 0);
        const int dy = ty < cy ? cy - ty : (ty >= cy + ch ? ty - (cy + ch - 1) : 0);
        const int d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = i; }
        if (d == 0) break;
    }
    return best;
}

static void cyd_adv_paint_osk(Gfx g)
{
    if (s_pt_mode != 1) return;
    cyd_adv_osk_layout();
    const int x = s_adv_osk_x - 2, y = s_adv_osk_y - 2;
    const int w = s_adv_osk_w + 4, h = s_adv_osk_h + 4;
    /* the bottom-anchored OSK sits over the plate: blank the plate (and its select
     * outline) between the pin strips so nothing peeks out beside the frame */
    g.fillRect(32, y, EW() - 64, EH() - 2 - y, PAL_BLACK);
    g.fillRect(x, y, w, h, PAL_BLACK);
    g.fillRect(x, y, w, 1, term_rim(0));
    g.fillRect(x, y + h - 1, w, 1, term_rim(1));
    g.fillRect(x, y, 1, h, term_rim(0));
    g.fillRect(x + w - 1, y, 1, h, term_rim(1));
    g.drawRect(x + 1, y + 1, w - 2, h - 2, PAL_TERM_M);
    for (int i = 0; i < CYD_OSK_N; ++i) {
        const CydOskSpec &sp = CYD_OSK_SPEC[i];
        int kx, ky, kw, kh, cx, cy, cw, ch;
        cyd_adv_osk_key_rect(i, &kx, &ky, &kw, &kh, &cx, &cy, &cw, &ch);
        g.fillRect(kx, ky, kw, kh, PAL_DEEP);
        g.drawRect(kx, ky, kw, kh, PAL_TERM_M);   /* Adv term magenta, not dim M22 (reads orange) */
        char lab[8] = {0};
        if (sp.kind == CYD_OSK_CHAR) lab[0] = s_pt_sym ? sp.symbol : sp.alpha;
        else if (sp.kind == CYD_OSK_SPACE) snprintf(lab, sizeof lab, "SPC");
        else if (sp.kind == CYD_OSK_BKSP) snprintf(lab, sizeof lab, "<");
        else if (sp.kind == CYD_OSK_ENTER) snprintf(lab, sizeof lab, "OK");
        else if (sp.kind == CYD_OSK_TAB) snprintf(lab, sizeof lab, "TAB");
        else if (sp.kind == CYD_OSK_SYM) snprintf(lab, sizeof lab, s_pt_sym ? "abc" : "SYM");
        else if (sp.kind == CYD_OSK_HIDE) snprintf(lab, sizeof lab, "BACK");
        txt(g, lab, kx + kw / 2, ky + kh / 2 - 4, PAL_WHITE, lgfx::textdatum::top_center);
    }
}

/* Idle plate (mode 0) + overlays drawn after the plate shell fills black. */
static void cyd_adv_paint_plate(Gfx g, int sx, int sy, int sw, int sh)
{
    s_pt_bx = sx; s_pt_by = sy; s_pt_bw = sw; s_pt_bh = sh;
    cyd_adv_geo(sx, sy, sw, sh);
    cyd_adv_paint_prompt(g);
    if (s_focus_cmds) cyd_adv_paint_palette(g);
    /* attack / gloss overlays: reuse Adv painters shifted into plate when they fit;
     * otherwise a compact attack list inside the plate. */
    if (s_term_ovl == 2 && !s_sheet) {
        /* Adv paint_gc_attack layout, fitted to the plate */
        const uint8_t ac = PAL_CYAN;
        const int bx = sx + 2, by = sy + 2;
        const int bw = cyd_adv_col_x() - 2 - bx, bh = s_adv_py - sy - 6;   /* ≡ / reticle stay clear */
        if (bh >= 40) {
            GATE_BOX(g, "OCCLUDE", "gcattack", bx, by, bw, bh, "-");
            g.fillRect(bx, by, bw, bh, PAL_BLACK);
            g.drawRect(bx, by, bw, bh, ac);
            g.fillRect(bx + 2, by + 2, bw - 4, 12, PAL_DEEP);
            const int keep = s_px; s_px = 6;
            txt(g, "ATTACK PROMPTS", bx + 8, by + 4, ac, lgfx::textdatum::top_left);
            txt(g, "GIBSON", bx + bw - 8, by + 4, PAL_MUTED, lgfx::textdatum::top_right);
            static const char *const lab[] = { "names", "core", "sig", "subs", "ok" };
            static const char *const val[] = {
                "wifi bluetooth deauth hack flock",
                "scan lock zap purge restart",
                "overload", "core root", "start defend"
            };
            const int max_rows = (bh - 20) / 12;
            const int show = 5 < max_rows ? 5 : max_rows;
            for (int k = 0; k < show; ++k) {
                const int y = by + 18 + k * 12;
                txt(g, lab[k], bx + 8, y, ac, lgfx::textdatum::top_left);
                char vbuf[40];
                int n = 0;
                while (val[k][n] && n < 39) { vbuf[n] = val[k][n]; ++n; }
                vbuf[n] = 0;
                const int max_ch = (bw - 56 - 8) / 5;
                if (max_ch > 0 && max_ch < n) {
                    if (max_ch <= 1) vbuf[0] = 0;
                    else { vbuf[max_ch - 1] = '.'; vbuf[max_ch] = 0; }
                }
                if (vbuf[0])
                    txt(g, vbuf, bx + 56, y, PAL_IVORY, lgfx::textdatum::top_left);
            }
            s_px = keep;
        }
    }
}

/* OSK overlay is painted on top of the whole board (upward from cmdline). */
static void cyd_adv_paint_osk_row(Gfx g)
{
    /* The plate cmdline, re-drawn full screen width directly above the OSK. Same Adv
     * prefix / typed / caret / accent focus box and term rims as the plate. */
    const int W = EW(), ry = s_adv_row_y, rh = CYD_ADV_ROW_H;
    const int keep = s_px;
    s_px = 6;
    g.fillRect(0, ry, W, rh, PAL_BLACK);
    g.fillRect(0, ry, W, 1, term_rim(0));
    g.fillRect(0, ry + rh - 1, W, 1, term_rim(1));
    g.fillRect(0, ry + 1, 1, rh - 2, term_rim(0));
    g.fillRect(W - 1, ry + 1, 1, rh - 2, term_rim(1));
    cyd_adv_cmdline(g, 3, ry + 5, 1, W - 4, true);
    cyd_adv_paint_cands(g);
    s_px = keep;
}

static void cyd_adv_paint_osk_overlay(Gfx g)
{
    if (s_pt_mode == 1 && s_pt_bw > 0) {
        cyd_adv_paint_osk(g);
        cyd_adv_paint_osk_row(g);
    }
}

/* ---- hits ---- */
static int cyd_adv_hit_idle(int tx, int ty)
{
    if (s_pt_bw <= 0) return -1;
    /* palette chips */
    if (s_focus_cmds && !s_sheet) {
        for (int n = 0; n < 12; ++n) {
            int x, y, ww, ch;
            if (!cyd_adv_pal_chip(n, &x, &y, &ww, &ch)) continue;
            if (tx >= x && tx < x + ww && ty >= y && ty < y + ch) {
                s_cmd_sel = n;
                if (strcmp(CMDS[n], "cmds") == 0) {
                    /* ≡ -> cmds opens the glossary sheet upward from the prompt */
                    s_focus_cmds = false; s_sheet = true; s_sheet_i = 0;
                    s_pt_mode = 0; s_term_ovl = 0;
                    s_dirty = true; return 0;
                }
                char line[32];
                snprintf(line, sizeof line, "%s", CMDS[n]);
                s_focus_cmds = false;
                run_cmd(line);
                s_dirty = true;
                return 0;
            }
        }
        /* tap outside palette closes it */
        if (ty < s_adv_sy || ty >= s_adv_sy + s_adv_sh || tx < s_adv_sx || tx >= s_adv_sx + s_adv_sw) {
            s_focus_cmds = false; s_dirty = true; return 0;
        }
    }
    /* sheet rows (variable pitch: portrait purpose wrap) */
    if (s_sheet) {
        int by, h, top, nshow;
        cyd_adv_sheet_box(&by, &h, &top, &nshow);
        const int bx = s_adv_sx + 2;
        const int bw = cyd_adv_sheet_w();
        if (tx >= bx && tx < bx + bw && ty >= by && ty < by + h) {
            int y = by + 5;
            for (int k = 0; k < nshow; ++k) {
                int hdr = -1, cmd = -1;
                menu_row(top + k, &hdr, &cmd);
                const int rh = cyd_adv_sheet_row_h(hdr, cmd, bw);
                if (ty >= y && ty < y + rh) {
                    if (cmd >= 0) {
                        snprintf(botcorps_t0_term_prompt, 60, "%s%s", CMDN[cmd],
                                 CMD_ARG[cmd] ? " " : "");
                        s_sheet = false;
                        s_cyd_adv_focus = true;
                        s_dirty = true;
                        return 0;
                    }
                    return 0;
                }
                y += rh;
            }
            return 0;
        }
    }
    /* ≡ / reticle pads */
    int hx, hy, hw, hh;
    cyd_adv_btn_hit(s_adv_cmd_x, s_adv_btn_y, &hx, &hy, &hw, &hh);
    if (tx >= hx && tx < hx + hw && ty >= hy && ty < hy + hh) {
        s_focus_cmds = !s_focus_cmds;
        if (s_focus_cmds) { s_sheet = false; s_term_ovl = 0; }
        s_dirty = true;
        return 0;
    }
    cyd_adv_btn_hit(s_adv_atk_x, s_adv_atk_y, &hx, &hy, &hw, &hh);
    if (tx >= hx && tx < hx + hw && ty >= hy && ty < hy + hh) {
        s_term_ovl = (s_term_ovl == 2) ? 0 : 2;
        if (s_term_ovl) { s_sheet = false; s_focus_cmds = false; }
        s_dirty = true;
        return 0;
    }
    /* cmdline row (full plate width; pads above win first) → open OSK */
    if (ty >= s_adv_py - 4 && ty < s_adv_sy + s_adv_sh &&
        tx >= s_adv_sx && tx < s_adv_sx + s_adv_sw) {
        s_pt_mode = 1;
        s_cyd_adv_focus = true;
        s_term_prompt = true;
        s_sheet = false;
        s_focus_cmds = false;
        s_term_ovl = 0;
        s_dirty = true;
        return 0;
    }
    /* completion list pick */
    if (botcorps_t0_term_prompt[0] && !s_sheet && !s_focus_cmds) {
        /* approximate: any tap in upper plate while cands show applies first cand via tab */
        if (ty >= s_adv_sy && ty < s_adv_py - 4 && tx >= s_adv_sx && tx < s_adv_sx + s_adv_sw) {
            /* cycle / apply tab completion */
            return 7; /* K_TAB */
        }
    }
    return -1;
}

static int cyd_adv_hit_osk(int tx, int ty, char *typed)
{
    if (s_pt_mode != 1) return -1;
    cyd_adv_osk_layout();
    if (ty < s_adv_osk_y - 2 || ty >= s_adv_osk_y + s_adv_osk_h + 2 ||
        tx < s_adv_osk_x - 2 || tx >= s_adv_osk_x + s_adv_osk_w + 2) {
        /* tap on the full-width input row keeps the OSK */
        if (ty >= s_adv_row_y && ty < s_adv_row_y + CYD_ADV_ROW_H && tx >= 0 && tx < EW()) return 0;
        /* tap a completion above the row: put it on the line */
        CydAdvCands c;
        if (cyd_adv_cands_calc(&c) && tx >= c.bx && tx < c.bx + c.w && ty >= c.by && ty < c.by + c.h) {
            const int r = (ty - (c.by + 3 + c.head)) / c.rh;
            if (r >= 0 && r < c.vis) {
                const char *pick = c.rows[c.start + r];
                if (c.sp) snprintf(botcorps_t0_term_prompt, 60, "%s %s", c.cmd, pick);
                else snprintf(botcorps_t0_term_prompt, 60, "%s ", pick);
                s_tab_i = -1;
                s_dirty = true;
            }
            return 0;
        }
        return -1;
    }
    const int i = cyd_adv_osk_at(tx, ty);
    if (i < 0) return 0;
    const CydOskSpec &sp = CYD_OSK_SPEC[i];
    if (sp.kind == CYD_OSK_HIDE) { s_pt_mode = 0; s_dirty = true; return 0; }
    if (sp.kind == CYD_OSK_SYM) { s_pt_sym = !s_pt_sym; s_dirty = true; return 0; }
    if (sp.kind == CYD_OSK_BKSP) return 8; /* K_BS */
    if (sp.kind == CYD_OSK_ENTER) return 5; /* K_OK */
    if (sp.kind == CYD_OSK_TAB) return 7; /* K_TAB */
    if (sp.kind == CYD_OSK_SPACE) { if (typed) *typed = ' '; return 9; /* K_CHAR */ }
    if (sp.kind == CYD_OSK_CHAR) {
        const char ch = s_pt_sym ? sp.symbol : sp.alpha;
        if (ch && typed) { *typed = ch; return 9; /* K_CHAR */ }
    }
    return 0;
}

/* Adv on_ok / peel for CYD plates (s_screen may be hub/scan/gchat). */
static bool cyd_adv_on_ok(void)
{
    if (s_sheet) {
        if (botcorps_t0_term_prompt[0]) {
            run_cmd(botcorps_t0_term_prompt);
            botcorps_t0_term_prompt[0] = 0;
            s_dirty = true;
            return true;
        }
        if (s_sheet_i < 0) s_sheet_i = 0;
        if (s_sheet_i >= CMD_N) s_sheet_i = CMD_N - 1;
        if (strcmp(CMDN[s_sheet_i], "cmds") == 0) { s_sheet_i = 0; s_dirty = true; return true; }
        snprintf(botcorps_t0_term_prompt, 60, "%s%s", CMDN[s_sheet_i],
                 CMD_ARG[s_sheet_i] ? " " : "");
        s_sheet = false;
        s_cyd_adv_focus = true;
        s_dirty = true;
        return true;
    }
    if (botcorps_t0_term_prompt[0]) {
        run_cmd(botcorps_t0_term_prompt);
        botcorps_t0_term_prompt[0] = 0;
        s_pt_mode = 0;
        s_dirty = true;
        return true;
    }
    if (s_term_ovl == 2) { s_term_ovl = 0; s_dirty = true; return true; }
    if (s_focus_cmds) {
        if (strcmp(CMDS[s_cmd_sel], "cmds") == 0) {
            s_focus_cmds = false; s_sheet = true; s_sheet_i = 0; s_pt_mode = 0;
            s_dirty = true; return true;
        }
        run_cmd(CMDS[s_cmd_sel]);
        s_focus_cmds = false;
        s_dirty = true;
        return true;
    }
    return false;
}

static bool cyd_adv_on_esc(void)
{
    /* peel: overlay → sheet → text/OSK → go back */
    if (s_term_ovl) { s_term_ovl = 0; s_dirty = true; return true; }
    if (s_focus_cmds) { s_focus_cmds = false; s_dirty = true; return true; }
    if (s_sheet) { s_sheet = false; s_dirty = true; return true; }
    if (s_pt_mode == 1) { s_pt_mode = 0; s_dirty = true; return true; }
    if (botcorps_t0_term_prompt[0]) {
        botcorps_t0_term_prompt[0] = 0;
        s_cyd_adv_focus = false;
        s_dirty = true;
        return true;
    }
    if (s_cyd_adv_focus) { s_cyd_adv_focus = false; s_term_prompt = false; s_dirty = true; return true; }
    return false;
}

static bool cyd_adv_on_char(char c)
{
    if (!(s_cyd_adv_focus || s_pt_mode == 1 || s_term_prompt)) return false;
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n + 1 >= 60) return true;
    if (s_term_ovl) { s_term_ovl = 0; }
    botcorps_t0_term_prompt[n] = c;
    botcorps_t0_term_prompt[n + 1] = 0;
    s_tab_i = -1;
    s_dirty = true;
    return true;
}

static bool cyd_adv_on_bs(void)
{
    if (!(s_cyd_adv_focus || s_pt_mode == 1 || s_term_prompt)) return false;
    const size_t n = strlen(botcorps_t0_term_prompt);
    if (n == 0) return true;
    botcorps_t0_term_prompt[n - 1] = 0;
    s_tab_i = -1;
    s_dirty = true;
    return true;
}

static bool cyd_adv_on_tab(void)
{
    /* Tab opens glossary sheet (Adv physical Tab → sheet); with text, also cycles cands first time */
    if (s_sheet) return false;
    if (!botcorps_t0_term_prompt[0]) {
        s_sheet = true;
        s_sheet_i = 0;
        s_pt_mode = 0;
        s_focus_cmds = false;
        s_term_ovl = 0;
        s_dirty = true;
        return true;
    }
    /* cycle prefix completions like Adv on_tab */
    const bool cont = s_tab_i >= 0 && strcmp(botcorps_t0_term_prompt, tab_last()) == 0;
    if (!cont) {
        snprintf(tab_base(), 60, "%s", botcorps_t0_term_prompt);
        s_tab_i = -1;
    }
    int match[PCMD_N];
    int n = 0;
    const size_t bn = strlen(tab_base());
    for (int i = 0; i < PCMD_N; ++i) {
        if (strncmp(PCMD[i], tab_base(), bn) == 0) match[n++] = i;
    }
    if (n == 0) {
        s_sheet = true; s_sheet_i = 0; s_pt_mode = 0; s_dirty = true;
        return true;
    }
    s_tab_i = (s_tab_i + 1) % n;
    snprintf(botcorps_t0_term_prompt, 60, "%s ", PCMD[match[s_tab_i]]);
    snprintf(tab_last(), 60, "%s", botcorps_t0_term_prompt);
    s_dirty = true;
    return true;
}
