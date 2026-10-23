/* cyd-term-fix items 1/5 (CYD only, included from botcorps_ui.cpp under BOTCORPS_CYD).
 *
 * Every landscape terminal (Terminal, Gibson hub prompt, scan / bot boards, Defend boards) has two
 * command-row buttons: the [abc] keyboard emblem and the command glossary (the ≡ menu art). Each one
 * opens its mode in the one pop-up area (y >= CYD_OSK_CLIP_Y, the keyboard area):
 *   keyboard  = the existing OSK (its HIDE key is the back button),
 *   glossary  = a scrolling list of this terminal's commands, with its own BACK.
 * The command line of the active terminal stays visible as the top row of the pop-up. Picking a list
 * entry fills that command line WITHOUT running it and switches to the keyboard; ENTER then runs the
 * line through the terminal's own key path, exactly like a hardware Enter.
 *
 * All geometry is rect-parameterised (CydRect) so paint and hit-test share one source. */

struct CydRect {
    int16_t x, y, w, h;
};

static bool cyd_in(CydRect r, int x, int y, int pad)
{
    return x >= r.x - pad && x < r.x + r.w + pad && y >= r.y - pad && y < r.y + r.h + pad;
}

static const CydRect CYD_POP = { 0, (int16_t)CYD_OSK_CLIP_Y, 320, (int16_t)(240 - CYD_OSK_CLIP_Y) };
static const int CYD_PL_COLS = 2, CYD_PL_ROWS = 4, CYD_PL_PAGE = CYD_PL_COLS * CYD_PL_ROWS;
enum { CYD_PB_BACK = 0, CYD_PB_UP = 1, CYD_PB_DN = 2, CYD_PB_KEYS = 3, CYD_PB_N = 4 };


/* List cell `slot` (0..7) inside pop-up rect a: 2 columns x 4 rows on the OSK row pitch. */
static CydRect cyd_pl_cell(CydRect a, int slot)
{
    const int c = slot % CYD_PL_COLS, r = slot / CYD_PL_COLS;
    return { (int16_t)(a.x + CYD_OSK_X0 + c * 152), (int16_t)(a.y + 14 + r * CYD_OSK_ROW),
             149, (int16_t)(CYD_OSK_ROW - 1) };
}
/* Bottom row of the list: BACK, up, down, keyboard - same cells as the OSK HIDE/.. row (span 2). */
static CydRect cyd_pl_btn(CydRect a, int k)
{
    return { (int16_t)(a.x + CYD_OSK_X0 + k * 2 * CYD_OSK_COL), (int16_t)(a.y + 14 + 4 * CYD_OSK_ROW),
             (int16_t)(2 * CYD_OSK_COL - 1), (int16_t)(CYD_OSK_ROW - 1) };
}
static CydRect cyd_cmdline_rect(CydRect a)
{
    return { a.x, (int16_t)(a.y + 1), a.w, 12 };
}

/* ---- per-terminal command line + glossary --------------------------------------------------- */
enum { CYD_T_NONE = 0, CYD_T_TERM, CYD_T_HUB, CYD_T_SBOX, CYD_T_GC };
static int cyd_term_kind(void)
{
    if (s_screen == SCR_TERMINAL) return CYD_T_TERM;
    if (s_screen == SCR_GIBSON) return CYD_T_HUB;
    if (sbox_on()) return CYD_T_SBOX;
    if (s_screen == SCR_GCHAT) return CYD_T_GC;
    return CYD_T_NONE;
}
static char *cyd_cmd_buf(size_t *cap)
{
    switch (cyd_term_kind()) {
    case CYD_T_TERM: *cap = 60; return botcorps_t0_term_prompt;
    case CYD_T_HUB:
    case CYD_T_GC: *cap = GP_BUF_N; return s_gp_buf;
    case CYD_T_SBOX: *cap = sizeof(s_sp_buf); return s_sp_buf;
    default: *cap = 0; return nullptr;
    }
}
static const char *cyd_cmd_label(void)
{
    return cyd_term_kind() == CYD_T_HUB ? "gibson>" : "user@botcorps:~$";
}

/* Entry i of this terminal's glossary into out; returns the entry count. Defend boards follow the
 * game control lock (zp_own): Gibson takes every name, a bot board only its own names. */
static int cyd_pl_item(int i, char *out, size_t cap)
{
    if (out && cap) out[0] = 0;
    int n = 0;
    auto put = [&](const char *a, const char *b, const char *c) {
        if (n == i && out) {
            snprintf(out, cap, "%s%s%s%s%s", a, b ? " " : "", b ? b : "", c ? " " : "", c ? c : "");
        }
        ++n;
    };
    switch (cyd_term_kind()) {
    case CYD_T_TERM:
        for (int k = 0; k < CMD_N; ++k) put(CMDN[k], CMD_ARG[k] ? "" : nullptr, nullptr);
        for (int b = 2; b < 12; ++b) put("defend", BOTS[b].short_name, nullptr);
        break;
    case CYD_T_HUB:
        for (int k = 0; k < (int)(sizeof(GP_CMD) / sizeof(GP_CMD[0])); ++k) put(GP_CMD[k], nullptr, nullptr);
        break;
    case CYD_T_SBOX:
        for (int k = 0; k < SP_CMD_N; ++k) put(SP_CMD[k], nullptr, nullptr);
        break;
    case CYD_T_GC: {
        const int bot = dl_bot();
        if (bot >= 2 && bot <= 12) {
            static const char *const V[4] = { "scan", "lock", "zap", "purge" };
            char nm[16];
            for (int v = 0; v < 4; ++v)
                for (int k = 0; k < ZP_NAMES; ++k) {
                    if (!zp_own(bot, k)) continue;
                    zp_name(k, nm, sizeof nm);
                    put(V[v], nm, nullptr);
                }
            put("restart", nullptr, nullptr);
            if (bot == 12) {
                put(ZP_SIG[bot], nullptr, nullptr);
                put(ZP_SIG[bot], ZP_SUB[bot][0], nullptr);
                put(ZP_SIG[bot], ZP_SUB[bot][1], nullptr);
            } else {
                zp_name(bot, nm, sizeof nm);
                put(ZP_SIG[bot], nm, nullptr);
                put(ZP_SIG[bot], nm, ZP_SUB[bot][0]);
                put(ZP_SIG[bot], nm, ZP_SUB[bot][1]);
            }
        }
        for (int k = 0; k < GC_N; ++k) put(GC_NM[k], GC_ARG[k] ? "" : nullptr, nullptr);
        break;
    }
    default:
        break;
    }
    /* put(x, "") leaves "x " : an argument command lands with its trailing space, like the Adv sheet */
    return n;
}

/* ---- paint ---------------------------------------------------------------------------------- */
static void cyd_cmdline(Gfx g, CydRect r)
{
    size_t cap = 0;
    const char *buf = cyd_cmd_buf(&cap);
    g.fillRect(r.x, r.y, r.w, r.h, PAL_DEEP);
    const int keep = s_px;
    s_px = 6;
    const char *lab = cyd_cmd_label();
    txt(g, lab, 6, r.y + 2, (uint8_t)194, lgfx::textdatum::top_left);
    const int bx = 6 + ((int)strlen(lab) + 2) * 6;
    const char *show = buf ? buf : "";
    const int maxc = (r.w - bx - 10) / 6;
    const int len = (int)strlen(show);
    if (len > maxc) show += len - maxc;                     /* keep the caret end in view */
    txt(g, show, bx, r.y + 2, COL_WHITE, lgfx::textdatum::top_left);
    g.fillRect(bx + (int)strlen(show) * 6 + 1, r.y + 2, 5, 8, accent());
    s_px = keep;
}

static void cyd_pop_key(Gfx g, CydRect k, const char *lab, bool hot)
{
    g.fillRect(k.x, k.y, k.w, k.h, hot ? (uint8_t)PAL_SEL : (uint8_t)PAL_PLATE);
    g.drawRect(k.x, k.y, k.w, k.h, hot ? accent() : (uint8_t)PAL_LINE);
    if (!lab || !lab[0]) return;
    const int maxc = (k.w - 4) / 6;
    char b[28];
    snprintf(b, sizeof b, "%.*s", maxc < 27 ? maxc : 27, lab);
    const int tw = (int)strlen(b) * 6;
    txt(g, b, k.x + (k.w - tw) / 2, k.y + 3, PAL_INK, lgfx::textdatum::top_left);
}

static void cyd_pl_paint(CydRect a)
{
    Gfx g(botcorps_disp());
    g.startWrite();
    g.setClipRect(a.x, a.y, a.w, a.h);
    g.fillRect(a.x, a.y, a.w, a.h, PAL_DEEP);
    g.fillRect(a.x, a.y, a.w, 1, PAL_LINE);
    cyd_cmdline(g, cyd_cmdline_rect(a));
    const int keep = s_px;
    s_px = 6;
    const int n = cyd_pl_item(-1, nullptr, 0);
    char e[GP_BUF_N + 16];
    for (int s = 0; s < CYD_PL_PAGE; ++s) {
        const int i = s_cyd_pl_top + s;
        if (i >= n) break;
        cyd_pl_item(i, e, sizeof e);
        const CydRect c = cyd_pl_cell(a, s);
        g.fillRect(c.x, c.y, c.w, c.h, PAL_PLATE);
        g.drawRect(c.x, c.y, c.w, c.h, PAL_LINE);
        txt(g, e, c.x + 4, c.y + 3, PAL_INK, lgfx::textdatum::top_left);
    }
    cyd_pop_key(g, cyd_pl_btn(a, CYD_PB_BACK), "BACK", false);
    cyd_pop_key(g, cyd_pl_btn(a, CYD_PB_UP), s_cyd_pl_top > 0 ? "\x18 UP" : "", false);
    cyd_pop_key(g, cyd_pl_btn(a, CYD_PB_DN), s_cyd_pl_top + CYD_PL_PAGE < n ? "\x19 DN" : "", false);
    cyd_pop_key(g, cyd_pl_btn(a, CYD_PB_KEYS), "abc", false);
    char pg[12];
    const int pages = (n + CYD_PL_PAGE - 1) / CYD_PL_PAGE;
    snprintf(pg, sizeof pg, "%d/%d", s_cyd_pl_top / CYD_PL_PAGE + 1, pages < 1 ? 1 : pages);
    const CydRect pr = cyd_pl_btn(a, CYD_PB_N);
    txt(g, pg, pr.x + (pr.w - (int)strlen(pg) * 6) / 2, pr.y + 3, COL_MUTED, lgfx::textdatum::top_left);
    s_px = keep;
    g.clearClipRect();
    g.endWrite();
}

/* Keyboard mode on a non-Terminal terminal: the OSK echo row shows THIS terminal's command line. */
static void cyd_osk_echo(void)
{
    if (s_screen == SCR_TERMINAL || cyd_term_kind() == CYD_T_NONE) return;
    Gfx g(botcorps_disp());
    g.startWrite();
    g.setClipRect(CYD_POP.x, CYD_POP.y, CYD_POP.w, CYD_POP.h);
    cyd_cmdline(g, cyd_cmdline_rect(CYD_POP));
    g.clearClipRect();
    g.endWrite();
}

/* ---- command-row buttons (the existing plate art, placed where a terminal lacks them) -------- */
/* Rects of the two buttons on the current terminal: [abc] then glossary (≡). false = none here. */
static bool cyd_term_btns(CydRect *abc, CydRect *gl)
{
    switch (cyd_term_kind()) {
    case CYD_T_TERM:
        *abc = { TERM_ABC_X, TERM_BTN_Y, 22, 12 };
        *gl = { TERM_CMD_X, TERM_BTN_Y, 20, 12 };
        return true;
    case CYD_T_HUB:
        *abc = { 235, 187, 22, 12 };
        *gl = { 258, 187, 18, 12 };
        return true;
    case CYD_T_SBOX:
    case CYD_T_GC:
        *abc = { 236, 175, 22, 12 };                /* left of the sbox strip ≡ (dx+dw-22, ty-2) */
        *gl = { 260, 175, 19, 12 };
        return true;
    default:
        return false;
    }
}

/* [abc] emblem in the scan-box strip style (sb plate, Font0). */
static void cyd_sb_abc(Gfx g, CydRect r, bool lit, uint8_t ac)
{
    sb_rect(g, r.x, r.y, r.w, r.h, lit ? sb_tone(3) : (uint8_t)8);
    sb_frame(g, r.x, r.y, r.w, r.h, lit ? ac : sb_tone(4));
    sb_text(g, "abc", r.x + 3, r.y + 2, lit ? ac : (uint8_t)COL_MUTED, lit ? sb_tone(3) : (uint8_t)8, false, 1);
}
/* ≡ in the scan-box strip style - drawn only on strips that had none (radio / list boards). */
static void cyd_sb_menu(Gfx g, CydRect r, bool lit, uint8_t ac)
{
    sb_rect(g, r.x, r.y, r.w, r.h, lit ? sb_tone(3) : (uint8_t)8);
    sb_frame(g, r.x, r.y, r.w, r.h, lit ? ac : sb_tone(4));
    for (int k = 0; k < 3; ++k) sb_rect(g, r.x + 4, r.y + 3 + k * 3, 11, 1, lit ? ac : COL_MUTED);
}
static void cyd_sb_btns(Gfx g, bool has_menu, uint8_t ac)
{
    CydRect a, m;
    if (!cyd_term_btns(&a, &m)) return;
    cyd_sb_abc(g, a, false, ac);
    if (!has_menu) cyd_sb_menu(g, m, false, ac);
}
/* Hub strip [abc], in the hub ≡ plate style (PAL_DEEP fill, COL_LINE edge). */
static void cyd_hub_abc(Gfx g)
{
    const CydRect r = { 235, 187, 22, 12 };
    g.fillRect(r.x, r.y, r.w, r.h, PAL_DEEP);
    g.drawRect(r.x, r.y, r.w, r.h, COL_LINE);
    g.setFont(&fonts::Font0);
    g.setTextSize(1);
    g.setTextColor((uint8_t)COL_MUTED);
    g.setCursor(r.x + 3, r.y + 3);
    g.print("abc");
}

/* ---- open / close / input ------------------------------------------------------------------- */
static void cyd_pop_open(bool list)
{
    s_osk_open = true;
    s_osk_sym = false;
    s_cyd_pl = list;
    if (list) s_cyd_pl_top = 0;
    if (s_screen == SCR_TERMINAL) {
        s_term_prompt = true; s_focus_cmds = false; s_cmd_btn = false; s_atk_btn = false;
    } else if (s_screen == SCR_GIBSON) {
        s_gp_focus = true;
        s_hub_zone = 1;
    }
    s_osk_dirty = true;
    s_dirty = true;
}

/* Fill the command line with entry i. Never runs it. */
static void cyd_pl_pick(int i)
{
    size_t cap = 0;
    char *buf = cyd_cmd_buf(&cap);
    if (!buf || !cap) return;
    char e[GP_BUF_N + 16];
    cyd_pl_item(i, e, sizeof e);
    snprintf(buf, cap, "%s", e);
    if (s_screen == SCR_GCHAT) { s_gc_caret = -1; s_gc_hist_i = -1; }
    if (sbox_on()) s_sp_cmd = -1;
    if (s_screen == SCR_TERMINAL) s_tab_i = -1;
    s_cyd_pl = false;                                    /* keyboard: edit, then ENTER runs it */
    s_osk_dirty = true;
    s_dirty = true;
}

/* Touch for the pop-up and the two buttons. Returns a key, or -1 to fall through to the old hits. */
static int cyd_pop_touch(int tx, int ty)
{
    if (s_osk_open && s_cyd_pl && ty >= CYD_POP.y) {
        const int n = cyd_pl_item(-1, nullptr, 0);
        for (int s = 0; s < CYD_PL_PAGE; ++s) {
            if (s_cyd_pl_top + s >= n) break;
            if (cyd_in(cyd_pl_cell(CYD_POP, s), tx, ty, 0)) { cyd_pl_pick(s_cyd_pl_top + s); return K_NONE; }
        }
        if (cyd_in(cyd_pl_btn(CYD_POP, CYD_PB_BACK), tx, ty, 0)) { cyd_close_osk(); return K_NONE; }
        if (cyd_in(cyd_pl_btn(CYD_POP, CYD_PB_UP), tx, ty, 0)) {
            s_cyd_pl_top = s_cyd_pl_top >= CYD_PL_PAGE ? s_cyd_pl_top - CYD_PL_PAGE : 0;
            s_osk_dirty = true;
            return K_NONE;
        }
        if (cyd_in(cyd_pl_btn(CYD_POP, CYD_PB_DN), tx, ty, 0)) {
            if (s_cyd_pl_top + CYD_PL_PAGE < n) s_cyd_pl_top += CYD_PL_PAGE;
            s_osk_dirty = true;
            return K_NONE;
        }
        if (cyd_in(cyd_pl_btn(CYD_POP, CYD_PB_KEYS), tx, ty, 0)) {
            s_cyd_pl = false;
            s_osk_dirty = true;
            return K_NONE;
        }
        return K_NONE;
    }
    if (s_osk_open && ty >= CYD_POP.y) return -1;            /* keyboard keys: existing OSK hits */
    CydRect a, m;
    if (!cyd_term_btns(&a, &m)) return -1;
    if (s_osk_open) return -1;
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet) return -1;                              /* Adv pull-up sheet keeps its own hits */
        /* [abc] keeps its existing Terminal hit; the ≡ hit (CYD_HIT_CMD_X.., y>=CYD_ROW_Y) opens the list */
        if (tx >= CYD_HIT_CMD_X && ty >= CYD_ROW_Y) { cyd_pop_open(true); return K_NONE; }
        return -1;
    }
    if (cyd_in(a, tx, ty, 3)) { cyd_pop_open(false); return K_NONE; }
    if (cyd_in(m, tx, ty, 3)) { cyd_pop_open(true); return K_NONE; }
    return -1;
}

/* After a dispatched key while the pop-up is open: repaint the echo (typing was invisible because
 * K_CHAR / K_BS never set s_osk_dirty), and close it when ENTER consumed the line or the screen
 * changed, so the command's output is not left under the keyboard. */
static void cyd_pop_after(int key, int scr_before)
{
    if (!s_osk_open) return;
    if (s_screen != scr_before) {
        cyd_close_osk();
        return;
    }
    if (key == K_OK) {
        size_t cap = 0;
        const char *buf = cyd_cmd_buf(&cap);
        if (!buf || !buf[0]) { cyd_close_osk(); return; }
    }
    s_osk_dirty = true;
}

/* cyd_overlay hook: paints the list mode; false = keyboard mode (paint_osk + cyd_osk_echo). */
static bool cyd_pop_paint(void)
{
    if (!s_cyd_pl) return false;
    cyd_pl_paint(CYD_POP);
    return true;
}

/* ---- item 3: chip press on the glass ------------------------------------------------------------
 * Root cause ("sketchy / doesn't respond"): the touch hit returned K_OK into ui_dispatch, so
 *  (a) after the first press on a Defend board the FSM sits in CH_PLAY and chip_on_key ignores every
 *      later press (the "press restarts the round" path never runs again) - dead chip;
 *  (b) any text left in s_gp_buf / s_sp_buf, or a strip command cursor (s_sp_cmd >= 0), makes
 *      chip_on_key bail, so the same tap ran the command line instead of pressing the chip.
 * A tap ON the chip is unambiguous, so the glass presses the FSM directly (same transitions as the
 * K_OK branch of chip_on_key; CH_PLAY on an open board re-arms, which restarts the round). */
static const CydRect CYD_CHIP = { 34, 67, 54, 54 };
static int cyd_chip_press(void)
{
    uint8_t *m = chip_mem();
    int bot = 0, def = 0;
    if (!chip_now(&bot, &def)) return K_NONE;
    if (m[5] != (uint8_t)bot || m[13] != (uint8_t)s_screen) {        /* stale owner: re-arm for this board */
        m[0] = CH_IDLE; m[1] = 0; m[3] = 0; m[4] = 0; m[7] = 0;
        m[5] = (uint8_t)bot; m[6] = (uint8_t)def; m[13] = (uint8_t)s_screen;
    }
    m[2] = 1;                                                        /* the chip holds focus (no arrows) */
    if (m[0] == CH_PLAY || m[0] == CH_DONE) { m[0] = CH_IDLE; m[1] = 0; }
    if (m[0] == CH_IDLE) {
        m[0] = chip_is_game(bot, def) ? (uint8_t)CH_RING : (uint8_t)CH_SCAN;
        m[1] = 0; m[3] = 0; m[7] = 0;
        m[4] = (m[0] == CH_SCAN) ? 1 : 0;
        chip_set_t0(millis());
    }
    s_dirty = true;
    return K_NONE;
}
static int cyd_chip_touch(int tx, int ty)
{
    int cb = 0, cd = 0;
    if (s_osk_open || !chip_now(&cb, &cd) || !cyd_in(CYD_CHIP, tx, ty, 0)) return -1;
    return cyd_chip_press();
}

/* ---- item 10: top-bar quick buttons ---------------------------------------------------------------
 * Root cause: the glass had no hit for any top-bar button at its DRAWN rect. The Terminal crown hit sat
 * at x 140..180 (the old centre crown) while paint_term_icons draws it at 203..227; the scan-shell
 * bar (sb_top: crown "back" 143..177, >_ 190..206), the Control bar (crown x160, TERM 194..216) and
 * the Diag crown_btn (140..180) had none - only the top-left corner mapped to Esc. Each rect below is
 * the painter's own rect, mapped to the Adv key / action it stands for. */
static const CydRect CYD_SB_CROWN = { 143, 0, 34, 17 };      /* sb_top GATE_BOX CROWN "back"   -> Esc */
static const CydRect CYD_SB_TERM = { 190, 2, 16, 13 };       /* sb_top >_ quick terminal chip -> terminal */
static const CydRect CYD_TERM_CROWN = { 203, 0, 24, 16 };    /* paint_term_icons CROWN "back" -> hub */
static const CydRect CYD_CP_CROWN = { 142, 0, 36, 16 };      /* paint_control crown (cursor CP_N+1) */
static const CydRect CYD_CP_TERM = { 194, 1, 22, 14 };       /* paint_control BTN "term" (cursor CP_N) */
static const CydRect CYD_CROWN_BTN = { 140, 0, 40, 20 };     /* crown_btn (Diag) -> Esc */

static int cyd_top_touch(int tx, int ty)
{
    if (ty >= 24) return -1;
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet || !cyd_in(CYD_TERM_CROWN, tx, ty, 2)) return -1;
        if (s_osk_open) cyd_close_osk();
        enter_screen(SCR_GIBSON, SCR_TERMINAL);              /* the Terminal crown's existing action */
        return K_NONE;
    }
    if (s_screen == SCR_CONTROL) {
        if (cyd_in(CYD_CP_CROWN, tx, ty, 2)) { s_cursor = CP_N + 1; return K_OK; }   /* Adv: focus + Enter */
        if (cyd_in(CYD_CP_TERM, tx, ty, 2)) { s_cursor = CP_N; return K_OK; }
        return -1;
    }
    if (sbox_on() || zap_sb_on()) {
        if (cyd_in(CYD_SB_CROWN, tx, ty, 2)) {
            if (s_osk_open) cyd_close_osk();
            return K_ESC;                                   /* Adv Esc: steps back one layer, then leaves */
        }
        if (cyd_in(CYD_SB_TERM, tx, ty, 2)) {
            if (s_osk_open) cyd_close_osk();
            const int from = s_screen;
            if (from == SCR_GCHAT && dl_live()) dl_end();
            enter_screen(SCR_TERMINAL, from);               /* as the "terminal" command */
            boot_log();
            s_dirty = true;
            return K_NONE;
        }
        return -1;
    }
    if (s_screen == SCR_DIAG && cyd_in(CYD_CROWN_BTN, tx, ty, 0)) return K_ESC;
    return -1;
}

/* ---- item 6: bot rain behind the Terminal boards ---------------------------------------------------
 * The 72x46 board tiles are opaque, so the Terminal's live device rain only showed in the 4 px gaps and
 * the margins. Each tile now draws the SAME live rain cells (botcorps_rain_ghost: whole icons only, the
 * icon's own inks dimmed, same cells as the open field) into its bare board areas right after the base
 * fill, so traces, pins, the chip footprint, the bot face and the label all stay painted on top. The
 * footprint (x+23..49, y+14..36) and the label strip (y+36..44) are left clear. */
static const CydRect CYD_TILE_RAIN[3] = {
    { 1, 1, 22, 35 },     /* left board field  */
    { 49, 1, 22, 35 },    /* right board field */
    { 23, 1, 26, 13 },    /* above the footprint (under the IC pin comb) */
};
static void cyd_tile_rain(Gfx g, int x, int y)
{
    if (s_screen != SCR_TERMINAL || !g.raw()) return;
    if (!(wants_rain() || rain_static())) return;
#ifdef BOTCORPS_HOST_RENDER
    if (host_gate_norain) return;
    host_gate_mute = 1;
#endif
    for (int k = 0; k < 3; ++k) {
        const CydRect &r = CYD_TILE_RAIN[k];
        botcorps_rain_ghost(*g.raw(), g.ox, g.oy, x + r.x, y + r.y, r.w, r.h, 0);
    }
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 0;
#endif
}
