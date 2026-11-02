/* CALbot accuracy check (host). Drives the SAME src/live/lv_core.h CALbot state machine with simulated
 * hold-to-capture taps on the four corner chips (32 noisy raw samples each, 15 ms apart), takes the c0..c7
 * it hands to cal_save (what the firmware writes to NVS), runs them through a verbatim copy of LovyanGFX's
 * setCalibrate + convertRawXY (tools/livescan/lgfx_cal_ref.h), and measures where noise-free taps land.
 *   g++ -O2 -std=gnu++17 -o /tmp/cal_accuracy tools/livescan/cal_accuracy.cpp && /tmp/cal_accuracy */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#define BOTCORPS_CYD_LIVESCAN 1
#include "../../src/live/lv_core.h"
#include "lgfx_cal_ref.h"
#include "touch_model.h"

static uint16_t g_c8[8]; static int g_saves;
static bool save(const uint16_t c[8]) { memcpy(g_c8, c, sizeof g_c8); ++g_saves; return true; }
static void nob(int, int, int, int, const uint16_t *) {}
static void nod(int, int, int, const uint8_t *) {}
static int z0(void) { return 0; } static int zp(LvFind *, int) { return 0; } static int z1(uint32_t) { return 0; }
static int zd(uint32_t *, uint32_t *, uint8_t *) { return 0; } static void zs(void) {}
static void zy(int, char *o, int n) { if (n) o[0] = 0; } static void zt(int) {}
static const LvRadio R = {z0, zp, z1, zp, z1, zd, zs, zy, zt, save};
static int infl(const uint8_t *, uint32_t, uint8_t *, uint32_t) { return -1; }

static uint32_t t;
static void step(LkIo &io, TouchModel &m, bool down, double sx, double sy, int ms)
{
    for (int e = 0; e < ms; e += 15) {
        t += 15; int rx = 0, ry = 0; if (down) m.raw(sx, sy, &rx, &ry);
        lv_cal_raw(down, rx, ry, t);
        if (t % 30 == 0) lv_tick(io, t);
    }
}
int main(void)
{
    TouchModel M[] = {
        {"nominal  rx~x ry~y       ", 300, 3600.0 / 239, 0, 300, 0, 3600.0 / 319, 30, 1},
        {"swap+flip (CYD-like)     ", 3820, 0, -11.2, 3760, -14.6, 0, 30, 2},
        {"rotated 2deg + shear     ", 380, 14.2, 0.55, 260, -0.48, 11.1, 30, 3},
        {"narrow span, +-60 noise  ", 900, 9.0, 0.3, 700, 0.2, 8.1, 60, 4},
    };
    static uint8_t ib[16]; LkIo io{nullptr, infl, lv_clip_draw, ib};
    double worst_all = 0; int runs = 0;
    for (auto &m0 : M) {
        double wmax = 0, wsum = 0, cmax = 0; int wn = 0, axmax = 0;
        for (int seed = 0; seed < 50; ++seed) {
            TouchModel m = m0; m.seed = m0.seed * 7919u + seed * 104729u;
            t = 1000; g_saves = 0; lv_begin(&R, nob, nod, 0, t, false);
            for (int i = 0; i < 4; ++i) {
                step(io, m, false, 0, 0, 90);
                step(io, m, true, LV_CAL_PT[i][0], LV_CAL_PT[i][1], 700);
                step(io, m, false, 0, 0, 90);
            }
            if (g_saves != 1 || lv_cal_phase != LV_CAL_OK) { printf("FAIL %s seed %d: saves=%d phase=%d\n", m.name, seed, g_saves, lv_cal_phase); return 1; }
            LgfxCalRef L; L.setCalibrate(g_c8);
            for (int i = 0; i < 4; ++i) {           /* the four chip centres */
                int rx, ry, x, y; m.raw(LV_CAL_PT[i][0], LV_CAL_PT[i][1], &rx, &ry, false); L.convertRawXY(rx, ry, &x, &y);
                const double d = hypot(x - LV_CAL_PT[i][0], y - LV_CAL_PT[i][1]); if (d > cmax) cmax = d;
            }
            for (int gy = 0; gy <= 10; ++gy) for (int gx = 0; gx <= 8; ++gx) {   /* 9x11 grid incl. the true corners */
                const int sx = gx * 239 / 8, sy = gy * 319 / 10; int rx, ry, x, y;
                m.raw(sx, sy, &rx, &ry, false); L.convertRawXY(rx, ry, &x, &y);
                const double d = hypot(x - sx, y - sy); if (d > wmax) wmax = d; wsum += d; ++wn;
                if (abs(x - sx) > axmax) axmax = abs(x - sx); if (abs(y - sy) > axmax) axmax = abs(y - sy);
            }
            if (seed == 0) printf("%s c0..c7 = %u %u %u %u %u %u %u %u\n", m.name, g_c8[0], g_c8[1], g_c8[2], g_c8[3], g_c8[4], g_c8[5], g_c8[6], g_c8[7]);
            ++runs;
        }
        printf("%s 50 runs: chip-centre max %.2f px | 99-pt grid max %.2f px (worst single axis %d px), mean %.2f px  %s\n", m0.name, cmax, wmax, axmax, wsum / wn, wmax <= 3.0 && cmax <= 3.0 ? "PASS (<=3 px)" : "FAIL");
        if (wmax > worst_all) worst_all = wmax; if (cmax > worst_all) worst_all = cmax;
    }
    printf("ALL %d cal runs: worst mapped error %.2f px -> %s\n", runs, worst_all, worst_all <= 3.0 ? "PASS" : "FAIL");
    return worst_all <= 3.0 ? 0 : 1;
}
