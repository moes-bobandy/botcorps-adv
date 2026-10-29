/* ============================================================================
 * Sim-only GIBSON v8 mock painter (LOCKED gp harness, re-authored 2026-10-29 after the box-restart loss).
 * Target: LOCKED/refs/game_gibson_v8@3x.mp4 (720x960, 100/3 fps, 630 frames, 3x nearest of a 240x320 sim).
 * Reuses firmware painters (edge board, headers, top bar, rain, plate, chips) from botcorps_ui.cpp,
 * which is #included ahead of this file in the same TU; everything v8-specific is mocked here.
 * Not part of any PlatformIO env; firmware sources are untouched.
 * ==========================================================================*/
extern void sim_dump_raw(FILE *f);
extern void sim_png(const char *path);
extern void sim_set_millis(uint32_t);

static uint32_t s_gp_t = 100000;   /* gp-owned virtual time for rain steps */
static void gp_rain_step(int n)
{
    const bool was = botcorps_rain_draw_enabled();
    botcorps_rain_set_draw(false);
    for (int i = 0; i < n; ++i) { s_gp_t += 90; sim_set_millis(s_gp_t); botcorps_rain_step(); }
    botcorps_rain_set_draw(was);
}
static void gp_rain_bg(Gfx g)
{
    const int keep = s_screen;
    s_screen = SCR_GIBSON;              /* v8 runs on Gibson's screen: hub dim, ivory Gibson rain */
    botcorps_rain_set_hub(true);
    rain_flags();
    botcorps_rain_set_covers(nullptr, 0, false);
    g.fillScreen(botcorps_pal_hub_of(COL_FIELD));
    if (g.raw()) botcorps_rain_draw(*g.raw(), 0, g.raw()->height());
    s_screen = keep;
}
/* rainfit: n frames of the rain-only background, one rain step apart, raw RGB888 to path */
void sim_gp_rainfit(const char *path, int n, int mode)
{
    (void)mode;
    FILE *f = fopen(path, "wb"); if (!f) return;
    Gfx g(botcorps_disp());
    for (int i = 0; i < n; ++i) { gp_rain_bg(g); sim_dump_raw(f); gp_rain_step(1); }
    fclose(f);
    fprintf(stderr, "[gp] rainfit %d frames -> %s\n", n, path);
}

/* ---- v8 timeline: frame f (1-based) is t = (f-1) * 30 ms; rain steps every 140 ms (fit to the clip) ---- */
static const int V8_RAIN0 = 146, V8_RAIN_PH = 69, V8_RAIN_MS = 140;
static int v8_rain_step_of(int f) { return V8_RAIN0 + ((f - 1) * 30 + V8_RAIN_PH) / V8_RAIN_MS; }
static int s_v8_rain_at = 0;   /* rain steps taken since the gp start state */

/* rain is visible only in these cell-aligned windows between the parts (measured over the clip) */
static const int16_t V8_RAINWIN[][4] = {
    { 32, 128, 72, 24 }, { 176, 128, 32, 32 }, { 176, 200, 32, 32 }, { 32, 232, 72, 8 },
    { 32, 240, 192, 8 }, { 32, 248, 176, 8 },
};
static void v8_rain_mask(Gfx g)
{
    const uint16_t bg = botcorps_rgb565(botcorps_pal_hub_of(COL_FIELD));
    static uint8_t keep[320][240];
    memset(keep, 0, sizeof keep);
    for (auto &w : V8_RAINWIN) for (int y = w[1]; y < w[1] + w[3]; ++y) memset(&keep[y][w[0]], 1, w[2]);
    for (int y = 26; y < 320; ++y)
        for (int x = 0; x < 240; ) {
            if (keep[y][x]) { ++x; continue; }
            int e = x; while (e < 240 && !keep[y][e]) ++e;
            g.fillRect(x, y, e - x, 1, bg);
            x = e;
        }
}

/* ---- v8 timeline (measured from the ref clip; frame numbers are 1-based) ---- */
/* theme: GRAPE (0) frames 1-210, TOXIC (2) 211-420, ICE (4) 421-630 (plate/overlay switch at 421, board seen at 433) */
static int v8_theme(int f) { return f < 211 ? 0 : f < 421 ? 2 : 4; }
struct V8Txt { int f; const char *s; };
/* typed command, one key per 4 frames (120 ms); chip taps insert whole words */
static const V8Txt V8_TYPED[] = {
    { 1, "" }, { 68, "scan " }, { 94, "scan tv " }, { 128, "" },
    { 161, "l" }, { 165, "lo" }, { 169, "loc" }, { 173, "lock" }, { 177, "lock " }, { 181, "lock t" }, { 185, "lock tv" }, { 196, "" },
    { 222, "z" }, { 226, "za" }, { 230, "zap" }, { 234, "zap " }, { 238, "zap t" }, { 242, "zap tv" }, { 253, "" },
    { 276, "p" }, { 280, "pu" }, { 284, "pur" }, { 288, "purg" }, { 292, "purge" }, { 296, "purge " }, { 300, "purge t" }, { 304, "purge tv" }, { 315, "" },
    { 338, "o" }, { 342, "ov" }, { 346, "ove" }, { 350, "over" }, { 354, "overl" }, { 358, "overlo" }, { 362, "overloa" }, { 366, "overload" }, { 370, "overload " },
    { 409, "overload core " }, { 443, "" },
    { 556, "r" }, { 560, "re" }, { 564, "res" }, { 568, "rest" }, { 572, "resta" }, { 576, "restar" }, { 580, "restart" }, { 591, "" },
};
static const V8Txt V8_LOG[] = {
    { 1, "" }, { 128, "[ .. ] SCAN TV 2 LIVE" }, { 196, "[ >> ] LOCK TV" }, { 253, "[ !! ] ZAP TV -2" }, { 315, "[ !! ] PURGE TV NULL" },
    { 443, "[ !! ] OVERLOAD 6 HIT" }, { 591, "[ OK ] ROUND RESTART" },
};
template <size_t N> static const char *v8_at(const V8Txt (&tb)[N], int f)
{
    const char *r = tb[0].s;
    for (size_t i = 0; i < N; ++i) if (tb[i].f <= f) r = tb[i].s;
    return r;
}
/* command-list overlay windows (firmware cyd_cl_paint on SCR_GCHAT), selected chip */
struct V8Ovl { int f0, f1, sel; };
static const V8Ovl V8_OVL[] = { { 34, 67, 0 }, { 68, 93, 0 }, { 94, 120, 1 }, { 379, 432, 0 } };
static const V8Ovl *v8_ovl(int f) { for (auto &o : V8_OVL) if (f >= o.f0 && f <= o.f1) return &o; return nullptr; }

/* nearest palette index for a measured RGB888 */
static uint8_t v8_ix(uint32_t rgb)
{
    const int r = (rgb >> 16) & 255, gg = (rgb >> 8) & 255, b = rgb & 255;
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < 256; ++i) {
        const uint16_t c = botcorps_rgb565((uint8_t)i);
        const int cr = ((c >> 11) & 31) * 255 / 31, cg = ((c >> 5) & 63) * 255 / 63, cb = (c & 31) * 255 / 31;
        const int d = (cr - r) * (cr - r) + (cg - gg) * (cg - gg) + (cb - b) * (cb - b);
        if (d < bd) { bd = d; best = i; }
    }
    return (uint8_t)best;
}
/* measured plate tones per theme slot (0 GRAPE, 1 TOXIC, 2 ICE): focus box dark / accent dim, scanline */
static const uint32_t V8_TH[3][3] = {
    { 0x211039, 0x9449AD, 0x5A2463 }, { 0x001000, 0xB5DB52, 0x4A6110 }, { 0x101821, 0x4ACFDE, 0x00656B },
};
static int v8_ts(int f) { const int t = v8_theme(f); return t == 0 ? 0 : t == 2 ? 1 : 2; }

/* plate interior: firmware Adv prompt layout re-flowed to the clip's 2-row cmdline
 * (log at sy+3, prompt row at sy+17, wrapped typed words on sy+25, focus box open at the bottom) */
static void v8_plate(Gfx g, int f)
{
    const int sx = 33, sy = 271;
    const uint32_t t = (uint32_t)(f - 1) * 30u;
    const int ts = v8_ts(f);
    const uint8_t ac = accent(), dark = v8_ix(V8_TH[ts][0]), dim = v8_ix(V8_TH[ts][1]), wire = v8_ix(V8_TH[ts][2]);
    g.fillRect(sx + 1, sy + 1, 146, 34, PAL_BLACK);
    const bool focus = f >= 34;
    if (focus) {
        /* extents measured on the clip (f60/f130/f300): dark x35-176, accent x35-175, dim edge x176 from row 285 */
        g.fillRect(sx + 2, sy + 13, 142, 1, dark);
        g.fillRect(sx + 1, sy + 14, 1, 19, dark);
        g.fillRect(sx + 2, sy + 14, 141, 1, ac);
        g.fillRect(sx + 2, sy + 15, 1, 18, ac);
        g.fillRect(sx + 143, sy + 14, 1, 19, dim);
    }
    {
        /* scanline sweep fitted to the clip (scanobs: 628/630 frames): period 448 ms from t=63 ms,
         * 1 row per 28 ms over rows 275..283, then rests on row 284; rows skipped since the
         * previous frame leave a 080808 trail. GRAPE rest tone is 7328a5 in f57-160 and f206-210 (measured). */
        auto line_y = [](int32_t tt) { int32_t ph = ((tt - 63) % 448 + 448) % 448; int y = 275 + ph / 28; return y > 284 ? 284 : y; };
        const int scy = line_y((int32_t)t), prv = f > 1 ? line_y((int32_t)t - 30) : 284;
        const int tr0 = prv >= 284 ? 274 : prv + 1;
        if (scy < 284 && tr0 < scy) g.fillRect(sx + 4, tr0, 139, scy - tr0, v8_ix(0x080808));
        const uint8_t wc = (ts == 0 && scy == 284 && ((f >= 57 && f <= 160) || (f >= 206 && f <= 210))) ? v8_ix(0x7328A5) : wire;
        g.fillRect(sx + 4, scy, 139, 1, wc);   /* x37-175 */
    }
    const int keep = s_px; s_px = 6;
    const char *ln = v8_at(V8_LOG, f);
    if (ln[0]) {
        const int lx = sx + 6, ly = sy + 4;
        const char *rb = strchr(ln, ']');
        char tag[12]; int tn = (int)(rb - ln) + 1; memcpy(tag, ln, (size_t)tn); tag[tn] = 0;
        txt_bg(g, tag, lx, ly, (uint8_t)PAL_OK, PAL_BLACK, lgfx::textdatum::top_left);
        txt_bg(g, rb + 1, lx + tn * 6, ly, botcorps_pal_fit(0xBFBF), PAL_BLACK, lgfx::textdatum::top_left);
    }
    /* prompt row: same glyph stack as cyd_adv_cmdline */
    const int px = sx + 5, py = sy + 17;
    /* clip colours (measured mode over all 630 frames): glow 101821, face b524ad */
    const uint8_t pglow = v8_ix(0x101821), pface = v8_ix(0xB524AD);
    txt(g, "user@botcorps", px - 1, py, pglow, lgfx::textdatum::top_left);
    txt(g, "user@botcorps", px + 1, py, pglow, lgfx::textdatum::top_left);
    txt(g, "user@botcorps", px, py + 1, pglow, lgfx::textdatum::top_left);
    txt(g, "user@botcorps", px, py, pface, lgfx::textdatum::top_left);
    txt(g, ":~$", px + 13 * 6 + 1, py, ac, lgfx::textdatum::top_left);
    txt(g, ":~$", px + 13 * 6, py, ac, lgfx::textdatum::top_left);
    /* typed words: row 1 after "$ ", a word whose end + caret passes the box edge drops to row 2 */
    const char *ty = v8_at(V8_TYPED, f);
    const int x1 = sx + 144;
    int cx = px + 17 * 6, cy = py;
    g.setFont(&fonts::Font0); g.setTextSize(1); g.setTextColor(COL_WHITE);
    for (const char *p = ty; *p; ) {
        const char *e = p; while (*e && *e != ' ') ++e;
        const int wl = (int)(e - p);
        if (cy == py && cx + wl * 6 + 6 > x1 && wl > 0) { cx = px; cy = py + 8; }
        { char w[24]; int n = wl < 23 ? wl : 23; memcpy(w, p, (size_t)n); w[n] = 0; g.setCursor(cx, cy); g.print(w); }
        cx += wl * 6;
        p = e;
        if (*p == ' ') { cx += 6; ++p; }
    }
    /* a trailing space that would push the caret past the edge leaves it on row 1 (clip f296 "purge ") */
    if (cy == py && cx + 6 > x1 + 1) cx -= 6;
    const bool caret = ty[0] ? true : ((t + 50u) / 530u) % 2u == 1u;
    if (caret && f >= 2) g.fillRect(cx + 2, cy, 6, 8, ac);
    s_px = keep;
}

/* ---- traced board art (gib_v8_art.inc, generated by v8art.py): per-segment delta patches ---- */
#include "gib_v8_art.inc"
static int32_t s_v8a[320][240];
static bool s_v8_noart = false;   /* tl "none": firmware/procedural base only (input to v8art.py) */
static int s_v8a_next = -1;            /* next V8A_SEG index to apply; -1 = layer not initialised */
static void v8_art(Gfx g, int f)
{
    const int nseg = (int)(sizeof V8A_SEG / sizeof V8A_SEG[0]) - 1;
    if (s_v8a_next < 0 || (s_v8a_next > 0 && V8A_SEG[s_v8a_next - 1].f0 > f)) {
        for (auto &row : s_v8a) for (auto &v : row) v = -1;
        s_v8a_next = 0;
    }
    while (s_v8a_next < nseg && V8A_SEG[s_v8a_next].f0 <= f) {
        for (uint32_t r = V8A_SEG[s_v8a_next].r0; r < V8A_SEG[s_v8a_next + 1].r0; ++r) {
            const V8ARun &u = V8A_RUN[r];
            for (int i = 0; i < u.n; ++i) s_v8a[u.y][u.x + i] = V8A_VAL[u.v + i];
        }
        ++s_v8a_next;
    }
    for (int y = 0; y < 320; ++y)
        for (int x = 0; x < 240; ) {
            const int32_t v = s_v8a[y][x];
            int e = x + 1; while (e < 240 && s_v8a[y][e] == v) ++e;
            if (v >= 0) g.fillRect(x, y, e - x, 1, (uint16_t)v);
            x = e;
        }
}

static void v8_paint(Gfx g, int f)
{
    const uint32_t t = (uint32_t)(f - 1) * 30u;
    sim_set_millis(200000u + t);
    s_color = v8_theme(f);
    gp_rain_bg(g);
    v8_rain_mask(g);
    const int keep = s_screen;
    const V8Ovl *ov = v8_ovl(f);
    s_screen = ov ? SCR_GCHAT : SCR_GIBSON;
    snprintf(botcorps_t0_term_prompt, 60, "%s", v8_at(V8_TYPED, f));
    s_cyd_cl = ov != nullptr; s_cyd_cl_scr = SCR_GCHAT; s_cyd_cl_top = 0; s_cyd_cl_sel = ov ? ov->sel : 0;
    s_cb_col = accent();
    e_board(g);
    e_plate(g, HB_TBOT_P[0], HB_TBOT_P[1], HB_TBOT_P[2], HB_TBOT_P[3], HB_TBOT_SILK, false);
    e_finish(g);
    if (!s_v8_noart) v8_art(g, f);   /* all frames: overlay frames are traced too (delta vs base incl. the cl overlay) */
    v8_plate(g, f);
    s_screen = keep;
}

void sim_gp_v8(const char *dir, int f0, int f1, const char *tl)
{
    Gfx g(botcorps_disp());
    if (tl && strcmp(tl, "icons") == 0) {   /* debug sheet: every glossary icon, 1x and 10px, for art matching */
        g.fillScreen(PAL_BLACK);
        for (int i = 0; i < GL_N; ++i) { gl_icon(g, i, 4 + 16 * (i % 14), 4 + 20 * (i / 14), false); }
        char p[512]; snprintf(p, sizeof p, "%s/icons.png", dir); sim_png(p);
        return;
    }
    s_v8_noart = tl && strcmp(tl, "none") == 0;
    for (int f = f0; f <= f1; ++f) {
        const int want = v8_rain_step_of(f);
        if (want > s_v8_rain_at) { gp_rain_step(want - s_v8_rain_at); s_v8_rain_at = want; }
        v8_paint(g, f);
        char p[512]; snprintf(p, sizeof p, "%s/f%04d.png", dir, f);
        sim_png(p);
    }
    fprintf(stderr, "[gp] gibv8 %d..%d -> %s\n", f0, f1, dir);
}
