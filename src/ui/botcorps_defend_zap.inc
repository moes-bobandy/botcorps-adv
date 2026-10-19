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
static const int16_t ZAP_PT[ZAP_NTR][5][2] = {   /* v1019w: exact mock v5 traces dict */
    { { -10, -17 }, { -10, -30 }, { -116, -30 }, { -116, -45 }, { -138, -45 } }, /* wifi UL */
    { { -17, -8 }, { -50, -8 }, { -50, -22 }, { -106, -22 }, { -134, -10 } },    /* camera L */
    { { 8, 17 }, { 8, 32 }, { 101, 32 }, { 101, 44 }, { 136, 44 } },             /* bt BR */
    { { 17, -6 }, { 42, -6 }, { 42, -43 }, { 128, -43 }, { 140, -49 } },         /* drone UR */
    { { -6, 17 }, { -6, 28 }, { -86, 28 }, { -86, 46 }, { -128, 46 } },          /* flock BL */
    { { 17, 6 }, { 58, 6 }, { 58, 24 }, { 138, 24 }, { 138, 24 } },              /* tower R */
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
    for (int t = 0; t < ZAP_NTR; ++t) if (!used[t]) free_tr[free_n++] = t;
    if (!free_n) return;
    const int tr = free_tr[dg_rng() % (uint32_t)free_n];
    for (int i = 0; i < ZAP_MAX; ++i) if (!s_zap[i].live) {
        s_zap[i].live = 1; s_zap[i].dock = 0; s_zap[i].tr = (int8_t)tr; s_zap_warn[i] = 0;
        s_zap[i].dist = 6.f + (float)(dg_rng() % 12u);
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
    const int cx = 160, cy = 82;
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

/* Gap 2: mock technical_grid_bg — clean dark-cyan circuit grid + sparse cyan/magenta vias. */
static void zap_grid_bg(Gfx g)
{
    const int x0 = ZAP_IX0, y0 = ZAP_IY0, x1 = ZAP_IX1, y1 = ZAP_IY1;
    g.fillRect(x0, y0, x1 - x0, y1 - y0, PAL_BLACK);
    for (int x = x0 + 6; x < x1; x += 8) g.fillRect(x, y0, 1, y1 - y0, PAL_BAR);     /* (18,28,36) */
    for (int y = y0 + 6; y < y1; y += 8) g.fillRect(x0, y, x1 - x0, 1, PAL_BAR);
    for (int x = x0 + 24; x < x1; x += 32) g.fillRect(x, y0, 1, y1 - y0, PAL_LINE);  /* (28,50,60) */
    for (int y = y0 + 24; y < y1; y += 32) g.fillRect(x0, y, x1 - x0, 1, ZC_GRID_H); /* (40,20,40) */
    uint32_t rng = 3u;
    for (int n = 0; n < 40; ++n) {
        rng = rng * 1664525u + 1013904223u;
        const int x = x0 + 4 + (int)((rng >> 8) % (uint32_t)(x1 - x0 - 8));
        rng = rng * 1664525u + 1013904223u;
        const int y = y0 + 4 + (int)((rng >> 8) % (uint32_t)(y1 - y0 - 8));
        rng = rng * 1664525u + 1013904223u;
        const bool cyan = ((rng >> 9) % 100u) < 55u;
        g.fillRect(x, y, 1, 1, cyan ? (uint8_t)PAL_CYAN : (uint8_t)PAL_TERM_M);
        if (((rng >> 17) % 100u) < 35u) g.drawRect(x - 1, y - 1, 3, 3, cyan ? (uint8_t)PAL_WIRE_C : ZC_VIA_MD);
    }
}

/* Gap 2: rain kept but sparse + faded (mock paint_device_rain alpha 0.30 reads as a faint hint). */
static void zap_device_rain(Gfx g)
{
    static const int8_t icons[4] = { GL_CAMERA, GL_AP, GL_DRONE, GL_SPEAKER };
    uint32_t rng = 1019u;
    for (int cx = ZAP_IX0 + 14; cx < ZAP_IX1 - 8; cx += 46) {
        rng = rng * 1664525u + 1013904223u;
        const int y = ZAP_IY0 + 4 + (int)((rng >> 8) % (uint32_t)(ZAP_IY1 - ZAP_IY0 - 20));
        if (((rng >> 20) % 100u) < 55u) continue;
        gl_icon(g, icons[(rng >> 4) % 4u], cx - 6, y, true);
    }
}

/* Gap 1: mock draw_trace — copper CU_DK(5) / CU(3) / CU_HI(1), CU_DK vias at bends, end pad, cyan dots. */
static void zap_trace(Gfx g, int tr)
{
    const int cx = 160, cy = 82;
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

/* Gap 3: mock draw_mini_bot_chip(S=14) — silver castellated bezel reads as a grey device chip. */
static void zap_dev_chip(Gfx g, int cx, int cy, int bot)
{
    const int S = 14, x = cx - S / 2, y = cy - S / 2;
    g.fillRect(x - 3, y - 3, S + 6, S + 6, PAL_BLACK);            /* halo=1 */
    GATE_BOX(g, "TCHIP", "bot", x, y, S, S, "-");
    g.drawRect(x - 1, y - 1, S + 2, S + 2, PAL_STEEL);
    g.fillRect(x, y, S, S, PAL_BLACK);
    g.drawRect(x, y, S, S, PAL_FIELD);
    for (int k = 0; k < 5; ++k) {                                   /* 2 px pins, 2 px pitch = silver bar */
        const int i = 2 + 2 * k;
        g.fillRect(x - 2, y + i, 4, 2, ZC_SILVER);
        g.fillRect(x + S - 2, y + i, 4, 2, ZC_SILVER);
        g.fillRect(x + i, y - 2, 2, 4, ZC_SILVER);
        g.fillRect(x + i, y + S - 2, 2, 4, ZC_SILVER);
    }
    g.fillRect(x + 2, y + 2, S - 4, S - 4, PAL_BLACK);
    const int gs = 8, fx = x + (S - gs) / 2, fy = y + (S - gs) / 2;
    const uint8_t fc = bot_color(bot);
    GATE_BOX(g, "SPR", "bot_face", fx, fy, gs, gs, "-");
    g.fillRect(fx, fy, gs, gs, fc);
    g.fillRect(fx, fy + gs - 1, gs, 1, PAL_BOARD_D);                 /* chin */
    g.fillRect(fx + 1, fy + 1, 1, 1, PAL_WHITE);                     /* top glint */
    g.fillRect(fx + 1, fy + 2, 2, 2, PAL_BG);                        /* eyes */
    g.fillRect(fx + 5, fy + 2, 2, 2, PAL_BG);
}

/* mock draw_support_pad: ic / res / cap beside each trace end */
static void zap_support_pad(Gfx g, int kind, int cx, int cy)
{
    if (kind == 1) {            /* res 10x6 */
        const int x = cx - 5, y = cy - 3;
        g.fillRect(x, y + 2, 2, 2, ZC_CU_HI); g.fillRect(x + 8, y + 2, 2, 2, ZC_CU_HI);
        g.fillRect(x + 2, y + 1, 6, 4, PAL_GOLD_PIN); g.drawRect(x + 2, y + 1, 6, 4, PAL_AMBER);
        g.fillRect(x + 4, y + 2, 1, 1, ZC_CU_DK); g.fillRect(x + 5, y + 3, 1, 1, ZC_CU_DK);
    } else if (kind == 2) {     /* cap 8x8 */
        const int x = cx - 4, y = cy - 4;
        g.fillRect(x + 2, y + 1, 4, 6, ZC_CAP); g.drawRect(x + 2, y + 1, 4, 6, ZC_CAP_E);
        g.fillRect(x + 1, y + 3, 6, 2, ZC_CU_HI);
    } else {                    /* ic 12x10 */
        const int x = cx - 6, y = cy - 5;
        g.fillRect(x + 2, y + 1, 8, 8, PAL_BAR); g.drawRect(x + 2, y + 1, 8, 8, PAL_STEEL);
        for (int i = 0; i < 3; ++i) {
            g.fillRect(x, y + 2 + i * 2, 3, 2, ZC_SILVER);
            g.fillRect(x + 9, y + 2 + i * 2, 3, 2, ZC_SILVER);
        }
        g.fillRect(x + 3, y + 2, 1, 1, PAL_BFB72);
    }
}

static void zap_chip_pos(int tr, int *bx, int *by)
{
    const int ex = 160 + ZAP_PT[tr][4][0], ey = 82 + ZAP_PT[tr][4][1];
    int x = ex + 4, y = ey - 1;
    if (x < ZAP_IX0 + 14) x = ZAP_IX0 + 14;
    if (x > ZAP_IX1 - 14) x = ZAP_IX1 - 14;
    if (y < ZAP_IY0 + 14) y = ZAP_IY0 + 14;
    if (y > ZAP_IY1 - 14) y = ZAP_IY1 - 14;
    *bx = x; *by = y;
}

static void zap_support(Gfx g, int tr)
{
    static const int8_t KIND[ZAP_NTR] = { 0, 1, 2, 0, 1, 2 };   /* wifi ic, cam res, bt cap, ... */
    const int ex = 160 + ZAP_PT[tr][4][0], ey = 82 + ZAP_PT[tr][4][1];
    zap_support_pad(g, KIND[tr], ex - 6, ey);
    int bx, by; zap_chip_pos(tr, &bx, &by);
    zap_dev_chip(g, bx, by, ZAP_BOT[tr]);
}

/* Gap 3: larger grey device riders (mock make_device_sprites x1.25 + light outline), grey bodies with
 * the trace's bot colour as the accent. mode 0 travel, 1 zapped (white rim), 2 docked (TIME rim).
 * Returns the half extents for TIME / NULL placement. */
static void zap_rider(Gfx g, int tr, int cx, int cy, int mode, int *hw, int *hh)
{
    const uint8_t body = PAL_ICON_C3, edge = mode == 1 ? (uint8_t)PAL_WHITE : mode == 2 ? ZC_TCOL : (uint8_t)PAL_INK;
    const uint8_t sh = PAL_STEEL, ac = bot_color(ZAP_BOT[tr]);
    int w = 18, h = 14;
    switch (tr) {
    case 1: case 4: {           /* camera / flock cam: body + side mount + lens */
        w = tr == 1 ? 20 : 16; h = tr == 1 ? 14 : 12;
        const int x = cx - w / 2, y = cy - h / 2;
        g.fillRect(x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
        g.fillRect(x, y + h / 2 - 2, 3, 5, sh);
        g.fillRect(x + 2, y + 1, w - 2, h - 2, body);
        g.drawRect(x + 2, y + 1, w - 2, h - 2, edge);
        const int lx = x + 2 + (w - 2) / 2 + 1, r = tr == 1 ? 4 : 3;
        g.fillCircle(lx, cy, r, PAL_BLACK);
        g.drawCircle(lx, cy, r, sh);
        g.fillRect(lx - 1, cy - 1, 3, 3, ac);
        g.fillRect(x + 4, y + 3, 1, 1, PAL_WHITE);
        break; }
    case 3: {                   /* drone: grey hull + 4 rotors */
        w = 20; h = 16;
        const int x = cx - w / 2, y = cy - h / 2;
        g.fillRect(x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
        g.drawLine(x + 3, y + 3, x + w - 4, y + h - 4, sh);
        g.drawLine(x + w - 4, y + 3, x + 3, y + h - 4, sh);
        static const int8_t RX[4] = { 0, 14, 0, 14 }, RY[4] = { 1, 1, 11, 11 };
        for (int k = 0; k < 4; ++k) {
            g.fillRect(x + RX[k], y + RY[k], 6, 4, PAL_SOFT);
            g.drawRect(x + RX[k], y + RY[k], 6, 4, edge);
        }
        g.fillRect(cx - 5, cy - 3, 10, 7, body);
        g.drawRect(cx - 5, cy - 3, 10, 7, edge);
        g.fillRect(cx - 2, cy - 1, 4, 3, ac);
        break; }
    case 0: {                   /* wifi AP: grey box + accent arcs */
        w = 18; h = 15;
        const int x = cx - w / 2, y = cy - h / 2;
        g.fillRect(x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
        g.fillRect(x + 2, y + 2, w - 4, 1, ac);
        g.fillRect(x + 1, y + 3, 1, 2, ac); g.fillRect(x + w - 2, y + 3, 1, 2, ac);
        g.fillRect(x + 5, y + 5, w - 10, 1, ac);
        g.fillRect(x + 4, y + 6, 1, 1, ac); g.fillRect(x + w - 5, y + 6, 1, 1, ac);
        g.fillRect(x + 4, y + 9, w - 8, 5, body);
        g.drawRect(x + 4, y + 9, w - 8, 5, edge);
        g.fillRect(cx - 1, y + 7, 2, 2, PAL_WHITE);
        g.fillRect(x + 7, y + 11, 1, 1, ac);
        break; }
    case 2: {                   /* bt tag: grey puck + accent rune */
        w = 14; h = 16;
        const int x = cx - w / 2, y = cy - h / 2;
        g.fillRect(x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
        g.fillRoundRect(x, y, w, h, 3, edge);
        g.fillRoundRect(x + 1, y + 1, w - 2, h - 2, 2, body);
        g.drawLine(cx, y + 3, cx, y + h - 4, ac);
        g.drawLine(cx, y + 3, cx + 3, y + 6, ac); g.drawLine(cx + 3, y + 6, cx - 3, y + h - 6, ac);
        g.drawLine(cx, y + h - 4, cx + 3, y + h - 7, ac); g.drawLine(cx + 3, y + h - 7, cx - 3, y + 5, ac);
        break; }
    default: {                  /* tower: grey mast + accent beacon */
        w = 14; h = 18;
        const int x = cx - w / 2, y = cy - h / 2;
        g.fillRect(x - 1, y - 1, w + 2, h + 2, PAL_BLACK);
        g.fillTriangle(cx, y, cx + 5, y + 6, cx - 5, y + 6, ac);
        g.fillRect(cx - 1, y + 2, 2, 2, PAL_WHITE);
        g.fillRect(cx - 2, y + 7, 5, 9, body);
        g.drawRect(cx - 2, y + 7, 5, 9, edge);
        g.fillRect(x + 1, y + h - 2, w - 2, 2, sh);
        break; }
    }
    *hw = w / 2; *hh = h / 2;
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
            const float amp = ((i & 1) ? 3.5f : -3.5f) + j;
            x += -uy * amp; y += ux * amp;
        }
        px[i] = (int)(x + (x >= 0.f ? 0.5f : -0.5f)); py[i] = (int)(y + (y >= 0.f ? 0.5f : -0.5f));
    }
    static const int W[3] = { 4, 2, 1 };
    const uint8_t C[3] = { ZC_ZEDGE, ZC_ZAP, (uint8_t)PAL_WHITE };
    for (int k = 0; k < 3; ++k)
        for (int i = 0; i < n; ++i) zap_thick(g, px[i], py[i], px[i + 1], py[i + 1], W[k], C[k]);
    if (2 < n) g.drawLine(px[2], py[2], px[2] - 3, py[2] - 4, ZC_ZAP);
    if (5 < n) g.drawLine(px[5], py[5], px[5] - 3, py[5] + 4, ZC_ZAP);
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

/* Gap 5: mock draw_time_urgency — triple rim + corner ticks; TIME, 2 lit pips, live seconds "02". */
static void zap_time_urgency(Gfx g, int tx, int ty, int shw, int shh, int bx, int by, int secs)
{
    const int bh = 9;                                   /* chip 14 + pins: half extent */
    const int x0 = (tx - shw < bx - bh ? tx - shw : bx - bh) - 5;
    const int y0 = (ty - shh < by - bh ? ty - shh : by - bh) - 5;
    const int x1 = (tx + shw > bx + bh ? tx + shw : bx + bh) + 5;
    const int y1 = (ty + shh > by + bh ? ty + shh : by + bh) + 5;
    g.drawRect(x0 - 2, y0 - 2, x1 - x0 + 5, y1 - y0 + 5, PAL_TERM_M);
    g.drawRect(x0 - 1, y0 - 1, x1 - x0 + 3, y1 - y0 + 3, ZC_TFLASH);
    g.drawRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, ZC_TCOL);
    g.drawRect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, ZC_TFLASH);
    g.fillRect(x0, y0, 6, 2, ZC_TFLASH); g.fillRect(x0, y0, 2, 6, ZC_TFLASH);
    g.fillRect(x1 - 5, y0, 6, 2, ZC_TFLASH); g.fillRect(x1 - 1, y0, 2, 6, ZC_TFLASH);
    g.fillRect(x0, y1 - 1, 6, 2, ZC_TFLASH); g.fillRect(x0, y1 - 5, 2, 6, ZC_TFLASH);
    g.fillRect(x1 - 5, y1 - 1, 6, 2, ZC_TFLASH); g.fillRect(x1 - 1, y1 - 5, 2, 6, ZC_TFLASH);
    const int keep = s_px; s_px = 6;
    const int tw = 4 * 6, pw = 5, pg = 2, pn = 4;
    const int plate_w = tw + 6 + pn * pw + (pn - 1) * pg + 6, plate_h = 12;
    int ttx = x0; if (ttx > ZAP_IX1 - plate_w - 2) ttx = ZAP_IX1 - plate_w - 2;
    if (ttx < ZAP_IX0 + 2) ttx = ZAP_IX0 + 2;
    int tty = y0 - plate_h - 3; if (tty < ZAP_IY0 + 2) tty = ZAP_IY0 + 2;
    g.drawRect(ttx - 2, tty - 2, plate_w + 4, plate_h + 4, ZC_TFLASH);
    g.drawRect(ttx - 1, tty - 1, plate_w + 2, plate_h + 2, ZC_TCOL);
    g.fillRect(ttx, tty, plate_w, plate_h, PAL_BLACK);
    txt(g, "TIME", ttx + 3, tty + 3, ZC_TFLASH, lgfx::textdatum::top_left);
    if (secs < 0) secs = 0;
    if (secs > 99) secs = 99;
    const int lit = secs >= 2 ? 2 : secs;
    const int px0 = ttx + 3 + tw + 5;
    for (int i = 0; i < 2; ++i) {
        const int px = px0 + i * (pw + pg);
        if (i < lit) { g.fillRect(px, tty + 3, pw, 7, ZC_TFLASH); g.fillRect(px, tty + 3, pw, 1, ZC_TCOL); }
        else { g.fillRect(px, tty + 3, pw, 7, PAL_BG); g.drawRect(px, tty + 3, pw, 7, ZC_CU_DK); }
    }
    char sb[4];
    snprintf(sb, sizeof sb, "%02d", secs);
    txt(g, sb, ttx + plate_w - 12 - 2, tty + 3, ZC_TCOL, lgfx::textdatum::top_left);
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

    /* riders (gap 3: grey device sprites; gap 4: jagged bolt; gap 5: TIME digits) */
    int null_x = -1, null_y = -1, null_hh = 7;
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live || s_zap[i].dock) continue;
        int x, y, hw, hh; zap_at(s_zap[i].tr, s_zap[i].dist, &x, &y);
        zap_rider(g, s_zap[i].tr, x, y, 0, &hw, &hh);
    }
    if (s_zap_fx_tr >= 0 && s_dg_ms < s_zap_fx_until) {
        const float to = s_zap_fx_to > s_zap_fx_from + 1.f ? s_zap_fx_to : s_zap_fx_from + 1.f;
        zap_draw_bolt(g, s_zap_fx_tr, s_zap_fx_from, to);
    }
    if (s_zap_null_tr >= 0 && s_dg_ms < s_zap_null_until) {
        /* NULL on the impact device position: the zapped rider drawn lit on top of the bolt tip */
        int x, y, hw, hh; zap_at(s_zap_null_tr, s_zap_fx_to > 0.f ? s_zap_fx_to + 6.f : 40.f, &x, &y);
        zap_rider(g, s_zap_null_tr, x, y, 1, &hw, &hh);
        g.drawRect(x - hw - 3, y - hh - 3, 2 * hw + 7, 2 * hh + 7, PAL_TERM_M);
        g.drawRect(x - hw - 2, y - hh - 2, 2 * hw + 5, 2 * hh + 5, ZC_ZAP);
        g.drawRect(x - hw - 1, y - hh - 1, 2 * hw + 3, 2 * hh + 3, PAL_TERM_M);
        null_x = x; null_y = y; null_hh = hh;
    }
    for (int i = 0; i < ZAP_MAX; ++i) {
        if (!s_zap[i].live || !s_zap[i].dock) continue;
        int x, y, hw, hh; zap_at(s_zap[i].tr, s_zap[i].dist, &x, &y);
        zap_rider(g, s_zap[i].tr, x, y, 2, &hw, &hh);
        int bx, by; zap_chip_pos(s_zap[i].tr, &bx, &by);
        int secs = 2;
        if (s_zap_dock_seen[i]) {
            const uint32_t el = s_dg_ms - s_zap_dock_t0[i];
            secs = el >= ZAP_TIME_MS ? 0 : (int)((ZAP_TIME_MS - el + 999u) / 1000u);
        }
        zap_time_urgency(g, x, y, hw, hh, bx, by, secs);
    }
    if (null_x >= 0) zap_null_label(g, null_x, null_y - null_hh + 6);
    zap_title_overlay(g);
    zap_controls(g, PAL_CYAN);
    zap_pink_rim(g); /* re-stroke after overlays */
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

static void zap_prompt_row(Gfx g, const char *typed, int caret)
{
    /* Gap 6: typed command only, mock `GIBSON > NUL` + caret. No on-screen instruction/hint text. */
    char row[40];
    int n = snprintf(row, sizeof row, "GIBSON > %s", typed ? typed : "");
    if (n < 0) n = 0;
    if (n > (int)sizeof row - 1) n = (int)sizeof row - 1;
    for (char *p = row; *p; ++p) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
    txt(g, row, 12, 196, PAL_CYAN, lgfx::textdatum::top_left);
    int ci = n;
    if (typed && typed[0] && caret >= 0 && caret < (int)strlen(typed)) ci = 9 + caret;
    g.fillRect(12 + ci * 6, 195, 5, 7, PAL_CYAN);
}

static void zap_paint_hud(Gfx g)
{
    /* Gap 7: mock log look — three log lines + typed row; no HACKERS ZAP header, no ZAP/LIFE line
     * (score and lives ride the NULL / BREACH log lines). CMD button separate. */
    const int keep = s_px; s_px = 6;
    GATE_BOX(g, "DGHUD", "zap", 8, 152, 250, 74, "-");
    g.fillRect(8, 152, 250, 74, PAL_BLACK);
    const int n = s_zap_log_n < 3 ? s_zap_log_n : 3;
    const int start = s_zap_log_n - n;
    for (int k = 0; k < n; ++k) {
        const char *ln = s_zap_log[start + k];
        txt(g, ln, 12, 156 + k * 10, zap_log_col(ln), lgfx::textdatum::top_left);
    }
    if (s_dg_state == DGS_PAUSE) txt(g, "[ -- ] PAUSED", 12, 186, PAL_MUTED, lgfx::textdatum::top_left);
    zap_prompt_row(g, s_gp_buf, s_gc_caret);
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
        zap_log("[ ** ] BOARD ARMED. TRACES LIVE");
        zap_log("[ !! ] ZAP BEFORE DOCK - CAMERA");
        zap_log("[ !! ] FLOCK DOCKED - TIME");
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
        txt(g, "[ ** ] BOARD ARMED. TRACES LIVE", 12, 156, PAL_INK, lgfx::textdatum::top_left);
        txt(g, "[ !! ] ZAP BEFORE DOCK - CAMERA", 12, 166, PAL_BFB72, lgfx::textdatum::top_left);
        txt(g, "[ !! ] FLOCK DOCKED - TIME", 12, 176, ZC_TCOL, lgfx::textdatum::top_left);
        zap_prompt_row(g, "nul", -1);
        s_px = keep;
    }
    zap_cmd_button(g);
}

/* v1019w: live zap on the device. dg_tick's generic path patched the OLD in-board HUD (paint_dg_hud_ext:
 * "type name  enter zap", "tab next  ` quit") and the old LOST/WIN plate over the full-width chrome, and
 * never repainted the riders, so the TIME digits could not count down. Zap now patches its own field
 * (rider / bolt / TIME change) and its own prompt-panel log + typed row (log / typed / state change). */
static void patch_world(void (*fn)(Gfx), int x, int y, int w, int h);
static void zap_field_part(Gfx g) { zap_paint_field(g); }
static void zap_hud_part(Gfx g) { zap_paint_hud(g); }
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
    return h;
}
static uint32_t zap_hud_sig(void)
{
    uint32_t h = 2166136261u;
    h = zap_mix(h, (uint32_t)s_zap_log_n);
    for (int k = 0; k < 4 && k < s_zap_log_n; ++k)
        for (const char *c = s_zap_log[k]; *c; ++c) h = zap_mix(h, (uint8_t)*c);
    for (const char *c = s_gp_buf; *c; ++c) h = zap_mix(h, (uint8_t)*c);
    h = zap_mix(h, (uint32_t)(s_gc_caret + 1));
    h = zap_mix(h, (uint32_t)s_dg_state);
    return h;
}
static void zap_tick_patch(void)
{
    static uint32_t fs = 0, hs = 0;
    const uint32_t f = zap_field_sig();
    if (f != fs) { fs = f; patch_world(zap_field_part, ZAP_BX0, ZAP_BY0, ZAP_BX1 - ZAP_BX0, ZAP_BY1 - ZAP_BY0); }
    const uint32_t h = zap_hud_sig();
    if (h != hs) { hs = h; patch_world(zap_hud_part, 8, 152, 250, 74); }
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
