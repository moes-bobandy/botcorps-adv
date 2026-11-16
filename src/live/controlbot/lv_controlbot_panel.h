/* fw-3locks-1116 (Mobandy 2026-11-16 2:18 PM ET: "It's the new control bot ui ... The control panel operates and everything").
 * The CONTROLbot v6 control panel (x45..196, y220..290) drawn LIVE from its settings, with the SAME pipeline that made the locked v6 stills:
 *   - base art = render_security_color_themes_v2.py paint_v2() panel (v9c draw_theme_bar / draw_cycle_btn / draw_meter, LV_GLCD 5x7), RGB565;
 *   - re-ink = controlbot_colorui_reink.cpp roles 3/4/5/6/7 (label+frame / value+lit / unlit / white text / badge fill), copied below;
 *   - theme = the locked v6 ORDER (render_controlbot_colorui.py ORDER) for the current slot.
 * At the default settings (SCAN B.RAIN, VOL 40%, BRIGHT 80%, CHIME ON, TIME RTC, BAR HAZRD, TONE BEEP) this reproduces the locked
 * stills' panel pixel for pixel (tools: docs/builds/fw-3locks-1116 proof). Settings persist in NVS ("botcorps": cbscan, cbvol, ...). */
#pragma once
/* ---- settings ---- */
struct LvCbSet { uint8_t scan, vol, bri, chime, time, bar, tone; };
static LvCbSet lv_cb_set = {0, 40, 80, 1, 0, 3, 0};   /* v6 defaults (the locked stills) */
#define LV_CB_NSCAN 3
static const char *const LV_CB_SCAN_NAME[LV_CB_NSCAN] = {"B.RAIN", "S.RAIN", "OFF"};   /* this build: GIBSON code rain DRIFT / SWEEP / off */
static const char *const LV_CB_TIME_NAME[2] = {"RTC", "NTP"};
#define LV_CB_NBAR 5
static const char *const LV_CB_BAR_NAME[LV_CB_NBAR] = {"RAIN", "DECOD", "TICK", "HAZRD", "HUD"};
#define LV_CB_NTONE 3
static const char *const LV_CB_TONE_NAME[LV_CB_NTONE] = {"BEEP", "8BIT", "SIREN"};
static void (*lv_cb_save_hook)(const LvCbSet &) = nullptr;   /* lv_ui.cpp: NVS */
static void (*lv_cb_bright_hook)(int pct) = nullptr;         /* lv_ui.cpp: backlight */
static void (*lv_cb_tone_hook)(int hz, int vol) = nullptr;   /* lv_ui.cpp: speaker (hz 0 = silent) */
static bool lv_cb_panel_dirty = true;

/* ---- re-ink, copied from controlbot_colorui_reink.cpp (v6-cyd-matrix4/sim), g_t -> lv_cb_g_t ---- */
static int lv_cb_g_t;
static uint32_t lv_cb_th_panel_at(int x, int y) { return lv_cb_g_t < SEC_NTHEME ? sec_panel_at(lv_cb_g_t, x, y) : ext_theme_at(lv_cb_g_t - SEC_NTHEME, x, y); }
static uint16_t lv_cb_cyd_sentinel(int role, int x, int y)
{
    const int c = lv_cb_g_t - SEC_NTHEME; const bool multi = ext_multi(c); const uint32_t T = lv_cb_th_panel_at(x, y);
    if (role == 3) return lv_hex(sec_legible(multi ? sec_mix3(T, 0xffffff, 2, 5) : EXT_HDR[c]));
    if (role == 5) return lv_hex(multi ? sec_mix3(T, 0x000000, 59, 100) : EXT_ACCD[c]);
    return lv_hex(sec_legible(T));
}
static uint16_t lv_cb_reink(int role, uint16_t c, int x, int y, int sx, int sy)
{
    const int t = lv_cb_g_t; const bool sec = t < SEC_NTHEME;
    switch (role) {
    case 3: return sec ? sec_sentinel(t, lv_hex(SEC_HDR[t]), x, y) : lv_cb_cyd_sentinel(3, x, y);
    case 4: return sec ? sec_sentinel(t, lv_hex(SEC_ACC[t]), x, y) : lv_cb_cyd_sentinel(4, x, y);
    case 5: return sec ? sec_sentinel(t, lv_hex(SEC_ACCD[t]), x, y) : lv_cb_cyd_sentinel(5, x, y);
    case 6: return lv_hex(0x080808);
    case 7: return sec ? sec_sentinel(t, lv_hex(SEC_ACC[t]), sx, sy) : lv_cb_cyd_sentinel(4, sx, sy);
    default: return c;
    }
}
/* v6 slot -> re-ink theme index (render_controlbot_colorui.py ORDER) */
static const uint8_t LV_CB_REINK_T[24] = {23, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 25, 13, 14, 24, 16, 17, 18, 19, 20, 21, 22, 26};

/* ---- panel canvas (heap, only while drawing) ---- */
#define LV_CB_PX0 45
#define LV_CB_PY0 220
#define LV_CB_PW 152
#define LV_CB_PH 71
static uint16_t *lv_cb_pan = nullptr;   /* final RGB565 of the panel, rebuilt when settings / theme change */
static int lv_cb_pan_slot = -1;
enum { CB_PLATE = 0x080808, CB_KEY = 0x9449ad, CB_VALUE = 0xde92ff, CB_SEGOFF = 0x210c31, CB_CAP = 0x8c9aa5, CB_WHITE = 0xffffff };
struct LvCbRect { int x0, y0, x1, y1; };   /* exclusive end */
static LvCbRect lv_cb_r7[5]; static int lv_cb_n7 = 0;   /* badge / value-box rects: role 4 -> 7, sampled at the centre */
static void lv_cb_put(int x, int y, uint32_t rgb, bool txt)
{
    if (x < LV_CB_PX0 || x >= LV_CB_PX0 + LV_CB_PW || y < LV_CB_PY0 || y >= LV_CB_PY0 + LV_CB_PH) return;
    const uint16_t c = lv_hex(rgb); int role = 0, sx = x, sy = y;
    if (txt && rgb == CB_WHITE) role = 6;                         /* TXT sentinel (v2 base_and_roles) */
    else if (rgb == CB_KEY) role = 3; else if (rgb == CB_VALUE) role = 4; else if (rgb == CB_SEGOFF) role = 5;
    if (role == 4) for (int k = 0; k < lv_cb_n7; ++k) { const LvCbRect &r = lv_cb_r7[k];
        if (x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1) { role = 7; sx = (r.x0 + r.x1) / 2; sy = (r.y0 + r.y1) / 2; break; } }
    lv_cb_pan[(y - LV_CB_PY0) * LV_CB_PW + (x - LV_CB_PX0)] = lv_cb_reink(role, c, x, y, sx, sy);
}
static void lv_cb_fill(int x, int y, int w, int h, uint32_t c) { for (int j = y; j < y + h; ++j) for (int i = x; i < x + w; ++i) lv_cb_put(i, j, c, false); }
static void lv_cb_frame(int x, int y, int w, int h, uint32_t c) { lv_cb_fill(x, y, w, 1, c); lv_cb_fill(x, y + h - 1, w, 1, c); lv_cb_fill(x, y, 1, h, c); lv_cb_fill(x + w - 1, y, 1, h, c); }
static int lv_cb_tw(const char *s) { const int n = (int)strlen(s); return n ? 6 * n - 1 : 0; }
static void lv_cb_text(int x, int y, const char *s, uint32_t c)
{
    for (int i = 0; s[i]; ++i) { const int g = s[i] < 32 || s[i] > 126 ? 0 : s[i] - 32;
        for (int cx = 0; cx < 5; ++cx) { const uint8_t bits = LV_GLCD[g * 5 + cx]; for (int cy = 0; cy < 7; ++cy) if (bits >> cy & 1) lv_cb_put(x + 6 * i + cx, y + cy, c, true); } }
}
static void lv_cb_meter(int x, int y, int w, int h, int lit)
{
    lv_cb_fill(x, y, w, h, 0x000000); lv_cb_fill(x, y, 2, h, CB_CAP); lv_cb_fill(x + w - 2, y, 2, h, CB_CAP);
    const int ix = x + 3, iw = w - 6, sw = (iw - 7) / 8 > 2 ? (iw - 7) / 8 : 2;
    for (int i = 0; i < 8; ++i) lv_cb_fill(ix + i * (sw + 1), y, sw, h, i < lit ? CB_VALUE : CB_SEGOFF);
}
static void lv_cb_bar(int x, int y, const char *label, const char *value, int lit, bool box)
{
    const int w = 68, h = 22;
    lv_cb_fill(x, y, w, h, CB_PLATE); lv_cb_frame(x, y, w, h, CB_KEY);
    lv_cb_text(x + 3, y + 3, label, CB_KEY);
    const int vw = lv_cb_tw(value), vx = x + w - 3 - vw, ty = y + 3;
    if (box) { lv_cb_r7[lv_cb_n7++] = {vx - 1, ty - 1, vx + vw + 1, ty + 8}; lv_cb_fill(vx - 1, ty - 1, vw + 2, 9, CB_VALUE); lv_cb_text(vx, ty, value, CB_WHITE); }
    else lv_cb_text(vx, ty, value, CB_VALUE);
    lv_cb_meter(x + 3, y + 12, w - 6, 5, lit);
}
static void lv_cb_btn(int x, int y, const char *label, const char *value, bool on)
{
    const int w = 39, h = 34;
    lv_cb_r7[lv_cb_n7++] = {x + 3, y + 12, x + w - 3, y + h - 3};
    lv_cb_fill(x, y, w, h, CB_PLATE); lv_cb_frame(x, y, w, h, CB_KEY);
    lv_cb_text(x + (w - lv_cb_tw(label)) / 2, y + 3, label, CB_KEY);
    const int by = y + 12, bh = h - 15, bx = x + 3, bw = w - 6;
    lv_cb_fill(bx, by, bw, bh, on ? 0xde92ff : 0x202020); lv_cb_frame(bx, by, bw, bh, on ? 0xffffff : 0x606060);
    lv_cb_text(bx + (bw - lv_cb_tw(value)) / 2, by + (bh - 7) / 2, value, on ? 0xffffff : 0xb0b0b0);
}
static int lv_cb_lit(int pct) { const int n = (8 * pct + 50) / 100; return n < 1 ? 1 : n; }
static bool lv_cb_panel_build(int slot)
{
    if (!lv_cb_pan) lv_cb_pan = (uint16_t *)malloc(LV_CB_PW * LV_CB_PH * 2);
    if (!lv_cb_pan) return false;
    lv_cb_g_t = LV_CB_REINK_T[slot]; lv_cb_n7 = 0;
    const LvCbSet &s = lv_cb_set; char v[8], b[8];
    lv_cb_fill(LV_CB_PX0, LV_CB_PY0, LV_CB_PW, LV_CB_PH, CB_PLATE); lv_cb_frame(LV_CB_PX0, LV_CB_PY0, LV_CB_PW, LV_CB_PH, CB_KEY);
    lv_cb_bar(46, 221, "SCAN", LV_CB_SCAN_NAME[s.scan], s.scan + 1, true);
    snprintf(v, sizeof v, "%d%%", s.vol); lv_cb_bar(46, 244, "VOL", v, lv_cb_lit(s.vol), false);
    snprintf(b, sizeof b, "%d%%", s.bri); lv_cb_bar(46, 267, "BRIGHT", b, lv_cb_lit(s.bri), false);
    lv_cb_btn(116, 221, "CHIME", s.chime ? "ON" : "OFF", s.chime);
    lv_cb_btn(157, 221, "TIME", LV_CB_TIME_NAME[s.time], true);
    lv_cb_btn(116, 256, "BAR", LV_CB_BAR_NAME[s.bar], true);
    lv_cb_btn(157, 256, "TONE", LV_CB_TONE_NAME[s.tone], true);
    lv_cb_pan_slot = slot; lv_cb_panel_dirty = false;
    return true;
}
static void lv_cb_panel_free(void) { free(lv_cb_pan); lv_cb_pan = nullptr; lv_cb_pan_slot = -1; }

/* ---- sound: a tiny note sequencer (hardware hook in lv_ui.cpp; the sim logs it) ---- */
struct LvCbNote { uint16_t hz, ms; };
static const LvCbNote LV_CB_T_BEEP[] = {{2000, 80}, {0, 0}};
static const LvCbNote LV_CB_T_8BIT[] = {{1319, 60}, {1568, 60}, {2093, 90}, {0, 0}};
static const LvCbNote LV_CB_T_SIREN[] = {{800, 40}, {1000, 40}, {1200, 40}, {1400, 40}, {1600, 40}, {1400, 40}, {1200, 40}, {1000, 40}, {0, 0}};
static const LvCbNote *const LV_CB_TONES[LV_CB_NTONE] = {LV_CB_T_BEEP, LV_CB_T_8BIT, LV_CB_T_SIREN};
static const LvCbNote *lv_cb_snd = nullptr; static uint32_t lv_cb_snd_t = 0; static int lv_cb_snd_hz = -1;
static void lv_cb_out(int hz) { if (hz != lv_cb_snd_hz) { lv_cb_snd_hz = hz; if (lv_cb_tone_hook) lv_cb_tone_hook(hz, lv_cb_set.vol); } }
static void lv_cb_play(void) { if (!lv_cb_set.vol) return; lv_cb_snd = LV_CB_TONES[lv_cb_set.tone]; lv_cb_snd_t = lv_now; lv_cb_out(lv_cb_snd->hz); }
static void lv_cb_sound_tick(uint32_t now)
{
    if (!lv_cb_snd) return;
    while (lv_cb_snd->ms && (int32_t)(now - lv_cb_snd_t) >= lv_cb_snd->ms) { lv_cb_snd_t += lv_cb_snd->ms; ++lv_cb_snd; }
    if (!lv_cb_snd->ms) { lv_cb_snd = nullptr; lv_cb_out(0); return; }
    lv_cb_out(lv_cb_snd->hz);
}
static void lv_cb_alert_chime(void) { if (lv_cb_set.chime) lv_cb_play(); }   /* lv_event(alert): new alert find / attack */

/* ---- apply + save ---- */
static void lv_cb_apply(void)   /* push settings into the firmware state they drive */
{
    LvCbSet &s = lv_cb_set;
    if (s.scan >= LV_CB_NSCAN) s.scan = 0; if (s.vol > 100) s.vol = 40; s.vol -= s.vol % 10;
    if (s.bri < 10 || s.bri > 100) s.bri = 80; s.bri -= s.bri % 10; if (s.chime > 1) s.chime = 1; if (s.time > 1) s.time = 0;
    if (s.bar >= LV_CB_NBAR) s.bar = 3; if (s.tone >= LV_CB_NTONE) s.tone = 0;
    lv_rain = s.scan != 2; lv_rainmode = s.scan == 1 ? 1 : 0;   /* SCAN: GIBSON radar code rain (old RAIN + RAIN MODE chips) */
    if (lv_cb_bright_hook) lv_cb_bright_hook(s.bri);
}
static void lv_cb_changed(void) { lv_cb_apply(); if (lv_cb_save_hook) lv_cb_save_hook(lv_cb_set); lv_cb_panel_dirty = true; }
