#pragma once
/* MOCK 2026-11-12 - INSIDE TOWERS v2 (add-on candidate screens; NOTHING locked is changed).
 * Each screen is a VARIATION OF THE REAL GIBSON SCAN TOWER: it calls the radar's OWN scene
 * (lv_scene_radar: sky, skyline, 3D neon grid + camera drift, code rain, extruded towers, lock
 * brackets, mini-terminal, corner BACKbot) with the radar globals world-swapped (lv_hc_swap) for
 * a per-bot world of FAKE towers = that bot's real functions, plus a per-bot accent via lv_rd_skin:
 *   LV_MO_INSIDE LV_SC_MO  MORPHEUSbot "Dojo Construct"  tatami dojo + 12 glossary-device pillars,
 *                          tap -> RED PILL drops -> tower floods Matrix green w/ code rain -> folders.
 *   LV_CT_INSIDE LV_SC_CT  CONTROLbot "The Grid Plant"  theme-accent towers, height = current value,
 *                          lock a tower -> its control folders drop, tap a folder to CYCLE (real setters).
 *   LV_TR_INSIDE LV_SC_TR  TERMINALbot "The Relay Ring" silver towers = the 10 device bots, TERMINALbot
 *                          big at top centre, lock -> data beam -> lv_go_soon(0,"sweep <name>") (unchanged TTY).
 * The real scan list (lv_f) is swapped out only during these draws; with every switch 0 and LV_RD_SKINS 0
 * the firmware is byte-identical. Selecting reuses EXISTING screens (gloss description page / terminal). */
#include "lv_inside_faces.h"

#if LV_MO_INSIDE || LV_CT_INSIDE || LV_TR_INSIDE
static void lv_go_soon(int scr, const char *tty_cmd);
static void lv_draw_backbot(LvBand &B, int x0, int y0);

/* ---- shared sprite blit (locked bot-face pixels, lv_inside_faces.h) ---- */
static inline void itw_blit_sc(LvBand &B, int dx, int dy, int dw, int dh, int sw, int sh, const uint16_t *px)
{ for (int r = 0; r < dh; ++r) for (int c = 0; c < dw; ++c) b_pix(B, dx + c, dy + r, px[(r * sh / dh) * sw + (c * sw / dw)]); }
static inline void itw_shade(LvBand &B, int x, int y, int w, int h, uint16_t c, int k)
{ for (int r = 0; r < h; ++r) for (int cc = 0; cc < w; ++cc) b_blend(B, x + cc, y + r, c, k); }

/* ---- the per-bot world of fake towers (built on lv_hc_w, drawn via lv_hc_swap + lv_scene_radar) ---- */
static int  itw_nt = 0;                 /* tower count */
static char itw_lab[24][14];            /* base label */
static int  itw_icon[24];               /* device icon id (lv_icons.h), -1 none */
/* screen-space geometry captured each frame from lv_rd_tw (indexed by find/tower index) */
static int  itw_hx0[24], itw_hx1[24], itw_hy0[24], itw_hy1[24];   /* widened tap/overlay box */
static int  itw_fx0[24], itw_fx1[24], itw_fyt[24], itw_fyb[24];   /* front face box */
static bool itw_seen[24];
static float itw_camx = 0;                         /* eased camera pan (reference feel) */
static int  itw_lx0[24], itw_ly0[24], itw_lx1[24], itw_ly1[24]; static bool itw_lseen[24];  /* drawn label rects (overlap gate) */

/* place one fake tower at (approx) screen column tx, depth band [zlo,zhi], via the radar's own MAC-hash law */
static void itw_place(int idx, uint8_t pre, float tx, float zlo, float zhi, int rssi, int kind)
{
    LvHcWorld &w = lv_hc_w; LvFind &f = w.f[idx]; memset(&f, 0, sizeof f);
    for (int k = 0; k < 400000; ++k) {
        const uint8_t mac[6] = {pre, 0xA9, (uint8_t)(idx + 1), (uint8_t)(k >> 8), (uint8_t)k, (uint8_t)(idx * 7 + 3)};
        const uint32_t h = lv_rd_hash(mac);
        const float Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f;
        const float X = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * Z, sx = 120.0f * X / Z;
        if (Z < zlo || Z > zhi || fabsf(sx - tx) > 3.0f) continue;
        memcpy(f.mac, mac, 6); break;
    }
    if (!f.mac[0]) { const uint8_t m[6] = {pre, 0xA9, (uint8_t)(idx + 1), 0, 0, 0}; memcpy(f.mac, m, 6); }
    f.rssi = (int8_t)rssi; f.kind = (uint8_t)kind; f.mfg = 0xFFFF; snprintf(f.name, sizeof f.name, "%s", itw_lab[idx]);
}

/* capture this frame's tower screen boxes (called while swapped, after lv_rd_layout) */
static void itw_capture(void)
{
    for (int i = 0; i < 24; ++i) { itw_seen[i] = false; itw_lseen[i] = false; }
    for (int n = 0; n < lv_rd_n; ++n) {
        const LvTower &T = lv_rd_tw[n]; const int i = T.i; if (i < 0 || i >= itw_nt) continue;
        int xa, xb, ya, yb; lv_rd_box(T, 3, xa, xb, ya, yb);
        const int cx = (xa + xb) / 2, hw = (xb - xa) / 2 + 7 < 18 ? 18 : (xb - xa) / 2 + 7;   /* >= 36 px wide */
        itw_hx0[i] = cx - hw < 0 ? 0 : cx - hw; itw_hx1[i] = cx + hw > 239 ? 239 : cx + hw;
        itw_hy0[i] = ya - 6 < LV_RD_Y0 ? LV_RD_Y0 : ya - 6; itw_hy1[i] = yb + 6 > LV_RD_Y1 ? LV_RD_Y1 : yb + 6;
        itw_fx0[i] = T.x0; itw_fx1[i] = T.x1; itw_fyt[i] = T.yt; itw_fyb[i] = T.yb; itw_seen[i] = true;
    }
}
static int itw_hit(int x, int y)
{ for (int i = 0; i < itw_nt; ++i) if (itw_seen[i] && x >= itw_hx0[i] && x <= itw_hx1[i] && y >= itw_hy0[i] && y <= itw_hy1[i]) return i; return -1; }

/* draw each tower's device icon + label at its base (the radar draws no labels) */
static void itw_labels(LvBand &B, uint16_t ink, uint16_t plate, bool names)
{
    for (int i = 0; i < 24; ++i) itw_lseen[i] = false;
    for (int i = 0; i < itw_nt; ++i) {
        if (!itw_seen[i]) continue;
        const int cx = (itw_fx0[i] + itw_fx1[i]) / 2; int y = itw_fyb[i] + 2; if (y > LV_RD_Y1 - 9) y = LV_RD_Y1 - 9;
        const int w = lv_w35(itw_lab[i]) + (itw_icon[i] >= 0 ? 11 : 2);
        b_fill(B, cx - w / 2 - 1, y, w + 2, 8, plate);
        int tx = cx - w / 2;
        if (itw_icon[i] >= 0) { b_icon(B, tx, y, itw_icon[i]); tx += 11; }
        b_text35(B, tx, y + 1, itw_lab[i], ink);
        itw_lx0[i] = cx - w / 2 - 1; itw_ly0[i] = y; itw_lx1[i] = cx - w / 2 - 1 + w + 2; itw_ly1[i] = y + 8; itw_lseen[i] = true;
        (void)names;
    }
}

/* the real alert ticker on rows 1..23 + the black frame, exactly as every non-game screen keeps it */
static void itw_top(void)
{
    /* v1.2.5: do not black-clear the banner; that flashed the ticker every Dojo frame */
    lv_ticker_tick();
}
static const LvRect ITW_BACK_HIT = {210, 289, 239, 319};
static bool lv_term_from_mo = false;   /* v1.2.6: TERM opened from the Dojo footer; BACK returns to Dojo */

/* a per-bot header plate (over the radar header strip) with the LOCKED bot face + title */
static void itw_header(LvBand &B, const char *title, const uint16_t *face, int fw, int fh, uint16_t bg, uint16_t ink)
{
    (void)face; (void)fw; (void)fh;   /* v5: no floating bot chip; this header is our chrome, not locked shared chrome */
    b_fill(B, 0, 24, 240, 22, bg);
    b_fill(B, 0, 45, 240, 1, lv_dim(ink, 4, 8));
    b_text(B, 6, 31, title, ink);
}
#endif

/* =========================== MORPHEUSbot "Dojo Construct" (v11) =========================== */
/* v6 (2026-11-12): the whole MO screen lives in lv_morpheus_v6.h (v5 code: /workspace/cyd-inside-towers-wt) */
#include "lv_morpheus_v13.h"   /* v12 (2026-11-13, v11 Dojo idle + tap = close-up pillar left, line-item search bar on top, Dojo folders right); v11 / v10 / v9 / v8 / v7 / v6 kept on disk */

/* =========================== CONTROLbot "The Grid Plant" =========================== */
#if LV_CT_INSIDE
struct ItwCtl { const char *name; const char *(*val)(void); void (*cyc)(void); };
static const char *ctv_theme(void) { return LV_THEME_NAME[lv_theme]; }
static void ctc_theme(void) { if (lv_theme_bounce()) return; lv_set_theme(lv_theme + 1); lv_event(false, "THEME: %s", LV_THEME_NAME[lv_theme]); lv_tk_off = 1 << 14; }
static const char *ctv_orient(void) { return LV_FLIP_NAME[lv_flip]; }
static void ctc_orient(void) { lv_flip = !lv_flip; if (lv_radio->set_rot) lv_radio->set_rot(lv_flip); lv_event(false, "ORIENT: %s", LV_FLIP_NAME[lv_flip]); lv_tk_off = 1 << 14; }
static const char *ctv_motion(void) { return lv_motion ? "ON" : "OFF"; }
static void ctc_motion(void) { lv_motion = !lv_motion; lv_event(false, "BG MOTION: %s", lv_motion ? "ON" : "OFF"); lv_tk_off = 1 << 14; }
static const char *ctv_focus(void) { return LV_FOCUS_NAME[lv_focus]; }
static void ctc_focus(void) { lv_focus = (lv_focus + 1) % 4; lv_tk_item = 0; lv_event(false, "TICKER FOCUS: %s", LV_FOCUS_NAME[lv_focus]); lv_tk_off = 1 << 14; }
static const char *ctv_mashup(void) { return LV_SPEED_NAME[lv_speed]; }
static void ctc_mashup(void) { lv_speed = (lv_speed + 1) % 3; lv_event(false, "MASHUP TICKER: %s", LV_SPEED_NAME[lv_speed]); lv_tk_off = 1 << 14; }
static const char *ctv_mix(void) { return LV_MIX_NAME[lv_scanmix]; }
static void ctc_mix(void) { lv_scanmix = (lv_scanmix + 1) % 3; lv_event(false, "MIX (SCAN): %s", LV_MIX_NAME[lv_scanmix]); lv_tk_off = 1 << 14; }
static const char *ctv_auto(void) { return LV_AUTO_NAME[lv_auto]; }
static void ctc_auto(void) { lv_auto = (lv_auto + 1) % 3; lv_auto_t = lv_now; lv_event(false, "MULTI AUTO SCAN: %s", LV_AUTO_NAME[lv_auto]); lv_tk_off = 1 << 14; }
static const char *ctv_rain(void) { return LV_ONOFF[lv_rain]; }
static void ctc_rain(void) { lv_rain = !lv_rain; lv_event(false, "RAIN: %s", LV_ONOFF[lv_rain]); lv_tk_off = 1 << 14; }
static const char *ctv_rmode(void) { return LV_RAINMODE_NAME[lv_rainmode]; }
static void ctc_rmode(void) { lv_rainmode = (lv_rainmode + 1) % 3; lv_event(false, "RAIN MODE: %s", LV_RAINMODE_NAME[lv_rainmode]); lv_tk_off = 1 << 14; }
static const char *ctv_rcol(void) { return LV_RAINCOL_NAME[lv_raincol]; }
static void ctc_rcol(void) { lv_raincol = (lv_raincol + 1) % 3; lv_event(false, "RAIN COL: %s", LV_RAINCOL_NAME[lv_raincol]); lv_tk_off = 1 << 14; }
static const char *ctv_up(void) { static char s[24]; snprintf(s, sizeof s, "%lus", (unsigned long)(lv_now / 1000)); return s; }
static void ctc_up(void) { char h[48] = ""; lv_radio->sys(0, h, sizeof h); lv_event(false, "UP %lus  %s", (unsigned long)(lv_now / 1000), h); lv_tk_off = 1 << 14; }
struct ItwCtGroup { const char *name; int nctl; ItwCtl ctl[6]; int (*lvl)(void); };
static int ctl_theme_lvl(void) { return 4 + lv_theme * 56 / 11; }
static int ctl_ticker_lvl(void) { return 4 + lv_focus * 56 / 3; }
static int ctl_scan_lvl(void) { return 4 + lv_scanmix * 56 / 2; }
static int ctl_rain_lvl(void) { return 4 + lv_rainmode * 56 / 2; }
static int ctl_sys_lvl(void) { return 34; }
static ItwCtGroup ITW_CT_G[5] = {
    {"DISPLAY", 3, {{"THEME", ctv_theme, ctc_theme}, {"BG MOTION", ctv_motion, ctc_motion}, {"ORIENT", ctv_orient, ctc_orient}}, ctl_theme_lvl},
    {"TICKER", 2, {{"TICKER FOCUS", ctv_focus, ctc_focus}, {"MASHUP TICKER", ctv_mashup, ctc_mashup}}, ctl_ticker_lvl},
    {"SCAN", 2, {{"MIX (SCAN)", ctv_mix, ctc_mix}, {"MULTI AUTO SCAN", ctv_auto, ctc_auto}}, ctl_scan_lvl},
    {"RAIN FX", 3, {{"RAIN", ctv_rain, ctc_rain}, {"RAIN MODE", ctv_rmode, ctc_rmode}, {"RAIN COL", ctv_rcol, ctc_rcol}}, ctl_rain_lvl},
    {"SYSTEM", 1, {{"UPTIME", ctv_up, ctc_up}}, ctl_sys_lvl},
};
static int itw_ct_sel = -1;
static const LvRect ITW_CT_CLOSE = {150, 282, 236, 318};   /* 86x36 */
static void itw_ct_enter(void)
{
    LvHcWorld &w = lv_hc_w; memset(&w, 0, sizeof w);
    itw_nt = 5; itw_ct_sel = -1;
    static const float SX[5] = {-76, -38, 0, 38, 76};
    for (int i = 0; i < 5; ++i) { snprintf(itw_lab[i], sizeof itw_lab[i], "%s", ITW_CT_G[i].name); itw_icon[i] = -1;
        itw_place(i, 0x43, SX[i], 1.75f, 2.05f, -100 + ITW_CT_G[i].lvl(), i & 1); }
    w.nf = 5; w.t0 = lv_now; snprintf(w.tl[0], sizeof w.tl[0], "THE GRID PLANT  5 BANKS");
    snprintf(w.tl[1], sizeof w.tl[1], "HEIGHT = CURRENT VALUE");
    snprintf(w.tl[2], sizeof w.tl[2], "TAP A TOWER TO OPEN CONTROLS");
}
static void itw_ct_folders(LvBand &B, int g, int frame)
{
    const ItwCtGroup &G = ITW_CT_G[g]; const uint16_t acc = lv_hex(0x1fd4c4);   /* CT identity: teal */
    const int n = G.nctl, top = 146, fh = 30, gap = 6; const int H = 16 + n * (fh + gap);
    itw_shade(B, 0, top - 14, 240, H + 14, lv_hex(0x05070f), 7); b_fill(B, 0, top - 14, 240, 1, acc);
    char t[40]; snprintf(t, sizeof t, "%s BANK", G.name); b_text35(B, 6, top - 11, t, acc);
    for (int j = 0; j < n; ++j) {
        const int fy = top + j * (fh + gap); const int dx = (frame * 6 - j * 8); const int ox = dx < 0 ? 0 : dx > 10 ? 10 : dx;
        b_fill(B, 8 + ox, fy, 224, fh, lv_hex(0x0b0f1c)); b_frame(B, 8 + ox, fy, 224, fh, acc);
        b_text35(B, 12 + ox, fy + 4, G.ctl[j].name, lv_hex(0xdfe6f2));
        char v[40]; snprintf(v, sizeof v, "> %s", G.ctl[j].val()); b_text35(B, 12 + ox, fy + 14, v, acc);
        b_text35(B, 232 + ox - 20, fy + 14, g == 4 ? "" : "TAP+", lv_hex(0x80d6c9));
    }
    b_fill(B, ITW_CT_CLOSE.x0, ITW_CT_CLOSE.y0, ITW_CT_CLOSE.x1 - ITW_CT_CLOSE.x0, ITW_CT_CLOSE.y1 - ITW_CT_CLOSE.y0, lv_hex(0x0b0f1c));
    b_frame(B, ITW_CT_CLOSE.x0, ITW_CT_CLOSE.y0, ITW_CT_CLOSE.x1 - ITW_CT_CLOSE.x0, ITW_CT_CLOSE.y1 - ITW_CT_CLOSE.y0, acc);
    b_text35(B, ITW_CT_CLOSE.x0 + 10, ITW_CT_CLOSE.y0 + 14, "[ < BACK ]", acc);
}
static void itw_ct_scene(LvBand &B, void *)
{
    const int frame = (int)(lv_now / 33);
    lv_hc_swap();
    for (int i = 0; i < 5; ++i) lv_f[i].rssi = (int8_t)(-100 + ITW_CT_G[i].lvl());   /* keep height live */
    lv_rd_hunting = false;
    if (itw_ct_sel >= 0) { lv_rd_lock = true; lv_rd_lock_t = lv_now - 500; memcpy(lv_rd_lockmac, lv_f[itw_ct_sel].mac, 6); lv_rd_sel = -1; }
    else { lv_rd_lock = false; lv_rd_sel = -1; }
    lv_rd_skin_tw = 2.2f;
    if (itw_ct_sel < 0) lv_rd_camx = 0.30f * sinf(((lv_now - lv_rd_t0) / 1000.0f) * 0.45f); else lv_rd_camx = 0.0f;
    lv_rd_place(); lv_rd_clamp(); itw_capture();
    lv_rd_skin = 3; lv_rd_skin_acc = 0x1fd4c4; lv_rd_skin_fa = 7; lv_scene_radar(B, 0); lv_rd_skin = 0; lv_rd_skin_tw = 1.0f; lv_rd_skin_fa = 3;
    lv_hc_swap();
    itw_labels(B, lv_hex(0xdff5f0), lv_hex(0x05211f), true);
    itw_header(B, "CONTROLbot // GRID PLANT", ITW_CT_FACE, ITW_CT_W, ITW_CT_H, lv_hex(0x05211f), lv_hex(0x1fd4c4));
    if (itw_ct_sel >= 0) itw_ct_folders(B, itw_ct_sel, frame);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
static void itw_ct_frame(void) { lv_render(lv_blit, {0, 0, 239, 319}, itw_ct_scene, 0); itw_top(); }
static const char *itw_ct_tap(int x, int y)
{
    if (lv_in(ITW_BACK_HIT, x, y)) { lv_go_soon(LV_SC_HOME, 0); return "ct-back-home"; }
    if (y <= 23) { lv_go_soon(LV_SC_HOME, 0); return "ct-ticker"; }
    if (itw_ct_sel >= 0) {
        const ItwCtGroup &G = ITW_CT_G[itw_ct_sel]; const int top = 146, fh = 30, gap = 6;
        if (lv_in(ITW_CT_CLOSE, x, y)) { itw_ct_sel = -1; return "ct-close"; }
        for (int j = 0; j < G.nctl; ++j) { const int fy = top + j * (fh + gap);
            if (x >= 8 && x <= 232 && y >= fy && y <= fy + fh + gap - 1) { if (itw_ct_sel != 4) G.ctl[j].cyc(); return G.ctl[j].name; } }
        return "ct-drop";
    }
    const int i = itw_hit(x, y);
    if (i >= 0) { itw_ct_sel = i; return ITW_CT_G[i].name; }
    return "ct-none";
}
#endif

/* =========================== TERMINALbot "The Relay Ring" =========================== */
#if LV_TR_INSIDE
static int itw_tr_sel = -1; static uint32_t itw_tr_t0 = 0; static bool itw_tr_nav = false;
#define ITW_TR_BEAM 620
static void itw_tr_enter(void)
{
    LvHcWorld &w = lv_hc_w; memset(&w, 0, sizeof w);
    itw_nt = 10; itw_tr_sel = -1; itw_tr_nav = false;
    static const float NX[5] = {-76, -38, 0, 38, 76}, FX[5] = {-58, -28, 2, 32, 60};
    for (int i = 0; i < 10; ++i) {
        char up[14]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[i]); for (char *p = up; *p; ++p) *p = (char)toupper((unsigned char)*p);
        snprintf(itw_lab[i], sizeof itw_lab[i], "%s", up); itw_icon[i] = LV_HUNT_ICON[i];
        const bool near = i < 5; const int s = i % 5;
        itw_place(i, 0x44, near ? NX[s] : FX[s], near ? 2.15f : 3.3f, near ? 2.55f : 3.7f, near ? -48 : -72, i & 1);
    }
    w.nf = 10; w.t0 = lv_now; snprintf(w.tl[0], sizeof w.tl[0], "THE RELAY RING  10 DEVICES");
    snprintf(w.tl[1], sizeof w.tl[1], "TAP A RELAY TO SWEEP");
    snprintf(w.tl[2], sizeof w.tl[2], "BEAM -> TERMINAL");
}
static void itw_tr_beam(LvBand &B, int i, int frame)
{
    const int cx = (itw_fx0[i] + itw_fx1[i]) / 2, y0 = itw_fyt[i], y1 = 0;   /* beam exits the top of the screen */
    for (int y = y1; y <= y0; ++y) { b_blend(B, cx, y, lv_hex(0xeaf2ff), 8); b_blend(B, cx - 1, y, lv_hex(0x8fd0ff), 5); b_blend(B, cx + 1, y, lv_hex(0x8fd0ff), 5); }
    for (int k = 0; k < 10; ++k) { const int y = y0 - ((frame * 5 + k * 11) % (y0 - y1 + 1)); b_fill(B, cx - 2, y, 5, 2, lv_hex(0xffffff)); }
}
static void itw_tr_scene(LvBand &B, void *)
{
    const int frame = (int)(lv_now / 33);
    lv_hc_swap();
    lv_rd_hunting = false;
    if (itw_tr_sel >= 0) { lv_rd_lock = true; lv_rd_lock_t = lv_now - 500; memcpy(lv_rd_lockmac, lv_f[itw_tr_sel].mac, 6); lv_rd_sel = -1; }
    else { lv_rd_lock = false; lv_rd_sel = -1; }
    lv_rd_skin_tw = 1.35f;
    if (itw_tr_sel < 0) lv_rd_camx = 0.38f * sinf(((lv_now - lv_rd_t0) / 1000.0f) * 0.45f); else lv_rd_camx = 0.0f;
    lv_rd_place(); lv_rd_clamp(); itw_capture();
    lv_rd_skin = 2; lv_rd_skin_acc = 0; lv_rd_skin_fa = 6; lv_scene_radar(B, 0); lv_rd_skin = 0; lv_rd_skin_tw = 1.0f; lv_rd_skin_fa = 3;
    lv_hc_swap();   /* v5: NO floating bot. The beam exits the top of the screen, then the TTY transition. */
    itw_header(B, "TERMINALbot // RELAY RING", 0, 0, 0, lv_hex(0x06131f), lv_hex(0x9fe6ff));
    if (itw_tr_sel >= 0 && itw_seen[itw_tr_sel]) {
        const int i = itw_tr_sel, cx = (itw_fx0[i] + itw_fx1[i]) / 2; const int tw = lv_w35(itw_lab[i]) + 2;
        int ly = itw_fyt[i] - 11; if (ly < LV_RD_HOR + 2) ly = LV_RD_HOR + 2;
        b_fill(B, cx - tw / 2 - 2, ly, tw + 4, 9, lv_hex(0x06131f)); b_text35(B, cx - tw / 2, ly + 1, itw_lab[i], lv_hex(0x9fe6ff));
        itw_tr_beam(B, i, frame);
    }
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
static void itw_tr_frame(void)
{
    lv_render(lv_blit, {0, 0, 239, 319}, itw_tr_scene, 0);
    itw_top();
    if (itw_tr_sel >= 0 && !itw_tr_nav && (lv_now - itw_tr_t0) >= ITW_TR_BEAM) {
        itw_tr_nav = true; char c[24]; snprintf(c, sizeof c, "sweep %s", LV_HUNT_NAME[itw_tr_sel]); lv_go_soon(0, c);
    }
}
static const char *itw_tr_tap(int x, int y)
{
    if (lv_in(ITW_BACK_HIT, x, y)) { lv_go_soon(LV_SC_HOME, 0); return "tr-back-home"; }
    if (y <= 23) { lv_go_soon(LV_SC_HOME, 0); return "tr-ticker"; }
    if (itw_tr_sel >= 0) return "tr-beam";
    const int i = itw_hit(x, y);
    if (i >= 0) { itw_tr_sel = i; itw_tr_t0 = lv_now; itw_tr_nav = false; return LV_HUNT_NAME[i]; }
    return "tr-none";
}
#endif

#if LV_MO_INSIDE || LV_CT_INSIDE || LV_TR_INSIDE
/* sim-only: dump the active inside screen's state + tower tap boxes (sim_live 'itwstate') */
static void itw_dump(void)
{
    printf("ITW scr=%d nt=%d\n", lv_scr, itw_nt);
    for (int i = 0; i < itw_nt; ++i)
        printf("  T%d %-12s icon=%d seen=%d hit=[%d,%d,%d,%d] face=[%d,%d,%d,%d] lbl=%d[%d,%d,%d,%d]\n",
               i, itw_lab[i], itw_icon[i], itw_seen[i] ? 1 : 0,
               itw_hx0[i], itw_hy0[i], itw_hx1[i], itw_hy1[i], itw_fx0[i], itw_fyt[i], itw_fx1[i], itw_fyb[i],
               itw_lseen[i] ? 1 : 0, itw_lx0[i], itw_ly0[i], itw_lx1[i], itw_ly1[i]);
#if LV_MO_INSIDE
    if (lv_scr == LV_SC_MO) { printf("  MO sel=%d phase=%d\n", itw_mo_sel, itw_mo_phase); mo_dump(); }
#endif
#if LV_CT_INSIDE
    if (lv_scr == LV_SC_CT) printf("  CT sel=%d\n", itw_ct_sel);
#endif
#if LV_TR_INSIDE
    if (lv_scr == LV_SC_TR) printf("  TR sel=%d nav=%d\n", itw_tr_sel, itw_tr_nav ? 1 : 0);
#endif
}
#endif
