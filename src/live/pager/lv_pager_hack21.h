/* MOCK 2026-11-26 (CYD-21, sim-only, NOT in any bin): HACKbot home pager page, lime-green metallic plate + black code rain.
 * Off by default: LV_CYD21_HACK_LIME is 0, so firmware and v1.2.23 renders are byte-identical. The mock sim builds with -DLV_CYD21_HACK_LIME=1.
 * Reuses the locked LOGbot finish+black-rain pipeline (memcpy the house chip-plate, luminance -> metal ramp + sheen,
 * skip remapping c==0 eye sockets, then pitch-black rain with skip=1 via lv_pg_keep_feat). Only the colour ramp is swapped
 * to lime green. Source art is blob 2 (lv_pg_art(HACKbot) / the same chip-plate TERMINALbot and LOGbot already use).
 * Locked face features (the art's black eye sockets) stay black and on top of the rain. The panel, board, banner, pips
 * and every other page are untouched. */
#pragma once
#ifndef LV_CYD21_HACK_LIME
#define LV_CYD21_HACK_LIME 0
#endif
#if LV_CYD21_HACK_LIME
struct LvHbStop { int16_t l; uint8_t r, g, b; };
/* lum -> lime metal. Dark recesses stay near-black green; mid is rich lime (around LV_PG_LIME 0xC8FF10);
 * highlights wash toward pale mint, the same shadow/mid/highlight shape as lv_pg_gold / lv_lb_ramp. */
static const LvHbStop LV_HB_LIME[] = {
    {0,   2,  10,  1},
    {40,  6,  28,  2},
    {80,  16,  72,  4},
    {110, 40, 140,  8},
    {140, 100, 200, 12},
    {170, 180, 245, 16},
    {200, 200, 255, 16},
    {230, 228, 255, 110},
    {255, 246, 255, 200}
};
static uint16_t lv_hb_ramp(const LvHbStop *s, int n, int l)
{
    if (l <= s[0].l) return lv_rgb(s[0].r, s[0].g, s[0].b);
    for (int i = 1; i < n; ++i) if (l <= s[i].l) {
        const int d = s[i].l - s[i - 1].l, k = l - s[i - 1].l;
        return lv_rgb((uint8_t)((s[i - 1].r * (d - k) + s[i].r * k) / d),
                      (uint8_t)((s[i - 1].g * (d - k) + s[i].g * k) / d),
                      (uint8_t)((s[i - 1].b * (d - k) + s[i].b * k) / d));
    }
    return lv_rgb(s[n - 1].r, s[n - 1].g, s[n - 1].b);
}
/* Same spatial sheen as lv_lb_metal (chrome/copper), plus a slow traveling highlight so the finish animates. */
static uint16_t lv_hb_metal(int l, int u, int v)
{
    const float t = (float)(lv_now % 4000u) * (1.f / 4000.f);
    float L = (float)l
        + 24.f * cosf(((float)(u + v) - 70.f) * 0.045f)
        - 20.f * ((float)v / 110.f)
        + 6.f * sinf((float)u * 0.21f)
        + 22.f * cosf(((float)(u - v) * 0.08f) - t * 6.2832f);
    if (L < 0.f) L = 0.f; if (L > 255.f) L = 255.f;
    return lv_hb_ramp(LV_HB_LIME, (int)(sizeof LV_HB_LIME / sizeof *LV_HB_LIME), (int)L);
}
static void lv_hb_face_row(uint16_t *row, int y, const uint8_t *fr)
{
    const int X0 = LV_PG_FX_X0, ly = y - LV_PG_FX_Y0;
    if (fr) memcpy(row + X0, fr, LV_PG_FACE_S * 2);
    for (int lx = 0; lx < LV_PG_FACE_S; ++lx) {
        const uint16_t c = row[X0 + lx];
        if (c == 0) continue;   /* the art's black eye sockets stay black */
        const int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
        row[X0 + lx] = lv_hb_metal((r * 3 + g * 6 + b) / 10, lx, ly);
    }
    { const uint16_t pal[3] = { 0, 0, 0 }; lv_pg_rain_row(row, y, pal, 1, LV_PG_HACKBOT); }   /* same pitch-black rain as gold TERMINALbot / oil-slick LOGbot; skip=1 keeps the sockets on top */
}
#endif
