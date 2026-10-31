/* Host sim for the cyd_livescan UI: the SAME src/live/lv_core.h the firmware runs, with zlib raw inflate
 * in place of ROM tinfl, an RGB565 framebuffer, and a FAKE radio (canned finds, clearly not real data).
 *   sim_live <script> <outdir>
 *   sim_live <script> <outdir> nocal   (power on with NO saved cal: CALbot captures first)
 * script lines: hold X Y MS (finger down on screen x,y -> simulated raw) | up MS | recstart NAME | recstop |
 *   tick N | tap X Y | type TEXT (taps the on-screen keys; '~' = ENTER) | dump NAME | theme N | log
 *   press X Y / release (finger down / up NOW: one raw sample at the current time, then the usual 15 ms samples)
 *   tickto T (advance to exactly T ms: 30 ms ticks, then 15 ms raw samples + one lv_tick at T) */
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#define BOTCORPS_CYD_LIVESCAN 1
#include "../../src/live/lv_core.h"
#include "lgfx_cal_ref.h"
#include "touch_model.h"

static uint16_t fb[320][240];
static std::vector<uint8_t> blob;
static uint8_t ibuf[LK_INFL_CAP];
static uint32_t t_ms = 0;
static int g_dead = 0;
static long g_px_pushed = 0;
/* flicker watch (wstart / wstop NAME): every framebuffer write that CHANGES a pixel is counted per pixel, so a pixel
 * that changes more than once between wstart and wstop showed an intermediate state on the panel (a flash). */
static bool g_w = false; static uint16_t g_w0[320][240]; static uint8_t g_wc[320][240]; static long g_wblack = 0, g_wpx = 0;
static int g_wpx_x = -1, g_wpx_y = -1;
static inline void wput(int y, int x, uint16_t v)
{
    if (g_w && x == g_wpx_x && y == g_wpx_y && fb[y][x] != v) printf("WPIX (%d,%d) %04x -> %04x after %ld px\n", x, y, fb[y][x], v, g_wpx);
    if (g_w) { ++g_wpx; if (fb[y][x] != v) { if (g_wc[y][x] < 255) ++g_wc[y][x]; if ((v & 0xE79C) == 0 && (fb[y][x] & 0xE79C) != 0) ++g_wblack; } }
    fb[y][x] = v;
}

static int infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    z_stream z; memset(&z, 0, sizeof z);
    inflateInit2(&z, -15);
    z.next_in = (Bytef *)src; z.avail_in = len; z.next_out = dst; z.avail_out = cap;
    int r = inflate(&z, Z_FINISH); int out = (int)z.total_out; inflateEnd(&z);
    return r == Z_STREAM_END ? out : -1;
}
/* SIM_FWPAL=1 models the firmware's lv_ui.cpp palette cache exactly: s_pal is refreshed by lv_use_pal() only
 * before lv_tap() and before lv_tick(), never inside them (pre-fix builds). SIM_FWPAL=2 models the fixed
 * firmware, where ui_raw_draw() also re-validates the cache on every draw. Default: the palette ui_raw_draw resolves. */
static int g_fwpal = -1, g_fwpal_clip = -1;
static void sim_use_pal(void) { if (g_fwpal > 0) g_fwpal_clip = lk_clip_of_seg(lv_play.seg); }
static void raw_draw(int y, int x, int n, const uint8_t *idx)
{
    if (g_fwpal < 0) { const char *e = getenv("SIM_FWPAL"); g_fwpal = e && *e == '1' ? 1 : e && *e == '2' ? 2 : 0; }
    if (g_fwpal == 2) g_fwpal_clip = lk_clip_of_seg(lv_play.seg); /* fixed firmware: ui_raw_draw re-validates s_pal */
    const uint16_t *pal = LK_CLIP[g_fwpal && g_fwpal_clip >= 0 ? g_fwpal_clip : lk_clip_of_seg(lv_play.seg)].pal;
    for (int i = 0; i < n; ++i) wput(y, x + i, pal[idx[i]]);
}
static void blit(int x, int y, int w, int h, const uint16_t *px)
{
    for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) wput(y + r, x + c, px[r * w + c]);
    g_px_pushed += w * h;
}

/* ---- fake radio: canned finds; scans "take" 1.2 s (wifi) / 5 s (ble) of sim time ---- */
static uint32_t fk_t0; static int fk_job = 0;
static LvFind mk(const char *n, int rssi, int ch, int auth, uint16_t mfg, uint8_t apple, uint16_t svc, uint8_t m5)
{
    LvFind f; memset(&f, 0, sizeof f); snprintf(f.name, sizeof f.name, "%s", n);
    const uint8_t mac[6] = {0x3C, 0x84, 0x6A, 0x12, (uint8_t)(m5 * 7), m5}; memcpy(f.mac, mac, 6);
    f.rssi = (int8_t)rssi; f.ch = (uint8_t)ch; f.auth = (uint8_t)auth; f.mfg = mfg; f.apple = apple; f.svc = svc; return f;
}
static int fk_overlap = 0, fk_starts = 0;
static int fk_wifi_start(void) { fk_starts++; if (fk_job) fk_overlap++; fk_job = 1; fk_t0 = t_ms; return 0; }
static uint32_t fk_ble_ms = 5000; static int fk_ovr_m5 = -1, fk_ovr_rssi = 0, fk_ble_n = 0;
static int fk_ble_start(uint32_t ms) { fk_ble_ms = ms; fk_ble_n++; fk_starts++; if (fk_job) fk_overlap++; fk_job = 2; fk_t0 = t_ms; return 0; }
static int fk_deauth_start(uint32_t) { fk_starts++; if (fk_job) fk_overlap++; fk_job = 3; fk_t0 = t_ms; return 0; }
static int fk_wifi_poll(LvFind *o, int max)
{
    if (t_ms - fk_t0 < 1200) return -1;
    const LvFind F[] = {mk("HomeNet-5G", -52, 6, 3, 0, 0, 0, 1), mk("Flock-A7F21C", -61, 11, 3, 0, 0, 0, 2), mk("TELLO-5A21B3", -66, 1, 0, 0, 0, 0, 3),
                        mk("xfinitywifi", -70, 1, 0, 0, 0, 0, 4), mk("DIRECT-6B-HP OfficeJet", -73, 6, 3, 0, 0, 0, 5), mk("", -80, 149, 6, 0, 0, 0, 6),
                        mk("Tesla_Model3", -82, 36, 3, 0, 0, 0, 7), mk("NETGEAR42", -84, 9, 4, 0, 0, 0, 8), mk("CoffeeShop Guest", -88, 3, 0, 0, 0, 0, 9)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0;
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5) o[i].rssi = (int8_t)fk_ovr_rssi;   /* 'sig' command: SAMPLE dBm for a hunt test */
    return n;
}
static int fk_ble_poll(LvFind *o, int max)
{
    if (t_ms - fk_t0 < fk_ble_ms) return -1;
    const LvFind F[] = {mk("", -48, 0, 0, 0x004C, 0x12, 0, 10), mk("Flipper Zeph", -58, 0, 0, 0xFFFF, 0, 0, 11), mk("", -63, 0, 0, 0x0075, 0, 0xFD5A, 12),
                        mk("RB Meta 0A1F", -67, 0, 0, 0x01AB, 0, 0, 13), mk("HC-05", -71, 0, 0, 0xFFFF, 0, 0, 14), mk("Galaxy Watch6", -74, 0, 0, 0x0075, 0, 0, 15),
                        mk("Tile", -77, 0, 0, 0xFFFF, 0, 0xFEED, 16), mk("JBL Flip 6", -79, 0, 0, 0xFFFF, 0, 0, 17), mk("", -83, 0, 0, 0x004C, 0x10, 0, 18),
                        mk("[TV] Samsung Q60", -86, 0, 0, 0x0075, 0, 0, 19), mk("", -91, 0, 0, 0xFFFF, 0, 0, 20)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0;
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5) o[i].rssi = (int8_t)fk_ovr_rssi;   /* 'sig' command: SAMPLE dBm for a hunt test */
    return n;
}
static int fk_deauth_poll(uint32_t *d, uint32_t *m, uint8_t *ch)
{
    const uint32_t e = t_ms - fk_t0; *ch = (uint8_t)(1 + (e / 250) % 13); *m = e / 7; *d = e > 4000 ? (e - 4000) / 300 : 0;
    if (e < 10000) return -1; fk_job = 0; return 0;
}
static void fk_stop(void) { fk_job = 0; }
static void fk_sys(int w, char *o, int n)
{
    if (w == 0) snprintf(o, n, "heap 171k free, min 160k (SIM)");
    else if (w == 1) snprintf(o, n, "ESP32-D0WD rev3 240MHz (SIM)");
    else if (w == 2) snprintf(o, n, "reset: POWERON (SIM)");
    else if (w == 3) snprintf(o, n, "flash 4MB DIO, app slot 3MB (SIM)");
    else if (w == 4) snprintf(o, n, "touch cal: 8-pt matrix OK (SIM)");
}
static void fk_save(int) {}
static uint8_t fk_nvs[2][16 * 6]; static int fk_nvs_n[2] = {0, 0};   /* SIM NVS for radar rdign / rdwat */
static int fk_mac_load(const char *k, uint8_t *o, int max) { const int w = !strcmp(k, "rdwat"); int n = fk_nvs_n[w] < max ? fk_nvs_n[w] : max; memcpy(o, fk_nvs[w], n * 6); return n; }
static void fk_mac_save(const char *k, const uint8_t *m, int n) { const int w = !strcmp(k, "rdwat"); fk_nvs_n[w] = n; memcpy(fk_nvs[w], m, n * 6); printf("t=%u NVS %s = %d MACs (SIM)\n", t_ms, k, n); }
static int fk_rot = 2;   /* the UI's setRotation: 2 = portrait USB at the bottom (default), 0 = ORIENT FLIP */
/* simulated XPT2046 (CYD-like swapped + flipped axes, +-30 raw noise) and the fake NVS save */
static TouchModel g_tm = {"swap+flip (CYD-like)", 3820, 0, -11.2, 3760, -14.6, 0, 30, 2};
static bool fk_cal_save(const uint16_t c[8])
{
    printf("t=%u CAL SAVE c0..c7 = %u %u %u %u %u %u %u %u (SIM NVS)\n", t_ms, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
    LgfxCalRef L; L.setCalibrate(c); int worst = 0;
    for (int i = 0; i < 4; ++i) {
        int rx, ry, x, y; g_tm.raw(LV_CAL_PT[i][0], LV_CAL_PT[i][1], &rx, &ry, false); L.convertRawXY(rx, ry, &x, &y);
        if (fk_rot == 2) { x = 239 - x; y = 319 - y; }   /* Panel_Device::convertRawXY, r == 2: both axes flip */
        const int d = abs(x - LV_CAL_PT[i][0]) > abs(y - LV_CAL_PT[i][1]) ? abs(x - LV_CAL_PT[i][0]) : abs(y - LV_CAL_PT[i][1]);
        if (d > worst) worst = d;
        printf("  chip %d at %d,%d -> lgfx maps to %d,%d\n", i + 1, LV_CAL_PT[i][0], LV_CAL_PT[i][1], x, y);
    }
    printf("  worst axis error %d px\n", worst);
    return true;
}
static void fk_set_rot(int flip) { fk_rot = flip ? 0 : 2; printf("[sim] set_rot -> setRotation(%d)\n", fk_rot); }
static const LvRadio FAKE = {fk_wifi_start, fk_wifi_poll, fk_ble_start, fk_ble_poll, fk_deauth_start, fk_deauth_poll, fk_stop, fk_sys, fk_save, fk_cal_save, fk_set_rot, fk_mac_load, fk_mac_save};
static bool g_down = false; static int g_fx, g_fy; static FILE *g_rec = nullptr;
static void dump(const char *path);
static void rec_frame(void);
static void tick1(const LkIo &io)
{
    for (int k = 0; k < 2; ++k) { t_ms += 15; int rx = 0, ry = 0; if (g_down) g_tm.raw(g_fx, g_fy, &rx, &ry); lv_cal_raw(g_down, rx, ry, t_ms); }
    sim_use_pal();
    lv_touch(g_down, g_fx, g_fy);
    lv_tick(io, t_ms);
    rec_frame();
}
static void rec_frame(void)
{
    if (g_rec) { dump("/tmp/.sim_rec_part"); FILE *b = fopen("/tmp/.sim_rec_part", "rb"); static uint8_t buf[240 * 320 * 3]; size_t k = fread(buf, 1, sizeof buf, b); fwrite(buf, 1, k, g_rec); fclose(b); }
}

static void tickto(const LkIo &io, uint32_t T)
{
    while (t_ms + 30 <= T) tick1(io);
    if (t_ms >= T) return;
    while (t_ms < T) { t_ms = t_ms + 15 <= T ? t_ms + 15 : T; int rx = 0, ry = 0; if (g_down) g_tm.raw(g_fx, g_fy, &rx, &ry); lv_cal_raw(g_down, rx, ry, t_ms); }
    sim_use_pal();
    lv_tick(io, t_ms);
    rec_frame();
}
static void dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    static uint8_t row[240 * 3];
    for (int y = 0; y < 320; ++y) {
        for (int x = 0; x < 240; ++x) {
            uint16_t v = fb[y][x];
            int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
            row[x * 3] = (r << 3) | (r >> 2); row[x * 3 + 1] = (g << 2) | (g >> 4); row[x * 3 + 2] = (b << 3) | (b >> 2);
        }
        fwrite(row, 1, sizeof row, f);
    }
    fclose(f);
}
static void tap_key(int k, LkIo &io)
{
    const LvRect r = lv_key_rect(k);
    printf("  key %d at %d,%d -> %s\n", k, (r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2, lv_tap((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2));
    for (int i = 0; i < 3; ++i) tick1(io);
}
int main(int argc, char **argv)
{
    FILE *bf = fopen("src/lock/live/lock_blob_live.bin", "rb");
    if (!bf || argc < 3) { fprintf(stderr, "usage: sim_live script outdir (run from repo root)\n"); return 1; }
    blob.resize(LK_BLOB_SIZE);
    if (fread(blob.data(), 1, LK_BLOB_SIZE, bf) != LK_BLOB_SIZE) return 1;
    LkIo io{blob.data(), infl, lv_clip_draw, ibuf};
    lv_begin(&FAKE, blit, raw_draw, 0, 0, !(argc > 3 && !strcmp(argv[3], "nocal")));
    lv_tick(io, 0);                            /* power-on paint at t=0 (the firmware loop ticks at once too) */
    FILE *sc = fopen(argv[1], "r"); char cmd[32];
    while (fscanf(sc, "%31s", cmd) == 1) {
        if (!strcmp(cmd, "tick")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; for (int i = 0; i < n; ++i) tick1(io); }
        else if (!strcmp(cmd, "hold")) { int ms; if (fscanf(sc, "%d %d %d", &g_fx, &g_fy, &ms) != 3) return 2; g_down = true; for (int e = 0; e < ms; e += 30) tick1(io); g_down = false; printf("t=%u hold %d,%d -> scr=%d cal_n=%d phase=%d msg=%d\n", t_ms, g_fx, g_fy, lv_scr, lv_cal_n, lv_cal_phase, lv_cal_msg); }
        else if (!strcmp(cmd, "press")) { if (fscanf(sc, "%d %d", &g_fx, &g_fy) != 2) return 2; g_down = true; int rx, ry; g_tm.raw(g_fx, g_fy, &rx, &ry); lv_cal_raw(true, rx, ry, t_ms); }
        else if (!strcmp(cmd, "release")) { g_down = false; lv_cal_raw(false, 0, 0, t_ms); }
        else if (!strcmp(cmd, "tickto")) { unsigned T; if (fscanf(sc, "%u", &T) != 1) return 2; tickto(io, T); }
        else if (!strcmp(cmd, "up")) { int ms; if (fscanf(sc, "%d", &ms) != 1) return 2; g_down = false; for (int e = 0; e < ms; e += 30) tick1(io); }
        else if (!strcmp(cmd, "recstart")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); g_rec = fopen(p, "wb"); }
        else if (!strcmp(cmd, "recstop")) { if (g_rec) fclose(g_rec); g_rec = nullptr; remove("/tmp/.sim_rec_part"); }
        else if (!strcmp(cmd, "tap")) { int x, y; if (fscanf(sc, "%d %d", &x, &y) != 2) return 2; sim_use_pal(); const char *r = lv_tap(x, y); printf("t=%u tap %d,%d -> %s (scr=%d seg=%d)\n", t_ms, x, y, r, lv_scr, lv_play.seg); }
        else if (!strcmp(cmd, "rhold")) {   /* finger down on (x,y) for ms, then the release-edge tap, as lv_ui.cpp does */
            int x, y, ms; if (fscanf(sc, "%d %d %d", &x, &y, &ms) != 3) return 2; g_fx = x; g_fy = y; g_down = true;
            for (int e = 0; e < ms; e += 30) tick1(io);
            g_down = false; tick1(io); sim_use_pal(); const char *r = lv_tap(x, y); printf("t=%u rhold %d,%d %dms -> %s (scr=%d)\n", t_ms, x, y, ms, r, lv_scr);
        }
        else if (!strcmp(cmd, "sig")) { if (fscanf(sc, "%d %d", &fk_ovr_m5, &fk_ovr_rssi) != 2) return 2; }
        else if (!strcmp(cmd, "towers")) { lv_rd_layout(); for (int n = 0; n < lv_rd_n; ++n) { const LvTower &T = lv_rd_tw[n]; printf("  tower f%d %s %d dBm m5=%d x%d-%d y%d-%d\n", T.i, lv_f[T.i].name, lv_f[T.i].rssi, lv_f[T.i].mac[5], T.x0, T.x1, T.byt, T.yb); } }
        else if (!strcmp(cmd, "radarstate")) printf("t=%u RADAR sel=%d lock=%d hunt=%d reads=%d trend=%d miss=%d ign=%d wat=%d job=%d tk='%s'\n", t_ms, lv_rd_sel, lv_rd_lock, lv_rd_hunting, lv_rd_reads, lv_rd_trend, lv_rd_miss, lv_rd_nign, lv_rd_nwat, lv_job, lv_tk_msg);
        else if (!strcmp(cmd, "theme")) { int t; if (fscanf(sc, "%d", &t) != 1) return 2; lv_set_theme(t); }
        else if (!strcmp(cmd, "type")) {
            char line[128]; if (!fgets(line, sizeof line, sc)) return 2;
            char *s = line; while (*s == ' ') ++s;
            for (; *s && *s != '\n'; ++s) {
                if (*s == '~') { tap_key(LV_KEY_ENTER, io); continue; }
                if (*s == ' ') { tap_key(LV_KEY_SPACE, io); continue; }
                int k = -1; bool sh = false;
                for (int r = 0; r < 4 && k < 0; ++r) for (int c = 0; c < 10; ++c) { if (LV_KROW[r][c] == *s) { k = r * 10 + c; break; } if (LV_KROW_S[r][c] == *s) { k = r * 10 + c; sh = true; break; } }
                if (k < 0) { fprintf(stderr, "no key for '%c'\n", *s); return 3; }
                if (sh != lv_shift) tap_key(LV_KEY_SHIFT, io);
                tap_key(k, io);
                if (sh) tap_key(LV_KEY_SHIFT, io);
            }
        }
        else if (!strcmp(cmd, "dump")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); dump(p); printf("DUMP %s scr=%d seg=%d clipframe=%d t=%u\n", nm, lv_scr, lv_play.seg, lv_play.seg >= 0 && lv_play.shown >= 0 ? LK_SEG[lv_play.seg].f0 + lv_play.shown : -1, t_ms);
            /* live overlay rects at this instant: masks (always repainted) + tints that are not plain clip pixels + an active press flash */
            printf("OVL %s", nm);
            for (int i = 0; i < lv_nmask; ++i) printf(" m:%d,%d,%d,%d", lv_masks[i].x0, lv_masks[i].y0, lv_masks[i].x1, lv_masks[i].y1);
            for (int i = 0; i < lv_ntint; ++i) { const LvTS t = lv_tint_state(lv_tints[i].id); if (t.mode != LV_TM_RAW || t.deco) printf(" t%d:%d,%d,%d,%d", lv_tints[i].id, lv_tints[i].r.x0, lv_tints[i].r.y0, lv_tints[i].r.x1, lv_tints[i].r.y1); }
            if (lv_flash_r.x1 >= lv_flash_r.x0) printf(" f:%d,%d,%d,%d", lv_flash_r.x0, lv_flash_r.y0, lv_flash_r.x1, lv_flash_r.y1);
            printf("\n"); }
        else if (!strcmp(cmd, "rec")) { /* rec N NAME: N ticks, one frame per tick appended to NAME.rgb */
            int n; char nm[64]; if (fscanf(sc, "%d %63s", &n, nm) != 2) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm);
            FILE *o = fopen(p, "wb"); fclose(o);
            for (int i = 0; i < n; ++i) { tick1(io); char q[520]; snprintf(q, sizeof q, "%s.part", p); dump(q); FILE *a = fopen(p, "ab"), *b = fopen(q, "rb"); static uint8_t buf[240 * 320 * 3]; size_t k = fread(buf, 1, sizeof buf, b); fwrite(buf, 1, k, a); fclose(a); fclose(b); remove(q); }
            printf("t=%u rec %s %d frames\n", t_ms, nm, n);
        }
        else if (!strcmp(cmd, "backface")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; lv_backbot_face = k & 3; lv_go(lv_scr); for (int i = 0; i < 4; ++i) tick1(io); printf("backface %d\n", lv_backbot_face); }
        else if (!strcmp(cmd, "go")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; lv_go(k); }
        else if (!strcmp(cmd, "probe")) { /* probe NAME X Y: what a tap there does, in a forked child (no side effects) */
            char nm[64]; int x, y; if (fscanf(sc, "%63s %d %d", nm, &x, &y) != 3) return 2;
            int pf[2]; if (pipe(pf)) return 3; fflush(stdout);
            const pid_t pid = fork();
            if (pid == 0) { close(pf[0]); FILE *o = freopen("/dev/null", "w", stdout); (void)o; const char *r = lv_tap(x, y); char b[64]; snprintf(b, sizeof b, "%s", r); if (write(pf[1], b, strlen(b) + 1) < 0) {} _exit(0); }
            close(pf[1]); char b[64] = {0}; if (read(pf[0], b, sizeof b - 1) < 0) {} close(pf[0]); waitpid(pid, nullptr, 0);
            const bool dead = !strcmp(b, "none"); if (dead) ++g_dead;
            printf("PROBE %-14s scr=%d %3d,%3d -> %s%s\n", nm, lv_scr, x, y, b, dead ? "   <<< DEAD" : "");
        }
        else if (!strcmp(cmd, "audit")) { /* audit NAME STEP: tap every STEP px in forked children, map + region list */
            char nm[64]; int st; if (fscanf(sc, "%63s %d", nm, &st) != 2) return 2;
            std::vector<std::string> names; std::vector<int> cnt, bx0, by0, bx1, by1; std::vector<int> grid((240 / st + 1) * (320 / st + 1), -1);
            for (int y = 0; y < 320; y += st) {
                const int gw = 240 / st + 1; int pf[2]; if (pipe(pf)) return 3; fflush(stdout);
                const pid_t pid = fork();
                if (pid == 0) { /* one child per row: each tap in its own grandchild */
                    close(pf[0]); FILE *o = freopen("/dev/null", "w", stdout); (void)o;
                    for (int x = 0; x < 240; x += st) {
                        int q[2]; if (pipe(q)) _exit(3);
                        const pid_t c = fork();
                        if (c == 0) { close(q[0]); const char *r = lv_tap(x, y); char b[48] = {0}; snprintf(b, sizeof b, "%s", r); if (write(q[1], b, 48) < 0) {} _exit(0); }
                        close(q[1]); char b[48] = {0}; if (read(q[0], b, 48) < 0) {} close(q[0]); waitpid(c, nullptr, 0);
                        if (write(pf[1], b, 48) < 0) {}
                    }
                    _exit(0);
                }
                close(pf[1]);
                for (int x = 0; x < 240; x += st) {
                    char b[48] = {0}; if (read(pf[0], b, 48) != 48) break;
                    if (!strcmp(b, "none")) continue;
                    int k = -1; for (size_t j = 0; j < names.size(); ++j) if (names[j] == b) k = (int)j;
                    if (k < 0) { k = (int)names.size(); names.push_back(b); cnt.push_back(0); bx0.push_back(999); by0.push_back(999); bx1.push_back(-1); by1.push_back(-1); }
                    ++cnt[k]; if (x < bx0[k]) bx0[k] = x; if (y < by0[k]) by0[k] = y; if (x > bx1[k]) bx1[k] = x; if (y > by1[k]) by1[k] = y;
                    grid[(y / st) * gw + x / st] = k;
                }
                close(pf[0]); waitpid(pid, nullptr, 0);
            }
            printf("AUDIT %s scr=%d step=%d: %zu actions\n", nm, lv_scr, st, names.size());
            for (size_t j = 0; j < names.size(); ++j) printf("  %-18s %5d pts  bbox {%d,%d,%d,%d}\n", names[j].c_str(), cnt[j], bx0[j], by0[j], bx1[j], by1[j]);
            char p[512]; snprintf(p, sizeof p, "%s/%s.audit.txt", argv[2], nm); FILE *f = fopen(p, "w");
            if (f) { fprintf(f, "%d\n", st); for (size_t j = 0; j < names.size(); ++j) fprintf(f, "%s\n", names[j].c_str()); fprintf(f, "--\n");
                     const int gw = 240 / st + 1; for (int y = 0; y < 320; y += st) { for (int x = 0; x < 240; x += st) fprintf(f, "%d ", grid[(y / st) * gw + x / st]); fprintf(f, "\n"); } fclose(f); }
        }
        else if (!strcmp(cmd, "radartime")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2;
            double tot = 0, mx = 0; const long px0 = g_px_pushed;
            for (int i = 0; i < n; ++i) { lv_now = t_ms; timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a); lv_radar_frame(); clock_gettime(CLOCK_MONOTONIC, &b);
                const double ms = (b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6; tot += ms; if (ms > mx) mx = ms; t_ms += 30; }
            const double ppf = (double)(g_px_pushed - px0) / n;
            printf("RADARTIME frames=%d host avg %.3f ms max %.3f ms; %.0f px pushed/frame -> SPI 40MHz floor %.1f ms/frame (%.0f fps cap); towers=%d\n",
                   n, tot / n, mx, ppf, ppf * 16 / 40e6 * 1e3, 1000.0 / (ppf * 16 / 40e6 * 1e3), lv_rd_n);
        }
        else if (!strcmp(cmd, "heap50")) { /* 50 scan cycles through the UI; one radio at a time, radios off after, no growth */
            int stopped = 0, done = 0; size_t h1 = 0; const int st0 = fk_starts;
            for (int c = 0; c < 50; ++c) {
                if (c == 1) h1 = mallinfo2().uordblks;
                lv_scanmix = c % 3; lv_tap(120, 90);   /* SCANbot: start the mix scan */
                const uint32_t t0 = t_ms; bool cut = (c % 5) == 4;
                while (lv_job != LV_J_NONE && t_ms - t0 < 20000) { tick1(io); if (cut && t_ms - t0 > 600) { lv_tap(120, 90); cut = false; ++stopped; } }
                if (lv_job == LV_J_NONE) ++done;
                if (fk_job != 0) printf("  cycle %d: RADIO LEFT ON (fk_job=%d)\n", c, fk_job);
                for (int i = 0; i < 4; ++i) tick1(io);
            }
            const size_t h50 = mallinfo2().uordblks;
            printf("HEAP50 cycles=50 completed=%d stopped_mid=%d radio_starts=%d overlaps=%d radio_on_at_end=%d lv_nf=%d/%d host_heap c1=%zu c50=%zu delta=%ld\n",
                   done, stopped, fk_starts - st0, fk_overlap, fk_job, lv_nf, LV_MAXF, h1, h50, (long)h50 - (long)h1);
        }
        else if (!strcmp(cmd, "caltest")) { /* rot 2 and rot 0: noise-free chip medians -> lv_cal_solve -> LovyanGFX math -> screen */
            for (int rot = 2; rot >= 0; rot -= 2) for (int wrong = 0; wrong < 2; ++wrong) {
                uint16_t med[4][2], c8[8]; TouchModel tm = g_tm; tm.noise = 0;
                /* raw is a function of where the finger is on the GLASS; at rot 0 the glass point for screen (x,y) is the 180-deg turned one */
                for (int i = 0; i < 4; ++i) { int rx, ry; const int gx = rot == 2 ? LV_CAL_PT[i][0] : 239 - LV_CAL_PT[i][0], gy = rot == 2 ? LV_CAL_PT[i][1] : 319 - LV_CAL_PT[i][1]; tm.raw(gx, gy, &rx, &ry, false); med[i][0] = (uint16_t)rx; med[i][1] = (uint16_t)ry; }
                const int rot2 = (rot == 2) ^ wrong; lv_cal_solve(med, c8, rot2);
                LgfxCalRef L; L.setCalibrate(c8); int worst = 0;
                const int P[9][2] = {{0, 0}, {239, 0}, {239, 319}, {0, 319}, {120, 160}, {52, 55}, {187, 226}, {224, 300}, {10, 290}};
                char line[400]; int o = 0;
                for (int k = 0; k < 9; ++k) {
                    int rx, ry, x, y; const int gx = rot == 2 ? P[k][0] : 239 - P[k][0], gy = rot == 2 ? P[k][1] : 319 - P[k][1];
                    tm.raw(gx, gy, &rx, &ry, false); L.convertRawXY(rx, ry, &x, &y); if (rot == 2) { x = 239 - x; y = 319 - y; }
                    const int d = abs(x - P[k][0]) > abs(y - P[k][1]) ? abs(x - P[k][0]) : abs(y - P[k][1]); if (d > worst) worst = d;
                    if (k < 4) o += snprintf(line + o, sizeof line - o, " (%d,%d)->(%d,%d)", P[k][0], P[k][1], x, y);
                }
                printf("CALTEST setRotation(%d) solve(rot2=%d)%s worst %d px |%s\n", rot, rot2, wrong ? " [WRONG FLAG, control]" : "", worst, line);
            }
        }
        else if (!strcmp(cmd, "wstart")) { memcpy(g_w0, fb, sizeof fb); memset(g_wc, 0, sizeof g_wc); g_wblack = 0; g_wpx = 0; g_w = true; }
        else if (!strcmp(cmd, "wpix")) { if (fscanf(sc, "%d %d", &g_wpx_x, &g_wpx_y) != 2) return 2; }
        else if (!strcmp(cmd, "wstop")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; g_w = false;
            long once = 0, multi = 0, back = 0; static uint8_t m[320][240][3];
            for (int y = 0; y < 320; ++y) for (int x = 0; x < 240; ++x) { const int c = g_wc[y][x]; once += c == 1; multi += c >= 2; back += c >= 2 && fb[y][x] == g_w0[y][x];
                m[y][x][0] = c >= 2 ? 255 : 0; m[y][x][1] = c == 1 ? 160 : 0; m[y][x][2] = 0; }
            char p[512]; snprintf(p, sizeof p, "%s/%s.wmap.rgb", argv[2], nm); FILE *f = fopen(p, "wb"); fwrite(m, 1, sizeof m, f); fclose(f);
            printf("WATCH %s t=%u px_written=%ld changed_once=%ld changed_2plus=%ld (back_to_start=%ld) went_black=%ld\n", nm, t_ms, g_wpx, once, multi, back, g_wblack); }
        else if (!strcmp(cmd, "log")) { for (int k = (lv_tn < 20 ? lv_tn : 20) - 1; k >= 0; --k) printf("  | %s\n", lv_line_back(k)); }
    }
    printf("px pushed by overlays: %ld\n", g_px_pushed);
    printf("SHADOW peak %d of %d bytes\n", lv_shadow_peak, LV_SHADOW_CAP);
    if (g_dead) printf("DEAD TARGETS: %d\n", g_dead);
    return 0;
}
