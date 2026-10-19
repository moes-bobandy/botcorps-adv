#include <string>
#include <stdlib.h>
#include "host_shim.h"
#include "ui/botcorps_pal.h"
#include "ui/botcorps_pal_tables.inc"
#include "ui/botcorps_term_ov.h"
extern int botcorps_cp_overlap_count(void);
extern int botcorps_cp_edge_overflow_count(void);
extern int botcorps_cp_sprite_inside(const unsigned char *buf);
extern void host_rain_init(int mode);

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint16_t pack332(uint8_t i)
{
    const int r3 = (i >> 5) & 7;
    const int g3 = (i >> 2) & 7;
    const int b2 = i & 3;
    const int r8 = (((r3 << 3) + r3) << 2) + (r3 >> 1);
    const int g8 = (((g3 << 3) + g3) << 2) + (g3 >> 1);
    const int b8 = b2 * 0x55;
    const int r5 = r8 >> 3;
    const int g6 = g8 >> 2;
    const int b5 = b8 >> 3;
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static uint16_t word_of(uint8_t idx, int term)
{
    /* Same native word push_slice byte-swaps onto the EXT panel. */
    const uint16_t ov = botcorps_term_ov_native(idx, term);
    if (ov) return ov;
    if (k_is_res[idx]) return k_exact_nat[idx];
    return pack332(idx);
}

static void rgb_of(uint8_t idx, int term, uint8_t *r, uint8_t *g, uint8_t *b)
{
    const uint16_t c = word_of(idx, term);
    const int r5 = (c >> 11) & 31;
    const int g6 = (c >> 5) & 63;
    const int b5 = c & 31;
    *r = (uint8_t)((r5 << 3) | (r5 >> 2));
    *g = (uint8_t)((g6 << 2) | (g6 >> 4));
    *b = (uint8_t)((b5 << 3) | (b5 >> 2));
    if (!term && idx == 0) { *r = *g = *b = 0; }
    if (!term && idx == 255) { *r = *g = *b = 255; }
}

static void write_raw(const char *path, const uint8_t *idx, int term)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    for (int i = 0; i < 320 * 240; ++i) {
        uint8_t r, g, b;
        rgb_of(idx[i], term, &r, &g, &b);
        fputc(r, f);
        fputc(g, f);
        fputc(b, f);
    }
    fclose(f);
}

static void hist_term(const uint8_t *idx)
{
    int hit[17];
    for (int n = 0; n < 17; ++n) hit[n] = 0;
    int other = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        int found = 0;
        for (int n = 0; n < botcorps_term_ov_n; ++n)
            if (idx[i] == botcorps_term_ov_i[n]) { hit[n]++; found = 1; break; }
        if (!found && idx[i] >= 230 && idx[i] <= 248) other++;
    }
    FILE *f = fopen("/tmp/term_idx_hist.txt", "w");
    int sum = 0;
    for (int n = 0; n < botcorps_term_ov_n && n < 17; ++n) {
        printf("code %u explicit %d fit 0\n", botcorps_term_ov_i[n], hit[n]);
        if (f) fprintf(f, "code %u explicit %d fit 0\n", botcorps_term_ov_i[n], hit[n]);
        sum += hit[n];
    }
    printf("other 230-248 pixels %d\n", other);
    printf("sheet-code pixels %d\n", sum);
    if (f) {
        fprintf(f, "other 230-248 pixels %d\n", other);
        fprintf(f, "sheet-code pixels %d\n", sum);
        fclose(f);
    }
}

extern void host_gate_dump(const char *path);
extern void botcorps_host_scan_n(uint8_t *buf, int bot);
extern void botcorps_host_control(uint8_t *buf, int cursor);
extern void botcorps_host_gchat(uint8_t *buf);
extern void botcorps_host_gmenu(uint8_t *buf);
extern void botcorps_host_jackin(uint8_t *buf);
extern void botcorps_host_sbox(uint8_t *buf, int band, int mode, int force_dbm);
extern void botcorps_host_found_alert_b(uint8_t *buf, int band, int dev);
extern void botcorps_host_set_color(int c);
extern void botcorps_host_enter(uint8_t *buf, int id);
extern void botcorps_host_seq(uint8_t *buf, const char *keys);
extern void botcorps_host_sheet(uint8_t *buf, int sel);
extern void botcorps_host_gc_gloss(uint8_t *buf, int bot);
extern void botcorps_host_gc_attack(uint8_t *buf, int bot);
extern void botcorps_host_gc_title(uint8_t *buf, int bot);

/* gate.sh: render every screen twice (rain on / rain off) at a fixed clock,
 * dumping <name>.rgb, <name>.norain.rgb and <name>.log (element + text bboxes). */
static uint8_t g_buf[320 * 240];
static void gate_one(const char *dir, const char *name, int term, void (*fn)(void *), void *arg)
{
    char p[256];
    for (int pass = 0; pass < 2; ++pass) {
        host_gate_norain = pass;
        host_rain_init(0);
        host_set_millis(9000u);
        fn(arg);
        snprintf(p, sizeof p, "%s/%s%s.rgb", dir, name, pass ? ".norain" : "");
        write_raw(p, g_buf, term);
        if (!pass) { snprintf(p, sizeof p, "%s/%s.log", dir, name); host_gate_dump(p); }
        /* v1017c S3: device BANDS slice paint must compose to the same frame (rain on and off) */
        extern int botcorps_host_slice_diff(void);
        const int sd = botcorps_host_slice_diff();
        snprintf(p, sizeof p, "%s/slice.txt", dir);
        FILE *sf = fopen(p, "a");
        if (sf) { fprintf(sf, "SLICE\t%s%s\t%d\n", name, pass ? ".norain" : "", sd); fclose(sf); }
    }
    host_gate_norain = 0;
    printf("rendered %s\n", name);
}
struct GArg { int a, b, c; const char *keys; };
extern void botcorps_host_paint(uint8_t *buf);
static void f_enter(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(g->b); botcorps_host_enter(g_buf, g->a); if (g->keys) botcorps_host_seq(g_buf, g->keys);
    if (g->c > 0) { host_set_millis(9000u + (uint32_t)g->c); memset(g_buf, 0, sizeof g_buf); botcorps_host_paint(g_buf); } }
extern void botcorps_host_scan_gibhunt(uint8_t *buf);
static void f_gibhunt(void *) { botcorps_host_set_color(0); botcorps_host_scan_gibhunt(g_buf); }
static void f_scan(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(0); botcorps_host_scan_n(g_buf, g->a); }
static void f_ctl(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(g->b); botcorps_host_control(g_buf, g->a); }
/* v1019m: "intro" is the LIVE intro (boot rain + Gibson chip in the square rings, SCR_RAIN, same path as
 * "boot"). It used to render paint_intro's ID board, which the device never reached (dead code, removed). */
static void f_intro(void *) { botcorps_host_set_color(0); botcorps_host_enter(g_buf, 0); }
static void f_gchat(void *) { botcorps_host_set_color(0); botcorps_host_gchat(g_buf); }
static void f_gmenu(void *) { botcorps_host_set_color(0); botcorps_host_gmenu(g_buf); }
static void f_jackin(void *) { botcorps_host_set_color(0); botcorps_host_jackin(g_buf); }
static void f_sbox(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(0); botcorps_host_sbox(g_buf, g->a, g->b, g->c); }
static void f_alert(void *) { botcorps_host_set_color(0); botcorps_host_found_alert_b(g_buf, 1, 2); }
extern void botcorps_host_defend(uint8_t *buf, int bot, int warm_ms);
static void f_defend(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(0); botcorps_host_defend(g_buf, g->a, g->b); }
extern void botcorps_host_defend_g(uint8_t *buf, int bot, int slot, int warm_ms);
extern void botcorps_host_defend_zap(uint8_t *buf, int warm_ms);
static void f_defend_g(void *v) { GArg *g = (GArg *)v; botcorps_host_set_color(0); botcorps_host_defend_g(g_buf, g->a, g->c, g->b); }
static void f_defend_zap(void *) { botcorps_host_set_color(0); botcorps_host_defend_zap(g_buf, 4500); }
static void f_sheet(void *) { botcorps_host_set_color(0); botcorps_host_enter(g_buf, 2); botcorps_host_sheet(g_buf, 0); }
static int run_gate(const char *dir)
{
    static const char *BN[13] = { "wifi", "bluetooth", "deauth", "hack", "flock", "drone", "airtag", "keyfob", "meta", "skimmer", "flipper", "samsungtag", "gibson" };
    char nm[64];
    GArg a;
    a = { 0, 0, 0, nullptr }; gate_one(dir, "intro", 0, f_intro, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "boot", 0, f_enter, &a);
    a = { 1, 0, 0, nullptr }; gate_one(dir, "hub", 0, f_enter, &a);
    a = { 1, 1, 0, nullptr }; gate_one(dir, "hub-amber", 0, f_enter, &a);
    a = { 2, 0, 0, nullptr }; gate_one(dir, "terminal", 0, f_enter, &a);
    a = { 0, 0, 0, "\n\nsca" }; gate_one(dir, "terminal-typed", 0, f_enter, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "terminal-tab", 1, f_sheet, &a);
    /* v1016n: screen ids after the home/bot-radar removal: 0 rain 1 gibson 2 terminal 3 wifiradar 4 wifilist
     * 5 btradar 6 btlist 7 scan 8 control 9 diag 10 gchat */
    a = { 3, 0, 14000, nullptr }; gate_one(dir, "scan-wifi", 0, f_enter, &a);
    a = { 5, 0, 14000, nullptr }; gate_one(dir, "scan-ble", 0, f_enter, &a);
    a = { 0, 1, 0, nullptr }; gate_one(dir, "sbox-selected", 0, f_sbox, &a);
    a = { 0, 2, 0, nullptr }; gate_one(dir, "sbox-info", 0, f_sbox, &a);
    a = { 0, 3, -50, nullptr }; gate_one(dir, "sbox-hunt", 0, f_sbox, &a);
    a = { 0, 4, 0, nullptr }; gate_one(dir, "sbox-glossary", 0, f_sbox, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "found-alert", 0, f_alert, &a);
    for (int b = 2; b <= 12; ++b) { a = { b, 0, 0, nullptr }; snprintf(nm, sizeof nm, "scanboard-%s", BN[b]); gate_one(dir, nm, 0, f_scan, &a); }
    for (int c = 0; c < 8; ++c) { a = { c, 0, 0, nullptr }; snprintf(nm, sizeof nm, "settings-c%d", c); gate_one(dir, nm, 0, f_ctl, &a); }
    a = { 0, 1, 0, nullptr }; gate_one(dir, "settings-amber", 0, f_ctl, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "game-board", 0, f_gchat, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "game-menu", 0, f_gmenu, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "game-jackin", 0, f_jackin, &a);
    a = { 9, 0, 0, nullptr }; gate_one(dir, "diag", 0, f_enter, &a);
    /* v1017a (rendered last so earlier renders keep their rain RNG order): terminal commands button
     * focused, then opened with Enter; control-panel focus on TERM (8) and crown (9). */
    /* v1017c (#3): boards -> prompt -> commands button, so the button is one Down further */
    a = { 0, 0, 0, "\n\n\x02\x02\x02" }; gate_one(dir, "terminal-prompt", 0, f_enter, &a);
    a = { 0, 0, 0, "\n\n\x02\x02\x02\x02" }; gate_one(dir, "terminal-cmdbtn", 0, f_enter, &a);
    a = { 0, 0, 0, "\n\n\x02\x02\x02\x02\n" }; gate_one(dir, "terminal-cmdmenu", 0, f_enter, &a);
    a = { 8, 0, 0, nullptr }; gate_one(dir, "ctlfocus-term", 0, f_ctl, &a);
    a = { 9, 0, 0, nullptr }; gate_one(dir, "ctlfocus-crown", 0, f_ctl, &a);
    /* v1018b (rendered after every locked screen so their rain RNG order is unchanged):
     * #10 Terminal attack-prompt button (one Down past the commands button), Enter = attack
     * prompts, Right = device glossary; #5 Gibson's board with hunt mode on (mock gibHuntStart). */
    a = { 0, 0, 0, "\n\n\x02\x02\x02\x02\x02" }; gate_one(dir, "terminal-atkbtn", 0, f_enter, &a);
    a = { 0, 0, 0, "\n\n\x02\x02\x02\x02\x02\n" }; gate_one(dir, "terminal-atkmenu", 0, f_enter, &a);
    a = { 0, 0, 0, "\n\n\x02\x02\x02\x02\x02\n\x04" }; gate_one(dir, "terminal-atkgloss", 0, f_enter, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "scanboard-hunt-gibson", 0, f_gibhunt, &a);
    /* v1019j (rendered after every earlier screen so their rain RNG order is unchanged): the four
     * Defend mini-games, each entered through enter_defend for a bot of that device category and
     * played 6 s by the host autopilot: Wi-Fi (wifibot) Rain Catch, BLE (bluetoothbot) Beacon Sort,
     * IoT (flockbot) Firewall Wall, Rogue (Gibson) Gibson Maze. */
    a = { 0, 6000, 0, nullptr }; gate_one(dir, "defend-raincatch", 0, f_defend, &a);
    a = { 1, 6000, 0, nullptr }; gate_one(dir, "defend-beaconsort", 0, f_defend, &a);
    a = { 4, 6000, 0, nullptr }; gate_one(dir, "defend-firewall", 0, f_defend, &a);
    a = { 12, 6000, 0, nullptr }; gate_one(dir, "defend-maze", 0, f_defend_g, &a);  /* slot 0 = maze */
    /* v1019m (rendered after every earlier screen so their order is unchanged): the eight added Defend
     * games, each entered through enter_defend for the same category bot, then the real Tab key to the
     * game's slot, played by the host autopilot (6 s; Evil Twin 5 s = pair mid-board, Patch Run 3.3 s =
     * a vulnerable cell with its bar, Signal Trace 1.2 s = a heat scan + the found rogue). */
    a = { 0, 5000, 1, nullptr }; gate_one(dir, "defend-eviltwin", 0, f_defend_g, &a);
    a = { 0, 6000, 2, nullptr }; gate_one(dir, "defend-channelhop", 0, f_defend_g, &a);
    a = { 1, 6000, 1, nullptr }; gate_one(dir, "defend-rangering", 0, f_defend_g, &a);
    a = { 1, 6000, 2, nullptr }; gate_one(dir, "defend-pairlock", 0, f_defend_g, &a);
    a = { 4, 3300, 1, nullptr }; gate_one(dir, "defend-patchrun", 0, f_defend_g, &a);
    a = { 4, 6000, 2, nullptr }; gate_one(dir, "defend-botnet", 0, f_defend_g, &a);
    a = { 12, 1200, 1, nullptr }; gate_one(dir, "defend-signaltrace", 0, f_defend_g, &a);
    a = { 12, 6000, 2, nullptr }; gate_one(dir, "defend-jamdodge", 0, f_defend_g, &a);
    a = { 0, 0, 0, nullptr }; gate_one(dir, "defend-zap", 0, f_defend_zap, &a);  /* v1019v last so rain L1 of earlier screens stay */
    botcorps_host_set_color(0);
    /* functional: Esc goes back on every screen; each of the 8 settings edits + writes NVS */
    {
        extern int botcorps_host_esc_test(uint8_t *, int, int *);
        extern int botcorps_host_setfn(uint8_t *, int, char *, char *, int *);
        static const char *nm[11] = { "rain", "gibson", "terminal", "wifiradar", "wifilist", "btradar", "btlist", "scan", "control", "diag", "gchat" };
        char p[256]; snprintf(p, sizeof p, "%s/functional.txt", dir);
        FILE *f = fopen(p, "w");
        if (f) fprintf(f, "SCREENS\t%d\n", 11);
        for (int id = 1; id < 11; ++id) {
            int ex = -1;
            const int got = botcorps_host_esc_test(g_buf, id, &ex);
            if (f) fprintf(f, "ESC\t%s\t%d\t%d\n", nm[id], got, ex);
        }
        /* v1016n NAV: the routes that used to open per-bot screens, then Esc down to the hub;
         * every screen id seen must be a live one (0..10) and never a removed one. */
        {
            struct { const char *name, *keys; } R[] = {
                { "go-wifi", "\n\ngo wifi\n" }, { "go-ble", "\n\ngo ble\n" }, { "go-bots", "\n\ngo bots\n" },
                { "load-wifi", "\n\nload wifi\n" }, { "load-ble", "\n\nload ble\n" }, { "load-bots", "\n\nload bots\n" },
                { "go-diag", "\n\ngo diag\n" },   /* control: proves the typed command really runs */
            };
            for (auto &r : R) {
                botcorps_host_enter(g_buf, 0);
                botcorps_host_seq(g_buf, r.keys);
                const int s0 = botcorps_host_screen_now();
                int chain[4];
                for (int k = 0; k < 4; ++k) { botcorps_host_seq(g_buf, "\x1b"); chain[k] = botcorps_host_screen_now(); }
                if (f) fprintf(f, "NAV\t%s\t%d\t%d,%d,%d,%d\n", r.name, s0, chain[0], chain[1], chain[2], chain[3]);
            }
            /* terminal Select on each of the 12 boards -> that bot's scan screen */
            extern int botcorps_host_open_bot(uint8_t *, int, int *);
            for (int b = 0; b < 12; ++b) {
                int sb = -1;
                const int got = botcorps_host_open_bot(g_buf, b, &sb);
                const int e1 = (botcorps_host_seq(g_buf, "\x1b"), botcorps_host_screen_now());
                if (f) fprintf(f, "OPEN\t%d\t%d\t%d\t%d\n", b, got, sb, e1);
            }
        }
        /* v1017a SIM: real draw loop on the band simulator; glitch = a push showing pixels that differ
         * from a full paint at the same millis (what the eye sees as a flash) */
        {
            extern int botcorps_host_sim(int, const char *, long *, long *, long *, long *);
            extern void botcorps_host_scan_n(uint8_t *, int);
            extern void botcorps_host_gchat(uint8_t *);
            struct { const char *name; int kind, arg; const char *keys; } S[] = {
                { "scan-gibson", 0, 12, nullptr }, { "scan-flock", 0, 4, nullptr }, { "scan-airtag", 0, 6, nullptr },
                { "scan-wifi", 1, 3, nullptr }, { "scan-ble", 1, 5, nullptr },
                { "game-board", 2, 0, "help\nscan\n" }, { "game-jackin", 3, 0, nullptr },  /* v1019v: zap chrome — idle stability */
                { "terminal-typing", 4, 0, "scan wifi\b\b\b\b" },
                { "gibson-game", 1, 1, "hack\b\n" },
                /* v1017c #1: the scan box in each of its animated modes (hunt meter, info scan line, glossary) */
                { "scan-sbox-hunt", 5, 3, nullptr }, { "scan-sbox-info", 5, 2, nullptr }, { "scan-sbox-gloss", 5, 4, nullptr },
                /* v1017c #5: control panel navigation (every arrow, Enter edits) on the real draw loop */
                { "control-nav", 1, 8, "\x04\x04\x04\x02\x01\x03\x04\x04\x04\x04\x02\x02\x01\x01\x03" },
                { "hub-to-control", 1, 1, "\x04\x04\n\x04\x04\n\x02\n\x01\x01\n" },
                { "control-to-hub", 1, 8, "\x1b\x04\x04" },
                /* v1019j: each Defend game on the real draw loop with live keys (moves, sorts, launch) */
                { "defend-raincatch", 6, 0, "\x03\x03\x04\x04\x04\x03\x04\x03\x03\x04\x04\x04\x03\x03\x04\x03\x04\x04\x03" },
                { "defend-beaconsort", 6, 1, "\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04" },
                { "defend-firewall", 6, 4, "\n\x03\x03\x04\x04\x04\x03\x03\x04\x03\x04\x04\x03\x03\x04\x03\x04\x03" },
                { "defend-maze", 7, 12 | 0 << 4, "\x03\x01\x04\x02\x03\x01\x04\x02\x03\x01\x04\x02\x03\x01\x04\x02\x03" },
                { "defend-zap", 8, 0, nullptr },  /* v1019v: still; live zap uses full presents */
                /* v1019m: the eight added games (arg = bot | slot << 4) with live keys, incl. a Tab game switch */
                { "defend-eviltwin", 7, 0 | 1 << 4, "\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03\x04\x03" },
                { "defend-channelhop", 7, 0 | 2 << 4, "\x03\x03\x04\x04\x04\x03\n\x04\x03\x03\x04\x04\x03\x03\x04\x03" },
                { "defend-rangering", 7, 1 | 1 << 4, "\x01\x02\x03\x04\x01\x02\x03\x04\x01\x02\x03\x04\x01\x02\x03\x04" },
                { "defend-pairlock", 7, 1 | 2 << 4, "12\b3456\n7890\n1\b2\b34\n5678\n" },
                { "defend-patchrun", 7, 4 | 1 << 4, "\x04\x02\n\x03\x01\n\x04\x04\x02\n\x03\x02\n\x01\x04\n\x03\x03\n" },
                { "defend-botnet", 7, 4 | 2 << 4, "1234512345123451234512345" },
                { "defend-signaltrace", 7, 12 | 1 << 4, "\n\x04\n\x02\n\x03\x03\n\x01\n\x04\n\x02\x02\n\x04\n" },
                { "defend-jamdodge", 7, 12 | 2 << 4, "\x01\x02\x02\x01\x01\x02\x02\x02\x01\x01\x01\x02\x02\x01\t\x01" },
            };
            for (auto &q : S) {
                host_set_millis(20000u);
                if (q.kind == 0) botcorps_host_scan_n(g_buf, q.arg);
                else if (q.kind == 1) botcorps_host_enter(g_buf, q.arg);
                else if (q.kind == 2) botcorps_host_gchat(g_buf);
                else if (q.kind == 3) { extern void botcorps_host_jackin(uint8_t *); botcorps_host_jackin(g_buf); }
                else if (q.kind == 5) botcorps_host_sbox(g_buf, 0, q.arg, q.arg == 3 ? -50 : 0);
                else if (q.kind == 6) botcorps_host_defend(g_buf, q.arg, 2000);
                else if (q.kind == 7) { extern void botcorps_host_defend_g(uint8_t *, int, int, int); botcorps_host_defend_g(g_buf, q.arg & 15, q.arg >> 4, 2000); }
                else if (q.kind == 8) { extern void botcorps_host_defend_zap(uint8_t *, int); botcorps_host_defend_zap(g_buf, 2000); }
                else botcorps_host_enter(g_buf, 2);
                long pr = 0, pu = 0, rc = 0, wo = 0;
                const int bad = botcorps_host_sim(6000, q.keys, &pr, &pu, &rc, &wo);
                if (f) fprintf(f, "SIM\t%s\t%d\t%ld\t%ld\t%ld\t%ld\n", q.name, bad, pr, pu, rc, wo);
            }
        }
        for (int c = 0; c < 8; ++c) {
            char b[32], a[32]; int nv = 0;
            const int ch = botcorps_host_setfn(g_buf, c, b, a, &nv);
            extern int botcorps_host_cp_cycle(void);
            if (f) fprintf(f, "SET\t%d\t%s\t%s\t%d\t%d\t%d\n", c, b, a, ch, nv, botcorps_host_cp_cycle());
        }
        /* v1017a CPMOVE: every control-panel stop x every arrow -> new stop; no setting may change */
        {
            extern int botcorps_host_cp_move(uint8_t *, int, int, int *);
            for (int st = 0; st < 10; ++st)
                for (int d = 1; d <= 4; ++d) {
                    int chg = 0;
                    const int to = botcorps_host_cp_move(g_buf, st, d, &chg);
                    if (f) fprintf(f, "CPMOVE\t%d\t%d\t%d\t%d\n", st, d, to, chg);
                }
            extern int botcorps_host_cmd_btn(uint8_t *, int *, int *, int *, int *, int *);
            int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0;
            botcorps_host_cmd_btn(g_buf, &r1, &r2, &r3, &r4, &r5);
            if (f) fprintf(f, "CMDBTN\t%d\t%d\t%d\t%d\t%d\n", r1, r2, r3, r4, r5);
            /* v1017c K1: real read_key on a scripted key matrix (rollover typing + Fn chord) */
            extern int botcorps_host_k1(uint8_t *, char *, int, int *);
            char kp[64] = {0};
            int kf = 0;
            const int kok = botcorps_host_k1(g_buf, kp, sizeof kp, &kf);
            if (f) fprintf(f, "K1\t%d\t%d\t%s\n", kok, kf, kp);
            extern int botcorps_host_cp_start(void);
            if (f) fprintf(f, "CPSTART\t%d\n", botcorps_host_cp_start());
        }
        /* v1019j DG: each Defend game's win and lose conditions on its real step code (host autopilot =
         * the same keys a player sends; "idle" = no keys). DGR: which Defend path launches which game,
         * Esc (backtick) quits the game to the Defend board, a second Esc goes back as before. */
        {
            extern int botcorps_host_defend_run(int, int, bool, bool, int, int *, int *, int *, int *, int *);
            extern int botcorps_host_defend_cycle(int, int *);
            extern int botcorps_host_defend_route(int, int *, int *, int *, int *);
            struct { const char *name; int bot, slot; bool ai, noghost; int max_ms; } T[] = {
                { "raincatch-play", 0, 0, true, false, 600000 }, { "raincatch-idle", 0, 0, false, false, 600000 },
                { "beaconsort-play", 1, 0, true, false, 600000 }, { "beaconsort-idle", 1, 0, false, false, 600000 },
                { "firewall-play", 4, 0, true, false, 900000 }, { "firewall-idle", 4, 0, false, false, 600000 },
                { "maze-clear", 12, 0, true, true, 900000 }, { "maze-idle", 12, 0, false, false, 600000 },
                /* v1019m: the eight added games, played (autopilot) and idle (no keys), same bots + Tab slot */
                { "eviltwin-play", 0, 1, true, false, 600000 }, { "eviltwin-idle", 0, 1, false, false, 600000 },
                { "channelhop-play", 0, 2, true, false, 600000 }, { "channelhop-idle", 0, 2, false, false, 600000 },
                { "rangering-play", 1, 1, true, false, 600000 }, { "rangering-idle", 1, 1, false, false, 600000 },
                { "pairlock-play", 1, 2, true, false, 600000 }, { "pairlock-idle", 1, 2, false, false, 600000 },
                { "patchrun-play", 4, 1, true, false, 600000 }, { "patchrun-idle", 4, 1, false, false, 600000 },
                { "botnet-play", 4, 2, true, false, 600000 }, { "botnet-idle", 4, 2, false, false, 600000 },
                { "signaltrace-play", 12, 1, true, false, 600000 }, { "signaltrace-idle", 12, 1, false, false, 600000 },
                { "jamdodge-play", 12, 2, true, false, 600000 }, { "jamdodge-idle", 12, 2, false, false, 600000 },
            };
            for (auto &q : T) {
                int gm = -1, sc = 0, mi = 0, ju = 0, ms = 0;
                const int st = botcorps_host_defend_run(q.bot, q.slot, q.ai, q.noghost, q.max_ms, &gm, &sc, &mi, &ju, &ms);
                if (f) fprintf(f, "DG\t%s\t%d\t%d\t%d\t%d\t%d\t%d\n", q.name, gm, st, sc, mi, ju, ms);
            }
            for (int w = 0; w < 7; ++w) {
                int scr = -1, e1 = -1, e1g = -1, e2 = -1;
                const int gm = botcorps_host_defend_route(w, &scr, &e1, &e1g, &e2);
                if (f) fprintf(f, "DGR\t%d\t%d\t%d\t%d\t%d\t%d\n", w, gm, scr, e1, e1g, e2);
            }
            /* v1019m DGC: Tab walks each category's three games and wraps; , / step on a LOST stop */
            static const int CB[4] = { 0, 1, 4, 12 };
            for (int c = 0; c < 4; ++c) {
                int ids[6] = { -1, -1, -1, -1, -1, -1 };
                const int cat = botcorps_host_defend_cycle(CB[c], ids);
                if (f) fprintf(f, "DGC\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", CB[c], cat, ids[0], ids[1], ids[2], ids[3], ids[4], ids[5]);
            }
        }
        if (f) fclose(f);
    }
    return 0;
}

/* mod pass 2026-10-18 (items 9/10/15): review renders, not gate items. rend mod <dir> */
extern void botcorps_host_mod_sbox(uint8_t *buf, int band, int n, int cursor, int color, uint32_t since, const char *keys);
static void mod_out(const char *dir, const char *name)
{
    char p[256];
    snprintf(p, sizeof p, "%s/%s.rgb", dir, name);
    write_raw(p, g_buf, 0);
    printf("rendered %s\n", name);
}
static int run_mod(const char *dir)
{
    struct S { const char *nm; int band, n, cur, col; uint32_t since; const char *keys; };
    static const S SB[] = {
        { "09-scan-wifi-12-halo",        0, 12, 9, 0, 9400u, nullptr },
        { "09-scan-ble-12-halo",         1, 12, 3, 0, 9400u, nullptr },
        { "09-scan-wifi-12-halo-pink",   0, 12, 4, 1, 9400u, nullptr },
        { "09-scan-wifi-35-overflow-top", 0, 35, 2, 0, 9400u, nullptr },
        { "09-scan-wifi-35-overflow-scrolled", 0, 35, 2, 0, 9400u, "\x02\x02\x02\x02\x04\x04" },
        { "09-scan-ble-30-overflow-scrolled", 1, 30, 10, 4, 9400u, "\x02\x02\x03" },
        { "09-scan-wifi-12-midsweep",    0, 12, 0, 0, 700u, nullptr },
    };
    for (const S &k : SB) {
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_mod_sbox(g_buf, k.band, k.n, k.cur, k.col, k.since, k.keys);
        mod_out(dir, k.nm);
    }
    extern void botcorps_host_mod_gibdrop(uint8_t *buf, int lane, int dev, int color);
    struct G { const char *nm; int lane, dev, col; };
    static const G GD[] = {
        { "10-gibson-dropdown-wifi-lane", 0, -1, 0 },
        { "10-gibson-dropdown-wifi-tile", 0, 9, 0 },
        { "10-gibson-dropdown-flipper-tile", 10, 3, 0 },
        { "10-gibson-dropdown-flock-lane-synth", 4, -1, 3 },
    };
    for (const G &k : GD) {
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_mod_gibdrop(g_buf, k.lane, k.dev, k.col);
        mod_out(dir, k.nm);
    }
    static const int TC[3] = { 0, 3, 1 };
    static const char *TN[3] = { "15-terminal-device-rain-grape", "15-terminal-device-rain-synth", "15-terminal-device-rain-pink" };
    for (int i = 0; i < 3; ++i) {
        host_rain_init(0);
        host_set_millis(9000u);
        GArg a = { 2, TC[i], 0, nullptr };
        f_enter(&a);
        mod_out(dir, TN[i]);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 && strcmp(argv[1], "modc") == 0) return run_mod(argv[2]);   /* Pass C review set (was "mod": clashed with Pass B mod scr/ctl) */
    if (argc > 2 && strcmp(argv[1], "gate") == 0) return run_gate(argv[2]);
    /* v1019x review (not a gate item): rend zaprain <dir> <frames> - Gibson Defend mid-game still, then the
     * rain stepped once per frame (game held); writes f<k>.rgb and f<k>.norain.rgb at the same millis. */
    if (argc > 3 && strcmp(argv[1], "zaprain") == 0) {
        extern void botcorps_host_zap_rain_step(uint8_t *buf);
        extern void botcorps_host_repaint(uint8_t *buf);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(0);
        botcorps_host_defend_zap(g_buf, 0);
        const int n = atoi(argv[3]);
        char p[256];
        for (int k = 0; k < n; ++k) {
            if (k) botcorps_host_zap_rain_step(g_buf);
            snprintf(p, sizeof p, "%s/f%d.rgb", argv[2], k); write_raw(p, g_buf, 0);
            host_gate_norain = 1; botcorps_host_repaint(g_buf);
            snprintf(p, sizeof p, "%s/f%d.norain.rgb", argv[2], k); write_raw(p, g_buf, 0);
            host_gate_norain = 0; botcorps_host_repaint(g_buf);
        }
        return 0;
    }
    /* v1017c: one-off review render (not a gate item): rend shot <screen id> <out.rgb> [keys] */
    if (argc > 3 && strcmp(argv[1], "shot") == 0) {
        host_rain_init(0);
        host_set_millis(9000u);
        GArg a = { atoi(argv[2]), getenv("RC") ? atoi(getenv("RC")) : 0, 0, argc > 4 ? argv[4] : nullptr };
        f_enter(&a);
        write_raw(argv[3], g_buf, 0);
        return 0;
    }
    /* v1019n-mod review renders (not gate items):
     *   rend mod ctl <focus> <out>    control panel        env RC theme, BG style, RM rain mode, BGM motion
     *   rend mod scr <screen> <out>   any screen by id     same env
     *   rend intflash                 #11: full INT clears while Enter cycles UI COLOR through every theme */
    if (argc > 4 && strcmp(argv[1], "mod") == 0) {
        extern void botcorps_host_set_bg(int, int);
        botcorps_host_set_bg(getenv("BG") ? atoi(getenv("BG")) : 0, getenv("BGM") ? atoi(getenv("BGM")) : 1);
        host_rain_init(getenv("RM") ? atoi(getenv("RM")) : 0);
        host_set_millis(9000u);
        const int rc = getenv("RC") ? atoi(getenv("RC")) : 0;
        if (strcmp(argv[2], "ctl") == 0) { botcorps_host_set_color(rc); botcorps_host_control(g_buf, atoi(argv[3])); }
        else { GArg a = { atoi(argv[3]), rc, 0, nullptr }; f_enter(&a); }
        write_raw(argv[4], g_buf, 0);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "intflash") == 0) {
        extern int botcorps_host_int_flash(uint8_t *, int, int *);
        extern int botcorps_host_theme_n(void);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(0);
        int ext_bad = -1;
        const int n = botcorps_host_theme_n();
        const int full = botcorps_host_int_flash(g_buf, n, &ext_bad);
        printf("INTFLASH\tpresses=%d\tint_full_clears=%d\text_glitch_pushes=%d\n", n, full, ext_bad);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "audit") == 0) {
        extern void botcorps_host_prompt_audit(void);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_prompt_audit();
        return 0;
    }
    /* v1019j: review render of a Defend game: rend dg <bot> <warm_ms> <out> [hud] (v1019m: env DG_SLOT=0-2) */
    if (argc > 4 && strcmp(argv[1], "dg") == 0) {
        extern void botcorps_host_defend_hud(uint8_t *, int, int, int);
        extern void botcorps_host_defend_g(uint8_t *, int, int, int);
        const int slot = getenv("DG_SLOT") ? atoi(getenv("DG_SLOT")) : 0;
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(0);
        if (argc > 5) botcorps_host_defend_hud(g_buf, atoi(argv[2]), atoi(argv[3]), slot);
        else botcorps_host_defend_g(g_buf, atoi(argv[2]), slot, atoi(argv[3]));
        write_raw(argv[4], g_buf, 0);
        return 0;
    }
    /* v1019p: play a Defend game from the idle board through the REAL key matrix: rend play <bot> <script> <out> */
    if (argc > 4 && strcmp(argv[1], "play") == 0) {
        extern void botcorps_host_play(uint8_t *, int, const char *);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(0);
        std::string sc;
        for (const char *q = argv[3]; *q; ++q) {
            if (q[0] == '\\' && q[1] == 'n') { sc += '\n'; ++q; }
            else if (q[0] == '\\' && q[1] == 't') { sc += '\t'; ++q; }
            else if (q[0] == '\\' && q[1] == 'b') { sc += '\b'; ++q; }
            else sc += *q;
        }
        botcorps_host_play(g_buf, atoi(argv[2]), sc.c_str());
        write_raw(argv[4], g_buf, 0);
        return 0;
    }
    /* v1019r: palette dump (index rgb565) for the theme / bot-colour picks */
    if (argc > 1 && strcmp(argv[1], "pal") == 0) {
        extern uint16_t botcorps_rgb565(uint8_t);
        for (int i = 0; i < 256; ++i) printf("%d %04X\n", i, botcorps_rgb565((uint8_t)i));
        return 0;
    }
    /* v1019r: Gibson games: rend gib <12 night|13 sweep> <warm_ms> <panel 0|3> <rounds> <out> */
    if (argc > 6 && strcmp(argv[1], "gib") == 0) {
        extern void botcorps_host_gib(uint8_t *, int, int, int, int);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(getenv("RC") ? atoi(getenv("RC")) : 0);
        botcorps_host_gib(g_buf, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        write_raw(argv[6], g_buf, 0);
        return 0;
    }
    /* v1019r: real key-matrix proof on any screen: rend kb <screen> <scan_bot|defend bot> <hunt> <script> <out> (env RC theme) */
    if (argc > 6 && strcmp(argv[1], "kb") == 0) {
        extern void botcorps_host_kb(uint8_t *, int, int, int, const char *);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(getenv("RC") ? atoi(getenv("RC")) : 0);
        std::string sc;
        for (const char *q = argv[5]; *q; ++q) {
            if (q[0] == '\\' && q[1] == 'n') { sc += '\n'; ++q; }
            else if (q[0] == '\\' && q[1] == 't') { sc += '\t'; ++q; }
            else if (q[0] == '\\' && q[1] == 'b') { sc += '\b'; ++q; }
            else sc += *q;
        }
        botcorps_host_kb(g_buf, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), sc.c_str());
        write_raw(argv[6], g_buf, 0);
        return 0;
    }
    /* v1019r: band-simulator proof with keys: rend ksim <kind 0 scan board|1 screen> <arg> <keys> <out> -> glitch pushes */
    if (argc > 5 && strcmp(argv[1], "ksim") == 0) {
        extern int botcorps_host_sim(int, const char *, long *, long *, long *, long *);
        extern void host_sim_dump(const char *);
        host_rain_init(0);
        host_set_millis(20000u);
        botcorps_host_set_color(getenv("RC") ? atoi(getenv("RC")) : 0);
        std::string sc;
        for (const char *q = argv[4]; *q; ++q) {
            if (q[0] == '\\' && q[1] == 'n') { sc += '\n'; ++q; }
            else if (q[0] == '\\' && q[1] == 'b') { sc += '\b'; ++q; }
            else sc += *q;
        }
        if (atoi(argv[2]) == 0) botcorps_host_scan_n(g_buf, atoi(argv[3])); else botcorps_host_enter(g_buf, atoi(argv[3]));
        long pr = 0, pu = 0, rc = 0, wo = 0;
        const int bad = botcorps_host_sim(6000 + (int)sc.size() * 300, sc.c_str(), &pr, &pu, &rc, &wo);
        printf("KSIM bad=%d presents=%ld pushes=%ld recomp=%ld worst=%ld\n", bad, pr, pu, rc, wo);
        extern uint8_t host_sim_panel[320 * 240];
        write_raw(argv[5], host_sim_panel, 0);
        return 0;
    }
    /* MOD review: the Defend screen + prompt arrows: rend dgp <mode> <out> */
    if (argc > 3 && strcmp(argv[1], "dgp") == 0) {
        extern void botcorps_host_defend_prompt(uint8_t *, int);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(0);
        botcorps_host_defend_prompt(g_buf, atoi(argv[2]));
        write_raw(argv[3], g_buf, 0);
        return 0;
    }
    /* v1019: one-off review render of the Defend game title for one bot: rend gtitle <bot> <out> */
    if (argc > 3 && strcmp(argv[1], "gtitle") == 0) {
        extern void botcorps_host_gc_title(uint8_t *, int);
        host_rain_init(0);
        host_set_millis(9000u);
        botcorps_host_set_color(getenv("RC") ? atoi(getenv("RC")) : 0);
        botcorps_host_gc_title(g_buf, atoi(argv[2]));
        write_raw(argv[3], g_buf, 0);
        return 0;
    }
    /* v1019: one-off review render of the control panel in a UI theme: rend ctl <focus> <theme> <out> */
    if (argc > 4 && strcmp(argv[1], "ctl") == 0) {
        host_rain_init(0);
        host_set_millis(9000u);
        GArg a = { atoi(argv[2]), atoi(argv[3]), 0, nullptr };
        f_ctl(&a);
        write_raw(argv[4], g_buf, 0);
        return 0;
    }
    host_rain_init(0); /* v1016i: warm the REAL rain so it shows in renders */
    static uint8_t term[320 * 240];
    static uint8_t hunt[320 * 240];
    static uint8_t list[320 * 240];
    static uint8_t idle[320 * 240];
    static uint8_t bus[320 * 240];
    static uint8_t amber[320 * 240];
    static uint8_t bots[320 * 240];
    host_fit_calls = 0;
    botcorps_host_terminal(term);
    const int term_lut = host_term_lut;
    printf("sheet pal_fit calls %d\n", host_fit_calls);
    printf("cmds sheet swap %d\n", botcorps_host_cmds_sheet());
    printf("int override %d span %d\n", botcorps_host_int_span(), host_rgb_span);
    printf("int full %d strip %d partial %d\n", host_int_full, host_int_strip, host_int_part);
    write_raw("/tmp/term.rgb", term, term_lut);
    hist_term(term);
    botcorps_host_idle(idle);
    const int idle_lut = host_term_lut;
    write_raw("/tmp/idle.rgb", idle, idle_lut);
    botcorps_host_hunt(hunt);
    const int hunt_lut = host_term_lut;
    write_raw("/tmp/hunt.rgb", hunt, hunt_lut);
    botcorps_host_list(list);
    const int list_lut = host_term_lut;
    write_raw("/tmp/list.rgb", list, list_lut);
    botcorps_host_bus(bus);
    const int bus_lut = host_term_lut;
    write_raw("/tmp/bus.rgb", bus, bus_lut);
    botcorps_host_amber(amber);
    const int amber_lut = host_term_lut;
    write_raw("/tmp/amber.rgb", amber, amber_lut);
    int amber_237 = 0;
    int amber_123 = 0;
    int amber_213 = 0;
    int amber_229 = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        if (amber[i] == 237) amber_237++;
        if (amber[i] == 123) amber_123++;
        if (amber[i] == 213) amber_213++;
        if (amber[i] == 229) amber_229++;
    }
    printf("amber code237 %d pal_cyan123 %d pal_amber213 %d name229 %d\n",
           amber_237, amber_123, amber_213, amber_229);
    botcorps_host_botscan(bots);
    const int bot_lut = host_term_lut;
    write_raw("/tmp/bots.rgb", bots, bot_lut);
    static uint8_t onebot[320 * 240];
    for (int b = 0; b < 12; ++b) {
        char path[64];
        snprintf(path, sizeof(path), "/tmp/bot-%02d.rgb", b);
        botcorps_host_botscan_n(onebot, b);
        write_raw(path, onebot, bot_lut);
    }
    /* Tab-open sheet rect at s_sheet_i=2: x=14..306, y=104..222.
     * Override pixels outside that rect must be 0. Frames with no
     * sheet count the whole 320x240. */
    const int frames_n = 7;
    const char *names[] = { "tab-open", "idle", "amber", "hunt", "list", "gibson", "botscan" };
    const uint8_t *frames[] = { term, idle, amber, hunt, list, bus, bots };
    const int sheet_on[] = { 1, 0, 1, 0, 0, 0, 0 };
    for (int f = 0; f < frames_n; ++f) {
        int out = 0;
        int by_code[17];
        for (int n = 0; n < 17; ++n) by_code[n] = 0;
        int minx = 320, miny = 240, maxx = 0, maxy = 0;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                const int inside = sheet_on[f] && x >= 14 && x <= 306 && y >= 104 && y <= 222;
                if (inside) continue;
                const uint8_t c = frames[f][y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n) {
                    if (c == botcorps_term_ov_i[n]) {
                        out++;
                        by_code[n]++;
                        if (x < minx) minx = x;
                        if (y < miny) miny = y;
                        if (x > maxx) maxx = x;
                        if (y > maxy) maxy = y;
                        break;
                    }
                }
            }
        }
        printf("outside-sheet %s %d", names[f], out);
        if (out) {
            printf(" box %d,%d-%d,%d", minx, miny, maxx, maxy);
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (by_code[n]) printf(" c%u=%d", botcorps_term_ov_i[n], by_code[n]);
        }
        printf("\n");
    }
    int idle_bad = 0;
    int hunt_bad = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        if (idle[i] >= 230 && idle[i] <= 248) idle_bad++;
        if (hunt[i] >= 230 && hunt[i] <= 248) hunt_bad++;
    }
    int idle13 = 0;
    int hunt13 = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        for (int n = 0; n < 13; ++n) {
            if (idle[i] == botcorps_term_ov_i[n]) idle13++;
            if (hunt[i] == botcorps_term_ov_i[n]) hunt13++;
        }
    }
    printf("idle codes 230-248 pixels %d explicit13 %d\n", idle_bad, idle13);
    printf("hunt codes 230-248 pixels %d explicit13 %d\n", hunt_bad, hunt13);
    /* One out-and-back of the 7000 ms scout period, twelve frames. */
    static uint8_t seq[320 * 240];
    for (int f = 0; f < 12; ++f) {
        host_set_millis((uint32_t)f * (7000u / 12u));
        botcorps_host_bus(seq);
        int out = 0;
        for (int i = 0; i < 320 * 240; ++i) {
            for (int n = 0; n < botcorps_term_ov_n; ++n) {
                if (seq[i] == botcorps_term_ov_i[n]) { out++; break; }
            }
        }
        printf("outside-sheet gibson-f%02d %d\n", f, out);
        char path[64];
        snprintf(path, sizeof(path), "/tmp/bus-f%02d.rgb", f);
        write_raw(path, seq, 0);
    }
    host_set_millis(2500);
    /* Sheet selection through paint_ext. Stepped arrows versus a clean frame. */
    static uint8_t sheet[320 * 240];
    static uint8_t clean[320 * 240];
    static uint8_t stepped[320 * 240];
    botcorps_host_sheet(clean, 0);
    memcpy(stepped, clean, sizeof(stepped));
    const int keys[] = { 2, 2, 2, 1, 1, 1 };
    const int expect[] = { 1, 2, 3, 2, 1, 0 };
    for (int i = 0; i < 6; ++i) {
        botcorps_host_sheet_key(stepped, keys[i]);
        botcorps_host_sheet(clean, expect[i]);
        int diff = 0;
        for (int p = 0; p < 320 * 240; ++p) if (stepped[p] != clean[p]) diff++;
        printf("sheet-step %d sel %d diff %d\n", i, expect[i], diff);
        int out = 0;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                const uint8_t c = stepped[y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n)
                    if (c == botcorps_term_ov_i[n]) { out++; break; }
            }
        }
        printf("outside-sheet step-sel%d %d\n", expect[i], out);
        if (expect[i] == 3) write_raw("/tmp/sheet-step3.rgb", stepped, 1);
    }
    write_raw("/tmp/sheet-step.rgb", stepped, 1);
    botcorps_host_sheet(sheet, 0);
    write_raw("/tmp/sheet0.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 1);
    write_raw("/tmp/sheet1.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 2);
    write_raw("/tmp/sheet2.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 3);
    write_raw("/tmp/sheet3.rgb", sheet, 1);
    /* Probe the sheet the device path just painted. */
    botcorps_host_sheet(sheet, 0);
    const int probes[][2] = {
        { 14, 104 }, { 15, 104 }, { 14, 105 }, { 16, 104 }, { 14, 106 },
        { 306, 104 }, { 305, 104 }, { 306, 105 }, { 304, 104 }, { 306, 106 },
        { 14, 222 }, { 15, 222 }, { 14, 221 }, { 16, 222 }, { 14, 220 },
        { 306, 222 }, { 305, 222 }, { 306, 221 }, { 304, 222 }, { 306, 220 },
        { 62, 114 }, { 100, 114 }, { 293, 114 }, { 100, 115 }, { 100, 116 }, { 100, 117 },
        { 21, 220 }, { 150, 220 }, { 301, 220 },
        { 17, 120 }, { 18, 120 }, { 40, 120 }, { 17, 121 }, { 40, 121 },
        { 30, 110 }, { 22, 114 }, { 22, 115 }
    };
    printf("probe sel0");
    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
        printf(" %d,%d=%u", probes[i][0], probes[i][1], sheet[probes[i][1] * 320 + probes[i][0]]);
    printf("\n");
    botcorps_host_sheet(sheet, 1);
    printf("probe sel1");
    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
        printf(" %d,%d=%u", probes[i][0], probes[i][1], sheet[probes[i][1] * 320 + probes[i][0]]);
    printf("\n");
    int out0 = 0;
    int out1 = 0;
    botcorps_host_sheet(sheet, 0);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out0++; break; }
        }
    botcorps_host_sheet(sheet, 1);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out1++; break; }
        }
    printf("outside-sheet row0 %d row1 %d\n", out0, out1);
    botcorps_host_sheet(sheet, 0);
    static const uint8_t corn[6][6] = {
        { 235, 230, 230, 230, 230, 230 },
        { 230, 236, 236, 235, 235, 235 },
        { 230, 236, 236, 232, 232, 232 },
        { 230, 235, 232, 234, 234, 234 },
        { 230, 235, 232, 234, 236, 236 },
        { 230, 235, 232, 234, 236, 236 },
    };
    int corn_bad = 0;
    const int oxs[4] = { 14, 306, 14, 306 };
    const int oys[4] = { 104, 104, 222, 222 };
    const int sxs[4] = { 1, -1, 1, -1 };
    const int sys[4] = { 1, 1, -1, -1 };
    for (int k = 0; k < 4; ++k) {
        for (int dy = 0; dy < 6; ++dy) {
            for (int dx = 0; dx < 6; ++dx) {
                const int x = oxs[k] + sxs[k] * dx;
                const int y = oys[k] + sys[k] * dy;
                const uint8_t got = sheet[y * 320 + x];
                if (x >= 17 && x <= 49 && y >= 105 && y <= 121) continue;
                if (got != corn[dy][dx]) {
                    corn_bad++;
                    if (corn_bad <= 16) printf(" corner %d,%d got %u want %u\n", x, y, got, corn[dy][dx]);
                }
            }
        }
    }
    printf("corners-bad %d\n", corn_bad);
    int y105_236 = 0;
    int y107_234 = 0;
    for (int x = 17; x <= 49; ++x) {
        if (sheet[105 * 320 + x] == 236) y105_236++;
        if (sheet[107 * 320 + x] == 234) y107_234++;
    }
    printf("glow y105-236 %d y107-234 %d\n", y105_236, y107_234);
    printf("bottom");
    printf(" y222=%u", sheet[222 * 320 + 100]);
    printf(" y221=%u", sheet[221 * 320 + 100]);
    printf(" y220=%u", sheet[220 * 320 + 100]);
    printf(" y219=%u", sheet[219 * 320 + 100]);
    printf(" y218h=%u", sheet[218 * 320 + 100]);
    printf(" y218e=%u", sheet[218 * 320 + 18]);
    printf(" y218g=%u", sheet[218 * 320 + 300]);
    printf(" y217=%u\n", sheet[217 * 320 + 100]);
    int n127 = 0;
    for (int y = 120; y <= 132; ++y)
        for (int x = 17; x <= 302; ++x)
            if (sheet[y * 320 + x] == 127) n127++;
    printf("sel-text-127 %d\n", n127);
    printf(" prompt");
    printf(" %d,%d=%u", 9, 227, sheet[227 * 320 + 9]);
    printf(" %d,%d=%u", 8, 227, sheet[227 * 320 + 8]);
    printf(" %d,%d=%u", 9, 228, sheet[228 * 320 + 9]);
    int halo232 = 0;
    for (int y = 115; y <= 117; ++y)
        for (int x = 62; x <= 293; ++x)
            if (sheet[y * 320 + x] == 232) halo232++;
    printf(" rule-halo232 %d\n", halo232);
    static uint8_t sheetb[320 * 240];
    botcorps_host_sheet(sheetb, 1);
    int glyph_diff = 0;
    int glow236 = 0;
    int n194 = 0;
    int n5 = 0;
    for (int y = 108; y <= 118; ++y) {
        for (int x = 18; x <= 52; ++x) {
            const uint8_t a = sheet[y * 320 + x];
            const uint8_t b = sheetb[y * 320 + x];
            if ((a == 230 || b == 230) && a != b) {
                glyph_diff++;
                if (glyph_diff <= 12) printf(" glyph %d,%d %u->%u\n", x, y, a, b);
            }
            if (b == 236) glow236++;
        }
    }
    for (int y = 220; y <= 236; ++y)
        for (int x = 0; x <= 120; ++x) {
            if (sheet[y * 320 + x] == 194) n194++;
            if (sheet[y * 320 + x] == 5) n5++;
        }
    printf("core-glyph-diff %d unselected-236 %d prompt194 %d glow5 %d\n",
           glyph_diff, glow236, n194, n5);
    botcorps_host_idle(idle);
    int tile_diff = 0;
    for (int y = 69; y <= 103; ++y)
        for (int x = 7; x <= 78; ++x)
            if (sheet[y * 320 + x] != idle[y * 320 + x]) tile_diff++;
    int idle170 = 0;
    for (int x = 4; x <= 315; ++x) if (idle[167 * 320 + x] == 170) idle170++;
    printf("tile-outside-diff %d idle-170 %d\n", tile_diff, idle170);
    int tab_out = 0;
    botcorps_host_sheet(sheet, 0);
    botcorps_host_idle(idle);
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            if (sheet[y * 320 + x] != idle[y * 320 + x]) {
                tab_out++;
                if (tab_out <= 8) printf(" tab-out %d,%d %u %u\n", x, y, sheet[y * 320 + x], idle[y * 320 + x]);
            }
        }
    }
    printf("tab-open-vs-closed-outside %d\n", tab_out);
    /* Signal docks. Shift rotates the existing bar triple; paint_ext still draws the scan. */
    botcorps_host_sig(0);
    host_set_millis(2500);
    botcorps_host_bus(bus);
    write_raw("/tmp/bus-sig0.rgb", bus, 0);
    const int docks[][2] = {
        { 101, 77 }, { 65, 117 }, { 47, 167 }, { 219, 77 },
        { 140, 55 }, { 160, 55 }, { 180, 55 },
        { 140, 180 }, { 160, 180 }, { 180, 180 }
    };
    printf("docks");
    for (unsigned i = 0; i < sizeof(docks) / sizeof(docks[0]); ++i) {
        const int x = docks[i][0];
        const int y = docks[i][1];
        printf(" %d,%d edge=%u", x, y, bus[y * 320 + (x - 7)]);
    }
    printf("\n");
    printf("old-edge");
    const int oldd[][2] = { { 53, 67 }, { 53, 117 }, { 53, 167 }, { 267, 67 } };
    for (unsigned i = 0; i < 4; ++i)
        printf(" %d,%d=%u", oldd[i][0], oldd[i][1], bus[oldd[i][1] * 320 + (oldd[i][0] - 7)]);
    printf("\n");
    botcorps_host_sig(1);
    botcorps_host_bus(bus);
    write_raw("/tmp/bus-sig1.rgb", bus, 0);
    printf("sig1-edge 65,67=%u 101,77=%u 47,167=%u 140,55=%u\n",
           bus[67 * 320 + (65 - 7)], bus[77 * 320 + (101 - 7)],
           bus[167 * 320 + (47 - 7)], bus[55 * 320 + (140 - 7)]);
    botcorps_host_sig(0);
    /* sel 0 (top 0), sel 20 (top 19), sel 39 (top 35). */
    {
        static const uint8_t grid[6][6] = {
            { 235, 230, 230, 230, 230, 230 },
            { 230, 236, 236, 235, 235, 235 },
            { 230, 236, 236, 232, 232, 232 },
            { 230, 235, 232, 234, 234, 234 },
            { 230, 235, 232, 234, 236, 236 },
            { 230, 235, 232, 234, 236, 236 },
        };
        const int sels[] = { 0, 20, 39 };
        const char *paths[] = { "/tmp/sheet-s0.rgb", "/tmp/sheet-s20.rgb", "/tmp/sheet-s39.rgb" };
        for (int si = 0; si < 3; ++si) {
            botcorps_host_sheet(sheet, sels[si]);
            write_raw(paths[si], sheet, 1);
            int bad = 0;
            printf("dump sel %d\n", sels[si]);
            printf(" TL");
            for (int dy = 0; dy < 6; ++dy) {
                for (int dx = 0; dx < 6; ++dx) {
                    const uint8_t g = sheet[(104 + dy) * 320 + (14 + dx)];
                    printf(" %u", g);
                    if (g != grid[dy][dx]) bad++;
                }
            }
            printf("\n grid-bad %d\n", bad);
            printf(" top");
            for (int y = 104; y <= 109; ++y) {
                printf(" y%d", y);
                for (int x = 14; x <= 25; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n");
            int hair = 0;
            for (int x = 20; x <= 299; ++x) if (sheet[218 * 320 + x] != 231) hair++;
            printf(" hair-not-231 %d y220=%u text127", hair, sheet[220 * 320 + 100]);
            int n127 = 0;
            for (int i = 0; i < 320 * 240; ++i) if (sheet[i] == 127) n127++;
            printf(" %d\n", n127);
            printf(" y216");
            for (int x = 14; x <= 19; ++x) printf(" %u", sheet[216 * 320 + x]);
            printf(" |");
            for (int x = 301; x <= 306; ++x) printf(" %u", sheet[216 * 320 + x]);
            printf("\n y217");
            for (int x = 14; x <= 22; ++x) printf(" %u", sheet[217 * 320 + x]);
            printf(" mid%u", sheet[217 * 320 + 160]);
            printf(" |");
            for (int x = 300; x <= 306; ++x) printf(" %u", sheet[217 * 320 + x]);
            printf("\n");
            printf(" up-arrow");
            for (int y = 110; y <= 117; ++y) {
                printf(" y%d", y);
                for (int x = 293; x <= 306; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n dn-arrow");
            for (int y = 208; y <= 215; ++y) {
                printf(" y%d", y);
                for (int x = 293; x <= 306; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n");
            int out = 0;
            for (int y = 0; y < 240; ++y)
                for (int x = 0; x < 320; ++x) {
                    if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                    for (int n = 0; n < botcorps_term_ov_n; ++n)
                        if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out++; break; }
                }
            printf(" outside %d\n", out);
        }
    }
    botcorps_host_sig(0);
    botcorps_host_bus_ble(bus);
    write_raw("/tmp/bus-ble.rgb", bus, 0);
    printf("ble-edge 83,67=%u 101,77=%u 65,117=%u 47,167=%u 140,55=%u\n",
           bus[67 * 320 + (83 - 7)], bus[77 * 320 + (101 - 7)],
           bus[117 * 320 + (65 - 7)], bus[167 * 320 + (47 - 7)],
           bus[55 * 320 + (140 - 7)]);
    static uint8_t hit[320 * 240];
    botcorps_host_gchat(hit);
    write_raw("/tmp/hit-gchat.rgb", hit, 0);
    botcorps_host_gmenu(hit);
    write_raw("/tmp/hit-gmenu.rgb", hit, 0);
    botcorps_host_jackin(hit);
    write_raw("/tmp/hit-jackin.rgb", hit, 0);
    botcorps_host_scanall(hit);
    write_raw("/tmp/hit-scanall.rgb", hit, 1);
    {
        const int rings[3][3] = {
            { 14, 167, 230 }, { 15, 168, 235 }, { 16, 169, 232 }
        };
        int ring_ok = 0;
        for (int i = 0; i < 3; ++i)
            if (hit[rings[i][1] * 320 + rings[i][0]] == rings[i][2]) ring_ok++;
        const int bot = hit[222 * 320 + 14];
        int prompt_glow = 0;
        int salmon = 0;
        int hold = 0;
        for (int y = 223; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                const uint8_t c = hit[y * 320 + x];
                if (c == 230 || c == 235 || c == 232 || c == 234 || c == 236) prompt_glow++;
            }
        for (int y = 167; y <= 222; ++y) {
            const uint8_t l = hit[y * 320 + 14];
            const uint8_t r = hit[y * 320 + 306];
            if (l == 205 || r == 205) hold++;
            if (l == 238 || r == 238) salmon++;
        }
        for (int x = 14; x <= 306; ++x) {
            const uint8_t t = hit[167 * 320 + x];
            const uint8_t b = hit[222 * 320 + x];
            if (t == 205 || b == 205) hold++;
            if (t == 238 || b == 238) salmon++;
        }
        int ov = 0;
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                const uint8_t c = hit[y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n)
                    if (c == botcorps_term_ov_i[n]) { ov++; break; }
            }
        printf("scan-term-m rings %d/3 bottom %u prompt-glow %d hold %d bfb72 %d ov-out %d\n",
               ring_ok, bot, prompt_glow, hold, salmon, ov);
    }
    static uint8_t sweep_closed[320 * 240];
    memcpy(sweep_closed, hit, sizeof(sweep_closed));
    botcorps_host_scanall_sheet(hit);
    write_raw("/tmp/hit-scanall-tab.rgb", hit, 1);
    {
        int frame = 0;
        int minx = 320, miny = 240, maxx = 0, maxy = 0;
        for (int y = 160; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                const uint8_t c = hit[y * 320 + x];
                const int term = c == 230 || c == 231 || c == 232 || c == 234 || c == 235 || c == 236;
                if (!term) continue;
                const int in_sheet = x >= 14 && x <= 306 && y >= 104 && y <= 222;
                if (in_sheet) continue;
                frame++;
                if (x < minx) minx = x;
                if (y < miny) miny = y;
                if (x > maxx) maxx = x;
                if (y > maxy) maxy = y;
            }
        printf("tab-covers-log frame-outside %d", frame);
        if (frame) printf(" box %d,%d-%d,%d", minx, miny, maxx, maxy);
        printf(" sheet-edge %u\n", hit[167 * 320 + 14]);
    }
    {
        int diff = 0;
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                if (sweep_closed[y * 320 + x] != hit[y * 320 + x]) {
                    if (diff < 6)
                        printf(" sweep-out %d,%d %u->%u\n", x, y,
                               sweep_closed[y * 320 + x], hit[y * 320 + x]);
                    diff++;
                }
            }
        printf("sweep-tab-outside %d\n", diff);
    }
    botcorps_host_stale_sweep(hit);
    {
        int stale170 = 0;
        int frame = 0;
        for (int x = 4; x <= 315; ++x) if (hit[167 * 320 + x] == 170) stale170++;
        for (int i = 0; i < 320 * 240; ++i)
            if (hit[i] == 230 || hit[i] == 235 || hit[i] == 232) frame++;
        printf("stale-170 %d stale-frame %d\n", stale170, frame);
        write_raw("/tmp/hit-stale.rgb", hit, 0);
    }
    botcorps_host_scanall(hit);
    write_raw("/tmp/hit-scanall.rgb", hit, 1);
    botcorps_host_keyfob(hit);
    write_raw("/tmp/hit-keyfob.rgb", hit, 0);
    botcorps_host_hub(hit);
    write_raw("/tmp/hit-hub.rgb", hit, 0);
    botcorps_host_sheet(hit, 0);
    printf("core-col");
    for (int y = 105; y <= 109; ++y) printf(" %u", hit[y * 320 + 30]);
    printf(" ramp-mid");
    for (int y = 104; y <= 109; ++y) printf(" %u", hit[y * 320 + 160]);
    printf(" halo %u %u %u\n", hit[110 * 320 + 160], hit[140 * 320 + 20], hit[216 * 320 + 160]);
    write_raw("/tmp/hit-tab0.rgb", hit, 1);
    botcorps_host_sheet(hit, 20);
    write_raw("/tmp/hit-tab20.rgb", hit, 1);
    printf("y109-sel20 %u %u %u %u\n",
           hit[109 * 320 + 20], hit[109 * 320 + 21],
           hit[109 * 320 + 299], hit[109 * 320 + 300]);
    botcorps_host_sheet(hit, 39);
    write_raw("/tmp/hit-tab39.rgb", hit, 1);
    printf("y109-sel39 %u %u %u %u\n",
           hit[109 * 320 + 20], hit[109 * 320 + 21],
           hit[109 * 320 + 299], hit[109 * 320 + 300]);
    botcorps_host_idle(hit);
    write_raw("/tmp/hit-idle.rgb", hit, 0);

    /* Master v1016 screens: wifi/ble lists, actions, hunt states, found-alert, hub, scan tint */
    botcorps_host_list(hit);
    write_raw("/tmp/adv-wifi-list.rgb", hit, 0);
    botcorps_host_ble_list(hit);
    write_raw("/tmp/adv-ble-list.rgb", hit, 0);
    botcorps_host_hunt(hit);
    write_raw("/tmp/adv-action-list.rgb", hit, 0);
    botcorps_host_hunt_view(hit, -62);
    write_raw("/tmp/adv-hunt.rgb", hit, 0);
    botcorps_host_hunt_view(hit, -50);
    write_raw("/tmp/adv-hunt-close.rgb", hit, 0);
    botcorps_host_hunt_view(hit, -95);
    write_raw("/tmp/adv-hunt-lost.rgb", hit, 0);
    botcorps_host_found_alert(hit);
    write_raw("/tmp/adv-found-alert.rgb", hit, 0);
    botcorps_host_hub(hit);
    write_raw("/tmp/adv-gibson-hub.rgb", hit, 0);
    botcorps_host_scan_tint(hit, 0);
    write_raw("/tmp/adv-scan-tint-cyan.rgb", hit, 0);
    botcorps_host_scan_tint(hit, 2);
    write_raw("/tmp/adv-scan-tint-mint.rgb", hit, 0);
    /* v1016 scan box (icons, selected+prompt, info, hunt in box, glossary, BLE) + found alert */
    {   /* v1016f: real device entry path */
        static const char *const nm[11] = { "rain", "gibson", "terminal", "wifiradar", "wifilist", "btradar", "btlist", "scan", "control", "diag", "gchat" };
        for (int id = 0; id < 11; ++id) {
            botcorps_host_enter(hit, id);
            char fn[64];
            snprintf(fn, sizeof(fn), "/tmp/dev-%02d-%s.rgb", id, nm[id]);
            write_raw(fn, hit, 0);
        }
    }
    {   /* v1016f: key paths from boot (SCR_RAIN), through the real dispatcher */
        struct { const char *name, *keys; } P[] = {
            { "p00-boot", "" },
            { "p01-enter", "\n" },
            { "p02-hub-scan", "\n\x04\n" },
            { "p03-hub-term", "\n\n" },
            { "p04-term-typed", "\n\nsca" },
            { "p05-term-fn-down", "\n\nsca\x02\x02" },
            { "p06-term-crown", "\n\n\x01" },
            { "p07-crown-ok", "\n\n\x01\n" },
            { "p08-term-open-bot", "\n\n\x04\x04\n" },
            { "p09-term-open-wifi", "\n\n\n" },
            { "p10-gp-defend", "\ndefend\n" },
            { "p11-gp-scan", "\nscan\n" },
            { "p12-gp-ble", "\nble\n" },
            { "p13-term-esc-clear", "\n\nsca\x1b" },
            { "p14-term-open-gibson", "\n\n\x1b\x04\n" },
        };
        for (auto &q : P) {
            botcorps_host_enter(hit, 0);
            botcorps_host_seq(hit, q.keys);
            char fn[64];
            snprintf(fn, sizeof(fn), "/tmp/%s.rgb", q.name);
            write_raw(fn, hit, 0);
            printf("%s screen=%d\n", q.name, botcorps_host_screen_now());
        }
    }
    {   /* v1016f-2: scan board follows the UI colour (accent) across accents */
        const char *an[3] = { "cyan", "amber", "mint" };
        for (int ci = 0; ci < 3; ++ci) {
            botcorps_host_set_color(ci);
            botcorps_host_enter(hit, 0);            /* rain */
            botcorps_host_seq(hit, "\n\x04\n");  /* hub -> scan */
            char fn[64];
            snprintf(fn, sizeof(fn), "/tmp/acc-scan-%s.rgb", an[ci]);
            write_raw(fn, hit, 0);
            botcorps_host_enter(hit, 1);            /* hub in this accent */
            snprintf(fn, sizeof(fn), "/tmp/acc-hub-%s.rgb", an[ci]);
            write_raw(fn, hit, 0);
        }
        botcorps_host_set_color(0);
    }
    {   /* v1016f-2 (Dirt 7:43): board settings via the REAL dispatcher (hub gibson> control) */
        const char *an[2] = { "cyan", "amber" };
        for (int ci = 0; ci < 2; ++ci) {
            botcorps_host_set_color(ci);
            botcorps_host_enter(hit, 0);
            botcorps_host_seq(hit, "\ncontrol\n");
            char fn[64];
            snprintf(fn, sizeof(fn), "/tmp/ctl-%s.rgb", an[ci]);
            write_raw(fn, hit, 0);
            printf("ctl-%s screen=%d\n", an[ci], botcorps_host_screen_now());
        }
        botcorps_host_set_color(0);
    }
    {   /* v1016h: harness overlap check — no two settings part bboxes may intersect */
        botcorps_host_set_color(0);                 /* render a fresh cyan control for the checks */
        botcorps_host_enter(hit, 0);
        botcorps_host_seq(hit, "\ncontrol\n");
        const int ov = botcorps_cp_overlap_count();
        printf("CP_OVERLAP %d %s\n", ov, ov == 0 ? "OK" : "FAIL");
        const int eo = botcorps_cp_edge_overflow_count();
        printf("CP_EDGE_OVERFLOW %d %s\n", eo, eo == 0 ? "OK" : "FAIL");
        const int si = botcorps_cp_sprite_inside(hit);
        printf("CP_SPRITE_INSIDE %d %s\n", si, si == 0 ? "OK" : "FAIL");
    }
    botcorps_host_sbox(hit, 0, 0, 0); write_raw("/tmp/sb-wifi-icons.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 1, 0); write_raw("/tmp/sb-selected-prompt.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 2, 0); write_raw("/tmp/sb-info.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 3, -50); write_raw("/tmp/sb-hunt.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 3, -95); write_raw("/tmp/sb-hunt-lost.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 4, 0); write_raw("/tmp/sb-glossary.rgb", hit, 0);
    botcorps_host_sbox(hit, 1, 0, 0); write_raw("/tmp/sb-ble.rgb", hit, 0);
    botcorps_host_sbox(hit, 0, 5, 0); write_raw("/tmp/sb-wifi-sweep.rgb", hit, 0);
    botcorps_host_found_alert_b(hit, 1, 2); write_raw("/tmp/sb-found-alert.rgb", hit, 0);
    /* #9/#10/#11: Defend device glossary, attack-prompt menu, and dynamic per-bot titles */
    botcorps_host_gc_gloss(hit, 12); write_raw("/tmp/gc-gloss.rgb", hit, 0);
    botcorps_host_gc_attack(hit, 12); write_raw("/tmp/gc-attack.rgb", hit, 0);
    botcorps_host_gc_title(hit, 12); write_raw("/tmp/gc-title-gibson.rgb", hit, 0);
    botcorps_host_gc_title(hit, 4); write_raw("/tmp/gc-title-flock.rgb", hit, 0);
    botcorps_host_gc_title(hit, 6); write_raw("/tmp/gc-title-airtag.rgb", hit, 0);
    botcorps_host_gc_title(hit, 11); write_raw("/tmp/gc-title-samsungtag.rgb", hit, 0);
    botcorps_host_gc_title(hit, 1); write_raw("/tmp/gc-title-bluetooth.rgb", hit, 0);
    /* v1018b #10: Terminal attack-prompt button + the overlays it pulls up; #5 Gibson hunt mode */
    { extern void botcorps_host_term_ovl(uint8_t *, int); extern void botcorps_host_scan_gibhunt(uint8_t *);
      botcorps_host_term_ovl(hit, 0); write_raw("/tmp/term-atk-btn.rgb", hit, 0);
      botcorps_host_term_ovl(hit, 2); write_raw("/tmp/term-atk-menu.rgb", hit, 0);
      botcorps_host_term_ovl(hit, 1); write_raw("/tmp/term-atk-gloss.rgb", hit, 0);
      botcorps_host_scan_gibhunt(hit); write_raw("/tmp/bh-gibson-hunt.rgb", hit, 0); }
    return 0;
}
