#pragma once
/* v1.2.10 (Dojo fix): the release-edge tap filter that lv_ui.cpp's lv_read_tap always ran, split into
 * sample (feed one getTouch reading) + take (consume a finished tap). Same rules as before: a tap fires
 * after >= 2 finger-up samples spanning >= 45 ms; the reported point is the 2nd-to-last finger-down sample
 * (or the only one if there were < 3). Splitting it lets the Dojo feed samples BETWEEN render bands, so a
 * tap that starts and ends inside one long Dojo frame is no longer lost. The host sim includes this same file. */
#include <stdint.h>
struct LvTapFilt { bool down_prev; int n, up, px[2], py[2]; uint32_t upt; bool pend; int ox, oy; };
static inline void lv_tf_sample(LvTapFilt &F, bool down, int tx, int ty, uint32_t now)
{
    if (down) {
        if (!F.down_prev) { F.down_prev = true; F.n = 0; }
        F.px[0] = F.px[1]; F.py[0] = F.py[1]; F.px[1] = tx; F.py[1] = ty; ++F.n; F.up = 0;
        return;
    }
    if (!F.down_prev) return;
    if (++F.up == 1) F.upt = now;
    if (F.up < 2 || (int32_t)(now - F.upt) < 45) return;
    F.down_prev = false; F.up = 0;
    const int use = F.n >= 3 ? 0 : 1;
    F.ox = F.px[use]; F.oy = F.py[use]; F.n = 0; F.pend = true;
}
static inline bool lv_tf_take(LvTapFilt &F, int *ox, int *oy)
{
    if (!F.pend) return false;
    F.pend = false; *ox = F.ox; *oy = F.oy; return true;
}
