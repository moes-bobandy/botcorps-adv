/* MOCK 2026-11-28 (CYD-27, sim-only, NOT in any bin): LOGbot SUBSCREEN = "live log plus a device health strip (SD, touch, heap, uptime)".
 * Off by default: LV_CYD27_LOGSUB is 0, so firmware and v1.2.25 renders are byte-identical (every hook below compiles away).
 * Opened by tapping the LOGbot face on the LOGbot pager page; the existing banner Stripe-Slot back (lv_banner_back_draw, LV_BANNER_BACK_LS)
 * returns to that page. The pager arrow chips, pips and swipe are not on a sub-screen, so they are off here.
 * Reused verbatim: board still, LOGbot chip face + rain (110x110 art, oil-slick finish), log plate frame + SCAN/SYS tabs + EX.DATA tag
 * (lv_lb_render in lv_pager_log19.h), the log row text generator (lv_lb_row) and the F2 theme-ink roles (lv_lb_ink / lv_c17_role_px).
 * Added: (1) the log rows now scroll: a new row arrives every 1.8 s and the rows slide up 9 px (270 ms ease), the arriving row flares for
 * 240 ms, rows clip to the plate; (2) the health strip, which replaces the coil band at y34..69: four cells SD / TOUCH / HEAP / UPTIME.
 * Every glyph is theme ink per pixel (lv_theme_at), never hardcoded; no decorative bars, no GPIO/NVS text.
 * SAMPLE DATA: SD free, heap and the uptime base are simulated (the log plate carries EX.DATA); TOUCH shows the real last tap x,y the engine received. */
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
static int lv_lsb_pause_a = 0;          /* arrivals frozen at the PAUSE tap */
static int lv_lsb_cleared = 0;          /* hide every row whose idx is < this */
static uint32_t lv_lsb_flash_t = 0;     /* press-highlight until this time */
static int lv_lsb_flash_id = -1;        /* 0..3 filters, 4 PAUSE, 5 CLEAR, 6 SAVE */
static uint32_t lv_lsb_save_t = 0;      /* SAVE label reads SAVED for 1.2 s */
static int lv_lsb_saved_n = 0;          /* SAMPLE count written to /logs (not a real SD write in the host sim) */
#endif
#define LV_LS_ARR_MS 1800
#define LV_LS_SLIDE_MS 270
#define LV_LS_CLIP_Y0 236
#define LV_LS_CLIP_Y1 287
static inline bool lv_ls_active(void) { return lv_ls_on && lv_scr == LV_SC_HOME && lv_pg_page == LV_PG_LOGBOT; }
static void lv_ls_enter(void) {
    lv_ls_on = true; lv_ls_t0 = lv_now; lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
#if LV_CYD27_LOGSUB_B
    lv_lsb_filt = LV_LSB_ALL; lv_lsb_paused = 0; lv_lsb_pause_a = 0; lv_lsb_cleared = 0;
    lv_lsb_flash_t = 0; lv_lsb_flash_id = -1; lv_lsb_save_t = 0; lv_lsb_saved_n = 0;
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
/* the panel buffer only re-rasters when a visible thing changed: a new arrival, a slide frame, or the 240 ms flare ending */
static bool lv_ls_stale(void)
{
    const int p = lv_ls_phase();
    uint32_t k = (uint32_t)lv_ls_arrivals() * 64u + (uint32_t)(p < LV_LS_SLIDE_MS ? 1 + p / 30 : (p < 400 ? 40 : 41));
#if LV_CYD27_LOGSUB_B
    k ^= (uint32_t)lv_lsb_filt * 0x10001u ^ (uint32_t)lv_lsb_paused * 0x8000u ^ (uint32_t)lv_lsb_cleared * 17u;
    k ^= (uint32_t)((int32_t)(lv_lsb_flash_t - lv_now) > 0) * (uint32_t)(1 + lv_lsb_flash_id);
    k ^= (uint32_t)(((int32_t)(lv_now - lv_lsb_save_t) < 1200 && lv_lsb_save_t) ? 1 : 0) * 0x111u;
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
    const uint32_t sec = 8049u + lv_now / 1000u;                       /* sample boot offset 2:14:09 + live seconds */
    char up[12]; snprintf(up, sizeof up, "%u:%02u:%02u", (unsigned)(sec / 3600), (unsigned)(sec / 60 % 60), (unsigned)(sec % 60));
    const uint32_t h = lv_lb_hash(lv_now / 700u + 77u);
    const int heap10 = 1184 + (int)(h % 9u) - 4;                       /* free KB x10, +-0.4 KB sample wobble */
    char hp[12]; snprintf(hp, sizeof hp, "%d.%dK", heap10 / 10, heap10 % 10);
    char tp[12]; if (lv_ls_tx >= 0) snprintf(tp, sizeof tp, "%d/%d", lv_ls_tx, lv_ls_ty); else snprintf(tp, sizeof tp, "NO TAP");
    lv_ls_cell_text(row, y, 0, 37, "SD OK", C17_MID);    lv_ls_cell_text(row, y, 0, 47, "14.2G", C17_LEAD);  lv_ls_cell_text(row, y, 0, 57, "FREE", C17_MID);
    lv_ls_cell_text(row, y, 1, 37, "TOUCH", C17_MID);    lv_ls_cell_text(row, y, 1, 47, "OK", C17_LEAD);     lv_ls_cell_text(row, y, 1, 57, tp, C17_MID);
    lv_ls_cell_text(row, y, 2, 37, "HEAP", C17_MID);     lv_ls_cell_text(row, y, 2, 47, hp, C17_LEAD);      lv_ls_cell_text(row, y, 2, 57, "FREE", C17_MID);
    lv_ls_cell_text(row, y, 3, 37, "UPTIME", C17_MID);   lv_ls_cell_text(row, y, 3, 47, up, C17_LEAD);      lv_ls_cell_text(row, y, 3, 57, "H:M:S", C17_MID);
}
#if LV_CYD27_LOGSUB_B
/* variant B drawing (state declared earlier) */
/* mixed log: cycle WIFI / BLE / SYS so the four filters each have something to show. */
static int lv_lsb_kind(int i)
{
    /* 0 WIFI, 1 BLE, 2 SYS, slightly irregular so a filter never looks empty for long */
    static const int8_t K[10] = {0, 2, 0, 1, 0, 2, 1, 0, 2, 1};
    return K[((i % 10) + 10) % 10];
}
static int lv_lsb_fit(char out[28], int ev)
{
    /* face window holds 16 glyphs at 6 px pitch (95 px in a 102 px inner width). Keep HH:MM + body, never left-trim. */
    char src[28]; memcpy(src, out, 28);
    /* src is "HH:MM:SS rest". Drop the :SS so the body stays intact. */
    if (src[2] == ':' && src[5] == ':' && src[8] == ' ') {
        char body[20]; snprintf(body, sizeof body, "%s", src + 9);
        snprintf(out, 28, "%.5s %s", src, body);          /* HH:MM + space + body */
    }
    if ((int)strlen(out) > 16) out[16] = 0;
    /* ev was an index into the original "HH:MM:SS rest"; after dropping :SS it shifts left by 3 */
    if (ev >= 9) ev -= 3;
    if (ev >= 16) ev = -1;
    return ev;
}
static int lv_lsb_row(int i, char out[28])
{
    const int kind = lv_lsb_kind(i);
    int ev;
    if (kind == 2) ev = lv_lb_row(1, i, out);
    else if (kind == 0) ev = lv_lb_row(0, i, out);
    else {
        const uint32_t h = lv_lb_hash((uint32_t)i * 2654435761u + 1954u);
        int secs = 14 * 3600 + 1 * 60 + 7 + i * 11 + (int)(h % 7);
        char t[12]; snprintf(t, sizeof t, "%02d:%02d:%02d", (secs / 3600) % 24, (secs / 60) % 60, secs % 60);
        const uint32_t m = lv_lb_hash(h);
        static const char *NM[] = { "AirPods", "Flipper", "Watch-9", "KB-Mini", "Tile-22" };
        ev = 9;
        switch (i % 5) {
        case 0: snprintf(out, 28, "%s BLE %02X%02X%02X", t, (m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255); break;
        case 1: snprintf(out, 28, "%s BLE %s", t, NM[(h >> 16) % 5]); break;
        case 2: snprintf(out, 28, "%s BLE rssi %d", t, -(50 + (int)((h >> 8) % 40))); break;
        case 3: snprintf(out, 28, "%s BLE lock %02X", t, m & 255); break;
        default: snprintf(out, 28, "%s BLE end %ds", t, 6 + (int)((h >> 4) % 30)); break;
        }
    }
    return lv_lsb_fit(out, ev);
}
/* absolute source index of the k-th visible row under the current filter (0 = oldest of the window, growing up).
 * Walk back from the current arrival so filters only show matching kinds. Returns -1 when the stream is exhausted. */
static int lv_lsb_vis(int a, int nth)
{
    int seen = 0;
    /* Walk newest -> oldest. i runs from the current arrival down through a 40-row backlog
     * (so a WIFI/BLE/SYS filter still fills 12 rows) but never below lv_lsb_cleared. */
    const int lo = lv_lsb_cleared;
    for (int i = a; i >= lo; --i) {
        const int kind = lv_lsb_kind(i + 8);   /* 0 WIFI, 1 BLE, 2 SYS; filt 1/2/3 map onto those */
        if (lv_lsb_filt != LV_LSB_ALL && kind != lv_lsb_filt - 1) continue;
        if (seen == nth) return i;
        ++seen;
    }
    /* before any arrivals, still show the photographed backlog (idx -1,-2,..) so the window isn't empty on entry.
     * CLEAR sets lo to now_a+1, so this backlog is skipped after a clear (seen stays 0 and we return -1). */
    if (lo > 0) return -999;   /* cleared: do not resurrect the backlog */
    for (int i = -1; i >= -40; --i) {
        const int kind = lv_lsb_kind(i + 8);
        if (lv_lsb_filt != LV_LSB_ALL && kind != lv_lsb_filt - 1) continue;
        if (seen == nth) return i;
        ++seen;
    }
    return -999;   /* no such row (not a backlog index) */
}
static int lv_lsb_now_a(void) { return lv_lsb_paused ? lv_lsb_pause_a : lv_ls_arrivals(); }
/* 12 rows, 8 px pitch, clipped to the face window. Newest at the bottom, highlighted. */
static void lv_lsb_put(uint16_t *row, int x, int y, int gy, char ch, int role)
{
    if (x < LV_LSB_X0 || x > LV_LSB_X1) return;
    if (y < LV_LSB_Y0 || y > LV_LSB_Y1) return;
    const int bit = y - gy;
    if (bit < 0 || bit > 6) return;
    const unsigned uc = (unsigned char)ch; if (uc <= 32 || uc > 126) return;
    const uint8_t *g = LV_GLCD + (uc - 32) * 5;
    for (int c = 0; c < 5; ++c) if ((g[c] >> bit & 1) && x + c >= LV_LSB_X0 && x + c <= LV_LSB_X1)
        row[x + c] = lv_lb_ink(role, x + c, y);
}
static void lv_lsb_face_row(uint16_t *row, int y)
{
    if (y < LV_PG_FX_Y0 || y > LV_PG_FX_Y1) return;
    const uint16_t fill = lv_rgb(3, 5, 9);
    for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = fill;
    /* 1 px theme-ink inner rim just inside the window, so the black chip bezel stays and the log still reads as a plate */
    if (y == LV_PG_FX_Y0 || y == LV_PG_FX_Y1 || y == LV_PG_FX_Y0 + 1 || y == LV_PG_FX_Y1 - 1) {
        for (int x = LV_PG_FX_X0; x <= LV_PG_FX_X0 + LV_PG_FX_S - 1; ++x) row[x] = lv_lb_ink((y == LV_PG_FX_Y0 || y == LV_PG_FX_Y1) ? C17_T1 : C17_MID, x, y);
    } else {
        row[LV_PG_FX_X0] = row[LV_PG_FX_X0 + LV_PG_FX_S - 1] = lv_lb_ink(C17_T1, LV_PG_FX_X0, y);
        row[LV_PG_FX_X0 + 1] = row[LV_PG_FX_X0 + LV_PG_FX_S - 2] = lv_lb_ink(C17_MID, LV_PG_FX_X0 + 1, y);
    }
    const int a = lv_lsb_now_a();
    const int off = (lv_lsb_paused || a == 0) ? 0 : lv_ls_off();
    const int p = lv_ls_phase();
    const bool flare = !lv_lsb_paused && a > 0 && p < 240;
    /* 13 slots so the arriving row can slide in from below; clip keeps it inside the window */
    for (int k = -1; k < LV_LSB_NROW; ++k) {
        const int vis = LV_LSB_NROW - 1 - k;          /* vis 0 = oldest (top), vis N-1 = newest (bottom) */
        const int idx = lv_lsb_vis(a, vis);
        if (idx == -999) continue;
        const int gy = LV_LSB_Y0 + 8 + k * LV_LSB_ROW + off;   /* 7 px EX.DATA on top, then 12 rows */
        char buf[28]; const int ev = lv_lsb_row(idx + 8, buf);
        const int age = vis;   /* 0 oldest .. 11 newest */
        const int base = age == 0 ? C17_MID : (age >= LV_LSB_NROW - 1 ? C17_LEAD : (age >= LV_LSB_NROW - 3 ? C17_TAG : C17_BODY));
        for (int c = 0; buf[c]; ++c) {
            int role = base;
            if (c < 5) role = age >= LV_LSB_NROW - 1 ? C17_HOT : C17_MID;   /* HH:MM */
            if (ev >= 0 && c >= ev && c < ev + 5 && buf[c] != ' ') role = C17_LEAD;
            if (age == LV_LSB_NROW - 1 && flare && c >= 5) role = C17_LEAD;
            lv_lsb_put(row, LV_LSB_X0 + c * 6, y, gy, buf[c], role);
        }
    }
    /* EX.DATA on the top inner rim, never over a log row */
    if (y >= LV_LSB_Y0 && y <= LV_LSB_Y0 + 6) {
        const char *ex = "EX.DATA";
        const int x0 = LV_PG_FX_X0 + LV_PG_FX_S - 5 - 7 * 6;
        for (int c = 0; ex[c]; ++c) lv_lsb_put(row, x0 + c * 6, y, LV_LSB_Y0, ex[c], C17_MID);
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
    for (int i = 0; i < 4; ++i)
        lv_lsb_btn(LV_LSB_F[i].x0, LV_LSB_F[i].y0, LV_LSB_F[i].x1, LV_LSB_F[i].y1, LV_LSB_FLAB[i], lv_lsb_filt == i, lv_lsb_flashing(i));
    const char *pause = lv_lsb_paused ? "PAUSE" : "LIVE";
    const char *save  = ((int32_t)(lv_now - lv_lsb_save_t) < 1200 && lv_lsb_save_t) ? "SAVED" : "SAVE";
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
        if (!lv_lsb_paused) lv_lsb_pause_a = lv_ls_arrivals();
        else {
            /* resume: keep the frozen view as the new baseline so we don't jump */
            lv_ls_t0 = lv_now - (uint32_t)lv_lsb_pause_a * (uint32_t)LV_LS_ARR_MS;
        }
        lv_lsb_paused ^= 1;
        lv_lsb_flash_id = 4; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return lv_lsb_paused ? "logsub-pause" : "logsub-live";
    }
    if (lv_in(LV_LSB_A[1], x, y)) {
        lv_lsb_cleared = lv_lsb_now_a() + 1;
        lv_lsb_flash_id = 5; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return "logsub-clear";
    }
    if (lv_in(LV_LSB_A[2], x, y)) {
        /* SAMPLE: pretend we wrote the visible rows to SD /logs. Host sim has no SD; the chip reads SAVED. */
        int n = 0; const int a = lv_lsb_now_a();
        for (int k = 0; k < 64; ++k) if (lv_lsb_vis(a, k) != -999) ++n; else break;
        lv_lsb_saved_n = n;
        lv_lsb_save_t = lv_now;
        lv_lsb_flash_id = 6; lv_lsb_flash_t = lv_now + 160;
        lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true;
        return "logsub-save";
    }
    return 0;
}
#endif /* LV_CYD27_LOGSUB_B */
#else
#define lv_ls_active() false
#endif
