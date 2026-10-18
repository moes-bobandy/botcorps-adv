/* ===== v1019j (Dirt LOCKED 2026-10-18): Defend mini-games, one per device category ==============
 * Included by botcorps_ui.cpp just before paint_gchat. Every Defend path (enter_defend: hub, Terminal,
 * scan box; game chat `defend` / `load game`) lands on the Defend board AND starts the game for the
 * active bot's device category:
 *   Wi-Fi  -> RAIN CATCH   (1A) catch 20 of your own device icons; 3 missed icons or 3 bits = lose
 *   BLE    -> BEACON SORT  (2A) sort falling BLE icons: tag 1 = trusted (`,`), tag 0 = unknown (`/`);
 *                               25 right = win, 3 mistakes = lose, faster every 5
 *   IoT    -> FIREWALL WALL(3B) Breakout: the active bot is the paddle guarding the IoT icons; clear
 *                               every hostile 1-bit block = win, 3 packets lost = lose
 *   Rogue  -> GIBSON MAZE  (4A) eat every bit-pellet, jammers chase as ghosts, device icons let the
 *                               bot chase jammers for 5 s; caught 3x = lose
 * Keys (doc): ; . , / arrows, Enter action (launch / replay), Space second action (launch on the
 * Firewall, pause elsewhere), backtick quits to the Defend board. Art is reused only: square chip bots
 * (chip_mini, black die), the 0/1 rain glyphs (the rain's own 3x5 bit art) and device icons (gl_icon /
 * dev_icon), matte-black board framed like the game-chat panel. EXT = play field; INT = score / lives /
 * timer as plain text. The board is opaque, rain shows as a static layer around it; moving parts are
 * patched in place (dirty rects, same painter as the full paint). Deterministic LCG, idempotent per
 * millis, so a patch matches a full paint pixel for pixel (flicker gate S1). */
/* v1019m: twelve games, three per category. Ids 0-3 are the v1019j games (unchanged ids). */
enum { DG_RAIN = 0, DG_BEACON, DG_WALL, DG_MAZE, DG_TWIN, DG_HOP, DG_RING, DG_PAIR, DG_PATCH, DG_BOTNET, DG_TRACE, DG_JAM, DG_N };
static const char *const DG_NAME[DG_N] = { "RAIN CATCH", "BEACON SORT", "FIREWALL WALL", "GIBSON MAZE",
    "EVIL TWIN", "CHANNEL HOP", "RANGE RING", "PAIR LOCK", "PATCH RUN", "BOTNET BREAKER", "SIGNAL TRACE", "JAM DODGE" };
/* per category: slot 0 = the v1019j game every Defend entry starts, then 1C/2C/3A/4C, then 1B/2B/3C/4B */
static const int8_t DG_SET[4][3] = { { DG_RAIN, DG_TWIN, DG_HOP }, { DG_BEACON, DG_RING, DG_PAIR },
                                     { DG_WALL, DG_PATCH, DG_BOTNET }, { DG_MAZE, DG_TRACE, DG_JAM } };
/* bot -> device category: 0 Wi-Fi, 1 BLE, 2 IoT, 3 Rogue (12 = Gibson, the Rogue flagship) */
static const int8_t DG_CAT[13] = { 0, 1, 3, 0, 2, 3, 1, 2, 1, 3, 2, 1, 3 };
enum { DGS_PLAY = 0, DGS_WIN, DGS_LOST, DGS_PAUSE };

static const int DG_BX = 8, DG_BY = 20, DG_BW = 304, DG_BH = 214;   /* matte board, rain gutter around */
static const int DG_PX = 10, DG_PY = 22, DG_PW = 300, DG_PH = 210;  /* play area inside the frame */


static int s_dg_bot = 12;
static int s_dg_slot = 0;          /* v1019m: which of the category's three games */
static int s_dg_state = DGS_PLAY;
static uint32_t s_dg_t = 0, s_dg_rng = 1, s_dg_ms = 0;   /* last step, rng, played ms (pause excluded) */
static int s_dg_score = 0, s_dg_miss = 0, s_dg_junk = 0;
static uint32_t s_dg_hold = 0;     /* maze: frozen until (after a catch) */
static bool s_dg_launch = false;   /* key edge for the step (Enter / Space) */
#ifdef BOTCORPS_HOST_RENDER
static bool s_dg_noghost = false;  /* host logic test only: maze without jammers (proves clear = win) */
#endif


/* v1019l (Dirt concept: games = software side): the active bot plays as its ROUND sprite, exactly as in
 * the per-bot hunt game (bot's own shape + fixed colour; Gibson = round ivory). Same centre as the old
 * chip; the sprite fits inside the old chip's patch rect. DGBOT is a host gate log entry only. */
static void dg_bot(Gfx g, int cx, int cy, int scale)
{
    const bool gib = s_dg_bot >= 12;
    const int shape = gib ? 0 : botcorps_shape_for(s_dg_bot);
#ifdef BOTCORPS_HOST_RENDER
    static const int8_t SW[4] = { 7, 9, 7, 5 }, SH[4] = { 7, 6, 9, 5 };
    GATE_BOX(g, "DGBOT", gib ? "gibson" : "bot", cx - SW[shape] * scale / 2, cy - SH[shape] * scale / 2,
             SW[shape] * scale, SH[shape] * scale, "-");
#endif
    draw_blob(g, cx, cy, scale, gib ? (uint8_t)PAL_IVORY : bot_color(s_dg_bot), shape, false);
}

static uint32_t dg_rng(void) { s_dg_rng = s_dg_rng * 1664525u + 1013904223u; return s_dg_rng; }
static float dg_rngf(void) { return (float)(dg_rng() >> 8) / 16777216.f; }

/* the rain's own 3x5 bit art (botcorps_rain_ghost_ex B0/B1), scaled */
static const uint8_t DG_B0[5] = { 7, 5, 5, 5, 7 };
static const uint8_t DG_B1[5] = { 2, 6, 2, 2, 7 };
static void dg_bit(Gfx g, int x, int y, bool one, int s, uint8_t ink)
{
    const uint8_t *b = one ? DG_B1 : DG_B0;
    for (int j = 0; j < 5; ++j)
        for (int i = 0; i < 3; ++i)
            if (b[j] & (4 >> i)) g.fillRect(x + i * s, y + j * s, s, s, ink);
}
static uint8_t dg_fade(uint8_t ink, int pct) { return sb_mix(ink, botcorps_rgb565(PAL_BLACK), pct); }

/* ---------- 1A Rain Catch (reuses the #5 hunt game's per-bot device kinds bh_kind + BHDrop) ---------- */
static const int RC_L = 8, RC_LW = 37, RC_X0 = 12, RC_BOTY = 220, RC_TOP = 210, RC_MAX = 6;
static BHDrop s_rc_d[RC_MAX];      /* x = lane, k >= 0 device icon, k = -1 / -2 plain bit 0 / 1 */
static int s_rc_lane = 3;
static int rc_cx(int lane) { return RC_X0 + lane * RC_LW + RC_LW / 2; }

static void rc_reset(void)
{
    for (int i = 0; i < RC_MAX; ++i) s_rc_d[i].live = 0;
    s_rc_lane = 3;
}
static void rc_step(float dt)
{
    int nlive = 0;
    for (int i = 0; i < RC_MAX; ++i) if (s_rc_d[i].live) ++nlive;
    const float rate = 1.25f + (float)s_dg_score * 0.04f;
    if (nlive < RC_MAX && dg_rngf() < dt * rate) {
        const int lane = (int)(dg_rng() % (uint32_t)RC_L);
        bool clear = true;
        for (int i = 0; i < RC_MAX; ++i)
            if (s_rc_d[i].live && (int)s_rc_d[i].x == lane && s_rc_d[i].y < (float)(DG_PY + 40)) clear = false;
        if (clear)
            for (int i = 0; i < RC_MAX; ++i) if (!s_rc_d[i].live) {
                BHDrop &d = s_rc_d[i];
                d.live = 1;
                d.x = (float)lane;
                d.y = (float)(DG_PY - 12);
                d.v = 34.f + dg_rngf() * 14.f + (float)s_dg_score * 1.2f;
                d.k = dg_rngf() < 0.6f ? (int8_t)bh_kind(s_dg_bot, dg_rng()) : (int8_t)((dg_rng() & 1u) ? -2 : -1);
                break;
            }
    }
    for (int i = 0; i < RC_MAX; ++i) {
        BHDrop &d = s_rc_d[i];
        if (!d.live) continue;
        d.y += d.v * dt;
        if (d.y + 12.f >= (float)RC_TOP && d.y < (float)(RC_BOTY + 6) && (int)d.x == s_rc_lane) {
            if (d.k >= 0) ++s_dg_score; else ++s_dg_junk;       /* caught: your icon / a plain bit */
            d.live = 0;
            continue;
        }
        if (d.y >= (float)(DG_PY + DG_PH - 12)) {               /* reached the floor */
            if (d.k >= 0) ++s_dg_miss;
            d.live = 0;
        }
    }
    if (s_dg_score >= 20) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3 || s_dg_junk >= 3) s_dg_state = DGS_LOST;
}
static void rc_paint(Gfx g)
{
    for (int l = 1; l < RC_L; ++l)                                /* strand guides (faint rain columns) */
        for (int y = DG_PY + 4; y < RC_TOP - 4; y += 14) g.fillRect(RC_X0 + l * RC_LW, y, 1, 2, PAL_DEEP);
    for (int i = 0; i < RC_MAX; ++i) {
        const BHDrop &d = s_rc_d[i];
        if (!d.live) continue;
        const int cx = rc_cx((int)d.x), y = (int)lroundf(d.y);
        for (int k = 1; k <= 3; ++k) {                            /* fading 0/1 strand trail above */
            const int ty = y - k * 7;
            if (ty >= DG_PY) dg_bit(g, cx - 1, ty, ((i + k) & 1) != 0, 1, dg_fade(PAL_CODE, 80 - k * 20));
        }
        GATE_BOX(g, "DGOBJ", d.k >= 0 ? "icon" : "bit", cx - 6, y < DG_PY ? DG_PY : y, 12, 12, "-");
        if (d.k >= 0) {
            s_gl_clip_y = DG_PY;
            gl_icon(g, d.k, cx - 6, y, false);
            s_gl_clip_y = -32768;
        } else if (y >= DG_PY) {
            dg_bit(g, cx - 3, y + 1, d.k == -2, 2, PAL_HEAD);
        }
    }
    dg_bot(g, rc_cx(s_rc_lane), RC_BOTY, 2);
}

/* ---------- 2A Beacon Sort ---------- */
static const int BS_MAX = 3, BS_CX = 160, BS_BOTY = 220, BS_TOP = 208;
struct DGBeacon { float y, fx; int8_t k, tag, live, fly; };   /* fly: 0 falling, -1/+1 sorted left/right */
static DGBeacon s_bs[BS_MAX];
static const int8_t BS_KIND[4] = { GL_SPEAKER, GL_WATCH, GL_TRACKER, GL_PHONE };
static float bs_speed(void) { return 30.f + 8.f * (float)(s_dg_score / 5); }
static void bs_reset(void) { for (int i = 0; i < BS_MAX; ++i) s_bs[i].live = 0; }
static int bs_lowest(void)
{
    int b = -1;
    for (int i = 0; i < BS_MAX; ++i)
        if (s_bs[i].live && !s_bs[i].fly && (b < 0 || s_bs[i].y > s_bs[b].y)) b = i;
    return b;
}
static void bs_sort(bool trusted)
{
    const int b = bs_lowest();
    if (b < 0 || s_dg_state != DGS_PLAY) return;
    const bool right = (s_bs[b].tag == 1) == trusted;
    if (right) ++s_dg_score; else ++s_dg_miss;
    s_bs[b].fly = trusted ? -1 : 1;
    s_bs[b].fx = 0.f;
    if (!right) s_bs[b].tag = (int8_t)(s_bs[b].tag | 2);         /* mark: drawn dim while it flies out */
}
static void bs_step(float dt)
{
    bool room = true;
    for (int i = 0; i < BS_MAX; ++i)
        if (s_bs[i].live && !s_bs[i].fly && s_bs[i].y < (float)(DG_PY + 64)) room = false;
    if (room)
        for (int i = 0; i < BS_MAX; ++i) if (!s_bs[i].live) {
            s_bs[i].live = 1; s_bs[i].fly = 0; s_bs[i].fx = 0.f;
            s_bs[i].y = (float)(DG_PY - 12);
            s_bs[i].k = BS_KIND[dg_rng() % 4u];
            s_bs[i].tag = (int8_t)(dg_rng() & 1u);
            break;
        }
    const float v = bs_speed();
    for (int i = 0; i < BS_MAX; ++i) {
        DGBeacon &b = s_bs[i];
        if (!b.live) continue;
        if (b.fly) {
            b.fx += 260.f * dt;
            if (b.fx > 150.f) b.live = 0;
            continue;
        }
        b.y += v * dt;
        if (b.y + 12.f >= (float)BS_TOP) { ++s_dg_miss; b.live = 0; }   /* reached the bot unsorted */
    }
    if (s_dg_score >= 25) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void bs_rect(int i, int *x, int *y)
{
    const DGBeacon &b = s_bs[i];
    *x = BS_CX - 6 + (int)lroundf((float)b.fly * b.fx);
    *y = (int)lroundf(b.y);
}
static void bs_paint(Gfx g)
{
    for (int y = DG_PY + 4; y < BS_TOP - 4; y += 14) g.fillRect(BS_CX, y, 1, 2, PAL_DEEP);   /* the strand */
    for (int i = 0; i < BS_MAX; ++i) {
        const DGBeacon &b = s_bs[i];
        if (!b.live) continue;
        int x, y;
        bs_rect(i, &x, &y);
        if (x + 22 <= DG_PX || x >= DG_PX + DG_PW) continue;
        GATE_BOX(g, "DGOBJ", "beacon", x < DG_PX ? DG_PX : x, y < DG_PY ? DG_PY : y, 12, 12, "-");
        const bool dim = (b.tag & 2) != 0;
        s_gl_clip_y = DG_PY;
        gl_icon(g, b.k, x, y, dim);
        s_gl_clip_y = -32768;
        if (y >= DG_PY && x + 15 + 6 <= DG_PX + DG_PW && x + 15 >= DG_PX)
            dg_bit(g, x + 15, y + 1, (b.tag & 1) != 0, 2, dim ? (uint8_t)PAL_LINE : (uint8_t)PAL_HEAD);
    }
    dg_bot(g, BS_CX, BS_BOTY, 2);
}

/* ---------- 3B Firewall Wall ---------- */
static const int FW_C = 8, FW_R = 4, FW_X0 = 12, FW_BW = 35, FW_BH = 12, FW_Y0 = 34, FW_PY = 204, FW_ICY = 220;
static uint32_t s_fw_alive = 0;
static float s_fw_px = 160.f, s_fw_bx = 0.f, s_fw_by = 0.f, s_fw_vx = 0.f, s_fw_vy = 0.f;
static bool s_fw_serve = true;
static uint8_t s_fw_dim = 0;
static const int8_t FW_IOT[FW_C] = { GL_CAMERA, GL_TV, GL_SPEAKER, GL_CAR, GL_CAMERA, GL_PRINTER, GL_TV, GL_SPEAKER };
static const float FW_V = 125.f;
static void fw_reset(void)
{
    static_assert(FW_C * FW_R == 32, "block mask is one 32-bit word");
    s_fw_alive = 0xFFFFFFFFu;                                    /* all 32 blocks (no 1u << 32 UB) */
    s_fw_px = 160.f; s_fw_serve = true; s_fw_dim = 0;
}
static void fw_park(void) { s_fw_bx = s_fw_px - 3.f; s_fw_by = (float)(FW_PY - 11 - 11); s_fw_vx = s_fw_vy = 0.f; }
static void fw_launch(void)
{
    s_fw_serve = false;
    const float a = -0.6f + dg_rngf() * 1.2f;                    /* +-34 degrees off vertical */
    s_fw_vx = FW_V * sinf(a);
    s_fw_vy = -FW_V * cosf(a);
}
static void fw_step(float dt)
{
    if (s_fw_serve) { fw_park(); if (s_dg_launch) fw_launch(); return; }
    const int sub = 3;
    for (int s = 0; s < sub; ++s) {
        const float h = dt / (float)sub;
        s_fw_bx += s_fw_vx * h;
        s_fw_by += s_fw_vy * h;
        if (s_fw_bx < (float)DG_PX) { s_fw_bx = (float)DG_PX; s_fw_vx = fabsf(s_fw_vx); }
        if (s_fw_bx + 6.f > (float)(DG_PX + DG_PW)) { s_fw_bx = (float)(DG_PX + DG_PW - 6); s_fw_vx = -fabsf(s_fw_vx); }
        if (s_fw_by < (float)DG_PY) { s_fw_by = (float)DG_PY; s_fw_vy = fabsf(s_fw_vy); }
        /* hostile blocks */
        for (int r = 0; r < FW_R; ++r)
            for (int c = 0; c < FW_C; ++c) {
                const int i = r * FW_C + c;
                if (!(s_fw_alive & (1u << i))) continue;
                const float x0 = (float)(FW_X0 + c * (FW_BW + 2)), y0 = (float)(FW_Y0 + r * (FW_BH + 4));
                if (s_fw_bx + 6.f <= x0 || s_fw_bx >= x0 + FW_BW || s_fw_by + 10.f <= y0 || s_fw_by >= y0 + FW_BH) continue;
                s_fw_alive &= ~(1u << i);
                ++s_dg_score;
                const float ox = fminf(s_fw_bx + 6.f - x0, x0 + FW_BW - s_fw_bx);
                const float oy = fminf(s_fw_by + 10.f - y0, y0 + FW_BH - s_fw_by);
                if (ox < oy) s_fw_vx = -s_fw_vx; else s_fw_vy = -s_fw_vy;
                r = FW_R; break;
            }
        /* the bot paddle */
        if (s_fw_vy > 0.f && s_fw_by + 10.f >= (float)(FW_PY - 11) && s_fw_by + 10.f <= (float)(FW_PY + 2)
            && fabsf(s_fw_bx + 3.f - s_fw_px) <= 18.f) {
            const float off = (s_fw_bx + 3.f - s_fw_px) / 18.f;       /* -1..1 */
            const float a = off * 1.0f;
            s_fw_vx = FW_V * sinf(a);
            s_fw_vy = -FW_V * cosf(a);
            s_fw_by = (float)(FW_PY - 11 - 10);
        }
        if (s_fw_by > (float)(FW_ICY - 4)) {                          /* packet slipped past to the IoT row */
            int c = (int)((s_fw_bx + 3.f - (float)FW_X0) / (float)(FW_BW + 2));
            if (c < 0) c = 0;
            if (c >= FW_C) c = FW_C - 1;
            s_fw_dim |= (uint8_t)(1u << c);
            ++s_dg_miss;
            s_fw_serve = true;
            fw_park();
            break;
        }
    }
    if (!s_fw_alive) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void fw_paint(Gfx g)
{
    static const uint8_t ROWINK[FW_R] = { PAL_HEAD, PAL_OK, PAL_CODE, PAL_CODE };
    for (int r = 0; r < FW_R; ++r)
        for (int c = 0; c < FW_C; ++c) {
            if (!(s_fw_alive & (1u << (r * FW_C + c)))) continue;
            const int x0 = FW_X0 + c * (FW_BW + 2), y0 = FW_Y0 + r * (FW_BH + 4);
            GATE_BOX(g, "DGOBJ", "block", x0, y0, FW_BW, FW_BH, "-");
            const uint8_t ink = r == 3 ? dg_fade(PAL_CODE, 70) : ROWINK[r];
            for (int k = 0; k < 4; ++k) dg_bit(g, x0 + 2 + k * 8, y0 + 1, true, 2, ink);   /* "1111" */
        }
    for (int c = 0; c < FW_C; ++c) {
        const int x = FW_X0 + c * (FW_BW + 2) + FW_BW / 2 - 6;
        GATE_BOX(g, "DGOBJ", "iot", x, FW_ICY, 12, 12, "-");
        gl_icon(g, FW_IOT[c], x, FW_ICY, (s_fw_dim >> c) & 1u);
    }
    dg_bot(g, (int)lroundf(s_fw_px), FW_PY, 2);
    const int bx = (int)lroundf(s_fw_bx), by = (int)lroundf(s_fw_by);
    GATE_BOX(g, "DGOBJ", "packet", bx, by, 6, 10, "-");
    dg_bit(g, bx, by, false, 2, PAL_WHITE);                        /* the bit packet */
}

/* ---------- 4A Gibson Maze ---------- */
static const int MZ_C = 19, MZ_R = 13, MZ_T = 15, MZ_X = 17, MZ_Y = 29, MZ_G = 2;
static const char *const MZ_MAP[MZ_R] = {
    "###################",
    "#o.......#.......o#",
    "#.##.###.#.###.##.#",
    "#.................#",
    "#.##.#.#####.#.##.#",
    "#....#...#...#....#",
    "####.###.#.###.####",
    "#.......J.J.......#",
    "#.##.#.#####.#.##.#",
    "#o.#.....G.....#.o#",
    "#..#.#.##.##.#.#..#",
    "#.................#",
    "###################",
};
static const int8_t MZ_DX[4] = { 0, 0, -1, 1 }, MZ_DY[4] = { -1, 1, 0, 0 };   /* up down left right */
static const int8_t MZ_POW[4] = { GL_AP, GL_CAMERA, GL_PHONE, GL_TV };
struct DGMover { float x, y; int8_t dir, want; };
static uint8_t s_mz_pel[MZ_R][MZ_C];   /* 1 bit-pellet, 2 device icon (shield) */
static int s_mz_left = 0;
static DGMover s_mz_p, s_mz_j[MZ_G];
static uint32_t s_mz_fright = 0, s_mz_t0 = 0;
static bool mz_open(int tx, int ty) { return tx >= 0 && ty >= 0 && tx < MZ_C && ty < MZ_R && MZ_MAP[ty][tx] != '#'; }
static void mz_place(void)
{
    int gi = 0;
    for (int y = 0; y < MZ_R; ++y)
        for (int x = 0; x < MZ_C; ++x) {
            const char ch = MZ_MAP[y][x];
            if (ch == 'G') { s_mz_p.x = (float)(x * MZ_T + 7); s_mz_p.y = (float)(y * MZ_T + 7); s_mz_p.dir = -1; s_mz_p.want = -1; }
            if (ch == 'J' && gi < MZ_G) { s_mz_j[gi].x = (float)(x * MZ_T + 7); s_mz_j[gi].y = (float)(y * MZ_T + 7); s_mz_j[gi].dir = 0; ++gi; }
        }
}
static void mz_reset(void)
{
    s_mz_left = 0;
    for (int y = 0; y < MZ_R; ++y)
        for (int x = 0; x < MZ_C; ++x) {
            const char ch = MZ_MAP[y][x];
            s_mz_pel[y][x] = ch == '.' ? 1 : ch == 'o' ? 2 : 0;
            if (s_mz_pel[y][x]) ++s_mz_left;
        }
    s_mz_fright = 0;
    mz_place();
}
/* move along the grid; at each tile centre call pick(). Never overshoots a centre. */
static void mz_move(DGMover &m, float dist, int who)
{
    int guard = 0;
    while (dist > 0.001f && guard++ < 8) {
        const bool atc = ((int)lroundf(m.x) % MZ_T) == 7 && ((int)lroundf(m.y) % MZ_T) == 7
                         && fabsf(m.x - lroundf(m.x)) < 0.001f && fabsf(m.y - lroundf(m.y)) < 0.001f;
        if (atc) {
            const int tx = (int)m.x / MZ_T, ty = (int)m.y / MZ_T;
            if (who < 0) {                                     /* the player: queued turn, else keep going */
                if (s_mz_pel[ty][tx]) {
                    if (s_mz_pel[ty][tx] == 2) s_mz_fright = s_dg_t + 5000u;
                    s_mz_pel[ty][tx] = 0; --s_mz_left; ++s_dg_score;
                }
                if (m.want >= 0 && mz_open(tx + MZ_DX[m.want], ty + MZ_DY[m.want])) m.dir = m.want;
                if (m.dir >= 0 && !mz_open(tx + MZ_DX[m.dir], ty + MZ_DY[m.dir])) m.dir = -1;
            } else {                                           /* a jammer: no reversing, chase or wander */
                const int rev = m.dir < 0 ? -1 : (m.dir ^ 1);
                const int ptx = (int)s_mz_p.x / MZ_T, pty = (int)s_mz_p.y / MZ_T;
                const bool fr = s_dg_t < s_mz_fright;
                const bool chase = !fr && dg_rngf() < (who == 0 ? 0.8f : 0.55f);
                int best = -1; int bd = 1 << 30; int nopt = 0; int opt[4];
                for (int d = 0; d < 4; ++d) {
                    if (d == rev || !mz_open(tx + MZ_DX[d], ty + MZ_DY[d])) continue;
                    opt[nopt++] = d;
                    const int ex = tx + MZ_DX[d] - ptx, ey = ty + MZ_DY[d] - pty;
                    const int q = ex * ex + ey * ey;
                    if (q < bd) { bd = q; best = d; }
                }
                if (nopt == 0) m.dir = (int8_t)rev;
                else m.dir = (int8_t)(chase ? best : opt[dg_rng() % (uint32_t)nopt]);
            }
            if (m.dir < 0) return;
        }
        /* distance to the next centre along dir */
        const float cxn = (float)(((int)m.x / MZ_T) * MZ_T + 7), cyn = (float)(((int)m.y / MZ_T) * MZ_T + 7);
        float rem;
        if (MZ_DX[m.dir]) {
            const float tgt = fabsf(m.x - cxn) < 0.001f ? cxn + (float)(MZ_DX[m.dir] * MZ_T) : (MZ_DX[m.dir] > 0 ? (m.x < cxn ? cxn : cxn + MZ_T) : (m.x > cxn ? cxn : cxn - MZ_T));
            rem = fabsf(tgt - m.x);
            const float s = dist < rem ? dist : rem;
            m.x += (float)MZ_DX[m.dir] * s; dist -= s;
            if (s >= rem) m.x = tgt;
        } else {
            const float tgt = fabsf(m.y - cyn) < 0.001f ? cyn + (float)(MZ_DY[m.dir] * MZ_T) : (MZ_DY[m.dir] > 0 ? (m.y < cyn ? cyn : cyn + MZ_T) : (m.y > cyn ? cyn : cyn - MZ_T));
            rem = fabsf(tgt - m.y);
            const float s = dist < rem ? dist : rem;
            m.y += (float)MZ_DY[m.dir] * s; dist -= s;
            if (s >= rem) m.y = tgt;
        }
    }
}
static void mz_step(float dt)
{
    if (s_dg_t < s_dg_hold) return;
    mz_move(s_mz_p, 60.f * dt, -1);
    const bool fr = s_dg_t < s_mz_fright;
#ifdef BOTCORPS_HOST_RENDER
    const int ng = s_dg_noghost ? 0 : MZ_G;
#else
    const int ng = MZ_G;
#endif
    for (int i = 0; i < ng; ++i) {
        if (s_dg_t - s_mz_t0 < (uint32_t)(1500 * (i + 1))) continue;   /* jammers leave one by one */
        mz_move(s_mz_j[i], (fr ? 30.f : 48.f) * dt, i);
        if (fabsf(s_mz_j[i].x - s_mz_p.x) < 9.f && fabsf(s_mz_j[i].y - s_mz_p.y) < 9.f) {
            if (fr) {                                          /* the bot chases jammers off the board */
                s_dg_score += 5;
                for (int y = 0; y < MZ_R; ++y) for (int x = 0; x < MZ_C; ++x)
                    if (MZ_MAP[y][x] == 'J') { s_mz_j[i].x = (float)(x * MZ_T + 7); s_mz_j[i].y = (float)(y * MZ_T + 7); }
                s_mz_j[i].dir = 0;
            } else {                                           /* caught */
                ++s_dg_miss;
                s_dg_hold = s_dg_t + 900u;
                s_mz_t0 = s_dg_t;
                mz_place();
                break;
            }
        }
    }
    if (s_mz_left <= 0) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void mz_paint(Gfx g)
{
    const uint8_t wire = PAL_WIRE_C, core = PAL_CYAN;
    for (int y = 0; y < MZ_R; ++y)                         /* walls as PCB traces */
        for (int x = 0; x < MZ_C; ++x) {
            if (MZ_MAP[y][x] != '#') continue;
            const int cx = MZ_X + x * MZ_T + 7, cy = MZ_Y + y * MZ_T + 7;
            g.fillRect(cx - 2, cy - 2, 5, 5, wire);
            if (x + 1 < MZ_C && MZ_MAP[y][x + 1] == '#') { g.fillRect(cx, cy - 2, MZ_T, 5, wire); g.fillRect(cx, cy, MZ_T, 1, core); }
            if (y + 1 < MZ_R && MZ_MAP[y + 1][x] == '#') { g.fillRect(cx - 2, cy, 5, MZ_T, wire); g.fillRect(cx, cy, 1, MZ_T, core); }
            g.fillRect(cx, cy, 1, 1, core);
        }
    for (int y = 0; y < MZ_R; ++y)
        for (int x = 0; x < MZ_C; ++x) {
            const int px = MZ_X + x * MZ_T, py = MZ_Y + y * MZ_T;
            if (s_mz_pel[y][x] == 1) dg_bit(g, px + 6, py + 5, ((x + y) & 1) != 0, 1, PAL_CODE);
            else if (s_mz_pel[y][x] == 2) {
                GATE_BOX(g, "DGOBJ", "shield", px + 1, py + 1, 12, 12, "-");
                int k = (x < MZ_C / 2 ? 0 : 1) + (y < MZ_R / 2 ? 0 : 2);
                gl_icon(g, MZ_POW[k], px + 1, py + 1, false);
            }
        }
    const bool fr = s_dg_t < s_mz_fright;
    for (int i = 0; i < MZ_G; ++i) {
#ifdef BOTCORPS_HOST_RENDER
        if (s_dg_noghost) break;
#endif
        const int jx = MZ_X + (int)lroundf(s_mz_j[i].x) - 6, jy = MZ_Y + (int)lroundf(s_mz_j[i].y) - 6;
        GATE_BOX(g, "DGOBJ", "jammer", jx, jy, 12, 12, "-");
        dev_icon(g, DEV_DEAUTH, jx, jy, fr);
    }
    dg_bot(g, MZ_X + (int)lroundf(s_mz_p.x), MZ_Y + (int)lroundf(s_mz_p.y), 1);
}

/* ===== v1019m (Dirt: FINISHED build, all 12 concepts fit the measured budget): eight more Defend games,
 * three per device category. Same framework, same art (round in-game bots, 0/1 rain bits, gl_icon /
 * dev_icon device icons, matte-black board), no new chrome. Each category's first game is the v1019j one
 * (every Defend entry still starts it); Tab steps to the next game of that category, and `,` `/` step
 * prev / next on the WIN / LOST / PAUSE stop. ---------------------------------------------------------- */

/* ---------- 1C Evil Twin Spotter: a pair of the bot's Wi-Fi icons drops; the twin's label bit flickers ---------- */
static const int TW_LX = 110, TW_RX = 210, TW_BOTY = 220, TW_FLOOR = 200;
static float s_tw_y = 0.f;
static int8_t s_tw_k = GL_AP, s_tw_side = 0, s_tw_live = 0;
static uint32_t s_tw_wait = 0;
static void tw_reset(void) { s_tw_live = 0; s_tw_wait = 0; }
static bool tw_flick(void) { return ((s_dg_ms / 300u) & 1u) != 0; }
static void tw_pick(int side)
{
    if (!s_tw_live || s_dg_state != DGS_PLAY) return;
    if (side == s_tw_side) ++s_dg_score; else ++s_dg_miss;
    s_tw_live = 0;
    s_tw_wait = s_dg_ms + 450u;
}
static void tw_step(float dt)
{
    if (!s_tw_live && s_dg_ms >= s_tw_wait) {
        s_tw_live = 1;
        s_tw_y = (float)(DG_PY - 12);
        s_tw_k = (int8_t)bh_kind(s_dg_bot, dg_rng());
        s_tw_side = (int8_t)(dg_rng() & 1u);
    }
    if (s_tw_live) {
        s_tw_y += (28.f + 2.f * (float)s_dg_score) * dt;
        if (s_tw_y + 26.f >= (float)TW_FLOOR) { ++s_dg_miss; s_tw_live = 0; s_tw_wait = s_dg_ms + 450u; }
    }
    if (s_dg_score >= 15) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void tw_paint(Gfx g)
{
    for (int y = DG_PY + 4; y < TW_FLOOR - 4; y += 14) { g.fillRect(TW_LX, y, 1, 2, PAL_DEEP); g.fillRect(TW_RX, y, 1, 2, PAL_DEEP); }
    if (s_tw_live) {
        const int y = (int)lroundf(s_tw_y);
        for (int s = 0; s < 2; ++s) {
            const int cx = s ? TW_RX : TW_LX;
            GATE_BOX(g, "DGOBJ", s == s_tw_side ? "twin" : "ap", cx - 6, y < DG_PY ? DG_PY : y, 12, 12, "-");
            s_gl_clip_y = DG_PY;
            gl_icon(g, s_tw_k, cx - 6, y, false);
            s_gl_clip_y = -32768;
            const bool one = s == s_tw_side ? tw_flick() : true;   /* the twin's label bit flickers */
            if (y + 15 >= DG_PY) dg_bit(g, cx - 3, y + 15, one, 2, PAL_HEAD);
        }
    }
    dg_bot(g, 160, TW_BOTY, 2);
}

/* ---------- 1B Channel Hop: five Wi-Fi channel lanes, dense 1-columns of interference roll down ---------- */
static const int CH_L = 5, CH_LW = 56, CH_X0 = 20, CH_BOTY = 220, CH_MAX = 6;
struct DGDrop { float y; int8_t lane, live; };
static DGDrop s_ch[CH_MAX];
static int s_ch_lane = 2, s_ch_boost = 1;
static float s_ch_next = 0.f;
static int ch_cx(int l) { return CH_X0 + l * CH_LW + CH_LW / 2; }
static void ch_reset(void) { for (int i = 0; i < CH_MAX; ++i) s_ch[i].live = 0; s_ch_lane = 2; s_ch_boost = 1; s_ch_next = 0.6f; }
static void ch_step(float dt)
{
    const float secs = (float)s_dg_ms / 1000.f;
    s_ch_next -= dt;
    if (s_ch_next <= 0.f) {
        s_ch_next = 0.95f - secs * 0.006f;
        if (s_ch_next < 0.55f) s_ch_next = 0.55f;
        int busy = 0; bool used[CH_L] = { false };
        for (int i = 0; i < CH_MAX; ++i)
            if (s_ch[i].live && s_ch[i].y < (float)(DG_PY + 60)) { if (!used[s_ch[i].lane]) ++busy; used[s_ch[i].lane] = true; }
        const int lane = (int)(dg_rng() % (uint32_t)CH_L);
        if (!used[lane] && busy < CH_L - 2)                 /* always at least two clean channels */
            for (int i = 0; i < CH_MAX; ++i) if (!s_ch[i].live) {
                s_ch[i].live = 1; s_ch[i].lane = (int8_t)lane; s_ch[i].y = (float)(DG_PY - 34);
                break;
            }
    }
    const float v = 62.f + secs * 0.6f;
    for (int i = 0; i < CH_MAX; ++i) {
        DGDrop &d = s_ch[i];
        if (!d.live) continue;
        d.y += v * dt;
        if (d.lane == s_ch_lane && d.y + 34.f >= (float)(CH_BOTY - 9) && d.y <= (float)(CH_BOTY + 9)) { ++s_dg_miss; d.live = 0; continue; }
        if (d.y >= (float)(DG_PY + DG_PH - 34)) { ++s_dg_score; d.live = 0; }
    }
    if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
    else if (s_dg_ms >= 60000u) s_dg_state = DGS_WIN;
}
static void ch_boost(void)
{
    if (!s_ch_boost || s_dg_state != DGS_PLAY) return;
    s_ch_boost = 0;
    for (int i = 0; i < CH_MAX; ++i) s_ch[i].live = 0;
}
static void ch_paint(Gfx g)
{
    for (int l = 1; l < CH_L; ++l)
        for (int y = DG_PY + 4; y < DG_PY + DG_PH - 4; y += 14) g.fillRect(CH_X0 + l * CH_LW, y, 1, 2, PAL_DEEP);
    for (int i = 0; i < CH_MAX; ++i) {
        const DGDrop &d = s_ch[i];
        if (!d.live) continue;
        const int cx = ch_cx(d.lane), y = (int)lroundf(d.y);
        GATE_BOX(g, "DGOBJ", "noise", cx - 8, y < DG_PY ? DG_PY : y, 16, y < DG_PY ? 34 - (DG_PY - y) : 34, "-");
        for (int k = 0; k < 3; ++k) {
            const int by = y + k * 12;
            if (by < DG_PY) continue;
            const uint8_t ink = k == 2 ? (uint8_t)PAL_HEAD : dg_fade(PAL_CODE, 40 + k * 30);
            dg_bit(g, cx - 8, by, true, 2, ink);
            dg_bit(g, cx + 2, by, true, 2, ink);
        }
    }
    dg_bot(g, ch_cx(s_ch_lane), CH_BOTY, 2);
}

/* ---------- 2B Pair Lock: type the 4-digit pairing PIN before the rogue tracker reaches the bot ---------- */
static const int PL_BOTY = 220, PL_TOP = 88, PL_HIT = 206;
static char s_pl_pin[5] = "0000", s_pl_in[5] = "";
static float s_pl_y = 0.f;
static void pl_new(void)
{
    for (int i = 0; i < 4; ++i) s_pl_pin[i] = (char)('0' + dg_rng() % 10u);
    s_pl_pin[4] = 0;
    s_pl_in[0] = 0;
    s_pl_y = (float)PL_TOP;
}
static void pl_reset(void) { pl_new(); }
static void pl_type(char c)
{
    const size_t n = strlen(s_pl_in);
    if (n < 4) { s_pl_in[n] = c; s_pl_in[n + 1] = 0; }
}
static void pl_enter(void)
{
    if (strlen(s_pl_in) < 4) return;
    if (strcmp(s_pl_in, s_pl_pin) == 0) { ++s_dg_score; pl_new(); }
    else s_pl_in[0] = 0;                                      /* wrong PIN: entry cleared, try again */
}
static void pl_step(float dt)
{
    s_pl_y += (13.f + 1.5f * (float)s_dg_score) * dt;
    if (s_pl_y + 12.f >= (float)PL_HIT) { ++s_dg_miss; pl_new(); }
    if (s_dg_score >= 10) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void pl_paint(Gfx g)
{
    const int keep = s_px;
    s_px = 8;
    char b[12];
    snprintf(b, sizeof b, "%c %c %c %c", s_pl_pin[0], s_pl_pin[1], s_pl_pin[2], s_pl_pin[3]);
    GATE_BOX(g, "DGOBJ", "pin", 124, 38, 56, 8, "-");
    txt(g, b, 124, 38, PAL_CODE, lgfx::textdatum::top_left);
    const int typed = (int)strlen(s_pl_in);
    for (int i = 0; i < 4; ++i) b[i * 2] = i < typed ? s_pl_in[i] : '_';
    txt(g, b, 124, 60, PAL_HEAD, lgfx::textdatum::top_left);
    s_px = keep;
    for (int y = PL_TOP + 14; y < PL_HIT - 4; y += 14) g.fillRect(160, y, 1, 2, PAL_DEEP);
    const int y = (int)lroundf(s_pl_y);
    for (int k = 1; k <= 3; ++k) {
        const int ty = y - k * 7;
        if (ty >= PL_TOP - 4) dg_bit(g, 159, ty, (k & 1) != 0, 1, dg_fade(PAL_CODE, 80 - k * 20));
    }
    GATE_BOX(g, "DGOBJ", "rogue", 154, y, 12, 12, "-");
    gl_icon(g, GL_TRACKER, 154, y, false);
    dg_bot(g, 160, PL_BOTY, 2);
}

/* ---------- 2C Range Ring: BLE icons drift in from the four sides; ping that side before the inner ring ---------- */
static const int RR_CX = 160, RR_CY = 127, RR_IN = 26, RR_OUT = 92;
static float s_rr_d[4];               /* distance from centre per side (up down left right); < 0 = none */
static int8_t s_rr_k[4];
static int s_rr_wave = 0, s_rr_spawned = 0;
static float s_rr_next = 0.f;
static int rr_need(void) { return 6 + 2 * s_rr_wave; }
static void rr_reset(void) { for (int i = 0; i < 4; ++i) s_rr_d[i] = -1.f; s_rr_wave = 0; s_rr_spawned = 0; s_rr_next = 0.8f; }
static void rr_pos(int s, int *x, int *y)
{
    const int d = (int)lroundf(s_rr_d[s]);
    *x = RR_CX + MZ_DX[s] * d - 6;
    *y = RR_CY + MZ_DY[s] * d - 6;
}
static void rr_ping(int s)
{
    if (s < 0 || s > 3 || s_rr_d[s] < 0.f || s_dg_state != DGS_PLAY) return;
    s_rr_d[s] = -1.f;
    ++s_dg_score;
}
static void rr_step(float dt)
{
    int live = 0;
    for (int s = 0; s < 4; ++s) if (s_rr_d[s] >= 0.f) ++live;
    if (s_rr_spawned >= rr_need() && live == 0) {            /* wave cleared */
        if (s_rr_wave >= 2) { s_dg_state = DGS_WIN; return; }
        ++s_rr_wave; s_rr_spawned = 0; s_rr_next = 1.2f;
    }
    s_rr_next -= dt;
    if (s_rr_spawned < rr_need() && s_rr_next <= 0.f) {
        const int s = (int)(dg_rng() % 4u);
        if (s_rr_d[s] < 0.f) {
            s_rr_d[s] = (s < 2) ? 96.f : 140.f;
            s_rr_k[s] = BS_KIND[dg_rng() % 4u];
            ++s_rr_spawned;
            s_rr_next = 1.5f - 0.3f * (float)s_rr_wave;
        } else s_rr_next = 0.2f;
    }
    const float v = 16.f + 6.f * (float)s_rr_wave;
    for (int s = 0; s < 4; ++s) {
        if (s_rr_d[s] < 0.f) continue;
        s_rr_d[s] -= v * dt;
        if (s_rr_d[s] <= (float)(RR_IN + 7)) { s_rr_d[s] = -1.f; ++s_dg_miss; }
    }
    if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void rr_paint(Gfx g)
{
    g.drawCircle(RR_CX, RR_CY, RR_OUT, PAL_WIRE_C);
    g.drawCircle(RR_CX, RR_CY, RR_IN, PAL_CYAN);
    for (int s = 0; s < 4; ++s) {
        if (s_rr_d[s] < 0.f) continue;
        int x, y; rr_pos(s, &x, &y);
        GATE_BOX(g, "DGOBJ", "ble", x, y, 12, 12, "-");
        gl_icon(g, s_rr_k[s], x, y, false);
    }
    dg_bot(g, RR_CX, RR_CY, 2);
}

/* ---------- 3A Patch Run: a 5x3 grid of IoT icons; patch the blinking vulnerable ones before the bar runs out ---------- */
static const int PR_C = 5, PR_R = 3, PR_N = 15, PR_X0 = 15, PR_Y0 = 28, PR_CW = 58, PR_CH = 66, PR_BAR = 40;
static const int8_t PR_IOT[PR_N] = { GL_CAMERA, GL_TV, GL_SPEAKER, GL_PRINTER, GL_CAR, GL_SPEAKER, GL_CAMERA, GL_DEVBOARD,
                                      GL_TV, GL_CAMERA, GL_PRINTER, GL_CAR, GL_TV, GL_SPEAKER, GL_CAMERA };
static uint8_t s_pr_st[PR_N];         /* 0 secure, 1 vulnerable, 2 down (red) */
static uint32_t s_pr_due[PR_N], s_pr_len[PR_N];
static int s_pr_cur = 7;
static float s_pr_next = 0.f;
static void pr_reset(void) { memset(s_pr_st, 0, sizeof s_pr_st); s_pr_cur = 7; s_pr_next = 1.0f; }
static int pr_bar(int i)
{
    if (s_pr_st[i] != 1) return 0;
    const uint32_t left = s_pr_due[i] > s_dg_ms ? s_pr_due[i] - s_dg_ms : 0u;
    return (int)((uint32_t)PR_BAR * left / (s_pr_len[i] ? s_pr_len[i] : 1u));
}
static bool pr_blink(void) { return ((s_dg_ms / 250u) & 1u) != 0; }
static void pr_patch(void)
{
    if (s_dg_state != DGS_PLAY || s_pr_st[s_pr_cur] != 1) return;
    s_pr_st[s_pr_cur] = 0;
    ++s_dg_score;
}
static void pr_step(float dt)
{
    const float secs = (float)s_dg_ms / 1000.f;
    s_pr_next -= dt;
    int vul = 0;
    for (int i = 0; i < PR_N; ++i) if (s_pr_st[i] == 1) ++vul;
    if (s_pr_next <= 0.f) {
        s_pr_next = 2.2f - secs * 0.009f;
        if (s_pr_next < 1.4f) s_pr_next = 1.4f;
        if (vul < 3) {
            int i = (int)(dg_rng() % (uint32_t)PR_N);
            for (int k = 0; k < PR_N && s_pr_st[i] != 0; ++k) i = (i + 1) % PR_N;
            if (s_pr_st[i] == 0) {
                s_pr_st[i] = 1;
                s_pr_len[i] = secs < 45.f ? 7000u : 5500u;
                s_pr_due[i] = s_dg_ms + s_pr_len[i];
            }
        }
    }
    int down = 0;
    for (int i = 0; i < PR_N; ++i) {
        if (s_pr_st[i] == 1 && s_dg_ms >= s_pr_due[i]) { s_pr_st[i] = 2; ++s_dg_miss; }
        if (s_pr_st[i] == 2) ++down;
    }
    (void)down;
    if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
    else if (s_dg_ms >= 90000u) s_dg_state = DGS_WIN;
}
static void pr_cell(int i, int *x, int *y) { *x = PR_X0 + (i % PR_C) * PR_CW; *y = PR_Y0 + (i / PR_C) * PR_CH; }
static void pr_paint(Gfx g)
{
    for (int i = 0; i < PR_N; ++i) {
        int x, y; pr_cell(i, &x, &y);
        const int cx = x + PR_CW / 2;
        if (s_pr_st[i] == 2) g.drawRect(x + 2, y + 2, PR_CW - 4, PR_CH - 4, PAL_AMBER);
        GATE_BOX(g, "DGOBJ", s_pr_st[i] == 1 ? "vuln" : s_pr_st[i] == 2 ? "down" : "iot", cx - 6, y + 8, 12, 12, "-");
        gl_icon(g, PR_IOT[i], cx - 6, y + 8, s_pr_st[i] == 2 || (s_pr_st[i] == 1 && pr_blink()));
        if (s_pr_st[i] == 1) {
            const int w = pr_bar(i);
            g.fillRect(cx - PR_BAR / 2, y + 56, PR_BAR, 3, PAL_DEEP);
            if (w > 0) g.fillRect(cx - PR_BAR / 2, y + 56, w, 3, PAL_AMBER);
        }
        if (i == s_pr_cur) dg_bot(g, cx, y + 38, 2);
    }
}

/* ---------- 3C Botnet Breaker: infection drips down five strands; cut a strand (1-5) before 3 in a row fall ---------- */
static const int BN_N = 5, BN_X0 = 40, BN_DX = 60, BN_TOP = 40, BN_ICY = 192, BN_MAXB = 12, BN_BOTY = 220;
static uint8_t s_bn_n[BN_N], s_bn_inf[BN_N];
static int s_bn_at = 2;               /* the bot stands under the strand it last cut */
static float s_bn_t[BN_N], s_bn_rate[BN_N];
static void bn_reset(void)
{
    s_bn_at = 2;
    for (int i = 0; i < BN_N; ++i) { s_bn_n[i] = 0; s_bn_inf[i] = 0; s_bn_t[i] = -(0.5f + dg_rngf() * 2.5f); s_bn_rate[i] = 0.8f + dg_rngf() * 0.6f; }
}
static void bn_cut(int i)
{
    if (i < 0 || i >= BN_N || s_dg_state != DGS_PLAY) return;
    s_bn_at = i;
    if (s_bn_inf[i] || s_bn_n[i] == 0) return;
    s_bn_n[i] = 0;
    s_bn_t[i] = -(0.5f + dg_rngf() * 1.5f);
    s_bn_rate[i] = 0.7f + dg_rngf() * 0.6f;
    ++s_dg_score;
}
static void bn_step(float dt)
{
    for (int i = 0; i < BN_N; ++i) {
        if (s_bn_inf[i]) continue;
        s_bn_t[i] += dt;
        while (s_bn_t[i] >= s_bn_rate[i]) {
            s_bn_t[i] -= s_bn_rate[i];
            if (++s_bn_n[i] >= BN_MAXB) { s_bn_inf[i] = 1; s_bn_n[i] = 0; ++s_dg_miss; break; }
        }
    }
    for (int i = 0; i + 2 < BN_N; ++i)
        if (s_bn_inf[i] && s_bn_inf[i + 1] && s_bn_inf[i + 2]) { s_dg_state = DGS_LOST; return; }
    if (s_dg_ms >= 60000u) s_dg_state = DGS_WIN;
}
static void bn_paint(Gfx g)
{
    static const int8_t BN_IOT[BN_N] = { GL_CAMERA, GL_TV, GL_SPEAKER, GL_PRINTER, GL_CAMERA };
    const int keep = s_px;
    s_px = 6;
    for (int i = 0; i < BN_N; ++i) {
        const int cx = BN_X0 + i * BN_DX;
        char d[2] = { (char)('1' + i), 0 };
        txt(g, d, cx - 2, DG_PY + 4, PAL_MUTED, lgfx::textdatum::top_left);
        for (int y = BN_TOP + 4; y < BN_ICY - 4; y += 14) g.fillRect(cx, y, 1, 2, PAL_DEEP);
        for (int k = 0; k < s_bn_n[i]; ++k) {
            const bool head = k == s_bn_n[i] - 1;
            dg_bit(g, cx - 3, BN_TOP + k * 12, ((i + k) & 1) != 0, 2, head ? (uint8_t)PAL_HEAD : dg_fade(PAL_CODE, 30 + 50 * k / BN_MAXB));
        }
        if (s_bn_n[i]) GATE_BOX(g, "DGOBJ", "drip", cx - 3, BN_TOP, 6, s_bn_n[i] * 12 - 2, "-");
        GATE_BOX(g, "DGOBJ", s_bn_inf[i] ? "infected" : "iot", cx - 6, BN_ICY, 12, 12, "-");
        if (s_bn_inf[i]) {                                    /* turned into a rain-coloured block */
            g.fillRect(cx - 6, BN_ICY, 12, 12, dg_fade(PAL_CODE, 55));
            dg_bit(g, cx - 3, BN_ICY + 1, true, 2, PAL_CODE);
        } else gl_icon(g, BN_IOT[i], cx - 6, BN_ICY, false);
    }
    s_px = keep;
    dg_bot(g, BN_X0 + s_bn_at * BN_DX, BN_BOTY, 2);
}

/* ---------- 4B Jam Dodge: jammers sweep noise walls (rows of 1s) across six lanes; slip through the gap ---------- */
static const int JD_L = 6, JD_Y0 = 34, JD_LH = 32, JD_BX = 44, JD_WW = 20, JD_MAX = 3;
struct DGWall { float x; int8_t gap, live, hit; };
static DGWall s_jd[JD_MAX];
static int s_jd_lane = 2;
static int jd_cy(int l) { return JD_Y0 + l * JD_LH + JD_LH / 2; }
static void jd_reset(void) { for (int i = 0; i < JD_MAX; ++i) s_jd[i].live = 0; s_jd_lane = 2; }
static bool jd_open(const DGWall &w, int lane) { return lane == w.gap || lane == w.gap + 1; }
static void jd_step(float dt)
{
    const float secs = (float)s_dg_ms / 1000.f;
    float far = -1e9f;
    for (int i = 0; i < JD_MAX; ++i) if (s_jd[i].live && s_jd[i].x > far) far = s_jd[i].x;
    if (far < (float)(DG_PX + DG_PW - 150))
        for (int i = 0; i < JD_MAX; ++i) if (!s_jd[i].live) {
            s_jd[i].live = 1; s_jd[i].hit = 0; s_jd[i].x = (float)(DG_PX + DG_PW - JD_WW - 2);
            s_jd[i].gap = (int8_t)(dg_rng() % (uint32_t)(JD_L - 1));
            break;
        }
    const float v = 70.f + secs * 0.7f;
    for (int i = 0; i < JD_MAX; ++i) {
        DGWall &w = s_jd[i];
        if (!w.live) continue;
        w.x -= v * dt;
        if (!w.hit && w.x <= (float)(JD_BX + 9) && w.x + JD_WW >= (float)(JD_BX - 9) && !jd_open(w, s_jd_lane)) { w.hit = 1; ++s_dg_miss; }
        if (w.x < (float)DG_PX) { if (!w.hit) ++s_dg_score; w.live = 0; }
    }
    if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
    else if (s_dg_ms >= 45000u) s_dg_state = DGS_WIN;
}
static void jd_paint(Gfx g)
{
    for (int l = 1; l < JD_L; ++l)
        for (int x = DG_PX + 4; x < DG_PX + DG_PW - 4; x += 14) g.fillRect(x, JD_Y0 + l * JD_LH, 2, 1, PAL_DEEP);
    for (int i = 0; i < JD_MAX; ++i) {
        const DGWall &w = s_jd[i];
        if (!w.live) continue;
        const int x = (int)lroundf(w.x);
        GATE_BOX(g, "DGOBJ", "jammer", x + 4, DG_PY, 12, 12, "-");
        dev_icon(g, DEV_DEAUTH, x + 4, DG_PY, w.hit != 0);
        for (int l = 0; l < JD_L; ++l) {
            if (jd_open(w, l)) continue;
            const uint8_t ink = w.hit ? (uint8_t)PAL_LINE : dg_fade(PAL_CODE, 75);
            for (int k = 0; k < 3; ++k) dg_bit(g, x + k * 7, jd_cy(l) - 5, true, 2, ink);   /* a row of 1s */
        }
        GATE_BOX(g, "DGOBJ", "noise", x, JD_Y0, JD_WW, JD_L * JD_LH, "-");
    }
    dg_bot(g, JD_BX, jd_cy(s_jd_lane), 2);
}

/* ---------- 4C Signal Trace: a rogue hides in one of 9 cells; scan for hotter / colder, best of 5 rounds ---------- */
static const int ST_X0 = 16, ST_Y0 = 34, ST_CW = 96, ST_CH = 64, ST_SCANS = 4;
static const uint32_t ST_ROUND_MS = 15000u, ST_SHOW_MS = 1200u;
static int s_st_cur = 4, s_st_hid = 0, s_st_left = ST_SCANS, s_st_reveal = 0;
static int8_t s_st_heat[9];           /* -1 not scanned, else manhattan distance to the rogue */
static uint32_t s_st_t0 = 0, s_st_show = 0;
static void st_round(void)
{
    for (int i = 0; i < 9; ++i) s_st_heat[i] = -1;
    s_st_hid = (int)(dg_rng() % 9u);
    s_st_left = ST_SCANS;
    s_st_reveal = 0;
    s_st_t0 = s_dg_ms;
}
static void st_reset(void) { s_st_cur = 4; st_round(); }
static int st_dist(int a, int b) { return abs(a % 3 - b % 3) + abs(a / 3 - b / 3); }
static void st_end(bool won)
{
    if (won) ++s_dg_score; else ++s_dg_miss;
    s_st_reveal = won ? 1 : 2;
    s_st_show = s_dg_ms + ST_SHOW_MS;
}
static void st_scan(void)
{
    if (s_dg_state != DGS_PLAY || s_st_reveal || s_st_heat[s_st_cur] >= 0) return;
    s_st_heat[s_st_cur] = (int8_t)st_dist(s_st_cur, s_st_hid);
    --s_st_left;
    if (s_st_heat[s_st_cur] == 0) st_end(true);
    else if (s_st_left <= 0) st_end(false);
}
static void st_step(float dt)
{
    (void)dt;
    if (s_st_reveal) {
        if (s_dg_ms < s_st_show) return;
        if (s_dg_score >= 3) { s_dg_state = DGS_WIN; return; }
        if (s_dg_miss >= 3) { s_dg_state = DGS_LOST; return; }
        st_round();
        return;
    }
    if (s_dg_ms - s_st_t0 >= ST_ROUND_MS) st_end(false);   /* out of time = round lost */
}
static void st_paint(Gfx g)
{
    for (int i = 0; i < 9; ++i) {
        const int x = ST_X0 + (i % 3) * ST_CW, y = ST_Y0 + (i / 3) * ST_CH;
        g.drawRect(x + 2, y + 2, ST_CW - 4, ST_CH - 4, i == s_st_cur ? (uint8_t)PAL_CYAN : (uint8_t)PAL_WIRE_C);
        if (i == s_st_cur) dg_bot(g, x + 24, y + ST_CH / 2, 2);
        const int h = s_st_heat[i];
        if (h > 0) {                                           /* hotter = more 1s */
            const int n = h >= 4 ? 0 : 4 - h;
            GATE_BOX(g, "DGOBJ", "heat", x + 46, y + 27, n ? n * 8 - 2 : 6, 10, "-");
            if (n == 0) dg_bit(g, x + 46, y + 27, false, 2, PAL_LINE);
            for (int k = 0; k < n; ++k) dg_bit(g, x + 46 + k * 8, y + 27, true, 2, h == 1 ? (uint8_t)PAL_HEAD : (uint8_t)PAL_CODE);
        }
        if ((h == 0 || (s_st_reveal == 2 && i == s_st_hid))) {
            GATE_BOX(g, "DGOBJ", "rogue", x + 56, y + 26, 12, 12, "-");
            dev_icon(g, DEV_DEAUTH, x + 56, y + 26, h != 0);
        }
    }
}

/* ---------- shared: start / step / keys / board / patches / INT HUD ---------- */
static void dg_begin_round(void)
{
    s_dg_state = DGS_PLAY;
    s_dg_score = s_dg_miss = s_dg_junk = 0;
    s_dg_ms = 0;
    s_dg_hold = 0;
    s_dg_launch = false;
    s_dg_rng = 2166136261u ^ (uint32_t)((s_dg_bot + 1) * 2654435761u) ^ (uint32_t)(s_dg * 40503u);
    s_mz_t0 = s_dg_t;
    if (s_dg == DG_RAIN) rc_reset();
    else if (s_dg == DG_BEACON) bs_reset();
    else if (s_dg == DG_WALL) { fw_reset(); fw_park(); }
    else if (s_dg == DG_MAZE) mz_reset();
    else if (s_dg == DG_TWIN) tw_reset();
    else if (s_dg == DG_HOP) ch_reset();
    else if (s_dg == DG_RING) rr_reset();
    else if (s_dg == DG_PAIR) pl_reset();
    else if (s_dg == DG_PATCH) pr_reset();
    else if (s_dg == DG_BOTNET) bn_reset();
    else if (s_dg == DG_TRACE) st_reset();
    else jd_reset();
}
/* v1019m: step to another game of the same category (Tab, or , / on a WIN / LOST / PAUSE stop) */
static void dg_select(int slot)
{
    slot = ((slot % 3) + 3) % 3;
    s_dg_slot = slot;
    s_dg = DG_SET[DG_CAT[s_dg_bot]][slot];
    dg_begin_round();
    s_dirty = true;
    s_int_dirty = true;
    Serial.printf("[defend] game %s bot=%d\n", DG_NAME[s_dg], s_dg_bot);
}
static void dg_start(int bot)
{
    s_dg_bot = (bot < 0 || bot > 12) ? 12 : bot;
    s_dg_slot = 0;
    s_dg = DG_SET[DG_CAT[s_dg_bot]][0];
    s_dg_t = millis();
    dg_begin_round();
    s_dirty = true;
    s_int_dirty = true;
    Serial.printf("[defend] start %s bot=%d\n", DG_NAME[s_dg], s_dg_bot);
}
static void dg_stop(void)
{
    if (s_dg < 0) return;
    s_dg = -1;
    s_dirty = true;
    s_int_dirty = true;
}
static void dg_step(uint32_t t)
{
    if (s_dg < 0) return;
    if (t == s_dg_t) return;                       /* idempotent within a frame */
    float dt = (float)(t - s_dg_t) / 1000.f;
    s_dg_t = t;
    if (dt > 0.05f) dt = 0.05f;
    if (s_dg_state != DGS_PLAY) { s_dg_launch = false; return; }
    s_dg_ms += (uint32_t)(dt * 1000.f);
    if (s_dg == DG_RAIN) rc_step(dt);
    else if (s_dg == DG_BEACON) bs_step(dt);
    else if (s_dg == DG_WALL) fw_step(dt);
    else if (s_dg == DG_MAZE) mz_step(dt);
    else if (s_dg == DG_TWIN) tw_step(dt);
    else if (s_dg == DG_HOP) ch_step(dt);
    else if (s_dg == DG_RING) rr_step(dt);
    else if (s_dg == DG_PAIR) pl_step(dt);
    else if (s_dg == DG_PATCH) pr_step(dt);
    else if (s_dg == DG_BOTNET) bn_step(dt);
    else if (s_dg == DG_TRACE) st_step(dt);
    else jd_step(dt);
    s_dg_launch = false;
}
/* game keys; returns true when the key was used by the game */
static bool dg_key(int key, char typed)
{
    if (key == K_ESC) { dg_stop(); return true; }         /* backtick quits to the Defend board */
    const bool enter = key == K_OK, space = key == K_CHAR && typed == ' ';
    if (key == K_TAB) { dg_select(s_dg_slot + 1); return true; }   /* v1019m: next game, same category */
    if (s_dg_state == DGS_WIN || s_dg_state == DGS_LOST || s_dg_state == DGS_PAUSE) {
        if (key == K_LT) { dg_select(s_dg_slot - 1); return true; }
        if (key == K_RT) { dg_select(s_dg_slot + 1); return true; }
    }
    if (s_dg_state == DGS_WIN || s_dg_state == DGS_LOST) {
        if (enter) { dg_begin_round(); s_int_dirty = true; }
        return true;
    }
    if (space && s_dg != DG_WALL) {                       /* second action: pause */
        s_dg_state = s_dg_state == DGS_PAUSE ? DGS_PLAY : DGS_PAUSE;
        return true;
    }
    if (s_dg_state == DGS_PAUSE) return true;
    int dir = -1;
    if (key == K_UP) dir = 0; else if (key == K_DN) dir = 1; else if (key == K_LT) dir = 2; else if (key == K_RT) dir = 3;
    if (s_dg == DG_RAIN) {
        if (dir == 2 && s_rc_lane > 0) --s_rc_lane;
        if (dir == 3 && s_rc_lane < RC_L - 1) ++s_rc_lane;
    } else if (s_dg == DG_BEACON) {
        if (dir == 2) bs_sort(true);                      /* , trusted */
        if (dir == 3) bs_sort(false);                     /* / unknown */
    } else if (s_dg == DG_WALL) {
        if (dir == 2) s_fw_px -= 18.f;
        if (dir == 3) s_fw_px += 18.f;
        if (s_fw_px < (float)(DG_PX + 11)) s_fw_px = (float)(DG_PX + 11);
        if (s_fw_px > (float)(DG_PX + DG_PW - 11)) s_fw_px = (float)(DG_PX + DG_PW - 11);
        if (enter || space) s_dg_launch = true;
    } else if (s_dg == DG_MAZE) {
        if (dir >= 0) s_mz_p.want = (int8_t)dir;
    } else if (s_dg == DG_TWIN) {
        if (dir == 2) tw_pick(0);
        if (dir == 3) tw_pick(1);
    } else if (s_dg == DG_HOP) {
        if (dir == 2 && s_ch_lane > 0) --s_ch_lane;
        if (dir == 3 && s_ch_lane < CH_L - 1) ++s_ch_lane;
        if (enter) ch_boost();
    } else if (s_dg == DG_RING) {
        if (dir >= 0) rr_ping(dir);
    } else if (s_dg == DG_PAIR) {
        if (key == K_CHAR && typed >= '0' && typed <= '9') pl_type(typed);
        else if (key == K_BS) { const size_t n = strlen(s_pl_in); if (n) s_pl_in[n - 1] = 0; }
        else if (enter) pl_enter();
    } else if (s_dg == DG_PATCH) {
        if (dir == 0 && s_pr_cur >= PR_C) s_pr_cur -= PR_C;
        if (dir == 1 && s_pr_cur < PR_N - PR_C) s_pr_cur += PR_C;
        if (dir == 2 && s_pr_cur % PR_C > 0) --s_pr_cur;
        if (dir == 3 && s_pr_cur % PR_C < PR_C - 1) ++s_pr_cur;
        if (enter) pr_patch();
    } else if (s_dg == DG_BOTNET) {
        if (key == K_CHAR && typed >= '1' && typed <= '5') bn_cut(typed - '1');
    } else if (s_dg == DG_TRACE) {
        if (dir == 0 && s_st_cur >= 3) s_st_cur -= 3;
        if (dir == 1 && s_st_cur < 6) s_st_cur += 3;
        if (dir == 2 && s_st_cur % 3 > 0) --s_st_cur;
        if (dir == 3 && s_st_cur % 3 < 2) ++s_st_cur;
        if (enter) st_scan();
    } else {
        if (dir == 0 && s_jd_lane > 0) --s_jd_lane;
        if (dir == 1 && s_jd_lane < JD_L - 1) ++s_jd_lane;
    }
    return true;
}
static void paint_dg_board(Gfx g)
{
    const uint8_t ac = PAL_CYAN;
    GATE_BOX(g, "BOARD", "defend", DG_BX, DG_BY, DG_BW, DG_BH, "-");
    g.fillRect(DG_BX, DG_BY, DG_BW, DG_BH, PAL_BLACK);
    g.drawRect(DG_BX, DG_BY, DG_BW, DG_BH, ac);
    g.drawRect(DG_BX + 1, DG_BY + 1, DG_BW - 2, DG_BH - 2, PAL_WIRE_C);
    if (s_dg == DG_RAIN) rc_paint(g);
    else if (s_dg == DG_BEACON) bs_paint(g);
    else if (s_dg == DG_WALL) fw_paint(g);
    else if (s_dg == DG_MAZE) mz_paint(g);
    else if (s_dg == DG_TWIN) tw_paint(g);
    else if (s_dg == DG_HOP) ch_paint(g);
    else if (s_dg == DG_RING) rr_paint(g);
    else if (s_dg == DG_PAIR) pl_paint(g);
    else if (s_dg == DG_PATCH) pr_paint(g);
    else if (s_dg == DG_BOTNET) bn_paint(g);
    else if (s_dg == DG_TRACE) st_paint(g);
    else if (s_dg == DG_JAM) jd_paint(g);
}

/* dirty rects: one fixed slot per moving part, union of last and current box, keyed by its look */
struct DGRect { int16_t x, y, w, h; uint32_t key; };
static const int DG_SLOTS = 16;   /* v1019m: 16 (Patch Run = one slot per cell); the v1019j games use 0-11 */
static DGRect s_dg_prev[DG_SLOTS];
static void dg_rects(DGRect *r)
{
    for (int i = 0; i < DG_SLOTS; ++i) r[i] = DGRect{ 0, 0, 0, 0, 0 };
    if (s_dg == DG_RAIN) {
        for (int i = 0; i < RC_MAX; ++i) {
            if (!s_rc_d[i].live) continue;
            const int cx = rc_cx((int)s_rc_d[i].x), y = (int)lroundf(s_rc_d[i].y);
            r[i] = DGRect{ (int16_t)(cx - 7), (int16_t)(y - 23), 14, 37, (uint32_t)(s_rc_d[i].k + 8) };
        }
        r[RC_MAX] = DGRect{ (int16_t)(rc_cx(s_rc_lane) - 11), (int16_t)(RC_BOTY - 11), 22, 22, 1 };
    } else if (s_dg == DG_BEACON) {
        for (int i = 0; i < BS_MAX; ++i) {
            if (!s_bs[i].live) continue;
            int x, y; bs_rect(i, &x, &y);
            r[i] = DGRect{ (int16_t)(x - 1), (int16_t)(y - 1), 24, 14, (uint32_t)(s_bs[i].k * 8 + s_bs[i].tag + 1) };
        }
    } else if (s_dg == DG_WALL) {
        r[0] = DGRect{ (int16_t)(lroundf(s_fw_px) - 12), (int16_t)(FW_PY - 12), 24, 24, 1 };
        r[1] = DGRect{ (int16_t)(lroundf(s_fw_bx) - 1), (int16_t)(lroundf(s_fw_by) - 1), 8, 12, 1 };
        r[2] = DGRect{ (int16_t)FW_X0, (int16_t)FW_Y0, (int16_t)(FW_C * (FW_BW + 2)), (int16_t)(FW_R * (FW_BH + 4)), s_fw_alive };
        r[3] = DGRect{ (int16_t)FW_X0, (int16_t)FW_ICY, (int16_t)(FW_C * (FW_BW + 2)), 12, (uint32_t)s_fw_dim + 1u };
    } else if (s_dg == DG_MAZE) {
        const int px = MZ_X + (int)lroundf(s_mz_p.x), py = MZ_Y + (int)lroundf(s_mz_p.y);
        r[0] = DGRect{ (int16_t)(px - 8), (int16_t)(py - 8), 16, 16, (uint32_t)s_mz_left + 1u };
        const uint32_t fr = s_dg_t < s_mz_fright ? 2u : 1u;
        for (int i = 0; i < MZ_G; ++i) {
            const int jx = MZ_X + (int)lroundf(s_mz_j[i].x), jy = MZ_Y + (int)lroundf(s_mz_j[i].y);
            r[1 + i] = DGRect{ (int16_t)(jx - 7), (int16_t)(jy - 7), 14, 14, fr };
        }
    } else if (s_dg == DG_TWIN) {
        if (s_tw_live) {
            const int y = (int)lroundf(s_tw_y);
            const uint32_t k = (uint32_t)(s_tw_k + 1) * 4u;
            r[0] = DGRect{ (int16_t)(TW_LX - 8), (int16_t)(y - 1), 16, 28, k + (s_tw_side == 0 ? 1u + (tw_flick() ? 1u : 0u) : 0u) };
            r[1] = DGRect{ (int16_t)(TW_RX - 8), (int16_t)(y - 1), 16, 28, k + (s_tw_side == 1 ? 1u + (tw_flick() ? 1u : 0u) : 0u) };
        }
    } else if (s_dg == DG_HOP) {
        for (int i = 0; i < CH_MAX; ++i) {
            if (!s_ch[i].live) continue;
            r[i] = DGRect{ (int16_t)(ch_cx(s_ch[i].lane) - 9), (int16_t)(lroundf(s_ch[i].y) - 1), 18, 36, 1 };
        }
        r[CH_MAX] = DGRect{ (int16_t)(ch_cx(s_ch_lane) - 11), (int16_t)(CH_BOTY - 11), 22, 22, 1 };
    } else if (s_dg == DG_PAIR) {
        uint32_t h = 2166136261u;
        for (int i = 0; i < 4; ++i) { h = (h ^ (uint8_t)s_pl_pin[i]) * 16777619u; }
        r[0] = DGRect{ 122, 36, 60, 12, h };
        uint32_t t = 2166136261u;
        for (int i = 0; i < 5 && s_pl_in[i]; ++i) t = (t ^ (uint8_t)s_pl_in[i]) * 16777619u;
        r[1] = DGRect{ 122, 58, 60, 12, t + (uint32_t)strlen(s_pl_in) };
        const int y = (int)lroundf(s_pl_y);
        r[2] = DGRect{ 153, (int16_t)(y - 22), 14, 35, 1 };
    } else if (s_dg == DG_RING) {
        for (int s = 0; s < 4; ++s) {
            if (s_rr_d[s] < 0.f) continue;
            int x, y; rr_pos(s, &x, &y);
            r[s] = DGRect{ (int16_t)(x - 1), (int16_t)(y - 1), 14, 14, (uint32_t)(s_rr_k[s] + 1) };
        }
    } else if (s_dg == DG_PATCH) {
        for (int i = 0; i < PR_N; ++i) {
            int x, y; pr_cell(i, &x, &y);
            const uint32_t k = 1u + s_pr_st[i] + 4u * (uint32_t)(i == s_pr_cur)
                             + (s_pr_st[i] == 1 ? 8u * (uint32_t)pr_blink() + 16u * (uint32_t)(pr_bar(i) + 1) : 0u);
            r[i] = DGRect{ (int16_t)x, (int16_t)y, (int16_t)PR_CW, (int16_t)PR_CH, k };
        }
    } else if (s_dg == DG_BOTNET) {
        for (int i = 0; i < BN_N; ++i) {
            const int cx = BN_X0 + i * BN_DX;
            if (s_bn_n[i]) r[i] = DGRect{ (int16_t)(cx - 4), (int16_t)BN_TOP, 8, (int16_t)(s_bn_n[i] * 12), (uint32_t)s_bn_n[i] };
            r[BN_N + i] = DGRect{ (int16_t)(cx - 7), (int16_t)(BN_ICY - 1), 14, 14, 1u + s_bn_inf[i] };
        }
        r[2 * BN_N] = DGRect{ (int16_t)(BN_X0 + s_bn_at * BN_DX - 11), (int16_t)(BN_BOTY - 11), 22, 22, 1 };
    } else if (s_dg == DG_TRACE) {
        for (int i = 0; i < 9; ++i) {
            const uint32_t k = 1u + (uint32_t)(s_st_heat[i] + 1) + 8u * (uint32_t)(i == s_st_cur)
                             + 16u * (uint32_t)(s_st_reveal == 2 && i == s_st_hid);
            r[i] = DGRect{ (int16_t)(ST_X0 + (i % 3) * ST_CW), (int16_t)(ST_Y0 + (i / 3) * ST_CH), (int16_t)ST_CW, (int16_t)ST_CH, k };
        }
    } else if (s_dg == DG_JAM) {
        for (int i = 0; i < JD_MAX; ++i) {
            if (!s_jd[i].live) continue;
            r[i] = DGRect{ (int16_t)(lroundf(s_jd[i].x) - 1), (int16_t)DG_PY, (int16_t)(JD_WW + 2), (int16_t)(JD_Y0 + JD_L * JD_LH - DG_PY), 1u + (uint32_t)s_jd[i].hit };
        }
        r[JD_MAX] = DGRect{ (int16_t)(JD_BX - 11), (int16_t)(jd_cy(s_jd_lane) - 11), 22, 22, 1 };
    }
}
static void dg_snap(void) { dg_rects(s_dg_prev); }
static void draw_dg_part(Gfx g) { paint_dg_board(g); }
static void patch_world(void (*fn)(Gfx), int x, int y, int w, int h);
static void dg_patch_one(int x, int y, int w, int h) { botcorps_band_patch_rect(draw_dg_part, x, y, w, h); }
static void dg_patch(int x, int y, int w, int h)
{
    int x1 = x + w, y1 = y + h;                            /* clamp to the opaque board */
    if (x < DG_BX) x = DG_BX;
    if (y < DG_BY) y = DG_BY;
    if (x1 > DG_BX + DG_BW) x1 = DG_BX + DG_BW;
    if (y1 > DG_BY + DG_BH) y1 = DG_BY + DG_BH;
    w = x1 - x; h = y1 - y;
    if (w <= 0 || h <= 0 || !botcorps_dual_ok()) return;
    if (botcorps_band_direct() || !botcorps_band_live()) { patch_world(draw_dg_part, x, y, w, h); return; }
    if (y < 120 && y + h > 120) { dg_patch_one(x, y, w, 120 - y); dg_patch_one(x, 120, w, y + h - 120); }
    else dg_patch_one(x, y, w, h);
}
static void dg_tick(void)
{
    static uint32_t last = 0;
    const uint32_t t = millis();
    if (last != 0 && (uint32_t)(t - last) < 33u) return;
    last = t;
    dg_step(t);
    DGRect cur[DG_SLOTS];
    dg_rects(cur);
    for (int i = 0; i < DG_SLOTS; ++i) {
        const DGRect &a = s_dg_prev[i], &b = cur[i];
        if (a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h && a.key == b.key) continue;
        if (a.w <= 0) { if (b.w > 0) dg_patch(b.x, b.y, b.w, b.h); continue; }
        if (b.w <= 0) { dg_patch(a.x, a.y, a.w, a.h); continue; }
        const int x0 = a.x < b.x ? a.x : b.x, y0 = a.y < b.y ? a.y : b.y;
        const int x1 = a.x + a.w > b.x + b.w ? a.x + a.w : b.x + b.w, y1 = a.y + a.h > b.y + b.h ? a.y + a.h : b.y + b.h;
        if ((x1 - x0) * (y1 - y0) > 2 * (a.w * a.h + b.w * b.h)) { dg_patch(a.x, a.y, a.w, a.h); dg_patch(b.x, b.y, b.w, b.h); }
        else dg_patch(x0, y0, x1 - x0, y1 - y0);
    }
    for (int i = 0; i < DG_SLOTS; ++i) s_dg_prev[i] = cur[i];
}

/* INT: score / lives / timer as plain text in the menu style (under the unchanged BOTcorps bar) */
static void dg_hud_lines(char l[4][20])
{
    const uint32_t s = s_dg_ms / 1000u;
    if (s_dg == DG_RAIN) {
        snprintf(l[0], 20, "SCORE %02d/20", s_dg_score);
        snprintf(l[1], 20, "MISS %d/3 BITS %d/3", s_dg_miss, s_dg_junk);
    } else if (s_dg == DG_BEACON) {
        snprintf(l[0], 20, "SORTED %02d/25", s_dg_score);
        snprintf(l[1], 20, "MISTAKES %d/3", s_dg_miss);
    } else if (s_dg == DG_WALL) {
        snprintf(l[0], 20, "BLOCKS %02d/%d", s_dg_score, FW_C * FW_R);
        snprintf(l[1], 20, "PACKETS %d", 3 - s_dg_miss < 0 ? 0 : 3 - s_dg_miss);
    } else if (s_dg == DG_MAZE) {
        snprintf(l[0], 20, "BITS LEFT %03d", s_mz_left);
        snprintf(l[1], 20, "LIVES %d", 3 - s_dg_miss < 0 ? 0 : 3 - s_dg_miss);
    } else if (s_dg == DG_TWIN) {
        snprintf(l[0], 20, "TWINS %02d/15", s_dg_score);
        snprintf(l[1], 20, "WRONG %d/3", s_dg_miss);
    } else if (s_dg == DG_HOP) {
        const unsigned left = s_dg_ms >= 60000u ? 0u : (unsigned)((60000u - s_dg_ms + 999u) / 1000u);
        snprintf(l[0], 20, "LEFT %02us DODGED %d", left, s_dg_score % 100);
        snprintf(l[1], 20, "HITS %d/3 BOOST %d", s_dg_miss, s_ch_boost);
    } else if (s_dg == DG_RING) {
        snprintf(l[0], 20, "WAVE %d/3 PINGED %d", s_rr_wave + 1, s_dg_score % 100);
        snprintf(l[1], 20, "REACHED %d/3", s_dg_miss);
    } else if (s_dg == DG_PAIR) {
        snprintf(l[0], 20, "LOCKS %02d/10", s_dg_score);
        snprintf(l[1], 20, "REACHED %d/3", s_dg_miss);
    } else if (s_dg == DG_PATCH) {
        const unsigned left = s_dg_ms >= 90000u ? 0u : (unsigned)((90000u - s_dg_ms + 999u) / 1000u);
        snprintf(l[0], 20, "LEFT %02us PATCH %d", left, s_dg_score % 100);
        snprintf(l[1], 20, "DOWN %d/3", s_dg_miss);
    } else if (s_dg == DG_BOTNET) {
        const unsigned left = s_dg_ms >= 60000u ? 0u : (unsigned)((60000u - s_dg_ms + 999u) / 1000u);
        snprintf(l[0], 20, "LEFT %02us CUTS %d", left, s_dg_score % 100);
        snprintf(l[1], 20, "INFECTED %d", s_dg_miss);
    } else if (s_dg == DG_TRACE) {
        snprintf(l[0], 20, "ROUNDS %d-%d OF 5", s_dg_score, s_dg_miss);
        snprintf(l[1], 20, "SCANS %d/%d", s_st_left, ST_SCANS);
    } else {
        const unsigned left = s_dg_ms >= 45000u ? 0u : (unsigned)((45000u - s_dg_ms + 999u) / 1000u);
        snprintf(l[0], 20, "LEFT %02us PASSED %d", left, s_dg_score % 100);
        snprintf(l[1], 20, "HITS %d/3", s_dg_miss);
    }
    snprintf(l[2], 20, "TIME %02u:%02u", (unsigned)((s / 60u) % 100u), (unsigned)(s % 60u));
    snprintf(l[3], 20, "%s", s_dg_state == DGS_WIN ? "WIN" : s_dg_state == DGS_LOST ? "LOST"
                             : s_dg_state == DGS_PAUSE ? "PAUSE" : "LIVE");
}
static void dg_paint_hud(Gfx g, int width, bool full)
{
    char l[4][20];
    dg_hud_lines(l);
    const int keep = s_px;
    if (full) {
        g.fillRect(0, BAR_H + 1, width, 135 - BAR_H - 1, PAL_DEEP);
        s_px = 8;
        txt(g, DG_NAME[s_dg], 8, 30, PAL_CYAN, lgfx::textdatum::top_left);
        s_px = 6;
        char who[24];
        snprintf(who, sizeof(who), "%s %d/3", s_dg_bot >= 12 ? "gibson" : BOTS[s_dg_bot].id, s_dg_slot + 1);
        txt(g, who, 8, 42, s_dg_bot >= 12 ? (uint8_t)PAL_IVORY : bot_color(s_dg_bot), lgfx::textdatum::top_left);
    }
    s_px = 8;
    char pad[24];
    for (int k = 0; k < 3; ++k) {
        snprintf(pad, sizeof(pad), "%-18.19s", l[k]);
        txt_bg(g, pad, 8, 60 + k * 16, k == 2 ? (uint8_t)COL_INK : (uint8_t)COL_WHITE, PAL_DEEP, lgfx::textdatum::top_left);
    }
    snprintf(pad, sizeof(pad), "%-5.19s", l[3]);
    const uint8_t sc = s_dg_state == DGS_WIN ? (uint8_t)PAL_OK : s_dg_state == DGS_LOST ? (uint8_t)PAL_AMBER : (uint8_t)PAL_MUTED;
    txt_bg(g, pad, 8, 112, sc, PAL_DEEP, lgfx::textdatum::top_left);
    s_px = keep;
}
static void dg_int_tick(lgfx::LGFX_Device &menu)
{
    static char drawn[4][20];
    char l[4][20];
    dg_hud_lines(l);
    if (memcmp(drawn, l, sizeof(l)) == 0) return;
    memcpy(drawn, l, sizeof(l));
    botcorps_set_surface(BOTCORPS_SURFACE_MENU);
    menu.startWrite();
    dg_paint_hud(Gfx(menu), menu.width(), false);
    menu.endWrite();
    menu.clearClipRect();
}

#ifdef BOTCORPS_HOST_RENDER
/* host autopilot (renders + logic tests only): a fair player using the same keys */
static void dg_ai(void)
{
    if (s_dg_state != DGS_PLAY) return;
    if (s_dg == DG_RAIN) {
        int best = -1; float by = -1e9f;
        for (int i = 0; i < RC_MAX; ++i)
            if (s_rc_d[i].live && s_rc_d[i].k >= 0 && s_rc_d[i].y > by) { by = s_rc_d[i].y; best = i; }
        int want = best >= 0 ? (int)s_rc_d[best].x : s_rc_lane;
        for (int i = 0; i < RC_MAX; ++i)          /* do not sit under a bit about to land */
            if (s_rc_d[i].live && s_rc_d[i].k < 0 && (int)s_rc_d[i].x == want && s_rc_d[i].y + 30.f > (float)RC_TOP
                && (best < 0 || s_rc_d[i].y > s_rc_d[best].y)) want = want > 0 ? want - 1 : want + 1;
        if (want < s_rc_lane) dg_key(K_LT, 0); else if (want > s_rc_lane) dg_key(K_RT, 0);
    } else if (s_dg == DG_BEACON) {
        const int b = bs_lowest();
        if (b >= 0 && s_bs[b].y > (float)(DG_PY + 100)) dg_key(s_bs[b].tag & 1 ? K_LT : K_RT, 0);
    } else if (s_dg == DG_WALL) {
        if (s_fw_serve) { dg_key(K_OK, 0); return; }
        const float tx = s_fw_bx + 3.f;
        if (tx < s_fw_px - 9.f) dg_key(K_LT, 0); else if (tx > s_fw_px + 9.f) dg_key(K_RT, 0);
    } else if (s_dg == DG_TWIN) {
        if (s_tw_live && s_tw_y > (float)(DG_PY + 60)) dg_key(s_tw_side ? K_RT : K_LT, 0);
    } else if (s_dg == DG_HOP) {
        bool bad[CH_L] = { false };
        for (int i = 0; i < CH_MAX; ++i)
            if (s_ch[i].live && s_ch[i].y + 34.f > (float)(CH_BOTY - 150) && s_ch[i].y < (float)(CH_BOTY + 10)) bad[s_ch[i].lane] = true;
        if (!bad[s_ch_lane]) return;
        int best = -1;
        for (int d = 1; d < CH_L && best < 0; ++d) {
            if (s_ch_lane - d >= 0 && !bad[s_ch_lane - d]) best = s_ch_lane - d;
            else if (s_ch_lane + d < CH_L && !bad[s_ch_lane + d]) best = s_ch_lane + d;
        }
        if (best < 0) return;
        const int nx = best < s_ch_lane ? s_ch_lane - 1 : s_ch_lane + 1;
        bool hot = false;                          /* never step into a block that is level with the bot */
        for (int i = 0; i < CH_MAX; ++i)
            if (s_ch[i].live && s_ch[i].lane == nx && s_ch[i].y + 34.f >= (float)(CH_BOTY - 12) && s_ch[i].y <= (float)(CH_BOTY + 12)) hot = true;
        if (!hot) dg_key(best < s_ch_lane ? K_LT : K_RT, 0);
    } else if (s_dg == DG_RING) {
        int b = -1;
        for (int k = 0; k < 4; ++k) if (s_rr_d[k] >= 0.f && s_rr_d[k] < 70.f && (b < 0 || s_rr_d[k] < s_rr_d[b])) b = k;
        static const int KD[4] = { K_UP, K_DN, K_LT, K_RT };
        if (b >= 0) dg_key(KD[b], 0);
    } else if (s_dg == DG_PAIR) {
        const size_t n = strlen(s_pl_in);
        if (n < 4) dg_key(K_CHAR, s_pl_pin[n]); else dg_key(K_OK, 0);
    } else if (s_dg == DG_PATCH) {
        int b = -1;
        for (int i = 0; i < PR_N; ++i) if (s_pr_st[i] == 1 && (b < 0 || s_pr_due[i] < s_pr_due[b])) b = i;
        if (b < 0) return;
        if (b == s_pr_cur) { dg_key(K_OK, 0); return; }
        const int bx = b % PR_C, by = b / PR_C, cx = s_pr_cur % PR_C, cy = s_pr_cur / PR_C;
        if (bx < cx) dg_key(K_LT, 0); else if (bx > cx) dg_key(K_RT, 0);
        else if (by < cy) dg_key(K_UP, 0); else dg_key(K_DN, 0);
    } else if (s_dg == DG_BOTNET) {
        int b = -1;
        for (int i = 0; i < BN_N; ++i) if (!s_bn_inf[i] && s_bn_n[i] >= 6 && (b < 0 || s_bn_n[i] > s_bn_n[b])) b = i;
        if (b >= 0) dg_key(K_CHAR, (char)('1' + b));
    } else if (s_dg == DG_TRACE) {
        if (s_st_reveal) return;
        /* candidates consistent with every scan; scan the cell whose worst-case answer leaves the fewest */
        bool cand[9]; int nc = 0;
        for (int c = 0; c < 9; ++c) {
            cand[c] = true;
            for (int i = 0; i < 9; ++i) if (s_st_heat[i] >= 0 && st_dist(i, c) != s_st_heat[i]) cand[c] = false;
            nc += cand[c];
        }
        int best = -1, bw = 99;
        for (int i = 0; i < 9; ++i) {
            if (s_st_heat[i] >= 0) continue;
            int cnt[5] = { 0 }, w = 0;
            for (int c = 0; c < 9; ++c) if (cand[c] && c != i) ++cnt[st_dist(i, c)];
            for (int k = 1; k < 5; ++k) if (cnt[k] > w) w = cnt[k];
            w = w * 2 + (cand[i] ? 0 : 1);
            if (w < bw) { bw = w; best = i; }
        }
        (void)nc;
        if (best < 0) return;
        if (best == s_st_cur) { dg_key(K_OK, 0); return; }
        const int bx = best % 3, by = best / 3, cx = s_st_cur % 3, cy = s_st_cur / 3;
        if (bx < cx) dg_key(K_LT, 0); else if (bx > cx) dg_key(K_RT, 0);
        else if (by < cy) dg_key(K_UP, 0); else dg_key(K_DN, 0);
    } else if (s_dg == DG_JAM) {
        int b = -1;
        for (int i = 0; i < JD_MAX; ++i)
            if (s_jd[i].live && !s_jd[i].hit && s_jd[i].x + JD_WW >= (float)(JD_BX - 9) && (b < 0 || s_jd[i].x < s_jd[b].x)) b = i;
        if (b < 0) return;
        if (jd_open(s_jd[b], s_jd_lane)) return;
        dg_key(s_jd_lane < s_jd[b].gap ? K_DN : K_UP, 0);
    } else {
        /* BFS to the nearest pellet; first step of that path */
        static int8_t q[MZ_R * MZ_C][2]; static int8_t from[MZ_R][MZ_C];
        const int sx = (int)lroundf(s_mz_p.x) / MZ_T, sy = (int)lroundf(s_mz_p.y) / MZ_T;
        memset(from, -1, sizeof(from));
        int h = 0, n = 0; q[n][0] = (int8_t)sx; q[n][1] = (int8_t)sy; ++n; from[sy][sx] = 4;
        int gx = -1, gy = -1;
        while (h < n) {
            const int x = q[h][0], y = q[h][1]; ++h;
            if (s_mz_pel[y][x] && !(x == sx && y == sy)) { gx = x; gy = y; break; }
            for (int d = 0; d < 4; ++d) {
                const int nx = x + MZ_DX[d], ny = y + MZ_DY[d];
                if (!mz_open(nx, ny) || from[ny][nx] >= 0) continue;
                from[ny][nx] = (int8_t)d; q[n][0] = (int8_t)nx; q[n][1] = (int8_t)ny; ++n;
            }
        }
        if (gx < 0) return;
        int x = gx, y = gy, d = from[y][x];
        while (!(x - MZ_DX[d] == sx && y - MZ_DY[d] == sy)) { x -= MZ_DX[d]; y -= MZ_DY[d]; d = from[y][x]; }
        static const int KD[4] = { K_UP, K_DN, K_LT, K_RT };
        if (s_mz_p.want != d) dg_key(KD[d], 0);
    }
}
#endif
