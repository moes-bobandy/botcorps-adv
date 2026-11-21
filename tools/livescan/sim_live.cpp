/* Host sim for the cyd_livescan UI: the SAME src/live/lv_core.h the firmware runs, with zlib raw inflate
 * in place of ROM tinfl, an RGB565 framebuffer, and a FAKE radio (canned finds, clearly not real data).
 *   sim_live <script> <outdir>
 *   sim_live <script> <outdir> nocal   (power on with NO saved cal: CALbot captures first)
 * script lines: hold X Y MS (finger down on screen x,y -> simulated raw) | up MS | recstart NAME | recstop |
 *   tick N | tap X Y | type TEXT (taps the on-screen keys; '~' = ENTER) | dump NAME | theme N | log
 *   press X Y / release (finger down / up NOW: one raw sample at the current time, then the usual 15 ms samples)
 *   tickto T (advance to exactly T ms: 30 ms ticks, then 15 ms raw samples + one lv_tick at T)
 *   hmsched T0 (hunt meter: restart it now, fill from the locked V2 mock schedule relative to T0; 0 = live dBm)
 *   motion 0|1 (BG MOTION off / on, as the CONTROLbot chip sets it)
 *   termface a | termface b [PHASE]  (MOCK ONLY: overpaint TERMINALbot face (65,88)-(174,197)
 *     on the already-rendered locked page. a = 4x4 color chips + locked eyes; b = Matrix rain +
 *     green Matrix eyes. Not compiled into firmware.) */
#include <stdint.h>
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
static uint32_t g_tscr = 0;   /* v1.1 waitscr / tickrel */
#define BOTCORPS_CYD_LIVESCAN 1
#define LV_RD_HM_SIM 1   /* hunt meter test hook (hmsched): sim only, never in firmware */
#include "../../src/live/lv_core.h"
#include "../../src/live/lv_tapfilt.h"
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
#include "dev_model.h"   /* v1.2.17: device timeline model (devrec), host only */
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
    if (g_dev) { static uint16_t l[240]; for (int i = 0; i < n; ++i) l[i] = pal[idx[i]]; dev_blit(x, y, n, 1, l); }
    for (int i = 0; i < n; ++i) wput(y, x + i, pal[idx[i]]);
    dev_after_blit();
}
static void blit(int x, int y, int w, int h, const uint16_t *px)
{
    dev_blit(x, y, w, h, px);
    for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) wput(y + r, x + c, px[r * w + c]);
    g_px_pushed += w * h;
    dev_after_blit();
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
static uint32_t fk_ble_ms = 5000; static int fk_ovr_m5 = -1, fk_ovr_rssi = 0, fk_ble_n = 0, fk_holdpoll = 0;  /* scanhold: freeze in-flight poll (sim only, for HOLD stills) */
static int fk_ble_start(uint32_t ms) { fk_ble_ms = ms; fk_ble_n++; fk_starts++; if (fk_job) fk_overlap++; fk_job = 2; fk_t0 = t_ms; return 0; }
static int fk_deauth_start(uint32_t) { fk_starts++; if (fk_job) fk_overlap++; fk_job = 3; fk_t0 = t_ms; return 0; }
static int fk_wifi_poll(LvFind *o, int max)
{
    if (fk_holdpoll) return -1;   /* sim-only: freeze this pass */
    if (t_ms - fk_t0 < 1200) return -1;
    const LvFind F[] = {mk("HomeNet-5G", -52, 6, 3, 0, 0, 0, 1), mk("Flock-A7F21C", -61, 11, 3, 0, 0, 0, 2), mk("TELLO-5A21B3", -66, 1, 0, 0, 0, 0, 3),
                        mk("xfinitywifi", -70, 1, 0, 0, 0, 0, 4), mk("DIRECT-6B-HP OfficeJet", -73, 6, 3, 0, 0, 0, 5), mk("", -80, 149, 6, 0, 0, 0, 6),
                        mk("Tesla_Model3", -82, 36, 3, 0, 0, 0, 7), mk("NETGEAR42", -84, 9, 4, 0, 0, 0, 8), mk("CoffeeShop Guest", -88, 3, 0, 0, 0, 0, 9)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0;
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5) o[i].rssi = (int8_t)fk_ovr_rssi;   /* 'sig' command: SAMPLE dBm for a hunt test */
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5 && fk_ovr_rssi <= -127) { memmove(o + i, o + i + 1, (n - i - 1) * sizeof(LvFind)); --n; break; }   /* 1109 mock: sig M -127 = WiFi target missed this pass */
    if (lv_hunt == LV_H_EVILTWIN && n + 2 <= max) {   /* EX.DATA twin pair, only while this hunt is running (gate scripts never do) */
        o[n] = mk("CafeWiFi", -55, 6, 3, 0, 0, 0, 30);
        o[n + 1] = mk("CafeWiFi", -62, 6, 0, 0, 0, 0, 31);
        n += 2;
    }
    return n;
}
static int fk_ble_poll(LvFind *o, int max)
{
    if (fk_holdpoll) return -1;   /* sim-only: freeze this pass */
    if (t_ms - fk_t0 < fk_ble_ms) return -1;
    const LvFind F[] = {mk("", -48, 0, 0, 0x004C, 0x12, 0, 10), mk("Flipper Zeph", -58, 0, 0, 0xFFFF, 0, 0, 11), mk("", -63, 0, 0, 0x0075, 0, 0xFD5A, 12),
                        mk("RB Meta 0A1F", -67, 0, 0, 0x01AB, 0, 0, 13), mk("HC-05", -71, 0, 0, 0xFFFF, 0, 0, 14), mk("Galaxy Watch6", -74, 0, 0, 0x0075, 0, 0, 15),
                        mk("Tile", -77, 0, 0, 0xFFFF, 0, 0xFEED, 16), mk("JBL Flip 6", -79, 0, 0, 0xFFFF, 0, 0, 17), mk("", -83, 0, 0, 0x004C, 0x10, 0, 18),
                        mk("[TV] Samsung Q60", -86, 0, 0, 0x0075, 0, 0, 19), mk("", -91, 0, 0, 0xFFFF, 0, 0, 20)};
    int n = (int)(sizeof F / sizeof F[0]); if (n > max) n = max; memcpy(o, F, n * sizeof(LvFind)); fk_job = 0;
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5) o[i].rssi = (int8_t)fk_ovr_rssi;   /* 'sig' command: SAMPLE dBm for a hunt test */
    for (int i = 0; i < n; ++i) if (o[i].mac[5] == fk_ovr_m5 && fk_ovr_rssi <= -127) { memmove(o + i, o + i + 1, (n - i - 1) * sizeof(LvFind)); --n; break; }   /* scratch mock: sig M -127 = target gone */
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
static void fk_save(int t) { printf("t=%u NVS lktheme=%d (SIM)\n", t_ms, t); }   /* fw-3locks: log the theme save (was a no-op) */
static uint8_t fk_nvs[2][16 * 6]; static int fk_nvs_n[2] = {0, 0};   /* SIM NVS for radar rdign / rdwat */
static int fk_mac_load(const char *k, uint8_t *o, int max) { const int w = !strcmp(k, "rdwat"); int n = fk_nvs_n[w] < max ? fk_nvs_n[w] : max; memcpy(o, fk_nvs[w], n * 6); return n; }
static void fk_mac_save(const char *k, const uint8_t *m, int n) { const int w = !strcmp(k, "rdwat"); fk_nvs_n[w] = n; memcpy(fk_nvs[w], m, n * 6); printf("t=%u NVS %s = %d MACs (SIM)\n", t_ms, k, n); }
#if LV_BG_ENGINE
static int fk_bgs = 0, fk_bgc = 0;   /* SIM NVS lkbgs / lkbgc (background engine) */
static void fk_bg_save(int s, int c) { fk_bgs = s; fk_bgc = c; printf("t=%u NVS lkbgs=%d lkbgc=%d (SIM)\n", t_ms, s, c); }
#endif
static int fk_rot = 2;   /* the UI's setRotation: 2 = portrait USB at the bottom (default), 0 = ORIENT FLIP */
/* simulated XPT2046 (CYD-like swapped + flipped axes, +-30 raw noise) and the fake NVS save */
static TouchModel g_tm = {"swap+flip (CYD-like)", 3820, 0, -11.2, 3760, -14.6, 0, 30, 2};
/* v1.1: verbatim copy of the release-edge filter in src/live/lv_ui.cpp lv_read_tap (firmware-only file), fed with the calibrated
 * touch; millis() -> now. Used by rtap so a sim tap goes raw -> cal matrix -> this filter -> lv_tap like the CYD. */
/* v1.2.10: the firmware's own tap filter (src/live/lv_tapfilt.h), not a copy */
static LvTapFilt g_tf = {};
static bool sim_read_tap(bool down, int tx, int ty, uint32_t now, int *ox, int *oy) { lv_tf_sample(g_tf, down, tx, ty, now); return lv_tf_take(g_tf, ox, oy); }
/* v1.2.10 hardware frame-cost model for the Dojo (rtap only). framecost MS: each Dojo lv_tick takes MS of device time
 * (13 bands). bandtouch 0 = v1.2.9 firmware (touch read only between frames); 1 = v1.2.10 (read after every band,
 * exactly what lv_ui.cpp's ui_band_hook does). The finger is modelled by time, so a tap can begin and end inside one frame. */
static int g_framecost = 0, g_bandtouch = 1; static bool g_rt_on = false; static int g_rt_X, g_rt_Y; static uint32_t g_rt_end;
static int g_rt_seen_down = 0; static uint32_t g_rt_t0 = 0;
static void sim_finger(uint32_t now, bool *down, int *sx, int *sy);
static void sim_band_hook(void)
{
    if (!g_rt_on || !g_framecost || lv_scr != LV_SC_MO) return;
    t_ms += (uint32_t)(g_framecost / 13);
    if (!g_bandtouch) return;
    bool d; int x, y; sim_finger(t_ms, &d, &x, &y); if (d) ++g_rt_seen_down;
    lv_tf_sample(g_tf, d, x, y, t_ms);
}
static uint16_t g_cal8[8]; static bool g_cal_ok = false;   /* the c0..c7 CALbot saved (SIM NVS) */
static void sim_finger(uint32_t now, bool *down, int *sx, int *sy)   /* rtap's finger at device time now (raw XPT2046 -> saved cal -> rot flip) */
{
    *down = g_rt_on && now >= g_rt_t0 && now < g_rt_end; *sx = 0; *sy = 0;
    if (!*down) return;
    LgfxCalRef L; L.setCalibrate(g_cal8); int rx, ry; g_tm.raw(g_rt_X, g_rt_Y, &rx, &ry); L.convertRawXY(rx, ry, sx, sy); *sx = 239 - *sx; *sy = 319 - *sy;
}
static bool fk_cal_save(const uint16_t c[8])
{
    printf("t=%u CAL SAVE c0..c7 = %u %u %u %u %u %u %u %u (SIM NVS)\n", t_ms, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
    LgfxCalRef L; L.setCalibrate(c); int worst = 0;
    memcpy(g_cal8, c, sizeof g_cal8); g_cal_ok = true;   /* v1.1: rtap converts raw touches with exactly this saved matrix */
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
#if LV_CB   /* fw-3locks: CONTROLbot v6 panel hooks, logged (SIM NVS / backlight / speaker) */
static void fk_cb_save(const LvCbSet &c) { printf("t=%u NVS scan=%u vol=%u bri=%u chime=%u time=%u bar=%u tone=%u sleep=%u (SIM)\n", t_ms, c.scan, c.vol, c.bri, c.chime, c.time, c.bar, c.tone, c.sleep); }
static void fk_cb_bright(int pct) { printf("t=%u BACKLIGHT %d%% (duty %d/255) (SIM)\n", t_ms, pct, pct * 255 / 100); }
static void fk_cb_tone(int hz, int vol) { printf("t=%u SPEAKER %d Hz vol %d%% (SIM)\n", t_ms, hz, vol); }
#endif
static const LvRadio FAKE = {fk_wifi_start, fk_wifi_poll, fk_ble_start, fk_ble_poll, fk_deauth_start, fk_deauth_poll, fk_stop, fk_sys, fk_save, fk_cal_save, fk_set_rot, fk_mac_load, fk_mac_save
#if LV_BG_ENGINE
    , fk_bg_save
#endif
};
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

/* MOCK ONLY — never called from firmware. Overpaints the TERMINALbot face box after the locked
 * pager page has been rendered. Eyes stay in the locked 22x22 boxes at face-local (22,22)-(43,43)
 * and (66,22)-(87,43). */
#define SIM_TF_X0 65
#define SIM_TF_Y0 88
#define SIM_TF_S  110
static const int SIM_TF_EYE[2][4] = {{22, 22, 43, 43}, {66, 22, 87, 43}};
static void sim_tf_px(int x, int y, uint16_t c)
{
    if (x < SIM_TF_X0 || x > SIM_TF_X0 + SIM_TF_S - 1 || y < SIM_TF_Y0 || y > SIM_TF_Y0 + SIM_TF_S - 1) return;
    fb[y][x] = c;
}
static void sim_tf_a(void)
{
    static const uint32_t PAL[16] = {
        0xE03030, 0xF07018, 0xE0A010, 0xE8E030,
        0x88D020, 0x20A040, 0x149080, 0x20C8D8,
        0x2078E0, 0x2030C0, 0x5030D0, 0x9020C8,
        0xD02088, 0xE070A8, 0x8A5A28, 0xE8E8F0
    };
    for (int ly = 0; ly < SIM_TF_S; ++ly) for (int lx = 0; lx < SIM_TF_S; ++lx) {
        uint16_t c = 0;
        if (lx && ly && lx < SIM_TF_S - 1 && ly < SIM_TF_S - 1) {
            int gx = (lx - 1) / 27, gy = (ly - 1) / 27;
            if (gx > 3) gx = 3; if (gy > 3) gy = 3;
            c = lv_hex(PAL[gy * 4 + gx]);
        }
        sim_tf_px(SIM_TF_X0 + lx, SIM_TF_Y0 + ly, c);
    }
    for (int e = 0; e < 2; ++e)
        for (int y = SIM_TF_EYE[e][1]; y <= SIM_TF_EYE[e][3]; ++y)
            for (int x = SIM_TF_EYE[e][0]; x <= SIM_TF_EYE[e][2]; ++x)
                sim_tf_px(SIM_TF_X0 + x, SIM_TF_Y0 + y, 0);
}
static void sim_tf_glyph(int x0, int y0, char ch, uint16_t c)
{
    if (ch < 32 || ch > 126) ch = 48;
    const int g = ch - 32;
    for (int cx = 0; cx < 5; ++cx) {
        const uint8_t bits = LV_GLCD[g * 5 + cx];
        for (int cy = 0; cy < 7; ++cy) if ((bits >> cy) & 1) sim_tf_px(x0 + cx, y0 + cy, c);
    }
}
static void sim_tf_b(int phase)
{
    for (int ly = 0; ly < SIM_TF_S; ++ly)
        for (int lx = 0; lx < SIM_TF_S; ++lx)
            sim_tf_px(SIM_TF_X0 + lx, SIM_TF_Y0 + ly, 0);
    static const char GL[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#$%*+<>/|";
    const int ngl = (int)sizeof(GL) - 1, CW = 6, CH = 8, NCOL = 18;
    const uint16_t head = lv_hex(0xD8FFD8), mid = lv_hex(0x00FF41), dim = lv_hex(0x001800);
    for (int col = 0; col < NCOL; ++col) {
        const uint32_t s = 0xC0DEU ^ (uint32_t)(col * 2654435761u);
        const int speed = 1 + (int)((s >> 8) % 3);
        const int trail = 9 + (int)((s >> 3) % 7);
        const int period = SIM_TF_S + trail * CH + 24;
        const int off = (int)((s >> 16) % (uint32_t)period);
        const int head_y = (phase * speed + off) % period;
        const int x0 = SIM_TF_X0 + 1 + col * CW;
        for (int t = 0; t < trail; ++t) {
            const int gy = head_y - t * CH;
            if (gy > SIM_TF_S - 1 || gy + 6 < 0) continue;
            const unsigned k = (unsigned)(s + (unsigned)((gy + period) / CH) * 17u + (unsigned)col * 3u);
            const char ch = GL[k % (unsigned)ngl];
            const uint16_t c = t == 0 ? head : t == 1 ? mid : lv_mix(mid, dim, t, trail);
            sim_tf_glyph(x0, SIM_TF_Y0 + gy, ch, c);
        }
    }
    const uint16_t ehi = lv_hex(0x00FF55), emid = lv_hex(0x00B834), elo = lv_hex(0x003010), slit = lv_hex(0xD0FFD0);
    for (int e = 0; e < 2; ++e) {
        const int x0 = SIM_TF_X0 + SIM_TF_EYE[e][0], y0 = SIM_TF_Y0 + SIM_TF_EYE[e][1];
        const int x1 = SIM_TF_X0 + SIM_TF_EYE[e][2], y1 = SIM_TF_Y0 + SIM_TF_EYE[e][3];
        const int cy = (y0 + y1) / 2;
        for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
            uint16_t c = elo;
            const bool edge = (x == x0 || x == x1 || y == y0 || y == y1);
            const bool inner = (x == x0 + 1 || x == x1 - 1 || y == y0 + 1 || y == y1 - 1);
            if (edge) c = ehi; else if (inner) c = emid;
            if (y >= cy - 1 && y <= cy + 1 && x >= x0 + 5 && x <= x1 - 5) c = slit;
            if (y == cy && x >= x0 + 4 && x <= x1 - 4) c = ehi;
            fb[y][x] = c;
        }
    }
}

int main(int argc, char **argv)
{
    FILE *bf = fopen("src/lock/live/lock_blob_live.bin", "rb");
    if (!bf || argc < 3) { fprintf(stderr, "usage: sim_live script outdir (run from repo root)\n"); return 1; }
    blob.resize(LK_BLOB_SIZE);
    if (fread(blob.data(), 1, LK_BLOB_SIZE, bf) != LK_BLOB_SIZE) return 1;
    LkIo io{blob.data(), infl, lv_clip_draw, ibuf};
    /* fw-3locks: SIM_THEME=N boots with NVS lktheme=N (persistence test) */ lv_begin(&FAKE, blit, raw_draw, getenv("SIM_THEME") ? atoi(getenv("SIM_THEME")) : 0, 0, !(argc > 3 && !strcmp(argv[3], "nocal")));
#if LV_CB   /* fw-3locks: SIM_CB="scan,vol,bri,chime,time,bar,tone" = the CONTROLbot NVS values read at boot (persistence test) */
    if (getenv("SIM_CB")) { int v[7]; if (sscanf(getenv("SIM_CB"), "%d,%d,%d,%d,%d,%d,%d", v, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6) == 7)
        lv_cb_set = LvCbSet{(uint8_t)v[0], (uint8_t)v[1], (uint8_t)v[2], (uint8_t)v[3], (uint8_t)v[4], (uint8_t)v[5], (uint8_t)v[6]}; }
    lv_cb_apply(); lv_cb_save_hook = fk_cb_save; lv_cb_tone_hook = fk_cb_tone; lv_cb_bright_hook = fk_cb_bright; fk_cb_bright(lv_cb_set.bri);
    printf("t=%u CONTROLbot boot settings scan=%u vol=%u bri=%u chime=%u time=%u bar=%u tone=%u rain=%d rainmode=%d\n", t_ms, lv_cb_set.scan, lv_cb_set.vol, lv_cb_set.bri, lv_cb_set.chime, lv_cb_set.time, lv_cb_set.bar, lv_cb_set.tone, lv_rain, lv_rainmode);
#endif
    lv_tick(io, 0);                            /* power-on paint at t=0 (the firmware loop ticks at once too) */
    FILE *sc = fopen(argv[1], "r"); char cmd[32];
    while (fscanf(sc, "%31s", cmd) == 1) {
        if (!strcmp(cmd, "tick")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; for (int i = 0; i < n; ++i) tick1(io); }
#ifdef LV_MOCK_HC_DPAD
        else if (!strcmp(cmd, "mockpad")) { if (fscanf(sc, "%d %d", &lv_hc_dpad_held, &lv_dl_fsel) != 2) return 2; }   /* MOCK ONLY: held key + folder cursor */
#endif
        else if (!strcmp(cmd, "stick")) { int n, ms; if (fscanf(sc, "%d %d", &n, &ms) != 2) return 2; for (int i = 0; i < n; ++i) { t_ms += ms > 30 ? ms - 30 : 0; tick1(io); } }   /* v1.2.11 device-speed model: one frame every MS ms (a slow CYD frame) */
        else if (!strcmp(cmd, "hold")) { int ms; if (fscanf(sc, "%d %d %d", &g_fx, &g_fy, &ms) != 3) return 2; g_down = true; for (int e = 0; e < ms; e += 30) tick1(io); g_down = false; printf("t=%u hold %d,%d -> scr=%d cal_n=%d phase=%d msg=%d\n", t_ms, g_fx, g_fy, lv_scr, lv_cal_n, lv_cal_phase, lv_cal_msg); }
        else if (!strcmp(cmd, "press")) { if (fscanf(sc, "%d %d", &g_fx, &g_fy) != 2) return 2; g_down = true; int rx, ry; g_tm.raw(g_fx, g_fy, &rx, &ry); lv_cal_raw(true, rx, ry, t_ms); }
        else if (!strcmp(cmd, "release")) { g_down = false; lv_cal_raw(false, 0, 0, t_ms); }
        else if (!strcmp(cmd, "tickto")) { unsigned T; if (fscanf(sc, "%u", &T) != 1) return 2; tickto(io, T); }
        else if (!strcmp(cmd, "up")) { int ms; if (fscanf(sc, "%d", &ms) != 1) return 2; g_down = false; for (int e = 0; e < ms; e += 30) tick1(io); }
        else if (!strcmp(cmd, "recstart")) { char nm[64]; if (fscanf(sc, "%63s", nm) != 1) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); g_rec = fopen(p, "wb"); }
        else if (!strcmp(cmd, "recstop")) { if (g_rec) fclose(g_rec); g_rec = nullptr; remove("/tmp/.sim_rec_part"); }
        else if (!strcmp(cmd, "tap")) { int x, y; if (fscanf(sc, "%d %d", &x, &y) != 2) return 2; sim_use_pal(); const char *r = lv_tap(x, y); printf("t=%u tap %d,%d -> %s (scr=%d seg=%d)\n", t_ms, x, y, r, lv_scr, lv_play.seg); }
        else if (!strcmp(cmd, "waitscr")) {   /* v1.1 waitscr N: tick (30 ms) until lv_scr == N (max 5 s); remembers that time for tickrel */
            int n; if (fscanf(sc, "%d", &n) != 1) return 2; int k = 0; while (lv_scr != n && k++ < 167) tick1(io);
            g_tscr = t_ms; printf("t=%u waitscr %d -> scr=%d%s\n", t_ms, n, lv_scr, lv_scr == n ? "" : " (TIMEOUT)"); }
        else if (!strcmp(cmd, "skip")) { unsigned ms; if (fscanf(sc, "%u", &ms) != 1) return 2; t_ms += ms; }   /* v1.1: move the sim clock without a tick (align a reference run with rtap's sample clock) */
        else if (!strcmp(cmd, "tickrel")) { unsigned ms; if (fscanf(sc, "%u", &ms) != 1) return 2; tickto(io, g_tscr + ms); }   /* v1.1: tickto (waitscr time + ms) */
        else if (!strcmp(cmd, "rtap")) {   /* v1.1 rtap X Y [MS]: a finger on the glass at screen (X,Y) for MS (default 120) through the REAL path:
                                            * simulated XPT2046 raw (+-noise) -> LovyanGFX convertRawXY with the CALbot-saved c0..c7 + rot 2 flip
                                            * (= g_cyd.getTouch) -> sim_read_tap (copy of lv_ui.cpp lv_read_tap's filter) -> lv_tap. Samples every 5 ms,
                                            * lv_tick every 30 ms, like lv_ui_loop. */
            int X, Y, ms = 120; if (fscanf(sc, "%d %d", &X, &Y) != 2) return 2; { long p = ftell(sc); if (fscanf(sc, "%d", &ms) != 1) { fseek(sc, p, SEEK_SET); ms = 120; } }
            if (!g_cal_ok) { printf("t=%u rtap: no CALbot-saved cal (run the CALbot capture first)\n", t_ms); return 3; }
            int lead = -1; { long p = ftell(sc); if (fscanf(sc, "%d", &lead) != 1) { fseek(sc, p, SEEK_SET); lead = -1; } }
            /* lead >= 0 (device model): a frame starts NOW and the finger lands lead ms later, i.e. possibly mid-frame */
            const char *r = nullptr; int ox = -1, oy = -1, last_x = -1, last_y = -1;
            const uint32_t t0 = t_ms + (lead > 0 ? (uint32_t)lead : 0), t_end = t0 + ms; uint32_t next_tick = lead >= 0 ? t_ms : t_ms + 30;
            g_rt_on = true; g_rt_X = X; g_rt_Y = Y; g_rt_t0 = t0; g_rt_end = t_end; g_rt_seen_down = 0; int seen_loop = 0; lv_band_hook = sim_band_hook;
            if (lead >= 0) t_ms -= 5;
            for (int guard = 0; guard < 1200 && !r && t_ms < t_end + 3000; ++guard) {
                t_ms += 5;
                int sx = 0, sy = 0; bool down; sim_finger(t_ms, &down, &sx, &sy); if (down) { last_x = sx; last_y = sy; ++seen_loop; }
                g_down = down; g_fx = sx; g_fy = sy;
                if (sim_read_tap(down, sx, sy, t_ms, &ox, &oy)) { sim_use_pal(); r = lv_tap(ox, oy); }
                if (lv_scr != LV_SC_CAL) lv_touch(down, sx, sy);
                if ((int32_t)(t_ms - next_tick) >= 0) { if ((int32_t)(t_ms - next_tick) >= 30) next_tick = t_ms; next_tick += 30; sim_use_pal(); lv_tick(io, t_ms); rec_frame(); }
            }
            g_rt_on = false; lv_band_hook = nullptr;
            if (g_framecost) printf("t=%u rtap model framecost=%d bandtouch=%d: finger down %u..%u ms, down samples seen loop=%d band=%d\n", t_ms, g_framecost, g_bandtouch, t0, t_end, seen_loop, g_rt_seen_down);
            g_down = false;
            printf("t=%u rtap glass %d,%d %dms -> last raw->cal %d,%d -> tap %d,%d -> %s (scr=%d)\n", t_ms, X, Y, ms, last_x, last_y, ox, oy, r ? r : "(no tap)", lv_scr);
        }
        else if (!strcmp(cmd, "molive")) { mo_live_update(); printf("t=%u MOLIVE nf=%d job=%d tot=%d pillars=%d:", t_ms, lv_nf, lv_job, mo_live_tot, mo_live_np); for (int i = 0; i < MO_N; ++i) if (mo_live_n[i]) printf(" %s=%d@%d", MO_DEV[i].name, mo_live_n[i], mo_live_db[i]); printf("  tk='%s'\n", lv_tk_msg); }
        else if (!strcmp(cmd, "framecost")) { if (fscanf(sc, "%d", &g_framecost) != 1) return 2; printf("t=%u framecost %d ms per Dojo frame (device model)\n", t_ms, g_framecost); }
        else if (!strcmp(cmd, "bandtouch")) { if (fscanf(sc, "%d", &g_bandtouch) != 1) return 2; printf("t=%u bandtouch %d (%s)\n", t_ms, g_bandtouch, g_bandtouch ? "v1.2.10: touch read after every band" : "v1.2.9: touch read between frames only"); }
        else if (!strcmp(cmd, "rhold")) {   /* finger down on (x,y) for ms, then the release-edge tap, as lv_ui.cpp does */
            int x, y, ms; if (fscanf(sc, "%d %d %d", &x, &y, &ms) != 3) return 2; g_fx = x; g_fy = y; g_down = true;
            for (int e = 0; e < ms; e += 30) tick1(io);
            g_down = false; tick1(io); sim_use_pal(); const char *r = lv_tap(x, y); printf("t=%u rhold %d,%d %dms -> %s (scr=%d)\n", t_ms, x, y, ms, r, lv_scr);
        }
        else if (!strcmp(cmd, "hmsched")) {   /* hmsched T0: restart the hunt meter now and replay the locked V2 mock's fill schedule from T0 (0 = live dBm) */
            unsigned T; if (fscanf(sc, "%u", &T) != 1) return 2;
#if LV_RD_CHARGE && LV_RD_HUNT_METER
            lv_rd_hm_sim_t0 = T; lv_rd_hm_on = false;
#endif
        }
        else if (!strcmp(cmd, "sig")) { if (fscanf(sc, "%d %d", &fk_ovr_m5, &fk_ovr_rssi) != 2) return 2; }
        else if (!strcmp(cmd, "scanhold")) { fk_holdpoll = 1; printf("t=%u scanhold (SIM: radio poll frozen)\n", t_ms); }
        else if (!strcmp(cmd, "infodump")) {   /* infodump F NAME: tick to INFO frame F the same way v2 did (tickto T-30, tick 1, dump) */
            int f; char nm[64]; if (fscanf(sc, "%d %63s", &f, nm) != 2) return 2;
            unsigned T = lv_rd_info_t + (unsigned)((f * 100 + 2) / 3);   /* t0 + ceil(f*100/3) */
            if (T >= 30) tickto(io, T - 30);
            tick1(io);
            char path[512]; snprintf(path, sizeof path, "%s/%s.rgb", argv[2], nm); dump(path);
            printf("DUMP %s infodump f=%d t=%u info_t=%u frame=%d hunt=%d miss=%d rssi=%d\n",
                   nm, f, t_ms, lv_rd_info_t, (int)((uint32_t)(t_ms - lv_rd_info_t) * 3u / 100u),
                   lv_rd_hunting, lv_rd_miss, lv_rd_info_f.rssi);
        }
#if LV_HC_GAME
        else if (!strcmp(cmd, "hctowers")) {   /* MOCK 1111 game: the fake nodes, their heat, the target and the finger boxes this frame */
            printf("HC t=%u phase=%d round=%d n=%d target=NODE %02d probes=%d heat=%d prev=%d score=%d best=%d\n", t_ms, lv_hc_phase, lv_hc_round, lv_hc_n, lv_hc_tgt + 1, lv_hc_probes, lv_hc_heat, lv_hc_prev, lv_hc_score, lv_hc_best);
            for (int k = 0; k < lv_hc_nhb; ++k) { const LvHcHit &h = lv_hc_hb[k]; printf("  HCT NODE %02d heat=%d Z=%.2f hit={%d,%d,%d,%d} cx=%d%s\n", h.i + 1, lv_hc_tgt >= 0 ? lv_hc_heat_of(h.i) : -1, h.Z, h.xa, h.ya, h.xb, h.yb, h.cx, h.i == lv_hc_tgt ? "  <TARGET>" : ""); }
        }
#endif
#if LV_DL_GAME
        else if (!strcmp(cmd, "dllevel")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; lv_dl_round = n; lv_dl_try = 0; lv_dl_enter(); printf("t=%u DL SIM shortcut: level %d intro\n", t_ms, n); }   /* MOCK 1112: jump to a level (renders only) */
        else if (!strcmp(cmd, "dlboot")) { lv_go(LV_SC_DL); if (lv_scr != LV_SC_DL) lv_go(LV_SC_DL); lv_dl_boot(); printf("t=%u DL SIM shortcut: slot intro (EXAMPLE DATA)\n", t_ms); }
        else if (!strcmp(cmd, "dlslot")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; if (n>=1 && n<=LV_DL_NSLOT) lv_dl_slot_i = n-1; printf("t=%u DL SIM shortcut: select slot %d\n", t_ms, lv_dl_slot_i+1); }
        else if (!strcmp(cmd, "dlapply")) { lv_dl_apply_slot(lv_dl_slot_i); printf("t=%u DL SIM shortcut: apply slot %d phase=%d\n", t_ms, lv_dl_slot_i+1, lv_dl_phase); }
        else if (!strcmp(cmd, "dlpause")) { if (lv_dl_phase != LV_DL_PLAY) { if (lv_dl_phase != LV_DL_PLAY) lv_dl_play(); } lv_dl_tw = -1; lv_dl_open_pause(); printf("t=%u DL SIM shortcut: pause\n", t_ms); }
        else if (!strcmp(cmd, "dlsave")) { if (lv_dl_phase != LV_DL_PAUSE && lv_dl_phase != LV_DL_PLAY) lv_dl_play(); if (lv_dl_phase == LV_DL_PLAY) lv_dl_open_pause(); lv_dl_do_save(); printf("t=%u DL SIM shortcut: save slot %d\n", t_ms, lv_dl_slot_i+1); }
        else if (!strcmp(cmd, "dlopen")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; if (lv_dl_phase != LV_DL_PLAY) lv_dl_play(); if (n>=1 && n<=LV_DL_NT) lv_dl_open_tower(n-1); printf("t=%u DL SIM shortcut: open tower %d\n", t_ms, n); }
#if LV_RD_INFO_TOWER_PULL
        else if (!strcmp(cmd, "infoscr")) {   /* HOST-SIM MOCK: open the original INFO screen on the locked/selected node */
            int i = lv_rd_sel; if (i < 0) i = lv_rd_find(lv_rd_lockmac); if (i < 0 && lv_nf) i = 0;
            if (i >= 0) { if (!lv_rd_lock) lv_rd_lock_on(i); lv_rd_info_open(i); }
            printf("t=%u infoscr i=%d info=%d\n", t_ms, i, (int)lv_rd_info);
        }
#endif
        else if (!strcmp(cmd, "dlnf")) { int n; if (fscanf(sc, "%d", &n) != 1) return 2; lv_dl_nf_force = n; lv_dl_enter(); printf("t=%u DL SIM feasibility: %d files a tower (not the progression)\n", t_ms, n); }
        else if (!strcmp(cmd, "dlstate")) {   /* MOCK 1112 HACKbot game: phase, trace, files (FAKE game data), finger boxes this frame */
            printf("DL t=%u phase=%d round=%d tower=%d cam=%d nf=%d need=%d got=%d trace=%d wrong=%d dls=%d score=%d best=%d dl=%d deny=%d camx=%.3f slot=%d menu=%d sd=%d\n", t_ms, lv_dl_phase, lv_dl_round, lv_dl_tw + 1, lv_dl_cam_mode, lv_dl_nf, lv_dl_need, lv_dl_got, lv_dl_trace, lv_dl_wrong, lv_dl_dls, lv_dl_score, lv_dl_best, lv_dl_pf + 1, (int)lv_dl_deny, lv_dl_cam_at, lv_dl_slot_i + 1, lv_dl_menu, (int)lv_dl_sd_ok);
            for (int i = 0; i < LV_DL_NSLOT; ++i) { const LvDlSave &s = lv_dl_slot[i]; printf("  DLSL %d %s L%d SCORE %d TOWERS %d/%d FILES %d/%d\n", i+1, s.used ? "USED" : "NEW", s.level, (int)s.score, s.towers, LV_DL_NT, s.files_got, s.files_need); }
            for (int t = 0; t < LV_DL_NT; ++t) { printf("  DLF T%d%s:", t + 1, lv_dl_seen[t] ? "*" : ""); for (int k = 0; k < lv_dl_nf; ++k) { char sz[12]; lv_dl_size(lv_dl_fo[t][k].mb10, sz, sizeof sz); printf(" [%d %s %s%s s=%d]", k + 1, lv_dl_name(lv_dl_fo[t][k]), sz, lv_dl_fo[t][k].tgt >= 0 ? " TARGET" : lv_dl_fo[t][k].la ? " LOOKALIKE" : "", lv_dl_fo[t][k].st); } printf("\n"); }
            for (int k = 0; k < lv_dl_need; ++k) { char sz[12]; lv_dl_size(lv_dl_tmb[k], sz, sizeof sz); printf("  DLTG %d %s %s T%d F%d got=%d\n", k + 1, LV_DL_NAMES[lv_dl_tnm[k]], sz, lv_dl_ttw[k] + 1, lv_dl_tf[k] + 1, (int)lv_dl_tgot[k]); }
            for (int k = 0; k < lv_hc_nhb; ++k) { const LvHcHit &h = lv_hc_hb[k]; printf("  DLT T%d Z=%.2f hit={%d,%d,%d,%d} cx=%d\n", h.i + 1, h.Z, h.xa, h.ya, h.xb, h.yb, h.cx); }
            for (int k = 0; k < lv_dl_ntag; ++k) printf("  DLTAG T%d={%d,%d,%d,%d} drawn_open=%d\n", lv_dl_tags[k].i + 1, lv_dl_tags[k].x0, lv_dl_tags[k].y0, lv_dl_tags[k].x1, lv_dl_tags[k].y1, lv_dl_tag_tw + 1);
            for (int i = 0; i < lv_dl_nf; ++i) { const LvRect r = lv_dl_ftap(i); printf("  DLFT F%d={%d,%d,%d,%d}\n", i + 1, r.x0, r.y0, r.x1, r.y1); }
        }
#endif
        else if (!strcmp(cmd, "towers")) { lv_rd_layout(); for (int n = 0; n < lv_rd_n; ++n) { const LvTower &T = lv_rd_tw[n]; printf("  tower f%d %s %d dBm m5=%d x%d-%d y%d-%d\n", T.i, lv_f[T.i].name, lv_f[T.i].rssi, lv_f[T.i].mac[5], T.x0, T.x1, T.byt, T.yb); } }
#if LV_BG_ENGINE
        else if (!strcmp(cmd, "bgstate")) printf("t=%u BG rain=%d style=%d col=%d nvs lkbgs=%d lkbgc=%d\n", t_ms, lv_rain, lv_bgstyle, lv_raincol, fk_bgs, fk_bgc);
        else if (!strcmp(cmd, "bgset")) { int a, b; if (fscanf(sc, "%d %d", &a, &b) != 2) return 2; lv_rain = a < LV_NBGSTYLE; lv_bgstyle = a < LV_NBGSTYLE ? a : 0; lv_raincol = b; lv_dirty_ctl = true; }   /* direct state set, same timing as a no-op (screen-isolation tests) */
        else if (!strcmp(cmd, "bgboot")) { lv_rain = fk_bgs < LV_NBGSTYLE; lv_bgstyle = fk_bgs < LV_NBGSTYLE ? fk_bgs : 0; lv_raincol = fk_bgc < LV_NBGCOL ? fk_bgc : 0; printf("t=%u BG boot-load from SIM NVS -> rain=%d style=%d col=%d\n", t_ms, lv_rain, lv_bgstyle, lv_raincol); }
#endif
        else if (!strcmp(cmd, "radarstate")) printf("t=%u RADAR sel=%d lock=%d hunt=%d reads=%d trend=%d miss=%d ign=%d wat=%d job=%d tk='%s'\n", t_ms, lv_rd_sel, lv_rd_lock, lv_rd_hunting, lv_rd_reads, lv_rd_trend, lv_rd_miss, lv_rd_nign, lv_rd_nwat, lv_job, lv_tk_msg);
        else if (!strcmp(cmd, "theme")) { int t; if (fscanf(sc, "%d", &t) != 1) return 2;
#if LV_CB
            lv_set_theme_any(t); lv_cb_dirty = true; lv_pg_dirty = true;
#else
            lv_set_theme(t);
#endif
        }
        else if (!strcmp(cmd, "page")) { int p; if (fscanf(sc, "%d", &p) != 1) return 2; lv_pg_set_page(p); lv_pg_dirty = true; }
#if LV_CB
#if LV_CB
        else if (!strcmp(cmd, "cblauncher")) { if (fscanf(sc, "%d", &lv_cb_has_launcher) != 1) return 2; lv_cb_panel_dirty = true; }   /* v1.2.12 SIM: pretend a Launcher is (1) / is not (0) in the test partition */
        else if (!strcmp(cmd, "cbalert")) { lv_event(true, "SIM ALERT (CHIME TEST)"); }   /* fw-3locks: an alert event, as a hunt match / deauth raises */
#endif
        else if (!strcmp(cmd, "controlbot")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; lv_set_theme_any(LV_CB_FW[k]); lv_cb_dirty = true; }   /* fw-3locks: CONTROLbot v6 slot k (locked order) */
        else if (!strcmp(cmd, "tkstill")) {   /* fw-3locks: redraw the firmware ticker at the lock still's clock (now off alert), default text, then restore lv_now */
            unsigned nw; int off, al; if (fscanf(sc, "%u %d %d", &nw, &off, &al) != 3) return 2;
            const uint32_t keep = lv_now; lv_now = nw; lv_tk_off = off; lv_tk_alert = al != 0; snprintf(lv_tk_msg, sizeof lv_tk_msg, "%s", "NO THREATS - TAP SCAN");
            LvTickCtx c{LV_R_TICK.x0, LV_R_TICK.x1}; lv_render(lv_cb_tint_active() ? lv_blit_hw : lv_blit, LV_R_TICK, lv_scene_ticker, &c); lv_now = keep; }
#endif
        else if (!strcmp(cmd, "motion")) { int m; if (fscanf(sc, "%d", &m) != 1) return 2; lv_motion = m != 0; }   /* BG MOTION (CONTROLbot chip) without the CTL tap */
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
        else if (!strcmp(cmd, "devrec")) { /* v1.2.17 devrec N NAME: N frames on the DEVICE timeline (dev_model.h). NAME.rgb = each finished
            * frame (same pixels the plain sim draws), NAME.cam.rgb = the panel every 20 ms of device time, NAME.dev.txt = per-frame cost */
            int n; char nm[64]; if (fscanf(sc, "%d %63s", &n, nm) != 2) return 2;
            char p[512], pc[512], pt[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm); snprintf(pc, sizeof pc, "%s/%s.cam.rgb", argv[2], nm); snprintf(pt, sizeof pt, "%s/%s.dev.txt", argv[2], nm);
            FILE *fo = fopen(p, "wb"), *fc = fopen(pc, "wb"), *ft = fopen(pt, "w");
            g_dev_k = getenv("SIM_DEV_K") ? atof(getenv("SIM_DEV_K")) : 1.0; g_dev = true; memcpy(g_pb, fb, sizeof g_pb); g_dev_ev.clear(); g_dev_ev_i = 0;
            double E = t_ms * 1000.0, next = E, cam = E, last_s = -1; g_dev_dma = E;
            static uint8_t row[240 * 3];
            auto put = [&](FILE *f, uint16_t (*b)[240]) { for (int y = 0; y < 320; ++y) { for (int x = 0; x < 240; ++x) { uint16_t v = b[y][x]; int r = (v >> 11) & 31, g = (v >> 5) & 63, bl = v & 31;
                row[x * 3] = (r << 3) | (r >> 2); row[x * 3 + 1] = (g << 2) | (g >> 4); row[x * 3 + 2] = (bl << 3) | (bl >> 2); } fwrite(row, 1, sizeof row, f); } };
            for (int i = 0; i < n; ++i) {
                double now = E > next ? E : next; now = (double)(long)(now / 1000.0 + 0.999999) * 1000.0;   /* millis() granularity */
                if (now - next >= 30000.0) next = now;
                next += 30000.0;
                t_ms = (uint32_t)(now / 1000.0);
                { int rx = 0, ry = 0; if (g_down) g_tm.raw(g_fx, g_fy, &rx, &ry); lv_cal_raw(g_down, rx, ry, t_ms); }
                sim_use_pal(); lv_touch(g_down, g_fx, g_fy);
                g_dev_cpu = now; g_dev_cpu_acc = 0; const long px0 = g_dev_px; const int w0 = g_dev_nwin; dev_mark();
                lv_tick(io, t_ms);
                dev_cpu(); E = g_dev_cpu > g_dev_dma ? g_dev_cpu : g_dev_dma;
                fprintf(ft, "f=%d start=%.3f ms=%.3f cpu=%.3f step=%.3f px=%ld win=%d\n", i, now / 1000.0, (E - now) / 1000.0, g_dev_cpu_acc / 1000.0, last_s < 0 ? 0.0 : (now - last_s) / 1000.0, g_dev_px - px0, g_dev_nwin - w0);
                last_s = now;
                put(fo, fb);
                while (cam <= E) { dev_panel_at(cam); put(fc, g_cam); cam += LV_DEV_CAM_MS * 1000.0; }
            }
            g_dev = false; fclose(fo); fclose(fc); fclose(ft); t_ms = (uint32_t)(E / 1000.0);
            printf("t=%u devrec %s %d frames\n", t_ms, nm, n);
        }
        else if (!strcmp(cmd, "rec")) { /* rec N NAME: N ticks, one frame per tick appended to NAME.rgb */
            int n; char nm[64]; if (fscanf(sc, "%d %63s", &n, nm) != 2) return 2; char p[512]; snprintf(p, sizeof p, "%s/%s.rgb", argv[2], nm);
            FILE *o = fopen(p, "wb"); fclose(o);
            for (int i = 0; i < n; ++i) { tick1(io); char q[520]; snprintf(q, sizeof q, "%s.part", p); dump(q); FILE *a = fopen(p, "ab"), *b = fopen(q, "rb"); static uint8_t buf[240 * 320 * 3]; size_t k = fread(buf, 1, sizeof buf, b); fwrite(buf, 1, k, a); fclose(a); fclose(b); remove(q); }
            printf("t=%u rec %s %d frames\n", t_ms, nm, n);
        }
        else if (!strcmp(cmd, "backface")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; lv_backbot_face = k & 3; lv_go(lv_scr); for (int i = 0; i < 4; ++i) tick1(io); printf("backface %d\n", lv_backbot_face); }
#if LV_MO_INSIDE || LV_CT_INSIDE || LV_TR_INSIDE
        else if (!strcmp(cmd, "itwstate")) { itw_dump(); }
#endif
        else if (!strcmp(cmd, "go")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; lv_go(k); }
        else if (!strcmp(cmd, "mofold")) { int k; if (fscanf(sc, "%d", &k) != 1) return 2; mo_term_fold = k; printf("t=%u mo_term_fold=%d nfold=%d termj=%d\n", t_ms, mo_term_fold, mo_nfold(), mo_term_j()); }
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
        else if (!strcmp(cmd, "termface")) {   /* MOCK ONLY: overpaint the TERMINALbot face box; does not touch firmware */
            char kind[8]; if (fscanf(sc, "%7s", kind) != 1) return 2;
            int phase = (int)t_ms; long pos = ftell(sc); int v;
            if (fscanf(sc, "%d", &v) == 1) phase = v; else fseek(sc, pos, SEEK_SET);
            if (kind[0] == 'a' || kind[0] == 'A') sim_tf_a();
            else if (kind[0] == 'b' || kind[0] == 'B') sim_tf_b(phase);
            else return 2;
            printf("t=%u termface %s phase=%d\n", t_ms, kind, phase);
        }
        else if (!strcmp(cmd, "log")) { for (int k = (lv_tn < 20 ? lv_tn : 20) - 1; k >= 0; --k) printf("  | %s\n", lv_line_back(k)); }
    }
    printf("px pushed by overlays: %ld\n", g_px_pushed);
    printf("SHADOW peak %d of %d bytes\n", lv_shadow_peak, LV_SHADOW_CAP);
    if (g_dead) printf("DEAD TARGETS: %d\n", g_dead);
    return 0;
}
