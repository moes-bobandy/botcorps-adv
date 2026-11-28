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
#define LV_LS_ARR_MS 1800
#define LV_LS_SLIDE_MS 270
#define LV_LS_CLIP_Y0 236
#define LV_LS_CLIP_Y1 287
static inline bool lv_ls_active(void) { return lv_ls_on && lv_scr == LV_SC_HOME && lv_pg_page == LV_PG_LOGBOT; }
static void lv_ls_enter(void) { lv_ls_on = true; lv_ls_t0 = lv_now; lv_ls_rkey = 0xFFFFFFFFu; lv_lb_key = 0xFFFFFFFFu; lv_pg_dirty = true; }
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
    const uint32_t k = (uint32_t)lv_ls_arrivals() * 64u + (uint32_t)(p < LV_LS_SLIDE_MS ? 1 + p / 30 : (p < 400 ? 40 : 41));
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
#else
#define lv_ls_active() false
#endif
