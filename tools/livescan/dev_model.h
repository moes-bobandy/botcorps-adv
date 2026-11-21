/* dev_model.h - HOST SIM ONLY (never in firmware). v1.2.17 pull-up glitch probe (Mobandy 2026-11-20 3:5x PM ET).
 * The plain sim renders a frame, then shows it: every GIF frame is a finished frame at a fixed 30 ms step, so a frame that
 * costs 300 ms on the CYD looks the same as one that costs 3 ms. This model replays the firmware's real timeline instead:
 *   CPU   : host thread-CPU time of each band x SIM_DEV_K (calibrated: idle GIBSON radar = the QEMU icount 16.0 ms/frame,
 *           docs/v1.2.12/qemu_tour_v1.2.12.txt), plus the in-place byte swap ui_blit does on the device.
 *   SPI   : 40 MHz HSPI write (lgfx_ili9341.h freq_write), 16 bit/px, + LV_DEV_WIN_US per address window.
 *   DMA   : lv_ui.cpp ui_blit: waitDMA() for the previous band, then pushImageDMA async; the next band computes meanwhile
 *           (ping-pong halves); lv_render drains at the end (lv_band_drain), ui_live_end waits again.
 *   LOOP  : lv_ui_loop's 30 ms deadline (catch-up bound: next = now after a >= 30 ms overrun), lv_tick(now = frame start).
 *   PANEL : a second framebuffer that only receives each band when its SPI transfer has (partly) finished, sampled by a
 *           "camera" every LV_DEV_CAM_MS of device time = what the eye sees on the CYD (half-drawn frames included).
 * The panel never feeds back into rendering (there is no MISO: lgfx_ili9341.h pin_miso = -1), exactly as on the board. */
#pragma once
#include <time.h>
#include <vector>
#ifndef LV_DEV_WIN_US
#define LV_DEV_WIN_US 8.0
#endif
#define LV_DEV_CAM_MS 20.0
struct DevEv { double t0, t1; int x, y, w, h, gid; std::vector<uint16_t> px; };
static int g_pr_open = 0, g_pr_gid = 0;   /* Dojo coherent present: latch the whole frame at once */
static bool g_dev = false; static double g_dev_k = 1.0, g_dev_cpu = 0, g_dev_dma = 0; static timespec g_dev_mark;
static std::vector<DevEv> g_dev_ev; static size_t g_dev_ev_i = 0; static uint16_t g_pb[320][240];
static long g_dev_px = 0; static int g_dev_nwin = 0;
static double dev_host_us(void) { timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return t.tv_sec * 1e6 + t.tv_nsec / 1e3; }
static double g_dev_mark_us = 0;
static void dev_mark(void) { g_dev_mark_us = dev_host_us(); }
static double g_dev_cpu_acc = 0;   /* device CPU us spent rendering this frame (excludes DMA waits) */
static void dev_cpu(void) { const double h = dev_host_us(); g_dev_cpu += (h - g_dev_mark_us) * g_dev_k; g_dev_cpu_acc += (h - g_dev_mark_us) * g_dev_k; g_dev_mark_us = h; }
static void dev_blit(int x, int y, int w, int h, const uint16_t *px)
{
    if (!g_dev) return;
    dev_cpu();
    const int n = w * h;
    g_dev_cpu += n * 0.0083;                         /* ui_blit's in-place 32-bit byte swap (~2 cyc/px @ 240 MHz) */
    if (g_dev_cpu < g_dev_dma) g_dev_cpu = g_dev_dma; /* waitDMA(): the previous band's transfer must finish first */
    DevEv e; e.t0 = g_dev_cpu; e.t1 = g_dev_cpu + LV_DEV_WIN_US + n * 0.4; e.x = x; e.y = y; e.w = w; e.h = h; e.gid = g_pr_open; e.px.assign(px, px + n);
    g_dev_dma = e.t1; g_dev_px += n; ++g_dev_nwin;
    g_dev_ev.push_back(std::move(e));
}
static void dev_after_blit(void) { if (g_dev) dev_mark(); }   /* the sim's own framebuffer copy is not device time */
/* panel state at device time T into g_cam.
 * Ungrouped transfers (gid==0): finished windows land whole; the one in flight fills row by row (GRAM order).
 * Grouped transfers (lv_present_begin/end): the previous completed image stays on the panel until the group's last
 * SPI byte has landed, then every window in the group is latched at once. Mid-burst samples are never a mix of
 * old and new pillar geometry. That is the idle no-tear / no-split contract (Mobandy 12:00 PM ET). */
static uint16_t g_cam[320][240];
static void dev_apply_ev(uint16_t dst[320][240], const DevEv &e, int rows)
{
    if (rows > e.h) rows = e.h;
    if (rows < 0) rows = 0;
    for (int r = 0; r < rows; ++r) memcpy(&dst[e.y + r][e.x], &e.px[r * e.w], e.w * 2);
}
static void dev_present_begin(void)
{
    if (!g_dev) return;
    dev_cpu();
    g_pr_open = ++g_pr_gid;
}
static void dev_present_end(void)
{
    if (!g_dev || !g_pr_open) return;
    dev_cpu();
    g_pr_open = 0;
}
static void dev_panel_at(double T)
{
    while (g_dev_ev_i < g_dev_ev.size()) {
        const DevEv &e0 = g_dev_ev[g_dev_ev_i];
        if (e0.gid != 0) {
            size_t j = g_dev_ev_i;
            double t1 = e0.t1;
            const int gid = e0.gid;
            while (j < g_dev_ev.size() && g_dev_ev[j].gid == gid) {
                if (g_dev_ev[j].t1 > t1) t1 = g_dev_ev[j].t1;
                ++j;
            }
            if (g_pr_open == gid || T < t1) break;   /* group still open, or last byte not yet landed */
            for (size_t k = g_dev_ev_i; k < j; ++k) dev_apply_ev(g_pb, g_dev_ev[k], g_dev_ev[k].h);
            g_dev_ev_i = j;
            continue;
        }
        if (e0.t1 > T) break;
        dev_apply_ev(g_pb, e0, e0.h);
        ++g_dev_ev_i;
    }
    memcpy(g_cam, g_pb, sizeof g_cam);
    if (g_dev_ev_i < g_dev_ev.size() && g_dev_ev[g_dev_ev_i].t0 < T && g_dev_ev[g_dev_ev_i].gid == 0) {
        const DevEv &e = g_dev_ev[g_dev_ev_i];
        const int rows = (int)((T - e.t0) / (e.t1 - e.t0) * e.h);
        dev_apply_ev(g_cam, e, rows);
    }
}
