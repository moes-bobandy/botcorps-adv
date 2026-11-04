"""MOCK radar-charge design D4 = D3 made thicker, a little brighter and CALMER (owner review of c17bc28, 2026-11-04 12:40 PM ET:
"Make the current thicker and brighter" + "Make it less aggressive though" + "It can't over power the top meter").
Same grid route as D3. 2 px ice core in a 3-step blue glow band, steady (only a slow 2.4 s breathing), evenly spaced smooth packets at
0.2 px/ms every 40 px, about half of D3's crackle (small, no forks, re-rolled every 4 frames), steady charge tip, soft contact glow and
soft panel port. No pure white: the brightest current pixel stays below the top banner's yellow text. Wire through glass at 13/16.
No glyphs. Applies on top of apply_charge.py to a SCRATCH copy of src/live/lv_radar.h @3240b03; never the firmware.
usage: python3 apply_charge_D4.py <scratch>/src/live/lv_radar.h   (run AFTER apply_charge.py)"""
import sys
p = sys.argv[1]; s = open(p).read()
def rep(old, new):
    global s
    assert s.count(old) == 1, (s.count(old), old[:70]); s = s.replace(old, new)

# D uses A's button rect + A's button drawing, byte for byte
rep("#if LV_RD_CHARGE == 1\nstatic const LvRect LV_RD_CG_BTN", "#if LV_RD_CHARGE == 1 || LV_RD_CHARGE == 4\nstatic const LvRect LV_RD_CG_BTN")
rep("#if LV_RD_CHARGE == 1   /* A: a bar button", "#if LV_RD_CHARGE == 1 || LV_RD_CHARGE == 4   /* A (and D): a bar button")
# D: after the lock the stream keeps flowing (no hold/fade-out)
rep("    if (lv_rd_lock) { const int e = (int)(lv_now - lv_rd_lock_t); if (e >= 600) return;",
    "    if (lv_rd_lock && LV_RD_CHARGE == 4) alpha = 1.0f;   /* D: keeps flowing while locked / hunting */\n"
    "    else if (lv_rd_lock) { const int e = (int)(lv_now - lv_rd_lock_t); if (e >= 600) return;")
rep("#if LV_RD_CHARGE == 1\n        if (lv_rd_route(LV_RD_CG_BTN.x0 - 1, 255, 0, h1, h2))",
    "#if LV_RD_CHARGE == 4\n        if (lv_rd_route(LV_RD_CG_BTN.x0 - 1, 255, 0, h1, h2)) { lv_rd_cg_logpath(0); lv_rd_dstream(B, lvl); }\n"
    "#elif LV_RD_CHARGE == 1\n        if (lv_rd_route(LV_RD_CG_BTN.x0 - 1, 255, 0, h1, h2))")

DCODE = r'''
#if LV_RD_CHARGE == 4
/* D2 fix 1: occlusion. Every other tower's silhouette = convex hull of its 8 box corners, rebuilt per frame. A tower hides a stream
 * pixel when the pixel is inside its hull and the tower's front base (yb) is at or below the stream element's ground row (= nearer). */
struct LvRdHull { int n, xa, xb, ya, yb, gyb, ns; float x[8], y[8], s[10][4]; };   /* s: the tower's drawn edge segments */
static LvRdHull lv_rd_hull[LV_MAXF]; static int lv_rd_nh = 0; static int lv_rd_gy = 0; static int lv_rd_glass = 1;   /* 0: element hidden entirely behind a nearer tower (no see-through) */
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
        LvRdHull &H = lv_rd_hull[lv_rd_nh++]; H.n = m; H.xa = 9999; H.xb = -9999; H.ya = 9999; H.yb = -9999; H.gyb = T.yb;
        for (int k = 0; k < m; ++k) { H.x[k] = px[h[k]]; H.y[k] = py[h[k]];
            const int xi = (int)H.x[k], yi = (int)H.y[k]; if (xi < H.xa) H.xa = xi; if (xi > H.xb) H.xb = xi; if (yi < H.ya) H.ya = yi; if (yi > H.yb) H.yb = yi; }
        const float S[10][4] = { {(float)T.x0, (float)T.yt, (float)T.x0, (float)T.yb}, {(float)T.x1, (float)T.yt, (float)T.x1, (float)T.yb},
            {(float)T.x0, (float)T.yt, (float)T.x1, (float)T.yt}, {(float)T.x0, (float)T.yb, (float)T.x1, (float)T.yb},
            {(float)T.bx0, (float)T.byt, (float)T.bx1, (float)T.byt}, {(float)T.bx0, (float)T.byt, (float)T.x0, (float)T.yt},
            {(float)T.bx1, (float)T.byt, (float)T.x1, (float)T.yt}, {(float)T.bx1, (float)T.byt, (float)T.bx1, (float)T.byb},
            {(float)T.bx0, (float)T.byt, (float)T.bx0, (float)T.byb}, {(float)T.bx0, (float)T.byt, (float)T.bx0, (float)T.byt} };
        H.ns = 0; for (int k = 0; k < 10; ++k) { if (k == 7 && !(T.bx1 > T.x1)) continue; if (k == 8 && !(T.bx0 < T.x0)) continue; if (k == 9) continue;
            for (int c = 0; c < 4; ++c) H.s[H.ns][c] = S[k][c]; ++H.ns; }
    }
}
static float lv_rd_segd2(const float *g, float x, float y)
{
    const float ex = g[2] - g[0], ey = g[3] - g[1], l2 = ex * ex + ey * ey; float u = l2 > 0 ? ((x - g[0]) * ex + (y - g[1]) * ey) / l2 : 0;
    if (u < 0) u = 0; if (u > 1) u = 1; const float dx = g[0] + u * ex - x, dy = g[1] + u * ey - y; return dx * dx + dy * dy;
}
/* 0 = free; 1 = behind a nearer tower's translucent face (drawn dim, as if through its glass); 2 = on that tower's edge line (skip) */
static int lv_rd_occ(int x, int y)
{
    int r = 0;
    for (int q = 0; q < lv_rd_nh; ++q) {
        const LvRdHull &H = lv_rd_hull[q];
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
    if (k <= 0 || y < LV_RD_Y0 || y > ymax) return; if (k > n) k = n;
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
    const int front = lv_rd_lock ? n - 1 : (int)lroundf(lvl * (n - 1));
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
        int pi = -1; for (int i = 0; i < n; ++i) if (lv_rd_py[i] == 247) { pi = i; break; }
        if (pi >= 0) {
            const int x = lv_rd_px[pi]; const int pulse = 6 + br;          /* 4..8, slow */
            lv_rd_gy = 0;
            for (int dy = -5; dy <= -1; ++dy) for (int dx = -4; dx <= 5; ++dx) { const int cx = dx > 0 ? dx - 1 : dx, d2 = cx * cx + dy * dy; if (d2 > 20) continue;
                lv_rd_bly(B, x + dx, 247 + dy, ELEC, (20 - d2) * pulse / 30 + 1, 16, 246); }
            lv_rd_gy = 1 << 30;
            for (int dx = -3; dx <= 4; ++dx) { const int a = dx <= 0 ? -dx : dx - 1;
                lv_rd_bly(B, x + dx, 247, a == 0 ? PK : a == 1 ? ICE2 : ELEC, a == 0 ? 12 : a == 1 ? 10 : 9 - a * 2, 16, 248); }
        }
    }
}
#endif
'''
rep("static void lv_rd_cg_log(const char *tag", DCODE + "static void lv_rd_cg_log(const char *tag")
open(p, 'w').write(s); print("patched D4", p)
