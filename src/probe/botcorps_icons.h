#pragma once

#include <stdint.h>

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* 12x12, 2 bits per pixel, 36 bytes, PROGMEM. */
enum {
    BOTCORPS_ICON_SKIMMER = 0,
    BOTCORPS_ICON_FLOCK,
    BOTCORPS_ICON_DRONE,
    BOTCORPS_ICON_AIRTAG,
    BOTCORPS_ICON_CAMERA,
    BOTCORPS_ICON_DEAUTH,
    BOTCORPS_ICON_WIFI,
    BOTCORPS_ICON_BT,
    BOTCORPS_ICON_TV,
    BOTCORPS_ICON_UNKNOWN,
    BOTCORPS_ICON_N
};

void botcorps_icon_blit(lgfx::LovyanGFX &dst, int x, int y, int id,
                        uint16_t c1, uint16_t c2, uint16_t c3);
