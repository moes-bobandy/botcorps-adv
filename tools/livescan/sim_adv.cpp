/* Host renderer for the Adv landscape plate.
 * Same lv_core.h as cyd_livescan, composited by lv_land.h onto 320x240.
 *   sim_adv <outdir>
 * Writes <screen>.rgb (320x240 RGB), <screen>.port.rgb (240x320 RGB shadow)
 * and <screen>.slots. Prints ADV_GATE lines. Exit code = fail count. */
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <initializer_list>
#define BOTCORPS_CYD_LIVESCAN 1
#include "../../src/live/lv_core.h"
#include "../../src/live/lv_land.h"

static std::vector<uint8_t> blob;
static uint8_t ibuf[LK_INFL_CAP];
static uint32_t t_ms = 0;
static int g_fail = 0;

static int infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    z_stream z; memset(&z, 0, sizeof z);
    inflateInit2(&z, -15);
    z.next_in = (Bytef *)src; z.avail_in = len; z.next_out = dst; z.avail_out = cap;
    int r = inflate(&z, Z_FINISH); int out = (int)z.total_out; inflateEnd(&z);
    return r == Z_STREAM_END ? out : -1;
}
static void raw_draw(int y, int x, int n, const uint8_t *idx)
{
    const uint16_t *pal = LK_CLIP[lk_clip_of_seg(lv_play.seg)].pal;
    for (int i = 0; i < n; ++i) lv_land_plot(x + i, y, pal[idx[i]]);
}
static void blit(int x, int y, int w, int h, const uint16_t *px) { lv_land_blit_port(x, y, w, h, px); }

static int fk_wifi_start(void) { return 0; }
static int fk_wifi_poll(LvFind *, int) { return 0; }
static int fk_ble_start(uint32_t) { return 0; }
static int fk_ble_poll(LvFind *, int) { return 0; }
static int fk_deauth_start(uint32_t) { return 0; }
static int fk_deauth_poll(uint32_t *d, uint32_t *m, uint8_t *ch) { *d = 0; *m = 0; *ch = 1; return 0; }
static void fk_stop(void) {}
static void fk_sys(int, char *o, int n) { snprintf(o, n, "adv-land-sim"); }
static void fk_save(int) {}
static bool fk_cal_save(const uint16_t *) { return false; }
static void fk_set_rot(int) {}
static int fk_mac_load(const char *, uint8_t *, int) { return 0; }
static void fk_mac_save(const char *, const uint8_t *, int) {}
static const LvRadio FAKE = {fk_wifi_start, fk_wifi_poll, fk_ble_start, fk_ble_poll, fk_deauth_start, fk_deauth_poll,
                             fk_stop, fk_sys, fk_save, fk_cal_save, fk_set_rot, fk_mac_load, fk_mac_save};

static void tick(const LkIo &io, int n)
{
    for (int i = 0; i < n; ++i) { t_ms += 30; lv_tick(io, t_ms); }
}

static void rgb565_row(const uint16_t *px, int w, uint8_t *row)
{
    for (int x = 0; x < w; ++x) {
        const uint16_t v = px[x];
        const int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
        row[x * 3] = (uint8_t)((r << 3) | (r >> 2));
        row[x * 3 + 1] = (uint8_t)((g << 2) | (g >> 4));
        row[x * 3 + 2] = (uint8_t)((b << 3) | (b >> 2));
    }
}

static int slot_overlap(const LvSlot *o, int n)
{
    int bad = 0;
    for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) {
        const int ax0 = o[i].dx, ay0 = o[i].dy, ax1 = o[i].dx + o[i].sw, ay1 = o[i].dy + o[i].sh;
        const int bx0 = o[j].dx, by0 = o[j].dy, bx1 = o[j].dx + o[j].sw, by1 = o[j].dy + o[j].sh;
        if (ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1) {
            printf("OVERLAP %s %s\n", o[i].name, o[j].name);
            ++bad;
        }
        if (o[i].dx < 0 || o[i].dy < 0 || o[i].dx + o[i].sw > LV_LAND_W || o[i].dy + o[i].sh > LV_LAND_H) {
            printf("OFFSCREEN %s\n", o[i].name);
            ++bad;
        }
    }
    return bad;
}

static void dump_screen(const char *dir, const char *name)
{
    lv_land_compose(0);
    char path[512];
    snprintf(path, sizeof path, "%s/%s.rgb", dir, name);
    FILE *f = fopen(path, "wb");
    uint8_t row[LV_LAND_W * 3];
    for (int y = 0; y < LV_LAND_H; ++y) {
        rgb565_row(lv_land_px + y * LV_LAND_W, LV_LAND_W, row);
        fwrite(row, 1, sizeof row, f);
    }
    fclose(f);
    snprintf(path, sizeof path, "%s/%s.port.rgb", dir, name);
    f = fopen(path, "wb");
    uint8_t prow[LV_PORT_W * 3];
    for (int y = 0; y < LV_PORT_H; ++y) {
        rgb565_row(lv_port_px + y * LV_PORT_W, LV_PORT_W, prow);
        fwrite(prow, 1, sizeof prow, f);
    }
    fclose(f);
    LvSlot slots[24];
    const int n = lv_land_slots(slots, 24);
    snprintf(path, sizeof path, "%s/%s.slots", dir, name);
    f = fopen(path, "w");
    for (int i = 0; i < n; ++i)
        fprintf(f, "%s %d %d %d %d %d %d\n", slots[i].name, slots[i].sx, slots[i].sy, slots[i].sw, slots[i].sh, slots[i].dx, slots[i].dy);
    fclose(f);
    const int ov = slot_overlap(slots, n);
    printf("DUMP %s scr=%d slots=%d overlap=%d\n", name, lv_scr, n, ov);
    if (ov) { printf("ADV_GATE FAIL %s slot geometry\n", name); ++g_fail; }
    else printf("ADV_GATE PASS %s slot geometry\n", name);
}

static void seed_finds(void)
{
    auto put = [](int i, const char *name, int kind, int rssi, uint16_t mfg, uint8_t apple, uint16_t svc) {
        LvFind &f = lv_f[i];
        memset(&f, 0, sizeof f);
        snprintf(f.name, sizeof f.name, "%s", name);
        f.mac[0] = 0x3C; f.mac[5] = (uint8_t)(i + 1);
        f.rssi = (int8_t)rssi; f.kind = (uint8_t)kind; f.mfg = mfg; f.apple = apple; f.svc = svc; f.auth = 3; f.ch = 6;
        lv_classify(f);
    };
    put(0, "HomeNet-5G", LV_K_WIFI, -52, 0xFFFF, 0, 0);
    put(1, "Flock-A7F21C", LV_K_WIFI, -61, 0xFFFF, 0, 0);
    put(2, "TELLO-5A21B3", LV_K_WIFI, -66, 0xFFFF, 0, 0);
    put(3, "", LV_K_BLE, -48, 0x004C, 0x12, 0);
    put(4, "Flipper Zeph", LV_K_BLE, -58, 0xFFFF, 0, 0);
    put(5, "Tile", LV_K_BLE, -70, 0xFFFF, 0, 0xFEED);
    lv_nf = 6;
    lv_dirty_box = true;
}

static void expect_scr(const char *what, int want)
{
    if (lv_scr == want) printf("ADV_GATE PASS %s scr=%d\n", what, lv_scr);
    else { printf("ADV_GATE FAIL %s scr=%d want=%d\n", what, lv_scr, want); ++g_fail; }
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: sim_adv outdir\n"); return 2; }
    FILE *bf = fopen("src/lock/live/lock_blob_live.bin", "rb");
    if (!bf) { fprintf(stderr, "missing blob\n"); return 2; }
    blob.resize(LK_BLOB_SIZE);
    if (fread(blob.data(), 1, LK_BLOB_SIZE, bf) != LK_BLOB_SIZE) return 2;
    fclose(bf);
    if (!lv_land_init(malloc)) { fprintf(stderr, "buffer alloc failed\n"); return 2; }
    LkIo io{blob.data(), infl, lv_clip_draw, ibuf};
    lv_begin(&FAKE, blit, raw_draw, 0, 0, true);
    lv_go(LV_SC_BOOT);
    tick(io, 2);
    expect_scr("boot-open", LV_SC_BOOT);
    dump_screen(argv[1], "boot");

    lv_go(LV_SC_HOME_INTRO);
    tick(io, 2);
    dump_screen(argv[1], "intro");

    lv_go(LV_SC_HOME);
    tick(io, 4);
    expect_scr("home-open", LV_SC_HOME);
    dump_screen(argv[1], "home");
    const int f0 = lv_land_focus();
    const char *mv = lv_land_nav(LV_NAV_RIGHT, 0);
    const int f1 = lv_land_focus();
    if (f1 != f0 && mv && strcmp(mv, "focus-stay")) printf("ADV_GATE PASS home-focus move %d->%d (%s)\n", f0, f1, mv);
    else { printf("ADV_GATE FAIL home-focus move %d->%d (%s)\n", f0, f1, mv ? mv : "-"); ++g_fail; }

    lv_tap((31 + 74) / 2, (52 + 112) / 2); /* boot chip */
    tick(io, 6);
    expect_scr("home-boot-chip", LV_SC_BOOT);
    lv_go(LV_SC_HOME);
    tick(io, 2);
    lv_tap((33 + 101) / 2, (156 + 255) / 2); /* scan chip */
    tick(io, 6);
    expect_scr("home-scan-chip", LV_SC_SCAN);

    seed_finds();
    lv_go(LV_SC_SCAN);
    tick(io, 3);
    dump_screen(argv[1], "scan");
    lv_land_nav(LV_NAV_BACK, 0);
    tick(io, 6);
    expect_scr("scan-back-home", LV_SC_HOME);

    lv_go(LV_SC_TERM);
    tick(io, 3);
    dump_screen(argv[1], "term");
    lv_tap((35 + 70) / 2, (30 + 80) / 2); /* deauth chip -> sweep soon */
    tick(io, 6);
    expect_scr("term-deauth-opens", LV_SC_TTY);

    lv_go(LV_SC_CTL);
    tick(io, 3);
    dump_screen(argv[1], "ctl");
    const int theme0 = lv_theme;
    lv_tap((35 + 113) / 2, (33 + 48) / 2); /* theme row */
    tick(io, 4);
    if (lv_theme != theme0) printf("ADV_GATE PASS ctl-theme %d->%d\n", theme0, lv_theme);
    else { printf("ADV_GATE FAIL ctl-theme stayed %d\n", lv_theme); ++g_fail; }
    lv_go(LV_SC_CTL);
    tick(io, 2);

    lv_go(LV_SC_TTY);
    tick(io, 2);
    lv_land_nav(LV_NAV_CHAR, 'h');
    lv_land_nav(LV_NAV_CHAR, 'e');
    lv_land_nav(LV_NAV_CHAR, 'l');
    lv_land_nav(LV_NAV_CHAR, 'p');
    lv_land_nav(LV_NAV_OK, 0);
    tick(io, 2);
    dump_screen(argv[1], "tty");
    if (lv_tn > 1) printf("ADV_GATE PASS tty-help lines=%d\n", lv_tn);
    else { printf("ADV_GATE FAIL tty-help lines=%d\n", lv_tn); ++g_fail; }

    seed_finds();
    lv_go(LV_SC_RADAR);
    tick(io, 3);
    dump_screen(argv[1], "radar");
    expect_scr("radar-open", LV_SC_RADAR);

    printf("ADV_GATE done fails=%d\n", g_fail);
    return g_fail ? 1 : 0;
}
