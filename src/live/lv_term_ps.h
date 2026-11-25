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

/* ---- E: B's Format-Table layout + C rain-lock materialize + D dark sparse backdrop ---- */
static void lv_tps_e_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* D: pure #000000 */
    lv_tps_build_b();                              /* B: per-class NAME/CLASS/RSSI/CH tables */

    const int rel = (int)(lv_now - lv_tps_t0);
    const int rain_step = rel > 0 ? rel / LV_TPS_PERIOD : 0;
    const int step_3 = rain_step / 3;

    /* Same slow ominous cadence as C/D so Wi-Fi (1.2s) / BLE (6.2s) finds exist before later tables form. */
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

    uint8_t busy[LV_TPSC_ROWS][LV_TPSC_NCOL];
    memset(busy, 0, sizeof busy);

    for (int row = 0; row < LV_TPSC_ROWS; ++row) {
        const int li = start + row;
        if (li >= lv_tps_n) break;
        const char *s = lv_tps_line[li];
        const int L = (int)strlen(s);
        const int y = LV_TPSC_Y + row * LV_TPSC_ROW;
        const bool is_cmd = lv_tpsc_is_cmd_line(s) || !strncmp(s, "NAME", 4) || s[0] == '-';

        for (int c = 0; c < L && c < 35; ++c) {
            const int rc = c + 2;
            if (s[c] == ' ') {
                if (rel >= t_line[li] - 240) busy[row][rc] = 1;
                continue;
            }
            const int x = LV_TPSC_X + c * LV_TPSC_CW;
            const int jit = ((li * 7 + c * 11) % 5) * 80;          /* C: column scatter */
            const int t_lock = t_line[li] + c * 32 + jit;
            const int dt = rel - t_lock;
            const uint32_t cseed = (uint32_t)(li * 131u + c * 37u + 9u);

            /* C rain-lock: carrier rain descends into the cell, locks with #E0FCD9 flare, cools to green */
            if (dt < -840) continue;
            if (dt < 0) {
                const int dist = (-dt) / LV_TPS_PERIOD;
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
                    lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_LEAD, LV_TPSC_MID, LV_TPSC_T1);
                    const int sub = 1 + dt / 120;
                    for (int k = 0; k < 2; ++k) {
                        const int rr = row + sub + k;
                        if (rr < LV_TPSC_ROWS && !busy[rr][rc]) {
                            const int ry = LV_TPSC_Y + rr * LV_TPSC_ROW;
                            /* drip under lock stays D-dark so the backdrop stays pitch-black */
                            lv_tpsc_draw_rain(B, x, ry, cseed + (uint32_t)rr * 13u, step_3, k == 0 ? LV_TPSC_T1 : LV_TPSC_T0, 0, 0);
                        }
                    }
                } else if (dt < 600) {
                    lv_tpsc_draw_ascii(B, x, y, s[c], LV_TPSC_TAG, LV_TPSC_T1, 0);
                } else {
                    lv_tpsc_draw_ascii(B, x, y, s[c], is_cmd ? LV_TPSC_TAG : LV_TPSC_BODY, LV_TPSC_T1, 0);
                }
            }
        }
    }

    /* D ambient: ~20% columns (mod 5), short trails, #001F04/#00351d decay, no bright leaders */
    for (int rc = 0; rc < LV_TPSC_NCOL; ++rc) {
        const uint32_t seed = 0x9E3779B9u ^ (uint32_t)(rc * 2654435761u);
        if ((int)((seed >> 4) % 5u) != 0) continue;
        const int speed = ((seed >> 8) % 2) + 1;
        const int trail = 4 + (int)((seed >> 11) % 6);
        const int period = LV_TPSC_ROWS + trail + 18 + (int)((seed >> 15) % 14);
        const int off = (int)((seed >> 19) % (uint32_t)period);
        const int head = (rain_step * speed + off) % period;
        const int x = 2 + rc * LV_TPSC_CW;
        for (int t = 0; t < trail; ++t) {
            const int rr = head - t;
            if (rr < 0 || rr >= LV_TPSC_ROWS) continue;
            if (busy[rr][rc]) continue;
            const uint16_t fg = t == 0 ? LV_TPSC_T1 : LV_TPSC_T0;
            const int ry = LV_TPSC_Y + rr * LV_TPSC_ROW;
            lv_tpsc_draw_rain(B, x, ry, seed + (uint32_t)rr * 17u, step_3, fg, 0, 0);
        }
    }
}

/* ---- F (Trace Program: scrambling digits lock into place) & G (Wake up, Neo: cursor typing) ----
 * Both fit EVERY detected device for all 10 classes + WIFI/BLE on ONE 240x320 screen (no scroll)
 * using 3x5 LV_F35 glyphs (4 px pitch = 56 cols, 8 px row pitch = 38 rows). No rain. */
#define LV_TPSF_X     8
#define LV_TPSF_Y     8
#define LV_TPSF_CW    4
#define LV_TPSF_ROW   8
#define LV_TPSF_ROWS 38
#define LV_TPSF_COLS 56

struct LvTpsFG {
    char line[44][60];
    uint8_t kind[44];   /* 0=header/cmd, 1=status, 2=device row, 3=no-grid */
    int n;
};

static void lv_tpsfg_fmt_row(char *out, size_t osz, const LvFind &f, const char *cls)
{
    char nm[24]; lv_label(f, nm, sizeof nm);
    if ((int)strlen(nm) > 22) { nm[19] = '.'; nm[20] = '.'; nm[21] = '.'; nm[22] = 0; }
    char rssi[8], ch[8];
    snprintf(rssi, sizeof rssi, "%d", (int)f.rssi);
    if (f.kind == LV_K_WIFI && f.ch) snprintf(ch, sizeof ch, "%d", (int)f.ch); else ch[0] = 0;
    snprintf(out, osz, "%-22s %-10s %4s %3s", nm, cls, rssi, ch);
}

static void lv_tpsfg_build(LvTpsFG &fg, bool is_trace, bool trace_done)
{
    fg.n = 0;
    auto push = [&](uint8_t k, const char *fmt, ...) {
        if (fg.n >= 44) return;
        va_list ap; va_start(ap, fmt);
        vsnprintf(fg.line[fg.n], sizeof fg.line[0], fmt, ap);
        va_end(ap);
        fg.kind[fg.n] = k;
        ++fg.n;
    };

    if (is_trace)
        push(0, "CALL OPT: RX  GIBSON> TRACE: %s [SAMPLE]", trace_done ? "COMPLETE" : "RUNNING ");
    else
        push(0, "GIBSON> SCAN ALL  [SAMPLE]");
    push(0, "CORE: SCAN LOCK ZAP PURGE RESTART");
    push(0, "NAME                   CLASS      RSSI  CH");
    push(0, "---------------------- ---------- ---- ---");

    int nw = 0, nb = 0;
    for (int i = 0; i < lv_nf; ++i) { if (lv_f[i].kind == LV_K_WIFI) ++nw; else ++nb; }

    if (nw) push(1, "GIBSON> SCAN WIFI      [ .. ] SCAN WIFI %d LIVE", nw);
    else    push(3, "GIBSON> SCAN WIFI      [ -- ] NO WIFI ON GRID");
    for (int i = 0; i < lv_nf; ++i) {
        if (lv_f[i].kind != LV_K_WIFI || lv_tps_class(lv_f[i]) >= 0) continue;
        char row[60]; lv_tpsfg_fmt_row(row, sizeof row, lv_f[i], "WIFIbot");
        push(2, "%s", row);
    }

    if (nb) push(1, "GIBSON> SCAN BLUETOOTH [ .. ] SCAN BLUETOOTH %d LIVE", nb);
    else    push(3, "GIBSON> SCAN BLUETOOTH [ -- ] NO BLUETOOTH ON GRID");
    for (int i = 0; i < lv_nf; ++i) {
        if (lv_f[i].kind != LV_K_BLE || lv_tps_class(lv_f[i]) >= 0) continue;
        char row[60]; lv_tpsfg_fmt_row(row, sizeof row, lv_f[i], "BLEbot");
        push(2, "%s", row);
    }

    for (int i = 0; i < LV_NHUNT; ++i) {
        const int h = LV_TPS_ORD[i];
        const int n = lv_tps_count_class(h, -1);
        char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[h]); lv_upcase(up);
        if (n > 0) push(1, "GIBSON> SCAN %-9s [ .. ] SCAN %s %d LIVE", up, up, n);
        else       push(3, "GIBSON> SCAN %-9s [ -- ] NO %s ON GRID", up, up);
        for (int k = 0; k < lv_nf; ++k) if (lv_hunt_match(h, lv_f[k])) {
            char row[60]; lv_tpsfg_fmt_row(row, sizeof row, lv_f[k], LV_HUNT_BOT[h]);
            push(2, "%s", row);
        }
    }
    push(0, "GIBSON>");
}

static inline void b_char35(LvBand &B, int x, int y, char ch, uint16_t c)
{
    int uc = (uint8_t)ch; if (uc < 0x20 || uc > 0x7e) uc = '?';
    if (uc == ' ' || y >= B.y0 + B.h || y + 5 <= B.y0) return;
    const uint16_t g = LV_F35[uc - 0x20];
    for (int k = 0; k < 15; ++k) if (g >> (14 - k) & 1) b_pix(B, x + k % 3, y + k / 3, c);
}

/* F: Matrix 1999 TRACE program — cycling digit columns snap & lock character-by-character */
static void lv_tps_f_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* pure #000000 CRT black */
    const int rel = (int)(lv_now - lv_tps_t0);
    const bool done = (rel >= 15200);
    LvTpsFG fg;
    lv_tpsfg_build(fg, true, done);

    const int cyc_tick = rel > 0 ? rel / 90 : 0;   /* rapid slot-machine digit spin */
    for (int r = 0; r < fg.n && r < LV_TPSF_ROWS; ++r) {
        const char *s = fg.line[r];
        const int L = (int)strlen(s);
        const int y = LV_TPSF_Y + r * LV_TPSF_ROW;
        /* Header 0..3 locks first (0..900ms); Wi-Fi section rows 4..10 lock after 1.5s;
         * BLE + hunt sections (11+) hold spinning digits until BLE arrives at 6.2s, then lock 6.5..14.8s. */
        int r_base;
        if (r <= 3) r_base = 120 + r * 200;
        else if (r <= 10) r_base = 1600 + (r - 4) * 320;
        else r_base = 6500 + (r - 11) * 290;

        for (int c = 0; c < L && c < LV_TPSF_COLS; ++c) {
            if (s[c] == ' ') continue;
            const int x = LV_TPSF_X + c * LV_TPSF_CW;
            /* Digits / hex / values lock slightly staggered like the 10-digit trace wheels */
            const int t_lock = r_base + c * 18 + ((r * 5 + c * 3) % 4) * 35;
            const int dt = rel - t_lock;
            if (dt < 0) {
                /* Scrambling digits ('0'..'9') cycling continuously before lock */
                if (rel < r_base - 4800 && r > 10) {
                    /* Still show ambient cycling trace digits in all active trace columns */
                }
                const uint32_t h = (uint32_t)(cyc_tick * 131u + r * 37u + c * 17u);
                const char dig = (char)('0' + (h % 10u));
                const uint16_t col = ((cyc_tick + r + c) % 5 == 0) ? LV_TPSC_HOT :
                                     ((cyc_tick + c) % 2 == 0) ? LV_TPSC_MID : LV_TPSC_T1;
                b_char35(B, x, y, dig, col);
            } else if (dt < 180) {
                b_char35(B, x, y, s[c], LV_TPSC_LEAD);   /* bright lock snap */
            } else if (dt < 420) {
                b_char35(B, x, y, s[c], LV_TPSC_TAG);
            } else {
                const uint16_t col = (fg.kind[r] == 0 || fg.kind[r] == 1) ? LV_TPSC_TAG :
                                     (fg.kind[r] == 3) ? LV_TPSC_MID : LV_TPSC_BODY;
                b_char35(B, x, y, s[c], col);
            }
        }
    }
}

/* G: Matrix 1999 "Wake up, Neo..." — black CRT, green text typed at blinking block cursor, deliberate pauses */
static void lv_tps_g_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* pure #000000 CRT black */
    const int rel = (int)(lv_now - lv_tps_t0);
    LvTpsFG fg;
    lv_tpsfg_build(fg, false, false);

    /* Compute per-line start/end times with deliberate pauses after commands/headers */
    int t_cur = 500;   /* initial 500 ms cursor blink on empty black screen */
    int cur_x = LV_TPSF_X, cur_y = LV_TPSF_Y;
    bool typing_active = false;

    for (int r = 0; r < fg.n && r < LV_TPSF_ROWS; ++r) {
        const char *s = fg.line[r];
        const int L = (int)strlen(s);
        const int y = LV_TPSF_Y + r * LV_TPSF_ROW;
        /* Slower operator pace on commands (65ms/char), brisker on table data (22ms/char) so all 38 rows fit cleanly */
        const int ms_per = (r == 0 || fg.kind[r] != 2) ? 32 : 16;
        const int pause_after = (r == 0) ? 700 : (r == 3) ? 450 : (fg.kind[r] == 1 || fg.kind[r] == 3) ? 320 : 80;

        if (rel < t_cur) break;
        const int line_dt = rel - t_cur;
        int chars_shown = 0;
        int acc = 0;
        for (int c = 0; c < L && c < LV_TPSF_COLS; ++c) {
            /* Subtle human cadence jitter */
            const int step = ms_per + ((r * 7 + c * 11) % 3) * (ms_per / 3);
            if (line_dt >= acc) chars_shown = c + 1;
            acc += step;
        }
        if (line_dt < acc) typing_active = true;

        for (int c = 0; c < chars_shown && c < L && c < LV_TPSF_COLS; ++c) {
            const int x = LV_TPSF_X + c * LV_TPSF_CW;
            const bool fresh = (c == chars_shown - 1) && (line_dt < acc);
            const uint16_t col = fresh ? LV_TPSC_LEAD :
                                 (fg.kind[r] == 0 || fg.kind[r] == 1) ? LV_TPSC_TAG :
                                 (fg.kind[r] == 3) ? LV_TPSC_MID : LV_TPSC_BODY;
            b_char35(B, x, y, s[c], col);
        }
        cur_x = LV_TPSF_X + chars_shown * LV_TPSF_CW;
        if (cur_x > LV_TPSF_X + (LV_TPSF_COLS - 1) * LV_TPSF_CW)
            cur_x = LV_TPSF_X + (LV_TPSF_COLS - 1) * LV_TPSF_CW;
        cur_y = y;

        t_cur += acc + pause_after;
        if (rel < t_cur) break;
    }

    /* Blinking pale block cursor (#B2FFFE, solid while actively typing, blinking when paused) */
    if (typing_active || ((lv_now / 360) & 1))
        b_fill(B, cur_x, cur_y - 1, 4, 7, LV_TPS_CUR);
}

/* ---- F2 (Trace 5x7 mixed-case) & G2 (Wake up Neo 5x7 mixed-case) ----
 * Solves 3x5 size & case loss: uses 5x7 LV_GLCD mixed-case ("WIFIbot", "DRONEbot"),
 * dedupes TELLO to its specific hunt class (DRONEbot), merges status headers to 2 lines
 * ([ .. ] SCAN WIFI 9 LIVE  BLE 11 LIVE + [ -- ] NO DEAUTH ON GRID), and fits all 20
 * deduped devices across all 10 classes + WIFI/BLE on ONE 240x320 screen (27 rows at
 * 11 px pitch, 38 cols at 6 px pitch, no scroll).
 * Overflow rule: <=20 dev -> 5x7 11px pitch; 21..24 -> 5x7 10px pitch; 25..31 -> auto-switch
 * to 3x5 LV_F35 8px pitch; >31 -> strongest 31 devices (by RSSI) + "+K more" row. */
#define LV_TPS2_X     6
#define LV_TPS2_CW    6
#define LV_TPS2_COLS 38
#define LV_TPS2_MAXR 40

struct LvTps2Model {
    char line[LV_TPS2_MAXR][60];
    uint8_t kind[LV_TPS2_MAXR];   /* 0=cmd/hdr, 1=live-status, 2=device, 3=no-grid, 4=overflow */
    uint8_t grp[LV_TPS2_MAXR];    /* group break index for G2 pauses */
    int n;
    int y0, row_h;
    bool dense35;                 /* true only if overflow >24 devices triggers 3x5 fallback */
};

/* Primary class index per find: check specific hunt classes first (DRONE before HACK so
 * TELLO-5A21B3 resolves cleanly once to DRONEbot instead of duplicating under HACKbot). */
static int lv_tps2_primary_hunt(const LvFind &f)
{
    static const int PRI[LV_NHUNT] = {
        LV_H_AIRTAG, LV_H_KEYFOB, LV_H_DEAUTH, LV_H_META, LV_H_SKIMMER,
        LV_H_FLOCK, LV_H_FLIPPER, LV_H_DRONE, LV_H_SAMSUNG, LV_H_HACK
    };
    for (int i = 0; i < LV_NHUNT; ++i) {
        if (lv_hunt_match(PRI[i], f)) return PRI[i];
    }
    return -1;
}

static void lv_tps2_fmt_row(char *out, size_t osz, const LvFind &f, const char *cls, bool dense35)
{
    const int nw = dense35 ? 22 : 16;
    char nm[26]; lv_label(f, nm, sizeof nm);
    if ((int)strlen(nm) > nw) { nm[nw - 3] = '.'; nm[nw - 2] = '.'; nm[nw - 1] = '.'; nm[nw] = 0; }
    char rssi[8], ch[8];
    snprintf(rssi, sizeof rssi, "%d", (int)f.rssi);
    if (f.kind == LV_K_WIFI && f.ch) snprintf(ch, sizeof ch, "%d", (int)f.ch); else ch[0] = 0;
    if (dense35) snprintf(out, osz, "%-22s %-10s %4s %3s", nm, cls, rssi, ch);
    else         snprintf(out, osz, "%-16s %-10s %4s %3s", nm, cls, rssi, ch);
}

static void lv_tps2_build(LvTps2Model &m, bool is_trace, bool trace_done)
{
    struct Item { int idx; int cls; uint8_t rank; };
    Item items[LV_MAXF];
    int n_dev = 0;
    int nw = 0, nb = 0;

    static const int ORD2[LV_NHUNT] = {
        LV_H_AIRTAG, LV_H_KEYFOB, LV_H_META, LV_H_HACK, LV_H_SKIMMER,
        LV_H_FLOCK, LV_H_FLIPPER, LV_H_DRONE, LV_H_SAMSUNG, LV_H_DEAUTH
    };

    for (int i = 0; i < lv_nf; ++i) {
        if (lv_f[i].kind == LV_K_WIFI) ++nw; else ++nb;
        const int ph = lv_tps2_primary_hunt(lv_f[i]);
        uint8_t rk = 10;
        if (ph >= 0) {
            for (int j = 0; j < LV_NHUNT; ++j) if (ORD2[j] == ph) { rk = (uint8_t)j; break; }
        } else {
            rk = (lv_f[i].kind == LV_K_WIFI) ? 10 : 11;
        }
        items[n_dev++] = { i, ph, rk };
    }

    /* Overflow tiering:
     * <= 20 devices -> 5x7 LV_GLCD, 11 px pitch (27 rows, generous breathing room)
     * 21..24 devices -> 5x7 LV_GLCD, 10 px pitch (up to 31 rows)
     * 25..31 devices -> auto-switch to 3x5 LV_F35, 8 px pitch (up to 38 rows)
     * > 31 devices   -> 3x5 LV_F35, strongest 31 by RSSI + "+K more" row */
    m.dense35 = (n_dev > 24);
    int cap = m.dense35 ? 31 : (n_dev <= 20 ? 20 : 24);
    int extra = 0;
    if (n_dev > cap) {
        /* Sort by strongest RSSI descending so the +K more overflow keeps strongest signals */
        for (int a = 0; a < n_dev; ++a)
            for (int b = a + 1; b < n_dev; ++b)
                if (lv_f[items[b].idx].rssi > lv_f[items[a].idx].rssi) {
                    Item tmp = items[a]; items[a] = items[b]; items[b] = tmp;
                }
        extra = n_dev - (cap - 1);   /* reserve 1 row for "+K more" */
        n_dev = cap - 1;
    }
    /* Group visible devices cleanly by bot class order */
    for (int a = 0; a < n_dev; ++a)
        for (int b = a + 1; b < n_dev; ++b)
            if (items[b].rank < items[a].rank || (items[b].rank == items[a].rank && items[b].idx < items[a].idx)) {
                Item tmp = items[a]; items[a] = items[b]; items[b] = tmp;
            }

    m.y0 = m.dense35 ? 8 : (n_dev <= 20 ? 10 : 6);
    m.row_h = m.dense35 ? 8 : (n_dev <= 20 ? 11 : 10);
    m.n = 0;

    auto push = [&](uint8_t k, uint8_t g, const char *fmt, ...) {
        if (m.n >= LV_TPS2_MAXR) return;
        va_list ap; va_start(ap, fmt);
        vsnprintf(m.line[m.n], sizeof m.line[0], fmt, ap);
        va_end(ap);
        m.kind[m.n] = k; m.grp[m.n] = g;
        ++m.n;
    };

    if (is_trace)
        push(0, 0, "CALL OPT:RX gibson> trace:%s SAMPLE", trace_done ? "done" : "run ");
    else
        push(0, 0, "gibson> scan all               SAMPLE");
    push(0, 1, "core: scan lock zap purge restart");
    push(1, 2, "[ .. ] SCAN WIFI %d LIVE  BLE %d LIVE", nw, nb);
    push(3, 3, "[ -- ] NO DEAUTH ON GRID");
    if (m.dense35) {
        push(0, 3, "NAME                   CLASS      RSSI  CH");
        push(0, 3, "---------------------- ---------- ---- ---");
    } else {
        push(0, 3, "NAME             CLASS      RSSI  CH");
        push(0, 3, "---------------- ---------- ---- ---");
    }

    for (int i = 0; i < n_dev; ++i) {
        const LvFind &f = lv_f[items[i].idx];
        const char *cls = items[i].cls >= 0 ? LV_HUNT_BOT[items[i].cls]
                        : (f.kind == LV_K_WIFI ? "WIFIbot" : "BLEbot");
        char row[60];
        lv_tps2_fmt_row(row, sizeof row, f, cls, m.dense35);
        push(2, (uint8_t)(4 + items[i].rank), "%s", row);
    }
    if (extra > 0) push(4, 20, "+%d more (strongest %d shown)", extra, n_dev);
    push(0, 21, "gibson>");
}

/* CRT scanline + subtle phosphor bloom for 5x7 LV_GLCD (only lights when pixel > 0; pitch-black bg) */
static inline uint16_t lv_tps2_scan_mod(uint16_t c, int y)
{
    return (y & 1) ? lv_dim(c, 7, 8) : c;
}

static void lv_tps2_draw_char(LvBand &B, int x, int y, char ch, uint16_t fg, uint16_t g1, uint16_t g2, bool dense35)
{
    if (dense35) { b_char35(B, x, y, ch, lv_tps2_scan_mod(fg, y)); return; }
    unsigned uc = (unsigned char)ch;
    if (uc <= 32 || uc > 126) return;
    const uint8_t *col5 = &LV_GLCD[(uc - 32) * 5];
    if (g2) {
        for (int c = 0; c < 5; ++c) {
            const uint8_t m = col5[c];
            for (int r = 0; r < 7; ++r) if (m & (1u << r)) {
                const int px = x + c, py = y + r;
                const uint16_t gm = lv_tps2_scan_mod(g2, py);
                b_max_pix(B, px - 1, py - 1, gm); b_max_pix(B, px + 1, py - 1, gm);
                b_max_pix(B, px - 1, py + 1, gm); b_max_pix(B, px + 1, py + 1, gm);
            }
        }
    }
    if (g1) {
        for (int c = 0; c < 5; ++c) {
            const uint8_t m = col5[c];
            for (int r = 0; r < 7; ++r) if (m & (1u << r)) {
                const int px = x + c, py = y + r;
                b_max_pix(B, px - 1, py, lv_tps2_scan_mod(g1, py));
                b_max_pix(B, px + 1, py, lv_tps2_scan_mod(g1, py));
                b_max_pix(B, px, py - 1, lv_tps2_scan_mod(g1, py - 1));
                b_max_pix(B, px, py + 1, lv_tps2_scan_mod(g1, py + 1));
            }
        }
    }
    for (int c = 0; c < 5; ++c) {
        const uint8_t m = col5[c];
        for (int r = 0; r < 7; ++r) if (m & (1u << r))
            b_max_pix(B, x + c, y + r, lv_tps2_scan_mod(fg, y + r));
    }
}

/* F2: Refined 5x7 Matrix Trace screen — vertical pulsing green digit streams snap & lock per cell */
static void lv_tps_f2_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* pure #000000 CRT black */
    /* Ultra-faint 1-step green CRT raster only on even scanlines inside active trace window? Keep pure #000000
     * background for maximum contrast; scanline modulation sits on every lit phosphor & bloom pixel. */
    const int rel = (int)(lv_now - lv_tps_t0);
    const bool done = (rel >= 15600);
    LvTps2Model m;
    lv_tps2_build(m, true, done);

    const int cw = m.dense35 ? 4 : LV_TPS2_CW;
    const int maxc = m.dense35 ? 56 : LV_TPS2_COLS;
    const int cyc_tick = rel > 0 ? rel / 90 : 0;

    for (int r = 0; r < m.n; ++r) {
        const char *s = m.line[r];
        const int L = (int)strlen(s);
        const int y = m.y0 + r * m.row_h;
        int r_base;
        if (r <= 1) r_base = 160 + r * 260;
        else if (r <= 5) r_base = 1500 + (r - 2) * 300;
        else r_base = 6500 + (r - 6) * 410;   /* BLE arrives at 6.2s; locks rows 6..26 smoothly to ~15s */

        for (int c = 0; c < L && c < maxc; ++c) {
            if (s[c] == ' ') continue;
            const int x = LV_TPS2_X + c * cw;
            const int t_lock = r_base + c * 22 + ((r * 5 + c * 3) % 4) * 45;
            const int dt = rel - t_lock;
            if (dt < 0) {
                /* Racing vertical pulse of digits ('0'..'9') with bright leading pulse wave */
                const uint32_t h = (uint32_t)(cyc_tick * 131u + r * 37u + c * 17u);
                const char dig = (char)('0' + (h % 10u));
                const int wave = (cyc_tick + r + ((c * 3) % 7)) % 7;
                const uint16_t fg = (wave == 0) ? LV_TPSC_HOT : (wave <= 2) ? LV_TPSC_MID : LV_TPSC_T1;
                const uint16_t g1 = (wave == 0) ? LV_TPSC_T1 : 0;
                lv_tps2_draw_char(B, x, y, dig, fg, g1, 0, m.dense35);
            } else if (dt < 240) {
                /* Slot-machine snap flare (#E0FCD9 + #00990B/#00351d bloom) */
                lv_tps2_draw_char(B, x, y, s[c], LV_TPSC_LEAD, LV_TPSC_MID, LV_TPSC_T1, m.dense35);
            } else if (dt < 520) {
                lv_tps2_draw_char(B, x, y, s[c], LV_TPSC_TAG, LV_TPSC_T1, 0, m.dense35);
            } else {
                const uint16_t col = (m.kind[r] == 0 || m.kind[r] == 1) ? LV_TPSC_TAG :
                                     (m.kind[r] == 3) ? LV_TPSC_MID :
                                     (m.kind[r] == 4) ? LV_TPSC_HOT : LV_TPSC_BODY;
                lv_tps2_draw_char(B, x, y, s[c], col, LV_TPSC_T1, 0, m.dense35);
            }
        }
    }
}

/* G2: Refined 5x7 "Wake up, Neo..." terminal — blinking #B2FFFE block cursor, real pauses, CRT bloom */
static void lv_tps_g2_scene(LvBand &B, void *)
{
    b_fill(B, B.x0, B.y0, B.w, B.h, LV_TPSC_BG);   /* pure #000000 CRT black */
    const int rel = (int)(lv_now - lv_tps_t0);
    LvTps2Model m;
    lv_tps2_build(m, false, false);

    const int cw = m.dense35 ? 4 : LV_TPS2_CW;
    const int maxc = m.dense35 ? 56 : LV_TPS2_COLS;

    int t_cur = 700;   /* ominous initial pause on empty black with pulsing cursor */
    int cur_x = LV_TPS2_X, cur_y = m.y0;
    bool typing_active = false;

    for (int r = 0; r < m.n; ++r) {
        const char *s = m.line[r];
        const int L = (int)strlen(s);
        const int y = m.y0 + r * m.row_h;
        const int ms_per = (r == 0) ? 55 : (m.kind[r] != 2) ? 34 : 20;
        const bool grp_end = (r + 1 < m.n && m.grp[r + 1] != m.grp[r]);
        const int pause_after = (r == 0) ? 850 : (r == 1) ? 450 : (m.kind[r] == 1 || m.kind[r] == 3) ? 420 : grp_end ? 220 : 70;

        if (rel < t_cur) break;
        const int line_dt = rel - t_cur;
        int chars_shown = 0, acc = 0;
        for (int c = 0; c < L && c < maxc; ++c) {
            const int step = ms_per + ((r * 7 + c * 11) % 3) * (ms_per / 3);
            if (line_dt >= acc) chars_shown = c + 1;
            acc += step;
        }
        if (line_dt < acc) typing_active = true;

        for (int c = 0; c < chars_shown && c < L && c < maxc; ++c) {
            const int x = LV_TPS2_X + c * cw;
            const bool fresh = (c == chars_shown - 1) && (line_dt < acc);
            const uint16_t col = fresh ? LV_TPSC_LEAD :
                                 (m.kind[r] == 0 || m.kind[r] == 1) ? LV_TPSC_TAG :
                                 (m.kind[r] == 3) ? LV_TPSC_MID :
                                 (m.kind[r] == 4) ? LV_TPSC_HOT : LV_TPSC_BODY;
            lv_tps2_draw_char(B, x, y, s[c], col, fresh ? LV_TPSC_MID : LV_TPSC_T1, 0, m.dense35);
        }
        cur_x = LV_TPS2_X + chars_shown * cw;
        if (cur_x > LV_TPS2_X + (maxc - 1) * cw) cur_x = LV_TPS2_X + (maxc - 1) * cw;
        cur_y = y;

        t_cur += acc + pause_after;
        if (rel < t_cur) break;
    }

    /* Film's pale teal-white block cursor #B2FFFE (~1.14:1 tall on 5x7 glyph) */
    if (typing_active || ((lv_now / 420) & 1)) {
        const int bw = m.dense35 ? 4 : 6, bh = m.dense35 ? 7 : 8;
        b_fill(B, cur_x, cur_y, bw, bh, LV_TPS_CUR);
    }
}

static void lv_tps_frame(void)
{
    /* first paint is a full push; later frames use dirty-row blit so only changed text rows hit the panel */
    const lv_scene_fn sc = (lv_scr == LV_SC_TPSF2) ? lv_tps_f2_scene
                        : (lv_scr == LV_SC_TPSG2) ? lv_tps_g2_scene
                        : (lv_scr == LV_SC_TPSF) ? lv_tps_f_scene
                        : (lv_scr == LV_SC_TPSG) ? lv_tps_g_scene
                        : (lv_scr == LV_SC_TPSE) ? lv_tps_e_scene
                        : (lv_scr == LV_SC_TPSC || lv_scr == LV_SC_TPSD) ? lv_tps_cd_wrap
                        : lv_tps_scene;
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
