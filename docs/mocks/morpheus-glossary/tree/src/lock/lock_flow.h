#pragma once
/*
 * lock_flow — screen flow over the LOCKED clips (portrait 240x320).
 * BOOTbot intro -> GIBSONbot home intro -> home theme loop.
 * Home: GIBSON -> GIBSON game, CONTROL, SCANbot, TERMINALbot, BOOT -> replay intro.
 * TERMINALbot -> the 10 device games. Games play a phase and hold its
 * last frame; a tap plays the next phase; after the last phase a tap
 * restarts the round. Top-left icon = back to home. Top-bar swatches =
 * next theme (6 themes, shared by home / TERMINALbot / SCANbot).
 */
#include "lock_core.h"

enum LkScr { LK_SC_BOOT, LK_SC_HOME_INTRO, LK_SC_HOME, LK_SC_TERM, LK_SC_SCAN, LK_SC_CTL, LK_SC_GAME };

#define LK_NTHEME 6
static const char *const LK_THEME_NAME[LK_NTHEME] = {"GRAPE", "MAGENTA", "TOXIC", "ICE", "BLUE", "ORANGE"};

/* Phase ends (segment frame index where each phase holds). Phase starts are
 * the typing starts measured on the clips (tools/typed_starts.py). */
static const int16_t LK_HOLD_GIB[] = {66, 159, 220, 274, 336, 554, 629};
static const int16_t LK_HOLD_AIR[] = {32, 120, 208, 319, 498, 636, 738};
static const int16_t LK_HOLD_KEY[] = {32, 104, 176, 271, 434, 556, 658};

struct LkGame { const char *name; int seg; const int16_t *hold; int nhold; bool own_clip; };
#define LK_G(name, s, h, own) {name, s, h, (int)(sizeof(h) / sizeof(h[0])), own}
/* Index 0 = GIBSON (home). 1..10 = TERMINALbot devices, each on its own LOCKED clip
 * (LOCKED/ui_game_<game>/<game>@3x.mp4, built on AIR TAG v3: same 739 frames and
 * the same 7 phases, so they share LK_HOLD_AIR; typing starts re-measured per clip). */
enum { LK_G_GIBSON, LK_G_DEAUTH, LK_G_META, LK_G_HACK, LK_G_SKIMMER, LK_G_FLOCK, LK_G_FLIPPER,
       LK_G_DRONE, LK_G_AIRTAG, LK_G_KEYFOB, LK_G_SAMSUNG, LK_NGAME };
static const LkGame LK_GAME[LK_NGAME] = {
    LK_G("GIBSON", LK_S_GIB, LK_HOLD_GIB, true),
    LK_G("DEAUTH", LK_S_DEAUTH, LK_HOLD_AIR, true),
    LK_G("META", LK_S_META, LK_HOLD_AIR, true),
    LK_G("HACK", LK_S_HACK, LK_HOLD_AIR, true),
    LK_G("SKIMMER", LK_S_SKIMMER, LK_HOLD_AIR, true),
    LK_G("FLOCK", LK_S_FLOCK, LK_HOLD_AIR, true),
    LK_G("FLIPPER", LK_S_FLIPPER, LK_HOLD_AIR, true),
    LK_G("DRONE", LK_S_DRONE, LK_HOLD_AIR, true),
    LK_G("AIR TAG", LK_S_AIR, LK_HOLD_AIR, true),
    LK_G("KEYFOB", LK_S_KEY, LK_HOLD_KEY, true),
    LK_G("SAMSUNG", LK_S_SAMSUNG, LK_HOLD_AIR, true),
};

struct LkRect { int16_t x0, y0, x1, y1; int8_t act; };
static inline bool lk_in(const LkRect &r, int x, int y) { return x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1; }

enum { LK_A_BACK = -1, LK_A_THEME = -2, LK_A_BOOT = -3, LK_A_TERM = -4, LK_A_SCAN = -5, LK_A_CTL = -6 };
/* act >= 0 is a game index. */
static const LkRect LK_HOT_COMMON[] = {
    {0, 0, 22, 22, LK_A_BACK},
    {28, 0, 137, 17, LK_A_THEME},
};
static const LkRect LK_HOT_HOME[] = {
    {18, 22, 50, 62, LK_A_BOOT},
    {68, 20, 158, 130, LK_G_GIBSON},
    {170, 20, 205, 75, LK_A_CTL},
    {18, 135, 105, 215, LK_A_SCAN},
    {115, 135, 205, 215, LK_A_TERM},
};
static const LkRect LK_HOT_TERM[] = {
    {35, 30, 70, 80, LK_G_DEAUTH},   {170, 30, 205, 80, LK_G_META},
    {35, 87, 70, 137, LK_G_HACK},    {170, 87, 205, 137, LK_G_SKIMMER},
    {35, 145, 70, 195, LK_G_FLOCK},  {170, 145, 205, 195, LK_G_FLIPPER},
    {35, 200, 70, 250, LK_G_DRONE},  {80, 200, 115, 250, LK_G_AIRTAG},
    {124, 200, 160, 250, LK_G_KEYFOB}, {170, 200, 205, 250, LK_G_SAMSUNG},
};

struct LkFlow {
    LkPlay p;
    int scr = LK_SC_BOOT;
    int theme = 0;
    int game = 0;
    int phase = 0;
    bool theme_dirty = false;

    int theme_seg(int base) const { return base + theme; }
    void go(int s)
    {
        scr = s;
        switch (s) {
        case LK_SC_BOOT: p.start(LK_S_BOOT); break;
        case LK_SC_HOME_INTRO: p.start(LK_S_HOME_INTRO); break;
        case LK_SC_HOME: p.start(theme_seg(LK_S_HOME_T0)); break;
        case LK_SC_TERM: p.start(theme_seg(LK_S_TERM_T0)); break;
        case LK_SC_SCAN: p.start(theme_seg(LK_S_SCAN_T0)); break;
        case LK_SC_CTL: p.start(LK_S_CTL); break;
        case LK_SC_GAME: phase = 0; p.start(LK_GAME[game].seg, LK_GAME[game].hold[0]); break;
        }
    }
    void begin(int th) { theme = (th >= 0 && th < LK_NTHEME) ? th : 0; go(LK_SC_BOOT); }
    int period_ms(void) const { return scr == LK_SC_CTL ? 90 : 30; }

    void tick(const LkIo &io)
    {
        p.step(io);
        if (p.done && scr == LK_SC_BOOT) go(LK_SC_HOME_INTRO);
        else if (p.done && scr == LK_SC_HOME_INTRO) go(LK_SC_HOME);
    }

    void act(int a)
    {
        if (a >= 0) { game = a; go(LK_SC_GAME); return; }
        switch (a) {
        case LK_A_BACK: go(LK_SC_HOME); break;
        case LK_A_THEME:
            theme = (theme + 1) % LK_NTHEME; theme_dirty = true;
            go(scr);
            break;
        case LK_A_BOOT: go(LK_SC_BOOT); break;
        case LK_A_TERM: go(LK_SC_TERM); break;
        case LK_A_SCAN: go(LK_SC_SCAN); break;
        case LK_A_CTL: go(LK_SC_CTL); break;
        }
    }

    /* Tap at portrait (x,y). Returns the action taken, or 0 for none. */
    int tap(int x, int y)
    {
        if (scr == LK_SC_BOOT || scr == LK_SC_HOME_INTRO) { go(LK_SC_HOME); return 1; } /* tap skips intro */
        const bool themed = scr == LK_SC_HOME || scr == LK_SC_TERM || scr == LK_SC_SCAN;
        if (scr != LK_SC_HOME && lk_in(LK_HOT_COMMON[0], x, y)) { act(LK_A_BACK); return 1; }
        if (themed && lk_in(LK_HOT_COMMON[1], x, y)) { act(LK_A_THEME); return 1; }
        if (scr == LK_SC_HOME) {
            for (const auto &r : LK_HOT_HOME) if (lk_in(r, x, y)) { act(r.act); return 1; }
        } else if (scr == LK_SC_TERM) {
            for (const auto &r : LK_HOT_TERM) if (lk_in(r, x, y)) { act(r.act); return 1; }
        } else if (scr == LK_SC_GAME) {
            const LkGame &g = LK_GAME[game];
            if (!p.holding()) return 0; /* let a phase finish */
            if (phase + 1 < g.nhold) { ++phase; p.stop = g.hold[phase]; }
            else { phase = 0; p.start(g.seg, g.hold[0]); }
            return 1;
        }
        return 0;
    }
};
