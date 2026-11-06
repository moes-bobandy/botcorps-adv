/* item 2 (fw-fix7-1106): the LOCKED MORPHEUSbot Device Glossary.
 *   LV_SC_GLOSS       ui_glossary_morpheus_rain: 12 device chips around the big MORPHEUSbot, chip rain in his black body
 *   LV_SC_GLOSS_DESC  ui_glossary: one description page per device (lv_gloss_dev 0..11)
 * Pixels come from lv_glossary_data.h (tools/port/gen_glossary.py): a base layer + one patch layer per screen, scanline
 * byte RLE in flash, decoded row by row into a 240-entry id line and pushed through the band buffer. Each id is a palette
 * entry with 6 theme slots (2 = the locked green, 5 = the locked orange, verbatim); themed ids on 6..11 take slot 3 through
 * lv_multi_ink like the port layer. The rain is the mock generator itself (12 columns, 37-step device sequences, 6 levels):
 * step t is the mock frame t, the locked still is t = 9; it advances every 140 ms with BG MOTION on, and only the rain pixels
 * that change are pushed. Pushed with lv_blit_hw: these are final colours, lv_blit_theme must not re-ink them a second time.
 * Rows 1..23 x 1..238 are the alert ticker (lv_ticker_tick); BACKbot {221,301,239,319} is drawn over the board. */
#pragma once
static inline bool lv_gl_themed(int i) { return (LV_GL_THEMED[i >> 3] >> (i & 7)) & 1; }
static inline uint16_t lv_gl_col(uint16_t i, int x, int y)
{
    if (lv_theme < LV_THEME_ORIG) return LV_GL_PAL[i][lv_theme];
    const uint16_t c = LV_GL_PAL[i][LV_THEME_BASE];
    return lv_gl_themed(i) && lv_reink_on() ? lv_multi_ink(c, x, y) : c;
}
/* decode row y of layer L into ids[] (pixels the layer keeps transparent are left alone) */
static void lv_gl_dec(const LvGlLayer &L, int y, uint16_t *ids)
{
    if (y < L.y0 || y > L.y1) return;
    const uint8_t *p = L.dat + L.off[y - L.y0], *e = L.dat + L.off[y - L.y0 + 1];
    uint16_t m0 = L.m0, m1 = L.m1, m2 = L.m2; int x = L.x0;
    while (p < e) {
        const uint8_t b = *p++; const int sel = b >> 6; uint16_t v; int n;
        if (sel == 3) {
            const uint8_t c = *p++; v = (uint16_t)((b & 63) << 4 | c >> 4); n = (c & 15) + 1;
            if (v == m1) { m1 = m0; m0 = v; } else { m2 = m1; m1 = m0; m0 = v; }
        } else {
            n = (b & 63) + 1;
            if (sel == 1) { v = m1; m1 = m0; m0 = v; }
            else if (sel == 2) { v = m2; m2 = m1; m1 = m0; m0 = v; }
            else v = m0;
        }
        if (v != LV_GL_TRANSP) for (int k = 0; k < n && x + k < 240; ++k) ids[x + k] = v;
        x += n;
    }
}
static inline int lv_gl_mod(int a, int m) { a %= m; return a < 0 ? a + m : a; }
/* rain pixel at step t (only for pixels in LV_GL_RUN): the last cell of the row (paint order) with ink there, else black */
static uint16_t lv_gl_rain_px(int t, int x, int y)
{
    if (y < 49) return 0;
    const int r = (y - 49) >> 3;
    if (r >= 13) return 0;
    uint16_t out = 0;
    for (int i = LV_GL_CROW[r]; i < LV_GL_CROW[r + 1]; ++i) {
        const LvGlCell &C = LV_GL_CELL[i];
        if (x < C.cx || x >= C.cx + 8) continue;
        const int s = LV_GL_SEQ[C.c][lv_gl_mod(t - C.row, 37)];
        const int ch = (LV_GL_ART[s][y - C.cy] >> (2 * (x - C.cx))) & 3;
        if (ch) out = LV_GL_RCOL[s][ch - 1][lv_gl_mod(t - C.row + LV_GL_PHASE[C.c], 6)][y >= LV_GL_LROW0 && y <= LV_GL_LROW1];
    }
    return out;
}
static int lv_gl_layer(void) { return lv_scr == LV_SC_GLOSS ? 1 : 2 + (lv_gloss_dev < 0 ? 0 : lv_gloss_dev > 11 ? 11 : lv_gloss_dev); }
static void lv_gl_scene(LvBand &B, void *)
{
    uint16_t ids[240];   /* stack: lv_bandbuf is this band's pixels */
    const int L = lv_gl_layer();
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        lv_gl_dec(LV_GL_LAYER[0], y, ids); lv_gl_dec(LV_GL_LAYER[L], y, ids);
        uint16_t *o = B.px + (y - B.y0) * B.w;
        for (int x = B.x0; x < B.x0 + B.w; ++x) o[x - B.x0] = lv_gl_col(ids[x], x, y);
        if (L == 1 && y >= LV_GL_RY0 && y <= LV_GL_RY1)
            for (int i = 0; i < LV_GL_NRUN; ++i) {
                const LvGlRun &R = LV_GL_RUN[i];
                if (R.y != y) continue;
                for (int x = R.x; x < R.x + R.n; ++x) if (x >= B.x0 && x < B.x0 + B.w) o[x - B.x0] = lv_gl_rain_px(lv_gloss_step, x, y);
            }
    }
    const LvRect bb = {(int16_t)B.x0, (int16_t)B.y0, (int16_t)(B.x0 + B.w - 1), (int16_t)(B.y0 + B.h - 1)};
    if (lv_rect_hit(LV_R_BACKBOT, bb)) lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
/* repaint r (clipped; never the ticker interior). lv_render brightens the press-flash rect (lv_hi) on the way. */
static void lv_gloss_paint(const LvRect &r0)
{
    const int x0 = r0.x0 < 0 ? 0 : r0.x0, x1 = r0.x1 > 239 ? 239 : r0.x1, y0 = r0.y0 < 0 ? 0 : r0.y0, y1 = r0.y1 > 319 ? 319 : r0.y1;
    if (x0 > x1 || y0 > y1) return;
    auto part = [](int a, int b, int c, int d) { if (a <= c && b <= d) lv_render(lv_blit_hw, {(int16_t)a, (int16_t)b, (int16_t)c, (int16_t)d}, lv_gl_scene, 0); };
    if (y0 == 0) part(x0, 0, x1, 0);
    const int t0 = y0 > 1 ? y0 : 1, t1 = y1 < 23 ? y1 : 23;
    if (t0 <= t1) { if (x0 == 0) part(0, t0, 0, t1); if (x1 == 239) part(239, t0, 239, t1); }
    part(x0, y0 > 24 ? y0 : 24, x1, y1);
}
static void lv_gloss_tick(void)
{
    if (lv_enter) { lv_enter = false; lv_gloss_paint({0, 0, 239, 319}); }
    if (lv_scr != LV_SC_GLOSS || !lv_motion) return;   /* BG MOTION OFF: the rain stays on the locked still */
    const int t = (int)((LV_GL_T_STILL + (lv_now - lv_gloss_t0) / 140) % LV_GL_RAIN_PERIOD);
    if (t == lv_gloss_step) return;
    const int p = lv_gloss_step; lv_gloss_step = t;
    uint16_t *const lb = lv_bandbuf;   /* not inside a render: reuse the band buffer, no static RAM */
    const bool fl = lv_flash_r.x1 >= lv_flash_r.x0;
    for (int i = 0; i < LV_GL_NRUN; ++i) {   /* push only the rain pixels that change, run by run */
        const LvGlRun &R = LV_GL_RUN[i];
        int s = -1;
        for (int x = R.x; x <= R.x + R.n; ++x) {
            bool ch = false; uint16_t v = 0;
            if (x < R.x + R.n) { v = lv_gl_rain_px(t, x, R.y); ch = v != lv_gl_rain_px(p, x, R.y); if (fl && lv_in(lv_flash_r, x, R.y)) v = lv_hi(v); }
            if (ch) { if (s < 0) s = x; lb[x - s] = v; }
            else if (s >= 0) { lv_blit_hw(s, R.y, x - s, 1, lb); s = -1; }
        }
    }
}
