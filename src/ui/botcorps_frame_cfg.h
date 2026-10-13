#pragma once

/* EXT frame buffer. BANDS is the default (320x120 RGB332, pushed twice).
 * FRAME is the full 320x240 RGB332 buffer. Change only this setting.
 * Do not pass it from platformio.ini.
 */
#define BANDS 0
#define FRAME 1

#ifndef BOTCORPS_FRAME_MODE
#define BOTCORPS_FRAME_MODE BANDS
#endif
