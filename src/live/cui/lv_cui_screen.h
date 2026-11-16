/* fw-3locks-1116: CONTROLbot Color UI v6 screen (LOCKED a87fe22 / 585810c, Mobandy 2026-11-16 12:59 PM ET "Lock it in").
 * LV_SC_CUI draws the locked v6 frame of the selected theme (lv_cui_data.h: the 24 locked stills, RGB565, ticker rect blank) and the
 * live tinted ticker on top (lv_cui_tint.h). Tap the big face = next theme in the locked order (as tap_cycle_24themes_ticker@3x.mp4);
 * the theme is applied through lv_set_theme's NVS path (lktheme, now 0..25) so it persists and every CYD screen follows it.
 * BACKbot (bottom-right, drawn in the locked frame) = back home. Nothing else on the frame is a live control (mock controls). */
#pragma once
#include "lv_cui_data.h"
/* the locked v6 order (render_v6_ticker.py ORDER/NAMES) -> this firmware's theme index */
static const uint8_t LV_CUI_FW[LV_CUI_N] = {22, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0, 24, 2, 3, 23, 5, 6, 7, 8, 9, 10, 11, 25};
static const char *const LV_CUI_NAME[LV_CUI_N] = {"CIRCUIT", "HI-VIS BLUE", "NV GREEN BLOOM", "NV WHITE PHOSPHOR", "IR IRONBOW", "IR RAINBOW HEAT",
    "IR WHITE-HOT", "IR RED-HOT", "ALARM STROBE", "POLICE SIREN", "PERIMETER GRID", "NEON GRAPE", "HOLO FOIL", "TOXIC", "ICE", "SONAR",
    "BLAZE ORANGE", "GOLD RUSH", "SYNTHWAVE", "AURORA", "PRISM", "GLITCH", "HOT ROSE", "DIGI CAMO"};
static const LkIo *lv_cui_io = nullptr;   /* set by lv_tick: the clip inflater + its LK_INFL_CAP buffer (no clip plays on this screen) */
static int lv_cui_cached = -1;            /* theme * 20 + strip held in io->buf */
static bool lv_cui_dirty = true;
/* the locked slot for the current firmware theme; HOT PINK / BLUE (dropped from v6) show the slots that replaced them */
static int lv_cui_slot(void)
{
    for (int k = 0; k < LV_CUI_N; ++k) if (LV_CUI_FW[k] == lv_theme) return k;
    return lv_theme == 1 ? 12 : lv_theme == 4 ? 15 : 0;   /* HOT PINK -> HOLO FOIL slot, CYD BLUE -> SONAR slot (LOCK.md) */
}
static const uint8_t *lv_cui_strip(int slot, int s)
{
    if (!lv_cui_io) return nullptr;
    const int key = slot * LV_CUI_NSTRIP + s;
    if (key != lv_cui_cached) {
        const uint32_t a = LV_CUI_OFF[key], b = LV_CUI_OFF[key + 1];
        const int n = lv_cui_io->inflate(LV_CUI_Z + a, b - a, lv_cui_io->buf, LK_INFL_CAP);
        if (n != 240 * LV_CUI_STRIP * 2) { lv_cui_cached = -1; return nullptr; }
        lv_cui_cached = key;
    }
    return lv_cui_io->buf;
}
static void lv_cui_scene(LvBand &B, void *)
{
    const int slot = lv_cui_slot();
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        const uint8_t *st = lv_cui_strip(slot, y / LV_CUI_STRIP);
        uint16_t *o = B.px + (y - B.y0) * B.w;
        if (!st) { for (int x = 0; x < B.w; ++x) o[x] = 0; continue; }
        const uint8_t *r = st + (y % LV_CUI_STRIP) * 240 * 2;
        for (int x = B.x0; x < B.x0 + B.w; ++x) o[x - B.x0] = (uint16_t)(r[2 * x] | (r[2 * x + 1] << 8));   /* byte loads: io->buf has no 16-bit alignment */
    }
}
static void lv_cui_enter(void) { lv_cui_dirty = true; lv_cui_cached = -1; }
static void lv_cui_frame(bool full)   /* the locked frames are final colours: lv_blit_hw (no sentinel re-ink) */
{
    if (full || lv_cui_dirty) {
        lv_cui_cached = -1;   /* io->buf may have been used by a clip since the last frame */
        lv_render(lv_blit_hw, LvRect{0, 0, 239, 0}, lv_cui_scene, 0);
        lv_render(lv_blit_hw, LvRect{0, 1, 0, 23}, lv_cui_scene, 0);
        lv_render(lv_blit_hw, LvRect{239, 1, 239, 23}, lv_cui_scene, 0);
        lv_render(lv_blit_hw, LvRect{0, 24, 239, 319}, lv_cui_scene, 0);
        lv_cui_dirty = false;
    }
    lv_ticker_tick();
}
static const LvRect LV_CUI_FACE = {53, 78, 187, 212};   /* the big chip face in the locked frame (135 x 135 finger target) */
static const char *lv_cui_tap(int x, int y)
{
    if (lv_in(LV_R_BACKBOT_HIT, x, y)) return lv_back();
    if (lv_in(LV_CUI_FACE, x, y)) {
        if (lv_theme_bounce()) return "theme-debounce";   /* same 250 ms guard as the CONTROLbot face */
        const int k = (lv_cui_slot() + 1) % LV_CUI_N;
        lv_set_theme_any(LV_CUI_FW[k]); lv_cui_dirty = true;   /* no ticker event: the banner keeps the firmware text (LOCK.md) */
        return "cui-theme";
    }
    return "none";
}
