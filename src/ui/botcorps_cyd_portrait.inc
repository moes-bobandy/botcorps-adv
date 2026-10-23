/* BOTCORPS_CYD portrait painters + shared terminal (pterm). Included from botcorps_ui.cpp
 * under #ifdef BOTCORPS_CYD only. Landscape painters stay in the .cpp; Adv is unchanged. */

/* One-line A/B swap: 0 = layout A (default), 1 = mirrored layout B. */
static int CP_P_LAYOUT = 0;
static int CP_PIN_STYLE = 0; /* 0=A tiny beside pin (default), 1=B between pins */
static int CP_SPREAD = 0; /* 0=default+pterm, 1=2-col grid, 2=centered core (host preview) */

static int cp_px(int x, int w)
{
    return CP_P_LAYOUT ? (240 - x - w) : x;
}


static void cp_pad(Gfx g, int x, int y, int w, int h); /* defined below paint_pterm */

/* ---- Portrait Control SPREAD (host-only; CP_SPREAD=0 keeps default) ----
 * Pin art: botcorps_id.cpp:gpio_pad 990-1002 + teeth 1038-1042 + copper 1086-1089.
 * mix_nat/split565/pack565 unused on palette path; indexes 177/217/110 match gold/goldh/steel. */

/* Exact gpio_pad geometry on palette Gfx (file:function botcorps_id.cpp:gpio_pad). */
static void cp_gpio_pad(Gfx g, int x, int y, int kind, bool on, bool tip)
{
    const uint8_t gold = 177, goldh = 217, steel = (uint8_t)PAL_STEEL;
    const uint8_t red = botcorps_pal_fit(0xE1A6); /* id 3V3 pad 0xE1A6 */
    uint8_t pad = gold;
    if (kind == 0) pad = red;
    else if (kind == 1) pad = steel;
    else if (on) pad = tip ? (uint8_t)PAL_WHITE : accent();
    g.fillRect(x, y, 7, 7, pad);
    g.fillRect(x, y, 7, 1, (on && kind > 1) ? (uint8_t)PAL_WHITE : goldh);
    g.fillRect(x + 2, y + 2, 3, 3, PAL_BLACK);
    if (on && kind > 1) g.fillRect(x + 3, y + 3, 1, 1, tip ? accent() : (uint8_t)PAL_WHITE);
}

/* Module castellated tooth — botcorps_id.cpp:1038-1042 (also chip_edge_pin for die pins). */
static void cp_mod_tooth(Gfx g, int x, int y, bool left)
{
    const uint8_t gold = 177, goldd = 104;
    g.fillRect(x, y, 4, 3, gold);
    if (left) g.fillRect(x, y, 1, 3, goldd);
    else      g.fillRect(x + 3, y, 1, 3, goldd);
}

static bool cp_part_rect_spread(int index, int *x, int *y, int *w, int *h)
{
    int X = 0, Y = 0, W = 0, H = 0;
    if (CP_SPREAD == 1) {
        /* Option 1: 2-col chip grid; power bay; R1-6 left-bottom; switches right-bottom. */
        switch (index) {
        case 0: X = 48;  Y = 28;  W = 40; H = 56; break;
        case 1: X = 140; Y = 28;  W = 40; H = 56; break;
        case 2: X = 48;  Y = 100; W = 40; H = 56; break;
        case 5: X = 140; Y = 100; W = 40; H = 56; break;
        case 7: X = 48;  Y = 172; W = 40; H = 56; break;
        case 6: X = 186; Y = 248; W = 12; H = 10; break; /* PWR right of C1 */
        case 4: X = 120; Y = 286; W = 22; H = 22; break;
        case 3: X = 156; Y = 294; W = 22; H = 12; break;
        case 8: X = 192; Y = 294; W = 22; H = 12; break;
        default: return false;
        }
    } else if (CP_SPREAD == 2) {
        /* Option 2: theme row; BOOT/EN centre; power bay; R1-6 then switches (no overlap). */
        switch (index) {
        case 0: X = 42;  Y = 26;  W = 40; H = 54; break;
        case 1: X = 100; Y = 26;  W = 40; H = 54; break;
        case 2: X = 158; Y = 26;  W = 40; H = 54; break;
        case 5: X = 42;  Y = 98;  W = 40; H = 54; break;
        case 7: X = 158; Y = 98;  W = 40; H = 54; break;
        case 6: X = 196; Y = 182; W = 12; H = 10; break; /* PWR right of C1, clear */
        case 4: X = 120; Y = 286; W = 22; H = 22; break;
        case 3: X = 156; Y = 294; W = 22; H = 12; break;
        case 8: X = 192; Y = 294; W = 22; H = 12; break;
        default: return false;
        }
    } else {
        return false;
    }
    *x = cp_px(X, W); *y = Y; *w = W; *h = H;
    return true;
}

static int cp_lab_y_spread(int index)
{
    int x, y, w, h;
    if (!cp_part_rect_spread(index, &x, &y, &w, &h)) return 0;
    if (index == 0 || index == 1 || index == 2 || index == 5 || index == 7) return -1;
    return y + h + 2;
}

/* Gutter spines — traces stay in aisles; never cross a part/label box. */
static int cp_spread_gutter_x(int index)
{
    /* Left-column chips / left bay → left aisle; else right aisle. */
    if (CP_SPREAD == 1) {
        if (index == 0 || index == 2 || index == 7) return cp_px(96, 1);
        return cp_px(128, 1);
    }
    /* spread 2: aisles clear of BOOT (100..140) and theme packages.
     * Left aisle x=86 (between RAIN/GRAPE and BOOT); right aisle x=148. */
    if (index == 0 || index == 5 || index == 1) return cp_px(86, 1);
    if (index == 2 || index == 7) return cp_px(148, 1);
    return cp_px(148, 1); /* bottom bank: right aisle past bay */
}

static int cp_route_spread(int index, int *px, int *py)
{
    int x, y, w, h;
    if (!cp_part_rect_spread(index, &x, &y, &w, &h)) return 0;
    const bool chip = index == 0 || index == 1 || index == 2 || index == 5 || index == 7;
    const int cx = x + w / 2;
    const int gx = cp_spread_gutter_x(index);
    const int fy = (CP_SPREAD == 2) ? 166 : 236;
    int n = 0;
    if (chip) {
        /* Spine in side aisle (never through BOOT 100..140); attach under chip only. */
        const int bot = y + h + 2;
        px[n] = gx; py[n++] = fy;
        px[n] = gx; py[n++] = bot;
        px[n] = cx; py[n++] = bot;
        px[n] = cx; py[n++] = y + h + 1;
    } else {
        /* Bay / bottom parts: H along feed aisle at fy, then V only at part cx (no bay cross). */
        const int ax = (CP_SPREAD == 2) ? cx : cp_spread_gutter_x(index);
        px[n] = ax; py[n++] = fy;
        px[n] = cx; py[n++] = fy;
        px[n] = cx; py[n++] = y - 1;
    }
    return n;
}

static void cp_draw_headers_spread(Gfx g)
{
    const uint8_t silk = sh_rgb(0xe9, 0xe9, 0xee);
    const uint8_t strip = botcorps_pal_fit(0x18C3); /* id housing top line idx 68 */
    static const char *const TP[12] = {
        "3V", "GND", "D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16"
    };
    static const char *const BP[12] = {
        "EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12"
    };
    const int n = 12, pitch = 22, y0 = 28;
    const int lx = 6, rx = 227;
    /* Black header strip + top highlight (botcorps_id.cpp:1127-1128) */
    g.fillRect(lx, y0 - 2, 11, n * pitch + 2, PAL_BLACK);
    g.fillRect(lx, y0 - 2, 11, 1, strip);
    g.fillRect(rx, y0 - 2, 11, n * pitch + 2, PAL_BLACK);
    g.fillRect(rx, y0 - 2, 11, 1, strip);
    for (int k = 0; k < n; ++k) {
        const int py = y0 + k * pitch;
        const int kind = (k == 0) ? 0 : (k == 1) ? 1 : 2;
        cp_gpio_pad(g, lx + 2, py, kind, false, false);
        cp_gpio_pad(g, rx + 2, py, kind == 0 ? 2 : kind, false, false);
        cp_mod_tooth(g, lx + 11, py + 2, true);
        cp_mod_tooth(g, rx - 4, py + 2, false);
        const char *L = CP_P_LAYOUT ? BP[k] : TP[k];
        const char *R = CP_P_LAYOUT ? TP[k] : BP[k];
        const int ly = py + 1;
        g.fillRect(18, ly - 1, 18, 7, PAL_BLACK);
        g.fillRect(204, ly - 1, 18, 7, PAL_BLACK);
        tiny_text(g, L, 19, ly, silk);
        tiny_text(g, R, 221 - tiny_w(R), ly, silk);
    }
    /* Short copper stubs off each pad toward the board (no full-width bus over parts). */
    const uint8_t trace = sh_rgb(0xb4, 0x6a, 0x28);
    for (int k = 0; k < n; ++k) {
        const int py = y0 + k * pitch + 3;
        g.fillRect(lx + 12, py, 4, 1, trace);
        g.fillRect(rx - 5, py, 4, 1, trace);
    }
}

static void cp_lab_back(Gfx g, const char *lab, int x, int y, uint8_t col)
{
    const int tw = tiny_w(lab);
    g.fillRect(x - 1, y - 1, tw + 2, 7, PAL_BLACK);
    tiny_text(g, lab, x, y, col);
}

static void cp_draw_power_bay(Gfx g, int fx, int fy, int fw, int fh)
{
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = sh_rgb(0xe9, 0xe9, 0xee);
    const uint8_t silk2 = sh_rgb(0x9a, 0x9a, 0xa6), trace = sh_rgb(0xb4, 0x6a, 0x28);
    g.drawRect(fx, fy, fw, fh, sh_rgb(0x2b, 0x2b, 0x33));
    /* DC | 1117C | USB5V | C1 — leave x>=fx+128 clear for PWR LED (part 6). */
    {
        const int x = fx + 3, y = fy + 3;
        cp_pad(g, x, y, 30, 34);
        g.fillRect(x, y, 30, 26, sh_rgb(0x15, 0x15, 0x18));
        g.fillRect(x + 3, y + 3, 24, 20, sh_rgb(0x05, 0x05, 0x06));
        g.fillCircle(x + 15, y + 13, 5, sh_rgb(0x2a, 0x2a, 0x30));
        cp_lab_back(g, "DC6.5", x + 3, y + 28, silk);
    }
    {
        const int x = fx + 36, y = fy + 6;
        cp_pad(g, x, y, 28, 26);
        g.fillRect(x + 3, y, 22, 5, sh_rgb(0xa9, 0xaf, 0xbb));
        g.fillRect(x, y + 5, 28, 12, sh_rgb(0x1a, 0x1a, 0x1f));
        for (int k = 0; k < 3; ++k) g.fillRect(x + 4 + k * 9, y + 19, 3, 5, sh_rgb(0xa9, 0xaf, 0xbb));
        cp_lab_back(g, "1117C", x + 2, y + 8, silk2);
    }
    {
        const int x = fx + 70, y = fy + 6, w = 14, hh = 22;
        cp_pad(g, x, y, w, hh + 8);
        g.fillRect(x, y, w, hh, sh_rgb(0x9a, 0xa1, 0xb2));
        g.fillRect(x + 1, y + 1, w - 2, hh - 2, sh_rgb(0xd0, 0xd5, 0xdf));
        g.fillRect(x + 3, y + 3, w - 6, hh - 6, sh_rgb(0x0a, 0x0a, 0x10));
        g.fillRect(x + 6, y + 6, 2, hh - 12, gold);
        cp_lab_back(g, "USB5V", x - 4, y + hh + 2, silk);
    }
    {
        /* C1 fully left of PWR (PWR at fx+148); label has black back, full "C1". */
        const int x = fx + 92, y = fy + 10;
        cp_pad(g, x, y, 24, 20);
        g.fillRect(x, y, 24, 12, sh_rgb(0xe0, 0xa2, 0x1c));
        g.fillRect(x, y, 5, 12, sh_rgb(0x7a, 0x52, 0x08));
        g.fillRect(x + 8, y + 3, 12, 6, sh_rgb(0x5a, 0x3a, 0x00));
        cp_lab_back(g, "C1", x + 8, y + 14, silk);
    }
    /* Copper in the bottom aisle only — never through part bodies or labels. */
    g.fillRect(fx + 4, fy + fh - 5, 120, 1, trace);
}

static void cp_draw_boot_en(Gfx g, int bx, int by)
{
    const uint8_t silk = sh_rgb(0xe9, 0xe9, 0xee), metal = sh_rgb(0x8f, 0x95, 0xa3);
    /* 40×70: caps stacked; labels centered under each with black back, 4px pad to frame. */
    g.drawRect(bx, by, 40, 70, sh_rgb(0x2b, 0x2b, 0x33));
    for (int b = 0; b < 2; ++b) {
        const int x = bx + 10, y = by + (b ? 38 : 5), w = 20, h = 11;
        /* Pad only around the cap — do not extend into the label strip below. */
        cp_pad(g, x - 1, y - 1, w + 2, h + 1);
        g.fillRect(x, y, w, h, metal);
        g.fillRect(x + 1, y + 1, w - 2, h - 2, sh_rgb(0xc3, 0xc9, 0xd6));
        g.fillCircle(x + w / 2, y + h / 2, 3, sh_rgb(0x22, 0x23, 0x2c));
        const char *lab = b ? "EN" : "BOOT";
        const int ly = y + h + 4; /* clear gap under cap; EN ends ~by+53, frame by+70 */
        cp_lab_back(g, lab, x + (w - tiny_w(lab)) / 2, ly, silk);
    }
}

static void cp_draw_r16(Gfx g, int x, int y)
{
    const uint8_t silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    cp_pad(g, x, y, 66, 18);
    g.fillRect(x, y, 66, 18, PAL_BLACK);
    for (int k = 0; k < 6; ++k) {
        g.fillRect(x + k * 11, y + 3, 6, 3, sh_rgb(0xb8, 0xb8, 0xc0));
        g.fillRect(x + 2 + k * 11, y + 3, 2, 3, sh_rgb(0x20, 0x20, 0x24));
    }
    cp_lab_back(g, "R1-6", x + 20, y + 10, silk2);
}


/* Portrait Control part rects (layout A). Index map matches landscape CP_N. */
static bool cp_part_rect_p(int index, int *x, int *y, int *w, int *h)
{
    if (CP_SPREAD) return cp_part_rect_spread(index, x, y, w, h);
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
    if (CP_SPREAD) return cp_lab_y_spread(index);
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
    if (CP_SPREAD) return cp_route_spread(index, px, py);
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

/* Shared portrait terminal box — mock B: y 249..302, [abc] 34×28 @ 191,273. */
static const int PT_X = 10, PT_Y = 249, PT_W = 220, PT_H = 53;
/* Two command-row buttons (absolute): abc + glossary. */
static const int PT_ABC_X = 191, PT_ABC_Y = 273, PT_ABC_W = 34, PT_ABC_H = 28;
static const int PT_GL_X  = 154, PT_GL_Y  = 273, PT_GL_W  = 34, PT_GL_H  = 28;
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

static const char *pt_pfx(void)
{
    return s_screen == SCR_GIBSON ? "gibson> " : "user@botcorps:~$ ";
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
        txt(g, lab, kx + kw / 2, ky + (kh > 8 ? 2 : 1), PAL_WHITE, lgfx::textdatum::top_center);
    }
}

static void pt_draw_list(Gfx g, int x, int y, int w, int h)
{
    int n = 0;
    const char *const *cmds = pt_list(&n);
    const int back_h = 12;
    const int rh = 9;
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
        txt(g, cmds[i], x + 2, ry + 1, on ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE,
            lgfx::textdatum::top_left);
    }
    /* Dedicated BACK under the list — never on the cmdline (no overlap with cursor). */
    pt_draw_btn(g, x + w - 40, y + h - back_h, 38, back_h - 1, "BACK", false);
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

static void paint_pterm(Gfx g, int x, int y, int w, int h)
{
    s_pt_bx = x; s_pt_by = y; s_pt_bw = w; s_pt_bh = h;
    g.fillRect(x, y, w, h, PAL_BLACK);
    sb_frame(g, x, y, w, h, PAL_LINE);
    sb_frame(g, x + 1, y + 1, w - 2, h - 2, PAL_DEEP);
    if (s_pt_mode == 0) {
        /* Mock B scan/boards: log lines on top, cmdline above the two buttons. */
        pt_draw_btn(g, PT_ABC_X, PT_ABC_Y, PT_ABC_W, PT_ABC_H, "abc", false);
        pt_draw_btn(g, PT_GL_X, PT_GL_Y, PT_GL_W, PT_GL_H, "=", false);
        const int cmd_y = PT_ABC_Y + 8;
        const int log_top = y + 3;
        const int rh = 9;
        int rows = (cmd_y - log_top) / rh;
        if (rows > 2) rows = 2;
        if (rows < 0) rows = 0;
        for (int i = 0; i < rows && i < s_log_n; ++i) {
            const int idx = s_log_n - rows + i;
            if (idx < 0) continue;
            pt_draw_log_line(g, log_line(idx), x + 3, log_top + i * rh);
        }
        /* Cmdline left of the two buttons (Terminal two-button rule). */
        pt_draw_cmdline(g, x + 2, cmd_y, PT_GL_X - x - 4);
    } else {
        /* Keys/list modes: cmdline stays on top. */
        pt_draw_cmdline(g, x + 2, y + 2, w - 4);
        const int by2 = y + 15;
        const int bh2 = h - 17;
        if (s_pt_mode == 1) {
            /* Keys fill under the line; BACK is the OSK HIDE key (labelled BACK). */
            pt_draw_keys(g, x + 2, by2, w - 4, bh2);
        } else {
            pt_draw_list(g, x + 2, by2, w - 4, bh2);
        }
    }
}

/* Opaque pad + rain cover helper for portrait Control parts. */
static void cp_pad(Gfx g, int x, int y, int w, int h)
{
    g.fillRect(x, y, w, h, PAL_BLACK);
}


static void paint_control_spread_p(Gfx g)
{
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = sh_rgb(0xe9, 0xe9, 0xee), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t rail = sh_rgb(0x02, 0x02, 0x03), trace = sh_rgb(0x1c, 0x1c, 0x24), metal = sh_rgb(0x8f, 0x95, 0xa3);
    const uint8_t ac = accent();
    /* No terminal: clear pterm hit box. */
    s_pt_bx = 0; s_pt_by = 0; s_pt_bw = 0; s_pt_bh = 0;
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
    /* Full-height board — no terminal strip. */
    sb_frame(g, 2, 18, 236, 300, PAL_LINE);
    sb_frame(g, 3, 19, 234, 298, PAL_DEEP);
    GATE_BOX(g, "BOARD", "settings", 10, 20, 220, 296, "-");
    cp_draw_headers_spread(g);

    if (CP_SPREAD == 1) {
        cp_draw_boot_en(g, cp_px(140, 40), 170);
        cp_draw_power_bay(g, cp_px(48, 160), 236, 160, 40);
        cp_draw_r16(g, cp_px(48, 66), 286);
    } else {
        cp_draw_boot_en(g, cp_px(100, 40), 90);
        cp_draw_power_bay(g, cp_px(40, 160), 168, 160, 44);
        cp_draw_r16(g, cp_px(48, 66), 258);
    }

    int px[6], py[6];
    for (int i = 0; i < CP_N; ++i) {
        const int n = cp_route_p(i, px, py);
        for (int q = 0; q + 1 < n; ++q) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 1, trace, false);
    }
    if (s_cursor >= 0 && s_cursor < CP_N) {
        const int n = cp_route_p(s_cursor, px, py);
        for (int q = 0; q + 1 < n; ++q) cp_seg(g, px[q], py[q], px[q + 1], py[q + 1], 2, th_wire(), true);
    }
    for (int i = 0; i < CP_N; ++i) {
        if (i == 0 || i == 1 || i == 2 || i == 5 || i == 7) {
            int x, y, w, h;
            if (cp_part_rect_p(i, &x, &y, &w, &h)) cp_pad(g, x - 1, y - 1, w + 2, h + 2);
            cp_chip(g, i, s_cursor == i);
            /* Solid black backing under theme labels — rain never touches text. */
            char up[16];
            cp_short(i, up, (int)sizeof(up));
            const int ly = y + h - 9;
            g.fillRect(x, y + h - 13, w, 13, PAL_BLACK);
            cp_lab_back(g, up, x + (w - tiny_w(up)) / 2, ly, s_cursor == i ? (uint8_t)PAL_WHITE : silk2);
        } else {
            int x, y, w, h;
            if (!cp_part_rect_p(i, &x, &y, &w, &h)) continue;
            cp_pad(g, x - 2, y - 2, w + 4, h + 14);
            const bool on = s_cursor == i;
            if (on) {
                sel_rect(g, x - 3, y - 3, w + 6, h + 6);
                GATE_BOX(g, "SEL", "outline", x - 3, y - 3, w + 6, h + 6, "-");
            }
            const char *lab = "PWR";
            if (i == 3) {
                const bool solid = botcorps_rain_mode() > 0;
                g.fillRect(x, y + 2, w, 6, rail);
                for (int k = 0; k < 3; ++k) g.fillRect(x + 4 + k * 7, y + 4, 2, 2, gold);
                const int jx = solid ? x + 12 : x + 2;
                g.fillRect(jx, y, 12, 10, sh_rgb(0xff, 0xd4, 0x00));
                lab = "JUMP";
            } else if (i == 8) {
                const bool port = botcorps_cyd_orient != 0;
                g.fillRect(x, y + 2, w, 6, rail);
                for (int k = 0; k < 3; ++k) g.fillRect(x + 4 + k * 7, y + 4, 2, 2, gold);
                const int jx = port ? x + 12 : x + 2;
                g.fillRect(jx, y, 12, 10, sh_rgb(0x00, 0xc8, 0xff));
                lab = "ORIENT";
            } else if (i == 4) {
                g.fillRect(x, y, w, h, sh_rgb(0x16, 0x16, 0x1b));
                for (int k = 0; k < 5; ++k) {
                    g.fillRect(x + 3 + k * 4, y - 2, 2, 2, metal);
                    g.fillRect(x + 3 + k * 4, y + h, 2, 2, metal);
                    g.fillRect(x - 2, y + 3 + k * 4, 2, 2, metal);
                    g.fillRect(x + w, y + 3 + k * 4, 2, 2, metal);
                }
                cp_lab_back(g, "CP", x + w / 2 - 3, y + 4, silk2);
                const int m = botcorps_rain_mode();
                g.fillRect(x + 4, y + 14, w - 8, 5, m <= 0 ? botcorps_rain_palette(0) : botcorps_rain_solid());
                lab = "RAIN";
            } else if (i == 6) {
                const bool pon = s_bg_motion != 0;
                g.fillRect(x, y, w, h, sh_rgb(0xd9, 0xd9, 0xdf));
                g.fillRect(x + 2, y + 1, w - 4, h - 2, pon ? sh_rgb(0xff, 0x22, 0x44) : sh_rgb(0x3a, 0x0a, 0x12));
                lab = "PWR";
            }
            cp_lab_back(g, lab, x + (w - tiny_w(lab)) / 2, cp_lab_y_p(i), on ? (uint8_t)PAL_WHITE : silk);
        }
    }
}

static void paint_control_p(Gfx g)
{
    if (CP_SPREAD) { paint_control_spread_p(g); return; }
    const uint8_t gold = sh_rgb(0xd4, 0xa9, 0x3c), silk = sh_rgb(0xe9, 0xe9, 0xee), silk2 = sh_rgb(0x9a, 0x9a, 0xa6);
    const uint8_t rail = sh_rgb(0x02, 0x02, 0x03), trace = sh_rgb(0x1c, 0x1c, 0x24), metal = sh_rgb(0x8f, 0x95, 0xa3);
    const uint8_t ac = accent();
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
    sb_frame(g, 2, 19, 236, 230, PAL_LINE);
    sb_frame(g, 3, 20, 234, 228, PAL_DEEP);
    for (int side = 0; side < 2; ++side) {
        const int mx = side ? 233 : 6;
        g.fillRect(mx, 24, 1, 220, trace);
        for (int k = 0; k < 10; ++k) g.fillRect(mx - 1, 30 + k * 22, 3, 3, silk2);
    }
    GATE_BOX(g, "BOARD", "settings", 10, 20, 220, 228, "-");
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
        /* Pin-out labels: tiny 3×5 silk only — style A (default) tight beside pin,
         * style B between adjacent pins. Pads keep rain behind the glyphs. */
        const char *L = CP_P_LAYOUT ? BP[k] : TP[k];
        const char *R = CP_P_LAYOUT ? TP[k] : BP[k];
        const int lw = tiny_w(L), rw = tiny_w(R);
        if (CP_PIN_STYLE == 0) {
            /* A: small labels tight beside each pin header */
            const int ly = py - 1;
            g.fillRect(20, ly - 1, 16, 7, PAL_BLACK);
            g.fillRect(204, ly - 1, 16, 7, PAL_BLACK);
            tiny_text(g, L, 21, ly, silk);
            tiny_text(g, R, 219 - rw, ly, silk);
        } else {
            /* B: very small labels in the gap between this pin and the next (or under last) */
            const int ly = (k < 9) ? (py + 8) : (py + 6);
            g.fillRect(20, ly - 1, 16, 7, PAL_BLACK);
            g.fillRect(204, ly - 1, 16, 7, PAL_BLACK);
            tiny_text(g, L, 21, ly, silk);
            tiny_text(g, R, 219 - rw, ly, silk);
        }
    }
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
    {
        const int fx = cp_px(36, 162);
        g.drawRect(fx, 162, 162, 47, sh_rgb(0x2b, 0x2b, 0x33));
        {
            const int x = cp_px(40, 40), y = 164;
            cp_pad(g, x, y, 40, 40);
            g.fillRect(x, y, 40, 36, sh_rgb(0x15, 0x15, 0x18));
            g.fillRect(x + 5, y + 5, 30, 26, sh_rgb(0x05, 0x05, 0x06));
            g.fillCircle(x + 20, y + 18, 6, sh_rgb(0x2a, 0x2a, 0x30));
            txt(g, "DC6.5", x + 20, 202, silk, lgfx::textdatum::top_center);
        }
        {
            const int x = cp_px(88, 30), y = 166;
            cp_pad(g, x, y, 30, 30);
            g.fillRect(x + 4, y, 22, 6, sh_rgb(0xa9, 0xaf, 0xbb));
            g.fillRect(x, y + 6, 30, 16, sh_rgb(0x1a, 0x1a, 0x1f));
            for (int k = 0; k < 3; ++k) g.fillRect(x + 5 + k * 9, y + 22, 3, 6, sh_rgb(0xa9, 0xaf, 0xbb));
            txt(g, "1117C", x + 15, y + 8, silk2, lgfx::textdatum::top_center);
        }
        {
            const int x = cp_px(126, 17), y = 166, w = 17, hh = 30;
            cp_pad(g, x, y, w, hh + 8);
            g.fillRect(x, y, w, hh, sh_rgb(0x9a, 0xa1, 0xb2));
            g.fillRect(x + 1, y + 1, w - 2, hh - 2, sh_rgb(0xd0, 0xd5, 0xdf));
            g.fillRect(x + 4, y + 4, w - 8, hh - 8, sh_rgb(0x0a, 0x0a, 0x10));
            g.fillRect(x + 7, y + 8, 2, hh - 16, gold);
            txt(g, "USB5V", x + w / 2, 200, silk, lgfx::textdatum::top_center);
        }
        {
            const int x = cp_px(151, 22), y = 174;
            cp_pad(g, x, y, 22, 20);
            g.fillRect(x, y, 22, 12, sh_rgb(0xe0, 0xa2, 0x1c));
            g.fillRect(x, y, 5, 12, sh_rgb(0x7a, 0x52, 0x08));
            g.fillRect(x + 7, y + 3, 12, 6, sh_rgb(0x5a, 0x3a, 0x00));
            txt(g, "C1", x + 11, 190, silk, lgfx::textdatum::top_center);
        }
        {
            const int dcx = cp_px(80, 1), usbx = cp_px(126, 1), c1x = cp_px(162, 1);
            const int regx = cp_px(88, 1);
            g.fillRect(dcx < regx ? dcx : regx, 180, abs(regx - dcx) + 1, 1, trace);
            g.fillRect(usbx < (regx + 30) ? usbx : (regx + 18), 180, abs(usbx - (regx + 18)) + 1, 1, trace);
            g.fillRect(c1x, 160, 1, 15, trace);
        }
    }
    {
        cp_pad(g, cp_px(40, 66), 220, 66, 20);
        for (int k = 0; k < 6; ++k) {
            const int x = cp_px(40 + k * 11, 6);
            g.fillRect(x, 224, 6, 3, sh_rgb(0xb8, 0xb8, 0xc0));
            g.fillRect(x + 2, 224, 2, 3, sh_rgb(0x20, 0x20, 0x24));
        }
        txt(g, "R1-6", cp_px(40, 66) + 33, 239, silk2, lgfx::textdatum::top_center);
    }
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
    for (int i = 0; i < CP_N; ++i) {
        if (i == 0 || i == 1 || i == 2 || i == 5 || i == 7) {
            int x, y, w, h;
            if (cp_part_rect_p(i, &x, &y, &w, &h)) cp_pad(g, x - 1, y - 1, w + 2, h + 2);
            cp_chip(g, i, s_cursor == i);
        } else {
            int x, y, w, h;
            if (!cp_part_rect_p(i, &x, &y, &w, &h)) continue;
            cp_pad(g, x - 2, y - 2, w + 4, h + 16);
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
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

/* Portrait covers for Control: pads + pin-label text strips (rain stays BEHIND labels). */
static void layout_covers_control_p(void)
{
    if (CP_SPREAD) {
        cover_add(0, 0, 240, 17);
        cover_add(6, 26, 11, 270);
        cover_add(227, 26, 11, 270);
        cover_add(18, 26, 18, 270);
        cover_add(204, 26, 18, 270);
        for (int i = 0; i < CP_N; ++i) {
            int x, y, w, h;
            if (cp_part_rect_p(i, &x, &y, &w, &h)) cover_add(x - 1, y - 1, w + 2, h + 14);
        }
        if (CP_SPREAD == 1) {
            cover_add(cp_px(140, 40), 170, 40, 70);
            cover_add(cp_px(48, 160), 236, 160, 40);
            cover_add(cp_px(48, 66), 286, 66, 18);
        } else {
            cover_add(cp_px(100, 40), 90, 40, 70);
            cover_add(cp_px(40, 160), 168, 160, 44);
            cover_add(cp_px(48, 66), 258, 66, 18);
        }
        return;
    }
    cover_add(0, 0, 240, 17);
    cover_add(PT_X, PT_Y, PT_W, PT_H);
    cover_add(cp_px(13, 6), 24, 6, 220);
    cover_add(cp_px(222, 6), 24, 6, 220);
    /* Left/right pin label columns — tiny silk above rain (both styles). */
    cover_add(20, 24, 16, 220);
    cover_add(204, 24, 16, 220);
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

/* ---- Shared portrait chrome (mock B top-bar + frame) ---- */
static void paint_chrome_p(Gfx g)
{
    const uint8_t ac = accent();
    /* Top bar: Gibson chip + 12 bot swatches + crown + >_ + counters (240-wide fit). */
    g.fillRect(0, 0, 240, 26, PAL_BLACK);
    chip_mini(g, 14, 12, 14, 12, false, 0);
    static const uint8_t DIM[12] = { 9, 9, 9, 12, 76, 100, 33, 13, 76, 13, 100, 69 };
    for (int i = 0; i < 12; ++i)
        g.fillRect(30 + i * 8, 9, 5, 5, DIM[i]);
    gib_crown(g, 146, 3);
    g.drawRect(167, 2, 16, 13, ac);
    txt(g, ">", 170, 5, ac, lgfx::textdatum::top_left);
    g.fillRect(176, 11, 4, 1, ac);
    g.drawCircle(200, 12, 3, ac);
    g.fillRect(199, 11, 3, 3, ac);
    txt(g, "12", 206, 6, PAL_WHITE, lgfx::textdatum::top_left);
    txt(g, "0", 228, 9, ac, lgfx::textdatum::top_left);
    g.fillRect(0, 24, 240, 1, sb_tone(3));
    /* Frame ornaments — default PAL_LINE; Wi-Fi/BT overpaint via paint_chrome_bot_p. */
    g.drawRect(5, 26, 230, 294, PAL_LINE);
    g.drawRect(6, 27, 228, 292, PAL_DEEP);
}

/* Mock B: Wi-Fi/BT outer chrome is bot colour (blue / cyan), not theme grape. */
static void paint_chrome_bot_p(Gfx g, uint8_t botc)
{
    paint_chrome_p(g);
    g.drawRect(5, 26, 230, 294, botc);
    g.drawRect(6, 27, 228, 292, botc);
}

static void paint_scan_grid_p(Gfx g, int gx, int gy, int cols, int rows, int pitch, uint8_t frame_col, bool sparse = false)
{
    /* Black plate (matches mock charcoal within tol=24) + double frame + green grid.
     * sparse=true (AirTag/bots): frame + faint vertical guides only — mock B is nearly empty. */
    const int gw = cols * pitch + 1, gh = rows * pitch + 1;
    g.fillRect(gx - 3, gy - 3, gw + 6, gh + 6, PAL_BLACK);
    sb_frame(g, gx - 3, gy - 3, gw + 6, gh + 6, frame_col);
    sb_frame(g, gx - 2, gy - 2, gw + 4, gh + 4, frame_col);
    g.fillRect(gx, gy, gw, gh, PAL_BLACK);
    if (sparse) {
        for (int c = 0; c <= cols; ++c)
            g.fillRect(gx + c * pitch, gy, 1, gh, (uint8_t)8); /* charcoal guides */
        for (int r = 0; r <= rows; r += 2)
            g.fillRect(gx, gy + r * pitch, gw, 1, (uint8_t)8);
    } else {
        for (int c = 0; c <= cols; ++c) g.fillRect(gx + c * pitch, gy, 1, gh, (uint8_t)1);
        for (int r = 0; r <= rows; ++r) g.fillRect(gx, gy + r * pitch, gw, 1, (uint8_t)1);
        for (int c = 0; c <= cols; ++c)
            for (int r = 0; r <= rows; ++r)
                g.fillRect(gx + c * pitch - 1, gy + r * pitch - 1, 3, 3, (uint8_t)65);
    }
    /* White tooth pads along top/bottom frame (mock B). */
    for (int c = 0; c < cols; ++c) {
        const int tx = gx + 4 + c * pitch;
        g.fillRect(tx, gy - 2, 3, 2, PAL_WHITE);
        g.fillRect(tx, gy + gh, 3, 2, PAL_WHITE);
    }
}

/* Hub portrait B: side bot columns, Gibson core centre, 2 plates, terminal. */
static void paint_gibson_p(Gfx g)
{
    paint_chrome_p(g);
    const uint32_t t = millis();
    const uint8_t ac = accent();
    const uint8_t wire = wire_ink();
    const int cx = 120, cy = 114, rr = 40;
    bool asleep = false;
    const int mood = hub_mood(t, &asleep);
    /* Side bot columns (mock: left @10,44 / right @192,44). */
    for (int i = 0; i < 12; ++i) {
        const bool left = i < 6;
        const int sx = left ? 10 : 192;
        const int sy = 44 + (i % 6) * 23;
        const int ys = sy + 10;
        g.fillRect(sx - 1, sy - 1, 30, 22, wire);
        GATE_BOX(g, "TILE", "hubbot", sx, sy, 28, 20, "-");
        chip_plate(g, sx, sy, 28, 20, PAL_DEEP, th_item(i, 12));
        g.drawRect(sx - 1, sy - 1, 30, 22, th_item(i, 12));
        chip_mini(g, sx + 11, sy + 10, 18, i, false, 0);
        g.fillRect(sx + 22, sy + 3, 4, 4, bot_color(i));
        /* Trace toward core */
        const int x1 = left ? sx + 28 : cx - rr;
        const int x2 = left ? cx - rr : sx;
        if (x2 > x1) {
            g.fillRect(x1, ys, x2 - x1, 1, th_item(i, 12));
        }
    }
    /* Gibson core (rings + chip) centred in upper grid zone. */
    GATE_BOX(g, "HUBCORE", "rings", cx - 50, cy - 50, 100, 100, "-");
    hub_die(g, cx, cy, 30);
    core_bezel(g, cx, cy, 30);
    const int spin = (int)(t / 90u);
    GIB_CORE_RING(g, cx, cy, 33, th_ring(0), 5, 5, spin);
    GIB_CORE_RING(g, cx, cy, 38, th_ring(1), 2, 4, (int)(t / 140u));
    GIB_CORE_RING(g, cx, cy, 44, th_ring(2), 1, 5, (int)(t / 80u));
    gib_core_chip_d(g, cx, cy, 4);
    if (s_hub_zone == 3) {
        g.drawCircle(cx, cy, 28, PAL_WHITE);
        g.drawCircle(cx, cy, 27, ac);
    }
    gib_crown(g, cx, 48);
    /* 2 nav plates only (LL-20) — Terminal + Control, mock y≈207. */
    static const int xs[2] = { 88, 124 };
    for (int i = 0; i < 2; ++i) {
        const int x = xs[i], y = 207;
        const bool sel = s_hub_zone == 0 && s_cursor == i;
        GATE_BOX(g, "BTN", "hubnav", x, y, BS, BS, "-");
        g.fillRect(x + 1, y, BS - 2, BS, ac);
        g.fillRect(x, y + 1, BS, BS - 2, ac);
        g.fillRect(x + 2, y + 2, BS - 4, BS - 4, PAL_DEEP);
        if (sel) g.drawRect(x - 2, y - 2, BS + 4, BS + 4, PAL_WHITE);
        if (i == 0) {
            g.fillRect(x + 4, y + 8, 20, 15, PAL_WHITE);
            g.fillRect(x + 5, y + 9, 18, 13, PAL_BLACK);
            g.drawLine(x + 7, y + 12, x + 10, y + 15, PAL_WHITE);
            g.drawLine(x + 10, y + 15, x + 7, y + 18, PAL_WHITE);
            g.fillRect(x + 12, y + 18, 6, 1, PAL_WHITE);
        } else {
            g.fillRect(x + 4, y + 6, 20, 16, PAL_BLACK);
            g.drawRect(x + 4, y + 6, 20, 16, ac);
            g.fillRect(x + 10, y + 10, 8, 8, ac);
        }
    }
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

/* Terminal (KING) portrait B: real 3×4 boards via term_xy + paint_boards. */
static void paint_terminal_p(Gfx g)
{
    paint_chrome_p(g);
    paint_boards(g);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

/* ---- Mock B scan helpers: device panel + traced chip cluster + logs ---- */
static void port_seed_scan_logs(bool wifi, bool ble, bool gib, int bot)
{
    log_clear();
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

/* Traces from MCU chip to peri + long runs up to the scan grid (mock B).
 * Manhattan stubs stay in the gutters — never cross silk / chip faces. */
static void paint_chip_traces_p(Gfx g, int mcx, int mcy, int ms, int px, int py, int ps, uint8_t col, int n_up)
{
    const int mx0 = mcx - ms / 2, mx1 = mcx + ms / 2;
    const int my0 = mcy - ms / 2, my1 = mcy + ms / 2;
    const int grid_bot = 168; /* just under the device panel */
    for (int i = 0; i < n_up; ++i) {
        const int pin = mx0 + 6 + i * ((ms - 12) / (n_up > 1 ? n_up - 1 : 1));
        const int top = grid_bot - (i & 1) * 4;
        g.fillRect(pin, top, 1, my0 - top, col);
        g.fillRect(pin - 1, top, 3, 2, col);
        g.fillRect(pin - 1, my0 - 2, 3, 3, col);
        g.fillRect(pin, my0 - 1, 1, 1, PAL_BLACK);
    }
    /* Side rails in the chrome gutters — mock B uses slate, not bot-colour runs. */
    for (int s = 0; s < 2; ++s) {
        const int rx = s ? 226 : 12;
        for (int k = 0; k < 6; ++k) {
            const int yy = 40 + k * 20;
            g.fillRect(rx, yy, 2, 12, (uint8_t)PAL_LINE);
        }
    }
    /* Horizontal MCU → peri: mock B uses bot-colour runs (4 lanes). */
    const int x0 = mx1, x1 = px - ps / 2;
    if (x1 > x0) {
        for (int k = -1; k <= 2; ++k) {
            const int yy = mcy + k * 4;
            g.fillRect(x0, yy, x1 - x0, 1, col);
            g.fillRect(x1 - 2, yy - 1, 3, 3, col);
        }
    }
    /* Left stub teeth — mock Wi-Fi blue cluster reaches ~x60 from MCU. */
    for (int k = -1; k <= 2; ++k) {
        const int yy = mcy + k * 4;
        g.fillRect(mx0 - 18, yy, 18, 1, col);
        g.fillRect(mx0 - 20, yy - 1, 3, 3, col);
    }
    if (py > mcy + 10) {
        const int x2 = px - ps / 2;
        g.fillRect(mx1 - 4, my1, 1, py - my1, col);
        g.fillRect(mx1 - 4, py, x2 - (mx1 - 4), 1, col);
    }
    /* No downward botc stubs — mock B chip sits on black plate. */
}


/* Mock B peri: ~26 px black die + cyan (or bot) geared rim + device glyph. */
static void paint_peri_p(Gfx g, int cx, int cy, int bot, uint8_t rim)
{
    const int T = 24;
    const int x = cx - T / 2, y = cy - T / 2;
    g.fillRect(x, y, T, T, PAL_BLACK);
    g.drawRect(x, y, T, T, rim);
    g.drawRect(x - 1, y - 1, T + 2, T + 2, rim);
    for (int k = 0; k < 5; ++k) {
        const int i = 2 + k * 5;
        g.fillRect(x - 2, y + i, 2, 3, rim);
        g.fillRect(x + T, y + i, 2, 3, rim);
        g.fillRect(x + i, y - 2, 3, 2, rim);
        g.fillRect(x + i, y + T, 3, 2, rim);
    }
    gl_icon(g, bot_dev_kind(bot), cx - 6, cy - 6, false);
}

/* Silkscreen block left of chip cluster (WIFIBOT / ESP32-WROOM-32 / RADIO CORE / U2). */
static void paint_scan_silk_p(Gfx g, int bot, bool gib, bool wifi, bool ble)
{
    /* Mock B: black plate under silk; wifi/ble text botc (cyan wash on glyphs). */
    const uint8_t silk = gib ? (uint8_t)PAL_IVORY
                             : (wifi || ble) ? bot_color(wifi ? 0 : 1)
                             : bot_color(bot);
    char nm[16];
    if (gib) snprintf(nm, sizeof nm, "GIBSON");
    else if (wifi) snprintf(nm, sizeof nm, "WIFIBOT");
    else if (ble) snprintf(nm, sizeof nm, "BLEBOT");
    else {
        sb_upper(nm, 16, BOTS[bot].short_name, 12);
        if (strlen(nm) <= 9) strcat(nm, "BOT");
    }
    const char *l2 = "ESP32-WROOM-32";
    const char *l3 = gib ? "CORE + PRIME" : "RADIO CORE";
    char l4[20];
    if (gib) snprintf(l4, sizeof l4, "PRIME");
    else if (wifi) snprintf(l4, sizeof l4, "WI-FI AP U2");
    else if (ble) snprintf(l4, sizeof l4, "BLE U2");
    else snprintf(l4, sizeof l4, "%s U2", GL_LABEL[bot_dev_kind(bot)]);
    g.fillRect(14, 176, 72, 48, PAL_FIELD);
    tiny_text(g, nm, 16, 180, silk);
    tiny_text(g, l2, 16, 188, silk);
    tiny_text(g, l3, 16, 196, silk);
    tiny_text(g, l4, 16, 208, silk);
}

/* Landscape recipe: 18×18 rim 149 / face 5; sel = white rim + sb_tone(3) + sb_halo. */
static void paint_scan_tiles_p(Gfx g, int gx, int gy, int pitch, int n, bool wifi)
{
    int hx0 = -1, hy0 = -1;
    for (int k = 0; k < n; ++k) {
        const int c = k % 8, r = k / 8;
        if (r >= 4) break;
        const int x = gx + 2 + c * pitch, y = gy + 2 + r * pitch;
        const bool sel = (k == 0);
        g.fillRect(x, y, 18, 18, sel ? (uint8_t)PAL_WHITE : (uint8_t)149);
        g.fillRect(x + 1, y + 1, 16, 16, sel ? sb_tone(3) : (uint8_t)5);
        if (wifi) v2_icon_at(g, k, x + 3, y + 3);
        else gl_icon(g, (int)LIST_BLE_TY[k % SB_N], x + 3, y + 3, false);
        if (sel) { hx0 = x; hy0 = y; }
    }
    if (hx0 >= 0) sb_halo(g, hx0, hy0, 18);
}

/* Wi-Fi / BT: bot-colour chrome + landscape tile recipe + traced chips. */
static void paint_radar_p(Gfx g, bool wifi)
{
    const int bot = wifi ? 0 : 1;
    const uint8_t botc = bot_color(bot);
    paint_chrome_bot_p(g, botc);
    g.fillRect(10, 28, 220, 218, PAL_FIELD);
    /* Mock B top bus + white pin teeth (y≈30/34). */
    g.fillRect(16, 30, 208, 1, botc);
    for (int i = 0; i < 24; ++i) {
        const int tx = 20 + i * 8;
        g.fillRect(tx, 33, 3, 2, PAL_WHITE);
    }
    const int gx = 33, gy = 46, cols = 8, rows = 5, pitch = 22;
    paint_scan_grid_p(g, gx, gy, cols, rows, pitch, botc);
    paint_scan_tiles_p(g, gx, gy, pitch, wifi ? 17 : 16, wifi);
    /* Side rails mid-board (mock B). */
    for (int i = 0; i < 4; ++i) {
        g.fillRect(gx - 3, gy + 20 + i * 22, 2, 10, botc);
        g.fillRect(gx + cols * pitch + 1, gy + 20 + i * 22, 2, 10, botc);
    }
    /* Mock B: MCU face ~y192–222; peri cyan die ~24–26 px. */
    const int mcx = 120, mcy = 207, ms = 48;
    const int pcx = 172, pcy = 205, ps = 24;
    paint_chip_traces_p(g, mcx, mcy, ms, pcx, pcy, ps, botc, 0);
    paint_scan_silk_p(g, bot, false, wifi, !wifi);
    chip_block(g, mcx, mcy, ms, bot);
    paint_peri_p(g, pcx, pcy, bot, (uint8_t)PAL_CYAN);
    port_seed_scan_logs(wifi, !wifi, false, bot);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

/* Gibson accordion vs mock B: purple frame, teal WIFI open, 3-row tiles, BT/DEAUTH meters. */
static void paint_gibson_accordion_p(Gfx g)
{
    const uint8_t ice = bot_color(1);      /* BT cyan */
    const uint8_t ice_bar = (uint8_t)155;  /* RGB332 light ice top bus */
    const uint8_t wifi_c = bot_color(0);
    const uint8_t grape = (uint8_t)69;     /* muted grape ~mock (90,36,99) */
    const uint8_t chalk = (uint8_t)8;
    const uint8_t deauth_c = (uint8_t)224; /* red accent */
    /* Top bus + ice/copper pins. */
    g.fillRect(16, 30, 208, 1, ice_bar);
    for (int i = 0; i < 24; ++i) {
        const int tx = 20 + i * 8;
        g.fillRect(tx, 33, 3, 2, (uint8_t)217);
        g.fillRect(tx + 1, 34, 1, 1, PAL_BLACK);
    }
    /* Outer accordion — mock 1px muted-purple rim ~x32–208, y41–160. */
    const int ax = 32, aw = 176, ay = 41, ah = 120;
    g.fillRect(ax, ay, aw, ah, chalk);
    sb_frame(g, ax, ay, aw, ah, grape);
    /* Crew header. */
    chip_mini(g, ax + 10, ay + 8, 12, 12, false, 0);
    tiny_text(g, "GIBSON", ax + 20, ay + 3, PAL_IVORY);
    tiny_text(g, "CREW DROP-DOWN", ax + 20, ay + 9, COL_MUTED);
    tiny_text(g, "63 FINDS", ax + aw - 40, ay + 5, PAL_IVORY);
    /* WIFI open lane — landscape recipe: sb_mix(bc,0,72) body + left accent. */
    const int y1 = 58, x = ax, w = aw;
    const uint8_t wifi_body = sb_mix(wifi_c, 0x0000, 72);
    g.fillRect(x, y1, w, 12, wifi_body);
    g.fillRect(x, y1, 3, 12, wifi_c);
    chip_mini(g, x + 10, y1 + 5, 10, 0, false, 0);
    tiny_text(g, "WIFI", x + 20, y1 + 3, PAL_WHITE);
    tiny_text(g, "12", x + w - 28, y1 + 3, PAL_IVORY);
    tiny_text(g, "v", x + w - 12, y1 + 3, PAL_IVORY);
    /* 3-row tiles (mock y≈70–128): 16 wifi + 1 ble start; rest empty black. */
    const int gx = 40, gy = 72, pitch = 20;
    paint_scan_grid_p(g, gx, gy, 8, 3, pitch, (uint8_t)65);
    paint_scan_tiles_p(g, gx, gy, pitch, 17, true);
    /* White corner brackets around open panel (mock B). */
    g.fillRect(gx - 2, gy - 2, 6, 2, PAL_WHITE);
    g.fillRect(gx - 2, gy - 2, 2, 6, PAL_WHITE);
    g.fillRect(gx + 8 * pitch - 4, gy - 2, 6, 2, PAL_WHITE);
    g.fillRect(gx + 8 * pitch, gy - 2, 2, 6, PAL_WHITE);
    g.fillRect(gx - 2, gy + 3 * pitch - 0, 6, 2, PAL_WHITE);
    g.fillRect(gx - 2, gy + 3 * pitch - 4, 2, 6, PAL_WHITE);
    /* Side blue jog traces (mock), not copper teeth. */
    for (int i = 0; i < 4; ++i) {
        const int yy = 70 + i * 20;
        g.fillRect(ax - 5, yy, 4, 1, wifi_c);
        g.fillRect(ax - 5, yy, 1, 12, wifi_c);
        g.fillRect(ax + aw + 1, yy, 4, 1, wifi_c);
        g.fillRect(ax + aw + 4, yy, 1, 12, wifi_c);
    }
    /* BT collapsed — charcoal + ice + meter pips (mock y≈136). */
    const int yb = 136;
    g.fillRect(x, yb, w, 10, chalk);
    g.fillRect(x, yb, 3, 10, ice);
    sb_frame(g, x, yb, w, 10, ice);
    chip_mini(g, x + 10, yb + 4, 10, 1, false, 0);
    tiny_text(g, "BLUETOOTH", x + 20, yb + 2, PAL_WHITE);
    for (int k = 0; k < 6; ++k)
        g.fillRect(x + 78 + k * 5, yb + 3, 3, 3, ice);
    tiny_text(g, "12", x + w - 28, yb + 2, PAL_IVORY);
    tiny_text(g, ">", x + w - 12, yb + 2, ice);
    /* DEAUTH collapsed — red accent + meter pips (mock y≈148). */
    const int yd = 148;
    g.fillRect(x, yd, w, 10, chalk);
    g.fillRect(x, yd, 3, 10, deauth_c);
    sb_frame(g, x, yd, w, 10, deauth_c);
    chip_mini(g, x + 10, yd + 4, 10, 2, false, 0);
    tiny_text(g, "DEAUTH", x + 20, yd + 2, PAL_WHITE);
    for (int k = 0; k < 4; ++k)
        g.fillRect(x + 78 + k * 5, yd + 3, 3, 3, deauth_c);
    tiny_text(g, "4", x + w - 28, yd + 2, PAL_IVORY);
    tiny_text(g, ">", x + w - 12, yd + 2, deauth_c);
}

/* AirTag pattern: PAL_LINE chrome (caller), gold top bus, radar targets + chips. */
static void paint_bot_board_p(Gfx g, int bot)
{
    const uint8_t bc = bot_color(bot);
    const uint8_t gold = (uint8_t)248; /* RGB332 gold top bus (255,219,0) */
    g.fillRect(10, 28, 220, 218, PAL_FIELD);
    g.fillRect(16, 30, 208, 1, gold);
    /* White pin teeth (mock y=34). */
    for (int i = 0; i < 24; ++i) {
        const int tx = 20 + i * 8;
        g.fillRect(tx, 33, 3, 2, PAL_WHITE);
    }
    const int gx = 34, gy = 50, cols = 8, rows = 5, pitch = 21;
    /* Frame stays PAL_LINE — bot colour only on targets / traces, not outer chrome. */
    paint_scan_grid_p(g, gx, gy, cols, rows, pitch, (uint8_t)PAL_LINE, true);
    chip_mini(g, gx + 40, gy + 36, 18, bot, false, 0);
    g.fillRect(gx + 34, gy + 30, 12, 12, bc);
    g.drawRect(gx + 33, gy + 29, 14, 14, PAL_WHITE);
    /* Targets — mock AirTag cluster around x≈150,y≈75. */
    static const int TX[3] = { 120, 150, 170 };
    static const int TY[3] = { 28, 40, 55 };
    for (int t = 0; t < 3; ++t) {
        const int cx = gx + TX[t], cy = gy + TY[t];
        g.drawCircle(cx, cy, 7, bc);
        g.drawCircle(cx, cy, 5, bc);
        g.fillCircle(cx, cy, 2, PAL_BLACK);
        for (int d = 1; d <= 4; ++d)
            g.fillRect(cx, cy + 8 + d * 3, 1, 1, sb_tone(3));
    }
    for (int i = 0; i < 4; ++i) {
        g.fillRect(gx - 3, gy + 16 + i * 18, 2, 10, bc);
        g.fillRect(gx + cols * pitch + 1, gy + 16 + i * 18, 2, 10, bc);
    }
    const int mcx = 120, mcy = 207, ms = 48;
    const int pcx = 172, pcy = 205, ps = 24;
    paint_chip_traces_p(g, mcx, mcy, ms, pcx, pcy, ps, bc, 0);
    paint_scan_silk_p(g, bot, false, false, false);
    chip_block(g, mcx, mcy, ms, bot);
    paint_peri_p(g, pcx, pcy, bot, bc);
}

/* Bot / Gibson SCAN — Gibson keeps PAL_LINE chrome; bots keep PAL_LINE (not bot chrome). */
static void paint_scan_p(Gfx g)
{
    paint_chrome_p(g);
    const bool gib = s_scan_bot >= 12;
    const int bot = gib ? 12 : (s_scan_bot % 12);
    const uint8_t ac = accent();
    if (gib) {
        g.fillRect(10, 28, 220, 218, PAL_FIELD);
        paint_gibson_accordion_p(g);
        const int mcx = 120, mcy = 207, ms = 48;
        paint_chip_traces_p(g, mcx, mcy, ms, 172, 192, 24, (uint8_t)PAL_IVORY, 0);
        paint_chip_traces_p(g, mcx, mcy, ms, 172, 220, 24, (uint8_t)PAL_IVORY, 0);
        paint_scan_silk_p(g, 12, true, false, false);
        chip_block(g, mcx, mcy, ms, 12);
        chip_block(g, 172, 192, 24, 0);
        chip_block(g, 172, 220, 24, 1);
        port_seed_scan_logs(false, false, true, 12);
    } else {
        /* Outer chrome stays PAL_LINE (mock B); gold bus is inside paint_bot_board_p. */
        paint_bot_board_p(g, bot);
        port_seed_scan_logs(false, false, false, bot);
    }
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
    (void)ac;
}

static void paint_list_p(Gfx g, bool wifi)
{
    paint_chrome_p(g);
    g.fillRect(10, 30, 220, 210, PAL_DEEP);
    sb_frame(g, 10, 30, 220, 210, PAL_LINE);
    txt(g, wifi ? "WIFI LIST" : "BLE LIST", 120, 40, accent(), lgfx::textdatum::top_center);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void paint_gchat_p(Gfx g)
{
    paint_chrome_p(g);
    g.fillRect(10, 30, 220, 210, PAL_DEEP);
    sb_frame(g, 10, 30, 220, 210, PAL_LINE);
    paint_pterm(g, PT_X, PT_Y, PT_W, PT_H);
}

static void layout_covers_portrait_common(void)
{
    cover_add(0, 0, 240, 26);           /* top bar */
    cover_add(PT_X, PT_Y, PT_W, PT_H);  /* terminal block */
    cover_add(5, 26, 5, 279);           /* frame L */
    cover_add(230, 26, 5, 279);         /* frame R */
    /* Scan boards: keep rain out of silk/chip/grid gutters (mock matte plate). */
    if (s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR || s_screen == SCR_SCAN) {
        cover_add(10, 28, 220, 218);
        cover_add(14, 176, 72, 48);   /* silk plate */
        cover_add(90, 176, 100, 60);  /* MCU + peri */
    }
}

static void paint_ext_portrait(Gfx g)
{
#ifdef BOTCORPS_HOST_RENDER
    host_gate_reset();
    layout_covers();
#endif
    g.clearClipRect();
    g.setTextWrap(false, false);
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
/* ---- Host crop-compose from live landscape paint (mock v5 layout B recipe) ---- */
static uint8_t s_port_land[320 * 240];

static void port_blit(uint8_t *dst, const uint8_t *src,
                      int sx, int sy, int w, int h, int dx, int dy)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int X = dx + x, Y = dy + y;
            if ((unsigned)X >= 240u || (unsigned)Y >= 320u) continue;
            const int Sx = sx + x, Sy = sy + y;
            if ((unsigned)Sx >= 320u || (unsigned)Sy >= 240u) continue;
            dst[Y * 240 + X] = src[Sy * 320 + Sx];
        }
    }
}

static void port_blit_flip_h(uint8_t *dst, const uint8_t *src,
                             int sx, int sy, int w, int h, int dx, int dy)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int X = dx + x, Y = dy + y;
            if ((unsigned)X >= 240u || (unsigned)Y >= 320u) continue;
            const int Sx = sx + (w - 1 - x), Sy = sy + y;
            if ((unsigned)Sx >= 320u || (unsigned)Sy >= 240u) continue;
            dst[Y * 240 + X] = src[Sy * 320 + Sx];
        }
    }
}

static void port_repeat_col(uint8_t *dst, const uint8_t *src,
                            int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
    for (int y = 0; y < dh; ++y) {
        for (int x = 0; x < dw; ++x) {
            const int X = dx + x, Y = dy + y;
            if ((unsigned)X >= 240u || (unsigned)Y >= 320u) continue;
            const int Sx = sx + (sw > 0 ? x % sw : 0);
            const int Sy = sy + (sh > 1 ? (y % sh) : 0);
            if ((unsigned)Sx >= 320u || (unsigned)Sy >= 240u) continue;
            dst[Y * 240 + X] = src[Sy * 320 + Sx];
        }
    }
}

static void port_repeat_row(uint8_t *dst, const uint8_t *src,
                            int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
    for (int y = 0; y < dh; ++y) {
        for (int x = 0; x < dw; ++x) {
            const int X = dx + x, Y = dy + y;
            if ((unsigned)X >= 240u || (unsigned)Y >= 320u) continue;
            const int Sx = sx + (sw > 1 ? (x % sw) : 0);
            const int Sy = sy + (sh > 0 ? y % sh : 0);
            if ((unsigned)Sx >= 320u || (unsigned)Sy >= 240u) continue;
            dst[Y * 240 + X] = src[Sy * 320 + Sx];
        }
    }
}

static void port_fill(uint8_t *dst, int x, int y, int w, int h, uint8_t c)
{
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) {
            const int X = x + xx, Y = y + yy;
            if ((unsigned)X < 240u && (unsigned)Y < 320u) dst[Y * 240 + X] = c;
        }
}

static void port_paint_land(int screen)
{
    uint8_t *saved = host_frame;
    const int sfw = host_fw, sfh = host_fh;
    const uint8_t sor = botcorps_cyd_orient;
    host_frame = s_port_land;
    host_fw = 320; host_fh = 240;
    botcorps_cyd_orient = 0;
    botcorps_rain_set_view(320, 240);
    botcorps_band_force_direct(false);
    memset(s_port_land, PAL_FIELD, sizeof s_port_land);
    botcorps_host_enter(s_port_land, screen);
    /* botcorps_host_enter paints via paint_ext when orient=0 */
    host_frame = saved;
    host_fw = sfw; host_fh = sfh;
    botcorps_cyd_orient = sor;
    botcorps_rain_set_view(240, 320);
    botcorps_band_force_direct(true);
}

/* Shared chrome + frame from a landscape source (hub/term/scan land paint). */
static void port_compose_chrome(uint8_t *dst, const uint8_t *src)
{
    port_fill(dst, 0, 0, 240, 320, PAL_FIELD);
    port_repeat_col(dst, src, 130, 0, 1, 26, 0, 0, 240, 26);
    port_blit(dst, src, 0, 0, 126, 26, 0, 0);
    port_blit(dst, src, 141, 0, 38, 24, 127, 0);
    port_blit(dst, src, 188, 0, 20, 24, 167, 0);
    port_blit(dst, src, 230, 0, 34, 24, 188, 0);
    port_blit(dst, src, 294, 0, 9, 24, 227, 0);
    port_blit(dst, src, 14, 26, 20, 12, 5, 26);
    port_repeat_col(dst, src, 40, 26, 8, 12, 25, 26, 190, 12);
    port_blit(dst, src, 286, 26, 20, 12, 215, 26);
    port_blit(dst, src, 14, 38, 5, 151, 5, 38);
    port_repeat_row(dst, src, 14, 100, 5, 1, 5, 189, 5, 116);
    port_blit_flip_h(dst, src, 14, 38, 5, 151, 230, 38);
    port_repeat_row(dst, src, 14, 100, 5, 1, 230, 189, 5, 116);
    /* flip each row of the repeated right border — approximate via blit_flip of source col */
    port_blit(dst, src, 14, 194, 20, 15, 5, 305);
    port_repeat_col(dst, src, 40, 194, 8, 15, 25, 305, 190, 15);
    port_blit(dst, src, 286, 194, 20, 15, 215, 305);
    port_fill(dst, 10, 38, 220, 267, PAL_FIELD);
}

static void port_compose_hub(uint8_t *dst)
{
    port_paint_land(SCR_GIBSON);
    port_compose_chrome(dst, s_port_land);
    port_blit(dst, s_port_land, 0, 22, 38, 138, 10, 44);
    port_blit(dst, s_port_land, 282, 22, 38, 138, 192, 44);
    port_blit(dst, s_port_land, 96, 28, 128, 132, 56, 48);
    port_blit(dst, s_port_land, 40, 202, 240, 38, 0, 207);
    host_frame = dst;
    host_fw = 240; host_fh = 320;
    paint_pterm(Gfx(), PT_X, PT_Y, PT_W, PT_H);
}

static void port_compose_term(uint8_t *dst)
{
    port_paint_land(SCR_TERMINAL);
    port_compose_chrome(dst, s_port_land);
    /* 4×3 land tiles → 3×4 portrait (mock B recipe destinations). */
    static const int SRC[12][2] = {
        {5, 18}, {83, 18}, {161, 18}, {239, 18},
        {5, 68}, {83, 68}, {161, 68}, {239, 68},
        {5, 118}, {83, 118}, {161, 118}, {239, 118}
    };
    static const int DST[12][2] = {
        {6, 19}, {84, 19}, {162, 19},
        {6, 69}, {84, 69}, {162, 69},
        {6, 119}, {84, 119}, {162, 119},
        {6, 169}, {84, 169}, {162, 169}
    };
    for (int i = 0; i < 12; ++i)
        port_blit(dst, s_port_land, SRC[i][0], SRC[i][1], 76, 50, DST[i][0], DST[i][1]);
    host_frame = dst;
    host_fw = 240; host_fh = 320;
    paint_pterm(Gfx(), PT_X, PT_Y, PT_W, PT_H);
}

static void port_compose_scan(uint8_t *dst, int screen)
{
    /* Host path: same mock-B painters as device (device list + traced chips + logs). */
    port_paint_land(screen); /* keep land buffer warm for optional log blit */
    (void)s_port_land;
    host_frame = dst;
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    botcorps_rain_set_view(240, 320);
    botcorps_band_force_direct(true);
    memset(dst, PAL_BLACK, 240u * 320u);
    if (screen == SCR_WIFI_RADAR || screen == SCR_BT_RADAR)
        paint_radar_p(Gfx(), screen == SCR_WIFI_RADAR);
    else
        paint_scan_p(Gfx());
}

void botcorps_host_control_p(uint8_t *buf, int cursor, int layout)
{
    extern int host_fw, host_fh;
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    /* layout: bit0=geom A/B, bit1=pin B, bits4..=CP_SPREAD (1/2 = no-terminal spreads). */
    CP_P_LAYOUT = (layout & 1) ? 1 : 0;
    CP_PIN_STYLE = (layout & 2) ? 1 : 0;
    CP_SPREAD = (layout >> 4) & 7;
    if (CP_SPREAD > 2) CP_SPREAD = 0;
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
    /* Seed screen state (scan bot etc.) via enter, then compose. */
    /* screen 70..81 = bot boards 0..11 (AirTag = 76); SCR_SCAN alone = Gibson SCAN. */
    if (screen >= 70 && screen <= 81) {
        s_scan_bot = screen - 70;
        screen = SCR_SCAN;
    } else if (screen == SCR_SCAN) {
        s_scan_bot = 12; /* Gibson SCAN default */
    }
    botcorps_host_enter(buf, screen);
    s_pt_mode = (uint8_t)pt_mode;
    s_pt_sym = pt_sym != 0;
    if (pt_mode == 1 || pt_mode == 2) {
        int cap = 0;
        char *b = pt_buf(&cap);
        if (b && pt_mode == 1)
            snprintf(b, (size_t)cap, "scan wifi and a-very-long-line");
        if (b && pt_mode == 2) {
            int n = 0; const char *const *L = pt_list(&n);
            if (n > 0) { s_pt_sel = 0; snprintf(b, (size_t)cap, "%s", L[0]); }
        }
    }
    memset(buf, PAL_BLACK, 240u * 320u);
    host_frame = buf;
    /* Seed scan cmdline so pterm matches mock B ("…$ scan"). */
    if (screen == SCR_WIFI_RADAR || screen == SCR_BT_RADAR || screen == SCR_SCAN) {
        int cap = 0;
        char *b = pt_buf(&cap);
        if (b && cap > 0) snprintf(b, (size_t)cap, "scan");
    }
    if (screen == SCR_CONTROL) {
        paint_ext_portrait(Gfx());
        return;
    }
    if (screen == SCR_GIBSON) port_compose_hub(buf);
    else if (screen == SCR_TERMINAL) port_compose_term(buf);
    else if (screen == SCR_WIFI_RADAR || screen == SCR_BT_RADAR || screen == SCR_SCAN)
        paint_ext_portrait(Gfx()); /* rain + mock-B painters (not stub crop) */
    else
        paint_ext_portrait(Gfx());
}
#endif
