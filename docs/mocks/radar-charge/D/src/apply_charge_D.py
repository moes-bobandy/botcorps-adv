"""MOCK radar-charge design D (owner 2026-11-04 8:41 AM ET): "Make it Gibson only rain and use the bar button ui keep the stream
constantly flowing as it's being hunted. Make the Gibson bot electrical stream look electrical."
Applies on top of apply_charge.py (A/B/C, unchanged) to a SCRATCH copy of src/live/lv_radar.h @3240b03; never the firmware.
LV_RD_CHARGE=4 -> D: A's HOLD TO LOCK bar button (x166..234 y250..260, identical drawing) + C's route (grid row -> up a real floor
line -> tower base row) carrying GIBSON-only rain (LOCKED ui_main_bot_rain GIBSON glyph, white, lead 100 %, trail 92/90/88/86 %)
made electric: hot lead, flicker, crackle arcs hugging the path, an electric-blue glow halo. After the lock the stream never fades:
it keeps flowing to the locked tower through LOCK, IGNORE / WATCH / HUNT, and while hunting. LV_RD_CHARGE 0..3 unchanged.
usage: python3 apply_charge_D.py <scratch>/src/live/lv_radar.h   (run AFTER apply_charge.py)"""
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
static const int LV_RD_GIBLV[5] = {100, 92, 90, 88, 86};   /* GIBSON: lead #FFFFFF, trail 92..86 % white (never grey) */
static inline void lv_rd_bl(LvBand &B, int x, int y, uint16_t c, int k, int n)   /* finer blend than b_blend's /8, clipped to the radar */
{
    if (k <= 0 || y < LV_RD_Y0 || y > LV_RD_CG_YMAX) return; if (k > n) k = n;
    if (x >= B.x0 && x < B.x0 + B.w && y >= B.y0 && y < B.y0 + B.h) { uint16_t &q = B.px[(y - B.y0) * B.w + (x - B.x0)]; q = lv_mix(q, c, k, n); }
}
static void lv_rd_dstream(LvBand &B, float lvl)
{
    const uint16_t W = lv_hex(0xffffff), ICE = lv_hex(0xd2f4ff), ELEC = lv_hex(0x3aa8ff), VIO = lv_hex(0x7a1cff), BLK = lv_hex(0x000000);
    const int n = lv_rd_np; if (n < 2) return;
    const int front = lv_rd_lock ? n - 1 : (int)lroundf(lvl * (n - 1));
    const uint32_t fr = lv_now / 30;                       /* one sim frame: flicker / crackle re-roll every frame */
    const int cell = 9, step = (int)(lv_now / 60), P = 8;  /* rain cells 9 px apart along the route, one cell per 60 ms */
    /* 1. the conduit: the floor line it rides. Energized part = a flickering ice-white wire in a blue glow; ahead = faint violet dots */
    for (int i = 0; i < n; ++i) {
        const int x = lv_rd_px[i], y = lv_rd_py[i];
        if (i > front) { if (i & 1) lv_rd_bl(B, x, y, VIO, 3, 8); continue; }
        const uint32_t h = lv_rd_ch(fr, i * 7 + 1); const int k = 7 + (int)(h % 7);   /* 7..13 / 16 brightness jitter */
        lv_rd_bl(B, x, y, ICE, k, 16);
        const int kg = 4 + (int)((h >> 8) % 3);
        lv_rd_bl(B, x - 1, y, ELEC, kg, 16); lv_rd_bl(B, x + 1, y, ELEC, kg, 16); lv_rd_bl(B, x, y - 1, ELEC, kg, 16); lv_rd_bl(B, x, y + 1, ELEC, kg, 16);
    }
    /* 1b. surges: bright packets racing up the wire (0.4 px/ms, every 23 px), so the current visibly flows between rain steps */
    { const int ph = (int)(lv_now * 4 / 10 % 23);
      for (int i = ph; i <= front && i < n; i += 23) for (int t = 0; t < 4; ++t) { const int j = i - t; if (j < 0) break;
          lv_rd_bl(B, lv_rd_px[j], lv_rd_py[j], t ? ICE : W, 16 - t * 3, 16); } }
    /* 2. GIBSON rain flowing along it: sparse runs (3-5 lit per 8-cell cycle) racing to the tower, lead white-hot */
    int lit[64], litrel[64], nl = 0;
    for (int c = 0; c * cell < n && nl < 64; ++c) {
        const int i = c * cell; if (i > front) break;
        const int rel = ((step - c) % P + P) % P, run = 3 + (int)(lv_rd_ch((step - c + 800) / P, 5) % 3);
        if (rel >= run) continue;
        lit[nl] = c; litrel[nl++] = rel;
    }
    /* 2a. glow halo under every glyph (electric blue, flickers; the lead's is bigger and whiter) */
    for (int q = 0; q < nl; ++q) {
        const int i = lit[q] * cell, x = lv_rd_px[i], y = lv_rd_py[i]; const bool lead = litrel[q] == 0;
        const uint32_t h = lv_rd_ch(fr, lit[q] * 31 + 5); const int fl = 5 + (int)(h % 4);   /* 5..8: the halo flickers */
        const int R2 = lead ? 46 : 34;
        for (int dy = -7; dy <= 7; ++dy) for (int dx = -7; dx <= 7; ++dx) {
            const int d2 = dx * dx + (2 * dy - 1) * (2 * dy - 1) / 4; if (d2 > R2) continue;   /* centred on the glyph body (y+0.5) */
            int k = (R2 - d2) * fl * (lead ? 2 : 1) / R2 + 2;
            lv_rd_bl(B, x + dx, y + dy, lead && d2 < 14 ? ICE : ELEC, k, 16);
        }
    }
    /* 2b. the glyphs: white at GIBSON's levels, brightness jitter (never below 80 %), lead stays white-hot */
    for (int q = 0; q < nl; ++q) {
        const int c = lit[q], rel = litrel[q], i = c * cell, x = lv_rd_px[i], y = lv_rd_py[i]; const bool lead = rel == 0;
        const uint32_t h = lv_rd_ch(fr, c * 13 + 3);
        int lv = LV_RD_GIBLV[rel]; if (!lead) { lv -= (int)(h % 9); if ((h >> 8) % 9 == 0) lv = 80; }   /* flicker */
        const uint16_t col = lv_dim(W, lv, 100);
        for (int yy = 0; yy < 8; ++yy) for (int xx = 0; xx < 8; ++xx) {
            const char ch = LV_RD_GIB[yy][xx]; if (ch == '.') continue;
            const int gx = x - 4 + xx, gy = y - 4 + yy; if (gy < LV_RD_Y0 || gy > LV_RD_CG_YMAX) continue;
            b_pix(B, gx, gy, ch == 'e' ? BLK : col);
        }
        if (lead) { const int ax = x, ay = y - 4; if (ay - 1 >= LV_RD_Y0 && ay - 1 <= LV_RD_CG_YMAX) lv_rd_bl(B, ax, ay - 1, ICE, (int)(10 + h % 6), 16); }   /* antenna spark */
    }
    /* 3. crackle: short arcs hugging the route (parallel to it, 5-6 px off to either side), re-rolled every frame */
    const int na = 4 + nl;
    for (int a = 0; a < na; ++a) {
        const uint32_t r = lv_rd_ch(fr * 7 + 3, a * 977 + 11);
        if (front < 8) break;
        const int j = 3 + (int)(r % (uint32_t)(front - 5 > 1 ? front - 5 : 1)); if (j + 3 >= n) continue;
        const float tx = lv_rd_px[j + 3] - lv_rd_px[j - 3], ty = lv_rd_py[j + 3] - lv_rd_py[j - 3], tl = sqrtf(tx * tx + ty * ty);
        if (tl < 1) continue;
        const float ux = tx / tl, uy = ty / tl, nx = -uy, ny = ux; const int side = (r >> 9) & 1 ? 1 : -1, off = 5 + (int)((r >> 10) & 1);
        const int len = 4 + (int)((r >> 12) % 5); int wob = 0;
        for (int t = 0; t < len; ++t) {
            const uint32_t rr = lv_rd_ch(r, t); if (t) wob += (int)(rr % 3) - 1; if (wob > 1) wob = 1; if (wob < -1) wob = -1;
            const int x = (int)lroundf(lv_rd_px[j] + ux * t + nx * side * (off + wob)), y = (int)lroundf(lv_rd_py[j] + uy * t + ny * side * (off + wob));
            bool nearp = false;   /* keep every arc pixel within 7 px of the route (corners: the tangent can overshoot) */
            for (int m = j - 12; m <= j + 12 && !nearp; ++m) { if (m < 0 || m >= n) continue;
                const int ex = x > lv_rd_px[m] ? x - lv_rd_px[m] : lv_rd_px[m] - x, ey = y > lv_rd_py[m] ? y - lv_rd_py[m] : lv_rd_py[m] - y; nearp = ex <= 7 && ey <= 7; }
            if (!nearp) continue;
            lv_rd_bl(B, x, y, t == 0 ? W : (t & 1 ? ICE : ELEC), 16 - t * 2, 16);
        }
    }
    /* 4. the charge front (while charging): a white-hot tip with a tiny spray of sparks ahead */
    if (!lv_rd_lock && front > 0 && front < n - 1) {
        const int x = lv_rd_px[front], y = lv_rd_py[front];
        lv_rd_bl(B, x, y, W, 16, 16); lv_rd_bl(B, x - 1, y, ICE, 12, 16); lv_rd_bl(B, x + 1, y, ICE, 12, 16); lv_rd_bl(B, x, y - 1, ICE, 12, 16); lv_rd_bl(B, x, y + 1, ICE, 12, 16);
        for (int sp = 0; sp < 3; ++sp) { const uint32_t r = lv_rd_ch(fr, 4001 + sp); const int jj = front + 1 + (int)(r % 4); if (jj >= n) break;
            lv_rd_bl(B, lv_rd_px[jj] + (int)((r >> 8) % 5) - 2, lv_rd_py[jj] + (int)((r >> 12) % 5) - 2, (r >> 16) & 1 ? W : ICE, 14, 16); }
    }
    /* 5. locked: the stream lands on the tower; a small crackling contact glow at the base-row end */
    if (lv_rd_lock) {
        const int x = lv_rd_px[n - 1], y = lv_rd_py[n - 1]; const uint32_t r = lv_rd_ch(fr, 9001);
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) { const int d2 = dx * dx + dy * dy; if (d2 > 5) continue; lv_rd_bl(B, x + dx, y + dy, d2 ? ELEC : W, d2 ? 6 + (int)(r % 5) : 15, 16); }
    }
}
#endif
'''
rep("static void lv_rd_cg_log(const char *tag", DCODE + "static void lv_rd_cg_log(const char *tag")
open(p, 'w').write(s); print("patched D", p)
