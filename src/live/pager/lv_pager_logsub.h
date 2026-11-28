/* v1.2.26 FIRMWARE (was the CYD-27 mock 2026-11-28; on in env cyd_livescan with LV_CYD27_LOGSUB=1 LV_CYD27_LOGSUB_B=1): LOGbot SUBSCREEN = "live log plus a device health strip (SD, touch, heap, uptime)".
 * Off by default: LV_CYD27_LOGSUB is 0, so firmware and v1.2.25 renders are byte-identical (every hook below compiles away).
 * Opened by tapping the LOGbot face on the LOGbot pager page; the existing banner Stripe-Slot back (lv_banner_back_draw, LV_BANNER_BACK_LS)
 * returns to that page. The pager arrow chips, pips and swipe are not on a sub-screen, so they are off here.
 * Reused verbatim: board still, LOGbot chip face + rain (110x110 art, oil-slick finish), log plate frame + SCAN/SYS tabs + EX.DATA tag
 * (lv_lb_render in lv_pager_log19.h), the log row text generator (lv_lb_row) and the F2 theme-ink roles (lv_lb_ink / lv_c17_role_px).
 * Added: (1) the log rows now scroll: a new row arrives every 1.8 s and the rows slide up 9 px (270 ms ease), the arriving row flares for
 * 240 ms, rows clip to the plate; (2) the health strip, which replaces the coil band at y34..69: four cells SD / TOUCH / HEAP / UPTIME.
 * Every glyph is theme ink per pixel (lv_theme_at), never hardcoded; no decorative bars, no GPIO/NVS text.
 * v1.2.26: variant B ships. No sample data: the log is the lv_log.h ring, SD / HEAP come from lv_ui.cpp hooks, UPTIME is millis(), TOUCH the last tap. */
#pragma once
#if LV_CYD27_LOGSUB
#if !LV_CYD19_LOGBOT
#error "LV_CYD27_LOGSUB needs LV_CYD19_LOGBOT"
#endif
static int lv_ls_tx = -1, lv_ls_ty = -1; static uint32_t lv_ls_t0 = 0, lv_ls_rkey = 0xFFFFFFFFu;
#if LV_CYD27_LOGSUB_B
/* ---- CYD-27 variant B (2026-11-28): the EXACT face rect (65,88)-(174,197) is the live log window.
 * The black chip frame / bezel and the board around it stay. The LOGbot face is removed on this
 * subscreen only (the pager page still shows it until the tap). The bottom 152x71 plate becomes
 * uniform sub-screen chips: row 1 ALL/WIFI/BLE/SYS, row 2 PAUSE|LIVE, CLEAR, SAVE (writes SAMPLE
 * data to SD /logs). Ink is live theme accent, never hardcoded. */
#ifndef LV_LSB_NROW
#define LV_LSB_NROW 12
#endif
#define LV_LSB_ROW 8          /* 7 px glyph + 1 px leading: 12 rows fit in 110-10 = 100 px with 5 px pad */
#define LV_LSB_X0  (LV_PG_FX_X0 + 4)
#define LV_LSB_Y0  (LV_PG_FX_Y0 + 3)
#define LV_LSB_X1  (LV_PG_FX_X0 + LV_PG_FX_S - 5)
#define LV_LSB_Y1  (LV_PG_FX_Y0 + LV_PG_FX_S - 4)
enum { LV_LSB_ALL = 0, LV_LSB_WIFI, LV_LSB_BLE, LV_LSB_SYS };
static int lv_lsb_filt = LV_LSB_ALL;
static int lv_lsb_paused = 0;
static uint32_t lv_lsb_view = 0;        /* v1.2.26: PAUSE freezes the view at this lv_log_seq */
static uint32_t lv_lsb_from = 0;        /* v1.2.26: CLEAR hides every entry below this lv_log_seq */
static uint32_t lv_lsb_flash_t = 0;     /* press-highlight until this time */
static int lv_lsb_flash_id = -1;        /* 0..3 filters, 4 PAUSE, 5 CLEAR, 6 SAVE */
static uint32_t lv_lsb_save_t = 0;      /* SAVE label reads SAVED / SD ERR for 1.2 s */
static int lv_lsb_save_ok = 0;          /* last SAVE: lines written (> 0), 0 nothing to save, -1 SD error */
/* health: SD is read once just after entry (mount is ~ms, off the flip-in frame) and after SAVE */
static int lv_lsb_sd = -2;              /* -2 not read yet, -1 no hook, 0 no card, 1 mounted */
static uint32_t lv_lsb_sd_free = 0, lv_lsb_sd_size = 0;
static uint32_t lv_lsb_tap_t = 0;       /* lv_now of the last tap on the subscreen */
#endif
#define LV_LS_ARR_MS 1800
#define LV_LS_SLIDE_MS 270
#define LV_LS_CLIP_Y0 236
#define LV_LS_CLIP_Y1 287
static inline bool lv_ls_active(void) { return lv_ls_on && lv_scr == LV_SC_HOME && lv_pg_page == LV_PG_LOGBOT; }
static void lv_ls_enter(void) {
    lv_ls_on = true; lv_ls_t0 = lv_now; lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
#if LV_CYD27_LOGSUB_B
    lv_lsb_filt = LV_LSB_ALL; lv_lsb_paused = 0; lv_lsb_view = 0;
    lv_lsb_flash_t = 0; lv_lsb_flash_id = -1; lv_lsb_save_t = 0; lv_lsb_save_ok = 0; lv_lsb_sd = -2;
#endif
}
static void lv_ls_leave(void) { lv_ls_on = false; lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true; }
static int lv_ls_arrivals(void) { return (int)((uint32_t)(lv_now - lv_ls_t0) / LV_LS_ARR_MS); }
static int lv_ls_phase(void) { return (int)((uint32_t)(lv_now - lv_ls_t0) % LV_LS_ARR_MS); }
/* px the rows still have to travel up (9 -> 0 over the slide) */
static int lv_ls_off(void)
{
    const int p = lv_ls_phase(); if (p >= LV_LS_SLIDE_MS || lv_ls_arrivals() == 0) return 0;
    const float t = (float)p / (float)LV_LS_SLIDE_MS, e = t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) / 2.f;
    return (int)lroundf(9.f * (1.f - e));
}
#if LV_CYD27_LOGSUB_B
static uint32_t lv_lsb_key(void)
{
    uint32_t k = 0x5A5A0000u ^ (uint32_t)lv_lsb_filt * 0x10001u ^ (uint32_t)lv_lsb_paused * 0x8000u;
    k ^= (uint32_t)((int32_t)(lv_lsb_flash_t - lv_now) > 0) * (uint32_t)(1 + lv_lsb_flash_id) * 0x100u;
    k ^= (uint32_t)(((int32_t)(lv_now - lv_lsb_save_t) < 1200 && lv_lsb_save_t) ? 1 + (lv_lsb_save_ok < 0) : 0) * 0x1111u;
    return k;
}
#endif
/* the panel buffer only re-rasters when a visible thing changed: a new arrival, a slide frame, or the 240 ms flare ending */
static bool lv_ls_stale(void)
{
    const int p = lv_ls_phase();
    uint32_t k = (uint32_t)lv_ls_arrivals() * 64u + (uint32_t)(p < LV_LS_SLIDE_MS ? 1 + p / 30 : (p < 400 ? 40 : 41));
#if LV_CYD27_LOGSUB_B
    k = lv_lsb_key();   /* v1.2.26: the B plate only changes with its own chip states (it used to re-raster every row: rkey was never set) */
#endif
    return k != lv_ls_rkey;
}
static void lv_ls_put_clip(int role, int px, int py) { if (py >= LV_LS_CLIP_Y0 && py <= LV_LS_CLIP_Y1) lv_c17_put(role, px, py); }
static void lv_ls_char(int x, int y, char ch, int fg, int g1)
{
    const unsigned uc = (unsigned char)ch; if (uc <= 32 || uc > 126) return;
    const uint8_t *col5 = &LV_GLCD[(uc - 32) * 5];
    for (int pass = 0; pass < 2; ++pass)
        for (int c = 0; c < 5; ++c) for (int r = 0; r < 7; ++r) if (col5[c] >> r & 1) {
            const int px = x + c, py = y + r;
            if (pass == 0 && g1 >= 0) { lv_ls_put_clip(g1, px - 1, py); lv_ls_put_clip(g1, px + 1, py); lv_ls_put_clip(g1, px, py - 1); lv_ls_put_clip(g1, px, py + 1); }
            if (pass == 1) lv_ls_put_clip(fg, px, py);
        }
}
/* called from lv_lb_render in place of the static 5-row block: same text, same roles, scrolling */
static void lv_ls_rows(void)
{
    const int PX0 = LV_CB_PX0, a = lv_ls_arrivals(), off = lv_ls_off(), p = lv_ls_phase();
    lv_ls_rkey = (uint32_t)a * 64u + (uint32_t)(p < LV_LS_SLIDE_MS ? 1 + p / 30 : (p < 400 ? 40 : 41));
    const bool flare = a > 0 && p < 240;
    for (int k = -1; k < 5; ++k) {
        const int idx = a + k, y = 237 + k * 9 + off, age = 4 - k;
        char buf[28]; const int ev = lv_lb_row(lv_lb_tab, idx + 8, buf);
        const int base = age == 0 ? C17_TAG : age <= 2 ? C17_BODY : age == 3 ? C17_HOT : C17_MID;
        for (int c = 0; buf[c]; ++c) {
            int role = base;
            if (c < 8) role = age == 0 ? C17_HOT : C17_MID;
            if (ev >= 0 && c >= ev && c < ev + 5 && buf[c] != ' ') role = age == 4 ? C17_HOT : C17_LEAD;
            if (age == 0 && flare && c >= 8) role = C17_LEAD;
            lv_ls_char(PX0 + 4 + c * 6, y, buf[c], role, age == 0 ? C17_T1 : C17_NONE);
        }
    }
}
/* ---- health strip: replaces the coil band (x34..205, y34..69). SAMPLE values except TOUCH's last tap. ---- */
#define LV_LS_SX0 34
#define LV_LS_SW 43
static void lv_ls_cell_text(uint16_t *row, int y, int ci, int gy, const char *s, int role)
{
    const int w = (int)strlen(s) * 6 - 1, x0 = LV_LS_SX0 + ci * LV_LS_SW + (LV_LS_SW - w) / 2 + (ci == 0 ? 0 : 0);
    lv_lb_gl_row(row, y, x0, gy, s, role, LV_LS_SX0, LV_LS_SX0 + 4 * LV_LS_SW - 1);
}
static void lv_ls_strip_row(uint16_t *row, int y)
{
    if (y < 34 || y > 69) return;
    const uint16_t fill = lv_rgb(24, 24, 24);
    for (int x = LV_LS_SX0; x < LV_LS_SX0 + 4 * LV_LS_SW; ++x) row[x] = fill;
    if (y >= 38 && y <= 64) for (int i = 1; i < 4; ++i) row[LV_LS_SX0 + i * LV_LS_SW - 1 + (i == 1 ? 0 : 0)] = lv_lb_ink(C17_T1, LV_LS_SX0 + i * LV_LS_SW, y);
    /* v1.2.26: all four are real. SD = card free space (lv_ui.cpp SD lib, read after entry / SAVE), TOUCH = last tap
     * x/y + its age, HEAP = ESP.getFreeHeap(), UPTIME = millis() (lv_now). Host sim without hooks shows "--". */
    const uint32_t sec = lv_now / 1000u;
    char up[12]; snprintf(up, sizeof up, "%u:%02u:%02u", (unsigned)(sec / 3600), (unsigned)(sec / 60 % 60), (unsigned)(sec % 60));
    char hp[12]; if (lv_log_heap_hook) { const uint32_t hb = lv_log_heap_hook(); snprintf(hp, sizeof hp, "%u.%uK", (unsigned)(hb / 1024u), (unsigned)(hb % 1024u * 10u / 1024u)); } else snprintf(hp, sizeof hp, "--");
    char tp[12]; if (lv_ls_tx >= 0) snprintf(tp, sizeof tp, "%d/%d", lv_ls_tx, lv_ls_ty); else snprintf(tp, sizeof tp, "NO TAP");
    char ta[12]; if (lv_ls_tx >= 0) { const uint32_t ag = (lv_now - lv_lsb_tap_t) / 1000u; if (ag < 60) snprintf(ta, sizeof ta, "%us AGO", (unsigned)ag); else snprintf(ta, sizeof ta, "%um AGO", (unsigned)(ag / 60 > 99 ? 99 : ag / 60)); } else snprintf(ta, sizeof ta, "--");
    char sd0[8], sd1[12];
    if (lv_lsb_sd == 1) { snprintf(sd0, sizeof sd0, "SD OK"); if (lv_lsb_sd_free >= 10240u) snprintf(sd1, sizeof sd1, "%u.%uG", (unsigned)(lv_lsb_sd_free / 1024u), (unsigned)(lv_lsb_sd_free % 1024u * 10u / 1024u)); else snprintf(sd1, sizeof sd1, "%uM", (unsigned)lv_lsb_sd_free); }
    else if (lv_lsb_sd == 0) { snprintf(sd0, sizeof sd0, "SD"); snprintf(sd1, sizeof sd1, "NONE"); }
    else if (lv_lsb_sd == -2) { snprintf(sd0, sizeof sd0, "SD"); snprintf(sd1, sizeof sd1, ".."); }
    else { snprintf(sd0, sizeof sd0, "SD"); snprintf(sd1, sizeof sd1, "--"); }
    lv_ls_cell_text(row, y, 0, 37, sd0, C17_MID);        lv_ls_cell_text(row, y, 0, 47, sd1, C17_LEAD);   lv_ls_cell_text(row, y, 0, 57, lv_lsb_sd == 1 ? "FREE" : "", C17_MID);
    lv_ls_cell_text(row, y, 1, 37, "TOUCH", C17_MID);    lv_ls_cell_text(row, y, 1, 47, tp, C17_LEAD);    lv_ls_cell_text(row, y, 1, 57, ta, C17_MID);
    lv_ls_cell_text(row, y, 2, 37, "HEAP", C17_MID);     lv_ls_cell_text(row, y, 2, 47, hp, C17_LEAD);    lv_ls_cell_text(row, y, 2, 57, "FREE", C17_MID);
    lv_ls_cell_text(row, y, 3, 37, "UPTIME", C17_MID);   lv_ls_cell_text(row, y, 3, 47, up, C17_LEAD);    lv_ls_cell_text(row, y, 3, 57, "H:M:S", C17_MID);
}
#if LV_CYD27_LOGSUB_B
/* variant B drawing (state declared earlier) */
/* v1.2.26: rows are the real lv_log ring (lv_log.h), oldest at the top, newest at the bottom and highlighted.
 * Engine 3x5 caps font (LV_F35, 4 px advance): 25 glyphs a line, 14 lines at a 7 px pitch in the 102x104 window.
 * Every entry prints whole: "MM:SS body", the body word-wraps under itself (indent 6) onto up to 3 lines; an
 * entry that would not fit whole at the top is left out, never cut. No sample tag, no sample rows. */
#define LV_LSB_COLS 25
#define LV_LSB_IND  6
#define LV_LSB_PITCH 7
#define LV_LSB_NL   14
#define LV_LSB_MAXL 3
static bool lv_lsb_match(const LvLogE &e) { return lv_lsb_filt == LV_LSB_ALL || (int)e.kind == lv_lsb_filt - 1; }
static uint32_t lv_lsb_end(void) { return lv_lsb_paused ? lv_lsb_view : lv_log_seq; }
/* word-wrap body into lines of w glyphs; returns line count (<= LV_LSB_MAXL), starts[i] / lens[i] index body */
static int lv_lsb_wrap(const char *b, int w, int st[LV_LSB_MAXL], int ln[LV_LSB_MAXL])
{
    const int n = (int)strlen(b); int p = 0, k = 0;
    while (p < n && k < LV_LSB_MAXL) {
        while (p < n && b[p] == ' ') ++p;
        if (p >= n) break;
        int e = p + w;
        if (e >= n) e = n;
        else { int q = e; while (q > p && b[q] != ' ') --q; if (q > p) e = q; }   /* break at the last space, else hard */
        st[k] = p; ln[k] = e - p; ++k; p = e;
    }
    if (!k) { st[0] = 0; ln[0] = 0; k = 1; }
    return k;
}
static void lv_lsb_put35(uint16_t *row, int x, int y, int gy, char ch, int role)
{
    const int bit = y - gy;
    if (bit < 0 || bit > 4 || y < LV_LSB_Y0 || y > LV_LSB_Y1) return;
    int uc = (unsigned char)ch; if (uc >= 'a' && uc <= 'z') uc -= 32; if (uc <= 32 || uc > 126) return;
    const uint16_t g = LV_F35[uc - 32];
    for (int c = 0; c < 3; ++c) if (g >> (14 - (bit * 3 + c)) & 1) { const int px = x + c; if (px >= LV_LSB_X0 && px <= LV_LSB_X1) row[px] = lv_lb_ink(role, px, y); }
}
static void lv_lsb_text35(uint16_t *row, int y, int x, int gy, const char *s, int n, int role)
{
    for (int i = 0; i < n && s[i]; ++i) lv_lsb_put35(row, x + i * 4, y, gy, s[i], role);
}
/* one frame's layout, cached per (seq end, from, filter): entry seq + first line slot for each visible entry */
struct LvLsbLay { uint32_t key0, key1; int n; uint32_t seq[LV_LSB_NL]; int8_t line0[LV_LSB_NL], nl[LV_LSB_NL]; };
static LvLsbLay lv_lsb_lay = {0xFFFFFFFFu, 0, 0, {0}, {0}, {0}};
static void lv_lsb_layout(void)
{
    const uint32_t end = lv_lsb_end(), k0 = end, k1 = lv_lsb_from * 8u + (uint32_t)lv_lsb_filt;
    if (lv_lsb_lay.key0 == k0 && lv_lsb_lay.key1 == k1) return;
    lv_lsb_lay.key0 = k0; lv_lsb_lay.key1 = k1;
    uint32_t sq[LV_LSB_NL]; int8_t nl[LV_LSB_NL]; int ne = 0, used = 0;
    uint32_t lo = lv_log_first(); if (lo < lv_lsb_from) lo = lv_lsb_from;
    for (uint32_t s = end; s > lo && ne < LV_LSB_NL; ) {   /* newest -> oldest */
        --s;
        const LvLogE *e = lv_log_at(s); if (!e || !lv_lsb_match(*e)) continue;
        int st[LV_LSB_MAXL], ln[LV_LSB_MAXL]; const int L = lv_lsb_wrap(e->t, LV_LSB_COLS - LV_LSB_IND, st, ln);
        if (used + L > LV_LSB_NL) break;   /* would not fit whole: stop (older ones are above it) */
        sq[ne] = s; nl[ne] = (int8_t)L; used += L; ++ne;
    }
    /* oldest at the top: entries are laid top-down so the newest ends on the bottom line */
    int line = LV_LSB_NL - used; lv_lsb_lay.n = ne;
    for (int i = 0; i < ne; ++i) {
        const int j = ne - 1 - i;
        lv_lsb_lay.seq[i] = sq[j]; lv_lsb_lay.nl[i] = nl[j]; lv_lsb_lay.line0[i] = (int8_t)line; line += nl[j];
    }
}
static void lv_lsb_face_row(uint16_t *row, int y)
{
    if (y < LV_PG_FX_Y0 || y > LV_PG_FX_Y1) return;
    const uint16_t fill = lv_rgb(3, 5, 9);
    for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = fill;
    /* 1 px theme-ink inner rim just inside the window, so the black chip bezel stays and the log still reads as a plate */
    if (y == LV_PG_FX_Y0 || y == LV_PG_FX_Y1 || y == LV_PG_FX_Y0 + 1 || y == LV_PG_FX_Y1 - 1) {
        for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = lv_lb_ink((y == LV_PG_FX_Y0 || y == LV_PG_FX_Y1) ? C17_T1 : C17_MID, x, y);
        return;
    }
    row[LV_PG_FX_X0] = row[LV_PG_FX_X0 + LV_PG_FX_S - 1] = lv_lb_ink(C17_T1, LV_PG_FX_X0, y);
    row[LV_PG_FX_X0 + 1] = row[LV_PG_FX_X0 + LV_PG_FX_S - 2] = lv_lb_ink(C17_MID, LV_PG_FX_X0 + 1, y);
    if (y < LV_LSB_Y0 || y > LV_LSB_Y1) return;
    lv_lsb_layout();
    const int gy0 = LV_LSB_Y0 + 3;   /* 14 x 7 = 98 px inside 104 */
    const int li = (y - gy0) / LV_LSB_PITCH; if (y < gy0 || li >= LV_LSB_NL) return;
    const int gy = gy0 + li * LV_LSB_PITCH;
    if (lv_lsb_lay.n == 0) {   /* honest empty state */
        if (li == LV_LSB_NL / 2 - 1) {
            const char *m = lv_lsb_from ? "CLEARED" : (lv_lsb_filt == LV_LSB_ALL ? "NO LOG YET" : "NO LINES");
            const int w = (int)strlen(m) * 4 - 1; lv_lsb_text35(row, y, LV_PG_FX_X0 + (LV_PG_FX_S - w) / 2, gy, m, 24, C17_MID);
        }
        return;
    }
    for (int i = 0; i < lv_lsb_lay.n; ++i) {
        const int l0 = lv_lsb_lay.line0[i];
        if (li < l0 || li >= l0 + lv_lsb_lay.nl[i]) continue;
        const LvLogE *e = lv_log_at(lv_lsb_lay.seq[i]); if (!e) return;
        const bool newest = (i == lv_lsb_lay.n - 1);
        const bool flare = newest && !lv_lsb_paused && (int32_t)(lv_now - e->ms) < 400;
        int st[LV_LSB_MAXL], ln[LV_LSB_MAXL]; lv_lsb_wrap(e->t, LV_LSB_COLS - LV_LSB_IND, st, ln);
        const int k = li - l0;
        if (newest) for (int x = LV_LSB_X0 - 1; x <= LV_LSB_X1 + 1; ++x) if (y >= gy - 1 && y <= gy + 5) row[x] = lv_mix(row[x], lv_lb_ink(C17_T1, x, y), 3, 8);   /* newest band */
        if (k == 0) { char ts[8]; lv_log_stamp(e->ms, ts); lv_lsb_text35(row, y, LV_LSB_X0, gy, ts, 5, newest ? C17_HOT : C17_MID); }
        const int age = lv_lsb_lay.n - 1 - i;
        const int role = newest ? (flare ? C17_HOT : C17_LEAD) : (age <= 2 ? C17_TAG : C17_BODY);
        lv_lsb_text35(row, y, LV_LSB_X0 + LV_LSB_IND * 4, gy, e->t + st[k], ln[k], role);
        return;
    }
}

/* ---- bottom plate: two rows of uniform sub-screen chips (D-grey frame, theme-ink label, selected art brightens) ---- */
/* Row 1 (filters) y 226..250, four 34x21 chips. Row 2 (actions) y 255..285, three 46x28 chips.
 * Drawn rects == tap rects. A 160 ms invert flash on press; PAUSE stays in the LIVE/PAUSE state; SAVE reads SAVED briefly. */
static const LvRect LV_LSB_F[4] = { {49, 226, 82, 250}, {86, 226, 119, 250}, {123, 226, 156, 250}, {160, 226, 193, 250} };
static const LvRect LV_LSB_A[3] = { {49, 255, 94, 285}, {98, 255, 143, 285}, {147, 255, 192, 285} };
static const char *const LV_LSB_FLAB[4] = { "ALL", "WIFI", "BLE", "SYS" };
static int lv_lsb_flashing(int id) { return lv_lsb_flash_id == id && (int32_t)(lv_lsb_flash_t - lv_now) > 0; }
static void lv_lsb_btn(int x0, int y0, int x1, int y1, const char *lab, int lit, int flash)
{
    /* same recipe as lv_scene_minterm: D-grey frame, black well, 5x7 theme-ink label. lit = selected / active. */
    const uint16_t fr = lv_rgb(41, 52, 66);
    for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
        const int o = (x == x0 || x == x1 || y == y0 || y == y1);
        uint16_t c = o ? fr : lv_rgb(0, 0, 0);
        if (lit && o) c = lv_lb_ink(C17_HOT, x, y);
        else if (lit && !o) c = lv_lb_ink(C17_T1, x, y);
        if (flash) c = lv_hi(c);
        const int bx = x - LV_CB_PX0, by = y - LV_CB_PY0;
        if (bx >= 2 && bx <= LV_CB_PW - 3 && by >= 2 && by <= LV_CB_PH - 3) lv_c17_buf[by][bx] = c;
    }
    if (!lab) return;
    const int lw = (int)strlen(lab) * 6 - 1, cx = (x0 + x1 - lw) / 2, cy = (y0 + y1 - 6) / 2;
    for (const char *s = lab; *s; ++s) lv_c17_char(cx + (int)(s - lab) * 6, cy, *s, lit ? C17_LEAD : C17_BODY, lit ? C17_T1 : C17_NONE, C17_NONE);
}
static void lv_lsb_panel(void)
{
    /* frame already painted by lv_lb_render (oil-slick metal ring on a near-black field). Just the chips. */
    lv_ls_rkey = lv_lsb_key();
    for (int i = 0; i < 4; ++i)
        lv_lsb_btn(LV_LSB_F[i].x0, LV_LSB_F[i].y0, LV_LSB_F[i].x1, LV_LSB_F[i].y1, LV_LSB_FLAB[i], lv_lsb_filt == i, lv_lsb_flashing(i));
    const char *pause = lv_lsb_paused ? "PAUSE" : "LIVE";
    const bool sv = (int32_t)(lv_now - lv_lsb_save_t) < 1200 && lv_lsb_save_t;
    const char *save  = sv ? (lv_lsb_save_ok > 0 ? "SAVED" : lv_lsb_save_ok == 0 ? "EMPTY" : "SD ERR") : "SAVE";
    lv_lsb_btn(LV_LSB_A[0].x0, LV_LSB_A[0].y0, LV_LSB_A[0].x1, LV_LSB_A[0].y1, pause, lv_lsb_paused, lv_lsb_flashing(4));
    lv_lsb_btn(LV_LSB_A[1].x0, LV_LSB_A[1].y0, LV_LSB_A[1].x1, LV_LSB_A[1].y1, "CLEAR", 0, lv_lsb_flashing(5));
    lv_lsb_btn(LV_LSB_A[2].x0, LV_LSB_A[2].y0, LV_LSB_A[2].x1, LV_LSB_A[2].y1, save, ((int32_t)(lv_now - lv_lsb_save_t) < 1200 && lv_lsb_save_t), lv_lsb_flashing(6));
}
static const char *lv_lsb_tap(int x, int y)
{
    for (int i = 0; i < 4; ++i) if (lv_in(LV_LSB_F[i], x, y)) {
        lv_lsb_filt = i; lv_lsb_flash_id = i; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return i == 0 ? "logsub-filt-all" : i == 1 ? "logsub-filt-wifi" : i == 2 ? "logsub-filt-ble" : "logsub-filt-sys";
    }
    if (lv_in(LV_LSB_A[0], x, y)) {
        if (!lv_lsb_paused) lv_lsb_view = lv_log_seq;   /* freeze: new lines keep arriving in the ring, the view stays */
        lv_lsb_paused ^= 1;
        lv_lsb_flash_id = 4; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return lv_lsb_paused ? "logsub-pause" : "logsub-live";
    }
    if (lv_in(LV_LSB_A[1], x, y)) {
        lv_lsb_from = lv_lsb_end();   /* hides what is on screen now; the ring itself is kept (SAVE after CLEAR saves the new lines) */
        lv_lsb_flash_id = 5; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return "logsub-clear";
    }
    if (lv_in(LV_LSB_A[2], x, y)) {
        /* real: lv_ui.cpp writes every kept line under the current filter to SD /logs/logbot_<n>.txt and reads it back */
        char path[40] = "";
        const uint32_t lo = lv_log_first() > lv_lsb_from ? lv_log_first() : lv_lsb_from;
        lv_lsb_save_ok = lv_log_save_hook ? lv_log_save_hook(lv_lsb_filt, lo, lv_lsb_end(), path, sizeof path) : -1;
        if (lv_lsb_save_ok > 0) lv_log_pushf(LV_LOG_SYS, "SAVED %d lines %s", lv_lsb_save_ok, path);
        else if (lv_lsb_save_ok < 0) lv_log_pushf(LV_LOG_SYS, "SAVE failed: %s", lv_log_save_hook ? "SD" : "no SD here");
        lv_lsb_sd = -2;   /* re-read free space */
        lv_lsb_save_t = lv_now;
        lv_lsb_flash_id = 6; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return "logsub-save";
    }
    return 0;
}
/* once a frame while the subscreen is up: the SD cell's one mount, 300 ms after entry (never on the flip-in frame) */
static void lv_lsb_tick(void)
{
    if (lv_lsb_sd == -2 && (uint32_t)(lv_now - lv_ls_t0) >= 300u) {
        lv_lsb_sd = lv_log_sd_hook ? lv_log_sd_hook(&lv_lsb_sd_free, &lv_lsb_sd_size) : -1;
        lv_pg_dirty = true;
    }
}
#endif /* LV_CYD27_LOGSUB_B */
#else
#define lv_ls_active() false
#endif
