"""MOCK radar-jolt: patch a SCRATCH copy of src/live/lv_radar.h (never the repo's firmware).
Everything new is behind LV_RD_JOLT (default 0 -> the file compiles to exactly the old radar).
usage: python3 apply_jolt.py <scratch>/src/live/lv_radar.h"""
import sys
p = sys.argv[1]; s = open(p).read()
def rep(old, new, n=1):
    global s
    c = s.count(old)
    assert c == n, (c, old[:80])
    s = s.replace(old, new)

JOLT = r'''
#ifndef LV_RD_JOLT
#define LV_RD_JOLT 0
#endif
#if LV_RD_JOLT
/* MOCK radar-jolt (owner 2026-11-04 6:41 AM ET): no hold. Tap a tower (the highlighted one or any other) and a jagged energy
 * bolt shoots from the mini terminal (the lower control panel) up to the tower base, ~360 ms with crackle frames, in the
 * radar's magenta / violet neon. The tower lights up in its OWN colour (never white, never hue-shifted) and stays lit while
 * locked; then the same lock (brackets + LOCK label, IGNORE / WATCH / HUNT) as the hold used to give. */
#define LV_RD_JOLT_MS 360
static bool lv_rd_jolt = false; static uint32_t lv_rd_jolt_t = 0; static uint8_t lv_rd_joltmac[6];
static uint32_t lv_rd_jh(uint32_t a, uint32_t b) { uint32_t h = a * 2654435761u ^ (b + 0x9e3779b9u) * 2246822519u; h ^= h >> 15; h *= 2654435761u; h ^= h >> 13; return h; }
static bool lv_rd_jtarget(int i)
{
    if (i < 0 || i >= lv_nf) return false;
    return (lv_rd_jolt && !memcmp(lv_f[i].mac, lv_rd_joltmac, 6)) || (lv_rd_lock && !memcmp(lv_f[i].mac, lv_rd_lockmac, 6));
}
static float lv_rd_jlit(void)   /* 0 = normal tower, 1 = fully lit */
{
    if (lv_rd_lock) return 1.0f;
    if (!lv_rd_jolt) return 0.0f;
    const int e = (int)(lv_now - lv_rd_jolt_t); if (e < 150) return 0.0f;
    float u = (e - 150) / 210.0f; return u > 1 ? 1 : u;
}
static void lv_rd_jpix(LvBand &B, int x, int y, int ymin, uint16_t c) { if (y >= ymin && y >= LV_RD_Y0 && y <= LV_RD_Y1) b_pix(B, x, y, c); }
static void lv_rd_jline(LvBand &B, int x0, int y0, int x1, int y1, int ymin, uint16_t c)   /* b_line, only rows >= ymin */
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (int n = 0; n < 800; ++n) { lv_rd_jpix(B, x0, y0, ymin, c); if (x0 == x1 && y0 == y1) break; const int e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
}
/* the lit tower: its own colour at full strength, filling from the base up while the energy arrives, then held */
static void lv_rd_jlight(LvBand &B)
{
    const float L = lv_rd_jlit(); if (L <= 0) return;
    const uint8_t *m = lv_rd_lock ? lv_rd_lockmac : lv_rd_joltmac;
    const LvTower *T = lv_rd_tower_of(m); if (!T) return;
    const LvFind &f = lv_f[T->i]; const uint16_t C = lv_rd_col(f, 8, 8);
    const int ylit = L >= 1 ? -9999 : T->yb - (int)lroundf(L * (T->yb - T->byt));   /* rising light line */
    const int pop = lv_rd_lock ? (int)(lv_now - lv_rd_lock_t) : 9999;               /* short over-bright right after the strike */
    const int kf = pop < 150 ? 4 - pop * 2 / 150 : 2;                                   /* extra face fill, blend of 8 */
    for (int y = T->byt; y <= T->yt; ++y) { if (y < ylit || y < LV_RD_Y0 || y > LV_RD_Y1) continue; const float u = T->yt == T->byt ? 1.0f : (float)(y - T->byt) / (T->yt - T->byt);
        const int xa = (int)(T->bx0 + (T->x0 - T->bx0) * u), xb = (int)(T->bx1 + (T->x1 - T->bx1) * u); for (int x = xa; x <= xb; ++x) b_blend(B, x, y, C, kf); }
    for (int y = T->yt; y <= T->yb; ++y) { if (y < ylit || y < LV_RD_Y0 || y > LV_RD_Y1) continue; for (int x = T->x0; x <= T->x1; ++x) b_blend(B, x, y, C, kf); }
    lv_rd_jline(B, T->x0, T->yt, T->x0, T->yb, ylit, C); lv_rd_jline(B, T->x1, T->yt, T->x1, T->yb, ylit, C);
    lv_rd_jline(B, T->x0, T->yt, T->x1, T->yt, ylit, C); lv_rd_jline(B, T->x0, T->yb, T->x1, T->yb, ylit, C);
    lv_rd_jline(B, T->bx0, T->byt, T->bx1, T->byt, ylit, C); lv_rd_jline(B, T->bx0, T->byt, T->x0, T->yt, ylit, C); lv_rd_jline(B, T->bx1, T->byt, T->x1, T->yt, ylit, C);
    if (T->bx1 > T->x1) lv_rd_jline(B, T->bx1, T->byt, T->bx1, T->byb, ylit, C);
    if (T->bx0 < T->x0) lv_rd_jline(B, T->bx0, T->byt, T->bx0, T->byb, ylit, C);
    if (L >= 1) {   /* steady own-colour halo, the cycle glow's two rings but in the tower's colour (the LOCK brackets sit at pad 3) */
        int xa, xb, ya, yb; const int k1 = pop < 150 ? 7 : 4;
        lv_rd_box(*T, 1, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, C, k1);
        lv_rd_box(*T, 2, xa, xb, ya, yb); lv_rd_rect_blend(B, xa, ya, xb, yb, C, k1 / 2);
    }
}
/* the bolt: a distance field around a jagged polyline (+ 2 forks + impact sparks), re-jagged every frame (crackle) */
#define LV_RD_JN 10
static void lv_rd_jbolt(LvBand &B)
{
    if (!lv_rd_jolt) return;
    const int e = (int)(lv_now - lv_rd_jolt_t); if (e < 0 || e >= LV_RD_JOLT_MS) return;
    const LvTower *T = lv_rd_tower_of(lv_rd_joltmac); if (!T) return;
    const uint16_t HOT = lv_hex(0xffd8f6), MAG = lv_hex(0xff2ad4), VIO = lv_hex(0xb040ff);
    const int fr = e / 30;   /* one sim / firmware frame = 30 ms */
    /* the lower control panel charges: its frame flares hot pink, fading over the first 150 ms */
    if (e < 150) { const int k = 7 - e * 6 / 150; lv_rd_rect_blend(B, 1, 287, 218, 319, HOT, k); lv_rd_rect_blend(B, 2, 288, 217, 318, MAG, k / 2); }
    const float x0 = 110, y0 = 287, x1 = (T->x0 + T->x1) / 2.0f, y1 = (float)(T->yb < LV_RD_Y1 ? T->yb : LV_RD_Y1);
    const float dxl = x1 - x0, dyl = y1 - y0, len = sqrtf(dxl * dxl + dyl * dyl) + 1e-3f, nx = -dyl / len, ny = dxl / len;
    float sx[40], sy[40], ex[40], ey[40], sw[40]; int ns = 0;
    float px[LV_RD_JN + 1], py[LV_RD_JN + 1];
    for (int k = 0; k <= LV_RD_JN; ++k) {
        const float t = (float)k / LV_RD_JN, amp = (k == 0 || k == LV_RD_JN) ? 0 : 9.0f * sinf(t * 3.1416f) + 2.0f;
        const float o = ((int)(lv_rd_jh(fr, k) % 1000) / 500.0f - 1.0f) * amp;
        px[k] = x0 + dxl * t + nx * o; py[k] = y0 + dyl * t + ny * o;
    }
    float grow = e < 60 ? 0.0f : (e - 60) / 90.0f; if (grow > 1) grow = 1;   /* tip races up 60..150 ms */
    if (grow <= 0) { /* charge frame: only a spark on the panel edge */ }
    const float reach = grow * LV_RD_JN;
    for (int k = 0; k < LV_RD_JN && k < reach; ++k) {
        const float fpart = reach - k < 1 ? reach - k : 1;
        sx[ns] = px[k]; sy[ns] = py[k]; ex[ns] = px[k] + (px[k + 1] - px[k]) * fpart; ey[ns] = py[k] + (py[k + 1] - py[k]) * fpart; sw[ns++] = 1.0f;
    }
    for (int b = 0; b < 2; ++b) {   /* forks off the main bolt */
        const int at = 3 + b * 3; if (reach < at + 1) continue;
        const uint32_t h = lv_rd_jh(fr * 7 + b, 99); const float side = (h & 1) ? 1.0f : -1.0f, L1 = 6 + h % 7, L2 = 4 + (h >> 4) % 5;
        const float ux = dxl / len, uy = dyl / len;
        const float ax = px[at] + (ux * 0.55f + nx * side * 0.85f) * L1, ay = py[at] + (uy * 0.55f + ny * side * 0.85f) * L1;
        const float bx = ax + (ux * 0.8f - nx * side * 0.6f) * L2, by = ay + (uy * 0.8f - ny * side * 0.6f) * L2;
        sx[ns] = px[at]; sy[ns] = py[at]; ex[ns] = ax; ey[ns] = ay; sw[ns++] = 0.6f;
        sx[ns] = ax; sy[ns] = ay; ex[ns] = bx; ey[ns] = by; sw[ns++] = 0.45f;
    }
    const bool hit = grow >= 1;
    if (hit) for (int r = 0; r < 6; ++r) {   /* impact sparks at the tower base */
        const uint32_t h = lv_rd_jh(fr * 13 + r, 7); const float a = (h % 628) / 100.0f, l = 3 + (h >> 10) % 5;
        sx[ns] = x1; sy[ns] = y1; ex[ns] = x1 + cosf(a) * l; ey[ns] = y1 - fabsf(sinf(a)) * l; sw[ns++] = 0.45f;
    }
    /* brightness: full while growing, then crackle (flicker) and fade out by 360 ms */
    static const float FL[12] = {1, 1, 1, 1, 1, 1, 0.65f, 1, 0.8f, 0.95f, 0.5f, 0.35f};
    const float fk = FL[fr < 12 ? fr : 11];
    int bx0 = 239, by0 = 319, bx1 = 0, by1 = 0;
    for (int k = 0; k < ns; ++k) { const float a[4] = {sx[k], ex[k], sy[k], ey[k]};
        if (a[0] < bx0) bx0 = (int)a[0]; if (a[1] < bx0) bx0 = (int)a[1]; if (a[0] > bx1) bx1 = (int)a[0]; if (a[1] > bx1) bx1 = (int)a[1];
        if (a[2] < by0) by0 = (int)a[2]; if (a[3] < by0) by0 = (int)a[3]; if (a[2] > by1) by1 = (int)a[2]; if (a[3] > by1) by1 = (int)a[3]; }
    bx0 -= 6; by0 -= 6; bx1 += 6; by1 += 6; if (bx0 < 0) bx0 = 0; if (bx1 > 239) bx1 = 239;
    const int ya = by0 > B.y0 ? by0 : B.y0, yb = by1 < B.y0 + B.h - 1 ? by1 : B.y0 + B.h - 1;
    for (int y = ya; y <= yb && ns; ++y) for (int x = bx0; x <= bx1; ++x) {
        float dn = 1e9f;
        for (int k = 0; k < ns; ++k) {
            const float vx = ex[k] - sx[k], vy = ey[k] - sy[k], wx = x - sx[k], wy = y - sy[k], vv = vx * vx + vy * vy;
            float t = vv > 0 ? (wx * vx + wy * vy) / vv : 0; t = t < 0 ? 0 : t > 1 ? 1 : t;
            const float qx = wx - vx * t, qy = wy - vy * t, d = sqrtf(qx * qx + qy * qy) / sw[k]; if (d < dn) dn = d;
        }
        uint16_t c; int k;
        if (dn <= 1.0f) { c = HOT; k = 8; } else if (dn <= 2.0f) { c = MAG; k = 8; } else if (dn <= 3.4f) { c = MAG; k = 4; } else if (dn <= 5.4f) { c = VIO; k = 2; } else continue;
        k = (int)lroundf(k * fk); if (k > 0) b_blend(B, x, y, c, k);
    }
    /* flare where it leaves the panel, flash where it lands */
    for (int a = -3; a <= 3; ++a) for (int b = -3; b <= 3; ++b) { const int d2 = a * a + b * b; if (d2 > 9) continue;
        b_blend(B, (int)x0 + b, (int)y0 + a, d2 <= 2 ? HOT : MAG, (int)lroundf((d2 <= 2 ? 8 : 4) * fk));
        if (hit) b_blend(B, (int)x1 + b, (int)y1 + a, d2 <= 2 ? HOT : MAG, (int)lroundf((d2 <= 2 ? 8 : 3) * fk)); }
}
#endif
'''
rep("static void lv_rd_fx(LvBand &B)   /* drawn after", JOLT + "static void lv_rd_fx(LvBand &B)   /* drawn after")
# lit tower overlay inside fx (after the cycle glow, before brackets); white snap flash dropped in jolt mode
rep("""    if (lv_rd_holding && !lv_rd_lock) {
        const LvTower *T = lv_rd_tower_of(lv_rd_holdmac); if (!T) return;""",
"""#if LV_RD_JOLT
    lv_rd_jlight(B);
#endif
    if (lv_rd_holding && !lv_rd_lock) {
        const LvTower *T = lv_rd_tower_of(lv_rd_holdmac); if (!T) return;""")
rep("""    if (e < 200) lv_rd_rect_blend(B, xa - 1, ya - 1, xb + 1, yb + 1, W, 7 - e * 7 / 200);       /* lock snap flash */""",
"""    if (e < 200 && !LV_RD_JOLT) lv_rd_rect_blend(B, xa - 1, ya - 1, xb + 1, yb + 1, W, 7 - e * 7 / 200);       /* lock snap flash (jolt: the strike is the flash) */""")
# the target tower is never white-edged in jolt mode (it lights in its own colour instead)
rep("        const bool sel = T.i == lv_rd_sel;",
    "        const bool sel = T.i == lv_rd_sel\n#if LV_RD_JOLT\n            && !lv_rd_jtarget(T.i)\n#endif\n            ;")
# bolt over panel + terminal (it leaves the terminal), under BACKbot
rep("""    lv_rd_term(B, hdr);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);""",
"""    lv_rd_term(B, hdr);
#if LV_RD_JOLT
    lv_rd_jbolt(B);
#endif
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);""")
# release clears a jolt
rep("""    lv_rd_lock = false; lv_rd_holding = false;
}""", """    lv_rd_lock = false; lv_rd_holding = false;
#if LV_RD_JOLT
    lv_rd_jolt = false;
#endif
}""")
rep('lv_rd_log(lv_nf ? "TAP=CYCLE  HOLD 1S=LOCK" : "NO NODES YET - SCANNING");',
    'lv_rd_log(lv_nf ? (LV_RD_JOLT ? "TAP=CYCLE  TAP TOWER=LOCK" : "TAP=CYCLE  HOLD 1S=LOCK") : "NO NODES YET - SCANNING");')
# strike lands -> lock
rep("""static void lv_radar_frame(void)
{""", """static void lv_radar_frame(void)
{
#if LV_RD_JOLT
    if (lv_rd_jolt && (int32_t)(lv_now - lv_rd_jolt_t) >= LV_RD_JOLT_MS) { lv_rd_jolt = false; const int i = lv_rd_find(lv_rd_joltmac); if (i >= 0) lv_rd_lock_on(i); }
#endif""")
# no hold gesture
rep("""static void lv_radar_touch(bool down, int x, int y)
{""", """static void lv_radar_touch(bool down, int x, int y)
{
#if LV_RD_JOLT
    (void)down; (void)x; (void)y; return;   /* jolt mode: no hold-to-lock */
#endif""")
rep("""    if (lv_rd_swallow) { lv_rd_swallow = false; return "radar-locked"; }""",
"""    if (lv_rd_swallow) { lv_rd_swallow = false; return "radar-locked"; }
#if LV_RD_JOLT
    if (lv_rd_jolt) return "radar-jolt-busy";   /* taps during the 360 ms strike do nothing */
#endif""")
rep("""    if (i >= 0) { lv_rd_select(i); return "radar-tower"; }""",
"""#if LV_RD_JOLT
    if (i >= 0) { lv_rd_select(i); lv_rd_glow = false; lv_rd_jolt = true; lv_rd_jolt_t = lv_now; memcpy(lv_rd_joltmac, lv_f[i].mac, 6); return "radar-jolt"; }
#endif
    if (i >= 0) { lv_rd_select(i); return "radar-tower"; }""")
open(p, 'w').write(s); print("patched", p)
