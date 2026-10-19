/* v1019y BLOCK 1: old Defend mini-games are gone (DG_* bodies, ZAP, NIGHT, SWEEP, Dex).
 * defend / play open the v1019x photo-match scan shell with an idle empty grid.
 * No charge / acquire / deauth-pulse loop — that waits on GIBSON's locked mock.
 * Kept: the shell's grid rect, left-chip rain window, and the live top-bar / sweep tick. */
enum { DG_ZAP = 14 };
/* Host review scratch only. Kept off the device image: it is 492 B of static RAM and the dual cap is frozen. */
#ifdef BOTCORPS_HOST_RENDER
int host_zap_dbg_n = 0;
int host_zap_noclip = 0, host_zap_hide = 0;
int16_t host_zap_dbg[48][5];
#endif

static bool dg_zap_full(void)
{
    /* Every Defend board is the photo-match shell, idle, for whichever bot is set. */
    return s_screen == SCR_GCHAT;
}
static bool zap_sb_on(void) { return s_screen == SCR_GCHAT && dg_zap_full(); }

static void dg_start(int) {}
static void dg_start_own(int) {}
static void dg_start_game(int, int) {}
static void dg_stop(void) { s_dg = -1; }
static void dg_step(uint32_t) {}
static bool dg_key(int, char) { return false; }
static bool dg_repeat_ok(void) { return false; }
static void dg_tick(void) {}
static void dg_snap(void) {}
template <class S>
static void dg_int_tick(S &) {}
static void dg_ai(void) {}
static void dg_note(void) {}
static void dg_lobby(void) {}
static void dg_scores(void) {}
static void dg_scores_bot(int) {}
static void dg_loadout(int) {}
static void dg_rank(void) {}
static bool dg_play_n(int) { return false; }
static bool zap_try_cmd(const char *) { return false; }
static void paint_dg_board(Gfx) {}
static void paint_dg_demo(Gfx, int) {}
static void paint_dg_demo_hint(Gfx, int) {}
static void paint_dg_hud_ext(Gfx) {}
static void paint_dex(Gfx) {}
static void dex_open(void) {}
static void dg_paint_hud(Gfx, int, bool) {}

/* Deauth loop inside the photo-match grid. Fork B: field glow follows career HI.
 * Left panel is not drawn here — sb_board keeps the framed chip. */
enum {
    DL_AW_BLOOD = 1 << 0, DL_AW_DWELL = 1 << 1, DL_AW_FULL = 1 << 2, DL_AW_SWEEP = 1 << 3,
    DL_AW_COMBO = 1 << 4, DL_AW_HI = 1 << 5, DL_AW_TIER = 1 << 6, DL_AW_KINDS = 1 << 7,
    DL_AW_SURV = 1 << 8, DL_AW_ROOT = 1 << 9
};
static const int DL_KINDS[12] = {
    GL_AP, GL_PHONE, GL_LAPTOP, GL_TV, GL_SPEAKER, GL_TRACKER,
    GL_WATCH, GL_CAMERA, GL_CAR, GL_UNKNOWN, GL_DRONE, GL_PRINTER
};
static const char *const DL_AW_NM[10] = {
    "first blood", "dwell master", "full charge", "clean sweep", "combo x4",
    "hi score", "tier up", "all kinds", "survivor", "root access"
};
static const char *const DL_TIER_NM[5] = { "INIT", "ARMED", "HOT", "OVERCLOCK", "ROOT" };
static const int DL_TIER_HI[5] = { 0, 500, 1500, 4000, 10000 };
enum { DL_MAX = 6, DL_CHG = 300, DL_TIME0 = 45000 };
struct DlDev { float p; int kind, hp, acq, live; };
/* Device: the existing tier-0 bot_ui scratch (boot-touched, otherwise unused). Host: a tiny local. */
#ifdef BOTCORPS_HOST_RENDER
alignas(8) static uint8_t dl_scratch[256];
#else
extern uint8_t botcorps_t0_bot_ui[2560];
#define dl_scratch botcorps_t0_bot_ui
#endif
struct DlMem {
    int on, hi, aw, score, combo;
    int best_combo, nulls, docks, dwell_kills, kinds;
    int rx, ry, aim, pop;
    uint32_t t0, hold, bolt, pop_until;
    DlDev d[DL_MAX];
};
static DlMem &dl_mem()
{
    return *reinterpret_cast<DlMem *>(static_cast<void *>(dl_scratch));
}
static_assert(sizeof(DlMem) <= 256, "deauth state fits the bot_ui scratch");
#define s_dl_on (dl_mem().on)
#define s_dl_hi (dl_mem().hi)
#define s_dl_aw (dl_mem().aw)
#define s_dl_score (dl_mem().score)
#define s_dl_combo (dl_mem().combo)
#define s_dl_best_combo (dl_mem().best_combo)
#define s_dl_nulls (dl_mem().nulls)
#define s_dl_docks (dl_mem().docks)
#define s_dl_dwell_kills (dl_mem().dwell_kills)
#define s_dl_kinds (dl_mem().kinds)
#define s_dl_rx (dl_mem().rx)
#define s_dl_ry (dl_mem().ry)
#define s_dl_aim (dl_mem().aim)
#define s_dl_pop (dl_mem().pop)
#define s_dl_t0 (dl_mem().t0)
#define s_dl_hold (dl_mem().hold)
#define s_dl_bolt (dl_mem().bolt)
#define s_dl_pop_until (dl_mem().pop_until)
#define s_dl_d (dl_mem().d)

static int dl_tier_of(int hi)
{
    int t = 0;
    for (int i = 0; i < 5; ++i) if (hi >= DL_TIER_HI[i]) t = i;
    return t;
}
static int dl_lv_of(int hi)
{
    int lv = 1 + hi / 250;
    return lv > 40 ? 40 : lv;
}
static int dl_bot(void)
{
    return (s_defend_bot >= 0 && s_defend_bot <= 12) ? s_defend_bot : 12;
}
static uint8_t dl_accent(void)
{
    const int b = dl_bot();
    return (b >= 0 && b < 12) ? bot_color(b) : (uint8_t)PAL_IVORY;
}
static int dl_pick_kind(int bot, int n)
{
    if (bot >= 12) return DL_KINDS[n % 12];
    return bot_dev_kind(bot);
}
static bool dl_live(void) { return s_dl_on && s_screen == SCR_GCHAT; }
static void dl_save(void)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return;
    prefs.putUInt("dl_hi", (unsigned)s_dl_hi);
    prefs.putUInt("dl_aw", (unsigned)s_dl_aw);
    prefs.end();
}
static void dl_load(void)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return;
    s_dl_hi = (int)prefs.getUInt("dl_hi", (unsigned)s_dl_hi);
    s_dl_aw = (int)prefs.getUInt("dl_aw", (unsigned)s_dl_aw);
    prefs.end();
}
static void dl_award(int bit)
{
    if (s_dl_aw & bit) return;
    s_dl_aw |= bit;
    for (int i = 0; i < 10; ++i) if (bit == (1 << i)) s_dl_pop = i;
    s_dl_pop_until = millis() + 1800u;
    dl_save();
}
static void dl_note_hi(void)
{
    const int was = dl_tier_of(s_dl_hi);
    if (s_dl_score > s_dl_hi) {
        s_dl_hi = s_dl_score;
        dl_award(DL_AW_HI);
        if (dl_tier_of(s_dl_hi) > was) dl_award(DL_AW_TIER);
        if (dl_tier_of(s_dl_hi) >= 4) dl_award(DL_AW_ROOT);
        dl_save();
    }
}
static void dl_place(int i, int kind)
{
    s_dl_d[i].live = 1;
    s_dl_d[i].kind = kind;
    s_dl_d[i].hp = 3;
    s_dl_d[i].acq = 0;
    s_dl_d[i].p = 0.08f + (float)(i % 3) * 0.07f;
}
static void dl_begin(int bot)
{
    dl_load();
    s_defend_bot = (bot < 0 || bot > 12) ? 12 : bot;
    s_dl_on = 1;
    s_dl_score = 0;
    s_dl_combo = 0;
    s_dl_best_combo = 0;
    s_dl_nulls = 0;
    s_dl_docks = 0;
    s_dl_dwell_kills = 0;
    s_dl_kinds = 0;
    s_dl_aim = -1;
    s_dl_hold = 0;
    s_dl_bolt = 0;
    s_dl_pop = -1;
    s_dl_t0 = millis();
    s_dl_rx = 190;
    s_dl_ry = 96;
    for (int i = 0; i < DL_MAX; ++i) dl_place(i, dl_pick_kind(s_defend_bot, i));
}
static void dl_end(void)
{
    if (!s_dl_on) return;
    const uint32_t spent = millis() - s_dl_t0 + (uint32_t)s_dl_docks * 3000u;
    if (spent < (uint32_t)DL_TIME0 && (uint32_t)DL_TIME0 - spent > 15000u) dl_award(DL_AW_SURV);
    if (s_dl_nulls >= 12 && s_dl_docks == 0) dl_award(DL_AW_SWEEP);
    dl_note_hi();
    s_dl_on = 0;
}
static int dl_charge_at(uint32_t t)
{
    if (!s_dl_on) return 0;
    int c = (int)((t - s_dl_t0) / 25u);
    return c > DL_CHG ? DL_CHG : c;
}
static int dl_time_left(uint32_t t)
{
    const uint32_t spent = (t - s_dl_t0) + (uint32_t)s_dl_docks * 3000u;
    if (spent >= (uint32_t)DL_TIME0) return 0;
    return (int)(DL_TIME0 - spent);
}
static void dl_xy(int i, int *x, int *y)
{
    const int cx = 190, cy = 96;
    const int ex[6] = { 118, 262, 190, 190, 130, 250 };
    const int ey[6] = { 96, 96, 60, 132, 64, 128 };
    const float p = s_dl_d[i].p;
    *x = ex[i] + (int)((float)(cx - ex[i]) * p);
    *y = ey[i] + (int)((float)(cy - ey[i]) * p);
}
static int dl_under(void)
{
    int best = -1, bd = 12;
    for (int i = 0; i < DL_MAX; ++i) {
        if (!s_dl_d[i].live) continue;
        int x, y; dl_xy(i, &x, &y);
        const int dx = x - s_dl_rx, dy = y - s_dl_ry;
        const int q = dx < 0 ? -dx : dx, r = dy < 0 ? -dy : dy;
        const int m = q > r ? q : r;
        if (m < bd) { bd = m; best = i; }
    }
    return best;
}
static void dl_kill(int i, int dwell)
{
    if (s_dl_nulls == 0) dl_award(DL_AW_BLOOD);
    s_dl_nulls++;
    s_dl_score += 40 + s_dl_combo * 10;
    if (dwell) { s_dl_dwell_kills++; if (s_dl_dwell_kills >= 5) dl_award(DL_AW_DWELL); }
    if (s_defend_bot >= 12 && s_dl_d[i].kind < 16) s_dl_kinds |= 1 << s_dl_d[i].kind;
    if (s_defend_bot >= 12) {
        int got = 0;
        for (int k = 0; k < 12; ++k) if (s_dl_kinds & (1 << DL_KINDS[k])) got++;
        if (got >= 12) dl_award(DL_AW_KINDS);
    }
    s_dl_combo++;
    if (s_dl_combo > s_dl_best_combo) s_dl_best_combo = s_dl_combo;
    if (s_dl_best_combo >= 4) dl_award(DL_AW_COMBO);
    dl_note_hi();
    dl_place(i, dl_pick_kind(s_defend_bot, s_dl_nulls + i));
    s_dl_bolt = millis();
}
static void dl_steer(int key)
{
    if (key == 1) s_dl_ry -= 6;
    if (key == 2) s_dl_ry += 6;
    if (key == 3) s_dl_rx -= 6;
    if (key == 4) s_dl_rx += 6;
    if (s_dl_rx < 116) s_dl_rx = 116;
    if (s_dl_rx > 268) s_dl_rx = 268;
    if (s_dl_ry < 58) s_dl_ry = 58;
    if (s_dl_ry > 134) s_dl_ry = 134;
}
static void dl_fire(void)
{
    const int ch = dl_charge_at(millis());
    if (ch < DL_CHG / 3) return;
    int dmg = ch >= DL_CHG ? 3 : ch >= (DL_CHG * 2) / 3 ? 2 : 1;
    if (ch >= DL_CHG) dl_award(DL_AW_FULL);
    const int i = dl_under();
    s_dl_bolt = millis();
    if (i < 0) { s_dl_combo = 0; return; }
    if (s_dl_d[i].acq) dmg++;
    s_dl_d[i].hp -= dmg;
    s_dl_score += dmg * 5;
    if (s_dl_d[i].hp <= 0) dl_kill(i, 0);
    else dl_note_hi();
}
static void dl_step(uint32_t t)
{
#ifdef BOTCORPS_HOST_RENDER
    extern int host_band_sim;
    if (host_band_sim) return;
#endif
    if (!dl_live()) return;
    if (dl_time_left(t) <= 0) { dl_end(); return; }
    for (int i = 0; i < DL_MAX; ++i) {
        if (!s_dl_d[i].live) continue;
        s_dl_d[i].p += 0.004f;
        if (s_dl_d[i].p >= 1.f) { s_dl_docks++; s_dl_combo = 0; dl_place(i, dl_pick_kind(s_defend_bot, s_dl_docks + i)); }
    }
    const int u = dl_under();
    if (u < 0) { s_dl_aim = -1; s_dl_hold = 0; }
    else if (s_dl_aim != u) { s_dl_aim = u; s_dl_hold = t; }
    else if ((uint32_t)(t - s_dl_hold) >= 350u) {
        s_dl_d[u].acq = 1;
        if ((t / 400u) != ((t - 30u) / 400u)) {
            s_dl_d[u].hp--;
            if (s_dl_d[u].hp <= 0) dl_kill(u, 1);
        }
    }
}

static void zap_paint_grid(Gfx g)
{
    GATE_BOX(g, "BOARD", "defend", 106, 42, 176, 148, "-");
    if (!dl_live()) { bh_grid(g); return; }
    const uint32_t t = millis();
    const int tier = dl_tier_of(s_dl_hi);
    const int mix = 8 + tier * 16;
    const uint8_t field = botcorps_pal_fit(mix16(botcorps_rgb565(PAL_CODE), 0x0000, mix > 80 ? 80 : mix, 100));
    const uint8_t glow = botcorps_pal_fit(mix16(botcorps_rgb565(PAL_CODE), 0xFFFF, 20 + tier * 12, 100));
    const uint8_t bc = dl_accent();
    sb_rect(g, BH_X, BH_Y, BH_W, BH_H, field);
    for (int c = 0; c <= BH_GC; ++c) sb_rect(g, BH_X + c * BH_C, BH_Y, 1, BH_H, glow);
    for (int r = 0; r <= BH_GR; ++r) sb_rect(g, BH_X, BH_Y + r * BH_C, BH_W, 1, glow);
    for (int i = 0; i < 6 + tier * 8; ++i) {
        const int sx = BH_X + 2 + (int)((t / 40u + (uint32_t)i * 37u) % (uint32_t)(BH_W - 4));
        const int sy = BH_Y + 2 + (int)((t / 55u + (uint32_t)i * 19u) % (uint32_t)(BH_H - 4));
        sb_rect(g, sx, sy, 1, 2, (uint8_t)PAL_CODE);
    }
    const uint8_t copper = botcorps_pal_fit(mix16(0xC300, botcorps_rgb565(bc), 40, 100));
    for (int i = 0; i < DL_MAX; ++i) {
        int x, y; dl_xy(i, &x, &y);
        sb_rect(g, x < 190 ? x : 190, 96, (x > 190 ? x - 190 : 190 - x) + 1, 1, copper);
        sb_rect(g, 190, y < 96 ? y : 96, 1, (y > 96 ? y - 96 : 96 - y) + 1, copper);
        if (!s_dl_d[i].live) continue;
        gl_icon(g, s_dl_d[i].kind, x - 6, y - 6, false);
        if (s_dl_d[i].acq) sb_text(g, "ACQ", x - 8, y - 12, 228, field, false, 1);
    }
    if (g.raw()) botcorps_sprite_draw(*g.raw(), 190 - g.ox, 96 - g.oy, BOTCORPS_SHAPE_ROUND, 2, bc, false);
    sb_frame(g, s_dl_rx - 5, s_dl_ry - 5, 11, 11, bc);
    sb_rect(g, s_dl_rx - 1, s_dl_ry, 3, 1, (uint8_t)PAL_WHITE);
    sb_rect(g, s_dl_rx, s_dl_ry - 1, 1, 3, (uint8_t)PAL_WHITE);
    const int ch = s_dl_on ? dl_charge_at(t) : 0;
    sb_rect(g, 150, BH_Y + 2, 70, 4, (uint8_t)8);
    if (ch) sb_rect(g, 150, BH_Y + 2, 70 * ch / DL_CHG, 4, bc);
    char hud[24];
    snprintf(hud, sizeof hud, "%ds", s_dl_on ? dl_time_left(t) / 1000 : DL_TIME0 / 1000);
    /* Index prompt occupies x108..123, y43..53. "45s" starts at x128 so the boxes do not meet. */
    sb_text(g, hud, 128, BH_Y + 2, (uint8_t)PAL_WHITE, field, false, 1);
    snprintf(hud, sizeof hud, "L%d %d", dl_lv_of(s_dl_hi), s_dl_score);
    sb_text(g, hud, 228, BH_Y + 1, (uint8_t)PAL_WHITE, field, false, 1);
    for (int i = 0; i < DL_MAX; ++i) if (s_dl_d[i].live && s_dl_d[i].acq)
        sb_text(g, "TARGET ACQUIRED", 128, 78, (uint8_t)PAL_WHITE, 228, false, 1);
    if (s_dl_bolt && (uint32_t)(t - s_dl_bolt) < 280u) {
        sb_rect(g, s_dl_rx < 190 ? s_dl_rx : 190, s_dl_ry, (s_dl_rx > 190 ? s_dl_rx - 190 : 190 - s_dl_rx) + 1, 1, (uint8_t)PAL_WHITE);
        sb_frame(g, s_dl_rx - 8, s_dl_ry - 8, 17, 17, bc);
    }
    if (s_dl_pop >= 0 && (int32_t)(s_dl_pop_until - t) > 0) {
        sb_rect(g, 124, 70, 140, 36, (uint8_t)8);
        sb_frame(g, 124, 70, 140, 36, bc);
        sb_text(g, "AWARD UNLOCKED", 132, 74, bc, 8, false, 1);
        gl_icon(g, DL_KINDS[s_dl_pop % 12], 132, 86, false);
        sb_text(g, DL_AW_NM[s_dl_pop], 148, 88, (uint8_t)PAL_WHITE, 8, false, 1);
    }
}

static void zap_strip(Gfx g, int dx, int th, int ty, int dw)
{
    if (!dl_live()) {
        const uint8_t ac = accent();
        const int b = (s_defend_bot >= 0 && s_defend_bot <= 12) ? s_defend_bot : 12;
        const char *who = b >= 12 ? "gibson" : BOTS[b].short_name;
        char body[40];
        int x = sb_tt(g, "[ ok ] ", dx + 3, th, PAL_OK);
        snprintf(body, sizeof body, "%s core linked", who);
        sb_tt(g, body, x, th + 1, sb_tone(3));
        sb_tt(g, body, x, th, sb_ink());
        x = sb_tt(g, "[ ok ] ", dx + 3, th + 11, PAL_OK);
        snprintf(body, sizeof body, "%s bus idle", who);
        sb_tt(g, body, x, th + 12, sb_tone(3));
        sb_tt(g, body, x, th + 11, sb_ink());
        x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
        x = sb_tt(g, s_gp_buf, x, ty, COL_INK);
        if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
        const bool lit = s_def_cmd >= 0;
        sb_rect(g, dx + dw - 22, ty - 2, 19, 12, lit ? sb_tone(3) : (uint8_t)8);
        sb_frame(g, dx + dw - 22, ty - 2, 19, 12, lit ? ac : sb_tone(4));
        for (int k = 0; k < 3; ++k)
            sb_rect(g, dx + dw - 18, ty + 1 + k * 3, 11, 1, lit ? ac : (uint8_t)COL_MUTED);
        return;
    }
    const uint8_t ac = dl_accent();
    char body[40];
    snprintf(body, sizeof body, "HI %05d  LV%02d  T%d", s_dl_hi < 0 ? 0 : s_dl_hi, dl_lv_of(s_dl_hi), dl_tier_of(s_dl_hi));
    sb_tt(g, body, dx + 3, th, (uint8_t)PAL_WHITE);
    sb_tt(g, DL_TIER_NM[dl_tier_of(s_dl_hi)], dx + 3, th + 11, ac);
    int x = sb_tt(g, "user@botcorps:~$ ", dx + 3, ty, ac);
    x = sb_tt(g, s_gp_buf, x, ty, COL_INK);
    if ((millis() / 530u) % 2u == 0u) sb_rect(g, x + 1, ty, 5, 9, ac);
    const bool lit = s_def_cmd >= 0;
    sb_rect(g, dx + dw - 22, ty - 2, 19, 12, lit ? sb_tone(3) : (uint8_t)8);
    sb_frame(g, dx + dw - 22, ty - 2, 19, 12, lit ? ac : sb_tone(4));
    for (int k = 0; k < 3; ++k)
        sb_rect(g, dx + dw - 18, ty + 1 + k * 3, 11, 1, lit ? ac : (uint8_t)COL_MUTED);
}

/* Device-only rain in the dark rect behind both chips. Cells that touch a chip, the
 * device name (left of the right-mounted chip), or the silk are skipped. */
enum { ZR_X = 20, ZR_Y = 40, ZR_W = 76, ZR_H = 152 };
static void zr_exclusions(int16_t ex[4][4])
{
    const int cxm = 61, ccy = 78;
    const int dcx = cxm + DEVICE_CHIP_OFF;
    const int dcy = ccy + 36;
    ex[0][0] = 39; ex[0][1] = 56; ex[0][2] = 46; ex[0][3] = 48;
    ex[1][0] = (int16_t)(dcx - 14); ex[1][1] = (int16_t)(dcy - 14); ex[1][2] = 28; ex[1][3] = 28;
    ex[2][0] = 18; ex[2][1] = (int16_t)(dcy - 6); ex[2][2] = (int16_t)(dcx - 14 - 18); ex[2][3] = 14;
    ex[3][0] = 22; ex[3][1] = (int16_t)(ccy + 48); ex[3][2] = 74; ex[3][3] = 40;
}
#ifdef BOTCORPS_HOST_RENDER
bool host_gfx_clip_get(int *x, int *y, int *w, int *h);
void host_gfx_clip_set(bool on, int x, int y, int w, int h);
#endif
struct ZrClip { int32_t x, y, w, h; bool on; };
static ZrClip zr_clip_get(Gfx g)
{
    ZrClip c = { 0, 0, 320, 240, false };
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
static void zr_clip_set(Gfx g, const ZrClip &c)
{
#ifdef BOTCORPS_HOST_RENDER
    (void)g;
    host_gfx_clip_set(c.on, (int)c.x, (int)c.y, (int)c.w, (int)c.h);
#else
    if (g.raw()) g.raw()->setClipRect(c.x, c.y, c.w, c.h);
#endif
}
static ZrClip zr_clip_push(Gfx g, int x, int y, int w, int h)
{
    const ZrClip old = zr_clip_get(g);
    int x0 = x - g.ox, y0 = y - g.oy, x1 = x0 + w, y1 = y0 + h;
    if (old.on) {
        if (x0 < old.x) x0 = (int)old.x;
        if (y0 < old.y) y0 = (int)old.y;
        if (x1 > old.x + old.w) x1 = (int)(old.x + old.w);
        if (y1 > old.y + old.h) y1 = (int)(old.y + old.h);
    }
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    const ZrClip n = { x0, y0, x1 - x0, y1 - y0, true };
    zr_clip_set(g, n);
    return old;
}
static void zap_rain_rect(Gfx g)
{
    GATE_BOX(g, "RAIN", "layer", ZR_X, ZR_Y, ZR_W, ZR_H, "-");
#ifdef BOTCORPS_HOST_RENDER
    if (host_gate_norain) return;
#endif
    if (!g.raw() || g.ox != 0) return;
    const ZrClip cl = zr_clip_push(g, ZR_X, ZR_Y, ZR_W, ZR_H);
    const ZrClip n = zr_clip_get(g);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 1;
#endif
    int16_t ex[4][4];
    zr_exclusions(ex);
    botcorps_rain_draw_win(*g.raw(), g.oy, g.oy + g.raw()->height(), (int)n.x + g.ox, (int)n.y + g.oy,
                           (int)n.w, (int)n.h, ex, 4);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 0;
#endif
    zr_clip_set(g, cl);
}

static void patch_world(void (*fn)(Gfx), int x, int y, int w, int h);
static void zap_sb_part(Gfx g) { paint_sbox(g, true); }
static void zap_sb_patch(int x, int y, int w, int h)
{
    s_pc_x = x; s_pc_y = y; s_pc_w = w; s_pc_h = h;
    patch_world(zap_sb_part, x, y, w, h);
    s_pc_x = 0; s_pc_y = 0; s_pc_w = 320; s_pc_h = 240;
}
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
    static uint32_t k_top = ~0u, k_rule = ~0u, k_sr = ~0u, k_mod = ~0u, k_led = ~0u, k_caret = ~0u;
    const uint32_t t = millis();
    uint32_t top, rule, sr;
    sb_bus_keys(t, &top, &rule, &sr);
    if (top != k_top) { if (k_top != ~0u) zap_sb_patch(0, 0, 320, 24); k_top = top; }
    if (rule != k_rule) { if (k_rule != ~0u) zap_sb_patch(0, 24, 320, 1); k_rule = rule; }
    if (sr != k_sr) { if (k_sr != ~0u) zap_sb_patch(106, 144, 176, 1); k_sr = sr; }
    const uint32_t md = t / 120u, ld = (t / 500u) % 2u, ct = (t / 530u) % 2u;
    if (md != k_mod) { if (k_mod != ~0u) zap_sb_patch(88, 80, 18, 40); k_mod = md; }
    if (ld != k_led) { if (k_led != ~0u) zap_sb_patch(93, 54, 5, 3); k_led = ld; }
    if (ct != k_caret) { if (k_caret != ~0u) zap_sb_patch(106, 164, 176, 26); k_caret = ct; }
    dl_step(t);
    zap_rain_tick();
}
