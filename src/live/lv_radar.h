/* GIBSON radar (item 10): full-screen Hackers-style "Gibson" city. Included from lv_core.h.
 * Every find is a translucent neon tower: height + brightness = signal (dBm), ground position = MAC hash,
 * WiFi cyan, BLE magenta, hunt match red. Perspective ground grid scrolls toward you, a wireframe skyline on
 * the horizon, green code streaks (CONTROLbot RAIN / RAIN COL), camera DRIFT or a SWEEP line (RAIN MODE).
 * Tap a tower: its info shows in the panel and its full line goes on the ticker. BACKbot returns home. */
#include <math.h>
#include <stdarg.h>
static void lv_press(LvRect r);
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
/* ---- IGNORE / WATCH (owner 7:29-7:30 AM ET Oct 31): per-MAC lists kept in NVS (botcorps "rdign" / "rdwat") ---- */
static const uint16_t WATCH_COLOR = 0xA7E0;   /* RGB565 lime (RGB 160,252,0), theme-independent; rain is 0x20ff60, grid violet */
#define LV_RD_NL 16
static uint8_t lv_rd_ign[LV_RD_NL][6], lv_rd_wat[LV_RD_NL][6];
static int lv_rd_nign = 0, lv_rd_nwat = 0;
static bool lv_rd_lists_loaded = false;
static int lv_rd_lfind(uint8_t (*L)[6], int n, const uint8_t *m) { for (int i = 0; i < n; ++i) if (!memcmp(L[i], m, 6)) return i; return -1; }
static void lv_rd_lists_load(void)
{
    if (lv_rd_lists_loaded) return;
    lv_rd_lists_loaded = true;
    if (lv_radio && lv_radio->mac_load) {
        lv_rd_nign = lv_radio->mac_load("rdign", &lv_rd_ign[0][0], LV_RD_NL);
        lv_rd_nwat = lv_radio->mac_load("rdwat", &lv_rd_wat[0][0], LV_RD_NL);
        if (lv_rd_nign < 0 || lv_rd_nign > LV_RD_NL) lv_rd_nign = 0;
        if (lv_rd_nwat < 0 || lv_rd_nwat > LV_RD_NL) lv_rd_nwat = 0;
    }
}
static void lv_rd_lists_save(bool ign)
{
    if (lv_radio && lv_radio->mac_save) {
        if (ign) lv_radio->mac_save("rdign", &lv_rd_ign[0][0], lv_rd_nign);
        else lv_radio->mac_save("rdwat", &lv_rd_wat[0][0], lv_rd_nwat);
    }
}
static bool lv_rd_ignored(const uint8_t *m) { return lv_rd_lfind(lv_rd_ign, lv_rd_nign, m) >= 0; }
static bool lv_rd_watched(const uint8_t *m) { return lv_rd_lfind(lv_rd_wat, lv_rd_nwat, m) >= 0; }
static void lv_rd_ladd(uint8_t (*L)[6], int &n, const uint8_t *m)
{
    if (lv_rd_lfind(L, n, m) >= 0) return;
    if (n == LV_RD_NL) { memmove(L[0], L[1], 6 * (LV_RD_NL - 1)); --n; }   /* full: oldest drops */
    memcpy(L[n++], m, 6);
}
static void lv_rd_ldel(uint8_t (*L)[6], int &n, int i) { if (i < 0 || i >= n) return; memmove(L[i], L[i + 1], 6 * (n - i - 1)); --n; }
static uint16_t lv_rd_col(const LvFind &f, int num, int den)
{
    if (!f.alert && lv_rd_watched(f.mac)) return lv_dim(WATCH_COLOR, num, den);
    const uint32_t c = f.alert ? 0xff2020 : f.kind == LV_K_WIFI ? 0x00f7ff : 0xff2ad4;
    return lv_dim(lv_hex(c), num, den);
}
static int lv_rd_level(const LvFind &f) { int v = f.rssi + 100; if (v < 4) v = 4; if (v > 60) v = 60; return v; } /* 4..60 */
static void lv_rd_clamp(void);
static void lv_rd_place(void)
{
    lv_rd_n = 0;
    for (int i = 0; i < lv_nf; ++i) {
        const LvFind &f = lv_f[i]; const uint32_t h = lv_rd_hash(f.mac);
        if (lv_rd_ignored(f.mac) && i != lv_rd_sel) continue;   /* IGNORE: no tower (kept only while it is the target) */
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
static void lv_rd_layout(void)
{
    const float t = (lv_now - lv_rd_t0) / 1000.0f;
    lv_rd_camx = lv_rainmode == 0 ? 0.5f * sinf(t * 0.45f) : 0.0f;
    lv_rd_place();
    lv_rd_clamp();   /* while locked: camera DRIFT is clamped so the locked tower (and its brackets) stays fully on screen */
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
/* ---- item 8: TAP EMPTY SPACE TO CYCLE + HOLD TO LOCK ON (+ IGNORE / WATCH / HUNT), ported from the approved mocks
 * /workspace/out/gibson-ideas (mock1_tap_cycle, mock2_hold_lock). Height is the real dBm; the angle/position is a MAC hash,
 * NOT a direction. Nothing here draws unless a selection jump, a hold or a lock is active, so the base render is unchanged. */
static uint32_t lv_rd_glow_t = 0; static bool lv_rd_glow = false;     /* mock 1: 300 ms white glow on the tower a cycle jumped to */
static bool lv_rd_holding = false; static uint32_t lv_rd_hold_t = 0;   /* mock 2: brackets close in over a 1 s hold */
static uint8_t lv_rd_holdmac[6];
static bool lv_rd_lock = false; static uint32_t lv_rd_lock_t = 0; static uint8_t lv_rd_lockmac[6];
static bool lv_rd_swallow = false;   /* the release-edge tap that ends a lock-on hold must not act as a tap */
static bool lv_rd_hunting = false;   /* HUNT: focused re-scans of the locked device, live height / dBm / ticker */
static bool lv_rd_trk = false;       /* the running job is a hunt re-scan (no SCAN DONE event, no list reset) */
static LvFind lv_rd_tgt; static bool lv_rd_tgt_have = false;          /* last real reading of the hunt target */
static int lv_rd_trend = 0, lv_rd_reads = 0, lv_rd_miss = 0;
static int lv_rd_from = LV_SC_HOME;  /* where BACK goes (home, SCANbot list, TTY) */
static int lv_rd_ret_cat = -1, lv_rd_ret_cpage = 0, lv_rd_ret_mode = 0; static bool lv_rd_ret_detail = false; static uint8_t lv_rd_ret_mac[6]; static bool lv_rd_ret_sel = false;
static const LvRect LV_RD_BTN[3] = {{5, 250, 76, 260}, {84, 250, 155, 260}, {163, 250, 234, 260}};   /* IGNORE WATCH HUNT, inside the grown panel */
/* touch (hit-list item 1): finger targets for the three buttons, padded ~8 px inside the panel (panel y 247..285; the
 * radar field above y247 keeps its own taps) and split at the gap midpoints so they never overlap. Drawn rects unchanged. */
static const LvRect LV_RD_BTN_TAP[3] = {{0, 247, 80, 268}, {81, 247, 159, 268}, {160, 247, 239, 268}};
static int lv_rd_find(const uint8_t *m) { for (int i = 0; i < lv_nf; ++i) if (!memcmp(lv_f[i].mac, m, 6)) return i; return -1; }
static const LvTower *lv_rd_tower_of(const uint8_t *m) { const int i = lv_rd_find(m); if (i < 0) return nullptr; for (int n = 0; n < lv_rd_n; ++n) if (lv_rd_tw[n].i == i) return &lv_rd_tw[n]; return nullptr; }
static void lv_rd_box(const LvTower &T, int pad, int &xa, int &xb, int &ya, int &yb)
{
    xa = (T.x0 < T.bx0 ? T.x0 : T.bx0) - pad; xb = (T.x1 > T.bx1 ? T.x1 : T.bx1) + pad; ya = T.byt - pad; yb = T.yb + pad;
}
static void lv_rd_clamp(void)
{
    if (!lv_rd_lock) return;
    for (int it = 0; it < 3; ++it) {
        const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (!T) return;
        int xa, xb, ya, yb; lv_rd_box(*T, 4, xa, xb, ya, yb);   /* brackets sit at pad 3, +1 for the lock flash */
        int dx = 0; if (xa < 0) dx = xa; else if (xb > 239) dx = xb - 239;
        if (!dx) return;
        lv_rd_camx += dx * T->Z / LV_RD_F;   /* the front face moves the most (smallest Z), so this always covers the back face too */
        lv_rd_place();
    }
}
static void lv_rd_rect_blend(LvBand &B, int xa, int ya, int xb, int yb, uint16_t c, int k)
{
    for (int x = xa; x <= xb; ++x) { b_blend(B, x, ya, c, k); b_blend(B, x, yb, c, k); }
    for (int y = ya + 1; y < yb; ++y) { b_blend(B, xa, y, c, k); b_blend(B, xb, y, c, k); }
}
static void lv_rd_brackets(LvBand &B, int xa, int ya, int xb, int yb, int L, uint16_t c)
{
    b_line(B, xa, ya, xa + L, ya, c); b_line(B, xa, ya, xa, ya + L, c);
    b_line(B, xb, ya, xb - L, ya, c); b_line(B, xb, ya, xb, ya + L, c);
    b_line(B, xa, yb, xa + L, yb, c); b_line(B, xa, yb, xa, yb - L, c);
    b_line(B, xb, yb, xb - L, yb, c); b_line(B, xb, yb, xb, yb - L, c);
}
static void lv_rd_fx(LvBand &B)   /* drawn after the towers, before header / panel / footer (the mock's gib_draw hook) */
{
    const uint16_t W = lv_hex(0xffffff), MAG = lv_hex(0xff2ad4);
    if (lv_rd_glow && lv_rd_sel >= 0) {
        const int e = (int)(lv_now - lv_rd_glow_t);
        const LvTower *T = lv_rd_tower_of(lv_f[lv_rd_sel].mac);
        if (e >= 0 && e <= 300 && T) {
            const int k = 7 - e * 7 / 300; int xa, xb, ya, yb;
            lv_rd_box(*T, 1, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, W, k);
            lv_rd_box(*T, 2, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, W, k / 2);
        } else if (e > 300) lv_rd_glow = false;
    }
    if (lv_rd_holding && !lv_rd_lock) {
        const LvTower *T = lv_rd_tower_of(lv_rd_holdmac); if (!T) return;
        float u = (lv_now - lv_rd_hold_t) / 1000.0f; if (u > 1) u = 1; if (u < 0) u = 0;
        const int pad = 3 + (int)lroundf((1 - u) * 12); int xa, xb, ya, yb;
        lv_rd_box(*T, pad, xa, xb, ya, yb); lv_rd_brackets(B, xa, ya, xb, yb, 4, lv_dim(MAG, 4 + (int)(u * 4), 8));
        return;
    }
    if (!lv_rd_lock) return;
    const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (!T) return;
    const int e = (int)(lv_now - lv_rd_lock_t);
    const int pulse = 5 + (int)lroundf(3 * (0.5f + 0.5f * sinf(e / 1000.0f * 6.283f * 1.6f)));   /* 5..8 of 8 */
    int xa, xb, ya, yb; lv_rd_box(*T, 3, xa, xb, ya, yb);
    if (e < 200) lv_rd_rect_blend(B, xa - 1, ya - 1, xb + 1, yb + 1, W, 7 - e * 7 / 200);       /* lock snap flash */
    lv_rd_brackets(B, xa, ya, xb, yb, 4, lv_dim(MAG, pulse, 8));
    const int cx = (xa + xb) / 2; b_line(B, cx, ya - 1, cx, ya - 3, lv_dim(W, pulse, 8));
    const char *lab = lv_rd_hunting ? "HUNT" : "LOCK";   /* same 3x5 label, same width (4 chars) */
    int ly = ya - 9; if (ly < LV_RD_Y0 + 1) ly = LV_RD_Y0 + 1;
    b_text35(B, cx - lv_w35(lab) / 2, ly, lab, lv_dim(MAG, pulse, 8));
}
static void lv_rd_panel_buttons(LvBand &B)   /* locked: IGNORE / WATCH / HUNT in the radar's own panel style */
{
    const bool wat = lv_rd_watched(lv_rd_lockmac);
    const char *L[3] = {"IGNORE", wat ? "UNWATCH" : "WATCH", lv_rd_hunting ? "STOP HUNT" : "HUNT"};
    const uint16_t C[3] = {lv_hex(0xa0a0a8), WATCH_COLOR, lv_hex(0xff2ad4)};
    for (int k = 0; k < 3; ++k) {
        const LvRect &r = LV_RD_BTN[k]; const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
        const bool on = (k == 1 && wat) || (k == 2 && lv_rd_hunting);
        b_fill(B, r.x0, r.y0, w, h, on ? lv_dim(C[k], 3, 8) : lv_hex(0x000000));
        b_frame(B, r.x0, r.y0, w, h, C[k]);
        b_text35(B, r.x0 + (w - lv_w35(L[k])) / 2, r.y0 + 3, L[k], on ? lv_hex(0xffffff) : C[k]);
    }
}
/* item 13: radar mini terminal (3 lines, newest types out ~65 chars/s) */
#define LV_RD_TW 52
static char lv_rd_tl[3][LV_RD_TW + 1]; static uint32_t lv_rd_tl_t = 0;
static void lv_rd_log(const char *fmt, ...)
{
    char t[96]; va_list ap; va_start(ap, fmt); vsnprintf(t, sizeof t, fmt, ap); va_end(ap);
    for (char *q = t; *q; ++q) if (*q >= 'a' && *q <= 'z') *q -= 32;
    memmove(lv_rd_tl[0], lv_rd_tl[1], 2 * sizeof lv_rd_tl[0]); snprintf(lv_rd_tl[2], sizeof lv_rd_tl[2], "%s", t); lv_rd_tl_t = lv_now;
}
static void lv_rd_term(LvBand &B, uint16_t hdr)
{
    const int x0 = 1, y0 = 287, x1 = 218, y1 = 319;   /* BACKbot {221,301,239,319} stays outside */
    b_fill(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(0x000000)); b_frame(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_ink(LV_ACC));
    int typed = (int)((lv_now - lv_rd_tl_t) / 15);
    for (int l = 0; l < 3; ++l) {
        char t[LV_RD_TW + 1]; snprintf(t, sizeof t, "%s", lv_rd_tl[l]);
        if (l == 2 && typed < (int)strlen(t)) t[typed < 0 ? 0 : typed] = 0;
        b_text35(B, x0 + 3, y0 + 3 + l * 7, t, l == 2 ? lv_hex(0xffffff) : lv_hex(0xa0a0a8));
    }
    b_fill(B, x0 + 2, y0 + 23, x1 - x0 - 3, 1, lv_ink(LV_ACCD));
    const int py = y0 + 25; int x = b_text35(B, x0 + 3, py, ">", lv_hex(LV_PROMPT_INK));
    if ((lv_now / 500) & 1) b_fill(B, x + 1, py + 4, 4, 1, lv_hex(0xffffff));
    char c[24]; int nw = 0; for (int i = 0; i < lv_nf; ++i) nw += lv_f[i].kind == LV_K_WIFI;
    snprintf(c, sizeof c, "%dW %dB", nw, lv_nf - nw);
    int rx = x1 - 3 - lv_w35(c); b_text35(B, rx, py, c, hdr);
    rx -= 4 + 5 + lv_w35("ALRT"); b_fill(B, rx, py, 3, 5, lv_hex(0xff2020)); b_text35(B, rx + 5, py, "ALRT", lv_hex(0xff4040));
    rx -= 4 + 5 + lv_w35("BLE"); b_fill(B, rx, py, 3, 5, lv_hex(0xff2ad4)); b_text35(B, rx + 5, py, "BLE", lv_hex(0xff2ad4));
    rx -= 4 + 5 + lv_w35("WIFI"); b_fill(B, rx, py, 3, 5, lv_hex(0x00f7ff)); b_text35(B, rx + 5, py, "WIFI", lv_hex(0x00f7ff));
    rx -= 4 + lv_w35("HEIGHT=SIGNAL"); b_text35(B, rx, py, "HEIGHT=SIGNAL", lv_hex(0xffffff));
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
    lv_rd_fx(B);
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
        const int py0 = lv_rd_lock ? 247 : 262;   /* locked: the panel grows up 15 px for the IGNORE / WATCH / HUNT row */
        b_fill(B, 2, py0, 236, 285 - py0, lv_hex(0x000000)); b_frame(B, 2, py0, 236, 285 - py0, lv_rd_col(f, 8, 8));
        if (lv_rd_lock) lv_rd_panel_buttons(B);
        b_text(B, 5, 264, l, lv_hex(0xffffff));
        snprintf(t, sizeof t, "%d", f.rssi); b_text(B, 235 - ((int)strlen(t) + 4) * 6, 264, t, lv_rd_col(f, 8, 8)); b_text35(B, 235 - 4 * 6 + 4, 265, "DBM", lv_rd_col(f, 8, 8));
        snprintf(t, sizeof t, "%s %s  %s", f.kind == LV_K_WIFI ? "WIFI" : "BLE", LV_ICON_NAME[f.icon], m); b_text35(B, 5, 275, t, lv_hex(LV_TAN));
    }
    /* footer (item 13): a 3-line boxed mini terminal that types out live radar info over a >_ prompt, legend folded into the prompt */
    lv_rd_term(B, hdr);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
/* hunt ticker line: the target's full info + a trend that is ONLY the change of its real dBm (no direction) */
static void lv_rd_hunt_line(void)
{
    if (!lv_rd_tgt_have) return;
    char l[LV_TKLEN]; lv_full_line(lv_rd_tgt, 0, 0, l, sizeof l);
    const char *tr = lv_rd_reads < 2 ? "FIRST READ" : lv_rd_trend > 0 ? "WARMER" : lv_rd_trend < 0 ? "COLDER" : "STEADY";
    char d[24] = ""; if (lv_rd_reads >= 2 && lv_rd_trend) snprintf(d, sizeof d, " %+ddB", lv_rd_trend);
    if (lv_rd_miss) snprintf(lv_tk_msg, sizeof lv_tk_msg, "HUNT: NO SIGNAL x%d (LAST %ddBm)  %s", lv_rd_miss, lv_rd_tgt.rssi, l);
    else snprintf(lv_tk_msg, sizeof lv_tk_msg, "HUNT: %s  %s%s  #%d", l, tr, d, lv_rd_reads);
    lv_tk_alert = false;
}
static void lv_rd_track_start(void)   /* one focused re-scan of the target's radio (the radio API has no per-channel / per-address scan) */
{
    if (lv_job != LV_J_NONE || !lv_rd_tgt_have) return;
    lv_rd_trk = true; lv_hunt = -1;
    lv_job = lv_rd_tgt.kind == LV_K_WIFI ? LV_J_WIFI : LV_J_BLE; lv_job_phase = 0; lv_job_t0 = lv_now;
    const int r = lv_job == LV_J_WIFI ? lv_radio->wifi_start() : lv_radio->ble_start(1500);   /* BLE: 1.5 s windows */
    if (r != 0) { lv_job = LV_J_NONE; lv_rd_trk = false; }
}
/* called by lv_job_tick after a hunt re-scan stored its results */
static void lv_rd_track_done(void)
{
    lv_rd_trk = false;
    const int i = lv_rd_find(lv_rd_tgt.mac);
    if (i >= 0) {
        const int was = lv_rd_tgt.rssi; lv_rd_tgt = lv_f[i]; lv_rd_miss = 0;
        const int d = lv_rd_tgt.rssi - was; lv_rd_trend = (d >= 3 || d <= -3) ? d : 0;   /* +-2 dB = STEADY (noise) */
        ++lv_rd_reads;
        lv_rd_log("#%d %dDBM %s", lv_rd_reads, lv_rd_tgt.rssi, lv_rd_trend > 0 ? "WARMER" : lv_rd_trend < 0 ? "COLDER" : "STEADY");
        if (lv_rd_trend) { char *e = lv_rd_tl[2] + strlen(lv_rd_tl[2]); snprintf(e, sizeof lv_rd_tl[2] - (e - lv_rd_tl[2]), " %+dDB", lv_rd_trend); }
    } else if (lv_nf < LV_MAXF) {   /* not heard this pass: keep its last real reading as the target, say so */
        ++lv_rd_miss; lv_f[lv_nf++] = lv_rd_tgt; lv_sort();
        lv_rd_log("NO SIGNAL X%d  LAST %dDBM", lv_rd_miss, lv_rd_tgt.rssi);
    }
    lv_rd_sel = lv_rd_find(lv_rd_tgt.mac);
    lv_rd_hunt_line(); lv_tk_off = 196;
}
static void lv_rd_hunt_stop(void)
{
    if (lv_rd_hunting && lv_rd_trk && lv_job != LV_J_NONE) { lv_radio->stop(); lv_job = LV_J_NONE; }
    lv_rd_hunting = false; lv_rd_trk = false; lv_dirty_box = true;
}
static void lv_rd_lock_on(int i)
{
    lv_rd_lock = true; lv_rd_lock_t = lv_now; memcpy(lv_rd_lockmac, lv_f[i].mac, 6);
    lv_rd_sel = i; memcpy(lv_rd_selmac, lv_f[i].mac, 6); lv_rd_holding = false;
    char m[18]; lv_macstr(lv_f[i].mac, m); lv_rd_log("LOCK %s  IGNORE / WATCH / HUNT", m);
}
static void lv_rd_hunt_go(int i)
{
    if (i < 0 || i >= lv_nf) return;
    if (!lv_rd_lock || memcmp(lv_rd_lockmac, lv_f[i].mac, 6)) lv_rd_lock_on(i);
    lv_rd_hunting = true; lv_rd_tgt = lv_f[i]; lv_rd_tgt_have = true; lv_rd_reads = 1; lv_rd_trend = 0; lv_rd_miss = 0;
    lv_rd_hunt_line(); lv_tk_off = 196;
    { char l[26]; lv_label(lv_f[i], l, 20); lv_rd_log("HUNT %s  RESCAN %s", l, lv_f[i].kind == LV_K_WIFI ? "WIFI" : "BLE"); }
    lv_rd_track_start();
}
static void lv_rd_release(void)
{
    lv_rd_hunt_stop();
    lv_rd_lock = false; lv_rd_holding = false;
}
static void lv_radar_enter(void)
{
    lv_rd_lists_load();
    { int nw = 0; for (int i = 0; i < lv_nf; ++i) nw += lv_f[i].kind == LV_K_WIFI;
      lv_rd_log("GIBSON UP  %dW %dB  HEIGHT=DBM", nw, lv_nf - nw); lv_rd_log("ANGLE/DEPTH = MAC HASH, NOT DIRECTION"); lv_rd_log(lv_nf ? "TAP=CYCLE  HOLD 1S=LOCK" : "NO NODES YET - SCANNING"); lv_rd_tl_t = lv_now; }
    lv_rd_t0 = lv_now; lv_rd_sel = -1; lv_rd_glow = false; lv_rd_holding = false; lv_rd_swallow = false;
    if (lv_nf == 0 && lv_job == LV_J_NONE) lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1);
}
static void lv_radar_leave(void) { if (lv_rd_hunting) lv_tk_off = 1 << 14; lv_rd_release(); lv_rd_sel = -1; }
static void lv_radar_frame(void)
{
    if (lv_rd_sel >= 0) { /* keep the selection on the same device across re-sorts */
        int k = -1; for (int i = 0; i < lv_nf; ++i) if (!memcmp(lv_f[i].mac, lv_rd_selmac, 6)) { k = i; break; }
        lv_rd_sel = k;
        if (k < 0 && lv_rd_lock && !lv_rd_hunting) lv_rd_lock = false;   /* locked device left the list */
    }
    static int jprev = LV_J_NONE;
    if (jprev != LV_J_NONE && lv_job == LV_J_NONE && !lv_rd_hunting) { int nw = 0; for (int i = 0; i < lv_nf; ++i) nw += lv_f[i].kind == LV_K_WIFI; lv_rd_log("SCAN DONE %dW %dB", nw, lv_nf - nw); }
    jprev = lv_job;
    if (lv_rd_hunting && lv_job == LV_J_NONE) lv_rd_track_start();   /* back-to-back re-scans, as fast as the radio allows */
    lv_rd_layout();
    lv_render(lv_blit, LV_R_RADAR, lv_scene_radar, 0);
}
static int lv_rd_hit(int x, int y)   /* index into lv_f of the nearest tower under (x,y), -1 none */
{
    for (int n = lv_rd_n - 1; n >= 0; --n) {    /* near first */
        const LvTower &T = lv_rd_tw[n]; int xa, xb, ya, yb; lv_rd_box(T, 3, xa, xb, ya, yb);
        if (x >= xa && x <= xb && y >= ya && y <= yb) return T.i;
    }
    return -1;
}
static void lv_rd_select(int i)   /* what a tower tap always did: select + its full line on the ticker */
{
    lv_rd_sel = i; memcpy(lv_rd_selmac, lv_f[i].mac, 6);
    lv_full_line(lv_f[i], 1, lv_f[i].alert ? lv_matches() : 0, lv_tk_msg, sizeof lv_tk_msg);
    lv_tk_alert = lv_f[i].alert; lv_tk_off = 0;
    char l[26]; lv_label(lv_f[i], l, 20); lv_rd_log("SEL %s %dDBM %s%s", l, lv_f[i].rssi, lv_f[i].kind == LV_K_WIFI ? "WIFI CH" : "BLE", lv_f[i].kind == LV_K_WIFI ? "" : "");
    if (lv_f[i].kind == LV_K_WIFI) { char *e = lv_rd_tl[2] + strlen(lv_rd_tl[2]); snprintf(e, sizeof lv_rd_tl[2] - (e - lv_rd_tl[2]), "%d", lv_f[i].ch); }
}
/* touch state every loop (firmware: lv_ui.cpp; sim: press / rhold). Hold a tower ~1 s -> lock on. */
static void lv_radar_touch(bool down, int x, int y)
{
    if (!down) { lv_rd_holding = false; return; }
    if (lv_rd_lock || y < LV_RD_Y0 || y > LV_RD_Y1) return;
    if (!lv_rd_holding) {
        const int i = lv_rd_hit(x, y); if (i < 0) return;
        lv_rd_holding = true; lv_rd_hold_t = lv_now; memcpy(lv_rd_holdmac, lv_f[i].mac, 6);
        lv_rd_select(i);   /* the press selects at once (mock 2), the panel shows it while the brackets close */
        return;
    }
    const LvTower *T = lv_rd_tower_of(lv_rd_holdmac);
    int xa = 0, xb = -1, ya = 0, yb = -1; if (T) lv_rd_box(*T, 6, xa, xb, ya, yb);
    if (!T || x < xa || x > xb || y < ya || y > yb) { lv_rd_holding = false; return; }   /* slid off: no lock */
    if ((int32_t)(lv_now - lv_rd_hold_t) >= 1000) { const int i = lv_rd_find(lv_rd_holdmac); if (i >= 0) { lv_rd_lock_on(i); lv_rd_swallow = true; } }
}
static int lv_rd_cycle_next(void)   /* strongest -> weakest dBm, wraps; ignored devices skipped */
{
    int ord[LV_MAXF], n = 0;
    for (int i = 0; i < lv_nf; ++i) if (!lv_rd_ignored(lv_f[i].mac)) ord[n++] = i;
    for (int a = 1; a < n; ++a) { const int t = ord[a]; int b = a - 1; while (b >= 0 && lv_f[ord[b]].rssi < lv_f[t].rssi) { ord[b + 1] = ord[b]; --b; } ord[b + 1] = t; }
    if (!n) return -1;
    int p = -1; for (int k = 0; k < n; ++k) if (ord[k] == lv_rd_sel) p = k;
    return ord[(p + 1) % n];
}
static const char *lv_radar_tap(int x, int y)
{
    if (lv_rd_swallow) { lv_rd_swallow = false; return "radar-locked"; }
    if (y > LV_RD_Y1 + 1) return "radar-term";   /* the footer terminal is read-only (BACKbot is handled before this) */
    if (lv_rd_lock) {
        for (int k = 0; k < 3; ++k) if (lv_in(LV_RD_BTN_TAP[k], x, y)) {
            lv_press(LV_RD_BTN[k]);
            const int i = lv_rd_find(lv_rd_lockmac);
            if (k == 0) {   /* IGNORE: gone from the radar + tap-cycle, kept in NVS; lock released */
                lv_rd_ladd(lv_rd_ign, lv_rd_nign, lv_rd_lockmac); lv_rd_lists_save(true);
                const int w = lv_rd_lfind(lv_rd_wat, lv_rd_nwat, lv_rd_lockmac); if (w >= 0) { lv_rd_ldel(lv_rd_wat, lv_rd_nwat, w); lv_rd_lists_save(false); }
                char m[18]; lv_macstr(lv_rd_lockmac, m); lv_event(false, "IGNORED %s  (TTY: unignore)", m); lv_rd_log("IGNORED %s  TTY: UNIGNORE", m);
                lv_rd_release(); lv_rd_sel = -1; return "radar-ignore";
            }
            if (k == 1) {   /* WATCH toggles; the tower turns WATCH_COLOR */
                const int w = lv_rd_lfind(lv_rd_wat, lv_rd_nwat, lv_rd_lockmac);
                if (w >= 0) lv_rd_ldel(lv_rd_wat, lv_rd_nwat, w); else lv_rd_ladd(lv_rd_wat, lv_rd_nwat, lv_rd_lockmac);
                lv_rd_lists_save(false); { char m[18]; lv_macstr(lv_rd_lockmac, m); lv_rd_log("%s %s", w >= 0 ? "UNWATCH" : "WATCH", m); } return w >= 0 ? "radar-unwatch" : "radar-watch";
            }
            if (lv_rd_hunting) { lv_rd_hunt_stop(); lv_rd_log("HUNT STOPPED  %d READS", lv_rd_reads); return "radar-hunt-stop"; }
            lv_rd_hunt_go(i); return "radar-hunt";
        }
        if (y >= 247 && y <= 285) return "radar-panel";   /* the panel itself does not release the lock */
        const int i = lv_rd_hit(x, y);
        if (i >= 0 && !memcmp(lv_f[i].mac, lv_rd_lockmac, 6)) return "radar-locked";
        lv_rd_release(); lv_rd_log("RELEASED"); return "radar-unlock";   /* a tap anywhere else releases the lock (and ends a hunt) */
    }
    const int i = lv_rd_hit(x, y);
    if (i >= 0) { lv_rd_select(i); return "radar-tower"; }
    const int nx = lv_rd_cycle_next();
    if (nx < 0) return "none";
    lv_rd_select(nx); lv_rd_glow = true; lv_rd_glow_t = lv_now; return "radar-cycle";
}
/* open the radar locked + hunting device i (SCANbot HUNT, TTY 'hunt <x>'); BACK returns to `from` */
static void lv_rd_hunt_open(int i, int from)
{
    (void)from;   /* lv_go records where the radar was opened from (BACK goes there) */
    uint8_t mac[6]; memcpy(mac, lv_f[i].mac, 6);
    lv_go(LV_SC_RADAR);
    const int k = lv_rd_find(mac);
    if (k >= 0) { lv_rd_lock_t = lv_now; lv_rd_hunt_go(k); }
}
static void lv_rd_save_scan(void)
{
    lv_rd_ret_cat = lv_cat; lv_rd_ret_cpage = lv_cpage; lv_rd_ret_mode = lv_mode; lv_rd_ret_detail = lv_detail; lv_rd_ret_sel = false;
    if (lv_cat >= 0 && lv_sel >= 0) { int r[LV_MAXF]; const int n = lv_cat_rows(r); if (lv_sel < n) { memcpy(lv_rd_ret_mac, lv_f[r[lv_sel]].mac, 6); lv_rd_ret_sel = true; } }
}
static void lv_rd_restore_scan(void)   /* back on SCANbot exactly where HUNT was tapped (hunt re-scans re-sort the list) */
{
    lv_mode = lv_rd_ret_mode; lv_cat = lv_rd_ret_cat; lv_cpage = lv_rd_ret_cpage; lv_sel = -1; lv_detail = false;
    if (lv_cat >= 0 && lv_rd_ret_sel) {
        int r[LV_MAXF]; const int n = lv_cat_rows(r);
        for (int k = 0; k < n; ++k) if (!memcmp(lv_f[r[k]].mac, lv_rd_ret_mac, 6)) { lv_sel = k; lv_cpage = k / 8; lv_detail = lv_rd_ret_detail; }
    }
    lv_dirty_box = true;
}
static bool lv_rd_is_trk(void) { return lv_rd_trk; }
static bool lv_rd_hunt_tk(void) { if (!lv_rd_hunting || lv_scr != LV_SC_RADAR) return false; lv_rd_hunt_line(); lv_tk_off = 0; return true; }
/* TTY 'ignored' (a = "?") lists; 'unignore' / 'unignore all' clears all; 'unignore <mac|part>' one. Returns how many removed. */
static int lv_rd_unignore(const char *a)
{
    lv_rd_lists_load();
    if (a && !strcmp(a, "?")) {
        if (!lv_rd_nign) lv_print("ignored: none");
        for (int i = 0; i < lv_rd_nign; ++i) { char m[18]; lv_macstr(lv_rd_ign[i], m); lv_print("ignored %d %s", i, m); }
        return -1;
    }
    int n = 0;
    if (!a || !strcmp(a, "all")) { n = lv_rd_nign; lv_rd_nign = 0; }
    else for (int i = lv_rd_nign - 1; i >= 0; --i) { char m[18]; lv_macstr(lv_rd_ign[i], m); if (lv_has(m, a)) { lv_rd_ldel(lv_rd_ign, lv_rd_nign, i); ++n; } }
    lv_rd_lists_save(true);
    return n;
}
