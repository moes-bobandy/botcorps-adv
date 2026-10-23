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

/* ---- item 7: light live rain preview on CYD Control --------------------------------------------------
 * The Control board (SB_BX/BY/BW/BH) is an opaque matte plate, so the theme chips (GRAPE / MASHUP / MIX
 * / RAIN / MULTI) and the RAIN / JUMP jumpers changed a rain you could only see in the 8 px frame. The
 * plate now draws the live rain (the existing engine: botcorps_rain_draw_win = same cells, glyphs,
 * palette, mode, mix and shape, hub-dimmed inks) right after its fill, before any board art, so every
 * part, label and trace stays on top and a theme change shows on the next frame. */
static const int CYD_CP_RAIN_EVERY = 3;
static void cyd_cp_rain(Gfx g)
{
    if (s_screen != SCR_CONTROL || !g.raw() || g.ox != 0) return;
    if (!wants_rain()) return;
#ifdef BOTCORPS_HOST_RENDER
    if (host_gate_norain) return;
#endif
    const ZrClip cl = zr_clip_push(g, SB_BX + 2, SB_BY + 2, SB_BW - 4, SB_BH - 4);
    const ZrClip n = zr_clip_get(g);
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 1;
#endif
    botcorps_rain_set_covers(nullptr, 0, false);         /* the plate cover keeps the FIELD rain off it */
    if (botcorps_rain_bg() != 0) {
        /* a BACKGROUND style (saver) is sparse by itself: preview it whole */
        botcorps_rain_draw_win(*g.raw(), g.oy, g.oy + g.raw()->height(), (int)n.x + g.ox, (int)n.y + g.oy,
                               (int)n.w, (int)n.h, nullptr, 0);
    } else {
        /* light: one rain column in CYD_CP_RAIN_EVERY, the cells/glyphs/inks are the live ones */
        for (int cx = 0; cx < 320; cx += 8 * CYD_CP_RAIN_EVERY) {
            const int x0 = cx > (int)n.x ? cx : (int)n.x;
            const int x1 = (cx + 8) < (int)(n.x + n.w) ? (cx + 8) : (int)(n.x + n.w);
            if (x1 <= x0) continue;
            botcorps_rain_draw_win(*g.raw(), g.oy, g.oy + g.raw()->height(), x0 + g.ox, (int)n.y + g.oy,
                                   x1 - x0, (int)n.h, nullptr, 0);
        }
    }
    botcorps_rain_set_covers(s_box, s_box_n, s_box_radar);  /* restored exactly */
#ifdef BOTCORPS_HOST_RENDER
    host_gate_mute = 0;
#endif
    zr_clip_set(g, cl);
}

/* ---- item 2: reachability audit - touch at the DRAWN rect for elements only arrows could reach -----
 * Each hit maps the tap to the key/state the Adv arrows+Enter already produce for that element, so the
 * existing handlers run unchanged. Rects are the painters' own (cited per line). -1 = not ours. */
static const CydRect CYD_RAIN_DISC = { 104, 44, 112, 112 };  /* rain ring disc r56 at (160,100) -> Enter */
static const CydRect CYD_HUB_CORE = { 106, 46, 108, 108 };   /* paint_hub GATE_BOX HUBCORE cx-54,cy-54 */
static const CydRect CYD_HUB_GP = { 38, 164, 196, 38 };      /* partial_gp_strip band, left of [abc] */
static const CydRect CYD_GRID = { SB_X, SB_Y, SB_W, SB_H };  /* scan-box grid (radar cells, Defend field) */
static const CydRect CYD_TOVL[4] = {                         /* paint_term overlays 1 gloss, 2 attack, 3 help */
    { 0, 0, 0, 0 }, { 24, 24, 272, 196 }, { 10, 20, 300, 208 }, { 6, 19, 308, 146 }
};
static const CydRect CYD_SB_GL = { 108, 43, 15, 10 };     /* scan-box ">" glossary prompt (dx 106, dy 42) */
static const CydRect CYD_GC_SHEET = { 14, 103, 292, 106 };   /* paint_gc_sheet bx 14, by 214-5-h, h 8*12+10 */

/* Gibson SCAN crew drop-down: walk gib_dropdown's own layout (lanes, open panel tiles, both rails). */
static int cyd_gd_touch(int tx, int ty)
{
#if BOTCORPS_GIB_DROPDOWN
    if (!(s_screen == SCR_SCAN && s_scan_bot == 12 && !s_gib_hunt) || s_sp_buf[0] || !bh_orbit()) return -1;
    if (tx < GD_X || tx >= GD_X + GD_W + 12 || ty < GD_LY || ty > GD_BOT) return -1;
    gd_fix();
    if (tx >= GD_X + GD_W + 1) {                              /* outer rail: lane up / down (Up/Down) */
        s_gd_dev = -1;                                        /* the rail walks lanes, not tiles */
        gd_key(ty < (GD_LY + GD_BOT) / 2 ? 1 : 2);
        s_dirty = true;
        return K_NONE;
    }
    int y = GD_LY;
    for (int b = s_gd_top; b < 12; ++b) {
        const bool open = b == s_gd_lane;
        const int need = GD_LH + (open ? gd_panel_h(b) : 0);
        if (y + need > GD_BOT + 1) break;
        if (ty >= y && ty < y + GD_LH) {                     /* lane header: open it / climb back to it */
            if (!open) { s_gd_lane = b; s_gd_row0 = 0; }
            s_gd_dev = -1;
            gd_fix();
            s_dirty = true;
            return K_NONE;
        }
        y += GD_LH;
        if (open) {
            const int ph = gd_panel_h(b), pr = gd_prows(b), n = gd_cnt(b);
            if (ty >= y && ty < y + ph) {
                const int rows = (n + 6) / 7;
                if (rows > pr && tx >= GD_X + GD_W - 9) {    /* inner rail: page the tile rows */
                    if (ty < y + ph / 2) { if (s_gd_row0 > 0) --s_gd_row0; }
                    else if (s_gd_row0 + pr < rows) ++s_gd_row0;
                    s_dirty = true;
                    return K_NONE;
                }
                const int c = (tx - GD_X) / SG_C, r = (ty - y) / SG_C;
                const int i = (s_gd_row0 + r) * 7 + c;
                if (c >= 0 && c < 7 && r < pr && i < n) {     /* a tile: select it (Right/arrows) */
                    s_gd_dev = i;
                    const int keep = s_gd_row0;
                    gd_fix();
                    s_gd_row0 = keep;
                    s_dirty = true;
                }
                return K_NONE;
            }
            y += ph;
        }
        y += 1;
    }
    return K_NONE;
#else
    (void)tx; (void)ty;
    return -1;
#endif
}

/* GCHAT command sheet: the same top/row mapping paint_gc_sheet draws. */
static int cyd_gc_sheet_touch(int tx, int ty)
{
    const CydRect r = CYD_GC_SHEET;
    const int bn = gc_bn(), sn = bn + GC_N;
    if (!cyd_in(r, tx, ty, 0)) { s_sheet = false; s_dirty = true; return K_NONE; }   /* outside = Esc */
    if (tx >= r.x + r.w - 20) {                               /* right strip: Up / Down */
        s_sheet_i = ty < r.y + r.h / 2 ? (s_sheet_i + sn - 1) % sn : (s_sheet_i + 1) % sn;
        s_dirty = true;
        return K_NONE;
    }
    const int rh = 12, vis = 8, bh = bn ? bn + 1 : 0, rows = bh + GC_N + 5;
    int sel = s_sheet_i;
    if (sel < 0) sel = 0;
    if (sel >= sn) sel = sn - 1;
    int sel_row = sel < bn ? 1 + sel : 0;
    for (int row = 0; sel >= bn && row < GC_N + 5; ++row) {
        int hdr = -1, cmd = 0;
        gc_row_at(row, &hdr, &cmd);
        if (hdr < 0 && cmd == sel - bn) { sel_row = bh + row; break; }
    }
    int top = sel_row - 3;
    if (top < 0) top = 0;
    if (top > rows - vis) top = rows - vis;
    const int k = (ty - (r.y + 5)) / rh;
    if (ty < r.y + 5 || k >= vis) return K_NONE;
    const int row = top + k;
    if (row < bh) {
        if (row == 0) return K_NONE;                          /* BOARD header */
        s_sheet_i = row - 1;
    } else {
        int hdr = -1, cmd = 0;
        if (row - bh >= GC_N + 5) return K_NONE;
        gc_row_at(row - bh, &hdr, &cmd);
        if (hdr >= 0) return K_NONE;                          /* group header */
        s_sheet_i = cmd + bn;
    }
    gc_pick();                                                /* Enter on the row */
    s_dirty = true;
    return K_NONE;
}

static int cyd_audit_touch(int tx, int ty)
{
    if (s_osk_open && ty >= CYD_OSK_CLIP_Y) return -1;        /* the keyboard owns its band */
    if (s_cyd_pl) return -1;                                  /* the list pop-up owns the glass */
    if (s_screen == SCR_RAIN) return cyd_in(CYD_RAIN_DISC, tx, ty, 0) ? K_OK : -1;
    if (s_screen == SCR_GIBSON) {
        if (cyd_in(CYD_HUB_CORE, tx, ty, 0)) { s_hub_zone = 3; s_gp_focus = false; return K_OK; }
        if (cyd_in(CYD_HUB_GP, tx, ty, 0)) { s_hub_zone = 1; s_gp_focus = true; s_dirty = true; return K_NONE; }
        if (s_hub_zone != 0) { s_hub_zone = 0; s_gp_focus = false; s_dirty = true; }   /* buttons zone */
        return -1;
    }
    if (s_screen == SCR_TERMINAL) {
        if (s_sheet || s_term_ovl < 1 || s_term_ovl > 3 || ty >= CYD_ROW_Y) return -1;
        const CydRect o = CYD_TOVL[s_term_ovl];
        if (!cyd_in(o, tx, ty, 0)) return K_ESC;              /* outside the panel = Esc */
        if (s_term_ovl != 3 && ty < o.y + 16) return K_RT;    /* title bar: flip glossary <-> attack */
        return s_term_ovl == 2 ? K_OK : K_ESC;                /* attack: open the board; viewers: close */
    }
    if (s_screen == SCR_GCHAT && zap_sb_on()) {
        if (ty >= CYD_ROW_Y) return -1;
        if (s_sheet) return cyd_gc_sheet_touch(tx, ty);
        if (s_gc_panel) { s_gc_panel = 0; s_dirty = true; return K_NONE; }   /* panel: tap = back */
        if (dl_live() && !s_gp_buf[0] && cyd_in(CYD_GRID, tx, ty, 0)) {
            s_dl_rx = tx; s_dl_ry = ty;
            dl_steer(0);                                      /* clamp exactly as the arrows do */
            chip_mem()[2] = 0;                                /* the grid takes focus (Adv: the steer arrow) */
            return K_OK;                                      /* Enter = fire at the reticle */
        }
        return -1;
    }
    if (s_screen == SCR_SCAN) return cyd_gd_touch(tx, ty);
    if ((s_screen == SCR_WIFI_RADAR || s_screen == SCR_BT_RADAR) && ty < 172 && !cyd_in(CYD_CHIP, tx, ty, 0)
        && tx >= SB_X - 8) {
        chip_mem()[2] = 0;                                    /* the grid takes focus (Adv: an arrow) */
        /* ">" glossary prompt at its drawn rect (paint dx+2,dy+1 15x10); the old hit sat at SB_Y..+14,
         * under cell 0's own hit, so it never fired. */
        if (s_act < 0 && s_info_dev < 0 && s_gloss < 0 && s_hunt_dev < 0 && !s_fa_on
            && tx >= CYD_SB_GL.x - 2 && tx < CYD_SB_GL.x + CYD_SB_GL.w + 2 && ty >= CYD_SB_GL.y - 2
            && ty < CYD_SB_GL.y + CYD_SB_GL.h) {
            s_cursor = LIST_N;
            return K_OK;                                      /* on_ok -> s_gloss = 0 */
        }
    }
    return -1;
}
