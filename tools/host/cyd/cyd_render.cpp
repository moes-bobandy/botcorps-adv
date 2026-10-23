/* CYD-only host render modes. Built only by tools/host/build_cyd.sh
 * (BOTCORPS_HOST_RENDER + BOTCORPS_CYD). Returns -1 when argv is not a CYD mode. */
#include "host_shim.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern void write_raw(const char *path, const uint8_t *idx, int term);
extern void botcorps_host_cyd_osk(uint8_t *buf);
extern void botcorps_host_control_p(uint8_t *buf, int cursor, int layout);
extern void botcorps_host_portrait(uint8_t *buf, int screen, int pt_mode, int pt_sym);
extern void host_rain_init(int mode);
extern void host_set_millis(uint32_t t);
extern void botcorps_host_set_color(int c);
extern void botcorps_rain_set_view(int w, int h);
extern void botcorps_rain_set_mix(int);
extern void botcorps_rain_set_shape(int);
extern void botcorps_rain_set_bg(int);
extern void botcorps_rain_set_mode(int);
extern int host_fw, host_fh;
extern uint8_t botcorps_cyd_orient;
extern int host_gate_norain;
extern void botcorps_ui_draw(void);
extern void ui_dispatch(int key, int ch);
extern void botcorps_band_force_direct(bool on);

enum { K_OK = 10 }; /* match ui key if different — host uses '\n' path via sim; we call nudge via setfn */

static uint8_t s_cyd_buf[320 * 240];

static void render_ctlp(const char *path, int layout, int cursor, int norain)
{
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    botcorps_rain_set_view(240, 320);
    botcorps_band_force_direct(true);
    host_rain_init(0);
    host_set_millis(getenv("MS") ? (uint32_t)atoi(getenv("MS")) : 9000u);
    botcorps_host_set_color(0);
    host_gate_norain = norain ? 1 : 0;
    memset(s_cyd_buf, 0, sizeof s_cyd_buf);
    botcorps_host_control_p(s_cyd_buf, cursor, layout);
    write_raw(path, s_cyd_buf, 0);
    host_gate_norain = 0;
}

extern void botcorps_host_ctlbot_p(uint8_t *buf, int cursor);
extern int botcorps_host_ctlbot_tap(int tx, int ty);
extern int botcorps_host_ctlbot_box(int i, int *x, int *y, int *w, int *h);
extern void botcorps_host_ctlbot_board(uint8_t *buf, uint8_t fill);
extern void botcorps_host_cp_text(int i, char *b, int n);
extern int botcorps_host_screen(void);
extern int botcorps_host_cursor(void);
extern int botcorps_host_take_dirty(void);
extern int botcorps_host_focus_pend(void);
extern int botcorps_host_flush_focus(void);
extern void botcorps_host_set_bg(int style, int motion);
extern int botcorps_rain_mode(void);

/* firmware portrait Control (controlbot), set up like the mock's rend ctlbot */
static void ctlbot_setup(int cursor)
{
    host_fw = 240; host_fh = 320;
    botcorps_cyd_orient = 1;
    botcorps_rain_set_view(240, 320);
    host_rain_init(getenv("RM") ? atoi(getenv("RM")) : 0);
    if (getenv("RM")) botcorps_rain_set_mode(atoi(getenv("RM")));
    host_set_millis(getenv("MS") ? (uint32_t)atoi(getenv("MS")) : 9000u);
    botcorps_host_set_color(getenv("RC") ? atoi(getenv("RC")) : 0);
    memset(s_cyd_buf, 0, sizeof s_cyd_buf);
    botcorps_host_ctlbot_p(s_cyd_buf, cursor);
}

int cyd_render_main(int argc, char **argv)
{
    /* rend ctlbot <out.rgb> <cursor>   env RC=theme RM=rain mode MS=millis RAIN_WARM=steps */
    if (argc > 3 && strcmp(argv[1], "ctlbot") == 0) {
        ctlbot_setup(atoi(argv[3]));
        write_raw(argv[2], s_cyd_buf, 0);
        return 0;
    }
    /* rend ctlbot-mask <out.bin> [cursor] : 240x320 bytes, 1 = the board paints this pixel (two fills differ = 0) */
    if (argc > 2 && strcmp(argv[1], "ctlbot-mask") == 0) {
        ctlbot_setup(argc > 3 ? atoi(argv[3]) : 0);
        static uint8_t a[240 * 320], b[240 * 320];
        botcorps_host_ctlbot_board(a, 0xFE);
        botcorps_host_ctlbot_board(b, 0xFD);
        for (int i = 0; i < 240 * 320; ++i) a[i] = a[i] == b[i] ? 1 : 0;
        FILE *f = fopen(argv[2], "wb"); fwrite(a, 1, sizeof a, f); fclose(f);
        return 0;
    }
    /* rend ctlbot-boxes : every hit box (index x y w h), 0..8 parts, 9 >_, 10 crown, 11 BOOT, 12 EN */
    if (argc > 1 && strcmp(argv[1], "ctlbot-boxes") == 0) {
        ctlbot_setup(0);
        for (int i = 0; i < 13; ++i) {
            int x, y, w, h;
            if (botcorps_host_ctlbot_box(i, &x, &y, &w, &h)) printf("BOX %d %d %d %d %d\n", i, x, y, w, h);
        }
        return 0;
    }
    /* rend ctlbot-tap <box> [n] : fresh Control, then n (default 2) taps at the centre of hit box <box>;
     * prints the cursor / screen / orient and every Control value before and after. */
    if (argc > 2 && strcmp(argv[1], "ctlbot-tap") == 0) {
        ctlbot_setup(getenv("TC") ? atoi(getenv("TC")) : 0);
        const int bi = atoi(argv[2]), n = argc > 3 ? atoi(argv[3]) : 2;
        int x, y, w, h;
        if (!botcorps_host_ctlbot_box(bi, &x, &y, &w, &h)) { printf("no box %d\n", bi); return 1; }
        const int tx = x + w / 2, ty = y + h / 2;
        char before[9][24], after[24];
        for (int i = 0; i < 9; ++i) botcorps_host_cp_text(i, before[i], 24);
        const int s0 = botcorps_host_screen();
        printf("TAP box=%d at (%d,%d) start cursor=%d screen=%d orient=%d\n", bi, tx, ty, botcorps_host_cursor(), s0, botcorps_cyd_orient);
        for (int t = 0; t < n; ++t) {
            botcorps_host_take_dirty();
            const int k = botcorps_host_ctlbot_tap(tx, ty);
            const int fp = botcorps_host_focus_pend();
            const int d = botcorps_host_take_dirty();
            printf("  tap%d key=%d cursor=%d screen=%d orient=%d dirty=%d focus_pend=%d", t + 1, k, botcorps_host_cursor(),
                   botcorps_host_screen(), botcorps_cyd_orient, d, fp);
            if (fp) printf(" flush->dirty=%d", botcorps_host_flush_focus());
            printf("\n");
        }
        int nch = 0;
        for (int i = 0; i < 9; ++i) {
            botcorps_host_cp_text(i, after, 24);
            if (strcmp(after, before[i])) { printf("  CHANGED row%d: %s -> %s\n", i, before[i], after); ++nch; }
        }
        printf("  changed_rows=%d screen %d->%d\n", nch, s0, botcorps_host_screen());
        return 0;
    }
    /* rend osk <out.rgb> — Terminal with the [abc] on-screen keyboard open. */
    if (argc > 2 && strcmp(argv[1], "osk") == 0) {
        host_rain_init(0);
        host_set_millis(getenv("MS") ? (uint32_t)atoi(getenv("MS")) : 9000u);
        botcorps_host_set_color(0);
        memset(s_cyd_buf, 0, sizeof s_cyd_buf);
        botcorps_host_cyd_osk(s_cyd_buf);
        write_raw(argv[2], s_cyd_buf, 0);
        return 0;
    }
    /* rend ctlp <out.rgb> [layoutA|B] [cursor] — portrait Control */
    if (argc > 2 && strcmp(argv[1], "ctlp") == 0) {
        int layout = 0;
        int cursor = 0;
        if (argc > 3 && (argv[3][0] == 'B' || argv[3][0] == 'b' || argv[3][0] == '1')) layout = 1;
        if (argc > 4) cursor = atoi(argv[4]);
        int norain = getenv("NORAIN") ? 1 : 0;
        render_ctlp(argv[2], layout, cursor, norain);
        return 0;
    }
    /* rend ctlp-seq <outdir> — A/B rain+.norain + 5 theme frames each (next-frame change) */
    if (argc > 2 && strcmp(argv[1], "ctlp-seq") == 0) {
        const char *dir = argv[2];
        char path[512];
        for (int layout = 0; layout < 2; ++layout) {
            const char L = layout ? 'B' : 'A';
            snprintf(path, sizeof path, "%s/control-portrait-%c.rgb", dir, L);
            render_ctlp(path, layout, 0, 0);
            snprintf(path, sizeof path, "%s/control-portrait-%c.norain.rgb", dir, L);
            render_ctlp(path, layout, 0, 1);
            /* Five frames: theme/chip changes; rain updates on the NEXT frame. */
            static const int cursors[5] = { 1, 2, 3, 4, 5 }; /* mix, shape, jump, rain col, bg */
            host_fw = 240; host_fh = 320;
            botcorps_cyd_orient = 1;
            botcorps_rain_set_view(240, 320);
            botcorps_band_force_direct(true);
            host_rain_init(0);
            host_set_millis(9000u);
            botcorps_host_set_color(0);
            host_gate_norain = 0;
            memset(s_cyd_buf, 0, sizeof s_cyd_buf);
            botcorps_host_control_p(s_cyd_buf, 0, layout);
            for (int f = 0; f < 5; ++f) {
                /* apply change */
                botcorps_host_control_p(s_cyd_buf, cursors[f], layout);
                /* simulate Enter = nudge_row(1) via re-entering with toggled state helpers */
                if (cursors[f] == 1) botcorps_rain_set_mix((f + 1) % 3);
                else if (cursors[f] == 2) botcorps_rain_set_shape((f + 1) % 5);
                else if (cursors[f] == 3) botcorps_rain_set_mode(f % 2 == 0 ? 1 : 0);
                else if (cursors[f] == 4) botcorps_rain_set_mode(1 + (f % 6));
                else if (cursors[f] == 5) botcorps_rain_set_bg((f + 1) % 5);
                host_set_millis(9000u + (uint32_t)(f + 1) * 200u);
                memset(s_cyd_buf, 0, sizeof s_cyd_buf);
                botcorps_host_control_p(s_cyd_buf, cursors[f], layout);
                snprintf(path, sizeof path, "%s/control-portrait-%c-f%d.rgb", dir, L, f);
                write_raw(path, s_cyd_buf, 0);
            }
        }
        return 0;
    }
    /* rend port <screen> <out.rgb> [pt_mode 0|1|2] [sym 0|1]
     * screen: hub|term|control|wifi|ble|scan|gchat */
    if (argc > 3 && strcmp(argv[1], "port") == 0) {
        int scr = 1; /* SCR_GIBSON */
        if (!strcmp(argv[2], "hub")) scr = 1;
        else if (!strcmp(argv[2], "term")) scr = 2;
        else if (!strcmp(argv[2], "control")) scr = 8;
        else if (!strcmp(argv[2], "wifi")) scr = 3;
        else if (!strcmp(argv[2], "ble")) scr = 5;
        else if (!strcmp(argv[2], "scan") || !strcmp(argv[2], "gibson")) scr = 7;
        else if (!strcmp(argv[2], "airtag")) scr = 76; /* bot 6 */
        else if (!strcmp(argv[2], "wifi-bot")) scr = 70;
        else if (!strcmp(argv[2], "bt-bot") || !strcmp(argv[2], "bluetooth")) scr = 71;
        else if (!strcmp(argv[2], "deauth")) scr = 72;
        else if (!strcmp(argv[2], "hack")) scr = 73;
        else if (!strcmp(argv[2], "flock")) scr = 74;
        else if (!strcmp(argv[2], "drone")) scr = 75;
        else if (!strcmp(argv[2], "keyfob")) scr = 77;
        else if (!strcmp(argv[2], "meta")) scr = 78;
        else if (!strcmp(argv[2], "skimmer")) scr = 79;
        else if (!strcmp(argv[2], "flipper")) scr = 80;
        else if (!strcmp(argv[2], "samsung")) scr = 81;
        else if (!strcmp(argv[2], "gchat")) scr = 10;
        else if (argv[2][0] == 'b' && argv[2][1] >= '0' && argv[2][1] <= '9') {
            int bi = atoi(argv[2] + 1);
            if (bi >= 0 && bi <= 11) scr = 70 + bi;
        }
        int pt = argc > 4 ? atoi(argv[4]) : 0;
        int sym = argc > 5 ? atoi(argv[5]) : 0;
        host_fw = 240; host_fh = 320;
        botcorps_cyd_orient = 1;
        botcorps_rain_set_view(240, 320);
        botcorps_band_force_direct(true);
        host_rain_init(0);
        host_set_millis(getenv("MS") ? (uint32_t)atoi(getenv("MS")) : 9000u);
        botcorps_host_set_color(0);
        memset(s_cyd_buf, 0, sizeof s_cyd_buf);
        botcorps_host_portrait(s_cyd_buf, scr, pt, sym);
        write_raw(argv[3], s_cyd_buf, 0);
        return 0;
    }
    /* rend ctlp-pins <outdir> — portrait Control geom A, pin styles A+B @ native; caller upscales */
    if (argc > 2 && strcmp(argv[1], "ctlp-pins") == 0) {
        const char *dir = argv[2];
        char path[512];
        snprintf(path, sizeof path, "%s/control-pins-A.rgb", dir);
        render_ctlp(path, 0 /* geomA + pinA */, 0, getenv("NORAIN") ? 1 : 0);
        snprintf(path, sizeof path, "%s/control-pins-B.rgb", dir);
        render_ctlp(path, 2 /* geomA + pinB */, 0, getenv("NORAIN") ? 1 : 0);
        return 0;
    }

    /* rend ctlp-spread <outdir> — Control spread options 1/2 @ native (layout = spread<<4).
     * Emits rain + norain + theme t0/t1/t2 (GRAPE/MASHUP/MIX cursors) for each. */
    if (argc > 2 && strcmp(argv[1], "ctlp-spread") == 0) {
        const char *dir = argv[2];
        char path[512];
        static const int themes[3] = { 0, 1, 2 }; /* GRAPE, MASHUP, MIX part cursors */
        for (int sp = 1; sp <= 2; ++sp) {
            const int layout = sp << 4;
            snprintf(path, sizeof path, "%s/control-spread-%d.rgb", dir, sp);
            render_ctlp(path, layout, 0, 0);
            snprintf(path, sizeof path, "%s/control-spread-%d.norain.rgb", dir, sp);
            render_ctlp(path, layout, 0, 1);
            for (int t = 0; t < 3; ++t) {
                snprintf(path, sizeof path, "%s/control-spread-%d-t%d.rgb", dir, sp, t);
                render_ctlp(path, layout, themes[t], 0);
            }
        }
        return 0;
    }
    return -1;
}
