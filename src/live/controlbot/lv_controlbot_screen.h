/* fw-3locks-1116: CONTROLbot v6 screen (LOCKED a87fe22 / 585810c, Mobandy 2026-11-16 12:59 PM ET "Lock it in").
 * LV_SC_CONTROLBOT draws the locked v6 frame of the selected theme (lv_controlbot_data.h: the 24 locked stills, RGB565, ticker rect blank) and the
 * live tinted ticker on top (lv_controlbot_tint.h). Tap the big face = next theme in the locked order (as tap_cycle_24themes_ticker@3x.mp4);
 * the theme is applied through lv_set_theme's NVS path (lktheme, now 0..25) so it persists and every CYD screen follows it.
 * BACKbot (bottom-right, drawn in the locked frame) = back home. The control panel is live (lv_controlbot_panel.h); a face tap flashes the
 * square bot itself (x54..185, y79..210: face + its black frame), nothing outside it (Mobandy 2026-11-16 2:20 PM ET). */
#pragma once
#include "lv_controlbot_data.h"
#include "lv_controlbot_panel.h"   /* the live control panel (SCAN / VOL / BRIGHT / CHIME / TIME / BAR / TONE) */
/* the locked v6 order (render_v6_ticker.py ORDER/NAMES) -> this firmware's theme index */
static const uint8_t LV_CB_FW[LV_CB_N] = {22, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0, 24, 2, 3, 23, 5, 6, 7, 8, 9, 10, 11, 25};
static const char *const LV_CB_NAME[LV_CB_N] = {"CIRCUIT", "HI-VIS BLUE", "NV GREEN BLOOM", "NV WHITE PHOSPHOR", "IR IRONBOW", "IR RAINBOW HEAT",
    "IR WHITE-HOT", "IR RED-HOT", "ALARM STROBE", "POLICE SIREN", "PERIMETER GRID", "NEON GRAPE", "HOLO FOIL", "TOXIC", "ICE", "SONAR",
    "BLAZE ORANGE", "GOLD RUSH", "SYNTHWAVE", "AURORA", "PRISM", "GLITCH", "HOT ROSE", "DIGI CAMO"};
static const LkIo *lv_cb_io = nullptr;   /* set by lv_tick: the clip inflater + its LK_INFL_CAP buffer (no clip plays on this screen) */
static int lv_cb_cached = -1;            /* theme * 20 + strip held in io->buf */
static bool lv_cb_dirty = true;
/* the locked slot for the current firmware theme; HOT PINK / BLUE (dropped from v6) show the slots that replaced them */
static int lv_cb_slot(void)
{
    for (int k = 0; k < LV_CB_N; ++k) if (LV_CB_FW[k] == lv_theme) return k;
    return lv_theme == 1 ? 12 : lv_theme == 4 ? 15 : 0;   /* HOT PINK -> HOLO FOIL slot, CYD BLUE -> SONAR slot (LOCK.md) */
}
static const uint8_t *lv_cb_strip(int slot, int s)
{
    if (!lv_cb_io) return nullptr;
    const int key = slot * LV_CB_NSTRIP + s;
    if (key != lv_cb_cached) {
        const uint32_t a = LV_CB_OFF[key], b = LV_CB_OFF[key + 1];
        const int n = lv_cb_io->inflate(LV_CB_Z + a, b - a, lv_cb_io->buf, LK_INFL_CAP);
        if (n != 240 * LV_CB_STRIP * 2) { lv_cb_cached = -1; return nullptr; }
        lv_cb_cached = key;
    }
    return lv_cb_io->buf;
}
static const LvRect LV_CB_BOT = {54, 79, 185, 210};   /* the square bot: face + black frame, exact outline in every v6 still */
static bool lv_cb_flash_on = false; static uint32_t lv_cb_flash_t = 0;
/* v1.1 (Mobandy 2026-11-16 4:36 PM ET: controls feel unresponsive, only the small writing changes): a tapped control block flashes
 * inverted for LV_CB_BLK_MS - its dark plate takes the theme's value ink (re-ink role 4 at that pixel, so multi-colour themes stay
 * themselves) and its light ink goes to the plate colour - then shows its new value. Only that block's rect is touched. */
#define LV_CB_BLK_MS 200
static const LvRect LV_CB_BLK[7] = {{46, 221, 113, 242}, {46, 244, 113, 265}, {46, 267, 113, 288},     /* SCAN, VOL, BRIGHT (68 x 22 bars) */
                                    {116, 221, 154, 254}, {157, 221, 195, 254}, {116, 256, 154, 289}, {157, 256, 195, 289}};   /* CHIME, TIME, BAR, TONE */
static bool lv_cb_blk_on = false; static uint32_t lv_cb_blk_t = 0; static LvRect lv_cb_blk = {0, 0, -1, -1};
static uint16_t lv_cb_invert(uint16_t c, int x, int y)
{
    const int r = (c >> 11) << 1, g = (c >> 5) & 63, b = (c & 31) << 1, mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    return mx < 32 ? lv_cb_reink(4, lv_hex(CB_VALUE), x, y, x, y) : lv_hex(CB_PLATE);   /* dark -> theme ink, light -> plate */
}
static void lv_cb_scene(LvBand &B, void *)
{
    const int slot = lv_cb_slot();
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        const uint8_t *st = lv_cb_strip(slot, y / LV_CB_STRIP);
        uint16_t *o = B.px + (y - B.y0) * B.w;
        if (!st) { for (int x = 0; x < B.w; ++x) o[x] = 0; continue; }
        const uint8_t *r = st + (y % LV_CB_STRIP) * 240 * 2;
        for (int x = B.x0; x < B.x0 + B.w; ++x) o[x - B.x0] = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));   /* byte loads: io->buf has no 16-bit alignment */
        if (lv_cb_pan && lv_cb_pan_slot == slot && y >= LV_CB_PY0 && y < LV_CB_PY0 + LV_CB_PH)   /* live panel over the locked one */
            for (int x = B.x0 > LV_CB_PX0 ? B.x0 : LV_CB_PX0; x < B.x0 + B.w && x < LV_CB_PX0 + LV_CB_PW; ++x) o[x - B.x0] = lv_cb_pan[(y - LV_CB_PY0) * LV_CB_PW + x - LV_CB_PX0];
        if (lv_cb_blk_on && lv_cb_pan && y >= lv_cb_blk.y0 && y <= lv_cb_blk.y1)   /* v1.1 tap feedback: the tapped control block inverts in the theme ink */
            for (int x = B.x0 > lv_cb_blk.x0 ? B.x0 : lv_cb_blk.x0; x < B.x0 + B.w && x <= lv_cb_blk.x1; ++x) o[x - B.x0] = lv_cb_invert(o[x - B.x0], x, y);
        if (lv_cb_flash_on && y >= LV_CB_BOT.y0 && y <= LV_CB_BOT.y1)   /* tap flash: the square bot only */
            for (int x = B.x0 > LV_CB_BOT.x0 ? B.x0 : LV_CB_BOT.x0; x < B.x0 + B.w && x <= LV_CB_BOT.x1; ++x) o[x - B.x0] = lv_hi(o[x - B.x0]);
    }
}
static void lv_cb_enter(void) { lv_cb_dirty = true; lv_cb_cached = -1; }
static void lv_cb_frame(bool full)   /* the locked frames are final colours: lv_blit_hw (no sentinel re-ink) */
{
    const int slot = lv_cb_slot();
    if (full || lv_cb_dirty || lv_cb_panel_dirty || lv_cb_pan_slot != slot) lv_cb_panel_build(slot);   /* heap only while drawing */
    if (full || lv_cb_dirty) {
        lv_cb_cached = -1;   /* io->buf may have been used by a clip since the last frame */
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 0}, lv_cb_scene, 0);
        lv_render(lv_blit_hw, LvRect{0, 1, 0, 23}, lv_cb_scene, 0);
        lv_render(lv_blit_hw, LvRect{239, 1, 239, 23}, lv_cb_scene, 0);
        lv_render(lv_blit_hw, LvRect{0, 24, 239, 319}, lv_cb_scene, 0);
        lv_cb_dirty = false;
    } else if (lv_cb_pan) {   /* a setting changed: repaint the panel only */
        lv_cb_cached = -1;
        lv_render(lv_blit_hw, LvRect{LV_CB_PX0, LV_CB_PY0, LV_CB_PX0 + LV_CB_PW - 1, LV_CB_PY0 + LV_CB_PH - 1}, lv_cb_scene, 0);
    }
    if (lv_cb_blk_on && (int32_t)(lv_now - lv_cb_blk_t) >= LV_CB_BLK_MS) { lv_cb_blk_on = false; lv_cb_panel_dirty = true; }   /* v1.1: flash over -> next frame shows the new value plain */
    if (lv_cb_flash_on && (int32_t)(lv_now - lv_cb_flash_t) >= 130) {   /* end of the tap flash (same 130 ms as the firmware press flash) */
        lv_cb_flash_on = false; lv_cb_cached = -1; lv_render(lv_blit_hw, LV_CB_BOT, lv_cb_scene, 0);
    }
    lv_cb_panel_free();
    lv_ticker_tick();
}
static const LvRect LV_CB_FACE = {53, 78, 187, 212};   /* the big chip face in the locked frame (135 x 135 finger target) */
static const char *lv_controlbot_tap(int x, int y)
{
    if (lv_in(LV_R_BACKBOT_HIT, x, y)) return lv_back();
    if (lv_in(LV_CB_FACE, x, y)) {
        if (lv_theme_bounce()) return "theme-debounce";   /* same 250 ms guard as the CONTROLbot face */
        const int k = (lv_cb_slot() + 1) % LV_CB_N;
        lv_set_theme_any(LV_CB_FW[k]); lv_cb_dirty = true;   /* no ticker event: the banner keeps the firmware text (LOCK.md) */
        lv_cb_flash_on = true; lv_cb_flash_t = lv_now;       /* flash the square bot only (LV_CB_BOT) */
        lv_print("CONTROLbot THEME %s", LV_CB_NAME[k]);
        return "controlbot-theme";
    }
    /* v1.1: a tap in the 1-2 px gaps between blocks or just outside the panel used to do nothing. Like every other screen
     * (LV_PAD / lv_pad_snap), a tap within LV_TAP_PAD px of a block now snaps to the nearest block. The guard against a bouncy
     * contact double-stepping is 150 ms (was 250 ms, which swallowed quick deliberate repeat taps on VOL / BRIGHT). */
    lv_pad_snap(LV_CB_BLK, 7, x, y);
    static uint32_t tap_t = 0; static bool tapped = false;
    auto hit = [&](int x0, int y0, int x1, int y1) { return x >= x0 && x <= x1 && y >= y0 && y <= y1; };
    int blk = -1; for (int k = 0; k < 7; ++k) if (lv_in(LV_CB_BLK[k], x, y)) { blk = k; break; }
    if (blk >= 0) {
        if (tapped && (int32_t)(lv_now - tap_t) < 150) return "panel-debounce";
        tapped = true; tap_t = lv_now;
        lv_cb_blk = LV_CB_BLK[blk]; lv_cb_blk_on = true; lv_cb_blk_t = lv_now; lv_cb_panel_dirty = true;   /* flash the tapped block */
    }
    LvCbSet &s = lv_cb_set;
    if (hit(46, 221, 113, 242)) { s.scan = (s.scan + 1) % LV_CB_NSCAN; lv_cb_changed(); lv_print("CONTROLbot SCAN %s", LV_CB_SCAN_NAME[s.scan]); return "cb-scan"; }
    if (hit(46, 244, 113, 265)) { const int d = x < 80 ? -10 : 10; const int v = s.vol + d; s.vol = v < 0 ? 0 : v > 100 ? 100 : v; lv_cb_changed(); lv_cb_play(); lv_print("CONTROLbot VOL %d%%", s.vol); return d < 0 ? "cb-vol-" : "cb-vol+"; }
    if (hit(46, 267, 113, 288)) { const int d = x < 80 ? -10 : 10; const int v = s.bri + d; s.bri = v < 10 ? 10 : v > 100 ? 100 : v; lv_cb_changed(); lv_print("CONTROLbot BRIGHT %d%%", s.bri); return d < 0 ? "cb-bright-" : "cb-bright+"; }
    if (hit(116, 221, 154, 254)) { s.chime = !s.chime; lv_cb_changed(); if (s.chime) lv_cb_play(); lv_print("CONTROLbot CHIME %s", s.chime ? "ON" : "OFF"); return "cb-chime"; }
    if (hit(157, 221, 195, 254)) { s.time = !s.time; lv_cb_changed(); lv_print("CONTROLbot TIME %s", LV_CB_TIME_NAME[s.time]); return "cb-time"; }
    if (hit(116, 256, 154, 289)) { s.bar = (s.bar + 1) % LV_CB_NBAR; lv_cb_changed(); lv_print("CONTROLbot BAR %s", LV_CB_BAR_NAME[s.bar]); return "cb-bar"; }
    if (hit(157, 256, 195, 289)) { s.tone = (s.tone + 1) % LV_CB_NTONE; lv_cb_changed(); lv_cb_play(); lv_print("CONTROLbot TONE %s", LV_CB_TONE_NAME[s.tone]); return "cb-tone"; }
    return "none";
}
