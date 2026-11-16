/* CONTROLbot Color UI v6 ticker sim (MOCK ONLY).
 * Compiles the verbatim lv_scene_ticker from cyd-portrait-wt src/live/lv_core.h @049ff59
 * (sim/lv_scene_ticker_049ff59.inc, byte-identical to that header) against the real
 * lv_draw.h / lv_font.h via zig c++. Hunt-bar stubs always return off so this mock
 * stays on the scrolling ticker. Tint path is a 1:1 port of the same loops; when its
 * colours are the firmware constants it must pixel-match the verbatim function.
 *
 * usage:
 *   cyd_ticker one  fw   theme now off alert out.rgb     (theme ignored)
 *   cyd_ticker one  tint theme now off alert out.rgb
 *   cyd_ticker batch plan.txt out.rgb     plan lines: mode theme now off alert
 *   cyd_ticker selftest
 * Header only: 23 x 238 RGB888, expanded the same way as tools/livescan/sim_live.cpp dump().
 */
#define LV_HOST 1
#include "lv_draw.h"
#include "themes_spec_v2.h"
#include "cyd_themes_lv_core_copy.h"
#include "new_themes_v3.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static uint32_t lv_now = 0;
static bool lv_tk_alert = false;
#define LV_TKLEN 200
static char lv_tk_msg[LV_TKLEN] = "NO THREATS - TAP SCAN";
static int lv_tk_off = 0;
static bool lv_hunt_bar_on() { return false; }
static void lv_hunt_bar(LvBand &B) { (void)B; }

#include "lv_scene_ticker_049ff59.inc"

/* --- tinted 1:1 port: same geometry, motion, and text as lv_scene_ticker; colours come from g_* --- */
static uint32_t g_bg, g_stripe_a, g_stripe_b, g_ink, g_shadow = 0x200000;
static void lv_scene_ticker_tint(LvBand &B, void *vctx)
{
    LvTickCtx *c = (LvTickCtx *)vctx;
    if (lv_hunt_bar_on()) { lv_hunt_bar(B); return; }
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    (void)flash; /* alert colours are pre-resolved into g_* so the flash period stays the firmware's 350 ms */
    const uint16_t bg = lv_hex(g_bg);
    b_fill(B, B.x0, B.y0, B.w, B.h, bg);
    const int ph = (int)(lv_now / 40);
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) {
            const bool red = (((x + (y <= 3 ? y : -y) + ph) / 5) & 1);
            b_pix(B, x, y, red ? lv_hex(g_stripe_a) : lv_hex(g_stripe_b));
        }
    }
    const int tx = c->span1 + 1 - lv_tk_off;
    const uint16_t ink = lv_hex(g_ink);
    b_text(B, tx + 1, 6, lv_tk_msg, lv_hex(g_shadow), 2);
    b_text(B, tx, 5, lv_tk_msg, ink, 2);
}

static void rgb565_expand(uint16_t v, unsigned char *o)
{
    int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
    o[0] = (unsigned char)((r << 3) | (r >> 2));
    o[1] = (unsigned char)((g << 2) | (g >> 4));
    o[2] = (unsigned char)((b << 3) | (b >> 2));
}

static uint16_t g_fb[320][240];
static void blit(int x, int y, int w, int h, const uint16_t *px)
{
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) g_fb[y + r][x + c] = px[r * w + c];
}

static int lum(uint32_t c)
{
    int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    return (r * 3 + g * 6 + b) / 10;
}
static uint32_t dark_shade(uint32_t c)
{
    uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) o |= (uint32_t)(((c >> sh) & 255) * 40 / 100) << sh;
    return o;
}
static void theme_palette(int t, uint32_t *acc, uint32_t *accd, uint32_t *hdr)
{
    if (t < 0) { *acc = 0xe00000; *accd = 0x2a0400; *hdr = 0xffb000; return; }
    if (t < SEC_NTHEME) { *acc = SEC_ACC[t]; *accd = SEC_ACCD[t]; *hdr = SEC_HDR[t]; return; }
    const int e = t - SEC_NTHEME;
    *acc = EXT_ACC[e]; *accd = EXT_ACCD[e]; *hdr = EXT_HDR[e];
}

/* Alert: exact firmware flash, untinted. Normal: two contrasting theme colours for the
 * stripe phases, text = theme bright (HDR), bg = dark shade of ACCD, shadow kept 0x200000. */
static void set_tint_colors(int theme, bool force_fw_palette)
{
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    if (lv_tk_alert) {
        g_bg = flash ? 0x6a0000 : 0x3a0000;
        g_stripe_a = flash ? 0xff2020 : 0xe00000;
        g_stripe_b = 0xffb000;
        g_ink = 0xffffff;
        g_shadow = 0x200000;
        return;
    }
    if (force_fw_palette || theme < 0) {
        g_bg = 0x2a0400;
        g_stripe_a = 0xe00000;
        g_stripe_b = 0xffb000;
        g_ink = 0xffc830;
        g_shadow = 0x200000;
        return;
    }
    uint32_t acc, accd, hdr;
    theme_palette(theme, &acc, &accd, &hdr);
    g_bg = dark_shade(accd);
    g_stripe_b = hdr;                 /* amber phase -> theme bright */
    g_stripe_a = acc;                 /* red phase -> theme accent */
    if (lum(g_stripe_a) - lum(g_stripe_b) < 40 && lum(g_stripe_b) - lum(g_stripe_a) < 40)
        g_stripe_a = lv_mix3(acc, accd, 1, 2);   /* low-contrast themes: pull the dark phase toward ACCD */
    g_ink = hdr;
    g_shadow = 0x200000;
}

static const LvRect LV_R_TICK = {1, 1, 238, 23};

static void render_to(unsigned char *dst, bool tint, int theme, bool force_fw)
{
    memset(g_fb, 0, sizeof g_fb);
    if (tint) set_tint_colors(theme, force_fw);
    LvTickCtx c{LV_R_TICK.x0, LV_R_TICK.x1};
    lv_render(blit, LV_R_TICK, tint ? lv_scene_ticker_tint : lv_scene_ticker, &c);
    int k = 0;
    for (int y = 1; y < 24; ++y)
        for (int x = 1; x < 239; ++x) { rgb565_expand(g_fb[y][x], dst + k); k += 3; }
}

static void write_rgb(const char *path, const unsigned char *p, int n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(p, 1, (size_t)n, f);
    fclose(f);
}

static int parse_mode(const char *s)
{
    if (!strcmp(s, "fw")) return 0;
    if (!strcmp(s, "tint")) return 1;
    if (!strcmp(s, "tintfw")) return 2; /* tint path, firmware palette (clone check) */
    fprintf(stderr, "bad mode %s\n", s); exit(2);
}

static unsigned char hdr[23 * 238 * 3];
static unsigned char hdr2[23 * 238 * 3];

static int selftest(void)
{
    const char *msgs[] = {"NO THREATS - TAP SCAN", "SCANNING WIFI..."};
    const int nows[] = {0, 40, 160, 350, 700, 1234, 2000, 3500};
    const int offs[] = {0, 1, 20, 100, 220, 234, 400, 489};
    int fail = 0, n = 0;
    for (int mi = 0; mi < 2; ++mi)
    for (int a = 0; a < 2; ++a)
    for (int ni = 0; ni < 8; ++ni)
    for (int oi = 0; oi < 8; ++oi) {
        snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", msgs[mi]);
        lv_now = (uint32_t)nows[ni];
        lv_tk_off = offs[oi];
        lv_tk_alert = a != 0;
        render_to(hdr, false, -1, false);
        render_to(hdr2, true, -1, true);
        ++n;
        if (memcmp(hdr, hdr2, sizeof hdr) != 0) {
            int d = 0;
            for (int i = 0; i < (int)sizeof hdr; i += 3)
                if (hdr[i] != hdr2[i] || hdr[i + 1] != hdr2[i + 1] || hdr[i + 2] != hdr2[i + 2]) ++d;
            fprintf(stderr, "MISMATCH msg='%s' now=%u off=%d alert=%d px=%d\n", lv_tk_msg, lv_now, lv_tk_off, a, d);
            ++fail;
        }
    }
    printf("selftest frames=%d fail=%d\n", n, fail);
    return fail ? 1 : 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: cyd_ticker one|batch|selftest ...\n"); return 2; }
    if (!strcmp(argv[1], "selftest")) return selftest();
    if (!strcmp(argv[1], "one")) {
        if (argc < 8) { fprintf(stderr, "one mode theme now off alert out.rgb\n"); return 2; }
        int mode = parse_mode(argv[2]);
        int theme = atoi(argv[3]);
        lv_now = (uint32_t)atoi(argv[4]);
        lv_tk_off = atoi(argv[5]);
        lv_tk_alert = atoi(argv[6]) != 0;
        if (argc >= 9) snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", argv[8]);
        render_to(hdr, mode != 0, theme, mode == 2);
        write_rgb(argv[7], hdr, (int)sizeof hdr);
        return 0;
    }
    if (!strcmp(argv[1], "batch")) {
        if (argc < 4) { fprintf(stderr, "batch plan.txt out.rgb\n"); return 2; }
        FILE *pl = fopen(argv[2], "r");
        if (!pl) { perror(argv[2]); return 1; }
        FILE *out = fopen(argv[3], "wb");
        if (!out) { perror(argv[3]); return 1; }
        char mode[16]; int theme, off, alert; unsigned now;
        while (fscanf(pl, "%15s %d %u %d %d", mode, &theme, &now, &off, &alert) == 5) {
            lv_now = now; lv_tk_off = off; lv_tk_alert = alert != 0;
            int m = parse_mode(mode);
            render_to(hdr, m != 0, theme, m == 2);
            fwrite(hdr, 1, sizeof hdr, out);
        }
        fclose(pl); fclose(out);
        return 0;
    }
    fprintf(stderr, "unknown cmd %s\n", argv[1]);
    return 2;
}
