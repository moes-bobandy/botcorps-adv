/* VERBATIM copy of /workspace/cyd-portrait-wt/src/live/lv_core.h lines 98-130 (commit 049ff59): the CYD CONTROLbot theme table (LV_NTHEME 12) + lv_multi_theme / lv_mix3 / lv_theme_at. Copied, not edited. */
#define LV_NTHEME 12
static const char *const LV_THEME_NAME[LV_NTHEME] = {"GRAPE", "MAGENTA", "TOXIC", "ICE", "BLUE", "ORANGE", "GOLD", "SYNTH", "AURA", "PRISM", "GLTCH", "ROSE"};
static const char *const LV_THEME_FULL[LV_NTHEME] = {"NEON GRAPE", "HOT PINK", "TOXIC", "ICE", "BLUE", "BLAZE ORANGE", "GOLD RUSH", "SYNTHWAVE", "AURORA", "PRISM", "GLITCH", "HOT ROSE"};
#define LV_THEME_ORIG 6          /* first re-inked theme */
#define LV_THEME_BASE 3          /* ICE: the clip the re-inked themes play */
/* Theme inks sampled from the LOCKED SCANbot/TERMINALbot clips (frame 0 of each theme loop). 6..11: the
 * originals' accents (botcorps_rgb565 of ac); on the multi themes (7..10) these four values are sentinels
 * that lv_blit_theme re-inks per pixel, so every overlay stroke follows the fade / spectrum / split. */
static const uint32_t LV_ACC[LV_NTHEME] = {0xb324fd, 0xdd00ac, 0x92ff02, 0x00ffff, 0x345efb, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052};
static const uint32_t LV_ACCD[LV_NTHEME] = {0x53205d, 0x68004f, 0x4a6010, 0x00636d, 0x0e5166, 0x61480f, 0x6a5a00, 0x690d69, 0x0d690d, 0x086908, 0x690405, 0x690022};
static const uint32_t LV_HDR[LV_NTHEME] = {0xb36afe, 0xdc4aac, 0xb9fb4f, 0x4afffc, 0x6891ff, 0xf4b24c, 0xffe86a, 0xff6afd, 0x6aff6b, 0x6afe6c, 0xff6a6b, 0xff6a96};
static const uint32_t LV_FRAME[LV_NTHEME] = {0xff25ae, 0xdf00ae, 0x92fd00, 0x00feff, 0xfd22ff, 0xfd6b00, 0xffda00, 0xff24fd, 0x20fe01, 0x00fe02, 0xfe0001, 0xff0052};
static bool lv_multi_theme(int t) { return t >= 7 && t <= 10; }
/* the theme's ink at screen pixel (x, y), 0xRRGGBB (originals: th_ink_at / sel_ink in botcorps_ui.cpp) */
static uint32_t lv_mix3(uint32_t a, uint32_t b, int num, int den)
{
    uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) { const int ca = (a >> sh) & 255, cb = (b >> sh) & 255; o |= (uint32_t)(ca + (cb - ca) * num / den) << sh; }
    return o;
}
static uint32_t lv_theme_at(int t, int x, int y)
{
    switch (t) {
    case 7: return lv_mix3(0xff24ff, 0xff8918, y, 319);                                    /* SYNTHWAVE: magenta top -> sunset orange */
    case 8: {                                                                                 /* AURORA: ice sky -> green -> violet */
        const int k = y < 40 ? 40 : y > 280 ? 280 : y;
        return k < 160 ? lv_mix3(0x00daff, 0x20ff00, k - 40, 120) : lv_mix3(0x20ff00, 0xb400ff, k - 160, 120);
    }
    case 9: { static const uint32_t H[6] = {0xff24ff, 0xff8918, 0xffda00, 0x20ff00, 0x00ffff, 0x3161ff}; return H[((x + y) / 22) % 6]; } /* PRISM */
    case 10: return ((y / 3) % 4 == 1) ? 0x00ffff : 0xff0000;                                /* GLITCH: red, cyan split bands */
    default: return LV_ACC[t];
    }
}
