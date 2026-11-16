/* v4: theme index 0..26 (0..10 SEC, 11..22 CYD, 23..26 NEW). v3 alert bars (mock 2026-11-16): dump the selected Color UI theme's ink across the header rows y1..23, x1..238.
 * Same theme sources as the v2 CONTROLbot reink: sec_theme_at (themes_spec_v2.h, locked 2b79d83) for 0..10, lv_theme_at (lv_core.h 049ff59 copy) for 11..22.
 * The 320-row face pattern is squeezed into the 23 header rows so gradients / splits show across the bar; min brightness = same lift as v2 coil/pins.
 * usage: bar_theme_ink theme out.rgb */
#define LV_HOST 1
#include "lv_draw.h"
#include "themes_spec_v2.h"
#include "cyd_themes_lv_core_copy.h"
#include "new_themes_v3.h"   /* v4: + the 4 NEW themes (23..26) */
#include <cstdio>
#include <cstdlib>
static int g_t;
static uint32_t th_at(int x, int y) { return g_t < SEC_NTHEME ? sec_theme_at(g_t, x, y) : ext_theme_at(g_t - SEC_NTHEME, x, y); }
static uint32_t lift(uint32_t c, int lum) { for (int k = 0; k < 12 && sec_lum1000(c) < lum; ++k) c = sec_mix3(c, 0xffffff, 1, 5); return c; }
int main(int argc, char **argv)
{
    g_t = atoi(argv[1]); FILE *f = fopen(argv[2], "wb");
    for (int y = 1; y < 24; ++y) for (int x = 1; x < 239; ++x) {
        const int fy = 7 + (y - 1) * 306 / 22; const uint32_t T = lift(th_at(x, fy), 200);
        const unsigned char p[3] = {(unsigned char)(T >> 16), (unsigned char)(T >> 8), (unsigned char)T}; fwrite(p, 1, 3, f);
    }
    fclose(f); return 0;
}
