/* Host check for the shared CYD keyboard table. No device, no Arduino.
 * Build: g++ -std=gnu++17 -I src tools/host/cyd_osk_test.cpp -o /tmp/cyd_osk_test
 *
 * Typing path: OSK characters return K_CHAR (including space, comma, period)
 * and botcorps_ui_input feeds them to on_char(). on_char appends on
 * SCR_TERMINAL and returns on every other screen, so a letter cannot change
 * screens or clear the prompt. botcorps_host_type_letters() in botcorps_ui.cpp
 * walks that path when the UI is built with BOTCORPS_HOST_RENDER.
 *
 * Frames: botcorps_host_idle / botcorps_host_terminal already paint the
 * 320x240 surface. The CYD badge is inside paint_prompt on BOTCORPS_CYD,
 * so a host binary that defines that flag shows it. This file does not
 * link the UI.
 */
#include "ui/botcorps_cyd_osk.h"

#include <stdio.h>

int main(void)
{
    const int space = cyd_osk_hit(CYD_OSK_X0 + 2 * CYD_OSK_COL + 4,
                                  CYD_OSK_Y0 + 4 * CYD_OSK_ROW + 4);
    const CydOskKey sp = cyd_osk_at(space);
    if (sp.kind != CYD_OSK_SPACE || cyd_osk_char(sp, false) != ' ') {
        printf("space key mismatch\n");
        return 1;
    }
    const CydOskKey comma = cyd_osk_at(6);
    const CydOskKey period = cyd_osk_at(5);
    if (comma.kind != CYD_OSK_CHAR || comma.symbol != ',') return 2;
    if (period.kind != CYD_OSK_CHAR || period.symbol != '.') return 3;
    printf("cyd osk table ok keys=%d clip_y=%d\n", CYD_OSK_N, CYD_OSK_CLIP_Y);
    return 0;
}
