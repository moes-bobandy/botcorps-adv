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
#define LV_TPS_MAXL   64
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

static int lv_tps_full = 1;
static void lv_tps_frame(void)
{
    /* first paint is a full push; later frames use dirty-row blit so only changed text rows hit the panel */
    lv_render(lv_tps_full ? lv_blit : lv_blit_dirty, (LvRect){0, 0, 239, 319}, lv_tps_scene, 0);
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
