/* MOCK 2026-11-25 (rev CYD-15): two full-screen power-save typists, no chrome.
 * A = scrolling wrapped log. B = Format-Table, one table per hunt class.
 * Passive pre-scan: auto-typed commands and labels use the locked Defend
 * command language (verb + device [+ sub]; core scan/lock/zap/purge/restart;
 * BOTS[] / LV_HUNT_NAME short names; zap_try_cmd tags "gibson> …",
 * "[ .. ] SCAN NAME N LIVE", "[ -- ] NO NAME ON GRID"). Only the scan verb
 * is issued — this screen does not take input. Each find is listed by name
 * and device-bot class (LV_HUNT_BOT / short_name). SAMPLE data.
 * Visuals are guided by 1999 Matrix opening references (field, phosphor green,
 * type-out cadence, block cursor, glyph glow) but the language and layout
 * are BOTcorps, not a film replica. No film lines. Adv operator mock was
 * not recovered.
 * Enter A from TERM LIST (or psterm / tpsa). Enter B from tpsb / psterm-table.
 * Exit either by tapping anywhere (back to TERM). No on-screen controls. */
#pragma once
#if LV_TERM_PS

#define LV_TPS_PERIOD 120          /* power-save frame interval, ms */
#define LV_TPS_MS      80          /* typist ms/char — deliberate; see README */
#define LV_TPS_PAD     3           /* extra typist ticks at end of each line */
#define LV_TPS_X      14           /* left margin: sparse black field */
#define LV_TPS_Y      16           /* top margin */
#define LV_TPS_ROW    14           /* 7 px glyph + 7 px black */
#define LV_TPS_CW      6
#define LV_TPS_COLS   35           /* 14 + 35*6 = 224, 16 px right margin */
#define LV_TPS_MAXL   96
#define LV_TPS_LSZ    40
#define LV_TPS_DIM_BRI 20          /* backlight percent while this screen is up */

/* Frame-sampled / measured greens (comparison refs, not film frames shipped):
 * body #33e59a (terminal text, kirocrew 4K sample, hue ~154°);
 * glow / trail #00351d and field #000200 (Jonny Campbell rain/field samples);
 * fresh glyph #5FFFAF (MatrixHasYou closest 8-bit match);
 * block cursor #B2FFFE (kirocrew opening-frame cursor). */
static const uint16_t LV_TPS_BG    = lv_rgb(0, 2, 0);
static const uint16_t LV_TPS_INK   = lv_hex(0x33e59a);
static const uint16_t LV_TPS_GLOW  = lv_hex(0x00351d);
static const uint16_t LV_TPS_HOT   = lv_hex(0x5FFFAF);
static const uint16_t LV_TPS_CUR   = lv_hex(0xB2FFFE);

static uint32_t lv_tps_t0 = 0;
static int lv_tps_bri_saved = 80, lv_tps_bri_on = 0;
static char lv_tps_line[LV_TPS_MAXL][LV_TPS_LSZ];
static int lv_tps_n = 0, lv_tps_emit = 0, lv_tps_phase = 0;   /* 0 header, 1 body, 2 done */

static void lv_tps_add_raw(const char *s)
{
    if (lv_tps_n >= LV_TPS_MAXL) {
        memmove(lv_tps_line[0], lv_tps_line[1], (LV_TPS_MAXL - 1) * LV_TPS_LSZ);
        --lv_tps_n;
    }
    snprintf(lv_tps_line[lv_tps_n], LV_TPS_LSZ, "%s", s);
    ++lv_tps_n;
}

static void lv_tps_add(const char *fmt, ...)
{
    char buf[96];
    if (!fmt || !fmt[0]) { lv_tps_add_raw(""); return; }
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    const char *p = buf;
    while (*p) {
        int n = (int)strlen(p);
        if (n <= LV_TPS_COLS) { lv_tps_add_raw(p); break; }
        int k = LV_TPS_COLS;
        for (int i = LV_TPS_COLS; i > LV_TPS_COLS / 2; --i) if (p[i] == ' ') { k = i; break; }
        char cut[LV_TPS_LSZ]; memcpy(cut, p, (size_t)k); cut[k] = 0;
        lv_tps_add_raw(cut);
        p += k; if (*p == ' ') ++p;
    }
}

static int lv_tps_class(const LvFind &f)
{
    for (int h = 0; h < LV_NHUNT; ++h) if (lv_hunt_match(h, f)) return h;
    return -1;
}

static void lv_tps_header(void)
{
    /* Locked command-sheet core row (botcorps_ui.cpp: "core" = "scan lock zap purge restart"). */
    lv_tps_add("core  scan lock zap purge restart");
    lv_tps_add("SAMPLE. low power pre-scan.");
    lv_tps_add("");
}

/* Hunt-class order as Mobandy listed; names are locked LV_HUNT_NAME / BOTS[].short_name. */
static const int LV_TPS_ORD[LV_NHUNT] = {
    LV_H_AIRTAG, LV_H_KEYFOB, LV_H_DEAUTH, LV_H_META, LV_H_HACK,
    LV_H_SKIMMER, LV_H_FLOCK, LV_H_FLIPPER, LV_H_DRONE, LV_H_SAMSUNG
};

/* ---- A: scrolling typist log. Commands and tags from LOCK-DEFEND-CMDS (verb + device [+ sub]);
 * output labels from zap_try_cmd: "gibson> …", "[ .. ] SCAN NAME N LIVE", "[ -- ] NO NAME ON GRID".
 * Passive only: auto-typed verb is scan (list what's around). lock/zap/purge are not issued. ---- */
static void lv_tps_log_find_a(const LvFind &f, int h)
{
    char name[20], cls[16];
    lv_label(f, name, sizeof name);
    if (h >= 0) snprintf(cls, sizeof cls, "%s", LV_HUNT_BOT[h]);
    else snprintf(cls, sizeof cls, "%s", f.kind == LV_K_WIFI ? "wifi" : "bluetooth");
    if (f.kind == LV_K_WIFI && f.ch)
        lv_tps_add("%s  %s  %ddBm ch%d", name, cls, (int)f.rssi, (int)f.ch);
    else
        lv_tps_add("%s  %s  %ddBm", name, cls, (int)f.rssi);
}

static int lv_tps_count_class(int h, int kind)
{
    int n = 0;
    for (int i = 0; i < lv_nf; ++i) {
        if (kind >= 0 && lv_f[i].kind != kind) continue;
        if (h >= 0 && !lv_hunt_match(h, lv_f[i])) continue;
        if (h < 0 && lv_tps_class(lv_f[i]) >= 0) continue;   /* uncategorized only */
        ++n;
    }
    return n;
}

static void lv_tps_list_class_a(int h, int kind)
{
    for (int i = 0; i < lv_nf; ++i) {
        if (kind >= 0 && lv_f[i].kind != kind) continue;
        if (h >= 0 && !lv_hunt_match(h, lv_f[i])) continue;
        if (h < 0 && lv_tps_class(lv_f[i]) >= 0) continue;
        lv_tps_log_find_a(lv_f[i], h);
    }
}

/* ---- B: Format-Table layout (MS Learn: header + dash underline, column alignment),
 * issued with the same Defend `scan <device>` lines. Columns = name + device-bot class. ---- */
static void lv_tps_ft_head(void)
{
    char h[LV_TPS_LSZ], r[LV_TPS_LSZ];
    snprintf(h, sizeof h, "%-14s %-8s %4s %2s", "NAME", "CLASS", "RSSI", "CH");
    snprintf(r, sizeof r, "%-14s %-8s %4s %2s", "--------------", "--------", "----", "--");
    lv_tps_add_raw(h);
    lv_tps_add_raw(r);
}

static void lv_tps_ft_row(const char *name, const char *cls, const char *rssi, const char *ch)
{
    char name14[16], cls8[10];
    snprintf(name14, sizeof name14, "%s", name);
    if ((int)strlen(name14) > 14) { name14[11] = '.'; name14[12] = '.'; name14[13] = '.'; name14[14] = 0; }
    snprintf(cls8, sizeof cls8, "%s", cls);
    if ((int)strlen(cls8) > 8) { cls8[8] = 0; }
    char row[LV_TPS_LSZ];
    snprintf(row, sizeof row, "%-14s %-8s %4s %2s", name14, cls8, rssi, ch);
    lv_tps_add_raw(row);
}

static void lv_tps_ft_find(const LvFind &f, int h)
{
    char name[20], cls[16], rssi[8], ch[8];
    lv_label(f, name, sizeof name);
    if (h >= 0) snprintf(cls, sizeof cls, "%s", LV_HUNT_NAME[h]);
    else snprintf(cls, sizeof cls, "%s", f.kind == LV_K_WIFI ? "wifi" : "ble");
    snprintf(rssi, sizeof rssi, "%d", (int)f.rssi);
    if (f.kind == LV_K_WIFI && f.ch) snprintf(ch, sizeof ch, "%d", (int)f.ch); else ch[0] = 0;
    lv_tps_ft_row(name, cls, rssi, ch);
}

static void lv_tps_scan_banner(int h, int n)
{
    /* zap_try_cmd scan result: "[ .. ] SCAN NAME N LIVE" or "[ -- ] NO NAME ON GRID" */
    char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[h]); lv_upcase(up);
    if (n > 0) lv_tps_add("[ .. ] SCAN %s %d LIVE", up, n);
    else lv_tps_add("[ -- ] NO %s ON GRID", up);
}

static void lv_tps_build_a(void)
{
    /* Full rebuild each pump (typist is time-budgeted from t0). Radio banners
     * stay current as WIFI/BLE finds arrive; names are listed under each class. */
    lv_tps_n = 0;
    lv_tps_header();
    int nw = 0, nb = 0;
    for (int i = 0; i < lv_nf; ++i) { if (lv_f[i].kind == LV_K_WIFI) ++nw; else ++nb; }
    lv_tps_add("gibson> scan wifi");
    if (nw) lv_tps_add("[ .. ] SCAN WIFI %d LIVE", nw); else lv_tps_add("[ -- ] NO WIFI ON GRID");
    lv_tps_add("");
    lv_tps_add("gibson> scan bluetooth");
    if (nb) lv_tps_add("[ .. ] SCAN BLUETOOTH %d LIVE", nb); else lv_tps_add("[ -- ] NO BLUETOOTH ON GRID");
    lv_tps_add("");
    lv_tps_emit = 0;
    for (int i = 0; i < LV_NHUNT; ++i) {
        const int h = LV_TPS_ORD[i];
        const int n = lv_tps_count_class(h, -1);
        lv_tps_add("gibson> scan %s", LV_HUNT_NAME[h]);
        lv_tps_scan_banner(h, n);
        lv_tps_list_class_a(h, -1);
        lv_tps_add("");
        lv_tps_emit += n;
    }
    if (lv_job == LV_J_NONE && lv_nf > 0) lv_tps_add("gibson>");
}

static void lv_tps_build_b(void)
{
    lv_tps_n = 0;
    lv_tps_header();
    for (int i = 0; i < LV_NHUNT; ++i) {
        const int h = LV_TPS_ORD[i];
        const int n = lv_tps_count_class(h, -1);
        lv_tps_add("gibson> scan %s", LV_HUNT_NAME[h]);
        lv_tps_scan_banner(h, n);
        lv_tps_ft_head();
        if (n) {
            for (int k = 0; k < lv_nf; ++k) if (lv_hunt_match(h, lv_f[k])) lv_tps_ft_find(lv_f[k], h);
        }
        /* DEAUTH is not in the canned wifi/ble set — locked "NO … ON GRID", plus SAMPLE mark, no fake find */
        lv_tps_add("");
    }
    lv_tps_add("SAMPLE");
}

static void lv_tps_pump(void)
{
    if (lv_scr == LV_SC_TPSB) lv_tps_build_b();
    else lv_tps_build_a();
}

static int lv_tps_budget(void)
{
    int dt = (int)(lv_now - lv_tps_t0);
    return dt > 0 ? dt / LV_TPS_MS : 0;
}

static void lv_tps_text(LvBand &B, int x, int y, const char *s, int n, int hot)
{
    if (n <= 0 || !s[0]) return;
    char tmp[LV_TPS_LSZ];
    if (n >= LV_TPS_LSZ) n = LV_TPS_LSZ - 1;
    memcpy(tmp, s, (size_t)n); tmp[n] = 0;
    if (tmp[0]) b_text(B, x + 1, y, tmp, LV_TPS_GLOW);   /* 1 px phosphor bloom, trail green */
    b_text(B, x, y, tmp, hot ? LV_TPS_HOT : LV_TPS_INK);
    if (hot && n > 0 && ((lv_now / 90) & 1)) {           /* fresh glyph flicker, slightly hotter */
        char last[2] = { tmp[n - 1], 0 };
        b_text(B, x + (n - 1) * LV_TPS_CW, y, last, LV_TPS_CUR);
    }
}

static void lv_tps_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPS_BG);   /* #000200 field, no frame */
    lv_tps_pump();
    const int rows = (320 - LV_TPS_Y - 8) / LV_TPS_ROW;
    int budget = lv_tps_budget();
    int open = 0, remain = budget;
    for (int i = 0; i < lv_tps_n; ++i) {
        const int L = (int)strlen(lv_tps_line[i]);
        ++open;
        if (remain <= L) break;
        remain -= L + LV_TPS_PAD;
    }
    const int start = open > rows ? open - rows : 0;
    remain = budget;
    int shown = 0, cur_x = LV_TPS_X, cur_y = LV_TPS_Y, cutting = 0;
    for (int i = 0; i < lv_tps_n; ++i) {
        const int L = (int)strlen(lv_tps_line[i]);
        const int take = remain < L ? (remain < 0 ? 0 : remain) : L;
        remain -= L + LV_TPS_PAD;
        if (i < start) continue;
        if (shown >= rows) break;
        const int y = LV_TPS_Y + shown * LV_TPS_ROW;
        const int hot = (remain < 0 || i == open - 1);
        lv_tps_text(B, LV_TPS_X, y, lv_tps_line[i], take, hot);
        cur_x = LV_TPS_X + take * LV_TPS_CW;
        cur_y = y;
        cutting = remain < 0;
        ++shown;
        if (remain < 0) break;
    }
    /* film opening uses a pale block cursor (~#B2FFFE), slightly taller than the glyph */
    if ((lv_now / 500) & 1)
        b_fill(B, cur_x, cur_y, 6, 8, LV_TPS_CUR);
    (void)cutting;
}

/* ---- C & D: Matrix rain / dark-backdrop materialization (LV_SC_TPSC, LV_SC_TPSD) ----
 * Palette from 1999 Matrix opening refs (authentic.html + Jonny Campbell + kirocrew):
 * pure black #000000, stops #001F04 / #00351d / #00990B / #33e59a / #5FFFAF / #43FF33 / #E0FCD9. */
static const uint16_t LV_TPSC_BG    = lv_hex(0x000000);
static const uint16_t LV_TPSC_T0    = lv_hex(0x001F04);
static const uint16_t LV_TPSC_T1    = lv_hex(0x00351d);
static const uint16_t LV_TPSC_MID   = lv_hex(0x00990B);
static const uint16_t LV_TPSC_BODY  = lv_hex(0x33e59a);
static const uint16_t LV_TPSC_TAG   = lv_hex(0x5FFFAF);
static const uint16_t LV_TPSC_HOT   = lv_hex(0x43FF33);
static const uint16_t LV_TPSC_LEAD  = lv_hex(0xE0FCD9);

#define LV_TPSC_X     14
#define LV_TPSC_Y     16
#define LV_TPSC_CW     6
#define LV_TPSC_ROW   13           /* 7 px glyph + 6 px black gap -> 22 rows */
#define LV_TPSC_ROWS  22
#define LV_TPSC_NCOL  39           /* x = 2 + rc*6 .. 230; text col tc is rc = tc + 2 */

/* 28 Matrix-inspired 5x7 glyphs (column-major, bit0=top..bit6=bottom):
 * mirrored katakana silhouettes, 日, flipped 2/3/5/9, Z, +, *, :, =, |. */
static const uint8_t LV_TPS_RAIN_GL[28][5] = {
    {0x44,0x28,0x10,0x28,0x44}, /* X/katakana cross */
    {0x42,0x22,0x1E,0x02,0x40}, /* mirrored ﾊ/ﾇ */
    {0x40,0x44,0x7F,0x04,0x04}, /* mirrored ﾋ */
    {0x44,0x24,0x14,0x0B,0x00}, /* mirrored ｼ */
    {0x04,0x48,0x20,0x1F,0x04}, /* mirrored ﾂ */
    {0x06,0x49,0x29,0x1F,0x01}, /* mirrored ｳ */
    {0x04,0x44,0x3F,0x04,0x04}, /* mirrored ﾅ */
    {0x22,0x24,0x48,0x48,0x10}, /* mirrored ﾐ */
    {0x12,0x12,0x3F,0x12,0x42}, /* mirrored ﾓ */
    {0x22,0x22,0x22,0x22,0x22}, /* ﾆ bars */
    {0x08,0x3F,0x08,0x48,0x3F}, /* mirrored ｻ */
    {0x01,0x41,0x21,0x1F,0x01}, /* mirrored ﾜ */
    {0x04,0x24,0x5F,0x0C,0x04}, /* mirrored ｵ */
    {0x00,0x1F,0x00,0x40,0x3F}, /* mirrored ﾘ */
    {0x14,0x08,0x7F,0x08,0x14}, /* mirrored ﾎ */
    {0x02,0x46,0x2A,0x12,0x02}, /* mirrored ﾏ */
    {0x41,0x41,0x7F,0x41,0x41}, /* ｴ */
    {0x12,0x7F,0x12,0x24,0x04}, /* mirrored ｷ */
    {0x40,0x38,0x44,0x42,0x38}, /* mirrored ﾑ */
    {0x05,0x45,0x25,0x1D,0x05}, /* mirrored ﾃ */
    {0x04,0x44,0x3E,0x05,0x04}, /* mirrored ｹ */
    {0x42,0x24,0x18,0x24,0x42}, /* ﾒ */
    {0x40,0x3F,0x04,0x04,0x7E}, /* mirrored ｶ */
    {0x7F,0x49,0x49,0x49,0x7F}, /* 日 */
    {0x46,0x49,0x49,0x49,0x32}, /* flipped 2/5 */
    {0x61,0x59,0x49,0x4D,0x43}, /* Z */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x14,0x7F,0x14,0x00}, /* |+ */
};

static inline uint16_t lv_tpsc_max_rgb(uint16_t a, uint16_t b)
{
    uint16_t r = (a >> 11) > (b >> 11) ? (a & 0xF800) : (b & 0xF800);
    uint16_t g = ((a >> 5) & 63) > ((b >> 5) & 63) ? (a & 0x07E0) : (b & 0x07E0);
    uint16_t bl = (a & 31) > (b & 31) ? (a & 31) : (b & 31);
    return r | g | bl;
}

static inline void b_max_pix(LvBand &B, int x, int y, uint16_t c)
{
    if (x < 0 || x >= 240 || y < B.y0 || y >= B.y0 + B.h) return;
    uint16_t &dst = B.px[(y - B.y0) * B.w + x];
    dst = lv_tpsc_max_rgb(dst, c);
}

static void lv_tpsc_draw_cols(LvBand &B, int x, int y, const uint8_t col5[5], uint16_t fg, uint16_t g1, uint16_t g2)
{
    if (g2) {
        for (int c = 0; c < 5; ++c) {
            const uint8_t m = col5[c];
            for (int r = 0; r < 7; ++r) if (m & (1u << r)) {
                const int px = x + c, py = y + r;
                b_max_pix(B, px - 2, py, g2); b_max_pix(B, px + 2, py, g2);
                b_max_pix(B, px, py - 2, g2); b_max_pix(B, px, py + 2, g2);
                b_max_pix(B, px - 1, py - 1, g2); b_max_pix(B, px + 1, py - 1, g2);
                b_max_pix(B, px - 1, py + 1, g2); b_max_pix(B, px + 1, py + 1, g2);
            }
        }
    }
    if (g1) {
        for (int c = 0; c < 5; ++c) {
            const uint8_t m = col5[c];
            for (int r = 0; r < 7; ++r) if (m & (1u << r)) {
                const int px = x + c, py = y + r;
                b_max_pix(B, px - 1, py, g1); b_max_pix(B, px + 1, py, g1);
                b_max_pix(B, px, py - 1, g1); b_max_pix(B, px, py + 1, g1);
            }
        }
    }
    for (int c = 0; c < 5; ++c) {
        const uint8_t m = col5[c];
        for (int r = 0; r < 7; ++r) if (m & (1u << r)) b_max_pix(B, x + c, y + r, fg);
    }
}

static void lv_tpsc_draw_ascii(LvBand &B, int x, int y, char ch, uint16_t fg, uint16_t g1, uint16_t g2)
{
    unsigned uc = (unsigned char)ch;
    if (uc <= 32 || uc > 126) return;
    lv_tpsc_draw_cols(B, x, y, &LV_GLCD[(uc - 32) * 5], fg, g1, g2);
}

static void lv_tpsc_draw_rain(LvBand &B, int x, int y, uint32_t seed, int step_3, uint16_t fg, uint16_t g1, uint16_t g2)
{
    /* Matrix opening: selected cells swap glyphs every 3 frames, synchronized on step_3 */
    const uint32_t h = seed * 2654435761u;
    const uint32_t swap = ((h >> 11) % 3 == 0) ? (uint32_t)step_3 * 7u : 0u;
    const uint32_t idx = ((h >> 16) + swap) % 28u;
    lv_tpsc_draw_cols(B, x, y, LV_TPS_RAIN_GL[idx], fg, g1, g2);
}

static void lv_tpsc_build_lines(void)
{
    lv_tps_n = 0;
    lv_tps_header();

    int nw = 0, nb = 0;
    for (int i = 0; i < lv_nf; ++i) { if (lv_f[i].kind == LV_K_WIFI) ++nw; else ++nb; }

    /* List uncategorized wifi devices under locked BOTS[0] (WIFIbot) so everything around is named + typed */
    lv_tps_add("gibson> scan wifi");
    if (nw) lv_tps_add("[ .. ] SCAN WIFI %d LIVE", nw); else lv_tps_add("[ -- ] NO WIFI ON GRID");
    for (int i = 0; i < lv_nf; ++i) {
        if (lv_f[i].kind != LV_K_WIFI || lv_tps_class(lv_f[i]) >= 0) continue;
        char nm[20]; lv_label(lv_f[i], nm, sizeof nm);
        if (lv_f[i].ch) lv_tps_add("%s  WIFIbot  %ddBm ch%d", nm, (int)lv_f[i].rssi, (int)lv_f[i].ch);
        else lv_tps_add("%s  WIFIbot  %ddBm", nm, (int)lv_f[i].rssi);
    }
    lv_tps_add("");

    /* List uncategorized bluetooth devices under locked BOTS[1] (BLE / bluetooth) */
    lv_tps_add("gibson> scan bluetooth");
    if (nb) lv_tps_add("[ .. ] SCAN BLUETOOTH %d LIVE", nb); else lv_tps_add("[ -- ] NO BLUETOOTH ON GRID");
    for (int i = 0; i < lv_nf; ++i) {
        if (lv_f[i].kind != LV_K_BLE || lv_tps_class(lv_f[i]) >= 0) continue;
        char nm[20]; lv_label(lv_f[i], nm, sizeof nm);
        lv_tps_add("%s  BLEbot  %ddBm", nm, (int)lv_f[i].rssi);
    }
    lv_tps_add("");

    for (int i = 0; i < LV_NHUNT; ++i) {
        const int h = LV_TPS_ORD[i];
        const int n = lv_tps_count_class(h, -1);
        lv_tps_add("gibson> scan %s", LV_HUNT_NAME[h]);
        lv_tps_scan_banner(h, n);
        lv_tps_list_class_a(h, -1);
        lv_tps_add("");
    }
    if (lv_job == LV_J_NONE && lv_nf > 0) lv_tps_add("gibson>");
}

static bool lv_tpsc_is_cmd_line(const char *s)
{
    return !strncmp(s, "gibson>", 7) || !strncmp(s, "[ .. ]", 6) || !strncmp(s, "[ -- ]", 6) || !strncmp(s, "core ", 5);
}

static void lv_tps_cd_scene(LvBand &B, bool veil_mode)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* pure #000000 black */
    lv_tpsc_build_lines();

    const int rel = (int)(lv_now - lv_tps_t0);
    const int rain_step = rel > 0 ? rel / LV_TPS_PERIOD : 0;
    const int step_3 = rain_step / 3;

    /* Assign ominous line start times: line 0 at 500 ms, line 1 at 1500 ms, then +840 ms per non-blank line
     * so Wi-Fi (1.2s) and BLE (6.2s) finds are already in lv_f[] before their sections begin forming. */
    int t_line[LV_TPS_MAXL];
    int tc = 500;
    for (int i = 0; i < lv_tps_n; ++i) {
        t_line[i] = tc;
        if (lv_tps_line[i][0]) tc += (i == 0 ? 1000 : 860);
    }

    int open = 0;
    for (int i = 0; i < lv_tps_n; ++i) {
        if (rel >= t_line[i] - 720) open = i + 1;
    }
    const bool all_settled = (lv_tps_n > 0 && rel > t_line[lv_tps_n - 1] + 2200);
    int start = 0;
    if (all_settled) {
        start = lv_tps_n > LV_TPSC_ROWS ? lv_tps_n - LV_TPSC_ROWS : 0;
    } else if (open > LV_TPSC_ROWS - 4) {
        start = open - (LV_TPSC_ROWS - 4);
        if (start > lv_tps_n - LV_TPSC_ROWS && lv_tps_n > LV_TPSC_ROWS) start = lv_tps_n - LV_TPSC_ROWS;
        if (start < 0) start = 0;
    }

    /* Track occupied / active cells on the visible 39x22 grid so ambient rain never clobbers settled letters */
    uint8_t busy[LV_TPSC_ROWS][LV_TPSC_NCOL];
    memset(busy, 0, sizeof busy);

    for (int row = 0; row < LV_TPSC_ROWS; ++row) {
        const int li = start + row;
        if (li >= lv_tps_n) break;
        const char *s = lv_tps_line[li];
        const int L = (int)strlen(s);
        const int y = LV_TPSC_Y + row * LV_TPSC_ROW;
        const bool is_cmd = lv_tpsc_is_cmd_line(s);

        for (int c = 0; c < L && c < 35; ++c) {
            const int rc_sp = c + 2;
            /* Keep word spaces inside settled/forming lines clear of bright rain clutter */
            if (s[c] == ' ') {
                if (rel >= t_line[li] - 240) busy[row][rc_sp] = 1;
                continue;
            }
            const int rc = c + 2;
            const int x = LV_TPSC_X + c * LV_TPSC_CW;
            /* Organic column scatter so glyphs lock out of rain rather than a flat typewriter sweep */
            const int jit = ((li * 7 + c * 11) % 5) * 80;
            const int t_lock = t_line[li] + c * 32 + jit;
            const int dt = rel - t_lock;
            const uint32_t cseed = (uint32_t)(li * 131u + c * 37u + 9u);

            if (!veil_mode) {
                /* ---- Mock C: Rain-Lock Materialize ---- */
                if (dt < -840) continue;
                if (dt < 0) {
                    /* Carrier rain stream drops down column rc toward row */
                    const int dist = (-dt) / LV_TPS_PERIOD;   /* 0..7 rows above */
                    const int r_head = row - dist;
                    for (int k = 0; k < 6; ++k) {
                        const int rr = r_head - k;
                        if (rr < 0 || rr >= LV_TPSC_ROWS || busy[rr][rc]) continue;
                        if (start + rr < li && lv_tps_line[start + rr][0] && rel >= t_line[start + rr] - 360) continue;
                        if (rr == row && dt < -360) continue;
                        const int ry = LV_TPSC_Y + rr * LV_TPSC_ROW;
                        const uint16_t fg = k == 0 ? LV_TPSC_LEAD : k == 1 ? LV_TPSC_HOT : k <= 3 ? LV_TPSC_MID : LV_TPSC_T1;
                        const uint16_t g1 = k == 0 ? LV_TPSC_MID : k == 1 ? LV_TPSC_T1 : 0;
                        const uint16_t g2 = k == 0 ? LV_TPSC_T1 : 0;
                        lv_tpsc_draw_rain(B, x, ry, cseed + (uint32_t)(rr * 19 + rain_step), step_3 + rr, fg, g1, g2);
                    }
                    if (dt >= -360) {
                        busy[row][rc] = 1;
                        const uint16_t fg = dt >= -120 ? LV_TPSC_LEAD : dt >= -240 ? LV_TPSC_HOT : LV_TPSC_MID;
                        const uint16_t g1 = dt >= -240 ? LV_TPSC_MID : LV_TPSC_T1;
                        lv_tpsc_draw_rain(B, x, y, cseed + (uint32_t)(rain_step * 5), rain_step, fg, g1, dt >= -120 ? LV_TPSC_T1 : 0);
                    }
                } else {
                    busy[row][rc] = 1;
                    if (dt < 240) {
                        /* Bright white-green lock flare + faint rain drop passing beneath */
                        lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_LEAD, LV_TPSC_MID, LV_TPSC_T1);
                        const int sub = 1 + dt / 120;
                        for (int k = 0; k < 2; ++k) {
                            const int rr = row + sub + k;
                            if (rr < LV_TPSC_ROWS && !busy[rr][rc]) {
                                const int ry = LV_TPSC_Y + rr * LV_TPSC_ROW;
                                lv_tpsc_draw_rain(B, x, ry, cseed + (uint32_t)rr * 13u, step_3, k == 0 ? LV_TPSC_MID : LV_TPSC_T0, 0, 0);
                            }
                        }
                    } else if (dt < 600) {
                        lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_TAG, LV_TPSC_T1, 0);
                    } else {
                        lv_tpsc_draw_ascii(B, x, y, s[c], is_cmd ? LV_TPSC_TAG : LV_TPSC_BODY, LV_TPSC_T1, 0);
                    }
                }
            } else {
                /* ---- Mock D: Dark-Backdrop Emerge + Rain-Veil ---- */
                if (dt < -600) continue;
                busy[row][rc] = 1;
                if (dt < -360) {
                    lv_tpsc_draw_rain(B, x, y, cseed + (uint32_t)(rain_step * 3), rain_step, LV_TPSC_T0, 0, 0);
                } else if (dt < -120) {
                    const uint16_t fg = dt >= -240 ? LV_TPSC_MID : LV_TPSC_T1;
                    lv_tpsc_draw_rain(B, x, y, cseed + (uint32_t)(rain_step * 7), rain_step, fg, LV_TPSC_T0, 0);
                } else if (dt < 0) {
                    lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_MID, LV_TPSC_T1, 0);
                } else if (dt < 240) {
                    lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_LEAD, LV_TPSC_MID, LV_TPSC_T1);
                    if (row + 1 < LV_TPSC_ROWS && !busy[row + 1][rc])
                        lv_tpsc_draw_rain(B, x, y + LV_TPSC_ROW, cseed + 29u, step_3, LV_TPSC_T1, 0, 0);
                } else if (dt < 600) {
                    lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_TAG, LV_TPSC_T1, 0);
                } else {
                    lv_tpsc_draw_ascii(B, x, y, s[c], is_cmd ? LV_TPSC_TAG : LV_TPSC_BODY, LV_TPSC_T1, 0);
                }
            }
        }
    }

    /* Ambient falling code rain behind & below lines (Matrix 1999: sparse columns, invisible gaps, 1/4 bright leaders) */
    for (int rc = 0; rc < LV_TPSC_NCOL; ++rc) {
        const uint32_t s = 0x9E3779B9u ^ (uint32_t)(rc * 2654435761u);
        const int active_mod = veil_mode ? 5 : 3;
        if ((int)((s >> 4) % (uint32_t)active_mod) != 0) continue;   /* sparse black backdrop */
        const int speed = ((s >> 8) % 2) + 1;
        const int trail = (veil_mode ? 4 : 6) + (int)((s >> 11) % 6);
        const int period = LV_TPSC_ROWS + trail + 18 + (int)((s >> 15) % 14);
        const int off = (int)((s >> 19) % (uint32_t)period);
        const int head = (rain_step * speed + off) % period;
        const bool has_lead = !veil_mode && ((s >> 2) % 4 == 0);
        const int x = 2 + rc * LV_TPSC_CW;

        for (int t = 0; t < trail; ++t) {
            const int rr = head - t;
            if (rr < 0 || rr >= LV_TPSC_ROWS) continue;
            if (busy[rr][rc]) continue;   /* never overwrite settled/locking text */
            /* behind settled upper lines, keep ambient rain faint so text dominates cleanly */
            const bool below_open = (start + rr >= open);
            uint16_t fg, g1 = 0, g2 = 0;
            if (veil_mode) {
                fg = t == 0 ? LV_TPSC_T1 : LV_TPSC_T0;
            } else if (below_open) {
                fg = (t == 0 && has_lead) ? LV_TPSC_LEAD : t == 0 ? LV_TPSC_HOT : t <= 2 ? LV_TPSC_MID : t <= 5 ? LV_TPSC_T1 : LV_TPSC_T0;
                if (t == 0 && has_lead) { g1 = LV_TPSC_MID; g2 = LV_TPSC_T1; }
                else if (t == 0) g1 = LV_TPSC_T1;
            } else {
                fg = (t == 0 && has_lead) ? LV_TPSC_MID : t <= 1 ? LV_TPSC_T1 : LV_TPSC_T0;
            }
            const int ry = LV_TPSC_Y + rr * LV_TPSC_ROW;
            lv_tpsc_draw_rain(B, x, ry, s + (uint32_t)rr * 17u, step_3, fg, g1, g2);
        }
    }
}

static int lv_tps_full = 1;
static void lv_tps_cd_wrap(LvBand &B, void *)
{
    lv_tps_cd_scene(B, lv_scr == LV_SC_TPSD);
}

static void lv_tps_frame(void)
{
    /* first paint is a full push; later frames use dirty-row blit so only changed text rows hit the panel */
    const lv_scene_fn sc = (lv_scr == LV_SC_TPSC || lv_scr == LV_SC_TPSD) ? lv_tps_cd_wrap : lv_tps_scene;
    lv_render(lv_tps_full ? lv_blit : lv_blit_dirty, (LvRect){0, 0, 239, 319}, sc, 0);
    lv_tps_full = 0;
}

static void lv_tps_dim(int on)
{
#if LV_CB
    if (on && !lv_tps_bri_on) {
        lv_tps_bri_saved = (int)lv_cb_set.bri;
        if (lv_tps_bri_saved < 10 || lv_tps_bri_saved > 100) lv_tps_bri_saved = 80;
        lv_cb_blset(LV_TPS_DIM_BRI);
        lv_tps_bri_on = 1;
    } else if (!on && lv_tps_bri_on) {
        lv_cb_blset(lv_tps_bri_saved);
        lv_tps_bri_on = 0;
    }
#else
    (void)on;
#endif
}

static void lv_tps_enter(void)
{
    lv_tps_t0 = lv_now;
    lv_tps_n = 0;
    lv_tps_emit = 0;
    lv_tps_phase = 0;
    lv_tps_full = 1;
    lv_tps_dim(1);
    if (lv_job == LV_J_NONE) lv_job_start(LV_J_ALL, -1);   /* keep scanning while the typist runs */
}

static void lv_tps_leave(void)
{
    lv_tps_dim(0);
}

static const char *lv_tps_tap(int x, int y)
{
    (void)x; (void)y;
    return lv_back();   /* no chrome: any tap leaves */
}

#endif /* LV_TERM_PS */
