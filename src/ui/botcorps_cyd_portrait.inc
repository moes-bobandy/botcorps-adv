/* BOTCORPS_CYD portrait painters + shared terminal (pterm). Included from botcorps_ui.cpp
 * under #ifdef BOTCORPS_CYD only. Landscape painters stay in the .cpp; Adv is unchanged. */

/* One-line A/B swap: 0 = layout A (default), 1 = mirrored layout B. */
static int CP_P_LAYOUT = 0;

static int cp_px(int x, int w)
{
    return CP_P_LAYOUT ? (240 - x - w) : x;
}

/* Portrait Control part rects (layout A). Index map matches landscape CP_N. */
static bool cp_part_rect_p(int index, int *x, int *y, int *w, int *h)
{
    int X = 0, Y = 0, W = 0, H = 0;
    switch (index) {
    case 0: X = 40;  Y = 24;  W = 40; H = 62; break; /* GRAPE / UI COLOR */
    case 1: X = 100; Y = 24;  W = 40; H = 62; break; /* MASHUP */
    case 2: X = 160; Y = 24;  W = 40; H = 62; break; /* MIX */
    case 5: X = 40;  Y = 94;  W = 40; H = 62; break; /* RAIN bg style */
    case 7: X = 100; Y = 94;  W = 40; H = 62; break; /* MULTI */
    case 4: X = 106; Y = 214; W = 22; H = 22; break; /* qfn RAIN colour */
    case 3: X = 138; Y = 220; W = 20; H = 10; break; /* JUMP */
    case 8: X = 172; Y = 220; W = 20; H = 10; break; /* ORIENT */
    case 6: X = 181; Y = 176; W = 10; H = 8;  break; /* PWR LED */
    default: return false;
    }
    *x = cp_px(X, W); *y = Y; *w = W; *h = H;
    return true;
}

static int cp_lab_y_p(int index)
{
    if (index == 0 || index == 1 || index == 2 || index == 5 || index == 7)
        return -1; /* chip label inside package (cp_chip) */
    if (index == 6) return 188;
    if (index == 4) return 239;
    if (index == 3 || index == 8) return 239;
    return 239;
}

/* Portrait power feed from 1117C output (103,166). Max 6 points like landscape. */
static int cp_route_p(int index, int *px, int *py)
{
    int x, y, w, h;
    if (!cp_part_rect_p(index, &x, &y, &w, &h)) return 0;
    const bool chip = index == 0 || index == 1 || index == 2 || index == 5 || index == 7;
    const int cx = x + w / 2;
    const int cy = chip ? y + h : y; /* top of bay parts / bottom of chips */
    const int fx = cp_px(103, 1); /* 1117C out */
    int n = 0;
    if (index == 5 || index == 7) {
        px[n] = fx; py[n++] = 166;
        px[n] = fx; py[n++] = 160;
        px[n] = cx; py[n++] = 160;
        px[n] = cx; py[n++] = 156;
        return n;
    }
    if (index == 0 || index == 1) {
        const int riser = cp_px(90, 1);
        px[n] = fx; py[n++] = 166;
        px[n] = fx; py[n++] = 160;
        px[n] = riser; py[n++] = 160;
        px[n] = riser; py[n++] = 90;
        px[n] = cx; py[n++] = 90;
        px[n] = cx; py[n++] = 86;
        return n;
    }
    if (index == 2) {
        const int riser = cp_px(150, 1);
        px[n] = fx; py[n++] = 166;
        px[n] = fx; py[n++] = 160;
        px[n] = riser; py[n++] = 160;
        px[n] = riser; py[n++] = 90;
        px[n] = cx; py[n++] = 90;
        px[n] = cx; py[n++] = 86;
        return n;
    }
    if (index == 6) {
        const int pxpwr = cp_px(186, 1);
        px[n] = fx; py[n++] = 166;
        px[n] = fx; py[n++] = 160;
        px[n] = pxpwr; py[n++] = 160;
        px[n] = pxpwr; py[n++] = 176;
        return n;
    }
    /* Row D: qfn / JUMP / ORIENT via riser x 198 */
    {
        const int riser = cp_px(198, 1);
        px[n] = fx; py[n++] = 166;
        px[n] = fx; py[n++] = 160;
        px[n] = riser; py[n++] = 160;
        px[n] = riser; py[n++] = 210;
        px[n] = cx; py[n++] = 210;
        px[n] = cx; py[n++] = cy;
        return n;
    }
}

/* Shared portrait terminal box: (2,250,236,68) on every portrait screen. */
static const int PT_X = 2, PT_Y = 250, PT_W = 236, PT_H = 68;
static uint8_t s_pt_mode = 0; /* 0 log, 1 keys, 2 list */
static bool s_pt_sym = false;
static int s_pt_top = 0;
static int s_pt_sel = 0;
static int s_pt_bx = PT_X, s_pt_by = PT_Y, s_pt_bw = PT_W, s_pt_bh = PT_H;

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

static void pt_draw_cmdline(Gfx g, int x, int y, int w)
{
    int cap = 0;
    char *buf = pt_buf(&cap);
    char line[72];
    const char *pfx = "user@botcorps:~$ ";
    snprintf(line, sizeof(line), "%s%s", pfx, buf ? buf : "");
    /* Horizontal scroll: keep the tail visible. */
    const int maxc = (w - 4) / 6;
    const char *show = line;
    int len = (int)strlen(line);
    if (len > maxc) show = line + (len - maxc);
    g.fillRect(x, y, w, 11, PAL_BLACK);
    g.drawRect(x, y, w, 11, PAL_LINE);
    txt(g, show, x + 2, y + 2, PAL_WHITE, lgfx::textdatum::top_left);
    /* caret */
    if (((millis() / 400u) & 1u) == 0) {
        const int cx = x + 2 + (int)strlen(show) * 6;
        if (cx < x + w - 2) g.fillRect(cx, y + 2, 4, 8, accent());
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
        else if (sp.kind == CYD_OSK_HIDE) snprintf(lab, sizeof(lab), "HID");
        txt(g, lab, kx + kw / 2, ky + (kh > 8 ? 2 : 1), PAL_WHITE, lgfx::textdatum::top_center);
    }
}

static void pt_draw_list(Gfx g, int x, int y, int w, int h)
{
    int n = 0;
    const char *const *cmds = pt_list(&n);
    const int rh = 9;
    const int rows = h / rh;
    if (s_pt_sel < s_pt_top) s_pt_top = s_pt_sel;
    if (s_pt_sel >= s_pt_top + rows) s_pt_top = s_pt_sel - rows + 1;
    if (s_pt_top < 0) s_pt_top = 0;
    for (int r = 0; r < rows; ++r) {
        const int i = s_pt_top + r;
        if (i >= n) break;
        const bool on = i == s_pt_sel;
        const int ry = y + r * rh;
        g.fillRect(x, ry, w, rh - 1, on ? accent() : (uint8_t)PAL_DEEP);
        txt(g, cmds[i], x + 2, ry + 1, on ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE,
            lgfx::textdatum::top_left);
    }
}

static void paint_pterm(Gfx g, int x, int y, int w, int h)
{
    s_pt_bx = x; s_pt_by = y; s_pt_bw = w; s_pt_bh = h;
    g.fillRect(x, y, w, h, PAL_BLACK);
    sb_frame(g, x, y, w, h, PAL_LINE);
    sb_frame(g, x + 1, y + 1, w - 2, h - 2, PAL_DEEP);
    /* command line */
    pt_draw_cmdline(g, x + 2, y + 2, w - 4);
    /* mode buttons on the right of the cmdline row when in log mode */
    const int by = y + 2;
    if (s_pt_mode == 0) {
        /* [abc] and list (≡) */
        g.drawRect(x + w - 44, by, 20, 11, accent());
        txt(g, "abc", x + w - 34, by + 2, accent(), lgfx::textdatum::top_center);
        g.drawRect(x + w - 22, by, 18, 11, accent());
        txt(g, "=", x + w - 13, by + 2, accent(), lgfx::textdatum::top_center);
        /* log lines in the rest of the box */
        const int ly = y + 14;
        const int lh = h - 16;
        const int rows = lh / 9;
        for (int i = 0; i < rows && i < s_log_n; ++i) {
            const char *ln = log_line(s_log_n - rows + i);
            if (!ln) continue;
            if (s_log_n - rows + i < 0) continue;
            txt(g, ln, x + 3, ly + i * 9, PAL_LINE, lgfx::textdatum::top_left);
        }
    } else {
        /* BACK/HIDE */
        g.drawRect(x + w - 28, by, 24, 11, accent());
        txt(g, "BACK", x + w - 16, by + 2, accent(), lgfx::textdatum::top_center);
        const int by2 = y + 14;
        const int bh2 = h - 16;
        if (s_pt_mode == 1) pt_draw_keys(g, x + 2, by2, w - 4, bh2);
        else pt_draw_list(g, x + 2, by2, w - 4, bh2);
    }
}

/* Opaque pad + rain cover helper for portrait Control parts. */
static void cp_pad(Gfx g, int x, int y, int w, int h)
{
    g.fillRect(x, y, w, h, PAL_BLACK);
}

static void paint_control_p(Gfx g)
{
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = sh_rgb(0xe9, 0xe9, 0xee), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t rail = sh_rgb(0x02, 0x02, 0x03), trace = sh_rgb(0x1c, 0x1c, 0x24), metal = sh_rgb(0x8f, 0x95, 0xa3);
    const uint8_t ac = accent();
    /* Top bar 0..16 (same art language as landscape, 240 wide). */
    g.fillRect(0, 0, 240, 16, PAL_BLACK);
    if (theme().grad == 0) g.fillRect(0, 16, 240, 1, ac);
    else for (int x = 0; x < 240; x += 4) g.fillRect(x, 16, 4, 1, th_ink_at(x, 240));
    for (int k = 0; k < CP_N; ++k) g.fillRect(6 + k * 6, 7, 4, 3, k == s_cursor ? ac : sh_rgb(0x2a, 0x2a, 0x34));
    {
        const bool bsel = s_cursor == CP_N + 1;
        gib_crown(g, 120, 3);
        if (bsel) g.drawRect(102, 0, 36, 16, PAL_WHITE);
        const bool tsel = s_cursor == CP_N;
        const uint8_t tc = tsel ? (uint8_t)PAL_WHITE : ac;
        const int tx = 206;
        GATE_BOX(g, "BTN", "term", tx - 2, 1, 22, 14, "-");
        g.drawRect(tx, 3, 18, 11, tc); g.fillRect(tx, 3, 18, 2, tc);
        g.drawLine(tx + 3, 7, tx + 6, 9, tc); g.drawLine(tx + 6, 9, tx + 3, 11, tc);
        g.fillRect(tx + 8, 11, 5, 1, tc);
        if (tsel) g.drawRect(tx - 2, 1, 22, 14, PAL_WHITE);
    }
    /* Outer double frame around the board area; rain shows outside the panel. */
    sb_frame(g, 2, 19, 236, 230, PAL_LINE);
    sb_frame(g, 3, 20, 234, 228, PAL_DEEP);
    /* Margin vertical traces */
    for (int side = 0; side < 2; ++side) {
        const int mx = side ? 233 : 6;
        g.fillRect(mx, 24, 1, 220, trace);
        for (int k = 0; k < 10; ++k) g.fillRect(mx - 1, 30 + k * 22, 3, 3, silk2);
    }
    /* Board panel (10,20,220,228) — no solid PCB fill; pads per part. */
    GATE_BOX(g, "BOARD", "settings", 10, 20, 220, 228, "-");
    /* Header rails left/right */
    static const char *const TP[10] = { "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16" };
    static const char *const BP[10] = { "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27" };
    const int lx0 = cp_px(13, 6), rx0 = cp_px(222, 6);
    g.fillRect(lx0, 24, 6, 220, rail);
    g.fillRect(rx0, 24, 6, 220, rail);
    for (int k = 0; k < 10; ++k) {
        const int py = 30 + k * 22;
        for (int q = 0; q < 2; ++q) {
            g.fillRect(lx0 + 1, py + q * 3, 2, 2, gold);
            g.fillRect(rx0 + 1, py + q * 3, 2, 2, gold);
        }
        if (!CP_P_LAYOUT) {
            txt(g, TP[k], 21, py - 3, silk, lgfx::textdatum::top_left);
            txt(g, BP[k], 220, py - 3, silk, lgfx::textdatum::top_right);
        } else {
            txt(g, BP[k], 21, py - 3, silk, lgfx::textdatum::top_left);
            txt(g, TP[k], 220, py - 3, silk, lgfx::textdatum::top_right);
        }
    }
    /* Boot section */
    {
        const int bx = cp_px(164, 36);
        g.drawRect(bx, 93, 36, 60, sh_rgb(0x2b, 0x2b, 0x33));
        for (int b = 0; b < 2; ++b) {
            const int x = cp_px(172, 16), y = b ? 126 : 96, w = 16, h = 14;
            cp_pad(g, x - 1, y - 1, w + 2, h + 14);
            g.fillRect(x, y, w, h, metal);
            g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xc3, 0xc9, 0xd6));
            g.fillCircle(x + w / 2, y + h / 2, 4, sh_rgb(0x22, 0x23, 0x2c));
            txt(g, b ? "EN" : "BOOT", x + w / 2, b ? 142 : 112, silk, lgfx::textdatum::top_center);
        }
    }
    /* Power section frame */
    {
        const int fx = cp_px(36, 162);
        g.drawRect(fx, 162, 162, 47, sh_rgb(0x2b, 0x2b, 0x33));
        /* DC jack */
        {
            const int x = cp_px(40, 40), y = 164;
            cp_pad(g, x, y, 40, 40);
            g.fillRect(x, y, 40, 36, sh_rgb(0x15, 0x15, 0x18));
            g.fillRect(x + 5, y + 5, 30, 26, sh_rgb(0x05, 0x05, 0x06));
            g.fillCircle(x + 20, y + 18, 6, sh_rgb(0x2a, 0x2a, 0x30));
            txt(g, "DC6.5", x + 20, 202, silk, lgfx::textdatum::top_center);
        }
        /* 1117C */
        {
            const int x = cp_px(88, 30), y = 166;
            cp_pad(g, x, y, 30, 30);
            g.fillRect(x + 4, y, 22, 6, sh_rgb(0xa9, 0xaf, 0xbb));
            g.fillRect(x, y + 6, 30, 16, sh_rgb(0x1a, 0x1a, 0x1f));
            for (int k = 0; k < 3; ++k) g.fillRect(x + 5 + k * 9, y + 22, 3, 6, sh_rgb(0xa9, 0xaf, 0xbb));
            txt(g, "1117C", x + 15, y + 8, silk2, lgfx::textdatum::top_center);
        }
        /* USB5V */
        {
            const int x = cp_px(126, 17), y = 166, w = 17, hh = 30;
            cp_pad(g, x, y, w, hh + 8);
            g.fillRect(x, y, w, hh, sh_rgb(0x9a, 0xa1, 0xb2));
            g.fillRect(x + 1, y + 1, w - 2, hh - 2, sh_rgb(0xd0, 0xd5, 0xdf));
            g.fillRect(x + 4, y + 4, w - 8, hh - 8, sh_rgb(0x0a, 0x0a, 0x10));
            g.fillRect(x + 7, y + 8, 2, hh - 16, gold);
            txt(g, "USB5V", x + w / 2, 200, silk, lgfx::textdatum::top_center);
        }
        /* C1 */
        {
            const int x = cp_px(151, 22), y = 174;
            cp_pad(g, x, y, 22, 20);
            g.fillRect(x, y, 22, 12, sh_rgb(0xe0, 0xa2, 0x1c));
            g.fillRect(x, y, 5, 12, sh_rgb(0x7a, 0x52, 0x08));
            g.fillRect(x + 7, y + 3, 12, 6, sh_rgb(0x5a, 0x3a, 0x00));
            txt(g, "C1", x + 11, 190, silk, lgfx::textdatum::top_center);
        }
        /* static power traces */
        {
            const int dcx = cp_px(80, 1), usbx = cp_px(126, 1), c1x = cp_px(162, 1);
            const int regx = cp_px(88, 1);
            g.fillRect(dcx < regx ? dcx : regx, 180, abs(regx - dcx) + 1, 1, trace);
            g.fillRect(usbx < (regx + 30) ? usbx : (regx + 18), 180, abs(usbx - (regx + 18)) + 1, 1, trace);
            g.fillRect(c1x, 160, 1, 15, trace);
        }
    }
    /* R1-6 */
    {
        cp_pad(g, cp_px(40, 66), 220, 66, 20);
        for (int k = 0; k < 6; ++k) {
            const int x = cp_px(40 + k * 11, 6);
            g.fillRect(x, 224, 6, 3, sh_rgb(0xb8, 0xb8, 0xc0));
            g.fillRect(x + 2, 224, 2, 3, sh_rgb(0x20, 0x20, 0x24));
        }
        txt(g, "R1-6", cp_px(40, 66) + 33, 239, silk2, lgfx::textdatum::top_center);
    }
    /* Dark feed traces then selected accent feed */
    int px[6], py[6];
    for (int i = 0; i < CP_N; ++i) {
        const int n = cp_route_p(i, px, py);
        for (int q = 0; q + 1 < n; ++q) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 1, trace, false);
    }
    if (s_cursor >= 0 && s_cursor < CP_N) {
        const int n = cp_route_p(s_cursor, px, py);
        int len = 0;
        for (int q = 0; q + 1 < n; ++q) len += abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
        int run = 0;
        for (int q = 0; q + 1 < n; ++q) {
            const int l2 = abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
            if (theme().grad == 0) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 2, th_wire(), true);
            else {
                for (int d = 0; d < l2; d += 2) {
                    const int qx = px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0);
                    const int qy = py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0);
                    g.fillRect(qx, qy, 2, 2, th_wire_at(run + d, len));
                }
            }
            run += l2;
        }
        for (int k = 0; k < 3 && len > 0; ++k) {
            int d = (int)(((millis() * 120u) / 1000u + (uint32_t)(k * len / 3)) % (uint32_t)len);
            for (int q = 0; q + 1 < n; ++q) {
                const int l2 = abs(px[q] - px[q + 1]) + abs(py[q] - py[q + 1]);
                if (d <= l2) {
                    const int qx = px[q] + (l2 ? (px[q + 1] - px[q]) * d / l2 : 0);
                    const int qy = py[q] + (l2 ? (py[q + 1] - py[q]) * d / l2 : 0);
                    g.fillRect(qx - 1, qy - 1, 4, 4, ac);
                    break;
                }
                d -= l2;
            }
        }
    }
    /* Parts: chips use cp_chip (reads cp_part_rect which branches to portrait). */
    for (int i = 0; i < CP_N; ++i) {
        if (i == 0 || i == 1 || i == 2 || i == 5 || i == 7) {
            /* chip packages: pad under, then paint. Label y inside chip uses h-12. */
            int x, y, w, h;
            if (cp_part_rect_p(i, &x, &y, &w, &h)) cp_pad(g, x - 1, y - 1, w + 2, h + 2);
            cp_chip(g, i, s_cursor == i);
        } else {
            int x, y, w, h;
            if (!cp_part_rect_p(i, &x, &y, &w, &h)) continue;
            cp_pad(g, x - 2, y - 2, w + 4, h + 16);
            /* Reuse landscape jumper/LED/qfn art via cp_part, but override label y. */
            const uint8_t gold2 = gold, rail2 = rail, metal2 = metal, silk3 = silk, silk22 = silk2;
            const bool on = s_cursor == i;
            if (on) {
                sel_rect(g, x - 3, y - 3, w + 6, h + 6);
                GATE_BOX(g, "SEL", "outline", x - 3, y - 3, w + 6, h + 6, "-");
            }
            const char *lab = "PWR";
            if (i == 3) {
                const bool solid = botcorps_rain_mode() > 0;
                g.fillRect(x, y + 2, w, 6, rail2);
                for (int k = 0; k < 3; ++k) g.fillRect(x + 3 + k * 6, y + 4, 2, 2, gold2);
                const int jx = solid ? x + 9 : x + 1;
                g.fillRect(jx, y, 11, 10, sh_rgb(0xff, 0xd4, 0x00));
                g.fillRect(jx + 2, y + 2, 7, 3, sh_rgb(0xb8, 0x94, 0x00));
                lab = "JUMP";
            } else if (i == 8) {
                const bool port = botcorps_cyd_orient != 0;
                g.fillRect(x, y + 2, w, 6, rail2);
                for (int k = 0; k < 3; ++k) g.fillRect(x + 3 + k * 6, y + 4, 2, 2, gold2);
                const int jx = port ? x + 9 : x + 1;
                g.fillRect(jx, y, 11, 10, sh_rgb(0x00, 0xc8, 0xff));
                g.fillRect(jx + 2, y + 2, 7, 3, sh_rgb(0x00, 0x7a, 0x9a));
                lab = "ORIENT";
            } else if (i == 4) {
                g.fillRect(x, y, w, h, sh_rgb(0x16, 0x16, 0x1b));
                for (int k = 0; k < 5; ++k) {
                    g.fillRect(x + 3 + k * 4, y - 2, 2, 2, metal2);
                    g.fillRect(x + 3 + k * 4, y + h, 2, 2, metal2);
                    g.fillRect(x - 2, y + 3 + k * 4, 2, 2, metal2);
                    g.fillRect(x + w, y + 3 + k * 4, 2, 2, metal2);
                }
                tiny_text(g, "CP", x + w / 2 - 3, y + 3, silk22);
                const int m = botcorps_rain_mode();
                g.fillRect(x + 4, y + 13, w - 8, 5, m <= 0 ? botcorps_rain_palette(0) : botcorps_rain_solid());
                g.fillRect(x + 2, y + 2, 2, 2, sh_rgb(0x5d, 0x5d, 0x6a));
                lab = "RAIN";
            } else if (i == 6) {
                const bool pon = s_bg_motion != 0;
                g.fillRect(x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
                g.fillRect(x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
                lab = "PWR";
            }
            txt(g, lab, x + w / 2, cp_lab_y_p(i), on ? (uint8_t)PAL_WHITE : silk3, lgfx::textdatum::top_center);
        }
    }
    /* Bottom terminal */
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

/* Portrait covers for Control: pads, not one solid board. */
static void layout_covers_control_p(void)
{
    cover_add(0, 0, 240, 17);
    cover_add(PT_X, PT_Y, PT_W, PT_H);
    cover_add(cp_px(13, 6), 24, 6, 220);
    cover_add(cp_px(222, 6), 24, 6, 220);
    for (int i = 0; i < CP_N; ++i) {
        int x, y, w, h;
        if (cp_part_rect_p(i, &x, &y, &w, &h)) cover_add(x - 1, y - 1, w + 2, h + 14);
    }
    cover_add(cp_px(164, 36), 93, 36, 60);
    cover_add(cp_px(40, 40), 164, 40, 40);
    cover_add(cp_px(88, 30), 166, 30, 30);
    cover_add(cp_px(126, 17), 166, 17, 38);
    cover_add(cp_px(151, 22), 174, 22, 20);
    cover_add(cp_px(40, 66), 220, 66, 20);
}

/* ---- Minimal portrait painters for remaining screens (hub 2-plate, Terminal, boards) ---- */

static void paint_topbar_p(Gfx g, const char *title)
{
    g.fillRect(0, 0, 240, 16, PAL_BLACK);
    if (theme().grad == 0) g.fillRect(0, 16, 240, 1, accent());
    else for (int x = 0; x < 240; x += 4) g.fillRect(x, 16, 4, 1, th_ink_at(x, 240));
    gib_crown(g, 120, 3);
    if (title) txt(g, title, 4, 4, accent(), lgfx::textdatum::top_left);
}

static void paint_gibson_p(Gfx g)
{
    paint_topbar_p(g, "HUB");
    /* Two plates only: Terminal + Control (LL-20). */
    const int pw = 100, ph = 44;
    const int x0 = 20, y0 = 80;
    for (int i = 0; i < 2; ++i) {
        const int x = x0, y = y0 + i * (ph + 16);
        const bool on = s_cursor == i;
        g.fillRect(x, y, pw, ph, PAL_DEEP);
        sb_frame(g, x, y, pw, ph, on ? accent() : PAL_LINE);
        txt(g, i == 0 ? "TERMINAL" : "CONTROL", x + pw / 2, y + 18,
            on ? accent() : (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
    }
    /* Gibson chip opens SCAN (LL-19) */
    {
        const int cs = 48;
        const int cx = 170, cy = 130;
        g.fillRect(cx - cs / 2, cy - cs / 2, cs, cs, COL_IVORY);
        sb_frame(g, cx - cs / 2, cy - cs / 2, cs, cs, s_hub_zone == 3 ? accent() : PAL_LINE);
        txt(g, "GIB", cx, cy - 4, PAL_BLACK, lgfx::textdatum::top_center);
    }
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_terminal_p(Gfx g)
{
    paint_topbar_p(g, "TERM");
    /* 12 bot chips in a 3x4 grid above the terminal box */
    const int gw = 72, gh = 48;
    for (int i = 0; i < 12; ++i) {
        const int col = i % 3, row = i / 3;
        const int x = 8 + col * (gw + 4);
        const int y = 22 + row * (gh + 4);
        const bool on = s_cursor == i && !s_term_prompt;
        g.fillRect(x, y, gw, gh, PAL_DEEP);
        sb_frame(g, x, y, gw, gh, on ? accent() : PAL_LINE);
        chip_mini(g, x + gw / 2, y + 20, 18, i, false, 0);
        txt(g, BOTS[i].short_name, x + gw / 2, y + gh - 12,
            on ? accent() : (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
    }
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_radar_p(Gfx g, bool wifi)
{
    paint_topbar_p(g, wifi ? "WIFI" : "BLE");
    /* chip cluster left, info unchanged spirit */
    g.fillRect(8, 22, 72, 72, PAL_DEEP);
    sb_frame(g, 8, 22, 72, 72, PAL_LINE);
    chip_mini(g, 44, 50, 18, wifi ? 0 : 1, false, 0);
    g.fillRect(88, 22, 144, 200, PAL_DEEP);
    sb_frame(g, 88, 22, 144, 200, PAL_LINE);
    txt(g, wifi ? "WI-FI SCAN" : "BLE SCAN", 160, 30, accent(), lgfx::textdatum::top_center);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_scan_p(Gfx g)
{
    const bool gib = s_scan_bot >= 12;
    paint_topbar_p(g, gib ? "GIBSON" : "SCAN");
    g.fillRect(8, 22, 72, 72, gib ? (uint8_t)COL_IVORY : (uint8_t)PAL_DEEP);
    sb_frame(g, 8, 22, 72, 72, PAL_LINE);
    if (!gib) chip_mini(g, 44, 50, 18, s_scan_bot % 12, false, 0);
    else txt(g, "GIB", 44, 50, PAL_BLACK, lgfx::textdatum::top_center);
    g.fillRect(88, 22, 144, 200, PAL_DEEP);
    sb_frame(g, 88, 22, 144, 200, PAL_LINE);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_list_p(Gfx g, bool wifi)
{
    paint_topbar_p(g, wifi ? "WIFI LIST" : "BLE LIST");
    g.fillRect(8, 22, 224, 220, PAL_DEEP);
    sb_frame(g, 8, 22, 224, 220, PAL_LINE);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_gchat_p(Gfx g)
{
    paint_topbar_p(g, "GAME");
    g.fillRect(8, 22, 224, 220, PAL_DEEP);
    sb_frame(g, 8, 22, 224, 220, PAL_LINE);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_ext_portrait(Gfx g)
{
#ifdef BOTCORPS_HOST_RENDER
    host_gate_reset();
    layout_covers();
#endif
    g.clearClipRect();
    g.setTextWrap(false, false);
    /* Light (hub-dim) rain on Control; full rain elsewhere. */
    const bool hub = s_screen == SCR_GIBSON;
    const bool ctl = s_screen == SCR_CONTROL;
    botcorps_rain_set_hub(hub || ctl);
    rain_flags();
    const bool rain_bg = (wants_rain() || rain_static()) && !zap_sb_on();
    if (hub || ctl) g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
    else g.fillScreen(COL_FIELD);
#ifdef BOTCORPS_HOST_RENDER
    if (rain_bg) GATE_BOX(g, "RAIN", "layer", 0, 0, 240, 320, "-");
    if (rain_bg && g.raw() && !host_gate_norain) {
        host_gate_mute = 1;
#else
    if (rain_bg && g.raw()) {
#endif
        const int y0 = botcorps_band_oy();
        botcorps_rain_draw(*g.raw(), y0, y0 + g.raw()->height());
#ifdef BOTCORPS_HOST_RENDER
        host_gate_mute = 0;
#endif
    }
    switch (s_screen) {
    case SCR_RAIN: paint_rain_btn(g); break;
    case SCR_GIBSON: paint_gibson_p(g); break;
    case SCR_GCHAT: paint_gchat_p(g); break;
    case SCR_TERMINAL: paint_terminal_p(g); break;
    case SCR_WIFI_RADAR:
    case SCR_BT_RADAR: paint_radar_p(g, s_screen == SCR_WIFI_RADAR); break;
    case SCR_SCAN: paint_scan_p(g); break;
    case SCR_WIFI_LIST: paint_list_p(g, true); break;
    case SCR_BT_LIST: paint_list_p(g, false); break;
    case SCR_CONTROL: paint_control_p(g); break;
    case SCR_DIAG: paint_diag(g); break;
    default: break;
    }
    if (fa_live()) paint_found_alert(g);
}

#ifdef BOTCORPS_HOST_RENDER
void botcorps_host_control_p(uint8_t *buf, int cursor, int layout)
{
    extern int host_fw, host_fh;
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    CP_P_LAYOUT = layout ? 1 : 0;
    botcorps_rain_set_view(240, 320);
    botcorps_band_force_direct(true);
    botcorps_host_enter(buf, SCR_CONTROL);
    s_cursor = cursor;
    memset(buf, PAL_BLACK, 240u * 320u);
    host_frame = buf;
    paint_ext_portrait(Gfx());
}
void botcorps_host_portrait(uint8_t *buf, int screen, int pt_mode, int pt_sym)
{
    extern int host_fw, host_fh;
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    botcorps_rain_set_view(240, 320);
    botcorps_band_force_direct(true);
    botcorps_host_enter(buf, screen);
    s_pt_mode = (uint8_t)pt_mode;
    s_pt_sym = pt_sym != 0;
    if (pt_mode == 1 || pt_mode == 2) {
        int cap = 0;
        char *b = pt_buf(&cap);
        if (b && !b[0] && pt_mode == 1)
            snprintf(b, (size_t)cap, "scan wifi and a-very-long-line");
        if (b && pt_mode == 2) {
            int n = 0; const char *const *L = pt_list(&n);
            if (n > 0) { s_pt_sel = 0; snprintf(b, (size_t)cap, "%s", L[0]); }
        }
    }
    memset(buf, PAL_BLACK, 240u * 320u);
    host_frame = buf;
    paint_ext_portrait(Gfx());
}
#endif
