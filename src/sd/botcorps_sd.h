#pragma once

/*
 * Cardputer Adv microSD on SPI3, shared SCK/MOSI with the external
 * panel when BOTCORPS_DUAL_SCREEN=1.
 *
 *   SCK 40  MISO 39  MOSI 14  CS 12
 *
 * Every mount quiesces the panel first and resumes it after
 * (botcorps_lcd_quiesce / botcorps_lcd_resume_after_bus). GPIO5
 * stays high across the transfer so the SD slot stays enabled.
 * Absent card is not fatal. No format.
 */

bool botcorps_sd_mount(void);
bool botcorps_sd_mounted(void);
