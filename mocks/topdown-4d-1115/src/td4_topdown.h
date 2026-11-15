/* MOCK 2026-11-15 (Mobandy 5:34 PM ET): TOP-DOWN 4D scan concept. NOT firmware, EX.DATA only.
 * A third viewpoint over the SAME Morpheus world (EB3 build): an overhead camera looks straight down on the scan field.
 *  - map axes: across = the device's lateral X; up the screen = its Gibson signal depth Z = 1.6 + (60 - (rssi + 100)) / 56 * 3.2
 *    (lv_radar.h lv_rd_place law), so distance from YOU (bottom centre) = weaker example signal. Range rings mark -40/-60/-80 dBm on that law.
 *  - every pillar = its EB3 enamel body (MO_EMB_COL) seen from above, height = the EB3 example-signal height (H), cap = its real emblem
 *    (mo_emb_draw) + threat lamp (mo_tc: HIGH red / MED amber / LOW green). True Gibson depth (EB3's 0.6 "forward" move is a front-view trick, not used).
 *  - 4th dimension: each device's trail = its real Morpheus drift law (mo11_dx / mo11_dz) sampled back over the last 60 s, fading with age.
 *  - time bar: 3 min window; dragging back replays positions + trails at that moment (scripted drag in this mock).
 * Floor grid spacing, scroll law and colours, code rain, header/terminal chrome are cloned from lv_scene_radar (skin 1) / mo_chrome / mo_terminal. */
#define TD_S 30.0f      /* px per world unit on the floor */
#define TD_HC 16.0f      /* camera height above the floor (world units) */
#define TD_ZC 3.0f      /* the camera looks straight down at Z = 2.8 */
#define TD_YOU 246      /* floor row of YOU (Z = 0) */
#define TD_FY1 247      /* map field rows 35..247; the time bar owns 249..285 */
#define TD_WIN 180.0f   /* scrub window, seconds */
#define TD_TRAIL 20.0f  /* trail length, seconds */
static float td_back = 0;   /* seconds scrubbed back from NOW (0 = live) */
static float td_px(float X, float Y) { return 120 + TD_S * (X - lv_rd_camx * 0.4f) * TD_HC / (TD_HC - Y); }
static float td_py(float Z, float Y) { return TD_YOU - TD_S * (TD_ZC + (Z - TD_ZC) * TD_HC / (TD_HC - Y)); }
static void td_pos(int i, float tau, float &X, float &Z)   /* the device's EX.DATA map position at scene time tau (Morpheus idle drift law, true Gibson depth) */
{
    const float Z0 = MO11_Z0[i], X0 = (MO11_SX[i] - 120) * Z0 / LV_RD_F;
    X = X0 + mo11_dx(i, tau); Z = Z0 + mo11_dz(i, tau);
}
static float td_rz(int rssi) { const float lv = (float)(rssi + 100); return 1.6f + (60.0f - lv) / 56.0f * 3.2f; }
static float td_sched(float t)   /* scripted drag for the mock: live, drag back to -1:30, hold, release back to NOW */
{
    auto ez = [](float u) { u = u < 0 ? 0 : u > 1 ? 1 : u; return u * u * (3 - 2 * u); };
    if (t < 79.4f) return 0; if (t < 81.2f) return 90.0f * ez((t - 79.4f) / 1.8f);
    if (t < 84.6f) return 90.0f; if (t < 85.8f) return 90.0f * (1 - ez((t - 84.6f) / 1.2f)); return 0;
}
static void td_chrome(LvBand &B)   /* mo_chrome, same bar / inks, new title */
{
    const uint16_t ink = lv_hex(0xe8dec8); char t[32];
    b_fill(B, 0, 24, 240, 1, lv_hex(MO11_BEAM)); b_fill(B, 0, 25, 240, 9, lv_hex(MO11_CHAR)); b_fill(B, 0, 34, 240, 1, lv_hex(MO11_G));
    int x = b_text35(B, 3, 27, "OVERHEAD", ink); x = b_text35(B, x + 4, 27, "//", lv_hex(MO11_GH)); b_text35(B, x + 4, 27, "4D SCAN", ink);
    snprintf(t, sizeof t, "%d BOTS  EX.DATA", MO_N); b_text35(B, 237 - lv_w35(t), 27, t, lv_hex(0xbfb294));
}
static void td_terminal(LvBand &B)   /* mo_terminal, same box / inks, top-down legend */
{
    const int x0 = 1, y0 = 287, x1 = 218, y1 = 319; const uint16_t g = lv_hex(0xbfb294), ink = lv_hex(0xe8dec8);
    b_fill(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(MO11_CHAR)); b_frame(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(MO11_SEAM));
    b_text35(B, x0 + 4, y0 + 4, "CLOSER TO YOU = STRONGER SIGNAL  EX.DATA", g);
    b_text35(B, x0 + 4, y0 + 11, "TRAIL = WHERE IT MOVED, LAST 20 S", ink);
    b_fill(B, x0 + 2, y0 + 18, x1 - x0 - 3, 1, lv_hex(0x3a3228));
    int x = b_text(B, x0 + 4, y0 + 22, ">", lv_hex(MO11_GH)); x = b_text(B, x + 3, y0 + 22, td_back > 0.5f ? "DRAG TO NOW = LIVE" : "DRAG TIME = REPLAY", lv_hex(MO11_GH));
    if ((lv_now / 500) & 1) b_fill(B, x + 1, y0 + 29, 5, 1, lv_hex(MO11_GH));
}
static void td_timebar(LvBand &B)   /* rows 249..285 (37 px touch row): the 4th dimension */
{
    const int y0 = 249, h = 37; b_fill(B, 0, y0, 240, h, lv_hex(MO11_CHAR)); b_fill(B, 0, y0, 240, 1, lv_hex(MO11_BEAM)); b_fill(B, 0, y0 + h - 1, 240, 1, lv_hex(MO11_SEAM));
    const int tx0 = 14, tx1 = 225, ty = 270; auto xa = [&](float back) { return (int)lroundf(tx1 - (tx1 - tx0) * back / TD_WIN); };
    static const char *LB[4] = {"-3:00", "-2:00", "-1:00", "NOW"};
    for (int k = 0; k <= 6; ++k) { const int x = xa(TD_WIN - k * 30.0f); b_fill(B, x, ty - (k % 2 ? 2 : 4), 1, k % 2 ? 2 : 4, lv_hex(0xbfb294)); }
    for (int k = 0; k < 4; ++k) { const int x = xa(TD_WIN - k * 60.0f), w = lv_w35(LB[k]); int lx = x - w / 2; if (lx < 2) lx = 2; if (lx + w > 238) lx = 238 - w; b_text35(B, lx, y0 + 4, LB[k], k == 3 ? lv_hex(MO11_GH) : lv_hex(0xbfb294)); }
    b_fill(B, tx0, ty, tx1 - tx0 + 1, 3, lv_hex(0x3a3228));                       /* the 3 min track */
    const int xh = xa(td_back), xt = xa(td_back + TD_TRAIL < TD_WIN ? td_back + TD_TRAIL : TD_WIN);
    b_fill(B, xt, ty, xh - xt + 1, 3, lv_hex(MO11_G));                             /* the trail window (60 s) the map is showing */
    b_fill(B, xh - 3, ty - 6, 7, 15, lv_hex(0xe8dec8)); b_frame(B, xh - 3, ty - 6, 7, 15, lv_hex(MO11_GH)); b_fill(B, xh, ty - 3, 1, 9, lv_hex(MO11_BEAM));   /* handle */
    char t[32]; const int s = (int)lroundf(td_back);
    if (s > 0) snprintf(t, sizeof t, "REPLAY -%d:%02d", s / 60, s % 60); else snprintf(t, sizeof t, "LIVE");
    { const int x = b_text35(B, 3, y0 + h - 8, "TIME", lv_hex(0xbfb294)); b_text35(B, x + 6, y0 + h - 8, t, s > 0 ? lv_hex(0xffb000) : lv_hex(MO11_GH)); }
}
static void td_scene(LvBand &B)
{
    const float t = mo_t(); td_back = td_sched(t); const float tau = t - td_back;
    const uint16_t grid = lv_hex(0x5c4a32);
    for (int y = B.y0; y < B.y0 + B.h; ++y) b_fill(B, B.x0, y, B.w, 1, (y >= LV_RD_Y0 && y <= LV_RD_Y1) ? lv_hex(0xe8dec8) : 0);   /* lv_scene_radar row fill: skin-1 parchment floor, black outside the field */
    /* Construct floor grid seen from above: the same 0.5 lanes (g = -12..12) and the same depth seams scrolling toward you (ph = t * 0.8) */
    const float ph = lv_rainmode == 2 ? 0.0f : fmodf(t * 0.8f, 1.0f);
    for (int g = -12; g <= 12; ++g) { const int x = (int)lroundf(td_px(g * 0.5f, 0)); for (int y = 35; y <= TD_FY1; ++y) mo13_px(B, x, y, lv_mix(lv_hex(0xe8dec8), grid, 4, 8), 256); }
    for (int r = 0; r <= 8; ++r) { const int y = (int)lroundf(td_py(r - ph + 1.0f, 0)); if (y >= 35 && y <= TD_FY1) for (int x = 0; x < 240; ++x) mo13_px(B, x, y, lv_mix(lv_hex(0xe8dec8), grid, 4, 8), 256); }
    /* signal range rings (Gibson depth law) around YOU */
    static const int RR[3] = {-40, -60, -80};
    for (int k = 0; k < 3; ++k) { const float R = td_rz(RR[k]);
        for (int a = 0; a < 360; ++a) { const float th = a * 3.14159265f / 360.0f; const int x = (int)lroundf(td_px(R * cosf(th), 0)), y = (int)lroundf(td_py(R * sinf(th), 0));
            if (a % 3 && y >= 36 && y <= TD_FY1) mo13_px(B, x, y, lv_hex(MO11_G), 150); }
        char s[12]; snprintf(s, sizeof s, "%d DBM", RR[k]); const int y = (int)lroundf(td_py(R, 0)) - 6; if (y > 36) b_text35(B, 121, y, s, lv_hex(0x0a7a30)); }
    /* code rain: lv_scene_radar's 18 streaks, skin-1 greens, verbatim law */
    if (lv_rain) for (int s = 0; s < 18; ++s) {
        const int x = (s * 53 + 17) % 236 + 2, len = 10 + (s * 7) % 18, sp = 40 + (s * 29) % 70;
        const int head = LV_RD_Y0 + (int)((lv_now / 1000.0f * sp + s * 37)) % (LV_RD_Y1 - LV_RD_Y0 + len);
        for (int k = 0; k < len; k += 2) { const int y = head - k; if (y >= LV_RD_Y0 && y <= TD_FY1) b_blend(B, x, y, lv_hex(k == 0 ? 0x10d858 : 0x00a83c), k == 0 ? 8 : 6 - k * 5 / len); }
    }
    /* YOU */
    { const int x = (int)lroundf(td_px(0, 0)); for (int d = 0; d < 5; ++d) b_fill(B, x - d, TD_YOU - 4 + d, 2 * d + 1, 1, lv_hex(MO11_GH)); b_text35(B, x + 6, TD_YOU - 6, "YOU", lv_hex(0x0a7a30)); }
    /* 4th dimension: trails (the last 60 s before the shown moment), oldest faintest */
    for (int i = 0; i < MO_N; ++i) { float px0 = -1, py0 = -1;
        for (int k = 40; k >= 0; --k) { float X, Z; td_pos(i, tau - k * (TD_TRAIL / 40), X, Z); const float x = td_px(X, 0), y = td_py(Z, 0);
            const int a = 40 + (40 - k) * 150 / 40;
            if (px0 >= 0 && y > 36 && y < TD_FY1) mo_aline(B, px0, py0, x, y, lv_hex(MO_EMB_COL[i]), a);
            if (k % 10 == 0 && k) { const int xi = (int)lroundf(x), yi = (int)lroundf(y); for (int dy = -1; dy <= 0; ++dy) for (int dx = -1; dx <= 0; ++dx) if (yi + dy > 35 && yi + dy < TD_FY1) mo13_px(B, xi + dx, yi + dy, lv_hex(mo_mixc(MO_EMB_COL[i], 0x000000, 0.35f)), a); }   /* a tick every 5 s */
            px0 = x; py0 = y; } }
    /* pillars from above, shortest first (taller caps sit over shorter neighbours) */
    int ord[MO_N]; float Hs[MO_N]; for (int i = 0; i < MO_N; ++i) { ord[i] = i; Hs[i] = MO_HFIX[MO_LOOKOF[i]] * MO11_HK[i]; }
    for (int a = 1; a < MO_N; ++a) { const int v = ord[a]; int b = a - 1; while (b >= 0 && Hs[ord[b]] > Hs[v]) { ord[b + 1] = ord[b]; --b; } ord[b + 1] = v; }
    const float w = 0.25f;
    for (int n = 0; n < MO_N; ++n) { const int i = ord[n]; float X, Z; td_pos(i, tau, X, Z); const float H = Hs[i]; const uint32_t col = MO_EMB_COL[i];
        const float bx[4] = {X - w, X + w, X + w, X - w}, bz[4] = {Z - w, Z - w, Z + w, Z + w};
        float BX[4], BY[4], TX[4], TY[4]; for (int k = 0; k < 4; ++k) { BX[k] = td_px(bx[k], 0); BY[k] = td_py(bz[k], 0); TX[k] = td_px(bx[k], H); TY[k] = td_py(bz[k], H); }
        { const float sx[4] = {BX[0] + 3, BX[1] + 3, BX[2] + 3, BX[3] + 3}, sy[4] = {BY[0] + 2, BY[1] + 2, BY[2] + 2, BY[3] + 2}; mo_quad(B, sx, sy, lv_hex(0x9a8a6e), 110); }   /* floor shadow */
        for (int k = 0; k < 4; ++k) { const int k2 = (k + 1) & 3; const float qx[4] = {BX[k], BX[k2], TX[k2], TX[k]}, qy[4] = {BY[k], BY[k2], TY[k2], TY[k]};
            const float sh = k == 0 ? 0.30f : k == 1 ? 0.52f : k == 2 ? 0.62f : 0.42f;   /* fixed light from the top-left: each side its own shade of the enamel */
            mo_quad(B, qx, qy, lv_hex(mo_mixc(col, 0x000000, sh)), 256);
            mo_aline(B, BX[k], BY[k], TX[k], TY[k], lv_hex(mo_mixc(col, 0x000000, 0.75f)), 200); }
        mo_quad(B, TX, TY, lv_hex(col), 256);                                                        /* enamel cap */
        const int cx0 = (int)ceilf(TX[0]), cx1 = (int)floorf(TX[1]), cy0 = (int)ceilf(TY[2]), cy1 = (int)floorf(TY[0]);
        b_fill(B, cx0, cy0, cx1 - cx0 + 1, 1, lv_hex(mo_mixc(col, 0xffffff, 0.55f))); for (int y = cy0; y <= cy1; ++y) mo13_px(B, cx0, y, lv_hex(mo_mixc(col, 0xffffff, 0.4f)), 256);   /* lit bevel */
        const int cw = cx1 - cx0 + 1; int S = cw - 6; if (S > 16) S = 16; S &= ~1;
        if (S >= 6) { const int ex = cx0 + (cw - S) / 2, ey = cy0 + (cy1 - cy0 + 1 - S) / 2;
            b_fill(B, ex - 1, ey - 1, S + 2, S + 2, lv_hex(0x1c120c)); mo_emb_draw(B, i, ex, ey, S, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, 256, 256, 9999); }
        b_fill(B, cx1 - 3, cy0 + 1, 3, 3, lv_hex(mo_tc(i))); mo13_px(B, cx1 - 3, cy0 + 1, lv_hex(mo_mixc(mo_tc(i), 0xffffff, 0.6f)), 256);   /* threat lamp */
    }
    td_timebar(B); td_terminal(B); td_chrome(B);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
    if (td_back > 0.5f) { char m[LV_TKLEN]; const int s = (int)lroundf(td_back); snprintf(m, sizeof m, "REPLAY -%d:%02d  18 BOTS  TRAILS = LAST 20 S  EX.DATA", s / 60, s % 60);
        if (strcmp(m, lv_tk_msg)) { snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", m); lv_tk_off = LV_R_TICK.x1 - LV_R_TICK.x0 - 4; } lv_tk_alert = false; }
}
