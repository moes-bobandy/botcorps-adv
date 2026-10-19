/* ===== v1019v: Gibson Hackers zap Defend (full-width scan playfield) ============================
 * Mock v5 (Mobandy glass PASS 2026-10-19): NO left chip/rail; pink-border scan spans x0-319 under
 * status, above the cyan prompt; centre 5-pin gold Gibson (+15% vs old left-chip ref); branching
 * PCB traces to support pads + silver-pin bot chips; devices travel traces; type a command to zap
 * before dock; docked = TIME window; mid-zap hits the DEVICE with a NULL callout (not Gibson).
 * Prompt-corner Gibson = larger CMD button. Gibson ONLY (s_defend_bot == 12). Other bots keep the
 * old in-board DG_* games. Included from botcorps_defend.inc.
 * v1019w (GIBSONbot glass vs mock v5, 7 gaps): copper CU/CU_HI/CU_DK traces, clean cyan technical
 * grid (sparse rain), larger grey silver-pin device chips + grey device riders, jagged 3-layer cyan
 * bolt, TIME tag with live "02" digits, no on-screen hint (typed GIBSON > row), mock-style log. */
/* v1019x (Mobandy 2:22 PM photo order): the zap plays INSIDE the per-bot scan screen's right-hand cyan
 * grid (bh_grid: BH_X 108, BH_Y 44, 9x5 cells of 19 px) and nowhere else. The full-width v5 layout (pink
 * rim, title plate, control row, CMD button, prompt well) is gone: the screen around the grid is the
 * restored pre-mod scan shell (paint_sbox, 5608764). Trace geometry = v1019w mock v5 scaled x0.56 / y0.80
 * about the grid centre; everything is clipped to the grid rect.
 * v1019x QA (Mobandy 2:44 / 2:47 PM): NO chip of any kind inside the grid (centre Gibson chip, device
 * chips and ic/res/cap pads removed; the left-panel chip stays). Minimum fit only, no new mechanics: the
 * traces start at the grid centre and end at the left/right docks (rows -30 / 0 / +30), riders are the
 * v1019w device sprites at ~0.6 scale (12x10), TIME / NULL tags use the board's 3x5 etch font, and every
 * tag is placed where it overlaps no sprite. All zap pixels sit inside the grid inner rect with a 2 px
 * margin (ZAP_MX0..ZAP_MY1), so the clip below never cuts anything. */
enum {
    ZAP_BX0 = 108, ZAP_BY0 = 44, ZAP_BX1 = 280, ZAP_BY1 = 140,  /* grid rect incl. its right tone line */
    ZAP_IX0 = 109, ZAP_IY0 = 45, ZAP_IX1 = 279, ZAP_IY1 = 139,  /* grid inner */
    ZAP_CX = 194, ZAP_CY = 92,                                  /* grid centre */
    ZAP_MX0 = 111, ZAP_MY0 = 47, ZAP_MX1 = 277, ZAP_MY1 = 137,  /* content box (exclusive): inner - 2 px */
    ZAP_NTR = 6, ZAP_MAX = 6, ZAP_WIN = 12, ZAP_TIME_MS = 2800u, ZAP_NULL_MS = 700u
};
/* Trace ids match mock riders: wifi camera bt drone flock tower */
static const char *const ZAP_CMD[ZAP_NTR] = { "wifi", "camera", "bt", "drone", "flock", "tower" };
static const char *const ZAP_ALIAS[ZAP_NTR] = { "ap", "cam", "bluetooth", "uav", "flockbot", "hack" };
static const int8_t ZAP_BOT[ZAP_NTR] = { 0, 8, 1, 5, 4, 3 };          /* face colour bots */
static const int8_t ZAP_ICON[ZAP_NTR] = { GL_AP, GL_CAMERA, GL_SPEAKER, GL_DRONE, GL_CAMERA, GL_DEVBOARD };
/* Polyline waypoints relative to the grid centre (ZAP_CX,ZAP_CY); ends near support pads. */
static const int16_t ZAP_PT[ZAP_NTR][5][2] = {   /* v1019x: centre -> dock, rows -30 / 0 / +30 */
    { { -7, 0 }, { -7, -22 }, { -30, -22 }, { -30, -30 }, { -66, -30 } },        /* wifi UL */
    { { -14, 0 }, { -30, 0 }, { -30, 4 }, { -50, 4 }, { -66, 0 } },              /* camera L */
    { { 7, 0 }, { 7, 22 }, { 30, 22 }, { 30, 30 }, { 66, 30 } },                 /* bt BR */
    { { 7, 0 }, { 7, -22 }, { 30, -22 }, { 30, -30 }, { 66, -30 } },             /* drone UR */
    { { -7, 0 }, { -7, 22 }, { -30, 22 }, { -30, 30 }, { -66, 30 } },            /* flock BL */
    { { 14, 0 }, { 30, 0 }, { 30, -4 }, { 50, -4 }, { 66, 0 } },                 /* tower R */
};
static const char *const ZAP_NAME[ZAP_NTR] = { "WIFI", "CAMERA", "BT", "DRONE", "FLOCK", "TOWER" };
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
static uint8_t s_zap_warn[ZAP_MAX];   /* v1019w: ZAP BEFORE DOCK logged once per rider */

static bool dg_zap_full(void)
{
    /* v1019x: now "Gibson Defend = scan shell + zap in the grid" (name kept for the call sites). */
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
    const int cx = ZAP_CX, cy = ZAP_CY;
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
    for (int i = 0; i < ZAP_MAX; ++i) { s_zap[i].live = 0; s_zap_dock_seen[i] = 0; s_zap_warn[i] = 0; }
    s_zap_null_tr = -1; s_zap_null_until = 0;
    s_zap_fx_tr = -1; s_zap_fx_until = 0;
    s_zap_log_n = 0;
    zap_log("[ ** ] BOARD ARMED. TRACES LIVE");   /* v1019w: no hint line (spec: no instruction text) */
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
    /* v1019x fit: no respawn on the trace whose NULL ghost is still showing (it would sit on it) */
    if (s_zap_null_tr >= 0 && s_dg_ms < s_zap_null_until) used[s_zap_null_tr] = 1;
    for (int t = 0; t < ZAP_NTR; ++t) if (!used[t]) free_tr[free_n++] = t;
    if (!free_n) return;
    const int tr = free_tr[dg_rng() % (uint32_t)free_n];
    for (int i = 0; i < ZAP_MAX; ++i) if (!s_zap[i].live) {
        s_zap[i].live = 1; s_zap[i].dock = 0; s_zap[i].tr = (int8_t)tr; s_zap_warn[i] = 0;
        /* v1019x: spawn clear of the other traces' riders at the centre (vertical-start traces 12+) */
        s_zap[i].dist = (tr == 1 || tr == 5 ? 6.f : 12.f) + (float)(dg_rng() % 6u);
        return;
    }
}

static void zap_do_hit(int di)
{
    const int tr = s_zap[di].tr;
    s_zap_fx_tr = (int8_t)tr;
    s_zap_fx_from = 4.f;
    s_zap_fx_to = s_zap[di].dist > 11.f ? s_zap[di].dist - 6.f : 5.f;   /* mock: bolt ends 6 px short of the device */
    s_zap_fx_until = s_dg_ms + ZAP_NULL_MS;
    s_zap_null_tr = tr;
    s_zap_null_until = s_dg_ms + ZAP_NULL_MS;
    s_zap[di].live = 0;
    ++s_dg_score;
    char b[36];
    snprintf(b, sizeof b, "[ !! ] NULL - %s %02d/%02d", ZAP_NAME[tr], s_dg_score, ZAP_WIN);
    zap_log(b);
    if (s_dg_score >= ZAP_WIN) { s_dg_state = DGS_WIN; zap_log("[ OK ] BOARD CLEAN"); }
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
        snprintf(b, sizeof b, "[ -- ] NO %s ON TRACES", ZAP_NAME[tr]);
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
            if (!s_zap_warn[i] && s_zap[i].dist >= 0.6f * L) {
                s_zap_warn[i] = 1;
                char b[36];
                snprintf(b, sizeof b, "[ !! ] ZAP BEFORE DOCK - %s", ZAP_NAME[s_zap[i].tr]);
                zap_log(b);
            }
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
                snprintf(b, sizeof b, "[ !! ] %s DOCKED - TIME", ZAP_NAME[s_zap[i].tr]);
                zap_log(b);
            } else if (s_dg_ms - s_zap_dock_t0[i] >= ZAP_TIME_MS) {
                s_zap[i].live = 0;
                s_zap_dock_seen[i] = 0;
                ++s_dg_miss;
                char b[36];
                snprintf(b, sizeof b, "[ XX ] BREACH - LIFE %d", 3 - s_dg_miss > 0 ? 3 - s_dg_miss : 0);
                zap_log(b);
                if (s_dg_miss >= 3) { s_dg_state = DGS_LOST; zap_log("[ XX ] BOARD LOST"); }
            }
        } else s_zap_dock_seen[i] = 0;
    }
    if (s_dg_ms >= s_zap_null_until) s_zap_null_tr = -1;
    if (s_dg_ms >= s_zap_fx_until) s_zap_fx_tr = -1;
}

/* ---- paint helpers (palette indices only; host + device) ---- */
/* v1019w palette for the mock v5 copper / bolt / TIME look. Named slots where one is close; the rest
 * are free RGB332 (pack332) slots, same pattern as PAL_GOLD_PIN. Mock RGB in brackets. */
static const uint8_t ZC_CU_DK  = 100;              /* exact 0x60E2 (99,29,16)   [CU_DK 70,38,0]     */
static const uint8_t ZC_CU     = 136;              /* 332 (146,73,0)            [CU 150,90,10]      */
static const uint8_t ZC_CU_HI  = PAL_R_FC43;       /* (248,136,24)              [CU_HI 231,146,49]  */
static const uint8_t ZC_VIA_HI = 127;              /* 332 (109,255,255)         [trace dot 100,230,255] */
static const uint8_t ZC_ZEDGE  = 50;               /* 332 (36,146,170)          [ZAP_EDGE 40,150,200] */
static const uint8_t ZC_ZAP    = 191;              /* 332 (182,255,255)         [ZAP 180,255,255]   */
static const uint8_t ZC_SILVER = 150;              /* exact 0xA534 (165,166,165)[SILVER 170,180,190] */
static const uint8_t ZC_GRID_H = 33;               /* exact 0x2087 (32,16,56)   [grid major H 40,20,40] */
static const uint8_t ZC_VIA_MD = 101;              /* 332 (109,36,85)           [VIA_MAG/2]         */
static const uint8_t ZC_CAP    = 74;               /* 332 (73,73,170)           [cap 60,80,140]     */
static const uint8_t ZC_CAP_E  = 151;              /* 332 (146,182,255)         [cap edge 140,170,220] */
static const uint8_t ZC_TFLASH = PAL_R_FA69;       /* (248,76,72)               [TIME_FLASH 255,80,80] */
static const uint8_t ZC_TCOL   = PAL_R_F689;       /* (240,208,72)              [TIME_COL 255,220,64] */

/* thick poly stroke (PIL line width w) via w x w stamps along the segment */
static void zap_thick(Gfx g, int x0, int y0, int x1, int y1, int w, uint8_t c)
{
    if (w <= 1) { g.drawLine(x0, y0, x1, y1, c); return; }
    const int dx = x1 - x0, dy = y1 - y0;
    const int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    const int n = (adx > ady ? adx : ady) > 0 ? (adx > ady ? adx : ady) : 1;
    const int o = w / 2;
    for (int s = 0; s <= n; ++s) {
        const int x = x0 + (dx * s + (dx >= 0 ? n / 2 : -n / 2)) / n;
        const int y = y0 + (dy * s + (dy >= 0 ? n / 2 : -n / 2)) / n;
        g.fillRect(x - o, y - o, w, w, c);
    }
}

/* position + unit-ish direction along a trace (octagon lengths, no libm) */
static void zap_atd(int tr, float dist, float *ox, float *oy, float *ux, float *uy)
{
    const int cx = ZAP_CX, cy = ZAP_CY;
    float left = dist;
    for (int i = 0; i < 4; ++i) {
        const float L = zap_seglen(tr, i);
        if (L <= 0.f && i < 3) continue;
        if (left <= L || i == 3) {
            const float u = L > 0.f ? (left / L > 1.f ? 1.f : left / L) : 0.f;
            const float dx = (float)(ZAP_PT[tr][i + 1][0] - ZAP_PT[tr][i][0]);
            const float dy = (float)(ZAP_PT[tr][i + 1][1] - ZAP_PT[tr][i][1]);
            *ox = (float)(cx + ZAP_PT[tr][i][0]) + dx * u;
            *oy = (float)(cy + ZAP_PT[tr][i][1]) + dy * u;
            *ux = L > 0.f ? dx / L : 1.f; *uy = L > 0.f ? dy / L : 0.f;
            return;
        }
        left -= L;
    }
}

/* Gap 1: mock draw_trace — copper CU_DK(5) / CU(3) / CU_HI(1), CU_DK vias at bends, end pad, cyan dots. */
static void zap_trace(Gfx g, int tr)
{
    const int cx = ZAP_CX, cy = ZAP_CY;
    const uint8_t accent = bot_color(ZAP_BOT[tr]);
    static const int W[3] = { 5, 3, 1 };
    const uint8_t C[3] = { ZC_CU_DK, ZC_CU, ZC_CU_HI };
    for (int k = 0; k < 3; ++k)
        for (int i = 0; i < 4; ++i)
            zap_thick(g, cx + ZAP_PT[tr][i][0], cy + ZAP_PT[tr][i][1],
                      cx + ZAP_PT[tr][i + 1][0], cy + ZAP_PT[tr][i + 1][1], W[k], C[k]);
    for (int i = 1; i < 4; ++i) {
        const int x = cx + ZAP_PT[tr][i][0], y = cy + ZAP_PT[tr][i][1];
        g.fillRect(x - 2, y - 2, 5, 5, ZC_CU_DK);
        g.fillRect(x, y, 1, 1, accent);
    }
    const int ex = cx + ZAP_PT[tr][4][0], ey = cy + ZAP_PT[tr][4][1];
    g.fillRect(ex - 3, ey - 3, 7, 7, PAL_PLATE);
    g.drawRect(ex - 3, ey - 3, 7, 7, ZC_CU);
    g.fillRect(ex - 1, ey - 1, 3, 3, accent);
    const float L = zap_len(tr);
    for (float t = 8.f; t < L - 4.f; t += 10.f) {
        float x, y, ux, uy; zap_atd(tr, t, &x, &y, &ux, &uy);
        g.fillRect((int)(x + 0.5f), (int)(y + 0.5f), 1, 1, ZC_VIA_HI);
    }
}

/* v1019w device riders at ~0.6 scale (v1019x fit): 12x10 body + 1 px black halo, grey body with the
 * trace's bot colour as the accent. mode 0 travel, 1 zapped (white rim), 2 docked (TIME rim).
 * Half extents incl. the halo: hw 7 (cx-7..cx+6), hh 6 (cy-6..cy+5). */
enum { ZR_HW = 7, ZR_HH = 6 };
static void zap_rider(Gfx g, int tr, int cx, int cy, int mode, int *hw, int *hh)
{
    const uint8_t body = PAL_ICON_C3, edge = mode == 1 ? (uint8_t)PAL_WHITE : mode == 2 ? ZC_TCOL : (uint8_t)PAL_INK;
    const uint8_t sh = PAL_STEEL, ac = bot_color(ZAP_BOT[tr]);
    const int x = cx - 6, y = cy - 5;
    g.fillRect(x - 1, y - 1, 14, 12, PAL_BLACK);
    switch (tr) {
    case 1: case 4:             /* camera / flock cam: side mount + body + lens */
        g.fillRect(x, y + 3, 2, 4, sh);
        g.fillRect(x + 2, y + 1, 10, 8, body);
        g.drawRect(x + 2, y + 1, 10, 8, edge);
        g.fillCircle(x + 7, cy, 2, PAL_BLACK);
        g.drawCircle(x + 7, cy, 2, sh);
        g.fillRect(x + 7, cy, 1, 1, ac);
        g.fillRect(x + 3, y + 2, 1, 1, PAL_WHITE);
        break;
    case 3:                     /* drone: hull + 4 rotors */
        g.drawLine(x + 2, y + 2, x + 9, y + 7, sh);
        g.drawLine(x + 9, y + 2, x + 2, y + 7, sh);
        g.fillRect(x, y, 4, 3, PAL_SOFT); g.drawRect(x, y, 4, 3, edge);
        g.fillRect(x + 8, y, 4, 3, PAL_SOFT); g.drawRect(x + 8, y, 4, 3, edge);
        g.fillRect(x, y + 7, 4, 3, PAL_SOFT); g.drawRect(x, y + 7, 4, 3, edge);
        g.fillRect(x + 8, y + 7, 4, 3, PAL_SOFT); g.drawRect(x + 8, y + 7, 4, 3, edge);
        g.fillRect(cx - 3, cy - 2, 6, 5, body);
        g.drawRect(cx - 3, cy - 2, 6, 5, edge);
        g.fillRect(cx - 1, cy, 2, 1, ac);
        break;
    case 0:                     /* wifi AP: accent arcs + box */
        g.fillRect(x + 2, y, 8, 1, ac);
        g.fillRect(x + 1, y + 1, 1, 1, ac); g.fillRect(x + 10, y + 1, 1, 1, ac);
        g.fillRect(x + 4, y + 2, 4, 1, ac);
        g.fillRect(cx - 1, y + 3, 2, 1, PAL_WHITE);
        g.fillRect(x + 2, y + 5, 8, 5, body);
        g.drawRect(x + 2, y + 5, 8, 5, edge);
        g.fillRect(x + 4, y + 7, 1, 1, ac);
        break;
    case 2:                     /* bt tag: puck + accent rune */
        g.fillRoundRect(x + 2, y, 8, 10, 2, edge);
        g.fillRoundRect(x + 3, y + 1, 6, 8, 2, body);
        g.drawLine(cx, y + 2, cx, y + 7, ac);
        g.drawLine(cx, y + 2, cx + 2, y + 4, ac); g.drawLine(cx + 2, y + 4, cx - 2, y + 6, ac);
        break;
    default:                    /* tower: accent beacon + mast + base */
        g.fillRect(cx - 1, y, 2, 2, PAL_WHITE);
        g.fillRect(cx - 3, y + 2, 6, 2, ac);
        g.fillRect(cx - 2, y + 4, 4, 5, body);
        g.drawRect(cx - 2, y + 4, 4, 5, edge);
        g.fillRect(x + 2, y + 9, 8, 1, sh);
        break;
    }
    *hw = ZR_HW; *hh = ZR_HH;
}

/* Gap 4: mock draw_zap — jagged perpendicular-jitter bolt, ZAP_EDGE 4 / ZAP 2 / core 1 + two forks. */
static void zap_draw_bolt(Gfx g, int tr, float d0, float d1)
{
    int n = (int)((d1 - d0) / 4.f); if (n < 7) n = 7;
    if (n > 40) n = 40;
    int px[41], py[41];
    uint32_t rng = 7u;
    for (int i = 0; i <= n; ++i) {
        const float dd = d0 + (d1 - d0) * (float)i / (float)n;
        float x, y, ux, uy; zap_atd(tr, dd, &x, &y, &ux, &uy);
        if (i > 0 && i < n) {
            rng = rng * 1664525u + 1013904223u;
            const float j = ((float)((rng >> 8) % 201u) - 100.f) / 100.f;   /* uniform(-1,1) */
            const float amp = (((i & 1) ? 3.5f : -3.5f) + j) * 0.6f;   /* v1019x: 0.6 scale */
            x += -uy * amp; y += ux * amp;
        }
        px[i] = (int)(x + (x >= 0.f ? 0.5f : -0.5f)); py[i] = (int)(y + (y >= 0.f ? 0.5f : -0.5f));
    }
    static const int W[3] = { 3, 2, 1 };   /* v1019x: 0.6 scale */
    const uint8_t C[3] = { ZC_ZEDGE, ZC_ZAP, (uint8_t)PAL_WHITE };
    for (int k = 0; k < 3; ++k)
        for (int i = 0; i < n; ++i) zap_thick(g, px[i], py[i], px[i + 1], py[i + 1], W[k], C[k]);
    if (2 < n) g.drawLine(px[2], py[2], px[2] - 2, py[2] - 2, ZC_ZAP);
    if (5 < n) g.drawLine(px[5], py[5], px[5] - 2, py[5] + 2, ZC_ZAP);
}

/* v1019x tags: occupancy list so a tag never sits on a sprite / rim / other tag; all inside ZAP_M* */
struct ZapBox { int16_t x0, y0, x1, y1; };   /* inclusive */
static ZapBox s_zocc[24];
static int s_zocc_n = 0;
static void zocc_add(int x0, int y0, int x1, int y1)
{
    if (s_zocc_n < 24) { s_zocc[s_zocc_n].x0 = (int16_t)x0; s_zocc[s_zocc_n].y0 = (int16_t)y0;
                         s_zocc[s_zocc_n].x1 = (int16_t)x1; s_zocc[s_zocc_n].y1 = (int16_t)y1; ++s_zocc_n; }
}
static bool zocc_free(int x0, int y0, int x1, int y1)
{
    if (x0 < ZAP_MX0 || y0 < ZAP_MY0 || x1 >= ZAP_MX1 || y1 >= ZAP_MY1) return false;
    for (int i = 0; i < s_zocc_n; ++i)
        if (x0 <= s_zocc[i].x1 && x1 >= s_zocc[i].x0 && y0 <= s_zocc[i].y1 && y1 >= s_zocc[i].y0) return false;
    return true;
}
/* first free candidate spot for a w x h tag next to rim r (inside toward the grid centre first) */
static bool zap_tag_spot(const ZapBox &r, int w, int h, int *ox, int *oy)
{
    const int cy = (r.y0 + r.y1) / 2 - h / 2, cx = (r.x0 + r.x1) / 2 - w / 2;
    const bool left = (r.x0 + r.x1) / 2 < ZAP_CX;
    const int in_x = left ? r.x1 + 2 : r.x0 - 2 - w + 1, out_x = left ? r.x0 - 2 - w + 1 : r.x1 + 2;
    const int cand[8][2] = { { in_x, cy }, { cx, r.y0 - 2 - h + 1 }, { cx, r.y1 + 2 }, { out_x, cy },
                             { in_x, r.y0 - 2 - h + 1 }, { in_x, r.y1 + 2 }, { left ? r.x0 : r.x1 - w + 1, r.y0 - 2 - h + 1 },
                             { left ? r.x0 : r.x1 - w + 1, r.y1 + 2 } };
    for (int k = 0; k < 8; ++k)
        if (zocc_free(cand[k][0], cand[k][1], cand[k][0] + w - 1, cand[k][1] + h - 1)) { *ox = cand[k][0]; *oy = cand[k][1]; return true; }
    return false;
}
#ifdef BOTCORPS_HOST_RENDER
/* host proof hooks: every zap element's bbox per paint (kind 0 sprite, 1 rim, 2 tag, 3 bolt) */
int host_zap_dbg_n = 0;
int16_t host_zap_dbg[48][5];
static void zap_dbg(int k, int x0, int y0, int x1, int y1)
{
    if (host_zap_dbg_n < 48) { int16_t *d = host_zap_dbg[host_zap_dbg_n++]; d[0] = (int16_t)k; d[1] = (int16_t)x0; d[2] = (int16_t)y0; d[3] = (int16_t)x1; d[4] = (int16_t)y1; }
}
int host_zap_noclip = 0, host_zap_hide = 0;
#else
static inline void zap_dbg(int, int, int, int, int) {}
#endif
/* rim: 3 strokes around a rider's halo box (NULL: TERM_M / ZAP / TERM_M, TIME: TERM_M / TFLASH / TCOL) */
static ZapBox zap_rim(Gfx g, int x, int y, bool time)   /* TIME dock rim */
{
    const int x0 = x - ZR_HW - 3, y0 = y - ZR_HH - 3, w = 2 * ZR_HW + 6, h = 2 * ZR_HH + 6;
    g.drawRect(x0, y0, w, h, PAL_TERM_M);
    g.drawRect(x0 + 1, y0 + 1, w - 2, h - 2, time ? ZC_TFLASH : ZC_ZAP);
    g.drawRect(x0 + 2, y0 + 2, w - 4, h - 4, time ? ZC_TCOL : (uint8_t)PAL_TERM_M);
    const ZapBox b = { (int16_t)x0, (int16_t)y0, (int16_t)(x0 + w - 1), (int16_t)(y0 + h - 1) };
    zap_dbg(1, b.x0, b.y0, b.x1, b.y1);
    return b;
}
/* NULL tag: etch-font NULL on a black plate, pink double rim (v1019w look at board-silk size) */
enum { ZN_W = 23, ZN_H = 13 };
static void zap_null_tag(Gfx g, int x, int y)
{
    g.drawRect(x, y, ZN_W, ZN_H, PAL_TERM_M);
    g.drawRect(x + 1, y + 1, ZN_W - 2, ZN_H - 2, PAL_BFB72);
    g.fillRect(x + 2, y + 2, ZN_W - 4, ZN_H - 4, PAL_BLACK);
    tiny_text(g, "NULL", x + 4, y + 4, PAL_TERM_M);
    zap_dbg(2, x, y, x + ZN_W - 1, y + ZN_H - 1);
}
/* TIME tag: TIME, 2 pips, live seconds (v1019w content, etch font) */
enum { ZT_W = 41, ZT_H = 13 };
static void zap_time_tag(Gfx g, int x, int y, int secs)
{
    g.drawRect(x, y, ZT_W, ZT_H, ZC_TFLASH);
    g.drawRect(x + 1, y + 1, ZT_W - 2, ZT_H - 2, ZC_TCOL);
    g.fillRect(x + 2, y + 2, ZT_W - 4, ZT_H - 4, PAL_BLACK);
    tiny_text(g, "TIME", x + 4, y + 4, ZC_TFLASH);
    if (secs < 0) secs = 0;
    if (secs > 99) secs = 99;
    const int lit = secs >= 2 ? 2 : secs;
    for (int i = 0; i < 2; ++i) {
        const int px = x + 21 + i * 4;
        if (i < lit) { g.fillRect(px, y + 4, 3, 5, ZC_TFLASH); g.fillRect(px, y + 4, 3, 1, ZC_TCOL); }
        else g.drawRect(px, y + 4, 3, 5, ZC_CU_DK);
    }
    char sb[4];
    snprintf(sb, sizeof sb, "%02d", secs);
    tiny_text(g, sb, x + 30, y + 4, ZC_TCOL);
    zap_dbg(2, x, y, x + ZT_W - 1, y + ZT_H - 1);
}

static uint8_t zap_log_col(const char *ln)
{
    /* Gap 7: mock log colours — armed white, ZAP BEFORE DOCK red, DOCKED - TIME yellow. */
    if (ln[0] != '[') return PAL_INK;
    if (ln[2] == '!') {
        if (strstr(ln, "DOCKED")) return ZC_TCOL;
        if (strstr(ln, "NULL")) return PAL_TERM_M;
        return PAL_BFB72;
    }
    if (ln[2] == 'X') return PAL_BFB72;
    if (ln[2] == 'O') return PAL_OK;
    if (ln[2] == '-') return PAL_MUTED;
    return PAL_INK;
}


/* v1019x: clip helper. Intersects the current clip (a patch_world / band patch may already clip) with a
 * world rect and restores it after, so nested draws never widen a patch's clip. Device: the Gfx clip IS
 * the raw sprite/panel clip. Host: the Gfx clip lives in gfx_host.cpp (host_gfx_clip_get/set). */
struct ZapClip { int32_t x, y, w, h; bool on; };
#ifdef BOTCORPS_HOST_RENDER
bool host_gfx_clip_get(int *x, int *y, int *w, int *h);
void host_gfx_clip_set(bool on, int x, int y, int w, int h);
#endif
static ZapClip zap_clip_get(Gfx g)
{
    ZapClip c = { 0, 0, 320, 240, false };
#ifdef BOTCORPS_HOST_RENDER
    (void)g;
    int x, y, w, h;
    c.on = host_gfx_clip_get(&x, &y, &w, &h);
    c.x = x; c.y = y; c.w = w; c.h = h;
#else
    if (g.raw()) { g.raw()->getClipRect(&c.x, &c.y, &c.w, &c.h); c.on = true; }
#endif
    return c;
}
static void zap_clip_set(Gfx g, const ZapClip &c)
{
#ifdef BOTCORPS_HOST_RENDER
    (void)g;
    host_gfx_clip_set(c.on, (int)c.x, (int)c.y, (int)c.w, (int)c.h);
#else
    if (g.raw()) g.raw()->setClipRect(c.x, c.y, c.w, c.h);
#endif
}
static ZapClip zap_clip_push(Gfx g, int x, int y, int w, int h)
{
    const ZapClip old = zap_clip_get(g);
    int x0 = x - g.ox, y0 = y - g.oy, x1 = x0 + w, y1 = y0 + h;
    if (old.on) {
        if (x0 < old.x) x0 = old.x;
        if (y0 < old.y) y0 = old.y;
        if (x1 > old.x + old.w) x1 = old.x + old.w;
        if (y1 > old.y + old.h) y1 = old.y + old.h;
    }
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    const ZapClip n = { x0, y0, x1 - x0, y1 - y0, true };
    zap_clip_set(g, n);
    return old;
}
static void zap_clip_pop(Gfx g, const ZapClip &c) { zap_clip_set(g, c); }

/* Gibson Defend screen = the restored scan shell (paint_sbox) while Gibson is on Defend. */
static bool zap_sb_on(void)
{
    return s_screen == SCR_GCHAT && dg_zap_full();
}

/* v1019x: the zap INSIDE the shell's cyan grid. Called from paint_sbox in place of bh_grid + bot_hunt;
 * paint_sbox has already filled the display glass (8). Shell grid lines first, then the v1019w zap
 * content (copper traces, device chips, centre Gibson, riders, bolt, NULL, TIME), all clipped to the
 * grid rect. */
static void zap_paint_grid_live(Gfx g)
{
    /* gate: the Defend board is the shell's display (grid + strip, = the MINI screen box) so the shell's
     * own MINI / PROMPT boxes sit inside it (G4); the zap itself stays inside the grid rect below. */
    GATE_BOX(g, "BOARD", "defend", 106, 42, 176, 148, "-");
    bh_grid(g);
#ifdef BOTCORPS_HOST_RENDER
    host_zap_dbg_n = 0;
    if (host_zap_hide) return;
    ZapClip cl = { 0, 0, 0, 0, false };
    if (!host_zap_noclip) cl = zap_clip_push(g, ZAP_IX0, ZAP_IY0, ZAP_IX1 - ZAP_IX0, ZAP_IY1 - ZAP_IY0);
#else
    const ZapClip cl = zap_clip_push(g, ZAP_IX0, ZAP_IY0, ZAP_IX1 - ZAP_IX0, ZAP_IY1 - ZAP_IY0);
#endif
    for (int t = 0; t < ZAP_NTR; ++t) zap_trace(g, t);
    s_zocc_n = 0;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live || s_zap[i].dock) continue;
        int x, y, hw, hh; zap_at(s_zap[i].tr, s_zap[i].dist, &x, &y);
        zap_rider(g, s_zap[i].tr, x, y, 0, &hw, &hh);
        GATE_BOX(g, "DGOBJ", "device", x - hw, y - hh, 2 * hw, 2 * hh, "-");
        zocc_add(x - hw, y - hh, x + hw - 1, y + hh - 1);
        zap_dbg(0, x - hw, y - hh, x + hw - 1, y + hh - 1);
    }
    if (s_zap_fx_tr >= 0 && s_dg_ms < s_zap_fx_until) {
        const float to = s_zap_fx_to > s_zap_fx_from + 1.f ? s_zap_fx_to : s_zap_fx_from + 1.f;
        zap_draw_bolt(g, s_zap_fx_tr, s_zap_fx_from, to);
    }
    bool has_null = false;
    ZapBox nr = { 0, 0, 0, 0 };
    if (s_zap_null_tr >= 0 && s_dg_ms < s_zap_null_until) {
        int x, y, hw, hh; zap_at(s_zap_null_tr, s_zap_fx_to > 0.f ? s_zap_fx_to + 6.f : 40.f, &x, &y);
        zap_rider(g, s_zap_null_tr, x, y, 1, &hw, &hh);
        GATE_BOX(g, "DGOBJ", "null", x - hw, y - hh, 2 * hw, 2 * hh, "-");
        zap_dbg(0, x - hw, y - hh, x + hw - 1, y + hh - 1);
        /* NULL rim = one ZAP stroke on the halo (inside the sprite box: the vertical traces sit 2 px apart) */
        g.drawRect(x - hw, y - hh, 2 * hw, 2 * hh, ZC_ZAP);
        nr.x0 = (int16_t)(x - hw); nr.y0 = (int16_t)(y - hh); nr.x1 = (int16_t)(x + hw - 1); nr.y1 = (int16_t)(y + hh - 1);
        zap_dbg(1, nr.x0, nr.y0, nr.x1, nr.y1);
        zocc_add(nr.x0, nr.y0, nr.x1, nr.y1);
        has_null = true;
    }
    int nd = 0, dx[ZAP_MAX], dsec[ZAP_MAX];
    ZapBox dr[ZAP_MAX];
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live || !s_zap[i].dock) continue;
        int x, y, hw, hh; zap_at(s_zap[i].tr, s_zap[i].dist, &x, &y);
        zap_rider(g, s_zap[i].tr, x, y, 2, &hw, &hh);
        GATE_BOX(g, "DGOBJ", "device", x - hw, y - hh, 2 * hw, 2 * hh, "-");
        zap_dbg(0, x - hw, y - hh, x + hw - 1, y + hh - 1);
        dr[nd] = zap_rim(g, x, y, true);
        zocc_add(dr[nd].x0, dr[nd].y0, dr[nd].x1, dr[nd].y1);
        int secs = 2;
        if (s_zap_dock_seen[i]) {
            const uint32_t el = s_dg_ms - s_zap_dock_t0[i];
            secs = el >= ZAP_TIME_MS ? 0 : (int)((ZAP_TIME_MS - el + 999u) / 1000u);
        }
        dx[nd] = i; dsec[nd] = secs; ++nd;
    }
    (void)dx;
    for (int k = 0; k < nd; ++k) {
        int tx, ty;
        if (zap_tag_spot(dr[k], ZT_W, ZT_H, &tx, &ty)) { zap_time_tag(g, tx, ty, dsec[k]); zocc_add(tx, ty, tx + ZT_W - 1, ty + ZT_H - 1); }
    }
    if (has_null) {
        int tx, ty;
        if (zap_tag_spot(nr, ZN_W, ZN_H, &tx, &ty)) { zap_null_tag(g, tx, ty); zocc_add(tx, ty, tx + ZN_W - 1, ty + ZN_H - 1); }
    }
#ifdef BOTCORPS_HOST_RENDER
    if (!host_zap_noclip) zap_clip_pop(g, cl);
#else
    zap_clip_pop(g, cl);
#endif
}

/* the idle Defend (no round running) shows a fixed still of the zap, as v1019w's idle demo did */
static void zap_demo_seed(void)
{
    zap_reset();
    s_dg_score = 0; s_dg_miss = 0; s_dg_state = DGS_PLAY;
    s_zap[0].live = 1; s_zap[0].tr = 1; s_zap[0].dist = 30.f; s_zap[0].dock = 0;
    s_zap[1].live = 1; s_zap[1].tr = 4; s_zap[1].dist = zap_len(4) - 2.f; s_zap[1].dock = 1;
    s_zap[2].live = 1; s_zap[2].tr = 0; s_zap[2].dist = 40.f; s_zap[2].dock = 0;
    s_zap[3].live = 1; s_zap[3].tr = 2; s_zap[3].dist = 34.f; s_zap[3].dock = 0;
    s_zap[4].live = 1; s_zap[4].tr = 3; s_zap[4].dist = 36.f; s_zap[4].dock = 0;
    s_zap[5].live = 1; s_zap[5].tr = 5; s_zap[5].dist = 44.f; s_zap[5].dock = 0;
    s_zap_fx_tr = 1; s_zap_fx_from = 2.f; s_zap_fx_to = 24.f;
    s_zap_fx_until = s_dg_ms + 99999u;
    s_zap_null_tr = 1; s_zap_null_until = s_dg_ms + 99999u;
}
static void zap_paint_grid(Gfx g)
{
    if (dg_on() && s_dg == DG_ZAP) { zap_paint_grid_live(g); return; }
    const int keep = s_dg, keep_b = s_dg_bot, keep_sc = s_dg_score, keep_mi = s_dg_miss, keep_st = s_dg_state;
    ZapDev keep_z[ZAP_MAX];
    memcpy(keep_z, s_zap, sizeof s_zap);
    const int keep_null = s_zap_null_tr; const uint32_t keep_nu = s_zap_null_until;
    const int keep_fx = s_zap_fx_tr; const uint32_t keep_fu = s_zap_fx_until;
    const float keep_ff = s_zap_fx_from, keep_ft = s_zap_fx_to;
    const int keep_ln = s_zap_log_n;
    char keep_log[4][36];
    memcpy(keep_log, s_zap_log, sizeof s_zap_log);
    uint8_t keep_ds[ZAP_MAX];
    memcpy(keep_ds, s_zap_dock_seen, sizeof keep_ds);
    s_dg = DG_ZAP; s_dg_bot = 12;
    zap_demo_seed();
    zap_paint_grid_live(g);
    memcpy(s_zap, keep_z, sizeof s_zap);
    memcpy(s_zap_log, keep_log, sizeof s_zap_log);
    memcpy(s_zap_dock_seen, keep_ds, sizeof keep_ds);
    s_zap_log_n = keep_ln;
    s_zap_null_tr = keep_null; s_zap_null_until = keep_nu;
    s_zap_fx_tr = keep_fx; s_zap_fx_until = keep_fu;
    s_zap_fx_from = keep_ff; s_zap_fx_to = keep_ft;
    s_dg = keep; s_dg_bot = keep_b; s_dg_score = keep_sc; s_dg_miss = keep_mi; s_dg_state = keep_st;
}

/* v1019x: the shell's terminal strip under the grid, same positions / font / inks as paint_sbox's core
 * strip. Before any zap event it reads exactly like the shell ("gibson core linked" / "gibson bus
 * idle"); once events land the two newest zap log lines take those two rows (tag in its log ink, body in
 * the shell ink). The prompt row is the shell prompt with the typed zap command. No hint text. */
static void zap_strip(Gfx g, int dx, int th, int ty, int dw)
{
    const uint8_t ac = accent();
    const bool play = dg_on() && s_dg == DG_ZAP;
    char body[2][40];
    char tag[2][8];
    uint8_t tc[2] = { PAL_OK, PAL_OK };
    if (!play || s_zap_log_n <= 1) {
        snprintf(tag[0], 8, "[ ok ] "); snprintf(body[0], 40, "gibson core linked");
        snprintf(tag[1], 8, "[ ok ] "); snprintf(body[1], 40, "gibson bus idle");
    } else {
        const int n0 = s_zap_log_n - 2;
        for (int k = 0; k < 2; ++k) {
            const char *ln = s_zap_log[n0 + k];
            const bool tagged = ln[0] == '[' && strlen(ln) >= 7;
            if (tagged) {
                tag[k][0] = '['; tag[k][1] = ' ';
                tag[k][2] = (char)((ln[2] >= 'A' && ln[2] <= 'Z') ? ln[2] + 32 : ln[2]);
                tag[k][3] = (char)((ln[3] >= 'A' && ln[3] <= 'Z') ? ln[3] + 32 : ln[3]);
                tag[k][4] = ' '; tag[k][5] = ']'; tag[k][6] = ' '; tag[k][7] = 0;
                tc[k] = (ln[2] == '*' || ln[2] == 'O') ? (uint8_t)PAL_OK : zap_log_col(ln);
            } else { tag[k][0] = 0; }
            snprintf(body[k], 40, "%s", tagged ? ln + 7 : ln);
            for (char *p = body[k]; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p + 32);
        }
    }
    for (int k = 0; k < 2; ++k) {
        const int y = th + k * 11;
        int x = sb_tt(g, tag[k], dx + 3, y, tc[k]);
        sb_tt(g, body[k], x, y + 1, sb_tone(3));
        sb_tt(g, body[k], x, y, sb_ink());
    }
    int x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
    x = sb_tt(g, s_gp_buf, x, ty, COL_INK);
    if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
    sb_rect(g, dx + dw - 22, ty - 2, 19, 12, 8);
    sb_frame(g, dx + dw - 22, ty - 2, 19, 12, sb_tone(4));
    for (int k = 0; k < 3; ++k) sb_rect(g, dx + dw - 18, ty + 1 + k * 3, 11, 1, COL_MUTED);
}

/* v1019x (Mobandy 2:26 PM): the background rain runs ONLY in the dark PCB rectangle behind the left
 * chip, under the chip and its silk. The rect is the clean PCB between the board's gold edge / mount
 * holes / headers and the display bezel side bar. */
enum { ZR_X = 20, ZR_Y = 44, ZR_W = 76, ZR_H = 148 };
/* v1019x QA 1 (Mobandy 2:44 PM): no rain on the chip or its name / RADIO CORE silk: cells touching these
 * world rects {x, y, w, h} are skipped (measured from the no-rain render, +2 px). */
static const int16_t ZR_EX[2][4] = { { 31, 64, 59, 60 }, { 32, 124, 58, 27 } };   /* chip+pins, silk block */
static void zap_rain_rect(Gfx g)
{
    GATE_BOX(g, "RAIN", "layer", ZR_X, ZR_Y, ZR_W, ZR_H, "-");
#ifdef BOTCORPS_HOST_RENDER
    if (host_gate_norain) return;
#endif
    if (!g.raw() || g.ox != 0) return;
    /* window = the rect intersected with the caller's clip (a patch may clip tighter), world coords */
    const ZapClip cl = zap_clip_push(g, ZR_X, ZR_Y, ZR_W, ZR_H);
    const ZapClip n = zap_clip_get(g);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 1;
#endif
    botcorps_rain_draw_win(*g.raw(), g.oy, g.oy + g.raw()->height(), (int)n.x + g.ox, (int)n.y + g.oy, (int)n.w, (int)n.h, ZR_EX, 2);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 0;
#endif
    zap_clip_pop(g, cl);
}

/* Patches: paint_sbox with the patch window (sb_vis), exactly like sb_patch on the scan screen, so a
 * patch is the full paint's own pixels. */
static void patch_world(void (*fn)(Gfx), int x, int y, int w, int h);
static void zap_sb_part(Gfx g) { paint_sbox(g, true); }
static void zap_sb_patch(int x, int y, int w, int h)
{
    s_pc_x = x; s_pc_y = y; s_pc_w = w; s_pc_h = h;
    patch_world(zap_sb_part, x, y, w, h);
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
}
static uint32_t zap_mix(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t zap_field_sig(void)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live) { h = zap_mix(h, 0xFFu); continue; }
        h = zap_mix(h, (uint32_t)s_zap[i].tr * 4096u + (uint32_t)((int)s_zap[i].dist >> 1) * 2u + (uint32_t)s_zap[i].dock);
        if (s_zap[i].dock && s_zap_dock_seen[i]) h = zap_mix(h, (s_dg_ms - s_zap_dock_t0[i]) / 1000u);
    }
    const bool fx = s_zap_fx_tr >= 0 && s_dg_ms < s_zap_fx_until;
    const bool nl = s_zap_null_tr >= 0 && s_dg_ms < s_zap_null_until;
    h = zap_mix(h, fx ? (uint32_t)s_zap_fx_tr * 256u + (uint32_t)(int)s_zap_fx_to : 0xFFFFu);
    h = zap_mix(h, nl ? (uint32_t)s_zap_null_tr : 0xFFu);
    h = zap_mix(h, (uint32_t)(dg_on() && s_dg == DG_ZAP));
    return h;
}
static uint32_t zap_hud_sig(void)
{
    uint32_t h = 2166136261u;
    h = zap_mix(h, (uint32_t)s_zap_log_n);
    for (int k = 0; k < 4 && k < s_zap_log_n; ++k)
        for (const char *c = s_zap_log[k]; *c; ++c) h = zap_mix(h, (uint8_t)*c);
    for (const char *c = s_gp_buf; *c; ++c) h = zap_mix(h, (uint8_t)*c);
    h = zap_mix(h, (uint32_t)(dg_on() && s_dg == DG_ZAP));
    h = zap_mix(h, (millis() / 530u) % 2u);   /* shell caret blink */
    return h;
}
/* rain: advance the shared rain state one 140 ms step WITHOUT its own panel paint (set_draw off, so
 * the direct path cannot paint the open screen), then patch only the rain rect. */
static void zap_rain_tick(void)
{
    if (!s_bg_motion) return;
    const bool was = botcorps_rain_draw_enabled();
    botcorps_rain_set_draw(false);
    const bool st = botcorps_rain_step();
    botcorps_rain_set_draw(was);
    if (st) zap_sb_patch(ZR_X, ZR_Y, ZR_W, ZR_H);
}
static void sb_bus_keys(uint32_t t, uint32_t *top, uint32_t *rule, uint32_t *sr);
static void zap_tick_patch(void)
{
    static uint32_t fs = 0, hs = 0;
    /* v1019x QA 4: the shell's live bus (top-bar bot squares + top rule + under-grid sweep) and the left
     * module's trace pulse / LED, on the same keys as the scan screen's sb_tick */
    static uint32_t k_top = ~0u, k_rule = ~0u, k_sr = ~0u, k_mod = ~0u, k_led = ~0u;
    const uint32_t t = millis();
    uint32_t top, rule, sr;
    sb_bus_keys(t, &top, &rule, &sr);
    if (top != k_top) { if (k_top != ~0u) zap_sb_patch(0, 0, 320, 24); k_top = top; }
    if (rule != k_rule) { if (k_rule != ~0u) zap_sb_patch(0, 24, 320, 1); k_rule = rule; }
    if (sr != k_sr) { if (k_sr != ~0u) zap_sb_patch(106, 144, 176, 1); k_sr = sr; }
    const uint32_t md = t / 120u, ld = (t / 500u) % 2u;
    if (md != k_mod) { if (k_mod != ~0u) zap_sb_patch(88, 97, 9, 38); k_mod = md; }
    if (ld != k_led) { if (k_led != ~0u) zap_sb_patch(93, 54, 5, 3); k_led = ld; }
    const uint32_t f = zap_field_sig();
    if (f != fs) { fs = f; zap_sb_patch(ZAP_BX0, ZAP_BY0, ZAP_BX1 - ZAP_BX0, ZAP_BY1 - ZAP_BY0); }
    const uint32_t h = zap_hud_sig();
    if (h != hs) { hs = h; zap_sb_patch(106, 152, 176, 38); }
    zap_rain_tick();
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
