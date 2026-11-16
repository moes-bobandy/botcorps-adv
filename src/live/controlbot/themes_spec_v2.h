#pragma once
/* SECURITYbot Color UI theme set v2 - SPEC (THEME_LOCK 2026-11-09 8:18 AM ET: 11 themes, names + order locked) (mock only, not wired into firmware).
 * Mirrors the native BOTCORPS-CYD theme system in cyd-portrait-wt/src/live/lv_core.h (LV_NTHEME tables
 * LV_ACC / LV_ACCD / LV_HDR / LV_FRAME, LV_THEME_NAME <= 5 chars for the 31 px GLCD field, LV_THEME_FULL,
 * lv_multi_theme, lv_mix3, lv_theme_at, lv_multi_ink, lv_blit_theme sentinel re-ink).
 * Cycled by tapping the CONTROLbot big face. 2 solids (LIME, HIVIS) + 9 designed multi-colour themes.
 * Needs lv_rgb / lv_hex from lv_draw.h. Every pattern is integer-only (no float, no trig, no sqrt) so it is
 * cheap enough to run per pixel inside lv_blit_theme on the ESP32. */
#include <stdint.h>
#define SEC_NTHEME 11
enum { SEC_LIME, SEC_HIVIS, SEC_NVG, SEC_NVW, SEC_IRON, SEC_HEAT, SEC_WHOT, SEC_REDHT, SEC_ALARM, SEC_SIREN, SEC_PERIM };
static const char *const SEC_THEME_NAME[SEC_NTHEME] = {"LIME", "HIVIS", "NVG", "NVWHT", "IRON", "HEAT", "WHHOT", "REDHT", "ALARM", "SIREN", "PERIM"};
static const char *const SEC_THEME_FULL[SEC_NTHEME] = {"LIME GREEN", "HI-VIS BLUE", "NV GREEN BLOOM", "NV WHITE PHOSPHOR", "IR IRONBOW",
                                                       "IR RAINBOW HEAT", "IR WHITE-HOT", "IR RED-HOT", "ALARM STROBE", "POLICE SIREN", "PERIMETER GRID"};
/* solids: real inks. multi (2..10): representative inks that double as lv_blit_theme sentinels (re-inked per pixel). */
static const uint32_t SEC_ACC[SEC_NTHEME]   = {0x7fff00, 0x0096ff, 0x8cff3c, 0xe4eef6, 0xff7a10, 0xffe000, 0xd0d4d8, 0xff5a14, 0xff1418, 0xff1e2e, 0xb050ff};
static const uint32_t SEC_ACCD[SEC_NTHEME]  = {0x346900, 0x003d69, 0x396918, 0x5d6165, 0x693206, 0x695c00, 0x55575a, 0x692508, 0x690809, 0x690c13, 0x482068};
static const uint32_t SEC_HDR[SEC_NTHEME]   = {0xb2ff66, 0x66c0ff, 0xbaff8a, 0xeff5f9, 0xffaf70, 0xffec66, 0xe2e5e7, 0xff9c72, 0xff7274, 0xff787f, 0xcf96ff};
static const uint32_t SEC_FRAME[SEC_NTHEME] = {0x7fff00, 0x0096ff, 0x8cff3c, 0xe4eef6, 0xff7a10, 0xffe000, 0xd0d4d8, 0xff5a14, 0xff1418, 0xff1e2e, 0xb050ff};
static bool sec_multi_theme(int t) { return t >= SEC_NVG && t <= SEC_PERIM; }

static uint32_t sec_mix3(uint32_t a, uint32_t b, int num, int den)   /* == lv_mix3 */
{
    uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) { const int ca = (a >> sh) & 255, cb = (b >> sh) & 255; o |= (uint32_t)(ca + (cb - ca) * num / den) << sh; }
    return o;
}
/* k in 0..255 across n evenly spaced stops */
static uint32_t sec_lut(const uint32_t *s, int n, int k)
{
    if (k < 0) k = 0; if (k > 255) k = 255;
    const int seg = 255 / (n - 1), i = k / seg >= n - 1 ? n - 2 : k / seg;
    return sec_mix3(s[i], s[i + 1], k - i * seg, seg);
}
static int sec_iabs(int v) { return v < 0 ? -v : v; }
/* bloom distance from screen centre (diamond, NVG tube look): 0 centre .. 255 edge */
static int sec_bloom(int x, int y) { const int d = (sec_iabs(x - 120) * 5 + sec_iabs(y - 160) * 4) / 4; return d > 255 ? 255 : d; }
/* thermal heat 255 = hottest (screen centre) .. 0 (corners); elliptical, squared distance, no sqrt */
static int sec_heat(int x, int y) { const int dx = x - 120, dy = (y - 160) * 3 / 4, d = (dx * dx + dy * dy) / 113; return d > 255 ? 0 : 255 - d; }

/* infrared palettes as a function of heat k (0 cold .. 255 hot); shared by the face patterns and the v2 panel ramp */
static uint32_t sec_ir_at(int t, int k)
{
    if (k < 0) k = 0; if (k > 255) k = 255;
    switch (t) {
    case SEC_IRON: { static const uint32_t S[7] = {0x14051e, 0x4a0f7a, 0xa0148c, 0xe8305a, 0xff7a10, 0xffc030, 0xfff6c0}; return sec_lut(S, 7, k); }
    case SEC_HEAT: { static const uint32_t S[8] = {0x1a1aff, 0x0090ff, 0x00e0e0, 0x20e040, 0xb0f000, 0xffe000, 0xff8000, 0xff1010}; return S[k * 8 / 256]; }
    case SEC_WHOT: { const int g = 0x30 + (k * 6 / 256) * (0xff - 0x30) / 5; return (uint32_t)g << 16 | (uint32_t)g << 8 | (uint32_t)g; }
    case SEC_REDHT: { static const uint32_t S[6] = {0x200404, 0x7a0a08, 0xd81810, 0xff5a14, 0xffb060, 0xffffff}; return sec_lut(S, 6, k); }
    default: return SEC_ACC[t];
    }
}
static bool sec_ir_theme(int t) { return t >= SEC_IRON && t <= SEC_REDHT; }
static uint32_t sec_theme_at(int t, int x, int y)
{
    switch (t) {
    case SEC_NVG: {   /* Gen3 green-yellow intensifier: white-hot bloom centre -> phosphor -> dark tube edge, every 4th row a scanline */
        static const uint32_t S[5] = {0xf2ffd8, 0xb4ff50, 0x6cf02a, 0x2c9a18, 0x0e4a0a};
        const uint32_t c = sec_lut(S, 5, sec_bloom(x, y));
        return (y % 4 == 3) ? sec_mix3(c, 0, 35, 100) : c;
    }
    case SEC_NVW: {   /* white phosphor: neutral white bloom -> cool slate edge, scanlines + sparse photon sparkle */
        static const uint32_t S[5] = {0xffffff, 0xe4eef6, 0xafc2d2, 0x6c8096, 0x2a3644};
        if ((x * 37 + y * 101) % 97 == 0) return 0xffffff;
        const uint32_t c = sec_lut(S, 5, sec_bloom(x, y));
        return (y % 4 == 3) ? sec_mix3(c, 0, 30, 100) : c;
    }
    case SEC_IRON:    /* FLIR-style ironbow: black-violet cold edge -> purple -> magenta -> orange -> yellow-white hotspot */
    case SEC_HEAT:    /* rainbow thermal, posterised into 8 isotherm bands (blue cold -> red hot) */
        return sec_ir_at(t, sec_heat(x, y));
    case SEC_WHOT: {  /* white-hot greyscale, 6 posterised levels with a dark isotherm contour on each band edge */
        const int h = sec_heat(x, y), lv = h * 6 / 256;
        const uint32_t c = sec_ir_at(t, h);
        return ((h * 6) % 256 < 14 && lv > 0) ? sec_mix3(c, 0x101418, 55, 100) : c;
    }
    case SEC_REDHT: { /* red-hot vertical fade (cold top -> white-hot bottom) with heat-shimmer offsets every 5 rows */
        const int k = y * 255 / 319 + (((y / 5) & 1) ? ((x & 15) < 8 ? 10 : -10) : 0);
        return sec_ir_at(t, k);
    }
    case SEC_ALARM:   /* hazard strobe: diagonal red / red / white bands. v2 fix 1: 14 px bands (was 8) - calmer */
        return ((x + y) / 14) % 3 == 2 ? 0xffffff : 0xff1418;
    case SEC_SIREN: { /* police light bar: red left | white seam | blue right, LED module gaps every 15 rows */
        const uint32_t c = x < 116 ? 0xff1e2e : x > 124 ? 0x2f6bff : 0xffffff;
        return (y % 15 == 14) ? sec_mix3(c, 0, 45, 100) : c;
    }
    case SEC_PERIM: { /* perimeter grid. v2 fix 4: UV laser-tripwire violet rings (bright / deep) + pale-violet 20 px grid
                       * lines, moved off native ICE cyan (was 00E8FF / 00A0B8 / C8FFFF) */
        if (x % 20 == 0 || y % 20 == 0) return 0xf0d8ff;
        const int r = sec_iabs(x - 120) > sec_iabs(y - 160) * 3 / 4 ? sec_iabs(x - 120) : sec_iabs(y - 160) * 3 / 4;
        return ((r / 10) & 1) ? 0x6a20c0 : 0xb050ff;
    }
    default: return SEC_ACC[t];
    }
}
/* v2: ink for CONTROLbot panel chrome (bars, meters, buttons, labels).
 * fix 3 (IR themes): each left bar (SCAN / VOL / BRIGHT, x 46..113) runs the FULL heat palette cold->hot
 *   left->right (a thermal scale bar; meter segments step through it), and the 4 buttons take 4 rising steps of the
 *   same palette (CHIME ~107, BAR ~145, TIME ~169, TONE ~207 of 255), instead of only the outer edge of the face blob.
 * fix 1 (ALARM): panel chrome is flat alarm red so labels never sit on stripes (face + chip frame keep the strobe).
 * every other theme: unchanged (= sec_theme_at at screen position, as v1). */
static uint32_t sec_panel_at(int t, int x, int y)
{
    if (sec_ir_theme(t)) {
        if (x < 115) return sec_ir_at(t, 24 + (x - 46) * 231 / 67);              /* left bars: full cold->hot scale per bar */
        return sec_ir_at(t, 60 + (x - 116) * 120 / 79 + (y - 221) * 75 / 69);   /* buttons: CHIME < BAR < TIME < TONE steps */
    }
    if (t == SEC_ALARM) return 0xff1418;
    return sec_theme_at(t, x, y);
}
/* WCAG relative luminance x1000 of 0xRRGGBB (approx sRGB linearisation via integer table-free square law) */
static int sec_lum1000(uint32_t c)
{
    const int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    return (2126 * r * r / 65025 + 7152 * g * g / 65025 + 722 * b * b / 65025) / 10;
}
/* legibility lift for text / meter / badge inks on the 0x080808 plate: mix toward white until lum >= ~0.25 */
static uint32_t sec_legible(uint32_t c)
{
    for (int k = 0; k < 10 && sec_lum1000(c) < 250; ++k) c = sec_mix3(c, 0xffffff, 1, 5);
    return c;
}
/* == lv_multi_ink with sec_theme_at: cyan accent pixel -> theme ink at (x, y), keeps shading (mx) + white lift (R) */
static uint16_t sec_multi_ink(int t, uint16_t c, int x, int y)
{
    const int R = (c >> 11) << 3, G = ((c >> 5) & 63) << 2, Bl = (c & 31) << 3;
    const int mx = G > Bl ? G : Bl, mn = G < Bl ? G : Bl;
    if (mx < 12 || R * 10 > mn * 7 || Bl * 10 < G * 6 || G * 10 < Bl * 5) return c;
    const uint32_t T = sec_theme_at(t, x, y);
    const int W = R * 255 / mx;
    int o[3];
    for (int k = 0; k < 3; ++k) { const int tc = (int)((T >> (16 - 8 * k)) & 255); o[k] = (tc * (255 - W) + 255 * W) / 255 * mx / 255; }
    return lv_rgb((uint8_t)o[0], (uint8_t)o[1], (uint8_t)o[2]);
}
/* CONTROLbot face is the theme preview (native: lv_tx LV_TM_HUE maps the fade over the face height): the face
 * square (65..174, 88..197) samples the WHOLE 240x320 pattern so every theme shows its full design on the face. */
/* integer scales (x2, x3) so scanlines / grids stay regular on the face instead of aliasing into thick bands */
static void sec_face_map(int px, int py, int *fx, int *fy)
{
    const int x = (px - 65) * 2 + 10, y = (py - 88) * 3 - 5;
    *fx = x < 0 ? 0 : x > 239 ? 239 : x; *fy = y < 0 ? 0 : y > 319 ? 319 : y;
}
/* == lv_blit_theme sentinel re-ink: HDR -> mix(T, white, 2/5), ACCD -> mix(T, black, 59/100), ACC / FRAME -> T.
 * Text-bearing roles (HDR, ACC) additionally pass sec_legible. */
static uint16_t sec_sentinel(int t, uint16_t v, int x, int y)
{
    const uint16_t sa = lv_hex(SEC_ACC[t]), sf = lv_hex(SEC_FRAME[t]), sh = lv_hex(SEC_HDR[t]), sd = lv_hex(SEC_ACCD[t]);
    if (v != sa && v != sf && v != sh && v != sd) return v;
    const uint32_t T = sec_panel_at(t, x, y);   /* v2: panel ink (sentinels are only ever drawn by the panel) */
    return (v == sh) ? lv_hex(sec_legible(sec_mix3(T, 0xffffff, 2, 5))) : (v == sd) ? lv_hex(sec_mix3(T, 0x000000, 59, 100))
         : (v == sa) ? lv_hex(sec_legible(T)) : lv_hex(T);
}
