/* MOCK 2026-11-26 (CYD-19, sim-only, NOT in any bin): LOGbot home pager page, between TERMINALbot and CONTROLbot.
 * Off by default: LV_CYD19_LOGBOT is 0, so firmware and v1.2.22 renders are byte-identical. The mock sim builds with -DLV_CYD19_LOGBOT=1.
 * Cloned verbatim from the locked pager page: board, chrome, pins, banner, pips, rain face box (65,88)-(174,197) and role panel
 * (45,220) 152x71. Added: (1) LOGbot's chip face, the SAME 110x110 chip-plate art as the gold TERMINALbot page (blob 2, lv_pg_ch_row
 * in lv_pager.h, pixels in LV_PG_FACE_Z / lv_pager_data.h), remapped through a metal finish + a smooth sheen. Both eyes are the
 * art's locked sockets, left untouched and identical — the same black chip-plate eyes TERMINALbot uses (kept as c==0, rain skip=1
 * via lv_pg_keep_feat, no lit/tab state on the eyes). No mouth / counter slot: the lower plate is the art, unfilled. (2) The panel
 * is a two-tab terminal log (SCAN LOG / SYSTEM LOG), F2 look from CYD-17 (lv_pager_term17.h), ink = live theme per pixel
 * (lv_theme_at + sec_legible), never hardcoded. The active log tab shows only on the panel SCAN/SYS tabs. Tap the face or the
 * panel to flip tabs. All log rows are generated SAMPLE data (EX.DATA tag on the panel): no live scan or system log is read.
 * Locked finish: oil-slick (LV_LB_FIN default 2; animated iridescent sheen). Host sim still accepts LB_FINISH=0/1/2 (chrome / copper / oil-slick).
 * Mock-only RAM note: reuses the 21 KB CYD-17 panel buffer; a firmware port would draw row-wise. */
#pragma once
#if LV_CYD19_LOGBOT
#include <stdlib.h>
#ifndef LV_LB_FIN
#define LV_LB_FIN 2
#endif
struct LvLbStop { int16_t l; uint8_t r, g, b; };
/* lum -> colour, piecewise linear. Chrome is non-monotone on purpose (dark horizon band between two bright reflections). */
static const LvLbStop LV_LB_CHROME[] = { {0, 4, 9, 24}, {55, 14, 36, 88}, {95, 44, 98, 190}, {125, 12, 36, 96}, {150, 96, 160, 240}, {178, 214, 236, 255}, {205, 255, 255, 255}, {255, 255, 255, 255} };
static const LvLbStop LV_LB_COPPER[] = { {0, 30, 10, 5}, {70, 78, 32, 18}, {100, 120, 54, 30}, {125, 164, 84, 46}, {150, 214, 126, 78}, {175, 250, 184, 134}, {205, 255, 226, 196}, {255, 255, 242, 226} };
static uint16_t lv_lb_ramp(const LvLbStop *s, int n, int l)
{
    if (l <= s[0].l) return lv_rgb(s[0].r, s[0].g, s[0].b);
    for (int i = 1; i < n; ++i) if (l <= s[i].l) {
        const int d = s[i].l - s[i - 1].l, k = l - s[i - 1].l;
        return lv_rgb((uint8_t)((s[i - 1].r * (d - k) + s[i].r * k) / d), (uint8_t)((s[i - 1].g * (d - k) + s[i].g * k) / d), (uint8_t)((s[i - 1].b * (d - k) + s[i].b * k) / d));
    }
    return lv_rgb(s[n - 1].r, s[n - 1].g, s[n - 1].b);
}
static int lv_lb_fin(void)
{
    static int f = -1;
    if (f < 0) {
        f = LV_LB_FIN;
#if !defined(ARDUINO)
        const char *e = getenv("LB_FINISH"); if (e && *e >= '0' && *e <= '2') f = *e - '0';
#endif
    }
    return f;
}
static void lv_lb_hsv(float h, float s, float v, int &R, int &G, int &B)
{
    h -= floorf(h); const float h6 = h * 6.f; const int i = (int)h6; const float f = h6 - i;
    const float p = v * (1.f - s), q = v * (1.f - s * f), t = v * (1.f - s * (1.f - f));
    float r, g, b;
    switch (i % 6) { case 0: r = v; g = t; b = p; break; case 1: r = q; g = v; b = p; break; case 2: r = p; g = v; b = t; break;
                     case 3: r = p; g = q; b = v; break; case 4: r = t; g = p; b = v; break; default: r = v; g = p; b = q; }
    R = (int)(r * 255.f + .5f); G = (int)(g * 255.f + .5f); B = (int)(b * 255.f + .5f);
}
/* finish colour for source luminance l (0..255) at face-local (u,v) 0..109; panel frame passes its own (u,v)
 * v1.2.26 (Mobandy 2026-11-28, LOGbot flip-in lag): this ran cosf + 3 sinf + sqrtf + floorf per face pixel
 * (~12k px, every frame and every slide frame: ~70k libm calls, the face is animated by lv_now). The position-only
 * terms are now tables built once, the time term is one 110-entry table per frame, and the thin-film sine plus
 * the value/saturation curves are interpolated tables. Same formula, same colours (rounding-level). */
#define LV_LB_SN 1024
static float lv_lb_sin_t[LV_LB_SN + 1], lv_lb_cuv[220], lv_lb_su[110], lv_lb_vv[110];
static float lv_lb_val_t[258], lv_lb_sat_t[258];
static float lv_lb_ut[110]; static uint32_t lv_lb_ut_at = 0xFFFFFFFFu;
static bool lv_lb_tab_ok = false;
static void lv_lb_tables(void)
{
    if (lv_lb_tab_ok) return;
    for (int i = 0; i <= LV_LB_SN; ++i) lv_lb_sin_t[i] = sinf((float)i * (6.2831853f / LV_LB_SN));
    for (int i = 0; i < 220; ++i) lv_lb_cuv[i] = cosf(((float)i - 70.f) * 0.045f);
    for (int i = 0; i < 110; ++i) { lv_lb_su[i] = sinf((float)i * 0.21f); lv_lb_vv[i] = 20.f * ((float)i / 110.f); }
    for (int i = 0; i < 258; ++i) {
        const float L = (float)(i > 255 ? 255 : i), x = L / 230.f;
        float val = 0.10f + 0.90f * x * sqrtf(x); if (val > 1.f) val = 1.f;
        lv_lb_val_t[i] = val; lv_lb_sat_t[i] = 0.88f - 0.6f * x * x * x;
    }
    lv_lb_tab_ok = true;
}
static inline float lv_lb_sinr(float ph)   /* sinf(ph) for ph > -400, linear-interpolated 1024 table */
{
    const float k = ph * (LV_LB_SN / 6.2831853f) + (float)(LV_LB_SN * 64);   /* ph > -400: positive, no floorf */
    const int ki = (int)k; const float f = k - (float)ki; const int i = ki & (LV_LB_SN - 1);
    return lv_lb_sin_t[i] + (lv_lb_sin_t[i + 1] - lv_lb_sin_t[i]) * f;
}
static inline float lv_lb_lerp(const float *t, float L) { const int i = (int)L; const float f = L - (float)i; return t[i] + (t[i + 1] - t[i]) * f; }
static uint16_t lv_lb_metal(int l, int u, int v)
{
    lv_lb_tables();
    if (u < 0) u = 0; if (u > 109) u = 109; if (v < 0) v = 0; if (v > 109) v = 109;
    /* smooth anisotropic sheen laid over the art's banded shading, so no band is a flat fill */
    float L = (float)l + (lv_lb_fin() == 1 ? 34.f : 24.f) * lv_lb_cuv[u + v] - lv_lb_vv[v] + 6.f * lv_lb_su[u];
    if (L < 0.f) L = 0.f; if (L > 255.f) L = 255.f;
    const int f = lv_lb_fin();
    if (f == 0) return lv_lb_ramp(LV_LB_CHROME, (int)(sizeof LV_LB_CHROME / sizeof *LV_LB_CHROME), (int)L);
    if (f == 1) return lv_lb_ramp(LV_LB_COPPER, (int)(sizeof LV_LB_COPPER / sizeof *LV_LB_COPPER), (int)L);
    /* oil slick: near-black substrate whose value follows the shading, plus a thin-film hue (green->teal->blue->violet->magenta)
     * that drifts with position, shading and time; highlights wash toward white like a real slick */
    const uint32_t nm = lv_now % 12000u;
    const float t = (float)nm * (1.f / 12000.f);
    if (lv_lb_ut_at != nm) { for (int i = 0; i < 110; ++i) lv_lb_ut[i] = 0.7f * sinf((float)i * 0.06f + t * 12.566f); lv_lb_ut_at = nm; }
    const float ph = (float)(u + v) * 0.045f + L * 0.016f + lv_lb_ut[u] + t * 6.2832f;
    const float s1 = 0.5f + 0.5f * lv_lb_sinr(ph);
    const float hue = 0.32f + 0.55f * s1;
    const float val = lv_lb_lerp(lv_lb_val_t, L), sat = lv_lb_lerp(lv_lb_sat_t, L);
    int R, G, B; lv_lb_hsv(hue, sat, val, R, G, B);
    return lv_rgb((uint8_t)R, (uint8_t)G, (uint8_t)B);
}

/* ---- face ---- */
static inline uint16_t lv_lb_ink(int role, int x, int y)
{
    uint16_t c = lv_c17_role_px(role, lv_theme_at(lv_theme, x, y));
    return (y & 1) ? lv_dim(c, 7, 8) : c;
}
static void lv_lb_gl_row(uint16_t *row, int y, int x0, int gy, const char *s, int role, int xlo, int xhi)
{
    const int bit = y - gy;
    if (bit < 0 || bit > 6) return;
    for (; *s; ++s, x0 += 6) {
        const unsigned uc = (unsigned char)*s; if (uc <= 32 || uc > 126) continue;
        const uint8_t *g = LV_GLCD + (uc - 32) * 5;
        for (int c = 0; c < 5; ++c) if ((g[c] >> bit & 1) && x0 + c >= xlo && x0 + c <= xhi) row[x0 + c] = lv_lb_ink(role, x0 + c, y);
    }
}
/* v1.2.25: the 1.8 s arrival window was a SAMPLE typist loop — the same 8-line
 * cycle (BOOT / SD mount / TOUCH / WARN / FLASH / ...) kept sliding in and
 * "materializing" forever. That also forced a full panel re-raster (trig metal
 * frame + glyphs) every tick. Freeze arrivals at 0 so the 5 visible SYS rows
 * stay the photographed set (BOOT v1.2.25, SD mount ok, TOUCH cal ok, WARN,
 * FLASH ok 2.6MB). Look unchanged; no replay. */
static int lv_lb_arrivals(void) { return 0; }
static void lv_lb_face_row(uint16_t *row, int y, const uint8_t *fr)
{
    const int X0 = LV_PG_FX_X0, ly = y - LV_PG_FX_Y0;
    if (fr) memcpy(row + X0, fr, LV_PG_FACE_S * 2);
    for (int lx = 0; lx < LV_PG_FACE_S; ++lx) {
        const uint16_t c = row[X0 + lx];
        if (c == 0) continue;   /* the art's black eye sockets stay black */
        const int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
        row[X0 + lx] = lv_lb_metal((r * 3 + g * 6 + b) / 10, lx, ly);
    }
    { const uint16_t pal[3] = { 0, 0, 0 }; lv_pg_rain_row(row, y, pal, 1, LV_PG_LOGBOT); }   /* same black code rain as the gold TERMINALbot face; skip=1 keeps the art's eye sockets on top */
}

/* ---- panel: SCAN LOG / SYSTEM LOG terminal ---- */
static uint32_t lv_lb_hash(uint32_t i) { i ^= i >> 16; i *= 0x7feb352dU; i ^= i >> 15; i *= 0x846ca68bU; i ^= i >> 16; return i; }
/* row i (absolute, grows by one per 1.8 s) -> text (<= 24 chars) and the index of its event word (-1 = none) */
static int lv_lb_row(int tab, int i, char out[28])
{
    const uint32_t h = lv_lb_hash((uint32_t)i * 2654435761u + (uint32_t)tab * 977u);
    int secs = tab == 0 ? 14 * 3600 + 2 * 60 + 11 + i * 8 + (int)(h % 5) : 14 * 3600 + 0 * 60 + 1 + i * 41 + (int)(h % 17);
    char t[12]; snprintf(t, sizeof t, "%02d:%02d:%02d", (secs / 3600) % 24, (secs / 60) % 60, secs % 60);
    static const char *SSID[] = { "CafeGuest5G", "xfinitywifi", "Lobby-AP", "HP-Print-3C", "NETGEAR47", "ATTx2Kq9" };
    const int rssi = -(44 + (int)((h >> 8) % 44));
    const uint32_t m = lv_lb_hash(h);
    int ev = -1;
    if (tab == 0) {
        switch (i % 8) {
        case 0: case 2: case 7: snprintf(out, 28, "%s %d %02X:%02X:%02X:%02X", t, rssi, (m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255, m & 255); break;
        case 1: case 5: snprintf(out, 28, "%s %d %s", t, rssi, SSID[(h >> 16) % 6]); break;
        case 3: snprintf(out, 28, "%s HUNT %02X:%02X:%02X", t, (m >> 24) & 255, (m >> 16) & 255, (m >> 8) & 255); ev = 9; break;
        case 4: snprintf(out, 28, "%s HUNT lock %d", t, -(40 + (int)((h >> 5) % 9))); ev = 9; break;
        default: snprintf(out, 28, "%s HUNT end %ds", t, 18 + (int)((h >> 4) % 40)); ev = 9; break;
        }
    } else {
        switch (i % 8) {
        case 0: snprintf(out, 28, "%s BOOT v1.2.26", t); ev = 9; break;
        case 1: snprintf(out, 28, "%s SD mount ok", t); break;
        case 2: snprintf(out, 28, "%s TOUCH cal ok", t); break;
        case 3: snprintf(out, 28, "%s WARN SD %dms", t, 180 + (int)(h % 90)); ev = 9; break;
        case 4: snprintf(out, 28, "%s FLASH ok 2.6MB", t); ev = 9; break;
        case 5: snprintf(out, 28, "%s WIFI radio on", t); break;
        case 6: snprintf(out, 28, "%s ERR BLE init -1", t); ev = 9; break;
        default: snprintf(out, 28, "%s REBOOT sw", t); ev = 9; break;
        }
    }
    return ev;
}
#if LV_CYD27_LOGSUB
static inline bool lv_ls_active(void); static void lv_ls_rows(void); static bool lv_ls_stale(void);   /* CYD-27 mock, defined in lv_pager_logsub.h */
#if LV_CYD27_LOGSUB_B
static void lv_lsb_panel(void);
#endif
#endif
static void lv_lb_render(void)
{
    const int PW = LV_CB_PW, PH = LV_CB_PH;
    const uint16_t fill = lv_rgb(3, 5, 9);
    for (int by = 0; by < PH; ++by) for (int bx = 0; bx < PW; ++bx) {   /* plate: metal frame (same ring geometry as lv_pg_pan_plate) on a near-black field */
        const bool o = (bx == 0 || bx == PW - 1 || by == 0 || by == PH - 1), o2 = (bx == 1 || bx == PW - 2 || by == 1 || by == PH - 2);
        uint16_t c = fill;
        if (o || o2) { const uint16_t m = lv_lb_metal(150 + (int)(60.f * cosf((float)(bx + by) * 0.07f)), bx * 110 / PW, by * 110 / PH); c = o ? m : lv_mix(m, fill, 5, 8); }
        lv_c17_buf[by][bx] = c;
    }
    const int PX0 = LV_CB_PX0, PY0 = LV_CB_PY0;
#if LV_CYD27_LOGSUB && LV_CYD27_LOGSUB_B
    if (lv_ls_active()) { lv_lsb_panel(); return; }   /* CYD-27 B: skip the SCAN/SYS tab strip; the plate is the control chips */
#endif
    /* tab strip: label of the open tab is lit + underlined, the other is dim; EX.DATA marks every number below as sample data */
    const char *L0 = "SCAN", *L1 = "SYS";
    for (const char *s = L0; *s; ++s) lv_c17_char(PX0 + 6 + (int)(s - L0) * 6, 224, *s, lv_lb_tab == 0 ? C17_TAG : C17_MID, lv_lb_tab == 0 ? C17_T1 : C17_NONE, C17_NONE);
    for (const char *s = L1; *s; ++s) lv_c17_char(PX0 + 36 + (int)(s - L1) * 6, 224, *s, lv_lb_tab == 1 ? C17_TAG : C17_MID, lv_lb_tab == 1 ? C17_T1 : C17_NONE, C17_NONE);
    const char *EX = "EX.DATA";
    for (const char *s = EX; *s; ++s) lv_c17_char(PX0 + 104 + (int)(s - EX) * 6, 224, *s, C17_MID, C17_NONE, C17_NONE);
    for (int x = PX0 + 3; x <= PX0 + PW - 4; ++x) lv_c17_put(C17_T1, x, 233);   /* tab baseline */
    const int ux = lv_lb_tab == 0 ? PX0 + 6 : PX0 + 36, uw = lv_lb_tab == 0 ? 24 : 18;
    for (int x = ux; x < ux + uw; ++x) { lv_c17_put(C17_HOT, x, 232); lv_c17_put(C17_HOT, x, 233); }
#if LV_CYD27_LOGSUB && !LV_CYD27_LOGSUB_B
    if (lv_ls_active()) { lv_ls_rows(); return; }   /* CYD-27 A: scrolling rows, same text and roles */
#endif
    /* log rows: 5 newest, newest at the bottom, older rows step down in brightness.
     * age_ms is settled (>=240) so the arriving-row snap flare does not replay. */
    const int a = lv_lb_arrivals(), age_ms = 240;
    for (int k = 0; k < 5; ++k) {
        const int idx = a + k, y = 237 + k * 9, age = 4 - k;
        char buf[28]; const int ev = lv_lb_row(lv_lb_tab, idx + 8, buf);
        const int base = age == 0 ? C17_TAG : age <= 2 ? C17_BODY : age == 3 ? C17_HOT : C17_MID;
        for (int c = 0; buf[c]; ++c) {
            int role = base;
            if (c < 8) role = age == 0 ? C17_HOT : C17_MID;                       /* timestamp one step down */
            if (ev >= 0 && c >= ev && c < ev + 5 && buf[c] != ' ') role = age == 4 ? C17_HOT : C17_LEAD;   /* event word (HUNT/ERR/BOOT...) pops */
            if (age == 0 && age_ms < 240 && c >= 8) role = C17_LEAD;               /* snap flare on the arriving row */
            lv_c17_char(PX0 + 4 + c * 6 - 0, y, buf[c], role, age == 0 ? C17_T1 : C17_NONE, C17_NONE);
        }
    }
}
static uint32_t lv_lb_key = 0xFFFFFFFFu; static int lv_lb_keytab = -1, lv_lb_keyslot = -1;
static void lv_lb_panel_row(uint16_t *row, int y)
{
    if (y < LV_CB_PY0 || y >= LV_CB_PY0 + LV_CB_PH) return;
    /* Rebuild only when the tab or theme slot changes, not every lv_now tick. */
#if LV_CYD27_LOGSUB
    if (lv_ls_active() && lv_ls_stale()) lv_lb_key = 0xFFFFFFFFu;
#endif
    const int slot = lv_cb_slot();
    if (lv_lb_keytab != lv_lb_tab || lv_lb_keyslot != slot || lv_lb_key == 0xFFFFFFFFu) {
        lv_lb_key = 1; lv_lb_keytab = lv_lb_tab; lv_lb_keyslot = slot; lv_lb_render();
    }
    memcpy(row + LV_CB_PX0, lv_c17_buf[y - LV_CB_PY0], LV_CB_PW * 2);
}
#endif
