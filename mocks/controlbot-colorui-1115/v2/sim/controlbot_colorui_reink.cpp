/* CONTROLbot Color UI reink v2 (mock 2026-11-15 9:45 PM ET fixes: per-pad sampling, min brightness, label contrast, solid-CYD shimmer).
 * CONTROLbot Color UI reink (mock 2026-11-15). Themes 0..10 = SECURITYbot Color UI v2 (security_themes_reink_v2.cpp cases, verbatim);
 * themes 11..22 = the CYD CONTROLbot themes (lv_core.h LV_THEME_*, lv_theme_at) through the same role pipeline.
 * NEW roles: 8 = coil / pin pad (gold) -> theme pattern at the mapped point, keeps the gold's shading; 9 = grey pin label -> legible theme pattern.
 * usage: reink theme base.rgb role.u8 out.rgb samp.i16 */
#define LV_HOST 1
#include "lv_draw.h"
#include "themes_spec_v2.h"
#include "cyd_themes_lv_core_copy.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
static int g_t;   /* 0..22 */
static uint32_t th_at(int x, int y) { return g_t < SEC_NTHEME ? sec_theme_at(g_t, x, y) : lv_theme_at(g_t - SEC_NTHEME, x, y); }
static uint32_t th_panel_at(int x, int y) { return g_t < SEC_NTHEME ? sec_panel_at(g_t, x, y) : lv_theme_at(g_t - SEC_NTHEME, x, y); }
static uint16_t cyd_multi_ink(uint16_t c, int x, int y)   /* sec_multi_ink with the CYD theme */
{
    const int R = (c >> 11) << 3, G = ((c >> 5) & 63) << 2, Bl = (c & 31) << 3;
    const int mx = G > Bl ? G : Bl, mn = G < Bl ? G : Bl;
    if (mx < 12 || R * 10 > mn * 7 || Bl * 10 < G * 6 || G * 10 < Bl * 5) return c;
    const uint32_t T = th_at(x, y); const int W = R * 255 / mx; int o[3];
    for (int k = 0; k < 3; ++k) { const int tc = (int)((T >> (16 - 8 * k)) & 255); o[k] = (tc * (255 - W) + 255 * W) / 255 * mx / 255; }
    return lv_rgb((uint8_t)o[0], (uint8_t)o[1], (uint8_t)o[2]);
}
static uint16_t cyd_sentinel(int role, int x, int y)   /* sec_sentinel with the CYD theme; solid CYD themes use their real LV_HDR / LV_ACCD */
{
    const int c = g_t - SEC_NTHEME; const bool multi = lv_multi_theme(c); const uint32_t T = th_panel_at(x, y);
    if (role == 3) return lv_hex(sec_legible(multi ? sec_mix3(T, 0xffffff, 2, 5) : LV_HDR[c]));
    if (role == 5) return lv_hex(multi ? sec_mix3(T, 0x000000, 59, 100) : LV_ACCD[c]);
    return lv_hex(sec_legible(T));
}
/* coil / pins: the theme pattern mapped across each strip so the whole design shows (like sec_face_map does for the face) */
static void cp_map(int x, int y, int *fx, int *fy)
{
    int X, Y;
    if (y < 64 && x >= 30 && x < 210) { X = (x - 30) * 240 / 180; Y = (y - 28) * 320 / 34; }        /* coil strip */
    else if (y >= 292 && x >= 31 && x < 209) { X = (x - 31) * 240 / 178; Y = 150 + (y - 292); }     /* bottom pads: centre row profile */
    else if (x < 120) { X = x * 120 / 31; Y = (y - 25) * 320 / 291; }                              /* left pins */
    else { X = 120 + (x - 209) * 120 / 31; Y = (y - 25) * 320 / 291; }                             /* right pins */
    *fx = X < 0 ? 0 : X > 239 ? 239 : X; *fy = Y < 0 ? 0 : Y > 319 ? 319 : Y;
}
static uint32_t lift(uint32_t c, int lum) { for (int k = 0; k < 12 && sec_lum1000(c) < lum; ++k) c = sec_mix3(c, 0xffffff, 1, 5); return c; }
static bool solid_cyd(void) { return g_t >= SEC_NTHEME && !lv_multi_theme(g_t - SEC_NTHEME); }
/* v2: solid CYD themes get a stepped shimmer from their OWN real inks: darker ends (ACCD toward ACC) -> ACC -> HDR highlight mid-strip */
static uint32_t solid_pat(int x, int y)
{
    const int c = g_t - SEC_NTHEME; int u;   /* 0..255 along the strip */
    if (y < 64 && x >= 30 && x < 210) u = (x - 30) * 255 / 179; else if (y >= 292 && x >= 31 && x < 209) u = (x - 31) * 255 / 177; else u = (y - 25) * 255 / 290;
    if (u < 0) u = 0; if (u > 255) u = 255; const int k = 255 - (u > 127 ? (u - 128) * 2 : (127 - u) * 2);   /* 0 at the ends .. 255 mid */
    const uint32_t d = sec_mix3(LV_ACCD[c], LV_ACC[c], 2, 5);
    return k < 128 ? sec_mix3(d, LV_ACC[c], k, 127) : sec_mix3(LV_ACC[c], LV_HDR[c], k - 128, 127);
}
static uint32_t cp_ink(int sx, int sy) { if (solid_cyd()) return solid_pat(sx, sy); int fx, fy; cp_map(sx, sy, &fx, &fy); return th_at(fx, fy); }
int main(int argc, char **argv)
{
    if (argc != 6) return 2;
    const int t = atoi(argv[1]); g_t = t; const int N = 240 * 320;
    std::vector<uint8_t> rgb(N * 3), role(N);
    FILE *f = fopen(argv[2], "rb"); if (!f || fread(rgb.data(), 1, N * 3, f) != (size_t)N * 3) return 3; fclose(f);
    f = fopen(argv[3], "rb"); if (!f || fread(role.data(), 1, N, f) != (size_t)N) return 4; fclose(f);
    std::vector<int16_t> samp(N * 2);
    f = fopen(argv[5], "rb"); if (!f || fread(samp.data(), 2, N * 2, f) != (size_t)N * 2) return 5; fclose(f);
    int gmax = 1; for (int i = 0; i < N; ++i) if (role[i] == 8) { const int m = rgb[i * 3] > rgb[i * 3 + 1] ? rgb[i * 3] : rgb[i * 3 + 1]; if (m > gmax) gmax = m; }
    const bool sec = t < SEC_NTHEME;
    for (int i = 0; i < N; ++i) {
        const int x = i % 240, y = i / 240; uint16_t c = lv_rgb(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]), o = c;
        switch (role[i]) {
        case 1: { int fx, fy; sec_face_map(x, y, &fx, &fy); o = sec ? sec_multi_ink(t, c, fx, fy) : cyd_multi_ink(c, fx, fy); break; }
        case 2: { const int R = (c >> 11) << 3, Bl = (c & 31) << 3, mx = R > Bl ? R : Bl; const uint32_t T = th_at(x, y);
                  o = lv_rgb((uint8_t)(((T >> 16) & 255) * mx / 255), (uint8_t)(((T >> 8) & 255) * mx / 255), (uint8_t)((T & 255) * mx / 255)); break; }
        case 3: o = sec ? sec_sentinel(t, lv_hex(SEC_HDR[t]), x, y) : cyd_sentinel(3, x, y); break;
        case 4: o = sec ? sec_sentinel(t, lv_hex(SEC_ACC[t]), x, y) : cyd_sentinel(4, x, y); break;
        case 5: o = sec ? sec_sentinel(t, lv_hex(SEC_ACCD[t]), x, y) : cyd_sentinel(5, x, y); break;
        case 6: o = lv_hex(0x080808); break;
        case 7: o = sec ? sec_sentinel(t, lv_hex(SEC_ACC[t]), samp[i * 2], samp[i * 2 + 1]) : cyd_sentinel(4, samp[i * 2], samp[i * 2 + 1]); break;
        case 8: { const int sx = samp[i * 2], sy = samp[i * 2 + 1]; uint32_t T = cp_ink(sx, sy);   /* v2: one sample per pad / coil tooth */
                  T = lift(T, 200);                                                                   /* v2: min brightness on the board */
                  int m = rgb[i * 3] > rgb[i * 3 + 1] ? rgb[i * 3] : rgb[i * 3 + 1]; m = m * 255 / gmax; if (m > 255) m = 255;
                  o = lv_rgb((uint8_t)(((T >> 16) & 255) * m / 255), (uint8_t)(((T >> 8) & 255) * m / 255), (uint8_t)((T & 255) * m / 255)); break; }
        case 9: o = lv_hex(lift(cp_ink(samp[i * 2], samp[i * 2 + 1]), 400)); break;                 /* v2: label = its pad's step, contrast floor */
        default: break;
        }
        if (!role[i]) continue;
        rgb[i * 3] = (uint8_t)((o >> 11) << 3); rgb[i * 3 + 1] = (uint8_t)(((o >> 5) & 63) << 2); rgb[i * 3 + 2] = (uint8_t)((o & 31) << 3);
    }
    f = fopen(argv[4], "wb"); fwrite(rgb.data(), 1, N * 3, f); fclose(f);
    return 0;
}
