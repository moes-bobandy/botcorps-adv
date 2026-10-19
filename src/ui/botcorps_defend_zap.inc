/* ===== v1019v: Gibson Hackers zap Defend (full-width scan playfield) ============================
 * Mock v5 (Mobandy glass PASS 2026-10-19): NO left chip/rail; pink-border scan spans x0-319 under
 * status, above the cyan prompt; centre 5-pin gold Gibson (+15% vs old left-chip ref); branching
 * PCB traces to support pads + silver-pin bot chips; devices travel traces; type a command to zap
 * before dock; docked = TIME window; mid-zap hits the DEVICE with a NULL callout (not Gibson).
 * Prompt-corner Gibson = larger CMD button. Gibson ONLY (s_defend_bot == 12). Other bots keep the
 * old in-board DG_* games. Included from botcorps_defend.inc. */
enum {
    ZAP_BX0 = 0, ZAP_BY0 = 17, ZAP_BX1 = 320, ZAP_BY1 = 148,   /* GRID_BORDER exclusive */
    ZAP_IX0 = 4, ZAP_IY0 = 21, ZAP_IX1 = 316, ZAP_IY1 = 144,   /* GRID_INNER */
    ZAP_CS = 40,                                               /* centre Gibson: 34*1.15 ~ 39 -> 40 */
    ZAP_NTR = 6, ZAP_MAX = 6, ZAP_WIN = 12, ZAP_TIME_MS = 2800u, ZAP_NULL_MS = 700u
};
/* Trace ids match mock riders: wifi camera bt drone flock tower */
static const char *const ZAP_CMD[ZAP_NTR] = { "wifi", "camera", "bt", "drone", "flock", "tower" };
static const char *const ZAP_ALIAS[ZAP_NTR] = { "ap", "cam", "bluetooth", "uav", "flockbot", "hack" };
static const int8_t ZAP_BOT[ZAP_NTR] = { 0, 8, 1, 5, 4, 3 };          /* face colour bots */
static const int8_t ZAP_ICON[ZAP_NTR] = { GL_AP, GL_CAMERA, GL_SPEAKER, GL_DRONE, GL_CAMERA, GL_DEVBOARD };
/* Polyline waypoints relative to centre (cx,cy)=(160,82); ends near support pads. Fixed mock-like. */
static const int16_t ZAP_PT[ZAP_NTR][5][2] = {
    { { -10, -18 }, { -10, -30 }, { -120, -30 }, { -120, -46 }, { -142, -46 } }, /* wifi UL */
    { { -18, -6 }, { -50, -6 }, { -50, -20 }, { -110, -20 }, { -138, -8 } },     /* camera L */
    { { 8, 18 }, { 8, 32 }, { 105, 32 }, { 105, 44 }, { 140, 44 } },             /* bt BR */
    { { 18, -4 }, { 42, -4 }, { 42, -40 }, { 132, -40 }, { 144, -46 } },         /* drone UR */
    { { -6, 18 }, { -6, 28 }, { -90, 28 }, { -90, 44 }, { -132, 44 } },          /* flock BL */
    { { 18, 6 }, { 58, 6 }, { 58, 24 }, { 142, 24 }, { 142, 24 } },              /* tower R */
};
struct ZapDev { float dist; int8_t tr; int8_t live; int8_t dock; }; /* dock=1 in TIME window */
static ZapDev s_zap[ZAP_MAX];
static int s_zap_null_tr = -1;
static uint32_t s_zap_null_until = 0, s_zap_fx_until = 0;
static float s_zap_fx_from = 0.f, s_zap_fx_to = 0.f;
static int8_t s_zap_fx_tr = -1;
static int s_zap_log_n = 0;
static char s_zap_log[4][36];
static uint32_t s_zap_dock_t0[ZAP_MAX];
static uint8_t s_zap_dock_seen[ZAP_MAX];

static bool dg_zap_full(void)
{
    /* Full-width chrome when Gibson is on Defend and idle-demo or playing ZAP (not maze/night). */
    if (s_defend_bot >= 0 && s_defend_bot < 12) return false;
    if (!dg_on()) return true;                 /* idle demo = own game = ZAP */
    return s_dg == DG_ZAP;
}

static void zap_log(const char *msg)
{
    if (s_zap_log_n < 4) {
        snprintf(s_zap_log[s_zap_log_n], sizeof s_zap_log[0], "%s", msg);
        ++s_zap_log_n;
    } else {
        for (int i = 0; i < 3; ++i) memcpy(s_zap_log[i], s_zap_log[i + 1], sizeof s_zap_log[0]);
        snprintf(s_zap_log[3], sizeof s_zap_log[0], "%s", msg);
    }
}

static float zap_seglen(int tr, int i)
{
    const int dx = (int)ZAP_PT[tr][i + 1][0] - (int)ZAP_PT[tr][i][0];
    const int dy = (int)ZAP_PT[tr][i + 1][1] - (int)ZAP_PT[tr][i][1];
    const int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    /* octagon approx — no libm dependency in the include */
    return (float)(adx > ady ? adx + (ady >> 1) : ady + (adx >> 1));
}
static float zap_len(int tr)
{
    float L = 0.f;
    for (int i = 0; i < 4; ++i) L += zap_seglen(tr, i);
    return L;
}
static void zap_at(int tr, float dist, int *ox, int *oy)
{
    const int cx = 160, cy = 82;
    float left = dist;
    for (int i = 0; i < 4; ++i) {
        const float L = zap_seglen(tr, i);
        if (left <= L || i == 3) {
            const float u = L > 0.f ? left / L : 0.f;
            const float u2 = u > 1.f ? 1.f : u;
            *ox = cx + (int)ZAP_PT[tr][i][0] + (int)(((int)ZAP_PT[tr][i + 1][0] - (int)ZAP_PT[tr][i][0]) * u2);
            *oy = cy + (int)ZAP_PT[tr][i][1] + (int)(((int)ZAP_PT[tr][i + 1][1] - (int)ZAP_PT[tr][i][1]) * u2);
            return;
        }
        left -= L;
    }
}

static void zap_reset(void)
{
    for (int i = 0; i < ZAP_MAX; ++i) { s_zap[i].live = 0; s_zap_dock_seen[i] = 0; }
    s_zap_null_tr = -1; s_zap_null_until = 0;
    s_zap_fx_tr = -1; s_zap_fx_until = 0;
    s_zap_log_n = 0;
    zap_log("[ ** ] board armed. traces live");
    zap_log("type wifi/cam/bt/drone/flock/tower");
}

static void zap_spawn(void)
{
    int nlive = 0;
    for (int i = 0; i < ZAP_MAX; ++i) if (s_zap[i].live) ++nlive;
    if (nlive >= 4) return;
    /* Prefer an empty trace. */
    int used[ZAP_NTR] = { 0 };
    for (int i = 0; i < ZAP_MAX; ++i) if (s_zap[i].live) used[s_zap[i].tr] = 1;
    int free_n = 0, free_tr[ZAP_NTR];
    for (int t = 0; t < ZAP_NTR; ++t) if (!used[t]) free_tr[free_n++] = t;
    if (!free_n) return;
    const int tr = free_tr[dg_rng() % (uint32_t)free_n];
    for (int i = 0; i < ZAP_MAX; ++i) if (!s_zap[i].live) {
        s_zap[i].live = 1; s_zap[i].dock = 0; s_zap[i].tr = (int8_t)tr;
        s_zap[i].dist = 6.f + (float)(dg_rng() % 12u);
        return;
    }
}

static void zap_do_hit(int di)
{
    const int tr = s_zap[di].tr;
    s_zap_fx_tr = (int8_t)tr;
    s_zap_fx_from = 4.f;
    s_zap_fx_to = s_zap[di].dist > 8.f ? s_zap[di].dist - 4.f : s_zap[di].dist;
    s_zap_fx_until = s_dg_ms + ZAP_NULL_MS;
    s_zap_null_tr = tr;
    s_zap_null_until = s_dg_ms + ZAP_NULL_MS;
    char b[36];
    snprintf(b, sizeof b, "[ !! ] NULL - %s", ZAP_CMD[tr]);
    zap_log(b);
    s_zap[di].live = 0;
    ++s_dg_score;
    if (s_dg_score >= ZAP_WIN) s_dg_state = DGS_WIN;
}

static bool zap_try_cmd(const char *line)
{
    if (!line || !line[0] || s_dg_state != DGS_PLAY) return false;
    char cmd[20];
    snprintf(cmd, sizeof cmd, "%s", line);
    for (char *p = cmd; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    int tr = -1;
    for (int i = 0; i < ZAP_NTR; ++i)
        if (strcmp(cmd, ZAP_CMD[i]) == 0 || strcmp(cmd, ZAP_ALIAS[i]) == 0) { tr = i; break; }
    if (tr < 0) return false;
    /* Prefer a docked target on that trace, else the nearest live on that trace. */
    int best = -1;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live || s_zap[i].tr != tr) continue;
        if (s_zap[i].dock) { best = i; break; }
        if (best < 0 || s_zap[i].dist > s_zap[best].dist) best = i;
    }
    if (best < 0) {
        char b[36];
        snprintf(b, sizeof b, "[ -- ] no %s on traces", ZAP_CMD[tr]);
        zap_log(b);
        return true; /* consumed as a zap attempt */
    }
    zap_do_hit(best);
    return true;
}

static void zap_step(float dt)
{
    if (s_dg_state != DGS_PLAY) return;
    /* Spawn cadence rises with score. */
    if (dg_rngf() < dt * (0.55f + 0.04f * (float)s_dg_score)) zap_spawn();
    const float spd = 22.f + 1.6f * (float)s_dg_score;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live) continue;
        const float L = zap_len(s_zap[i].tr);
        if (!s_zap[i].dock) {
            s_zap[i].dist += spd * dt;
            if (s_zap[i].dist >= L - 3.f) {
                s_zap[i].dist = L - 2.f;
                s_zap[i].dock = 1;
                s_zap[i].dist = L - 2.f; /* park */
                /* Encode dock deadline in unused: reuse hold via per-device - store as negative score bucket.
                 * Use s_dg_hold as global; track per-device via dist field abuse: store deadline in float bits via parallel.
                 * Simpler: use miss timer on first dock via s_dg_ms stamp in a parallel array. */
            }
        }
    }
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live) { s_zap_dock_seen[i] = 0; continue; }
        if (s_zap[i].dock) {
            if (!s_zap_dock_seen[i]) {
                s_zap_dock_seen[i] = 1;
                s_zap_dock_t0[i] = s_dg_ms;
                char b[36];
                snprintf(b, sizeof b, "[ !! ] %s docked - TIME", ZAP_CMD[s_zap[i].tr]);
                zap_log(b);
            } else if (s_dg_ms - s_zap_dock_t0[i] >= ZAP_TIME_MS) {
                s_zap[i].live = 0;
                s_zap_dock_seen[i] = 0;
                ++s_dg_miss;
                zap_log("[ XX ] breach - device docked");
                if (s_dg_miss >= 3) s_dg_state = DGS_LOST;
            }
        } else s_zap_dock_seen[i] = 0;
    }
    if (s_dg_ms >= s_zap_null_until) s_zap_null_tr = -1;
    if (s_dg_ms >= s_zap_fx_until) s_zap_fx_tr = -1;
}

/* ---- paint helpers (palette indices only; host + device) ---- */
static void zap_pink_rim(Gfx g)
{
    const int x0 = ZAP_BX0, y0 = ZAP_BY0, x1 = ZAP_BX1, y1 = ZAP_BY1;
    /* outer soft pink (HOLD), inner hot (BFB72 / red-pink), outer soft — matches mock 4px rim */
    g.fillRect(x0, y0, x1 - x0, 1, PAL_HOLD);
    g.fillRect(x0, y0 + 1, x1 - x0, 2, PAL_BFB72);
    g.fillRect(x0, y0 + 3, x1 - x0, 1, PAL_HOLD);
    g.fillRect(x0, y1 - 1, x1 - x0, 1, PAL_HOLD);
    g.fillRect(x0, y1 - 3, x1 - x0, 2, PAL_BFB72);
    g.fillRect(x0, y1 - 4, x1 - x0, 1, PAL_HOLD);
    g.fillRect(x0, y0, 1, y1 - y0, PAL_HOLD);
    g.fillRect(x0 + 1, y0, 2, y1 - y0, PAL_BFB72);
    g.fillRect(x0 + 3, y0, 1, y1 - y0, PAL_HOLD);
    g.fillRect(x1 - 1, y0, 1, y1 - y0, PAL_HOLD);
    g.fillRect(x1 - 3, y0, 2, y1 - y0, PAL_BFB72);
    g.fillRect(x1 - 4, y0, 1, y1 - y0, PAL_HOLD);
    /* corner hot pips */
    g.fillRect(x0 + 1, y0 + 1, 2, 2, PAL_TERM_M);
    g.fillRect(x1 - 3, y0 + 1, 2, 2, PAL_TERM_M);
    g.fillRect(x0 + 1, y1 - 3, 2, 2, PAL_TERM_M);
    g.fillRect(x1 - 3, y1 - 3, 2, 2, PAL_TERM_M);
}

static void zap_grid_bg(Gfx g)
{
    g.fillRect(ZAP_IX0, ZAP_IY0, ZAP_IX1 - ZAP_IX0, ZAP_IY1 - ZAP_IY0, PAL_BLACK);
    for (int x = ZAP_IX0 + 6; x < ZAP_IX1; x += 8)
        g.fillRect(x, ZAP_IY0, 1, ZAP_IY1 - ZAP_IY0, PAL_BG);
    for (int y = ZAP_IY0 + 6; y < ZAP_IY1; y += 8)
        g.fillRect(ZAP_IX0, y, ZAP_IX1 - ZAP_IX0, 1, PAL_BG);
    for (int x = ZAP_IX0 + 24; x < ZAP_IX1; x += 32)
        g.fillRect(x, ZAP_IY0, 1, ZAP_IY1 - ZAP_IY0, PAL_LINE);
    for (int y = ZAP_IY0 + 24; y < ZAP_IY1; y += 32)
        g.fillRect(ZAP_IX0, y, ZAP_IX1 - ZAP_IX0, 1, PAL_PLATE);
    /* sparse via dots (deterministic) */
    uint32_t rng = 3u;
    for (int n = 0; n < 40; ++n) {
        rng = rng * 1664525u + 1013904223u;
        const int x = ZAP_IX0 + 4 + (int)((rng >> 8) % (uint32_t)(ZAP_IX1 - ZAP_IX0 - 8));
        rng = rng * 1664525u + 1013904223u;
        const int y = ZAP_IY0 + 4 + (int)((rng >> 8) % (uint32_t)(ZAP_IY1 - ZAP_IY0 - 8));
        g.fillRect(x, y, 1, 1, (rng & 1u) ? (uint8_t)PAL_CYAN : (uint8_t)PAL_TERM_M);
    }
}

static void zap_device_rain(Gfx g)
{
    /* Dim device icons only (no 0/1 bits) inside the grid. */
    static const int8_t icons[8] = { GL_CAMERA, GL_AP, GL_DRONE, GL_SPEAKER, GL_TRACKER, GL_TV, GL_WATCH, GL_DEVBOARD };
    uint32_t rng = 1019u;
    for (int cx = ZAP_IX0 + 4; cx < ZAP_IX1; cx += 11) {
        rng = rng * 1664525u + 1013904223u;
        int y = ZAP_IY0 - (int)(rng % 14u);
        while (y < ZAP_IY1) {
            rng = rng * 1664525u + 1013904223u;
            if ((rng % 100u) < 22u) { y += 8 + (int)(rng % 6u); continue; }
            const int ik = icons[rng % 8u];
            if (y >= ZAP_IY0 && y + 10 < ZAP_IY1 && cx - 6 > ZAP_IX0 && cx + 6 < ZAP_IX1)
                gl_icon(g, ik, cx - 6, y, true);   /* dim */
            y += 10 + (int)(rng % 7u);
        }
    }
}

static void zap_trace(Gfx g, int tr)
{
    const int cx = 160, cy = 82;
    int px = cx + ZAP_PT[tr][0][0], py = cy + ZAP_PT[tr][0][1];
    const uint8_t accent = bot_color(ZAP_BOT[tr]);
    for (int i = 0; i < 4; ++i) {
        const int qx = cx + ZAP_PT[tr][i + 1][0], qy = cy + ZAP_PT[tr][i + 1][1];
        /* thick copper-ish poly (approx with rects along major axis) */
        const int dx = qx - px, dy = qy - py;
        const int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
        const int n = steps > 0 ? steps : 1;
        for (int s = 0; s <= n; ++s) {
            const int x = px + dx * s / n, y = py + dy * s / n;
            g.fillRect(x - 1, y - 1, 3, 3, PAL_BOARD_D);
            g.fillRect(x, y, 1, 1, PAL_SOLID_A);
        }
        if (i > 0) {
            g.fillRect(px - 2, py - 2, 5, 5, PAL_BOARD_D);
            g.fillRect(px, py, 1, 1, accent);
        }
        px = qx; py = qy;
    }
    /* end pad */
    g.drawRect(px - 3, py - 3, 7, 7, PAL_SOLID_A);
    g.fillRect(px - 1, py - 1, 3, 3, accent);
}

static void zap_support(Gfx g, int tr)
{
    int x, y; zap_at(tr, zap_len(tr), &x, &y);
    /* small IC / res / cap mark beside end */
    g.fillRect(x - 10, y - 4, 8, 7, PAL_BG);
    g.drawRect(x - 10, y - 4, 8, 7, PAL_STEEL);
    g.fillRect(x - 12, y - 2, 2, 1, PAL_SOFT);
    g.fillRect(x - 12, y + 1, 2, 1, PAL_SOFT);
    g.fillRect(x - 12, y + 4, 2, 1, PAL_SOFT);
    /* silver-pin mini bot chip marking the circuit */
    chip_mini(g, x + 8, y, 14, ZAP_BOT[tr], false, 0);
}

static void zap_draw_bolt(Gfx g, int tr, float d0, float d1)
{
    int n = (int)((d1 - d0) / 4.f); if (n < 5) n = 5;
    int lx = 0, ly = 0;
    uint32_t rng = 7u;
    for (int i = 0; i <= n; ++i) {
        const float dd = d0 + (d1 - d0) * (float)i / (float)n;
        int x, y; zap_at(tr, dd, &x, &y);
        if (i > 0 && i < n) {
            rng = rng * 1664525u + 1013904223u;
            const int amp = (i & 1) ? 3 : -3;
            /* perpendicular jitter approx */
            x += amp; y += (rng & 1u) ? 1 : -1;
        }
        if (i) {
            g.fillRect((lx + x) / 2 - 1, (ly + y) / 2 - 1, 3, 3, PAL_SOLID_C);
            g.fillRect((lx + x) / 2, (ly + y) / 2, 1, 1, PAL_WHITE);
        }
        lx = x; ly = y;
    }
}

static void zap_null_label(Gfx g, int x, int y)
{
    const int keep = s_px; s_px = 6;
    const int tw = 4 * 6;
    int lx = x - tw / 2; if (lx < ZAP_IX0 + 4) lx = ZAP_IX0 + 4;
    if (lx + tw + 4 > ZAP_IX1) lx = ZAP_IX1 - tw - 4;
    int ly = y - 18; if (ly < ZAP_IY0 + 4) ly = ZAP_IY0 + 4;
    g.fillRect(lx - 4, ly - 4, tw + 8, 14, PAL_PLATE);
    g.drawRect(lx - 4, ly - 4, tw + 8, 14, PAL_TERM_M);
    g.drawRect(lx - 3, ly - 3, tw + 6, 12, PAL_BFB72);
    g.fillRect(lx - 2, ly - 2, tw + 4, 10, PAL_BLACK);
    txt(g, "NULL", lx, ly, PAL_TERM_M, lgfx::textdatum::top_left);
    s_px = keep;
}

static void zap_time_urgency(Gfx g, int dx, int dy, int bx, int by)
{
    const int x0 = (dx < bx ? dx : bx) - 10, y0 = (dy < by ? dy : by) - 10;
    const int x1 = (dx > bx ? dx : bx) + 10, y1 = (dy > by ? dy : by) + 10;
    g.drawRect(x0 - 2, y0 - 2, (x1 - x0) + 5, (y1 - y0) + 5, PAL_TERM_M);
    g.drawRect(x0 - 1, y0 - 1, (x1 - x0) + 3, (y1 - y0) + 3, PAL_AMBER);
    g.drawRect(x0, y0, (x1 - x0) + 1, (y1 - y0) + 1, PAL_SOLID_A);
    const int keep = s_px; s_px = 6;
    const int tw = 4 * 6;
    int ttx = x0; if (ttx < ZAP_IX0 + 2) ttx = ZAP_IX0 + 2;
    int tty = y0 - 14; if (tty < ZAP_IY0 + 2) tty = ZAP_IY0 + 2;
    g.fillRect(ttx - 2, tty - 2, tw + 28, 12, PAL_BLACK);
    g.drawRect(ttx - 2, tty - 2, tw + 28, 12, PAL_AMBER);
    txt(g, "TIME", ttx, tty, PAL_AMBER, lgfx::textdatum::top_left);
    /* 2 lit / 2 empty pips */
    for (int i = 0; i < 4; ++i) {
        const int px = ttx + tw + 4 + i * 5;
        if (i < 2) g.fillRect(px, tty + 2, 4, 6, PAL_AMBER);
        else { g.drawRect(px, tty + 2, 4, 6, PAL_PLATE); g.fillRect(px + 1, tty + 3, 2, 4, PAL_BG); }
    }
    s_px = keep;
}

static void zap_controls(Gfx g, uint8_t ac)
{
    /* Slim top-right control row ON the scan chrome (mock overlays). */
    const int bw = 19, bh = 17, gap = 2, n = 4;
    const int total = n * (bw + gap) - gap;
    const int bx = 316 - total, by = 22;
    g.fillRect(bx - 3, by - 2, total + 6, bh + 4, PAL_BLACK);
    g.drawRect(bx - 3, by - 2, total + 6, bh + 4, ac);
    for (int i = 0; i < n; ++i) {
        const int x = bx + i * (bw + gap), y = by;
        const bool sel = (i == 3);
        g.fillRect(x, y, bw, bh, PAL_BLACK);
        g.drawRect(x, y, bw, bh, sel ? (uint8_t)PAL_HOLD : ac);
        if (sel) {
            g.drawRect(x - 1, y - 1, bw + 2, bh + 2, PAL_TERM_M);
            g.drawRect(x, y, bw, bh, ac);
            g.fillRect(x + 2, y + bh - 2, bw - 4, 1, ac); /* cyan underline */
        }
        if (i == 0) { g.drawRect(x + 4, y + 3, 10, 8, ac); g.fillRect(x + 6, y + 5, 6, 2, ac); }
        else if (i == 1) { g.drawCircle(x + 9, y + 7, 4, ac); g.fillRect(x + 12, y + 10, 4, 1, ac); }
        else if (i == 2) {
            g.fillRect(x + 4, y + 4, 10, 1, ac); g.fillRect(x + 4, y + 8, 10, 1, ac);
            g.fillRect(x + 4, y + 12, 10, 1, ac);
        } else {
            g.fillRect(x + 4, y + 6, 3, 3, PAL_HOLD);
            g.fillRect(x + 8, y + 4, 3, 3, PAL_IVORY);
            g.fillRect(x + 12, y + 7, 3, 3, ac);
        }
    }
}

static void zap_title_overlay(Gfx g)
{
    const int kt = s_px; s_px = 6;
    char dtitle[32];
    defend_title(dtitle, sizeof(dtitle), 12);
    const int n1 = (int)strlen(dtitle);
    const int px = 6, py = 22;
    const int pw = n1 * 6 + 6, ph = 15;
    plate(g, px, py, pw, ph, PAL_DEEP, PAL_CYAN);
    txt(g, dtitle, px + 3, py + 3, PAL_CYAN, lgfx::textdatum::top_left);
    g.fillRect(px + 3, py + ph - 4, n1 * 6 - 1, 1, PAL_WIRE_C);
    s_px = kt;
}

static void zap_cmd_button(Gfx g)
{
    /* Larger prompt-corner Gibson = command-prompt BUTTON (CMD). PROMPT_BTN=(268,188,314,232) */
    const int x0 = 268, y0 = 188, x1 = 314, y1 = 232;
    g.fillRect(x0 - 2, y0 - 2, (x1 - x0) + 4, (y1 - y0) + 4, PAL_BLACK);
    g.drawRect(x0 - 2, y0 - 2, (x1 - x0) + 4, (y1 - y0) + 4, PAL_TERM_M);
    g.drawRect(x0 - 1, y0 - 1, (x1 - x0) + 2, (y1 - y0) + 2, PAL_CYAN);
    g.drawRect(x0, y0, x1 - x0, y1 - y0, PAL_TERM_M);
    g.drawRect(x0 + 1, y0 + 1, (x1 - x0) - 2, (y1 - y0) - 2, PAL_WHITE);
    g.fillRect(x0 + 3, y0 + 3, (x1 - x0) - 6, (y1 - y0) - 6, PAL_BG);
    g.drawRect(x0 + 3, y0 + 3, (x1 - x0) - 6, (y1 - y0) - 6, PAL_CYAN);
    /* corner pips */
    g.fillRect(x0 + 5, y0 + 5, 5, 5, PAL_CYAN);
    g.fillRect(x1 - 10, y0 + 5, 5, 5, PAL_TERM_M);
    g.fillRect(x0 + 5, y1 - 10, 5, 5, PAL_TERM_M);
    g.fillRect(x1 - 10, y1 - 10, 5, 5, PAL_CYAN);
    /* Chip centred high so CMD caption sits clear of the CHIP gate-box (G1). */
    chip_block(g, (x0 + x1) / 2, y0 + 16, 22, 12);
    const int keep = s_px; s_px = 6;
    txt(g, "CMD", (x0 + x1) / 2 - 9, y1 - 10, PAL_CYAN, lgfx::textdatum::top_left);
    g.fillRect((x0 + x1) / 2 + 10, y1 - 9, 3, 5, PAL_TERM_M);
    s_px = keep;
}

static void zap_paint_field(Gfx g)
{
    GATE_BOX(g, "BOARD", "defend", ZAP_BX0, ZAP_BY0, ZAP_BX1 - ZAP_BX0, ZAP_BY1 - ZAP_BY0, "-");
    g.fillRect(ZAP_BX0, ZAP_BY0, ZAP_BX1 - ZAP_BX0, ZAP_BY1 - ZAP_BY0, PAL_BLACK);
    zap_pink_rim(g);
    zap_grid_bg(g);
    zap_device_rain(g);
    for (int t = 0; t < ZAP_NTR; ++t) zap_trace(g, t);
    for (int t = 0; t < ZAP_NTR; ++t) zap_support(g, t);
    /* centre 5-pin gold Gibson (square chip) */
    chip_block(g, 160, 82, ZAP_CS, 12);
    GATE_BOX(g, "DGBOT", "gibson", 160 - ZAP_CS / 2, 82 - ZAP_CS / 2, ZAP_CS, ZAP_CS, "-");

    /* riders */
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live) continue;
        int x, y; zap_at(s_zap[i].tr, s_zap[i].dist, &x, &y);
        gl_icon(g, ZAP_ICON[s_zap[i].tr], x - 6, y - 6, false);
        if (s_zap[i].dock) {
            int bx, by; zap_at(s_zap[i].tr, zap_len(s_zap[i].tr), &bx, &by);
            zap_time_urgency(g, x, y, bx + 8, by);
        }
    }
    if (s_zap_fx_tr >= 0 && s_dg_ms < s_zap_fx_until)
        zap_draw_bolt(g, s_zap_fx_tr, s_zap_fx_from, s_zap_fx_to);
    if (s_zap_null_tr >= 0 && s_dg_ms < s_zap_null_until) {
        /* NULL on the impact device position: use fx end */
        int x, y; zap_at(s_zap_null_tr, s_zap_fx_to > 0.f ? s_zap_fx_to : 40.f, &x, &y);
        zap_null_label(g, x, y);
        g.drawRect(x - 10, y - 10, 20, 20, PAL_TERM_M);
        g.drawRect(x - 9, y - 9, 18, 18, PAL_CYAN);
    }
    zap_title_overlay(g);
    zap_controls(g, PAL_CYAN);
    zap_pink_rim(g); /* re-stroke after overlays */
}

static void zap_paint_hud(Gfx g)
{
    /* Prompt panel content for zap: log lines + typed command row; CMD button separate. */
    const int keep = s_px; s_px = 6;
    GATE_BOX(g, "DGHUD", "zap", 8, 152, 250, 74, "-");
    g.fillRect(8, 152, 250, 74, PAL_BLACK);
    txt(g, "HACKERS ZAP", 10, 155, PAL_CYAN, lgfx::textdatum::top_left);
    const char *st = s_dg_state == DGS_WIN ? "WIN" : s_dg_state == DGS_LOST ? "LOST"
                    : s_dg_state == DGS_PAUSE ? "PAUSE" : "LIVE";
    const uint8_t sc = s_dg_state == DGS_WIN ? (uint8_t)PAL_OK : s_dg_state == DGS_LOST ? (uint8_t)PAL_AMBER
                     : s_dg_state == DGS_PAUSE ? PAL_CYAN : (uint8_t)PAL_OK;
    txt(g, st, 200, 155, sc, lgfx::textdatum::top_left);
    const int n = s_zap_log_n < 3 ? s_zap_log_n : 3;
    const int start = s_zap_log_n - n;
    for (int k = 0; k < n; ++k) {
        const char *ln = s_zap_log[start + k];
        uint8_t col = PAL_IVORY;
        if (ln[0] == '[' && ln[2] == '!') col = (ln[7] == 'N') ? (uint8_t)PAL_TERM_M : (uint8_t)PAL_SOLID_A;
        if (ln[0] == '[' && ln[2] == 'X') col = PAL_AMBER;
        txt(g, ln, 10, 166 + k * 10, col, lgfx::textdatum::top_left);
    }
    char b[28];
    snprintf(b, sizeof b, "ZAP %02d/%02d  LIFE %d", s_dg_score, ZAP_WIN, 3 - s_dg_miss);
    txt(g, b, 10, 198, PAL_HEAD, lgfx::textdatum::top_left);
    txt(g, "gibson", 10, 216, PAL_CYAN, lgfx::textdatum::top_left);
    txt(g, ">", 52, 216, COL_INK, lgfx::textdatum::top_left);
    if (s_gp_buf[0]) {
        txt(g, s_gp_buf, 64, 216, COL_WHITE, lgfx::textdatum::top_left);
        const int gl = (int)strlen(s_gp_buf);
        const int ci = (s_gc_caret < 0 || s_gc_caret > gl) ? gl : s_gc_caret;
        g.fillRect(64 + ci * 6, 216, 5, 8, PAL_CYAN);
    } else txt(g, "type target to zap  ` quit", 64, 216, PAL_MUTED, lgfx::textdatum::top_left);
    s_px = keep;
}

static void paint_zap_screen(Gfx g)
{
    /* Full Gibson zap Defend: status already painted by paint_gchat; we own mid + prompt. */
    const uint32_t t = millis();
    if (dg_on() && s_dg == DG_ZAP) zap_paint_field(g);
    else {
        /* Idle demo: run a short deterministic still of ZAP */
        const int keep = s_dg, keep_b = s_dg_bot, keep_sc = s_dg_score, keep_mi = s_dg_miss;
        const int keep_st = s_dg_state;
        ZapDev keep_z[ZAP_MAX];
        memcpy(keep_z, s_zap, sizeof s_zap);
        const int keep_null = s_zap_null_tr; const uint32_t keep_nu = s_zap_null_until;
        const int keep_fx = s_zap_fx_tr; const uint32_t keep_fu = s_zap_fx_until;
        const float keep_ff = s_zap_fx_from, keep_ft = s_zap_fx_to;
        s_dg = DG_ZAP; s_dg_bot = 12; s_dg_t = 1000u;
        zap_reset();
        s_dg_score = 0; s_dg_miss = 0; s_dg_state = DGS_PLAY;
        /* Place a mid-zap camera + a docked flock like the mock. */
        s_zap[0].live = 1; s_zap[0].tr = 1; s_zap[0].dist = 52.f; s_zap[0].dock = 0;
        s_zap[1].live = 1; s_zap[1].tr = 4; s_zap[1].dist = zap_len(4) - 2.f; s_zap[1].dock = 1;
        s_zap[2].live = 1; s_zap[2].tr = 0; s_zap[2].dist = 62.f; s_zap[2].dock = 0;
        s_zap[3].live = 1; s_zap[3].tr = 2; s_zap[3].dist = 50.f; s_zap[3].dock = 0;
        s_zap[4].live = 1; s_zap[4].tr = 3; s_zap[4].dist = 55.f; s_zap[4].dock = 0;
        s_zap[5].live = 1; s_zap[5].tr = 5; s_zap[5].dist = 70.f; s_zap[5].dock = 0;
        s_zap_fx_tr = 1; s_zap_fx_from = 4.f; s_zap_fx_to = 46.f;
        s_zap_fx_until = s_dg_ms + 99999u;
        s_zap_null_tr = 1; s_zap_null_until = s_dg_ms + 99999u;
        s_zap_log_n = 0;
        zap_log("[ ** ] board armed. traces live");
        zap_log("[ !! ] zap before dock - camera");
        zap_log("[ !! ] flock docked - TIME");
        zap_paint_field(g);
        /* DEMO tag */
        { const int kp = s_px; s_px = 6;
          plate(g, ZAP_IX0 + 4, ZAP_IY1 - 16, 30, 12, PAL_DEEP, PAL_WIRE_C);
          txt(g, "DEMO", ZAP_IX0 + 7, ZAP_IY1 - 14, PAL_CYAN, lgfx::textdatum::top_left);
          s_px = kp; }
        memcpy(s_zap, keep_z, sizeof s_zap);
        s_zap_null_tr = keep_null; s_zap_null_until = keep_nu;
        s_zap_fx_tr = keep_fx; s_zap_fx_until = keep_fu;
        s_zap_fx_from = keep_ff; s_zap_fx_to = keep_ft;
        s_dg = keep; s_dg_bot = keep_b; s_dg_score = keep_sc; s_dg_miss = keep_mi; s_dg_state = keep_st;
    }
    /* Prompt well */
    g.fillRect(4, 148, 312, 88, PAL_BLACK);
    g.drawRect(4, 148, 312, 88, PAL_CYAN);
    g.drawRect(5, 149, 310, 86, PAL_WIRE_C);
    g.fillRect(6, 150, 250, 1, (uint32_t)(t - s_bus_t0) < 160u ? (uint8_t)PAL_CYAN : (uint8_t)PAL_WIRE_C);
    if (dg_on() && s_dg == DG_ZAP) zap_paint_hud(g);
    else {
        const int keep = s_px; s_px = 6;
        txt(g, "[ ** ] board armed. traces live", 12, 156, PAL_IVORY, lgfx::textdatum::top_left);
        txt(g, "[ !! ] zap before dock - camera", 12, 166, PAL_BFB72, lgfx::textdatum::top_left);
        txt(g, "[ !! ] flock docked - TIME", 12, 176, PAL_SOLID_A, lgfx::textdatum::top_left);
        txt(g, "gibson >", 12, 196, PAL_CYAN, lgfx::textdatum::top_left);
        txt(g, "demo HACKERS ZAP - enter", 12, 216, PAL_MUTED, lgfx::textdatum::top_left);
        s_px = keep;
    }
    zap_cmd_button(g);
}

static void zap_ai(void)
{
    if (s_dg_state != DGS_PLAY) return;
    /* Auto-zap the most urgent: docked first, else farthest along a trace. */
    int best = -1;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live) continue;
        if (s_zap[i].dock) { best = i; break; }
        if (best < 0 || s_zap[i].dist > s_zap[best].dist) best = i;
    }
    if (best < 0) { zap_spawn(); return; }
    zap_try_cmd(ZAP_CMD[s_zap[best].tr]);
}
