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
static void paint_dg_board(Gfx) {}
static void paint_dg_demo(Gfx, int) {}
static void paint_dg_demo_hint(Gfx, int) {}
static void paint_dg_hud_ext(Gfx) {}
static void paint_dex(Gfx) {}
static void dex_open(void) {}
static void dg_paint_hud(Gfx, int, bool) {}

/* Deauth loop inside the photo-match grid. The field is the shell cyan grid
 * (bh_grid, same lines as the scan board). Tier still scores; it does not recolour the field.
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
/* Host: dl_scratch is declared with the prompt alias (ui.cpp, P0 R2). */
#ifndef BOTCORPS_HOST_RENDER
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
static_assert(sizeof(DlMem) <= GP_BUF_OFF, "the prompt alias sits past DlMem");
static_assert(sizeof(DlMem) + GP_BUF_N <= 256, "DlMem + prompt fit below livescan (+256)");
static_assert(GP_BUF_OFF + GP_BUF_N <= 256, "prompt alias below livescan (+256)");
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
/* NEW CUT (Look A): DlMem.on is the round state (no new RAM). Low nibble: 0 off, 1 live, 2 how-to card
 * (game entry, any key starts play), 3 down (death card: ENTER/CHIP restart, ESC back). Bit 0x10: a typed
 * zap command posted its result this round (zap_strip line 2 shows it). */
/* (the DL_* state enum is declared up front in botcorps_ui.cpp) */
static int dl_st(void) { return s_dl_on & 0x0F; }
static void dl_set_st(int st) { s_dl_on = st; }
static bool dl_live(void) { return dl_st() == DL_LIVE && s_screen == SCR_GCHAT; }
static bool dl_round(void) { return dl_st() != DL_OFF && s_screen == SCR_GCHAT && dl_bot() >= 2; }
/* How-to dismissed: the clock starts now. */
static void dl_go(void) { s_dl_t0 = millis(); dl_set_st(DL_LIVE); s_dirty = true; }
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
        if (dl_tier_of(s_dl_hi) >= 4) dl_award(DL_AW_ROOT);
        if (dl_tier_of(s_dl_hi) > was) {
            /* Look A tier-up card: every tier advance shows it for the 1.8 s pop (pop 6 = "tier up"),
             * even when the one-time award bit is already set. */
            dl_award(DL_AW_TIER);
            s_dl_pop = 6;
            s_dl_pop_until = millis() + 1800u;
            s_dirty = true;
        }
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
    dl_set_st(DL_LIVE);
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
static int dl_time_left(uint32_t t);
static void dl_end(void)
{
    if (dl_st() != DL_LIVE) { dl_set_st(DL_OFF); return; }
    const uint32_t spent = millis() - s_dl_t0 + (uint32_t)s_dl_docks * 3000u;
    if (spent < (uint32_t)DL_TIME0 && (uint32_t)DL_TIME0 - spent > 15000u) dl_award(DL_AW_SURV);
    if (s_dl_nulls >= 12 && s_dl_docks == 0) dl_award(DL_AW_SWEEP);
    dl_note_hi();
    s_dl_hold = (uint32_t)dl_time_left(millis());   /* the death card's frozen clock (aim dwell is unused now) */
    dl_set_st(DL_DOWN);
    s_dirty = true;
}
static int dl_charge_at(uint32_t t)
{
    if (dl_st() != DL_LIVE) return 0;
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

/* P0 Defend prompt (LOCK-DEFEND-CMDS 12:13): typed verb + device name [+ sub] on the open board.
 * Core verbs on every board; one signature verb and two subs per board; a bare name = zap. */
static const char *const ZP_CORE[5] = { "scan", "lock", "zap", "purge", "restart" };
static const char *const ZP_SIG[13] = {
    0, 0, "kick", "trace", "blind", "ground", "ghost", "roll", "blur", "void", "null", "echo", "overload"
};
static const char *const ZP_SUB[13][2] = {
    { 0, 0 }, { 0, 0 }, { "flood", "twin" }, { "rst", "burn" }, { "plate", "mask" }, { "spoof", "land" },
    { "mute", "unpair" }, { "relay", "seal" }, { "cover", "glitch" }, { "spot", "flag" }, { "replay", "clone" },
    { "ring", "unlink" }, { "core", "root" }
};
/* The 23-name universe: 12 bot names, then the DL_KINDS labels minus the duplicate "drone". */
enum { ZP_NAMES = 23 };
static void zp_name(int n, char *out, size_t cap)
{
    if (n < 12) { snprintf(out, cap, "%s", BOTS[n].short_name); return; }
    int k = 0;
    for (int i = 0; i < 12; ++i) {
        if (DL_KINDS[i] == GL_DRONE) continue;
        if (k++ == n - 12) {
            snprintf(out, cap, "%s", GL_LABEL[DL_KINDS[i]]);
            for (char *c = out; *c; ++c) if (*c >= 'A' && *c <= 'Z') *c = (char)(*c - 'A' + 'a');
            return;
        }
    }
    out[0] = 0;
}
static int zp_kind(int n)
{
    if (n < 12) return n == 5 ? GL_DRONE : bot_dev_kind(n);
    int k = 0;
    for (int i = 0; i < 12; ++i) {
        if (DL_KINDS[i] == GL_DRONE) continue;
        if (k++ == n - 12) return DL_KINDS[i];
    }
    return -1;
}
static bool zp_own(int bot, int n)
{
    if (bot >= 12) return true;
    if (n == bot) return true;
    return n >= 12 && zp_kind(n) == bot_dev_kind(bot);
}
/* Longest name at s that ends at a token boundary; returns its length (0 = none). */
static int zp_match(const char *s, int *name)
{
    int best = 0;
    char nm[16];
    for (int n = 0; n < ZP_NAMES; ++n) {
        zp_name(n, nm, sizeof nm);
        const int l = (int)strlen(nm);
        if (l > best && strncmp(s, nm, (size_t)l) == 0 && (s[l] == 0 || s[l] == ' ')) { best = l; *name = n; }
    }
    return best;
}
static void zp_upper(const char *in, char *out, size_t cap)
{
    snprintf(out, cap, "%s", in);
    for (char *c = out; *c; ++c) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
}
static bool zp_is_sig(const char *w)
{
    for (int b = 2; b <= 12; ++b) if (strcmp(w, ZP_SIG[b]) == 0) return true;
    return false;
}
static void zp_out(const char *row) { gc_push(dl_bot(), row); }   /* fix 2a: zap_strip line 2 shows the newest of these */
static void zp_miss(const char *line)
{
    char row[44];
    snprintf(row, sizeof row, "[ ?? ] %s", line);
    zp_out(row);
}
static bool zap_try_cmd(const char *in)
{
    if (s_screen != SCR_GCHAT) return false;
    const int bot = dl_bot();
    if (bot < 2 || bot > 12) return false;
    char line[GP_BUF_N];
    {   /* lowercase, single spaces, trimmed */
        size_t o = 0;
        bool sp = false;
        for (const char *c = in ? in : ""; *c && o + 1 < sizeof line; ++c) {
            char ch = *c;
            if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
            if (ch == ' ') { sp = o > 0; continue; }
            if (sp) { line[o++] = ' '; sp = false; if (o + 1 >= sizeof line) break; }
            line[o++] = ch;
        }
        line[o] = 0;
    }
    if (!line[0]) return false;
    /* first token */
    char verb[12];
    const size_t vw = strcspn(line, " ");
    snprintf(verb, sizeof verb, "%.*s", (int)(vw < sizeof verb ? vw : sizeof verb - 1), line);
    const char *rest = line[vw] ? line + vw + 1 : line + vw;
    const bool core = vw < sizeof verb && (strcmp(verb, "scan") == 0 || strcmp(verb, "lock") == 0
                      || strcmp(verb, "zap") == 0 || strcmp(verb, "purge") == 0 || strcmp(verb, "restart") == 0);
    const bool sig = vw < sizeof verb && zp_is_sig(verb);
    int name = -1;
    const char *after = 0;
    if (strcmp(line, "restart") == 0) {
        char echo[48];
        snprintf(echo, sizeof echo, "gibson> %s", line);
        log_add(echo);
        dl_begin(bot);
        zp_out("[ OK ] ROUND RESTART");
        return true;
    }
    if (core || sig) {
        if (!rest[0]) {
            if (strcmp(verb, "scan") == 0) return false;                  /* bare scan stays gc_run's */
        } else {
            const int l = zp_match(rest, &name);
            if (l) after = rest[l] ? rest + l + 1 : rest + l;
        }
    } else {
        const int l = zp_match(line, &name);
        if (!l) return false;                                              /* not a Defend line */
        snprintf(verb, sizeof verb, "zap");
        after = line[l] ? line + l + 1 : line + l;
    }
    char echo[48];
    snprintf(echo, sizeof echo, "gibson> %s", line);
    log_add(echo);
    /* validity (P4) */
    const bool overload = strcmp(verb, "overload") == 0;
    bool ok = true;
    if (sig && strcmp(verb, ZP_SIG[bot]) != 0) ok = false;                 /* another board's signature */
    if (strcmp(verb, "restart") == 0) ok = false;                          /* restart takes nothing */
    const char *subw = 0;
    if (ok) {
        if (name < 0) {
            if (rest[0] && !overload) ok = false;                          /* not a known name */
            else if (!rest[0] && !overload) ok = false;                    /* verb with no name */
            else if (rest[0] && overload) { subw = rest; }                 /* overload <sub> */
        } else {
            if (!zp_own(bot, name)) ok = false;                            /* foreign name */
            if (after && after[0]) subw = after;
        }
    }
    int sub = -1;
    if (ok && subw) {
        for (int k = 0; k < 2; ++k) if (strcmp(subw, ZP_SUB[bot][k]) == 0) sub = k;
        if (sub < 0) ok = false;                                           /* foreign sub or extra tokens */
    }
    if (!ok) { zp_miss(line); return true; }
    if (!dl_live()) { zp_out("[ -- ] ROUND OVER: RESTART"); return true; }
    char row[48], nmu[16], vu[12], su[10];
    zp_upper(verb, vu, sizeof vu);
    su[0] = 0;
    if (sub >= 0) zp_upper(ZP_SUB[bot][sub], su, sizeof su);
    if (overload) {
        int n = 0;
        for (int i = 0; i < DL_MAX; ++i) {
            if (!s_dl_d[i].live) continue;
            const int dmg = 1 + (s_dl_d[i].acq ? 1 : 0);
            s_dl_d[i].hp -= dmg;
            s_dl_score += dmg * 5;
            if (s_dl_d[i].hp <= 0) dl_kill(i, 0);
            else dl_note_hi();
            ++n;
        }
        if (su[0]) snprintf(row, sizeof row, "[ !! ] OVERLOAD %d HIT %s", n, su);
        else snprintf(row, sizeof row, "[ !! ] OVERLOAD %d HIT", n);
        zp_out(row);
        return true;
    }
    {
        char nm[16];
        zp_name(name, nm, sizeof nm);
        zp_upper(nm, nmu, sizeof nmu);
    }
    const int kind = zp_kind(name);
    if (strcmp(verb, "scan") == 0) {
        int n = 0;
        for (int i = 0; i < DL_MAX; ++i) if (s_dl_d[i].live && s_dl_d[i].kind == kind) ++n;
        snprintf(row, sizeof row, "[ .. ] SCAN %s %d LIVE", nmu, n);
        zp_out(row);
        return true;
    }
    int ti = -1;
    for (int i = 0; i < DL_MAX; ++i) {
        if (!s_dl_d[i].live || s_dl_d[i].kind != kind) continue;
        if (ti < 0 || s_dl_d[i].p > s_dl_d[ti].p) ti = i;
    }
    if (ti < 0) { snprintf(row, sizeof row, "[ -- ] NO %s ON GRID", nmu); zp_out(row); return true; }
    if (strcmp(verb, "lock") == 0) {
        s_dl_d[ti].acq = 1;
        int x = 0, y = 0;
        dl_xy(ti, &x, &y);
        s_dl_rx = x; s_dl_ry = y;
        snprintf(row, sizeof row, "[ >> ] LOCK %s", nmu);
        zp_out(row);
        return true;
    }
    if (strcmp(verb, "purge") == 0) {
        if (!s_dl_d[ti].acq) { zp_miss(line); return true; }
        s_dl_d[ti].hp = 0;
        dl_kill(ti, 0);
        snprintf(row, sizeof row, "[ !! ] PURGE %s NULL", nmu);
        zp_out(row);
        return true;
    }
    const int dmg = (strcmp(verb, "zap") == 0 ? 1 : 2) + (s_dl_d[ti].acq ? 1 : 0);
    s_dl_d[ti].hp -= dmg;
    s_dl_score += dmg * 5;
    const bool nulled = s_dl_d[ti].hp <= 0;
    if (nulled) dl_kill(ti, 0);
    else dl_note_hi();
    char tail[8];
    if (nulled) snprintf(tail, sizeof tail, "NULL");
    else snprintf(tail, sizeof tail, "-%d", dmg);
    if (su[0]) snprintf(row, sizeof row, "[ !! ] %s %s %s %s", vu, nmu, su, tail);
    else snprintf(row, sizeof row, "[ !! ] %s %s %s", vu, nmu, tail);
    zp_out(row);
    return true;
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

static void zap_look_a(Gfx g, uint32_t t);
static void zap_paint_grid(Gfx g)
{
    GATE_BOX(g, "BOARD", "defend", 106, 42, 176, 148, "-");
    /* Pre-Fork-B field: the scan-board cyan grid. No matrix fill, no tier glow. */
    bh_grid(g);
    if (!dl_round()) return;   /* NEW CUT: the how-to and death cards sit over this round's field */
    const uint32_t t = millis();
    const uint8_t bc = dl_accent();
    const uint8_t glass = 8;
    const uint8_t copper = botcorps_pal_fit(mix16(0xC300, botcorps_rgb565(bc), 40, 100));
    for (int i = 0; i < DL_MAX; ++i) {
        int x, y; dl_xy(i, &x, &y);
        sb_rect(g, x < 190 ? x : 190, 96, (x > 190 ? x - 190 : 190 - x) + 1, 1, copper);
        sb_rect(g, 190, y < 96 ? y : 96, 1, (y > 96 ? y - 96 : 96 - y) + 1, copper);
        if (!s_dl_d[i].live) continue;
        gl_icon(g, s_dl_d[i].kind, x - 6, y - 6, false);
        if (s_dl_d[i].acq) sb_text(g, "ACQ", x - 8, y - 12, 228, glass, false, 1);
    }
    if (g.raw()) botcorps_sprite_draw(*g.raw(), 190 - g.ox, 96 - g.oy, BOTCORPS_SHAPE_ROUND, 2, bc, false);
    sb_frame(g, s_dl_rx - 5, s_dl_ry - 5, 11, 11, bc);
    sb_rect(g, s_dl_rx - 1, s_dl_ry, 3, 1, (uint8_t)PAL_WHITE);
    sb_rect(g, s_dl_rx, s_dl_ry - 1, 1, 3, (uint8_t)PAL_WHITE);
    const int ch = dl_charge_at(t);
    sb_rect(g, 150, BH_Y + 2, 70, 4, (uint8_t)8);
    if (ch) sb_rect(g, 150, BH_Y + 2, 70 * ch / DL_CHG, 4, bc);
    char hud[24];
    const int st = dl_st();
    snprintf(hud, sizeof hud, "%ds", st == DL_LIVE ? dl_time_left(t) / 1000 : st == DL_DOWN ? (int)s_dl_hold / 1000 : DL_TIME0 / 1000);
    /* Index prompt occupies x108..123, y43..53. "45s" starts at x128 so the boxes do not meet. */
    sb_text(g, hud, 128, BH_Y + 2, (uint8_t)PAL_WHITE, glass, false, 1);
    snprintf(hud, sizeof hud, "L%d %d", dl_lv_of(s_dl_hi), s_dl_score);
    {   /* Look A header_score: right edge held at x256 once the score outgrows the x228 slot */
        const int hw = (int)strlen(hud) * 6 - 1;
        sb_text(g, hud, 257 - hw < 228 ? 257 - hw : 228, BH_Y + 1, (uint8_t)PAL_WHITE, glass, false, 1);
    }
    for (int i = 0; i < DL_MAX; ++i) if (s_dl_d[i].live && s_dl_d[i].acq)
        sb_text(g, "TARGET ACQUIRED", 128, 78, (uint8_t)PAL_WHITE, 228, false, 1);
    if (s_dl_bolt && (uint32_t)(t - s_dl_bolt) < 280u) {
        sb_rect(g, s_dl_rx < 190 ? s_dl_rx : 190, s_dl_ry, (s_dl_rx > 190 ? s_dl_rx - 190 : 190 - s_dl_rx) + 1, 1, (uint8_t)PAL_WHITE);
        sb_frame(g, s_dl_rx - 8, s_dl_ry - 8, 17, 17, bc);
    }
    if (s_dl_pop >= 0 && s_dl_pop != 6 && (int32_t)(s_dl_pop_until - t) > 0) {   /* pop 6: Look A tier card */
        sb_rect(g, 124, 70, 140, 36, (uint8_t)8);
        sb_frame(g, 124, 70, 140, 36, bc);
        sb_text(g, "AWARD UNLOCKED", 132, 74, bc, 8, false, 1);
        gl_icon(g, DL_KINDS[s_dl_pop % 12], 132, 86, false);
        sb_text(g, DL_AW_NM[s_dl_pop], 148, 88, (uint8_t)PAL_WHITE, 8, false, 1);
    }
    zap_look_a(g, t);
}

static void zap_strip(Gfx g, int dx, int th, int ty, int dw)
{
    if (!dl_round()) {
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
    /* fix 2a: line 2 is the last typed command's result (its own bot row) while the round runs;
     * the death card's round shows "<TIER>  DOWN" (mock prompt_stats); else the tier name. */
    const char *last = s_log_n ? log_line(s_log_n - 1) : "";
    if (dl_st() == DL_DOWN) {
        snprintf(body, sizeof body, "%s  DOWN", DL_TIER_NM[dl_tier_of(s_dl_hi)]);
        sb_tt(g, body, dx + 3, th + 11, ac);
    } else if (last[0] == '\x01' && (int)(uint8_t)last[1] == dl_bot()) {
        const char *r = last + 2;
        const bool ok = strncmp(r, "[ OK ]", 6) == 0;
        char tag[8];
        snprintf(tag, sizeof tag, "%.7s", r);
        const int x2 = sb_tt(g, tag, dx + 3, th + 11, ok ? (uint8_t)PAL_OK : ac);
        sb_tt(g, strlen(r) > 7 ? r + 7 : "", x2, th + 11, sb_ink());
    } else {
        sb_tt(g, DL_TIER_NM[dl_tier_of(s_dl_hi)], dx + 3, th + 11, ac);
    }
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
 * device name (right of the left-mounted chip), or the silk are skipped. */
/* Same window the gate already counts (top cell row at y=40). Spec crop (20,44)
 * is this rect with the top 4 px removed; those 4 px are the visible rain under the bar. */
enum { ZR_X = 20, ZR_Y = 40, ZR_W = 76, ZR_H = 152 };
static void zr_exclusions(int16_t ex[4][4])
{
    /* Chip 54 at (61,94), silk at +32/+42/+50, device 26 at (76,170), tiny label at x28. */
    ex[0][0] = 32; ex[0][1] = 64; ex[0][2] = 60; ex[0][3] = 60;
    ex[1][0] = 18; ex[1][1] = 124; ex[1][2] = 74; ex[1][3] = 28;
    ex[2][0] = 58; ex[2][1] = 152; ex[2][2] = 36; ex[2][3] = 36;
    ex[3][0] = 26; ex[3][1] = 160; ex[3][2] = 34; ex[3][3] = 20;
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
/* ===== NEW CUT: Look A (mocks game-prompt-restart v2/LOOK-A how-to + status-rail + death,
 * v1/LOOK-A tier-up). Geometry is the mock's render.py, inclusive boxes, ported op for op. ===== */
static uint8_t la_c(uint8_t r, uint8_t gr, uint8_t b)
{
    return botcorps_pal_fit((uint16_t)(((r & 0xF8u) << 8) | ((gr & 0xFCu) << 3) | (b >> 3)));
}
#define LA_BG la_c(8, 12, 16)
#define LA_PANEL la_c(16, 16, 16)
#define LA_PUR la_c(181, 36, 255)
#define LA_PUR2 la_c(148, 36, 173)
#define LA_PURD la_c(90, 36, 99)
#define LA_IVORY la_c(247, 243, 231)
#define LA_WHITE la_c(255, 255, 255)
#define LA_CYAN la_c(0, 255, 255)
#define LA_GOLD la_c(222, 219, 82)
#define LA_SOFT la_c(140, 154, 165)
#define LA_RED la_c(255, 60, 60)
#define LA_GREEN la_c(0, 255, 120)
static uint8_t la_tier_c(int t)
{
    switch (t) {
    case 1: return LA_GOLD;
    case 2: return la_c(255, 138, 24);
    case 3: return la_c(255, 82, 181);
    case 4: return LA_IVORY;
    default: return la_c(107, 121, 132);
    }
}
static void la_fill(Gfx g, int x0, int y0, int x1, int y1, uint8_t c) { g.fillRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, c); }
static void la_box(Gfx g, int x0, int y0, int x1, int y1, uint8_t c) { g.drawRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, c); }
static int la_gw(const char *s, int size = 1) { const int n = (int)strlen(s); return n ? (n * 6 - 1) * size : 0; }
static void la_glcd(Gfx g, int x, int y, const char *s, uint8_t c, int size = 1)
{
    g.setFont(&fonts::Font0);
    g.setTextSize(size);
    g.setTextColor(c);
    g.setTextDatum(lgfx::textdatum::top_left);
    g.drawString(s, x, y);
    g.setTextSize(1);
}
/* TINY3 3x5 plus the mock's extra glyphs (render.py TX). Separate from tiny_text so no locked silk moves. */
static void la_tiny(Gfx g, int x, int y, const char *s, uint8_t c)
{
    for (int i = 0; s[i]; ++i) {
        const char ch = s[i];
        uint16_t m = 0;
        if (ch >= '0' && ch <= '9') m = TINY3[ch - '0'];
        else if (ch >= 'A' && ch <= 'Z') m = TINY3[10 + ch - 'A'];
        else if (ch == '-') m = TINY3[36];
        else if (ch == ':') m = 0x0410;
        else if (ch == '/') m = 0x12A4;
        else if (ch == '+') m = 0x05D0;
        else if (ch == '.') m = 0x0002;
        if (!m) continue;
        for (int b = 0; b < 15; ++b)
            if (m & (1u << (14 - b))) g.fillRect(x + i * 4 + b % 3, y + b / 3, 1, 1, c);
    }
}
static void la_tiny_c(Gfx g, int cx, int y, const char *s, uint8_t c) { la_tiny(g, cx - tiny_w(s) / 2, y, s, c); }
/* dim(box, 0.30): every pixel already drawn in the box (clipped to the open patch) to 30 % RGB. */
static void la_dim(Gfx g, int x0, int y0, int x1, int y1)
{
    const ZrClip c = zr_clip_get(g);
    int bx0 = x0 - g.ox, by0 = y0 - g.oy, bx1 = x1 - g.ox + 1, by1 = y1 - g.oy + 1;
    if (c.on) {
        if (bx0 < c.x) bx0 = (int)c.x;
        if (by0 < c.y) by0 = (int)c.y;
        if (bx1 > c.x + c.w) bx1 = (int)(c.x + c.w);
        if (by1 > c.y + c.h) by1 = (int)(c.y + c.h);
    }
    if (bx1 <= bx0 || by1 <= by0) return;
#ifdef BOTCORPS_HOST_RENDER
    extern uint8_t *host_frame;
    extern int host_slice_h;
    if (!host_frame) return;
    for (int sy = by0; sy < by1; ++sy) {
        if (sy < 0 || sy >= 240 || (host_slice_h && sy >= host_slice_h)) continue;
        for (int sx = bx0; sx < bx1; ++sx) {
            if (sx < 0 || sx >= 320) continue;
            uint8_t &px = host_frame[sy * 320 + sx];
            const uint16_t v = botcorps_rgb565(px);
            px = botcorps_pal_fit((uint16_t)(((((v >> 11) & 31u) * 30u / 100u) << 11)
                                             | ((((v >> 5) & 63u) * 30u / 100u) << 5) | ((v & 31u) * 30u / 100u)));
        }
    }
#else
    if (!botcorps_band_dim_rect(bx0 + g.ox, by0 + g.oy, bx1 - bx0, by1 - by0, 30))
        g.fillRect(bx0 + g.ox, by0 + g.oy, bx1 - bx0, by1 - by0, la_c(2, 3, 4));
#endif
}
static void la_card(Gfx g, int x0, int y0, int x1, int y1, uint8_t rim, uint8_t fill, uint8_t rim2)
{
    la_fill(g, x0, y0, x1, y1, fill);
    la_box(g, x0, y0, x1, y1, rim);
    la_box(g, x0 + 1, y0 + 1, x1 - 1, y1 - 1, rim2);
}
static int la_key(Gfx g, int x, int y, const char *label, uint8_t col)
{
    const int w = la_gw(label) + 4;
    la_fill(g, x, y, x + w - 1, y + 9, col);
    la_glcd(g, x + 2, y + 1, label, LA_BG);
    return x + w;
}
/* rail_A: the 7 px dead strip between the grid frame (y144) and the prompt (y152). */
static void la_rail(Gfx g, bool flash)
{
    const int hi = s_dl_hi < 0 ? 0 : s_dl_hi, t = dl_tier_of(hi);
    const uint8_t tc = la_tier_c(t);
    char b[24];
    la_fill(g, 106, 145, 279, 151, LA_BG);
    snprintf(b, sizeof b, "LV%02d", dl_lv_of(hi));
    la_tiny(g, 108, 146, b, LA_IVORY);
    snprintf(b, sizeof b, "HI %05d", hi);
    la_tiny(g, 128, 146, b, LA_IVORY);
    snprintf(b, sizeof b, "T%d %s", t, DL_TIER_NM[t]);
    la_tiny(g, 164, 146, b, tc);
    for (int i = 0; i < 4; ++i) {
        const int px = 260 + i * 5;
        la_fill(g, px, 147, px + 2, 149, i < t ? la_tier_c(i + 1) : LA_PURD);
    }
    la_tiny(g, 236, 146, "AWD", LA_SOFT);
    if (flash) la_box(g, 162, 145, 162 + tiny_w(b) + 3, 151, tc);
}
/* Defend boards other than Gibson: the how-to's GIBSON row and worked example are his; the same card
 * names this board's own signature verb and subs from the lock table (derived, flagged in CLOSEOUT). */
static void la_howto(Gfx g)
{
    const int b = dl_bot(), CX = 193;
    char r3[40], r4[40], ex[4][24];
    if (b >= 12) {
        snprintf(r3, sizeof r3, "GIBSON: OVERLOAD  SUBS: CORE ROOT");
        snprintf(r4, sizeof r4, "OVERLOAD HITS ALL  ANY DEVICE NAME");
        snprintf(ex[0], 24, "scan airtag"); snprintf(ex[1], 24, "lock airtag");
        snprintf(ex[2], 24, "overload core"); snprintf(ex[3], 24, "purge airtag root");
    } else {
        char v[12], s0[12], s1[12], n[16];
        zp_upper(ZP_SIG[b], v, sizeof v);
        zp_upper(ZP_SUB[b][0], s0, sizeof s0);
        zp_upper(ZP_SUB[b][1], s1, sizeof s1);
        zp_upper(BOTS[b].short_name, n, sizeof n);
        snprintf(r3, sizeof r3, "%s: %s  SUBS: %s %s", n, v, s0, s1);
        snprintf(r4, sizeof r4, "%s HITS ITS DEVICE", v);
        char dn[16];
        zp_name(b, dn, sizeof dn);                            /* the bot's own name is always its device */
        for (int k = 12; k < ZP_NAMES; ++k) if (zp_kind(k) == bot_dev_kind(b)) { zp_name(k, dn, sizeof dn); break; }
        snprintf(ex[0], 24, "scan %s", dn); snprintf(ex[1], 24, "lock %s", dn);
        snprintf(ex[2], 24, "%s %s", ZP_SIG[b], dn); snprintf(ex[3], 24, "purge %s %s", dn, ZP_SUB[b][0]);
    }
    la_dim(g, 107, 55, 279, 142);
    la_card(g, 112, 55, 274, 142, LA_PUR, LA_PANEL, LA_PURD);
    la_fill(g, 114, 57, 272, 66, LA_PUR2);
    la_glcd(g, CX - la_gw("HOW TO DEFEND") / 2, 58, "HOW TO DEFEND", LA_WHITE);
    la_tiny_c(g, CX, 69, "TYPE AT PROMPT: VERB DEVICE SUB", LA_CYAN);
    la_tiny_c(g, CX, 75, "ALL: SCAN LOCK ZAP PURGE RESTART", LA_IVORY);
    la_tiny_c(g, CX, 81, r3, LA_GOLD);
    la_tiny_c(g, CX, 87, r4, LA_SOFT);
    la_fill(g, 118, 93, 268, 93, LA_PURD);
    for (int i = 0; i < 4; ++i) {
        const int y = 95 + i * 9;
        la_glcd(g, 120, y, "$", LA_PUR);
        la_glcd(g, 129, y, ex[i], LA_GREEN);
    }
    la_tiny_c(g, CX, 134, "RESTART AFTER DOWN  TAB: NAMES+VERBS", LA_SOFT);
}
static void la_tierup(Gfx g)
{
    const int hi = s_dl_hi < 0 ? 0 : s_dl_hi, t = dl_tier_of(hi);
    const uint8_t tc = la_tier_c(t);
    char b[24];
    la_box(g, 150, 73, 238, 113, tc);
    la_card(g, 152, 75, 236, 111, tc, LA_PANEL, LA_PURD);
    la_glcd(g, 194 - la_gw("TIER UP") / 2, 78, "TIER UP", tc);
    snprintf(b, sizeof b, "T%d %s", t, DL_TIER_NM[t]);
    la_glcd(g, 194 - la_gw(b, 2) / 2, 88, b, LA_WHITE, 2);
    snprintf(b, sizeof b, "LV%02d  AWARD +1", dl_lv_of(hi));
    la_tiny_c(g, 194, 104, b, LA_GOLD);
    static const uint8_t TK[4][2] = { { 146, 75 }, { 242, 75 }, { 146, 111 }, { 242, 111 } };
    for (int i = 0; i < 4; ++i) {
        la_fill(g, TK[i][0] - 1, TK[i][1], TK[i][0] + 1, TK[i][1], tc);
        la_fill(g, TK[i][0], TK[i][1] - 1, TK[i][0], TK[i][1] + 1, tc);
    }
}
static void la_death(Gfx g)
{
    const int hi = s_dl_hi < 0 ? 0 : s_dl_hi;
    char b[24];
    la_dim(g, 107, 55, 279, 142);
    la_card(g, 134, 62, 254, 136, LA_RED, LA_PANEL, LA_PURD);
    la_glcd(g, 194 - la_gw("DOWN", 2) / 2, 67, "DOWN", LA_RED, 2);
    la_tiny_c(g, 194, 84, "CORE BREACHED", LA_SOFT);
    snprintf(b, sizeof b, "HI %05d LV%02d", hi, dl_lv_of(hi));
    la_glcd(g, 194 - la_gw(b) / 2, 92, b, LA_IVORY);
    int r = la_key(g, 140, 105, "ENTER", LA_GOLD);
    la_tiny(g, r + 3, 107, "/CHIP", LA_GOLD);
    la_glcd(g, 202, 106, "RESTART", LA_WHITE);
    la_key(g, 140, 120, "ESC", LA_SOFT);
    la_glcd(g, 202, 121, "BACK", LA_SOFT);
}
static bool la_tier_pop(uint32_t t) { return s_dl_pop == 6 && (int32_t)(s_dl_pop_until - t) > 0; }
/* Called last in zap_paint_grid: the round's overlay card, then the rail. */
static void zap_look_a(Gfx g, uint32_t t)
{
    const int st = dl_st();
    if (st == DL_HOWTO) la_howto(g);
    else if (st == DL_DOWN) la_death(g);
    else if (la_tier_pop(t)) la_tierup(g);
    la_rail(g, st == DL_LIVE && la_tier_pop(t));
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
static void paint_gchat(Gfx g);
/* fix 2c: a patch repaints the whole Defend composition (paint_sbox, then the Tab sheet and gc panels),
 * so an open sheet's verb labels are no longer overpainted by the bare board. */
static void zap_sb_part(Gfx g) { paint_gchat(g); }
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
    /* NEW CUT: a pop's 1.8 s ends without a key; repaint so the tier card / award box leaves */
    if (s_dl_pop >= 0 && (int32_t)(s_dl_pop_until - t) <= 0) { s_dl_pop = -1; s_dirty = true; }
    zap_rain_tick();
}
