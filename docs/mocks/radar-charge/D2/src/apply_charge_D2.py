"""MOCK radar-charge design D2 = D + four fixes (owner review of c214e09, 2026-11-04 12:06 PM ET: "Fix the weak spots and show me again"):
 1. the stream passes BEHIND nearer towers: their edge lines are never touched and through their translucent faces it shows dimmed
    (8/16), like everything else behind the radar's glass towers;
 2. trail glyphs stay white / ice-white, the run fades through a shrinking halo instead of greying the faces;
 3. real stepped crackle arcs (axis-aligned multi-segment, with forks) hugging the path, re-rolled every frame;
 4. after the lock a glowing contact port on the panel's top frame where the stream leaves the control panel.
Base design D (owner 2026-11-04 8:41 AM ET): "Make it Gibson only rain and use the bar button ui keep the stream
constantly flowing as it's being hunted. Make the Gibson bot electrical stream look electrical."
Applies on top of apply_charge.py (A/B/C, unchanged) to a SCRATCH copy of src/live/lv_radar.h @3240b03; never the firmware.
LV_RD_CHARGE=4 -> D: A's HOLD TO LOCK bar button (x166..234 y250..260, identical drawing) + C's route (grid row -> up a real floor
line -> tower base row) carrying GIBSON-only rain (LOCKED ui_main_bot_rain GIBSON glyph, white, lead 100 %, trail 92/90/88/86 %)
made electric: hot lead, flicker, crackle arcs hugging the path, an electric-blue glow halo. After the lock the stream never fades:
it keeps flowing to the locked tower through LOCK, IGNORE / WATCH / HUNT, and while hunting. LV_RD_CHARGE 0..3 unchanged.
usage: python3 apply_charge_D2.py <scratch>/src/live/lv_radar.h   (run AFTER apply_charge.py)"""
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
/* GIBSON rain glyph (LOCKED ui_main_bot_rain: mbr.GIB_ART = blank row + src/ui/botcorps_sprite.cpp SPR_ROUND; a = white, e = black eyes) */
static const char LV_RD_GIB[8][9] = { "........", "....a...", "..aaaaa.", ".aaaaaaa", ".aeeaeea", ".aeeaeea", ".aaaaaaa", "..aaaaa." };
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
#define LV_RD_GLASS 8   /* /16: what of the stream shows through a nearer tower's face */
static inline void lv_rd_bly(LvBand &B, int x, int y, uint16_t c, int k, int n, int ymax)   /* fine blend, clipped, occluded */
{
    if (k <= 0 || y < LV_RD_Y0 || y > ymax) return; if (k > n) k = n;
    if (x < B.x0 || x >= B.x0 + B.w || y < B.y0 || y >= B.y0 + B.h) return;
    const int o = lv_rd_occ(x, y); if (o == 2) return; if (o == 1) { if (!lv_rd_glass) return; k = k * LV_RD_GLASS * 2 / n; n = 32; if (k <= 0) return; }
    uint16_t &q = B.px[(y - B.y0) * B.w + (x - B.x0)]; q = lv_mix(q, c, k, n);
}
static inline void lv_rd_bl(LvBand &B, int x, int y, uint16_t c, int k, int n) { lv_rd_bly(B, x, y, c, k, n, LV_RD_CG_YMAX); }
static bool lv_rd_nearp(int x, int y, int j)   /* within 7 px (Chebyshev) of the route near index j: keeps every arc pixel in the mask */
{
    for (int m = j - 16; m <= j + 16; ++m) { if (m < 0 || m >= lv_rd_np) continue;
        const int ex = x > lv_rd_px[m] ? x - lv_rd_px[m] : lv_rd_px[m] - x, ey = y > lv_rd_py[m] ? y - lv_rd_py[m] : lv_rd_py[m] - y;
        if (ex <= 7 && ey <= 7) return true; }
    return false;
}
/* D2 fix 3: one crackle = a short stepped (axis-aligned) arc: out from the wire, along it, kink, along, sometimes a fork.
 * <= 6 px off the route, ~6..14 px long: reads as an electric discharge on the wire, never as a bolt across the grid. */
static int lv_rd_gbx[64], lv_rd_gby[64], lv_rd_ngb = 0;   /* lit glyph centres: arcs never paint over a glyph face */
static void lv_rd_arc(LvBand &B, int j, uint32_t r, int ymax, int kmax)
{
    const uint16_t W = lv_hex(0xffffff), ICE = lv_hex(0xd2f4ff), ELEC = lv_hex(0x3aa8ff);
    const int n = lv_rd_np; if (j < 0 || j >= n) return;
    const int ja = j - 3 < 0 ? 0 : j - 3, jb = j + 3 >= n ? n - 1 : j + 3;
    const int tx = lv_rd_px[jb] - lv_rd_px[ja], ty = lv_rd_py[jb] - lv_rd_py[ja];
    const bool horiz = (tx < 0 ? -tx : tx) >= (ty < 0 ? -ty : ty);
    const int ux = horiz ? 1 : 0, uy = horiz ? 0 : 1, nx = horiz ? 0 : 1, ny = horiz ? 1 : 0;   /* along / across the wire */
    const int side = (r & 1) ? 1 : -1; int dir = (r & 2) ? 1 : -1;
    lv_rd_gy = lv_rd_py[j];
    int x = lv_rd_px[j], y = lv_rd_py[j], off = 0, t = 0;
    auto put = [&](int px, int py, int tt, int kk) {
        if (!lv_rd_nearp(px, py, j)) return;
        for (int g = 0; g < lv_rd_ngb; ++g) if (px >= lv_rd_gbx[g] - 4 && px <= lv_rd_gbx[g] + 3 && py >= lv_rd_gby[g] - 4 && py <= lv_rd_gby[g] + 3) return;
        const uint16_t c = tt == 0 ? W : tt < 3 ? ICE : (tt & 1 ? ICE : W);
        lv_rd_bly(B, px, py, c, kk, 16, ymax);
        for (int d = 0; d < 4; ++d) { const int gx = px + (d == 0) - (d == 1), gy = py + (d == 2) - (d == 3); if (lv_rd_nearp(gx, gy, j)) lv_rd_bly(B, gx, gy, ELEC, 4, 16, ymax); }
    };
    auto run = [&](int ax, int ay, int len, int kb) {   /* one straight step, axis-aligned */
        for (int s = 0; s < len; ++s) { x += ax; y += ay; ++t; if (ax * nx + ay * ny) off += (ax * nx + ay * ny) * side > 0 ? 1 : -1; const int k = kb - t; put(x, y, t, k < 7 ? 7 : k); }
    };
    const int k0 = kmax;
    run(nx * side, ny * side, 2 + (int)((r >> 4) % 4), k0);                   /* out from the wire (2-5: clears a glyph's side) */
    run(ux * dir, uy * dir, 2 + (int)((r >> 5) % 3), k0);                     /* along it */
    const int fx = x, fy = y, ft = t, foff = off;
    const bool outk = off < 5 && ((r >> 7) & 1);
    run(nx * side * (outk ? 1 : -1), ny * side * (outk ? 1 : -1), 1 + (int)((r >> 8) % 2), k0);   /* kink */
    if ((r >> 9) % 4 == 0) dir = -dir;
    run(ux * dir, uy * dir, 1 + (int)((r >> 10) % 3), k0);                    /* along again */
    if ((r >> 12) & 1) run(nx * side * (off > 1 ? -1 : 1), ny * side * (off > 1 ? -1 : 1), 1, k0);   /* tail */
    if ((r >> 13) % 3 == 0 && foff < 5) {                                     /* fork off the first corner */
        x = fx; y = fy; t = ft + 1; off = foff;
        run(nx * side, ny * side, 1 + (int)((r >> 15) % 2), k0 - 3);
        run(-ux * dir, -uy * dir, 1 + (int)((r >> 16) % 2), k0 - 3);
    }
}
static void lv_rd_dstream(LvBand &B, float lvl)
{
    const uint16_t W = lv_hex(0xffffff), ICE = lv_hex(0xd2f4ff), ELEC = lv_hex(0x3aa8ff), VIO = lv_hex(0x7a1cff), BLK = lv_hex(0x000000);
    const uint16_t ICEW = lv_hex(0xf0fbff);   /* fix 2: trail faces stay ice-white (no grey levels) */
    const int n = lv_rd_np; if (n < 2) return;
    const int front = lv_rd_lock ? n - 1 : (int)lroundf(lvl * (n - 1));
    lv_rd_hulls(lv_rd_sel);
    const uint32_t fr = lv_now / 30;                       /* one sim frame: flicker / crackle re-roll every frame */
    const int cell = 9, step = (int)(lv_now / 60), P = 8;  /* rain cells 9 px apart along the route, one cell per 60 ms */
    /* 1. the conduit: the floor line it rides. Energized part = a flickering ice-white wire in a blue glow; ahead = faint violet dots */
    for (int i = 0; i < n; ++i) {
        const int x = lv_rd_px[i], y = lv_rd_py[i]; lv_rd_gy = y;
        if (i > front) { if (i & 1) lv_rd_bl(B, x, y, VIO, 3, 8); continue; }
        const uint32_t h = lv_rd_ch(fr, i * 7 + 1); const int k = 7 + (int)(h % 7);   /* 7..13 / 16 brightness jitter */
        lv_rd_bl(B, x, y, ICE, k, 16);
        const int kg = 4 + (int)((h >> 8) % 3);
        lv_rd_bl(B, x - 1, y, ELEC, kg, 16); lv_rd_bl(B, x + 1, y, ELEC, kg, 16); lv_rd_bl(B, x, y - 1, ELEC, kg, 16); lv_rd_bl(B, x, y + 1, ELEC, kg, 16);
    }
    /* 1b. surges: bright packets racing up the wire (0.4 px/ms, every 23 px), so the current visibly flows between rain steps */
    { const int ph = (int)(lv_now * 4 / 10 % 23);
      for (int i = ph; i <= front && i < n; i += 23) for (int t = 0; t < 4; ++t) { const int j = i - t; if (j < 0) break; lv_rd_gy = lv_rd_py[j];
          lv_rd_bl(B, lv_rd_px[j], lv_rd_py[j], t ? ICE : W, 16 - t * 3, 16); } }
    /* 2. GIBSON rain flowing along it: sparse runs (3-5 lit per 8-cell cycle) racing to the tower */
    int lit[64], litrel[64], nl = 0;
    for (int c = 0; c * cell < n && nl < 64; ++c) {
        const int i = c * cell; if (i > front) break;
        const int rel = ((step - c) % P + P) % P, run = 3 + (int)(lv_rd_ch((step - c + 800) / P, 5) % 3);
        if (rel >= run) continue;
        lit[nl] = c; litrel[nl++] = rel;
    }
    /* 2a. glow halo under every glyph: fix 2 - the run fades by its halo (lead big + white-hot, then smaller / fainter), not its faces */
    static const int HR2[5] = {46, 32, 26, 20, 16}, HG[5] = {20, 13, 10, 8, 6};
    for (int q = 0; q < nl; ++q) {
        const int i = lit[q] * cell, x = lv_rd_px[i], y = lv_rd_py[i], rel = litrel[q] < 5 ? litrel[q] : 4; const bool lead = rel == 0;
        const uint32_t h = lv_rd_ch(fr, lit[q] * 31 + 5); const int fl = 5 + (int)(h % 4);   /* 5..8: the halo flickers */
        const int R2 = HR2[rel]; lv_rd_gy = y;
        for (int dy = -7; dy <= 7; ++dy) for (int dx = -7; dx <= 7; ++dx) {
            const int d2 = dx * dx + (2 * dy - 1) * (2 * dy - 1) / 4; if (d2 > R2) continue;   /* centred on the glyph body (y+0.5) */
            const int k = (R2 - d2) * fl * HG[rel] / (R2 * 10) + (lead ? 2 : 1);
            lv_rd_bl(B, x + dx, y + dy, lead && d2 < 14 ? ICE : ELEC, k, 16);
        }
    }
    /* 2b. the glyphs: lead #FFFFFF, trail ice-white #F0FBFF, black eyes; behind a nearer tower only the halo shows through its glass */
    for (int q = 0; q < nl; ++q) {
        const int c = lit[q], rel = litrel[q], i = c * cell, x = lv_rd_px[i], y = lv_rd_py[i]; const bool lead = rel == 0;
        const uint32_t h = lv_rd_ch(fr, c * 13 + 3); const uint16_t col = lead ? W : ICEW; lv_rd_gy = y;
        for (int yy = 0; yy < 8; ++yy) for (int xx = 0; xx < 8; ++xx) {
            const char ch = LV_RD_GIB[yy][xx]; if (ch == '.') continue;
            const int gx = x - 4 + xx, gy = y - 4 + yy; if (gy < LV_RD_Y0 || gy > LV_RD_CG_YMAX) continue;
            const int o = lv_rd_occ(gx, gy); if (o == 2) continue;
            if (o == 0) b_pix(B, gx, gy, ch == 'e' ? BLK : col);   /* faces never show through a nearer tower: only their halo glows through */
        }
        if (lead) { const int ax = x, ay = y - 4; lv_rd_bl(B, ax, ay - 1, ICE, (int)(10 + h % 6), 16); }   /* antenna spark */
    }
    /* 3. crackle: stepped arcs jumping off the wire and out from behind the glyphs (never over a face), re-rolled every frame */
    if (front >= 8) {
        const int na = 4 + nl;
        lv_rd_ngb = nl; for (int q = 0; q < nl; ++q) { lv_rd_gbx[q] = lv_rd_px[lit[q] * cell]; lv_rd_gby[q] = lv_rd_py[lit[q] * cell]; }
        for (int a = 0; a < na; ++a) {
            int j = -1;
            for (int tr = 0; tr < 1 && j < 0; ++tr) {
                const uint32_t r = lv_rd_ch(fr * 7 + 3, a * 977 + 11 + tr * 131);
                const int jj = 3 + (int)(r % (uint32_t)(front - 5 > 1 ? front - 5 : 1)); bool ok = jj < n;
                if (ok) j = jj;
            }
            if (j >= 0) { lv_rd_glass = 0; lv_rd_arc(B, j, lv_rd_ch(fr * 13 + 7, a * 7919 + j), LV_RD_CG_YMAX, 16); lv_rd_glass = 1; }
        }
    }
    /* 4. the charge front (while charging): a white-hot tip with a tiny spray of sparks + a crackle ahead */
    if (!lv_rd_lock && front > 0 && front < n - 1) {
        const int x = lv_rd_px[front], y = lv_rd_py[front]; lv_rd_gy = y;
        lv_rd_bl(B, x, y, W, 16, 16); lv_rd_bl(B, x - 1, y, ICE, 12, 16); lv_rd_bl(B, x + 1, y, ICE, 12, 16); lv_rd_bl(B, x, y - 1, ICE, 12, 16); lv_rd_bl(B, x, y + 1, ICE, 12, 16);
        for (int sp = 0; sp < 3; ++sp) { const uint32_t r = lv_rd_ch(fr, 4001 + sp); const int jj = front + 1 + (int)(r % 4); if (jj >= n) break;
            lv_rd_bl(B, lv_rd_px[jj] + (int)((r >> 8) % 5) - 2, lv_rd_py[jj] + (int)((r >> 12) % 5) - 2, (r >> 16) & 1 ? W : ICE, 14, 16); }
        if (front > 4) lv_rd_arc(B, front - 1, lv_rd_ch(fr, 4100), LV_RD_CG_YMAX, 16);
    }
    /* 5. locked: the stream lands on the tower; a small crackling contact glow at the base-row end */
    if (lv_rd_lock) {
        const int x = lv_rd_px[n - 1], y = lv_rd_py[n - 1]; const uint32_t r = lv_rd_ch(fr, 9001); lv_rd_gy = y;
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) { const int d2 = dx * dx + dy * dy; if (d2 > 5) continue; lv_rd_bl(B, x + dx, y + dy, d2 ? ELEC : W, d2 ? 6 + (int)(r % 5) : 15, 16); }
    }
    /* 6. D2 fix 4 - locked: the grown panel covers the route's first rows, so the stream gets a visible source: a glowing contact port
     * on the panel's top frame (y247) where the wire leaves the control panel - white-hot pad, ice frame glow either side, a pulsing
     * blue glow above it and crackles spitting off it. Only the frame pixels next to the wire change; nothing below y248. */
    if (lv_rd_lock) {
        int pi = -1; for (int i = 0; i < n; ++i) if (lv_rd_py[i] == 247) { pi = i; break; }
        if (pi >= 0) {
            const int x = lv_rd_px[pi]; const uint32_t r = lv_rd_ch(fr, 7001);
            const int ph = (int)(lv_now * 4 / 10 % 23), pulse = 8 + (int)(r % 5) + (ph < 6 ? 6 - ph : 0);   /* flares as each surge leaves */
            lv_rd_gy = 0;   /* glow above the frame: behind every tower */
            for (int dy = -6; dy <= -1; ++dy) for (int dx = -5; dx <= 5; ++dx) { const int d2 = dx * dx + dy * dy; if (d2 > 28) continue;
                lv_rd_bly(B, x + dx, 247 + dy, d2 < 5 ? ICE : ELEC, (28 - d2) * pulse / 40 + 1, 16, 246); }
            lv_rd_gy = 1 << 30;   /* on the panel frame itself: in front of everything */
            for (int dx = -4; dx <= 4; ++dx) { const int a = dx < 0 ? -dx : dx;
                lv_rd_bly(B, x + dx, 247, a == 0 ? W : a <= 2 ? ICE : ELEC, a == 0 ? 16 : 15 - a * 3 + (pulse - 8) / 3, 16, 248); }
            for (int dx = -2; dx <= 2; ++dx) { const int a = dx < 0 ? -dx : dx;
                lv_rd_bly(B, x + dx, 246, a == 0 ? W : ICE, 13 - a * 3, 16, 248); lv_rd_bly(B, x + dx, 248, ICE, 9 - a * 3, 16, 248); }
            for (int s = 0; s < 2; ++s) { const uint32_t rs = lv_rd_ch(fr, 7101 + s); if (s && (rs & 3)) continue;   /* 1-2 crackles off the port */
                lv_rd_arc(B, pi + 2 + (int)(rs % 4), rs >> 3, 246, 16); }
        }
    }
}
#endif
'''
rep("static void lv_rd_cg_log(const char *tag", DCODE + "static void lv_rd_cg_log(const char *tag")
open(p, 'w').write(s); print("patched D2", p)
