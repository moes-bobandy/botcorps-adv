/* v1019y BLOCK 1: old Defend mini-games are gone (DG_* bodies, ZAP, NIGHT, SWEEP, Dex).
 * defend / play open the v1019x photo-match scan shell with an idle empty grid.
 * No charge / acquire / deauth-pulse loop — that waits on GIBSON's locked mock.
 * Kept: the shell's grid rect, left-chip rain window, and the live top-bar / sweep tick. */
enum { DG_ZAP = 14 };
/* Review-only symbols the old zap painter filled. The grid is empty, so the debug list stays empty. */
int host_zap_dbg_n = 0;
int host_zap_noclip = 0, host_zap_hide = 0;
int16_t host_zap_dbg[48][5];

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

/* Idle grid: the shell's cyan grid only. No chip, no riders, no guessed loop. */
static void zap_paint_grid(Gfx g)
{
    GATE_BOX(g, "BOARD", "defend", 106, 42, 176, 148, "-");
    bh_grid(g);
}

/* Shell strip. Same rows as the scan core; names the bot. No zap log. */
static void zap_strip(Gfx g, int dx, int th, int ty, int dw)
{
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
}

/* Rain only in the dark PCB rect behind the left chip. Cells that touch a chip or the silk are skipped. */
enum { ZR_X = 20, ZR_Y = 40, ZR_W = 76, ZR_H = 152 };
static const int16_t ZR_EX[3][4] = {
    { 39, 56, 46, 48 },   /* bot chip + pins */
    { 48, 100, 28, 28 },  /* device chip under it */
    { 28, 124, 64, 32 }   /* silk */
};
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
    botcorps_rain_draw_win(*g.raw(), g.oy, g.oy + g.raw()->height(), (int)n.x + g.ox, (int)n.y + g.oy,
                           (int)n.w, (int)n.h, ZR_EX, 3);
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
    zap_rain_tick();
}
