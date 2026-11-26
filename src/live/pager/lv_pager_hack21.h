/* MOCK 2026-11-26 (CYD-21 v2, sim-only, NOT in any bin): HACKbot home pager page, solid lime-green plate + black code rain.
 * Off by default: LV_CYD21_HACK_LIME is 0, so firmware and v1.2.23 renders are byte-identical. The mock sim builds with -DLV_CYD21_HACK_LIME=1.
 * v2 (Mobandy 10:31 AM ET "No, I wanted solid lime green"): the plate is ONE flat solid lime (#7CFF00, RGB565),
 * no metal ramp, no gradient, no sheen. Same black-rain pipeline as LOGbot (memcpy the house chip-plate so the
 * art's c==0 eye sockets stay black, then pitch-black rain with skip=1 via lv_pg_keep_feat). Source art is blob 2.
 * The panel, board, banner, pips and every other page are untouched. */
#pragma once
#ifndef LV_CYD21_HACK_LIME
#define LV_CYD21_HACK_LIME 0
#endif
#if LV_CYD21_HACK_LIME
#ifndef LV_HB_SOLID
#define LV_HB_SOLID 0x7CFF00u   /* flat lime; RGB565 expands to #7BFF00 */
#endif
static void lv_hb_face_row(uint16_t *row, int y, const uint8_t *fr)
{
    const int X0 = LV_PG_FX_X0;
    const uint16_t lime = lv_hex(LV_HB_SOLID);
    if (fr) memcpy(row + X0, fr, LV_PG_FACE_S * 2);
    for (int lx = 0; lx < LV_PG_FACE_S; ++lx)
        if (row[X0 + lx] != 0) row[X0 + lx] = lime;   /* sockets stay black */
    { const uint16_t pal[3] = { 0, 0, 0 }; lv_pg_rain_row(row, y, pal, 1, LV_PG_HACKBOT); }
}
#endif
