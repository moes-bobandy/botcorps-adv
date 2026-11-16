/* fw-3locks-1116: CONTROLbot Color UI v6 ticker tint (LOCKED a87fe22 / 585810c, v6-cyd-ticker/sim/cyd_ticker.cpp).
 * set_tint_colors + lv_scene_ticker_tint are the lock sim's 1:1 port of the firmware lv_scene_ticker: same geometry, motion and
 * text; only the colours come from the selected theme. Alert = the firmware red flash, untinted, on every theme. The lock sim's
 * selftest proves the tint path with the firmware palette is pixel-identical to the verbatim lv_scene_ticker. Included from
 * lv_core.h just before lv_scene_ticker. */
#pragma once
#ifndef LV_CUI_TICKER_ALL
#define LV_CUI_TICKER_ALL 1   /* 1 = the theme carries into the top banner on every CYD screen (LOCK.md); 0 = only on Color UI */
#endif
static uint32_t lv_cui_g_bg, lv_cui_g_stripe_a, lv_cui_g_stripe_b, lv_cui_g_ink, lv_cui_g_shadow = 0x200000;
static int lv_cui_lum(uint32_t c) { const int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255; return (r * 3 + g * 6 + b) / 10; }
static uint32_t lv_cui_dark_shade(uint32_t c) { uint32_t o = 0; for (int sh = 0; sh <= 16; sh += 8) o |= (uint32_t)(((c >> sh) & 255) * 40 / 100) << sh; return o; }
/* == cyd_ticker.cpp set_tint_colors(theme, false) with theme_palette = this firmware's LV_ACC / LV_ACCD / LV_HDR[lv_theme]
 *    (the lock sim's SEC_* / EXT_* inks; 0..11 = the original CYD table, 12..21 = SEC 1..10, 22..25 = EXT 12..15). */
static void lv_cui_tint_colors(int theme)
{
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    if (lv_tk_alert) {
        lv_cui_g_bg = flash ? 0x6a0000 : 0x3a0000; lv_cui_g_stripe_a = flash ? 0xff2020 : 0xe00000; lv_cui_g_stripe_b = 0xffb000;
        lv_cui_g_ink = 0xffffff; lv_cui_g_shadow = 0x200000; return;
    }
    const uint32_t acc = LV_ACC[theme], accd = LV_ACCD[theme], hdr = LV_HDR[theme];
    lv_cui_g_bg = lv_cui_dark_shade(accd);
    lv_cui_g_stripe_b = hdr;                 /* amber phase -> theme bright */
    lv_cui_g_stripe_a = acc;                 /* red phase -> theme accent */
    if (lv_cui_lum(lv_cui_g_stripe_a) - lv_cui_lum(lv_cui_g_stripe_b) < 40 && lv_cui_lum(lv_cui_g_stripe_b) - lv_cui_lum(lv_cui_g_stripe_a) < 40)
        lv_cui_g_stripe_a = lv_mix3(acc, accd, 1, 2);   /* low-contrast themes: pull the dark phase toward ACCD */
    lv_cui_g_ink = hdr;
    lv_cui_g_shadow = 0x200000;
}
/* == cyd_ticker.cpp lv_scene_ticker_tint (hunt bar first, exactly like lv_scene_ticker) */
static void lv_scene_ticker_tint(LvBand &B, int span1)
{
    const uint16_t bg = lv_hex(lv_cui_g_bg);
    b_fill(B, B.x0, B.y0, B.w, B.h, bg);
    const int ph = (int)(lv_now / 40);
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) {
            const bool red = (((x + (y <= 3 ? y : -y) + ph) / 5) & 1);
            b_pix(B, x, y, red ? lv_hex(lv_cui_g_stripe_a) : lv_hex(lv_cui_g_stripe_b));
        }
    }
    const int tx = span1 + 1 - lv_tk_off;
    const uint16_t ink = lv_hex(lv_cui_g_ink);
    b_text(B, tx + 1, 6, lv_tk_msg, lv_hex(lv_cui_g_shadow), 2);
    b_text(B, tx, 5, lv_tk_msg, ink, 2);
}
/* the tinted banner is final colour (the lock renders it with no per-pixel theme re-ink): bypass lv_blit_theme for it */
static bool lv_cui_tint_active(void);
