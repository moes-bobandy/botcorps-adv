/* ===== v1019r (#10, /workspace/adv-gibson-game-concepts-2026-10-19.md): Gibson's own games ===========
 * Included by botcorps_defend.inc after the twelve v1019j/m games. Both play INSIDE the Defend board (the
 * 176x122 play area, MOD item 1: no full-screen takeover), with round in-game Gibson (the ivory round
 * sprite + Matrix shades, dg_bot) as the player and one shared DEVICE DEX (10 device types).
 *   NIGHT SHIFT (flagship, concept 2): Gibson is the night guard on a dark 20x13 floor (8 px tiles, six
 *     rooms + a corridor, all on one screen). Rogue devices hide; only a 3-tile light radius around Gibson
 *     is lit (walls once seen stay faintly drawn). Space = SCAN pulse (2 s cooldown): a ring sweeps out and
 *     every device in range pings a blip in its type colour, bigger = closer (airtag pings faint and only
 *     near). Walk onto / next to a device + Enter = neutralize (catch flash, Dex +1, ALERT -25). ALERT fills
 *     while devices stay live; ALERT 100 = breach = LOST; clock 12:00AM -> 6:00AM (150 s) = LOST if any
 *     device is left; all caught = WIN (+ seconds left). Enter on WIN = next shift: 3, 5, 7, 9, then all 10
 *     device types. Behaviours: airtag still + faint; skimmer hides on furniture; drone drifts through walls;
 *     flipper flees when Gibson is within 3 tiles; deauth jams the scanner 3 s when Gibson comes near; meta
 *     glasses move only while outside the light; bluetooth wanders slowly. Tab = floor map (pause, all lit),
 *     1-6 = light that room's outline + label for 2 s, ` = quit.
 *   SWEEP (bonus, concept 1): refined Pac-Man on a 15x11 board (10 px tiles, 68 data bits). Gibson eats the
 *     bits; three device traces leave the sandbox one by one: drone chases directly, deauth cuts 2 tiles
 *     ahead, skimmer wanders and drops sticky tiles (Gibson at half speed on them); level 3+ adds a flipper.
 *     Four ROOT nodes (corners) turn the traces blue for 5 s (capture = back to the sandbox, +10). A bonus
 *     device pops at the centre twice per level (Dex +1, +50). Space = once-per-level ROOT burst (traces
 *     within 4 tiles go blue 3 s). 3 lives. Clear = WIN, Enter = next level (+8 % trace speed). Tab = pause.
 * Keyboard tuning (#6, documented in the commit): tile-per-press guard moves (held arrows auto-repeat at
 * 130 ms after 260 ms, see dg_repeat), turn buffering in SWEEP, 2 s scan cooldown, 150 s shift, ALERT
 * 0.30 %/s per live device (+10 % per shift), SWEEP Gibson 40 px/s vs traces 30 px/s (18 when blue).
 * Deterministic LCG like every Defend game, so a patch matches a full paint (S1). */

/* ---------- shared Device Dex: wifi, bluetooth, drone, airtag, keyfob, skimmer, flipper, samsungtag, meta, deauth ---------- */
enum { DX_WIFI = 0, DX_BT, DX_DRONE, DX_AIRTAG, DX_KEYFOB, DX_SKIM, DX_FLIP, DX_SAMS, DX_META, DX_DEAUTH, DX_N };
static const char *const DX_NAME[DX_N] = { "WIFI", "BLUETOOTH", "DRONE", "AIRTAG", "KEYFOB", "SKIMMER", "FLIPPER",
                                           "SAMSUNGTAG", "META GLASS", "DEAUTH" };
static uint8_t s_dex[DX_N];          /* catches per device type (RAM; both Gibson games feed it) */
static int dex_have(void) { int n = 0; for (int i = 0; i < DX_N; ++i) n += s_dex[i] ? 1 : 0; return n; }
static void dex_add(int t) { if (t >= 0 && t < DX_N && s_dex[t] < 99) ++s_dex[t]; }
/* four device icons the v3.2 set lacks (12x12, a body / b deep / c highlight, like DEV_ROW) */
static const char *const DX_ROW[4][12] = {
    { "....aaa.....", "...a...a....", "...a...a....", "....aaa.....", "...aaaaaa...", "..abbbbbba..",
      "..abccccba..", "..abbbbbba..", "..abccccba..", "..abbbbbba..", "...aaaaaa...", "............" },   /* keyfob */
    { "............", "aaaaaaaaaaaa", "accccccaabaa", "acbbbbcabbba", "acbbbbcaabaa", "accccccaaaaa",
      "aaaaaaaaaaaa", ".aa......aa.", "............", "............", "............", "............" },   /* flipper */
    { "............", "..aaaaaaaa..", ".abbbbbbbba.", ".abccbbbbba.", ".abccbbbbba.", ".abbbbbbbba.",
      ".abbbbbbbba.", ".abbbbbbbba.", ".abbbbbbbba.", "..aaaaaaaa..", "............", "............" },   /* samsung tag */
    { "............", "............", "a..........a", ".aaaaaaaaaa.", ".abbbaabbba.", ".abcbaabcba.",
      ".abbba.abbba", "..aaa..aaa..", "............", "............", "............", "............" },   /* meta glasses */
};
static const uint8_t DX_INK[4][3] = { { 252, 64, 255 }, { 241, 64, 255 }, { 183, 37, 255 }, { 182, 68, 255 } };
/* blip / Dex ink per type (the icon's own body colour) */
static const uint8_t DX_BLIP[DX_N] = { 31, 91, 224, 252, 252, 158, 241, 183, 182, 207 };
static void dex_icon(Gfx g, int t, int x, int y, bool dim)
{
    switch (t) {
    case DX_WIFI: dev_icon(g, DEV_AP, x, y, dim); return;
    case DX_BT: dev_icon(g, DEV_BT, x, y, dim); return;
    case DX_DRONE: dev_icon(g, DEV_DRONE, x, y, dim); return;
    case DX_AIRTAG: dev_icon(g, DEV_TAG, x, y, dim); return;
    case DX_SKIM: dev_icon(g, DEV_SKIM, x, y, dim); return;
    case DX_DEAUTH: dev_icon(g, DEV_DEAUTH, x, y, dim); return;
    default: break;
    }
    const int k = t == DX_KEYFOB ? 0 : t == DX_FLIP ? 1 : t == DX_SAMS ? 2 : 3;
    for (int r = 0; r < 12; ++r)
        for (int c = 0; c < 12; ++c) {
            const char ch = DX_ROW[k][r][c];
            if (ch != 'a' && ch != 'b' && ch != 'c') continue;
            uint8_t ink = DX_INK[k][ch - 'a'];
            if (dim) ink = dg_fade(ink, 60);
            g.fillRect(x + c, y + r, 1, 1, ink);
        }
}
/* the Dex panel (Defend board overlay: `dex`, Esc closes) */
static void paint_dex(Gfx g)
{
    const uint8_t ac = dg_ac();
    const int bx = 24, by = 24, bw = 272, bh = 196;
    GATE_BOX(g, "OCCLUDE", "dex", bx, by, bw, bh, "-");
    g.fillRect(bx, by, bw, bh, PAL_BLACK);
    g.drawRect(bx, by, bw, bh, ac);
    g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, dg_wire());
    g.fillRect(bx + 2, by + 2, bw - 4, 12, PAL_DEEP);
    const int keep = s_px; s_px = 6;
    txt(g, "DEVICE DEX", bx + 8, by + 4, ac, lgfx::textdatum::top_left);
    char b[24];
    snprintf(b, sizeof b, "%d/10", dex_have());
    txt(g, b, bx + bw - 8, by + 4, PAL_WHITE, lgfx::textdatum::top_right);
    const int cw = (bw - 8) / 2;
    for (int i = 0; i < DX_N; ++i) {
        const int cx = bx + 4 + (i / 5) * cw, cy = by + 20 + (i % 5) * 34;
        const bool got = s_dex[i] != 0;
        g.fillRect(cx, cy, 24, 24, PAL_BG);
        g.drawRect(cx, cy, 24, 24, got ? DX_BLIP[i] : (uint8_t)PAL_LINE);
        if (got) dex_icon(g, i, cx + 6, cy + 6, false);
        else txt(g, "?", cx + 9, cy + 8, PAL_MUTED, lgfx::textdatum::top_left);
        txt(g, got ? DX_NAME[i] : "???", cx + 30, cy + 4, got ? (uint8_t)PAL_IVORY : (uint8_t)PAL_MUTED, lgfx::textdatum::top_left);
        if (got) { snprintf(b, sizeof b, "caught x%d", s_dex[i]); txt(g, b, cx + 30, cy + 14, PAL_MUTED, lgfx::textdatum::top_left); }
    }
    s_px = keep;
}

/* ---------- NIGHT SHIFT ---------- */
static const int NS_C = 20, NS_R = 13, NS_T = 8, NS_X = 142, NS_Y = 31, NS_MAX = 10, NS_LIGHT = 3;
static const uint32_t NS_SHIFT_MS = 150000u, NS_SCAN_CD = 2000u, NS_SCAN_SHOW = 1600u, NS_RING_MS = 500u;
static const char *const NS_MAP[NS_R] = {
    "####################",
    "#......#......#....#",
    "#.f....#..ff..#..f.#",
    "#...........f......#",
    "#......#......#....#",
    "###.######.#####.###",
    "#..................#",
    "###.###.#####.###.##",
    "#....#......#......#",
    "#.f..#..ff..#..f...#",
    "#.........f........#",
    "#....#......#......#",
    "####################",
};
static const char *const NS_ROOM[6] = { "LOBBY", "SERVER", "OFFICE", "LAB", "STORAGE", "DOCK" };
static const int8_t NS_RX[6][4] = { { 1, 1, 6, 4 }, { 8, 1, 13, 4 }, { 15, 1, 18, 4 }, { 1, 8, 4, 11 }, { 6, 8, 11, 11 }, { 13, 8, 18, 11 } };
static const int8_t NS_ORDER[DX_N] = { DX_AIRTAG, DX_SKIM, DX_DRONE, DX_FLIP, DX_WIFI, DX_DEAUTH, DX_META, DX_BT, DX_KEYFOB, DX_SAMS };
struct NSDev { int8_t x, y, t, live; uint32_t next, caught; };
static NSDev s_ns[NS_MAX];
static int s_ns_n = 3, s_ns_shift = 1, s_ns_gx = 10, s_ns_gy = 6, s_ns_room = -1;
static float s_ns_alert = 0.f;
static uint32_t s_ns_scan = 0, s_ns_jam = 0, s_ns_jam_cd = 0, s_ns_room_t = 0, s_ns_msg_t = 0;
static bool s_ns_map = false;
static uint8_t s_ns_seen[NS_R][(NS_C + 7) / 8];
static const char *s_ns_msg = "";
static bool ns_wall(int x, int y) { return x < 0 || y < 0 || x >= NS_C || y >= NS_R || NS_MAP[y][x] == '#'; }
static int ns_dist(int ax, int ay, int bx, int by) { return abs(ax - bx) + abs(ay - by); }
static bool ns_lit(int x, int y) { const int dx = x - s_ns_gx, dy = y - s_ns_gy; return s_ns_map || dx * dx + dy * dy <= NS_LIGHT * NS_LIGHT + 1; }
static void ns_see(void)
{
    for (int y = 0; y < NS_R; ++y)
        for (int x = 0; x < NS_C; ++x)
            if (ns_lit(x, y)) s_ns_seen[y][x >> 3] |= (uint8_t)(1u << (x & 7));
}
static bool ns_seen(int x, int y) { return (s_ns_seen[y][x >> 3] >> (x & 7)) & 1u; }
static int ns_count(int shift) { static const int8_t N[5] = { 3, 5, 7, 9, 10 }; return N[(shift < 1 ? 1 : shift > 5 ? 5 : shift) - 1]; }
static bool ns_free(int x, int y)
{
    if (x == s_ns_gx && y == s_ns_gy) return false;
    for (int i = 0; i < s_ns_n; ++i) if (s_ns[i].live && s_ns[i].x == x && s_ns[i].y == y) return false;
    return true;
}
static void ns_reset(void)
{
    s_ns_n = ns_count(s_ns_shift);
    s_ns_gx = 10; s_ns_gy = 6;
    s_ns_alert = 0.f; s_ns_scan = 0; s_ns_jam = 0; s_ns_jam_cd = 0; s_ns_map = false; s_ns_room = -1; s_ns_msg = ""; s_ns_msg_t = 0;
    memset(s_ns_seen, 0, sizeof s_ns_seen);
    for (int i = 0; i < NS_MAX; ++i) s_ns[i].live = 0;
    for (int i = 0; i < s_ns_n; ++i) {
        NSDev &d = s_ns[i];
        d.t = NS_ORDER[i]; d.live = 1; d.caught = 0; d.next = s_dg_ms + 1000u + (uint32_t)i * 170u;
        for (int tries = 0; tries < 400; ++tries) {
            const int x = 1 + (int)(dg_rng() % (uint32_t)(NS_C - 2)), y = 1 + (int)(dg_rng() % (uint32_t)(NS_R - 2));
            const char ch = NS_MAP[y][x];
            if (d.t == DX_SKIM ? ch != 'f' : ch != '.') continue;      /* the skimmer hides on furniture */
            if (ns_dist(x, y, s_ns_gx, s_ns_gy) < 8 || !ns_free(x, y)) continue;
            d.x = (int8_t)x; d.y = (int8_t)y; break;
        }
    }
    ns_see();
}
static int ns_left(void) { int n = 0; for (int i = 0; i < s_ns_n; ++i) n += s_ns[i].live == 1; return n; }
static void ns_say(const char *m) { s_ns_msg = m; s_ns_msg_t = s_dg_ms + 1500u; }
static void ns_scan(void)
{
    if (s_dg_ms < s_ns_jam) { ns_say("SCAN JAMMED"); return; }
    if (s_ns_scan && s_dg_ms < s_ns_scan + NS_SCAN_CD) return;   /* cooldown (ring shown on the HUD stat) */
    s_ns_scan = s_dg_ms ? s_dg_ms : 1u;
}
static void ns_catch(void)
{
    for (int i = 0; i < s_ns_n; ++i) {
        NSDev &d = s_ns[i];
        if (d.live != 1 || ns_dist(d.x, d.y, s_ns_gx, s_ns_gy) > 1) continue;
        d.live = 2; d.caught = s_dg_ms;
        ++s_dg_score; dex_add(d.t);
        s_ns_alert -= 25.f; if (s_ns_alert < 0.f) s_ns_alert = 0.f;
        ns_say(DX_NAME[d.t]);
        return;
    }
    ns_say("NOTHING HERE");
}
static void ns_walk(int dir)
{
    const int nx = s_ns_gx + MZ_DX[dir], ny = s_ns_gy + MZ_DY[dir];
    if (ns_wall(nx, ny)) return;
    s_ns_gx = nx; s_ns_gy = ny;
    ns_see();
}
static void ns_step(float dt)
{
    const int live = ns_left();
    /* #6 tuning: 0.9 %/s with every device live (scaled by the share still live), +15 % per shift: shift 1
     * breaches in ~110 s untouched, shift 5 in ~70 s; each catch knocks 25 off. */
    if (s_ns_n > 0) s_ns_alert += dt * 0.9f * (1.f + 0.15f * (float)(s_ns_shift - 1)) * (float)live / (float)s_ns_n;
    for (int i = 0; i < s_ns_n; ++i) {
        NSDev &d = s_ns[i];
        if (d.live == 2 && s_dg_ms >= d.caught + 500u) d.live = 0;
        if (d.live != 1) continue;
        const int dist = ns_dist(d.x, d.y, s_ns_gx, s_ns_gy);
        if (d.t == DX_DEAUTH && dist <= 3 && s_dg_ms >= s_ns_jam_cd) {   /* jams the scanner 3 s, then 2 s grace */
            s_ns_jam = s_dg_ms + 3000u; s_ns_jam_cd = s_dg_ms + 5000u; ns_say("DEAUTH JAM");
        }
        if (s_dg_ms < d.next) continue;
        int mv = -1;
        if (d.t == DX_DRONE) { d.next = s_dg_ms + 1200u; mv = (int)(dg_rng() & 3u); }
        else if (d.t == DX_FLIP && dist <= 3) {                                      /* flees */
            d.next = s_dg_ms + 520u;
            int bd = dist;
            for (int k = 0; k < 4; ++k) {
                const int nx = d.x + MZ_DX[k], ny = d.y + MZ_DY[k];
                if (ns_wall(nx, ny) || !ns_free(nx, ny)) continue;
                const int q = ns_dist(nx, ny, s_ns_gx, s_ns_gy);
                if (q > bd) { bd = q; mv = k; }
            }
        } else if (d.t == DX_META && !ns_lit(d.x, d.y)) { d.next = s_dg_ms + 1000u; mv = (int)(dg_rng() & 3u); }
        else if (d.t == DX_BT) { d.next = s_dg_ms + 2500u; mv = (int)(dg_rng() & 3u); }
        else d.next = s_dg_ms + 600u;
        if (mv < 0) continue;
        const int nx = d.x + MZ_DX[mv], ny = d.y + MZ_DY[mv];
        const bool through = d.t == DX_DRONE;                                         /* drifts through walls */
        if (nx < 1 || ny < 1 || nx > NS_C - 2 || ny > NS_R - 2) continue;
        if ((!through && ns_wall(nx, ny)) || !ns_free(nx, ny)) continue;
        d.x = (int8_t)nx; d.y = (int8_t)ny;
    }
    if (s_ns_alert >= 100.f) { s_ns_alert = 100.f; s_dg_state = DGS_LOST; s_dg_miss = 3; ns_say("BREACH"); return; }
    if (ns_left() == 0 && live == 0) {
        bool anim = false;
        for (int i = 0; i < s_ns_n; ++i) anim |= s_ns[i].live == 2;
        if (!anim) { s_dg_state = DGS_WIN; s_dg_junk = (int)((NS_SHIFT_MS - (s_dg_ms < NS_SHIFT_MS ? s_dg_ms : NS_SHIFT_MS)) / 1000u); }
        return;
    }
    if (s_dg_ms >= NS_SHIFT_MS) { s_dg_state = DGS_LOST; s_dg_miss = 3; ns_say("6:00 AM"); }
}
static void ns_clock(char *b, int n)
{
    const uint32_t m = (s_dg_ms < NS_SHIFT_MS ? s_dg_ms : NS_SHIFT_MS) * 360u / NS_SHIFT_MS;   /* 360 game minutes */
    const unsigned h = (unsigned)(m / 60u), mm = (unsigned)(m % 60u);
    snprintf(b, n, "%02u:%02uAM", h == 0 ? 12u : h, mm);
}
static int ns_ring_r(void)
{
    if (!s_ns_scan || s_dg_ms >= s_ns_scan + NS_RING_MS) return -1;
    return (int)(((s_dg_ms - s_ns_scan) * 4u / NS_RING_MS) + 1u) * 2 * NS_T;   /* 4 steps out to 8 tiles (the scan range) */
}
static bool ns_blips(void) { return s_ns_scan && s_dg_ms < s_ns_scan + NS_SCAN_SHOW; }
/* circle outline clipped to the floor (the scan ring may reach past the board edge) */
static void ns_ring(Gfx g, int cx, int cy, int r, uint8_t ink)
{
    const int x0 = NS_X, y0 = NS_Y, x1 = NS_X + NS_C * NS_T, y1 = NS_Y + NS_R * NS_T;
    int x = r, y = 0, e = 1 - r;
    while (x >= y) {
        const int px[8] = { cx + x, cx - x, cx + x, cx - x, cx + y, cx - y, cx + y, cx - y };
        const int py[8] = { cy + y, cy + y, cy - y, cy - y, cy + x, cy + x, cy - x, cy - x };
        for (int k = 0; k < 8; ++k)
            if (px[k] >= x0 && px[k] < x1 && py[k] >= y0 && py[k] < y1) g.fillRect(px[k], py[k], 1, 1, ink);
        ++y;
        if (e < 0) e += 2 * y + 1; else { --x; e += 2 * (y - x) + 1; }
    }
}
static void ns_paint(Gfx g)
{
    const int W = NS_C * NS_T, H = NS_R * NS_T;
    g.fillRect(NS_X, NS_Y, W, H, PAL_BLACK);
    for (int y = 0; y < NS_R; ++y)
        for (int x = 0; x < NS_C; ++x) {
            const int px = NS_X + x * NS_T, py = NS_Y + y * NS_T;
            const char ch = NS_MAP[y][x];
            const bool lit = ns_lit(x, y), seen = ns_seen(x, y);
            if (!lit && !seen) continue;
            if (ch == '#') {
                const uint8_t c = lit ? dg_ac() : (uint8_t)dg_wire();
                g.fillRect(px + 3, py + 3, 2, 2, c);
                if (!ns_wall(x + 1, y) || x + 1 >= NS_C) {} else g.fillRect(px + 4, py + 3, NS_T, 2, c);
                if (y + 1 < NS_R && NS_MAP[y + 1][x] == '#') g.fillRect(px + 3, py + 4, 2, NS_T, c);
            } else if (lit) {
                g.fillRect(px, py, NS_T, NS_T, PAL_DEEP);          /* the guard's light pool */
                g.fillRect(px + 3, py + 3, 1, 1, PAL_LINE);
                if (ch == 'f') { g.fillRect(px + 1, py + 2, 6, 4, PAL_BOARD_D); g.drawRect(px + 1, py + 2, 6, 4, PAL_STEEL); }
            } else if (ch == 'f') g.drawRect(px + 1, py + 2, 6, 4, PAL_SEL);
        }
    if (s_ns_room >= 0 && s_dg_ms < s_ns_room_t) {                  /* digit: that room's outline + label */
        const int8_t *r = NS_RX[s_ns_room];
        g.drawRect(NS_X + r[0] * NS_T - 1, NS_Y + r[1] * NS_T - 1, (r[2] - r[0] + 1) * NS_T + 2, (r[3] - r[1] + 1) * NS_T + 2, PAL_AMBER);
        const int keep = s_px; s_px = 6;
        txt(g, NS_ROOM[s_ns_room], NS_X + r[0] * NS_T + 1, NS_Y + r[1] * NS_T + 1, PAL_AMBER, lgfx::textdatum::top_left);
        s_px = keep;
    }
    /* light radius rim around Gibson */
    const int rr = ns_ring_r();
    if (rr > 0) {
        ns_ring(g, NS_X + s_ns_gx * NS_T + 4, NS_Y + s_ns_gy * NS_T + 4, rr, PAL_CODE);
        ns_ring(g, NS_X + s_ns_gx * NS_T + 4, NS_Y + s_ns_gy * NS_T + 4, rr - 2, dg_fade(PAL_CODE, 50));
    }
    for (int i = 0; i < s_ns_n; ++i) {
        const NSDev &d = s_ns[i];
        if (!d.live) continue;
        const int cx = NS_X + d.x * NS_T + 4, cy = NS_Y + d.y * NS_T + 4;
        if (d.live == 2) {                                           /* catch flash */
            const int k = (int)((s_dg_ms - d.caught) / 125u);
            g.drawRect(cx - 4 - k * 2, cy - 4 - k * 2, 8 + k * 4, 8 + k * 4, PAL_OK);
            continue;
        }
        if (ns_lit(d.x, d.y)) {
            GATE_BOX(g, "DGOBJ", "device", cx - 6, cy - 6, 12, 12, "-");
            dex_icon(g, d.t, cx - 6, cy - 6, false);
        } else if (ns_blips()) {
            const int dist = ns_dist(d.x, d.y, s_ns_gx, s_ns_gy);
            const int range = d.t == DX_AIRTAG ? 5 : 8;
            if (dist > range) continue;
            const int r = d.t == DX_AIRTAG ? 1 : dist <= 3 ? 3 : dist <= 6 ? 2 : 1;
            const uint8_t ink = d.t == DX_AIRTAG ? dg_fade(DX_BLIP[d.t], 45) : DX_BLIP[d.t];
            g.fillCircle(cx, cy, r, ink);
            if (r >= 2) g.drawCircle(cx, cy, r + 2, dg_fade(ink, 55));
        }
    }
    const int keep_b = s_dg_bot; s_dg_bot = 12;                      /* Gibson is always the guard */
    dg_bot(g, NS_X + s_ns_gx * NS_T + 4, NS_Y + s_ns_gy * NS_T + 4, 1);
    s_dg_bot = keep_b;
    if (s_ns_msg_t && s_dg_ms < s_ns_msg_t && s_ns_msg[0]) {
        const int keep = s_px; s_px = 6;
        txt(g, s_ns_msg, NS_X + W - 2, NS_Y + H + 2, PAL_AMBER, lgfx::textdatum::top_right);
        s_px = keep;
    }
    if (s_ns_map) {
        const int keep = s_px; s_px = 6;
        for (int r = 0; r < 6; ++r) txt(g, NS_ROOM[r], NS_X + NS_RX[r][0] * NS_T + 1, NS_Y + NS_RX[r][1] * NS_T + 1, PAL_MUTED, lgfx::textdatum::top_left);
        txt(g, "FLOOR MAP", NS_X + 2, NS_Y + H + 2, dg_ac(), lgfx::textdatum::top_left);
        s_px = keep;
    }
}
static uint32_t ns_sig(void)
{
    uint32_t h = 2166136261u;
    auto mix = [&h](uint32_t v) { h = (h ^ v) * 16777619u; };
    mix((uint32_t)(s_ns_gx * 64 + s_ns_gy)); mix((uint32_t)ns_ring_r()); mix(ns_blips()); mix(s_ns_map);
    mix((uint32_t)(s_ns_room >= 0 && s_dg_ms < s_ns_room_t ? s_ns_room + 1 : 0));
    mix((uint32_t)(s_ns_msg_t && s_dg_ms < s_ns_msg_t) * (uint32_t)(uintptr_t)s_ns_msg);
    for (int i = 0; i < s_ns_n; ++i) {
        const NSDev &d = s_ns[i];
        mix((uint32_t)(d.live * 4096 + d.x * 64 + d.y));
        if (d.live == 2) mix((s_dg_ms - d.caught) / 125u);
    }
    return h;
}

/* ---------- SWEEP ---------- */
static const int SW_C = 15, SW_R = 11, SW_T = 10, SW_H = 5, SW_X = 147, SW_Y = 28, SW_G = 4;
static const char *const SW_MAP[SW_R] = {
    "###############",
    "#o.....#.....o#",
    "#.##.#.#.#.##.#",
    "#....#...#....#",
    "##.#.##D##.#.##",
    "#..#.#SSS#.#..#",
    "#.##.#####.##.#",
    "#......G......#",
    "#.##.#.#.#.##.#",
    "#o...#...#...o#",
    "###############",
};
static const int8_t SW_KIND[SW_G] = { DX_DRONE, DX_DEAUTH, DX_SKIM, DX_FLIP };
static uint8_t s_sw_pel[SW_R][SW_C];   /* 1 data bit, 2 ROOT node */
static uint8_t s_sw_sticky[SW_R][SW_C];/* skimmer goo: 100 ms ticks left */
static DGMover s_sw_p, s_sw_t[SW_G];
static int s_sw_left = 0, s_sw_total = 0, s_sw_lv = 1, s_sw_pts = 0, s_sw_burst = 1, s_sw_bonus_n = 0, s_sw_bonus_k = -1;
static uint32_t s_sw_fright[SW_G], s_sw_t0 = 0, s_sw_bonus_until = 0, s_sw_goo_t = 0, s_sw_goo_tick = 0;
static bool sw_open(int tx, int ty, bool trace)
{
    if (tx < 0 || ty < 0 || tx >= SW_C || ty >= SW_R) return false;
    const char c = SW_MAP[ty][tx];
    return c != '#' && (trace || (c != 'S' && c != 'D'));
}
static int sw_traces(void) { return s_sw_lv >= 3 ? 4 : 3; }
static void sw_place(void)
{
    s_sw_p.x = (float)(7 * SW_T + SW_H); s_sw_p.y = (float)(7 * SW_T + SW_H); s_sw_p.dir = -1; s_sw_p.want = -1;
    for (int i = 0; i < SW_G; ++i) {
        s_sw_t[i].x = (float)((6 + (i % 3)) * SW_T + SW_H); s_sw_t[i].y = (float)(5 * SW_T + SW_H); s_sw_t[i].dir = 0; s_sw_t[i].want = -1;
        s_sw_fright[i] = 0;
    }
    s_sw_t0 = s_dg_ms;
}
static void sw_reset(void)
{
    s_sw_left = 0;
    for (int y = 0; y < SW_R; ++y)
        for (int x = 0; x < SW_C; ++x) {
            const char ch = SW_MAP[y][x];
            s_sw_pel[y][x] = ch == '.' ? 1 : ch == 'o' ? 2 : 0;
            if (s_sw_pel[y][x] == 1) ++s_sw_left;
        }
    memset(s_sw_sticky, 0, sizeof s_sw_sticky);
    s_sw_total = s_sw_left; s_sw_pts = 0; s_sw_burst = 1; s_sw_bonus_n = 0; s_sw_bonus_k = -1; s_sw_bonus_until = 0;
    s_sw_goo_t = 4000u; s_sw_goo_tick = 0;
    sw_place();
}
static void sw_move(DGMover &m, float dist, int who)
{
    int guard = 0;
    while (dist > 0.001f && guard++ < 8) {
        const bool atc = ((int)lroundf(m.x) % SW_T) == SW_H && ((int)lroundf(m.y) % SW_T) == SW_H
                         && fabsf(m.x - lroundf(m.x)) < 0.001f && fabsf(m.y - lroundf(m.y)) < 0.001f;
        if (atc) {
            const int tx = (int)m.x / SW_T, ty = (int)m.y / SW_T;
            if (who < 0) {
                if (s_sw_pel[ty][tx] == 1) { s_sw_pel[ty][tx] = 0; --s_sw_left; ++s_dg_score; ++s_sw_pts; }
                else if (s_sw_pel[ty][tx] == 2) {
                    s_sw_pel[ty][tx] = 0; s_sw_pts += 5;
                    for (int i = 0; i < SW_G; ++i) s_sw_fright[i] = s_dg_ms + 5000u;
                }
                if (s_sw_bonus_k >= 0 && tx == 7 && ty == 7 && s_dg_ms < s_sw_bonus_until) {
                    dex_add(s_sw_bonus_k); s_sw_pts += 50; s_sw_bonus_k = -1;
                }
                if (m.want >= 0 && sw_open(tx + MZ_DX[m.want], ty + MZ_DY[m.want], false)) m.dir = m.want;
                if (m.dir >= 0 && !sw_open(tx + MZ_DX[m.dir], ty + MZ_DY[m.dir], false)) m.dir = -1;
            } else {
                const int rev = m.dir < 0 ? -1 : (m.dir ^ 1);
                int gx = (int)s_sw_p.x / SW_T, gy = (int)s_sw_p.y / SW_T;
                if (SW_MAP[ty][tx] == 'S' || SW_MAP[ty][tx] == 'D') { gx = 7; gy = 3; }   /* leave the sandbox first */
                else if (SW_KIND[who] == DX_DEAUTH && s_sw_p.dir >= 0) { gx += 2 * MZ_DX[s_sw_p.dir]; gy += 2 * MZ_DY[s_sw_p.dir]; }
                const bool fr = s_dg_ms < s_sw_fright[who];
                const float chase = fr ? 0.f : SW_KIND[who] == DX_DRONE ? 0.9f : SW_KIND[who] == DX_DEAUTH ? 0.75f
                                  : SW_KIND[who] == DX_FLIP ? 0.5f : 0.25f;
                const bool home = SW_MAP[ty][tx] == 'S' || SW_MAP[ty][tx] == 'D';
                int best = -1, bd = fr ? -1 : 1 << 30, nopt = 0, opt[4];
                for (int d = 0; d < 4; ++d) {
                    if (d == rev || !sw_open(tx + MZ_DX[d], ty + MZ_DY[d], true)) continue;
                    if (!home && (SW_MAP[ty + MZ_DY[d]][tx + MZ_DX[d]] == 'D')) continue;   /* never re-enter */
                    opt[nopt++] = d;
                    const int ex = tx + MZ_DX[d] - gx, ey = ty + MZ_DY[d] - gy, q = ex * ex + ey * ey;
                    if (fr ? q > bd : q < bd) { bd = q; best = d; }
                }
                if (nopt == 0) m.dir = (int8_t)rev;
                else m.dir = (int8_t)((home || dg_rngf() < (fr ? 0.85f : chase)) ? best : opt[dg_rng() % (uint32_t)nopt]);
            }
            if (m.dir < 0) return;
        }
        const float cxn = (float)(((int)m.x / SW_T) * SW_T + SW_H), cyn = (float)(((int)m.y / SW_T) * SW_T + SW_H);
        if (MZ_DX[m.dir]) {
            const float tgt = fabsf(m.x - cxn) < 0.001f ? cxn + (float)(MZ_DX[m.dir] * SW_T) : (MZ_DX[m.dir] > 0 ? (m.x < cxn ? cxn : cxn + SW_T) : (m.x > cxn ? cxn : cxn - SW_T));
            const float rem = fabsf(tgt - m.x), s = dist < rem ? dist : rem;
            m.x += (float)MZ_DX[m.dir] * s; dist -= s;
            if (s >= rem) m.x = tgt;
        } else {
            const float tgt = fabsf(m.y - cyn) < 0.001f ? cyn + (float)(MZ_DY[m.dir] * SW_T) : (MZ_DY[m.dir] > 0 ? (m.y < cyn ? cyn : cyn + SW_T) : (m.y > cyn ? cyn : cyn - SW_T));
            const float rem = fabsf(tgt - m.y), s = dist < rem ? dist : rem;
            m.y += (float)MZ_DY[m.dir] * s; dist -= s;
            if (s >= rem) m.y = tgt;
        }
    }
}
static void sw_burst(void)
{
    if (s_sw_burst <= 0) return;
    --s_sw_burst;
    for (int i = 0; i < sw_traces(); ++i)
        if (fabsf(s_sw_t[i].x - s_sw_p.x) <= 4.f * SW_T && fabsf(s_sw_t[i].y - s_sw_p.y) <= 4.f * SW_T) s_sw_fright[i] = s_dg_ms + 3000u;
}
static void sw_step(float dt)
{
    if (s_dg_ms < s_dg_hold) return;
    /* sticky goo fades in 100 ms ticks */
    if (s_dg_ms >= s_sw_goo_tick + 100u) {
        s_sw_goo_tick = s_dg_ms;
        for (int y = 0; y < SW_R; ++y) for (int x = 0; x < SW_C; ++x) if (s_sw_sticky[y][x]) --s_sw_sticky[y][x];
    }
    const int ptx = (int)s_sw_p.x / SW_T, pty = (int)s_sw_p.y / SW_T;
    sw_move(s_sw_p, (s_sw_sticky[pty][ptx] ? 20.f : 40.f) * dt, -1);
    const float spd = 30.f * (1.f + 0.08f * (float)(s_sw_lv - 1));
    for (int i = 0; i < sw_traces(); ++i) {
        if (s_dg_ms - s_sw_t0 < (uint32_t)(2000 * (i + 1))) continue;      /* leave the sandbox one by one */
        const bool fr = s_dg_ms < s_sw_fright[i];
        sw_move(s_sw_t[i], (fr ? 18.f : spd) * dt, i);
        if (SW_KIND[i] == DX_SKIM && s_dg_ms >= s_sw_goo_t) {                /* skimmer drops a sticky tile */
            s_sw_goo_t = s_dg_ms + 4000u;
            const int sx = (int)s_sw_t[i].x / SW_T, sy = (int)s_sw_t[i].y / SW_T;
            if (sw_open(sx, sy, false)) s_sw_sticky[sy][sx] = 60;
        }
        if (fabsf(s_sw_t[i].x - s_sw_p.x) < 6.f && fabsf(s_sw_t[i].y - s_sw_p.y) < 6.f) {
            if (fr) {
                s_sw_pts += 10;
                s_sw_t[i].x = (float)(7 * SW_T + SW_H); s_sw_t[i].y = (float)(5 * SW_T + SW_H); s_sw_t[i].dir = 0;
                s_sw_fright[i] = 0;
            } else {
                ++s_dg_miss;
                s_dg_hold = s_dg_ms + 900u;
                sw_place();
                break;
            }
        }
    }
    /* bonus device: twice per level, at 30 % and 70 % of the bits, 8 s at the centre */
    const int eaten = s_sw_total - s_sw_left;
    if (s_sw_bonus_n < 2 && eaten * 10 >= s_sw_total * (s_sw_bonus_n == 0 ? 3 : 7)) {
        ++s_sw_bonus_n;
        s_sw_bonus_k = (int)(dg_rng() % (uint32_t)DX_N);
        s_sw_bonus_until = s_dg_ms + 8000u;
    }
    if (s_sw_bonus_k >= 0 && s_dg_ms >= s_sw_bonus_until) s_sw_bonus_k = -1;
    if (s_sw_left <= 0) s_dg_state = DGS_WIN;
    else if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
}
static void sw_paint(Gfx g)
{
    const uint8_t wire = dg_wire(), core = dg_ac();
    g.fillRect(SW_X, SW_Y, SW_C * SW_T, SW_R * SW_T, PAL_BLACK);
    for (int y = 0; y < SW_R; ++y)
        for (int x = 0; x < SW_C; ++x) {
            const char ch = SW_MAP[y][x];
            const int cx = SW_X + x * SW_T + SW_H, cy = SW_Y + y * SW_T + SW_H;
            if (ch == '#') {
                g.fillRect(cx - 1, cy - 1, 3, 3, wire);
                if (x + 1 < SW_C && SW_MAP[y][x + 1] == '#') { g.fillRect(cx, cy - 1, SW_T, 3, wire); g.fillRect(cx, cy, SW_T, 1, core); }
                if (y + 1 < SW_R && SW_MAP[y + 1][x] == '#') { g.fillRect(cx - 1, cy, 3, SW_T, wire); g.fillRect(cx, cy, 1, SW_T, core); }
                g.fillRect(cx, cy, 1, 1, core);
            } else if (ch == 'S') g.fillRect(cx - 4, cy - 4, SW_T - 1, SW_T - 1, PAL_DEEP);
            else if (ch == 'D') g.fillRect(cx - 4, cy, SW_T - 1, 2, PAL_HOLD);
            if (s_sw_sticky[y][x]) { g.fillRect(cx - 3, cy + 2, 7, 2, (uint8_t)207); g.fillRect(cx - 2, cy + 1, 5, 1, (uint8_t)207); }
            if (s_sw_pel[y][x] == 1) dg_bit(g, cx - 1, cy - 2, ((x + y) & 1) != 0, 1, PAL_CODE);
            else if (s_sw_pel[y][x] == 2) {                                     /* ROOT power node: a tiny gold-pin chip */
                const bool on = ((s_dg_ms / 250u) & 1u) != 0;
                g.fillRect(cx - 3, cy - 3, 7, 7, PAL_BLACK);
                g.drawRect(cx - 3, cy - 3, 7, 7, on ? (uint8_t)PAL_AMBER : (uint8_t)PAL_CHIN);
                g.fillRect(cx - 1, cy - 1, 3, 3, PAL_IVORY);
            }
        }
    if (s_sw_bonus_k >= 0) {
        GATE_BOX(g, "DGOBJ", "bonus", SW_X + 7 * SW_T - 1, SW_Y + 7 * SW_T - 1, 12, 12, "-");
        dex_icon(g, s_sw_bonus_k, SW_X + 7 * SW_T - 1, SW_Y + 7 * SW_T - 1, false);
    }
    for (int i = 0; i < sw_traces(); ++i) {
        const int jx = SW_X + (int)lroundf(s_sw_t[i].x) - 6, jy = SW_Y + (int)lroundf(s_sw_t[i].y) - 6;
        const bool fr = s_dg_ms < s_sw_fright[i];
        GATE_BOX(g, "DGOBJ", "trace", jx, jy, 12, 12, "-");
        if (fr) { g.fillRect(jx, jy, 12, 12, PAL_SOLID_B); g.drawRect(jx, jy, 12, 12, PAL_WHITE); }
        dex_icon(g, SW_KIND[i], jx, jy, fr);
    }
    const int keep_b = s_dg_bot; s_dg_bot = 12;
    dg_bot(g, SW_X + (int)lroundf(s_sw_p.x), SW_Y + (int)lroundf(s_sw_p.y), 1);
    s_dg_bot = keep_b;
}
static uint32_t sw_sig(void)
{
    uint32_t h = 2166136261u;
    auto mix = [&h](uint32_t v) { h = (h ^ v) * 16777619u; };
    mix((uint32_t)lroundf(s_sw_p.x) * 512u + (uint32_t)lroundf(s_sw_p.y)); mix((uint32_t)s_sw_left);
    mix((uint32_t)(s_dg_ms / 250u) & 1u); mix((uint32_t)(s_sw_bonus_k + 1));
    for (int i = 0; i < SW_G; ++i) {
        mix((uint32_t)lroundf(s_sw_t[i].x) * 512u + (uint32_t)lroundf(s_sw_t[i].y));
        mix(s_dg_ms < s_sw_fright[i]);
    }
    for (int y = 0; y < SW_R; ++y) for (int x = 0; x < SW_C; ++x) { mix(s_sw_pel[y][x]); mix(s_sw_sticky[y][x] != 0); }
    return h;
}

#ifdef BOTCORPS_HOST_RENDER
static bool dg_key(int key, char typed);
/* host autopilot for the two Gibson games (renders + logic proofs only), same keys as a player */
static int8_t s_ai_kx[NS_MAX], s_ai_ky[NS_MAX];   /* where the autopilot last saw / pinged each device (-1 unknown) */
static int gib_bfs_first(bool sweep, int sx, int sy, int *tx, int *ty)
{
    static int8_t q[SW_R * SW_C > NS_R * NS_C ? SW_R * SW_C : NS_R * NS_C][2];
    static int8_t from[NS_R > SW_R ? NS_R : SW_R][NS_C > SW_C ? NS_C : SW_C];
    const int C = sweep ? SW_C : NS_C, R = sweep ? SW_R : NS_R;
    memset(from, -1, sizeof from);
    int h = 0, n = 0; q[n][0] = (int8_t)sx; q[n][1] = (int8_t)sy; ++n; from[sy][sx] = 4;
    int gx = -1, gy = -1;
    while (h < n) {
        const int x = q[h][0], y = q[h][1]; ++h;
        bool goal = false;
        if (sweep) goal = s_sw_pel[y][x] && !(x == sx && y == sy);
        else {
            bool any = false;
            for (int i = 0; i < s_ns_n; ++i) {
                if (s_ns[i].live != 1 || s_ai_kx[i] < 0) continue;
                any = true;
                if (ns_dist(s_ai_kx[i], s_ai_ky[i], x, y) <= 1) goal = true;
            }
            if (!any && !ns_seen(x, y) && NS_MAP[y][x] != '#') goal = true;   /* nothing known: explore the dark */
        }
        if (goal) { gx = x; gy = y; break; }
        for (int d = 0; d < 4; ++d) {
            const int nx = x + MZ_DX[d], ny = y + MZ_DY[d];
            if (nx < 0 || ny < 0 || nx >= C || ny >= R || from[ny][nx] >= 0) continue;
            if (sweep ? !sw_open(nx, ny, false) : ns_wall(nx, ny)) continue;
            if (sweep) {                                  /* keep two tiles from a live trace */
                bool hot = false;
                for (int i = 0; i < sw_traces(); ++i)
                    if (s_dg_ms >= s_sw_fright[i] && abs((int)s_sw_t[i].x / SW_T - nx) + abs((int)s_sw_t[i].y / SW_T - ny) <= 2) hot = true;
                if (hot) continue;
            }
            from[ny][nx] = (int8_t)d; q[n][0] = (int8_t)nx; q[n][1] = (int8_t)ny; ++n;
        }
    }
    *tx = gx; *ty = gy;
    if (gx < 0) return -1;
    if (gx == sx && gy == sy) return 4;
    int x = gx, y = gy, d = from[y][x];
    while (!(x - MZ_DX[d] == sx && y - MZ_DY[d] == sy)) { x -= MZ_DX[d]; y -= MZ_DY[d]; d = from[y][x]; }
    return d;
}
static void gib_ai(void)
{
    static const int KD[4] = { K_UP, K_DN, K_LT, K_RT };
    if (s_dg == DG_NIGHT) {
        static uint32_t next = 0, prev = 0;
        if (s_dg_ms < prev) next = 0;                     /* a new round restarted the clock */
        prev = s_dg_ms;
        if (s_dg_ms < next) return;
        next = s_dg_ms + 250u;                            /* 4 key presses / s, a steady keyboard player */
        if (s_dg_ms < 300u) for (int i = 0; i < NS_MAX; ++i) s_ai_kx[i] = s_ai_ky[i] = -1;
        for (int i = 0; i < s_ns_n; ++i) {                /* only what a player could see: lit, or pinged */
            const NSDev &d = s_ns[i];
            const int range = d.t == DX_AIRTAG ? 5 : 8;
            if (d.live == 1 && (ns_lit(d.x, d.y) || (ns_blips() && s_dg_ms < s_ns_scan + 100u && ns_dist(d.x, d.y, s_ns_gx, s_ns_gy) <= range))) {
                s_ai_kx[i] = d.x; s_ai_ky[i] = d.y;
            } else if (s_ai_kx[i] >= 0 && ns_lit(s_ai_kx[i], s_ai_ky[i]) && !(d.x == s_ai_kx[i] && d.y == s_ai_ky[i])) s_ai_kx[i] = s_ai_ky[i] = -1;
        }
        if ((!s_ns_scan || s_dg_ms >= s_ns_scan + 4000u) && s_dg_ms >= s_ns_jam) { dg_key(K_CHAR, ' '); return; }
        int tx, ty;
        const int d = gib_bfs_first(false, s_ns_gx, s_ns_gy, &tx, &ty);
        if (d == 4) dg_key(K_OK, 0);
        else if (d >= 0) dg_key(KD[d], 0);
    } else {
        const bool atc = ((int)lroundf(s_sw_p.x) % SW_T) == SW_H && ((int)lroundf(s_sw_p.y) % SW_T) == SW_H;
        if (!atc && s_sw_p.dir >= 0) return;
        int tx, ty;
        const int d = gib_bfs_first(true, (int)s_sw_p.x / SW_T, (int)s_sw_p.y / SW_T, &tx, &ty);
        if (d >= 0 && d < 4 && s_sw_p.want != d) dg_key(KD[d], 0);
        for (int i = 0; i < sw_traces(); ++i)
            if (s_sw_burst && s_dg_ms >= s_sw_fright[i] && fabsf(s_sw_t[i].x - s_sw_p.x) + fabsf(s_sw_t[i].y - s_sw_p.y) < 16.f) dg_key(K_CHAR, ' ');
    }
}
#endif
