/* GIBSON radar (item 10): full-screen Hackers-style "Gibson" city. Included from lv_core.h.
 * Every find is a translucent neon tower: height + brightness = signal (dBm), ground position = MAC hash,
 * WiFi cyan, BLE magenta, hunt match red. Perspective ground grid scrolls toward you, a wireframe skyline on
 * the horizon, green code streaks (CONTROLbot RAIN / RAIN COL), camera DRIFT or a SWEEP line (RAIN MODE).
 * Tap a tower: its info shows in the panel and its full line goes on the ticker. BACKbot returns home. */
#include <math.h>
#include <stdarg.h>
#include "lv_select.h"   /* v1.2.11 LOCKED: the shared sub-screen selection (pull-up + folder drop) */
static void lv_press(LvRect r);
#define LV_RD_F 120.0f
#define LV_RD_HOR 138
#define LV_RD_CAMH 0.9f
#define LV_RD_Y0 34
#define LV_RD_Y1 246   /* v1.2.8: wider Joey deck starts at 247; was 285 */
static const LvRect LV_R_RADAR = {0, 24, 239, 319};
static uint32_t lv_rd_t0 = 0;
static int lv_rd_sel = -1;           /* index into lv_f */
static uint8_t lv_rd_selmac[6];
static float lv_rd_camx = 0;
struct LvTower { int i; float X, Z, H; int x0, x1, yt, yb, bx0, bx1, byt, byb; };
static LvTower lv_rd_tw[LV_MAXF];
static int lv_rd_n = 0;
#ifndef LV_RD_SKINS
#define LV_RD_SKINS 0
#endif
#if LV_RD_SKINS
static int lv_rd_skin = 0;            /* MOCK 1112: 0 = locked (byte-identical); 1 = MO Dojo Construct; 2 = TR silver; 3 = CT theme */
static uint32_t lv_rd_skin_acc = 0xffffff;  /* skin 3 (CT) accent hex */
static float lv_rd_skin_tw = 1.0f;     /* skin tower-width scale (1 = locked) */
static int   lv_rd_skin_fa = 3;        /* skin tower-face blend /8 (3 = locked) */
static float lv_rd_skin_th = 1.0f;     /* MOCK 1113: skin tower-height scale (1 = locked; MO v11 keeps 1) */
#if LV_MO_INSIDE
/* MOCK 1113: skin 1 = MORPHEUSbot v11 "Dojo Construct" (lv_morpheus_v11.h). Skin 1 runs the Gibson scene code (sky ramp, grid rows +
 * lanes, code rain, lv_rd_tw far -> near towers) with dojo hooks: mo11_drift (lv_rd_place: each tower its own X / Z / H path),
 * mo11_horizon (the skyline slots as a shoji wall + rafters), mo11_tatami (floor mats under the grid), mo11_ink, mo11_tower_fx, mo11_overlay. */
static void mo11_drift(LvTower &T);
static void mo11_horizon(LvBand &B, int shift);
static void mo11_tatami(LvBand &B, int y, float Z, float ph);
static void mo11_ink(const LvTower &T, uint16_t &face, uint16_t &edge);
static void mo11_tower_fx(LvBand &B, const LvTower &T);
static bool mo11_body_rows(const LvTower &T, int &ra, int &rb);   /* perf-all r2: rows the Dojo body will paint opaque over T's front face */
static void mo11_overlay(LvBand &B);
#endif
#endif
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
    const uint32_t c = f.alert ? 0xff2020 : f.kind == LV_K_WIFI ? LV_WIFI_COL : LV_BLE_COL;
    return lv_dim(lv_hex(c), num, den);
}
static int lv_rd_level(const LvFind &f) { int v = f.rssi + 100; if (v < 4) v = 4; if (v > 60) v = 60; return v; } /* 4..60 */
static void lv_rd_clamp(void);
/* v1.2.13 lv_select.h pull-up on GIBSON / HACKbot DOWNLOAD: the screen that owns the selection sets the tower (MAC), the eased
 * progress p and itself; lv_rd_place then flies that one tower toward the viewer. p = 0 (or another screen): untouched. */
static float lv_rd_pull_p = 0; static uint8_t lv_rd_pull_mac[6]; static int lv_rd_pull_scr = -1;
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
        float w = 0.12f; const float d = 0.24f;
#if LV_RD_SKINS
        w *= lv_rd_skin_tw; T.H *= lv_rd_skin_th;
#endif
#if LV_RD_SKINS && LV_MO_INSIDE
        if (lv_rd_skin == 1) mo11_drift(T);   /* MO v11: every tower its own 3D drift (X, Z, breathing H); the sort below re-orders far -> near */
#endif
        if (lv_rd_pull_p > 0 && lv_rd_pull_scr == lv_scr && !memcmp(f.mac, lv_rd_pull_mac, 6)
#if LV_RD_SKINS
            && lv_rd_skin == 0
#endif
           ) { const float c = lv_rd_camx;
               lv_sel_pull(c, lv_sel_close_x(c, LV_SEL_SX, LV_SEL_ZC, LV_RD_F), LV_SEL_ZC, lv_sel_close_h(LV_RD_CAMH, LV_RD_HOR, LV_SEL_YT, LV_SEL_ZC, LV_RD_F), lv_rd_pull_p, LV_RD_F, T.X, T.Z, T.H); }
        auto px = [&](float X, float Z) { return (int)lv_lroundf(120 + LV_RD_F * (X - lv_rd_camx) / Z); };
        auto py = [&](float Y, float Z) { return (int)lv_lroundf(LV_RD_HOR + LV_RD_F * (LV_RD_CAMH - Y) / Z); };
        T.x0 = px(T.X - w, T.Z); T.x1 = px(T.X + w, T.Z); T.yt = py(T.H, T.Z); T.yb = py(0, T.Z);
        T.bx0 = px(T.X - w, T.Z + d); T.bx1 = px(T.X + w, T.Z + d); T.byt = py(T.H, T.Z + d); T.byb = py(0, T.Z + d);
#if LV_RD_SKINS && LV_MO_INSIDE && defined(MO_EB2)
        if (lv_rd_skin == 1 && T.i >= 0 && T.i < 64) {
            const float kq = mo_eb2_k[T.i] > 0 ? mo_eb2_k[T.i] : 1.0f; float ww = w * kq, dd = d * kq;   /* near 5: footprint scales with the move */
            const float wmin = MO_EB2_MINFW * T.Z / (2.0f * LV_RD_F);   /* far faces >= ~10 px; side scaled by the same factor */
            if (ww < wmin) { dd *= wmin / ww; ww = wmin; }
            if (ww != w || dd != d) {
                T.x0 = px(T.X - ww, T.Z); T.x1 = px(T.X + ww, T.Z);
                T.bx0 = px(T.X - ww, T.Z + dd); T.bx1 = px(T.X + ww, T.Z + dd); T.byt = py(T.H, T.Z + dd); T.byb = py(0, T.Z + dd); } }
#endif
        lv_rd_tw[lv_rd_n++] = T;
    }
    for (int a = 1; a < lv_rd_n; ++a) { LvTower t = lv_rd_tw[a]; int b = a - 1; while (b >= 0 && lv_rd_tw[b].Z < t.Z) { lv_rd_tw[b + 1] = lv_rd_tw[b]; --b; } lv_rd_tw[b + 1] = t; } /* far -> near */
}
static bool lv_rd_cam_hold = false;   /* v1.2.4: HACKbot open-tower camera; skip sway + clamp so the tower stays at x=50 next to the folders */
/* v1.2.11 (Mobandy 2026-11-20 "on gibson game when hitting a button the tower should advance like dojo tower and show the
 * folders and that is where the hunt and all other ui exists" + "Replace the center buttons with a speed increase button"):
 * LV_RD_FOLD 1 = a lock (A or hold-charge) advances the camera onto the locked tower (HACKbot DOWNLOAD's own open-tower
 * camera law: front face to x 50, eased 400 ms) and drops three folders HUNT / IGNORE / WATCH that carry those actions;
 * the deck's centre stack is one SPEED button (1X -> 2X -> 3X: the radar's camera / scene clock). 0 = v1.2.10 deck. */
#ifndef LV_RD_FOLD
#define LV_RD_FOLD 1
#endif
#if LV_RD_FOLD
static int lv_rd_fold_mode = 0;   /* 0 off, 1 advancing / open, 2 easing back to the drift */
static uint32_t lv_rd_fold_t = 0; static LvSelClk lv_rd_fclk; static float lv_rd_fold_from = 0, lv_rd_fold_p0 = 0; static uint8_t lv_rd_fold_mac[6];
static int lv_rd_speed = 1; static uint32_t lv_rd_soff = 0, lv_rd_slast = 0;   /* scene clock = lv_now + soff (radar screen only) */
#define LV_RD_SOFF (lv_scr == LV_SC_RADAR ? lv_rd_soff : 0u)
#define LV_RD_FCAM_MS 400
#define LV_RD_FDROP0 300
#define LV_RD_FGAP 110
#define LV_RD_FDROP 380
static const LvRect LV_RD_FTAP[3] = {{88, 33, 238, 98}, {88, 99, 238, 164}, {88, 165, 238, 230}};   /* 151 x 66 finger boxes */
#else
#define LV_RD_SOFF 0u
#endif
static void lv_rd_layout(void)
{
    const float t = (lv_now - lv_rd_t0 + LV_RD_SOFF) / 1000.0f;
    if (!lv_rd_cam_hold) lv_rd_camx = lv_rainmode == 0 ? 0.5f * sinf(t * 0.45f) : 0.0f;
    lv_rd_place();
    if (!lv_rd_cam_hold) lv_rd_clamp();   /* while locked: camera DRIFT is clamped so the locked tower (and its brackets) stays fully on screen */
}
static inline void b_blend(LvBand &B, int x, int y, uint16_t c, int k)
{
    if (x >= B.x0 && x < B.x0 + B.w && y >= B.y0 && y < B.y0 + B.h) { uint16_t &p = B.px[(y - B.y0) * B.w + (x - B.x0)]; p = lv_mix(p, c, k, 8); }
}
static void b_line(LvBand &B, int x0, int y0, int x1, int y1, uint16_t c)
{
    if ((y0 < B.y0 && y1 < B.y0) || (y0 >= B.y0 + B.h && y1 >= B.y0 + B.h)) return;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
    const int ylo = B.y0, yhi = B.y0 + B.h;   /* perf-all: y is monotonic, so stop once it has left the band for good (same pixels) */
    for (int n = 0; n < 800; ++n) { if (y0 >= ylo && y0 < yhi) b_pix(B, x0, y0, c); else if (sy > 0 ? y0 >= yhi : y0 < ylo) break;
        if (x0 == x1 && y0 == y1) break; const int e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
}
static void lv_rd_vclip(LvBand &B, int &ya, int &yb) { if (ya < LV_RD_Y0) ya = LV_RD_Y0; if (yb > LV_RD_Y1) yb = LV_RD_Y1; (void)B; }
#if LV_BG_ENGINE
/* BG COLOR: element e of the background layer. MULTI gives each element its own hue and walks the palette slowly. */
static uint16_t lv_rd_bg_col(int e, int k)
{
    static const uint32_t MULTI[5] = {0x20ff60, 0x30e8ff, 0xb040ff, 0xffb000, 0xff2ad4};
    uint32_t c;
    switch (lv_raincol) {
    case 0: c = 0x20ff60; break;
    case 1: c = 0x30e8ff; break;
    case 2: c = LV_ACC[lv_theme]; break;
    case 3: c = MULTI[(unsigned)(e + lv_now / 1500) % 5]; break;
    case 4: c = 0xb040ff; break;
    default: c = 0xffb000; break;
    }
    return lv_dim(lv_hex(c), k, 8);
}
/* a tiny bot (BOTS / FLYBY): head box, two eyes, antenna; sc = pixel scale, k = blend 0..8 */
static void lv_rd_bot(LvBand &B, int x, int y, int sc, uint16_t c, int k)
{
    static const uint8_t BOT[7] = {0x08, 0x08, 0x7f, 0x41, 0x55, 0x41, 0x7f};   /* 7 x 7, MSB = left */
    for (int r = 0; r < 7; ++r) for (int q = 0; q < 7; ++q) if (BOT[r] & (0x40 >> q))
        for (int a = 0; a < sc; ++a) for (int b = 0; b < sc; ++b) { const int yy = y + r * sc + a; if (yy >= LV_RD_Y0 && yy <= LV_RD_Y1) b_blend(B, x + q * sc + b, yy, c, k); }
}
/* background engine: the GIBSON field's live layer between the grid and the towers (the only free background on screen) */
static void lv_rd_bg(LvBand &B)
{
    if (!lv_rain) return;
    const float ts = lv_now / 1000.0f;
    switch (lv_bgstyle) {
    case 0:   /* RAIN: the original code streaks, unchanged */
        for (int s = 0; s < 18; ++s) {
            const int x = (s * 53 + 17) % 236 + 2, len = 10 + (s * 7) % 18, sp = 40 + (s * 29) % 70;
            const int head = LV_RD_Y0 + (int)((lv_now / 1000.0f * sp + s * 37)) % (LV_RD_Y1 - LV_RD_Y0 + len) ;
            for (int k = 0; k < len; k += 2) { const int y = head - k; if (y >= LV_RD_Y0 && y <= LV_RD_Y1) b_blend(B, x, y, lv_rd_bg_col(s, 8), k == 0 ? 8 : 6 - k * 5 / len); }
        }
        break;
    case 1:   /* BOTS: a slow swarm of little bots drifting down the field and wrapping */
        for (int s = 0; s < 9; ++s) {
            const int span = LV_RD_Y1 - LV_RD_Y0 + 8, sp = 9 + (s * 7) % 13;
            const int y = LV_RD_Y0 - 7 + (int)(ts * sp + s * 41) % span;
            const int x = (s * 61 + 11) % 226 + (int)(6 * sinf(ts * 0.7f + s));
            lv_rd_bot(B, x, y, 1, lv_rd_bg_col(s, 8), 5);
        }
        break;
    case 2:   /* GIBSON: data packets racing along the ground-grid lanes toward you, plus uplinks rising off the horizon */
        for (int p = 0; p < 24; ++p) {
            const float g = (float)((p * 7) % 25 - 12) * 0.5f, life = 2.2f + (p % 4) * 0.4f;
            const float u = fmodf(ts / life + p * 0.37f, 1.0f), Z = 7.0f - u * 6.4f;
            for (int tr = 0; tr < 4; ++tr) {
                const float Zt = Z + tr * 0.25f;
                const int y = (int)lv_lroundf(LV_RD_HOR + LV_RD_F * LV_RD_CAMH / Zt), x = (int)lv_lroundf(120 + LV_RD_F * (g - lv_rd_camx) / Zt);
                if (y <= LV_RD_HOR || y > LV_RD_Y1) continue;
                const int k = tr == 0 ? 8 : 6 - tr * 2, sz = tr == 0 ? (Z < 2.5f ? 3 : 2) : 1;
                for (int a = 0; a < sz; ++a) for (int b = 0; b < sz; ++b) b_blend(B, x + b, y + a, lv_rd_bg_col(p, 8), k);
            }
        }
        for (int s = 0; s < 6; ++s) {   /* uplinks: short dashes climbing from the skyline */
            const int x = (s * 41 + 23) % 232 + 4, len = 6 + s % 4;
            const int h = (int)(ts * (18 + s * 5) + s * 29) % (LV_RD_HOR - LV_RD_Y0);
            for (int k = 0; k < len; ++k) { const int y = LV_RD_HOR - 1 - h + k; if (y >= LV_RD_Y0 && y < LV_RD_HOR) b_blend(B, x, y, lv_rd_bg_col(s + 3, 8), k == 0 ? 8 : 6 - k * 5 / len); }
        }
        break;
    case 3: {   /* FLYBY: one bot crosses the sky with a fading trail, a new height every pass */
        const float per = 5.0f; const int pass = (int)(ts / per); const float u = fmodf(ts, per) / per;
        const int dir = pass & 1 ? -1 : 1, y0 = LV_RD_Y0 + 14 + (pass * 37) % 70;
        const int x = dir > 0 ? -16 + (int)(u * 272) : 256 - (int)(u * 272), y = y0 + (int)(6 * sinf(u * 6.28f));
        const uint16_t c = lv_rd_bg_col(pass, 8);
        for (int tr = 1; tr <= 22; ++tr) {   /* trail */
            const float uu = u - tr * 0.006f; const int tx = dir > 0 ? -16 + (int)(uu * 272) : 256 - (int)(uu * 272), ty = y0 + (int)(6 * sinf(uu * 6.28f));
            const int k = 7 - tr * 6 / 22; b_blend(B, tx + 7, ty + 5, c, k); b_blend(B, tx + 7, ty + 9, c, k);
        }
        lv_rd_bot(B, x, y, 2, c, 8);
        break;
    }
    default:   /* BOUNCE: balls bouncing on the grid, each with a shadow on the ground */
        for (int s = 0; s < 3; ++s) {
            const float vx = 34.0f + s * 17.0f, w = 228.0f; float px = fmodf(ts * vx + s * 70.0f, 2 * w); if (px > w) px = 2 * w - px;
            const float per = 1.1f + s * 0.25f, ph = fmodf(ts / per + s * 0.31f, 1.0f);
            const int gy = 236 + s * 14, hgt = 60 + s * 25, cx = 6 + (int)px, cy = gy - (int)(hgt * 4 * ph * (1 - ph));
            const uint16_t c = lv_rd_bg_col(s, 8);
            for (int x = -4; x <= 4; ++x) b_blend(B, cx + x, gy + 5, lv_hex(0x000000), 4);   /* shadow */
            for (int a = -4; a <= 4; ++a) for (int b = -4; b <= 4; ++b) if (a * a + b * b <= 17) { const int y = cy + a; if (y >= LV_RD_Y0 && y <= LV_RD_Y1) b_blend(B, cx + b, y, a * a + b * b <= 4 && a < 0 ? lv_hex(0xffffff) : c, 7); }
        }
        break;
    }
}
#else
static uint16_t lv_rd_rain_col(int k)
{
    const uint32_t c = lv_raincol == 0 ? 0x20ff60 : lv_raincol == 1 ? 0x30e8ff : LV_ACC[lv_theme];
    return lv_dim(lv_hex(c), k, 8);
}
#endif
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
/* hunt fix (1109, Mobandy "the scanning tower screen still goes in and out of signal"): one missed re-scan pass is normal
 * radio luck (BLE 1.5 s window vs a 1-2 s advertiser, a WiFi beacon missed on its channel), not a lost signal. The meters
 * HOLD the last smoothed reading for up to LV_RD_MISS_GRACE-1 missed passes; NO SIG only from the GRACE-th miss in a row.
 * dBm is an EMA of the real reads (alpha 1/2, quarter-dB fixed point) so +-4 dB packet noise stops flipping WARMER/COLDER.
 * Nothing here implies direction: the trend is still only the change of the (smoothed) real dBm, +-2 dB = STEADY. */
#ifndef LV_RD_MISS_GRACE
#define LV_RD_MISS_GRACE 3
#endif
static int lv_rd_sm4 = 0;   /* smoothed target dBm x4 */
static inline bool lv_rd_nosig(void) { return lv_rd_miss >= LV_RD_MISS_GRACE; }
static int lv_rd_from = LV_SC_HOME;  /* where BACK goes (home, SCANbot list, TTY) */
static int lv_rd_ret_cat = -1, lv_rd_ret_cpage = 0, lv_rd_ret_mode = 0; static bool lv_rd_ret_detail = false; static uint8_t lv_rd_ret_mac[6]; static bool lv_rd_ret_sel = false;
static const LvRect LV_RD_BTN[3] = {{106, 287, 143, 297}, {106, 304, 143, 314}, {106, 270, 143, 280}};   /* v1.2.9: I mid, W bottom, H top; pills 38x11 */
/* touch (hit-list item 1): finger targets for the three buttons, padded ~8 px inside the panel (panel y 247..285; the
 * radar field above y247 keeps its own taps) and split at the gap midpoints so they never overlap. Drawn rects unchanged. */
static const LvRect LV_RD_BTN_TAP[3] = {{102, 284, 147, 299}, {102, 300, 147, 316}, {102, 268, 147, 283}};  /* v1.2.9: I mid, W bottom, H top; 46x16/17 */
#ifndef LV_RD_INFO
#define LV_RD_INFO 1   /* MOCK 1111 (Mobandy): IGNORE / WATCH / HUNT / INFO, a taller row, INFO = Joey file-drop node view. 0 = as locked */
#endif
#ifndef LV_RD_INFO_UI
#define LV_RD_INFO_UI 1   /* fw-3locks-1116: 0 = INFO code compiled (the HACKbot game draws with it) but the radar keeps its locked 3-button row */
#endif
#ifndef LV_RD_INFO_TOWER_PULL
#define LV_RD_INFO_TOWER_PULL 1   /* v1.2.16 locked (726a5d3): pull-up becomes the original INFO monolith (lv_rd_info_tower). */
#endif
#if LV_RD_INFO && LV_RD_INFO_UI
/* v3 (Mobandy 9:33 "why change the other ui"): on the radar ONLY the locked button row changes vs 049ff599 + HOLD. Name + dBm
 * (y264), type / MAC (y275), panel box, 3-line mini log, BACKbot, header, banner stay where they are. Widths: IGNORE 50, WATCH 50,
 * HUNT 78 (was 72), INFO 43, 4 px gaps, x5..234 as before. Height per LV_RD_ROW:
 *   0 = FIT   : drawn y249..262 (14 px, was 11) = the panel's own padding (y248..249 above, y261..263 below were empty);
 *               finger targets = the whole panel height y247..285 (that strip was already a no-op "radar-panel" zone)
 *   1 = GROW  : the panel top moves up 14 px (y233); drawn y235..262 (28 px); costs 14 rows of 3D view while locked
 *   2 = FLOAT : panel box unchanged; drawn y235..262 (28 px) float over the 3D view rows 235..246 and the panel's top frame */
#ifndef LV_RD_ROW
#define LV_RD_ROW 0
#endif
#define LV_RD_PTOP (LV_RD_ROW == 1 ? 233 : 247)          /* locked panel top frame */
#define LV_RD_BY0 (LV_RD_ROW == 0 ? 249 : 235)           /* drawn button top */
#define LV_RD_TY0 (LV_RD_ROW == 0 ? 247 : 233)           /* finger target top */
static const LvRect LV_RD_BTN4[4] = {{5, LV_RD_BY0, 54, 262}, {58, LV_RD_BY0, 107, 262}, {111, LV_RD_BY0, 188, 262}, {192, LV_RD_BY0, 234, 262}};
static const LvRect LV_RD_BTN4_TAP[4] = {{0, LV_RD_TY0, 56, 285}, {57, LV_RD_TY0, 109, 285}, {110, LV_RD_TY0, 190, 285}, {191, LV_RD_TY0, 239, 285}};
static bool lv_rd_info = false; static uint32_t lv_rd_info_t = 0; static LvFind lv_rd_info_f;
#define LV_RD_NBTN 4
#define LV_RD_BTNR LV_RD_BTN4
#define LV_RD_BTNT LV_RD_BTN4_TAP
#else
#if LV_RD_INFO
static bool lv_rd_info = false; static uint32_t lv_rd_info_t = 0; static LvFind lv_rd_info_f;   /* fw-3locks-1116: never opened from the radar (no INFO button) */
#else
static const bool lv_rd_info = false;
#endif
#define LV_RD_ROW 0
#define LV_RD_PTOP 247
#define LV_RD_NBTN 3
#define LV_RD_BTNR LV_RD_BTN
#define LV_RD_BTNT LV_RD_BTN_TAP
#endif
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

#ifndef LV_RD_CHARGE
#define LV_RD_CHARGE 1   /* LOCKED ui_gibson_radar_charge D4 (lock store, src 4a8197a); 0 = the locked radar 3240b03 hold-1s lock */
#define LV_RD_CG_NODRAW 1  /* v1.2.8: no power current to the tower; A on the footer pad locks */
#endif
#ifndef LV_RD_CG_NODRAW
#define LV_RD_CG_NODRAW 0
#endif
#if LV_RD_CHARGE
/* CHARGE LOCK (port of LOCKED ui_gibson_radar_charge D4, docs/mocks/radar-charge/D4, generated by the same apply_charge.py +
 * apply_charge_D4.py patches specialised to D4): select a tower (tap it, or tap empty space to cycle; wraps, never auto-jumps),
 * then HOLD the HOLD TO LOCK bar (design A); it fills over 1 s while a current runs from the lower control panel along the
 * radar's own grid (a grid row, up a floor line, along the tower's base row) to the tower; at full charge the lock fires, the
 * tower lights in its own colour, IGNORE / WATCH / HUNT as before, and the current keeps flowing while the tower is locked /
 * hunted. Let go early: the charge drains (300 ms). All state is static (no heap); animation is timed from lv_now. */
#define LV_RD_CG_MS 1000
#define LV_RD_CG_YMAX (lv_rd_lock ? LV_RD_PTOP - 2 : LV_RD_Y1)   /* once locked the panel grows to y247: the current stays above it */
static bool lv_rd_cg = false; static uint32_t lv_rd_cg_t = 0, lv_rd_cg_rel_t = 0; static float lv_rd_cg_rel = 0;
static const LvRect LV_RD_CG_BTN = {166, 250, 234, 260}, LV_RD_CG_HIT = {160, 244, 239, 261};
/* item 1 (Mobandy 2026-11-06 "hold to lock is still hard ... without pressing the screen and changing the tower"):
 * a generous arming zone around the HOLD TO LOCK bar (full bar width + pad above and below to y271, clear of BACKbot
 * {221,301,239,319} and the footer terminal at y287+), a touch-owner latch + 350 ms post-release guard so an aborted
 * hold / finger drift / release never cycles or re-picks a tower, and XPT2046 dropout tolerance (<=140 ms) while charging. */
static const LvRect LV_RD_CG_ARM = {144, 234, 239, 271};
static bool lv_rd_cg_owned = false, lv_rd_cg_dropping = false; static uint32_t lv_rd_cg_guard_until = 0, lv_rd_cg_drop_t = 0;
static inline bool lv_rd_in_pad(const LvRect &r, int x, int y, int pad) { return x >= r.x0 - pad && x <= r.x1 + pad && y >= r.y0 - pad && y <= r.y1 + pad; }
static float lv_rd_cg_level(void)
{
    if (lv_rd_lock) return 1.0f;
    if (lv_rd_cg) { const float u = (lv_now - lv_rd_cg_t) / (float)LV_RD_CG_MS; return u > 1 ? 1 : u < 0 ? 0 : u; }
    const float u = lv_rd_cg_rel - (lv_now - lv_rd_cg_rel_t) / 300.0f; return u > 0 ? u : 0;
}
static bool lv_rd_cg_show(void) { return !lv_rd_lock && lv_rd_sel >= 0 && lv_rd_sel < lv_nf; }
static uint32_t lv_rd_ch(uint32_t a, uint32_t b) { uint32_t h = a * 2654435761u ^ (b + 0x9e3779b9u) * 2246822519u; h ^= h >> 15; h *= 2654435761u; h ^= h >> 13; return h; }
/* ---- route along the grid: S -> along a grid row to floor line g -> up floor line g -> along the tower's base row -> tower ---- */
static int lv_rd_fx_x(int g, int y) { return (int)lv_lroundf(120 + (g * 0.5f - lv_rd_camx) * (y - LV_RD_HOR) / LV_RD_CAMH); }
#define LV_RD_CG_NP 900
/* occlusion hull of one nearer tower (see lv_rd_hulls); integer corners: every value is an int screen coordinate */
struct LvRdHull { int16_t n, xa, xb, ya, yb, gyb, ns; int16_t x[8], y[8], s[9][4]; };   /* s: the tower's drawn edge segments */
/* route + hull scratch: ONE heap block allocated on first use and kept (no per-frame heap; dram0 has ~7.5 KB left, see
 * fw-port-1103). If the allocation fails the hold still charges and locks; only the current is not drawn. */
struct LvRdCgBuf { int16_t px[LV_RD_CG_NP], py[LV_RD_CG_NP]; LvRdHull hull[LV_MAXF]; };
static LvRdCgBuf *lv_rd_cgb = nullptr;
static int16_t *lv_rd_px = nullptr, *lv_rd_py = nullptr; static int lv_rd_np = 0;
static int16_t lv_rd_vx[8], lv_rd_vy[8]; static int lv_rd_nv = 0;   /* corners */
static void lv_rd_cg_add(int x, int y) { if (lv_rd_np < LV_RD_CG_NP && (!lv_rd_np || lv_rd_px[lv_rd_np - 1] != x || lv_rd_py[lv_rd_np - 1] != y)) { lv_rd_px[lv_rd_np] = x; lv_rd_py[lv_rd_np] = y; ++lv_rd_np; } }
static void lv_rd_cg_hseg(int xa, int xb, int y) { const int d = xb >= xa ? 1 : -1; for (int x = xa; x != xb + d; x += d) lv_rd_cg_add(x, y); }
static bool lv_rd_route(int sx, int sy, int dof, int &hdir1, int &hdir2)   /* dof: bus lane offset (0 = single trace) */
{
    lv_rd_np = 0; lv_rd_nv = 0;
    if (!lv_rd_cgb) { lv_rd_cgb = (LvRdCgBuf *)malloc(sizeof(LvRdCgBuf)); if (!lv_rd_cgb) return false; lv_rd_px = lv_rd_cgb->px; lv_rd_py = lv_rd_cgb->py; }
    const LvTower *T = lv_rd_tower_of(lv_f[lv_rd_sel].mac); if (!T) return false;
    const int yb = T->yb < LV_RD_Y1 ? T->yb : LV_RD_Y1;
    int bg = 99, bc = 1 << 30;
    for (int g = -12; g <= 12; ++g) {
        const int xa = lv_rd_fx_x(g, sy), xb = lv_rd_fx_x(g, yb - 6);
        if (xa < 4 || xa > 235 || (xb >= T->x0 - 5 && xb <= T->x1 + 5)) continue;
        const int edge = xb < T->x0 ? T->x0 : T->x1, c = abs(sx - xa) + 2 * abs(xb - edge);
        if (c < bc) { bc = c; bg = g; }
    }
    if (bg == 99) return false;
    const int xa = lv_rd_fx_x(bg, sy), xb = lv_rd_fx_x(bg, yb - 6);
    hdir1 = xa >= sx ? 1 : -1; hdir2 = xb < T->x0 ? 1 : -1;
    const int a = dof < 0 ? -dof : dof;          /* lane: y-offset on the first row, x-offset on the floor line, inner lanes turn first */
    const int ox = hdir1 * dof; (void)a;   /* upper lane (dof<0) is the inner one at the first corner: it turns first */
    const int y1 = sy + dof;
    const int yend = yb - 6 + ox * hdir2;   /* the lane on the tower side turns first at the top corner */
    lv_rd_cg_hseg(sx, lv_rd_fx_x(bg, y1) + ox, y1);
    lv_rd_vx[lv_rd_nv] = lv_rd_fx_x(bg, y1) + ox; lv_rd_vy[lv_rd_nv++] = y1;
    for (int y = y1 - 1; y >= yend; --y) {   /* along the floor line, filling x gaps so the trace stays connected */
        const int x = lv_rd_fx_x(bg, y) + ox, xp = lv_rd_px[lv_rd_np - 1];
        if (abs(x - xp) > 1) { const int d = x > xp ? 1 : -1; for (int xx = xp + d; xx != x; xx += d) lv_rd_cg_add(xx, y + 1); }
        lv_rd_cg_add(x, y);
    }
    const int xc = lv_rd_fx_x(bg, yend) + ox; lv_rd_vx[lv_rd_nv] = xc; lv_rd_vy[lv_rd_nv++] = yend;
    const int xt = hdir2 > 0 ? T->x0 - 1 : T->x1 + 1;
    lv_rd_cg_hseg(xc, xt, yend);
    lv_rd_vx[lv_rd_nv] = xt; lv_rd_vy[lv_rd_nv++] = yend;
    return true;
}

/* D2 fix 1: occlusion. Every other tower's silhouette = convex hull of its 8 box corners, rebuilt per frame. A tower hides a stream
 * pixel when the pixel is inside its hull and the tower's front base (yb) is at or below the stream element's ground row (= nearer). */
static int lv_rd_nh = 0; static int lv_rd_gy = 0; static int lv_rd_glass = 1;   /* 0: element hidden entirely behind a nearer tower (no see-through) */
static float lv_rd_cross(float ox, float oy, float ax, float ay, float bx, float by) { return (ax - ox) * (by - oy) - (ay - oy) * (bx - ox); }
static void lv_rd_hulls(int target)
{
    lv_rd_nh = 0;
    for (int t = 0; t < lv_rd_n && lv_rd_nh < LV_MAXF; ++t) {
        const LvTower &T = lv_rd_tw[t]; if (T.i == target) continue;
        float px[8] = {(float)T.x0, (float)T.x1, (float)T.x0, (float)T.x1, (float)T.bx0, (float)T.bx1, (float)T.bx0, (float)T.bx1};
        float py[8] = {(float)T.yt, (float)T.yt, (float)T.yb, (float)T.yb, (float)T.byt, (float)T.byt, (float)T.byb, (float)T.byb};
        int o[8]; for (int k = 0; k < 8; ++k) o[k] = k;
        for (int a = 1; a < 8; ++a) { const int v = o[a]; int b = a - 1; while (b >= 0 && (px[o[b]] > px[v] || (px[o[b]] == px[v] && py[o[b]] > py[v]))) { o[b + 1] = o[b]; --b; } o[b + 1] = v; }
        int h[17], m = 0;
        for (int k = 0; k < 8; ++k) { while (m >= 2 && lv_rd_cross(px[h[m - 2]], py[h[m - 2]], px[h[m - 1]], py[h[m - 1]], px[o[k]], py[o[k]]) <= 0) --m; h[m++] = o[k]; }
        for (int k = 6, lo = m + 1; k >= 0; --k) { while (m >= lo && lv_rd_cross(px[h[m - 2]], py[h[m - 2]], px[h[m - 1]], py[h[m - 1]], px[o[k]], py[o[k]]) <= 0) --m; h[m++] = o[k]; }
        --m; if (m < 3) continue;
        LvRdHull &H = lv_rd_cgb->hull[lv_rd_nh++]; H.n = m; H.xa = 9999; H.xb = -9999; H.ya = 9999; H.yb = -9999; H.gyb = T.yb;
        for (int k = 0; k < m; ++k) { H.x[k] = (int16_t)px[h[k]]; H.y[k] = (int16_t)py[h[k]];
            const int xi = (int)H.x[k], yi = (int)H.y[k]; if (xi < H.xa) H.xa = xi; if (xi > H.xb) H.xb = xi; if (yi < H.ya) H.ya = yi; if (yi > H.yb) H.yb = yi; }
        const float S[10][4] = { {(float)T.x0, (float)T.yt, (float)T.x0, (float)T.yb}, {(float)T.x1, (float)T.yt, (float)T.x1, (float)T.yb},
            {(float)T.x0, (float)T.yt, (float)T.x1, (float)T.yt}, {(float)T.x0, (float)T.yb, (float)T.x1, (float)T.yb},
            {(float)T.bx0, (float)T.byt, (float)T.bx1, (float)T.byt}, {(float)T.bx0, (float)T.byt, (float)T.x0, (float)T.yt},
            {(float)T.bx1, (float)T.byt, (float)T.x1, (float)T.yt}, {(float)T.bx1, (float)T.byt, (float)T.bx1, (float)T.byb},
            {(float)T.bx0, (float)T.byt, (float)T.bx0, (float)T.byb}, {(float)T.bx0, (float)T.byt, (float)T.bx0, (float)T.byt} };
        H.ns = 0; for (int k = 0; k < 10; ++k) { if (k == 7 && !(T.bx1 > T.x1)) continue; if (k == 8 && !(T.bx0 < T.x0)) continue; if (k == 9) continue;
            for (int c = 0; c < 4; ++c) H.s[H.ns][c] = (int16_t)S[k][c];
            ++H.ns; }
    }
}
static float lv_rd_segd2(const int16_t *gi, float x, float y)
{
    const float g[4] = {(float)gi[0], (float)gi[1], (float)gi[2], (float)gi[3]};
    const float ex = g[2] - g[0], ey = g[3] - g[1], l2 = ex * ex + ey * ey; float u = l2 > 0 ? ((x - g[0]) * ex + (y - g[1]) * ey) / l2 : 0;
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    const float dx = g[0] + u * ex - x, dy = g[1] + u * ey - y; return dx * dx + dy * dy;
}
/* 0 = free; 1 = behind a nearer tower's translucent face (drawn dim, as if through its glass); 2 = on that tower's edge line (skip) */
static int lv_rd_occ(int x, int y)
{
    int r = 0;
    for (int q = 0; q < lv_rd_nh; ++q) {
        const LvRdHull &H = lv_rd_cgb->hull[q];
        if (H.gyb <= lv_rd_gy || x < H.xa - 2 || x > H.xb + 2 || y < H.ya - 2 || y > H.yb + 2) continue;
        bool in = true;
        for (int k = 0; k < H.n && in; ++k) { const int k2 = k + 1 == H.n ? 0 : k + 1; const float ex = H.x[k2] - H.x[k], ey = H.y[k2] - H.y[k];
            if (lv_rd_cross(H.x[k], H.y[k], H.x[k2], H.y[k2], (float)x, (float)y) < -0.5f * sqrtf(ex * ex + ey * ey)) in = false; }
        if (!in) continue;
        for (int k = 0; k < H.ns; ++k) if (lv_rd_segd2(H.s[k], (float)x, (float)y) <= 0.81f) return 2;
        r = 1;
    }
    return r;
}
#define LV_RD_CAP 176   /* D4: max luma (0..255) the current lights a pixel to */
static inline int lv_rd_l565(uint16_t c) { const int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3; return (54 * r + 183 * g + 19 * b) >> 8; }
#define LV_RD_GLASS 13  /* /16: what of the stream shows through a nearer tower's face */
static inline void lv_rd_bly(LvBand &B, int x, int y, uint16_t c, int k, int n, int ymax)   /* fine blend, clipped, occluded */
{
    if (k <= 0 || y < LV_RD_Y0 || y > ymax) return;
    if (k > n) k = n;
    if (x < B.x0 || x >= B.x0 + B.w || y < B.y0 || y >= B.y0 + B.h) return;
    const int o = lv_rd_occ(x, y); if (o == 2) return; if (o == 1) { if (!lv_rd_glass) return; k = k * LV_RD_GLASS * 2 / n; n = 32; if (k <= 0) return; }
    uint16_t &q = B.px[(y - B.y0) * B.w + (x - B.x0)]; const uint16_t m = lv_mix(q, c, k, n);
    /* D4: brightness ceiling - the current may light a pixel up to LV_RD_CAP luma, never past it (stacked blends included), so its
     * peak stays clearly below the top banner's yellow text (~203); a pixel already brighter than the cap is left alone */
    const int lq = lv_rd_l565(q), lm = lv_rd_l565(m);
    if (lm <= LV_RD_CAP || lm <= lq) { q = m; return; }
    if (lq >= LV_RD_CAP) return;
    q = lv_mix(q, m, LV_RD_CAP - lq, lm - lq);
}
static inline void lv_rd_bl(LvBand &B, int x, int y, uint16_t c, int k, int n) { lv_rd_bly(B, x, y, c, k, n, LV_RD_CG_YMAX); }
static bool lv_rd_nearp(int x, int y, int j)   /* within 7 px (Chebyshev) of the route near index j: keeps every arc pixel in the mask */
{
    for (int m = j - 16; m <= j + 16; ++m) { if (m < 0 || m >= lv_rd_np) continue;
        const int ex = x > lv_rd_px[m] ? x - lv_rd_px[m] : lv_rd_px[m] - x, ey = y > lv_rd_py[m] ? y - lv_rd_py[m] : lv_rd_py[m] - y;
        if (ex <= 7 && ey <= 7) return true; }
    return false;
}
/* D4: one crackle = a SMALL stepped (axis-aligned) arc: out 2-3 px, along 2-3, a 1 px kink, along 1-2. No forks, no tail.
 * Soft ice colour (never white), <= 5 px off the route: a calm discharge on the wire, never a bolt across the grid. */
static void lv_rd_arc(LvBand &B, int j, uint32_t r, int ymax, int kmax)
{
    const uint16_t ICE2 = lv_hex(0xa8dcff), ICE3 = lv_hex(0x8ccaff), ELEC = lv_hex(0x3aa8ff);
    const int n = lv_rd_np; if (j < 0 || j >= n) return;
    const int ja = j - 3 < 0 ? 0 : j - 3, jb = j + 3 >= n ? n - 1 : j + 3;
    const int tx = lv_rd_px[jb] - lv_rd_px[ja], ty = lv_rd_py[jb] - lv_rd_py[ja];
    const bool horiz = (tx < 0 ? -tx : tx) >= (ty < 0 ? -ty : ty);
    const int ux = horiz ? 1 : 0, uy = horiz ? 0 : 1, nx = horiz ? 0 : 1, ny = horiz ? 1 : 0;
    const int side = (r & 1) ? 1 : -1; int dir = (r & 2) ? 1 : -1;
    lv_rd_gy = lv_rd_py[j];
    int x = lv_rd_px[j] + (side > 0 ? nx : 0), y = lv_rd_py[j] + (side > 0 ? ny : 0), t = 0;   /* start at the core's outer pixel */
    auto put = [&](int px, int py, int kk) {
        if (!lv_rd_nearp(px, py, j)) return;
        lv_rd_bly(B, px, py, t < 3 ? ICE2 : ICE3, kk, 16, ymax);
        for (int d = 0; d < 4; ++d) { const int gx = px + (d == 0) - (d == 1), gy = py + (d == 2) - (d == 3); if (lv_rd_nearp(gx, gy, j)) lv_rd_bly(B, gx, gy, ELEC, 3, 16, ymax); }
    };
    auto run = [&](int ax, int ay, int len) { for (int s = 0; s < len; ++s) { x += ax; y += ay; ++t; const int k = kmax - t / 2; put(x, y, k < 6 ? 6 : k); } };
    run(nx * side, ny * side, 2 + (int)((r >> 4) % 2));        /* out from the wire (2-3) */
    run(ux * dir, uy * dir, 2 + (int)((r >> 5) % 2));          /* along it (2-3) */
    run(-nx * side, -ny * side, 1);                            /* 1 px kink back toward the wire */
    if ((r >> 9) % 4 == 0) dir = -dir;
    run(ux * dir, uy * dir, 1 + (int)((r >> 10) % 2));         /* along again (1-2) */
}
static inline void lv_rd_across(int i, int &ax, int &ay)   /* unit step across the wire at route index i (+ = down / right) */
{
    const int n = lv_rd_np, ja = i - 2 < 0 ? 0 : i - 2, jb = i + 2 >= n ? n - 1 : i + 2, tx = lv_rd_px[jb] - lv_rd_px[ja], ty = lv_rd_py[jb] - lv_rd_py[ja];
    const bool hz = (tx < 0 ? -tx : tx) >= (ty < 0 ? -ty : ty); ax = hz ? 0 : 1; ay = hz ? 1 : 0;
}
static inline int lv_rd_breath(int period, int amp)   /* slow smooth triangle breathing, -amp..+amp (no randomness, no flicker) */
{
    const int ph = (int)(lv_now % (uint32_t)period), h = period / 2; const int v = ph < h ? ph : period - ph;   /* 0..h */
    return (v * 2 * amp + h / 2) / h - amp;
}
static void lv_rd_dstream(LvBand &B, float lvl)
{
    /* D4 palette: no pure white anywhere - the brightest current pixel stays below the top banner's yellow text (luma ~203) */
    const uint16_t PK = lv_hex(0xc4eaff), ICE2 = lv_hex(0xa8dcff), ELEC = lv_hex(0x3aa8ff), VIO = lv_hex(0x7a1cff);
    const int n = lv_rd_np; if (n < 2) return;
    const int front = lv_rd_lock ? n - 1 : (int)lv_lroundf(lvl * (n - 1));
    lv_rd_hulls(lv_rd_sel);
    const uint32_t f4 = lv_now / 120;                      /* D4: crackle / sparks re-roll every 4 sim frames, not every frame */
    const int br = lv_rd_breath(2400, 2);                  /* gentle 2.4 s breathing, +-2/32 */
    /* 1. the conduit band: 2 px ice core (route px + the next px across) in a 3-step blue glow band either side. Steady, no flicker.
     * Where the route px sits on a nearer tower's edge line (skipped there), the core moves to the other side so it stays 2 px. */
    for (int i = 0; i < n; ++i) {
        const int x = lv_rd_px[i], y = lv_rd_py[i]; lv_rd_gy = y;
        if (i > front) { if (i & 1) lv_rd_bl(B, x, y, VIO, 3, 8); continue; }
        int ax, ay; lv_rd_across(i, ax, ay);
        const bool onedge = lv_rd_occ(x, y) == 2;
        const int kc = 19 + br;                            /* /32 */
        lv_rd_bly(B, x, y, ICE2, kc, 32, LV_RD_CG_YMAX); lv_rd_bly(B, x + ax, y + ay, ICE2, kc, 32, LV_RD_CG_YMAX);
        if (onedge) lv_rd_bly(B, x - ax, y - ay, ICE2, kc, 32, LV_RD_CG_YMAX);
        const int lo = onedge ? -2 : -1;
        lv_rd_bl(B, x + lo * ax, y + lo * ay, ELEC, 9, 16);          lv_rd_bl(B, x + 2 * ax, y + 2 * ay, ELEC, 9, 16);
        lv_rd_bl(B, x + (lo - 1) * ax, y + (lo - 1) * ay, ELEC, 5, 16); lv_rd_bl(B, x + 3 * ax, y + 3 * ay, ELEC, 5, 16);
        lv_rd_bl(B, x + (lo - 2) * ax, y + (lo - 2) * ay, ELEC, 2, 16); lv_rd_bl(B, x + 4 * ax, y + 4 * ay, ELEC, 2, 16);
    }
    /* 2. energy packets: smooth, evenly spaced (every 40 px), travelling at a steady 0.2 px/ms. 6 px bright core 2 px wide in a 4 px
     * glow each side + a 10 px fading tail. Fixed brightness (no per-frame boost, no random sparks). Hidden behind nearer towers. */
    { const int SP = 40, ph = (int)(lv_now / 5 % SP);
      lv_rd_glass = 0;
      for (int i = ph; i < n; i += SP) {
          if (i > front) break;
          for (int t = 0; t < 16; ++t) {
              const int j = i - t; if (j < 0) break;
              const int x = lv_rd_px[j], y = lv_rd_py[j]; lv_rd_gy = y; int ax, ay; lv_rd_across(j, ax, ay);
              const int e = lv_rd_occ(x, y) == 2 ? -1 : 0;   /* same edge-line shift as the core */
              if (t < 6) {
                  const int kc = t == 0 ? 9 : 11;
                  lv_rd_bl(B, x + e * ax, y + e * ay, PK, kc, 16); lv_rd_bl(B, x + (e + 1) * ax, y + (e + 1) * ay, PK, kc, 16);
                  lv_rd_bl(B, x + (e - 1) * ax, y + (e - 1) * ay, ICE2, 7, 16); lv_rd_bl(B, x + (e + 2) * ax, y + (e + 2) * ay, ICE2, 7, 16);
                  lv_rd_bl(B, x + (e - 2) * ax, y + (e - 2) * ay, ELEC, 8, 16); lv_rd_bl(B, x + (e + 3) * ax, y + (e + 3) * ay, ELEC, 8, 16);
                  lv_rd_bl(B, x + (e - 3) * ax, y + (e - 3) * ay, ELEC, 4, 16); lv_rd_bl(B, x + (e + 4) * ax, y + (e + 4) * ay, ELEC, 4, 16);
              } else {
                  const int k = 16 - t;          /* 10 .. 1 */
                  lv_rd_bl(B, x + e * ax, y + e * ay, ICE2, k * 3 / 4 + 1, 16); lv_rd_bl(B, x + (e + 1) * ax, y + (e + 1) * ay, ICE2, k * 3 / 4 + 1, 16);
                  lv_rd_bl(B, x + (e - 1) * ax, y + (e - 1) * ay, ELEC, k * 2 / 3, 16); lv_rd_bl(B, x + (e + 2) * ax, y + (e + 2) * ay, ELEC, k * 2 / 3, 16);
              }
          }
      }
      lv_rd_glass = 1; }
    /* 3. crackle: about half of D3 (1 + front/120 arcs), small, no forks, re-rolled every 4 frames */
    if (front >= 8) {
        const int na = 1 + front / 120;
        for (int a = 0; a < na; ++a) {
            const uint32_t r = lv_rd_ch(f4 * 7 + 3, a * 977 + 11);
            const int j = 3 + (int)(r % (uint32_t)(front - 5 > 1 ? front - 5 : 1));
            if (j < n) { lv_rd_glass = 0; lv_rd_arc(B, j, lv_rd_ch(f4 * 13 + 7, a * 7919 + j), LV_RD_CG_YMAX, 11); lv_rd_glass = 1; }
        }
    }
    /* 4. the charge front (while charging): a steady bright 2x2 tip in a soft glow; one small spark ahead, re-rolled every 4 frames */
    if (!lv_rd_lock && front > 0 && front < n - 1) {
        const int x = lv_rd_px[front], y = lv_rd_py[front]; lv_rd_gy = y; int ax, ay; lv_rd_across(front, ax, ay);
        for (int dy = -2; dy <= 3; ++dy) for (int dx = -2; dx <= 3; ++dx) {
            const bool core = dx >= 0 && dx <= 1 && dy >= 0 && dy <= 1; const int ex = dx < 0 ? -dx : dx > 1 ? dx - 1 : 0, ey = dy < 0 ? -dy : dy > 1 ? dy - 1 : 0;
            if (core) lv_rd_bl(B, x + dx, y + dy, PK, 11, 16); else if (ex + ey <= 2) lv_rd_bl(B, x + dx, y + dy, ex + ey == 1 ? ICE2 : ELEC, ex + ey == 1 ? 9 : 5, 16); }
        const uint32_t r = lv_rd_ch(f4, 4001); const int jj = front + 2 + (int)(r % 3);
        if (jj < n) lv_rd_bl(B, lv_rd_px[jj] + ((r >> 8) & 1), lv_rd_py[jj] + ((r >> 9) & 1), ICE2, 10, 16);
    }
    /* 5. locked: the current lands on the tower - a steady soft contact glow at the base-row end, breathing with the wire */
    if (lv_rd_lock) {
        const int x = lv_rd_px[n - 1], y = lv_rd_py[n - 1]; lv_rd_gy = y;
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) { const int d2 = dx * dx + dy * dy; if (d2 > 5) continue;
            lv_rd_bl(B, x + dx, y + dy, d2 <= 1 ? PK : ELEC, d2 <= 1 ? 12 + br / 2 : 7 + br / 2, 16); }
    }
    /* 6. locked: a SOFT contact port on the panel's top frame (y247) where the wire leaves the panel - a pale pad, a gentle breathing
     * blue glow above it, no crackles. Only the frame pixels next to the wire change; nothing below y248. */
    if (lv_rd_lock) {
        int pi = -1; for (int i = 0; i < n; ++i) if (lv_rd_py[i] == LV_RD_PTOP) { pi = i; break; }
        if (pi >= 0) {
            const int x = lv_rd_px[pi]; const int pulse = 6 + br;          /* 4..8, slow */
            lv_rd_gy = 0;
            for (int dy = -5; dy <= -1; ++dy) for (int dx = -4; dx <= 5; ++dx) { const int cx = dx > 0 ? dx - 1 : dx, d2 = cx * cx + dy * dy; if (d2 > 20) continue;
                lv_rd_bly(B, x + dx, LV_RD_PTOP + dy, ELEC, (20 - d2) * pulse / 30 + 1, 16, LV_RD_PTOP - 1); }
            lv_rd_gy = 1 << 30;
            for (int dx = -3; dx <= 4; ++dx) { const int a = dx <= 0 ? -dx : dx - 1;
                lv_rd_bly(B, x + dx, LV_RD_PTOP, a == 0 ? PK : a == 1 ? ICE2 : ELEC, a == 0 ? 12 : a == 1 ? 10 : 9 - a * 2, 16, LV_RD_PTOP + 1); }
        }
    }
}
static void lv_rd_cg_log(const char *tag, int x0, int y0, int x1, int y1)
{
#ifdef LV_RD_CG_LOG
    fprintf(stderr, "CG t=%u %s %d %d %d %d\n", (unsigned)lv_now, tag, x0, y0, x1, y1);
#else
    (void)tag; (void)x0; (void)y0; (void)x1; (void)y1;
#endif
}
static void lv_rd_cg_logpath(int lane)
{
#ifdef LV_RD_CG_LOG
    fprintf(stderr, "CGP t=%u lane=%d n=%d", (unsigned)lv_now, lane, lv_rd_np);
    for (int i = 0; i < lv_rd_np; ++i) fprintf(stderr, " %d,%d", lv_rd_px[i], lv_rd_py[i]);
    fprintf(stderr, "\n");
#else
    (void)lane;
#endif
}
/* the lit tower: its own colour, full strength (never white, never hue-shifted) */
static void lv_rd_cg_lit(LvBand &B)
{
    if (!lv_rd_lock) return;
    const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (!T) return;
    const LvFind &f = lv_f[T->i]; const uint16_t C = lv_rd_col(f, 8, 8);
    const int pop = (int)(lv_now - lv_rd_lock_t), kf = pop < 150 ? 4 - pop * 2 / 150 : 2;
    for (int y = T->byt; y <= T->yt; ++y) { if (y < LV_RD_Y0 || y > LV_RD_CG_YMAX) continue; const float u = T->yt == T->byt ? 1.0f : (float)(y - T->byt) / (T->yt - T->byt);
        const int xa = (int)(T->bx0 + (T->x0 - T->bx0) * u), xb = (int)(T->bx1 + (T->x1 - T->bx1) * u); for (int x = xa; x <= xb; ++x) b_blend(B, x, y, C, kf); }
    for (int y = T->yt; y <= T->yb; ++y) { if (y < LV_RD_Y0 || y > LV_RD_CG_YMAX) continue; for (int x = T->x0; x <= T->x1; ++x) b_blend(B, x, y, C, kf); }
    b_line(B, T->x0, T->yt, T->x0, T->yb, C); b_line(B, T->x1, T->yt, T->x1, T->yb, C);
    b_line(B, T->x0, T->yt, T->x1, T->yt, C); b_line(B, T->x0, T->yb, T->x1, T->yb, C);
    b_line(B, T->bx0, T->byt, T->bx1, T->byt, C); b_line(B, T->bx0, T->byt, T->x0, T->yt, C); b_line(B, T->bx1, T->byt, T->x1, T->yt, C);
    if (T->bx1 > T->x1) b_line(B, T->bx1, T->byt, T->bx1, T->byb, C);
    if (T->bx0 < T->x0) b_line(B, T->bx0, T->byt, T->bx0, T->byb, C);
    int xa, xb, ya, yb; const int k1 = pop < 150 ? 7 : 4;
    lv_rd_box(*T, 1, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, C, k1);
    lv_rd_box(*T, 2, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, C, k1 / 2);
}
/* the charge button + the current; drawn after the panel and terminal */
static void lv_rd_cg_draw(LvBand &B)
{
    const uint16_t MAG = lv_hex(0xff2ad4), VIO = lv_hex(0x7a1cff), W = lv_hex(0xffffff), HOT = lv_hex(0xffd8f6);
    const float lvl = lv_rd_cg_level();
    if (!lv_rd_lock && !lv_rd_cg_show()) return;   /* locked / hunting: the current keeps flowing */
    const bool live = lv_rd_cg || lvl > 0 || lv_rd_lock;
    /* ---- the current ---- */
    if (live && (lv_rd_lock || lv_rd_sel >= 0)) {
        const int saved = lv_rd_sel; if (lv_rd_lock) lv_rd_sel = lv_rd_find(lv_rd_lockmac);
        int h1, h2;
        if (lv_rd_route(LV_RD_CG_BTN.x0 - 1, 255, 0, h1, h2)) { lv_rd_cg_logpath(0); lv_rd_dstream(B, lvl); }
        lv_rd_sel = saved;
    }
    if (lv_rd_lock) return;   /* the button gives way to IGNORE / WATCH / HUNT exactly as today */
    /* ---- the charge button ---- */
    const LvRect &r = LV_RD_CG_BTN; const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
    char pct[12]; snprintf(pct, sizeof pct, "%d%%", (int)lv_lroundf(lvl * 100));
    lv_rd_cg_log("BTN", r.x0, r.y0, r.x1, r.y1);
    b_fill(B, r.x0, r.y0, w, h, lv_hex(0x000000)); b_frame(B, r.x0, r.y0, w, h, MAG);
    b_fill(B, r.x1 + 1, r.y0 + 3, 2, h - 6, MAG);                                        /* battery nub */
    const int cells = 10, cw = (w - 4) / cells, lit = (int)(lvl * cells + 0.001f);
    for (int c = 0; c < cells; ++c) { const int x = r.x0 + 2 + c * cw; b_fill(B, x, r.y0 + 2, cw - 1, h - 4, c < lit ? lv_mix(MAG, HOT, c * 6 / cells, 8) : lv_dim(VIO, 2, 8)); }
    const char *lab = lv_rd_cg ? pct : "HOLD TO LOCK";
    const int lw = lv_w35(lab), lx = r.x0 + (w - lw) / 2;
    b_fill(B, lx - 2, r.y0 + 2, lw + 3, 7, lv_hex(0x000000));
    b_text35(B, lx, r.y0 + 3, lab, lv_rd_cg ? W : MAG);
}
/* v1.2.12 (Mobandy 2026-11-20 10:44 AM ET, pt1477u: "get ride of hunt meter on scan tower if we can see it in the hunt folder"):
 * the GIBSON scan screen no longer draws a hunt meter; the live hunt level is read in the HUNT folder (lv_rd_fold level bar).
 * Code kept (never deleted), off by flag: LV_RD_HM_TOWER = the LOCKED A/V2 bands in the hunted tower's glass,
 * LV_RD_HM_TOPBAR = the 11-bar meter in the top hunt bar (it ran over the target name text). Set either to 1 to restore. */
#ifndef LV_RD_HM_TOWER
#define LV_RD_HM_TOWER 0
#endif
#ifndef LV_RD_HM_TOPBAR
#define LV_RD_HM_TOPBAR 0
#endif
#ifndef LV_RD_HUNT_METER
#define LV_RD_HUNT_METER 1   /* LOCKED ui_gibson_radar_hunt_meter (A, src c83a27e) + ui_gibson_radar_hunt_meter_interact (V2, src 057279f) */
#endif
#if LV_RD_HUNT_METER
/* HUNT METER while hunting (port of the LOCKED mocks, docs/mocks/radar-hunt-meter render_hunt_meter.py draw_A /
 * clear_top_meter and docs/mocks/radar-current-meter-interact render_interact.py V2): the top banner's 11-bar meter is gone;
 * 14 thermometer bands stack up the hunted tower's glass, green at the base -> yellow -> red at the top of the fill. The locked
 * D4 current drives it (V2 ARC): every D4 packet that reaches the contact throws a short stepped ice arc into one of the lowest
 * three bands and a brightness ripple climbs the lit stack (25 ms / band); 150 ms after the hit the packet lands: one more band
 * lights while the stack is below the signal (target = real dBm, the same 4..60 level as the tower height), one goes out while
 * it is above, else the top band flashes. The mocks are drawn at 3x; this draws each 1x pixel exactly as the mock paints its
 * centre sample (3x+1, 3y+1), so @1x it is the mock's own pixel grid (the locked @1x stills are those centre samples).
 * Added brightness is capped at 200 luma (below the banner's ~203). All state is static (~120 B), no heap. */
#ifndef LV_RD_HM_OCC
#define LV_RD_HM_OCC 0   /* 1 = meter behind nearer towers (13/16 through their glass, their edge lines on top). 0 = as locked: the
                            * hunted tower is drawn lit in front of every tower (lv_rd_cg_lit), so its meter is too */
#endif
#define LV_RD_HM_N 14
#define LV_RD_HM_NEV 8
struct LvRdHmEv { uint32_t hit, land; int8_t band; uint8_t isnew, landed; };
static LvRdHmEv lv_rd_hm_ev[LV_RD_HM_NEV]; static int lv_rd_hm_nev = 0, lv_rd_hm_lit = 0, lv_rd_hm_pd = 99;
static bool lv_rd_hm_on = false; static uint32_t lv_rd_hm_ft = 0;
#ifdef LV_RD_HM_SIM
static uint32_t lv_rd_hm_sim_t0 = 0;   /* sim only (sim_live 'hmsched T0'): replay the locked mock's fill schedule from T0 */
#endif
/* draw_A colours per band: color_at_height / glow_at_height / 35 % white highlight at (i + 0.5) / 14 (generated from the mock) */
static const uint32_t LV_RD_HM_COL[LV_RD_HM_N] = {0x40f858, 0x61ec49, 0x82df39, 0xa3d32a, 0xc3c61b, 0xe4ba0c, 0xffac01, 0xff9c07, 0xff8b0d, 0xff7a13, 0xff6a1a, 0xff5920, 0xff4826, 0xff382c};
static const uint32_t LV_RD_HM_GLO[LV_RD_HM_N] = {0x207c2c, 0x307624, 0x41701c, 0x516915, 0x62630d, 0x725d06, 0x805600, 0x804e03, 0x804506, 0x803d09, 0x80350d, 0x802c10, 0x802413, 0x801c16};
static const uint32_t LV_RD_HM_HI[LV_RD_HM_N] = {0x82fa92, 0x98f288, 0xadea7e, 0xc3e274, 0xd8d96a, 0xedd261, 0xffc959, 0xffbe5d, 0xffb361, 0xffa865, 0xff9e6a, 0xff936e, 0xff8871, 0xff7d75};
#define LV_RD_HM_DIM 0x3a2a34
static int lv_rd_hm_target(uint32_t t)   /* bands the signal asks for */
{
#ifdef LV_RD_HM_SIM
    if (lv_rd_hm_sim_t0) {   /* render_interact.py target_slots: 22 % -> 55 % -> 100 % */
        const double tr = (double)(int32_t)(t - lv_rd_hm_sim_t0); double f;
        if (tr < 1500) f = 0.22; else if (tr < 3500) f = 0.22 + (0.55 - 0.22) * (tr - 1500) / 2000; else if (tr < 4800) f = 0.55 + 0.45 * (tr - 3500) / 1300; else f = 1.0;
        return (int)nearbyint(f * LV_RD_HM_N);
    }
#endif
    (void)t;
    if (!lv_rd_tgt_have || lv_rd_nosig()) return 0;   /* NO SIG: the stack drains, one band per packet (a grace miss holds) */
    const int s = (lv_rd_level(lv_rd_tgt) * LV_RD_HM_N + 30) / 60;   /* -86 dBm 3 bands (22 %), -66 dBm 8 (55 %), -40 dBm 14 */
    return s < 1 ? 1 : s > LV_RD_HM_N ? LV_RD_HM_N : s;
}
static void lv_rd_hm_frame(int n)   /* once per frame: packet landings, then a new packet at the contact (D4 packet law) */
{
    const uint32_t t = lv_now;
    if (!lv_rd_hm_on) { lv_rd_hm_on = true; lv_rd_hm_lit = 0; lv_rd_hm_nev = 0; lv_rd_hm_pd = 99; }
    int k = 0;
    for (int i = 0; i < lv_rd_hm_nev; ++i) if (!(lv_rd_hm_ev[i].landed && (int32_t)(t - lv_rd_hm_ev[i].hit) > 700)) lv_rd_hm_ev[k++] = lv_rd_hm_ev[i];
    lv_rd_hm_nev = k;
    for (int i = 0; i < lv_rd_hm_nev; ++i) {
        LvRdHmEv &e = lv_rd_hm_ev[i]; if (e.landed || (int32_t)(t - e.land) < 0) continue;
        const int tg = lv_rd_hm_target(e.land);
        e.isnew = lv_rd_hm_lit < tg; if (e.isnew) ++lv_rd_hm_lit; else if (lv_rd_hm_lit > tg) --lv_rd_hm_lit;
        e.band = (int8_t)(lv_rd_hm_lit - 1); e.landed = 1;
    }
    if (n < 2) { lv_rd_hm_lit = lv_rd_hm_target(t); lv_rd_hm_pd = 99; return; }   /* no current drawn: the meter just shows the signal */
    /* D4 packets: heads at route index ph + 40 j, ph = now / 5 mod 40. A packet hits when the head nearest the contact comes
     * within 6 px of it (one 30 ms frame at 0.2 px/ms); once per packet even at faster frame rates */
    const int ph = (int)(t / 5 % 40), d = ((n - 1 - ph) % 40 + 40) % 40;
    if (d < 6 && lv_rd_hm_pd >= 6) {
        if (lv_rd_hm_nev == LV_RD_HM_NEV) { memmove(lv_rd_hm_ev, lv_rd_hm_ev + 1, sizeof lv_rd_hm_ev[0] * (LV_RD_HM_NEV - 1)); --lv_rd_hm_nev; }
        LvRdHmEv &e = lv_rd_hm_ev[lv_rd_hm_nev++]; e.hit = t; e.land = t + 150; e.band = -1; e.isnew = 0; e.landed = 0;
    }
    lv_rd_hm_pd = d;
}
static inline void lv_rd_hm_ld(float *c, uint32_t v) { c[0] = (float)(v >> 16 & 255); c[1] = (float)(v >> 8 & 255); c[2] = (float)(v & 255); }
static inline float lv_rd_hm_y(const float *c) { return 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2]; }
/* render_interact.py blend(): mix toward col by k with a luma ceiling (never brighter than max(cap, base)) */
static void lv_rd_hm_mix(float *b, uint32_t col, float k, float cap, bool &hit)
{
    if (!(k > 0)) return;
    if (k > 1) k = 1;
    float c[3], m[3]; lv_rd_hm_ld(c, col);
    for (int i = 0; i < 3; ++i) m[i] = b[i] + (c[i] - b[i]) * k;
    const float lb = lv_rd_hm_y(b), lm = lv_rd_hm_y(m);
    if (lm > cap && lm > lb) {
        const float lim = cap > lb ? cap : lb; float kk = lm - lb > 0 ? (lim - lb) / (lm - lb) : 0;
        if (kk < 0) kk = 0;
        if (kk > 1) kk = 1;
        for (int i = 0; i < 3; ++i) m[i] = b[i] + (m[i] - b[i]) * kk;
    }
    for (int i = 0; i < 3; ++i) b[i] = m[i];
    hit = true;
}
static inline bool lv_rd_hm_in(int X, int Y, int x0, int y0, int x1, int y1) { return X >= x0 && X <= x1 && Y >= y0 && Y <= y1; }
static void lv_rd_hm_draw(LvBand &B)
{
    if (!lv_rd_hunting || !lv_rd_lock) { lv_rd_hm_on = false; return; }
    const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (!T) return;
    const int n = lv_rd_np;   /* the route lv_rd_cg_draw just built for this band (0 = no current) */
    if (!lv_rd_hm_on || lv_rd_hm_ft != lv_now) { lv_rd_hm_frame(n); lv_rd_hm_ft = lv_now; }
    const uint32_t t = lv_now; const int lit = lv_rd_hm_lit;
    /* draw_A geometry (3x): the glass between the tower's outer edge columns, from under its top edge to above its base line */
    const int L = T->x0 < T->bx0 ? T->x0 : T->bx0, R = T->x1 > T->bx1 ? T->x1 : T->bx1;
    const int HX0 = 3 * (L + 1), HX1 = 3 * R, HYT = 3 * ((T->yt < T->byt ? T->yt : T->byt) + 1), HYB = 3 * (T->yb - 1);
    const double sh = (HYB - HYT) / (double)LV_RD_HM_N;
    int y1[LV_RD_HM_N], y0[LV_RD_HM_N];
    for (int i = 0; i < LV_RD_HM_N; ++i) { y1[i] = (int)(HYB - (i + 1) * sh) + 1; y0[i] = (int)(HYB - i * sh) - 1; }
#ifdef LV_RD_CG_LOG
    if (B.y0 == LV_R_RADAR.y0 || (B.y0 <= T->yb && T->yb < B.y0 + B.h)) fprintf(stderr, "HM t=%u y0=%d L=%d R=%d x0=%d x1=%d bx0=%d bx1=%d yt=%d byt=%d yb=%d lit=%d nev=%d n=%d\n", (unsigned)lv_now, B.y0, L, R, T->x0, T->x1, T->bx0, T->bx1, T->yt, T->byt, T->yb, lit, lv_rd_hm_nev, n);
#endif
    const uint32_t C = lv_rd_col(lv_f[T->i], 8, 8);   /* RGB565 tower colour (orange for WiFi, as in the mock) */
    const uint32_t C8 = (uint32_t)((((C >> 11) & 31) << 3 | ((C >> 11) & 31) >> 2) << 16 | (((C >> 5) & 63) << 2 | ((C >> 5) & 63) >> 4) << 8 | ((C & 31) << 3 | (C & 31) >> 2));
    /* V2 inputs: contact, breathing, bump, arcs */
    const bool cur = n >= 2; const int cx = cur ? lv_rd_px[n - 1] : 0, cy = cur ? lv_rd_py[n - 1] : 0;
    const float brt = 1 + (2.0f / 32) * sinf(6.2831853f * (float)(t % 2400) / 2400);
    float bump = 0; for (int i = 0; i < lv_rd_hm_nev; ++i) { const int dt = (int)(t - lv_rd_hm_ev[i].hit); if (dt >= 0 && dt < 200 && 1 - dt / 200.0f > bump) bump = 1 - dt / 200.0f; }
    int16_t ax[96], ay[96]; int as[LV_RD_HM_NEV], an[LV_RD_HM_NEV], na = 0;
    int bxa = L, bxb = R, bya = HYT / 3 - 1, byb = T->yb;
    for (int i = 0; i < lv_rd_hm_nev; ++i) {
        as[i] = na; an[i] = 0;
        const int dt = (int)(t - lv_rd_hm_ev[i].hit); if (!cur || dt < 0 || dt >= 120 || !lit) continue;
        const uint32_t h = lv_rd_ch(lv_rd_hm_ev[i].hit / 120, 7); const int tb = (int)(h % (uint32_t)(lit < 3 ? lit : 3));
        const int bx0 = HX0 + 2, by0 = y1[tb] + 1, by1 = y0[tb] - 1;
        const int ex = (bx0 + 3) / 3, ey = (by0 + by1) / 6, dx = cx > ex ? -1 : 1;   /* mock: contact right of the glass (dx -1) */
        int x = cx, y = cy, step = 0;
        while ((dx < 0 ? x > ex : x < ex) && na < 94) {
            ax[na] = (int16_t)x; ay[na++] = (int16_t)y; x += dx; ++step;
            if ((step == 2 || step == 4) && y != ey) { y += ey > y ? 1 : -1; ax[na] = (int16_t)x; ay[na++] = (int16_t)y; }
        }
        ax[na] = (int16_t)ex; ay[na++] = (int16_t)ey; an[i] = na - as[i];
    }
    for (int j = 0; j < na; ++j) { bxa = ax[j] < bxa ? ax[j] : bxa; bxb = ax[j] > bxb ? ax[j] : bxb; bya = ay[j] - 1 < bya ? ay[j] - 1 : bya; byb = ay[j] + 1 > byb ? ay[j] + 1 : byb; }
    if (cur) { bxa = cx - 2 < bxa ? cx - 2 : bxa; bxb = cx + 2 > bxb ? cx + 2 : bxb; bya = cy - 2 < bya ? cy - 2 : bya; byb = cy + 2 > byb ? cy + 2 : byb; }
    bya = bya > B.y0 ? bya : B.y0; byb = byb < B.y0 + B.h - 1 ? byb : B.y0 + B.h - 1;
    bya = bya > LV_RD_Y0 ? bya : LV_RD_Y0; byb = byb < LV_RD_CG_YMAX ? byb : LV_RD_CG_YMAX;
    bxa = bxa > B.x0 ? bxa : B.x0; bxb = bxb < B.x0 + B.w - 1 ? bxb : B.x0 + B.w - 1;
    if (bya > byb || bxa > bxb) return;
    if (lv_rd_cgb) lv_rd_hulls(T->i);
    const uint32_t PK = 0xc4eaff, ICE2 = 0xa8dcff, ICE3 = 0x8ccaff;
    const float CAP = 200;
    for (int y = bya; y <= byb; ++y) for (int x = bxa; x <= bxb; ++x) {
        const int X = 3 * x + 1, Y = 3 * y + 1;
        uint16_t &q = B.px[(y - B.y0) * B.w + (x - B.x0)];
        /* ---- A: thermometer bands (opaque, draw_A paint order) ---- */
        uint32_t a = 0xffffffffu;
        for (int i = 0; i < LV_RD_HM_N; ++i) {
            if (y0[i] <= y1[i]) continue;
            if (i < lit) {
                if (lv_rd_hm_in(X, Y, HX0 + 1, y1[i], HX1 - 1, y0[i])) a = LV_RD_HM_GLO[i];
                if (lv_rd_hm_in(X, Y, HX0 + 2, y1[i] + 1, HX1 - 2, y0[i] - 1)) a = LV_RD_HM_COL[i];
                if (lv_rd_hm_in(X, Y, HX0 + 2, y1[i] + 1, HX1 - 2, y1[i] + 2)) a = LV_RD_HM_HI[i];
            } else if (lv_rd_hm_in(X, Y, HX0 + 2, y1[i] + 2, HX1 - 2, y0[i] - 2)) a = LV_RD_HM_DIM;
        }
        if (lv_rd_hm_in(X, Y, HX0 - 3, HYT, HX0 - 1, HYB) || lv_rd_hm_in(X, Y, HX1 + 1, HYT, HX1 + 3, HYB) || lv_rd_hm_in(X, Y, HX0 - 3, HYB, HX1 + 3, HYB + 2)) a = C8;
        bool hit = a != 0xffffffffu; float c[3];
        if (hit) lv_rd_hm_ld(c, a);
        else { const int r5 = q >> 11 & 31, g6 = q >> 5 & 63, b5 = q & 31; c[0] = (float)(r5 << 3 | r5 >> 2); c[1] = (float)(g6 << 2 | g6 >> 4); c[2] = (float)(b5 << 3 | b5 >> 2); }
        /* ---- V2: band flashes on landing, ripple + arc per packet, contact port ---- */
        for (int i = 0; i < lv_rd_hm_nev; ++i) {
            const LvRdHmEv &e = lv_rd_hm_ev[i]; const int dt = (int)(t - e.land);
            if (!e.landed || dt < 0 || dt >= 300 || e.band < 0 || e.band >= lit) continue;
            if (lv_rd_hm_in(X, Y, HX0 + 2, y1[e.band] + 1, HX1 - 2, y0[e.band] - 1)) lv_rd_hm_mix(c, PK, (e.isnew ? 0.85f : 0.50f) * powf(1 - dt / 300.0f, 1.2f), CAP, hit);
        }
        for (int i = 0; i < lv_rd_hm_nev; ++i) {
            const int dt = (int)(t - lv_rd_hm_ev[i].hit); if (dt < 0) continue;
            for (int b = 0; b < lit; ++b) { const int db = dt - 25 * b; if (db < 0 || db >= 220) continue;
                if (lv_rd_hm_in(X, Y, HX0 + 2, y1[b] + 1, HX1 - 2, y0[b] - 1)) lv_rd_hm_mix(c, PK, (b < 3 ? 0.80f : 0.55f - 0.015f * b) * powf(1 - db / 220.0f, 1.6f), CAP, hit); }
            if (!an[i]) continue;
            const float kf = 1 - dt / 150.0f;
            for (int j = as[i]; j < as[i] + an[i]; ++j) { if (ax[j] != x) continue;
                if (ay[j] == y) lv_rd_hm_mix(c, PK, 0.95f * kf, CAP, hit);
                if (ay[j] - 1 == y) lv_rd_hm_mix(c, ICE3, 0.6f * kf, CAP, hit);
                if (ay[j] + 1 == y) lv_rd_hm_mix(c, ICE3, 0.6f * kf, CAP, hit); }
        }
        if (cur) {
            const float ddx = (float)(X - (3 * cx + 1)), ddy = (float)(Y - (3 * cy + 1)), dd = sqrtf(ddx * ddx + ddy * ddy);
            if (dd < 8) { const float u = 1 - dd / 8; lv_rd_hm_mix(c, ICE2, (0.42f * brt + 0.35f * bump) * u * u, CAP, hit); }
            if (x == cx && y == cy) lv_rd_hm_mix(c, PK, 0.55f + 0.3f * bump, CAP, hit);
        }
        if (!hit) continue;
        int o = 0; if (LV_RD_HM_OCC && lv_rd_cgb) { lv_rd_gy = T->yb; o = lv_rd_occ(x, y); }
        if (o == 2) continue;   /* a nearer tower's edge line stays on top */
        int v[3]; for (int i = 0; i < 3; ++i) { const int r = (int)(c[i] + 0.5f); v[i] = r < 0 ? 0 : r > 255 ? 255 : r; }
        const uint16_t m = lv_rgb((uint8_t)v[0], (uint8_t)v[1], (uint8_t)v[2]);
        q = o == 1 ? lv_mix(q, m, LV_RD_GLASS, 16) : m;   /* behind a nearer tower: through its glass */
    }
}
#endif
#endif
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
#if LV_RD_CHARGE && !LV_RD_CG_NODRAW
    lv_rd_cg_lit(B);
#endif
    if (lv_rd_holding && !lv_rd_lock) {
        const LvTower *T = lv_rd_tower_of(lv_rd_holdmac); if (!T) return;
        float u = (lv_now - lv_rd_hold_t) / 1000.0f; if (u > 1) u = 1; if (u < 0) u = 0;
        const int pad = 3 + (int)lv_lroundf((1 - u) * 12); int xa, xb, ya, yb;
        lv_rd_box(*T, pad, xa, xb, ya, yb); lv_rd_brackets(B, xa, ya, xb, yb, 4, lv_dim(MAG, 4 + (int)(u * 4), 8));
        return;
    }
    if (!lv_rd_lock) return;
    const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (!T) return;
    const int e = (int)(lv_now - lv_rd_lock_t);
    const int pulse = 5 + (int)lv_lroundf(3 * (0.5f + 0.5f * sinf(e / 1000.0f * 6.283f * 1.6f)));   /* 5..8 of 8 */
    int xa, xb, ya, yb; lv_rd_box(*T, 3, xa, xb, ya, yb);
    if (e < 200 && !(LV_RD_CHARGE && !LV_RD_CG_NODRAW)) lv_rd_rect_blend(B, xa - 1, ya - 1, xb + 1, yb + 1, W, 7 - e * 7 / 200);       /* lock snap flash (charge lock: never white) */
    lv_rd_brackets(B, xa, ya, xb, yb, 4, lv_dim(MAG, pulse, 8));
    const int cx = (xa + xb) / 2; b_line(B, cx, ya - 1, cx, ya - 3, lv_dim(W, pulse, 8));
    const char *lab = lv_rd_hunting ? "HUNT" : "LOCK";   /* same 3x5 label, same width (4 chars) */
    int ly = ya - 9; if (ly < LV_RD_Y0 + 1) ly = LV_RD_Y0 + 1;
    b_text35(B, cx - lv_w35(lab) / 2, ly, lab, lv_dim(MAG, pulse, 8));
}
static void lv_rd_panel_buttons(LvBand &B)   /* locked: IGNORE / WATCH / HUNT in the radar's own panel style */
{
    const bool wat = lv_rd_watched(lv_rd_lockmac);
    const char *L[4] = {"IGNORE", wat ? "UNWATCH" : "WATCH", lv_rd_hunting ? "STOP HUNT" : "HUNT", "INFO"};
    const uint16_t C[4] = {lv_hex(0xa0a0a8), WATCH_COLOR, lv_hex(0xff2ad4), lv_hex(0x30e8ff)};
    for (int k = 0; k < LV_RD_NBTN; ++k) {
        const LvRect &r = LV_RD_BTNR[k]; const int w = r.x1 - r.x0 + 1, h = r.y1 - r.y0 + 1;
        const bool on = (k == 1 && wat) || (k == 2 && lv_rd_hunting);
        b_fill(B, r.x0, r.y0, w, h, on ? lv_dim(C[k], 3, 8) : lv_hex(0x000000));
        b_frame(B, r.x0, r.y0, w, h, C[k]);
        b_text35(B, r.x0 + (w - lv_w35(L[k])) / 2, LV_RD_NBTN == 4 ? r.y0 + (h - 5) / 2 : r.y0 + 3, L[k], on ? lv_hex(0xffffff) : C[k]);
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
#include "lv_radar_pad.h"
static void lv_rd_term(LvBand &B, uint16_t hdr)
{
#if LV_RD_PAD
    lv_rd_pad_draw(B, hdr);
    return;
#endif

    const int x0 = 1, y0 = 287, x1 = 238, y1 = 319;   /* v1.2.9: full width, matching the 1 px left margin */
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
    rx -= 4 + 5 + lv_w35("BLE"); b_fill(B, rx, py, 3, 5, lv_hex(LV_BLE_COL)); b_text35(B, rx + 5, py, "BLE", lv_hex(LV_BLE_COL));
    rx -= 4 + 5 + lv_w35("WIFI"); b_fill(B, rx, py, 3, 5, lv_hex(LV_WIFI_COL)); b_text35(B, rx + 5, py, "WIFI", lv_hex(LV_WIFI_COL));
    rx -= 4 + lv_w35("HEIGHT=SIGNAL"); b_text35(B, rx, py, "HEIGHT=SIGNAL", lv_hex(0xffffff));
}
#if LV_RD_INFO
static void lv_rd_info_draw(LvBand &B);
#endif
#if LV_RD_FOLD
static void lv_rd_fold_draw(LvBand &B);
#endif
static void lv_rd_tower_rows(LvBand &B, const LvTower &T, int frame, const uint8_t *data);
static uint8_t lv_rd_dl_rows[6]; static int lv_rd_dl_rows_frame = 0;
#if LV_RD_INFO_TOWER_PULL
static void lv_rd_info_tower_fade(LvBand &B, int frame, const LvFind &f, const uint8_t *mac, float a);
#endif
static void lv_scene_radar(LvBand &B, void *)
{
#if LV_RD_SKINS
    const int sk = lv_rd_skin;
#else
    const int sk = 0;
#endif
#if LV_RD_INFO
    if (lv_rd_info) { lv_rd_info_draw(B); return; }
#endif
    const float t = (lv_now - lv_rd_t0 + LV_RD_SOFF) / 1000.0f;
    const uint16_t hdr = lv_ink(LV_HDR);
    uint16_t mag = lv_hex(0xff2ad4), grid = lv_hex(0x7a1cff);
    if (sk == 1) { mag = lv_hex(0x3b2e1e); grid = lv_hex(0x5c4a32); }        /* MO v11: dark-timber horizon rail + timber tatami seams */
    else if (sk == 2) { mag = lv_hex(0x6fd3ea); grid = lv_hex(0x2c3a4c); }   /* TR: cyan horizon + steel floor grid */
    else if (sk == 3) { mag = lv_hex(0x24d6c4); grid = lv_hex(0x0d5a63); }   /* CT: teal horizon + deep-teal floor grid */
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        uint16_t c = 0;
        if (y >= LV_RD_Y0 && y <= LV_RD_Y1) {
            if (sk == 1) { if (y < LV_RD_HOR) { const int k = (y - LV_RD_Y0) * 10 / (LV_RD_HOR - LV_RD_Y0); c = lv_mix(lv_hex(0x9c8360), lv_hex(0xc8b38c), k, 10); } else c = lv_hex(0xe8dec8); }   /* MO v11: Gibson's sky ramp as the cedar dojo ceiling + parchment floor */
            else if (sk == 2) { if (y < LV_RD_HOR) { const int k = (y - LV_RD_Y0) * 10 / (LV_RD_HOR - LV_RD_Y0); c = lv_rgb((uint8_t)(6 + k), (uint8_t)(12 + k * 2), (uint8_t)(20 + k * 3)); } else c = lv_hex(0x060b12); }   /* TR: deep steel void */
            else if (sk == 3) { if (y < LV_RD_HOR) { const int k = (y - LV_RD_Y0) * 10 / (LV_RD_HOR - LV_RD_Y0); c = lv_rgb(0, (uint8_t)(8 + k * 2), (uint8_t)(10 + k * 2)); } else c = lv_hex(0x02100f); }   /* CT: deep teal void */
            else if (y < LV_RD_HOR) { const int k = (y - LV_RD_Y0) * 10 / (LV_RD_HOR - LV_RD_Y0); c = lv_rgb((uint8_t)(4 + k * 2), 0, (uint8_t)(10 + k * 3)); }
            else c = lv_hex(0x050010);
        }
        b_fill(B, B.x0, y, B.w, 1, c);
    }
    /* skyline: a wireframe far city on the horizon */
    static const int8_t SKY[24] = {9, 14, 6, 22, 12, 30, 8, 18, 26, 11, 16, 34, 10, 20, 7, 28, 15, 24, 9, 13, 31, 17, 6, 19};
    const int shift = (int)(lv_rd_camx * 20);
#if LV_RD_SKINS && LV_MO_INSIDE
    if (sk == 1) mo11_horizon(B, shift); else   /* MO v11: the same 24 skyline slots as a far shoji wall + posts + rafters */
#endif
    for (int k = 0; k < 24; ++k) {
        const int x0 = k * 11 - 6 - shift, w = 9, ht = SKY[k];
        const uint16_t c = sk == 1 ? lv_mix(lv_hex(0xf4f6f8), grid, 5, 8) : lv_dim(grid, 4, 8);
        b_line(B, x0, LV_RD_HOR, x0, LV_RD_HOR - ht, c); b_line(B, x0 + w, LV_RD_HOR, x0 + w, LV_RD_HOR - ht, c); b_line(B, x0, LV_RD_HOR - ht, x0 + w, LV_RD_HOR - ht, c);
        for (int wy = LV_RD_HOR - ht + 3; wy < LV_RD_HOR - 1; wy += 4) for (int wx = x0 + 2; wx < x0 + w - 1; wx += 3) if (((wx * 7 + wy * 13 + k) % 5) == 0) b_pix(B, wx, wy, sk == 1 ? lv_hex(0x8a96a3) : lv_dim(lv_hex(0xffd040), 5, 8));
    }
    /* ground grid: per row depth, scrolling lines toward the viewer */
    const float ph = lv_rainmode == 2 ? 0.0f : fmodf(t * 0.8f, 1.0f);
    for (int y = (B.y0 > LV_RD_HOR + 1 ? B.y0 : LV_RD_HOR + 1); y < B.y0 + B.h && y <= LV_RD_Y1; ++y) {
        const float Z = LV_RD_F * LV_RD_CAMH / (y - LV_RD_HOR), Zn = LV_RD_F * LV_RD_CAMH / (y + 1 - LV_RD_HOR);
        const int k = Z > 6 ? 2 : Z > 3 ? 4 : 7;
#if LV_RD_SKINS && LV_MO_INSIDE
        if (sk == 1) mo11_tatami(B, y, Z, ph);   /* MO v11: two-tone tatami under Gibson's own seam rows + lanes */
#endif
        const uint16_t gc = (sk == 1) ? lv_mix(lv_hex(0xe8dec8), grid, k, 8) : lv_dim(grid, k, 8);
        if (floorf(Z + ph) != floorf(Zn + ph)) b_fill(B, 0, y, 240, 1, gc);
        for (int g = -12; g <= 12; ++g) { const int x = (int)lv_lroundf(120 + LV_RD_F * (g * 0.5f - lv_rd_camx) / Z); b_pix(B, x, y, gc); }
        if (lv_rainmode == 1) {   /* SWEEP: a bright line rolls from the horizon to you */
            const float sz = 6.0f - fmodf(t * 1.6f, 5.4f);
            if (Z >= sz && Zn < sz) b_fill(B, 0, y, 240, 1, lv_hex(0x40ff90));
        }
    }
    b_fill(B, 0, LV_RD_HOR, 240, 1, mag);
#if LV_BG_ENGINE
    /* background engine layer (BG STYLE / BG COLOR; default RAIN + GREEN = the original code streaks) */
    lv_rd_bg(B);
#else
    /* code streaks */
    if (lv_rain) for (int s = 0; s < 18; ++s) {
        const int x = (s * 53 + 17) % 236 + 2, len = 10 + (s * 7) % 18, sp = 40 + (s * 29) % 70;
        const int head = LV_RD_Y0 + (int)((lv_now / 1000.0f * sp + s * 37)) % (LV_RD_Y1 - LV_RD_Y0 + len) ;
        for (int k = 0; k < len; k += 2) { const int y = head - k; if (y >= LV_RD_Y0 && y <= LV_RD_Y1) b_blend(B, x, y, sk == 1 ? lv_hex(k == 0 ? 0x10d858 : 0x00a83c) : lv_rd_rain_col(8), k == 0 ? 8 : 6 - k * 5 / len); }   /* MO v11: Matrix green shimmer in the dojo */
    }
#endif
    /* towers, far to near: translucent faces, bright edges */
    for (int n = 0; n < lv_rd_n; ++n) {
        const LvTower &T = lv_rd_tw[n]; const LvFind &f = lv_f[T.i];
#if LV_RD_SKINS
        const int fa = lv_rd_skin_fa;
#else
        const int fa = 3;
#endif
        const int lev = lv_rd_level(f), br = 3 + lev * 5 / 60;           /* 3..8 of 8 */
        const bool sel = T.i == lv_rd_sel
#if LV_RD_CHARGE && !LV_RD_CG_NODRAW
            && !(lv_rd_lock && !memcmp(lv_f[T.i].mac, lv_rd_lockmac, 6))
#endif
            ;
        /* v1.2.14 (Mobandy 11:33 AM ET pt1481u "why is the tower clear"): the GIBSON pulled-up tower is drawn with HACKbot's
         * open-tower inputs. HACKbot's open tower is never lv_rd_sel (lv_dl_lock sets sel -1) and its game towers sit at
         * -70..-45 dBm (br 5..7; the open T4 = 7). GIBSON's was still lv_rd_sel (white edges; the v1.2.8 NODRAW build has no
         * lock exclusion above) at its real dBm (a -82 dBm node = br 4, the face 4/8 x 3/8 = nearly clear). Pulled: no
         * select-white, and br eases to HACKbot's 7 with the pull. The state colour (alert / watch / WIFI / BLE) is kept. */
        const bool pul = lv_rd_pull_p > 0 && lv_rd_pull_scr == LV_SC_RADAR && lv_scr == LV_SC_RADAR && sk == 0 && !memcmp(f.mac, lv_rd_pull_mac, 6);
        const int brp = pul && br < LV_SEL_OPEN_BR ? br + (int)lv_lroundf((LV_SEL_OPEN_BR - br) * lv_rd_pull_p) : br;
        uint16_t edge = sel && !pul ? lv_hex(0xffffff) : lv_rd_col(f, brp, 8), face = lv_rd_col(f, brp, 8);
#if LV_RD_SKINS
        if (sk == 2) { face = lv_hex(0xdfe6ef); edge = sel ? lv_hex(0x30e8ff) : lv_hex(0x8d99ab); }   /* TR: clean silver relays, cyan select */
        else if (sk == 3) { const uint16_t a = lv_hex(lv_rd_skin_acc); face = a; edge = sel ? lv_hex(0xffb000) : lv_hex(0x7af0e2); }  /* CT: teal tower, amber select */
#endif
#if LV_RD_SKINS && LV_MO_INSIDE
        if (sk == 1) mo11_ink(T, face, edge);   /* MO v11: dark-lacquer pillar + timber edges, the Gibson drawing below unchanged */
#endif
#if LV_RD_INFO_TOWER_PULL
        /* MOCK: at the end of the pull, this node is the INFO monolith (drawn after the loop), not a widened scan cube. */
        if (sk == 0 && lv_rd_pull_p >= 1.f && lv_rd_pull_scr == lv_scr && !memcmp(f.mac, lv_rd_pull_mac, 6)
            && (lv_scr == LV_SC_RADAR || lv_scr == LV_SC_DL)) continue;
#endif
        int ya = T.byt, yb = T.yb; lv_rd_vclip(B, ya, yb);
        /* top face (back-top edge to front-top edge) */
        for (int y = b_ylo(B, T.byt); y <= b_yhi(B, T.yt); ++y) { if (y < LV_RD_Y0 || y > LV_RD_Y1) continue; const float u = T.yt == T.byt ? 1.0f : (float)(y - T.byt) / (T.yt - T.byt);
            const int xa = (int)(T.bx0 + (T.x0 - T.bx0) * u), xb = (int)(T.bx1 + (T.x1 - T.bx1) * u); for (int x = xa; x <= xb; ++x) b_blend(B, x, y, face, fa); }
        /* front face */
        int cra = 1, crb = 0;   /* perf-all r2: rows the Dojo skin repaints opaque over this same front face (x0..x1) right after; blending there is dead work */
#if LV_RD_SKINS && LV_MO_INSIDE
        if (sk == 1 && !mo11_body_rows(T, cra, crb)) { cra = 1; crb = 0; }
#endif
        for (int y = b_ylo(B, T.yt); y <= b_yhi(B, T.yb); ++y) { if (y < LV_RD_Y0 || y > LV_RD_Y1 || (y >= cra && y <= crb)) continue; for (int x = T.x0; x <= T.x1; ++x) b_blend(B, x, y, face, fa); }   /* v1.2.11: band rows only (both face loops) */
        b_line(B, T.x0, T.yt, T.x0, T.yb, edge); b_line(B, T.x1, T.yt, T.x1, T.yb, edge);
        b_line(B, T.x0, T.yt, T.x1, T.yt, edge); b_line(B, T.x0, T.yb, T.x1, T.yb, lv_dim(edge, 1, 2));
        b_line(B, T.bx0, T.byt, T.bx1, T.byt, lv_dim(edge, 5, 8)); b_line(B, T.bx0, T.byt, T.x0, T.yt, edge); b_line(B, T.bx1, T.byt, T.x1, T.yt, edge);
        if (T.bx1 > T.x1) b_line(B, T.bx1, T.byt, T.bx1, T.byb, lv_dim(edge, 5, 8));
        if (T.bx0 < T.x0) b_line(B, T.bx0, T.byt, T.bx0, T.byb, lv_dim(edge, 5, 8));
        if (f.alert && ((lv_now / 300) & 1)) b_text35(B, (T.x0 + T.x1) / 2 - 1, T.byt - 7, "!", lv_hex(0xff4040));
#if !LV_RD_INFO_TOWER_PULL
        if (sk == 0 && lv_rd_lock && lv_rd_pull_p > 0 && lv_rd_pull_scr == lv_scr && !memcmp(f.mac, lv_rd_pull_mac, 6)
            && !memcmp(f.mac, lv_rd_lockmac, 6) && (lv_scr == LV_SC_RADAR || lv_scr == LV_SC_DL)) {
            const int frame = lv_scr == LV_SC_RADAR ? (int)((uint32_t)(lv_now - lv_rd_lock_t) * 3u / 100u) : lv_rd_dl_rows_frame;
            lv_rd_tower_rows(B, T, frame, lv_scr == LV_SC_RADAR ? f.mac : lv_rd_dl_rows);
        }
#endif
#if LV_RD_SKINS && LV_MO_INSIDE
        if (sk == 1) mo11_tower_fx(B, T);   /* MO v11: shoji ribs, green energy seam, frosted ghost icon, green flood on the open pillar */
#endif
    }
#if LV_RD_SKINS && LV_MO_INSIDE
    if (sk == 1) { mo11_overlay(B); return; }   /* MO v11: green corner ticks, pill, 3D shoji panels, dojo header / panel / terminal / BACKbot */
#endif
    lv_rd_fx(B);
#if LV_RD_INFO_TOWER_PULL
    if (lv_rd_pull_p > 0 && lv_rd_pull_scr == lv_scr && (lv_scr == LV_SC_RADAR || lv_scr == LV_SC_DL)) {
        const int i = lv_rd_find(lv_rd_pull_mac);
        if (i >= 0) {
            const int frame = lv_scr == LV_SC_RADAR ? (int)((uint32_t)(lv_now - lv_rd_lock_t) * 3u / 100u) : lv_rd_dl_rows_frame;
            const uint8_t *data = lv_scr == LV_SC_RADAR ? lv_f[i].mac : lv_rd_dl_rows;
            lv_rd_info_tower_fade(B, frame, lv_f[i], data, lv_rd_pull_p);
        }
    }
#endif
#if LV_RD_FOLD
    if (lv_rd_fold_mode == 1) lv_rd_fold_draw(B);
#endif
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
#if LV_RD_INFO
        const int py0 = lv_rd_lock ? LV_RD_PTOP : 262;   /* v3: 247 as before unless LV_RD_ROW = 1 (GROW) */
#else
        const int py0 = lv_rd_lock ? 247 : 262;   /* locked: the panel grows up 15 px for the IGNORE / WATCH / HUNT row */
#endif
#if LV_RD_PAD
        (void)l; (void)t; (void)m;   /* v1.2.8: name/dBm mid-panel overlaps the wider deck; pad status replaces it */
#else
        b_fill(B, 2, py0, 236, 285 - py0, lv_hex(0x000000)); b_frame(B, 2, py0, 236, 285 - py0, lv_rd_col(f, 8, 8));
        if (lv_rd_lock && LV_RD_ROW != 2) lv_rd_panel_buttons(B);   /* FLOAT: drawn last, over the current (below) */
        b_text(B, 5, 264, l, lv_hex(0xffffff));
        snprintf(t, sizeof t, "%d", f.rssi); b_text(B, 235 - ((int)strlen(t) + 4) * 6, 264, t, lv_rd_col(f, 8, 8)); b_text35(B, 235 - 4 * 6 + 4, 265, "DBM", lv_rd_col(f, 8, 8));
        snprintf(t, sizeof t, "%s %s  %s", f.kind == LV_K_WIFI ? "WIFI" : "BLE", LV_ICON_NAME[f.icon], m); b_text35(B, 5, 275, t, lv_hex(LV_TAN));
#endif
    }
    /* footer (item 13): a 3-line boxed mini terminal that types out live radar info over a >_ prompt, legend folded into the prompt */
    lv_rd_term(B, hdr);
#if LV_RD_CHARGE && !LV_RD_CG_NODRAW
    lv_rd_cg_draw(B);
#if LV_RD_HUNT_METER && LV_RD_HM_TOWER
    lv_rd_hm_draw(B);   /* after the current: the bands sit in the glass over the contact glow, as in the mock */
#endif
#endif
#if LV_RD_CHARGE && defined(LV_RD_CG_LOG)
    if (lv_rd_lock) { const LvTower *T = lv_rd_tower_of(lv_rd_lockmac); if (T) { int xa, xb, ya, yb; lv_rd_box(*T, 3, xa, xb, ya, yb); lv_rd_cg_log("TWR", xa, ya - 9, xb, yb); } }
    else if (lv_rd_sel >= 0) { const LvTower *T = lv_rd_tower_of(lv_f[lv_rd_sel].mac); if (T) { int xa, xb, ya, yb; lv_rd_box(*T, 3, xa, xb, ya, yb); lv_rd_cg_log("TWR", xa, ya - 9, xb, yb); } }
#endif
    if (LV_RD_ROW == 2 && lv_rd_lock && lv_rd_sel >= 0 && lv_rd_sel < lv_nf) lv_rd_panel_buttons(B);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
}
/* issue 5 (MOCK): while hunting on GIBSON the top banner (ticker rect, inside the gold rim) is a FIXED bar:
 * HUNT + MAC tail / name | a signal meter from the real dBm (lv_rd_level) | live dBm | WARMER / COLDER / STEADY = only the
 * change of the real dBm between two reads (+-2 dB = STEADY). No arrow, compass or bearing: nothing implies direction. */
/* item 6: the top-banner 11-bar meter is restored below, so the 3x clear_top_meter glyph helper (lv_rd_r3) is gone. */
static bool lv_hunt_bar_on(void) { return lv_rd_hunting && lv_rd_tgt_have && lv_scr == LV_SC_RADAR; }
static void lv_hunt_bar(LvBand &B)
{
    const uint16_t MAG = lv_hex(0xff2ad4), GOLD = lv_hex(0xffb000);
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x1a0012));
    for (int y = B.y0; y < B.y0 + B.h; ++y) {   /* the stripes stay, frozen, magenta + gold = hunt mode */
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) b_pix(B, x, y, (((x + (y <= 3 ? y : -y)) / 5) & 1) ? MAG : GOLD);
    }
    const LvFind &f = lv_rd_tgt; char m[18], t[24], l[26]; lv_macstr(f.mac, m);
    if (f.name[0]) snprintf(l, sizeof l, "%.13s", f.name); else snprintf(l, sizeof l, "%s %.8s", f.kind == LV_K_WIFI ? "WIFI" : "BLE", LV_ICON_NAME[f.icon]);   /* no name: kind + type, the MAC tail is on the line above */
    snprintf(t, sizeof t, "HUNT ..%s", m + 9);
    const int hx = LV_BANNER_BACK_ON ? (LV_R_BANNER_BACK.x1 + 3) : 3;
    b_text35(B, hx, 5, t, MAG); b_text(B, hx, 12, l, lv_hex(0xffffff));
    const int miss = lv_rd_nosig(), hold = lv_rd_miss > 0 && !miss;   /* hold: bars stay, number greys */
    /* item 6 (Mobandy 2026-11-06, "keep the meter for hunter on the top also ... the boarder alert ... both places"):
     * the top border-alert banner keeps the original 11-bar signal meter right where it used to be (3240b03 / 3b8b3de),
     * AND the hunted tower keeps the LOCKED A thermometer bands + V2 base arc / ripple (drawn by lv_rd_hm_draw). Both
     * read the same live target dBm (lv_rd_level). This supersedes the b29f8ab clear_top_meter glyph in the banner. */
#if LV_RD_HM_TOPBAR
    const int lit = (lv_rd_level(f) - 4) * 11 / 56 + 1;
    for (int i = 0; i < 11; ++i) {
        const int x = 84 + i * 6, h = 5 + i, y = 20 - h + 1;
        const uint32_t on = i < 3 ? 0xff3030 : i < 7 ? 0xffb000 : 0x30ff60;
        b_fill(B, x, y, 5, h, i < lit && !miss ? lv_hex(on) : lv_hex(0x3a2a34));
    }
#endif
    snprintf(t, sizeof t, "%d", f.rssi);
    const uint16_t num = miss || hold ? lv_hex(0x808080) : lv_hex(0xffffff);
    b_text(B, 190 - (int)strlen(t) * 12, 6, t, num, 2); b_text35(B, 191, 15, "DB", num);
    const char *w; char d[16] = ""; uint32_t k;
    if (miss) { w = "NO SIG"; snprintf(d, sizeof d, "X%d", lv_rd_miss); k = 0xff3030; }
    else if (hold) { w = "HOLD"; snprintf(d, sizeof d, "X%d", lv_rd_miss); k = 0xc0c0c0; }   /* NEW label: needs Mobandy's OK (mock first) */
    else if (lv_rd_reads < 2) { w = "1ST"; snprintf(d, sizeof d, "READ"); k = 0xc0c0c0; }
    else if (lv_rd_trend > 0) { w = "WARMER"; snprintf(d, sizeof d, "%+dDB", lv_rd_trend); k = 0xff8020; }
    else if (lv_rd_trend < 0) { w = "COLDER"; snprintf(d, sizeof d, "%+dDB", lv_rd_trend); k = 0x40c8ff; }
    else { w = "STEADY"; snprintf(d, sizeof d, "READ %d", lv_rd_reads); k = 0xc0c0c0; }
    b_text35(B, 236 - lv_w35(w), 6, w, lv_hex(k)); b_text35(B, 236 - lv_w35(d), 14, d, lv_hex(k));
    lv_banner_back_draw(B);
}
/* hunt ticker line: the target's full info + a trend that is ONLY the change of its real dBm (no direction) */
static void lv_rd_hunt_line(void)
{
    if (!lv_rd_tgt_have) return;
    char l[LV_TKLEN]; lv_full_line(lv_rd_tgt, 0, 0, l, sizeof l);
    const char *tr = lv_rd_reads < 2 ? "FIRST READ" : lv_rd_trend > 0 ? "WARMER" : lv_rd_trend < 0 ? "COLDER" : "STEADY";
    char d[24] = ""; if (lv_rd_reads >= 2 && lv_rd_trend) snprintf(d, sizeof d, " %+ddB", lv_rd_trend);
    if (lv_rd_nosig()) snprintf(lv_tk_msg, sizeof lv_tk_msg, "HUNT: NO SIGNAL x%d (LAST %ddBm)  %s", lv_rd_miss, lv_rd_tgt.rssi, l);
    else if (lv_rd_miss) snprintf(lv_tk_msg, sizeof lv_tk_msg, "HUNT: %s  HOLD (MISSED x%d)  #%d", l, lv_rd_miss, lv_rd_reads);
    else snprintf(lv_tk_msg, sizeof lv_tk_msg, "HUNT: %s  %s%s  #%d", l, tr, d, lv_rd_reads);
    lv_tk_alert = false;
}
static void lv_rd_track_start(void)   /* one focused re-scan of the target's radio (the radio API has no per-channel / per-address scan) */
{
    if (lv_job != LV_J_NONE || !lv_rd_tgt_have) return;
    lv_rd_trk = true; lv_hunt = -1; lv_store_keep = lv_rd_tgt.mac;   /* the cap never drops the target */
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
        const int was = lv_rd_tgt.rssi;
        lv_rd_sm4 += (lv_f[i].rssi * 4 - lv_rd_sm4) / 2;   /* EMA alpha 1/2 */
        lv_f[i].rssi = (int8_t)((lv_rd_sm4 - 2) / 4);       /* nearest dB (sm4 < 0); tower height, both meters + dBm read it */
        lv_rd_tgt = lv_f[i]; lv_rd_miss = 0;
        const int d = lv_rd_tgt.rssi - was; lv_rd_trend = (d >= 3 || d <= -3) ? d : 0;   /* +-2 dB = STEADY (noise) */
        ++lv_rd_reads;
#if 0   /* issue 5 (MOCK): per-read dBm / trend moved to the hunt bar; the mini terminal stays the live event log */
        lv_rd_log("#%d %dDBM %s", lv_rd_reads, lv_rd_tgt.rssi, lv_rd_trend > 0 ? "WARMER" : lv_rd_trend < 0 ? "COLDER" : "STEADY");
        if (lv_rd_trend) { char *e = lv_rd_tl[2] + strlen(lv_rd_tl[2]); snprintf(e, sizeof lv_rd_tl[2] - (e - lv_rd_tl[2]), " %+dDB", lv_rd_trend); }
#endif
    } else {   /* not heard this pass: keep its last real reading as the target (list full: it takes the weakest slot) */
        ++lv_rd_miss; if (lv_nf < LV_MAXF) lv_f[lv_nf++] = lv_rd_tgt; else lv_f[LV_MAXF - 1] = lv_rd_tgt; lv_sort();
        if (lv_rd_miss == LV_RD_MISS_GRACE) lv_rd_log("HUNT: SIGNAL LOST, RESCANNING");   /* one event line; the count + last dBm live on the hunt bar */
    }
    lv_rd_sel = lv_rd_find(lv_rd_tgt.mac);
    lv_rd_hunt_line(); lv_tk_off = 196;
}
static void lv_rd_hunt_stop(void)
{
    if (lv_rd_hunting && lv_rd_trk && lv_job != LV_J_NONE) { lv_radio->stop(); lv_job = LV_J_NONE; }
    lv_rd_hunting = false; lv_rd_trk = false; lv_dirty_box = true; lv_store_keep = nullptr;
}
static void lv_rd_lock_on(int i)
{
    lv_rd_lock = true; lv_rd_lock_t = lv_now; memcpy(lv_rd_lockmac, lv_f[i].mac, 6);
    lv_rd_sel = i; memcpy(lv_rd_selmac, lv_f[i].mac, 6); lv_rd_holding = false;
    char m[18]; lv_macstr(lv_f[i].mac, m); lv_rd_log("LOCK %s  IGNORE / WATCH / HUNT", m);
#if LV_RD_FOLD
    if (lv_rd_fold_mode != 1) { lv_rd_fold_mode = 1; lv_rd_fold_t = lv_sel_clk_now(lv_rd_fclk, lv_now); lv_rd_fold_from = lv_rd_camx; }
    memcpy(lv_rd_fold_mac, lv_f[i].mac, 6); lv_rd_fsel = 0;   /* v1.2.14: the folder cursor starts on HUNT */
#endif
}
static void lv_rd_hunt_go(int i)
{
    if (i < 0 || i >= lv_nf) return;
    if (!lv_rd_lock || memcmp(lv_rd_lockmac, lv_f[i].mac, 6)) lv_rd_lock_on(i);
    lv_rd_hunting = true; lv_rd_tgt = lv_f[i]; lv_rd_tgt_have = true; lv_rd_reads = 1; lv_rd_trend = 0; lv_rd_miss = 0; lv_rd_sm4 = lv_f[i].rssi * 4;
    lv_rd_hunt_line(); lv_tk_off = 196;
    { char l[26]; lv_label(lv_f[i], l, 20); lv_rd_log("HUNT %s  RESCAN %s", l, lv_f[i].kind == LV_K_WIFI ? "WIFI" : "BLE"); }
    lv_rd_track_start();
}
static void lv_rd_release(void)
{
    lv_rd_hunt_stop();
    lv_rd_lock = false; lv_rd_holding = false;
#if LV_RD_FOLD
    if (lv_rd_fold_mode == 1) { lv_rd_fold_mode = 2; lv_rd_fold_t = lv_sel_clk_now(lv_rd_fclk, lv_now); lv_rd_fold_from = lv_rd_camx; lv_rd_fold_p0 = lv_rd_pull_p; }
#endif
#if LV_RD_CHARGE
    lv_rd_cg = false; lv_rd_cg_rel = 0; lv_rd_cg_owned = false; lv_rd_cg_dropping = false;
#endif
}
#if LV_RD_INFO
/* ---- INFO (MOCK 1111): CYDbot's Joey file-extraction Gibson view (/workspace/out/cydbot-gibson-hackers, 2026-11-06) re-used
 * for the locked node. Only fields the firmware captures in LvFind are shown: name / SSID, MAC, WiFi or BLE, channel (WiFi),
 * dBm, security (WiFi auth) or vendor (BLE company id) + service UUID + Apple continuity, the TYPE the firmware classifies,
 * and the radar's own WATCH / hunt state. No first / last seen: LvFind keeps no timestamps. No invented hex / file names. */
/* ---- INFO v2 (Mobandy 2026-11-11 9:21 ET: "doesn't look like your other one"): a 1:1 port of CYDbot's 2026-11-06 Joey
 * file-extraction view (/workspace/out/cydbot-gibson-hackers/src/gibson_extract.cpp, same camera, towers, folders, chrome,
 * motion, colours and its own Bresenham lines). Only the TEXT is swapped for the locked node's captured LvFind fields, the
 * random hex for the node's real MAC bytes, the top 11-bar meter for its real dBm, and CYCLE TOWER for BACK. Frame clock =
 * the mock's 30 fps from the INFO tap. Folders = purposeful categories, each only with real data (see lv_rd_info_cats). */
#ifndef LV_RD_INFO_RBTN
#define LV_RD_INFO_RBTN 0   /* right button (was OPEN CAM FEED): 0 = HUNT / STOP HUNT, 1 = WATCH / UNWATCH, 2 = blank (frame only) */
#endif
static const LvRect LV_RD_INFO_BTN_L = {0, 262, 119, 289}, LV_RD_INFO_BTN_R = {120, 262, 239, 289};   /* finger targets (drawn 115x22) */
static inline void lvi_rect(LvBand &B, int x, int y, int w, int h, uint32_t c) { b_fill(B, x, y, w, h, lv_hex(c)); }
static inline void lvi_px(LvBand &B, int x, int y, uint32_t c) { if (x < 0 || y < 0 || x >= 240 || y >= 320) return; b_pix(B, x, y, lv_hex(c)); }
static void lvi_line(LvBand &B, int x, int y, int xx, int yy, uint32_t c)
{ if ((y < B.y0 && yy < B.y0) || (y >= B.y0 + B.h && yy >= B.y0 + B.h)) return;   /* v1.2.11 speed: wholly outside this band */
  /* v1.2.17 speed (pull-up glitch): a vertical line visits exactly the rows y..yy at column x (Bresenham with dx = 0), so only
   * the band's rows are walked. Same pixels; the INFO monolith's 103 full-height strokes no longer cost 199 steps per band
   * row (the v1.2.16 one-row fade ran them for every one of its 210 rows: ~13x the frame cost on the CYD). */
  if (x == xx) { int ya = y < yy ? y : yy, yb = y < yy ? yy : y; if (ya < B.y0) ya = B.y0; if (yb > B.y0 + B.h - 1) yb = B.y0 + B.h - 1;
    if (x < 0 || x >= 240) return; if (ya < 0) ya = 0; if (yb > 319) yb = 319;   /* lvi_px's own screen bounds */
    if (yb >= ya) b_fill(B, x, ya, 1, yb - ya + 1, lv_hex(c)); return; }
  int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1, e = dx + dy;
  for (;;) { lvi_px(B, x, y, c); if (x == xx && y == yy) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x += sx; } if (e2 <= dx) { e += dx; y += sy; } } }
static void lvi_box(LvBand &B, int x, int y, int w, int h, uint32_t c) { lvi_line(B, x, y, x + w - 1, y, c); lvi_line(B, x, y + h - 1, x + w - 1, y + h - 1, c); lvi_line(B, x, y, x, y + h - 1, c); lvi_line(B, x + w - 1, y, x + w - 1, y + h - 1, c); }
static inline void lvi_txt(LvBand &B, int x, int y, const char *s, uint32_t c, bool big = false) {
#ifdef LV_RD_INFO_NOTXT   /* host proof only: drop every glyph so the art can be diffed against the Nov 6 mock with its text dropped too */
  return;
#endif
 if (big) b_text(B, x, y, s, lv_hex(c)); else b_text35(B, x, y, s, lv_hex(c)); }
static unsigned lvi_band(int i, int f) { unsigned c = i < 6 ? 0x30ff90 : i < 9 ? 0xffb000 : 0xff385c; return i == (f / 4) % 11 ? c : ((c & 0xfefefe) >> 1); }
static void lvi_micro(LvBand &B, int x, int y, int f, int seed, const uint8_t *mac)   /* was random hex: now the node's MAC, 2 bytes per plate */
{ if (y + 5 < B.y0 || y - 1 >= B.y0 + B.h) return;   /* v1.2.17 speed: plate rows y-1..y+5 miss this band (draws nothing) */
  char s[6]; const int g = (unsigned)(seed + f / 3) % 3; snprintf(s, sizeof s, "%02X%02X", mac[g * 2], mac[g * 2 + 1]); lvi_rect(B, x - 1, y - 1, 18, 7, 0x030a14); lvi_txt(B, x, y, s, 0x23758c); }
/* v1.2.15: lv_rd_info_draw's tower-face band/glyph loop, verbatim (e4e3536:1310-1312; pre-extraction :1360-1362).
 * lvi_micro and lvi_band remain the original implementations above. Shared with the INFO screen, not a lookalike. */
static void lv_rd_info_rows(LvBand &B, int frame, const uint8_t *mac)
{
    int lit = 1 + frame / 11 < 11 ? 1 + frame / 11 : 11;   /* the monolith's bands: the mock's file-drop progress (decor, unchanged) */
    for (int i = 0; i < 11; i++) { int y = 238 - i * 18; unsigned co = i < lit ? lvi_band(i, frame) : 0x12313a; lvi_rect(B, 20, y - 4, 55, 5, co); lvi_rect(B, 21, y - 6, 53, 1, i < lit ? co : 0x17323d);
        lvi_micro(B, 24, y + 3 + (frame / 3 % 3), frame, 1471 + i * 131, mac); lvi_micro(B, 50, y + 3 + (frame / 3 % 3), frame, 717 + i * 131, mac); }
}
/* The original INFO close-up data tower (e4e3536 lv_rd_info_draw): front face 16,46,87x199/200, side 103..120,
 * roof/name plate, then lv_rd_info_rows. Coordinates, colours and strokes are cloned verbatim from that function.
 * Original colours are fixed (no node-state tint). Bounding box x16..120, y38..245. */
static void lv_rd_info_tower(LvBand &B, int frame, const LvFind &f, const uint8_t *mac)
{
    lvi_rect(B, 16, 46, 87, 199, 0x081b2a); for (int x = 17; x <= 101; x++) lvi_line(B, x, 46, x, 244, x % 8 == 0 ? 0x102c3a : 0x091d2b);
    for (int x = 103; x <= 120; x++) lvi_line(B, x, 46 - (x - 102) / 2, x, 245 - (x - 102) / 2, 0x071420);
    lvi_box(B, 16, 46, 87, 200, 0x42c9de); lvi_line(B, 16, 46, 34, 38, 0x69e7f0); lvi_line(B, 34, 38, 120, 38, 0x69e7f0); lvi_line(B, 103, 46, 120, 38, 0x30a2b8); lvi_line(B, 120, 38, 120, 236, 0x27849a); lvi_line(B, 103, 245, 120, 236, 0x27849a);
    if (b_yhit(B, 40, 46)) { char l[16]; lv_label(f, l, 13); lvi_rect(B, 19, 40, 56, 7, 0x050e18); lvi_txt(B, 21, 41, l, 0x30e8ff); }   /* v1.2.17: name plate rows 40..46 only */
    lv_rd_info_rows(B, frame, mac);
}
#if LV_RD_INFO_TOWER_PULL
/* v1.2.16: crossfade the verbatim INFO monolith over the current band (alpha = lv_sel_pull p).
 * One saved row at a time (the primitives already clip to the band) so firmware does not park a
 * 105*208 BSS bak in dram0 — that overflowed the linker by 42,904 B. Mix math is unchanged. */
static const int LV_RD_INFO_TX0 = 16, LV_RD_INFO_TX1 = 120, LV_RD_INFO_TY0 = 38, LV_RD_INFO_TY1 = 245;
static void lv_rd_info_tower_fade(LvBand &B, int frame, const LvFind &f, const uint8_t *mac, float a)
{
    if (a <= 0.f) return;
    const int ya = b_ylo(B, LV_RD_INFO_TY0), yb = b_yhi(B, LV_RD_INFO_TY1);
    if (yb < ya) return;
    if (a >= 1.f) { lv_rd_info_tower(B, frame, f, mac); return; }
    const int xa = LV_RD_INFO_TX0 > B.x0 ? LV_RD_INFO_TX0 : B.x0;
    const int xb = LV_RD_INFO_TX1 < B.x0 + B.w - 1 ? LV_RD_INFO_TX1 : B.x0 + B.w - 1;
    if (xb < xa) return;
    const int k = (int)lv_lroundf(a * 256.f);
    /* The monolith also stamps 1 px at y=37 (side peak) and y=246 (base). The mix window is
     * still y=38..245, same as the original bak[] fade; those two rows are drawn, not mixed. */
    const int y0d = b_ylo(B, LV_RD_INFO_TY0 - 1), y1d = b_yhi(B, LV_RD_INFO_TY1 + 1);
    for (int y = y0d; y <= y1d; ++y) {
        uint16_t line[105];
        uint16_t *row = B.px + (y - B.y0) * B.w;
        const bool mix = (y >= ya && y <= yb && xb >= xa);
        const int n = xb - xa + 1, off = xa - B.x0;
        if (mix) for (int i = 0; i < n; ++i) line[i] = row[off + i];
        LvBand one = B; one.px = row; one.y0 = y; one.h = 1;
        lv_rd_info_tower(one, frame, f, mac);
        if (mix) for (int i = 0; i < n; ++i) row[off + i] = lv_mix(line[i], row[off + i], k, 256);
    }
}
#endif
/* v1.2.15 (Mobandy 12:02 PM ET pt1501u "On the new tower"): the INFO monolith's band + hex-plate rows on the pulled-up
 * tower's front face. Source = the original raster of lv_rd_info_rows (1:1 at the full close-up, the 3x5 font unscaled).
 * The face is 40 px at the close-up (2 x 0.12 x LV_RD_F / LV_SEL_ZC) vs the monolith's 87, so it shows the source
 * window x21..60 (the left plate column whole, the right one cut by the tower edge), top-anchored at the monolith top y46.
 * While the tower flies in the same window scales down with the face (nearest-neighbour) = it zooms with the tower.
 * Drawn only inside the front face (outline kept), field rows only, right after that tower in the far -> near loop, so
 * nearer towers / folders / deck still draw over it. One 87-px source row of scratch per output row. */
static void lv_rd_tower_rows(LvBand &B, const LvTower &T, int frame, const uint8_t *data)
{
    const int x0 = T.x0 + 1, y0 = T.yt + 1, x1 = T.x1 - 1, yb0 = T.yb - 1;
    if (x1 < x0 || yb0 < y0) return;
    const int fw = (int)lv_lroundf(2 * 0.12f * LV_RD_F / LV_SEL_ZC);   /* 40: the face at the full close-up */
    const int wpx = T.x1 - T.x0; const bool one = wpx >= fw;        /* full close-up: exact 1:1 pixels */
    int ya = b_ylo(B, y0), yb = b_yhi(B, yb0); if (ya < LV_RD_Y0) ya = LV_RD_Y0; if (yb > LV_RD_Y1) yb = LV_RD_Y1;
    uint16_t row[87]; const uint16_t clear = 0x0821; int last = -1000;
    for (int y = ya; y <= yb; ++y) {
        const int sy = 46 + (one ? y - y0 : (y - y0) * fw / wpx); if (sy > 245) break;
        if (sy != last) { for (int k = 0; k < 87; ++k) row[k] = clear; LvBand src = {row, 16, sy, 87, 1}; lv_rd_info_rows(src, frame, data); last = sy; }
        const int xa = x0 > B.x0 ? x0 : B.x0, xe = x1 < B.x0 + B.w - 1 ? x1 : B.x0 + B.w - 1;
        for (int x = xa; x <= xe; ++x) { const int sx = 21 + (one ? x - x0 : (x - x0) * fw / wpx); if (sx > 102) break;
            const uint16_t c = row[sx - 16]; if (c != clear) b_pix(B, x, y, c); }
    }
}
static void lvi_icon(LvBand &B, int x, int y, int id, uint32_t c)
{ lvi_box(B, x, y, 8, 8, c); if (id == 0) { lvi_box(B, x + 2, y + 2, 4, 4, c); lvi_px(B, x + 3, y + 3, 0xffffff); } if (id == 1) { lvi_line(B, x + 1, y + 6, x + 3, y + 2, c); lvi_line(B, x + 3, y + 2, x + 6, y + 6, c); }
  if (id == 2) { lvi_box(B, x + 2, y + 2, 4, 4, c); lvi_line(B, x, y + 3, x + 7, y + 3, c); } if (id == 3) { lvi_line(B, x + 1, y + 1, x + 6, y + 1, c); lvi_line(B, x + 1, y + 1, x + 2, y + 6, c); lvi_line(B, x + 6, y + 1, x + 4, y + 6, c); } }
/* the four folders: tab, headline, line 2, line 3 (folder 4: headline + the status line; its bar is the live 0-11 signal / hunt state).
 * Sources: IDENTITY f.name / f.icon (lv_classify) / f.kind; HARDWARE f.mac / f.mfg (lv_mfg_name) / f.apple; RADIO f.kind / f.ch /
 * f.rssi (lv_rd_level) / lv_rd_hunting lv_rd_reads lv_rd_miss; SECURITY (WiFi) f.auth (lv_auth_name) or ACCESS (BLE) f.svc;
 * STATUS f.alert / lv_rd_watched(f.mac) / lv_rd_hunting. Nothing else is shown: LvFind has no timestamps, OUI or connectable flag. */
struct LvInfoTxt { char tab[24], l[3][40]; };
static void lv_rd_info_cats(const LvFind &f, LvInfoTxt *T)   /* name capped at 24 glyphs so it never runs under the folder-1 photo glyph at x209 */
{
    const bool wifi = f.kind == LV_K_WIFI; char mac[18]; lv_macstr(f.mac, mac); memset(T, 0, 4 * sizeof *T);
    snprintf(T[0].tab, 24, "IDENTITY"); snprintf(T[0].l[0], 40, "%.24s", f.name[0] ? f.name : wifi ? "(HIDDEN SSID)" : "(NO NAME)");
    snprintf(T[0].l[1], 40, "CLASS %s", LV_ICON_NAME[f.icon]); snprintf(T[0].l[2], 40, "%s", wifi ? "WIFI ACCESS POINT" : "BLE ADVERTISER");
    snprintf(T[1].tab, 24, "HARDWARE"); snprintf(T[1].l[0], 40, "%s", mac);
    if (wifi) snprintf(T[1].l[1], 40, "BSSID (AP RADIO MAC)");
    else { const char *mn = lv_mfg_name(f.mfg); if (mn) snprintf(T[1].l[1], 40, "VENDOR %s  ID %04X", mn, f.mfg); else if (f.mfg != 0xFFFF) snprintf(T[1].l[1], 40, "COMPANY ID %04X", f.mfg); else snprintf(T[1].l[1], 40, "NO MFG DATA");
           if (f.mfg == 0x004C && f.apple) snprintf(T[1].l[2], 40, "APPLE TYPE %02X%s", f.apple, f.apple == 0x12 ? "  FINDMY" : ""); }
    snprintf(T[2].tab, 24, "RADIO"); if (wifi) snprintf(T[2].l[0], 40, "WIFI  CH %d", f.ch); else snprintf(T[2].l[0], 40, "BLE");
    snprintf(T[2].l[1], 40, "%d DBM  STRENGTH %d/11", f.rssi, (lv_rd_level(f) - 4) * 11 / 56 + 1);
    if (lv_rd_hunting) snprintf(T[2].l[2], 40, lv_rd_miss ? "HUNT READ %d  HOLD X%d" : "HUNT READ %d  LIVE", lv_rd_reads, lv_rd_miss); else snprintf(T[2].l[2], 40, "LAST SCAN READING");
    snprintf(T[3].tab, 24, wifi ? "SECURITY / STATUS" : "ACCESS / STATUS");
    if (wifi) snprintf(T[3].l[0], 40, "AUTH %s", lv_auth_name(f.auth)); else if (f.svc) snprintf(T[3].l[0], 40, "SERVICE %04X", f.svc); else snprintf(T[3].l[0], 40, "NO WATCHED SERVICE UUID");
    { char *e = T[3].l[1]; int n = 0; const bool wat = lv_rd_watched(f.mac);
      if (f.alert) n += snprintf(e + n, 40 - n, "HUNT MATCH  ");
      if (lv_rd_hunting) n += snprintf(e + n, 40 - n, "HUNTING  ");
      n += snprintf(e + n, 40 - n, wat ? "WATCHED" : "NOT WATCHED"); }
}
static void lv_rd_info_open(int i)
{
    lv_rd_info = true; lv_rd_info_t = lv_now; lv_rd_info_f = lv_f[i];
    char l[26]; lv_label(lv_f[i], l, 20); lv_rd_log("INFO %s  FILE DROP", l);
}
static void lv_rd_info_close(void)
{
    lv_rd_info = false; lv_rd_swallow = false;
    const int i = lv_rd_find(lv_rd_lockmac); if (i >= 0) { lv_rd_sel = i; memcpy(lv_rd_selmac, lv_f[i].mac, 6); }   /* same tower, still locked */
    lv_rd_log("BACK TO GIBSON");
}
static int lv_rd_info_frame(void) { return (int)((uint32_t)(lv_now - lv_rd_info_t) * 3u / 100u); }   /* the mock's 30 fps clock */
static void lv_rd_info_draw(LvBand &B)
{
    { const int k = lv_rd_find(lv_rd_lockmac); if (k >= 0) lv_rd_info_f = lv_f[k]; }
    const LvFind &f = lv_rd_info_f; const int frame = lv_rd_info_frame(); const uint8_t *mac = f.mac; char mt[18]; lv_macstr(mac, mt);
    const bool wifi = f.kind == LV_K_WIFI;
    lvi_rect(B, 0, 0, 240, 320, 0x03050e);
    for (int y = 34; y < 265; y++) lvi_rect(B, 0, y, 240, 1, (y % 7 == 0) ? 0x080e20 : 0x040817);
    for (int j = 0; j < 4; j++) {
        int x = 124 + j * 28, y = 39 + j * 6, w = 22, h = 190 - j * 18; unsigned co = j % 2 ? 0x9b38c3 : 0x327bd2;
        lvi_rect(B, x - 1, y, 25, h, 0x101731); lvi_rect(B, x, y, w, h, 0x080c23);
        lvi_box(B, x, y, w, h, co); lvi_line(B, x + 1, y + 1, x + 1, y + h - 2, 0x1c3458); lvi_line(B, x + w - 2, y + 1, x + w - 2, y + h - 2, 0x162941);
        lvi_line(B, x + w - 1, y, x + w + 5, y - 4, co); lvi_line(B, x + w + 5, y - 4, x + w + 5, y + h - 8, co); lvi_line(B, x + w - 1, y + h - 1, x + w + 5, y + h - 8, co);
        for (int k = 1; k < 13; k++) { int ry = y + 9 + k * (h - 15) / 13; lvi_line(B, x + 2, ry, x + w - 2, ry, j % 2 ? 0x392344 : 0x163752); lvi_line(B, x + 2, ry + 1, x + w - 2, ry + 1, 0x0d192d);
            int dy = (229 - ry) * 6 / (x + w - 123 > 1 ? x + w - 123 : 1); lvi_line(B, x + w - 1, ry, x + w + 5, ry + (dy < 7 ? dy : 7), j % 2 ? 0x482755 : 0x1c4260); }
        lvi_micro(B, x + 3, y + 3, frame, 37 + j * 873, mac);
    }
    lvi_line(B, 109, 57, 177, 42, 0x265476); lvi_line(B, 114, 62, 224, 53, 0x353b72); lvi_line(B, 121, 147, 239, 100, 0x23284f);
    for (int x = 0; x < 240; x += 24) lvi_line(B, 123, 229, x, 264, 0x14334d);
    for (int y : {236, 241, 249, 262}) lvi_line(B, 0, y, 239, y, 0x123c51);
    for (int j = 0; j < 7; j++) { float t = ((frame * 2 + j * 29) % 180) / 180.f; int x = int((1 - t) * (j * 36) + t * 59), y = int(263 - 15 * t); lvi_rect(B, x, y, 3, 1, 0x2888a8); lvi_rect(B, x + 1, y - 1, 2, 1, 0x185c90); }
    int lit = 1 + frame / 11 < 11 ? 1 + frame / 11 : 11;   /* the monolith's bands: the mock's file-drop progress (decor, unchanged) */
    lv_rd_info_tower(B, frame, f, mac);
    lvi_rect(B, 78, 50, 158, 13, 0x05101c); lvi_box(B, 78, 50, 158, 13, frame % 30 < 15 ? 0xffb000 : 0x59cddc); lvi_line(B, 84, 64, 235, 64, 0x81efff); lvi_line(B, 84, 65, 84, 238, 0x2ca5b5); lvi_line(B, 86, 65, 86, 238, 0x124d61);
    int rad = 8 + (frame % 24); for (int a = 0; a < 360; a += 5) { double r = a * 3.14159265 / 180; lvi_px(B, 59 + int(cos(r) * rad), 249 + int(sin(r) * rad / 4), 0x266881); }
    for (int i = 0; i < 6; i++) { int x = 40 + i * 6, y = 249 - ((i + frame / 3) % 3); lvi_line(B, x, y, x + 3, y, 0x75c9ed); if (i < 5) lvi_line(B, x + 3, y, x + 3, y + 1, 0x428ea8); }
    LvInfoTxt T[4]; lv_rd_info_cats(f, T);
    static const unsigned neon[4] = {0x30e8ff, 0xff2ad4, 0xffb000, 0x30ff90};
    int ys[4], active[4]; float phase[4];
    for (int i = 0; i < 4; i++) { int t = frame - (18 + i * 21); active[i] = t >= 0; float u = t / 26.f; u = u < 0 ? 0 : u > 1 ? 1 : u; phase[i] = u;
        int start = i == 0 ? 62 : 78 + (i - 1) * 42 + 6, end = 78 + i * 42; float eased = 1.f - (1.f - u) * (1.f - u) * (1.f - u); ys[i] = start + int((end - start) * eased); }
    for (int i = 3; i >= 0; i--) if (active[i]) {
        int y = ys[i]; unsigned c = neon[i]; int x = 90; lvi_line(B, 84, y + 12, 89, y + 12, c);
        if (phase[i] >= 0.65f) lvi_rect(B, x + 3, y + 2, 145, 32, 0x061321);
        lvi_line(B, x + 145, y + 2, x + 148, y + 5, c); lvi_line(B, x + 148, y + 5, x + 148, y + 32, c); lvi_line(B, x + 3, y + 33, x + 148, y + 33, c);
        if (phase[i] >= 0.65f) { lvi_rect(B, x, y, 145, 32, 0x050c16); for (int yy = y + 2; yy < y + 31; yy += 4) lvi_rect(B, x + 1, yy, 143, 1, 0x081422); }
        if (phase[i] >= 0.65f) lvi_rect(B, x, y - 7, 88, 8, 0x050c16);
        lvi_line(B, x, y - 8, x + 83, y - 8, c); lvi_line(B, x + 83, y - 8, x + 88, y - 3, c); lvi_line(B, x + 88, y - 3, x + 88, y, c); lvi_line(B, x, y - 8, x, y + 31, c);
        lvi_line(B, x + 88, y, x + 144, y, c); lvi_line(B, x + 144, y, x + 144, y + 31, c); lvi_line(B, x, y + 31, x + 144, y + 31, c);
        if (phase[i] < 0.65f) continue;
        lvi_txt(B, x + 4, y - 6, T[i].tab, c); lvi_icon(B, x + 3, y + 4, i, c); lvi_txt(B, x + 14, y + 5, T[i].l[0], c);
        if (i == 0) { lvi_txt(B, x + 4, y + 16, T[0].l[1], 0x85a8bd); lvi_txt(B, x + 4, y + 24, T[0].l[2], 0x76b5c4);
            lvi_box(B, 209, y + 15, 22, 14, c); lvi_line(B, 211, y + 26, 217, y + 19, 0x427c83); lvi_line(B, 217, y + 19, 223, y + 25, 0x427c83); lvi_box(B, 222, y + 18, 5, 7, c); }
        if (i == 1) { lvi_txt(B, x + 4, y + 16, T[1].l[1], 0xb89ab6); lvi_txt(B, x + 4, y + 24, T[1].l[2], 0x9d73ad); }
        if (i == 2) { lvi_txt(B, x + 4, y + 16, T[2].l[1], 0xd5b879); lvi_txt(B, x + 4, y + 24, T[2].l[2], 0xb99968); }
        if (i == 3) {   /* same Nov 6 85x6 frame; fill = RADIO 0-11 strength from real dBm; label = dBm or hunt LIVE/HOLD/NO SIG */
            int sl = (lv_rd_level(f) - 4) * 11 / 56 + 1; if (sl < 0) sl = 0; if (sl > 11) sl = 11;
            lvi_box(B, x + 4, y + 16, 85, 6, c); lvi_rect(B, x + 6, y + 18, 81 * sl / 11, 2, c);
            char s[16];
            if (lv_rd_hunting) { if (lv_rd_nosig()) snprintf(s, sizeof s, "NO SIG"); else if (lv_rd_miss > 0) snprintf(s, sizeof s, "HOLD X%d", lv_rd_miss); else snprintf(s, sizeof s, "LIVE"); }
            else snprintf(s, sizeof s, "%d dBm", f.rssi);
            lvi_txt(B, x + 94, y + 17, s, c);
            lvi_txt(B, x + 4, y + 25, T[3].l[1], 0x81bfa4); }
    }
    { char t[32]; snprintf(t, sizeof t, "[NODE ..%s // OPEN]", mt + 9);
      lvi_rect(B, 78, 50, 158, 13, 0x05101c); lvi_box(B, 78, 50, 158, 13, frame % 30 < 15 ? 0xffb000 : 0x59cddc); lvi_txt(B, 82, 54, t, 0xffc34a); }
    /* top chrome (rows 0..23, rendered by lv_radar_frame + the ticker hook while INFO is open) */
    lvi_rect(B, 0, 0, 240, 24, 0x060913);
    for (int y = 0; y < 3; y++) for (int x = 0; x < 240; x++) lvi_px(B, x, y, ((x + y) / 5) & 1 ? 0xeb31ac : 0xdca137);
    for (int y = 21; y < 24; y++) for (int x = 0; x < 240; x++) lvi_px(B, x, y, ((x - y) / 5) & 1 ? 0xdca137 : 0xeb31ac);
    { char t[32], l[16]; snprintf(t, sizeof t, "LOCK ..%s %s", mt + 12, wifi ? "WIFI" : "BLE"); lvi_txt(B, 3, 5, t, 0xb9bdc9); lv_label(f, l, 13); lvi_txt(B, 3, 12, l, 0xf3baff, true); }
    { const int sl = (lv_rd_level(f) - 4) * 11 / 56 + 1, tl = lit < sl ? lit : sl;   /* real dBm, filling with the mock's clock */
      for (int i = 0; i < 11; i++) { int h = 5 + i; lvi_rect(B, 84 + i * 6, 20 - h, 5, h, i < tl ? lvi_band(i, frame) : 0x142637); } }
    { char t[12]; snprintf(t, sizeof t, "%ddB", f.rssi); lvi_txt(B, 183 - (int)strlen(t) * 6, 12, t, 0x80d6c9, true); }
    lvi_txt(B, 208, 5, "INFO", 0xffb000); lvi_txt(B, 208, 14, wifi ? "WIFI" : "BLE", 0x75d7e9);
    lvi_rect(B, 0, 24, 240, 10, 0x0b1826); lvi_txt(B, 3, 27, "GIBSON // NODE FILE DROP", 0x6fc9df); lvi_txt(B, 208, 27, wifi ? "WIFI" : "BLE", 0xe1ad45);
    /* bottom: the mock's two 115x22 buttons; left = BACK, right per LV_RD_INFO_RBTN; finger targets 120x28 */
    lvi_rect(B, 0, 265, 240, 55, 0x040912); lvi_rect(B, 3, 266, 115, 22, 0x071821); lvi_box(B, 3, 266, 115, 22, 0x30e8ff); lvi_rect(B, 122, 266, 115, 22, 0x071a16); lvi_box(B, 122, 266, 115, 22, 0x30ff90);
    { const char *l = "[ < BACK TO GIBSON ]";
      const char *r = LV_RD_INFO_RBTN == 0 ? (lv_rd_hunting ? "[ STOP HUNT ]" : "[ HUNT > ]") : LV_RD_INFO_RBTN == 1 ? (lv_rd_watched(f.mac) ? "[ UNWATCH ]" : "[ WATCH ]") : "";
      lvi_txt(B, 3 + (115 - lv_w35(l)) / 2, 275, l, 0x30e8ff); if (*r) lvi_txt(B, 122 + (115 - lv_w35(r)) / 2, 275, r, 0x30ff90); }
    lvi_box(B, 2, 290, 217, 29, 0x244457);   /* the mock's terminal: the radar's 3 real log lines, same rows + inks */
    { char t[LV_RD_TW + 1]; snprintf(t, sizeof t, "> %.48s", lv_rd_tl[0]); lvi_txt(B, 5, 294, t, 0x84c6ce);
      snprintf(t, sizeof t, "%.52s", lv_rd_tl[1]); lvi_txt(B, 5, 303, t, 0x58a6a9); snprintf(t, sizeof t, ">_ %.48s", lv_rd_tl[2]); lvi_txt(B, 5, 312, t, 0xd5a350); }
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);   /* the firmware's own corner BACKbot (the mock pasted a reference PNG here) */
}
static void lv_rd_info_scene_top(LvBand &B, void *) { lv_rd_info_draw(B); }
static bool lv_rd_info_on(void) { return lv_rd_info; }
#endif
#if LV_RD_FOLD
/* the three action folders: HACKbot DOWNLOAD's Joey folder frame (lv_dl_folder: tab, 3D lip, scanlines), same drop law */
static void lv_rd_fold1(LvBand &B, int j, int ys, float ph)
{
    static const unsigned C[3] = {0xff2ad4, 0xa0a0a8, 0xa0fc00};   /* the deck's HUNT / IGNORE / WATCH colours */
    const unsigned c = C[j]; const int x = 92, w = 143, y = ys + 8, h = 46;
    if (!b_yhit(B, y - 8, y + h + 1)) return;
    const int i = lv_rd_find(lv_rd_lockmac); const bool wat = lv_rd_watched(lv_rd_lockmac);
    if (ph >= 0.65f) lvi_rect(B, x + 3, y + 2, w, h, 0x061321);
    lvi_line(B, x + w, y + 2, x + w + 3, y + 5, c); lvi_line(B, x + w + 3, y + 5, x + w + 3, y + h + 1, c); lvi_line(B, x + 3, y + h + 1, x + w + 3, y + h + 1, c);
    if (ph >= 0.65f) { lvi_rect(B, x, y, w, h, j == 0 && lv_rd_hunting ? 0x200618 : 0x050c16); for (int yy = y + 2; yy < y + h - 1; yy += 4) lvi_rect(B, x + 1, yy, w - 2, 1, 0x081422);
                       lvi_rect(B, x, y - 7, 60, 8, 0x050c16); }
    lvi_line(B, x, y - 8, x + 55, y - 8, c); lvi_line(B, x + 55, y - 8, x + 60, y - 3, c); lvi_line(B, x + 60, y - 3, x + 60, y, c); lvi_line(B, x, y - 8, x, y + h - 1, c);
    lvi_line(B, x + 60, y, x + w - 1, y, c); lvi_line(B, x + w - 1, y, x + w - 1, y + h - 1, c); lvi_line(B, x, y + h - 1, x + w - 1, y + h - 1, c);
    if (ph < 0.65f) return;
    static const char *const TAB[3] = {"HUNT", "IGNORE", "WATCH"}; lvi_txt(B, x + 4, y - 6, TAB[j], c);
    char a[32], b[40], t[24];
    if (j == 0) {
        snprintf(a, sizeof a, "%s", lv_rd_hunting ? "STOP HUNT" : "HUNT THIS NODE");
        if (lv_rd_hunting && lv_rd_tgt_have) snprintf(b, sizeof b, "READ %d  %d DBM  %s", lv_rd_reads, lv_rd_tgt.rssi, lv_rd_miss ? "HOLD" : lv_rd_trend > 2 ? "WARMER" : lv_rd_trend < -2 ? "COLDER" : "STEADY");
        else snprintf(b, sizeof b, "%s", lv_rd_hunting ? "RESCANNING..." : "FOCUSED RE-SCANS, LIVE DBM");
        snprintf(t, sizeof t, "%s", lv_rd_hunting ? "TAP=STOP" : "TAP=HUNT");
    } else if (j == 1) { snprintf(a, sizeof a, "IGNORE NODE"); snprintf(b, sizeof b, "OFF RADAR + CYCLE  TTY: UNIGNORE"); snprintf(t, sizeof t, "TAP=IGNORE"); }
    else { snprintf(a, sizeof a, "%s", wat ? "UNWATCH NODE" : "WATCH NODE"); snprintf(b, sizeof b, "%s", wat ? "WATCHED: TOWER IS LIME" : "TOWER TURNS LIME, KEPT IN NVS"); snprintf(t, sizeof t, "%s", wat ? "TAP=UNWATCH" : "TAP=WATCH"); }
    lvi_icon(B, x + 4, y + 6, j == 1 ? 1 : j == 2 ? 2 : 0, c); lvi_txt(B, x + 16, y + 7, a, 0xffffff, true);
    lvi_txt(B, x + 4, y + 20, b, j == 0 && lv_rd_hunting ? c : 0x6a8090);
    /* bar: HUNT = the node's real signal level (lv_rd_level of the hunt read, else the scan read); IGNORE / WATCH = state */
    const int bw = 70, by = y + 34; int fill = 0;
    if (j == 0 && i >= 0) fill = (bw - 4) * lv_rd_level(lv_rd_hunting && lv_rd_tgt_have ? lv_rd_tgt : lv_f[i]) / 60;
    else if (j == 2 && wat) fill = bw - 4;
    lvi_box(B, x + 4, by, bw, 7, c); if (fill > 0) lvi_rect(B, x + 6, by + 2, fill, 3, c);
    lvi_txt(B, x + w - 4 - lv_w35(t), by + 1, t, c);
}
static void lv_rd_fold_draw(LvBand &B)
{
    if (!lv_rd_lock) return;
    const int el = (int)(lv_rd_fclk.vt - lv_rd_fold_t);
    for (int j = 2; j >= 0; --j) {   /* lv_select.h: the locked drop law */
        int y; float u; if (!lv_sel_drop(el, LV_RD_FDROP0, LV_RD_FGAP, LV_RD_FDROP, j, LV_RD_Y0 - 50, 42 + j * 66, y, u)) continue;
        lv_rd_fold1(B, j, y, u > 1 ? 1 : u);
    }
    if (lv_rd_fsel >= 0 && lv_rd_fsel < 3 && lv_sel_landed(el, LV_RD_FDROP0, LV_RD_FGAP, LV_RD_FDROP, lv_rd_fsel)) {   /* v1.2.14: d-pad folder cursor = the mock's theme frame */
        const LvRect r = LV_RD_FTAP[lv_rd_fsel]; const uint16_t k = lv_ink(LV_ACC);
        b_frame(B, r.x0 + 1, r.y0 + 1, r.x1 - r.x0 - 1, r.y1 - r.y0 - 1, k); b_frame(B, r.x0 + 2, r.y0 + 2, r.x1 - r.x0 - 3, r.y1 - r.y0 - 3, k); }
}
#endif
static void lv_radar_enter(void)
{
    lv_rd_lists_load();
    { int nw = 0; for (int i = 0; i < lv_nf; ++i) nw += lv_f[i].kind == LV_K_WIFI;
      lv_rd_log("GIBSON UP  %dW %dB  HEIGHT=DBM", nw, lv_nf - nw); lv_rd_log("ANGLE/DEPTH = MAC HASH, NOT DIRECTION"); lv_rd_log(lv_nf ? (LV_RD_CHARGE ? "TAP=CYCLE  HOLD CHARGE=LOCK" : "TAP=CYCLE  HOLD 1S=LOCK") : "NO NODES YET - SCANNING"); lv_rd_tl_t = lv_now; }
    lv_rd_t0 = lv_now; lv_rd_sel = -1; lv_rd_glow = false; lv_rd_holding = false; lv_rd_swallow = false;
    lv_rd_cam_hold = false;   /* v1.2.10: the GIBSON radar never parks its camera (only HACKbot's open tower does) */
#if LV_RD_FOLD
    lv_rd_fold_mode = 0; lv_rd_soff = 0; lv_rd_slast = lv_now; lv_sel_clk_reset(lv_rd_fclk, lv_now);
#endif
#if LV_RD_CHARGE
    lv_rd_cg = false; lv_rd_cg_owned = false; lv_rd_cg_dropping = false; lv_rd_cg_guard_until = 0;
#endif
    if (lv_nf == 0 && lv_job == LV_J_NONE) lv_job_start(lv_scanmix == 1 ? LV_J_WIFI : lv_scanmix == 2 ? LV_J_BLE : LV_J_ALL, -1);
}
static void lv_radar_leave(void) { if (lv_rd_hunting) lv_tk_off = 1 << 14; lv_rd_release(); lv_rd_sel = -1;
#if LV_RD_FOLD
    lv_rd_fold_mode = 0; lv_rd_cam_hold = false; lv_rd_soff = 0;
#endif
#if LV_RD_INFO
    lv_rd_info = false;
#endif
}
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
#if LV_RD_FOLD
    { uint32_t dt = lv_now - lv_rd_slast; lv_rd_slast = lv_now; if (dt > 250) dt = 250; lv_rd_soff += dt * (uint32_t)(lv_rd_speed - 1); }
    lv_rd_cam_hold = lv_rd_fold_mode == 1;   /* parked on the locked tower: no sway, no clamp push (as DOWNLOAD) */
    { const int fel = (int)(lv_rd_fclk.vt - lv_rd_fold_t);   /* lv_select.h clock: cap only while the pull-up / drop plays */
      lv_sel_clk_step(lv_rd_fclk, lv_now, lv_rd_fold_mode == 2 || (lv_rd_fold_mode == 1 && fel < LV_RD_FCAM_MS) ? 50u : lv_rd_fold_mode == 1 && fel < LV_RD_FDROP0 + 2 * LV_RD_FGAP + LV_RD_FDROP ? 100u : LV_SEL_NOCAP); }
    /* v1.2.13: the locked pull-up. Open: the camera parks where it was (no pan) and the locked tower flies in to the close-up
     * (lv_sel_pull, eased over LV_RD_FCAM_MS); close: it flies back out while the camera eases back to the drift. */
    { const float fel = (float)(int)(lv_rd_fclk.vt - lv_rd_fold_t);
      lv_rd_pull_scr = LV_SC_RADAR; memcpy(lv_rd_pull_mac, lv_rd_fold_mac, 6);
      lv_rd_pull_p = lv_rd_fold_mode == 1 ? lv_sel_ease(fel / LV_RD_FCAM_MS) : lv_rd_fold_mode == 2 ? lv_rd_fold_p0 * (1.0f - lv_sel_ease(fel / LV_RD_FCAM_MS)) : 0.0f; }
#endif
    lv_rd_layout();
#if LV_RD_FOLD
    if (lv_rd_fold_mode) {
        const float fel = (float)(int)(lv_rd_fclk.vt - lv_rd_fold_t), u = fel / LV_RD_FCAM_MS;
        const float drift = lv_rd_camx;
        lv_rd_camx = lv_rd_fold_mode == 1 ? lv_rd_fold_from : lv_sel_cam(lv_rd_fold_from, drift, fel, LV_RD_FCAM_MS);
        lv_rd_place(); if (lv_rd_fold_mode == 2) { lv_rd_clamp(); if (u >= 1) { lv_rd_fold_mode = 0; lv_rd_pull_p = 0; } }
    }
#endif
    lv_render(lv_blit_dirty, LV_R_RADAR, lv_scene_radar, 0);   /* v1.2.17: changed rows only */
#if LV_RD_INFO
    if (lv_rd_info) lv_render(lv_blit, LvRect{0, 0, 239, 23}, lv_rd_info_scene_top, 0);   /* the mock's top chrome, rim included */
#endif
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
#if LV_RD_PAD
    if (y >= PAD_Y0) { lv_rd_pad_touch(down, x, y); return; }
#endif
#if LV_RD_INFO
    if (lv_rd_info) return;
#endif
#if LV_RD_CHARGE && !LV_RD_CG_NODRAW   /* hold the CHARGE button (not the tower): 1 s to full, then the lock fires; let go early and it drains */
    const bool show = lv_rd_cg_show();
    if (lv_rd_cg) {
        if (!down) {   /* c) resistive dropout: hold the charge through a brief (<=140 ms) lost-contact blip */
            if (!lv_rd_cg_dropping) { lv_rd_cg_dropping = true; lv_rd_cg_drop_t = lv_now; }
            if ((int32_t)(lv_now - lv_rd_cg_drop_t) <= 140) return;
            lv_rd_cg_rel = lv_rd_cg_level(); lv_rd_cg_rel_t = lv_now; lv_rd_cg = false; lv_rd_cg_dropping = false; lv_rd_cg_guard_until = lv_now + 350;
            return;
        }
        lv_rd_cg_dropping = false;
        /* b) wide keep zone: the finger may wobble within ARM padded by 24 px without cancelling the charge */
        if (!(show && lv_rd_in_pad(LV_RD_CG_ARM, x, y, 24))) { lv_rd_cg_rel = lv_rd_cg_level(); lv_rd_cg_rel_t = lv_now; lv_rd_cg = false; lv_rd_cg_guard_until = lv_now + 350; return; }
        if ((int32_t)(lv_now - lv_rd_cg_t) >= LV_RD_CG_MS) { lv_rd_cg = false; lv_rd_cg_rel = 0; lv_rd_cg_guard_until = lv_now + 350; lv_rd_lock_on(lv_rd_sel); lv_rd_swallow = true; }
        return;
    }
    if (!(down && show && lv_in(LV_RD_CG_ARM, x, y))) return;   /* a) arm only from inside the (widened) HOLD TO LOCK zone */
    { const float l0 = lv_rd_cg_level(); lv_rd_cg = true; lv_rd_cg_t = lv_now - (uint32_t)(l0 * LV_RD_CG_MS); lv_rd_cg_owned = true; lv_rd_cg_dropping = false; }
    if ((int32_t)(lv_now - lv_rd_cg_t) >= LV_RD_CG_MS) { lv_rd_cg = false; lv_rd_cg_rel = 0; lv_rd_cg_guard_until = lv_now + 350; lv_rd_lock_on(lv_rd_sel); lv_rd_swallow = true; }
    return;
#endif
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
/* v1.2.14 (Mobandy 11:34 AM ET "just going side to side is slow. up and down cycle the rows"): d-pad pick on the placed
 * towers. Row = the grid depth row of the tower's locked home depth (floor of lv_rd_place's hash Z, 1.6..4.8 -> rows 1..4);
 * order in a row = its locked home X. dir 0 L / 1 R: the next tower left / right in the same row (wraps in the row);
 * 2 UP: the next farther row, 3 DOWN: the next nearer row (wrap), landing on the tower nearest in X. Nothing selected:
 * the nearest row's tower closest to the centre. Ignored towers skipped. Pure read: nothing about the scene changes. */
static int lv_rd_dpad_pick(int cur, int dir)
{
    int ci[LV_MAXF], cr[LV_MAXF]; float cx[LV_MAXF]; int n = 0, p = -1;
    for (int k = 0; k < lv_rd_n; ++k) { const int i = lv_rd_tw[k].i; if (i < 0 || i >= lv_nf || lv_rd_ignored(lv_f[i].mac)) continue;
        const uint32_t h = lv_rd_hash(lv_f[i].mac); const float Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f;
        ci[n] = i; cr[n] = (int)floorf(Z); cx[n] = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * Z + i * 1e-4f; if (i == cur) p = n; ++n; }
    if (!n) return -1;
    if (p < 0) { int b = 0; for (int k = 1; k < n; ++k) if (cr[k] < cr[b] || (cr[k] == cr[b] && fabsf(cx[k]) < fabsf(cx[b]))) b = k; return ci[b]; }
    if (dir <= 1) {   /* within the row */
        int b = -1, w = -1;
        for (int k = 0; k < n; ++k) { if (cr[k] != cr[p] || k == p) continue;
            if (dir == 1) { if (cx[k] > cx[p] && (b < 0 || cx[k] < cx[b])) b = k; if (w < 0 || cx[k] < cx[w]) w = k; }
            else { if (cx[k] < cx[p] && (b < 0 || cx[k] > cx[b])) b = k; if (w < 0 || cx[k] > cx[w]) w = k; } }
        return b >= 0 ? ci[b] : w >= 0 ? ci[w] : cur;
    }
    int tr = -1;   /* the next row up (farther) or down (nearer) that has a tower, wrapping */
    for (int k = 0; k < n; ++k) { if (cr[k] == cr[p]) continue;
        const int d = dir == 2 ? (cr[k] - cr[p] + 64) % 64 : (cr[p] - cr[k] + 64) % 64, dt = tr < 0 ? 99 : dir == 2 ? (tr - cr[p] + 64) % 64 : (cr[p] - tr + 64) % 64;
        if (d < dt) tr = cr[k]; }
    if (tr < 0) return cur;
    int b = -1; for (int k = 0; k < n; ++k) if (cr[k] == tr && (b < 0 || fabsf(cx[k] - cx[p]) < fabsf(cx[b] - cx[p]))) b = k;
    return ci[b];
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
#if LV_RD_INFO
    if (lv_rd_info) {   /* INFO view: only BACK (and the corner BACKbot, via lv_back) do anything */
        if (lv_in(LV_RD_INFO_BTN_L, x, y)) { lv_rd_info_close(); return "radar-info-back"; }
        if (lv_in(LV_RD_INFO_BTN_R, x, y) && LV_RD_INFO_RBTN != 2) {
            if (LV_RD_INFO_RBTN == 0) { const int i = lv_rd_find(lv_rd_lockmac); lv_rd_info_close(); if (lv_rd_hunting) { lv_rd_hunt_stop(); lv_rd_log("HUNT STOPPED  %d READS", lv_rd_reads); return "radar-info-hunt-stop"; } lv_rd_hunt_go(i); return "radar-info-hunt"; }
            const int w = lv_rd_lfind(lv_rd_wat, lv_rd_nwat, lv_rd_lockmac); if (w >= 0) lv_rd_ldel(lv_rd_wat, lv_rd_nwat, w); else lv_rd_ladd(lv_rd_wat, lv_rd_nwat, lv_rd_lockmac);
            lv_rd_lists_save(false); return w >= 0 ? "radar-info-unwatch" : "radar-info-watch";
        }
        return "radar-info";
    }
#endif
    if (lv_rd_lock) {
#if LV_RD_FOLD
        int fk = -1;   /* folders HUNT / IGNORE / WATCH = buttons k 2 / 0 / 1; the deck's centre stack is the SPEED button now */
        if (lv_rd_fold_mode == 1) { const int j = lv_sel_hit(LV_RD_FTAP, 3, x, y);
            if (j >= 0) { if (!lv_sel_landed((int)(lv_sel_clk_now(lv_rd_fclk, lv_now) - lv_rd_fold_t), LV_RD_FDROP0, LV_RD_FGAP, LV_RD_FDROP, j)) return "radar-folder-falling";
                static const int K[3] = {2, 0, 1}; fk = K[j]; lv_press(LV_RD_FTAP[j]); } }
        for (int k = 0; k < LV_RD_NBTN; ++k) if (k == fk) {
#else
        for (int k = 0; k < LV_RD_NBTN; ++k) if (lv_in(LV_RD_BTNT[k], x, y)) {
#endif
            const int i = lv_rd_find(lv_rd_lockmac);
#if LV_RD_INFO
            if (k == 3) { if (i >= 0) lv_rd_info_open(i); return "radar-info-open"; }   /* no lv_press: the INFO screen is the feedback (the flash would ghost over its first 3 frames) */
#endif
#if !LV_RD_FOLD
            lv_press(LV_RD_BTNR[k]);
#endif
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
#if LV_RD_PAD
        if (y >= PAD_Y0) {
            const char *a = lv_rd_pad_tap(x, y);
            if (a && strcmp(a, "radar-term")) return a;
            return "radar-panel";
        }
#endif
        if (y >= LV_RD_PTOP && y <= 285) return "radar-panel";   /* the panel itself does not release the lock */
        const int i = lv_rd_hit(x, y);
        if (i >= 0 && !memcmp(lv_f[i].mac, lv_rd_lockmac, 6)) return "radar-locked";
        lv_rd_release(); lv_rd_log("RELEASED"); return "radar-unlock";   /* a tap anywhere else releases the lock (and ends a hunt) */
    }
#if LV_RD_PAD
    if (y >= PAD_Y0 && !(LV_RD_INFO && lv_rd_info)) return lv_rd_pad_tap(x, y);
#endif
#if LV_RD_CHARGE && !LV_RD_CG_NODRAW
    /* item 1: touch-owner latch + 350 ms post-release guard + widened arm zone -> a hold / abort / drift / release NEVER cycles or re-picks a tower */
    if (lv_rd_cg_owned || (int32_t)(lv_now - lv_rd_cg_guard_until) < 0 || (lv_rd_cg_show() && lv_in(LV_RD_CG_ARM, x, y))) { lv_rd_cg_owned = false; return "radar-charge-short"; }
#endif
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
