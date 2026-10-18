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
enum { DG_RAIN = 0, DG_BEACON, DG_WALL, DG_MAZE, DG_N };
static const char *const DG_NAME[DG_N] = { "RAIN CATCH", "BEACON SORT", "FIREWALL WALL", "GIBSON MAZE" };
/* bot -> device category: 0 Wi-Fi, 1 BLE, 2 IoT, 3 Rogue (12 = Gibson, the Rogue flagship) */
static const int8_t DG_CAT[13] = { 0, 1, 3, 0, 2, 3, 1, 2, 1, 3, 2, 1, 3 };
enum { DGS_PLAY = 0, DGS_WIN, DGS_LOST, DGS_PAUSE };

static const int DG_BX = 8, DG_BY = 20, DG_BW = 304, DG_BH = 214;   /* matte board, rain gutter around */
static const int DG_PX = 10, DG_PY = 22, DG_PW = 300, DG_PH = 210;  /* play area inside the frame */


static int s_dg_bot = 12;
static int s_dg_state = DGS_PLAY;
static uint32_t s_dg_t = 0, s_dg_rng = 1, s_dg_ms = 0;   /* last step, rng, played ms (pause excluded) */
static int s_dg_score = 0, s_dg_miss = 0, s_dg_junk = 0;
static uint32_t s_dg_hold = 0;     /* maze: frozen until (after a catch) */
static bool s_dg_launch = false;   /* key edge for the step (Enter / Space) */
#ifdef BOTCORPS_HOST_RENDER
static bool s_dg_noghost = false;  /* host logic test only: maze without jammers (proves clear = win) */
#endif


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
    chip_mini(g, rc_cx(s_rc_lane), RC_BOTY, 20, s_dg_bot, false, 0);
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
    chip_mini(g, BS_CX, BS_BOTY, 20, s_dg_bot, false, 0);
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
    chip_mini(g, (int)lroundf(s_fw_px), FW_PY, 22, s_dg_bot, false, 0);
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
    chip_mini(g, MZ_X + (int)lroundf(s_mz_p.x), MZ_Y + (int)lroundf(s_mz_p.y), 13, s_dg_bot, false, 0);
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
    else mz_reset();
}
static void dg_start(int bot)
{
    s_dg_bot = (bot < 0 || bot > 12) ? 12 : bot;
    s_dg = DG_CAT[s_dg_bot];
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
    else mz_step(dt);
    s_dg_launch = false;
}
/* game keys; returns true when the key was used by the game */
static bool dg_key(int key, char typed)
{
    if (key == K_ESC) { dg_stop(); return true; }         /* backtick quits to the Defend board */
    const bool enter = key == K_OK, space = key == K_CHAR && typed == ' ';
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
    } else {
        if (dir >= 0) s_mz_p.want = (int8_t)dir;
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
}

/* dirty rects: one fixed slot per moving part, union of last and current box, keyed by its look */
struct DGRect { int16_t x, y, w, h; uint32_t key; };
static const int DG_SLOTS = 12;
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
    } else {
        snprintf(l[0], 20, "BITS LEFT %03d", s_mz_left);
        snprintf(l[1], 20, "LIVES %d", 3 - s_dg_miss < 0 ? 0 : 3 - s_dg_miss);
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
        snprintf(who, sizeof(who), "%s", s_dg_bot >= 12 ? "gibson" : BOTS[s_dg_bot].id);
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
