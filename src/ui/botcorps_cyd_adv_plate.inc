/* CYD Adv terminal plate — port of f1c08aa paint_prompt / paint_cands / sheet /
 * palette / attack chrome into the edge-board plate rect. BOTCORPS_CYD only.
 * Replaces the b98559e pt_paint_old0 / old12 / abc+list keyboard look. */

/* Plate-local geometry (set each paint). Art stays Adv-sized (6px Font0, 20x12 btns). */
static int s_adv_sx, s_adv_sy, s_adv_sw, s_adv_sh;
static int s_adv_py;          /* prompt y (screen) */
static int s_adv_cmd_x, s_adv_atk_x, s_adv_btn_y;
static bool s_cyd_adv_focus = false;   /* cmdline / OSK owns typing into term prompt */

static void cyd_adv_geo(int sx, int sy, int sw, int sh)
{
    s_adv_sx = sx; s_adv_sy = sy; s_adv_sw = sw; s_adv_sh = sh;
    s_adv_py = sy + sh - 11;                          /* Adv: PROMPT_Y = plate_bot - 8 */
    if (s_adv_py < sy + 10) s_adv_py = sy + 10;
    s_adv_btn_y = s_adv_py - 4;                       /* Adv: TERM_BTN_Y = 223, PROMPT 227 */
    s_adv_cmd_x = sx + sw - 24;                       /* ≡ at far right, 20-wide art */
    s_adv_atk_x = s_adv_cmd_x - 24;                   /* reticle just left */
    if (s_adv_atk_x < sx + 4) s_adv_atk_x = sx + 4;
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

/* ---- paint_cands port, plate-relative ---- */
static void cyd_adv_paint_cands(Gfx g)
{
    const char *buf = botcorps_t0_term_prompt;
    if (!buf[0] || s_sheet || s_focus_cmds || s_pt_mode == 1) return;
    const char *sp = strchr(buf, ' ');
    char cmd[16];
    const char *arg = "";
    cmd[0] = 0;
    if (sp) {
        int n = (int)(sp - buf);
        if (n > 15) n = 15;
        memcpy(cmd, buf, (size_t)n);
        cmd[n] = 0;
        arg = sp + 1;
    }
    const char *rows[8];
    int idx[8];
    int n = 0;
    if (!sp) {
        const size_t bn = strlen(buf);
        for (int i = 0; i < PCMD_N && n < 8; ++i) {
            if (strncmp(PCMD[i], buf, bn) == 0) { rows[n] = PCMD[i]; idx[n] = i; ++n; }
        }
    } else {
        n = opt_list(cmd, arg, rows, 8);
        for (int i = 0; i < n; ++i) idx[i] = -1;
    }
    if (n <= 0) return;
    const int vis = n > 6 ? 6 : n;
    int sel = 0;
    if (!sp && s_tab_i >= 0 && s_tab_i < n) sel = s_tab_i;
    int start = sel - 2;
    if (start < 0) start = 0;
    if (start > n - vis) start = n - vis;
    int w = 40;
    for (int i = 0; i < vis; ++i) {
        int ww = (int)strlen(rows[start + i]) * 6 + 14;
        if (ww > w) w = ww;
    }
    if (sp) {
        const int hw = (int)strlen(cmd) * 6 + 22;
        if (hw > w) w = hw;
    }
    w += 6;
    if (w > s_adv_sw - 12) w = s_adv_sw - 12;
    if (w < 40) w = 40;
    const int rh = 12;
    const int head = sp ? 10 : 0;
    const int h = vis * rh + 6 + head;
    const int py = s_adv_py;
    int bx = s_adv_sx + 6 + (18 + (int)strlen(buf)) * 6 + 14;
    if (bx + w > s_adv_sx + s_adv_sw - 2) bx = s_adv_sx + s_adv_sw - 2 - w;
    if (bx < s_adv_sx + 2) bx = s_adv_sx + 2;
    int by = py - 4 - h;
    if (by < s_adv_sy + 2) by = s_adv_sy + 2;
    plate(g, bx, by, w, h, COL_BG, accent());
    int y = by + 4;
    if (sp) {
        txt_bg(g, cmd, bx + 6, y - 1, accent(), COL_BG, lgfx::textdatum::top_left);
        txt_bg(g, ">", bx + 8 + (int)strlen(cmd) * 6, y - 1, COL_MUTED, COL_BG, lgfx::textdatum::top_left);
        y += 10;
    }
    for (int k = 0; k < vis; ++k) {
        const int on = (start + k) == sel;
        const uint8_t bg = on ? (uint8_t)COL_SEL : (uint8_t)COL_BG;
        if (on) {
            g.fillRect(bx + 2, y - 1, w - 4, rh - 1, COL_SEL);
            g.fillRect(bx + 2, y - 1, 2, rh - 1, accent());
        }
        txt_bg(g, rows[start + k], bx + 6, y, on ? COL_WHITE : COL_INK, bg, lgfx::textdatum::top_left);
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
        const int slw = s_adv_atk_x - sx - 8;
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
    const int maxc = (s_adv_atk_x - lx - 4) / 6;
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

    /* cmdline: user@botcorps (idx 194 + glow) :~$ accent, typed white, scroll on overflow */
    const int y = s_adv_py;
    const int text_right = s_adv_atk_x - 4;
    const int pfx_w = 16 * 6;   /* "user@botcorps" + ":~$" = 13+3 chars */
    {
        txt(g, "user@botcorps", sx + 4, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", sx + 6, y, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", sx + 5, y + 1, (uint8_t)5, lgfx::textdatum::top_left);
        txt(g, "user@botcorps", sx + 5, y, (uint8_t)194, lgfx::textdatum::top_left);
    }
    txt(g, ":~$", sx + 5 + 13 * 6 + 1, y, accent(), lgfx::textdatum::top_left);
    txt(g, ":~$", sx + 5 + 13 * 6, y, accent(), lgfx::textdatum::top_left);

    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor(COL_WHITE);
    const char *typed = botcorps_t0_term_prompt;
    const int typed_max = (text_right - (sx + 5 + pfx_w)) / 6;
    const int tlen = (int)strlen(typed);
    const char *showt = typed;
    if (typed_max > 0 && tlen > typed_max) showt = typed + (tlen - typed_max);
    g.setCursor(sx + 5 + pfx_w, y);
    g.print(showt);
    const int cx = sx + 5 + pfx_w + (int)strlen(showt) * 6 + 2;

    const bool pf = (s_cyd_adv_focus || s_term_prompt || s_pt_mode == 1) && !s_sheet && !s_focus_cmds && !s_cmd_btn;
    if (pf) {
        const int fx0 = sx + 2, fx1 = s_adv_atk_x - 4, fy = y - 3;
        g.fillRect(fx0, fy, fx1 - fx0, 1, accent());
        g.fillRect(fx0, fy, 1, 11, accent());
        g.fillRect(fx1, fy, 1, 11, accent());
    }
    const bool caret = pf && typed[0] ? true : (millis() / 530u) % 2u == 0;
    if (caret && cx + 6 <= text_right) g.fillRect(cx, y, 6, 8, accent());

    cyd_adv_paint_cands(g);

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
        const int ax = s_adv_atk_x, ay = s_adv_btn_y;
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

/* ---- palette over plate (paint_palette port) ---- */
static void cyd_adv_paint_palette(Gfx g)
{
    if (!s_focus_cmds || s_sheet) return;
    const int sx = s_adv_sx + 1, sy = s_adv_sy + 2;
    const int aw = s_adv_sw - 2;
    const int cw = aw / 6;
    const int ch = 15;
    const int bh = 2 * ch + 8;
    g.fillRect(sx, sy, aw, bh > s_adv_sh - 14 ? s_adv_sh - 14 : bh, COL_DEEP);
    for (int n = 0; n < 12; ++n) {
        const int x = sx + 3 + (n % 6) * cw;
        const int y = sy + 2 + (n / 6) * (ch + 3);
        const bool sel = n == s_cmd_sel;
        const int ww = cw - 3;
        if (ww < 20) continue;
        plate(g, x, y, ww, ch, sel ? COL_SEL : COL_BG, sel ? accent() : COL_LINE);
        txt_bg(g, CMDS[n], x + ww / 2, y + 4, sel ? COL_WHITE : COL_MUTED, sel ? COL_SEL : COL_BG,
               lgfx::textdatum::top_center);
    }
}

/* ---- glossary sheet opens upward from prompt (paint_sheet geometry) ---- */
static void cyd_adv_sheet_box(int *by, int *h, int *top_out, int *nshow_out)
{
    const int vis = 9;
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;
    const int sel_row = menu_sel_row(sel);
    int top = sel_row - 3;
    if (top < 0) top = 0;
    if (top > rows - vis) top = rows - vis;
    if (top < 0) top = 0;
    int nshow = rows - top;
    if (nshow > vis) nshow = vis;
    const int hh = nshow * 12 + 10;
    int y = s_adv_py - 5 - hh;
    if (y < 2) y = 2;
    /* keep sheet above the plate bottom; may extend above plate into board */
    *by = y;
    *h = hh;
    *top_out = top;
    *nshow_out = nshow;
}

static void cyd_adv_paint_sheet(Gfx g)
{
    /* Exact port of Adv paint_sheet onto the CYD plate (bx/bw from plate).
     * icon + name + right-aligned purpose, group headers, CORE glow, single
     * magenta rim. Portrait truncates purposes that would collide with names. */
    if (!s_sheet) return;
    const int keep = s_px;
    s_px = 6;
    const int rh = 12;
    int by = 0, h = 0, top = 0, nshow = 0;
    cyd_adv_sheet_box(&by, &h, &top, &nshow);
    const int bx = s_adv_sx + 2;
    int w = s_adv_sw - 4;
    if (w > 292) w = 292;
    if (w < 80) w = 80;
    const int rows = menu_rows();
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= CMD_N) sel = CMD_N - 1;

    GATE_BOX(g, "OCCLUDE", "sheet", bx, by, w + 1, h + 1, "-");
    g.fillRect(bx + 1, by + 1, w - 1, h - 1, PAL_BG);
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + r * rh;
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
        const int y = by + 5 + r * rh;
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
        const int y = by + 5 + r * rh;
        if (hdr == 0) g.fillRect(bx + 48, y + 5, w - 60, 1, PAL_TERM_M22);
        else glow_hline(g, bx + 48, y + 5, w - 60, STEP_RULE);
    }
    /* single magenta bottom ramp (Adv) — no second orange stroke */
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
            const int y = by + 5 + r * rh;
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
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0 || cmd != sel) continue;
        const int y = by + 5 + r * rh;
        g.fillRect(bx + 3, y - 1, w - 6, rh - 1, PAL_TERM_M18);
        g.fillRect(bx + 3, y - 1, 2, rh - 1, PAL_TERM_M);
    }
    /* text: icon + name + purpose (truncate on narrow plate) */
    for (int r = 0; r < nshow; ++r) {
        int hdr = -1, cmd = 0;
        menu_row(top + r, &hdr, &cmd);
        if (hdr >= 0) continue;
        const int y = by + 5 + r * rh;
        const bool on = cmd == sel;
        const uint8_t bg = on ? (uint8_t)PAL_TERM_M18 : (uint8_t)PAL_BG;
        blit_ico(g, CMD_ICO[cmd], bx + 10, y + 1, on ? (uint8_t)PAL_TERM_M : accent());
        const uint8_t name_c = on ? (uint8_t)127 : (uint8_t)PAL_TERM_INK;
        if (on) {
            txt(g, CMDN[cmd], bx + 25, y + 2, PAL_TERM_C, lgfx::textdatum::top_left);
            txt(g, CMDN[cmd], bx + 23, y, PAL_CYAN, lgfx::textdatum::top_left);
        }
        txt_bg(g, CMDN[cmd], bx + 24, y + 1, name_c, bg, lgfx::textdatum::top_left);
        /* purpose: Adv draw_purpose; on narrow P truncate so it clears the name */
        const char *pur = CMDP[cmd];
        char pbuf[48];
        int plen = 0;
        while (pur[plen] && plen < 47) { pbuf[plen] = pur[plen]; ++plen; }
        pbuf[plen] = 0;
        int name_w = 0;
        for (const char *p = CMDN[cmd]; *p; ++p) name_w += 6;
        const int name_end = bx + 24 + name_w + 6;
        const int pur_right = bx + w - 8;
        int max_pw = pur_right - name_end;
        if (max_pw < 0) max_pw = 0;
        /* purpose_px ≈ 6 per glyph (4 for \x7f); trim until it fits */
        while (plen > 0 && purpose_px(pbuf) > max_pw) {
            --plen; pbuf[plen] = 0;
        }
        if (plen > 0) {
            const uint8_t pc = on ? (uint8_t)127 : theme().pur;
            draw_purpose(g, pbuf, pur_right, y + 1, pc, bg);
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
    /* Outer frame is painted at (osk_y-2) with height osk_h+4, so its bottom
     * pixel is osk_y+osk_h+1. Buttons sit at s_adv_btn_y (= py-4); prompt at py.
     * Keep the outer rim fully above the button row so cmdline + ≡/◎ stay visible. */
    /* Outer rim bottom must clear button tops (btn_y) AND the prompt focus
     * hairline at py-3. Use 5 px air above btn_y so glyphs/icons stay whole. */
    int outer_bot = s_adv_btn_y - 5;
    if (outer_bot < s_adv_osk_h + 28) outer_bot = s_adv_osk_h + 28;
    s_adv_osk_y = outer_bot - 1 - s_adv_osk_h;
    if (s_adv_osk_y < 26) {
        s_adv_osk_y = 26;
        /* Prefer shrinking row pitch over covering the cmdline; never below 24. */
        int max_h = outer_bot - 1 - s_adv_osk_y;
        s_adv_osk_rp = (max_h - 4) / 5;
        if (s_adv_osk_rp < 24) s_adv_osk_rp = 24;
        s_adv_osk_h = s_adv_osk_rp * 5 + 4;
        s_adv_osk_y = outer_bot - 1 - s_adv_osk_h;
        if (s_adv_osk_y < 26) s_adv_osk_y = 26;
    }
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
        const int bw = sw - 4, bh = s_adv_py - sy - 6;
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
static void cyd_adv_paint_osk_overlay(Gfx g)
{
    if (s_pt_mode == 1 && s_pt_bw > 0) cyd_adv_paint_osk(g);
}

/* ---- hits ---- */
static int cyd_adv_hit_idle(int tx, int ty)
{
    if (s_pt_bw <= 0) return -1;
    /* palette chips */
    if (s_focus_cmds && !s_sheet) {
        const int sx = s_adv_sx + 1, sy = s_adv_sy + 2;
        const int aw = s_adv_sw - 2, cw = aw / 6, ch = 15;
        for (int n = 0; n < 12; ++n) {
            const int x = sx + 3 + (n % 6) * cw;
            const int y = sy + 2 + (n / 6) * (ch + 3);
            const int ww = cw - 3;
            if (tx >= x && tx < x + ww && ty >= y && ty < y + ch) {
                s_cmd_sel = n;
                if (strcmp(CMDS[n], "cmds") == 0) { s_dirty = true; return 0; }
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
    /* sheet rows */
    if (s_sheet) {
        int by, h, top, nshow;
        cyd_adv_sheet_box(&by, &h, &top, &nshow);
        const int bx = s_adv_sx + 2;
        int bw = s_adv_sw - 4;
        if (tx >= bx && tx < bx + bw && ty >= by && ty < by + h) {
            const int k = (ty - (by + 5)) / 12;
            if (k >= 0 && k < nshow) {
                int hdr = -1, cmd = -1;
                menu_row(top + k, &hdr, &cmd);
                if (cmd >= 0) {
                    snprintf(botcorps_t0_term_prompt, 60, "%s%s", CMDN[cmd],
                             CMD_ARG[cmd] ? " " : "");
                    s_sheet = false;
                    s_cyd_adv_focus = true;
                    s_dirty = true;
                    return 0;
                }
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
    cyd_adv_btn_hit(s_adv_atk_x, s_adv_btn_y, &hx, &hy, &hw, &hh);
    if (tx >= hx && tx < hx + hw && ty >= hy && ty < hy + hh) {
        s_term_ovl = (s_term_ovl == 2) ? 0 : 2;
        if (s_term_ovl) { s_sheet = false; s_focus_cmds = false; }
        s_dirty = true;
        return 0;
    }
    /* cmdline row → open OSK */
    if (ty >= s_adv_py - 4 && ty < s_adv_sy + s_adv_sh &&
        tx >= s_adv_sx && tx < s_adv_atk_x) {
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
        /* tap on visible cmdline keeps OSK; tap elsewhere closes */
        if (ty >= s_adv_py - 4 && ty < s_adv_sy + s_adv_sh &&
            tx >= s_adv_sx && tx < s_adv_atk_x) return 0;
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
        if (strcmp(CMDS[s_cmd_sel], "cmds") == 0) { s_dirty = true; return true; }
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
