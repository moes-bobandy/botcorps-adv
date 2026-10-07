#pragma once

/*
 * botcorps_ready — Ready Room on the menu surface.
 *
 * Seats that exist here are the ones the Adv contract names for this
 * cut: Wi-Fi, then Bluetooth, in that order, top-left first. Each seat
 * has its own color and a drawn icon. Nothing in the grid is a filler.
 *
 * The grid is keyboard-navigated on MENU (ST7789 240×135). On a dual
 * build the external panel shows that seat's name and color card,
 * repainted only when the selection changes.
 *
 * Enter opens a finished detail on MENU. Radio control is not started.
 * Backtick leaves the detail for the grid, and leaves the grid for
 * the hatch. The Adv matrix has no arrow keys; ; . , / are that cluster.
 */

#include <stdint.h>
#include <M5Cardputer.h>

enum botcorps_seat_id_t {
    BOTCORPS_SEAT_WIFI = 0,
    BOTCORPS_SEAT_BLUETOOTH = 1,
    BOTCORPS_SEAT_COUNT = 2,
};

/* Block on the Ready Room until the operator backs out to the hatch. */
void botcorps_ready_run(void);

const char *botcorps_seat_name(int seat);
uint16_t botcorps_seat_body(int seat);
uint16_t botcorps_seat_ink(int seat);
/* "2.4 GHz" for Wi-Fi, "BLE" for Bluetooth. */
const char *botcorps_seat_band(int seat);
void botcorps_seat_draw_icon(lgfx::LGFX_Device &d, int seat,
                             int cx, int cy, int radius);
