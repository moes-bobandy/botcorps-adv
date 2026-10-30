/* GIBSON radar (item 10): full-screen Hackers-style "Gibson" city. Included from lv_core.h.
 * Every find is a translucent neon tower: height + brightness = signal (dBm), ground position = MAC hash,
 * WiFi cyan, BLE magenta, hunt match red. Perspective ground grid scrolls toward you, a wireframe skyline on
 * the horizon, green code streaks (CONTROLbot RAIN / RAIN COL), camera DRIFT or a SWEEP line (RAIN MODE).
 * Tap a tower: its info shows in the panel and its full line goes on the ticker. BACKbot returns home. */
#include <math.h>
#define LV_RD_F 120.0f
#define LV_RD_HOR 138
#define LV_RD_CAMH 0.9f
#define LV_RD_Y0 34
#define LV_RD_Y1 285
static const LvRect LV_R_RADAR = {0, 24, 239, 319};
static uint32_t lv_rd_t0 = 0;
static int lv_rd_sel = -1;           /* index into lv_f */
static uint8_t lv_rd_selmac[6];
static float lv_rd_camx = 0;
struct LvTower { int i; float X, Z, H; int x0, x1, yt, yb, bx0, bx1, byt, byb; };
static LvTower lv_rd_tw[LV_MAXF];
static int lv_rd_n = 0;
static uint32_t lv_rd_hash(const uint8_t *m) { uint32_t h = 2166136261u; for (int k = 0; k < 6; ++k) { h ^= m[k]; h *= 16777619u; } return h; }
static uint16_t lv_rd_col(const LvFind &f, int num, int den)
{
    const uint32_t c = f.alert ? 0xff2020 : f.kind == LV_K_WIFI ? 0x00f7ff : 0xff2ad4;
    return lv_dim(lv_hex(c), num, den);
}
static int lv_rd_level(const LvFind &f) { int v = f.rssi + 100; if (v < 4) v = 4; if (v > 60) v = 60; return v; } /* 4..60 */
static void lv_rd_layout(void)
{
    const float t = (lv_now - lv_rd_t0) / 1000.0f;
    lv_rd_camx = lv_rainmode == 0 ? 0.5f * sinf(t * 0.45f) : 0.0f;
    lv_rd_n = 0;
    for (int i = 0; i < lv_nf; ++i) {
        const LvFind &f = lv_f[i]; const uint32_t h = lv_rd_hash(f.mac);
        LvTower T; T.i = i;
        T.Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f;
        T.X = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * T.Z;   /* spread to fill the view at every depth */
        T.H = 0.12f + lv_rd_level(f) / 60.0f * 1.9f;
        const float w = 0.12f, d = 0.24f;
        auto px = [&](float X, float Z) { return (int)lroundf(120 + LV_RD_F * (X - lv_rd_camx) / Z); };
        auto py = [&](float Y, float Z) { return (int)lroundf(LV_RD_HOR + LV_RD_F * (LV_RD_CAMH - Y) / Z); };
        T.x0 = px(T.X - w, T.Z); T.x1 = px(T.X + w, T.Z); T.yt = py(T.H, T.Z); T.yb = py(0, T.Z);
        T.bx0 = px(T.X - w, T.Z + d); T.bx1 = px(T.X + w, T.Z + d); T.byt = py(T.H, T.Z + d); T.byb = py(0, T.Z + d);
        lv_rd_tw[lv_rd_n++] = T;
    }
    for (int a = 1; a < lv_rd_n; ++a) { LvTower t = lv_rd_tw[a]; int b = a - 1; while (b >= 0 && lv_rd_tw[b].Z < t.Z) { lv_rd_tw[b + 1] = lv_rd_tw[b]; --b; } lv_rd_tw[b + 1] = t; } /* far -> near */
}
static inline void b_blend(LvBand &B, int x, int y, uint16_t c, int k)
{
    if (x >= B.x0 && x < B.x0 + B.w && y >= B.y0 && y < B.y0 + B.h) { uint16_t &p = B.px[(y - B.y0) * B.w + (x - B.x0)]; p = lv_mix(p, c, k, 8); }
}
static void b_line(LvBand &B, int x0, int y0, int x1, int y1, uint16_t c)
{
    if ((y0 < B.y0 && y1 < B.y0) || (y0 >= B.y0 + B.h && y1 >= B.y0 + B.h)) return;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (int n = 0; n < 800; ++n) { b_pix(B, x0, y0, c); if (x0 == x1 && y0 == y1) break; const int e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
}
static void lv_rd_vclip(LvBand &B, int &ya, int &yb) { if (ya < LV_RD_Y0) ya = LV_RD_Y0; if (yb > LV_RD_Y1) yb = LV_RD_Y1; (void)B; }
static uint16_t lv_rd_rain_col(int k)
{
    const uint32_t c = lv_raincol == 0 ? 0x20ff60 : lv_raincol == 1 ? 0x30e8ff : LV_ACC[lv_theme];
    return lv_dim(lv_hex(c), k, 8);
}
static void lv_scene_radar(LvBand &B, void *)
{
    const float t = (lv_now - lv_rd_t0) / 1000.0f;
    const uint16_t hdr = lv_ink(LV_HDR), mag = lv_hex(0xff2ad4), grid = lv_hex(0x7a1cff);
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        uint16_t c = 0;
        if (y >= LV_RD_Y0 && y <= LV_RD_Y1) {
            if (y < LV_RD_HOR) { const int k = (y - LV_RD_Y0) * 10 / (LV_RD_HOR - LV_RD_Y0); c = lv_rgb((uint8_t)(4 + k * 2), 0, (uint8_t)(10 + k * 3)); }
            else c = lv_hex(0x050010);
        }
        b_fill(B, B.x0, y, B.w, 1, c);
    }
    /* skyline: a wireframe far city on the horizon */
    static const int8_t SKY[24] = {9, 14, 6, 22, 12, 30, 8, 18, 26, 11, 16, 34, 10, 20, 7, 28, 15, 24, 9, 13, 31, 17, 6, 19};
    const int shift = (int)(lv_rd_camx * 20);
    for (int k = 0; k < 24; ++k) {
        const int x0 = k * 11 - 6 - shift, w = 9, ht = SKY[k];
        const uint16_t c = lv_dim(grid, 4, 8);
        b_line(B, x0, LV_RD_HOR, x0, LV_RD_HOR - ht, c); b_line(B, x0 + w, LV_RD_HOR, x0 + w, LV_RD_HOR - ht, c); b_line(B, x0, LV_RD_HOR - ht, x0 + w, LV_RD_HOR - ht, c);
        for (int wy = LV_RD_HOR - ht + 3; wy < LV_RD_HOR - 1; wy += 4) for (int wx = x0 + 2; wx < x0 + w - 1; wx += 3) if (((wx * 7 + wy * 13 + k) % 5) == 0) b_pix(B, wx, wy, lv_dim(lv_hex(0xffd040), 5, 8));
    }
    /* ground grid: per row depth, scrolling lines toward the viewer */
    const float ph = lv_rainmode == 2 ? 0.0f : fmodf(t * 0.8f, 1.0f);
    for (int y = (B.y0 > LV_RD_HOR + 1 ? B.y0 : LV_RD_HOR + 1); y < B.y0 + B.h && y <= LV_RD_Y1; ++y) {
        const float Z = LV_RD_F * LV_RD_CAMH / (y - LV_RD_HOR), Zn = LV_RD_F * LV_RD_CAMH / (y + 1 - LV_RD_HOR);
        const int k = Z > 6 ? 2 : Z > 3 ? 4 : 7;
        if (floorf(Z + ph) != floorf(Zn + ph)) b_fill(B, 0, y, 240, 1, lv_dim(grid, k, 8));
        for (int g = -12; g <= 12; ++g) { const int x = (int)lroundf(120 + LV_RD_F * (g * 0.5f - lv_rd_camx) / Z); b_pix(B, x, y, lv_dim(grid, k, 8)); }
        if (lv_rainmode == 1) {   /* SWEEP: a bright line rolls from the horizon to you */
            const float sz = 6.0f - fmodf(t * 1.6f, 5.4f);
            if (Z >= sz && Zn < sz) b_fill(B, 0, y, 240, 1, lv_hex(0x40ff90));
        }
    }
    b_fill(B, 0, LV_RD_HOR, 240, 1, mag);
    /* code streaks */
    if (lv_rain) for (int s = 0; s < 18; ++s) {
        const int x = (s * 53 + 17) % 236 + 2, len = 10 + (s * 7) % 18, sp = 40 + (s * 29) % 70;
        const int head = LV_RD_Y0 + (int)((lv_now / 1000.0f * sp + s * 37)) % (LV_RD_Y1 - LV_RD_Y0 + len) ;
        for (int k = 0; k < len; k += 2) { const int y = head - k; if (y >= LV_RD_Y0 && y <= LV_RD_Y1) b_blend(B, x, y, lv_rd_rain_col(8), k == 0 ? 8 : 6 - k * 5 / len); }
    }
    /* towers, far to near: translucent faces, bright edges */
    for (int n = 0; n < lv_rd_n; ++n) {
        const LvTower &T = lv_rd_tw[n]; const LvFind &f = lv_f[T.i];
        const int lev = lv_rd_level(f), br = 3 + lev * 5 / 60;           /* 3..8 of 8 */
        const bool sel = T.i == lv_rd_sel;
        const uint16_t edge = sel ? lv_hex(0xffffff) : lv_rd_col(f, br, 8), face = lv_rd_col(f, br, 8);
        int ya = T.byt, yb = T.yb; lv_rd_vclip(B, ya, yb);
        /* top face (back-top edge to front-top edge) */
        for (int y = T.byt; y <= T.yt; ++y) { if (y < LV_RD_Y0 || y > LV_RD_Y1) continue; const float u = T.yt == T.byt ? 1.0f : (float)(y - T.byt) / (T.yt - T.byt);
            const int xa = (int)(T.bx0 + (T.x0 - T.bx0) * u), xb = (int)(T.bx1 + (T.x1 - T.bx1) * u); for (int x = xa; x <= xb; ++x) b_blend(B, x, y, face, 3); }
        /* front face */
        for (int y = T.yt; y <= T.yb; ++y) { if (y < LV_RD_Y0 || y > LV_RD_Y1) continue; for (int x = T.x0; x <= T.x1; ++x) b_blend(B, x, y, face, 3); }
        b_line(B, T.x0, T.yt, T.x0, T.yb, edge); b_line(B, T.x1, T.yt, T.x1, T.yb, edge);
        b_line(B, T.x0, T.yt, T.x1, T.yt, edge); b_line(B, T.x0, T.yb, T.x1, T.yb, lv_dim(edge, 1, 2));
        b_line(B, T.bx0, T.byt, T.bx1, T.byt, lv_dim(edge, 5, 8)); b_line(B, T.bx0, T.byt, T.x0, T.yt, edge); b_line(B, T.bx1, T.byt, T.x1, T.yt, edge);
        if (T.bx1 > T.x1) b_line(B, T.bx1, T.byt, T.bx1, T.byb, lv_dim(edge, 5, 8));
        if (T.bx0 < T.x0) b_line(B, T.bx0, T.byt, T.bx0, T.byb, lv_dim(edge, 5, 8));
        if (f.alert && ((lv_now / 300) & 1)) b_text35(B, (T.x0 + T.x1) / 2 - 1, T.byt - 7, "!", lv_hex(0xff4040));
    }
    /* header (after the towers so tall near towers tuck under it) */
    b_fill(B, 0, 24, 240, 1, lv_ink(LV_ACCD));
    b_fill(B, 0, 25, 240, 9, lv_hex(0x101018));
    b_text35(B, 3, 27, "GIBSON // JACKED IN", hdr);
    char h[40];
    if (lv_job != LV_J_NONE) snprintf(h, sizeof h, "SCANNING %s", lv_job_now()); else snprintf(h, sizeof h, "%d NODES  %s", lv_nf, LV_RAINMODE_NAME[lv_rainmode]);
    b_text35(B, 237 - lv_w35(h), 27, h, hdr);
    /* info panel for the tapped tower */
    if (lv_rd_sel >= 0 && lv_rd_sel < lv_nf) {
        const LvFind &f = lv_f[lv_rd_sel]; char l[26], t[48], m[18]; lv_label(f, l, 20); lv_macstr(f.mac, m);
        b_fill(B, 2, 262, 236, 23, lv_hex(0x000000)); b_frame(B, 2, 262, 236, 23, lv_rd_col(f, 8, 8));
        b_text(B, 5, 264, l, lv_hex(0xffffff));
        snprintf(t, sizeof t, "%d", f.rssi); b_text(B, 235 - ((int)strlen(t) + 4) * 6, 264, t, lv_rd_col(f, 8, 8)); b_text35(B, 235 - 4 * 6 + 4, 265, "DBM", lv_rd_col(f, 8, 8));
        snprintf(t, sizeof t, "%s %s  %s", f.kind == LV_K_WIFI ? "WIFI" : "BLE", LV_ICON_NAME[f.icon], m); b_text35(B, 5, 275, t, lv_hex(LV_TAN));
    }
    /* footer: legend + HEIGHT = SIGNAL + BACKbot */
    b_fill(B, 0, 286, 240, 1, lv_ink(LV_ACCD));
    b_text(B, 4, 290, "HEIGHT = SIGNAL", lv_hex(0xffffff));
    b_fill(B, 4, 303, 5, 5, lv_hex(0x00f7ff)); b_text35(B, 11, 303, "WIFI", lv_hex(0x00f7ff));
    b_fill(B, 32, 303, 5, 5, lv_hex(0xff2ad4)); b_text35(B, 39, 303, "BLE", lv_hex(0xff2ad4));
    b_fill(B, 56, 303, 5, 5, lv_hex(0xff2020)); b_text35(B, 63, 303, "MATCH", lv_hex(0xff4040));
    b_text35(B, 4, 311, lv_nf ? "TAP A TOWER = INFO" : "NO NODES YET - SCANNING", hdr);
    char c[24]; int nw = 0; for (int i = 0; i < lv_nf; ++i) nw += lv_f[i].kind == LV_K_WIFI;
    snprintf(c, sizeof c, "%dW %dB", nw, lv_nf - nw); b_text35(B, 200 - lv_w35(c), 292, c, hdr);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
static void lv_radar_enter(void)
{
    lv_rd_t0 = lv_now; lv_rd_sel = -1;
    if (lv_nf == 0 && lv_job == LV_J_NONE) lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1);
}
static void lv_radar_frame(void)
{
    if (lv_rd_sel >= 0) { /* keep the selection on the same device across re-sorts */
        int k = -1; for (int i = 0; i < lv_nf; ++i) if (!memcmp(lv_f[i].mac, lv_rd_selmac, 6)) { k = i; break; }
        lv_rd_sel = k;
    }
    lv_rd_layout();
    lv_render(lv_blit, LV_R_RADAR, lv_scene_radar, 0);
}
static const char *lv_radar_tap(int x, int y)
{
    for (int n = lv_rd_n - 1; n >= 0; --n) {    /* near first */
        const LvTower &T = lv_rd_tw[n];
        const int xa = (T.x0 < T.bx0 ? T.x0 : T.bx0) - 3, xb = (T.x1 > T.bx1 ? T.x1 : T.bx1) + 3;
        if (x >= xa && x <= xb && y >= T.byt - 3 && y <= T.yb + 3) {
            lv_rd_sel = T.i; memcpy(lv_rd_selmac, lv_f[T.i].mac, 6);
            lv_full_line(lv_f[T.i], 1, lv_f[T.i].alert ? lv_matches() : 0, lv_tk_msg, sizeof lv_tk_msg);
            lv_tk_alert = lv_f[T.i].alert; lv_tk_off = 0;
            return "radar-tower";
        }
    }
    if (lv_rd_sel >= 0) { lv_rd_sel = -1; return "radar-deselect"; }
    return "none";
}
