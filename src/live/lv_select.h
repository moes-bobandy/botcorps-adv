/* lv_select.h — THE sub-screen selection module. LOCKED LAYOUT.
 *
 * Mobandy 2026-11-20 10:33 AM ET: "that needs to be the locked layout for subscreen selections".
 * Every sub-screen that selects a tower (Dojo / MORPHEUSbot Construct, GIBSON radar, HACKbot DOWNLOAD, and any future
 * sub-screen) uses this one module for the same two-step selection, never its own copy:
 *   1. PULL-UP : the camera eases onto the tapped / locked tower (cubic ease-out, lv_sel_ease) over the push time;
 *   2. DROP    : the folders fall in from the top of the scene one after another (start + j * gap, lv_sel_ease over the
 *                drop time). A folder is LANDED (drawn solid, tappable) at LV_SEL_LAND of its drop;
 *   3. TAP     : a landed folder is a button (lv_sel_hit over the screen's folder finger boxes, >= 40 px; the screen
 *                gives press feedback with lv_press); a falling folder is not tappable.
 * The animation runs on an LvSelClk: it follows lv_now, but while the pull-up / drop plays one frame may advance it by
 * at most the cap the screen passes (Dojo 50 / 100 ms), so a slow CYD frame never skips the pull-up (the v1.2.10 Dojo
 * "changes to a tower and folders" hard cut). At the locked 30 ms frame cadence the cap never binds, so every frame is
 * the locked frame. Screen-specific: geometry (folder x / y / pitch / size), the folder art, colours, push / drop
 * constants and the camera target; the timing law, landing rule, clock and hit test live only here. */
#pragma once
#include <stdint.h>

#define LV_SEL_LAND 0.65f   /* a folder is landed (solid, tappable) at 65 % of its drop */

static inline float lv_sel_ease(float u) { u = u < 0 ? 0 : u > 1 ? 1 : u; return 1.f - (1.f - u) * (1.f - u) * (1.f - u); }

struct LvSelClk { uint32_t vt, real; };   /* vt = the selection animation clock, real = lv_now at the last step */
static inline void lv_sel_clk_reset(LvSelClk &c, uint32_t now) { c.vt = c.real = now; }
static inline uint32_t lv_sel_clk_step(LvSelClk &c, uint32_t now, uint32_t cap) { uint32_t dt = now - c.real; c.real = now; if (dt > cap) dt = cap; c.vt += dt; return c.vt; }
static inline uint32_t lv_sel_clk_now(const LvSelClk &c, uint32_t now) { return now - (c.real - c.vt); }   /* the clock read between steps (taps) */
#define LV_SEL_NOCAP 0xFFFFFFFFu

/* folder j's drop: el = ms since the selection opened; returns true once it has started; y = its tab-top row, u = raw
 * drop progress (may exceed 1; > LV_SEL_LAND = landed) */
static inline bool lv_sel_drop(int el, int d0, int gap, int dur, int j, int start, int end, int &y, float &u)
{
    const int t = el - d0 - j * gap; u = t / (float)dur; y = start + (int)((end - start) * lv_sel_ease(u)); return t >= 0;
}
static inline bool lv_sel_landed(int el, int d0, int gap, int dur, int j) { return el - d0 - j * gap >= (int)(dur * LV_SEL_LAND); }
static inline float lv_sel_cam(float from, float to, float el, int dur) { return from + (to - from) * lv_sel_ease(el / (float)dur); }
template <typename R> static inline int lv_sel_hit(const R *r, int n, int x, int y)   /* folder finger box under (x,y), -1 none */
{
    for (int j = 0; j < n; ++j) if (x >= r[j].x0 && x <= r[j].x1 && y >= r[j].y0 && y <= r[j].y1) return j;
    return -1;
}
