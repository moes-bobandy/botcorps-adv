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
#include <malloc.h>
#define BOTCORPS_CYD_LIVESCAN 1
#include "../../src/live/lv_core.h"
#include "lgfx_cal_ref.h"
#include "touch_model.h"

static uint16_t fb[320][240];
static std::vector<uint8_t> blob;
static uint8_t ibuf[LK_INFL_CAP];
static uint32_t t_ms = 0;
static long g_px_pushed = 0;

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
    for (int i = 0; i < n; ++i) fb[y][x + i] = pal[idx[i]];
}
static void blit(int x, int y, int w, int h, const uint16_t *px)
{
    for (int r = 0; r < h; ++r) memcpy(&fb[y + r][x], px + r * w, w * 2);
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
static int fk_wifi_start(void) { fk_job = 1; fk_t0 = t_ms; return 0; }
static int fk_ble_start(uint32_t) { fk_job = 2; fk_t0 = t_ms; return 0; }
static int fk_deauth_start(uint32_t) { fk_job = 3; fk_t0 = t_ms; return 0; }
static int fk_wifi_poll(LvFind *o, int max)
{
    if (t_ms - fk_t0 < 1200) return -1;
    const LvFind F[] = {mk("HomeNet-5G", -52, 6, 3, 0, 0, 0, 1), mk("Flock-A7F21C", -61, 11, 3, 0, 0, 0, 2), mk("TELLO-5A21B3", -66, 1, 0, 0, 0, 0, 3),
                        mk("xfinitywifi", -70, 1, 0, 0, 0, 0, 4), mk("DIRECT-6B-HP OfficeJet", -73, 6, 3, 0, 0, 0, 5), mk("", -80, 149, 6, 0, 0, 0, 6),
                        mk("Tesla_Model3", -82, 36, 3, 0, 0, 0, 7), mk("NETGEAR42", -84, 9, 4, 0, 0, 0, 8), mk("CoffeeShop Guest", -88, 3, 0, 0, 0, 0, 9)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0; return n;
}
static int fk_ble_poll(LvFind *o, int max)
{
    if (t_ms - fk_t0 < 5000) return -1;
    const LvFind F[] = {mk("", -48, 0, 0, 0x004C, 0x12, 0, 10), mk("Flipper Zeph", -58, 0, 0, 0xFFFF, 0, 0, 11), mk("", -63, 0, 0, 0x0075, 0, 0xFD5A, 12),
                        mk("RB Meta 0A1F", -67, 0, 0, 0x01AB, 0, 0, 13), mk("HC-05", -71, 0, 0, 0xFFFF, 0, 0, 14), mk("Galaxy Watch6", -74, 0, 0, 0x0075, 0, 0, 15),
                        mk("Tile", -77, 0, 0, 0xFFFF, 0, 0xFEED, 16), mk("JBL Flip 6", -79, 0, 0, 0xFFFF, 0, 0, 17), mk("", -83, 0, 0, 0x004C, 0x10, 0, 18),
                        mk("[TV] Samsung Q60", -86, 0, 0, 0x0075, 0, 0, 19), mk("", -91, 0, 0, 0xFFFF, 0, 0, 20)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0; return n;
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
static void fk_set_flip(bool f) { printf("t=%u SET_FLIP %d (SIM NVS lkflip; firmware setRotation(%d))\n", t_ms, (int)f, f ? 0 : 2); }
/* finger at SCREEN (x,y) -> where it physically is on the native (rotation 0) panel: rot 2 = 180 deg */
static void scr_to_native(int x, int y, int *nx, int *ny) { if (lv_rot180) { *nx = 239 - x; *ny = 319 - y; } else { *nx = x; *ny = y; } }
/* simulated XPT2046 (CYD-like swapped + flipped axes, +-30 raw noise) and the fake NVS save */
static TouchModel g_tm = {"swap+flip (CYD-like)", 3820, 0, -11.2, 3760, -14.6, 0, 30, 2};
static bool fk_cal_save(const uint16_t c[8])
{
    printf("t=%u CAL SAVE c0..c7 = %u %u %u %u %u %u %u %u (SIM NVS)\n", t_ms, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
    LgfxCalRef L; L.setCalibrate(c); int worst = 0;
    for (int i = 0; i < 4; ++i) {
        int rx, ry, x, y, nx, ny; scr_to_native(LV_CAL_PT[i][0], LV_CAL_PT[i][1], &nx, &ny);
        g_tm.raw(nx, ny, &rx, &ry, false); L.convertRawXY(rx, ry, &x, &y);
        if (lv_rot180) { x = 239 - x; y = 319 - y; }   /* LovyanGFX convertRawXY r=2: affine to native, then flip both axes */
        const int d = abs(x - LV_CAL_PT[i][0]) > abs(y - LV_CAL_PT[i][1]) ? abs(x - LV_CAL_PT[i][0]) : abs(y - LV_CAL_PT[i][1]);
        if (d > worst) worst = d;
        printf("  chip %d at %d,%d -> lgfx maps to %d,%d\n", i + 1, LV_CAL_PT[i][0], LV_CAL_PT[i][1], x, y);
    }
    printf("  worst axis error %d px\n", worst);
    return true;
}
static const LvRadio FAKE = {fk_wifi_start, fk_wifi_poll, fk_ble_start, fk_ble_poll, fk_deauth_start, fk_deauth_poll, fk_stop, fk_sys, fk_save, fk_cal_save, fk_set_flip};
static bool g_down = false; static int g_fx, g_fy; static FILE *g_rec = nullptr;
static void dump(const char *path);
static void rec_frame(void);
static void tick1(const LkIo &io)
{
    for (int k = 0; k < 2; ++k) { t_ms += 15; int rx = 0, ry = 0; if (g_down) { int nx, ny; scr_to_native(g_fx, g_fy, &nx, &ny); g_tm.raw(nx, ny, &rx, &ry); } lv_cal_raw(g_down, rx, ry, t_ms); }
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
    while (t_ms < T) { t_ms = t_ms + 15 <= T ? t_ms + 15 : T; int rx = 0, ry = 0; if (g_down) { int nx, ny; scr_to_native(g_fx, g_fy, &nx, &ny); g_tm.raw(nx, ny, &rx, &ry); } lv_cal_raw(g_down, rx, ry, t_ms); }
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
    lv_press((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2);
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
        else if (!strcmp(cmd, "press")) { if (fscanf(sc, "%d %d", &g_fx, &g_fy) != 2) return 2; g_down = true; int rx, ry, nx, ny; scr_to_native(g_fx, g_fy, &nx, &ny); g_tm.raw(nx, ny, &rx, &ry); lv_cal_raw(true, rx, ry, t_ms); }
        else if (!strcmp(cmd, "release")) { g_down = false; lv_cal_raw(false, 0, 0, t_ms); }
        else if (!strcmp(cmd, "tickto")) { unsigned T; if (fscanf(sc, "%u", &T) != 1) return 2; tickto(io, T); }
        else if (!strcmp(cmd, "up")) { int ms; if (fscanf(sc, "%d", &ms) != 1) return 2; g_down = false; for (int e = 0; e < ms; e += 30) tick1(io); }
        else if (!strcmp(cmd, "recstart")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); g_rec = fopen(p, "wb"); }
        else if (!strcmp(cmd, "recstop")) { if (g_rec) fclose(g_rec); g_rec = nullptr; remove("/tmp/.sim_rec_part"); }
        else if (!strcmp(cmd, "pdown")) { int x, y; if (fscanf(sc, "%d %d", &x, &y) != 2) return 2; lv_press(x, y); printf("t=%u pdown %d,%d (flash)\n", t_ms, x, y); }
        else if (!strcmp(cmd, "audit")) {   /* item 9: every target on this screen, tapped at its centre on a scratch copy is not possible
                                               (static state), so list them; the 'auditrun' script taps each one for real */
            LvHit o[LV_MAXHIT]; const int n = lv_targets(o);
            printf("AUDIT scr=%d targets=%d\n", lv_scr, n);
            for (int i = 0; i < n; ++i) printf("  [%d] %-26s {%d,%d,%d,%d} act=%d arg=%d\n", i, o[i].name, o[i].r.x0, o[i].r.y0, o[i].r.x1, o[i].r.y1, o[i].act, o[i].arg);
        }
        else if (!strcmp(cmd, "heap50")) {  /* item 7: 50 WiFi+BLE scan cycles, heap (sim process) before/after */
            struct mallinfo2 m0 = mallinfo2(); size_t lo = m0.uordblks, hi = m0.uordblks;
            for (int c = 0; c < 50; ++c) {
                lv_job_start(LV_J_ALL, -1);
                while (lv_job != LV_J_NONE) tick1(io);
                struct mallinfo2 m = mallinfo2(); if (m.uordblks < lo) lo = m.uordblks; if (m.uordblks > hi) hi = m.uordblks;
                if (c % 10 == 9) printf("  cycle %d t=%u finds=%d heap_in_use=%zu\n", c + 1, t_ms, lv_nf, m.uordblks);
            }
            struct mallinfo2 m1 = mallinfo2();
            printf("HEAP50 start=%zu end=%zu min=%zu max=%zu delta=%ld finds=%d\n", m0.uordblks, m1.uordblks, lo, hi, (long)m1.uordblks - (long)m0.uordblks, lv_nf);
        }
        else if (!strcmp(cmd, "tap")) { int x, y; if (fscanf(sc, "%d %d", &x, &y) != 2) return 2; lv_press(x, y); const char *r = lv_tap(x, y); printf("t=%u tap %d,%d -> %s (scr=%d seg=%d)\n", t_ms, x, y, r, lv_scr, lv_play.seg); }
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
        else if (!strcmp(cmd, "dump")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); dump(p); printf("DUMP %s scr=%d seg=%d clipframe=%d t=%u\n", nm, lv_scr, lv_play.seg, lv_play.seg >= 0 && lv_play.shown >= 0 ? LK_SEG[lv_play.seg].f0 + lv_play.shown : -1, t_ms); }
        else if (!strcmp(cmd, "rec")) { /* rec N NAME: N ticks, one frame per tick appended to NAME.rgb */
            int n; char nm[64]; if (fscanf(sc, "%d %63s", &n, nm) != 2) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm);
            FILE *o = fopen(p, "wb"); fclose(o);
            for (int i = 0; i < n; ++i) { tick1(io); char q[520]; snprintf(q, sizeof q, "%s.part", p); dump(q); FILE *a = fopen(p, "ab"), *b = fopen(q, "rb"); static uint8_t buf[240 * 320 * 3]; size_t k = fread(buf, 1, sizeof buf, b); fwrite(buf, 1, k, a); fclose(a); fclose(b); remove(q); }
            printf("t=%u rec %s %d frames\n", t_ms, nm, n);
        }
        else if (!strcmp(cmd, "log")) { for (int k = (lv_tn < 20 ? lv_tn : 20) - 1; k >= 0; --k) printf("  | %s\n", lv_line_back(k)); }
    }
    printf("px pushed by overlays: %ld\n", g_px_pushed);
    return 0;
}
